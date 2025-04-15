#include <pch.h>
#include <game/rtech/assets/animseq.h>
#include <game/rtech/assets/rson.h>

#include <core/mdl/rmax.h>
#include <core/mdl/stringtable.h>
#include <core/mdl/cast.h>
#include <core/mdl/compdata.h>

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
		bones = &seqAsset->parentModel->bones;
	}
	else if (seqAsset->parentRig)
	{
		bones = &seqAsset->parentRig->bones;
	}
	assertm(!bones->empty(), "we should have bones at this point.");

	// ramen here

	const size_t boneCount = bones->size();

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

enum eAnimSeqExportSetting
{
	ASEQ_CAST,
	ASEQ_RMAX,
	ASEQ_RSEQ,
};

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

bool ExportSeqDescRMAX(const seqdesc_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones)
{
	const std::string fileNameBase = exportPath.stem().string();
	const std::string skelNameBase = std::filesystem::path(skelName).stem().string();

	const size_t boneCount = bones->size();

	for (int animIdx = 0; animIdx < seqdesc->AnimCount(); animIdx++)
	{
		const std::string animName = std::format("{}{}", fileNameBase.c_str(), animIdx);

		const std::string tmpName = std::format("{}.rmax", animName);
		exportPath.replace_filename(tmpName);

		rmax::RMAXExporter rmaxFile(exportPath, fileNameBase.c_str(), fileNameBase.c_str());

		// do bones
		rmaxFile.ReserveBones(boneCount);
		for (auto& bone : *bones)
			rmaxFile.AddBone(bone.name, bone.parentIndex, bone.pos, bone.quat, bone.scale);

		const animdesc_t* const animdesc = &seqdesc->anims.at(animIdx); // check flag 0x20000

		uint16_t animFlags = 0;

		if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA) // delta flag
			animFlags |= rmax::AnimFlags_t::ANIM_DELTA;

		if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID) || animdesc->parsedBufferIndex == invalidNoodleIdx)
			animFlags |= rmax::AnimFlags_t::ANIM_EMPTY;

		rmaxFile.AddAnim(animName.c_str(), static_cast<uint16_t>(animdesc->numframes), animdesc->fps, animFlags, boneCount);
		rmax::RMAXAnim* const anim = rmaxFile.GetAnimLast();

		if (anim->GetFlags() & rmax::AnimFlags_t::ANIM_EMPTY)
		{
			rmaxFile.ToFile();

			continue;
		}

		const std::unique_ptr<char[]> noodle = seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);
		CAnimData animData(noodle.get());

		for (size_t i = 0; i < boneCount; i++)
		{
			const uint8_t flags = animData.GetFlag(i);

			const char* dataPtr = animData.GetData(i);

			anim->SetTrack(flags, static_cast<uint16_t>(i));
			rmax::RMAXAnimTrack* const track = anim->GetTrack(i);

			const Vector* pos = nullptr;
			const Quaternion* q = nullptr;
			const Vector* scale = nullptr;

			if (flags & CAnimDataBone::ANIMDATA_POS)
			{
				pos = reinterpret_cast<const Vector*>(dataPtr);

				dataPtr += (sizeof(Vector) * animdesc->numframes);
			}

			if (flags & CAnimDataBone::ANIMDATA_ROT)
			{
				q = reinterpret_cast<const Quaternion*>(dataPtr);

				dataPtr += (sizeof(Quaternion) * animdesc->numframes);
			}

			if (flags & CAnimDataBone::ANIMDATA_SCL)
			{
				scale = reinterpret_cast<const Vector*>(dataPtr);

				dataPtr += (sizeof(Vector) * animdesc->numframes);
			}

			for (int frameIdx = 0; frameIdx < animdesc->numframes; frameIdx++)
			{
				track->AddFrame(frameIdx, &pos[frameIdx], &q[frameIdx], &scale[frameIdx]);
			}
		}

		rmaxFile.ToFile();
	}

	return true;
}

