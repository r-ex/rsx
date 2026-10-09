#include <pch.h>
#include <core/mdl/modeldata.h>

#include <core/mdl/qc.h>

#include <game/rtech/assets/animseq.h>

//#include <core/render/dx.h>
//#include <thirdparty/imgui/imgui.h>
#include <thirdparty/imgui/misc/imgui_utility.h>

extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;

// get the used material indices for skins
// todo: move to qc
const uint32_t QC_GetUsedSkinIndices(const int16_t* const skins, const int numSkinRef, const int numSkinFamilies, int16_t* const indices)
{
	// [rika]: no actual skins, just base materials
	assertm(numSkinFamilies >= 2, "model had no additional skins");

	memset(indices, 0xff, sizeof(int16_t) * numSkinRef);

	const int16_t* skinGroup = skins + numSkinRef;

	// [rika]: parse through each skingroup to see which indices changed
	// todo: setting to skip this and write all materials
	for (int family = 0; family < numSkinFamilies - 1; family++)
	{
		for (int material = 0; material < numSkinRef; material++)
		{
			if (skins[material] == skinGroup[material])
				continue;

			indices[material] = static_cast<int16_t>(material);
		}

		skinGroup += numSkinRef;
	}

	// [rika]: make our indices sequential
	uint32_t curIdx = 0;
	for (int i = 0; i < numSkinRef; i++)
	{
		if (indices[i] == -1)
			continue;

		indices[curIdx] = indices[i];
		curIdx++;
	}

	return curIdx;
}

