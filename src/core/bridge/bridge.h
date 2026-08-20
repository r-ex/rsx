#pragma once

constexpr uint32_t BRIDGE_PACKET_MAGIC = 0x42585352;
constexpr uint16_t BRIDGE_PACKET_VERSION = 1;

enum class BridgePacketType_e : uint8_t
{
    /// Get information about this installation of RSX
    GET_VERSION = 0,

    /// Set the directory path that assets export to
    SET_EXPORT_DIR = 1,

    /// Export assets by their GUIDs
    EXPORT_BY_GUID = 2,

    /// Search for an asset by its GUID (and then preview it in the future, but for now it's just a search)
    PREVIEW_BY_GUID = 3,

    /// Replace the currently loaded files with the provided set of asset container files
    LOAD_FILES = 4,

    /// Get information about the currently loaded asset container files
    GET_LOADED_FILES = 5,

    /// Register the client as a subscriber for notifications (such as "Exported X assets!")
    /// If a notification fails to send to a subscribed client, they are automatically unsubscribed
    SUBSCRIBE = 6,

    /// Unregister the client as a subscriber for notifications
    UNSUBSCRIBE = 7,

    _TYPE_COUNT,


    //// R2C Exclusive Messages

    /// Used to communicate an error message from RSX related to a request (e.g., the client sends a request with an unsupported request type)
    R2C_ERROR = 0xFF,

    /// UI notification relay
    R2C_NOTIFICATION = 0xFE,

    /// Indicates a packet that is a response to a request (i.e., RSX -> CLIENT)
    /// RSX must never receive a message with this set
    _TYPE_RESPONSE = 1 << 7,
    _TYPE_MASK = (_TYPE_RESPONSE - 1),
};
static_assert(BridgePacketType_e::_TYPE_COUNT <= BridgePacketType_e::_TYPE_RESPONSE, "Too many packet types! Increase type width!!!");

#define BP_TYPE(v) { BridgePacketType_e::v, #v }
static std::unordered_map<BridgePacketType_e, const char*> s_bpTypeNames = {
    BP_TYPE(GET_VERSION),
    BP_TYPE(SET_EXPORT_DIR),
    BP_TYPE(EXPORT_BY_GUID),
    BP_TYPE(PREVIEW_BY_GUID),
    BP_TYPE(LOAD_FILES),
    BP_TYPE(GET_LOADED_FILES),
    BP_TYPE(SUBSCRIBE),
    BP_TYPE(UNSUBSCRIBE),

    BP_TYPE(R2C_ERROR),
    BP_TYPE(R2C_NOTIFICATION),
};
#undef BP_TYPE

#pragma pack(push, 1)
struct BridgePacketBase_s
{
    uint32_t magic;
    uint16_t version;
    uint8_t type; // MSB is a bool; true if r2c response (rsx -> client), false if c2s
    uint32_t seq;

    void operator=(const BridgePacketBase_s& rhs)
    {
        this->magic = rhs.magic;
        this->version = rhs.version;
        this->type = rhs.type;
        this->seq = rhs.seq;
    };

    BridgePacketBase_s(const BridgePacketBase_s* b)
    {
        this->magic = b->magic;
        this->version = b->version;
        this->type = b->type;
        this->seq = b->seq;
    }

    BridgePacketBase_s(const BridgePacketBase_s& b)
    {
        this->magic = b.magic;
        this->version = b.version;
        this->type = b.type;
        this->seq = b.seq;
    }

    BridgePacketBase_s(uint8_t _type, uint32_t _seq)
    {
        magic = BRIDGE_PACKET_MAGIC;
        version = BRIDGE_PACKET_VERSION;
        type = _type;
        seq = _seq;
    }

    BridgePacketBase_s(BridgePacketType_e _type, uint32_t _seq)
    {
        magic = BRIDGE_PACKET_MAGIC;
        version = BRIDGE_PACKET_VERSION;
        type = (uint8_t)_type;
        seq = _seq;
    }

    BridgePacketBase_s SetResponse() { type |= (uint8_t)BridgePacketType_e::_TYPE_RESPONSE; return *this; };

    bool IsResponse() const { return type & (uint8_t)BridgePacketType_e::_TYPE_RESPONSE; };
    BridgePacketType_e GetType() const { return (BridgePacketType_e)(type & (uint8_t)BridgePacketType_e::_TYPE_MASK); };
};
#pragma pack(pop)

class BridgePacketHelper
{
public:
    BridgePacketHelper() : finished(false), _data() {};

    const char* GetData() const { return _data.data(); };
    size_t GetSize() const { return _data.size(); };

    template<typename T>
    bool SetData(size_t offset, T value)
    {
        assert(!finished);

        if (finished)
            return false;

        if (GetSize() < offset + sizeof(T))
        {
            const size_t requiredSize = offset + sizeof(T);
            _data.resize(requiredSize);
        }

        *reinterpret_cast<T*>(_data.data() + offset) = value;

        return true;
    }

    template<typename T>
    bool Append(T value)
    {
        return SetData(GetSize(), value);
    }

    bool AppendString(const std::string_view& sv)
    {
        if (finished)
            return false;

        const size_t startIdx = _data.size();

        _data.resize(GetSize() + sv.length() + 1);

        size_t i = startIdx;
        for (auto c : sv)
        {
            _data.at(i) = c;

            i++;
        }

        return true;
    }

    const char* Finish() { finished = true; return GetData(); };

private:
    std::vector<char> _data;

    bool finished;
};

struct BridgeGlobalData_s
{
    std::vector<sockaddr> subscribedClients;

    SOCKET serverSocket;

    SOCKET SetSVSocket(SOCKET sockfd) { serverSocket = sockfd; return sockfd; };
    SOCKET GetSVSocket() const { return serverSocket; };


    void Subscribe(const sockaddr& addr)
    {
        for (auto& it : subscribedClients)
        {
            if (memcmp(&addr, &it, sizeof(sockaddr)) == 0)
                return;
        }

        subscribedClients.emplace_back(addr);
    }

    void Unsubscribe(const sockaddr& addr)
    {
        size_t i = 0;
        for (auto& it : subscribedClients)
        {
            if (memcmp(&addr, &it, sizeof(sockaddr)) == 0)
            {
                subscribedClients.erase(subscribedClients.begin() + i);
                return;
            }

            i++;
        }
    }

    void PostNotification(const std::string_view& srcName, const std::string_view& msg)
    {
#if (HAS_BRIDGE)
        BridgePacketHelper helper;

        helper.Append(BridgePacketBase_s(BridgePacketType_e::R2C_NOTIFICATION, UINT32_MAX).SetResponse());
        helper.AppendString(srcName);
        helper.AppendString(msg);

        auto it = subscribedClients.begin();
        while(it != subscribedClients.end())
        {
            if (sendto(GetSVSocket(), helper.Finish(), static_cast<int>(helper.GetSize()), 0, &*it, sizeof(sockaddr)) == -1)
                it = subscribedClients.erase(it);
            else it++;
        }
#else
        UNUSED(srcName);
        UNUSED(msg);
#endif
    }
};

void Bridge_SetupSocketThread();

inline BridgeGlobalData_s g_bridgeData;