#include <pch.h>

#include <core/mdl/modeldata.h>
#include <core/mdl/animdata.h>

extern CBufferManager g_BufferManager;
extern RSXSettings_t g_rsxSettings;


//
// PARSEDDATA
//

// movement
ModelFrameMovement_t::ModelFrameMovement_t(const ModelFrameMovement_t& movement) : baseptr(movement.baseptr), sectionframes(movement.sectionframes), sectioncount(movement.sectioncount)
{
	std::memcpy(this->scale, movement.scale, sizeof(float) * 4);

	if (sectioncount > 0)
	{
		sections = new int[sectioncount];

		std::memcpy(sections, movement.sections, sizeof(int) * sectioncount);

		return;
	}

	std::memcpy(this->offset, movement.offset, sizeof(short) * 4);
}

ModelFrameMovement_t::ModelFrameMovement_t(const r1::mstudioframemovement_t* const movement) : baseptr(movement), sectionframes(0), sectioncount(0)
{
	std::memcpy(this->scale, movement->scale, sizeof(float) * 4);
	std::memcpy(this->offset, movement->offset, sizeof(short) * 4);
}

ModelFrameMovement_t::ModelFrameMovement_t(const r5::mstudioframemovement_t* const movement, const int frameCount, const bool indexType) : baseptr(movement), sectionframes(movement->sectionframes), sectioncount(movement->SectionCount(frameCount))
{
	std::memcpy(this->scale, movement->scale, sizeof(float) * 4);

	sections = new int[sectioncount];

	for (int i = 0; i < sectioncount; i++)
	{
		const int index = indexType ? FIX_OFFSET(reinterpret_cast<uint16_t*>((char*)baseptr + sizeof(r5::mstudioframemovement_t))[i]) : reinterpret_cast<int*>((char*)baseptr + sizeof(r5::mstudioframemovement_t))[i];

		sections[i] = index;
	}
}

// animdesc
ModelAnim_t::ModelAnim_t(const ModelAnim_t& animdesc) : baseptr(animdesc.baseptr), name(animdesc.name), fps(animdesc.fps),
	flags(animdesc.flags), numframes(animdesc.numframes), animindex(animdesc.animindex),
	nummovements(animdesc.nummovements), movements(animdesc.movements), framemovement(nullptr), numikrules(animdesc.numikrules), ikrules(nullptr),
	sectionframes(animdesc.sectionframes), sectionstallframes(animdesc.sectionstallframes), numsections(animdesc.numsections), sections(nullptr),
	sectionDataExtra(animdesc.sectionDataExtra), animData(animdesc.animData), animDataAsset(animdesc.animDataAsset), parsedBufferIndex(animdesc.parsedBufferIndex)
{
	if (animdesc.sections)
	{
		assertm(numsections, "section count should be set");

		const size_t size = sizeof(ModelAnimSection_t) * numsections;
		sections = new ModelAnimSection_t[numsections];
		memcpy_s(sections, size, animdesc.sections, size);
	}

	if (animdesc.framemovement)
	{
		framemovement = new ModelFrameMovement_t(*animdesc.framemovement);
	}

	if (animdesc.ikrules)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc.ikrules + i);
		}
	}
}

ModelAnim_t::ModelAnim_t(const r2::mstudioanimdesc_t* const animdesc) : baseptr(reinterpret_cast<const void* const>(animdesc)),
	name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
	nummovements(animdesc->nummovements), movements(animdesc->pMovement(0)), framemovement(nullptr), numikrules(animdesc->numikrules), ikrules(nullptr),
	sectionframes(animdesc->sectionframes), sectionstallframes(0), numsections(0), sections(nullptr), sectionDataExtra(nullptr),
	animData(nullptr), animDataAsset(0ull), parsedBufferIndex(invalidNoodleIdx)
{
	flags |= eStudioAnimFlags::ANIM_VALID;

	if (sectionframes)
	{
		numsections = SectionCount();
		sections = new ModelAnimSection_t[numsections];

		for (int i = 0; i < SectionCount(); i++)
		{
			sections[i] = ModelAnimSection_t(animdesc->pSection(i)->animindex);
		}
	}

	if (animdesc->framemovementindex && (flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT))
	{
		framemovement = new ModelFrameMovement_t(animdesc->pFrameMovement());
	}

	if (animdesc->numikrules && animdesc->ikruleindex)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc->pIKRule(i));
		}
	}
};

ModelAnim_t::ModelAnim_t(const r5::mstudioanimdesc_v8_t* const animdesc) : baseptr(reinterpret_cast<const void* const>(animdesc)),
	name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
	nummovements(animdesc->nummovements), movements(animdesc->pMovement(0)), framemovement(nullptr), numikrules(animdesc->numikrules), ikrules(nullptr),
	sectionframes(animdesc->sectionframes), sectionstallframes(0), numsections(0), sections(nullptr), sectionDataExtra(nullptr),
	animData(nullptr), animDataAsset(0ull), parsedBufferIndex(invalidNoodleIdx)
{
	// [rika]: there's no point to parse these without this flag, and we can't determine what the count will be properly
	// [rika]: it will also never get hit by a panim function, so no need to worry there
	if (sectionframes && (flags & eStudioAnimFlags::ANIM_VALID))
	{
		numsections = SectionCount();
		sections = new ModelAnimSection_t[numsections];

		for (int i = 0; i < SectionCount(); i++)
		{
			sections[i] = ModelAnimSection_t(animdesc->pSection(i)->animindex);
		}
	}

	if (animdesc->framemovementindex && (flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT))
	{
		framemovement = new ModelFrameMovement_t(animdesc->pFrameMovement(), numframes, false);
	}

	if (animdesc->numikrules && animdesc->ikruleindex)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc->pIKRule(i));
		}
	}
};

ModelAnim_t::ModelAnim_t(const r5::mstudioanimdesc_v12_1_t* const animdesc, const char* const ext) : baseptr(reinterpret_cast<const void* const>(animdesc)),
	name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
	nummovements(animdesc->nummovements), movements(animdesc->pMovement(0)), framemovement(nullptr), numikrules(animdesc->numikrules), ikrules(nullptr),
	sectionframes(animdesc->sectionframes), sectionstallframes(animdesc->sectionstallframes), numsections(0), sections(nullptr),
	sectionDataExtra(ext), animData(nullptr), animDataAsset(0ull), parsedBufferIndex(invalidNoodleIdx)
{
	// [rika]: there's no point to parse these without this flag, and we can't determine what the count will be properly
	// [rika]: it will also never get hit by a panim function, so no need to worry there
	if (sectionframes && (flags & eStudioAnimFlags::ANIM_VALID))
	{
		numsections = SectionCount();
		sections = new ModelAnimSection_t[numsections];

		for (int i = 0; i < SectionCount(); i++)
		{
			sections[i] = ModelAnimSection_t(animdesc->pSection(i)->animindex, animdesc->pSection(i)->isExternal);
		}
	}

	if (animdesc->framemovementindex && (flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT))
	{
		framemovement = new ModelFrameMovement_t(animdesc->pFrameMovement(), numframes, false);
	}

	if (animdesc->numikrules && animdesc->ikruleindex)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc->pIKRule(i));
		}
	}
};

ModelAnim_t::ModelAnim_t(const r5::mstudioanimdesc_v16_t* const animdesc, const char* const ext) : baseptr(reinterpret_cast<const void* const>(animdesc)),
	name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
	nummovements(0), movements(nullptr), framemovement(nullptr), numikrules(animdesc->numikrules), ikrules(nullptr),
	sectionframes(animdesc->sectionframes), sectionstallframes(animdesc->sectionstallframes), numsections(0), sections(nullptr),
	sectionDataExtra(ext), animData(nullptr), animDataAsset(0ull), parsedBufferIndex(invalidNoodleIdx)
{
	// [rika]: there's no point to parse these without this flag, and we can't determine what the count will be properly
	// [rika]: it will also never get hit by a panim function, so no need to worry there
	if (sectionframes && (flags & eStudioAnimFlags::ANIM_VALID))
	{
		numsections = SectionCount();
		sections = new ModelAnimSection_t[numsections];

		for (int i = 0; i < SectionCount(); i++)
		{
			sections[i] = ModelAnimSection_t(animdesc->pSection(i)->Index(), animdesc->pSection(i)->isExternal());
		}
	}

	if (animdesc->framemovementindex && (flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT))
	{
		framemovement = new ModelFrameMovement_t(animdesc->pFrameMovement(), numframes, true);
	}

	if (animdesc->numikrules && animdesc->ikruleindex)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc->pIKRule(i));
		}
	}
};

