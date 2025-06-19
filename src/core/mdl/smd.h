
// Source Model Data
namespace smd
{
	constexpr int maxBoneWeights = 3;
	constexpr int maxTexcoords = 1;

	class Node
	{
	public:
		Node() : name(nullptr), index(-1), parent(-1) {};

		const char* name;
		int index;
		int parent;
	};

	class Bone
	{
	public:
		Bone(const int nodeid, const Vector& position, const RadianEuler& rotation) : node(nodeid), pos(position), rot(rotation) {}

		int node;

		Vector pos;
		RadianEuler rot;
	};

	class Frame
	{
	public:
		std::vector<Bone> bones;
	};

	class Vertex
	{
	public:
		Vector position;
		Vector normal;

		Vector2D texcoords[maxTexcoords];

		float weight[maxBoneWeights];
		int bone[maxBoneWeights];
		int numBones;
	};

	class Triangle
	{
	public:
		Triangle(const char* mat) : material(mat), vertices() {}

		const char* material;
		Vertex vertices[3];
	};

	class CSourceModelData
	{
	public:
		CSourceModelData(const std::filesystem::path& path, const size_t nodeCount, const size_t frameCount) : exportPath(path), numNodes(nodeCount), nodes(nullptr), numFrames(frameCount), frames(nullptr)
		{
			assertm(numNodes, "must have at least one bone");
			assertm(numFrames, "must have at least one frame");

			nodes = new Node[numNodes];
			frames = new Frame[numFrames];

			for (size_t i = 0; i < numFrames; i++)
			{
				Frame& frame = frames[i];
				frame.bones.reserve(numNodes);
			}
		}

		~CSourceModelData()
		{
			FreeAllocArray(nodes);
			FreeAllocArray(frames);
		}

		void InitNode(const char* name, const int index, const int parent) const;
		void InitFrameBone(const int iframe, const int ibone, const Vector& pos, const RadianEuler& rot) const;
		void InitTriangle(const char* mat) { triangles.emplace_back(mat); }

		void AddTriangles(const size_t size) { triangles.reserve(triangles.size() + size); }
		Triangle* const TopTri() { return &triangles.back(); }

		void SetName(const std::string& name) { exportName = name; }

		const size_t NodeCount() const { return numNodes; }
		const size_t FrameCount() const { return numFrames; }

		// so we don't have to re parse nodes
		void ResetMeshData() { triangles.clear(); }
		void ResetFrameData(const size_t frameCount)
		{
			FreeAllocArray(frames);

			numFrames = frameCount;
			frames = new Frame[numFrames];
		}

		void Write() const;

	private:
		size_t numNodes;
		Node* nodes;

		size_t numFrames;
		Frame* frames;

		std::vector<Triangle> triangles;

		std::filesystem::path exportPath;
		std::string exportName;
	};
}