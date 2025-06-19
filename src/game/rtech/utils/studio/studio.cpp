#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>
#include <game/rtech/utils/studio/studio_r2.h>

StudioLooseData_t::StudioLooseData_t(const std::filesystem::path& path, const char* name, char* buffer) : vertexDataBuffer(nullptr), vertexDataOffset(), vertexDataSize(),
    physicsDataBuffer(nullptr), physicsDataOffset(0), physicsDataSize(0)
{
    std::filesystem::path filePath(path);

    //
    // parse and load vertex file
    //
    char* curpos = buffer;  // current position in the buffer
    size_t curoff = 0ull;   // current offset in the buffer

    const char* const fileName = keepAfterLastSlashOrBackslash(name);

    for (int i = 0; i < LooseDataType::SLD_COUNT; i++)
    {
        filePath.replace_filename(fileName); // because of vtx file extension
        filePath.replace_extension(s_StudioLooseDataExtensions[i]);

        // values should default to 0
        if (!std::filesystem::exists(filePath))
            continue;

        const size_t fileSize = std::filesystem::file_size(filePath);

        StreamIO fileIn(filePath, eStreamIOMode::Read);
        fileIn.R()->read(curpos, fileSize);

        vertexDataOffset[i] = static_cast<int>(curoff);
        vertexDataSize[i] = static_cast<int>(fileSize);

        curoff += IALIGN16(fileSize);
        curpos += IALIGN16(fileSize);

        assertm(curoff < managedBufferSize, "overflowed managed buffer");
    }

    // only allocate memory if we have vertex data
    if (curoff)
    {
        vertexDataBuffer = new char[curoff];
        memcpy_s(vertexDataBuffer, curoff, buffer, curoff);
    }

    //
    // parse and load phys
    //
    filePath.replace_extension(".phy");

    if (std::filesystem::exists(filePath))
    {
        const size_t fileSize = std::filesystem::file_size(filePath);

        StreamIO fileIn(filePath, eStreamIOMode::Read);
        fileIn.R()->read(buffer, fileSize);

        physicsDataOffset = 0;
        physicsDataSize = static_cast<int>(fileSize);

        physicsDataBuffer = new char[physicsDataSize];
        memcpy_s(physicsDataBuffer, physicsDataSize, buffer, physicsDataSize);
    }

    // here's where ani will go when I do animations (soontm)
}

StudioLooseData_t::StudioLooseData_t(char* file) : vertexDataBuffer(file), physicsDataBuffer(file)
{
    r2::studiohdr_t* const pStudioHdr = reinterpret_cast<r2::studiohdr_t*>(file);

    assertm(pStudioHdr->id == MODEL_FILE_ID, "invalid file");
    assertm(pStudioHdr->version == 53, "invalid file");

    vertexDataOffset[SLD_VTX] = pStudioHdr->vtxOffset;
    vertexDataOffset[SLD_VVD] = pStudioHdr->vvdOffset;
    vertexDataOffset[SLD_VVC] = pStudioHdr->vvcOffset;
    vertexDataOffset[SLD_VVW] = 0;

    vertexDataSize[SLD_VTX] = pStudioHdr->vtxSize;
    vertexDataSize[SLD_VVD] = pStudioHdr->vvdSize;
    vertexDataSize[SLD_VVC] = pStudioHdr->vvcSize;
    vertexDataSize[SLD_VVW] = 0;

    physicsDataOffset = pStudioHdr->phyOffset;
    physicsDataSize = pStudioHdr->phySize;
}

const char* studiohdr_short_t::pszName() const
{
	if (!UseHdr2())
	{
		const int offset = reinterpret_cast<const int* const>(this)[3];

		return reinterpret_cast<const char*>(this) + offset;
	}

	// very bad very not good. however these vars have not changed since HL2.
	const r1::studiohdr_t* const pStudioHdr = reinterpret_cast<const r1::studiohdr_t* const>(this);

	return pStudioHdr->pszName();
}

