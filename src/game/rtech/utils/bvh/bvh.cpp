
#include "pch.h"
#include "bvh.h"

//BEGIN_NAMESPACE(apex)

static void R_ParseBVHNode(CollisionModel_t& colModel, const int nodeIndex, const BVHModel_t* pModel);

//inline std::map<uint32_t, int> s_MaskMap{};

static void Coll_ParseTriLeaf4(CollisionModel_t& colModel, const dbvhleaf_poly_t* pLeaf, const Vector* pVertices, const uint32_t contents)
{
	const dbvhtri_t* triData = reinterpret_cast<const dbvhtri_t*>(&pLeaf[1]);

	int baseVert = pLeaf->baseVertexOffset << 10;

	for (uint32_t i = 0; i < pLeaf->polyCount + 1; ++i)
	{
		const dbvhtri_t& tri = triData[i];

		baseVert += tri.vertAIndex;

		Triangle t{ pVertices[baseVert], pVertices[baseVert + 1 + tri.vertBIndex], pVertices[baseVert + 1 + tri.vertCIndex] };

		t.flags = contents;

		colModel.tris.emplace_back(t);
	}
}

static void Coll_ParseTriPackedLeaf5(CollisionModel_t& colModel, const dbvhleaf_poly_t* pLeaf, const PackedVector* pVertices, const uint32_t contents, const float scale, const Vector& origin)
{
	const dbvhtri_t* triData = reinterpret_cast<const dbvhtri_t*>(&pLeaf[1]);

	int baseVert = pLeaf->baseVertexOffset << 10;

	for (uint32_t i = 0; i < pLeaf->polyCount + 1; ++i)
	{
		const dbvhtri_t& tri = triData[i];

		baseVert += tri.vertAIndex;
		PackedVector p1 = pVertices[baseVert];
		PackedVector p2 = pVertices[baseVert + 1 + tri.vertBIndex];
		PackedVector p3 = pVertices[baseVert + 1 + tri.vertCIndex];

		Triangle t{
			Vector(
				p1.x * 0xFFFF * scale + origin.x,
				p1.y * 0xFFFF * scale + origin.y,
				p1.z * 0xFFFF * scale + origin.z
			),
			Vector(
				p2.x * 0xFFFF * scale + origin.x,
				p2.y * 0xFFFF * scale + origin.y,
				p2.z * 0xFFFF * scale + origin.z
			),
			Vector(
				p3.x * 0xFFFF * scale + origin.x,
				p3.y * 0xFFFF * scale + origin.y,
				p3.z * 0xFFFF * scale + origin.z
			),
		};

		t.flags = contents;

		colModel.tris.emplace_back(t);
	}
}

static void Coll_ParseQuadLeaf6(CollisionModel_t& colModel, const dbvhleaf_poly_t* pLeaf, const Vector* pVertices, const uint32_t contents)
{
	const dbvhquad_t* quadData = reinterpret_cast<const dbvhquad_t*>(&pLeaf[1]);

	int baseVert = pLeaf->baseVertexOffset << 10;

	for (uint32_t i = 0; i < pLeaf->polyCount + 1; ++i)
	{
		const dbvhquad_t& quad = quadData[i];

		baseVert += quad.vertAIndex;

		Quad q{};

		q.a = pVertices[baseVert];
		q.b = pVertices[baseVert + 1 + quad.vertBIndex];
		q.c = pVertices[baseVert + 1 + quad.vertCIndex];

		q.d = Vector(
			q.a.x + (q.b.x - q.a.x) + (q.c.x - q.a.x),
			q.a.y + (q.b.y - q.a.y) + (q.c.y - q.a.y),
			q.a.z + (q.b.z - q.a.z) + (q.c.z - q.a.z)
		);

		q.flags = contents;

		colModel.quads.emplace_back(q);
	}
}

