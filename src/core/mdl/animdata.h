#pragma once

#include <game/rtech/cpakfile.h>

#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>
#include <game/rtech/utils/studio/studio_r2.h>
#include <game/rtech/utils/studio/studio_r5.h>
#include <game/rtech/utils/studio/studio_r5_v12.h>
#include <game/rtech/utils/studio/studio_r5_v16.h>

struct ModelBone_t;
struct ModelIKLock_t;
class ModelParsedData_t;

//
// PARSEDDATA
//

struct ModelIKRule_t
{
	ModelIKRule_t() = default;
	ModelIKRule_t(const ModelIKRule_t* const pIKRule) : attachment(pIKRule->attachment), compressedikerror(nullptr), ikerror(nullptr), index(pIKRule->index), type(pIKRule->type), chain(pIKRule->chain), bone(pIKRule->bone), slot(pIKRule->slot),
		height(pIKRule->height), radius(pIKRule->radius), floor(pIKRule->floor), pos(pIKRule->pos), q(pIKRule->q),
		iStart(pIKRule->iStart), start(pIKRule->start), peak(pIKRule->peak), tail(pIKRule->tail), end(pIKRule->end), contact(pIKRule->contact), drop(pIKRule->drop), top(pIKRule->top), endHeight(pIKRule->endHeight)
	{
		assertm(drop == 0.0f, "drop was used");
		assertm(top == 0.0f, "top was used");

		assertm(pIKRule->compressedikerror == nullptr, "unsupported");
		assertm(pIKRule->ikerror == nullptr, "unsupported");
	}
	ModelIKRule_t(const r2::mstudioikrule_t* const pIKRule) : attachment(pIKRule->pszAttachment()), compressedikerror(nullptr), ikerror(nullptr), index(pIKRule->index), type(pIKRule->type), chain(pIKRule->chain), bone(pIKRule->bone), slot(pIKRule->slot),
		height(pIKRule->height), radius(pIKRule->radius), floor(pIKRule->floor), pos(pIKRule->pos), q(pIKRule->q),
		iStart(pIKRule->iStart), start(pIKRule->start), peak(pIKRule->peak), tail(pIKRule->tail), end(pIKRule->end), contact(pIKRule->contact), drop(pIKRule->drop), top(pIKRule->top), endHeight(pIKRule->endHeight)
	{
		assertm(drop == 0.0f, "drop was used");
		assertm(top == 0.0f, "top was used");

		// todo
		if (pIKRule->compressedikerrorindex)
		{
			//assertm(false, "not implemented");
		}

		if (pIKRule->ikerrorindex)
		{
			assertm(false, "not implemented");
		}
	}
	ModelIKRule_t(const r5::mstudioikrule_v8_t* const pIKRule) : attachment(pIKRule->pszAttachment()), compressedikerror(nullptr), ikerror(nullptr), index(pIKRule->index), type(pIKRule->type), chain(pIKRule->chain), bone(pIKRule->bone), slot(pIKRule->slot),
		height(pIKRule->height), radius(pIKRule->radius), floor(pIKRule->floor), pos(pIKRule->pos), q(pIKRule->q),
		iStart(pIKRule->iStart), start(pIKRule->start), peak(pIKRule->peak), tail(pIKRule->tail), end(pIKRule->end), contact(pIKRule->contact), drop(pIKRule->drop), top(pIKRule->top), endHeight(pIKRule->endHeight)
	{
		assertm(drop == 0.0f, "drop was used");
		assertm(top == 0.0f, "top was used");

		// todo
		if (pIKRule->compressedikerrorindex)
		{
			//assertm(false, "not implemented");
		}

		if (pIKRule->ikerrorindex)
		{
			assertm(false, "not implemented");
		}
	}
	ModelIKRule_t(const r5::mstudioikrule_v16_t* const pIKRule) : attachment(pIKRule->pszAttachment()), compressedikerror(nullptr), ikerror(nullptr), index(pIKRule->slot), type(pIKRule->type), chain(pIKRule->chain), bone(pIKRule->bone), slot(pIKRule->slot),
		height(pIKRule->height), radius(pIKRule->radius), floor(pIKRule->floor), pos(pIKRule->pos), q(pIKRule->q),
		iStart(pIKRule->iStart), start(pIKRule->start), peak(pIKRule->peak), tail(pIKRule->tail), end(pIKRule->end), contact(pIKRule->contact), drop(pIKRule->drop), top(pIKRule->top), endHeight(pIKRule->endHeight)
	{
		assertm(drop == 0.0f, "drop was used");
		assertm(top == 0.0f, "top was used");

		// todo
		if (pIKRule->compressedikerrorindex)
		{
			//assertm(false, "not implemented");
		}

		if (pIKRule->ikerrorindex)
		{
			assertm(false, "not implemented");
		}
	}
	~ModelIKRule_t()
	{
		FreeAllocVar(compressedikerror);
		FreeAllocVar(ikerror);
	}

