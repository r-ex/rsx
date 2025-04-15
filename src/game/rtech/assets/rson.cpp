#include <pch.h>
#include <game/rtech/assets/rson.h>
#include <game/rtech/cpakfile.h>
#include <thirdparty/imgui/imgui.h>

extern ExportSettings_t g_ExportSettings;

void LoadRSONAsset(CAssetContainer* const pak, CAsset* const asset)
{
    UNUSED(pak);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    RSONAsset* rsonAsset = nullptr;

    switch (pakAsset->version())
    {
    case 1:
    {
        RSONAssetHeader_v1_t* hdr = reinterpret_cast<RSONAssetHeader_v1_t*>(pakAsset->header());
        rsonAsset = new RSONAsset(hdr);
        break;
    }
    default:
        return;
    }

    std::stringstream out;
    RSONAssetNode_t rootNode(rsonAsset);
    rootNode.R_ParseNodeValues(out, 0); // store raw text so we can preview or export it
    rsonAsset->rawText = out.str();

    pakAsset->setExtraData(rsonAsset);
}

void* PreviewRSONAsset(CAsset* const asset, const bool firstFrameForAsset)
{
    UNUSED(firstFrameForAsset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const RSONAsset* const rsonAsset = reinterpret_cast<RSONAsset*>(pakAsset->extraData());
    assertm(rsonAsset, "Extra data should be valid at this point.");

    ImGui::Text("Root node type: 0x%x", rsonAsset->type);

    if (ImGui::BeginChild("RSON Preview", ImVec2(-1, -1), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        ImGui::TextUnformatted(rsonAsset->rawText.c_str());
    }
    ImGui::EndChild();

    return nullptr;
}

static const char* const s_PathPrefixRSON = s_AssetTypePaths.find(AssetType_t::RSON)->second;
bool ExportRSONAsset(CAsset* const asset, const int setting)
{
    UNUSED(setting);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

    const RSONAsset* const rsonAsset = reinterpret_cast<RSONAsset*>(pakAsset->extraData());
    assertm(rsonAsset, "Extra data should be valid at this point.");

    // Create exported path + asset path.
	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path rsonPath(asset->GetAssetName());

	if (g_ExportSettings.exportPathsFull)
		exportPath.append(rsonPath.parent_path().string());
	else
		exportPath.append(s_PathPrefixRSON);

    if (!CreateDirectories(exportPath))
    {
        assertm(false, "Failed to create asset's directory.");
        return false;
    }
    
    exportPath.append(rsonPath.filename().string());
    exportPath.replace_extension(".rson");

    StreamIO out;
    if (!out.open(exportPath.string(), eStreamIOMode::Write))
    {
        assertm(false, "Failed to open file for write.");
        return false;
    }

    out.write(rsonAsset->rawText.c_str(), rsonAsset->rawText.length());
    out.close();

    return true;
}

void InitRSONAssetType()
{
    AssetTypeBinding_t type =
    {
		.type = 'nosr',
		.headerAlignment = 8,
		.loadFunc = LoadRSONAsset,
		.postLoadFunc = nullptr,
		.previewFunc = PreviewRSONAsset,
		.e = { ExportRSONAsset, 0, nullptr, 0ull },
    };

    REGISTER_TYPE(type);
}

// recursively parses all node values out of the rson node tree into our stringstream
void RSONAssetNode_t::R_ParseNodeValues(std::stringstream& out, const size_t indentIdx) const
{
	const std::string indentation = GetIndentation(indentIdx);

	if (this->name)
		out << indentation << this->name << ":";

	if (this->IsArray())
	{
		// Pointer to this node's array elements
		const RSONNodeValue_t* const nodeValues = reinterpret_cast<RSONNodeValue_t*>(values.valPtr);

		// no random line for root
		if (!IsRoot())
			out << "\n";

		// Non-root arrays always have square brackets.
		// Root arrays of objects do not typically have square brackets
		const bool isSquareBracketedArray = !IsRoot() || !IsArrayOfObjects();
		if(isSquareBracketedArray)
			out << indentation << "[\n";

		for (int i = 0; i < this->valueCount; i++)
		{
			const RSONNodeValue_t& val = nodeValues[i];

			// special case for arrays of objects since running it through WriteNodeValue will not give a desired output
			if (!IsArrayOfObjects())
			{
				out << indentation << "\t"; // [rika]: set our indentation for array values, do after we've checked if it's nodes, as we increase the indentation there already.

				R_WriteNodeValue(out, val, indentation, indentIdx + 1);
			}
			else
			{
				out << indentation << "{\n";
				// [rika]: skipping nullptrs for objects only, don't want to run ParseNodeValues on a null pointer
				if (val.valPtr == nullptr)
				{
					out << indentation << "}\n";
					continue;
				}

				const RSONAssetNode_t* curNode = reinterpret_cast<RSONAssetNode_t*>(val.valPtr);

				while (true)
				{
					curNode->R_ParseNodeValues(out, indentIdx + 2);

					if (!curNode->nextPeer)
						break;

					curNode = curNode->nextPeer;
				}

				out << indentation << "}\n";
			}
		}

		if(isSquareBracketedArray)
			out << indentation << "]\n";

		return;
	}

	// no random space for root
	if (this->name)
		out << " ";

	R_WriteNodeValue(out, values, indentation, indentIdx);
}

void RSONAssetNode_t::R_WriteNodeValue(std::stringstream& out, RSONNodeValue_t val, const std::string& indentation, const size_t indentIdx) const
{
	switch (this->type & 0x1ff)
	{
	case eRSONFieldType::RSON_NULL:
	{
		out << "null\n";

		return; // only needs to be parsed on export
	}
	case eRSONFieldType::RSON_STRING:
	{
		out << val.string << "\n";

		return;
	}
	case eRSONFieldType::RSON_VALUE: // what
	{
		assertm(false, "rson type RSON_VALUE used\n");

		out << "\n";

		return;
	}
	case eRSONFieldType::RSON_OBJECT:
	{
		const RSONAssetNode_t* const nodes = reinterpret_cast<RSONAssetNode_t*>(val.valPtr);

		// no random line if root
		if (this->name)
			out << "\n";

		out << indentation << "{\n";

		for (int i = 0; i < this->valueCount; i++)
		{
			if (!nodes)
				break;

			const RSONAssetNode_t* curNode = &nodes[i];

			while (true)
			{
				curNode->R_ParseNodeValues(out, indentIdx + 1);

				if (!curNode->nextPeer)
					break;

				curNode = curNode->nextPeer;
			}
		}

		out << indentation << "}\n";

		return;
	}
	case eRSONFieldType::RSON_BOOLEAN:
	{
		out << std::boolalpha << val.valueBool << "\n";
		return;
	}
	case eRSONFieldType::RSON_INTEGER:
	case eRSONFieldType::RSON_SIGNED_INTEGER:
	{
		out << static_cast<int64_t>(val.value) << "\n";
		return;
	}
	case eRSONFieldType::RSON_UNSIGNED_INTEGER:
	{
		out << val.value << "\n";
		return;
	}
	case eRSONFieldType::RSON_DOUBLE:
	{
		out << val.valueFP << "\n";
		return;
	}
	default:
	{
		assertm(0, "invalid RSON node type hit\n");
		return;
	}
	}
}

void WriteRSONDependencyArray(std::ofstream& out, const char* const name, const AssetGuid_t* const dependencies, const int count)
{
    out << name << ":\n[\n";

    for (int i = 0; i < count; i++)
    {
		if (!dependencies)
			break;

        CPakAsset* const asset = g_assetData.FindAssetByGUID<CPakAsset>(dependencies[i].guid);

        if (asset)
            out << "\t" << asset->GetAssetName() << "\n";
        else
            out << "\t" << std::hex << dependencies[i].guid << "\n";
    }

    out << "]\n";
}
