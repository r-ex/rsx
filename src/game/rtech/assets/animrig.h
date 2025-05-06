#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/assets/model.h>

struct AnimRigAssetHeader_v4_t
{
	void* data; // ptr to studiohdr & rrig buffer

	char* name;

	int unk_10;

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	int64_t unk_20; // probably reserved
};
static_assert(sizeof(AnimRigAssetHeader_v4_t) == 0x28);

struct AnimRigAssetHeader_v5_t
{
	void* data; // ptr to studiohdr & rrig buffer

	char* name;

	short unk_10;

	short numAnimSeqs;

	int unk_14; // 

	AssetGuid_t* animSeqs;

	int64_t unk_20; // probably reserved
};
static_assert(sizeof(AnimRigAssetHeader_v5_t) == 0x28);

// [rika]: if we use ModelParsedData_t we could reuse model exporting funcs
class AnimRigAsset
{
public:
	AnimRigAsset() = default;
	AnimRigAsset(AnimRigAssetHeader_v4_t* hdr) : data(hdr->data), name(hdr->name), numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), dataSize(reinterpret_cast<r5::studiohdr_v8_t*>(hdr->data)->length) {};
	AnimRigAsset(AnimRigAssetHeader_v5_t* hdr) : data(hdr->data), name(hdr->name), numAnimSeqs(hdr->numAnimSeqs), animSeqs(hdr->animSeqs), dataSize(-1) {};

	void* data; // ptr to studiohdr & rrig buffer
	char* name;

	int dataSize; // size of studio data

	int numAnimSeqs;
	AssetGuid_t* animSeqs;

	std::vector<ModelBone_t> bones;

	studiohdr_generic_t studiohdr;

	eMDLVersion studioVersion;
};