namespace vg
{
    // rmdl v9 to rmdl v12.3
    const int64_t VertexSizeFromFlags_V9(const uint64_t flags) // rmdl v9 to rmdl v12.3
    {
        int64_t vertexCacheSize =

            // add the size of position values, hard to explain this but it's clever and overly complicated, takes advantage of how negative numbers work
            ((-4 * flags) & 0xC)

            // add size of Color32, max size of four, shifts flag down so when masked and set the result will be a value of 0x4
            + ((flags >> 2) & 4)

            // add size of unknown, max size of eight, shifts flag down so when masked and set the result will be a value of 0x8 (what data is this?)
            + ((flags >> 3) & 8)

            // add the size of normals & tangents, max size of 60 (result & 0x3C)(would have to have all normal/tangent flags set), skip first two bits of mirco lut, 4 bits for each flag combo ((0x4 = VERT_BLENDINDICES, 0x8 = VERT_BLENDWEIGHTS_UNPACKED, 0x10 = VERT_BLENDWEIGHTS_PACKED) = 0x1C)
            + ((0x960174006301300u >> (((unsigned __int8)(flags >> 6) & 0x3Cu) + 2)) & 0x3C)

            // add the size of weights, max size of 12 (result & 0xC)(indices and one weight, packed or unpacked), skip first two bits of mirco lut, 4 bits for each flag combo ((0x4 = VERT_BLENDINDICES, 0x8 = VERT_BLENDWEIGHTS_UNPACKED, 0x10 = VERT_BLENDWEIGHTS_PACKED) = 0x1C)
            + ((0x2132100 >> (((flags >> 10) & 0x1C) + 2)) & 0xC);

        // texcoord flags start at 24 bits, shift this along to get texcord flags, stop looping once there are no more texcoords
        for (int64_t texcoordFlagShift = flags >> 24; texcoordFlagShift; texcoordFlagShift >>= VERT_TEXCOORD_BITS)
        {
            // three bits per texcoord format, one format per
            const uint8_t texcoordFlags = static_cast<uint8_t>(texcoordFlagShift);
            vertexCacheSize += (0x48A31A20 >> (3 * (texcoordFlags & VERT_TEXCOORD_MASK))) & 0x1C;
        }

        return vertexCacheSize;
    }

    // rmdl v13 to retail
    const int64_t VertexSizeFromFlags_V13(const uint64_t flags)
    {
        int64_t vertexCacheSize =

            // add the size of position values, mask to four bits per position type, two bits for a position enum (0-3).
            // 0 = 0, 1 = 12, 2 = 8, 3 = 6
            ((0x68C0 >> (4 * (flags & 3))) & 0xF)

            // add size of Color32, max size of four, shifts flag down so when masked and set the result will be a value of 0x4
            + ((flags >> 2) & 4)

            // add size of unknown, max size of eight, shifts flag down so when masked and set the result will be a value of 0x8 (what data is this?)
            + ((flags >> 3) & 8)

            // add the size of normals & tangents, max size of 60 (result & 0x3C)(would have to have all normal/tangent flags set), skip first two bits of mirco lut, 4 bits for each flag combo ((0x4 = VERT_BLENDINDICES, 0x8 = VERT_BLENDWEIGHTS_UNPACKED, 0x10 = VERT_BLENDWEIGHTS_PACKED) = 0x1C)
            + ((0x960174006301300u >> (((unsigned __int8)(flags >> 6) & 0x3Cu) + 2)) & 0x3C)

            // add the size of weights, max size of 12 (result & 0xC)(indices and one weight, packed or unpacked), skip first two bits of mirco lut, 4 bits for each flag combo ((0x4 = VERT_BLENDINDICES, 0x8 = VERT_BLENDWEIGHTS_UNPACKED, 0x10 = VERT_BLENDWEIGHTS_PACKED) = 0x1C)
            + ((0x2132100 >> (((flags >> 10) & 0x1C) + 2)) & 0xC);

        // texcoord flags start at 24 bits, shift this along to get texcord flags, stop looping once there are no more texcoords
        for (int64_t texcoordFlagShift = flags >> 24; texcoordFlagShift; texcoordFlagShift >>= VERT_TEXCOORD_BITS)
        {
            // three bits per texcoord format, one format per
            const uint8_t texcoordFlags = static_cast<uint8_t>(texcoordFlagShift);
            vertexCacheSize += (0x48A31A20 >> (3 * (texcoordFlags & VERT_TEXCOORD_MASK))) & 0x1C;
        }

        return vertexCacheSize;
    }
}