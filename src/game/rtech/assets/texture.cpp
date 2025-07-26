#include <pch.h>
#include <game/rtech/assets/texture.h>
#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

inline std::string FormatTextureAssetName(const char* const str)
{
    std::string name(str);
    if (!name.starts_with("texture/"))
        name = "texture/" + name;

    if (!name.ends_with(".rpak"))
        name += ".rpak";

    return name;
}

#undef max
void LoadTextureAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    TextureAsset* txtrAsset = nullptr;

    switch (pakAsset->version())
    {
    case 8:
    {
        TextureAssetHeader_v8_t* v8 = reinterpret_cast<TextureAssetHeader_v8_t*>(pakAsset->header());
        txtrAsset = new TextureAsset(v8);
        break;
    }
    case 9:
    {
        TextureAssetHeader_v9_t* v9 = reinterpret_cast<TextureAssetHeader_v9_t*>(pakAsset->header());
        txtrAsset = new TextureAsset(v9, pakAsset->data()->guid);
        break;
    }
    case 10:
    {
        TextureAssetHeader_v10_t* v10 = reinterpret_cast<TextureAssetHeader_v10_t*>(pakAsset->header());
        txtrAsset = new TextureAsset(v10, pakAsset->data()->guid);
        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
    }

    if (txtrAsset->name)
    {
        std::string name = FormatTextureAssetName(txtrAsset->name);

        pakAsset->SetAssetName(name, true);
    }

    // [rika]: verify we know this texture format
    assertm(txtrAsset->imgFormat < eTextureFormat::TEX_FMT_UNKNOWN, "unaccounted texture format!");

#ifdef _DEBUG
    if (txtrAsset->type != _UNUSED && s_TextureTypeMap.count(txtrAsset->type) == 0)
        Log("found texture '%s' with unknown texture type: %i\n", asset->GetAssetName().c_str(), txtrAsset->type);
#endif // _DEBUG

    txtrAsset->totalMipLevels = (txtrAsset->optStreamedMipLevels + txtrAsset->streamedMipLevels + txtrAsset->permanentMipLevels);
    txtrAsset->arraySize = txtrAsset->arraySize == 0 ? 1 : txtrAsset->arraySize;

    const AssetPtr_t optStreamEntry = pakAsset->getStarPakStreamEntry(true);
    const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false);

    struct
    {
        size_t offsetRPak = 0;
        size_t offsetStarPak = 0;
        size_t offsetOptStarPak = 0;
    } mipOffsets;

    assertm(txtrAsset->unkMipLevels == 0, "unhandled mip type");

    // start high, this will let us go from bottom to top
    for (int i = txtrAsset->totalMipLevels - 1, compIdx = txtrAsset->streamedMipLevels + txtrAsset->optStreamedMipLevels - 1; i >= 0; --i)
    {
        TextureMip_t mip {};

        if (i < txtrAsset->optStreamedMipLevels)
            mip.type = eTextureMipType::OptStarPak;
        else if (i < (txtrAsset->optStreamedMipLevels + txtrAsset->streamedMipLevels))
            mip.type = eTextureMipType::StarPak;
        else
            mip.type = eTextureMipType::RPak;

        const uint16_t fmtIdx = txtrAsset->imgFormat * 3;

        const uint8_t x = s_pBytesPerPixel[fmtIdx];
        const uint8_t yw = s_pBytesPerPixel[fmtIdx + 1];
        const uint8_t yh = s_pBytesPerPixel[fmtIdx + 2];

        const uint16_t mipWidth = static_cast<uint16_t>(std::max(1, (txtrAsset->width >> i)));
        const uint16_t mipHeight = static_cast<uint16_t>(std::max(1, (txtrAsset->height >> i)));

        // [rika]: retail divides by y, old shifts right, both yield the same result
        const uint16_t bppWidth = (yw + (mipWidth - 1)) / yw;
        const uint16_t bppHeight = (yh + (mipHeight - 1)) / yh;
        const uint32_t sliceWidth = x * (yw >> (yw >> 1));

        const uint32_t pitch = sliceWidth * bppWidth;
        const uint32_t slicePitch = x * bppWidth * bppHeight;

        // [rika]: nothing wrong with this code, the game just handles it different now, and the old code would not support some texture formats (astc, maybe others since the array is MASSIVE now, 304 formats as of r5_250)
        /*const uint8_t x = s_pBytesPerPixel[txtrAsset->imgFormat].first;
        const uint8_t y = s_pBytesPerPixel[txtrAsset->imgFormat].second;
        const uint8_t z = (y >> 1); // half of y, in retail this is stored in the exe within the s_pBytesPerPixel array

        const uint16_t mipWidth = static_cast<uint16_t>(std::max(1, (txtrAsset->width >> i)));
        const uint16_t mipHeight = static_cast<uint16_t>(std::max(1, (txtrAsset->height >> i)));

        const uint32_t bppWidth = (y + (mipWidth - 1)) >> z;
        const uint32_t bppHeight = (y + (mipHeight - 1)) >> z;
        const uint32_t sliceWidth = x * (y >> z);

        const uint32_t pitch = sliceWidth * bppWidth;
        const uint32_t slicePitch = x * bppWidth * bppHeight;*/

        // for swizzling the stored image is aligned.
        uint32_t slicePitchData = slicePitch;

        switch (txtrAsset->swizzle)
        {
        // ps4 swizzles are stored in blocks, and the block width/height is aligned to 8
        case eTextureSwizzle::SWIZZLE_PS4:
        {
            const int pixbl = static_cast<int>(CTexture::GetPixelBlock(s_PakToDxgiFormat[txtrAsset->imgFormat]));

            const int blocksX = mipWidth / pixbl;
            const int blocksY = mipHeight / pixbl;

            const uint16_t mipWidthData = static_cast<uint16_t>(IALIGN(blocksX, s_SwizzleChunkSizePS4) * pixbl);
            const uint16_t mipHeightData = static_cast<uint16_t>(IALIGN(blocksY, s_SwizzleChunkSizePS4) * pixbl);

            const uint32_t bppWidthData = (yw + (mipWidthData - 1)) / yw;
            const uint32_t bppHeightData = (yh + (mipHeightData - 1)) / yh;

            slicePitchData = x * bppWidthData * bppHeightData;

            break;
        }
#ifdef SWITCH_SWIZZLE
        // switch is weird, blocks within chunks within sections, we need to align for sectors.
        case eTextureSwizzle::SWIZZLE_SWITCH:
        {
            const uint8_t bpp = static_cast<uint8_t>(CTexture::GetBpp(s_PakToDxgiFormat[txtrAsset->imgFormat]));
            int vp = (bpp * 2);

            const int pixbl = static_cast<int>(CTexture::GetPixelBlock(s_PakToDxgiFormat[txtrAsset->imgFormat]));
            if (pixbl == 1)
                vp = bpp / 8;

            const int blocksX = mipWidth / pixbl;
            const int blocksY = mipHeight / pixbl;

            int blocksPerChunkY = blocksY / 8;

            if (blocksPerChunkY > 16)
                blocksPerChunkY = 16;

            int blocksPerChunkX = 1;
            switch (vp)
            {
            case 16:
                blocksPerChunkX = 1;
                break;
            case 8:
                blocksPerChunkX = 2;
                break;
            case 4:
                blocksPerChunkX = 4;
                break;
            default:
                break;
            }

            const int blocksXAligned = IALIGN(blocksX / s_SwizzleChunkSizeSwitchX, blocksPerChunkX) * s_SwizzleChunkSizeSwitchX;
            const int blocksYAligned = IALIGN(blocksY / s_SwizzleChunkSizeSwitchY, blocksPerChunkY) * s_SwizzleChunkSizeSwitchY;

            const uint16_t mipWidthData = static_cast<uint16_t>(blocksXAligned * pixbl);
            const uint16_t mipHeightData = static_cast<uint16_t>(blocksYAligned * pixbl);

            const uint32_t bppWidthData = (yw + (mipWidthData - 1)) / yw;
            const uint32_t bppHeightData = (yh + (mipHeightData - 1)) / yh;

            slicePitchData = x * bppWidthData * bppHeightData;

            break;
        }
#endif
        default:
            break;
        }

        // [rika]: if our value is 0, the alignment macro will align to 0, so set the min size if 0
        size_t sizeSingle = slicePitchData ? IALIGN(slicePitchData, s_MipAligment[txtrAsset->swizzle]) : s_MipAligment[txtrAsset->swizzle];
        size_t sizeAbsolute = sizeSingle * txtrAsset->arraySize;

        mip.isLoaded = true;
        switch (mip.type)
        {
            case eTextureMipType::RPak:
            {
                mip.compType = eCompressionType::NONE; // default

                if (!pakAsset->cpu())
                {
                    mip.isLoaded = false;
                    break;
                }

                mip.assetPtr.ptr = pakAsset->cpu() + mipOffsets.offsetRPak;
                mipOffsets.offsetRPak += sizeAbsolute;
                break;
            }
            case eTextureMipType::StarPak:
            {
                assertm(txtrAsset->arraySize == 1, "texture arrays should not be streamed"); // if they are, the packed compressed size should cover this

                mip.compType = txtrAsset->StreamMipCompression(static_cast<uint8_t>(compIdx));

                // Need to re-grab the size on compressed textures.
                if (mip.compType != eCompressionType::NONE)
                {
                    sizeSingle = static_cast<uint64_t>((static_cast<uint32_t>(txtrAsset->compressedBytes[compIdx]) + 1) << 12u); // (1 << 12) = 4096, seems to be aligned to starpak pages.
                    sizeAbsolute = sizeSingle;
                }

                compIdx--;

                // version 6 has some weird starpak stuff going on
                if (IS_ASSET_PTR_INVALID(streamEntry))
                    mip.isLoaded = false;

                mip.assetPtr.offset = streamEntry.offset + mipOffsets.offsetStarPak;
                mipOffsets.offsetStarPak += sizeAbsolute;
                break;
            }
            case eTextureMipType::OptStarPak:
            {
                assertm(txtrAsset->arraySize == 1, "texture arrays should not be streamed");

                mip.compType = txtrAsset->StreamMipCompression(static_cast<uint8_t>(compIdx));

                // Need to re-grab the size on compressed textures.
                if (mip.compType != eCompressionType::NONE)
                {
                    sizeSingle = static_cast<uint64_t>((static_cast<uint32_t>(txtrAsset->compressedBytes[compIdx]) + 1) << 12u); // (1 << 12) = 4096, seems to be aligned to starpak pages.
                    sizeAbsolute = sizeSingle;
                }

                compIdx--;

                if (IS_ASSET_PTR_INVALID(optStreamEntry))
                    mip.isLoaded = false;

                mip.assetPtr.offset = optStreamEntry.offset + mipOffsets.offsetOptStarPak;
                mipOffsets.offsetOptStarPak += sizeAbsolute;
                break;
            }
            default:
            {
                assertm(false, "Unknown mip type.");
                break;
            }
        }

        mip.level = txtrAsset->totalMipLevels - static_cast<uint8_t>(i);
        mip.width = mipWidth;
        mip.height = mipHeight;
        mip.pitch = pitch;
        mip.slicePitch = slicePitch;
        mip.sizeSingle = sizeSingle;
        mip.sizeAbsolute = sizeAbsolute;
        mip.swizzle = txtrAsset->swizzle;

        txtrAsset->dataSizeUnaligned += slicePitch; // used for making a block of mips (without pak alignment)
        txtrAsset->mipArray.push_back(mip);
    }

    txtrAsset->mipSorting = txtrAsset->swizzle != 0 ? eTextureMipSorting::SORT_TOP_BOTTOM : txtrAsset->mipSorting;

    // resort mip offset
    switch (txtrAsset->mipSorting)
    {
    case eTextureMipSorting::SORT_TOP_BOTTOM:
    case eTextureMipSorting::SORT_MIXED:
    {
        for (auto& mip : txtrAsset->mipArray)
        {
            // no shuffling, skip
            if (mip.type == eTextureMipType::RPak && txtrAsset->mipSorting != eTextureMipSorting::SORT_TOP_BOTTOM)
                continue;

            switch (mip.type)
            {
            // should only get hit on ps4 + switch paks
            case eTextureMipType::RPak:
            {
                mipOffsets.offsetRPak -= mip.sizeAbsolute;
                mip.assetPtr.ptr = pakAsset->cpu() + mipOffsets.offsetRPak;
                break;
            }
            case eTextureMipType::StarPak:
            {
                mipOffsets.offsetStarPak -= mip.sizeAbsolute;
                mip.assetPtr.offset = streamEntry.offset + mipOffsets.offsetStarPak;
                break;
            }
            case eTextureMipType::OptStarPak:
            {
                mipOffsets.offsetOptStarPak -= mip.sizeAbsolute;
                mip.assetPtr.offset = optStreamEntry.offset + mipOffsets.offsetOptStarPak;
                break;
            }
            default:
            {
                assertm(false, "Unknown mip type.");
                break;
            }
            }
        }
    }
    case eTextureMipSorting::SORT_BOTTOM_TOP:
    default:
        break;
    }

    std::sort(txtrAsset->mipArray.begin(), txtrAsset->mipArray.end(), [](const TextureMip_t& a, const TextureMip_t& b) { return a.level < b.level; });

    pakAsset->setExtraData(txtrAsset);
}

void PostLoadTextureAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const TextureAsset* const txtrAsset = reinterpret_cast<TextureAsset*>(pakAsset->extraData());

    if (!txtrAsset->name)
        pakAsset->SetAssetNameFromCache();
}

extern CDXParentHandler* g_dxHandler;
std::shared_ptr<CTexture> CreateTextureFromMip(CPakAsset* const asset, const TextureMip_t* const mip, const DXGI_FORMAT format, const size_t arrayIdx)
{
    if (format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
        return nullptr;

    // Texture isn't multiple of 4, most textures are BC which requires the width n height to be multiple of 4 causing a crash.
    if (mip->width < 3 || mip->height < 3)
        return nullptr;

    if (!mip->isLoaded)
        return nullptr;

    std::unique_ptr<char[]> txtrData = GetTextureDataForMip(asset, mip, format, arrayIdx);

    return std::move(g_dxHandler->CreateRenderTexture(txtrData.get(), mip->slicePitch, mip->width, mip->height, format, 1u, 1u));
};

struct TexturePreviewData_t
{
    enum eColumnID
    {
        TPC_Level,
        TPC_Dimensions,
        TPC_Status,
        TPC_Type,
        TPC_Comp,
        TPC_Origin,

        _TPC_COUNT,
    };

    const char* typeName; // perm, stream, opt stream
    const char* compName;
    const char* dataOrigin; // the file this data comes from

    uint16_t width;
    uint16_t height;
    uint8_t level;
    uint8_t index; // level minus one, for accessing arrays
    bool isLoaded;
    eTextureMipType type;
    eCompressionType comp;

    const bool operator==(const TexturePreviewData_t& in)
    {
        return height == in.height && width == in.width && level == in.level && isLoaded == in.isLoaded && type == in.type;
    }

    const bool operator==(const TexturePreviewData_t* in)
    {
        return height == in->height && width == in->width && level == in->level && isLoaded == in->isLoaded && type == in->type;
    }
};

struct TexCompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    bool operator() (const TexturePreviewData_t& a, const TexturePreviewData_t& b) const
    {

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case TexturePreviewData_t::eColumnID::TPC_Level:        delta = (static_cast<short>(a.level) - b.level);                        break; // no overflow please
            case TexturePreviewData_t::eColumnID::TPC_Dimensions:   delta = (static_cast<short>(a.level) - b.level);                        break; // no overflow please
            case TexturePreviewData_t::eColumnID::TPC_Type:         delta = (static_cast<short>(a.type) - static_cast<short>(b.level));     break;
            case TexturePreviewData_t::eColumnID::TPC_Comp:         delta = (static_cast<short>(a.comp) - static_cast<short>(b.comp));     break;
            default: IM_ASSERT(0); break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
        }

        return (static_cast<int8_t>(a.level) - static_cast<int8_t>(b.level)) > 0;
    }
};

void* PreviewTextureAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    TextureAsset* const txtrAsset = reinterpret_cast<TextureAsset*>(pakAsset->extraData());
    assertm(txtrAsset, "Extra data should be valid at this point.");

    static float textureZoom = 1.0f;
    static uint8_t lastSelectedMip = 0xff;
    static TexturePreviewData_t selectedMip { .index = 0xff };
    static std::shared_ptr<CTexture> selectedMipTexture = nullptr;
    static std::vector<TexturePreviewData_t> previewMips;
    static int selectedArrayIndex = 0;
    static int lastSelectedArrayIndex = 0;
    static std::vector<std::string> previewArrays;

    static constexpr const char* const mipId[] =
    {
        "PERM",
        "STREAMED",
        "OPT STREAMED"
    };

    static constexpr const char* const mipComp[] =
    {
        "NONE",
        "PAK",
        "SNOWFLAKE",
        "OODLE", // (n)oodle
    };

    static std::string currContainerStem[static_cast<uint8_t>(eTextureMipType::_COUNT)];

    static uint8_t previewMipSize = 0;
    if (firstFrameForAsset) // Reset if new asset.
    {
        // no reason to update if the array size hasn't changed
        if (txtrAsset->arraySize != previewArrays.size())
        {
            previewArrays.clear();
            previewArrays.resize(txtrAsset->arraySize);

            for (uint8_t idx = 0; idx < txtrAsset->arraySize; idx++)
            {
                previewArrays.at(idx) = std::to_string(idx);
            }
        }

        previewMips.clear();
        previewMips.resize(txtrAsset->totalMipLevels);
        previewMipSize = static_cast<uint8_t>(previewMips.size());

        selectedMip.index = 0xff;
        selectedMipTexture.reset();

        for (size_t i = 0; i < ARRAYSIZE(currContainerStem); i++)
            currContainerStem[i].clear();

        uint8_t mipIdx = 0;
        for (auto& mip : txtrAsset->mipArray)
        {
            TexturePreviewData_t& previewData = previewMips.at(mipIdx);

            previewData.width = mip.width;
            previewData.height = mip.height;
            previewData.level = mip.level;
            previewData.index = (mip.level - 1);
            previewData.isLoaded = mip.isLoaded;
            previewData.type = mip.type;
            previewData.typeName = mipId[static_cast<uint8_t>(mip.type)];
            previewData.comp = mip.compType;
            previewData.compName = mipComp[mip.compType];

            std::string& targetStem = currContainerStem[static_cast<uint8_t>(mip.type)];

            if (!targetStem.empty())
                previewData.dataOrigin = targetStem.c_str();
            else
            {
                if (mip.type == eTextureMipType::RPak)
                    targetStem = asset->GetContainerFileName();
                else
                    targetStem = pakAsset->getStarPakName(mip.type == eTextureMipType::OptStarPak);

                previewData.dataOrigin = targetStem.c_str();
            }

            mipIdx++;
        }

        selectedMip = selectedMip.index > previewMipSize ? previewMips.back() : selectedMip;
        selectedArrayIndex = selectedArrayIndex > txtrAsset->arraySize ? 0 : selectedArrayIndex;
    }

    assertm(selectedMip.index < previewMipSize, "Selected mip is out of bounds for mip array.");

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    ImGui::TextUnformatted(std::format("Texture: {} (0x{:X})", nullptr != txtrAsset->name ? txtrAsset->name : "null name", txtrAsset->guid).c_str());

    // temp, we should do better at somepoint
    if (txtrAsset->arraySize > 1)
    {
        ImGui::TextUnformatted("Texture Array Index:");
        ImGui::SameLine();

        static const char* arrayLabel = previewArrays.at(0).c_str();
        if (ImGui::BeginCombo("##ArrayIndex", arrayLabel, ImGuiComboFlags_NoPreview))
        {
            for (int i = 0; i < txtrAsset->arraySize; i++)
            {
                const bool isSelected = selectedArrayIndex == i || (firstFrameForAsset && selectedArrayIndex == lastSelectedArrayIndex);

                if (ImGui::Selectable(previewArrays.at(i).c_str(), isSelected))
                {
                    selectedArrayIndex = i;
                    arrayLabel = previewArrays.at(i).c_str();
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
    }

    if (ImGui::CollapsingHeader("Mip Levels", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::BeginTable("Texture Table", TexturePreviewData_t::eColumnID::_TPC_COUNT, tableFlags, outerSize))
        {
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, TexturePreviewData_t::eColumnID::TPC_Level);
            ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TexturePreviewData_t::eColumnID::TPC_Dimensions);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TexturePreviewData_t::eColumnID::TPC_Status);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TexturePreviewData_t::eColumnID::TPC_Type);
            ImGui::TableSetupColumn("Compression", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TexturePreviewData_t::eColumnID::TPC_Comp);
            ImGui::TableSetupColumn("Origin", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, TexturePreviewData_t::eColumnID::TPC_Origin);
            ImGui::TableSetupScrollFreeze(1, 1);

            ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); // get the sorting settings from this table

            if (sortSpecs && (firstFrameForAsset || sortSpecs->SpecsDirty) && previewMips.size() > 1)
            {
                std::sort(previewMips.begin(), previewMips.end(), TexCompare_t(sortSpecs));
                sortSpecs->SpecsDirty = false;
            }

            ImGui::TableHeadersRow();

            for (size_t i = 0; i < previewMips.size(); i++)
            {
                const TexturePreviewData_t* item = &previewMips.at(i);

                const bool isRowSelected = selectedMip == item || (firstFrameForAsset && item->level == lastSelectedMip);

                ImGui::PushID(item->level); // id of this line ?

                ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                if (ImGui::TableSetColumnIndex(TexturePreviewData_t::eColumnID::TPC_Level))
                {
                    if (ImGui::Selectable(std::to_string(item->level).c_str(), isRowSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0.f, 0.f)) || (firstFrameForAsset && isRowSelected))
                    {
                        selectedMip = *item;
                        lastSelectedMip = item->level;

                        if (item->isLoaded)
                        {
                            selectedMipTexture = CreateTextureFromMip(pakAsset, &txtrAsset->mipArray.at(selectedMip.index), s_PakToDxgiFormat[txtrAsset->imgFormat], selectedArrayIndex);
                        }
                        else
                        {
                            selectedMipTexture.reset();
                        }
                    }
                }

                if (ImGui::TableSetColumnIndex(TexturePreviewData_t::eColumnID::TPC_Dimensions))
                {
                    if (item->width > 0 && item->height > 0)
                        ImGui::Text("%i x %i", item->width, item->height);
                    else
                        ImGui::TextUnformatted("N/A");
                }

                if (ImGui::TableSetColumnIndex(TexturePreviewData_t::eColumnID::TPC_Status))
                {
                    if (item->isLoaded)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
                        ImGui::TextUnformatted("Loaded");
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
                        ImGui::TextUnformatted("Not Loaded");
                        ImGui::PopStyleColor();
                    }
                }

                if (ImGui::TableSetColumnIndex(TexturePreviewData_t::eColumnID::TPC_Type))
                    ImGui::TextUnformatted(item->typeName);

                if (ImGui::TableSetColumnIndex(TexturePreviewData_t::eColumnID::TPC_Origin))
                    ImGui::TextUnformatted(item->dataOrigin);

                ImGui::PopID(); // no longer working on this id
            }

            ImGui::EndTable();
        }
    }

    

    const CTexture* const selectedMipTxtr = selectedMipTexture.get();
    if (selectedMipTxtr)
    {
        const float aspectRatio = static_cast<float>(selectedMipTxtr->GetWidth()) / selectedMipTxtr->GetHeight();

        float imageHeight = std::max(std::clamp(static_cast<float>(selectedMipTxtr->GetHeight()), 0.f, std::max(ImGui::GetContentRegionAvail().y, 1.f)) - 2.5f, 4.f);
        float imageWidth = imageHeight * aspectRatio;

        imageWidth *= textureZoom;
        imageHeight *= textureZoom;

        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);

        ImGui::Separator();
        ImGui::Text("Scale: %.f%%", textureZoom * 100.f);
        ImGui::SameLine();
        ImGui::NextColumn();

        constexpr const char* const zoomHelpText = "Hold CTRL and scroll to zoom";
        IMGUI_RIGHT_ALIGN_FOR_TEXT(zoomHelpText);
        ImGui::TextUnformatted(zoomHelpText);
        if (ImGui::BeginChild("Texture Preview", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_HorizontalScrollbar)) // [rika]: todo smaller screens will not have the most ideal viewing experience do to the image being squashed
        {
            const bool previewHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            ImGui::Image(selectedMipTxtr->GetSRV(), ImVec2(imageWidth, imageHeight));
            if (previewHovering && ImGui::GetIO().KeyCtrl)
            {
                const float wheel = ImGui::GetIO().MouseWheel;
                const float scrollZoomFactor = ImGui::GetIO().KeyAlt ? (1.f / 20.f) : (1.f / 10.f);

                if (wheel != 0.0f)
                    textureZoom += (wheel * scrollZoomFactor);

                textureZoom = std::clamp(textureZoom, 0.1f, 5.0f);
            }

            static bool resetPos = true;
            static ImVec2 posPrev;
            if (previewHovering && ImGui::GetIO().MouseDown[2] && !ImGui::GetIO().KeyCtrl) // middle mouse
            {
                ImVec2 posCur = ImGui::GetIO().MousePos;

                if (resetPos)
                    posPrev = posCur;

                ImVec2 delta(posCur.x - posPrev.x, posCur.y - posPrev.y);
                ImVec2 scroll(0.0f, 0.0f);

                scroll.x = std::clamp(ImGui::GetScrollX() + delta.x, 0.0f, ImGui::GetScrollMaxX());
                scroll.y = std::clamp(ImGui::GetScrollY() + delta.y, 0.0f, ImGui::GetScrollMaxY());

                ImGui::SetScrollX(scroll.x);
                ImGui::SetScrollY(scroll.y);

                posPrev = posCur;
                resetPos = false;
            }
            else
            {
                resetPos = true;
            }
        }
        ImGui::EndChild();
    }
    else
    {
        const TextureMip_t* const mip = &txtrAsset->mipArray.at(selectedMip.index);
        selectedMipTexture = CreateTextureFromMip(pakAsset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], selectedArrayIndex);
    }

    if (lastSelectedArrayIndex != selectedArrayIndex)
    {
        lastSelectedArrayIndex = selectedArrayIndex;

        const TextureMip_t* const mip = &txtrAsset->mipArray.at(selectedMip.index);
        selectedMipTexture = CreateTextureFromMip(pakAsset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], selectedArrayIndex);
    }

    return nullptr;
}

