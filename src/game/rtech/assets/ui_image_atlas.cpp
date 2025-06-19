#include <pch.h>
#include <game/rtech/assets/ui_image_atlas.h>
#include <game/rtech/assets/texture.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

void LoadUIImageAtlasAsset(CAssetContainer* const pak, CAsset* const asset)
{
	UNUSED(pak);
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    const int version = pakAsset->version();

    UIImageAtlasAsset* uiAsset = nullptr;
    switch (version)
    {
    case 10:
    case 11:
    {
        UIImageAtlasAssetHeader_v10_t* hdr = reinterpret_cast<UIImageAtlasAssetHeader_v10_t*>(pakAsset->header());
        uiAsset = new UIImageAtlasAsset(hdr);

        break;
    }
    default:
    {
        assertm(false, "unaccounted asset version, will cause major issues!");
        break;
    }
    }

    assertm(pakAsset->cpu(), "no cpu data page");
    const UIImageAtlasBounds_t* const bounds = reinterpret_cast<const UIImageAtlasBounds_t* const>(pakAsset->cpu());

    // [rika]: allocate in one go because we know how many textures there are upfront
    uiAsset->imageArray.reserve(uiAsset->textureCount);

	for (uint16_t i = 0; i < uiAsset->textureCount; ++i)
	{
		UIAtlasImage img = {};

        // [rika]: this should be used for preview
        img.offsets = uiAsset->textureOffsets + i;

        const UIImageAtlasDimension_t* const dimensions = uiAsset->textureDimensions + i;
        img.dimensionsWidth = dimensions->width;
        img.dimensionsHeight = dimensions->height;

        // for fetching this from texture
        img.bounds = bounds + i;

        // [rika]: should this have the same rounding as below ? todo study this
		img.posX = static_cast<uint16_t>(img.bounds->minX * static_cast<float>(uiAsset->width));
		img.posY = static_cast<uint16_t>(img.bounds->minY * static_cast<float>(uiAsset->height));

        // [rika]: size within the image, SampleLevel (shader) rounds to nearest fixed point (source: trust me bro), hence the + 0.5f
        // note: while at first glance it seems everything is fine, if for some reason artifacting or bad exporting happens check this first
        img.width = static_cast<uint16_t>(img.bounds->sizeX * static_cast<float>(uiAsset->width) + 0.5f);
        img.height = static_cast<uint16_t>(img.bounds->sizeY * static_cast<float>(uiAsset->height) + 0.5f);

        const char* path = nullptr;

        // [rika]: pathTableOffset should be 0 without names, but we double check if textureNames is a valid ptr after just to be sure
        switch (version)
        {
        case 10:
        {
            const UIImageAtlasHash_v10_t* const imgHash = uiAsset->pTextureHash_V10(i);

            img.pathHash = imgHash->pathHash;
            path = uiAsset->textureNames + imgHash->pathTableOffset;

            break;
        }
        case 11:
        {
            const UIImageAtlasHash_v11_t* const imgHash = uiAsset->pTextureHash_V11(i);

            img.pathHash = imgHash->pathHash;
            path = uiAsset->textureNames + imgHash->pathTableOffset;

            break;
        }
        }

		if (uiAsset->textureNames && path)
		{
			img.path = path;
		}
        else
        {
            img.path = std::format("0x{:X}", img.pathHash);
        }

		uiAsset->imageArray.push_back(img);
	}
    std::sort(uiAsset->imageArray.begin(), uiAsset->imageArray.end(), [](const UIAtlasImage& a, const UIAtlasImage& b) { return a.width < b.width; });

    // Setup image container for root image.
    UIAtlasImage rootImg = {};

    rootImg.width = uiAsset->width;
    rootImg.height = uiAsset->height;

    rootImg.dimensionsWidth = uiAsset->width;
    rootImg.dimensionsHeight = uiAsset->height;

    uiAsset->imageArray.push_back(rootImg);

	pakAsset->setExtraData(uiAsset);
}