// parse qc commands from header
void QC_ParseStudioHeader(qc::QCFile* const qc, const ModelParsedData_t* const parsedData, const int version)
{
	using namespace qc;

	CmdParse(qc, QC_MODELNAME, parsedData->name);
	CmdParse(qc, QC_CONTENTS, &parsedData->contents);
	CmdParse(qc, QC_SURFACEPROP, parsedData->surfaceProp);

	// [rika]: flag based options
	if (parsedData->flags & STUDIOHDR_FLAGS_STATIC_PROP)
		CmdParse(qc, QC_STATICPROP, nullptr);

	if (version < 52)
	{
		if (parsedData->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
		{
			assertm(!(parsedData->flags & STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS), "translucent and opaque");
			CmdParse(qc, QC_OPAQUE, nullptr);
		}

		// removed in r1, parsed from materials. flag replaced in r5, used by arms and loading models a lot!
		if (parsedData->flags & STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS)
		{
			assertm(!(parsedData->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE), "translucent and opaque");
			CmdParse(qc, QC_MOSTLYOPAQUE, nullptr);
		}
	}
	else
	{
		// [rika]: pretty sure this one is still supported!
		if (parsedData->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE)
		{
			CmdParse(qc, QC_OPAQUE, nullptr);
		}
	}

	if (version != 54 && parsedData->flags & STUDIOHDR_FLAGS_OBSOLETE)
	{
		CmdParse(qc, QC_OBSOLETE, nullptr);
	}

	if (parsedData->flags & STUDIOHDR_FLAGS_NO_FORCED_FADE)
	{
		CmdParse(qc, QC_NOFORCEDFADE, nullptr);
	}

	if (parsedData->flags & STUDIOHDR_FLAGS_FORCE_PHONEME_CROSSFADE)
	{
		CmdParse(qc, QC_FORCEPHONEMECROSSFADE, nullptr);
	}

	if (parsedData->flags & STUDIOHDR_FLAGS_CONSTANT_DIRECTIONAL_LIGHT_DOT)
	{
		CmdParse(qc, QC_CONSTANTDIRECTIONALLIGHT, &parsedData->constdirectionallightdot);
	}

	if (version == 54 && parsedData->flags & STUDIOHDR_FLAGS_USES_EXTRA_BONE_WEIGHTS)
	{
		CmdParse(qc, QC_USEDETAILEDWEIGHTS, nullptr);
	}

	// believe this is different later seasons 
	if (parsedData->flags & STUDIOHDR_FLAGS_AMBIENT_BOOST)
	{
		CmdParse(qc, QC_AMBIENTBOOST, nullptr);
	}

	if (parsedData->flags & STUDIOHDR_FLAGS_DO_NOT_CAST_SHADOWS)
	{
		CmdParse(qc, QC_DONOTCASTSHADOWS, nullptr);
	}

	// believe this is different later seasons (used on non static props)
	if (parsedData->flags & STUDIOHDR_FLAGS_CAST_TEXTURE_SHADOWS)
	{
		CmdParse(qc, QC_CASTTEXTURESHADOWS, nullptr);
	}

	if (version != 54 && parsedData->flags & STUDIOHDR_FLAGS_SUBDIVISION_SURFACE)
	{
		CmdParse(qc, QC_SUBD, nullptr);
	}

	if (version >= 52 && parsedData->flags & STUDIOHDR_FLAGS_USES_VERTEX_COLOR)
	{
		CmdParse(qc, QC_USEVERTEXCOLOR, nullptr);
	}

	if (version >= 52 && parsedData->flags & STUDIOHDR_FLAGS_USES_UV2)
	{
		CmdParse(qc, QC_USEEXTRATEXCOORD, nullptr);
	}

	// [rika]: -1 is the default value to not fade, skip if it is set
	if (parsedData->fadeDistance != -1.0f)
	{
		CmdParse(qc, QC_FADEDISTANCE, &parsedData->fadeDistance);
	}

	CmdParse(qc, QC_EYEPOSITION, &parsedData->eyeposition, 3);

	if (parsedData->flMaxEyeDeflection != 0.0f)
	{
		CmdParse(qc, QC_MAXEYEDEFLECTION, &parsedData->flMaxEyeDeflection);
	}

	if (parsedData->MaterialCount() > 0)
	{
		const char** materials = new const char* [parsedData->MaterialCount()] {};
		const char** materialFullPaths = nullptr;

		// if we are going to rename materials
		if (g_rsxSettings.exportModelMatsTruncated)
		{
			materialFullPaths = new const char* [parsedData->MaterialCount()] {};
		}

		for (int i = 0; i < parsedData->MaterialCount(); i++)
		{
			materials[i] = parsedData->pMaterial(i)->GetName(true);

			// are the materials getting shortened ?
			if (!g_rsxSettings.exportModelMatsTruncated)
				continue;

			assertm(materialFullPaths, "material path array was not allocated");

			materialFullPaths[i] = materials[i];
			materials[i] = GetStringAfterLastSlash(materials[i]);
		}

		if (parsedData->SkinCount() > 1)
		{
			const int16_t* const skins = parsedData->pSkin(0)->indices; // this is toxic but works
			int16_t* const indices = new int16_t[parsedData->MaterialCount()]{};
			const char** names = nullptr;

			if (version == 54)
			{
				names = new const char* [parsedData->SkinCount()] {};

				for (int i = 0; i < parsedData->SkinCount(); i++)
				{
					names[i] = parsedData->pSkin(i)->name;
				}
			}

			const uint32_t usedIndices = QC_GetUsedSkinIndices(skins, parsedData->MaterialCount(), parsedData->SkinCount(), indices);

			const TextureGroupData_t texturegroup(materials, skins, indices, parsedData->MaterialCount(), parsedData->SkinCount(), usedIndices, names);
			CmdParse(qc, QC_TEXTUREGROUP, &texturegroup);

			FreeAllocArray(indices);
			FreeAllocArray(names);
		}

		if (g_rsxSettings.exportModelMatsTruncated)
		{
			for (int i = 0; i < parsedData->MaterialCount(); i++)
			{
				const CommandOptionPair_t renameData(materials[i], materialFullPaths[i]);
				CmdParse(qc, QC_RENAMEMATERIAL, &renameData);
			}
		}

		FreeAllocArray(materials);
		FreeAllocArray(materialFullPaths);
	}

	// [rika]: most models will have at least one (unless it's retail apex), it will be the path prefixing 'mdl' or 'models', in most cases this is just an empty string
	for (int i = 0; i < parsedData->CDMaterialCount(); i++)
		CmdParse(qc, QC_CDMATERIALS, parsedData->CDMaterial(i));

	if (parsedData->keyValueSize > 0 || (parsedData->keyValues && parsedData->keyValueSize == -1))
		CmdParse(qc, QC_KEYVALUES, parsedData->keyValues);

	if (parsedData->numAllowedRootLODs > 0)
		CmdParse(qc, QC_ALLOWROOTLODS, &parsedData->numAllowedRootLODs);

	if (parsedData->rootLOD > 0)
		CmdParse(qc, QC_MINLOD, &parsedData->rootLOD);
}

void QC_ParseStudioBone(qc::QCFile* const qc, const ModelParsedData_t* const parsedData, const ModelBone_t* const bone, const int version)
{
	using namespace qc;

	// [rika]: parse out the data required for a $definebone (and other) command(s)
	const char* const parent = bone->parent > -1 ? parsedData->pBone(bone->parent)->pszName() : nullptr;
	const matrix3x4_t* pPostTransform = nullptr;

	// [rika]: get data for fixups if it exits
	// [rika]: no names on v16 and above, how do we do that
	if (parsedData->numSrcBoneTransforms && 16 > parsedData->version.majorVer)
	{
		const mstudiosrcbonetransform_t* const pSrcBoneTransform = GetSrcBoneTransform(bone->name, parsedData->srcBoneTransforms, parsedData->numSrcBoneTransforms);

		if (pSrcBoneTransform)
			pPostTransform = &pSrcBoneTransform->posttransform;
	}

	const BoneData_t boneData(bone->pszName(), parent/*, bone->pszSurfaceProp()*/, bone->contents, bone->flags, &bone->pos, &bone->rot, pPostTransform);
	const CommandOptionPair_t boneSurface(bone->pszName(), bone->pszSurfaceProp());

	CmdParse(qc, QC_DEFINEBONE, &boneData);

	// THESE NEED NAMES, save data in DefineBoneData_t and pass to these funcs
	CmdParse(qc, QC_JOINTCONTENTS, &boneData);
	CmdParse(qc, QC_JOINTSURFACEPROP, &boneSurface);

	if (bone->flags & BONE_USED_BY_BONE_MERGE)
	{
		CmdParse(qc, QC_BONEMERGE, bone->pszName());
	}

	if (bone->proctype > 0)
	{
		switch (bone->proctype)
		{
		case STUDIO_PROC_JIGGLE:
		{
			JiggleData_t* jiggleData = nullptr;

			if (version == 54)
			{
				const r5::mstudiojigglebone_v8_t* const jigglebone = reinterpret_cast<const r5::mstudiojigglebone_v8_t* const>(bone->pProcedure());

				jiggleData = new JiggleData_t(bone->pszName(), jigglebone->flags);

				// todo: just do this in the constructor (I was lazy tonight)
				jiggleData->FixFlagsApex(); // not ideal but will work for now
				jiggleData->SetGeneral(jigglebone->length, jigglebone->tipMass, jigglebone->tipFriction);
				jiggleData->SetFlexible(jigglebone->yawStiffness, jigglebone->yawDamping, jigglebone->pitchStiffness, jigglebone->pitchDamping, jigglebone->alongStiffness, jigglebone->alongDamping);
				jiggleData->SetAngleConstraint(jigglebone->angleLimit);
				jiggleData->SetYawConstraint(jigglebone->minYaw, jigglebone->maxYaw, jigglebone->yawFriction, jigglebone->yawBounce);
				jiggleData->SetPitchConstraint(jigglebone->minPitch, jigglebone->maxPitch, jigglebone->pitchFriction, jigglebone->pitchBounce);
				jiggleData->SetBaseSpring(jigglebone->baseMass, jigglebone->baseStiffness, jigglebone->baseDamping, jigglebone->baseMinLeft, jigglebone->baseMaxLeft, jigglebone->baseLeftFriction,
					jigglebone->baseMinUp, jigglebone->baseMaxUp, jigglebone->baseUpFriction, jigglebone->baseMinForward, jigglebone->baseMaxForward, jigglebone->baseForwardFriction);
			}
			else
			{
				// temp
				break;
			}

			CmdParse(qc, QC_JIGGLEBONE, jiggleData);

			FreeAllocVar(jiggleData);

			break;
		}
		}
	}
}

constexpr const char* const s_QCModelNameFormat = "%s_%s%s\0"; // file name, model name, extension
static uint32_t s_QCMaxVerts = 0u;
void QC_ParseStudioBodypart(qc::QCFile* const qc, const ModelParsedData_t* const parsedData, const ModelBodyPart_t* const bodypart, const char* const stem, const int setting)
{
	using namespace qc;

	if (bodypart->numModels < 1)
	{
		assertm(false, "invalid model count");
		return;
	}

	assertm(parsedData->LODCount() > 0, "model had bodyparts but no lods");

	const ModelLODData_t* const lodData0 = parsedData->pLOD(0);

	char buf[MAX_PATH]{};

	// single model
	// handle $model (vertex anim) eventually
	if (bodypart->numModels == 1)
	{
		const ModelModelData_t* const modelData = lodData0->pModel(bodypart->modelIndex);

		std::string name(modelData->name);
		FixupExportLodNames(name, 0);
		snprintf(buf, MAX_PATH, s_QCModelNameFormat, stem, name.c_str(), s_ModelExportExtensions[setting]);

		const CommandOptionPair_t bodyData(bodypart->GetName(), buf);

		CmdParse(qc, QC_BODY, &bodyData, 1, true);

		s_QCMaxVerts = modelData->vertCount > s_QCMaxVerts ? modelData->vertCount : s_QCMaxVerts;

		return;
	}

	BodyGroupData_t bodyGroupData(bodypart->GetName(), bodypart->GetModelCount());

	for (uint32_t i = 0; i < static_cast<uint32_t>(bodypart->GetModelCount()); i++)
	{
		const ModelModelData_t* const modelData = lodData0->pModel(bodypart->modelIndex + i);

		if (modelData->meshCount == 0)
		{
			bodyGroupData.SetBlank(i);
			continue;
		}

		std::string name(modelData->name);
		FixupExportLodNames(name, 0);
		snprintf(buf, MAX_PATH, s_QCModelNameFormat, stem, name.c_str(), s_ModelExportExtensions[setting]);

		bodyGroupData.SetPart(qc, i, buf);

		s_QCMaxVerts = modelData->vertCount > s_QCMaxVerts ? modelData->vertCount : s_QCMaxVerts;
	}

	CmdParse(qc, QC_BODYGROUP, &bodyGroupData, 1, true);
}

const uint32_t QC_ParseStudioLODBone(const ModelParsedData_t* const parsedData, const size_t lod, const uint32_t boneId)
{
	const ModelBone_t* const bone = parsedData->pBone(boneId);

	// bone is used by this lod, don't replace
	if (bone->flags & BONE_USED_BY_VERTEX_AT_LOD(lod))
		return boneId;

	// cannot be collasped further
	if (bone->parent == -1)
	{
		// Robots/super_spectre/super_spectre_v1.mdl
		//assertm(false, "bone with no parent unused by LOD");
		return boneId;
	}

	const uint32_t out = QC_ParseStudioLODBone(parsedData, lod, bone->parent);
	return out;
}

void QC_ParseStudioLOD(qc::QCFile* const qc, const ModelParsedData_t* const parsedData, const size_t lod, const bool isShadowLOD, const char* const stem, const int setting)
{
	using namespace qc;

	const ModelLODData_t* const lodData0 = parsedData->pLOD(0ull);
	const ModelLODData_t* const lodData = parsedData->pLOD(lod);
	assertm(lodData->GetModelCount() == lodData0->GetModelCount(), "mismatched model count, very bad!");

	LodData_t lodGroupData(lodData->GetModelCount(), parsedData->BoneCount(), 0u, lodData->switchPoint, isShadowLOD);

	// models
	char bufBase[MAX_PATH]{};
	char bufReplace[MAX_PATH]{};

	for (uint32_t i = 0; i < static_cast<uint32_t>(lodData->GetModelCount()); i++)
	{
		const ModelModelData_t* const modelData0 = lodData0->pModel(i);
		const ModelModelData_t* const modelData = lodData->pModel(i);

		// blank model
		if (modelData->vertCount == 0 && modelData0->vertCount == 0)
			continue;

		std::string base(modelData->name);
		FixupExportLodNames(base, 0);
		snprintf(bufBase, MAX_PATH, s_QCModelNameFormat, stem, base.c_str(), s_ModelExportExtensions[setting]);

		if (modelData->vertCount == 0 && modelData0->vertCount > 0)
		{
			lodGroupData.RemoveModel(qc, i, bufBase);
			continue;
		}

		std::string replace(modelData->name);
		FixupExportLodNames(replace, static_cast<int>(lod));
		snprintf(bufReplace, MAX_PATH, s_QCModelNameFormat, stem, replace.c_str(), s_ModelExportExtensions[setting]);

		lodGroupData.ReplaceModel(qc, i, bufBase, bufReplace);

		s_QCMaxVerts = modelData->vertCount > s_QCMaxVerts ? modelData->vertCount : s_QCMaxVerts;
	}

	// bones
	// [rika]: do we need to check for attachments ? I don't think we can remove a bone if it has attachments
	for (uint32_t i = 0; i < static_cast<uint32_t>(parsedData->BoneCount()); i++)
	{
		const uint32_t boneId = QC_ParseStudioLODBone(parsedData, lod, i);

		if (boneId == i)
			continue;

		const ModelBone_t* const pBoneChild = parsedData->pBone(i);
		const ModelBone_t* const pBoneParent = parsedData->pBone(boneId);

		lodGroupData.ReplaceBone(i, pBoneChild->pszName(), pBoneParent->pszName());
	}

	if (lodGroupData.isShadowLOD && parsedData->flags & STUDIOHDR_FLAGS_USE_SHADOWLOD_MATERIALS)
		lodGroupData.useShadowLODMaterials = true;

	const CommandList_t lodType = isShadowLOD ? QC_SHADOWLOD : QC_LOD;

	CmdParse(qc, lodType, &lodGroupData);
}

void QC_ParseStudioAttachments(qc::QCFile* const qc, const ModelParsedData_t* const parsedData)
{
	using namespace qc;

	const bool staticProp = parsedData->flags & STUDIOHDR_FLAGS_STATIC_PROP;
	const bool useIllumAttachment = IllumPositionData_t::useAttachment(parsedData->illumpositionattachmentindex);
	int illumpositionattachmentindex = parsedData->illumpositionattachmentindex - 1;

	bool useAutoCenter = false;
	//size_t autoCenterIndex = 0ull;

	const int numAttachments = parsedData->numAttachments;
	for (int i = 0; i < numAttachments; i++)
	{
		// skip __illumPosition attachment
		if (useIllumAttachment && illumpositionattachmentindex == i)
			continue;

		const ModelAttachment_t* const attachment = parsedData->pAttachment(i);

		// should we skip this
		if (staticProp && !useAutoCenter && !strncmp("placementOrigin", attachment->name, 16))
		{
			useAutoCenter = true;
			//autoCenterIndex = i;
		}

		// invalid index in apex (sometimes?), yippee!!!
		// rspn101 always +8
		if (useIllumAttachment && illumpositionattachmentindex >= numAttachments && !strncmp("__illumPosition", attachment->name, 16))
		{
			illumpositionattachmentindex = i;
			continue;
		}

		const AttachmentData_t attachmentData(attachment->name, parsedData->pBone(attachment->localbone)->pszName(), attachment->flags, attachment->localmatrix);

		CmdParse(qc, QC_ATTACHMENT, &attachmentData);
	}

	// illumposition
	// [rika]: not a fan of this being done outside qc, but also don't wanna put out custom types into qc (supposed to be universial for [redacted])
	{
		const char* localbone = nullptr;
		const matrix3x4_t* localmatrix = nullptr;

		if (useIllumAttachment)
		{
			const ModelAttachment_t* const pAttachment = parsedData->pAttachment(illumpositionattachmentindex); // use fixed index incase it is invalid

			localbone = parsedData->pBone(pAttachment->localbone)->name;
			localmatrix = pAttachment->localmatrix;
		}

		const IllumPositionData_t illumdata(&parsedData->illumposition, parsedData->illumpositionattachmentindex, localbone, localmatrix);

		CmdParse(qc, QC_ILLUMPOSITION, &illumdata);
	}

	if (useAutoCenter)
	{
		CmdParse(qc, QC_AUTOCENTER, nullptr);
	}
}

void QC_ParseStudioBoxes(qc::QCFile* const qc, const ModelParsedData_t* const parsedData)
{
	using namespace qc;

	const bool autoGeneratedHitbox = parsedData->flags & STUDIOHDR_FLAGS_AUTOGENERATED_HITBOX;
	const CommandFormat_t cmdFmt = autoGeneratedHitbox ? QC_FMT_COMMENT : QC_FMT_NONE;
	bool skipBoneInBBox = false;

	const CommandOptionPair_t bboxData(parsedData->hull_min.Base(), parsedData->hull_max.Base(), 3, 3);
	CmdParse(qc, QC_BBOX, &bboxData);

	const CommandOptionPair_t cboxData(parsedData->view_bbmin.Base(), parsedData->view_bbmax.Base(), 3, 3);
	CmdParse(qc, QC_CBOX, &cboxData);

	// [rika]: it's important to parse these in order
	for (int i = 0; i < parsedData->HitboxSetCount(); i++)
	{
		const ModelHitboxSet_t* const hitboxSetData = parsedData->pHitboxSet(i);
		CmdParse(qc, QC_HBOXSET, hitboxSetData->name, 1, false, cmdFmt);

		for (int boxIdx = 0; boxIdx < hitboxSetData->numHitboxes; boxIdx++)
		{
			const ModelHitbox_t* const hitboxData = hitboxSetData->hitboxes + boxIdx;
			const Hitbox_t hboxData(parsedData->pBone(hitboxData->bone)->pszName(), hitboxData->group, hitboxData->bbmin, hitboxData->bbmax, hitboxData->name, nullptr, hitboxData->forceCritPoint);

			CmdParse(qc, QC_HBOX, &hboxData, 1, false, cmdFmt);

			// hgroup
			if (autoGeneratedHitbox)
			{
				const CommandOptionPair_t hgroupData(hitboxData->group, parsedData->pBone(hitboxData->bone)->pszName());
				CmdParse(qc, QC_HGROUP, &hgroupData);
			}

			if (autoGeneratedHitbox && !skipBoneInBBox)
			{
				const Vector& bbmin = *hitboxData->bbmin;
				const Vector& bbmax = *hitboxData->bbmax;

				for (uint32_t axis = 0; axis < 3; axis++)
				{
					skipBoneInBBox = bbmin[axis] > 0.0f || bbmax[axis] < 0.0f ? true : skipBoneInBBox;
				}
			}
		}
	}

	if (skipBoneInBBox)
	{
		CmdParse(qc, QC_SKIPBONEINBBOX, nullptr);
	}
}

void QC_ParseStudioAnimationTypes(qc::QCFile* const qc, const ModelParsedData_t* const parsedData)
{
	using namespace qc;

	for (int i = 0; i < parsedData->numPoseParm; i++)
	{
		assertm(parsedData->PoseParamCount(), "invalid pointer, wicked bad");

		const ModelPoseParam_t* const poseParam = parsedData->pPoseParam(i);
		const PoseParamData_t poseParamData(poseParam->name, poseParam->flags, poseParam->start, poseParam->end, poseParam->loop);
		CmdParse(qc, QC_POSEPARAMETER, &poseParamData);
	}

	for (int i = 0; i < parsedData->IKChainCount(); i++)
	{
		assertm(parsedData->ikChains, "invalid pointer, wicked bad");

		const ModelIKChain_t* const ikChain = parsedData->pIKChain(i);
		const IKChainData_t ikChainData(ikChain->name, parsedData->pBone(ikChain->links[ModelIKChain_t::IKLINK_FOOT].bone)->pszName(), &ikChain->links[ModelIKChain_t::IKLINK_THIGH].kneeDir, ikChain->unk_10);
		CmdParse(qc, QC_IKCHAIN, &ikChainData);
	}

	for (int i = 0; i < parsedData->numIkLocks; i++)
	{
		assertm(parsedData->ikLocks, "invalid pointer, wicked bad");

		const ModelIKLock_t* const ikLock = parsedData->pIKLock(i);
		assertm(ikLock->chain < parsedData->IKChainCount(), "invaild ik chain index");

		const IKLockData_t ikLockData(parsedData->pIKChain(ikLock->chain)->name, ikLock->flPosWeight, ikLock->flLocalQWeight);
		CmdParse(qc, QC_IKAUTOPLAYLOCK, &ikLockData);
	}

	for (int i = 0; i < parsedData->IncludeModelCount(); i++)
	{
		const mstudiomodelgroup_t* const includemodel = parsedData->pIncludeModel(i);
		CmdParse(qc, QC_INCLUDEMODEL, includemodel->pszName());
	}

	for (int i = 0; i < parsedData->numExternalIncludeModels; i++)
	{
		const uint64_t guid = parsedData->externalIncludeModels[i].guid;

		CPakAsset* const animRigAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

		if (nullptr == animRigAsset)
		{
			continue;
		}

		AnimRigAsset* const animRig = reinterpret_cast<AnimRigAsset* const>(animRigAsset->extraData());

		if (nullptr == animRig)
		{
			continue;
		}

		CmdParse(qc, QC_INCLUDEMODEL, animRig->name);
	}
}

struct ModelSectionData_t
{
	ModelSectionData_t(const int minFrames = qc::s_SectionFrames_DefaultMinFrame) : sectionFrameAccumulated(0), numSectionAnims(0), numMinFrames(minFrames) {}

	int sectionFrameAccumulated;
	int numSectionAnims;
	int numMinFrames;

	inline void AccumulateFrames(const int sectionframes)
	{
		sectionFrameAccumulated += sectionframes;
		numSectionAnims++;
	}

	inline void ParseMinFrames(const int numframes)
	{
		numMinFrames = numMinFrames < numframes ? numMinFrames : numframes;
	}

	inline const int SectionFrames() const
	{
		if (sectionFrameAccumulated == 0)
		{
			return 0;
		}

		const int mean = sectionFrameAccumulated / numSectionAnims;

		const int roundit = (mean + 9);
		const int out = roundit - (roundit % 10);

		return out;
	}
};

static const float s_WeightListDefault[256] = {
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
};

struct ModelWeightList_t
{
	ModelWeightList_t() = default;
	ModelWeightList_t(const float* const listWeights) : name(nullptr), weights(listWeights), useCount(1), markedAsDefault(true) {}
	ModelWeightList_t(const char* const listName, const float* const listWeights) : name(listName), weights(listWeights), useCount(1), markedAsDefault(false)
	{

	}

	const char* name;
	const float* weights;
	int useCount;
	bool markedAsDefault;

	static void NameFromIndex(char* const buf, const size_t size, const uint32_t index)
	{
		snprintf(buf, size, "weightlist_%u", index);
	}
};

struct ModelAnimInfo_t
{
	ModelAnimInfo_t(const ModelAnim_t* const source, const char* const uniqueId, const char* const uniqueName) : anim(source), unique(uniqueId), name(uniqueName) {};

	const ModelAnim_t* anim;
	const char* unique; // unique name
	const char* name;
};

struct ModelSeqInfo_t
{
	ModelSeqInfo_t() : seq(nullptr), blends(nullptr) {}

	const ModelSeq_t* seq;
	int* blends;
};

void QC_ParseStudioAnimation_OptionsFromFlags(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const int flags, qc::AnimationOptions_t* const animOptions)
{
	UNUSED(file);
	UNUSED(parsedData);

	// traditional
	if (flags & STUDIO_LOOPING)
	{
		animOptions->SetLooping();
	}

	if (flags & STUDIO_SNAP)
	{
		animOptions->SetSnap();
	}

	if (flags & STUDIO_DELTA)
	{
		animOptions->SetDelta();
	}

	if (flags & STUDIO_AUTOPLAY)
	{
		animOptions->SetAutoPlay();
	}

	// if STUDIO_POST can be set via commands, but those commands set other flags too, thankfully
	if (flags & STUDIO_POST && ((STUDIO_DELTA | STUDIO_WORLD) & flags) == false)
	{
		animOptions->SetPost();
	}

	if (flags & STUDIO_ALLZEROS)
	{
		animOptions->SetNoAnim();
	}

	if (flags & STUDIO_REALTIME)
	{
		animOptions->SetRealTime();
	}

	if (flags & STUDIO_HIDDEN)
	{
		animOptions->SetHidden();
	}

	if (flags & STUDIO_WORLD)
	{
		animOptions->SetWorldSpace();
	}

	if (flags & STUDIO_NOFORCELOOP)
	{
		animOptions->SetNoForceLoop();
	}

	// respawn specific
	if (flags & STUDIO_FRAMEMOVEMENT)
	{
		animOptions->SetRootMotion();
	}

	if (flags & STUDIO_ANIM_UNK40)
	{
		animOptions->SetSuppressGestures();
	}

	if ((flags & STUDIO_HAS_ANIM) == false)
	{
		animOptions->SetDefaultPose();
	}
}

void QC_ParseStudioAnimation_IKRules(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const ModelAnim_t* const anim, qc::AnimationData_t* const animData, qc::AnimationOptions_t* const animOptions)
{
	UNUSED(parsedData);

	using namespace qc;

	const uint32_t numIkChains = parsedData->IKLockCount();
	const int numIkRules = anim->numikrules;

	// [rika]: nothing to parse, ikrules require ikchains
	if (numIkChains == 0u)
	{
		return;
	}

	// [rika]: force no ikrules as they would be generated otherwise
	if (numIkRules == 0u)
	{
		animOptions->SetAutoIK(false);
		return;
	}

	int useCount[16]{};

	assertm(16 >= numIkChains, "too many ikchains");

	for (int i = 0; i < numIkRules; i++)
	{
		const ModelIKRule_t* const ikRule = anim->pIKRule(i);

		useCount[ikRule->chain]++;
	}

	bool autoIk = false;
	bool ignoredIKRules[MAXSTUDIOIKRULES]{};
	int usedIKRules = numIkRules;

	// auto generated ikrules will be the last ones
	for (int i = numIkRules - numIkChains; i < numIkRules; i++)
	{
		if (i < 0)
		{
			break;
		}

		const ModelIKRule_t* const ikRule = anim->pIKRule(i);

		// automatic ikrules are always IK_RELEASE, and only created if there is zero existing rules for that chain
		if (ikRule->type != ModelIKRule_t::IKRULE_RELEASE || useCount[ikRule->chain] > 1)
		{
			continue;
		}

		if (ikRule->start == 0.0f && ikRule->peak == 0.0f && ikRule->tail == 1.0f && ikRule->end == 1.0f)
		{
			ignoredIKRules[i] = true;
			autoIk = true;
			usedIKRules--;
		}
	}

	animOptions->SetAutoIK(autoIk);

	if (usedIKRules == 0)
		return;

	animData->EnableIKRules(file, usedIKRules);
	for (int i = 0, idx = 0; i < usedIKRules; idx++)
	{
		if (ignoredIKRules[idx])
		{
			continue;
		}

		const ModelIKRule_t* const ikRule = anim->pIKRule(idx);

		const int chain = ikRule->chain;
		const int bone = ikRule->bone;

		// init our ik rule
		animData->SetIKRule(i, ikRule->type, chain, bone, ikRule->slot, parsedData->pBone(bone)->pszName(), parsedData->pIKChain(chain)->name, ikRule->attachment);
		animData->ikRules[i].SetRange(ikRule->start, ikRule->peak, ikRule->tail, ikRule->end);
		animData->ikRules[i].SetOrigin(&ikRule->pos, &ikRule->q);
		animData->ikRules[i].SetFoot(ikRule->height, ikRule->radius, ikRule->floor, ikRule->endHeight);
		animData->ikRules[i].SetFootContact(ikRule->contact, ikRule->drop, ikRule->top);

		// mark as fixup if it has ikerrors

		i++;
	}
}

void QC_ParseStudioAnimation_Movement(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const ModelAnim_t* const anim, qc::AnimationData_t* const animData)
{
	UNUSED(parsedData);

	using namespace qc;

	const int numMovements = anim->nummovements;

	if (numMovements == 0)
		return;

	assertm(MAXSTUDIOMOVEKEYS >= numMovements, "animation exceeded maximum movements"); // non vanilla studio mdl

	const mstudiomovement_t* motion = anim->pMovement(0);
	int prevMotionFlags = motion->motionflags;
	int prevEndFrame = motion->endframe;

	AnimationMotion_t motions[MAXSTUDIOMOVEKEYS]{};
	int motionIndex = 0;

	for (int i = 1; i < numMovements; i++)
	{
		motion = anim->pMovement(i);
		const int motionFlags = motion->motionflags;
		const int endFrame = motion->endframe;

		if (motionFlags == prevMotionFlags)
		{
			prevEndFrame = endFrame;
			continue;
		}

		motions[motionIndex].motionflags = prevMotionFlags;
		motions[motionIndex].endframe = prevEndFrame;

		prevMotionFlags = motionFlags;
		prevEndFrame = endFrame;

		motionIndex++;
	}

	// set last motion
	motions[motionIndex].motionflags = prevMotionFlags;
	motions[motionIndex].endframe = prevEndFrame;

	motionIndex++;

	animData->EnableMotion(file, motionIndex);

	for (int i = 0; i < animData->numMotions; i++)
	{
		animData->SetMotion(i, motions[i].endframe, motions[i].motionflags);
	}
}

void QC_ParseStudioAnimation_Sections(qc::QCFile* const file, const ModelAnim_t* const anim, ModelSectionData_t& sectionFrame)
{
	UNUSED(file);

	using namespace qc;

	// parse data for section frames
	// apex should just be 60 but we will aproximate based off the variety of values
	const int numFrames = anim->numframes;
	const int sectionFrames = anim->sectionframes;
	if (sectionFrames)
	{
		sectionFrame.AccumulateFrames(sectionFrames);
		sectionFrame.ParseMinFrames(numFrames);
	}
}

void QC_ParseStudioWeightList(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const ModelSeqInfo_t* const sequences, const uint32_t numSeq, ModelWeightList_t* const weightLists, uint32_t* const listForSequence)
{
	using namespace qc;

	uint32_t weightListIndex = 0;

	// add the default weight list (all ones)
	weightLists[weightListIndex] = ModelWeightList_t(s_WeightListDefault);
	weightListIndex++;

	// cycle through sequences, mark which weightlist they use, and add new ones if found
	for (uint32_t i = 0u; i < numSeq; i++)
	{
		const ModelSeq_t* const seq = sequences[i].seq;

		if (seq == nullptr)
		{
			continue;
		}

		const float* const seqWeights = seq->weights;
		const size_t seqWeightSize = sizeof(float) * parsedData->BoneCount();

		bool listExists = false;
		for (uint32_t listIdx = 0; listIdx < weightListIndex; listIdx++)
		{
			ModelWeightList_t* const weightList = weightLists + listIdx;

			const int result = memcmp(weightList->weights, seqWeights, seqWeightSize);

			if (result)
			{
				continue;
			}

			listForSequence[i] = listIdx;
			weightList->useCount++;
			listExists = true;

			break;
		}

		// we don't have to add a new weight list
		if (listExists)
		{
			continue;
		}

		// weightlist name
		constexpr size_t nameBufSize = 32ull;
		char nameBuf[nameBufSize]{};
		ModelWeightList_t::NameFromIndex(nameBuf, nameBufSize, weightListIndex);
		const char* const listName = file->WriteString(nameBuf);

		assertm((MAXSTUDIOWEIGHTLIST + 1) > weightListIndex, "exceeded max allowed weight lists");

		// add a new weight list and seq this sequence as using that new weight list
		listForSequence[i] = weightListIndex;
		weightLists[weightListIndex] = ModelWeightList_t(listName, seqWeights);
		weightListIndex++;
	}

	// build bone name list for weight list commands
	const char** bones = reinterpret_cast<const char**>(file->ReserveData(sizeof(char*) * parsedData->BoneCount()));
	for (int i = 0u; i < parsedData->BoneCount(); i++)
	{
		bones[i] = parsedData->pBone(i)->name;
	}

	// add weight list commands
	for (uint32_t i = 0; i < weightListIndex; i++)
	{
		const ModelWeightList_t* weightList = weightLists + i;
		const WeightListData_t weightListData(weightList->name, bones, weightList->weights, parsedData->BoneCount(), weightList->markedAsDefault);

		if (weightList->markedAsDefault)
		{
			CmdParse(file, QC_DEFAULTWEIGHTLIST, &weightListData, 1u, false, QC_FMT_ARRAYSTYLE);

			continue;
		}

		CmdParse(file, QC_WEIGHTLIST, &weightListData, 1u, false, QC_FMT_ARRAYSTYLE);
	}
}

void FormNameAnim(char* const buf, const size_t size, const char* const stem, const char* const name, const int format)
{
	assertm(strnlen_s(name, MAX_PATH) < (size - 16ull), "big animation name");
	snprintf(buf, size, "anims_%s/%s%s", stem, name, s_ModelExportExtensions[format]);
}

void QC_ParseStudioAnimation(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const ModelAnimInfo_t* const info, const char* const stem, const int setting)
{
	using namespace qc;

	const ModelAnim_t* const anim = info->anim;
	const char* const name = info->name;

	if (anim->IsOverriden())
	{
		CmdParse(file, QC_DECLAREANIM, name);
		return;
	}

	// @ is auto added from sequence
	if (anim->IsSeqDeclared())
	{
		return;
	}

	// I saw what this is for but cannot find it now
	if (anim->IsNullName())
	{
		assertm(false, "funky animation");
		return;
	}

	constexpr size_t filepathLength = MAX_PATH;
	char filepath[filepathLength];
	FormNameAnim(filepath, filepathLength, stem, anim->pszName(), setting);

	AnimationData_t animData(name, filepath, anim->fps, anim->numframes);
	AnimationOptions_t animOptions(0u, AnimationOptions_t::ANIM_OPT_FEATURE_ANIM);

	QC_ParseStudioAnimation_OptionsFromFlags(file, parsedData, anim->flags, &animOptions);
	QC_ParseStudioAnimation_IKRules(file, parsedData, anim, &animData, &animOptions);
	QC_ParseStudioAnimation_Movement(file, parsedData, anim, &animData);

	// TEMP, need to store a fixup animation path here
	if (animOptions.IsDelta())
	{
		animData.SetSubtractAnim("");
	}

	animOptions.SetFPS();

	animData.SetOptions(animOptions);

	CmdParse(file, QC_ANIMATION, &animData, 1u, false, QC_FMT_ARRAYSTYLE);
}

void QC_ParseStudioSequence(qc::QCFile* const file, const ModelParsedData_t* const parsedData, const ModelSeqInfo_t* const info, const ModelAnimInfo_t* const animations, const char* const weightlist, const char* const stem, const int setting)
{
	using namespace qc;

	const ModelSeq_t* const seq = info->seq;

	char tmpName[MAX_PATH]{};
	strncpy_mem(tmpName, MAX_PATH, GetStringAfterLastSlash(seq->szlabel), MAX_PATH);
	removeExtension(tmpName);

	const char* const label = file->WriteString(tmpName);

	if (seq->IsOverriden())
	{
		CmdParse(file, QC_DECLARESEQ, label);
		return;
	}

	const int numBlends = seq->numblends;
	assertm(numBlends, "sequence without animations");

	// frame data is based off of the first animation
	const ModelAnim_t* const firstAnim = seq->Anim(0);

	SequenceData_t seqData(file, label, seq->szactivityname, seq->actweight, firstAnim->fps, firstAnim->numframes, weightlist, seq->groupsize[0], seq->groupsize[1], numBlends, seq->fadeintime, seq->fadeouttime,
		seq->entryphase, seq->exitphase, seq->keyvalues, seq->ikResetMask);

	// cycle through all animations and add their name or path, then parse the first animation to be implied, if there is one
	bool impliedAnimation = false;
	int firstImpliedAnim = -1;
	for (int i = 0; i < numBlends; i++)
	{
		const ModelAnimInfo_t* const animInfo = animations + info->blends[i];
		const ModelAnim_t* const anim = animInfo->anim;

		const char* name = animInfo->name;

		if (anim->IsSeqDeclared())
		{
			constexpr size_t filepathLength = MAX_PATH;
			char filepath[filepathLength];
			FormNameAnim(filepath, filepathLength, stem, anim->pszName(), setting);

			name = file->WriteString(filepath);

			if (impliedAnimation == false)
			{
				impliedAnimation = true;
				firstImpliedAnim = i;
			}
		}

		seqData.SetBlend(i, name);
	}

	AnimationOptions_t animOptions(0u, impliedAnimation ? AnimationOptions_t::ANIM_OPT_FEATURE_ALL : AnimationOptions_t::ANIM_OPT_FEATURE_SEQ);

	// parse animation data since we won't be writing an $animation command for this one
	if (impliedAnimation)
	{
		assertm(firstImpliedAnim == 0, "not ideal but someone could do this");

		const ModelAnim_t* const anim = seq->Anim(firstImpliedAnim);

		QC_ParseStudioAnimation_OptionsFromFlags(file, parsedData, anim->flags, &animOptions);
		QC_ParseStudioAnimation_IKRules(file, parsedData, anim, &seqData, &animOptions);
		QC_ParseStudioAnimation_Movement(file, parsedData, anim, &seqData);


		// TEMP, need to store a fixup animation path here
		// should work with multiple animations?
		if (animOptions.IsDelta())
		{
			seqData.SetSubtractAnim("");
		}

		animOptions.SetFPS();
	}

	// parse flags from sequence now that we've potentially set options from the implied animation
	constexpr int flagMask = ~STUDIO_HAS_SCALE;
	const int flags = (seq->flags & flagMask);
	// !!!!!
	QC_ParseStudioAnimation_OptionsFromFlags(file, parsedData, flags, &animOptions);
	seqData.SetOptions(animOptions);

	// parse nodes
	// if either are set, both should be set
	const int localEntryNode = seq->localentrynode;
	const int localExitNode = seq->localexitnode;
	if (localEntryNode || localExitNode)
	{
		// apex legends can have either or node set

		const char* const entryNode = localEntryNode ? parsedData->pszNodeName(localEntryNode - 1) : nullptr;
		const char* const exitNode = localExitNode ? parsedData->pszNodeName(localExitNode - 1) : nullptr;

		seqData.SetupNodes(entryNode, exitNode, localEntryNode, localExitNode, seq->nodeflags);
	}

	// parse poseparms if the sequence has any
	for (int i = 0; i < 2; i++)
	{
		const int paramIndex = seq->paramindex[i];

		// [rika]: apex legends seems to support setting these via independent blend options, making them not linear (for vertical blends)
		if (paramIndex == -1)
		{
			seqData.SetParam(i, nullptr, seq->paramstart[i], seq->paramend[i]);
			continue;
		}

		seqData.SetParam(i, parsedData->pPoseParam(paramIndex)->name, seq->paramstart[i], seq->paramend[i]);
	}

	// setup events
	const int numEvents = seq->numevents;
	if (numEvents)
	{
		seqData.EnableEvents(file, numEvents);

		for (int i = 0; i < numEvents; i++)
		{
			const ModelEvent_t* const event = seq->pEvent(i);
			seqData.SetEvent(i, event->name, event->event, event->type & NEW_EVENT_STYLE, event->cycle, event->unk, event->options);
		}
	}

	// setup autolayers
	const int numLayers = seq->numautolayers;
	if (numLayers)
	{
		seqData.EnableLayers(file, numLayers);

		for (int i = 0; i < numLayers; i++)
		{
			const ModelAutoLayer_t* const layer = seq->pAutoLayer(i);

			const int layerFlags = layer->flags;

			const char* sequence = nullptr;
			if (layer->iSequence > -1)
			{
				// should never not be external (cannot exist in pak based models)
				sequence = parsedData->pLocalSeq(layer->iSequence)->szlabel;
			}
			else
			{
				CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(layer->sequence.guid);

				if (nullptr == animSeqAsset)
				{
					assertm(false, "sequence not found!");
					continue;
				}

				AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());

				if (nullptr == animSeq)
				{
					assertm(false, "sequence not found!");
					continue;
				}

				strncpy_mem(tmpName, MAX_PATH, GetStringAfterLastSlash(animSeq->seqdesc.szlabel), MAX_PATH);
				removeExtension(tmpName);


				sequence = file->WriteString(tmpName);
			}

			const char* const pose = layerFlags & STUDIO_AL_POSE ? parsedData->pPoseParam(layer->iPose)->name : nullptr;

			seqData.SetLayer(i, sequence, pose, layerFlags, layer->start, layer->peak, layer->tail, layer->end);
		}
	}

	// setup ik locks
	const int numIKLocks = seq->numiklocks;
	if (numIKLocks)
	{
		seqData.EnableIKLocks(file, numIKLocks);

		for (int i = 0; i < numIKLocks; i++)
		{
			const ModelIKLock_t* const ikLock = seq->pIKLock(i);
			seqData.SetIKLock(i, parsedData->pIKChain(ikLock->chain)->name, ikLock->flPosWeight, ikLock->flLocalQWeight);
		}
	}

	if (seq->flags & STUDIO_CYCLEPOSE)
	{
		seqData.SetCyclePose(parsedData->pPoseParam(seq->cycleposeindex)->name);
	}

	// setup activity modifiers
	const int numActMods = seq->numactivitymodifiers;
	if (numActMods)
	{
		seqData.EnableActMods(file, numActMods);

		for (int i = 0; i < numActMods; i++)
		{
			const ModelActivityModifier_t* const actmod = seq->pActivityModifier(i);
			seqData.SetActMod(i, actmod->name, actmod->negate);
		}
	}

	CmdParse(file, QC_SEQUENCE, &seqData, 1u, false, QC_FMT_ARRAYSTYLE);
}

