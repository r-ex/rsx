#include <pch.h>
#include <game/rtech/utils/studio/studio_generic.h>

// generic studio data
// movement
animmovement_t::animmovement_t(const animmovement_t & movement) : baseptr(movement.baseptr), sectionframes(movement.sectionframes), sectioncount(movement.sectioncount)
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

animmovement_t::animmovement_t(r1::mstudioframemovement_t* movement) : baseptr(movement), sectionframes(0), sectioncount(0)
{
	std::memcpy(this->scale, movement->scale, sizeof(float) * 4);
	std::memcpy(this->offset, movement->offset, sizeof(short) * 4);
}

animmovement_t::animmovement_t(r5::mstudioframemovement_t* movement, const int frameCount, const bool indexType) : baseptr(movement), sectionframes(movement->sectionframes), sectioncount(movement->SectionCount(frameCount))
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
animdesc_t::animdesc_t(const animdesc_t& animdesc) : baseptr(animdesc.baseptr), name(animdesc.name), fps(animdesc.fps), flags(animdesc.flags), numframes(animdesc.numframes), animindex(animdesc.animindex),
sectionindex(animdesc.sectionindex), sectionframes(animdesc.sectionframes), sectionstallframes(animdesc.sectionstallframes), sectionDataExtra(animdesc.sectionDataExtra), nummovements(animdesc.nummovements), movementindex(animdesc.movementindex), framemovementindex(animdesc.framemovementindex), movement(nullptr), parsedBufferIndex(animdesc.parsedBufferIndex)
{
	if (animdesc.sections.size() > 0)
	{
		sections.resize(animdesc.sections.size());
		std::memcpy(sections.data(), animdesc.sections.data(), sizeof(animsection_t) * sections.size());
	}

	if (nullptr != animdesc.movement)
		movement = new animmovement_t(*animdesc.movement);
}

