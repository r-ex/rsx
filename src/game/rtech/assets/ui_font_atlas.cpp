#include <pch.h>
#include <game/rtech/assets/ui_font_atlas.h>
#include <game/rtech/assets/texture.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern CDXParentHandler* g_dxHandler;

extern ExportSettings_t g_ExportSettings;

// ui font header
inline const uint32_t UIFontHeader::GetTextureIndexFromUnicodeMap(int unicode) const
{
    if (numUnicodeMaps == 0)
        return FONT_TEXTURE_IDX_INVALID;

    assertm(unicodeMap, "unicode map should be valid if we're here");

    uint32_t mapIdx = 0;

    for (mapIdx = 0; mapIdx < numUnicodeMaps; ++mapIdx)
    {
        // if this font charmap's last character is above the requested char code then the map contains
        // this character, so break early
        if (unicodeMap[mapIdx].unicodeLast > unicode)
            break;
    }

    // if the loop has been exited with a mapIdx value that is == to numUnicodeMaps
    // then the loop did not 
    if (mapIdx >= numUnicodeMaps)
        return FONT_TEXTURE_IDX_INVALID;

    // if the found unicode map's first character is above the requested character, we do not have a valid charmap
    if (unicodeMap[mapIdx].unicodeFirst > unicode)
        return FONT_TEXTURE_IDX_INVALID;

    return unicodeMap[mapIdx].textureIndex + unicode - unicodeMap[mapIdx].unicodeFirst;
}

const uint32_t UIFontHeader::TextureFromUnicodeMap(int unicode) const
{
    const uint32_t foundTextureIndex = GetTextureIndexFromUnicodeMap(unicode);

    if (foundTextureIndex != FONT_TEXTURE_IDX_INVALID)
        return foundTextureIndex;

    // if we failed to find the requested character in the font's charmaps, look for the texture index
    // of a question mark to replace it as a placeholder 
    const uint32_t fallbackTextureIndex = GetTextureIndexFromUnicodeMap(FONT_UTF16_QMARK);

    if (fallbackTextureIndex != FONT_TEXTURE_IDX_INVALID)
        return fallbackTextureIndex;

    // if the fallback also fails, print an error message and return an invalid texture index
    Log("Font %s doesn't even have question mark \"?\"\n", name);

    return FONT_TEXTURE_IDX_INVALID;
}

const uint32_t UIFontHeader::TextureFromUnicode(int unicode) const
{
    while (true)
    {
        const int fixedUnicode = (unicode - unicodeIndex);
        const uint32_t chunkIndex = static_cast<uint32_t>(fixedUnicode) >> 6; // discard the lower bits for our chunk index, this preserves size

        if (chunkIndex < numUnicodeChunks)
        {
            const uint8_t bitIndex = fixedUnicode & 0x3F; // get our discarded bits back as an index, use this to get certain bits

            const int64_t tableIndex = unicodeChunks[chunkIndex]; // index into the other tables
            const int64_t unicodeMask = unicodeChunksMask[tableIndex]; // number of bits (unicodes/textures in this chunk). we get popcount to get the index for this texture, which doesn't count unused unicodes

            const int64_t bit = 1ll << bitIndex;

            // check that bitIndex has this bit set, if not, we don't have a texture for this unicode
            if (bit & unicodeMask)
                return unicodeChunksIndex[tableIndex] + static_cast<uint32_t>(__popcnt64((bit - 1) & unicodeMask));
        }

        if (unicode == FONT_UTF16_BOX)
            break;

        unicode = FONT_UTF16_BOX;
    }

    Log("Font %s doesn't have the code point U+%04X which is required to display a missing glyph.\n", name, FONT_UTF16_BOX);

    return FONT_TEXTURE_IDX_INVALID;
}