ModelSeqInfo_t* const BuildSequenceList(const ModelParsedData_t* const parsedData, const int numSeqs)
{
	assertm(numSeqs == (parsedData->LocalSeqCount() + parsedData->ExternalSeqCount()), "unexpected number of sequences");
	ModelSeqInfo_t* sequences = new ModelSeqInfo_t[numSeqs]{};

	int seqIdx = 0;
	for (int i = 0; i < parsedData->LocalSeqCount(); i++, seqIdx++)
	{
		sequences[seqIdx].seq = parsedData->pLocalSeq(i);
	}

	for (int i = 0; i < parsedData->ExternalSeqCount(); i++, seqIdx++)
	{
		const uint64_t guid = parsedData->externalSequences[i].guid;

		CPakAsset* const animSeqAsset = g_assetData.FindAssetByGUID<CPakAsset>(guid);

		if (nullptr == animSeqAsset)
		{
			continue;
		}

		AnimSeqAsset* const animSeq = reinterpret_cast<AnimSeqAsset* const>(animSeqAsset->extraData());

		if (nullptr == animSeq)
		{
			continue;
		}

		sequences[seqIdx].seq = &animSeq->seqdesc;
	}

	return sequences;
}

const char* const BuildUniqueAnimationString(qc::QCFile* const file, const ModelAnim_t* const anim)
{
	char tmp[MAX_PATH]{};
	snprintf(tmp, MAX_PATH, "%s_fps_%f_flags_%x_rules_%i\0", anim->pszName(), anim->fps, anim->flags, anim->numikrules);

	const char* const unique = file->WriteString(tmp);

	return unique;
}

