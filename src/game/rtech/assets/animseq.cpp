#include <pch.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/assets/animseq_data.h>
#include <game/rtech/assets/rson.h>

#include <core/mdl/rmax.h>
#include <core/mdl/stringtable.h>
#include <core/mdl/cast.h>
#include <core/mdl/modeldata.h>
#include <core/mdl/animdata.h>

#include <thirdparty/imgui/misc/imgui_utility.h>

extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;

void LoadAnimSeqAsset(CAssetContainer* const container, CAsset* const asset)
{
	UNUSED(container);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	AnimSeqAsset* seqAsset = nullptr;
	const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false);

	const eSeqVersion ver = GetAnimSeqVersionFromAsset(pakAsset, static_cast<CPakFile* const>(container));
	switch (ver)
	{
	case eSeqVersion::VERSION_7:
	{
		AnimSeqAssetHeader_v7_t* hdr = reinterpret_cast<AnimSeqAssetHeader_v7_t*>(pakAsset->header());
		seqAsset = new AnimSeqAsset(hdr, ver);
		break;
	}
	case eSeqVersion::VERSION_7_1:
	{
		AnimSeqAssetHeader_v7_1_t* hdr = reinterpret_cast<AnimSeqAssetHeader_v7_1_t*>(pakAsset->header());
		seqAsset = new AnimSeqAsset(hdr, streamEntry, ver);
		break;
	}
	case eSeqVersion::VERSION_8:
	case eSeqVersion::VERSION_9:
	case eSeqVersion::VERSION_10:
	case eSeqVersion::VERSION_11:
	case eSeqVersion::VERSION_12:
	case eSeqVersion::VERSION_12_1:
	case eSeqVersion::VERSION_13:
	{
		AnimSeqAssetHeader_v8_t* hdr = reinterpret_cast<AnimSeqAssetHeader_v8_t*>(pakAsset->header());
		seqAsset = new AnimSeqAsset(hdr, streamEntry, ver);
		break;
	}
	default:
		return;
	}

	switch (ver)
	{
	case eSeqVersion::VERSION_7_1:
	{
		asset->SetAssetVersion({ 7, 1 });

		break;
	}
	case eSeqVersion::VERSION_12_1:
	{
		asset->SetAssetVersion({ 12, 1 });

		break;
	}
	default:
		break;
	}

	pakAsset->SetAssetName(seqAsset->name, true);
	pakAsset->setExtraData(seqAsset);
}

bool AnimSeq_ParseExtraData(CPakAsset* pakAsset)
{
	AnimSeqAsset* const seqAsset = pakAsset->extraData<AnimSeqAsset*>();
	// do not parse this animation if there is no skeleton, if we go to export a sequence from a model/rig that has not been parsed, we will have to parse on export.
	// this also means this sequence will not export data when exported standalone
	if (nullptr == seqAsset->parentRig && nullptr == seqAsset->parentModel)
		return false;

	const std::vector<ModelBone_t>* bones = nullptr;

	if (seqAsset->parentModel)
	{
		bones = seqAsset->parentModel->GetRig();
	}
	else if (seqAsset->parentRig)
	{
		bones = seqAsset->parentRig->GetRig();
	}
	assertm(!bones->empty(), "we should have bones at this point.");

	switch (seqAsset->version)
	{
	case eSeqVersion::VERSION_7:
	{
		ParseSequence(&seqAsset->seqdesc, bones, AnimdataFuncType_t::ANIM_FUNC_NOSTALL);

		break;
	}
	case eSeqVersion::VERSION_7_1:
	case eSeqVersion::VERSION_8:
	case eSeqVersion::VERSION_10:
	case eSeqVersion::VERSION_11:
	{
		ParseSequence(&seqAsset->seqdesc, bones, AnimdataFuncType_t::ANIM_FUNC_STALL_BASEPTR);

		break;
	}
	case eSeqVersion::VERSION_12:
	{
		// [rika]: parse the animseq's raw data size in post load if we couldn't determine a bone count before.
		if (seqAsset->dataSize == 0)
			seqAsset->UpdateDataSize_V12(static_cast<int>(bones->size()));

		ParseSequence(&seqAsset->seqdesc, bones, AnimdataFuncType_t::ANIM_FUNC_STALL_BASEPTR);

		break;
	}
	case eSeqVersion::VERSION_12_1:
	{
		// [rika]: parse the animseq's raw data size in post load if we couldn't determine a bone count before.
		if (seqAsset->dataSize == 0)
			seqAsset->UpdateDataSize_V12_1(static_cast<int>(bones->size()));

		// [rika]: I love changing assets, but never ever would change a version!
		ParseAnimSeqDataForSeq(&seqAsset->seqdesc, bones->size(), ANIM_BONEFLAG_BITS_4);
		ParseSequence(&seqAsset->seqdesc, bones, AnimdataFuncType_t::ANIM_FUNC_STALL_ANIMDATA);

		break;
	}
	case eSeqVersion::VERSION_13:
	{
		// [rika]: parse the animseq's raw data size in post load if we couldn't determine a bone count before.
		if (seqAsset->dataSize == 0)
			seqAsset->UpdateDataSize_V12_1(static_cast<int>(bones->size()));

		// [rika]: I love changing assets, but never ever would change a version!
		ParseAnimSeqDataForSeq(&seqAsset->seqdesc, bones->size(), ANIM_BONEFLAG_BITS_6);
		ParseSequence(&seqAsset->seqdesc, bones, AnimdataFuncType_t::ANIM_FUNC_STALL_ANIMDATA, ANIM_BONEFLAG_BITS_6);

		break;
	}
	default:
		break;
	}

	// the sequence has been parsed for exporting
	seqAsset->animationParsed = true;
	return true;
}