const uint32_t UIFontHeader::TextureFromGlyph(int glyph) const
{
    while (true)
    {
        const int fixedGlyph = (glyph - glyphIndex);
        const uint32_t chunkIndex = static_cast<uint32_t>(fixedGlyph) >> 6;

        if (chunkIndex < numGlyphChunks)
        {
            const uint8_t bitIndex = fixedGlyph & 0x3F;

            const int64_t tableIndex = glyphChunks[chunkIndex];
            const int64_t glyphMask = glyphChunksMask[tableIndex];

            const int64_t bit = 1ll << bitIndex;

            if (bit & glyphMask)
                return glyphChunksIndex[tableIndex] + static_cast<uint32_t>(__popcnt64((bit - 1) & glyphMask));
        }

        if (!glyph)
            break;

        glyph = LOBYTE(errorGlyph) << 16; // 
    }

    Log("Font %s doesn't have the glyph index %u which is required to display a missing glyph.\n", name, 0u);

    return FONT_TEXTURE_IDX_INVALID;
}

void UIFontHeader::ParseProportions(Vector2D* const imageBoundsScale, Vector2D* const imageSizeBase) const
{
    // this is how R2TT does it exactly, I do not want to lose this.
    /*
    imageBoundsScale[0].x = proportionScaleX;
    imageBoundsScale[0].y = proportionScaleY;

    imageSizeBase[0].x = proportionSizeScale * imageBoundsScale[0].x;
    imageSizeBase[0].y = proportionSizeScale * imageBoundsScale[0].y;
    */

    assertm(numProportions <= FONT_MAX_PROPORTIONS, "this would cause issues");

    for (uint16_t i = 0; i < numProportions; i++)
    {
        imageBoundsScale[i].x = proportions[i].scaleBounds * proportionScaleX;
        imageBoundsScale[i].y = proportions[i].scaleBounds * proportionScaleY;

        // used to get a minimum character size (I think)
        imageSizeBase[i].x = proportions[i].scaleSize * imageBoundsScale[i].x;
        imageSizeBase[i].y = proportions[i].scaleSize * imageBoundsScale[i].y;
    }
}

void UIFontCharacter_t::SetBounds(const UIFontAtlasAsset* const fontAsset, const UIFontTexture* const texture, const Vector2D* const imageBoundsScale, const Vector2D* const imageSizeBase)
{
    if (texture->posMinX == texture->posMaxX) UNLIKELY
    {
        mins.x = 1.0f;
        mins.y = 1.0f;

        maxs.x = 0.0f;
        maxs.y = 0.0f;

        // not handled in the native function, handled in the shader! (probably)
        // hard set these to avoid unneeded math and checks.
        posX = fontAsset->width;
        posY = fontAsset->height;

        width = 0;
        height = 0;

        return;
    }

    mins.x = ((imageBoundsScale[texture->proportionIndex].x * texture->posMinX) + texture->posBaseX) - imageSizeBase[texture->proportionIndex].x;
    mins.y = ((imageBoundsScale[texture->proportionIndex].y * texture->posMinY) + texture->posBaseY) - imageSizeBase[texture->proportionIndex].y;

    maxs.x = ((imageBoundsScale[texture->proportionIndex].x * texture->posMaxX) + texture->posBaseX) + imageSizeBase[texture->proportionIndex].x;
    maxs.y = ((imageBoundsScale[texture->proportionIndex].y * texture->posMaxY) + texture->posBaseY) + imageSizeBase[texture->proportionIndex].y;

    // [rika]: same deal in this case with shadering rounding, hence the '+ 0.5f', but in this case it yielded noticeably better results after the change
    // [rika]: output is now improved, closer but maybe still a bit off, definitely handled within shader.
    posX = static_cast<uint16_t>(mins.x * static_cast<float>(fontAsset->width) + 0.5f);
    posY = static_cast<uint16_t>(mins.y * static_cast<float>(fontAsset->height) + 0.5f);

    width = static_cast<uint16_t>((maxs.x - mins.x) * static_cast<float>(fontAsset->width) + 0.5f);
    height = static_cast<uint16_t>((maxs.y - mins.y) * static_cast<float>(fontAsset->height) + 0.5f);
}