ModelAnim_t::ModelAnim_t(const r5::mstudioanimdesc_v19_1_t* const animdesc, const char* const ext) : baseptr(reinterpret_cast<const void* const>(animdesc)),
	name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(0),
	nummovements(0), movements(nullptr), framemovement(nullptr), numikrules(animdesc->numikrules), ikrules(nullptr),
	sectionframes(animdesc->sectionframes), sectionstallframes(animdesc->sectionstallframes), numsections(0), sections(nullptr),
	sectionDataExtra(ext), animData(nullptr), animDataAsset(animdesc->animDataAsset), parsedBufferIndex(invalidNoodleIdx)
{
	// [rika]: there's no point to parse these without this flag, and we can't determine what the count will be properly
	// [rika]: it will also never get hit by a panim function, so no need to worry there
	if (sectionframes && (flags & eStudioAnimFlags::ANIM_VALID))
	{
		numsections = SectionCount(true);
		sections = new ModelAnimSection_t[numsections];

		for (int i = 0; i < SectionCount(true); i++)
		{
			sections[i] = ModelAnimSection_t(animdesc->pSection(i)->Index(), animdesc->pSection(i)->isExternal());
		}
	}

	if (animdesc->framemovementindex && (flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT))
	{
		framemovement = new ModelFrameMovement_t(animdesc->pFrameMovement(), numframes, true);
	}

	if (animdesc->numikrules && animdesc->ikruleindex)
	{
		ikrules = new ModelIKRule_t[numikrules];

		for (int i = 0; i < numikrules; i++)
		{
			ikrules[i] = ModelIKRule_t(animdesc->pIKRule(i));
		}
	}
};

const char* const ModelAnim_t::pAnimdataNoStall(int* const piFrame, int* const _UNUSED) const
{
	UNUSED(_UNUSED);

	int index = animindex;
	int section = 0;

	if (sectionframes != 0)
	{
		if (numframes > sectionframes && *piFrame == numframes - 1)
		{
			// last frame on long anims is stored separately
			*piFrame = 0;
			section = (numframes - 1) / sectionframes + 1;
		}
		else
		{
			section = *piFrame / sectionframes;
			*piFrame -= section * sectionframes;
		}

		index = pSection(section)->animindex;
		assertm(pSection(section)->isExternal == false, "anim can't have stall frames");
	}

	return ((char*)baseptr + index);
}

// [rika]: the inital revision of pAnimdataStall, does not support datapoint animations
const char* const ModelAnim_t::pAnimdataStall_0(int* const piFrame, int* const _UNUSED) const
{
	UNUSED(_UNUSED);

	int index = animindex;
	int section = 0;

	if (sectionframes != 0)
	{
		if (*piFrame >= sectionstallframes)
		{
			if (numframes > sectionframes && *piFrame == numframes - 1)
			{
				*piFrame = 0;
				section = (numframes - sectionstallframes - 1) / sectionframes + 2;
			}
			else
			{
				section = (*piFrame - sectionstallframes) / sectionframes + 1;
				*piFrame = *piFrame - sectionframes * ((*piFrame - sectionstallframes) / sectionframes) - sectionstallframes;
			}

			index = pSection(section)->animindex;
		}

		if (pSection(section)->isExternal) // checks if it is external
		{
			if (sectionDataExtra)
				return (sectionDataExtra + index);

			// we will stall if this is not loaded, for whatever reason
			index = pSection(0)->animindex;
			*piFrame = sectionstallframes - 1; // gets set to last frame of 'static'/'stall' section if the external data offset has not been cached(?)
		}
	}

	return ((char*)baseptr + index);
}

// [rika]: revision that appeared when datapoint animations were added, does not read the trailing section of rle animations,
//         and stores the section's frame length
const char* const ModelAnim_t::pAnimdataStall_1(int* const piFrame, int* const pSectionLength) const
{
	int index = animindex;
	int section = 0;
	int sectionlength = numframes;

	if (sectionframes)
	{
		if (*piFrame >= sectionstallframes)
		{
			const int sectionbase = (*piFrame - sectionstallframes) / sectionframes;
			section = sectionbase + 1;

			const int frameoffset = sectionstallframes + (sectionbase * sectionframes);
			*piFrame -= frameoffset;

			const int remainingframes = numframes - frameoffset;

			sectionlength = sectionframes;
			if (remainingframes <= sectionframes)
				sectionlength = remainingframes;
		}
		else
		{
			section = 0;
			sectionlength = sectionstallframes;
		}

		index = pSection(section)->animindex;

		*pSectionLength = sectionlength;
		if (pSection(section)->isExternal) // checks if it is external
		{
			if (sectionDataExtra)
				return (sectionDataExtra + index);

			// we will stall if this is not loaded, for whatever reason
			index = pSection(0)->animindex;

			// gets set to last frame of 'static'/'stall' section if the external data offset has not been cached(?)
			*piFrame = sectionstallframes - 1;
		}
	}

	*pSectionLength = sectionlength;
	return ((char*)baseptr + index);
}

// [rika]: built upon the previous revision, but animindex is missing along with the first section,
//         they're always assumed to be in the animdata asset
const char* const ModelAnim_t::pAnimdataStall_2(int* const piFrame, int* const pSectionLength) const
{
	int index = 0;
	int section = 0;
	int sectionlength = numframes;

	if (sectionframes)
	{
		if (*piFrame >= sectionstallframes)
		{
			const int sectionbase = (*piFrame - sectionstallframes) / sectionframes;
			section = sectionbase + 1;

			const int frameoffset = sectionstallframes + (sectionbase * sectionframes);
			*piFrame -= frameoffset;

			const int remainingframes = numframes - frameoffset;

			sectionlength = sectionframes;
			if (remainingframes <= sectionframes)
				sectionlength = remainingframes;
		}
		else
		{
			section = 0;
			sectionlength = sectionstallframes;
		}

		if (section > 0)
		{
			section--;

			index = pSection(section)->animindex;

			*pSectionLength = sectionlength;
			if (pSection(section)->isExternal) // checks if it is external
			{
				if (sectionDataExtra)
					return (sectionDataExtra + index);

				// we will stall if this is not loaded, for whatever reason
				index = 0;
				*piFrame = sectionstallframes - 1; // gets set to last frame of 'static'/'stall' section if the external data offset has not been cached(?)
			}
		}
	}

	*pSectionLength = sectionlength;
	return ((char*)animData + index);
}

// seqdesc
ModelSeq_t::ModelSeq_t(const r2::mstudioseqdesc_t* const seqdesc) : baseptr(reinterpret_cast<const void* const>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), actweight(seqdesc->actweight),
	events(nullptr), numevents(seqdesc->numevents), blends(seqdesc->pAnimIndex(0)), numblends(seqdesc->numblends), fadeintime(seqdesc->fadeintime), fadeouttime(seqdesc->fadeouttime), localentrynode(seqdesc->localentrynode), localexitnode(seqdesc->localexitnode), nodeflags(seqdesc->nodeflags),
	entryphase(seqdesc->entryphase), exitphase(seqdesc->exitphase), lastframe(seqdesc->lastframe), nextseq(seqdesc->nextseq), pose(seqdesc->pose), autolayers(nullptr), numautolayers(seqdesc->numautolayers), weights(seqdesc->pBoneweight(0)),
	posekeys(seqdesc->posekeyindex ? seqdesc->pPoseKey(0, 0) : nullptr), keyvalues(seqdesc->pKeyValues()), iklocks(nullptr), numiklocks(seqdesc->numiklocks), cycleposeindex(seqdesc->cycleposeindex), activitymodifiers(nullptr), numactivitymodifiers(seqdesc->numactivitymodifiers),
	ikResetMask(seqdesc->ikResetMask),
	anims(nullptr), parsedData(seqdesc->AnimCount())
{
	paramindex[0] = seqdesc->paramindex[0];
	paramindex[1] = seqdesc->paramindex[1];
	paramstart[0] = seqdesc->paramstart[0];
	paramstart[1] = seqdesc->paramstart[1];
	paramend[0] = seqdesc->paramend[0];
	paramend[1] = seqdesc->paramend[1];
	groupsize[0] = seqdesc->groupsize[0];
	groupsize[1] = seqdesc->groupsize[1];

	const r2::studiohdr_t* const pStudioHdr = seqdesc->pStudiohdr();

	if (numblends)
	{
		anims = new ModelAnim_t[numblends]{};

		for (int i = 0; i < numblends; i++)
		{
			const r2::mstudioanimdesc_t* const pAnimdesc = pStudioHdr->pAnimdesc(seqdesc->GetAnimIndex(i));

			// no need to sanity check here

			anims[i] = ModelAnim_t(pAnimdesc);
		}
	}
	
	if (numevents)
	{
		events = new ModelEvent_t[numevents]{};

		for (int i = 0; i < numevents; i++)
		{
			events[i] = ModelEvent_t(seqdesc->pEvent(i));
		}
	}
	
	if (numautolayers)
	{
		autolayers = new ModelAutoLayer_t[numautolayers]{};

		for (int i = 0; i < numautolayers; i++)
		{
			autolayers[i] = ModelAutoLayer_t(seqdesc->pAutoLayer(i));
		}
	}
	
	if (numiklocks)
	{
		iklocks = new ModelIKLock_t[numiklocks]{};

		for (int i = 0; i < numiklocks; i++)
		{
			iklocks[i] = ModelIKLock_t(seqdesc->pIKLock(i));
		}
	}
	
	if (numactivitymodifiers)
	{
		activitymodifiers = new ModelActivityModifier_t[numactivitymodifiers]{};

		for (int i = 0; i < numactivitymodifiers; i++)
		{
			activitymodifiers[i] = ModelActivityModifier_t(seqdesc->pActivityModifier(i));
		}
	}

}

