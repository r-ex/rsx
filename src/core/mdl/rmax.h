#pragma once

/* RESPAWN MODEL ASSET EXCHANGE */
// simple model format ideal for parsing, not the best design but it works for now
namespace rmax
{
	static constexpr uint8_t s_FrameSizeLUT[8] = { 0, 12, 16, 28, 12, 24, 28, 40 };

	// if one of these is not set, default to bone value
	enum AnimTrackFlags_t
	{
		ANIM_TRACK_POS = 1 << 0,
		ANIM_TRACK_ROT = 1 << 1,
		ANIM_TRACK_SCL = 1 << 2,

		ANIM_TRACK_DATA = ANIM_TRACK_POS | ANIM_TRACK_ROT | ANIM_TRACK_SCL,
	};

	struct AnimTrack_t
	{
		uint16_t flags;
		uint16_t boneId; // index into this rmax's skeleton

		int dataOffset;
	};

	enum AnimFlags_t
	{
		ANIM_DELTA = 1 << 0,
		ANIM_EMPTY = 1 << 2, // empty animation, blank headers.
	};

	struct Anim_t
	{
		int nameOffset;

		uint16_t flags;

		uint16_t frameCount;
		float frameRate;

		int trackOffset;
	};

	struct Weight_t
	{
		Weight_t(const int boneIn, const float weightIn) : bone(boneIn), weight(weightIn) {};

		int bone;
		float weight;
	};

	struct Vertex_t
	{
		Vertex_t(const Vector& posIn, const Vector& normIn) : pos(posIn), norm(normIn), weightCount(0), weightIndex(0) {};

		Vector pos;
		Vector norm;

		// weights are stored externally, saves a bunch of space
		int weightCount;
		int weightIndex;
	};

	struct Indice_t
	{
		Indice_t(const uint16_t i0, const uint16_t i1, const uint16_t i2, const uint16_t i3) : a(i0), b(i1), c(i2), d(i3) {};

		uint16_t a, b, c, d;
	};

	struct Mesh_t
	{
		int16_t materialIndex; // index into materials
		int16_t collectionIndex; // index into collections

		// verts
		int vertexCount;
		int vertexIndex;

		// negative if this mesh doesn't contain this data
		int colorIndex;

		int uvIndex;
		int16_t uvCount; // how many channels per ver
		int16_t uvIndices; // index of the channels

		// faces
		int indiceCount;
		int indiceIndex;
	};

	struct MeshCollection_t
	{
		int nameOffset;
		int meshCount;
	};

	enum TextureType_t
	{
		ALBEDO,
		NORMAL,
		GLOSS,
		SPECULAR,
		EMISSIVE,
		AO,
		CAVITY,
	};

	static std::map<std::string, TextureType_t> s_TextureTypeMap = {
		{ "albedoTexture",		TextureType_t::ALBEDO },
		{ "normalTexture",		TextureType_t::NORMAL },
		{ "glossTexture",		TextureType_t::GLOSS },
		{ "specTexture",		TextureType_t::SPECULAR },
		{ "emissiveTexture",	TextureType_t::EMISSIVE },
		{ "aoTexture",			TextureType_t::AO },
		{ "cavityTexture",		TextureType_t::CAVITY },
	};

	struct Texture_t
	{
		int nameOffset;
		int type;
	};

	struct Material_t
	{
		int nameOffset;

		int16_t textureCount;
		int16_t textureIndex;
	};

	struct Bone_t
	{
		int nameOffset;
		int parentIndex;

		Vector pos;
		Quaternion quat;
		Vector scale;
	};

	constexpr int8_t curFmtVersion = 3;
	constexpr int8_t curFmtVersionMin = 2;
	constexpr int maxFileSize = 1024 * 1024 * 16;
	constexpr int rmaxFileId = MAKEFOURCC('r', 'm', 'a', 'x');

	// [rika]: add a sort of root path here, to do relative paths
	struct Hdr_t
	{
		int id;
		int8_t version;
		int8_t version_min;
		int16_t reserved;
		int nameOffset; // name of the skeleton (derived from base model)
		int skelNameOffset;

		int boneCount;
		int boneOffset;

		int textureCount;
		int textureOffset;

		int materialCount;
		int materialOffset;

		int meshCount;
		int meshOffset;

		// [rika]: basically bodypart sorting
		int meshCollectionCount;
		int meshCollectionOffset;