void UIFontHeader::SetCharacterForImages(const UIFontAtlasAsset* const fontAsset, const int maxCodepoint, const uint32_t numTextures, const uint32_t(UIFontHeader::* TextureFromCharacter)(int) const) const
{
    uint32_t remainingTextures = numTextures;
    for (int j = 0; j < maxCodepoint; j++)
    {
        if (remainingTextures == 0)
            break;

        const uint32_t idx = (this->*TextureFromCharacter)(j);

        if (idx >= numTextures)
            continue;

        UIFontCharacter_t* image = &fontAsset->images[textureIndex + idx];

        // has this image already been assigned a unicode character?
        // note: unsupported unicode characters often map to one image
        // note: on v6 everything gets bound to the '?' glyph, causing it to display the incorrect code.
        if (image->utf16 != -1)
        {
            //Log("image %u had existing character binding, current: %x new: %x\n", idx, image->utf16, j);
            continue;
        }

        image->utf16 = j;

        remainingTextures--;
    }

    //assertm(remainingTextures == 0, "not every image was assigned a unicode character.");
}

void LoadUIFontAtlasAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	UIFontAtlasAsset* fontAsset = nullptr;

	switch (pakAsset->version())
	{
	case 6:
	case 7:
	case 10:
	{
		const UIFontAtlasAssetHeader_v6_t* const hdr = reinterpret_cast<const UIFontAtlasAssetHeader_v6_t* const>(pakAsset->header());
		fontAsset = new UIFontAtlasAsset(hdr, pakAsset->version());

		break;
	}
    case 12:
    {
        const UIFontAtlasAssetHeader_v12_t* const hdr = reinterpret_cast<const UIFontAtlasAssetHeader_v12_t* const>(pakAsset->header());
        fontAsset = new UIFontAtlasAsset(hdr);

        break;
    }
	default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        return;
    }
	}

    const bool useGlyphLookup = pakAsset->version() >= 12 ? true : false;

    // [rika]: setup images
    {
        const UIFontHeader* const lastFont = &fontAsset->fontData.at(fontAsset->fontCount - 1);

        fontAsset->imageCount = lastFont->textureIndex + (useGlyphLookup ? lastFont->numGlyphTextures : lastFont->numUnicodeTextures);
        fontAsset->images = new UIFontCharacter_t[fontAsset->imageCount];
    }

    assertm(fontAsset->images, "invalid image pointer?");

    // handled as one buffer with an index in source
    // different proportions of characters
    Vector2D imageBoundsScale[8];
    Vector2D imageSizeBase[8];

    for (uint16_t i = 0; i < fontAsset->fontCount; i++)
    {
        const UIFontHeader* const font = &fontAsset->fontData.at(i);

        font->ParseProportions(imageBoundsScale, imageSizeBase);

        // [rika]: code checks for font 10 when deciding about glyphs for whatever reason...
        // [rika]: this is not quite right, also font 10 has no error codepoint
        // [rika]: switch doesn't use glyphs! could be useful for figuring out how glyphs work
        if (useGlyphLookup && font->fontIndex != 10 && font->glyphTextures) LIKELY
        {
            for (uint32_t j = 0; j < font->numGlyphTextures; j++)
            {
                UIFontCharacter_t* image = &fontAsset->images[font->textureIndex + j];

                image->utf16 = -1;
                image->glyph = -1;

                image->font = i;
                image->SetBounds(fontAsset, &font->glyphTextures[j], imageBoundsScale, imageSizeBase);
            }

            const int maxGlyph = (font->numGlyphChunks + 1) << 6;

            font->SetCharacterForImages(fontAsset, maxGlyph, font->numGlyphTextures, &UIFontHeader::TextureFromGlyph);

            continue;
        }

        for (uint32_t j = 0; j < font->numUnicodeTextures; j++)
        {
            UIFontCharacter_t* image = &fontAsset->images[font->textureIndex + j];

            image->utf16 = -1;
            image->glyph = -1;

            image->font = i;
            image->SetBounds(fontAsset, &font->unicodeTextures[j], imageBoundsScale, imageSizeBase);
        }

        // [rika]: r2tt handling
        if (font->numUnicodeMaps > 0) UNLIKELY
        {
            // get the max number of unicodes in here!
            const int fakeChunkCount = (font->unicodeMap[font->numUnicodeMaps - 1].unicodeLast >> 6); // fake it til we make it, get the last possible chunk if this wasn't a legacy map!
            const int maxUnicode = (fakeChunkCount + 1) << 6;

            font->SetCharacterForImages(fontAsset, maxUnicode, font->numUnicodeTextures, &UIFontHeader::TextureFromUnicodeMap);

            continue;
        }

        // [rika]: versions 7 & 10
        const int maxUnicode = (font->numUnicodeChunks + 1) << 6;

        font->SetCharacterForImages(fontAsset, maxUnicode, font->numUnicodeTextures, &UIFontHeader::TextureFromUnicode);
    }

    pakAsset->setExtraData(fontAsset);
}

