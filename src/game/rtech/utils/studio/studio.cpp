#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>
#include <game/rtech/utils/studio/studio_r2.h>
#include <game/rtech/utils/studio/studio_r5.h>

StudioLooseData_t::StudioLooseData_t(const std::filesystem::path& path, const char* name, char* buffer, const size_t bufferSize, const bool hasIDCV, const char* const aniname) : vertexDataBuffer(nullptr), vertexDataOffset(), vertexDataSize(),
physicsDataBuffer(nullptr), physicsDataOffset(0), physicsDataSize(0), animDataBuffer(nullptr), animDataOffset(0), animDataSize(0), vertexBufAllocated(false), physicsBufAllocated(false), animBufAllocated(false)
{
    std::filesystem::path filePath(path);

    //
    // parse and load vertex file
    //
    char* curpos = buffer;  // current position in the buffer
    size_t curoff = 0ull;   // current offset in the buffer

    const char* const fileName = GetStringAfterLastSlash(name);

    for (int i = 0; i < LooseDataType::SLD_COUNT; i++)
    {
        filePath.replace_filename(fileName); // because of vtx file extension
        filePath.replace_extension(s_StudioLooseDataExtensions[i]);

        // values should default to 0
        if (!std::filesystem::exists(filePath))
            continue;

        const size_t fileSize = std::filesystem::file_size(filePath);

#ifndef STREAMIO
        std::ifstream file(filePath, std::ios::binary | std::ios::in);
        file.read(curpos, fileSize);
#else
        StreamIO file(filePath, eStreamIOMode::Read);
        file.read(curpos, fileSize);
#endif // !STREAMIO

        vertexDataOffset[i] = static_cast<int>(curoff);
        vertexDataSize[i] = static_cast<int>(fileSize);

        curoff += IALIGN16(fileSize);
        curpos += IALIGN16(fileSize);

        assertm(curoff < bufferSize, "overflowed managed buffer");
    }

    // file exists but is not used (looking at you ogre_titan.mdl)
    if (hasIDCV == false)
    {
        vertexDataOffset[SLD_VVC] = 0;
        vertexDataSize[SLD_VVC] = 0;
    }

    // only allocate memory if we have vertex data
    if (curoff)
    {
        char* tmp = new char[curoff];
        memcpy_s(tmp, curoff, buffer, curoff);
        vertexDataBuffer = tmp;
        vertexBufAllocated = true;
    }

    //
    // parse and load phys
    //
    filePath.replace_extension(".phy");

    if (std::filesystem::exists(filePath))
    {
        const size_t fileSize = std::filesystem::file_size(filePath);

#ifndef STREAMIO
        std::ifstream file(filePath, std::ios::binary | std::ios::in);
        file.read(buffer, fileSize);
#else
        StreamIO file(filePath, eStreamIOMode::Read);
        file.read(curpos, fileSize);
#endif // !STREAMIO

        physicsDataOffset = 0;
        physicsDataSize = static_cast<int>(fileSize);

        char* tmp = new char[physicsDataSize];
        memcpy_s(tmp, physicsDataSize, buffer, physicsDataSize);
        physicsDataBuffer = tmp;
        physicsBufAllocated = true;
    }

    // ani file
    if (aniname == nullptr)
    {
        return;
    }

    filePath.replace_filename(aniname);

    if (std::filesystem::exists(filePath))
    {
        const size_t fileSize = std::filesystem::file_size(filePath);

#ifndef STREAMIO
        std::ifstream file(filePath, std::ios::binary | std::ios::in);
        file.read(buffer, fileSize);
#else
        StreamIO file(filePath, eStreamIOMode::Read);
        file.read(curpos, fileSize);
#endif // !STREAMIO

        animDataOffset = 0;
        animDataSize = static_cast<int>(fileSize);

        char* tmp = new char[animDataSize];
        memcpy_s(tmp, animDataSize, buffer, animDataSize);
        animDataBuffer = tmp;
        animBufAllocated = true;
    }
}