		// todo: remove counts, or don't, it only saves 16 bytes!
		int indiceCount;
		int indiceOffset;

		int vertexCount;
		int vertexOffset;

		int vertexColorCount;
		int vertexColorOffset;

		int vertexUvCount;
		int vertexUvOffset;

		int vertexWeightCount;
		int vertexWeightOffset;

		int animCount;
		int animOffset;
	};

	// exporting stuffzz
	class RMAXExporter;

	struct RMAXBone
	{
		RMAXBone(const char* nameIn, const int parentIn, const Vector& posIn, const Quaternion& quatIn, const Vector& scaleIn) : name(nameIn), parent(parentIn), pos(posIn), quat(quatIn), scale(scaleIn) {};

		const char* name;
		int parent;

		Vector pos;
		Quaternion quat;
		Vector scale;
	};

	struct RMAXTexture
	{
		RMAXTexture(const char* nameIn, const TextureType_t typeIn) : name(nameIn), type(typeIn) {}

		//const char* name;
		std::string name; // yucky

		int type;
	};

	struct RMAXMaterial
	{
		RMAXMaterial(const char* nameIn) :  name(nameIn) {};

		inline void AddTexture(const char* nameIn, const TextureType_t typeIn)
		{
			textures.emplace_back(nameIn, typeIn);
		}

		const char* name;

		std::vector<RMAXTexture> textures;
	};

	struct RMAXCollection
	{
		RMAXCollection(const char* nameIn, const int meshCountIn) : name(nameIn), meshCount(meshCountIn) {};

		const char* name;
		int meshCount;
	};

	struct RMAXMesh
	{
		RMAXMesh(RMAXExporter* const baseFile, const int16_t collectionIndexIn, const int16_t materialIndexIn) : base(baseFile), collectionIndex(collectionIndexIn), materialIndex(materialIndexIn), vertexCount(0), vertexIndex(0), colorIndex(0), uvIndex(0), uvCount(0), uvIndices(0),
			indiceCount(0), indiceIndex(0) {};

		RMAXExporter* const base;

		int16_t materialIndex; // index into materials
		int16_t collectionIndex; // index into collections

		// verts
		int vertexCount;
		int vertexIndex;

		// negative if this mesh doesn't contain this data
		int colorIndex;

		int uvIndex;
		int16_t uvCount; // how many channels per ver
		int16_t uvIndices; // index of the channels

		// faces
		int indiceCount;
		int indiceIndex;

		void AddVertex(const Vector& pos, const Vector& norm);
		void AddColor(const Color32 color);
		void AddTexcoord(const Vector2D& texcoord);
		void AddWeight(const size_t idx, const int bone, const float weight);
		void AddIndice(const uint16_t a, const uint16_t b, const uint16_t c, const uint16_t d = 0xFFFF);
	};

	struct RMAXAnimTrack
	{
	public:
		RMAXAnimTrack() = default;
		RMAXAnimTrack(const uint16_t flagsIn, const uint16_t boneIdIn, char* dataIn) : flags(flagsIn), boneId(boneIdIn), data(dataIn) {};

		void AddFrame(const int frameIndex, const Vector* pos, const Quaternion* q, const Vector* scale)
		{
			char* localPointer = data + (s_FrameSizeLUT[flags] * frameIndex);

			if (flags & rmax::AnimTrackFlags_t::ANIM_TRACK_POS)
			{
				std::memcpy(localPointer, pos, sizeof(Vector));
				localPointer += sizeof(Vector);
			}

			if (flags & rmax::AnimTrackFlags_t::ANIM_TRACK_ROT)
			{
				std::memcpy(localPointer, q, sizeof(Quaternion));
				localPointer += sizeof(Quaternion);
			}

			if (flags & rmax::AnimTrackFlags_t::ANIM_TRACK_SCL)
			{
				std::memcpy(localPointer, scale, sizeof(Vector));
				localPointer += sizeof(Vector);
			}
		}

		uint16_t flags;
		uint16_t boneId; // index into this rmax's skeleton

		char* data;
	};

	constexpr size_t maxFrameSize = s_FrameSizeLUT[AnimTrackFlags_t::ANIM_TRACK_DATA];