const char* const BuildUniqueAnimationName(qc::QCFile* const file, const ModelAnim_t* const anim, const int index)
{
	char tmp[MAX_PATH]{};
	strncpy_mem(tmp, MAX_PATH, anim->pszName(), MAX_PATH);

	char* const end = strstr(tmp, "_dmx__");

	if (end == nullptr)
	{
		return anim->pszName();
	}

	end[0] = '\0';
	snprintf(tmp, MAX_PATH, "%s__%i", tmp, index);

	const char* const name = file->WriteString(tmp);

	return name;
}

void BuildUniquieAnimationList(qc::QCFile* const file, ModelSeqInfo_t* const sequences, const int numSeqs, std::vector<ModelAnimInfo_t>& animationList)
{
	std::unordered_map<uint64_t, int> uniqueAnimations;

	const size_t maxAnimEstimate = numSeqs * 2;
	uniqueAnimations.reserve(maxAnimEstimate);
	animationList.reserve(maxAnimEstimate);

	for (int i = 0; i < numSeqs; i++)
	{
		ModelSeqInfo_t* const seqInfo = sequences + i;
		const ModelSeq_t* const seq = seqInfo->seq;

		if (seq == nullptr)
		{
			continue;
		}

		seqInfo->blends = reinterpret_cast<int* const>(file->ReserveData(sizeof(int) * seq->AnimCount()));

		for (int idx = 0; idx < seq->AnimCount(); idx++)
		{
			const ModelAnim_t* const anim = seq->Anim(idx);
			const char* const uniqueId = BuildUniqueAnimationString(file, anim);

			const uint64_t hash = RTech::StringToGuid(uniqueId);

			if (uniqueAnimations.contains(hash))
			{
				const int index = uniqueAnimations.at(hash);
				
				seqInfo->blends[idx] = index;
 
				continue;
			}

			const int index = static_cast<int>(animationList.size());
			const char* const uniqueName = BuildUniqueAnimationName(file, anim, index);

			uniqueAnimations.emplace(hash, index);
			animationList.emplace_back(anim, uniqueId, uniqueName);

			seqInfo->blends[idx] = index;
		}
	}
}

