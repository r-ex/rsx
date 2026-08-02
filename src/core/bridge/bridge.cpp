#include <pch.h>
#include "core/bridge/bridge.h"
#include <core/version.h>
#include <game/asset.h>
#include <core/filehandling/load.h>
#include <misc/imgui_utility.h>
#include <core/filehandling/export.h>

static void Bridge_ProcessPacket(SOCKET serverSock, const sockaddr& to, const char* buffer, int len)
{
    if (len < sizeof(BridgePacketBase_s))
    {
        Log("BRIDGE: Packet too small\n");
        return;
    }

    const BridgePacketBase_s* base = reinterpret_cast<const BridgePacketBase_s*>(buffer);

    if (base->magic != BRIDGE_PACKET_MAGIC)
    {
        Log("BRIDGE: Packet has invalid magic\n");
        return;
    }

    if (base->IsResponse())
    {
        Log("BRIDGE: Server received a response packet even though only the client should be receiving them\n");
        return;
    }

    size_t cursor = sizeof(BridgePacketBase_s);

    const size_t extLen = len - sizeof(BridgePacketBase_s);

    BridgePacketHelper helper;
    bool sendResponse = false;

    Log("BRIDGE: Received %s request - Seq 0x%X %s\n",
        s_bpTypeNames.contains(base->GetType()) ? s_bpTypeNames.at(base->GetType()) : "(unknown)",
        base->seq,
        base->IsResponse() ? "[!RESP!]" : "");

    switch (base->GetType())
    {
    case BridgePacketType_e::GET_VERSION:
    {
        sendResponse = true;

        helper.Append(BridgePacketBase_s(base).SetResponse());
        helper.AppendString(VERSION_STRING);
        helper.AppendString(FEATURE_STRING);

        break;
    }
    case BridgePacketType_e::SET_EXPORT_DIR:
    {
        if (extLen == 0)
        {
            Log("BRIDGE: SET_EXPORT_DIR - no path provided\n");
            return;
        }

        const char* path = reinterpret_cast<const char*>(buffer + cursor);

        g_rsxSettings.SetExportDirectory(path);

        break;
    }
    case BridgePacketType_e::EXPORT_BY_GUID:
    {
        if (extLen == 0)
        {
            Log("BRIDGE: EXPORT_BY_GUID - no guids provided\n");
            return;
        }

        if ((extLen % sizeof(uint64_t)) != 0)
        {
            Log("BRIDGE: EXPORT_BY_GUID - invalid data size\n");
            return;
        }

        std::vector<uint64_t> assetGuids;

        while (cursor < len)
        {
            assetGuids.emplace_back(*reinterpret_cast<const uint64_t*>(buffer + cursor));

            cursor += sizeof(uint64_t);
        }

        std::deque<CAsset*> exportAssets;

        for (auto& guid : assetGuids)
        {
            if (auto asset = g_assetData.FindAssetByGUID(guid); asset != nullptr)
                exportAssets.emplace_back(asset);
        }

        CThread(HandlePakAssetExportList, std::move(exportAssets), false).detach();

        break;
    }
    case BridgePacketType_e::PREVIEW_BY_GUID:
    {
        if (extLen < sizeof(uint64_t))
        {
            Log("BRIDGE: PREVIEW_BY_GUID - not enough data\n");
            return;
        }

        const uint64_t guid = *reinterpret_cast<const uint64_t*>(buffer + cursor);

        // ui mutex locks ui frames until the filter has been updated and rebuilt
        std::lock_guard lock(g_assetData.m_uiMutex);

        // Set asset list filter to just this guid
        FilterConfig->textFilter.SetText(std::format("{:X}", guid));
        FilterConfig->textFilter.Build();

        break;
    }
    case BridgePacketType_e::LOAD_FILES:
    {
        std::vector<std::string> filePaths;

        if (extLen == 0)
        {
            Log("BRIDGE: LOAD_FILES - no files to load\n");
            return;
        }

        while (cursor < len)
        {
            const char* pathCandidate = reinterpret_cast<const char*>(buffer + cursor);
            filePaths.emplace_back(pathCandidate);

            // keep the cursor going until after the null terminator of each string
            while (buffer[cursor] != '\0')
                cursor++;

            cursor++;
        }

        if (filePaths.empty())
        {
            Log("BRIDGE: LOAD_FILES - no files to load (found data but no paths)\n");
            return;
        }

        extern std::atomic<bool> inJobAction;

        // can't unload while loading
        if (!inJobAction)
        {
            extern void ClearLoadState();

            // Must acquire the asset ui mutex to clear the loaded data
            std::lock_guard lock(g_assetData.m_uiMutex);

            ClearLoadState();
            CThread(Bridge_HandleLoad, std::move(filePaths)).detach();
        }
        else
            Log("BRIDGE: LOAD_FILES - ignoring request while we are already loading files\n");

        break;
    }

    case BridgePacketType_e::GET_LOADED_FILES:
    {
        sendResponse = true;

        const bool useFullPaths = *reinterpret_cast<const bool*>(buffer + cursor);

        helper.Append(BridgePacketBase_s(base).SetResponse());

        for (auto& container : g_assetData.v_assetContainers)
        {
            const std::string path = useFullPaths ? container->GetFilePath().string() : container->GetFilePath().filename().string();

            helper.AppendString(path);
        }

        break;
    }
    case BridgePacketType_e::SUBSCRIBE:
    {
        g_bridgeData.Subscribe(to);

        break;
    }
    case BridgePacketType_e::UNSUBSCRIBE:
    {
        g_bridgeData.Unsubscribe(to);

        break;
    }
    default:
    {
        sendResponse = true;

        helper.Append(BridgePacketBase_s(BridgePacketType_e::R2C_ERROR, base->seq));
        helper.AppendString(std::format("Unsupported request type: {}", base->type));
    }
    }

    // Only use the packet helper if the request wants some sort of response
    if (sendResponse && sendto(serverSock, helper.Finish(), static_cast<int>(helper.GetSize()), 0, &to, sizeof(sockaddr)) == -1)
    {
        Log("BRIDGE: Failed to send %s response. Error: %u\n", s_bpTypeNames.contains(base->GetType()) ? s_bpTypeNames.at(base->GetType()) : "unsupported type", WSAGetLastError());
        return;
    }
}

void Bridge_SetupSocketThread()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return;

    const SOCKET serverSocket = g_bridgeData.SetSVSocket(socket(AF_INET, SOCK_DGRAM, 0));

    if (serverSocket == -1)
    {
        perror("socket");
        return;
    }

    const short port = g_rsxSettings.bridgePort;

    sockaddr_in serverAddr
    {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = INADDR_ANY
    };

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
    {
        perror("bind");
        return;
    }

    Log("BRIDGE: Listening on port %u\n", port);

    sockaddr clientAddr = {};
    int clientAddrLen = sizeof(clientAddr);

    char buffer[1024] = { 0 };

    while (int res = recvfrom(serverSocket, buffer, sizeof(buffer), 0, &clientAddr, &clientAddrLen))
    {
        if (res != -1)
            Bridge_ProcessPacket(serverSocket, clientAddr, buffer, res);
    }
}