	struct RMAXAnim
	{
	public:
		RMAXAnim(RMAXExporter* baseIn, const char* nameIn, const uint16_t frameCountIn, const float frameRateIn, const uint16_t flagsIn, const size_t boneCount) : base(baseIn), name(nameIn), frameCount(frameCountIn), frameRate(frameRateIn), flags(flagsIn), trackCount(boneCount)
		{
			tracks = new RMAXAnimTrack[trackCount];
			trackData = new char[(maxFrameSize * frameCount) * trackCount];
		};

		const uint16_t GetFlags() const { return flags; };
		RMAXAnimTrack* const GetTrack(const size_t index) const { return &tracks[index]; };

		void SetTrack(const uint16_t flagsIn, const uint16_t boneIdIn) { tracks[boneIdIn] = RMAXAnimTrack(flagsIn, boneIdIn, trackData + ((maxFrameSize * frameCount) * boneIdIn)); }

		RMAXExporter* base;

		const char* name;

		uint16_t flags;

		uint16_t frameCount;
		float frameRate;

		size_t trackCount;
		RMAXAnimTrack* tracks;
		char* trackData;
	};

	class RMAXExporter
	{
	public:
		RMAXExporter(const std::filesystem::path& pathIn, const char* const nameIn, const char* const nameSkelIn) : path(pathIn), name(nameIn), nameSkel(nameSkelIn) {};

		// to reduce allocations, not required for proper function
		inline void ReserveBones(const size_t count) { bones.reserve(count); };
		inline void ReserveMaterials(const size_t count) { materials.reserve(count); };
		inline void ReserveCollections(const size_t count) { collections.reserve(count); };
		inline void ReserveMeshes(const size_t count) { meshes.reserve(count); };
		void ReserveVertices(const size_t count, const int16_t maxTexcoords, const int16_t maxWeights);
		inline void ReserveIndices(const size_t count) { indices.reserve(count); };

		inline void AddBone(const char* nameIn, const int parentIn, const Vector& posIn, const Quaternion& quatIn, const Vector& scaleIn) { bones.emplace_back(nameIn, parentIn, posIn, quatIn, scaleIn); };
		inline void AddMaterial(const char* nameIn) { materials.emplace_back(nameIn); };
		inline void AddCollection(const char* nameIn, const int meshCountIn) { collections.emplace_back(nameIn, meshCountIn); };
		void AddMesh(const int16_t collectionIndex, const int16_t materialIndex, const int16_t uvCount, const int16_t uvIndices, const bool useColor);

		inline void AddVertex(const Vector& pos, const Vector& norm) { vertices.emplace_back(pos, norm); };
		inline void AddColor(const Color32 color) { colors.push_back(color); };
		inline void AddTexcood(const Vector2D& texcoord) { texcoords.push_back(texcoord); };
		inline void AddIndice(const uint16_t a, const uint16_t b, const uint16_t c, const uint16_t d) { indices.emplace_back(a, b, c, d); };
		inline void AddWeight(const int bone, const float weight) { weights.emplace_back(bone, weight); };

		inline void AddAnim(const char* nameIn, const uint16_t frameCountIn, const float frameRateIn, const uint16_t flagsIn, const size_t boneCountIn) { animations.emplace_back(this, nameIn, frameCountIn, frameRateIn, flagsIn, boneCountIn); };

		inline RMAXMaterial* const GetMaterialLast() { return &materials.back(); };
		inline RMAXMesh* const GetMeshLast() { return &meshes.back(); };
		inline Vertex_t* const GetVertexLast() { return &vertices.back(); };
		inline RMAXAnim* const GetAnimLast() { return &animations.back(); };

		inline Vertex_t* const GetVertex(const size_t index) { return &vertices.at(index); };

		inline const size_t CollectionCount() const { return collections.size(); };
		inline const size_t WeightCount() const { return weights.size(); };

		bool ToFile() const;

	private:
		std::filesystem::path path;

		const char* const name;
		const char* const nameSkel; // name of the parent skeleton

		// can we get uhhhhhh custom containter class ?
		std::vector<RMAXBone> bones;

		std::vector<RMAXMaterial> materials;

		std::vector<RMAXCollection> collections;
		std::vector<RMAXMesh> meshes;

		std::vector<Vertex_t> vertices;
		std::vector<Color32> colors;
		std::vector<Vector2D> texcoords;
		std::vector<Weight_t> weights;
		std::vector<Indice_t> indices;

		std::vector<RMAXAnim> animations;
	};
}