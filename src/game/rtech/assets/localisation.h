#pragma once

#pragma pack(push, 1)
struct LocalisationEntry_t
{
    uint64_t hash;
    int stringStartIndex; // wchar index at which the value for this entry starts
    int unkC;

};
#pragma pack(pop)

struct LocalisationHeader_t
{
    char* fileName;
    int numStrings;
    int f;
    size_t numEntries;
    LocalisationEntry_t* entries;
    wchar_t* strings;
    char unk28[16];
};

static_assert(sizeof(LocalisationHeader_t) == 56);

struct LocalisationAsset
{
    LocalisationAsset(LocalisationHeader_t* hdr) : fileName(hdr->fileName), numStrings(hdr->numStrings), numEntries(hdr->numEntries), entries(hdr->entries), strings(hdr->strings) {};

    char* fileName;
    int numStrings;
    size_t numEntries;
    LocalisationEntry_t* entries;
    wchar_t* strings;

    std::string getName() { return fileName; };
};