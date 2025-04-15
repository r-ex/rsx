#pragma once

struct VertexWeight_t
{
	float weight;
	int16_t bone;
};

struct Vertex_t
{
	Vector position;
	Normal32 normalPacked;
	Color32 color;
	Vector2D texcoord; // texcoord0

	// [rika]: if this vertex buffer is used for rendering our shader doesn't support bones anyway.
	uint32_t weightCount : 8;
	uint32_t weightIndex : 24; // max weight count in a mesh is 1048576 (2^20), 24 bits gives plenty of headroom with a max value of 16777216 (2^24)
};

class CMeshData
{
public:
	CMeshData() : indiceOffset(0ull), vertexOffset(0ull), weightOffset(0ull), texcoordOffset(0ull), writer(nullptr) {};
	~CMeshData() {};

	inline void InitWriter() { writer = (char*)this + IALIGN16(sizeof(CMeshData)); memset(&indiceOffset, 0, sizeof(CMeshData) - sizeof(writer)); };
	inline void DestroyWriter() { size = writer - reinterpret_cast<char*>(this); writer = nullptr; };

	void AddIndices(const uint16_t* const indices, const size_t indiceCount);
	void AddVertices(const Vertex_t* const vertices, const size_t vertexCount);
	void AddWeights(const VertexWeight_t* const weights, const size_t weightCount);
	void AddTexcoords(const Vector2D* const texcoords, const size_t texcoordCount);

	inline uint16_t* const GetIndices() const { return indiceOffset > 0 ? reinterpret_cast<uint16_t*>((char*)this + indiceOffset) : nullptr; };
	inline Vertex_t* const GetVertices() const { return vertexOffset > 0 ? reinterpret_cast<Vertex_t*>((char*)this + vertexOffset) : nullptr; };
	inline VertexWeight_t* const GetWeights() const { return weightOffset > 0 ? reinterpret_cast<VertexWeight_t*>((char*)this + weightOffset) : nullptr; };
	inline Vector2D* const GetTexcoords() const { return texcoordOffset > 0 ? reinterpret_cast<Vector2D*>((char*)this + texcoordOffset) : nullptr; };

	inline const int64_t GetSize() const { return size; };

private:
	int64_t indiceOffset;
	int64_t vertexOffset;
	int64_t weightOffset;
	int64_t texcoordOffset;

	int64_t size; // size of this data

	char* writer; // for writing only
};

// for parsing the animation data
class CAnimDataBone
{
public:
	CAnimDataBone(const size_t frameCount) : flags(0)
	{
		positions.resize(frameCount);
		rotations.resize(frameCount);
		scales.resize(frameCount);
	};

	inline void SetFlags(const uint8_t& flagsIn) { flags |= flagsIn; };
	inline void SetFrame(const int frameIdx, const Vector& pos, const Quaternion& quat, const Vector& scale)
	{
		positions.at(frameIdx) = pos;
		rotations.at(frameIdx) = quat;
		scales.at(frameIdx) = scale;
	}

	enum BoneFlags
	{
		ANIMDATA_POS = 0x1, // bone has pos values
		ANIMDATA_ROT = 0x2, // bone has rot values
		ANIMDATA_SCL = 0x4, // bone has scale values
		ANIMDATA_DATA = (ANIMDATA_POS | ANIMDATA_ROT | ANIMDATA_SCL), // bone has animation data
	};

	inline const uint8_t GetFlags() const { return flags; };
	inline const Vector* GetPosPtr() const { return positions.data(); };
	inline const Quaternion* GetRotPtr() const { return rotations.data(); };
	inline const Vector* GetSclPtr() const { return scales.data(); };

private:
	uint8_t flags;

	std::vector<Vector> positions;
	std::vector<Quaternion> rotations;
	std::vector<Vector> scales;
};

class CAnimData
{
public:
	CAnimData(const size_t boneCount, const size_t frameCount) : numBones(boneCount), numFrames(frameCount), memory(false), pBuffer(nullptr), pOffsets(nullptr), pFlags(nullptr) {};
	CAnimData(char* const buf);

	inline void ReserveVector() { bones.resize(numBones, numFrames); };
	CAnimDataBone& GetBone(const size_t idx) { return bones.at(idx); };

	// mem
	const uint8_t GetFlag(const size_t idx) const { return pFlags[idx]; };
	const char* const GetData(const size_t idx) const { return GetFlag(idx) & CAnimDataBone::ANIMDATA_DATA ? reinterpret_cast<const char* const>(pBuffer + pOffsets[idx]) : nullptr; }

	// returns allocated buffer
	const size_t ToMemory(char* const buf);

private:
	size_t numBones;
	size_t numFrames;

	bool memory; // memory format

	// mem
	char* const pBuffer;
	const size_t* pOffsets;
	const uint8_t* pFlags;

	std::vector<CAnimDataBone> bones;
};