#pragma once

/* CAST */
namespace cast
{
	enum class CastPropertyId : uint16_t
	{
		Byte = 'b',			// <uint8_t>
		Short = 'h',		// <uint16_t>
		Integer32 = 'i',	// <uint32_t>
		Integer64 = 'l',	// <uint64_t>

		Float = 'f',		// <float>
		Double = 'd',		// <double>

		String = 's',		// Null terminated UTF-8 string

		Vector2 = 'v2',		// Float precision vector XY
		Vector3 = 'v3',		// Float precision vector XYZ
		Vector4 = 'v4',		// Float precision vector XYZW

		_Count = 10,
	};

	// the size of each data type
	static const std::map<CastPropertyId, size_t> s_CastPropertySize
	{
		{ CastPropertyId::Byte, sizeof(uint8_t) },
		{ CastPropertyId::Short, sizeof(uint16_t) },
		{ CastPropertyId::Integer32, sizeof(uint32_t) },
		{ CastPropertyId::Integer64, sizeof(uint64_t) },
		{ CastPropertyId::Float, sizeof(float) },
		{ CastPropertyId::Double, sizeof(double) },
		{ CastPropertyId::String, 0 },
		{ CastPropertyId::Vector2, sizeof(Vector2D) },
		{ CastPropertyId::Vector3, sizeof(Vector) },
		{ CastPropertyId::Vector4, sizeof(Vector4D) },
	};

	struct CastPropertyHeader
	{
		CastPropertyId Identifier;	// The element type of this property
		uint16_t NameSize;			// The size of the name of this property
		uint32_t ArrayLength;		// The number of elements this property contains (1 for single)
	};

	enum class CastId : uint32_t
	{
		Root = 0x746F6F72,
		Model = 0x6C646F6D,
		Mesh = 0x6873656D,
		BlendShape = 0x68736C62, // unneeded
		Skeleton = 0x6C656B73,
		Bone = 0x656E6F62,
		IKHandle = 0x64686B69, // unneeded
		Constraint = 0x74736E63, // unneeded
		Animation = 0x6D696E61,
		Curve = 0x76727563,
		NotificationTrack = 0x6669746E,
		Material = 0x6C74616D,
		File = 0x656C6966,
		Instance = 0x74736E69, // unneeded

		_Count = 14,
	};

	struct CastNodeHeader
	{
		CastId Identifier;		// Used to signify which class this node uses
		uint32_t NodeSize;		// Size of all data and sub data following the node
		uint64_t NodeHash;		// Unique hash, like an id, used to link nodes together
		uint32_t PropertyCount;	// The count of properties
		uint32_t ChildCount;	// The count of direct children nodes

		// We must read until the node size hits, and that means we are done.
		// The nodes are in a stack layout, so it's easy to load, FILO order.
	};

	constexpr int castFileId = MAKEFOURCC('c', 'a', 's', 't');
	constexpr int castFileVersion = 1;
	constexpr size_t castFileSize = 1024 * 1024 * 32; // completely fake

	struct CastHeader
	{
		uint32_t Magic;			// char[4] cast	(0x74736163)
		uint32_t Version;		// 0x1
		uint32_t RootNodes;		// Number of root nodes, which contain various sub nodes if necessary
		uint32_t Flags;			// Reserved for flags, or padding, whichever is needed
	};

	// export
	enum class eCastPropertyStoreType : uint8_t
	{
		RAW,
		PTR,
		VECTOR,
		ALLOC, // allocated memory
	};

	class CastProperty
	{
	public:
		CastProperty() : propertyId(cast::CastPropertyId::_Count), nodeId(CastId::_Count), propertyNameIndex(-1),
			arraySize(0), raw(0ull), storeType(eCastPropertyStoreType::RAW), formatName(false), formatIndex(0) {};

		// copy constructor
		CastProperty(CastProperty& propIn);
		CastProperty(const CastProperty& propIn);

