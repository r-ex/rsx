#include <pch.h>
#include <game/rtech/assets/ui_image.h>

#include <core/render/dx.h>
#include <thirdparty/imgui/imgui.h>

void LoadUIImageAsset(CAssetContainer* const pak, CAsset* const asset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    if (pakAsset->version() < 2) // Does 1 even exist?
    {
        assertm(false, "Non handled uiia version.");
        return;
    }

    const UIImageAssetHeader_v2_t* const hdrV2 = reinterpret_cast<UIImageAssetHeader_v2_t*>(pakAsset->header());
    const UIImageAssetData_v2_t*   const cpuV2 = reinterpret_cast<UIImageAssetData_v2_t*>(pakAsset->cpu());
    if (!cpuV2)
    {
        assertm(false, "uiia should always have cpu data, otherwise no image is attached to it.");
        return;
    }

    UIImageAsset* const uiAsset = new UIImageAsset(hdrV2, cpuV2);
    if (uiAsset->name)
    {
        std::string name = uiAsset->name;
        if (!name.starts_with("ui_image/"))
            name = "ui_image/" + name;

        if (!name.ends_with(".rpak"))
            name += ".rpak";

        pakAsset->SetAssetName(name, true);
    }
    //else
    //{
    //    // first give a base name and then try and fetch from the cache if possible
    //    pakAsset->SetAssetName(std::format("ui_image/0x{:X}.rpak", pakAsset->data()->guid));
    //}

    // uiia can't be in opt starpak, LoadUIIAAsset calls this to get the streamedOffset // hdr->streamedOffset = *a4 >> 12, a4 is an array which consists of starpak, opstarpak.
    assertm(pakAsset->getStarPakStreamEntry(true).offset == 0u, "Opt starpak entry for uiia??");

    // Let's check if we have a starpak to get data from and fix up our members.
    const StarPak_t* const streamedEntry = static_cast<CPakFile*>(pak)->getStarPak(pakAsset->starpakIdx(), false);
    if (streamedEntry)
    {
        // Explicit rant about this member below, its ALWAYS UINT32_MAX, it's set in the load function and then packed?? Then in the streaming routine it's unpacked, why??
        uiAsset->streamedOffset = pakAsset->getStarPakStreamEntry(false).offset;
        uiAsset->streamedSize = static_cast<uint64_t>(static_cast<uint32_t>(uiAsset->streamedSize) << 12u); // Data is packed and needs to be aligned based on the starpak pages (1 << 12) = 4096.
        uiAsset->shouldStream = true; // Incase we don't have a starpak loaded, this will be false and we will always fall back to non-streamed data.
    }
    /*
    * In LoadUIIAsset
        v11 = 2;
        if ( (unsigned __int16)(*(_WORD *)(a2 + 36) + (~(unsigned __int8)(v8 >> 5) & 2)) > 2u )
        v9 = *(_WORD *)(a2 + 36) + (~(unsigned __int8)(v8 >> 5) & 2);
        v12 = *(_WORD *)(a2 + 38) + (~(v8 >> 6) & 2);
        v13 = ((unsigned int)v9 + 29) / 31;
        if ( v12 > 2u )
            v11 = v12;
        v14 = ((unsigned int)v11 + 29) / 31;
    */
    const uint16_t unkWidthOffset  = (~((*reinterpret_cast<uint16_t*>(&uiAsset->imgFlags)) >> 5) & 2);
    const uint16_t unkHeightOffset = (~((*reinterpret_cast<uint16_t*>(&uiAsset->imgFlags)) >> 6) & 2);

    // Wild guesses since we calculating blocks here.
    constexpr const uint32_t pixelOffset = 29;
    constexpr const uint32_t blockPixelSize = 32;
    constexpr const uint32_t blockSize = 31;
    constexpr const float rcpBlockSize = 1.f / blockSize; // Used so that the division is done at compile time instead of runtime.

    // Calculate how many blocks we have per resolution.
    // Most of this is based on the pseudo logic seen above in LoadUIIAsset.
    const uint16_t highResWidth  = uiAsset->resData[eUIImageTileType::TYPE_HQ].width;
    const uint16_t highResHeight = uiAsset->resData[eUIImageTileType::TYPE_HQ].height;
    const uint32_t highResWidthBlocks = static_cast<uint32_t>((highResWidth + unkWidthOffset + pixelOffset) * rcpBlockSize);
    const uint32_t highResHeightBlocks = static_cast<uint32_t>((highResHeight + unkHeightOffset + pixelOffset) * rcpBlockSize);

    uiAsset->resData[eUIImageTileType::TYPE_HQ].widthBlocks  = highResWidthBlocks;
    uiAsset->resData[eUIImageTileType::TYPE_HQ].heightBlocks = highResHeightBlocks;
    uiAsset->resData[eUIImageTileType::TYPE_HQ].totalTiles   = highResWidthBlocks * highResHeightBlocks;
    
    // We need to keep the actual since per resolution, we need the shifted size for tiling/unswizzling.
    uiAsset->resData[eUIImageTileType::TYPE_HQ].shiftedWidth  = static_cast<uint16_t>(highResWidthBlocks * blockPixelSize);
    uiAsset->resData[eUIImageTileType::TYPE_HQ].shiftedHeight = static_cast<uint16_t>(highResHeightBlocks * blockPixelSize);

    const uint16_t lowResWidth  = uiAsset->resData[eUIImageTileType::TYPE_LQ].width;
    const uint16_t lowResHeight = uiAsset->resData[eUIImageTileType::TYPE_LQ].height;
    const uint32_t lowResWidthBlocks = static_cast<uint32_t>((lowResWidth + unkWidthOffset + pixelOffset) * rcpBlockSize);
    const uint32_t lowResHeightBlocks = static_cast<uint32_t>((lowResHeight + unkHeightOffset + pixelOffset) * rcpBlockSize);

    uiAsset->resData[eUIImageTileType::TYPE_LQ].widthBlocks  = lowResWidthBlocks;
    uiAsset->resData[eUIImageTileType::TYPE_LQ].heightBlocks = lowResHeightBlocks;
    uiAsset->resData[eUIImageTileType::TYPE_LQ].totalTiles   = lowResWidthBlocks * lowResHeightBlocks;

    // We need to keep the actual since per resolution, we need the shifted size for tiling/unswizzling.
    uiAsset->resData[eUIImageTileType::TYPE_LQ].shiftedWidth  = static_cast<uint16_t>(lowResWidthBlocks * blockPixelSize);
    uiAsset->resData[eUIImageTileType::TYPE_LQ].shiftedHeight = static_cast<uint16_t>(lowResHeightBlocks * blockPixelSize);

    /*
        Here is how the game actually handles the 'tiles'.
        resType is flags & 3 which are our bitFields and determine which 'tables' exist.
        I decided to just handle both quality type of tiles since it's easier and I do not fully understand the shifting down there.

        if (resType == eUIImageResType::LQ_AND_HQ_RES)
        {
            uint32_t* data = reinterpret_cast<uint32_t*>(uiAsset->data);
            for (uint32_t i = 0; i < totalLowResBlocks; ++i)
            {
                const uint32_t dat = *data;
                if ((dat & 0xC0000000) == 0x40000000)
                {
                    size_t idx = static_cast<size_t>((dat - 0x40000000) >> 24);
                    uiAsset->totalBlocks[idx] += 1;
                }
                ++data;
            }
        }
        else
        {
            uiAsset->totalBlocks[resType - 1] = totalLowResBlocks;
            uiAsset->totalBlocks[2 - resType] = 0;
        }
    */

    auto GetTilePoints = [](const CPakAsset* const asset, UIImageAsset* const uiAsset, const bool streamedData) -> std::unique_ptr<UIImageTile_t[]>
    {
        const uint32_t totalTiles = uiAsset->resData[streamedData ? eUIImageTileType::TYPE_HQ : eUIImageTileType::TYPE_LQ].totalTiles;
        std::unique_ptr<UIImageTile_t[]> tilePoints = std::make_unique<UIImageTile_t[]>(totalTiles);

        if (!streamedData && (!uiAsset->imgFlags.hasLowTableBc1 || !uiAsset->imgFlags.hasLowTableBc7))  // If we have any low res tables present for BC1 or BC7 fall through.
        {
            for (uint32_t i = 0; i < totalTiles; i++)
            {
                tilePoints[i].opcode = uiAsset->imgFlags.hasLowTableBc7      ? BC7_FLAG      : BC1_FLAG; // BC7 or BC1?
                tilePoints[i].offset = i * (uiAsset->imgFlags.hasLowTableBc7 ? BC7_TILE_SIZE : BC1_TILE_SIZE); // BC7 or BC1?
            }
        }
        else if (streamedData && (!uiAsset->imgFlags.hasHighTableBc1 || !uiAsset->imgFlags.hasHighTableBc7)) // If we have any high res tables present for BC1 or BC7 fall through.
        {
            for (uint32_t i = 0; i < totalTiles; i++)
            {
                tilePoints[i].opcode = uiAsset->imgFlags.hasHighTableBc7      ? BC7_FLAG      : BC1_FLAG; // BC7 or BC1?
                tilePoints[i].offset = i * (uiAsset->imgFlags.hasHighTableBc7 ? BC7_TILE_SIZE : BC1_TILE_SIZE); // BC7 or BC1?
            }
        }
        else
        {
            const size_t tileTableSize = totalTiles * sizeof(UIImageTile_t);
            if (streamedData)
            {
                // We need to grab the whole buffer due to compression else we explode. Also it can't be in opt!!!!
                std::unique_ptr<char[]> tableData = asset->getStarPakData(uiAsset->streamedOffset, uiAsset->streamedSize, false);
                if (uiAsset->compType > eCompressionType::NONE)
                {
                    uint64_t bufSize = uiAsset->streamedSize;
                    tableData = RTech::DecompressStreamedBuffer(std::move(tableData), bufSize, uiAsset->compType);
                }

                assertm(tableData, "Failed to get starpak data?");
                memcpy_s(tilePoints.get(), tileTableSize, tableData.get(), tileTableSize);
            }
            else
            {
                assertm(uiAsset->data, "No rpak data for table?");
                memcpy_s(tilePoints.get(), tileTableSize, uiAsset->data, tileTableSize);
            }

            uiAsset->resData[streamedData ? eUIImageTileType::TYPE_HQ : eUIImageTileType::TYPE_LQ].copiedTable = true;
        }

        return std::move(tilePoints);
    };

    auto GetNumOfBcBlocks = [](UIImageAsset* const uiAsset, const eUIImageTileType tileType) -> void
    {
        // make sure we're using the correct tile count, if we don't have starpak data, and try to index into tile points with the incorrect 
        // count for what we've processed, there will be issues.
        UIImageAsset::QualityData* const resData = &uiAsset->resData[tileType];
        const uint32_t totalTiles = uiAsset->shouldStream ? resData->totalTiles : uiAsset->resData[eUIImageTileType::TYPE_LQ].totalTiles;

        for (uint32_t i = 0; i < totalTiles; i++)
        {
            switch (resData->tilePoints[i].opcode)
            {
            case 0x40: // BC1 tile
                resData->numBc1Tiles++;
                break;
            case 0x41: // BC7 tile
                resData->numBc7Tiles++;
                break;
            case 0xC0: // This opcode requires us to copy an existing opcode.
                resData->tilePoints[i] = resData->tilePoints[resData->tilePoints[i].offset];
                break;
            default:
                break;
            }
        }
    };

    // The image data is saved as 'blocks' aka tiles.
    // There is a table sometimes present which tells us how to interpret the data, as BC1 or as BC7 tiles.
    uiAsset->resData[eUIImageTileType::TYPE_HQ].tilePoints = GetTilePoints(pakAsset, uiAsset, uiAsset->shouldStream); // We assume we should stream HQ.
    GetNumOfBcBlocks(uiAsset, eUIImageTileType::TYPE_HQ);

    uiAsset->resData[eUIImageTileType::TYPE_LQ].tilePoints = GetTilePoints(pakAsset, uiAsset, false);
    GetNumOfBcBlocks(uiAsset, eUIImageTileType::TYPE_LQ);

    pakAsset->setExtraData(uiAsset);
}

