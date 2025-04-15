#pragma once
#include <core/math/vector2d.h>
#include <core/math/vector.h>

#define ANIR_MAX_ELEMENTS 15
#define ANIR_MAX_SEQUENCES 48
#define ANIR_MAX_RECORDED_FRAMES 3000

struct AnimRecordingFrame_s
{
	float currentTime;
	Vector origin;
	QAngle angles;
	int animRecordingButtons;
	int animSequenceIndex;
	float animCycle;
	float animPlaybackRate;
	int overlayIndex;
	int activeAnimOverlayCount;
	__int16 field_34;
	__int16 field_36;
	int field_38;
	int field_3C;
	__int16 field_40;
	char field_42;
	char field_43;
};

struct AnimRecordingOverlay_s
{
	int animSequenceIndex;
	float animOverlayCycle;
	float overlays[6];
	int slot;
};

struct AnimRecordingAssetHeader_v0_t
{
	// For both of these arrays, there is room for 15 elements, but the code that writes them stops after 12
	// See 0x140DD9360
	// Both of these take their values from mstudioposeparamdesc_t
	char* poseParamNames[ANIR_MAX_ELEMENTS];
	Vector2D poseParamValues[ANIR_MAX_ELEMENTS]; // { startValue, endValue - startValue }
	
	char* animSequences[ANIR_MAX_SEQUENCES];
	char* unkString[16];

	Vector startPos;
	Vector startAngles;

	AnimRecordingFrame_s* recordedFrames; // allocated as 204000 bytes by CPlayer::Script_StartRecordingAnimation
	AnimRecordingOverlay_s* recordedOverlays; // allocated as 108000 bytes by CPlayer::Script_StartRecordingAnimation

	int numRecordedFrames; // count for recordedFrames
	int numRecordedOverlays; // count for recordedOverlays

	int runtimeSlotIndex;
	int slotIndexSign; // -1 if runtimeSlotIndex is negative, does idx >> 31 in code at [r5apex_ds + 0xCBDC60].

	int animRecordingId;

	bool isPersistent;
	short runtimeRefCounter;
};

#define ANIR_FILE_MAGIC ('R'<<24 | 'I'<<16 | 'N'<<8 | 'A')
#define ANIR_FILE_VERSION 1 // increment this if the file format changes.

#pragma pack(push, 1)
struct AnimRecordingFileHeader_s
{
	int magic;
	unsigned short fileVersion;
	unsigned short assetVersion;

	Vector startPos;
	Vector startAngles;

	int stringBufSize;

	int numElements;
	int numSequences;

	int numRecordedFrames;
	int numRecordedOverlays;

	int animRecordingId;
};
#pragma pack(pop)

static_assert(sizeof(AnimRecordingAssetHeader_v0_t) == 0x330);