	ModelIKRule_t& operator=(const ModelIKRule_t&&) = delete;
	ModelIKRule_t& operator=(ModelIKRule_t&& ikrule) noexcept
	{
		if (this != &ikrule)
		{
			memcpy_s(this, sizeof(ModelIKRule_t), &ikrule, sizeof(ModelIKRule_t));

			compressedikerror = ikrule.compressedikerror;
			ikerror = ikrule.ikerror;

			ikrule.compressedikerror = nullptr;
			ikrule.ikerror = nullptr;
		}

		return *this;
	}

	enum Type_t : int
	{
		IKRULE_SELF = 1,
		IKRULE_WORLD = 2,
		IKRULE_GROUND = 3,
		IKRULE_RELEASE = 4,
		IKRULE_ATTACHMENT = 5,
		IKRULE_UNLATCH = 6,
	};

	const char* attachment;
	void* compressedikerror;
	void* ikerror;

	int index;
	int type;
	int chain;
	int bone;

	int slot; // iktarget slot. Usually same as chain.
	float height;
	float radius;
	float floor;
	Vector pos;
	Quaternion q;

	int iStart;

	float start; // beginning of influence
	float peak; // start of full influence
	float tail; // end of full influence
	float end; // end of all influence

	float contact; // frame footstep makes ground concact
	float drop; // how far down the foot should drop when reaching for IK
	float top;	// top of the foot box

	float endHeight;
};

struct ModelFrameMovement_t
{
	ModelFrameMovement_t(const ModelFrameMovement_t& movement);
	ModelFrameMovement_t(const r1::mstudioframemovement_t* const movement);
	ModelFrameMovement_t(const r5::mstudioframemovement_t* const movement, const int frameCount, const bool indexType);

	~ModelFrameMovement_t()
	{
		// [rika]: make sure we're using sections in this
		if (sectioncount > 0)
		{
			FreeAllocArray(sections);
		}
	}

	const void* baseptr; // the original data

	float scale[4];

	union
	{
		int* sections;
		short offset[4];
	};
	int sectionframes;
	int sectioncount;

	inline int SectionIndex(const int frame) const { return frame / sectionframes; } // get the section index for this frame
	inline int SectionOffset(const int frame) const { return sections[SectionIndex(frame)]; }
	inline uint16_t* pSection(const int frame) const { return reinterpret_cast<uint16_t*>((char*)baseptr + SectionOffset(frame)); }

	inline const mstudioanimvalue_t* const pAnimvalue(const int i, const uint16_t* section) const { return (section[i] > 0) ? reinterpret_cast<const mstudioanimvalue_t* const>((char*)section + section[i]) : nullptr; }
	inline const mstudioanimvalue_t* const pAnimvalue(int i) const { return (offset[i] > 0) ? reinterpret_cast<const mstudioanimvalue_t* const>((char*)baseptr + offset[i]) : nullptr; }
};