static void Coll_ParseQuadPackedLeaf7(CollisionModel_t& colModel, const dbvhleaf_poly_t* pLeaf, const PackedVector* pVertices, const uint32_t contents, const float scale, const Vector& origin)
{
	const dbvhquad_t* quadData = reinterpret_cast<const dbvhquad_t*>(&pLeaf[1]);

	int baseVert = pLeaf->baseVertexOffset << 10;

	for (uint32_t i = 0; i < pLeaf->polyCount + 1; ++i)
	{
		const dbvhquad_t& quad = quadData[i];

		baseVert += quad.vertAIndex;
		PackedVector p1 = pVertices[baseVert];
		PackedVector p2 = pVertices[baseVert + 1 + quad.vertBIndex];
		PackedVector p3 = pVertices[baseVert + 1 + quad.vertCIndex];

		Quad q{};

		q.a = Vector(
			p1.x * 0xFFFF * scale + origin.x,
			p1.y * 0xFFFF * scale + origin.y,
			p1.z * 0xFFFF * scale + origin.z
		);
		q.b = Vector(
			p2.x * 0xFFFF * scale + origin.x,
			p2.y * 0xFFFF * scale + origin.y,
			p2.z * 0xFFFF * scale + origin.z
		);
		q.c = Vector(
			p3.x * 0xFFFF * scale + origin.x,
			p3.y * 0xFFFF * scale + origin.y,
			p3.z * 0xFFFF * scale + origin.z
		);

		q.d = Vector(
			q.a.x + (q.b.x - q.a.x) + (q.c.x - q.a.x),
			q.a.y + (q.b.y - q.a.y) + (q.c.y - q.a.y),
			q.a.z + (q.b.z - q.a.z) + (q.c.z - q.a.z)
		);

		q.flags = contents;

		colModel.quads.emplace_back(q);
	}
}

static void Coll_ParseLeaf8(CollisionModel_t& colModel, const dbvhtype8_t* pLeaf, uint32_t contents, const BVHModel_t* /*pModel*/)
{
	const PackedVector* packedVertices = reinterpret_cast<const PackedVector*>(&pLeaf[1]);
	// Ignore the faces, no-one loves them!

	int offset = (sizeof(PackedVector) * pLeaf->vertexCount + 3 * pLeaf->faceCount + 3) / 4;

	const int* nodeIndex = reinterpret_cast<const int*>(&pLeaf[1]);
	for (int i = 0; i < pLeaf->triCount; ++i)
	{
		const dbvhleaf_poly_t* pTri = reinterpret_cast<const dbvhleaf_poly_t*>(nodeIndex + offset);

		Coll_ParseTriPackedLeaf5(colModel, pTri, packedVertices, contents, pLeaf->decodeScale, pLeaf->origin);
		offset += (sizeof(dbvhleaf_poly_t) + (pTri->polyCount + 1) * sizeof(dbvhtri_t)) / 4;
	}

	for (int i = 0; i < pLeaf->quadCount; ++i)
	{
		const dbvhleaf_poly_t* pQuad = reinterpret_cast<const dbvhleaf_poly_t*>(nodeIndex + offset);

		Coll_ParseQuadPackedLeaf7(colModel, pQuad, packedVertices, contents, pLeaf->decodeScale, pLeaf->origin);
		offset += (sizeof(dbvhleaf_poly_t) + (pQuad->polyCount + 1) * sizeof(dbvhquad_t)) / 4;
	}
}