StudioLooseData_t::StudioLooseData_t(const char* const file) : vertexDataBuffer(file), physicsDataBuffer(file), animDataBuffer(nullptr), animDataOffset(0), animDataSize(0), vertexBufAllocated(false), physicsBufAllocated(false), animBufAllocated(false)
{
    const r2::studiohdr_t* const pStudioHdr = reinterpret_cast<const r2::studiohdr_t* const>(file);

    assertm(pStudioHdr->id == IDSTUDIOHEADER, "invalid file");
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

StudioLooseData_t::StudioLooseData_t(const char* const file, const char* const vertex, const char* const physics) : vertexDataBuffer(vertex), physicsDataBuffer(physics), animDataBuffer(nullptr), animDataOffset(0), animDataSize(0), vertexBufAllocated(false), physicsBufAllocated(false), animBufAllocated(false)
{
    const r5::studiohdr_v8_t* const pStudioHdr = reinterpret_cast<const r5::studiohdr_v8_t* const>(file);

    assertm(pStudioHdr->id == IDSTUDIOHEADER, "invalid file");
    assertm(pStudioHdr->version == 54, "invalid file");

    vertexDataOffset[SLD_VTX] = pStudioHdr->vtxOffset > -1 ? pStudioHdr->vtxOffset : 0;
    vertexDataOffset[SLD_VVD] = pStudioHdr->vvdOffset;
    vertexDataOffset[SLD_VVC] = pStudioHdr->vvcOffset;
    vertexDataOffset[SLD_VVW] = pStudioHdr->vvwOffset;

    vertexDataSize[SLD_VTX] = pStudioHdr->vtxSize;
    vertexDataSize[SLD_VVD] = pStudioHdr->vvdSize;
    vertexDataSize[SLD_VVC] = pStudioHdr->vvcSize;
    vertexDataSize[SLD_VVW] = pStudioHdr->vvwSize;

    physicsDataOffset = pStudioHdr->phyOffset > -1 ? pStudioHdr->phyOffset : 0;
    physicsDataSize = pStudioHdr->phySize;
}

// verify all the loose files for a studio model
const bool StudioLooseData_t::VerifyFileIntegrity(const studiohdr_short_t* const pHdr) const
{
    assertm(pHdr, "header should not be nullptr");

    const OptimizedModel::FileHeader_t* const pVTX = GetVTX();
    const vvd::vertexFileHeader_t* const pVVD = GetVVD();
    const vvc::vertexColorFileHeader_t* const pVVC = GetVVC();
    const vvw::vertexBoneWeightsExtraFileHeader_t* const pVVW = GetVVW();

    const studiohdr_short_t* const pANI = reinterpret_cast<const studiohdr_short_t* const>(GetANI());

    // we have a vtx and the checksum doesn't match, or version is incorrect
    if (pVTX && (pHdr->checksum != pVTX->checkSum || pVTX->version != OPTIMIZED_MODEL_FILE_VERSION))
    {
        assertm(false, "invalid vtx file");
        return false;
    }

    // we have a vvd and the checksum doesn't match, version is incorrect, or id is incorrect
    if (pVVD && (pHdr->checksum != pVVD->checksum || pVVD->version != MODEL_VERTEX_FILE_VERSION || pVVD->id != MODEL_VERTEX_FILE_ID))
    {
        assertm(false, "invalid vvd file");
        return false;
    }

    // we have a vvc and the checksum doesn't match, version is incorrect, or id is incorrect
    if (pVVC && (pHdr->checksum != pVVC->checksum || pVVC->version != MODEL_VERTEX_COLOR_FILE_VERSION || pVVC->id != MODEL_VERTEX_COLOR_FILE_ID))
    {
        assertm(false, "invalid vvc file");
        return false;
    }

    // we have a vtx and the checksum doesn't match, or version is incorrect
    if (pVVW && (pHdr->checksum != pVVW->checksum || pVVW->version != MODEL_VERTEX_WEIGHT_FILE_VERSION))
    {
        assertm(false, "invalid vvw file");
        return false;
    }

    // missing vertex files, if we have a vtx we should have a vvd, and vice versa
    if ((pVTX && pVVD == nullptr) || (pVTX == nullptr && pVVD))
    {
        assertm(false, "not all required vertex files are present");
        return false;
    }

    if (pANI && (pANI->id != IDSTUDIOANIMGROUPHEADER || pANI->version != pHdr->version || pANI->checksum != pHdr->checksum))
    {
        assertm(false, "invalid ani file");
        return false;
    }

    // hard to check phys because two different versions, and later apex versions lack features to check if it's apex

    return true;
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

// gets a source bone transform for a bone if it exists
const mstudiosrcbonetransform_t* const GetSrcBoneTransform(const char* const bone, const mstudiosrcbonetransform_t* const pSrcBoneTransforms, const int numSrcBoneTransforms)
{
    const mstudiosrcbonetransform_t* pSrcBoneTransform = nullptr;

    // [rika]: parse through srcbonetransforms until we find a matching one (by name yikes) or return nullptr if there's no transform for this bone
    for (int i = 0; i < numSrcBoneTransforms; i++)
    {
        pSrcBoneTransform = pSrcBoneTransforms + i;

        if (!strncmp(pSrcBoneTransform->pszName(), bone, MAX_PATH))
            return pSrcBoneTransform;
    }

    return nullptr;
}

// previously in studio_generic
void StaticPropFlipFlop(Vector& in)
{
    const Vector tmp(in);

    in.x = tmp.y;
    in.y = -tmp.x;
}