void PostLoadAnimSeqAsset(CAssetContainer* const container, CAsset* const asset)
{
	UNUSED(container);
	//UNUSED(asset);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	if (!pakAsset->hasExtraData())
		return;

#ifndef DEBUG_NO_ASEQ_POSTLOAD
	AnimSeq_ParseExtraData(pakAsset);
#endif
}

void* PreviewAnimSeqAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

	const AnimSeqAsset* const animSeqAsset = pakAsset->extraData<const AnimSeqAsset* const>();
	if (!animSeqAsset)
		return nullptr;

	// [rika]: todo, preview settings and model lists? or is this covered in linked assets? unsure.

	PreviewSeqDesc(&animSeqAsset->seqdesc);

	return nullptr;
}

static bool ExportRawAnimSeqAsset(CPakAsset* const asset, const AnimSeqAsset* const animSeqAsset, std::filesystem::path& exportPath)
{
	UNUSED(asset);

	StreamIO seqOut(exportPath.string(), eStreamIOMode::Write);
	seqOut.write(reinterpret_cast<const char*>(animSeqAsset->data), animSeqAsset->dataSize);
	seqOut.close();

	if (animSeqAsset->dataExtraSize)
	{
		assert(animSeqAsset->dataExtraPerm);

		exportPath.replace_extension(".rseq_extn");

		StreamIO extOut(exportPath.string(), eStreamIOMode::Write);
		extOut.write(animSeqAsset->dataExtraPerm, animSeqAsset->dataExtraSize);
		extOut.close();
	}

	// make a manifest of this assets dependencies
	std::vector<AssetGuid_t> dependencies;
	asset->getDependencies(dependencies);

	const size_t numDependencies = dependencies.size();

	if (numDependencies > 0)
	{
		exportPath.replace_extension(".json");

		StreamIO depOut(exportPath.string(), eStreamIOMode::Write);
		std::ofstream& out = *depOut.W();

		out << "{\n";
		out << "\t\"dependencies\": [\n";

		for (size_t i = 0; i < numDependencies; i++)
		{
			CPakAsset* const dependencyAsset = g_assetData.FindAssetByGUID<CPakAsset>(dependencies[i].guid);
			const char* const commaChar = i != (numDependencies - 1) ? "," : "";

			if (dependencyAsset)
			{
				std::string dependencyAssetName = dependencyAsset->GetAssetName();
				FixSlashes(dependencyAssetName); // Needs to be called because json can't take un-escaped '\'

				out << "\t\t\"" << dependencyAssetName << "\"" << commaChar << "\n";
			}
			else
			{
				out << "\t\t\"" << std::hex << dependencies[i].guid << "\"" << commaChar << "\n";
			}
		}

		out << "\t]\n";
		out << "}\n";

		depOut.close();
	}

	return true;
}

bool ExportAnimSeqAsset(CPakAsset* const asset, const int setting, const AnimSeqAsset* const animSeqAsset, const std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones)
{
	std::filesystem::path exportPathCop = exportPath;

	// [rika]: please DO NOT remove the 'eAnimSeqExportSetting::ANIMSEQ_RSEQ' case from here, this function can be called elsewhere (MDL & ARIG)
	// with this export setting and will cause issues!
	switch (setting)
	{
	// exporting seqdesc
	case eAnimSeqExportSetting::ANIMSEQ_CAST:
	case eAnimSeqExportSetting::ANIMSEQ_RMAX:
	case eAnimSeqExportSetting::ANIMSEQ_SMD:
	{
		return ExportSeqDesc(setting, &animSeqAsset->seqdesc, exportPathCop, skelName, bones, asset->guid());
	}
	//	exporting asset
	case eAnimSeqExportSetting::ANIMSEQ_RSEQ:
	{
		return ExportRawAnimSeqAsset(asset, animSeqAsset, exportPathCop);
	}
	default:
	{
		assertm(false, "Export setting is not handled.");
		return false;
	}
	}
}