struct ModelAnimSection_t
{
	ModelAnimSection_t() = default;
	ModelAnimSection_t(const int index, const bool bExternal = false) : animindex(index), isExternal(bExternal) {};

	int animindex;
	bool isExternal;
};

struct ModelAnim_t
{
	ModelAnim_t() = default;
	ModelAnim_t(const ModelAnim_t& animdesc);
	ModelAnim_t(const r2::mstudioanimdesc_t* const animdesc);
	ModelAnim_t(const r5::mstudioanimdesc_v8_t* const animdesc);
	ModelAnim_t(const r5::mstudioanimdesc_v12_1_t* const animdesc, const char* const ext);
	ModelAnim_t(const r5::mstudioanimdesc_v16_t* const animdesc, const char* const ext);
	ModelAnim_t(const r5::mstudioanimdesc_v19_1_t* const animdesc, const char* const ext);
	~ModelAnim_t()
	{
		FreeAllocVar(framemovement);
		FreeAllocArray(ikrules);
		FreeAllocArray(sections);
	}

	ModelAnim_t& operator=(ModelAnim_t&& anim) noexcept
	{
		if (this != &anim)
		{
			memcpy_s(this, sizeof(ModelAnim_t), &anim, sizeof(ModelAnim_t));

			framemovement = anim.framemovement;
			ikrules = anim.ikrules;
			sections = anim.sections;

			anim.framemovement = nullptr;
			anim.ikrules = nullptr;
			anim.sections = nullptr;
		}

		return *this;
	}

	const void* baseptr; // for getting to the animations

	const char* name;
	inline const char* const pszName() const { return IsSeqDeclared() ? name + 1 : name; }

	float fps;
	inline const float Duration() const { return numframes / fps; };

	int flags;

	inline const bool IsOverriden() const { return flags & STUDIO_OVERRIDE; } // gets overwrote by external animation on load
	inline const bool IsSeqDeclared() const { return name[0] == '@'; }
	inline const bool IsNullName() const { return name[0] == '\0'; }

	int numframes;

	int nummovements;
	const mstudiomovement_t* movements; // don't reparse these, the struct never changes!
	ModelFrameMovement_t* framemovement;
	inline const mstudiomovement_t* const pMovement(const int i) const { return nummovements ? movements + i : nullptr; };

	ModelIKRule_t* ikrules;
	int numikrules;
	inline const ModelIKRule_t* const pIKRule(const int i) const { return ikrules + i; }

	// local hierarchy here if we support them

	int animindex;

	// data array, starting with per bone flags
	const char* const pAnimdataNoStall(int* const piFrame, int* const _UNUSED) const;			// v8 - v12
	const char* const pAnimdataStall_0(int* const piFrame, int* const _UNUSED) const;			// v12.1 - v18
	const char* const pAnimdataStall_1(int* const piFrame, int* const sectionFrameCount) const;	// v19
	const char* const pAnimdataStall_2(int* const piFrame, int* const sectionFrameCount) const;	// v19.1 - retail

	int sectionstallframes; // number of static frames inside the animation, the reset excluding the final frame are stored externally. when external data is not loaded(?)/found(?) it falls back on the last frame of this as a stall
	int sectionframes; // number of frames used in each fast lookup section, zero if not used
	int numsections;
	ModelAnimSection_t* sections;
	inline const bool HasStall() const { return sectionstallframes > 0; }
	inline const ModelAnimSection_t* pSection(const int i) const { return sections ? sections + i : nullptr; }

	inline const int SectionCount(const bool useAnimData = false) const
	{
		if (numsections)
		{
			return numsections;
		}

		const int useTrailSection = (flags & eStudioAnimFlags::ANIM_DATAPOINT) ? false : true; // rle anims have an extra section at the end, with only the last frame in full types (Quaternion48, Vector48, etc)
		const int useStallSection = HasStall();

		const int maxFrameIndex = (numframes - sectionstallframes - 1); // subtract to get the last frame index outside of stall

		const int sectionBase = (maxFrameIndex / sectionframes); // basic section index
		const int sectionMaxIndex = sectionBase + useTrailSection + useStallSection; // max index of a section

		// [rika]: where we'd normally add one to get the count, in retail the first section omitted as it's expected to always be at offset 0 in animdata
		// [rika]: only retail animations will have this set, and it is not optional, if not set there is no animation data
		if (useAnimData)
		{
			return sectionMaxIndex;
		}

		return sectionMaxIndex + 1; // add one to make this max index a max count
	}