// putting a note here, apex can have numblends set to 0 despite having anims
#define ANIMDESC_SANITY_CHECK(anim) (anim->fps < 0.0f || anim->fps > 2048.f || anim->numframes < 0 || anim->numframes > 0x20000)
ModelSeq_t::ModelSeq_t(const r5::mstudioseqdesc_v8_t* const seqdesc) : baseptr(reinterpret_cast<const void* const>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), actweight(seqdesc->actweight),
	events(nullptr), numevents(seqdesc->numevents), blends(defaultSequenceBlends), numblends(seqdesc->groupsize[0] * seqdesc->groupsize[1]), fadeintime(seqdesc->fadeintime), fadeouttime(seqdesc->fadeouttime), localentrynode(seqdesc->localentrynode), localexitnode(seqdesc->localexitnode), nodeflags(seqdesc->nodeflags),
	entryphase(seqdesc->entryphase), exitphase(seqdesc->exitphase), lastframe(seqdesc->lastframe), nextseq(seqdesc->nextseq), pose(seqdesc->pose), autolayers(nullptr), numautolayers(seqdesc->numautolayers), weights(seqdesc->pBoneweight(0)),
	posekeys(seqdesc->posekeyindex ? seqdesc->pPoseKey(0, 0) : nullptr), keyvalues(seqdesc->pKeyValues()), iklocks(nullptr), numiklocks(seqdesc->numiklocks), cycleposeindex(seqdesc->cycleposeindex), activitymodifiers(nullptr), numactivitymodifiers(seqdesc->numactivitymodifiers),
	ikResetMask(seqdesc->ikResetMask),
	anims(nullptr), parsedData(seqdesc->AnimCount())
{
	paramindex[0] = seqdesc->paramindex[0];
	paramindex[1] = seqdesc->paramindex[1];
	paramstart[0] = seqdesc->paramstart[0];
	paramstart[1] = seqdesc->paramstart[1];
	paramend[0] = seqdesc->paramend[0];
	paramend[1] = seqdesc->paramend[1];
	groupsize[0] = seqdesc->groupsize[0];
	groupsize[1] = seqdesc->groupsize[1];

	if (numblends)
	{
		assertm(maxSequenceBlends >= numblends, "higher than expected blend");
		anims = new ModelAnim_t[numblends]{};

		for (int i = 0; i < numblends; i++)
		{
			const r5::mstudioanimdesc_v8_t* const pAnimdesc = seqdesc->pAnimDesc_V8(i);

			// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
			// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
			if (ANIMDESC_SANITY_CHECK(pAnimdesc))
			{
				Log("ASEQ: %s had animation(s) (index %i), but no animdesc; skipping...\n", seqdesc->pszLabel(), i);
				numblends = 0;
				break;
			}

			anims[i] = ModelAnim_t(pAnimdesc);
		}
	}

	if (numevents)
	{
		events = new ModelEvent_t[numevents]{};

		for (int i = 0; i < numevents; i++)
		{
			events[i] = ModelEvent_t(seqdesc->pEvent_V8(i));
		}
	}

	if (numautolayers)
	{
		autolayers = new ModelAutoLayer_t[numautolayers]{};

		for (int i = 0; i < numautolayers; i++)
		{
			autolayers[i] = ModelAutoLayer_t(seqdesc->pAutoLayer(i));
		}
	}

	if (numiklocks)
	{
		iklocks = new ModelIKLock_t[numiklocks]{};

		for (int i = 0; i < numiklocks; i++)
		{
			iklocks[i] = ModelIKLock_t(seqdesc->pIKLock(i));
		}
	}

	if (numactivitymodifiers)
	{
		activitymodifiers = new ModelActivityModifier_t[numactivitymodifiers]{};

		for (int i = 0; i < numactivitymodifiers; i++)
		{
			activitymodifiers[i] = ModelActivityModifier_t(seqdesc->pActivityModifier(i));
		}
	}
}

ModelSeq_t::ModelSeq_t(const r5::mstudioseqdesc_v8_t* const seqdesc, const char* const ext) : baseptr(reinterpret_cast<const void* const>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), actweight(seqdesc->actweight),
	events(nullptr), numevents(seqdesc->numevents), blends(defaultSequenceBlends), numblends(seqdesc->groupsize[0] * seqdesc->groupsize[1]), fadeintime(seqdesc->fadeintime), fadeouttime(seqdesc->fadeouttime), localentrynode(seqdesc->localentrynode), localexitnode(seqdesc->localexitnode), nodeflags(seqdesc->nodeflags),
	entryphase(seqdesc->entryphase), exitphase(seqdesc->exitphase), lastframe(seqdesc->lastframe), nextseq(seqdesc->nextseq), pose(seqdesc->pose), autolayers(nullptr), numautolayers(seqdesc->numautolayers), weights(seqdesc->pBoneweight(0)),
	posekeys(seqdesc->posekeyindex ? seqdesc->pPoseKey(0, 0) : nullptr), keyvalues(seqdesc->pKeyValues()), iklocks(nullptr), numiklocks(seqdesc->numiklocks), cycleposeindex(seqdesc->cycleposeindex), activitymodifiers(nullptr), numactivitymodifiers(seqdesc->numactivitymodifiers),
	ikResetMask(seqdesc->ikResetMask),
	anims(nullptr), parsedData(seqdesc->AnimCount())
{
	paramindex[0] = seqdesc->paramindex[0];
	paramindex[1] = seqdesc->paramindex[1];
	paramstart[0] = seqdesc->paramstart[0];
	paramstart[1] = seqdesc->paramstart[1];
	paramend[0] = seqdesc->paramend[0];
	paramend[1] = seqdesc->paramend[1];
	groupsize[0] = seqdesc->groupsize[0];
	groupsize[1] = seqdesc->groupsize[1];

	if (numblends)
	{
		assertm(maxSequenceBlends >= numblends, "higher than expected blend");
		anims = new ModelAnim_t[numblends]{};

		for (int i = 0; i < numblends; i++)
		{
			const r5::mstudioanimdesc_v12_1_t* const pAnimdesc = seqdesc->pAnimDesc_V12_1(i);

			// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
			// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
			if (ANIMDESC_SANITY_CHECK(pAnimdesc))
			{
				Log("ASEQ: %s had animation(s) (index %i), but no animdesc; skipping...\n", seqdesc->pszLabel(), i);
				numblends = 0;
				break;
			}

			anims[i] = ModelAnim_t(pAnimdesc, ext);
		}
	}

	if (numevents)
	{
		events = new ModelEvent_t[numevents]{};

		const size_t sizeOfEvent = (seqdesc->autolayerindex - seqdesc->eventindex) / numevents;
		const bool version = sizeOfEvent == sizeof(r5::mstudioevent_v12_3_t);

		if (version)
		{
			for (int i = 0; i < numevents; i++)
			{
				events[i] = ModelEvent_t(seqdesc->pEvent_V12_3(i));
			}
		}
		else
		{
			for (int i = 0; i < numevents; i++)
			{
				events[i] = ModelEvent_t(seqdesc->pEvent_V8(i));
			}
		}
	}

	if (numautolayers)
	{
		autolayers = new ModelAutoLayer_t[numautolayers]{};

		for (int i = 0; i < numautolayers; i++)
		{
			autolayers[i] = ModelAutoLayer_t(seqdesc->pAutoLayer(i));
		}
	}

	if (numiklocks)
	{
		iklocks = new ModelIKLock_t[numiklocks]{};

		for (int i = 0; i < numiklocks; i++)
		{
			iklocks[i] = ModelIKLock_t(seqdesc->pIKLock(i));
		}
	}

	if (numactivitymodifiers)
	{
		activitymodifiers = new ModelActivityModifier_t[numactivitymodifiers]{};

		for (int i = 0; i < numactivitymodifiers; i++)
		{
			activitymodifiers[i] = ModelActivityModifier_t(seqdesc->pActivityModifier(i));
		}
	}
}

