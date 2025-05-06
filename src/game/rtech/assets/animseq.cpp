#include <pch.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/assets/rson.h>

#include <core/mdl/rmax.h>
#include <core/mdl/stringtable.h>
#include <core/mdl/cast.h>
#include <core/mdl/modeldata.h>

extern CBufferManager* g_BufferManager;
extern ExportSettings_t g_ExportSettings;

void LoadAnimSeqAsset(CAssetContainer* const container, CAsset* const asset)
{
	UNUSED(container);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	AnimSeqAsset* seqAsset = nullptr;
	const AssetPtr_t streamEntry = pakAsset->getStarPakStreamEntry(false);

	const eSeqVersion ver = GetAnimSeqVersionFromAsset(pakAsset);
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
		asset->SetAssetVersion({ 7, 1 });

		AnimSeqAssetHeader_v7_1_t* hdr = reinterpret_cast<AnimSeqAssetHeader_v7_1_t*>(pakAsset->header());
		seqAsset = new AnimSeqAsset(hdr, streamEntry, ver);
		break;
	}
	case eSeqVersion::VERSION_8:
	case eSeqVersion::VERSION_10:
	case eSeqVersion::VERSION_11:
	case eSeqVersion::VERSION_12:
	{
		AnimSeqAssetHeader_v8_t* hdr = reinterpret_cast<AnimSeqAssetHeader_v8_t*>(pakAsset->header());
		seqAsset = new AnimSeqAsset(hdr, streamEntry, ver);
		break;
	}
	default:
		return;
	}

	pakAsset->SetAssetName(seqAsset->name);
	pakAsset->setExtraData(seqAsset);
}

void PostLoadAnimSeqAsset(CAssetContainer* const container, CAsset* const asset)
{
	UNUSED(container);

	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	assertm(pakAsset->extraData(), "extra data should be valid");
	AnimSeqAsset* const seqAsset = reinterpret_cast<AnimSeqAsset*>(pakAsset->extraData());

	// do not parse this animation if there is no skeleton, if we go to export a sequence from a model/rig that has not been parsed, we will have to parse on export.
	// this also means this sequence will not export data when exported standalone
	if (nullptr == seqAsset->parentRig && nullptr == seqAsset->parentModel)
		return;

	const std::vector<ModelBone_t>* bones = nullptr;

	if (seqAsset->parentModel)
	{
		bones = seqAsset->parentModel->GetRig();
	}
	else if (seqAsset->parentRig)
	{
		bones = &seqAsset->parentRig->bones;
	}
	assertm(!bones->empty(), "we should have bones at this point.");

	// ramen here

	const size_t boneCount = bones->size();

	// [rika]: parse the animseq's raw data size in post load if we couldn't determine a bone count before.
	if (seqAsset->dataSize == 0 && asset->GetAssetVersion().majorVer >= 12)
		seqAsset->UpdateDataSizeNew(static_cast<int>(boneCount));

	// check flags
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_POS) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_POS), "flag mismatch");
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_ROT) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_ROT), "flag mismatch");
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_SCL) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_SCALE), "flag mismatch");

	switch (seqAsset->version)
	{
	case eSeqVersion::VERSION_7:
	case eSeqVersion::VERSION_7_1:
	case eSeqVersion::VERSION_8:
	case eSeqVersion::VERSION_10:
	case eSeqVersion::VERSION_11:
	case eSeqVersion::VERSION_12:
	{
		char* (animdesc_t::*pAnimdata)(int* const) const = seqAsset->UseStall() ? &animdesc_t::pAnimdataStall : &animdesc_t::pAnimdataNoStall;

		for (int i = 0; i < seqAsset->seqdesc.AnimCount(); i++)
		{
			animdesc_t* const animdesc = &seqAsset->seqdesc.anims.at(i);

			if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID))
			{
				animdesc->parsedBufferIndex = invalidNoodleIdx;

				continue;
			}

			// no point to allocate memory on empty animations!
			CAnimData animData(boneCount, animdesc->numframes);
			animData.ReserveVector();

			for (int frameIdx = 0; frameIdx < animdesc->numframes; frameIdx++)
			{				
				const float cycle = animdesc->numframes > 1 ? static_cast<float>(frameIdx) / static_cast<float>(animdesc->numframes - 1) : 0.0f;
				assertm(isfinite(cycle), "cycle was nan");

				float fFrame = cycle * static_cast<float>(animdesc->numframes - 1);

				const int iFrame = static_cast<int>(fFrame);
				const float s = (fFrame - static_cast<float>(iFrame));

				int iLocalFrame = iFrame;

				const uint8_t* boneFlagArray = reinterpret_cast<uint8_t*>((animdesc->*pAnimdata)(&iLocalFrame));
				const r5::mstudio_rle_anim_t* panim = reinterpret_cast<const r5::mstudio_rle_anim_t*>(&boneFlagArray[ANIM_BONEFLAG_SIZE(boneCount)]);

				for (int boneIdx = 0; boneIdx < boneCount; boneIdx++)
				{
					const ModelBone_t* const bone = &bones->at(boneIdx);

					Vector pos;
					Quaternion q;
					Vector scale;
					RadianEuler baseRot;

					if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA)
					{
						pos.Init(0.0f, 0.0f, 0.0f);
						q.Init(0.0f, 0.0f, 0.0f, 1.0f);
						scale.Init(1.0f, 1.0f, 1.0f);
						baseRot.Init(0.0f, 0.0f, 0.0f);
					}
					else
					{
						pos = bone->pos;
						q = bone->quat;
						scale = bone->scale;
						baseRot = bone->rot;
					}

					uint8_t boneFlags = ANIM_BONEFLAGS_FLAG(boneFlagArray, boneIdx); // truncate byte offset then shift if needed

					if (boneFlags & (r5::RleBoneFlags_t::STUDIO_ANIM_DATA)) // check if this bone has data
					{
						if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_POS)
							CalcBonePosition(iLocalFrame, s, panim, pos);
						if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_ROT)
							CalcBoneQuaternion(iLocalFrame, s, panim, baseRot, q, boneFlags);
						if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_SCALE)
							CalcBoneScale(iLocalFrame, s, panim, scale, boneFlags);


						panim = panim->pNext();
					}

					// adjust the origin bone
					// do after we get anim data, so our rotation does not get overwritten
					if (boneIdx == 0)
					{
						QAngle vecAngleBase(q);

						if (nullptr != animdesc->movement && animdesc->flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
						{
							Vector vecPos;
							QAngle vecAngle;

							r5::Studio_AnimPosition(animdesc, cycle, vecPos, vecAngle);

							pos += vecPos; // add our base movement position to our base position 
							vecAngleBase.y += (vecAngle.y);
						}
						
						vecAngleBase.y += -90; // rotate -90 degree on the yaw axis

						// adjust position as we are rotating on the Z axis
						const float x = pos.x;
						const float y = pos.y;

						pos.x = y;
						pos.y = -x;

						AngleQuaternion(vecAngleBase, q);

						// has pos/rot data regardless since we just adjusted pos/rot
						boneFlags |= (r5::RleBoneFlags_t::STUDIO_ANIM_POS | r5::RleBoneFlags_t::STUDIO_ANIM_ROT);
					}

					CAnimDataBone& animDataBone = animData.GetBone(boneIdx);
					animDataBone.SetFlags(boneFlags);
					animDataBone.SetFrame(frameIdx, pos, q, scale);
				}
			}

			// parse into memory and compress
			CManagedBuffer* buffer = g_BufferManager->ClaimBuffer();

			const size_t sizeInMem = animData.ToMemory(buffer->Buffer());
			animdesc->parsedBufferIndex = seqAsset->seqdesc.parsedData.addBack(buffer->Buffer(), sizeInMem);

			g_BufferManager->RelieveBuffer(buffer);
		}

		break;
	}
	default:
		break;
	}

	// the sequence has been parsed for exporting
	seqAsset->animationParsed = true;
}