	// zero frames here if we ever add r1 animation support (not planned)

	const char* sectionDataExtra;
	const char* animData;
	uint64_t animDataAsset;

	size_t parsedBufferIndex;

	inline const float GetCycle(const int iframe) const
	{
		const float cycle = numframes > 1 ? static_cast<float>(iframe) / static_cast<float>(numframes - 1) : 0.0f;
		assertm(isfinite(cycle), "cycle was nan");
		return cycle;
	}
};
typedef const char* const (ModelAnim_t::*AnimdataFunc_t)(int* const, int* const) const;

enum AnimdataFuncType_t : uint8_t
{
	ANIM_FUNC_NOSTALL,
	ANIM_FUNC_STALL_BASEPTR,
	ANIM_FUNC_STALL_ANIMDATA,

	ANIM_FUNC_COUNT,
};

static AnimdataFunc_t s_AnimdataFuncs_RLE[AnimdataFuncType_t::ANIM_FUNC_COUNT] =
{
	&ModelAnim_t::pAnimdataNoStall,
	&ModelAnim_t::pAnimdataStall_0,
	&ModelAnim_t::pAnimdataStall_2,
};

static AnimdataFunc_t s_AnimdataFuncs_DP[AnimdataFuncType_t::ANIM_FUNC_COUNT] =
{
	&ModelAnim_t::pAnimdataNoStall,
	&ModelAnim_t::pAnimdataStall_1,
	&ModelAnim_t::pAnimdataStall_2,
};

struct ModelEvent_t
{
	ModelEvent_t() = default;
	ModelEvent_t(const r2::mstudioevent_t* const pEvent) : name(pEvent->pszEvent()), options(pEvent->options), cycle(pEvent->cycle), unk(0.0f), event(pEvent->event), type(pEvent->type)
	{
		constexpr size_t maxOptionIndex = sizeof(pEvent->options) - 1;
		assertm(pEvent->options[maxOptionIndex] == '\0', "not null terminated");
	}
	ModelEvent_t(const r5::mstudioevent_v8_t* const pEvent) : name(pEvent->pszEvent()), options(pEvent->options), cycle(pEvent->cycle), unk(0.0f), event(pEvent->event), type(pEvent->type)
	{
		constexpr size_t maxOptionIndex = sizeof(pEvent->options) - 1;
		assertm(pEvent->options[maxOptionIndex] == '\0', "not null terminated");
	}
	ModelEvent_t(const r5::mstudioevent_v12_3_t* const pEvent) : name(pEvent->pszEvent()), options(pEvent->options), cycle(pEvent->cycle), unk(0.0f), event(pEvent->event), type(pEvent->type)
	{
		constexpr size_t maxOptionIndex = sizeof(pEvent->options) - 1;
		assertm(pEvent->options[maxOptionIndex] == '\0', "not null terminated");
	}
	ModelEvent_t(const r5::mstudioevent_v16_t* const pEvent) : name(pEvent->pszEvent()), options(pEvent->pszOptions()), cycle(pEvent->cycle), unk(0.0f), event(pEvent->event), type(pEvent->type) {}
	ModelEvent_t(const ModelEvent_t& eventIn) : name(eventIn.name), options(eventIn.options), cycle(eventIn.cycle), unk(eventIn.unk), event(eventIn.event), type(eventIn.type) {}
	~ModelEvent_t()
	{

	}

