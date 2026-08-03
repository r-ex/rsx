#include <pch.h>
#include <game/vpk/vpk.h>
#include <imgui.h>

#define OFFSET (cursor - headerEnd)
void CVPKPackage::ProcessAssets()
{
    const char* headerEnd = reinterpret_cast<const char*>(m_fileBuf.get() + sizeof(VPKDirHeader_s));
    const char* cursor = headerEnd;

    std::string extension;
    std::string path;
    std::string name;

    // ifh this format
    while (*cursor != '\0')
    {
        extension = cursor;

        cursor += extension.length() + 1;

        while (*cursor != '\0')
        {
            path = cursor;

            cursor += path.length() + 1;

            while (*cursor != '\0')
            {
                name = cursor;

                cursor += name.length() + 1;

                const VPKEntryBlock_s* entryBlock = reinterpret_cast<const VPKEntryBlock_s*>(cursor);

                cursor += sizeof(*entryBlock);

                CVPKFile* file = new CVPKFile();

                const std::string fileName = (path.length() != 0 && path != " " ? (path + "/") : "") + name + "." + extension;
                file->SetAssetName(fileName, false);
                file->SetAssetVersion({ this->majorVer, this->minorVer });
                file->SetFileCRC(entryBlock->fileCRC);

                // file ext determines preview method
                if (auto it = s_vpkFileTypes.find("." + extension); it != s_vpkFileTypes.end())
                    file->SetFileType(it->second);
                else
                    file->SetFileType(VPKFileType_e::UNKNOWN);
                
                do {
                    const VPKDataChunk_s* chunk = reinterpret_cast<const VPKDataChunk_s*>(cursor);

                    file->AddDataChunk(*chunk);

                    cursor += sizeof(*chunk);
                } while (cursor += sizeof(uint16_t), *reinterpret_cast<const uint16_t*>(cursor-2) != VPK_CHUNK_TERM); // hate this

                assets.push_back(file);

                g_assetData.v_assets.push_back({ entryBlock->fileCRC, file });
            }
            cursor++;
        }
        cursor++;
    }
}

bool CVPKPackage::ParseFromFile(const std::string& filePath)
{
    if (!std::filesystem::path(filePath).is_absolute())
        SetFilePath(std::filesystem::absolute(filePath));
    else
        SetFilePath(filePath);

    const std::string fileName = GetFilePath().filename().string();

    if (!FileSystem::ReadFileData(GetFilePath().string(), &m_fileBuf))
    {
        // i hate std::filesystem::path
        Log("VPK: Failed to read file data for file %s\n", fileName.c_str());
        return false;
    }

    VPKDirHeader_s* header = reinterpret_cast<VPKDirHeader_s*>(m_fileBuf.get());

    if (header->magic != VPK_FILE_MAGIC) // 0x55AA1234
    {
        Log("VPK: Invalid file magic (expected %X, found %X): %s\n", VPK_FILE_MAGIC, header->magic, fileName.c_str());
        return false;
    }

    if (header->majorVer != VPK_MAJOR_VERS || header->minorVer != VPK_MINOR_VERS)
    {
        Log("VPK: Invalid file version (expected %u.%u, found %u.%u): %s\n",
            VPK_MAJOR_VERS, VPK_MINOR_VERS,
            header->majorVer, header->minorVer,
            fileName.c_str()
        );
        return false;
    }

    this->majorVer = header->majorVer;
    this->minorVer = header->minorVer;

    this->ProcessAssets();

	return true;
}

void LoadVPKFileAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container); UNUSED(asset);
}

void* PreviewVPKFileAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);
    CVPKFile* file = reinterpret_cast<CVPKFile*>(asset);
    if (ImGui::BeginTable("VPKDataChunkTable", 5, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInner))
    {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 0, 0);
        ImGui::TableSetupColumn("Load Flags", ImGuiTableColumnFlags_WidthFixed, 0, 1);
        ImGui::TableSetupColumn("Texture Flags", ImGuiTableColumnFlags_WidthFixed, 0, 2);
        ImGui::TableSetupColumn("Compressed Size", ImGuiTableColumnFlags_WidthFixed, 0, 3);
        ImGui::TableSetupColumn("Decompressed Size", ImGuiTableColumnFlags_WidthFixed, 0, 4);

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        size_t i = 0;
        for (auto& chunk : file->GetDataChunks())
        {
            i++;

            ImGui::PushID(static_cast<int>(i));

            ImGui::TableNextRow();

            if (ImGui::TableSetColumnIndex(0))
                ImGui::Text("%lld", i);

#define FLAG(v) (v == 0) ? (ImGui::TextUnformatted("0")) : ImGui::Text("0x%X", v)
            if (ImGui::TableSetColumnIndex(1))
                FLAG(chunk.loadFlags);

            if (ImGui::TableSetColumnIndex(2))
                FLAG(chunk.texFlags);
#undef FLAG

            if (ImGui::TableSetColumnIndex(3))
                ImGui::Text("%lld", chunk.cmpSize);

            if (ImGui::TableSetColumnIndex(4))
                ImGui::Text("%lld", chunk.dcmpSize);

            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    return nullptr;
}

void InitVPKFileAssetType()
{
    AssetTypeBinding_t type =
    {
        .name = "VPK File",
        .type = 'fkpv',
        .headerAlignment = 4,
        .loadFunc = LoadVPKFileAsset,
        .postLoadFunc = nullptr,
        .previewFunc = PreviewVPKFileAsset,
        .e = { nullptr, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}