bool ExportModelQC(const ModelParsedData_t* const parsedData, std::filesystem::path& exportPath, const int setting, const int version)
{
#if defined(HAS_QC)
	using namespace qc;

	if (parsedData->baseptr == nullptr)
		return false;

	const std::string fileStem(exportPath.stem().string());

	CManagedBuffer* buf = g_BufferManager.ClaimBuffer();

	QCFile qcFile(exportPath, buf->Buffer(), managedBufferSize);

	QC_ParseStudioHeader(&qcFile, parsedData, version);

	for (int i = 0; i < parsedData->BoneCount(); i++)
		QC_ParseStudioBone(&qcFile, parsedData, parsedData->pBone(i), version);

	s_QCMaxVerts = 0u;
	for (size_t i = 0; i < parsedData->BodypartCount(); i++)
		QC_ParseStudioBodypart(&qcFile, parsedData, parsedData->pBodypart(i), fileStem.c_str(), setting);

	if (parsedData->LODCount() > 1)
	{
		for (size_t i = 1; i < parsedData->LODCount(); i++)
		{
			if (parsedData->flags & STUDIOHDR_FLAGS_HASSHADOWLOD && parsedData->pLOD(i)->switchPoint == -1.0f)
			{
				QC_ParseStudioLOD(&qcFile, parsedData, i, true, fileStem.c_str(), setting);
				continue;
			}


			QC_ParseStudioLOD(&qcFile, parsedData, i, false, fileStem.c_str(), setting);
		}
	}

	// do $maxverts command
	s_QCMaxVerts++;
	assertm(s_QCMaxVerts < s_MaxStudioVerts, "invalid max vert value");
	
	constexpr int maxVertThreshold = (s_MaxStudioVerts / 3);
	if (s_QCMaxVerts > maxVertThreshold)
	{
		CmdParse(&qcFile, QC_MAXVERTS, &s_QCMaxVerts);
	}

	// attachments
	QC_ParseStudioAttachments(&qcFile, parsedData);

	// boxes and boxes and boxes and
	QC_ParseStudioBoxes(&qcFile, parsedData);

	// animation and sequences
	QC_ParseStudioAnimationTypes(&qcFile, parsedData);

	const int numSeqs = parsedData->LocalSeqCount() + parsedData->ExternalSeqCount();
	if (numSeqs)
	{
		// first we have to build a list of sequences and animations
		ModelSeqInfo_t* const sequences = BuildSequenceList(parsedData, numSeqs);

		std::vector<ModelAnimInfo_t> animationList;
		BuildUniquieAnimationList(&qcFile, sequences, numSeqs, animationList);

		// pre parse weight lists
		constexpr size_t maxWeightListSize = sizeof(ModelWeightList_t) * (MAXSTUDIOWEIGHTLIST + 1); // 128 weight lists max plus the default one
		ModelWeightList_t* const weightLists = reinterpret_cast<ModelWeightList_t*>(qcFile.ReserveData(maxWeightListSize));
		uint32_t* const listForSequence = reinterpret_cast<uint32_t*>(qcFile.ReserveData(sizeof(uint32_t) * numSeqs));

		QC_ParseStudioWeightList(&qcFile, parsedData, sequences, numSeqs, weightLists, listForSequence);

		// parse animation commands
		const size_t numAnims = animationList.size();
		if (numAnims)
		{
			// $sectionframes
			ModelSectionData_t sectionFrameData;

			// $animation
			for (size_t i = 0u; i < numAnims; i++)
			{
				const ModelAnimInfo_t* const info = &animationList.at(i);

				QC_ParseStudioAnimation_Sections(&qcFile, info->anim, sectionFrameData);
				QC_ParseStudioAnimation(&qcFile, parsedData, info, fileStem.c_str(), setting);
			}

			const int sectionFrameCount = sectionFrameData.SectionFrames();

			if (sectionFrameCount > 0 && (sectionFrameCount != s_SectionFrames_DefaultFrameCount || sectionFrameData.numMinFrames != s_SectionFrames_DefaultMinFrame))
			{
				const SectionFrameData_t sectionFrames(sectionFrameCount, sectionFrameData.numMinFrames);
				CmdParse(&qcFile, QC_SECTIONFRAMES, &sectionFrames);
			}
		}

		for (int i = 0u; i < numSeqs; i++)
		{
			const ModelSeqInfo_t* const info = sequences + i;

			if (info->seq == nullptr)
			{
				continue;
			}

			QC_ParseStudioSequence(&qcFile, parsedData, info, animationList.data(), weightLists[listForSequence[i]].name, fileStem.c_str(), setting);
		}
	}

	// get version from settings
	Version_t qcTargetVersion;
	qcTargetVersion.SetVersion(static_cast<MajorVersion_t>(g_rsxSettings.qcMajorVersion), g_rsxSettings.qcMinorVersion);

	// export qc file
	QCFile::SetExportVersion(qcTargetVersion);
	qcFile.ParseToText(g_rsxSettings.exportQCIFiles);

	g_BufferManager.RelieveBuffer(buf);
#else
	UNUSED(version); UNUSED(setting); UNUSED(exportPath); UNUSED(parsedData);
#endif

	return true;
}