	ModelEvent_t& operator=(const ModelEvent_t&& event_type) = delete;
	ModelEvent_t& operator=(ModelEvent_t&& event_type) noexcept
	{
		if (this != &event_type)
		{
			memcpy_s(this, sizeof(ModelEvent_t), &event_type, sizeof(ModelEvent_t));
		}

		return *this;
	}

	const char* name; // "event"
	const char* options;

	float cycle;
	float unk; // as of apex v12.3
	int event;
	int type;
};

struct ModelActivityModifier_t
{
	ModelActivityModifier_t() = default;
	ModelActivityModifier_t(const r2::mstudioactivitymodifier_t* const pActivityModifier) : name(pActivityModifier->pszName()), negate(pActivityModifier->negate) {}
	ModelActivityModifier_t(const r5::mstudioactivitymodifier_v8_t* const pActivityModifier) : name(pActivityModifier->pszName()), negate(pActivityModifier->negate) {}
	ModelActivityModifier_t(const r5::mstudioactivitymodifier_v16_t* const pActivityModifier) : name(pActivityModifier->pszName()), negate(pActivityModifier->negate) {}
	ModelActivityModifier_t(const ModelActivityModifier_t& actModIn) : name(actModIn.name), negate(actModIn.negate) {}
	~ModelActivityModifier_t()
	{

	}

	ModelActivityModifier_t& operator=(const ModelActivityModifier_t&& actmod) = delete;
	ModelActivityModifier_t& operator=(ModelActivityModifier_t&& actmod) noexcept
	{
		if (this != &actmod)
		{
			name = actmod.name;
			negate = actmod.negate;
		}

		return *this;
	}

	const char* name;
	bool negate;
};

struct ModelAutoLayer_t
{
public:
	ModelAutoLayer_t() = default;
	ModelAutoLayer_t(const r2::mstudioautolayer_t* const pAutoLayer) : sequence(0ull), iSequence(pAutoLayer->iSequence), iPose(pAutoLayer->iPose), flags(pAutoLayer->flags),
		start(pAutoLayer->start), peak(pAutoLayer->peak), tail(pAutoLayer->tail), end(pAutoLayer->end) { sequence.guid = 0ull;};
	ModelAutoLayer_t(const r5::mstudioautolayer_v8_t* const pAutoLayer) : sequence(pAutoLayer->sequence), iSequence(-1), iPose(static_cast<short>(pAutoLayer->iPose)), flags(pAutoLayer->flags),
		start(pAutoLayer->start), peak(pAutoLayer->peak), tail(pAutoLayer->tail), end(pAutoLayer->end) {};
	~ModelAutoLayer_t() {};

	AssetGuid_t sequence;
	short iSequence;
	short iPose;

	int flags;
	float start;	// beginning of influence
	float peak;		// start of full influence
	float tail;		// end of full influence
	float end;		// end of all influence
};

constexpr int16_t maxSequenceBlends = (5 * 4); // max width and height I have observed
// because apex does not use indices!
constexpr const int16_t defaultSequenceBlends[maxSequenceBlends] = {
	0,	1,	2,	3,	4,
	5,	6,	7,	8,	9,
	10, 11, 12, 13, 14,
	15, 16, 17, 18, 19,
};

struct ModelSeq_t
{
	ModelSeq_t() = default;
	ModelSeq_t(const r2::mstudioseqdesc_t* const seqdesc);
	ModelSeq_t(const r5::mstudioseqdesc_v8_t* const seqdesc);
	ModelSeq_t(const r5::mstudioseqdesc_v8_t* const seqdesc, const char* const ext);
	ModelSeq_t(const r5::mstudioseqdesc_v16_t* const seqdesc, const char* const ext);
	ModelSeq_t(const r5::mstudioseqdesc_v18_t* const seqdesc, const char* const ext, const uint32_t version);
	~ModelSeq_t();

