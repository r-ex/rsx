#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/assets/model.h>
#include <game/rtech/assets/animrig.h>

// seasons 0 to 6
struct AnimSeqAssetHeader_v7_t
{
	void* data;
	char* name;

	AssetGuid_t* models; // guids, uint64_t*?
	uint64_t numModels;

	AssetGuid_t* settings; // guids, uint64_t*?
	uint64_t numSettings;
};
static_assert(sizeof(AnimSeqAssetHeader_v7_t) == 48);

// seasons 7 to 8
struct AnimSeqAssetHeader_v7_1_t
{
	void* data;
	char* name;

	AssetGuid_t* models; // guids, uint64_t*?
	uint32_t numModels;
	uint32_t dataExtraSize; // size of extra data

	AssetGuid_t* settings; // guids, uint64_t*?
	uint64_t numSettings;

	char* dataExtra; // external data, sometimes streamed
};
static_assert(sizeof(AnimSeqAssetHeader_v7_1_t) == 56);

// season 8.1 (v8) (R5pc_r5-81_J94_CL862269_2021_03_01_16_25)
// seasons 9 to 14 (v10)
// seasons 15 to
struct AnimSeqAssetHeader_v8_t
{
	void* data;
	char* name;

	void* unk_10; // gets set to an allocated struct for 16 bytes on asset load

	uint16_t numModels;
	uint16_t numSettings;

	uint32_t dataExtraSize; // size of extra data

	AssetGuid_t* models; // guids, uint64_t*?
	AssetGuid_t* effects; // guids, uint64_t*?
	AssetGuid_t* settings; // guids, uint64_t*?

	char* dataExtra; // external data, sometimes streamed
};
static_assert(sizeof(AnimSeqAssetHeader_v8_t) == 64);

enum class eSeqVersion : int
{
	VERSION_UNK = -1,
	VERSION_7,
	VERSION_7_1,
	VERSION_8,
	VERSION_10,
	VERSION_11,
	VERSION_12,
};

static const std::map<int, eSeqVersion> s_seqVersionMap
{
	{ 7, eSeqVersion::VERSION_7 },
	{ 8, eSeqVersion::VERSION_8 },
	{ 10, eSeqVersion::VERSION_10 },
	{ 11, eSeqVersion::VERSION_11 },
	{ 12, eSeqVersion::VERSION_12 },
};

inline const eSeqVersion GetAnimSeqVersionFromAsset(CPakAsset* const asset)
{
	eSeqVersion out = eSeqVersion::VERSION_UNK;

	if (s_seqVersionMap.count(asset->version()) == 1u)
		out = s_seqVersionMap.at(asset->version());

	if (out == eSeqVersion::VERSION_7)
		out = asset->data()->headerStructSize == sizeof(AnimSeqAssetHeader_v7_t) ? eSeqVersion::VERSION_7 : eSeqVersion::VERSION_7_1;

	return out;
}

class AnimSeqAsset
{
public:
	AnimSeqAsset(AnimSeqAssetHeader_v7_t* hdr, eSeqVersion ver) : name(hdr->name), data(hdr->data), models(hdr->models), effects(nullptr), settings(hdr->settings), numModels(static_cast<uint32_t>(hdr->numModels)), numSettings(static_cast<uint32_t>(hdr->numSettings)),
		dataExtraPerm(nullptr), dataExtraStreamed(), dataExtraSize(0), version(ver), seqdesc(reinterpret_cast<r5::mstudioseqdesc_v8_t*>(data)), parentModel(nullptr), parentRig(nullptr), animationParsed(false)
	{
		RawSizeV7();
	};

	AnimSeqAsset(AnimSeqAssetHeader_v7_1_t* hdr, AssetPtr_t streamedData, eSeqVersion ver) : name(hdr->name), data(hdr->data), models(hdr->models), effects(nullptr), settings(hdr->settings), numModels(hdr->numModels), numSettings(static_cast<uint32_t>(hdr->numSettings)),
		dataSize(0), dataExtraPerm(hdr->dataExtra), dataExtraStreamed(streamedData), dataExtraSize(hdr->dataExtraSize), version(ver), seqdesc(reinterpret_cast<r5::mstudioseqdesc_v8_t*>(data), dataExtraPerm), parentModel(nullptr), parentRig(nullptr), animationParsed(false)
	{
		RawSizeV7();
	};