void Coll_HandleNodeChildType(CollisionModel_t& colModel, const uint32_t contents, const int /*parentNodeIndex*/, const int nodeType, const int index, const BVHModel_t* pModel)
{
	//s_MaskMap[contents]++;

	if (nodeType != dbvhchildtype_e::NODE && nodeType != dbvhchildtype_e::UNK_3)
	{
		const uint32_t mask = (contents & pModel->maskFilter);
		const bool inFilter = pModel->filterAND ? mask == pModel->maskFilter : mask != 0;

		const bool skip = pModel->filterExclusive ? inFilter : !inFilter;

		if (skip)
		{
			//printf("Blocked node with contents %x, filter %x.\n", contents, pModel->maskFilter);
			return;
		}
	}

	switch (nodeType)
	{
	case dbvhchildtype_e::NO_CHILD: // 1
		break;
	case dbvhchildtype_e::NODE: // 0
	{
		R_ParseBVHNode(colModel, index, pModel);

		break;
	}
	case dbvhchildtype_e::UNK_3:
	{
		const int* pLeaf = reinterpret_cast<const int*>(&pModel->leafs[index * 4]);
		const int numLeaves = *pLeaf;

		int leafDataOffset = 1 + numLeaves;

		for (int i = 0; i < numLeaves; ++i)
		{
			const dbvhsubleaf_t* pSubLeaf = reinterpret_cast<const dbvhsubleaf_t*>(&pLeaf[i + 1]);
			const uint32_t contentMask = pModel->masks[pSubLeaf->cmIndex];

			Coll_HandleNodeChildType(colModel, contentMask, -1, pSubLeaf->primitiveType, index + leafDataOffset, pModel);
			leafDataOffset += pSubLeaf->length;
		}

		break;
	}
	case dbvhchildtype_e::PRIM_TRI:
	{
		const dbvhleaf_poly_t* pLeaf = reinterpret_cast<const dbvhleaf_poly_t*>(&pModel->leafs[index * 4]);
		Coll_ParseTriLeaf4(colModel, pLeaf, pModel->verts, contents);

		break;
	}
	case dbvhchildtype_e::PRIM_TRI_PACKED:
	{
		const dbvhleaf_poly_t* pLeaf = reinterpret_cast<const dbvhleaf_poly_t*>(&pModel->leafs[index * 4]);
		Coll_ParseTriPackedLeaf5(colModel, pLeaf, pModel->packedVerts, contents, pModel->scale, *pModel->origin);

		break;
	}
	case dbvhchildtype_e::PRIM_QUAD:
	{
		const dbvhleaf_poly_t* pLeaf = reinterpret_cast<const dbvhleaf_poly_t*>(&pModel->leafs[index * 4]);
		Coll_ParseQuadLeaf6(colModel, pLeaf, pModel->verts, contents);

		break;
	}
	case dbvhchildtype_e::PRIM_QUAD_PACKED:
	{
		const dbvhleaf_poly_t* pLeaf = reinterpret_cast<const dbvhleaf_poly_t*>(&pModel->leafs[index * 4]);
		Coll_ParseQuadPackedLeaf7(colModel, pLeaf, pModel->packedVerts, contents, pModel->scale, *pModel->origin);

		break;
	}
	case dbvhchildtype_e::UNK_8:
	{
		const dbvhtype8_t* pLeaf = reinterpret_cast<const dbvhtype8_t*>(&pModel->leafs[index * 4]);
		Coll_ParseLeaf8(colModel, pLeaf, contents, pModel);

		break;
	}
	//case dbvhchildtype_e::STATIC_PROP:
	//{
	//	// TODO, needs rpak access + rmdl parsing
	//	break;
	//}
	default:
		printf("Unhandled bvh node child type %u\n", nodeType);
	}
}


static void R_ParseBVHNode(CollisionModel_t& colModel, int nodeIndex, const BVHModel_t* pModel)
{
	const dbvhnode_t* startNode = &pModel->nodes[nodeIndex];
	const uint32_t contents = pModel->masks[startNode->cmIndex];

	Coll_HandleNodeChildType(colModel, contents, nodeIndex, startNode->child0Type, startNode->index0, pModel);
	Coll_HandleNodeChildType(colModel, contents, nodeIndex, startNode->child1Type, startNode->index1, pModel);
	Coll_HandleNodeChildType(colModel, contents, nodeIndex, startNode->child2Type, startNode->index2, pModel);
	Coll_HandleNodeChildType(colModel, contents, nodeIndex, startNode->child3Type, startNode->index3, pModel);
}