bool ExportSeqQC(const ModelParsedData_t* const parsedData, const ModelSeq_t* const sequence, std::filesystem::path& exportPath, const int setting, const int version)
{
	using namespace qc;

	// [rika]: don't bother for other studio versions, no point as all animations are local
	if (version != 54)
	{
		return true;
	}

	const std::string fileStem(exportPath.stem().string());

	CManagedBuffer* buf = g_BufferManager.ClaimBuffer();

	QCFile qcFile(exportPath, buf->Buffer(), managedBufferSize);

	CmdParse(&qcFile, QC_MODELNAME, sequence->szlabel);

	// [rika]: write the weight list
	char weightListName[MAX_PATH]{};
	snprintf(weightListName, MAX_PATH, "%s_weightlist", fileStem.c_str());
	
	const char** boneNames = reinterpret_cast<const char**>(qcFile.ReserveData(sizeof(char*) * parsedData->BoneCount()));
	for (int i = 0u; i < parsedData->BoneCount(); i++)
	{
		boneNames[i] = parsedData->pBone(i)->pszName();
	}

	const WeightListData_t weightListData(weightListName, boneNames, sequence->weights, parsedData->BoneCount(), false);

	CmdParse(&qcFile, QC_WEIGHTLIST, &weightListData, 1u, false, QC_FMT_ARRAYSTYLE);

	ModelSeqInfo_t seqInfo;
	seqInfo.seq = sequence;
	seqInfo.blends = reinterpret_cast<int* const>(qcFile.ReserveData(sizeof(int) * sequence->AnimCount()));

	ModelAnimInfo_t* const animInfo = reinterpret_cast<ModelAnimInfo_t* const>(qcFile.ReserveData(sizeof(ModelAnimInfo_t) * sequence->AnimCount()));

	for (int i = 0; i < sequence->AnimCount(); i++)
	{
		const ModelAnim_t* const anim = sequence->Anim(i);
		animInfo[i] = ModelAnimInfo_t(anim, nullptr, anim->pszName());

		QC_ParseStudioAnimation(&qcFile, parsedData, &animInfo[i], fileStem.c_str(), setting);
	}

	QC_ParseStudioSequence(&qcFile, parsedData, &seqInfo, animInfo, weightListName, fileStem.c_str(), setting);

	// get version from settings
	Version_t qcTargetVersion;
	qcTargetVersion.SetVersion(static_cast<MajorVersion_t>(g_rsxSettings.qcMajorVersion), g_rsxSettings.qcMinorVersion);

	// export qc file
	QCFile::SetExportVersion(qcTargetVersion);
	qcFile.ParseToText(g_rsxSettings.exportQCIFiles);

	g_BufferManager.RelieveBuffer(buf);

	return true;
}