	AnimSeqAsset(AnimSeqAssetHeader_v8_t* hdr, AssetPtr_t streamedData, eSeqVersion ver) : name(hdr->name), data(hdr->data), models(hdr->models), effects(hdr->effects), settings(hdr->settings), numModels(static_cast<uint32_t>(hdr->numModels)), numSettings(static_cast<uint32_t>(hdr->numSettings)),
		dataSize(0), dataExtraPerm(hdr->dataExtra), dataExtraStreamed(streamedData), dataExtraSize(hdr->dataExtraSize), version(ver), parentModel(nullptr), parentRig(nullptr), animationParsed(false)
	{
		switch (ver)
		{
		case eSeqVersion::VERSION_8:
		case eSeqVersion::VERSION_10:
		{
			seqdesc = seqdesc_t(reinterpret_cast<r5::mstudioseqdesc_v8_t*>(data), dataExtraPerm);

			RawSizeV7();

			break;
		}
		case eSeqVersion::VERSION_11:
		{
			r5::mstudioseqdesc_v16_t* const tmp = reinterpret_cast<r5::mstudioseqdesc_v16_t* const>(data);

			seqdesc = seqdesc_t(tmp, dataExtraPerm);

			const int boneCount = (tmp->activitymodifierindex - tmp->weightlistindex) / 4;

			RawSizeV11(boneCount);

			break;
		}
		case eSeqVersion::VERSION_12:
		{
			r5::mstudioseqdesc_v18_t* const tmp = reinterpret_cast<r5::mstudioseqdesc_v18_t* const>(data);

			seqdesc = seqdesc_t(tmp, dataExtraPerm);

			if (tmp->weightlistindex == 1 || tmp->weightlistindex == 3)
			{
				dataSize = 0;
				break;
			}

			const int boneCount = (tmp->activitymodifierindex - tmp->weightlistindex) / 4;

			RawSizeV11(boneCount);

			break;
		}
		default:
			break;
		}
	};

	char* name;
	void* data;

	AssetGuid_t* models; // guids, uint64_t*?
	AssetGuid_t* effects; // guids, uint64_t*?
	AssetGuid_t* settings; // guids, uint64_t*?

	uint32_t numModels;
	uint32_t numSettings;

	size_t dataSize;
	char* dataExtraPerm; // external data, sometimes streamed
	AssetPtr_t dataExtraStreamed;
	uint32_t dataExtraSize; // size of extra data

	ModelAsset* parentModel;
	AnimRigAsset* parentRig;

	bool animationParsed;

	eSeqVersion version;

	seqdesc_t seqdesc;

	inline const bool UseStall() const { return version == eSeqVersion::VERSION_7 ? false : true; };
	inline void UpdateDataSizeNew(const int boneCount) { RawSizeV11(boneCount); }

private:
	inline void RawSizeV7()
	{
		const char* end = seqdesc.szlabel;

		// the single sequence that doesn't have animations only has a label string, to set up parsing for all other strings would be a waste.

		// animation names will always be last
		for (auto& anim : seqdesc.anims)
			end = anim.name > end ? anim.name : end;

		end += strnlen_s(end, MAX_PATH) + 1; // plus null terminator
		dataSize = IALIGN4(end - (char*)seqdesc.baseptr);
	}

	// this can't be done with strings, as data is after it. from memory the data order is: animation, framemovement, compressedikerrors. to get the end parsing through these animvalue tracks will be required.