bool ExportAnimSeqFromAsset(const std::filesystem::path& exportPath, const std::string& stem, const char* const name, const int numAnimSeqs, const AssetGuid_t* const animSeqs, const std::vector<ModelBone_t>* const bones)
{
	auto aseqAssetBinding = g_assetData.m_assetTypeBindings.find('qesa');

	assertm(aseqAssetBinding != g_assetData.m_assetTypeBindings.end(), "Unable to find asset type binding for \"aseq\" assets");

	if (aseqAssetBinding != g_assetData.m_assetTypeBindings.end())
	{
		std::filesystem::path outputPath(exportPath);
		outputPath.append(std::format("anims_{}/temp", stem));

		if (!CreateDirectories(outputPath.parent_path()))
		{
			assertm(false, "Failed to create directory.");
			return false;
		}

		std::atomic<uint32_t> remainingSeqs = 0; // we don't actually need thread safe here
		const ProgressBarEvent_t* const seqExportProgress = g_pImGuiHandler->AddProgressBarEvent("Exporting Sequences..", static_cast<uint32_t>(numAnimSeqs), &remainingSeqs, true);
		for (int i = 0; i < numAnimSeqs; i++)
		{
			const uint64_t guid = animSeqs[i].guid;

			CPakAsset* const animSeq = g_assetData.FindAssetByGUID<CPakAsset>(guid);

			if (nullptr == animSeq)
			{
				Log("RSEQ DEP: animseq asset 0x%llX was not loaded, skipping...\n", guid);
				continue;
			}

			if (!animSeq->GetPostLoadStatus())
			{
				Log("RSEQ DEP: animseq assets were not loaded when this pak was loaded. skipping...\n");
				continue;
			}

			const AnimSeqAsset* const animSeqAsset = animSeq->extraData<const AnimSeqAsset* const>();

			// skip this animation if for some reason it has not been parsed. if a loaded mdl/animrig has sequence children, it should always be parsed. possibly move this to an assert.
			if (!animSeqAsset->animationParsed)
				continue;

			outputPath.replace_filename(std::filesystem::path(animSeqAsset->name).filename());

			ExportAnimSeqAsset(animSeq, aseqAssetBinding->second.e.exportSetting, animSeqAsset, outputPath, name, bones);

			++remainingSeqs;
		}
		g_pImGuiHandler->FinishProgressBarEvent(seqExportProgress);
	}

	return true;
}

static const char* const s_PathPrefixASEQ = s_AssetTypePaths.find(AssetType_t::ASEQ)->second;
bool ExportAnimSeqAsset(CAsset* const asset, const int setting)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const AnimSeqAsset* const animSeqAsset = pakAsset->extraData<const AnimSeqAsset* const>();

	if (!animSeqAsset)
		return false;

	assertm(animSeqAsset->name, "No name for anim rig.");

	const bool exportAsRaw = setting == eAnimSeqExportSetting::ANIMSEQ_RSEQ;

	if (!exportAsRaw && !animSeqAsset->animationParsed)
	{
		// no skeleton while trying to export it as a format that requires it.
		assertm(false, "animseq was not parsed (likely invalid rig)");
		return false;
	}

	// Create exported path + asset path.
	std::filesystem::path exportPath = g_rsxSettings.GetExportDirectory();
	const std::filesystem::path seqPath(animSeqAsset->name);
	const std::string seqStem(seqPath.stem().string());

	// truncate paths?
	if (g_rsxSettings.exportPathsFull)
		exportPath.append(seqPath.parent_path().string());
	else
		exportPath.append(std::format("{}/{}", s_PathPrefixASEQ, seqStem));

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset directory.");
		return false;
	}

	exportPath.append(std::format("{}.rseq", seqStem));

	// [rika]: get our rig if there is one available
	const ModelParsedData_t* parsedData = nullptr;
	const char* rigName = nullptr;

	// [rika]: only bother with this if we are going to use it!
	if (!exportAsRaw)
	{
		if (animSeqAsset->parentModel)
		{
			parsedData = animSeqAsset->parentModel->GetParsedData();
			rigName = animSeqAsset->parentModel->name;
		}
		else if (animSeqAsset->parentRig)
		{
			parsedData = animSeqAsset->parentRig->GetParsedData();
			rigName = animSeqAsset->parentRig->name;
		}
		assertm(parsedData && parsedData->BoneCount(), "we should have bones at this point.");
	}

	if (setting == eAnimSeqExportSetting::ANIMSEQ_SMD)
	{
		const bool result = ExportSeqQC(parsedData, &animSeqAsset->seqdesc, exportPath, setting, 54);
		if (result == false)
		{
			return false;
		}
	}

	return ExportAnimSeqAsset(pakAsset, setting, animSeqAsset, exportPath, rigName, parsedData->GetRig());
}

void InitAnimSeqAssetType()
{
	AssetTypeBinding_t type =
	{
		.name = "Animation Sequence",
		.type = 'qesa',
		.headerAlignment = 8,
		.loadFunc = LoadAnimSeqAsset,
		.postLoadFunc = PostLoadAnimSeqAsset,
		.previewFunc = PreviewAnimSeqAsset,
		.e = { ExportAnimSeqAsset, 0, s_AnimSeqExportSettingNames, ARRSIZE(s_AnimSeqExportSettingNames) },
	};

	REGISTER_TYPE(type);
}