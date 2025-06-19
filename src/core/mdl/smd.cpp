#include <pch.h>

#include <core/mdl/smd.h>

namespace smd
{
	void CSourceModelData::InitNode(const char* name, const int index, const int parent) const
	{
		Node& node = nodes[index];

		// [rika]: node has already been set
		if (node.name)
		{
			assertm(false, "should not be reinitalizing a node");
			return;
		}

		node.name = name;
		node.index = index;
		node.parent = parent;
	}

	void CSourceModelData::InitFrameBone(const int iframe, const int ibone, const Vector& pos, const RadianEuler& rot) const
	{
		assertm(iframe < numFrames, "frame out of range");
		assertm(ibone < numNodes, "node out of range");

		Frame* const frame = &frames[iframe];

		const int boneCount = static_cast<int>(frame->bones.size());

		if (ibone == boneCount)
		{
			frame->bones.emplace_back(ibone, pos, rot);

			return;
		}

		if (ibone < boneCount)
		{
			Bone& bone = frame->bones.at(ibone);

			bone.node = ibone;
			bone.pos = pos;
			bone.rot = rot;

			return;
		}

		assertm(false, "bones added out of order or not pre-sized");
	}

	// [rika]: this does not round floats to 6 decimal points as required by SMD, HOWEVER studiomdl supports scientific notation so we should be ok! 
	// [rika]: if this causes any weird issues in tthe future such as messed up skeletons we can change it
	void CSourceModelData::Write() const
	{
		std::filesystem::path outPath(exportPath);
		outPath.append(exportName);
		outPath.replace_extension(".smd");

		std::ofstream out(outPath, std::ios::out);

		out << "version 1\n";

		out << "nodes\n";
		for (size_t i = 0; i < numNodes; i++)
		{
			const Node& node = nodes[i];

			out << "\t" << node.index << " \"" << node.name << "\" " << node.parent << "\n";
		}
		out << "end\n";

		out << "skeleton\n";
		for (size_t iframe = 0; iframe < numFrames; iframe++)
		{
			const Frame& frame = frames[iframe];

			out << "\ttime " << iframe << "\n";

			for (const Bone& bone : frame.bones)
			{
				out << "\t\t" << bone.node << " ";
				out << bone.pos.x << " " << bone.pos.y << " " << bone.pos.z << " ";
				out << bone.rot.x << " " << bone.rot.y << " " << bone.rot.z << "\n";
			}
		}
		out << "end\n";

		if (triangles.size())
		{
			out << "triangles\n";
			for (size_t itriangle = 0; itriangle < triangles.size(); itriangle++)
			{
				const Triangle& triangle = triangles[itriangle];

				out << "" << triangle.material << "\n";

				for (int vertIdx = 0; vertIdx < 3; vertIdx++)
				{
					const Vertex& vert = triangle.vertices[vertIdx];

					out << "\t" << vert.bone[0] << " ";
					out << vert.position.x << " " << vert.position.y << " " << vert.position.z << " ";
					out << vert.normal.x << " " << vert.normal.y << " " << vert.normal.z << " ";
					out << vert.texcoords[0].x << " " << vert.texcoords[0].y << " " << vert.numBones << " ";

					for (int weightIdx = 0; weightIdx < vert.numBones; weightIdx++)
						out << vert.bone[weightIdx] << " " << vert.weight[weightIdx] << " ";

					out << "\n";
				}
			}
			out << "end\n";
		}

		out.close();
	}
}