extern CDXParentHandler* g_dxHandler;
void PostLoadUIFontAtlasAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    if (!pakAsset->extraData())
        return;

	// Setup main texture.
	UIFontAtlasAsset* const fontAsset = reinterpret_cast<UIFontAtlasAsset*>(pakAsset->extraData());
	assertm(fontAsset, "Extra data should be valid at this point.");

	CPakAsset* const textureAsset = g_assetData.FindAssetByGUID<CPakAsset>(fontAsset->atlasGUID);
	assertm(textureAsset, "Asset should be valid.");

	TextureAsset* const txtrAsset = reinterpret_cast<TextureAsset*>(textureAsset->extraData());
	assertm(txtrAsset, "Extra data should be valid.");

	if (txtrAsset->name)
	{
		std::string atlasName = "ui_font_atlas/" + std::string(txtrAsset->name) + ".rpak";

		assertm(pakAsset->data()->guid == RTech::StringToGuid(atlasName.c_str()), "hashed name for atlas did not match existing guid\n");

		pakAsset->SetAssetName(atlasName, true);
	}
    else
    {
        pakAsset->SetAssetNameFromCache();
    }

	assertm(txtrAsset->mipArray.size() >= 1, "Mip array should at least contain idx 0.");
	TextureMip_t* const highestMip = &txtrAsset->mipArray[0];

    fontAsset->txtrFormat = s_PakToDxgiFormat[txtrAsset->imgFormat];

    if (fontAsset->txtrFormat == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
    {
        assertm(false, "unknown format");
        return;
    }

    std::unique_ptr<char[]> txtrData = GetTextureDataForMip(textureAsset, highestMip, fontAsset->txtrFormat); // parse texture through this mip function instead of copying, that way if swizzling is present it gets fixed.

	// Only raw needs SRV.
	fontAsset->txtrRaw = std::make_shared<CTexture>(txtrData.get(), highestMip->slicePitch, highestMip->width, highestMip->height, fontAsset->txtrFormat, 1u, 1u);
	fontAsset->txtrRaw->CreateShaderResourceView(g_dxHandler->GetDevice());

	// Convert to respective srgb non srgb format for texture slicing later.
	fontAsset->txtrConverted = std::make_shared<CTexture>(txtrData.get(), highestMip->slicePitch, highestMip->width, highestMip->height, fontAsset->txtrFormat, 1u, 1u);
	fontAsset->txtrConverted->ConvertToFormat(IsSRGB(fontAsset->txtrFormat) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM); // Convert in advance to non compressed format for preview.
}

struct UICharacterPreviewData_t
{
    enum eColumnID
    {
        TPC_Index, // base should be -1
        TPC_Dimensions,
        TPC_Position,
        TPC_Unicode,

        _TPC_COUNT,
    };

    uint16_t width;
    uint16_t height;
    uint16_t posX;
    uint16_t posY;

    int utf16;

    int index;
    int localIndex;

    const bool operator==(const UICharacterPreviewData_t& in)
    {
        return height == in.height && width == in.width && posX == in.posX && posY == in.posY && in.utf16 == utf16 && index == in.index;
    }

    const bool operator==(const UICharacterPreviewData_t* in)
    {
        return height == in->height && width == in->width && posX == in->posX && posY == in->posY && in->utf16 == utf16 && index == in->index;
    }
};