void* PreviewAnimSeqAsset(CAsset* const asset, const bool firstFrameForAsset)
{
	UNUSED(firstFrameForAsset);

	CPakAsset* const pakAsset = static_cast<CPakAsset*>(asset);

	assertm(pakAsset->extraData(), "extra data should be valid");
	const AnimSeqAsset* const animSeqAsset = reinterpret_cast<const AnimSeqAsset* const>(pakAsset->extraData());

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
				out << "\t\t\"" << std::hex << dependencies[i].guid << "\"" << commaChar << "\n";
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

	case eAnimSeqExportSetting::ANIMSEQ_CAST:
	{
		return ExportSeqDescCast(&animSeqAsset->seqdesc, exportPathCop, skelName, bones, asset->GetAssetGUID());
	}
	case eAnimSeqExportSetting::ANIMSEQ_RMAX:
	{
		return ExportSeqDescRMAX(&animSeqAsset->seqdesc, exportPathCop, skelName, bones);
	}
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

static const char* const s_PathPrefixASEQ = s_AssetTypePaths.find(AssetType_t::ASEQ)->second;
bool ExportAnimSeqAsset(CAsset* const asset, const int setting)
{
	CPakAsset* pakAsset = static_cast<CPakAsset*>(asset);

	const AnimSeqAsset* const animSeqAsset = reinterpret_cast<AnimSeqAsset*>(pakAsset->extraData());
	assertm(animSeqAsset, "Extra data should be valid at this point.");
	assertm(animSeqAsset->name, "No name for anim rig.");

	const bool exportAsRaw = setting == eAnimSeqExportSetting::ANIMSEQ_RSEQ;

	if (!exportAsRaw && !animSeqAsset->animationParsed)
	{
		// no skeleton while trying to export it as a format that requires it.
		assertm(false, "animseq was not parsed (likely invalid rig)");
		return false;
	}

	// Create exported path + asset path.
	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path seqPath(animSeqAsset->name);
	const std::string seqStem(seqPath.stem().string());

	// truncate paths?
	if (g_ExportSettings.exportPathsFull)
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
	const std::vector<ModelBone_t>* bones = nullptr;
	char* skeletonName = nullptr;

	// [rika]: only bother with this if we are going to use it!
	if (!exportAsRaw)
	{
		if (animSeqAsset->parentModel)
		{
			bones = animSeqAsset->parentModel->GetRig();
			skeletonName = animSeqAsset->parentModel->name;
		}
		else if (animSeqAsset->parentRig)
		{
			bones = &animSeqAsset->parentRig->bones;
			skeletonName = animSeqAsset->parentRig->name;
		}
		assertm(bones && !bones->empty(), "we should have bones at this point.");
	}

	return ExportAnimSeqAsset(pakAsset, setting, animSeqAsset, exportPath, skeletonName, bones);
}

void InitAnimSeqAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'qesa',
		.headerAlignment = 8,
		.loadFunc = LoadAnimSeqAsset,
		.postLoadFunc = PostLoadAnimSeqAsset,
		.previewFunc = PreviewAnimSeqAsset,
		.e = { ExportAnimSeqAsset, 0, s_AnimSeqExportSettingNames, ARRSIZE(s_AnimSeqExportSettingNames) },
	};

	REGISTER_TYPE(type);
}