ModelSeq_t::ModelSeq_t(const r5::mstudioseqdesc_v16_t* const seqdesc, const char* const ext) : baseptr(reinterpret_cast<const void* const>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), actweight(seqdesc->actweight),
	events(nullptr), numevents(seqdesc->numevents), blends(defaultSequenceBlends), numblends(seqdesc->groupsize[0] * seqdesc->groupsize[1]), fadeintime(seqdesc->fadeintime), fadeouttime(seqdesc->fadeouttime), localentrynode(seqdesc->localentrynode), localexitnode(seqdesc->localexitnode), nodeflags(0),
	entryphase(0), exitphase(0), lastframe(0), nextseq(0), pose(0), autolayers(nullptr), numautolayers(seqdesc->numautolayers), weights(seqdesc->pBoneweight(0)),
	posekeys(seqdesc->posekeyindex ? seqdesc->pPoseKey(0, 0) : nullptr), keyvalues(nullptr), iklocks(nullptr), numiklocks(seqdesc->numiklocks), cycleposeindex(seqdesc->cycleposeindex), activitymodifiers(nullptr), numactivitymodifiers(seqdesc->numactivitymodifiers),
	ikResetMask(seqdesc->ikResetMask),
	anims(nullptr), parsedData(seqdesc->AnimCount())
{
	paramindex[0] = seqdesc->paramindex[0];
	paramindex[1] = seqdesc->paramindex[1];
	paramstart[0] = seqdesc->paramstart[0];
	paramstart[1] = seqdesc->paramstart[1];
	paramend[0] = seqdesc->paramend[0];
	paramend[1] = seqdesc->paramend[1];
	groupsize[0] = seqdesc->groupsize[0];
	groupsize[1] = seqdesc->groupsize[1];

	if (numblends)
	{
		assertm(maxSequenceBlends >= numblends, "higher than expected blend");
		anims = new ModelAnim_t[numblends]{};

		for (int i = 0; i < numblends; i++)
		{
			const r5::mstudioanimdesc_v16_t* const pAnimdesc = seqdesc->pAnimDesc(static_cast<uint16_t>(i));

			// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
			// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
			if (ANIMDESC_SANITY_CHECK(pAnimdesc))
			{
				Log("ASEQ: %s had animation(s) (index %i), but no animdesc; skipping...\n", seqdesc->pszLabel(), i);
				numblends = 0;
				break;
			}

			anims[i] = ModelAnim_t(pAnimdesc, ext);
		}
	}

	if (numevents)
	{
		events = new ModelEvent_t[numevents]{};

		for (int i = 0; i < numevents; i++)
		{
			events[i] = ModelEvent_t(seqdesc->pEvent(i));
		}
	}

	if (numautolayers)
	{
		autolayers = new ModelAutoLayer_t[numautolayers]{};

		for (int i = 0; i < numautolayers; i++)
		{
			autolayers[i] = ModelAutoLayer_t(seqdesc->pAutoLayer(i));
		}
	}

	if (numiklocks)
	{
		iklocks = new ModelIKLock_t[numiklocks]{};

		for (int i = 0; i < numiklocks; i++)
		{
			iklocks[i] = ModelIKLock_t(seqdesc->pIKLock(i));
		}
	}

	if (numactivitymodifiers)
	{
		activitymodifiers = new ModelActivityModifier_t[numactivitymodifiers]{};

		for (int i = 0; i < numactivitymodifiers; i++)
		{
			activitymodifiers[i] = ModelActivityModifier_t(seqdesc->pActivityModifier(i));
		}
	}
}

ModelSeq_t::ModelSeq_t(const r5::mstudioseqdesc_v18_t* const seqdesc, const char* const ext, const uint32_t version) : baseptr(reinterpret_cast<const void* const>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), actweight(seqdesc->actweight),
	events(nullptr), numevents(seqdesc->numevents), blends(defaultSequenceBlends), numblends(seqdesc->groupsize[0] * seqdesc->groupsize[1]), fadeintime(seqdesc->fadeintime), fadeouttime(seqdesc->fadeouttime), localentrynode(seqdesc->localentrynode), localexitnode(seqdesc->localexitnode), nodeflags(0),
	entryphase(0), exitphase(0), lastframe(0), nextseq(0), pose(0), autolayers(nullptr), numautolayers(seqdesc->numautolayers), weights(seqdesc->pBoneweight(0)),
	posekeys(seqdesc->posekeyindex ? seqdesc->pPoseKey(0, 0) : nullptr), keyvalues(nullptr), iklocks(nullptr), numiklocks(seqdesc->numiklocks), cycleposeindex(seqdesc->cycleposeindex), activitymodifiers(nullptr), numactivitymodifiers(seqdesc->numactivitymodifiers),
	ikResetMask(seqdesc->ikResetMask),
	anims(nullptr), parsedData(seqdesc->AnimCount())
{
	paramindex[0] = seqdesc->paramindex[0];
	paramindex[1] = seqdesc->paramindex[1];
	paramstart[0] = seqdesc->paramstart[0];
	paramstart[1] = seqdesc->paramstart[1];
	paramend[0] = seqdesc->paramend[0];
	paramend[1] = seqdesc->paramend[1];
	groupsize[0] = seqdesc->groupsize[0];
	groupsize[1] = seqdesc->groupsize[1];

	if (numblends)
	{
		assertm(maxSequenceBlends >= numblends, "higher than expected blend");
		anims = new ModelAnim_t[numblends]{};

		switch (version)
		{
		case 0u:
		{
			for (int i = 0; i < numblends; i++)
			{
				const r5::mstudioanimdesc_v16_t* const pAnimdesc = seqdesc->pAnimDesc(static_cast<uint16_t>(i));

				// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
				// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
				if (ANIMDESC_SANITY_CHECK(pAnimdesc))
				{
					Log("ASEQ: %s had animation(s) (index %i), but no animdesc; skipping...\n", seqdesc->pszLabel(), i);
					numblends = 0;
					break;
				}

				anims[i] = ModelAnim_t(pAnimdesc, ext);
			}

			break;
		}
		case 1u:
		{
			for (int i = 0; i < numblends; i++)
			{
				const r5::mstudioanimdesc_v19_1_t* const pAnimdesc = seqdesc->pAnimDesc_V19_1(static_cast<uint16_t>(i));

				// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
				// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
				if (ANIMDESC_SANITY_CHECK(pAnimdesc))
				{
					Log("ASEQ: %s had animation(s) (index %i), but no animdesc; skipping...\n", seqdesc->pszLabel(), i);
					numblends = 0;
					break;
				}

				anims[i] = ModelAnim_t(pAnimdesc, ext);
			}

			break;
		}
		default:
		{
			assertm(false, "invalid input");
			break;
		}
		}
	}

	if (numevents)
	{
		events = new ModelEvent_t[numevents]{};

		for (int i = 0; i < numevents; i++)
		{
			events[i] = ModelEvent_t(seqdesc->pEvent(i));
		}
	}

	if (numautolayers)
	{
		autolayers = new ModelAutoLayer_t[numautolayers]{};

		for (int i = 0; i < numautolayers; i++)
		{
			autolayers[i] = ModelAutoLayer_t(seqdesc->pAutoLayer(i));
		}
	}

	if (numiklocks)
	{
		iklocks = new ModelIKLock_t[numiklocks]{};

		for (int i = 0; i < numiklocks; i++)
		{
			iklocks[i] = ModelIKLock_t(seqdesc->pIKLock(i));
		}
	}

	if (numactivitymodifiers)
	{
		activitymodifiers = new ModelActivityModifier_t[numactivitymodifiers]{};

		for (int i = 0; i < numactivitymodifiers; i++)
		{
			activitymodifiers[i] = ModelActivityModifier_t(seqdesc->pActivityModifier(i));
		}
	}
}
#undef ANIMDESC_SANITY_CHECK // remove if needed in other files

ModelSeq_t::~ModelSeq_t()
{
	FreeAllocArray(events);
	FreeAllocArray(autolayers);
	FreeAllocArray(iklocks);
	FreeAllocArray(activitymodifiers);
	FreeAllocArray(anims);
}

const ModelIKLock_t* const ModelSeq_t::pIKLock(const int i) const
{
	return iklocks + i;
}