struct UICharCompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    bool operator() (const UICharacterPreviewData_t& a, const UICharacterPreviewData_t& b) const
    {

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case UICharacterPreviewData_t::eColumnID::TPC_Index:      delta = (a.index - b.index);                                                            break;
            case UICharacterPreviewData_t::eColumnID::TPC_Dimensions: delta = (static_cast<int>(a.width * a.height) - static_cast<int>(b.width * b.height));  break;
            case UICharacterPreviewData_t::eColumnID::TPC_Position:   delta = (static_cast<int>(a.posX * a.posY) - static_cast<int>(b.posX * b.posY));        break;
            case UICharacterPreviewData_t::eColumnID::TPC_Unicode:    delta = (static_cast<int>(a.utf16) - static_cast<int>(b.utf16));                        break;
            }
            if (delta > 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? false : true;
            if (delta < 0)
                return (sort_spec->SortDirection == ImGuiSortDirection_Ascending) ? true : false;
        }

        return (a.index - b.index) > 0;
    }
};

#undef max
// some performance issues ?
void* PreviewUIFontAtlasAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    assertm(asset, "Asset should be valid.");

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    if (!pakAsset->extraData())
    {
        ImGui::Text("This asset version is not currently supported for preview.");
        return nullptr;
    }

    static bool txtrRegistered = []() {
        auto it = g_assetData.m_assetTypeBindings.find('rtxt');
        return it != g_assetData.m_assetTypeBindings.end();
    } ();

    if (!txtrRegistered)
    {
        ImGui::TextUnformatted("Txtr was not registered as an asset binding.");
        return nullptr;
    }

    UIFontAtlasAsset* const fontAsset = reinterpret_cast<UIFontAtlasAsset*>(pakAsset->extraData());
    assertm(fontAsset, "Extra data should be valid at this point.");

    static float textureZoom = 1.0f;
    static int lastSelectedTexture = -1;
    static UICharacterPreviewData_t selectedFontCharacter = { .index = -1 };
    static std::shared_ptr<CTexture> selectedUITexture = nullptr;
    static std::vector<UICharacterPreviewData_t> previewTextures;

    struct UIFontPreviewData_t
    {
        UIFontPreviewData_t(const std::string& fontName, const int fontIndex) : name(fontName), index(fontIndex) {};

        const std::string name;
        const int index;
    };

    static UIFontPreviewData_t* selectedUIFont = nullptr;
    static int lastSelectedUIFont = 0;
    static std::vector<UIFontPreviewData_t> previewUIFonts;

    // TODO: ui_font_atlas and ui_image_atlas share this lambda, should we try to make it a function?
    auto CreateTextureForImage = [](UIFontAtlasAsset* const fontAsset, const UIFontCharacter_t* const fontCharacter) -> std::shared_ptr<CTexture>
    {
        assertm(fontAsset->txtrRaw, "Atlas texture wasn't created yet.");
        assertm(fontAsset->txtrConverted, "Converted atlas texture wasn't created yet.");

        if (!fontAsset->txtrRaw || !fontAsset->txtrConverted)
            return nullptr;

        // This will be the main texture.
        if (fontCharacter->width == fontAsset->width && fontCharacter->height == fontCharacter->height)
            return fontAsset->txtrRaw;

        // invalid image
        if (fontCharacter->width * fontCharacter->height == 0)
            return nullptr;

        // Create texture / shader and get slice for image.
        std::shared_ptr<CTexture> txtrData = std::make_shared<CTexture>(nullptr, 0u, fontCharacter->width, fontCharacter->height, (IsSRGB(fontAsset->txtrFormat) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
        txtrData->CopySourceTextureSlice(fontAsset->txtrConverted.get(), static_cast<size_t>(fontCharacter->posX), static_cast<size_t>(fontCharacter->posY), fontCharacter->width, fontCharacter->height, 0u, 0u);
        txtrData->CreateShaderResourceView(g_dxHandler->GetDevice());

        return txtrData;
    };

    static int imgArraySize = -1; // This cast is fine, we won't hit INT32_MAX.
    if (firstFrameForAsset) // Reset if new asset.
    {
        previewUIFonts.clear();
        previewUIFonts.reserve(fontAsset->fontCount);

        for (uint16_t i = 0; i < fontAsset->fontCount; i++)
        {
            const UIFontHeader* const font = &fontAsset->fontData.at(i);

            const std::string name = !font->name ? std::format("font_{}", font->fontIndex) : font->name;
            previewUIFonts.emplace_back(name, static_cast<int>(i));
        }

        previewTextures.clear();
        previewTextures.resize(fontAsset->imageCount); // account for base texture
        imgArraySize = static_cast<int>(fontAsset->imageCount);

        selectedFontCharacter.index = imgArraySize + 1;
        selectedUITexture.reset();

        for (int64_t i = 0; i < fontAsset->imageCount; i++)
        {
            UICharacterPreviewData_t& previewData = previewTextures.at(i);
            const UIFontCharacter_t& image = fontAsset->images[i];

            // [rika]: texture did not get initalized, likely from using unicode instead of glyphs
            if (image.font >= fontAsset->fontCount)
            {
                continue;
            }

            previewData.index = static_cast<int>(i);
            previewData.localIndex = static_cast<int>(i) - fontAsset->fontData.at(image.font).textureIndex;
            previewData.width = image.width;
            previewData.height = image.height;
            previewData.posX = image.posX;
            previewData.posY = image.posY;

            previewData.utf16 = image.utf16;
        }

        selectedFontCharacter = selectedFontCharacter.index >= imgArraySize ? previewTextures.front() : selectedFontCharacter; // if negative one, casting to unsign will mean it's larger than the size
        selectedUIFont = &previewUIFonts.front();

    }

    assertm(selectedFontCharacter.index < imgArraySize, "Selected ui image is out of bounds for img array.");

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    ImGui::TextUnformatted(std::format("Atlas: {} (0x{:X})", pakAsset->GetAssetName().c_str(), pakAsset->data()->guid).c_str());

    if (fontAsset->fontCount > 1)
    {
        ImGui::TextUnformatted("Font:");
        ImGui::SameLine();

        static const char* arrayLabel = selectedUIFont->name.c_str();
        if (ImGui::BeginCombo("##Font", arrayLabel, ImGuiComboFlags_NoArrowButton))
        {
            for (int i = 0; i < fontAsset->fontCount; i++)
            {
                const bool isSelected = selectedUIFont->index == i || (firstFrameForAsset && selectedUIFont->index == lastSelectedUIFont);

                if (ImGui::Selectable(previewUIFonts.at(i).name.c_str(), isSelected))
                {
                    selectedUIFont = &previewUIFonts.at(i);
                    arrayLabel = selectedUIFont->name.c_str();
                }

                if (isSelected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
    }

    if (ImGui::BeginTable("Image Table", UICharacterPreviewData_t::eColumnID::_TPC_COUNT, tableFlags, outerSize))
    {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, UICharacterPreviewData_t::eColumnID::TPC_Index);
        ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UICharacterPreviewData_t::eColumnID::TPC_Dimensions);
        ImGui::TableSetupColumn("Positions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UICharacterPreviewData_t::eColumnID::TPC_Position);
        ImGui::TableSetupColumn("Unicode", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UICharacterPreviewData_t::eColumnID::TPC_Unicode);
        ImGui::TableSetupScrollFreeze(1, 1);

        const UIFontHeader* const selectedFont = &fontAsset->fontData.at(selectedUIFont->index);

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); // get the sorting settings from this table

        if (sortSpecs && (firstFrameForAsset || sortSpecs->SpecsDirty) && previewTextures.size() > 1)
        {
            std::sort(previewTextures.begin() + selectedFont->textureIndex, previewTextures.begin() + (selectedFont->textureIndex + selectedFont->numUnicodeTextures), UICharCompare_t(sortSpecs));
            sortSpecs->SpecsDirty = false;
        }

        ImGui::TableHeadersRow();
        
        ImGuiListClipper clipper;
        clipper.Begin(selectedFont->numUnicodeTextures);
        while (clipper.Step())
        {
            for (int rowNum = clipper.DisplayStart; rowNum < clipper.DisplayEnd; rowNum++)
            {
                const uint32_t i = rowNum + selectedFont->textureIndex;
                const UICharacterPreviewData_t* item = &previewTextures.at(i);

                const bool isRowSelected = selectedFontCharacter == item || (firstFrameForAsset && item->index == lastSelectedTexture);

                ImGui::PushID(item->index); // id of this line ?

                ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

                if (ImGui::TableSetColumnIndex(UICharacterPreviewData_t::eColumnID::TPC_Index))
                {
                    if (ImGui::Selectable(std::to_string(item->localIndex).c_str(), isRowSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0.f, 0.f)) || (firstFrameForAsset && isRowSelected))
                    {
                        selectedFontCharacter = *item;
                        lastSelectedTexture = item->index;

                        selectedUITexture = CreateTextureForImage(fontAsset, &fontAsset->images[selectedFontCharacter.index]);
                    }
                }

                if (ImGui::TableSetColumnIndex(UICharacterPreviewData_t::eColumnID::TPC_Dimensions))
                {
                    if (item->width > 0 && item->height > 0)
                        ImGui::Text("%i x %i", item->width, item->height);
                    else
                        ImGui::TextUnformatted("N/A");
                }

                if (ImGui::TableSetColumnIndex(UICharacterPreviewData_t::eColumnID::TPC_Position))
                {
                    ImGui::Text("(%i, %i)", item->posX, item->posY);
                }

                if (ImGui::TableSetColumnIndex(UICharacterPreviewData_t::eColumnID::TPC_Unicode))
                    ImGui::Text("U+%04x", item->utf16);

                ImGui::PopID(); // no longer working on this id

            }
        }

        ImGui::EndTable();
    }

    const CTexture* const selectedUITxtr = selectedUITexture.get();
    if (selectedUITxtr)
    {
        const float aspectRatio = static_cast<float>(selectedUITexture->GetWidth()) / selectedUITexture->GetHeight();

        float imageHeight = std::max(std::clamp(static_cast<float>(selectedUITexture->GetHeight()), 0.f, std::max(ImGui::GetContentRegionAvail().y, 1.f)) - 2.5f, 4.f);
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
        if (ImGui::BeginChild("Texture Preview", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const bool previewHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            ImGui::Image(selectedUITexture->GetSRV(), ImVec2(imageWidth, imageHeight));
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
        const UIFontCharacter_t* const fontCharacter = &fontAsset->images[selectedFontCharacter.index];
        selectedUITexture = CreateTextureForImage(fontAsset, fontCharacter);
    }

    if (lastSelectedUIFont != selectedUIFont->index)
    {
        lastSelectedUIFont = selectedUIFont->index;

        const UIFontCharacter_t* const fontCharacter = &fontAsset->images[fontAsset->fontData.at(selectedUIFont->index).textureIndex + selectedFontCharacter.localIndex];
        selectedUITexture = CreateTextureForImage(fontAsset, fontCharacter);
    }

    return nullptr;
}

enum eUIFontAtlasExportSetting
{
    PNG_AT, // PNG (Atlas)
    PNG_T,  // PNG (Textures)
    DDS_AT, // DDS (Atlas)
    DDS_T,  // DDS (Textures)
};

static const char* const s_PathPrefixFONT = s_AssetTypePaths.find(AssetType_t::FONT)->second;
bool ExportUIFontAtlasAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    if (!pakAsset->extraData())
        return false;

    UIFontAtlasAsset* const uiAsset = reinterpret_cast<UIFontAtlasAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path atlasPath(pakAsset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(atlasPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixFONT);

    if (!CreateDirectories(exportPath.parent_path()))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(atlasPath.stem().string());

    switch (setting)
    {
    case eUIFontAtlasExportSetting::PNG_AT:
    case eUIFontAtlasExportSetting::DDS_AT:
    {
        assertm(uiAsset->txtrRaw, "Atlas was not valid.");

        switch (setting)
        {
        case eUIFontAtlasExportSetting::PNG_AT:
        {
            // Add png for file extension.
            exportPath.replace_extension("png");

            if (!uiAsset->txtrRaw->ExportAsPng(exportPath))
                return false;

            return true;
        }
        case eUIFontAtlasExportSetting::DDS_AT:
        {
            // Add dds for file extension.
            exportPath.replace_extension("dds");

            if (!uiAsset->txtrRaw->ExportAsDds(exportPath))
                return false;

            return true;
        }
        }

        return false;
    }
    case eUIFontAtlasExportSetting::DDS_T:
    {
        assertm(uiAsset->txtrConverted, "Converted atlas was not valid.");

        std::atomic<uint32_t> remainingFonts = 0; // we don't actually need thread safe here
        const ProgressBarEvent_t* const fontExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Fonts..", static_cast<uint32_t>(uiAsset->fontCount), &remainingFonts, true);
        for (uint16_t idx = 0; idx < uiAsset->fontCount; idx++)
        {
            const UIFontHeader* const font = &uiAsset->fontData.at(idx);
            const std::string name = !font->name ? std::format("unnamed_{}", idx) : font->name;

            std::filesystem::path fontPath = exportPath;
            fontPath.append(name);

            if (!CreateDirectories(fontPath))
            {
                assertm(false, "Failed to create export type directory");
                return false;
            }

            fontPath.append("generic");

            for (uint32_t texIdx = 0; texIdx < font->numUnicodeTextures; texIdx++)
            {
                const UIFontCharacter_t* const character = &uiAsset->images[font->textureIndex + texIdx];

                if (character->width * character->height == 0)
                    continue;

                fontPath.replace_filename(std::format("U+{:04x}.dds", character->utf16));

                std::unique_ptr<CTexture> sliceData = std::make_unique<CTexture>(nullptr, 0u, character->width, character->height, (IsSRGB(uiAsset->txtrFormat) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
                sliceData->CopySourceTextureSlice(uiAsset->txtrConverted.get(), static_cast<size_t>(character->posX), static_cast<size_t>(character->posY), character->width, character->height, 0u, 0u);
                sliceData->ExportAsDds(fontPath);
            }

            remainingFonts++;
        }
        g_pImGuiHandler->FinishProgressBarEvent(fontExportProgress);

        return true;
    }
    case eUIFontAtlasExportSetting::PNG_T:
    {
        assertm(uiAsset->txtrConverted, "Converted atlas was not valid.");

        std::atomic<uint32_t> remainingFonts = 0; // we don't actually need thread safe here
        const ProgressBarEvent_t* const fontExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Fonts..", static_cast<uint32_t>(uiAsset->fontCount), &remainingFonts, true);
        for (uint16_t idx = 0; idx < uiAsset->fontCount; idx++)
        {
            const UIFontHeader* const font = &uiAsset->fontData.at(idx);
            const std::string name = !font->name ? std::format("unnamed_{}", idx) : font->name;

            std::filesystem::path fontPath = exportPath;
            fontPath.append(name);

            if (!CreateDirectories(fontPath))
            {
                assertm(false, "Failed to create export type directory");
                return false;
            }

            fontPath.append("generic");

            for (uint32_t texIdx = 0; texIdx < font->numUnicodeTextures; texIdx++)
            {
                const UIFontCharacter_t* const character = &uiAsset->images[font->textureIndex + texIdx];

                if (character->width * character->height == 0)
                    continue;

                fontPath.replace_filename(std::format("U+{:04x}.png", character->utf16));

                std::unique_ptr<CTexture> sliceData = std::make_unique<CTexture>(nullptr, 0u, character->width, character->height, (IsSRGB(uiAsset->txtrFormat) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
                sliceData->CopySourceTextureSlice(uiAsset->txtrConverted.get(), static_cast<size_t>(character->posX), static_cast<size_t>(character->posY), character->width, character->height, 0u, 0u);
                sliceData->ExportAsPng(fontPath);
            }

            remainingFonts++;
        }
        g_pImGuiHandler->FinishProgressBarEvent(fontExportProgress);

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

void InitUIFontAtlasAssetType()
{
    static const char* settings[] = { "PNG (Atlas)", "PNG (Textures)", "DDS (Atlas)", "DDS (Textures)" };
    AssetTypeBinding_t type =
    {
        .type = 'tnof',
        .headerAlignment = 8,
        .loadFunc = LoadUIFontAtlasAsset,
        .postLoadFunc = PostLoadUIFontAtlasAsset,
        .previewFunc = PreviewUIFontAtlasAsset,
        .e = { ExportUIFontAtlasAsset, 0, settings, ARRAYSIZE(settings) },
    };

    REGISTER_TYPE(type);
}