inline void NormalRecalc(const bool isNormal, CTexture* texture)
{
    if (!isNormal)
        return;

    switch (g_ExportSettings.exportNormalRecalcSetting)
    {
    case eNormalExportRecalc::NML_RECALC_DX:
    {
        texture->ConvertNormalOpenDX();

        return;
    }
    case eNormalExportRecalc::NML_RECALC_OGL:
    {
        texture->ConvertNormalOpenGL();

        return;
    }
    case eNormalExportRecalc::NML_RECALC_NONE:
    default:
        return;
    }
}

bool ExportPngTextureAsset(CPakAsset* const asset, const TextureAsset* const txtrAsset, std::filesystem::path& exportPath, const int setting, const bool isNormal)
{
    // Add extension | replace the .rpak ext.
    exportPath.replace_extension("png");

    const std::filesystem::path fileName = exportPath.filename().replace_extension("");

    switch (setting)
    {
    case eTextureExportSetting::PNG_HM:
    {
        for (size_t arrayIdx = 0; arrayIdx < txtrAsset->arraySize; arrayIdx++)
        {
            // Grab highest mip.
            const TextureMip_t* const mip = &txtrAsset->mipArray[txtrAsset->mipArray.size() - 1];
            std::unique_ptr<char[]> txtrData = GetTextureDataForMip(asset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], arrayIdx);

            if (txtrAsset->arraySize > 1)
            {
                std::string suffix = std::format("_{:03}.png", arrayIdx);
                exportPath.replace_filename(fileName).concat(suffix);
            }

            std::unique_ptr<CTexture> exportTexture = std::make_unique<CTexture>(txtrData.get(), mip->slicePitch, mip->width, mip->height, s_PakToDxgiFormat[txtrAsset->imgFormat], 1u, 1u);

            NormalRecalc(isNormal, exportTexture.get());

            if (!exportTexture->ExportAsPng(exportPath))
                return false;
        }

        return true;
    }
    case eTextureExportSetting::PNG_AM:
    {
        for (size_t arrayIdx = 0; arrayIdx < txtrAsset->arraySize; arrayIdx++)
        {
            for (size_t i = 0; i < txtrAsset->mipArray.size(); ++i)
            {
                const TextureMip_t* const mip = &txtrAsset->mipArray[i];
                
                if (!mip->isLoaded)
                    return false;
                
                std::unique_ptr<char[]> txtrData = GetTextureDataForMip(asset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], arrayIdx);

                // Label levels properly.
                std::string suffix = txtrAsset->arraySize > 1 ? std::format("_{:03}_level{}.png", arrayIdx, i) : std::format("_level{}.png", i);
                exportPath.replace_filename(fileName).concat(suffix);

                std::unique_ptr<CTexture> exportTexture = std::make_unique<CTexture>(txtrData.get(), mip->slicePitch, mip->width, mip->height, s_PakToDxgiFormat[txtrAsset->imgFormat], 1u, 1u);

                NormalRecalc(isNormal, exportTexture.get());

                if (!exportTexture->ExportAsPng(exportPath))
                    return false;
            }
        }

        return true;
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

static const char* GetMipNameForType(const eTextureMipType type)
{
    switch (type)
    {
    case eTextureMipType::RPak: return "permanent";
    case eTextureMipType::StarPak: return "mandatory";
    case eTextureMipType::OptStarPak: return "optional";
    }

    // Unknown type provided.
    assert(0);
    return "unknown";
}

static void ExportTextureMetaData(const TextureAsset* const txtrAsset, std::filesystem::path& exportPath)
{
    exportPath.replace_extension(".json");
    std::ofstream ofs(exportPath, std::ios::out);

    ofs << "{\n";

    const size_t numMips = txtrAsset->mipArray.size();

    if (numMips > 1)
    {
        ofs << "\t\"streamLayout\": [\n";

        for (size_t i = 1; i < numMips; i++) // first mip is always permanent, skip it.
        {
            const TextureMip_t& mip = txtrAsset->mipArray[i];

            const char* const commaChar = i != (numMips - 1) ? "," : "";
            ofs << "\t\t\"" << GetMipNameForType(mip.type) << "\"" << commaChar << "\n";
        }

        ofs << "\t],\n";

        if (txtrAsset->legacy)
        {
            ofs << "\t\"mipInfo\": [\n";

            for (size_t i = 0; i < numMips - 1; i++)
            {
                const char* const commaChar = i != (numMips - 2) ? "," : "";
                ofs << "\t\t" << std::dec << (uint32_t)txtrAsset->unkPerMip_Legacy[i] << commaChar << "\n";;
            }

            ofs << "\t],\n";
        }
    }

    ofs << "\t\"resourceFlags\": \"0x" << std::uppercase << std::hex << (uint32_t)txtrAsset->layerCount << "\",\n";
    ofs << "\t\"usageFlags\": \"0x" << std::uppercase << std::hex << (uint32_t)txtrAsset->usageFlags << "\"\n";

    ofs << "}\n";
}

bool ExportDdsTextureAsset(CPakAsset* const asset, const TextureAsset* const txtrAsset, std::filesystem::path& exportPath, const int setting, const bool isNormal)
{
    // [rika]: run this first, the file name can be altered if a texture is an array. not to mention we likely want this data if exporting as dds.
    ExportTextureMetaData(txtrAsset, exportPath);

    // Add extension | replace the .rpak ext.
    exportPath.replace_extension("dds");

    const std::filesystem::path fileName = exportPath.filename().replace_extension("");

    switch (setting)
    {
    case eTextureExportSetting::DDS_HM:
    {
        for (size_t arrayIdx = 0; arrayIdx < txtrAsset->arraySize; arrayIdx++)
        {
            // Grab highest mip.
            const TextureMip_t* const mip = &txtrAsset->mipArray[txtrAsset->mipArray.size() - 1];
            std::unique_ptr<char[]> txtrData = GetTextureDataForMip(asset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], arrayIdx);

            if (txtrAsset->arraySize > 1)
            {
                std::string suffix = std::format("_{:03}.dds", arrayIdx);
                exportPath.replace_filename(fileName).concat(suffix);
            }

            std::unique_ptr<CTexture> exportTexture = std::make_unique<CTexture>(txtrData.get(), mip->slicePitch, mip->width, mip->height, s_PakToDxgiFormat[txtrAsset->imgFormat], 1u, 1u);

            NormalRecalc(isNormal, exportTexture.get());

            if (!exportTexture->ExportAsDds(exportPath))
                return false;
        }

        return true;
    }
    case eTextureExportSetting::DDS_AM:
    {
        for (size_t arrayIdx = 0; arrayIdx < txtrAsset->arraySize; arrayIdx++)
        {
            for (size_t i = 0; i < txtrAsset->mipArray.size(); ++i)
            {
                const TextureMip_t* const mip = &txtrAsset->mipArray[i];

                if (!mip->isLoaded)
                    return false;

                std::unique_ptr<char[]> txtrData = GetTextureDataForMip(asset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], arrayIdx);

                // Label levels properly.
                std::string suffix = txtrAsset->arraySize > 1 ? std::format("_{:03}_level{}.dds", arrayIdx, i) : std::format("_level{}.dds", i);
                exportPath.replace_filename(fileName).concat(suffix);

                std::unique_ptr<CTexture> exportTexture = std::make_unique<CTexture>(txtrData.get(), mip->slicePitch, mip->width, mip->height, s_PakToDxgiFormat[txtrAsset->imgFormat], 1u, 1u);

                NormalRecalc(isNormal, exportTexture.get());

                if (!exportTexture->ExportAsDds(exportPath))
                    return false;
            }
        }

        return true;
    }
    case eTextureExportSetting::DDS_MM:
    {
        std::unique_ptr<char[]> txtrData(new char[txtrAsset->dataSizeUnaligned * txtrAsset->arraySize] {});
        char* pCurrent = txtrData.get(); // current position inside txtrData

        for (size_t arrayIdx = 0; arrayIdx < txtrAsset->arraySize; arrayIdx++)
        {
            const size_t mipCount = txtrAsset->mipArray.size();
            for (size_t i = 0; i < mipCount; ++i) // cycle through all mips
            {
                const TextureMip_t* const mip = &txtrAsset->mipArray[mipCount - 1 - i];

                if (!mip->isLoaded)
                    return false;

                std::unique_ptr<char[]> mipData = GetTextureDataForMip(asset, mip, s_PakToDxgiFormat[txtrAsset->imgFormat], arrayIdx);

                memcpy_s(pCurrent, mip->slicePitch, mipData.get(), mip->slicePitch); // copy this mip's data into txtr data
                pCurrent += mip->slicePitch; // adjust our current position
            }
        }

        std::unique_ptr<CTexture> exportTexture = std::make_unique<CTexture>(txtrData.get(), txtrAsset->dataSizeUnaligned, txtrAsset->width, txtrAsset->height, s_PakToDxgiFormat[txtrAsset->imgFormat], txtrAsset->arraySize, txtrAsset->mipArray.size());

        NormalRecalc(isNormal, exportTexture.get());

        if (!exportTexture->ExportAsDds(exportPath))
            return false;

        return true;
    }
    case eTextureExportSetting::DDS_MD:
    {
        return true;
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

static const char* const s_PathPrefixTXTR = s_AssetTypePaths.find(AssetType_t::TXTR)->second;
bool ExportTextureAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const TextureAsset* const txtrAsset = reinterpret_cast<TextureAsset*>(pakAsset->extraData());
    assertm(txtrAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path texturePath(asset->GetAssetName());

    const bool hasFullPath = txtrAsset->name != nullptr || g_cacheDBManager.LookupGuid(pakAsset->GetAssetGUID(), nullptr);

    // [rika]: there is no point to add the parent path if we don't have a proper name (it will just end up in 's_PathPrefixTXTR' anyway)
    if (hasFullPath && g_ExportSettings.exportPathsFull)
        exportPath.append(texturePath.parent_path().string());
    else
        exportPath.append(s_PathPrefixTXTR);

    // verify created directory
    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }

    exportPath.append(texturePath.filename().string());

    switch (setting)
    {
    case eTextureExportSetting::PNG_HM:
    case eTextureExportSetting::PNG_AM:
    {
        return ExportPngTextureAsset(pakAsset, txtrAsset, exportPath, setting, false);
    }
    case eTextureExportSetting::DDS_HM:
    case eTextureExportSetting::DDS_AM:
    case eTextureExportSetting::DDS_MM:
    case eTextureExportSetting::DDS_MD:
    {
        return ExportDdsTextureAsset(pakAsset, txtrAsset, exportPath, setting, false);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitTextureAssetType()
{
    static const char* settings[] = { "PNG (Highest Mip)", "PNG (All Mips)", "DDS (Highest Mip)", "DDS (All Mips)", "DDS (Mip Mapped)", "JSON (Meta Data)" };
    AssetTypeBinding_t type =
    {
        .type = 'rtxt',
        .headerAlignment = 8,
        .loadFunc = LoadTextureAsset,
        .postLoadFunc = PostLoadTextureAsset,
        .previewFunc = PreviewTextureAsset,
        .e = { ExportTextureAsset, 0, settings, ARRSIZE(settings) }
    };

    REGISTER_TYPE(type);
}

std::unique_ptr<char[]> UnswizlePS4(const TextureMip_t* const mip, const DXGI_FORMAT format, std::unique_ptr<char[]> txtrData)
{
    std::unique_ptr<char[]> txtrDataOut = std::make_unique<char[]>(mip->sizeSingle);

    const uint8_t bpp = static_cast<uint8_t>(CTexture::GetBpp(format));
    int vp = (bpp * 2);

    const int pixbl = static_cast<int>(CTexture::GetPixelBlock(format));
    if (pixbl == 1)
        vp = bpp / 8;

    const int blocksX = mip->width / pixbl;
    const int blocksY = mip->height / pixbl;

    char tmp[16]; // copy data to unswizzle into here
    int offset = 0; // offset into swizzled texture data

    // parsing in 8x8 chunks of blocks
    // bx block chunk x
    // by block chunk y
    for (int by = 0; by < (blocksY + 7) / 8; by++)
    {
        for (int bx = 0; bx < (blocksX + 7) / 8; bx++)
        {
            for (int i = 0; i < 64; i++)
            {
                const int mr = CTexture::Morton(i, 8, 8);
                const int y = mr / 8; // local y coord within chunk
                const int x = mr % 8; // local x coord within chunk

                if (bx * 8 + x < blocksX && by * 8 + y < blocksY)
                {
                    memcpy(tmp, txtrData.get() + offset, vp);

                    const int dstIdx = (vp) * ((by * 8 + y) * blocksX + bx * 8 + x);
                    memcpy(txtrDataOut.get() + dstIdx, tmp, vp);
                }

                offset += vp;
            }
        }
    }

    return std::move(txtrDataOut);
}

#ifdef SWITCH_SWIZZLE
std::unique_ptr<char[]> UnswizleSwitch(const TextureMip_t* const mip, const DXGI_FORMAT format, std::unique_ptr<char[]> txtrData)
{
    std::unique_ptr<char[]> txtrDataOut = std::make_unique<char[]>(mip->sizeSingle);

    const uint8_t bpp = static_cast<uint8_t>(CTexture::GetBpp(format));
    int vp = (bpp * 2);

    const int pixbl = static_cast<int>(CTexture::GetPixelBlock(format));
    if (pixbl == 1)
        vp = bpp / 8;

    const int blocksX = mip->width / pixbl;
    const int blocksY = mip->height / pixbl;

    int chunksPerSectorY = blocksY / 8;

    if (chunksPerSectorY > 16)
        chunksPerSectorY = 16;

    //const int chunksPerSectorX = 16 / vp;
    int chunksPerSectorX = 1;
    switch (vp)
    {
    case 16:
        chunksPerSectorX = 1;
        break;
    case 8:
        chunksPerSectorX = 2;
        break;
    case 4:
        chunksPerSectorX = 4;
        break;
    default:
        break;
    }

    char tmp[16]; // copy data to unswizzle into here
    int offset = 0;

    // bx chunk sector x
    // by chunk sector y
    for (int by = 0; by < IALIGN(blocksY / s_SwizzleChunkSizeSwitchY, chunksPerSectorY) / chunksPerSectorY; by++)
    {
        for (int bx = 0; bx < IALIGN(blocksX / s_SwizzleChunkSizeSwitchX, chunksPerSectorX) / chunksPerSectorX; bx++)
        {
            for (int blockYIdx = 0; blockYIdx < chunksPerSectorY; blockYIdx++)
            {
                for (int i = 0; i < 32; i++)
                {
                    for (int blockXIdx = 0; blockXIdx < chunksPerSectorX; blockXIdx++)
                    {
                        const int mr = s_SwitchSwizzleLUT[i]; // morton pattern ?
                        const int y = mr / 4; // local y coord within chunk
                        const int x = mr % 4; // local x coord within chunk

                        memcpy(tmp, txtrData.get() + offset, vp);

                        const int globalY = (by * chunksPerSectorY + blockYIdx) * 8 + y;
                        const int globalX = (bx * 4 + x) * chunksPerSectorX + blockXIdx;

                        const int dstIdx = vp * (globalY * blocksX + globalX);
                        memcpy(txtrDataOut.get() + dstIdx, tmp, vp);

                        offset += vp;
                    }
                }
            }
        }
    }

    return std::move(txtrDataOut);
}
#endif

std::unique_ptr<char[]> GetTextureDataForMip(CPakAsset* const asset, const TextureMip_t* const mip, const DXGI_FORMAT format, const size_t arrayIndex)
{
    // [rika]: I swapped back to size (from slicePitch) because it's the size of the mip on disk, and we just create a new buffer anyways if it's compressed. saves some allocation of bytes.
    std::unique_ptr<char[]> txtrData;
    switch (mip->type)
    {
    case eTextureMipType::RPak:
    {
        txtrData = std::make_unique<char[]>(mip->sizeSingle);
        memcpy_s(txtrData.get(), mip->sizeSingle, mip->assetPtr.ptr + (mip->sizeSingle * arrayIndex), mip->sizeSingle);
        break;
    }
    case eTextureMipType::StarPak:
    {
        txtrData = asset->getStarPakData(mip->assetPtr.offset + (mip->sizeSingle * arrayIndex), mip->sizeSingle, false);
        break;
    }
    case eTextureMipType::OptStarPak:
    {
        txtrData = asset->getStarPakData(mip->assetPtr.offset + (mip->sizeSingle * arrayIndex), mip->sizeSingle, true);
        break;
    }
    default:
        assertm(false, "Unknown mip type.");
        break;
    }

    if (mip->compType != eCompressionType::NONE)
    {
        uint64_t slicePitch = mip->slicePitch;
        txtrData = RTech::DecompressStreamedBuffer(std::move(txtrData), slicePitch, mip->compType);
    }

    if (mip->swizzle != eTextureSwizzle::SWIZZLE_NONE)
    {
        switch (mip->swizzle)
        {
        case eTextureSwizzle::SWIZZLE_PS4:
        {
            txtrData = UnswizlePS4(mip, format, std::move(txtrData));
            break;
        }
#ifdef SWITCH_SWIZZLE
        case eTextureSwizzle::SWIZZLE_SWITCH:
        {
            txtrData = UnswizleSwitch(mip, format, std::move(txtrData));
            break;
        }
#endif
        default:
            break;
        }
    }

    return std::move(txtrData);
}