void PostLoadUIImageAsset(CAssetContainer* container, CAsset* asset)
{
    UNUSED(container);

    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    assertm(pakAsset->extraData(), "extra data should be valid");
    UIImageAsset* const uiAsset = reinterpret_cast<UIImageAsset*>(pakAsset->extraData());

    if (!uiAsset->name)
        pakAsset->SetAssetNameFromCache();
}

// Or swizzle work..
std::unique_ptr<CTexture> DoTilingWork(std::unique_ptr<CTexture> texture, std::unique_ptr<char[]> const& buf, const size_t bufSize, const uint32_t widthBlocks, const uint32_t heightBlocks, const uint32_t bpp2x)
{    
    uint64_t tileOffset = 0ull;
    for (uint32_t y = 0u; y < heightBlocks; y++)
    {
        for (uint32_t x = 0u; x < widthBlocks; x++)
        {
            uint32_t mx = x;
            uint32_t my = y;

            // --- 2 ---
            int power = 2;
            int b_2 = (mx / 2 + my * (widthBlocks / power)) % (2 * (widthBlocks / power)) /
                2 + (widthBlocks / power) * ((mx / 2 + my * (widthBlocks / power)) % (2 * (widthBlocks / power)) % 2) +
                2 * (widthBlocks / power) * ((mx / 2 + my * (widthBlocks / power)) / (2 * (widthBlocks / power)));

            int c_2 = mx % 2 + 2 * (b_2 % (widthBlocks / power));
            mx = b_2 / (widthBlocks / power);
            my = c_2 / 4;
            c_2 %= 4;

            // --- 4 ---
            power = 4;
            int b_4 = (my + mx / 2 * (widthBlocks / power)) % (2 * (widthBlocks / power)) /
                2 + (widthBlocks / power) * ((my + mx / 2 * (widthBlocks / power)) % (2 * (widthBlocks / power)) % 2) +
                2 * (widthBlocks / power) * ((my + mx / 2 * (widthBlocks / power)) / (2 * (widthBlocks / power)));

            int c_4 = mx % 2 + 2 * (b_4 / (widthBlocks / power));
            mx = b_4 / (widthBlocks / power);
            my = c_4 / 4;
            c_4 %= 4;

            // --- 8 ---
            power = 8;
            int b_8 = ((c_2 + 4 * (b_4 % (widthBlocks / 4))) / 8 + my * (widthBlocks / power)) % (2 * (widthBlocks / power)) /
                2 + (widthBlocks / power) * (((c_2 + 4 * (b_4 % (widthBlocks / 4))) / 8 + my * (widthBlocks / power)) % (2 * (widthBlocks / power)) % 2) +
                2 * (widthBlocks / power) * (((c_2 + 4 * (b_4 % (widthBlocks / 4))) / 8 + my * (widthBlocks / power)) / (2 * (widthBlocks / power)));

            my = (c_4 + 4 * ((int)b_8 / (widthBlocks / power)));
            mx = (c_2 + 4 * (b_4 % (widthBlocks / 4))) % 8 + 8 * (uint32_t)(b_8 % (widthBlocks / power));

            const uint64_t destination = static_cast<uint64_t>(bpp2x * (my * widthBlocks + mx));
            assertm(destination < texture->GetSlicePitch(), "destination greater than our texture size?");

            assertm(tileOffset < bufSize, "offset greater than our buf?");
            memcpy_s(texture->GetPixels() + destination, texture->GetSlicePitch() - destination, buf.get() + tileOffset, bpp2x); // Same as above, account for max size in dest buf based on copy location.
            tileOffset += bpp2x;
        }
    }

    return std::move(texture);
}

