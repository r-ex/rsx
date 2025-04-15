#include "core/stdafx.h"
#include "branch/respawn/apex/bspfile.h"
#include "branch/respawn/apex/bvh.hpp"
#include "branch/bsp.h"
#include "model/model_stl.h"

BEGIN_NAMESPACE(apex)

// parse header and lump infos
void ParseBSPFile(CBSPFile* bsp, const fs::path& inputPath)
{
	std::ifstream inputFile(inputPath, std::ios::in | std::ios::binary);

	if (!inputFile.is_open())
		Error( "%s: failed to open file '%s' for read\n", __FUNCTION__, inputPath.string().c_str());

	const int bspFileSize = (int)fs::file_size(inputPath);

	if (bspFileSize < sizeof(BSPHeader_t))
		Error( "%s: failed to read file '%s'. file was too small for BSP header (expected %zu bytes, got %i)\n", __FUNCTION__ , inputPath.string().c_str(), sizeof(BSPHeader_t), bspFileSize);

	char* bspFileBuffer = new char[bspFileSize];

	inputFile.read(bspFileBuffer, bspFileSize);

	const BSPHeader_t* bspHeader = reinterpret_cast<BSPHeader_t*>(bspFileBuffer);

	const bool hasExternalLumps = bspFileSize == sizeof(BSPHeader_t);

	for (int i = 0; i < MAX_LUMPS; i++)
	{
		const respawn::lump_t& bspLump = bspHeader->lumps[i];

		BSPLump_t lump{};
		lump.size = bspLump.filelen;
		lump.version = bspLump.version;

		if (hasExternalLumps) 
		{
			if (lump.size == 0)
			{
				continue;  // don't effort for nothing.
			}
			char lumpTemplate[18];
			sprintf_s(lumpTemplate, "bsp.%04x.bsp_lump", i);

			fs::path lumpPath = fs::path(inputPath).replace_extension(lumpTemplate);

			if (!fs::exists(lumpPath) || !fs::is_regular_file(lumpPath))
			{
				Warning("LumpFile %s doesn't exist or isn't readable\n", lumpPath.string().c_str());
				continue;
			}

			const int lumpSize = (int)fs::file_size(lumpPath);
			if (fs::file_size(lumpPath) != bspLump.filelen)
			{
				Warning("LumpFile %s has different size from header. (%i vs expected %i) -- Still reading it\n", lumpPath.string().c_str(), lumpSize, lump.size);
				lump.size = lumpSize;
			}

			std::ifstream lumpFile(lumpPath, std::ios::in | std::ios::binary);

			if (!lumpFile.is_open())
			{
				Warning("LumpFile failed to open file '%s' for read\n", lumpPath.string().c_str());
				continue;
			}

			char* lumpFileBuffer = new char[lump.size];
			lumpFile.read(lumpFileBuffer, lump.size);

			lump.pData = lumpFileBuffer;
		}
		else 
		{
			if (bspLump.fileofs > bspFileSize)
			{
				Warning("Lump %04x file offset exceeds BSP file size. (size %i, offset %i)\n", i, bspFileSize, bspLump.fileofs);
				continue;
			}

			if (bspLump.fileofs != 0 && bspLump.filelen != 0)
			{
				lump.pData = bspFileBuffer + bspLump.fileofs;
			}
		}

		bsp->lumps[i] = lump;
	}
}

BSPModel_t& CollisionModel(CBSPFile* bsp)
{
	int numModels = bsp->lumps[LUMP_MODELS].size / sizeof(dmodel_t);

	dmodel_t* pModels = LUMP_DATA(bsp, LUMP_MODELS, dmodel_t);

	for (int i = 0; i < numModels; ++i)
	{
		const dmodel_t* pModel = &pModels[i];

		if (pModel->meshCount > 0)
		{
			const dbvhnode_t* startNode = &LUMP_DATA(bsp, LUMP_BVH_NODES, dbvhnode_t)[pModel->bvhNodeIndex];
			uint32_t contents = LUMP_DATA(bsp, LUMP_CONTENTS_MASKS, uint32_t)[startNode->cmIndex];

			Coll_HandleNodeChildType(bsp, contents, 0, startNode->child0Type, startNode->index0, pModel);
			Coll_HandleNodeChildType(bsp, contents, 0, startNode->child1Type, startNode->index1, pModel);
			Coll_HandleNodeChildType(bsp, contents, 0, startNode->child2Type, startNode->index2, pModel);
			Coll_HandleNodeChildType(bsp, contents, 0, startNode->child3Type, startNode->index3, pModel);
		}
	}

	return bsp->collisionModel;
}

void DumpBSPFile(const fs::path& inputPath)
{
	printf("Dumping bsp file '%s' as branch 'apex'\n", inputPath.string().c_str());

	CBSPFile* bsp = new CBSPFile();

	ParseBSPFile(bsp, inputPath);

	BSPModel_t& collisionModel = CollisionModel(bsp);

	printf("Exporting collision mesh...\n");

	//collisionModel.exportOBJ(fs::path(inputPath).replace_extension(".coll.obj"));
	collisionModel.exportSTL(fs::path(inputPath).replace_extension(".coll.stl"));

	printf("Exported collision mesh!\n");

	delete bsp;
}


END_NAMESPACE()