extern CDXParentHandler* g_dxHandler;
void PostLoadUIImageAtlasAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    // Setup main texture.
    UIImageAtlasAsset* const uiAsset = reinterpret_cast<UIImageAtlasAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    CPakAsset* const textureAsset = g_assetData.FindAssetByGUID<CPakAsset>(uiAsset->atlasGUID);
    assertm(textureAsset, "Asset should be valid.");

    TextureAsset* const txtrAsset = reinterpret_cast<TextureAsset*>(textureAsset->extraData());
    assertm(txtrAsset, "Extra data should be valid.");

    if (txtrAsset->name)
    {
        std::string atlasName = "ui_image_atlas/" + std::string(txtrAsset->name) + ".rpak";

        assertm(pakAsset->data()->guid == RTech::StringToGuid(atlasName.c_str()), "hashed name for atlas did not match existing guid\n");

        pakAsset->SetAssetName(atlasName, true);
    }
    else
    {
        pakAsset->SetAssetNameFromCache();
    }

    assertm(txtrAsset->mipArray.size() >= 1, "Mip array should at least contain idx 0.");
    TextureMip_t* const highestMip = &txtrAsset->mipArray[0];

    uiAsset->format = s_PakToDxgiFormat[txtrAsset->imgFormat];

    if (uiAsset->format == DXGI_FORMAT::DXGI_FORMAT_UNKNOWN)
    {
        assertm(false, "unknown format");
        return;
    }

    std::unique_ptr<char[]> txtrData = GetTextureDataForMip(textureAsset, highestMip, uiAsset->format); // parse texture through this mip function instead of copying, that way if swizzling is present it gets fixed.

    // Only raw needs SRV.
    uiAsset->rawTxtr = std::make_shared<CTexture>(txtrData.get(), highestMip->slicePitch, highestMip->width, highestMip->height, uiAsset->format, 1u, 1u);
    uiAsset->rawTxtr->CreateShaderResourceView(g_dxHandler->GetDevice());

    // Convert to respective srgb non srgb format for texture slicing later.
    uiAsset->convertedTxtr = std::make_shared<CTexture>(txtrData.get(), highestMip->slicePitch, highestMip->width, highestMip->height, uiAsset->format, 1u, 1u);
    uiAsset->convertedTxtr->ConvertToFormat(IsSRGB(uiAsset->format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM); // Convert in advance to non compressed format for preview.
}

struct UITexturePreviewData_t
{
    enum eColumnID
    {
        TPC_Index, // base should be -1
        TPC_Dimensions,
        TPC_Position,
        TPC_RenderSize,
        TPC_Name,

        _TPC_COUNT,
    };

    const std::string* name;

    uint16_t width;
    uint16_t height;

    uint16_t posX;
    uint16_t posY;
    int index;

    // data used to render this image in game
    uint16_t renderWidth;
    uint16_t renderHeight;

    const bool operator==(const UITexturePreviewData_t& in)
    {
        return height == in.height && width == in.width && posX == in.posX && posY == in.posY && index == in.index;
    }

    const bool operator==(const UITexturePreviewData_t* in)
    {
        return height == in->height && width == in->width && posX == in->posX && posY == in->posY && index == in->index;
    }
};

struct UITexCompare_t
{
    const ImGuiTableSortSpecs* sortSpecs;