std::unique_ptr<CTexture> CreateBC1TextureForUIImageAsset(CPakAsset* const asset, UIImageAsset* const uiAsset, const UIImageAsset::QualityData* const resData, const bool isStreamed, const bool doTiling)
{
    assertm(resData->numBc1Tiles > 0, "called CreateBC1Texture with no BC1 tiles.");

    const size_t bc1BufSize = static_cast<size_t>(resData->totalTiles * BC1_TILE_SIZE);
    std::unique_ptr<char[]> bc1Buf = std::make_unique<char[]>(bc1BufSize);

    std::unique_ptr<char[]> streamedData;
    const char* bc1Data = nullptr;
    if (isStreamed)
    {
        streamedData = asset->getStarPakData(uiAsset->streamedOffset, uiAsset->streamedSize, false);
        assertm(streamedData, "invalid table data even though streamed.");
        if (uiAsset->compType > eCompressionType::NONE)
        {
            uint64_t bufSize = uiAsset->streamedSize;
            streamedData = RTech::DecompressStreamedBuffer(std::move(streamedData), bufSize, uiAsset->compType);
        }
        assertm(streamedData, "invalid table data after decode.");

        bc1Data = streamedData.get();
    }
    else
    {
        bc1Data = static_cast<char*>(uiAsset->data);
    }

    for (uint32_t y = 0; y < resData->heightBlocks; ++y)
    {
        for (uint32_t x = 0; x < resData->widthBlocks; ++x)
        {
            // Grab the current tile and the offset to it.
            const UIImageTile_t* const tile = &resData->tilePoints[static_cast<size_t>(x + (y * static_cast<uint32_t>(resData->widthBlocks)))];
            const uint32_t tileOffset = (x * BC1_TILE_SIZE) + ((y * resData->widthBlocks) * BC1_TILE_SIZE);

            if (tile->opcode != BC1_FLAG) // Transparent tile, replace with our own.
            {
                memcpy_s(bc1Buf.get() + tileOffset, bc1BufSize, UIImageTileBc1, sizeof(UIImageTileBc1));
                continue;
            }

            // We account for the tileOffset in the destination size when copying.
            // We copy over the whole tile but it's not at it's proper destination yet, is it swizzling? I have no clue, I feel like it's just tiling.
            memcpy_s(bc1Buf.get() + tileOffset, bc1BufSize - tileOffset, bc1Data + tile->offset, BC1_TILE_SIZE);
        }
    }

    std::unique_ptr<CTexture> bc1Texture = std::make_unique<CTexture>(nullptr, 0ull, resData->shiftedWidth, resData->shiftedHeight, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM, 1ull, 1ull);
    assertm(bc1Texture, "BC1 texture creation failed.");

    if (doTiling)
    {
        const uint32_t widthTiles = resData->shiftedWidth / 4u;
        const uint32_t heightTiles = resData->shiftedHeight / 4u;
        constexpr uint32_t bc1Bpp2x = 4u * 2u;

        return std::move(DoTilingWork(std::move(bc1Texture), bc1Buf, bc1BufSize, widthTiles, heightTiles, bc1Bpp2x));
    }
    else
    {
        assertm(bc1BufSize <= bc1Texture->GetSlicePitch(), "bc1Buf greater than our texture buf?");
        memcpy_s(bc1Texture->GetPixels(), bc1Texture->GetSlicePitch(), bc1Buf.get(), bc1BufSize);

        return std::move(bc1Texture);
    }

    unreachable();
}