bool ExportSeqDescCast(const seqdesc_t* const seqdesc, std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones, const uint64_t guid)
{
	const std::string fileNameBase = exportPath.stem().string();
	const std::string skelNameBase = std::filesystem::path(skelName).stem().string();

	const size_t boneCount = bones->size();

	for (int animIdx = 0; animIdx < seqdesc->AnimCount(); animIdx++)
	{
		const animdesc_t* const animdesc = &seqdesc->anims.at(animIdx);

		const std::string tmpName(std::format("{}_{}.cast", fileNameBase, std::to_string(animIdx)));
		exportPath.replace_filename(tmpName);

		cast::CastExporter cast(exportPath.string());

		// cast
		cast::CastNode* const rootNode = cast.GetChild(0); // we only have one root node, no hash
		cast::CastNode* const animNode = rootNode->AddChild(cast::CastId::Animation, guid);

		// [rika]: we can predict how big this vector needs to be, however resizing it will make adding new members a pain.
		const size_t animChildrenCount = 1 + (boneCount * 7); // skeleton (one), curve per bone per data type
		animNode->ReserveChildren(animChildrenCount);
		animNode->ReserveProperties(2); // framerate, looping

		animNode->AddProperty(cast::CastPropertyId::Float, static_cast<int>(cast::CastPropsAnimation::Framerate), FLOAT_AS_UINT(animdesc->fps));
		animNode->AddProperty(cast::CastPropertyId::Byte, static_cast<int>(cast::CastPropsAnimation::Looping), animdesc->flags & eStudioAnimFlags::ANIM_LOOPING ? true : false);

		// do skeleton
		{
			// it would be more ideal to just feed it bones, but I don't want to deal with that mess of functions currently
			cast::CastNode* const skelNode = animNode->AddChild(cast::CastId::Skeleton, RTech::StringToGuid(fileNameBase.c_str()));
			skelNode->ReserveChildren(boneCount);

			// uses hashes for lookup, still gets bone parents by index :clown:
			for (size_t i = 0; i < boneCount; i++)
			{
				const ModelBone_t* const boneData = &bones->at(i);
				
				cast::CastNodeBone boneNode(skelNode);
				boneNode.MakeBone(boneData->name, boneData->parentIndex, &boneData->pos, &boneData->quat, false);
			}
		}

		if (!(animdesc->flags & eStudioAnimFlags::ANIM_VALID) || animdesc->parsedBufferIndex == invalidNoodleIdx)
		{
			cast.ToFile();

			continue;
		}

		const std::unique_ptr<char[]> noodle = seqdesc->parsedData.getIdx(animdesc->parsedBufferIndex);
		CAnimData animData(noodle.get());

		const cast::CastPropsCurveMode curveMode = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? cast::CastPropsCurveMode::MODE_ADDITIVE : cast::CastPropsCurveMode::MODE_ABSOLUTE;

		// setup the stupid key frame buffer thing that cast curves use
		cast::CastPropertyId frameBufferId;
		void* const frameBuffer = cast::CastNodeCurve::MakeCurveKeyFrameBuffer(static_cast<size_t>(animdesc->numframes), frameBufferId);

		Vector deltaPos(0.0f, 0.0f, 0.0f);
		Quaternion deltaQuat(0.0f, 0.0f, 0.0f, 1.0f);
		Vector deltaScale(1.0f, 1.0f, 1.0f);

		for (size_t i = 0; i < boneCount; i++)
		{
			const ModelBone_t* const boneData = &bones->at(i);

			// parsed data
			const uint8_t flags = animData.GetFlag(i);
			const char* dataPtr = animData.GetData(i);

			// weight for delta anims
			const float animWeight = seqdesc->Weight(static_cast<int>(i));

			if (flags & CAnimDataBone::ANIMDATA_POS)
			{
				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveVector(boneData->name, reinterpret_cast<const Vector*>(dataPtr), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::POS_X, curveMode, animWeight);
				
				dataPtr += (sizeof(Vector) * animdesc->numframes);
			}
			else
			{
				const Vector* const track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaPos : &boneData->pos;

				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveVector(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::POS_X, curveMode, animWeight);
			}

			if (flags & CAnimDataBone::ANIMDATA_ROT)
			{
				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveQuaternion(boneData->name, reinterpret_cast<const Quaternion*>(dataPtr), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), curveMode, animWeight);

				dataPtr += (sizeof(Quaternion) * animdesc->numframes);
			}
			else
			{
				const Quaternion* const track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaQuat : &boneData->quat;

				cast::CastNodeCurve curveNode(animNode);
				curveNode.MakeCurveQuaternion(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), curveMode, animWeight);
			}

			// check if the sequence has scale data.
			if (seqdesc->flags & 0x20000)
			{
				if (flags & CAnimDataBone::ANIMDATA_SCL)
				{
					cast::CastNodeCurve curveNode(animNode);
					curveNode.MakeCurveVector(boneData->name, reinterpret_cast<const Vector*>(dataPtr), static_cast<size_t>(animdesc->numframes), frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::SCL_X, curveMode, animWeight);
				}
				else
				{
					const Vector* track = animdesc->flags & eStudioAnimFlags::ANIM_DELTA ? &deltaScale : &boneData->scale;

					cast::CastNodeCurve curveNode(animNode);
					curveNode.MakeCurveVector(boneData->name, track, 1ull, frameBuffer, static_cast<size_t>(animdesc->numframes), cast::CastPropsCurveValue::SCL_X, curveMode, animWeight);
				}
			}
		}

		cast.ToFile();
		
		delete[] frameBuffer;
	}

	return true;
}