		// init with values
		CastProperty(const CastPropertyId idIn, const CastId nodeIdIn, const int propNameIn, const size_t rawIn, const bool strFormat = false, const uint16_t strIndex = 0xffff) : propertyId(idIn), nodeId(nodeIdIn), propertyNameIndex(propNameIn),
			arraySize(1), raw(rawIn), storeType(eCastPropertyStoreType::RAW), formatName(strFormat), formatIndex(strIndex) {};
		CastProperty(const CastPropertyId idIn, const CastId nodeIdIn, const int propNameIn, const void* ptrIn, const uint32_t sizeIn, const bool strFormat = false, const uint16_t strIndex = 0xffff) : propertyId(idIn), nodeId(nodeIdIn), propertyNameIndex(propNameIn),
			arraySize(sizeIn), ptr(ptrIn), storeType(eCastPropertyStoreType::PTR), formatName(strFormat), formatIndex(strIndex) {};
		CastProperty(const CastPropertyId idIn, const CastId nodeIdIn, const int propNameIn, std::vector<const void*>& vectorIn) : propertyId(idIn), nodeId(nodeIdIn), propertyNameIndex(propNameIn), arraySize(static_cast<uint32_t>(vectorIn.size())),
			vector(std::move(vectorIn)), storeType(eCastPropertyStoreType::VECTOR), raw(0ull), formatName(false), formatIndex(0xffff) {};
		CastProperty(const CastPropertyId idIn, const CastId nodeIdIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn, const bool strFormat = false, const uint16_t strIndex = 0xffff);

		~CastProperty();

		const uint32_t Size() const;
		char* Write(char* buf) const;

		void Name(char* nameBuf, uint16_t& nameSize) const;
		void Name(uint16_t& nameSize) const;

		inline void EditValue(const uint32_t idx, void* valIn)
		{
			assertm(storeType == eCastPropertyStoreType::VECTOR, "value edit was used inproperly");
			assertm(idx < arraySize, "value index was out of range");
			vector.at(idx) = valIn;
		}

		inline void* GetAllocPtr() { return alloc; };
		inline void EditAllocPtr(const size_t idx, void* value)
		{
			const size_t size = s_CastPropertySize.find(propertyId)->second;
			memcpy((char*)alloc + (size * idx), value, s_CastPropertySize.find(propertyId)->second);
		}

		// get the min size we can fit a number in
		static inline CastPropertyId ValueMinSize(const size_t value)
		{
			if (value < 0x100)
				return CastPropertyId::Byte;

			if (value < 0x10000)
				return CastPropertyId::Short;

			if (value < 0x100000000)
				return CastPropertyId::Integer32;

			return CastPropertyId::Integer64;
		};

	private:
		CastPropertyId propertyId;	// The element type of this property
		CastId nodeId;
		int propertyNameIndex;

		bool formatName;
		uint16_t formatIndex;

		// pointer to type of identifier
		eCastPropertyStoreType storeType; // how is this data stored?

		uint32_t arraySize; //

		std::vector<const void*> vector; // an array of pointers to raw data

		union {
			size_t raw;
			const void* ptr; // an array of raw data
			void* alloc;
		};
		
	};

	class CastNode
	{
	public:
		CastNode() : nodeId(CastId::_Count), hash(0ull) {};

		// basic construction
		CastNode(const CastId idIn) : nodeId(idIn), hash(0ull) {};
		CastNode(const CastId idIn, const uint64_t hashIn) : nodeId(idIn), hash(hashIn) {};
		CastNode(const CastId idIn, const uint64_t hashIn, const size_t childCount);

		// copy constructors
		CastNode(CastNode& node) : nodeId(node.nodeId), str(std::move(node.str)), hash(node.hash), children(std::move(node.children)), properties(std::move(node.properties)) {};
		CastNode(const CastNode& node) : nodeId(node.nodeId), str(node.str), hash(node.hash), children(node.children), properties(node.properties) {};
		
		// init with properties
		CastNode(const CastId idIn, const int propertyCount) : nodeId(idIn), hash(0ull) { properties.resize(propertyCount); };
		CastNode(const CastId idIn, const int propertyCount, const uint64_t hashIn) : nodeId(idIn), hash(hashIn) { properties.resize(propertyCount); };

		~CastNode() {};

		const uint32_t Size() const;
		char* Write(char* buf) const;

		inline const uint64_t GetHash() const { return hash; };
		static CastNode* GetChild(const uint64_t hashIn, std::vector<CastNode>& nodes);
		CastNode* GetChild(const uint64_t hashIn);
		CastNode* GetChild(const int idx) { return &children.at(idx); };
		const CastId GetId() const { return nodeId; };

		CastNode* AddChild(const CastId idIn, const uint64_t hashIn);
		CastNode* AddChild(const CastId idIn, const uint64_t hashIn, const size_t childCount);
		CastNode* AddChild(CastNode& node); // move the provided node

		void SetChild(const uint64_t hashIn, CastNode& node);
		void SetChild(const int idx, CastNode& node);

		void Set(CastNode& node);