void ParseAnimDesc_Origin(ModelAnim_t* const animdesc, CAnimData& animData, bool(*Studio_AnimPosition)(const ModelAnim_t* const, float, Vector&, QAngle&))
{
	// [rika]: adjust the origin bone
	// [rika]: do after we get anim data, so our rotation does not get overwritten
	CAnimDataBone& originBone = animData.GetBone(0);
	originBone.SetFlags(CAnimDataBone::ANIMDATA_POS | CAnimDataBone::ANIMDATA_ROT); // make sure it has position and rotation

	for (int frame = 0; frame < animdesc->numframes; frame++)
	{
		Vector pos(originBone.GetPosPtr()[frame]);
		Quaternion q(originBone.GetRotPtr()[frame]);
		const Vector& scale = originBone.GetSclPtr()[frame];

		QAngle vecAngleBase(q);

		const float cycle = animdesc->GetCycle(frame);

		if ((nullptr != animdesc->framemovement && animdesc->flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT) || (animdesc->nummovements && animdesc->movements))
		{
			Vector vecPos;
			QAngle vecAngle;

			Studio_AnimPosition(animdesc, cycle, vecPos, vecAngle);

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

		originBone.SetFrame(frame, pos, q, scale);
	}
}

void ParseAnimation(ModelSeq_t* const seqdesc, ModelAnim_t* const animdesc, const std::vector<ModelBone_t>* const bones, const r2::studiohdr_t* const pStudioHdr)
{
	const int boneCount = static_cast<int>(bones->size());

	Vector positions[256]{};
	Quaternion quats[256]{};
	Vector scales[256]{};

	if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA)
	{
		for (int i = 0; i < boneCount; i++)
		{
			positions[i].Init(0.0f, 0.0f, 0.0f);
			quats[i].Init(0.0f, 0.0f, 0.0f, 1.0f);
			scales[i].Init(1.0f, 1.0f, 1.0f);
		}
	}
	else
	{
		for (int i = 0; i < boneCount; i++)
		{
			const ModelBone_t* const bone = &bones->at(i);

			positions[i] = bone->pos;
			quats[i] = bone->quat;
			scales[i] = bone->scale;
		}
	}

	// no point to allocate memory on empty animations!
	CAnimData animData(boneCount, animdesc->numframes);
	animData.ReserveVector();

	// [rika]: parse through the bone tracks here
	for (int frame = 0; frame < animdesc->numframes; frame++)
	{
		const float cycle = animdesc->GetCycle(frame);

		float fFrame = cycle * static_cast<float>(animdesc->numframes - 1);

		const int iFrame = static_cast<int>(fFrame);
		const float s = (fFrame - static_cast<float>(iFrame));

		int iLocalFrame = iFrame;
		const r2::mstudio_rle_anim_t* const panimstart = reinterpret_cast<const r2::mstudio_rle_anim_t*>(animdesc->pAnimdataNoStall(&iLocalFrame, nullptr));

		for (int bone = 0; bone < boneCount; bone++)
		{
			Vector pos;
			Quaternion q;
			Vector scale;

			const r2::mstudio_rle_anim_t* panim = panimstart;

			// [rika]: titanfall 2 cycles through all the bone headers to get right one for a given bone
			// [rika]: handle like the game since sometimes there can be blocks of bone headers with '0xff' bone id (if my memory serves me)
			// [rika]: slightly slower this way which is likely why it's changed in apex
			while (panim->bone < bone)
			{
				panim = panim->pNext();

				if (!panim)
					break;
			}

			uint8_t boneFlags = 0u;

			if (panim && panim->bone == bone)
			{
				// r2 animations will always have position
				boneFlags |= CAnimDataBone::ANIMDATA_POS | (panim->flags & r2::RleFlags_t::STUDIO_ANIM_NOROT ? 0 : CAnimDataBone::ANIMDATA_ROT) | (seqdesc->flags & eStudioAnimFlags::ANIM_SCALE ? CAnimDataBone::ANIMDATA_SCL : 0);

				// need to add a bool here, we do not want the interpolated values (inbetween frames)
				r2::CalcBonePosition(iLocalFrame, s, pStudioHdr->pBone(panim->bone), pStudioHdr->pLinearBones(), panim, pos);
				r2::CalcBoneQuaternion(iLocalFrame, s, pStudioHdr->pBone(panim->bone), pStudioHdr->pLinearBones(), panim, q);
				r2::CalcBoneScale(iLocalFrame, s, pStudioHdr->pBone(panim->bone)->scale, pStudioHdr->pBone(panim->bone)->scalescale, panim, scale);
			}
			else
			{
				pos = positions[bone];
				q = quats[bone];
				scale = scales[bone];
			}

			CAnimDataBone& animDataBone = animData.GetBone(bone);
			animDataBone.SetFlags(boneFlags);
			animDataBone.SetFrame(frame, pos, q, scale);
		}
	}

	ParseAnimDesc_Origin(animdesc, animData, &r1::Studio_AnimPosition);

	// parse into memory and compress
	CManagedBuffer* buffer = g_BufferManager.ClaimBuffer();

	const size_t sizeInMem = animData.ToMemory(buffer->Buffer());
	animdesc->parsedBufferIndex = seqdesc->parsedData.addBack(buffer->Buffer(), sizeInMem);

	g_BufferManager.RelieveBuffer(buffer);
}

void ParseSequence(ModelSeq_t* const seqdesc, const std::vector<ModelBone_t>* const bones, const r2::studiohdr_t* const pStudioHdr)
{
	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		ModelAnim_t* const animdesc = seqdesc->anims + i;

		ParseAnimation(seqdesc, animdesc, bones, pStudioHdr);
	}
}