bool ExportAnimSeqAsset(CPakAsset* const asset, const int setting, const AnimSeqAsset* const animSeqAsset, const std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* const bones)
{
	std::filesystem::path exportPathCop = exportPath;

	switch (setting)
	{

	case eAnimSeqExportSetting::ASEQ_CAST:
	{
		return ExportSeqDescCast(&animSeqAsset->seqdesc, exportPathCop, skelName, bones, asset->GetAssetGUID());
	}
	case eAnimSeqExportSetting::ASEQ_RMAX:
	{
		return ExportSeqDescRMAX(&animSeqAsset->seqdesc, exportPathCop, skelName, bones);
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

	const bool exportAsRaw = setting == eAnimSeqExportSetting::ASEQ_RSEQ;

	if (!exportAsRaw && !animSeqAsset->animationParsed)
	{
		// no skeleton while trying to export it as a format that requires it.
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

	if (exportAsRaw)
	{
		// [amos]: eAnimSeqExportSetting::RSEQ is handled here because we do
		// not need the skeleton data when exporting the raw sequence data.
		// Some of these animation sequences are dependents of settings assets
		// and appear to be used in some way or another, so we must do it here.
		return ExportRawAnimSeqAsset(pakAsset, animSeqAsset, exportPath);
	}

	const std::vector<ModelBone_t>* bones = nullptr;
	char* skeletonName = nullptr;

	if (animSeqAsset->parentModel)
	{
		bones = &animSeqAsset->parentModel->bones;
		skeletonName = animSeqAsset->parentModel->name;
	}
	else if (animSeqAsset->parentRig)
	{
		bones = &animSeqAsset->parentRig->bones;
		skeletonName = animSeqAsset->parentRig->name;
	}
	assertm(bones && !bones->empty(), "we should have bones at this point.");

	return ExportAnimSeqAsset(pakAsset, setting, animSeqAsset, exportPath, skeletonName, bones);
}

void InitAnimSeqAssetType()
{
	static const char* settings[] = { "CAST", "RMAX", "RSEQ",  };
	AssetTypeBinding_t type =
	{
		.type = 'qesa',
		.headerAlignment = 8,
		.loadFunc = LoadAnimSeqAsset,
		.postLoadFunc = PostLoadAnimSeqAsset,
		.previewFunc = nullptr,
		.e = { ExportAnimSeqAsset, 0, settings, ARRSIZE(settings) },
	};

	REGISTER_TYPE(type);
}