animdesc_t::animdesc_t(r2::mstudioanimdesc_t* animdesc) : baseptr(reinterpret_cast<void*>(animdesc)), name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
sectionindex(animdesc->sectionindex), sectionframes(animdesc->sectionframes), sectionstallframes(0), sectionDataExtra(nullptr), nummovements(animdesc->nummovements), movementindex(animdesc->movementindex), framemovementindex(animdesc->framemovementindex), movement(nullptr), parsedBufferIndex(0ull)
{
	flags |= eStudioAnimFlags::ANIM_VALID;

	if (sectionframes)
	{
		sections.resize(SectionCount());

		for (int i = 0; i < SectionCount(); i++)
		{
			sections.at(i) = { .animindex = animdesc->pSection(i)->animindex, .isExternal = false };
		}
	}

	if (framemovementindex && flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
	{
		movement = new animmovement_t(animdesc->pFrameMovement());
	}
};

animdesc_t::animdesc_t(r5::mstudioanimdesc_v8_t* animdesc) : baseptr(reinterpret_cast<void*>(animdesc)), name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
sectionindex(animdesc->sectionindex), sectionframes(animdesc->sectionframes), sectionstallframes(0), sectionDataExtra(nullptr), nummovements(animdesc->nummovements), movementindex(animdesc->movementindex), framemovementindex(animdesc->framemovementindex), movement(nullptr), parsedBufferIndex(0ull)
{
	if (sectionframes)
	{
		sections.resize(SectionCount());

		for (int i = 0; i < SectionCount(); i++)
		{
			sections.at(i) = { .animindex = animdesc->pSection(i)->animindex, .isExternal = false };
		}
	}

	if (framemovementindex && flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
	{
		movement = new animmovement_t(animdesc->pFrameMovement(), numframes, false);
	}
};

animdesc_t::animdesc_t(r5::mstudioanimdesc_v12_1_t* animdesc, char* ext) : baseptr(reinterpret_cast<void*>(animdesc)), name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
sectionindex(animdesc->sectionindex), sectionframes(animdesc->sectionframes), sectionstallframes(animdesc->sectionstallframes), sectionDataExtra(ext), nummovements(animdesc->nummovements), movementindex(animdesc->movementindex), framemovementindex(animdesc->framemovementindex), movement(nullptr), parsedBufferIndex(0ull)
{
	if (sectionframes)
	{
		sections.resize(SectionCount());

		for (int i = 0; i < SectionCount(); i++)
		{
			sections.at(i) = { .animindex = animdesc->pSection(i)->animindex, .isExternal = animdesc->pSection(i)->isExternal };
		}
	}

	if (framemovementindex && flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
	{
		movement = new animmovement_t(animdesc->pFrameMovement(), numframes, false);
	}
};

animdesc_t::animdesc_t(r5::mstudioanimdesc_v16_t* animdesc, char* ext) : baseptr(reinterpret_cast<void*>(animdesc)), name(animdesc->pszName()), fps(animdesc->fps), flags(animdesc->flags), numframes(animdesc->numframes), animindex(animdesc->animindex),
sectionindex(static_cast<int>(FIX_OFFSET(animdesc->sectionindex))), sectionframes(static_cast<int>(animdesc->sectionframes)), sectionstallframes(static_cast<int>(animdesc->sectionstallframes)), sectionDataExtra(ext), nummovements(0), movementindex(0), framemovementindex(FIX_OFFSET(animdesc->framemovementindex)), movement(nullptr), parsedBufferIndex(0ull)
{
	if (sectionframes)
	{
		sections.resize(SectionCount());

		for (int i = 0; i < SectionCount(); i++)
		{
			sections.at(i) = { .animindex = animdesc->pSection(i)->Index(), .isExternal = animdesc->pSection(i)->isExternal() };
		}
	}

	if (framemovementindex && flags & eStudioAnimFlags::ANIM_FRAMEMOVEMENT)
	{
		movement = new animmovement_t(animdesc->pFrameMovement(), numframes, true);
	}
};

char* animdesc_t::pAnimdataStall(int* piFrame) const
{
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

char* animdesc_t::pAnimdataNoStall(int* piFrame) const
{
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

// seqdesc
seqdesc_t::seqdesc_t(r2::mstudioseqdesc_t* seqdesc) : baseptr(reinterpret_cast<void*>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivityName()), flags(seqdesc->flags), weightlistindex(seqdesc->weightlistindex), parsedData(seqdesc->AnimCount())
{
	r2::studiohdr_t* const pStudioHdr = reinterpret_cast<r2::studiohdr_t* const>(reinterpret_cast<char*>(seqdesc) + seqdesc->baseptr);

	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		r2::mstudioanimdesc_t* const pAnimdesc = pStudioHdr->pAnimdesc(seqdesc->GetAnimIndex(i));

		// no need to sanity check here

		anims.emplace_back(pAnimdesc);
	}
};

#define ANIMDESC_SANITY_CHECK(anim) (anim->fps < 0.0f || anim->fps > 2048.f || anim->numframes < 0 || anim->numframes > 0x20000)
seqdesc_t::seqdesc_t(r5::mstudioseqdesc_v8_t* seqdesc) : baseptr(reinterpret_cast<void*>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivity()), flags(seqdesc->flags), weightlistindex(seqdesc->weightlistindex), parsedData(seqdesc->AnimCount())
{
	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		r5::mstudioanimdesc_v8_t* const pAnimdesc = seqdesc->pAnimDescV8(i);

		// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
		// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
		if (ANIMDESC_SANITY_CHECK(pAnimdesc))
		{
			Log("Sequence %s had animation(s) (index %i), but no animation description, skipping...\n", seqdesc->pszLabel(), i);
			break;
		}

		anims.emplace_back(pAnimdesc);
	}
};

seqdesc_t::seqdesc_t(r5::mstudioseqdesc_v8_t* seqdesc, char* ext) : baseptr(reinterpret_cast<void*>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivity()), flags(seqdesc->flags), weightlistindex(seqdesc->weightlistindex), parsedData(seqdesc->AnimCount())
{
	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		r5::mstudioanimdesc_v12_1_t* const pAnimdesc = seqdesc->pAnimDescV12_1(i);

		// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
		// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
		if (ANIMDESC_SANITY_CHECK(pAnimdesc))
		{
			Log("Sequence %s had animation(s) (index %i), but no animation description, skipping...\n", seqdesc->pszLabel(), i);
			break;
		}

		anims.emplace_back(pAnimdesc, ext);
	}
};

seqdesc_t::seqdesc_t(r5::mstudioseqdesc_v16_t* seqdesc, char* ext) : baseptr(reinterpret_cast<void*>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivity()), flags(seqdesc->flags), weightlistindex(FIX_OFFSET(seqdesc->weightlistindex)), parsedData(seqdesc->AnimCount())
{
	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		r5::mstudioanimdesc_v16_t* const pAnimdesc = seqdesc->pAnimDesc(i);

		// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
		// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
		if (ANIMDESC_SANITY_CHECK(pAnimdesc))
		{
			Log("Sequence %s had animation(s) (index %i), but no animation description, skipping...\n", seqdesc->pszLabel(), i);
			break;
		}

		anims.emplace_back(pAnimdesc, ext);
	}
};

seqdesc_t::seqdesc_t(r5::mstudioseqdesc_v18_t* seqdesc, char* ext) : baseptr(reinterpret_cast<void*>(seqdesc)), szlabel(seqdesc->pszLabel()), szactivityname(seqdesc->pszActivity()), flags(seqdesc->flags), weightlistindex(seqdesc->weightlistindex), parsedData(seqdesc->AnimCount())
{
	if (weightlistindex != 1 && weightlistindex != 3)
		weightlistindex = FIX_OFFSET(weightlistindex);

	for (int i = 0; i < seqdesc->AnimCount(); i++)
	{
		r5::mstudioanimdesc_v16_t* const pAnimdesc = seqdesc->pAnimDesc(i);

		// sanity checks, there are sequences that have animations, but no data for the anim descriptions, and I am unsure how the game checks them.
		// fps can't be negative, fps practically shouldn't be more than 2048, 128k frames is an absurd amount, so this is a very good check, since the number (int) should never have those last bits filled.
		if (ANIMDESC_SANITY_CHECK(pAnimdesc))
		{
			Log("Sequence %s had animation(s) (index %i), but no animation description, skipping...\n", seqdesc->pszLabel(), i);
			break;
		}

		anims.emplace_back(pAnimdesc, ext);
	}
};
#undef ANIMDESC_SANITY_CHECK // remove if needed in other files

// studio hw group
studio_hw_groupdata_t::studio_hw_groupdata_t(const r5::studio_hw_groupdata_v16_t* const group) : dataOffset(group->dataOffset), dataSizeCompressed(group->dataSizeCompressed), dataSizeDecompressed(group->dataSizeDecompressed), dataCompression(group->dataCompression),
lodIndex(group->lodIndex), lodCount(group->lodCount), lodMap(group->lodMap)
{

};

studio_hw_groupdata_t::studio_hw_groupdata_t(const r5::studio_hw_groupdata_v12_1_t* const group) : dataOffset(group->dataOffset), dataSizeCompressed(-1), dataSizeDecompressed(group->dataSize), dataCompression(eCompressionType::NONE),
lodIndex(static_cast<uint8_t>(group->lodIndex)), lodCount(static_cast<uint8_t>(group->lodCount)), lodMap(static_cast<uint8_t>(group->lodMap))
{

};

// studiohdr
// clamp silly studiomdl offsets (negative is base ptr)
#define FIX_FILE_OFFSET(offset) (offset < 0 ? 0 : offset)
studiohdr_generic_t::studiohdr_generic_t(const r1::studiohdr_t* const pHdr, StudioLooseData_t* const pData) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(pData->VertOffset(StudioLooseData_t::SLD_VTX)), vvdOffset(pData->VertOffset(StudioLooseData_t::SLD_VVD)), vvcOffset(pData->VertOffset(StudioLooseData_t::SLD_VVC)), vvwOffset(0), phyOffset(pData->PhysOffset()),
vtxSize(pData->VertSize(StudioLooseData_t::SLD_VTX)), vvdSize(pData->VertSize(StudioLooseData_t::SLD_VVD)), vvcSize(pData->VertSize(StudioLooseData_t::SLD_VVC)), vvwSize(0), phySize(pData->PhysSize()), hwDataSize(0), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(0), boneStateCount(0), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(0), groups(), bvhOffset(-1), baseptr(reinterpret_cast<const char*>(pHdr))
{

};

studiohdr_generic_t::studiohdr_generic_t(const r2::studiohdr_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(0), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(0), phySize(pHdr->phySize), hwDataSize(0), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(0), boneStateCount(0), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(0), groups(), bvhOffset(-1), baseptr(reinterpret_cast<const char*>(pHdr))
{

};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v8_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(FIX_FILE_OFFSET(pHdr->vvwOffset)), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(pHdr->vvwSize), phySize(pHdr->phySize), hwDataSize(0)/*set in vertex parsing*/, textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(0), boneStateCount(0), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(1), groups(), bvhOffset(pHdr->bvhOffset), baseptr(reinterpret_cast<const char*>(pHdr))
{

};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v12_1_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(FIX_FILE_OFFSET(pHdr->vvwOffset)), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(pHdr->vvwSize), phySize(pHdr->phySize), hwDataSize(pHdr->hwDataSize), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(offsetof(r5::studiohdr_v12_1_t, boneStateOffset) + pHdr->boneStateOffset), boneStateCount(pHdr->boneStateCount), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(pHdr->groupHeaderCount), groups(), bvhOffset(pHdr->bvhOffset), baseptr(reinterpret_cast<const char*>(pHdr))
{
	assertm(pHdr->groupHeaderCount <= 8, "model has more than 8 lods");

	for (int i = 0; i < groupCount; i++)
	{
		if (i == 8)
			break;

		groups[i] = studio_hw_groupdata_t(pHdr->pLODGroup(i));
	}
};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v12_2_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(FIX_FILE_OFFSET(pHdr->vvwOffset)), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(pHdr->vvwSize), phySize(pHdr->phySize), hwDataSize(pHdr->hwDataSize), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(offsetof(r5::studiohdr_v12_2_t, boneStateOffset) + pHdr->boneStateOffset), boneStateCount(pHdr->boneStateCount), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(pHdr->groupHeaderCount), groups(), bvhOffset(pHdr->bvhOffset), baseptr(reinterpret_cast<const char*>(pHdr))
{
	assertm(pHdr->groupHeaderCount <= 8, "model has more than 8 lods");

	for (int i = 0; i < groupCount; i++)
	{
		if (i == 8)
			break;

		groups[i] = studio_hw_groupdata_t(pHdr->pLODGroup(i));
	}
};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v12_3_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(FIX_FILE_OFFSET(pHdr->vvwOffset)), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(pHdr->vvwSize), phySize(pHdr->phySize), hwDataSize(pHdr->hwDataSize), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(offsetof(r5::studiohdr_v12_3_t, boneStateOffset) + pHdr->boneStateOffset), boneStateCount(pHdr->boneStateCount), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(pHdr->groupHeaderCount), groups(), bvhOffset(pHdr->bvhOffset), baseptr(reinterpret_cast<const char*>(pHdr))
{
	assertm(pHdr->groupHeaderCount <= 8, "model has more than 8 lods");

	for (int i = 0; i < groupCount; i++)
	{
		if (i == 8)
			break;

		groups[i] = studio_hw_groupdata_t(pHdr->pLODGroup(i));
	}
};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v14_t* const pHdr) : length(pHdr->length), flags(pHdr->flags), contents(pHdr->contents), vtxOffset(FIX_FILE_OFFSET(pHdr->vtxOffset)), vvdOffset(FIX_FILE_OFFSET(pHdr->vvdOffset)), vvcOffset(FIX_FILE_OFFSET(pHdr->vvcOffset)), vvwOffset(FIX_FILE_OFFSET(pHdr->vvwOffset)), phyOffset(FIX_FILE_OFFSET(pHdr->phyOffset)),
vtxSize(pHdr->vtxSize), vvdSize(pHdr->vvdSize), vvcSize(pHdr->vvcSize), vvwSize(pHdr->vvwSize), phySize(pHdr->phySize), hwDataSize(pHdr->hwDataSize), textureOffset(pHdr->textureindex), textureCount(pHdr->numtextures), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(pHdr->skinindex),
boneOffset(pHdr->boneindex), boneDataOffset(-1), boneCount(pHdr->numbones), boneStateOffset(offsetof(r5::studiohdr_v14_t, boneStateOffset) + pHdr->boneStateOffset), boneStateCount(pHdr->boneStateCount), localSequenceOffset(pHdr->localseqindex), localSequenceCount(pHdr->numlocalseq),
groupCount(pHdr->groupHeaderCount), groups(), bvhOffset(pHdr->bvhOffset), baseptr(reinterpret_cast<const char*>(pHdr))
{
	assertm(pHdr->groupHeaderCount <= 8, "model has more than 8 lods");

	for (int i = 0; i < groupCount; i++)
	{
		if (i == 8)
			break;

		groups[i] = studio_hw_groupdata_t(pHdr->pLODGroup(i));
	}
};

studiohdr_generic_t::studiohdr_generic_t(const r5::studiohdr_v16_t* const pHdr, int dataSizePhys, int dataSizeModel) : length(dataSizeModel), flags(pHdr->flags), contents(0 /*TODO, where is this?*/), vtxOffset(-1), vvdOffset(-1), vvcOffset(-1), vvwOffset(-1), phyOffset(-1),
vtxSize(-1), vvdSize(-1), vvcSize(-1), vvwSize(-1), phySize(dataSizePhys), hwDataSize(0), textureOffset(FIX_OFFSET(pHdr->textureindex)), textureCount(static_cast<int>(pHdr->numtextures)), numSkinRef(pHdr->numskinref), numSkinFamilies(pHdr->numskinfamilies), skinOffset(FIX_OFFSET(pHdr->skinindex)),
boneOffset(FIX_OFFSET(pHdr->boneHdrOffset)), boneDataOffset(FIX_OFFSET(pHdr->boneDataOffset)), boneCount(static_cast<int>(pHdr->boneCount)), boneStateOffset(offsetof(r5::studiohdr_v16_t, boneStateOffset) + FIX_OFFSET(pHdr->boneStateOffset)), boneStateCount(pHdr->boneStateCount), localSequenceOffset(FIX_OFFSET(pHdr->localseqindex)), localSequenceCount(pHdr->numlocalseq),
groupCount(pHdr->groupHeaderCount), groups(), bvhOffset(FIX_OFFSET(pHdr->bvhOffset)), baseptr(reinterpret_cast<const char*>(pHdr))
{
	assertm(pHdr->groupHeaderCount <= 8, "model has more than 8 lods");

	for (int i = 0; i < groupCount; i++)
	{
		if (i == 8)
			break;

		groups[i] = studio_hw_groupdata_t(pHdr->pLODGroup(static_cast<uint16_t>(i)));
		hwDataSize += groups[i].dataSizeDecompressed;
	}

};
#undef FIX_FILE_OFFSET

// per version funcs that utilize generic data
namespace r1
{
	void Studio_FrameMovement(const animmovement_t* pFrameMovement, int frame, Vector& vecPos, float& yaw)
	{
		for (int i = 0; i < 3; i++)
		{
			ExtractAnimValue(frame, pFrameMovement->pAnimvalue(i), pFrameMovement->scale[i], vecPos[i]);
		}

		ExtractAnimValue(frame, pFrameMovement->pAnimvalue(3), pFrameMovement->scale[3], yaw);
	}

	void Studio_FrameMovement(const animmovement_t* pFrameMovement, int iFrame, Vector& v1Pos, Vector& v2Pos, float& v1Yaw, float& v2Yaw)
	{
		for (int i = 0; i < 3; i++)
		{
			ExtractAnimValue(iFrame, pFrameMovement->pAnimvalue(i), pFrameMovement->scale[i], v1Pos[i], v2Pos[i]);
		}

		ExtractAnimValue(iFrame, pFrameMovement->pAnimvalue(3), pFrameMovement->scale[3], v1Yaw, v2Yaw);
	}

	bool Studio_AnimPosition(const animdesc_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle)
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
				Studio_FrameMovement(panim->movement, iFrame, vecPos, vecAngle.y);
				return true;
			}
			else
			{
				Vector v1Pos, v2Pos;
				float v1Yaw, v2Yaw;

				Studio_FrameMovement(panim->movement, iFrame, v1Pos, v2Pos, v1Yaw, v2Yaw);

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
				mstudiomovement_t* pmove = panim->pMovement(i);

				if (pmove->endframe >= flFrame)
				{
					float f = (flFrame - prevframe) / (pmove->endframe - prevframe);

					float d = pmove->v0 * f + 0.5f * (pmove->v1 - pmove->v0) * f * f;

					vecPos = vecPos + (pmove->vector * d);
					vecAngle.y = vecAngle.y * (1 - f) + pmove->angle * f;
					if (iLoops != 0)
					{
						mstudiomovement_t* pmove_1 = panim->pMovement(panim->nummovements - 1);
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
	static void Studio_FrameMovement(const animmovement_t* pFrameMovement, int iFrame, Vector& vecPos, float& yaw)
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

	static void Studio_FrameMovement(const animmovement_t* pFrameMovement, int iFrame, Vector& v1Pos, Vector& v2Pos, float& v1Yaw, float& v2Yaw)
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

	bool Studio_AnimPosition(const animdesc_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle)
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

			if (s == 0)
			{
				Studio_FrameMovement(panim->movement, iFrame, vecPos, vecAngle.y);
				return true;
			}
			else
			{
				Vector v1Pos, v2Pos;
				float v1Yaw, v2Yaw;

				Studio_FrameMovement(panim->movement, iFrame, v1Pos, v2Pos, v1Yaw, v2Yaw);

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