void ParseAnimation(ModelSeq_t* const seqdesc, ModelAnim_t* const animdesc, const std::vector<ModelBone_t>* const bones, const AnimdataFuncType_t funcType, const uint32_t flagWidth)
{
	const int boneCount = static_cast<int>(bones->size());

	if (bones->size() > 1024)
		Log("ParseAnimation: Animation %s exceeded 1024 bones.\n", animdesc->pszName());

	std::vector<Vector> positions(boneCount);
	std::vector<Quaternion> quats(boneCount);
	std::vector<Vector> scales(boneCount);
	std::vector<RadianEuler> rotations(boneCount);

	if (animdesc->flags & eStudioAnimFlags::ANIM_DELTA)
	{
		for (int i = 0; i < boneCount; i++)
		{
			positions[i].Init(0.0f, 0.0f, 0.0f);
			quats[i].Init(0.0f, 0.0f, 0.0f, 1.0f);
			scales[i].Init(1.0f, 1.0f, 1.0f);
			rotations[i].Init(0.0f, 0.0f, 0.0f);
		}
	}
	else
	{
		if (animdesc->flags & eStudioAnimFlags::ANIM_DATAPOINT)
		{
			for (int i = 0; i < boneCount; i++)
			{
				const ModelBone_t* const bone = &bones->at(i);

				positions[i].Init(0.0f, 0.0f, 0.0f);
				quats[i].Init(0.0f, 0.0f, 0.0f, 1.0f);
				scales[i].Init(1.0f, 1.0f, 1.0f);
				rotations[i] = bone->rot;
			}
		}
		else
		{
			for (int i = 0; i < boneCount; i++)
			{
				const ModelBone_t* const bone = &bones->at(i);

				positions[i] = bone->pos;
				quats[i] = bone->quat;
				scales[i] = bone->scale;
				rotations[i] = bone->rot;
			}
		}
	}

	CAnimData animData(boneCount, animdesc->numframes);
	animData.ReserveVector();

	// [rika]: parse through the bone tracks here
	if (animdesc->flags & eStudioAnimFlags::ANIM_VALID && animdesc->flags & eStudioAnimFlags::ANIM_DATAPOINT)
	{
		for (int frame = 0; frame < animdesc->numframes; frame++)
		{
			const float cycle = animdesc->GetCycle(frame);

			float fFrame = cycle * static_cast<float>(animdesc->numframes - 1);

			const int iFrame = static_cast<int>(fFrame);
			const float s = (fFrame - static_cast<float>(iFrame));

			int iLocalFrame = iFrame;

			int sectionlength = 0;
			const uint8_t* const boneFlagArray = reinterpret_cast<const uint8_t* const>((animdesc->*s_AnimdataFuncs_DP[funcType])(&iLocalFrame, &sectionlength));
			const r5::mstudio_rle_anim_t* panim = reinterpret_cast<const r5::mstudio_rle_anim_t*>(&boneFlagArray[ANIM_BONEFLAG_SIZE(boneCount, flagWidth)]);

			for (int bone = 0; bone < boneCount; bone++)
			{
				Vector pos(positions[bone]);
				Quaternion q(quats[bone]);
				Vector scale(scales[bone]);

				uint8_t boneFlags = ANIM_BONEFLAGS_FLAG(boneFlagArray, bone, flagWidth); // truncate byte offset then shift if needed
				const uint8_t* panimtrack = reinterpret_cast<const uint8_t*>(panim + 1);
				const float fLocalFrame = static_cast<float>(iLocalFrame) + s;

				if (boneFlags & (r5::RleBoneFlags_t::STUDIO_ANIM_DATA)) // check if this bone has data
				{
					if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_ROT)
						r5::CalcBoneQuaternion_DP(sectionlength, &panimtrack, fLocalFrame, q);

					if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_POS)
					{
						if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_UNK8)
							r5::CalcBonePositionVirtual_DP(sectionlength, &panimtrack, fLocalFrame, pos);
						else
							r5::CalcBonePosition_DP(sectionlength, &panimtrack, fLocalFrame, pos);
					}

					if (boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_SCALE)
						r5::CalcBoneScale_DP(sectionlength, &panimtrack, fLocalFrame, scale);

					panim = panim->pNext();
				}

				// [rika]: non delta animations are stored like a delta? weird.
				if (!(animdesc->flags & eStudioAnimFlags::ANIM_DELTA))
				{
					const ModelBone_t* const boneData = &bones->at(bone);

					// [rika]: adjust the rotation
					QuaternionMult(boneData->quat, q, q);

					// [rika]: reorient the position with the bone's rotation
					Quaternion posQ(pos.x, pos.y, pos.z, 0.0f);
					QuaternionMult(boneData->quat, posQ, posQ);

					// [rika]: get the bones inverted quaternion rotation
					Quaternion qBoneInverse;
					QuaternionConjugate(boneData->quat, qBoneInverse);

					// [rika]: reorient by inverted quaternion rotation
					QuaternionMult(posQ, qBoneInverse, posQ);

					pos.x = posQ.x;
					pos.y = posQ.y;
					pos.z = posQ.z;

					// [rika]: add the bones positon on top
					pos += boneData->pos;
				}

				assertm(pos.IsValid(), "invalid position");
				assertm(q.IsValid(), "invalid quaternion");

				CAnimDataBone& animDataBone = animData.GetBone(bone);
				animDataBone.SetFlags(boneFlags);
				animDataBone.SetFrame(frame, pos, q, scale);
			}
		}
	}
	else if (animdesc->flags & eStudioAnimFlags::ANIM_VALID)
	{
		for (int frame = 0; frame < animdesc->numframes; frame++)
		{
			const float cycle = animdesc->GetCycle(frame);

			float fFrame = cycle * static_cast<float>(animdesc->numframes - 1);

			const int iFrame = static_cast<int>(fFrame);
			const float s = (fFrame - static_cast<float>(iFrame));

			int iLocalFrame = iFrame;

			int sectionlength = 0;
			const uint8_t* const boneFlagArray = reinterpret_cast<const uint8_t* const>((animdesc->*s_AnimdataFuncs_RLE[funcType])(&iLocalFrame, &sectionlength));
			const r5::mstudio_rle_anim_t* panim = reinterpret_cast<const r5::mstudio_rle_anim_t*>(&boneFlagArray[ANIM_BONEFLAG_SIZE(boneCount, flagWidth)]);
			UNUSED(sectionlength);

			for (int bone = 0; bone < boneCount; bone++)
			{
				Vector pos(positions[bone]);
				Quaternion q(quats[bone]);
				Vector scale(scales[bone]);
				RadianEuler baseRot(rotations[bone]);

				uint8_t boneFlags = ANIM_BONEFLAGS_FLAG(boneFlagArray, bone, flagWidth); // truncate byte offset then shift if needed

				assertm((boneFlags & (r5::RleBoneFlags_t::STUDIO_ANIM_UNK10 | r5::RleBoneFlags_t::STUDIO_ANIM_UNK20)) == 0, "had new flags");

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

				CAnimDataBone& animDataBone = animData.GetBone(bone);
				animDataBone.SetFlags(boneFlags);
				animDataBone.SetFrame(frame, pos, q, scale);
			}
		}
	}
	// [rika]: or don't, if we don't have any
	else
	{
		// [rika]: does this sequence use scale?
		const uint8_t boneFlags = 0x0/*(seqdesc->flags & eStudioAnimFlags::ANIM_SCALE) ? CAnimDataBone::ANIMDATA_DATA : (CAnimDataBone::ANIMDATA_POS | CAnimDataBone::ANIMDATA_ROT)*/;

		for (int bone = 0; bone < boneCount; bone++)
		{
			CAnimDataBone& animDataBone = animData.GetBone(bone);

			for (int frame = 0; frame < animdesc->numframes; frame++)
			{
				animDataBone.SetFlags(boneFlags);
				animDataBone.SetFrame(frame, positions[bone], quats[bone], scales[bone]);
			}
		}
	}

	ParseAnimDesc_Origin(animdesc, animData, &r5::Studio_AnimPosition);

	// parse into memory and compress
	CManagedBuffer* buffer = g_BufferManager.ClaimBuffer();

	const size_t sizeInMem = animData.ToMemory(buffer->Buffer());
	animdesc->parsedBufferIndex = seqdesc->parsedData.addBack(buffer->Buffer(), sizeInMem);

	g_BufferManager.RelieveBuffer(buffer);
}

void ParseSequence(ModelSeq_t* const seqdesc, const std::vector<ModelBone_t>* const bones, const AnimdataFuncType_t funcType, const uint32_t flagWidth)
{
	// check flags
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_POS) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_POS), "flag mismatch");
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_ROT) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_ROT), "flag mismatch");
	assertm(static_cast<uint8_t>(CAnimDataBone::ANIMDATA_SCL) == static_cast<uint8_t>(r5::RleBoneFlags_t::STUDIO_ANIM_SCALE), "flag mismatch");

	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		ModelAnim_t* const animdesc = seqdesc->anims + i;

		// [rika]: data for this anim is not loaded, skip it
		if (animdesc->animDataAsset && !animdesc->animData)
		{
			Log("sequence %s required asset %llx but it was not loaded, skipping...\n", seqdesc->szlabel, animdesc->animDataAsset);
			continue;
		}

		ParseAnimation(seqdesc, animdesc, bones, funcType, flagWidth);
	}
}

// [rika]: this is for model internal sequence data (r5)
void ParseModelSequenceData_NoStall(ModelParsedData_t* const parsedData, char* const baseptr)
{
	assertm(parsedData->bones.size() > 0, "should have bones");

	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->localSequenceCount == 0)
		return;

	parsedData->numLocalSequences = pStudioHdr->localSequenceCount;
	parsedData->localSequences = new ModelSeq_t[pStudioHdr->localSequenceCount];

	for (int i = 0; i < pStudioHdr->localSequenceCount; i++)
	{
		parsedData->localSequences[i] = ModelSeq_t(reinterpret_cast<r5::mstudioseqdesc_v8_t* const>(baseptr + pStudioHdr->localSequenceOffset) + i);

		ParseSequence(&parsedData->localSequences[i], &parsedData->bones, AnimdataFuncType_t::ANIM_FUNC_NOSTALL);
	}
}

void ParseModelSequenceData_Stall_V8(ModelParsedData_t* const parsedData, char* const baseptr)
{
	assertm(parsedData->bones.size() > 0, "should have bones");

	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->localSequenceCount == 0)
		return;

	parsedData->numLocalSequences = pStudioHdr->localSequenceCount;
	parsedData->localSequences = new ModelSeq_t[pStudioHdr->localSequenceCount];

	for (int i = 0; i < pStudioHdr->localSequenceCount; i++)
	{
		parsedData->localSequences[i] = ModelSeq_t(reinterpret_cast<r5::mstudioseqdesc_v8_t* const>(baseptr + pStudioHdr->localSequenceOffset) + i, nullptr);

		ParseSequence(&parsedData->localSequences[i], &parsedData->bones, AnimdataFuncType_t::ANIM_FUNC_STALL_BASEPTR);
	}
}

void ParseModelSequenceData_Stall_V16(ModelParsedData_t* const parsedData, char* const baseptr)
{
	assertm(parsedData->bones.size() > 0, "should have bones");

	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->localSequenceCount == 0)
		return;

	parsedData->numLocalSequences = pStudioHdr->localSequenceCount;
	parsedData->localSequences = new ModelSeq_t[pStudioHdr->localSequenceCount];

	for (int i = 0; i < pStudioHdr->localSequenceCount; i++)
	{
		parsedData->localSequences[i] = ModelSeq_t(reinterpret_cast<r5::mstudioseqdesc_v16_t* const>(baseptr + pStudioHdr->localSequenceOffset) + i, nullptr);

		ParseSequence(&parsedData->localSequences[i], &parsedData->bones, AnimdataFuncType_t::ANIM_FUNC_STALL_BASEPTR);
	}
}

void ParseModelSequenceData_Stall_V18(ModelParsedData_t* const parsedData, char* const baseptr)
{
	assertm(parsedData->bones.size() > 0, "should have bones");

	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->localSequenceCount == 0)
		return;

	parsedData->numLocalSequences = pStudioHdr->localSequenceCount;
	parsedData->localSequences = new ModelSeq_t[pStudioHdr->localSequenceCount];

	for (int i = 0; i < pStudioHdr->localSequenceCount; i++)
	{
		parsedData->localSequences[i] = ModelSeq_t(reinterpret_cast<r5::mstudioseqdesc_v18_t* const>(baseptr + pStudioHdr->localSequenceOffset) + i, nullptr, 0u);

		ParseSequence(&parsedData->localSequences[i], &parsedData->bones, AnimdataFuncType_t::ANIM_FUNC_STALL_BASEPTR);
	}
}