std::unique_ptr<CTexture> CreateBC7TextureForUIImageAsset(CPakAsset* const asset, UIImageAsset* const uiAsset, const UIImageAsset::QualityData* const resData, const bool isStreamed, const bool doTiling)
{
    assertm(resData->numBc7Tiles > 0, "called CreateBC7Texture with no BC7 tiles.");

    const size_t bc7BufSize = static_cast<size_t>(resData->totalTiles * BC7_TILE_SIZE);
    std::unique_ptr<char[]> bc7Buf = std::make_unique<char[]>(bc7BufSize);

    std::unique_ptr<char[]> streamedData;
    const char* bc7Data = nullptr;
    if (isStreamed)
    {
        streamedData = asset->getStarPakData(uiAsset->streamedOffset, uiAsset->streamedSize, false);
        assertm(streamedData, "invalid table data even though streamed.");
        if (uiAsset->compType > eCompressionType::NONE)
        {
            uint64_t bufSize = uiAsset->streamedSize;
            streamedData = RTech::DecompressStreamedBuffer(std::move(streamedData), bufSize, uiAsset->compType);
        }
        assertm(streamedData, "invalid table data after decode.");

        bc7Data = streamedData.get();
    }
    else
    {
        bc7Data = static_cast<char*>(uiAsset->data);
    }

    for (uint32_t y = 0; y < resData->heightBlocks; ++y)
    {
        for (uint32_t x = 0; x < resData->widthBlocks; ++x)
        {
            // Grab the current tile and the offset to it.
            const UIImageTile_t* const tile = &resData->tilePoints[static_cast<size_t>(x + (y * static_cast<uint32_t>(resData->widthBlocks)))];
            const uint32_t tileOffset = (x * BC7_TILE_SIZE) + ((y * resData->widthBlocks) * BC7_TILE_SIZE);

            if (tile->opcode != BC7_FLAG) // Transparent tile, replace with our own.
            {
                memcpy_s(bc7Buf.get() + tileOffset, bc7BufSize, UIImageTileBc7, sizeof(UIImageTileBc7));
                continue;
            }

            // We account for the tileOffset in the destination size when copying.
            // We copy over the whole tile but it's not at it's proper destination yet, is it swizzling? I have no clue, I feel like it's just tiling.
            memcpy_s(bc7Buf.get() + tileOffset, bc7BufSize - tileOffset, bc7Data + tile->offset, BC7_TILE_SIZE);
        }
    }

    std::unique_ptr<CTexture> bc7Texture = std::make_unique<CTexture>(nullptr, 0ull, resData->shiftedWidth, resData->shiftedHeight, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM, 1ull, 1ull);
    assertm(bc7Texture, "BC7 texture creation failed.");

    if (doTiling)
    {
        const uint32_t widthTiles = resData->shiftedWidth / 4u;
        const uint32_t heightTiles = resData->shiftedHeight / 4u;
        constexpr uint32_t bc7Bpp2x = 8u * 2u;

        return std::move(DoTilingWork(std::move(bc7Texture), bc7Buf, bc7BufSize, widthTiles, heightTiles, bc7Bpp2x));
    }
    else
    {
        assertm(bc7Texture->GetSlicePitch() < bc7BufSize, "bc7Buf greater than our texture buf?");
        memcpy_s(bc7Texture->GetPixels(), bc7Texture->GetSlicePitch(), bc7Buf.get(), bc7BufSize);

        return std::move(bc7Texture);
    }

    unreachable();
}

