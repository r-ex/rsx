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