extern void ParseAnimSeqDataForSeq(ModelSeq_t* const seqdesc, const size_t boneCount, const uint32_t flagWidth);
void ParseModelSequenceData_Stall_V19_1(ModelParsedData_t* const parsedData, char* const baseptr, const uint32_t flagWidth)
{
	assertm(parsedData->bones.size() > 0, "should have bones");

	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->localSequenceCount == 0)
		return;

	parsedData->numLocalSequences = pStudioHdr->localSequenceCount;
	parsedData->localSequences = new ModelSeq_t[pStudioHdr->localSequenceCount];

	for (int i = 0; i < pStudioHdr->localSequenceCount; i++)
	{
		parsedData->localSequences[i] = ModelSeq_t(reinterpret_cast<r5::mstudioseqdesc_v18_t* const>(baseptr + pStudioHdr->localSequenceOffset) + i, nullptr, 1u);

		ParseAnimSeqDataForSeq(parsedData->localSequences + i, parsedData->bones.size(), flagWidth);

		ParseSequence(&parsedData->localSequences[i], &parsedData->bones, AnimdataFuncType_t::ANIM_FUNC_STALL_ANIMDATA, flagWidth);
	}
}

void ParseModelAnimTypes_V8(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->ikChainCount > 0)
	{
		parsedData->ikchains = new ModelIKChain_t[pStudioHdr->ikChainCount];
		const r5::mstudioikchain_v8_t* const ikchains = reinterpret_cast<const r5::mstudioikchain_v8_t* const>(pStudioHdr->baseptr + pStudioHdr->ikChainOffset);

		for (int i = 0; i < pStudioHdr->ikChainCount; i++)
		{
			const ModelIKChain_t ikchain(ikchains + i);
			memcpy_s(parsedData->ikchains + i, sizeof(ModelIKChain_t), &ikchain, sizeof(ModelIKChain_t));
		}
	}

	if (pStudioHdr->localPoseParamCount > 0)
	{
		parsedData->poseparams = new ModelPoseParam_t[pStudioHdr->localPoseParamCount];
		const mstudioposeparamdesc_t* const poseparams = reinterpret_cast<const mstudioposeparamdesc_t* const>(pStudioHdr->baseptr + pStudioHdr->localPoseParamOffset);

		for (int i = 0; i < pStudioHdr->localPoseParamCount; i++)
		{
			const ModelPoseParam_t poseparam(poseparams + i);
			memcpy_s(parsedData->poseparams + i, sizeof(ModelPoseParam_t), &poseparam, sizeof(ModelPoseParam_t));
		}
	}

	if (pStudioHdr->localNodeCount > 0)
	{
		parsedData->numLocalNodes = pStudioHdr->localNodeCount;
		parsedData->localNodeNames = new const char*[pStudioHdr->localNodeCount]{};
		const int* const nodeNameIndices = reinterpret_cast<const int* const>(pStudioHdr->baseptr + pStudioHdr->localNodeNameOffset);

		for (int i = 0; i < pStudioHdr->localNodeCount; i++)
		{
			parsedData->localNodeNames[i] = pStudioHdr->baseptr + (pStudioHdr->localNodeNameOffset * pStudioHdr->localNodeNameType) + nodeNameIndices[i];
		}
	}

	// technically supported but never used
	if (pStudioHdr->localIkAutoPlayLockCount > 0)
	{
		//printf("wooowowww~~!! iklocks in: %s\n", pStudioHdr->pszName());

		parsedData->iklocks = new ModelIKLock_t[pStudioHdr->localIkAutoPlayLockCount];
		const r5::mstudioiklock_v8_t* const iklocks = reinterpret_cast<const r5::mstudioiklock_v8_t* const>(pStudioHdr->baseptr + pStudioHdr->localIkAutoPlayLockOffset);

		for (int i = 0; i < pStudioHdr->localIkAutoPlayLockCount; i++)
		{
			const ModelIKLock_t iklock(iklocks + i);
			memcpy_s(parsedData->iklocks + i, sizeof(ModelIKLock_t), &iklock, sizeof(ModelIKLock_t));
		}
	}
}

void ParseModelAnimTypes_V16(ModelParsedData_t* const parsedData)
{
	const studiohdr_generic_t* const pStudioHdr = parsedData->pStudioHdr();

	if (pStudioHdr->ikChainCount > 0)
	{
		parsedData->ikchains = new ModelIKChain_t[pStudioHdr->ikChainCount];
		const r5::mstudioikchain_v16_t* const ikchains = reinterpret_cast<const r5::mstudioikchain_v16_t* const>(pStudioHdr->baseptr + pStudioHdr->ikChainOffset);

		for (int i = 0; i < pStudioHdr->ikChainCount; i++)
		{
			const ModelIKChain_t ikchain(ikchains + i);
			memcpy_s(parsedData->ikchains + i, sizeof(ModelIKChain_t), &ikchain, sizeof(ModelIKChain_t));
		}
	}

	if (pStudioHdr->localPoseParamCount > 0)
	{
		parsedData->poseparams = new ModelPoseParam_t[pStudioHdr->localPoseParamCount];
		const r5::mstudioposeparamdesc_v16_t* const poseparams = reinterpret_cast<const r5::mstudioposeparamdesc_v16_t* const>(pStudioHdr->baseptr + pStudioHdr->localPoseParamOffset);

		for (int i = 0; i < pStudioHdr->localPoseParamCount; i++)
		{
			const ModelPoseParam_t poseparam(poseparams + i);
			memcpy_s(parsedData->poseparams + i, sizeof(ModelPoseParam_t), &poseparam, sizeof(ModelPoseParam_t));
		}
	}

	if (pStudioHdr->localNodeCount > 0)
	{
		parsedData->numLocalNodes = pStudioHdr->localNodeCount;
		parsedData->localNodeNames = new const char* [pStudioHdr->localNodeCount] {};
		const uint16_t* const nodeNameIndices = reinterpret_cast<const uint16_t* const>(pStudioHdr->baseptr + pStudioHdr->localNodeNameOffset);

		for (int i = 0; i < pStudioHdr->localNodeCount; i++)
		{
			parsedData->localNodeNames[i] = pStudioHdr->baseptr + (pStudioHdr->localNodeNameOffset * pStudioHdr->localNodeNameType) + FIX_OFFSET(nodeNameIndices[i]);
		}
	}

	// no global iklocks in v16 and later	
}

// per version funcs that utilize generic data
namespace r1
{
	void Studio_FrameMovement(const ModelFrameMovement_t* pFrameMovement, int frame, Vector& vecPos, float& yaw)
	{
		for (int i = 0; i < 3; i++)
		{
			ExtractAnimValue(frame, pFrameMovement->pAnimvalue(i), pFrameMovement->scale[i], vecPos[i]);
		}

		ExtractAnimValue(frame, pFrameMovement->pAnimvalue(3), pFrameMovement->scale[3], yaw);
	}

	void Studio_FrameMovement(const ModelFrameMovement_t* pFrameMovement, int iFrame, Vector& v1Pos, Vector& v2Pos, float& v1Yaw, float& v2Yaw)
	{
		for (int i = 0; i < 3; i++)
		{
			ExtractAnimValue(iFrame, pFrameMovement->pAnimvalue(i), pFrameMovement->scale[i], v1Pos[i], v2Pos[i]);
		}

		ExtractAnimValue(iFrame, pFrameMovement->pAnimvalue(3), pFrameMovement->scale[3], v1Yaw, v2Yaw);
	}