extern CDXParentHandler* g_dxHandler;
std::shared_ptr<CTexture> CreateTextureForImage(CPakAsset* const asset, UIImageAsset* const uiAsset, const UIImageAsset::QualityData* const resData, const bool doStreaming, const bool doTiling)
{
    // We want max 2 threads but clamp to 1 incase we get an invalid count.
    CParallelTask tasks(std::clamp(CThread::GetConCurrentThreads(), 1u, 2u));

    std::unique_ptr<CTexture> bc1Texture = nullptr;
    std::unique_ptr<CTexture> bc7Texture = nullptr;
    if (resData->numBc1Tiles)
    {
        tasks.addTask([resData, &bc1Texture, asset, uiAsset, doStreaming, doTiling]
        {
            bc1Texture = CreateBC1TextureForUIImageAsset(asset, uiAsset, resData, doStreaming, doTiling);
            bc1Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
        }, 1u);
    }
 
    if (resData->numBc7Tiles)
    {
        tasks.addTask([resData, &bc7Texture, asset, uiAsset, doStreaming, doTiling]
        {
            bc7Texture = CreateBC7TextureForUIImageAsset(asset, uiAsset, resData, doStreaming, doTiling);
            bc7Texture->ConvertToFormat(DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM);
        }, 1u);
    }

    tasks.execute();
    tasks.wait();

    if (!bc1Texture && !bc7Texture)
    {
        //Log("ERROR: failed to export ui image asset %llX. image had no tiles.\n", asset->data()->guid);
        //assertm(false, "no bc1 and no bc7??????????");
        return nullptr;
    }

    std::shared_ptr<CTexture> uiShiftedTexture = nullptr;
    if (bc1Texture)
    {
        uiShiftedTexture = std::move(bc1Texture);
        if (bc7Texture)
        {
            for (uint32_t y = 0; y < resData->heightBlocks; y++)
            {
                for (uint32_t x = 0; x < resData->widthBlocks; x++)
                {
                    const UIImageTile_t* const tile = &resData->tilePoints[static_cast<size_t>(x + (y * static_cast<uint32_t>(resData->widthBlocks)))];

                    if (tile->opcode == BC7_FLAG)
                    {
                        uiShiftedTexture->CopySourceTextureSlice(bc7Texture.get(), (x * 32ull), (y * 32ull), 32ull, 32ull, (x * 32ull), (y * 32ull));
                    }
                }
            }
        }
    }
    else
    {
        uiShiftedTexture = std::move(bc7Texture);
    }
    assertm(uiShiftedTexture, "shifted texture is nullptr.");

    std::shared_ptr<CTexture> uiTexture = std::make_shared<CTexture>(nullptr, 0ull, resData->width, resData->height, DXGI_FORMAT_R8G8B8A8_UNORM, 1ull, 1ull);
    assertm(uiTexture, "uiTexture is nullptr.");

    for (uint32_t y = 0; y < resData->heightBlocks; y++)
    {
        const uint32_t sizeY = y * 31u + 30u < resData->height ? 31u : resData->height - y * 31u;
        for (uint32_t x = 0; x < resData->widthBlocks; x++)
        {
            const uint32_t sizeX = x * 31u + 30u < resData->width ? 31u : resData->width - x * 31u;
            uiTexture->CopySourceTextureSlice(uiShiftedTexture.get(), (x * 32ull), (y * 32ull), sizeX, sizeY, (x * 31ull), (y * 31ull));
        }
    }

    return std::move(uiTexture);
};