		void SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const size_t rawIn);
		void SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const void* ptrIn, const uint32_t sizeIn); // ptr
		void SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn); // alloc
		void SetProperty(const size_t idx, const CastPropertyId idIn, const int propNameIn, std::vector<const void*>& vectorIn);

		CastProperty* AddProperty(const CastPropertyId idIn, const int propNameIn, const size_t rawIn, const bool strFormat = false, const uint16_t strIndex = 0xffff);
		CastProperty* AddProperty(const CastPropertyId idIn, const int propNameIn, const void* ptrIn, const uint32_t sizeIn, const bool strFormat = false, const uint16_t strIndex = 0xffff);
		CastProperty* AddProperty(const CastPropertyId idIn, const int propNameIn, const uint32_t sizeIn, const void* ptrIn, const bool strFormat = false, const uint16_t strIndex = 0xffff);
		CastProperty* AddProperty(const CastPropertyId idIn, const int propNameIn, std::vector<const void*>& vectorIn);

		void SetString(const std::string& strIn) { str = std::move(strIn); };
		const char* GetString() const { return str.c_str(); };

		// reserve to prevent allocations
		void ReserveChildren(const size_t count) { children.reserve(count); }
		void ReserveProperties(const size_t count) { properties.reserve(count); }


	private:
		CastId nodeId;		// Used to signify which class this node uses
		uint64_t hash;		// hash of name
		std::string str;	// container for string data

		// temp
		std::vector<CastNode> children;
		std::vector<CastProperty> properties;
	};

	// 'Root'
	// parent:
	// children: 'Model',

	// 'Model'
	// parent: 'Root'
	// children: 'Skeleton', 'Mesh', 'Material'
	enum class CastPropsModel : int
	{
		Name, // str

		_Count,
	};

	static constexpr const char* s_CastPropsModel[static_cast<int>(CastPropsModel::_Count)] = {
		"n",
	};

	// 'Mesh'
	// parent: 'Model'
	// children:
	enum class CastPropsMesh : int
	{
		Name, // str
		Vertex_Postion_Buffer, // vec3 array
		Vertex_Normal_Buffer, // vec3 array
		Vertex_Tangent_Buffer, // vec3 array
		Vertex_Color_Buffer, // i32 array
		Vertex_UV_Buffer, // vec2 array
		Vertex_Weight_Bone_Buffer, // // i32, i16, i8 array
		Vertex_Weight_Value_Buffer, // f array
		Face_Buffer, // i32, i16, i8 array
		UV_Layer_Count, // i32, i16, i8
		Max_Weight_Influence, // i32, i16, i8
		Skinning_Method, // str [linear, quaternion]
		Material, // i64 (CastNode:Material)

		_Count,
	};

	static constexpr const char* s_CastPropsMesh[static_cast<int>(CastPropsMesh::_Count)] = {
		"n",
		"vp",
		"vn",
		"vt",
		"vc",
		"u%d", // formatted with index
		"wb",
		"wv",
		"f",
		"ul",
		"mi",
		"sm",
		"m",
	};

	// 'BlendShape'
	// parent: 'Model'
	// children:
	enum class CastPropsBlendShape : int
	{
		Name, // str
		Blend_Shape, // i64 (CastNode:Mesh)
		Target_Shapes, // i64 array (CastNode:Mesh)
		Target_Weight_Scales, // float array

		_Count,
	};

	static constexpr const char* s_CastPropsBlendShape[static_cast<int>(CastPropsBlendShape::_Count)] = {
		"n",
		"b",
		"t",
		"ts",
	};

	// 'Skeleton'
	// parent: 'Model'
	// children: 'Bone', 'IKHandle', 'Constraint'

	// 'Bone'
	// parent: 'Skeleton', 'Animation'
	// children:
	enum class CastPropsBone : int
	{
		Name, // str
		Parent_Index, // i32
		Segment_Scale, // i8
		Local_Position, // vec3
		Local_Rotation, // vec4
		World_Position, // vec3
		World_Rotation, // vec4
		Scale, // vec3

		_Count,
	};

	static constexpr const char* s_CastPropsBone[static_cast<int>(CastPropsBone::_Count)] = {
		"n",
		"p",
		"ssc",
		"lp",
		"lr",
		"wp",
		"wr",
		"s",
	};

	class CastNodeBone
	{
	public:
		CastNodeBone(CastNode* base) : skel(base), bone(nullptr)
		{
			//assertm(nullptr != skel, "Node cannot be a null pointer");
			//assertm(CastId::Skeleton == skel->GetId(), "Node cannot be a type other than 'Skeleton'");
		};

		CastNode* skel;
		CastNode* bone;

		void MakeBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, bool isGlobal);
		void MakeBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale, bool isGlobal);

	private:
		inline void MakeNameParent(const char* name, const int parent);
		inline void MakeLocalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q);
		inline void MakeLocalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale);
		inline void MakeGlobalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q);
		inline void MakeGlobalBone(const char* name, const int parent, const Vector* pos, const Quaternion* q, const Vector* scale);
	};

	// 'IKHandle'

	// 'Constraint'

	// 'Material'
	// parent: 'Model'
	// children: 'File(s)'
	enum class CastPropsMaterial : int
	{
		Name, // str
		Type, // str
		Albedo_File_Hash, // i64
		Diffuse_File_Hash, // i64
		Normal_File_Hash, // i64
		Specular_File_Hash, // i64
		Emissive_File_Hash, // i64
		Gloss_File_Hash, // i64
		Roughness_File_Hash, // i64
		Ambient_Occlusion_File_Hash, // i64
		Cavity_File_Hash, // i64
		Extra_File_Hash, // i64

		_Count,
	};

	static std::map<std::string, CastPropsMaterial> s_TextureTypeMap = {
		{ "albedoTexture",		CastPropsMaterial::Albedo_File_Hash },
		{ "diffuseTexture",		CastPropsMaterial::Diffuse_File_Hash }, // worth looking into what the color texture in r1 is called, maybe diffuse?
		{ "normalTexture",		CastPropsMaterial::Normal_File_Hash },
		{ "glossTexture",		CastPropsMaterial::Gloss_File_Hash },
		{ "roughnessTexture",	CastPropsMaterial::Roughness_File_Hash },
		{ "specTexture",		CastPropsMaterial::Specular_File_Hash },
		{ "emissiveTexture",	CastPropsMaterial::Emissive_File_Hash },
		{ "aoTexture",			CastPropsMaterial::Ambient_Occlusion_File_Hash },
		{ "cavityTexture",		CastPropsMaterial::Cavity_File_Hash },
	};

	static constexpr const char* s_CastPropsMaterial[static_cast<int>(CastPropsMaterial::_Count)] = {
		"n",
		"t",
		"albedo",
		"diffuse",
		"normal",
		"specular",
		"emissive",
		"gloss",
		"roughness",
		"ao",
		"cavity",
		"extra%d", // formatted with index
	};

	// 'File'
	// parent: 'CastNode'
	// children:
	enum class CastPropsFile : int
	{
		Path, // str

		_Count,
	};

	static constexpr const char* const s_CastPropsFile[static_cast<int>(CastPropsFile::_Count)] = {
		"p",
	};

	// 'Animation'
	// parent: 'Root'
	// children: 'Skeleton', 'Curve', 'NotificationTrack'
	enum class CastPropsAnimation : int
	{
		Name, // str
		Framerate, // float
		Looping, // i8

		_Count,
	};

	static constexpr const char* const s_CastPropsAnimation[static_cast<int>(CastPropsAnimation::_Count)] = {
		"n",
		"fr",
		"lo",
	};

	// 'Curve'
	// parent: 'Animation'
	// children:
	enum class CastPropsCurve : int
	{
		Node_Name, // str
		Key_Property_Name, // str [rq, tx, ty, tz, sx, sy, sz, vb]
		Key_Frame_Buffer, // i8, i16, i32
		Key_Value_Buffer, // i8, i16, i32, float, vec4
		Mode, // str [additive, absolute, relative]
		Additive_Blend_Weight, // float

		_Count,
	};

	static constexpr const char* const s_CastPropsCurve[static_cast<int>(CastPropsCurve::_Count)] = {
		"nn",
		"kp",
		"kb",
		"kv",
		"m",
		"ab",
	};

	enum class CastPropsCurveMode : int
	{
		MODE_ADDITIVE,
		MODE_ABSOLUTE,
		MODE_RELATIVE,

		_Count,
	};

	static constexpr const char* const s_CastPropsCurveMode[static_cast<int>(CastPropsCurveMode::_Count)] = {
		"additive",
		"absolute",
		"relative",
	};

	enum class CastPropsCurveValue : int
	{
		ROT_QUAT,
		POS_X,
		POS_Y,
		POS_Z,
		SCL_X,
		SCL_Y,
		SCL_Z,
		VIS,

		_Count,
	};

	static constexpr const char* const s_CastPropsCurveValue[static_cast<int>(CastPropsCurveValue::_Count)] = {
		"rq",
		"tx",
		"ty",
		"tz",
		"sx",
		"sy",
		"sz",
		"vb",
	};

	class CastNodeCurve
	{
	public:
		CastNodeCurve(CastNode* base) : anim(base), curve(nullptr) {};

		CastNode* anim;
		CastNode* curve;

		void MakeCurveQuaternion(const char* name, const Quaternion* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveMode mode, const float weight = 1.0f);
		void MakeCurveQuaternion(const char* name, const Quaternion* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveMode mode, const float weight = 1.0f);
		void MakeCurveFloat(const char* name, const float* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveValue type, const CastPropsCurveMode mode, const float weight = 1.0f);
		void MakeCurveFloat(const char* name, const float* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveValue type, const CastPropsCurveMode mode, const float weight = 1.0f);
		void MakeCurveVector(const char* name, const Vector* const track, const size_t trackLength, const size_t numFrames, const CastPropsCurveValue type, const CastPropsCurveMode mode, const float weight = 1.0f);
		void MakeCurveVector(const char* name, const Vector* const track, const size_t trackLength, const void* frameBuf, const size_t numFrames, const CastPropsCurveValue type, const CastPropsCurveMode mode, const float weight = 1.0f);

		static void* MakeCurveKeyFrameBuffer(const size_t numFrames, CastPropertyId& propType);
		template<class T> static inline void MakeCurveKeyFrameBuffer(void* bufPtr, const size_t numFrames)
		{
			T* frameIndices = reinterpret_cast<T*>(bufPtr);

			for (size_t i = 0; i < numFrames; i++)
				frameIndices[i] = static_cast<T>(i);
		}

	private:
		inline void MakeCurveName(const char* name, const CastPropsCurveValue type);
		inline void MakeCurveKeyFrames(const size_t numFrames);
		inline void MakeCurveKeyFrames(const void* frameBuf, const size_t numFrames);
		template<class PropType> inline void MakeCurveKeyValues(const PropType* const track, const size_t trackLength, const size_t numFrames, const CastPropertyId propId);
		inline void MakeCurveMode(const CastPropsCurveMode mode, const float weight);
		
	};

	// 'NotificationTrack'
	// parent: 'Animation'
	// children:
	enum class CastPropsNotificationTrack : int
	{
		Name, // str
		Key_Frame_Buffer, // i8, i16, i32

		_Count,
	};

	static constexpr const char* const s_CastPropsNotificationTrack[static_cast<int>(CastPropsNotificationTrack::_Count)] = {
		"n",
		"kb",
	};

	// 'Instance'
	// parent: 'Root'
	// children 'File'
	enum class CastPropsInstance : int
	{
		Name, // str
		Reference_File, // i64
		Position, // vec3
		Rotation, // vec4
		Scale, // vec3

		_Count,
	};

	static constexpr const char* const s_CastPropsInstance[static_cast<int>(CastPropsInstance::_Count)] = {
		"n",
		"rf",
		"p",
		"r",
		"s",
	};

	static const std::map<CastId, const char* const* const> s_CastPropListMap = {
		{ CastId::Root,					nullptr },
		{ CastId::Model,				s_CastPropsModel },
		{ CastId::Mesh,					s_CastPropsMesh },
		{ CastId::BlendShape,			s_CastPropsBlendShape },
		{ CastId::Skeleton,				nullptr },
		{ CastId::Bone,					s_CastPropsBone },
		{ CastId::IKHandle,				nullptr },
		{ CastId::Constraint,			nullptr },
		{ CastId::Material,				s_CastPropsMaterial },
		{ CastId::File,					s_CastPropsFile },
		{ CastId::Animation,			s_CastPropsAnimation },
		{ CastId::Curve,				s_CastPropsCurve },
		{ CastId::NotificationTrack,	s_CastPropsNotificationTrack },
		{ CastId::Instance,				s_CastPropsInstance },
	};

	class CastExporter
	{
	public:
		CastExporter(const std::filesystem::path& pathIn) : path(pathIn)
		{
			rootNodes.emplace_back(CastId::Root);
		};

		CastNode* GetChild(const uint64_t hash);
		CastNode* GetChild(const int idx) { return &rootNodes.at(idx); };

		void ToFile() const;

	private:
		std::filesystem::path path;

		std::vector<CastNode> rootNodes;
	};
}

#define FLOAT_AS_UINT(fl) *reinterpret_cast<const uint32_t*>(&fl)