	bool Studio_AnimPosition(const ModelAnim_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle)
	{
		float	prevframe = 0;
		vecPos.Init();
		vecAngle.Init();

		int iLoops = 0;
		if (flCycle > 1.0)
		{
			iLoops = (int)flCycle;
		}
		else if (flCycle < 0.0)
		{
			iLoops = (int)flCycle - 1;
		}
		flCycle = flCycle - iLoops;

		float	flFrame = flCycle * (panim->numframes - 1);

		if (panim->flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
		{
			int iFrame = (int)flFrame;
			float s = (flFrame - iFrame);

			if (s == 0)
			{
				Studio_FrameMovement(panim->framemovement, iFrame, vecPos, vecAngle.y);
				return true;
			}
			else
			{
				Vector v1Pos, v2Pos;
				float v1Yaw, v2Yaw;

				Studio_FrameMovement(panim->framemovement, iFrame, v1Pos, v2Pos, v1Yaw, v2Yaw);

				vecPos = ((v2Pos - v1Pos) * s) + v1Pos; // this should work on paper?

				float yawDelta = AngleDiff(v2Yaw, v1Yaw);
				vecAngle.y = (yawDelta * s) + v1Yaw;

				return true;
			}
		}
		else
		{
			for (int i = 0; i < panim->nummovements; i++)
			{
				const mstudiomovement_t* const pmove = panim->pMovement(i);

				if (pmove->endframe >= flFrame)
				{
					float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

					float d = pmove->v0 * f + 0.5f * (pmove->v1 - pmove->v0) * f * f;

					vecPos = vecPos + (pmove->vector * d);
					vecAngle.y = vecAngle.y * (1 - f) + pmove->angle * f;
					if (iLoops != 0)
					{
						const mstudiomovement_t* const pmove_1 = panim->pMovement(panim->nummovements - 1);
						vecPos = vecPos + (pmove_1->position * iLoops);
						vecAngle.y = vecAngle.y + iLoops * pmove_1->angle;
					}
					return true;
				}
				else
				{
					prevframe = static_cast<float>(pmove->endframe);
					vecPos = pmove->position;
					vecAngle.y = pmove->angle;
				}
			}
		}

		return false;
	}
}

namespace r5
{
	static void Studio_FrameMovement(const ModelFrameMovement_t* pFrameMovement, int iFrame, Vector& vecPos, float& yaw)
	{
		uint16_t* section = pFrameMovement->pSection(iFrame);

		const int iLocalFrame = iFrame % pFrameMovement->sectionframes; // get the local frame for this section by doing a modulus (returns excess frames)

		for (int i = 0; i < 3; i++)
		{
			if (section[i])
				ExtractAnimValue(iLocalFrame, pFrameMovement->pAnimvalue(i, section), pFrameMovement->scale[i], vecPos[i]);
			else
				vecPos[i] = 0.0f;
		}

		if (section[3])
			ExtractAnimValue(iLocalFrame, pFrameMovement->pAnimvalue(3, section), pFrameMovement->scale[3], yaw);
		else
			yaw = 0.0f;
	}

	static void Studio_FrameMovement(const ModelFrameMovement_t* pFrameMovement, int iFrame, Vector& v1Pos, Vector& v2Pos, float& v1Yaw, float& v2Yaw)
	{
		uint16_t* section = pFrameMovement->pSection(iFrame);

		const int iLocalFrame = iFrame % pFrameMovement->sectionframes; // get the local frame for this section by doing a modulus (returns excess frames)

		for (int i = 0; i < 3; i++)
		{
			if (section[i])
				ExtractAnimValue(iLocalFrame, pFrameMovement->pAnimvalue(i, section), pFrameMovement->scale[i], v1Pos[i], v2Pos[i]);
			else
			{
				v1Pos[i] = 0.0f;
				v2Pos[i] = 0.0f;
			}
		}

		if (section[3])
			ExtractAnimValue(iLocalFrame, pFrameMovement->pAnimvalue(3, section), pFrameMovement->scale[3], v1Yaw, v2Yaw);
		else
		{
			v1Yaw = 0.0f;
			v2Yaw = 0.0f;
		}
	}

	// [rika]: decode datapoint compressed position track for framemovement
	static void Studio_FrameMovement_DP_Pos(const float fFrame, const int sectionlength, const uint8_t** const panimtrack, Vector& pos)
	{
		const uint16_t* ptrack = *reinterpret_cast<const uint16_t** const>(panimtrack);

		const uint16_t valid = ptrack[0];
		const uint16_t total = ptrack[1];

		const uint16_t* pFrameIndices = ptrack + 2;
		*panimtrack = reinterpret_cast<const uint8_t*>(pFrameIndices);

		// [rika]: return zeros if no data
		if (!total)
		{
			pos.Init(0.0f, 0.0f, 0.0f);
			return;
		}

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const AnimPos64* pPackedData = nullptr;
		const AxisFixup_t* pAxisFixup = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);

		pAxisFixup = reinterpret_cast<const AxisFixup_t*>(pPackedData + valid);

		// [rika]: see to our datapoint
		int validIdx = 0;
		uint32_t remainingFrames = 0;

		const uint32_t prevTarget = prevFrame;
		CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, /*valid,*/ prevTarget);

		Vector pos1;
		AnimPos64::Unpack(pos1, pPackedData[validIdx], &pAxisFixup[prevFrame]);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			pos = pos1;
		}
		else
		{
			// [rika]: see to our datapoint (the sequal)
			const uint32_t nextTarget = (nextFrame - prevFrame) + remainingFrames; // to check if our current data will contain this interp data, seek until we have it
			CalcBoneSeek_DP(pPackedData, validIdx, remainingFrames, nextTarget);

			Vector pos2;
			AnimPos64::Unpack(pos2, pPackedData[validIdx], &pAxisFixup[nextFrame]);

			pos = pos1 * (1.0f - s) + pos2 * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pAxisFixup + total);
	}

	// [rika]: decode datapoint compressed yaw track for framemovement
	static void Studio_FrameMovement_DP_Rot(const float fFrame, const int sectionlength, const uint8_t** const panimtrack, float& yaw)
	{
		const uint16_t* ptrack = *reinterpret_cast<const uint16_t** const>(panimtrack);

		const uint16_t total = ptrack[0];

		const uint16_t* pFrameIndices = ptrack + 1;
		*panimtrack = reinterpret_cast<const uint8_t*>(pFrameIndices);

		// [rika]: return zeros if no data
		if (!total)
		{
			yaw = 0.0f;
			return;
		}

		int prevFrame = 0, nextFrame = 0;
		float s = 0.0f; // always init as 0!

		// [rika]: get data pointers
		const float16* pPackedData = nullptr;

		CalcBoneInterpFrames_DP(prevFrame, nextFrame, s, fFrame, total, sectionlength, pFrameIndices, &pPackedData);

		// [rika]: check if we need interp or not
		if (prevFrame == nextFrame)
		{
			yaw = pPackedData[prevFrame].GetFloat();
		}
		else
		{
			const float v1Yaw = pPackedData[prevFrame].GetFloat();
			const float v2Yaw = pPackedData[nextFrame].GetFloat();

			// [rika]: differs slightly from AngleDiff()
			float yawDelta = v2Yaw;
			if ((v2Yaw - v1Yaw) <= 180.0f)
			{
				if ((v1Yaw - v2Yaw) > 180.0f)
					yawDelta = v2Yaw + 360.0f;
			}
			else
			{
				yawDelta = v2Yaw + -360.0f;
			}

			yaw = v1Yaw * (1.0f - s) + yawDelta * s;
		}

		// [rika]: advance the data ptr for other functions
		*panimtrack = reinterpret_cast<const uint8_t*>(pPackedData + total);
	}

	bool Studio_AnimPosition(const ModelAnim_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle)
	{
		vecPos.Init();
		vecAngle.Init();

		int iLoops = 0;
		if (flCycle > 1.0)
		{
			iLoops = static_cast<int>(flCycle);
		}
		else if (flCycle < 0.0)
		{
			iLoops = static_cast<int>(flCycle) - 1;
		}
		flCycle = flCycle - iLoops;

		float flFrame = flCycle * (panim->numframes - 1);

		if (panim->flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
		{
			int iFrame = static_cast<int>(flFrame);
			float s = (flFrame - iFrame);

			if (panim->flags & eStudioAnimFlags::ANIM_DATAPOINT)
			{
				const mstudioframemovement_t* const pFrameMovement = reinterpret_cast<const mstudioframemovement_t* const>(panim->framemovement->baseptr);

				// [rika]: remove after testing
#ifdef _DEBUG
				assertm(pFrameMovement->SectionCount(panim->numframes) == 1, "more than one section ?");
#endif // _DEBUG

				const uint8_t* panimtrack = pFrameMovement->pDatapointTrack();

				Studio_FrameMovement_DP_Pos(flFrame, panim->numframes, &panimtrack, vecPos);
				Studio_FrameMovement_DP_Rot(flFrame, panim->numframes, &panimtrack, vecAngle.y);

				return true;
			}
			else if (s == 0.0f)
			{
				Studio_FrameMovement(panim->framemovement, iFrame, vecPos, vecAngle.y);
				return true;
			}
			else
			{
				Vector v1Pos, v2Pos;
				float v1Yaw, v2Yaw;

				Studio_FrameMovement(panim->framemovement, iFrame, v1Pos, v2Pos, v1Yaw, v2Yaw);

				vecPos = ((v2Pos - v1Pos) * s) + v1Pos; // this should work on paper?

				float yawDelta = AngleDiff(v2Yaw, v1Yaw);
				vecAngle.y = (yawDelta * s) + v1Yaw;

				return true;
			}
		}

		assertm(false, "we should not be here");

		return false;
	}
}