	void RawSizeV11(const int boneCount)
	{
		if (boneCount == 0)
		{
			dataSize = 0;
			return;
		}

		// start at the last, and work back if required.
		for (int i = seqdesc.AnimCount() - 1; i >= 0; i--)
		{
			const animdesc_t* const anim = &seqdesc.anims.at(i);			

			const r5::mstudioanimdesc_v16_t* const animdesc = reinterpret_cast<const r5::mstudioanimdesc_v16_t* const>(anim->baseptr);
			int lastFrame = anim->numframes - 1;

			// if the last data is framemovement data
			if (anim->flags & ANIM_FRAMEMOVEMENT && anim->framemovementindex > 0)
			{
				const r5::mstudioframemovement_t* const pFrameMovement = animdesc->pFrameMovement();
				const uint16_t* const pSection = pFrameMovement->pSection(lastFrame, true);

				for (int j = 3; j >= 0; j--)
				{
					const mstudioanimvalue_t* panimvalue = pFrameMovement->pAnimvalue(j, pSection);

					if (!panimvalue)
						continue;

					// track seeking can probably be done better at a later date, but for now this will work for our purposes.
					int k = lastFrame % pFrameMovement->sectionframes;

					// seek through tracks
					while (panimvalue->num.total <= k)
					{
						k -= panimvalue->num.total;
						panimvalue += r5::GetAnimValueOffset(panimvalue); // [rika]: this is a macro because I thought it was used more than once initally
					}

					// seek through final animvalue track
					panimvalue += r5::GetAnimValueOffset(panimvalue);

					// the animseq's data ends at an animvalue track's end, seek to find the end, and use it as our endpoint.
					dataSize = IALIGN4(reinterpret_cast<const char*>(panimvalue) - reinterpret_cast<const char*>(seqdesc.baseptr));

					return;
				}

				// the animseq's data ends at the last section in a framemovement, take the pointer for it, and add the section's size to get our endpoint.
				dataSize = IALIGN4(reinterpret_cast<const char*>(pSection + 4) - reinterpret_cast<const char*>(seqdesc.baseptr));

				return;
			}

			// if the last data is animation data
			if (anim->flags & ANIM_VALID || !(anim->flags & ANIM_ALLZEROS))
			{
				const uint8_t* boneFlagArray = reinterpret_cast<uint8_t*>(anim->pAnimdataStall(&lastFrame));

				const r5::mstudio_rle_anim_t* panim = reinterpret_cast<const r5::mstudio_rle_anim_t*>(&boneFlagArray[ANIM_BONEFLAG_SIZE(boneCount)]);

				for (int j = 0; j < boneCount; j++)
				{
					const uint8_t boneFlags = ANIM_BONEFLAGS_FLAG(boneFlagArray, j);

					if (!(boneFlags & r5::RleBoneFlags_t::STUDIO_ANIM_DATA))
						continue;

					panim = panim->pNext();
				}

				// the animseq's data ends at the last rle bone, parse through all bones until the pointer is no longer advanced, and use the final pointer for our endpoint.
				dataSize = IALIGN4(reinterpret_cast<const char*>(panim) - reinterpret_cast<const char*>(seqdesc.baseptr));

				return;
			}

			// ikrules are static and do not have compresedikerrors (new as of v12)
			if (animdesc->ikruleindex == 3 || animdesc->ikruleindex == 5)
				return;

			// not likely to get hit, but we should cover our bases.
			// if the last data is compressedikerror data
			if (animdesc->numikrules > 0)
			{
				for (int j = static_cast<int>(animdesc->numikrules) - 1; j >= 0; j--)
				{
					const r5::mstudioikrule_v16_t* const pIkRule = animdesc->pIKRule(j); // bad ptr

					if (pIkRule->compressedikerror.sectionframes == 0)
						continue;


					const uint16_t* const pSection = pIkRule->pSection(lastFrame);

					// indices 3-5 (rotation) are invalid in apex for whatever reason (at least retail versions).
					for (int l = 2; l >= 0; l--)
					{
						const mstudioanimvalue_t* panimvalue = pIkRule->pAnimvalue(l, pSection);

						if (!panimvalue)
							continue;

						// track seeking can probably be done better at a later date, but for now this will work for our purposes.
						int k = lastFrame % pIkRule->compressedikerror.sectionframes;

						// seek through tracks
						while (panimvalue->num.total <= k)
						{
							k -= panimvalue->num.total;
							panimvalue += r5::GetAnimValueOffset(panimvalue); // [rika]: this is a macro because I thought it was used more than once initally
						}

						// seek through final animvalue track
						panimvalue += r5::GetAnimValueOffset(panimvalue);

						// the animseq's data ends at an animvalue track's end, seek to find the end, and use it as our endpoint.
						dataSize = IALIGN4(reinterpret_cast<const char*>(panimvalue) - reinterpret_cast<const char*>(seqdesc.baseptr));

						return;
					}

					// the animseq's data ends at the last section in a framemovement, take the pointer for it, and add the section's size to get our endpoint.
					dataSize = IALIGN4(reinterpret_cast<const char*>(pSection + 4) - reinterpret_cast<const char*>(seqdesc.baseptr));

					return;
				}
			}
		}

		const char* end = seqdesc.szlabel;
		end += strnlen_s(end, MAX_PATH) + 1; // plus null terminator

		dataSize = IALIGN4(end - (char*)seqdesc.baseptr);

		return;
	}
};

bool ExportAnimSeqAsset(CPakAsset* const asset, const int setting, const AnimSeqAsset* const animSeqAsset, const std::filesystem::path& exportPath, const char* const skelName, const std::vector<ModelBone_t>* bones);
bool ExportAnimSeqFromAsset(const std::filesystem::path& exportPath, const std::string& stem, const char* const name, const int numAnimSeqs, const AssetGuid_t* const animSeqs, const std::vector<ModelBone_t>* const bones);