#undef max
void* PreviewUIImageAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UIImageAsset* const uiAsset = reinterpret_cast<UIImageAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    static float textureZoom = 1.0f;
    static eUIImageTileType selectedUIImage = eUIImageTileType::TYPE_HQ;
    static std::shared_ptr<CTexture> selectedUITexture = nullptr;

    // Reset if new asset.
    if (firstFrameForAsset)
    {
        selectedUIImage = eUIImageTileType::TYPE_HQ;
        selectedUITexture.reset();
    }

    constexpr const char* const resNames[] =
    {
        "Low Quality",
        "High Quality"
    };

    assertm(selectedUIImage < eUIImageTileType::TYPE_COUNT, "Selected ui image is out of bounds for tile types.");
    if (ImGui::BeginTable("Image Table", 2, ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        ImGuiStyle& style = ImGui::GetStyle();
        if (ImGui::BeginListBox("##List", ImVec2(-1, ImGui::GetTextLineHeightWithSpacing() * 7.25f + style.FramePadding.y * 4.25f)))
        {
            for (uint8_t i = 0; i < eUIImageTileType::TYPE_COUNT; ++i)
            {
                const UIImageAsset::QualityData* const resData = &uiAsset->resData[i];

                const std::string imgDescriptor = resNames[i];
                const bool isSelectedUIImage = selectedUIImage == i;
                if (ImGui::Selectable(imgDescriptor.c_str(), isSelectedUIImage))
                {
                    selectedUIImage = static_cast<eUIImageTileType>(i);
                    textureZoom = 1.0f;

                    // in the future, we should probably load LQ only if streamed data is not present ? (since HQ is using LQ data at that point)
                    const bool canStream = (uiAsset->shouldStream && selectedUIImage == eUIImageTileType::TYPE_HQ);
                    selectedUITexture = CreateTextureForImage(pakAsset, uiAsset, resData, canStream, true);
                    if (selectedUITexture)
                        selectedUITexture->CreateShaderResourceView(g_dxHandler->GetDevice());
                }
            }
            ImGui::EndListBox();
        }

        ImGui::TableNextColumn();

        if (selectedUIImage != -1)
        {
            if (!uiAsset->shouldStream && selectedUIImage == eUIImageTileType::TYPE_HQ)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.f, 0.f, 1.f));
                ImGui::TextUnformatted("Not Loaded");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 1.f, 0.f, 1.f));
                ImGui::TextUnformatted("Loaded");
                ImGui::PopStyleColor();
            }

            ImGui::Separator();

            const UIImageAsset::QualityData* const resData = &uiAsset->resData[selectedUIImage];
            ImGui::Text("Width: %hu", uiAsset->width);
            ImGui::Text("Height: %hu", uiAsset->height);
            ImGui::Separator();
            ImGui::Text("Shifted Width: %hu", resData->shiftedWidth);
            ImGui::Text("Shifted Height: %hu", resData->shiftedHeight);
            ImGui::Separator();
            ImGui::Text("Actual Width: %hu", resData->width);
            ImGui::Text("Actual Height: %hu", resData->height);
            ImGui::Separator();
            ImGui::Text("Num BC1 Tiles: %u", resData->numBc1Tiles);
            ImGui::Text("Num BC7 Tiles: %u", resData->numBc7Tiles);
        }

        ImGui::EndTable();
    }

    const CTexture* const selectedUITxtr = selectedUITexture.get();
    if (selectedUITxtr)
    {
        const float aspectRatio = static_cast<float>(selectedUITxtr->GetWidth()) / selectedUITxtr->GetHeight();

        float imageHeight = std::max(std::clamp(static_cast<float>(selectedUITxtr->GetHeight()), 0.f, std::max(ImGui::GetContentRegionAvail().y, 1.f)) - 2.5f, 4.f);
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
        if (ImGui::BeginChild("Image Preview", ImVec2(0.f, 0.f), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const bool previewHovering = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            ImGui::Image(selectedUITxtr->GetSRV(), ImVec2(imageWidth, imageHeight));
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
        const UIImageAsset::QualityData* const resData = &uiAsset->resData[selectedUIImage];
        const bool canStream = (uiAsset->shouldStream && selectedUIImage == eUIImageTileType::TYPE_HQ);
        selectedUITexture = CreateTextureForImage(pakAsset, uiAsset, resData, canStream, true);
        if(selectedUITexture)
            selectedUITexture->CreateShaderResourceView(g_dxHandler->GetDevice());
        else
            ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Failed to preview selected UI Image. This asset does not contain any valid image data.\n");
    }

    return nullptr;
}

enum eUIImageExportSetting
{
    PNG_HQ, // PNG (High Quality)
    PNG_LQ, // PNG (Low Quality)
    DDS_HQ, // DDS (High Quality)
    DDS_LQ, // DDS (Low Quality)
};

static bool ExportPngUIImageAsset(CPakAsset* const asset, UIImageAsset* const uiAsset, std::filesystem::path& exportPath, const int setting)
{
    exportPath.replace_extension("png");

    switch (setting)
    {
    case eUIImageExportSetting::PNG_HQ:
    {
        std::shared_ptr<CTexture> uiTexture = CreateTextureForImage(asset, uiAsset, &uiAsset->resData[eUIImageTileType::TYPE_HQ], uiAsset->shouldStream, true);

        if (uiTexture)
            return uiTexture->ExportAsPng(exportPath);
        else
        {
            Log("Failed to export ui image asset '%s'. Received nullptr for extracted texture.\n", asset->GetAssetName().c_str());
            return false;
        }
    }
    case eUIImageExportSetting::PNG_LQ:
    {
        std::shared_ptr<CTexture> uiTexture = CreateTextureForImage(asset, uiAsset, &uiAsset->resData[eUIImageTileType::TYPE_LQ], false, true);
        
        if (uiTexture)
            return uiTexture->ExportAsPng(exportPath);
        else
        {
            Log("Failed to export ui image asset '%s'. Received nullptr for extracted texture.\n", asset->GetAssetName().c_str());
            return false;
        }
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

static bool ExportDdsUIImageAsset(CPakAsset* const asset, UIImageAsset* const uiAsset, std::filesystem::path& exportPath, const int setting)
{
    exportPath.replace_extension("dds");

    switch (setting)
    {
    case eUIImageExportSetting::DDS_HQ:
    {
        std::shared_ptr<CTexture> uiTexture = CreateTextureForImage(asset, uiAsset, &uiAsset->resData[eUIImageTileType::TYPE_HQ], uiAsset->shouldStream, true);

        if (uiTexture)
            return uiTexture->ExportAsDds(exportPath);
        else
        {
            Log("Failed to export ui image asset '%s'. Received nullptr for extracted texture.\n", asset->GetAssetName().c_str());
            return false;
        }
    }
    case eUIImageExportSetting::DDS_LQ:
    {
        std::shared_ptr<CTexture> uiTexture = CreateTextureForImage(asset, uiAsset, &uiAsset->resData[eUIImageTileType::TYPE_LQ], false, true);

        if (uiTexture)
            return uiTexture->ExportAsDds(exportPath);
        else
        {
            Log("Failed to export ui image asset '%s'. Received nullptr for extracted texture.\n", asset->GetAssetName().c_str());
            return false;
        }
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

bool ExportUIImageAsset(CAsset* const asset, const int setting)
{
    CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);
    UIImageAsset* const uiAsset = reinterpret_cast<UIImageAsset*>(pakAsset->extraData());
    assertm(uiAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
    std::filesystem::path exportPath = std::filesystem::current_path().append(std::format("{}\\{}", EXPORT_DIRECTORY_NAME, fourCCToString(asset->GetAssetType())));
    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset type directory.");
        return false;
    }

    exportPath.append(asset->GetAssetName());
    if (!CreateDirectories(exportPath.parent_path()))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }

    switch (setting)
    {
    case eUIImageExportSetting::PNG_HQ:
    case eUIImageExportSetting::PNG_LQ:
    {
        return ExportPngUIImageAsset(pakAsset, uiAsset, exportPath, setting);
    }
    case eUIImageExportSetting::DDS_HQ:
    case eUIImageExportSetting::DDS_LQ:
    {
        return ExportDdsUIImageAsset(pakAsset, uiAsset, exportPath, setting);
    }
    default:
    {
        assertm(false, "Export setting is not handled.");
        return false;
    }
    }

    unreachable();
}

void InitUIImageAssetType()
{
    static const char* settings[] = { "PNG (High Quality)", "PNG (Low Quality)", "DDS (High Quality)", "DDS (Low Quality)" };
    AssetTypeBinding_t type =
    {
        .type = 'aiiu',
        .headerAlignment = 8,
        .loadFunc = LoadUIImageAsset,
        .postLoadFunc = PostLoadUIImageAsset,
        .previewFunc = PreviewUIImageAsset,
        .e = { ExportUIImageAsset, 0, settings, ARRSIZE(settings) },
    };

    REGISTER_TYPE(type);
}