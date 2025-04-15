#include <pch.h>
#include <core/mdl/compdata.h>

// mesh data
void CMeshData::AddIndices(const uint16_t* const indices, const size_t indiceCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	indiceOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = indiceCount * sizeof(uint16_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (indices)
		memcpy(writer, indices, bufferSize);

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddVertices(const Vertex_t* const vertices, const size_t vertexCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	vertexOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = vertexCount * sizeof(Vertex_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (vertices)
		memcpy(writer, vertices, bufferSize);

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddWeights(const VertexWeight_t* const weights, const size_t weightCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	weightOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = weightCount * sizeof(VertexWeight_t);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (weights)
		memcpy(writer, weights, bufferSize);

	writer += IALIGN16(bufferSize);
}

void CMeshData::AddTexcoords(const Vector2D* const texcoords, const size_t texcoordCount)
{
	assertm(writer, "attempting to write data, but writer is not initialized.");

	texcoordOffset = writer - reinterpret_cast<char*>(this);

	const size_t bufferSize = texcoordCount * sizeof(Vector2D);
	assertm((writer + bufferSize) - reinterpret_cast<char*>(this) < managedBufferSize, "data exceeded buffer size!!!");
	if (texcoords)
		memcpy(writer, texcoords, bufferSize);

	writer += IALIGN16(bufferSize);
}

CAnimData::CAnimData(char* const buf) : pBuffer(buf), memory(true)
{
	assertm(nullptr != pBuffer, "invalid pointer provided");

	const char* curpos = pBuffer;

	memcpy(&numBones, curpos, sizeof(size_t) * 2);
	curpos += sizeof(size_t) * 2;

	pOffsets = reinterpret_cast<const size_t* const>(curpos);
	curpos += IALIGN16(sizeof(size_t) * numBones);

	pFlags = reinterpret_cast<const uint8_t*>(curpos);
	curpos += IALIGN16(sizeof(uint8_t) * numBones);
};

const size_t CAnimData::ToMemory(char* const buf)
{
	char* curpos = buf;

	memcpy(curpos, &numBones, sizeof(size_t) * 2);
	curpos += sizeof(size_t) * 2;

	size_t* offsets = reinterpret_cast<size_t*>(curpos);
	curpos += IALIGN16(sizeof(size_t) * numBones);

	uint8_t* flags = reinterpret_cast<uint8_t*>(curpos);
	curpos += IALIGN16(sizeof(uint8_t) * numBones);

	for (size_t i = 0; i < numBones; i++)
	{
		const CAnimDataBone& bone = bones.at(i);

		offsets[i] = static_cast<size_t>(curpos - buf);

		flags[i] = bone.GetFlags();

		if (flags[i] & CAnimDataBone::ANIMDATA_POS)
		{
			memcpy(curpos, bone.GetPosPtr(), sizeof(Vector) * numFrames);
			curpos += sizeof(Vector) * numFrames;
		}

		if (flags[i] & CAnimDataBone::ANIMDATA_ROT)
		{
			memcpy(curpos, bone.GetRotPtr(), sizeof(Quaternion) * numFrames);
			curpos += sizeof(Quaternion) * numFrames;
		}

		if (flags[i] & CAnimDataBone::ANIMDATA_SCL)
		{
			memcpy(curpos, bone.GetSclPtr(), sizeof(Vector) * numFrames);
			curpos += sizeof(Vector) * numFrames;
		}
	}

	const size_t size = static_cast<size_t>(curpos - buf);
	assertm(size < CBufferManager::MaxBufferSize(), "animation data too large");

	return size;
};