#pragma pack(push, 1)
struct stlheader_t
{
	char caption[80];
	unsigned int numTris;
};

struct stltri_t
{
	Vector nml;
	Vector a;
	Vector b;
	Vector c;
	unsigned short flags;
};

#pragma pack(pop)

inline Vector normal(Vector p1, Vector p2, Vector p3) {
	float x = (p2.y - p1.y) * (p3.z - p1.z) - (p3.y - p1.y) * (p2.z - p1.z);
	float y = (p2.z - p1.z) * (p3.x - p1.x) - (p2.x - p1.x) * (p3.z - p1.z);
	float z = (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);

	float len = sqrtf(x * x + y * y + z * z);
	return Vector(x / len, y / len, z / len);
}

bool CollisionModel_t::exportSTL(const std::filesystem::path& outPath)
{
	std::ofstream out(outPath, std::ios::out | std::ios::binary);

	if (!out.is_open())
		return false;

	// They seem to not be in the right spot yet, so might not want to export for now
	//bool include_packed = false;

	size_t numTris = this->tris.size() + this->quads.size() * 2;

	size_t stlFileSize = sizeof(stlheader_t) + (sizeof(stltri_t) * numTris);
	char* stlBuf = new char[stlFileSize];

	stlheader_t* pStlHeader = reinterpret_cast<stlheader_t*>(stlBuf);
	stltri_t* pStlTris = reinterpret_cast<stltri_t*>(&pStlHeader[1]);

	size_t i = 0;
	for (Triangle& tri : this->tris)
	{
		stltri_t& stlTri = pStlTris[i];
		i++;

		stlTri.nml = normal(tri.a, tri.b, tri.c);
		stlTri.a = tri.a;
		stlTri.b = tri.b;
		stlTri.c = tri.c;
		stlTri.flags = 0;
	}

	for (Quad& quad : this->quads)
	{
		stltri_t& triA = pStlTris[i];
		i++;

		triA.nml = normal(quad.a, quad.b, quad.c);
		triA.a = quad.a;
		triA.b = quad.b;
		triA.c = quad.c;
		triA.flags = 0;

		stltri_t& triB = pStlTris[i];
		i++;

		triB.nml = normal(quad.b, quad.d, quad.c);
		triB.a = quad.b;
		triB.b = quad.d;
		triB.c = quad.c;
		triB.flags = 0;
	}

	memset(stlBuf, 0, sizeof(stlheader_t));
	pStlHeader->numTris = (unsigned int)numTris;

	out.write(stlBuf, stlFileSize);
	delete[] stlBuf;

	return !out.fail();
}

bool CollisionModel_t::exportOBJ(const std::filesystem::path& outFile)
{
	std::ofstream out(outFile, std::ios::out | std::ios::binary);

	if (!out.is_open())
		return false;

	out << "# " << this->tris.size() << " tris\no tris\n";

	printf("Writing tris...\n");
	for (Triangle& tri : this->tris)
	{
		out
			<< "v " << tri.a.x << " " << tri.a.y << " " << tri.a.z << "\n"
			<< "v " << tri.b.x << " " << tri.b.y << " " << tri.b.z << "\n"
			<< "v " << tri.c.x << " " << tri.c.y << " " << tri.c.z << "\n"
			<< "f -3 -2 -1\n";
	}

	out << "\n# " << this->quads.size() << " quads\no quads\n";

	printf("Writing quads...\n");
	for (Quad& quad : this->quads)
	{
		out
			<< "v " << quad.a.x << " " << quad.a.y << " " << quad.a.z << "\n"
			<< "v " << quad.b.x << " " << quad.b.y << " " << quad.b.z << "\n"
			<< "v " << quad.c.x << " " << quad.c.y << " " << quad.c.z << "\n"
			<< "v " << quad.d.x << " " << quad.d.y << " " << quad.d.z << "\n"
			<< "f -3 -4 -2 -1\n";
	}

	return !out.fail();
}

//END_NAMESPACE()