    bool operator() (const UITexturePreviewData_t& a, const UITexturePreviewData_t& b) const
    {

        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            // Here we identify columns using the ColumnUserID value that we ourselves passed to TableSetupColumn()
            // We could also choose to identify columns based on their index (sort_spec->ColumnIndex), which is simpler!
            const ImGuiTableColumnSortSpecs* sort_spec = &sortSpecs->Specs[n];
            __int64 delta = 0;
            switch (sort_spec->ColumnUserID)
            {
            case UITexturePreviewData_t::eColumnID::TPC_Index:      delta = (a.index - b.index);                                                            break;
            case UITexturePreviewData_t::eColumnID::TPC_Dimensions: delta = (static_cast<int>(a.width * a.height) - static_cast<int>(b.width * b.height));  break;
            case UITexturePreviewData_t::eColumnID::TPC_Position:   delta = (static_cast<int>(a.posX * a.posY) - static_cast<int>(b.posX * b.posY));        break;
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
void* PreviewUIImageAtlasAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    assertm(pakAsset, "Asset should be valid.");

    static bool txtrRegistered = []() {
        auto it = g_assetData.m_assetTypeBindings.find('rtxt');
        return it != g_assetData.m_assetTypeBindings.end();
    } ();

    if (!txtrRegistered)
    {
        ImGui::TextUnformatted("Txtr was not registered as an asset binding.");
        return nullptr;
    }

    UIImageAtlasAsset* const uiAsset = reinterpret_cast<UIImageAtlasAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    static float textureZoom = 1.0f;
    static int lastSelectedTexture = -1;
    static UITexturePreviewData_t selectedUIImage = { .index = -1 };
    static std::shared_ptr<CTexture> selectedUITexture = nullptr;
    static std::vector<UITexturePreviewData_t> previewTextures;

    // TODO: ui_font_atlas and ui_image_atlas share this lambda, should we try to make it a function?
    // TODO: parse and preview into render size
    auto CreateTextureForImage = [](UIImageAtlasAsset* const uiAsset, const UIAtlasImage* const uiImage) -> std::shared_ptr<CTexture>
    {
        assertm(uiAsset->rawTxtr, "Atlas texture wasn't created yet.");
        assertm(uiAsset->convertedTxtr, "Converted atlas texture wasn't created yet.");

        if (!uiAsset->rawTxtr || !uiAsset->convertedTxtr)
            return nullptr;

        // This will be the main texture.
        if (uiImage->width == uiAsset->width && uiImage->height == uiImage->height)
            return uiAsset->rawTxtr;

        // invalid image
        if (!uiImage->width || !uiImage->height)
            return nullptr;

        // Create texture / shader and get slice for image.
        std::shared_ptr<CTexture> txtrData = std::make_shared<CTexture>(nullptr, 0u, uiImage->width, uiImage->height, (IsSRGB(uiAsset->format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
        txtrData->CopySourceTextureSlice(uiAsset->convertedTxtr.get(), static_cast<size_t>(uiImage->posX), static_cast<size_t>(uiImage->posY), uiImage->width, uiImage->height, 0u, 0u);
        txtrData->CreateShaderResourceView(g_dxHandler->GetDevice());

        return txtrData;
    };

    const std::vector<UIAtlasImage>& imgArray = uiAsset->imageArray;
    static int imgArraySize = -1; // This cast is fine, we won't hit INT32_MAX.
    if (firstFrameForAsset) // Reset if new asset.
    {
        previewTextures.clear();
        previewTextures.resize(imgArray.size()); // account for base texture
        imgArraySize = static_cast<int>(imgArray.size());

        selectedUIImage.index = imgArraySize + 1;
        selectedUITexture.reset();

        int index = 0;
        for (auto& img : imgArray)
        {
            UITexturePreviewData_t& previewData = previewTextures.at(index);

            previewData.index = index;
            previewData.width = img.width;
            previewData.height = img.height;
            previewData.posX = img.posX;
            previewData.posY = img.posY;

            previewData.renderWidth = img.dimensionsWidth;
            previewData.renderHeight = img.dimensionsHeight;

            previewData.name = &img.path;

            index++;
        }

        selectedUIImage = selectedUIImage.index > imgArraySize ? previewTextures.back() : selectedUIImage; // if negative one, casting to unsign will mean it's larger than the size
    }

    assertm(selectedUIImage.index < imgArraySize, "Selected ui image is out of bounds for img array.");

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable
        | ImGuiTableFlags_Sortable | ImGuiTableFlags_SortMulti
        | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_NoBordersInBody
        | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;

    const ImVec2 outerSize = ImVec2(0.f, ImGui::GetTextLineHeightWithSpacing() * 12.f);

    ImGui::TextUnformatted(std::format("Atlas: {} (0x{:X})", pakAsset->GetAssetName().c_str(), pakAsset->data()->guid).c_str());

    if (ImGui::BeginTable("Image Table", UITexturePreviewData_t::eColumnID::_TPC_COUNT, tableFlags, outerSize))
    {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHide, 0.0f, UITexturePreviewData_t::eColumnID::TPC_Index);
        ImGui::TableSetupColumn("Dimensions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UITexturePreviewData_t::eColumnID::TPC_Dimensions);
        ImGui::TableSetupColumn("Positions", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UITexturePreviewData_t::eColumnID::TPC_Position);
        ImGui::TableSetupColumn("Render Size", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UITexturePreviewData_t::eColumnID::TPC_Dimensions);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_WidthFixed, 0.0f, UITexturePreviewData_t::eColumnID::TPC_Name);
        ImGui::TableSetupScrollFreeze(1, 1);

        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs(); // get the sorting settings from this table

        if (sortSpecs && (firstFrameForAsset || sortSpecs->SpecsDirty) && previewTextures.size() > 1)
        {
            std::sort(previewTextures.begin(), previewTextures.end(), UITexCompare_t(sortSpecs));
            sortSpecs->SpecsDirty = false;
        }

        ImGui::TableHeadersRow();

        for (int i = 0; i < imgArraySize; i++)
        {
            const UITexturePreviewData_t* item = &previewTextures.at(i);

            const bool isRowSelected = selectedUIImage == item || (firstFrameForAsset && item->index == lastSelectedTexture);

            ImGui::PushID(item->index); // id of this line ?

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 0.0f);

            if (ImGui::TableSetColumnIndex(UITexturePreviewData_t::eColumnID::TPC_Index))
            {
                if (ImGui::Selectable(std::to_string(item->index).c_str(), isRowSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap, ImVec2(0.f, 0.f)) || (firstFrameForAsset && isRowSelected))
                {
                    selectedUIImage = *item;
                    lastSelectedTexture = item->index;

                    selectedUITexture = CreateTextureForImage(uiAsset, &imgArray.at(selectedUIImage.index));
                }
            }

            if (ImGui::TableSetColumnIndex(UITexturePreviewData_t::eColumnID::TPC_Dimensions))
            {
                ImGui::Text("%i x %i", item->width, item->height);
            }

            if (ImGui::TableSetColumnIndex(UITexturePreviewData_t::eColumnID::TPC_Position))
            {
                ImGui::Text("(%i, %i)", item->posX, item->posY);
            }

            if (ImGui::TableSetColumnIndex(UITexturePreviewData_t::eColumnID::TPC_RenderSize))
            {
                ImGui::Text("%i x %i", item->renderWidth, item->renderHeight);
            }

            if (ImGui::TableSetColumnIndex(UITexturePreviewData_t::eColumnID::TPC_Name))
                ImGui::TextUnformatted(item->name->c_str());

            ImGui::PopID(); // no longer working on this id
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
        const UIAtlasImage* const uiImage = &imgArray[selectedUIImage.index];
        selectedUITexture = CreateTextureForImage(uiAsset, uiImage);
    }

    return nullptr;
}

enum eUIImageAtlasExportSetting
{
    PNG_AT, // PNG (Atlas)
    PNG_T,  // PNG (Textures)
    DDS_AT, // DDS (Atlas)
    DDS_T,  // DDS (Textures)
};

//static_assert(s_AssetTypePaths.count(PakAssetType_t::UIMG));
static const char* const s_PathPrefixUIMG = s_AssetTypePaths.find(AssetType_t::UIMG)->second;
bool ExportUIImageAtlasAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UIImageAtlasAsset* const uiAsset = reinterpret_cast<UIImageAtlasAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME); // 
    const std::filesystem::path atlasPath(asset->GetAssetName());

    // truncate paths?
    if (g_ExportSettings.exportPathsFull)
        exportPath.append(atlasPath.parent_path().string());
    else
        exportPath.append(s_PathPrefixUIMG);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(atlasPath.stem().string());

    switch (setting)
    {
    case eUIImageAtlasExportSetting::PNG_AT:
    case eUIImageAtlasExportSetting::DDS_AT:
    {
        assertm(uiAsset->rawTxtr, "Atlas was not valid.");

        switch (setting)
        {
        case eUIImageAtlasExportSetting::PNG_AT:
        {
            // Add png for file extension.
            exportPath.replace_extension("png");

            if (!uiAsset->rawTxtr->ExportAsPng(exportPath))
                return false;

            return true;
        }
        case eUIImageAtlasExportSetting::DDS_AT:
        {
            // Add dds for file extension.
            exportPath.replace_extension("dds");

            if (!uiAsset->rawTxtr->ExportAsDds(exportPath))
                return false;

            return true;
        }
        }

        return false;
    }
    case eUIImageAtlasExportSetting::DDS_T:
    {
        assertm(uiAsset->convertedTxtr, "Converted atlas was not valid.");
        for (auto it = uiAsset->imageArray.rbegin() + 1; it != uiAsset->imageArray.rend(); ++it) // We skip the last element, this is the main atlas texture.
        {
            // [amos] if either of them are null, the DirectX::CopyRectangle call will crash.
            // the preview function above logs it as N/A so I think we should just skip them.
            if (!it->width || !it->height)
            {
                printf("skipping image %x in altas %llx (invalid texture)...\n", it->pathHash, asset->GetAssetGUID());

                continue;
            }

            std::filesystem::path currentPath = exportPath;
            std::filesystem::path itemPath = it->path;

            // Setup paths.
            if (currentPath.has_parent_path())
                currentPath.append(itemPath.parent_path().string());

            // [amos] Need to remove trailing slash because otherwise std::filesystem::create_directories
            // will fail with the error message "The operation completed successfully".
            // See https://developercommunity.visualstudio.com/t/stdfilesystemcreate-directories-returns-false-if-p/278829
            currentPath = currentPath.parent_path();

            if (!CreateDirectories(currentPath))
            {
                assertm(false, "Failed to create export type directory");
                return false;
            }
            currentPath.concat(std::format("\\{}.dds", itemPath.filename().string()));

            printf("Format = %d\n", (int)uiAsset->format);

            std::unique_ptr<CTexture> sliceData = std::make_unique<CTexture>(nullptr, 0u, it->width, it->height, (IsSRGB(uiAsset->format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
            sliceData->CopySourceTextureSlice(uiAsset->convertedTxtr.get(), static_cast<size_t>(it->posX), static_cast<size_t>(it->posY), it->width, it->height, 0u, 0u);
            sliceData->ExportAsDds(currentPath);
        }

        return true;
    }
    case eUIImageAtlasExportSetting::PNG_T:
    {
        assertm(uiAsset->convertedTxtr, "Converted atlas was not valid.");
        for (auto it = uiAsset->imageArray.rbegin() + 1; it != uiAsset->imageArray.rend(); ++it) // We skip the last element, this is the main atlas texture.
        {
            // [amos] if either of them are null, the DirectX::CopyRectangle call will crash.
            // the preview function above logs it as N/A so I think we should just skip them.
            if (!it->width || !it->height)
                continue;

            std::filesystem::path currentPath = exportPath;
            std::filesystem::path itemPath = it->path;

            // Setup paths.
            if (currentPath.has_parent_path())
                currentPath.append(itemPath.parent_path().string());

            // [amos] Need to remove trailing slash because otherwise std::filesystem::create_directories
            // will fail with the error message "The operation completed successfully".
            // See https://developercommunity.visualstudio.com/t/stdfilesystemcreate-directories-returns-false-if-p/278829
            currentPath = currentPath.parent_path();

            if (!CreateDirectories(currentPath))
            {
                assertm(false, "Failed to create export type directory");
                return false;
            }
            currentPath.concat(std::format("\\{}.png", itemPath.filename().string()));

            std::unique_ptr<CTexture> sliceData = std::make_unique<CTexture>(nullptr, 0u, it->width, it->height, (IsSRGB(uiAsset->format) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM), 1u, 1u);
            sliceData->CopySourceTextureSlice(uiAsset->convertedTxtr.get(), static_cast<size_t>(it->posX), static_cast<size_t>(it->posY), it->width, it->height, 0u, 0u);
            sliceData->ExportAsPng(currentPath);
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

void InitUIImageAtlasAssetType()
{
    static const char* settings[] = { "PNG (Atlas)", "PNG (Textures)", "DDS (Atlas)", "DDS (Textures)" };
    AssetTypeBinding_t type =
    {
        .type = 'gmiu',
        .headerAlignment = 8,
        .loadFunc = LoadUIImageAtlasAsset,
        .postLoadFunc = PostLoadUIImageAtlasAsset,
        .previewFunc = PreviewUIImageAtlasAsset,
        .e = { ExportUIImageAtlasAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}