	ModelSeq_t& operator=(ModelSeq_t&& seqdesc) noexcept
	{
		if (this != &seqdesc)
		{
			memcpy_s(this, sizeof(ModelSeq_t), &seqdesc, sizeof(ModelSeq_t));
			memset(&parsedData, 0, sizeof(CRamen)); // so we don't mess things up when moving!

			events = seqdesc.events;
			autolayers = seqdesc.autolayers;
			iklocks = seqdesc.iklocks;
			activitymodifiers = seqdesc.activitymodifiers;
			anims = seqdesc.anims;

			seqdesc.events = nullptr;
			seqdesc.autolayers = nullptr;
			seqdesc.iklocks = nullptr;
			seqdesc.activitymodifiers = nullptr;
			seqdesc.anims = nullptr;

			parsedData.move(seqdesc.parsedData);
		}

		return *this;
	}

	const void* baseptr;

	const char* szlabel;
	const char* szactivityname;

	int flags; // looping/non-looping flags

	inline const bool IsOverriden() const { return flags & STUDIO_OVERRIDE; } // gets overwrote by external animation on load

	int actweight;

	ModelEvent_t* events;
	int numevents;
	inline const ModelEvent_t* const pEvent(const int i) const { return events + i; }

	int numblends;
	const int16_t* blends;

	int groupsize[2];	// width x height of blends
	int paramindex[2];	// X, Y, Z, XR, YR, ZR
	float paramstart[2];// local (0..1) starting value
	float paramend[2];	// local (0..1) ending value

	float fadeintime; // ideal cross fate in time (0.2 default)
	float fadeouttime; // ideal cross fade out time (0.2 default)

	int localentrynode; // transition node at entry
	int localexitnode; // transition node at exit
	int nodeflags; // transition rules

	float entryphase; // used to match entry gait
	float exitphase; // used to match exit gait

	float lastframe; // frame that should generation EndOfSequence

	int nextseq; // auto advancing sequences
	int pose; // index of delta animation between end and nextseq

	ModelAutoLayer_t* autolayers;
	int numautolayers;
	inline const ModelAutoLayer_t* const pAutoLayer(const int i) const { return autolayers + i; }

	const float* weights;
	inline const float weight(const int i) const { return weights[i]; }

	const float* posekeys;
	const char* keyvalues;

	ModelIKLock_t* iklocks;
	int numiklocks;
	const ModelIKLock_t* const pIKLock(const int i) const;

	int cycleposeindex;

	ModelActivityModifier_t* activitymodifiers;
	int numactivitymodifiers;
	inline const ModelActivityModifier_t* const pActivityModifier(const int i) const { return activitymodifiers + i; }

	int ikResetMask;

	ModelAnim_t* anims;
	CRamen parsedData;

	inline const ModelAnim_t* const Anim(const int i) const { return anims + i; }
	inline const int AnimCount() const { return numblends; }
};

void ParseSequence(ModelSeq_t* const seqdesc, const std::vector<ModelBone_t>* const bones, const r2::studiohdr_t* const pStudioHdr);
void ParseSequence(ModelSeq_t* const seqdesc, const std::vector<ModelBone_t>* const bones, const AnimdataFuncType_t funcType, const uint32_t flagWidth = ANIM_BONEFLAG_BITS_4);

// [rika]: this is for model internal sequence data (r5)
void ParseModelSequenceData_NoStall(ModelParsedData_t* const parsedData, char* const baseptr);
void ParseModelSequenceData_Stall_V8(ModelParsedData_t* const parsedData, char* const baseptr);
void ParseModelSequenceData_Stall_V16(ModelParsedData_t* const parsedData, char* const baseptr);
void ParseModelSequenceData_Stall_V18(ModelParsedData_t* const parsedData, char* const baseptr);
void ParseModelSequenceData_Stall_V19_1(ModelParsedData_t* const parsedData, char* const baseptr, const uint32_t flagWidth);

namespace r1
{
	bool Studio_AnimPosition(const ModelAnim_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle);
}

namespace r5
{
	// movements
	bool Studio_AnimPosition(const ModelAnim_t* const panim, float flCycle, Vector& vecPos, QAngle& vecAngle);
}
