#pragma once

// season 16 (or maybe 15.1?)
struct RTKAssetHeader_v2_t
{
    char* elementName; // short version of file path, e.g. stats_menu\dropdown_button
    char* fullScriptPath; // full file path relative to depot root, e.g. content\r2\rtk\stats_menu\dropdown_button.rtk
    int elementDataSize;
    int unk_1C;
    char* elementData; // pointer to text data for rtk element script
};
static_assert(sizeof(RTKAssetHeader_v2_t) == 0x20);

// season 16.1
struct RTKAssetHeader_v2_1_t
{
    char* elementName; // short version of file path, e.g. stats_menu\dropdown_button
    char* fullScriptPath; // full file path relative to depot root, e.g. content\r2\rtk\stats_menu\dropdown_button.rtk
    void* guids; // unk_10 - contains an array of asset guids
    int elementDataSize;
    int unk_1C;
    char* elementData; // pointer to text data for rtk element script
};
static_assert(sizeof(RTKAssetHeader_v2_1_t) == 0x28);

struct RTKAssetHeader_v2_2_t
{
    char* elementName; // short version of file path, e.g. stats_menu\dropdown_button
    char* fullScriptPath; // full file path relative to depot root, e.g. content\r2\rtk\stats_menu\dropdown_button.rtk
    void* guids; // unk_10 - contains an array of asset guids
    int elementDataSize;
    int unk_1C;
    int64_t unk_20;
    char* elementData; // pointer to text data for rtk element script
};
static_assert(sizeof(RTKAssetHeader_v2_2_t) == 0x30);

struct RTKAsset
{
    RTKAsset(const RTKAssetHeader_v2_t* rtk) : elementName(rtk->elementName), fullScriptPath(rtk->fullScriptPath), guids(nullptr), elementDataSize(static_cast<int64_t>(rtk->elementDataSize)), elementData(rtk->elementData) {};
    RTKAsset(const RTKAssetHeader_v2_1_t* rtk) : elementName(rtk->elementName), fullScriptPath(rtk->fullScriptPath), guids(rtk->guids), elementDataSize(static_cast<int64_t>(rtk->elementDataSize)), elementData(rtk->elementData) {};
    RTKAsset(const RTKAssetHeader_v2_2_t* rtk) : elementName(rtk->elementName), fullScriptPath(rtk->fullScriptPath), guids(rtk->guids), elementDataSize(static_cast<int64_t>(rtk->elementDataSize)), elementData(rtk->elementData) {};

    char* elementName; // short version of file path, e.g. stats_menu\dropdown_button
    char* fullScriptPath; // full file path relative to depot root, e.g. content\r2\rtk\stats_menu\dropdown_button.rtk
    void* guids; // unk_10 - contains an array of asset guids
    int64_t elementDataSize;

    char* elementData; // pointer to text data for rtk element script

    std::string getName() { return elementName; };
};