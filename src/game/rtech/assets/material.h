#pragma once
#include <d3d11.h>
#include <game/rtech/cpakfile.h>
#include <game/rtech/utils/utils.h>
#include <game/rtech/assets/shaderset.h>

/////////////////////////////////////////////
// D3D11_DEPTH_STENCIL_DESC creation flags //
/////////////////////////////////////////////

// DepthEnable
#define DF_DEPTH_ENABLE 0x1

// DepthFunc
#define DF_COMPARISON_NEVER   0b0000
#define DF_COMPARISON_LESS    0b0010
#define DF_COMPARISON_EQUAL   0b0100
#define DF_COMPARISON_GREATER 0b1000

#define DF_COMPARISON_LESS_EQUAL (DF_COMPARISON_EQUAL   | DF_COMPARISON_LESS )    // 0b0110
#define DF_COMPARISON_NOT_EQUAL  (DF_COMPARISON_GREATER | DF_COMPARISON_LESS )    // 0b1010
#define DF_COMPARISON_GREATER_EQUAL (DF_COMPARISON_GREATER | DF_COMPARISON_EQUAL) // 0b1100
#define DF_COMPARISON_ALWAYS (DF_COMPARISON_GREATER | DF_COMPARISON_EQUAL | DF_COMPARISON_LESS) // 0b1110

// DepthWriteMask
#define DF_DEPTH_WRITE_MASK_ALL  0b10000

// StencilEnable
#define DF_STENCIL_ENABLE (1 << 7)


// D3D11_STENCIL_OP
//desc.FrontFace.StencilFailOp = ((visFlags >> 8) & 7) + 1;
//desc.FrontFace.StencilDepthFailOp = ((visFlags >> 11) & 7) + 1;
//desc.FrontFace.StencilPassOp = ((visFlags >> 14) & 7) + 1;



//////////////////////////////////////////
// D3D11_RASTERIZER_DESC creation flags //
//////////////////////////////////////////
// https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_rasterizer_desc


#define RF_FILL_WIREFRAME 0x1

#define RF_CULL_NONE  0b0010
#define RF_CULL_FRONT 0b0100
#define RF_CULL_BACK (RF_CULL_FRONT | RF_CULL_NONE)

// If this parameter is TRUE, a triangle will be considered front-facing if its vertices are counter-clockwise on the render target
// and considered back-facing if they are clockwise.
// If this parameter is FALSE, the opposite is true.
#define RF_INVERT_FACE_DIR 0b1000 // sets FrontCounterClockwise

// used to define values of DepthBias, DepthBiasClamp, SlopeScaledDepthBias
#define RF_PRESET_DECAL     0x10
#define RF_PRESET_SHADOWMAP 0x20
#define RF_PRESET_TIGHTSHADOWMAP 0x40
#define RF_PRESET_UI        0x50
#define RF_PRESET_ZFILL     0x60

// Enable scissor-rectangle culling. All pixels outside an active scissor rectangle are culled.
#define RF_SCISSOR_ENABLE 0x80

// pak data
struct MaterialBlendState_t
{
	__int32 unk : 1;
	__int32 blendEnable : 1;
	__int32 srcBlend : 5;
	__int32 destBlend : 5;
	__int32 blendOp : 3;
	__int32 srcBlendAlpha : 5;
	__int32 destBlendAlpha : 5;
	__int32 blendOpAlpha : 3;
	__int32 renderTargetWriteMask : 4;

	MaterialBlendState_t() = default;

	MaterialBlendState_t(bool blendEnable, __int8 renderTargetWriteMask)
	{
		UNUSED(blendEnable);
		memset(this, 0, sizeof(MaterialBlendState_t));
		this->renderTargetWriteMask = renderTargetWriteMask & 0xF;
	}

	MaterialBlendState_t(bool blendEnable,
		D3D11_BLEND srcBlend, D3D11_BLEND destBlend,
		D3D11_BLEND_OP blendOp, D3D11_BLEND srcBlendAlpha,
		D3D11_BLEND destBlendAlpha, D3D11_BLEND_OP blendOpAlpha,
		__int8 renderTargetWriteMask)
	{
		this->blendEnable = blendEnable ? 1 : 0;
		this->srcBlend = srcBlend - 1;
		this->destBlend = destBlend - 1;
		this->blendOp = blendOp - 1;
		this->srcBlendAlpha = srcBlendAlpha - 1;
		this->destBlendAlpha = destBlendAlpha - 1;
		this->blendOpAlpha = blendOpAlpha - 1;

		this->renderTargetWriteMask = renderTargetWriteMask & 0xF;
	}

	uint32_t Get() const
	{
		return *(uint32_t*)this;
	}
};

// Disabled alignment warning.
DISABLE_WARNING(4324);

struct __declspec(align(16)) MaterialDXState_v12_t
{
	// r2 only supports 4 render targets?
	MaterialBlendState_t blendStates[4];

	uint32_t blendStateMask; // [rika]: no reason this should be different than r5

	// flags to determine how the D3D11_DEPTH_STENCIL_DESC is defined for this material
	uint16_t depthStencilFlags;

	// flags to determine how the D3D11_RASTERIZER_DESC is defined
	uint16_t rasterizerFlags;
};
static_assert(sizeof(MaterialDXState_v12_t) == 0x20);
ENABLE_WARNING();

// Disabled alignment warning.
DISABLE_WARNING(4324);

// aligned to 16 bytes so it can be loaded as 3 m128i structs in engine
struct __declspec(align(16)) MaterialDXState_t
{
	MaterialDXState_t() = default;
	MaterialDXState_t(MaterialDXState_v12_t& dxState) : blendStates(), blendStateMask(dxState.blendStateMask), depthStencilFlags(dxState.depthStencilFlags), rasterizerFlags(dxState.rasterizerFlags)
	{
		memcpy_s(blendStates, 4 * sizeof(MaterialBlendState_t), dxState.blendStates, 4 * sizeof(MaterialBlendState_t));
	};

	// bitfield defining a D3D11_RENDER_TARGET_BLEND_DESC for each of the 8 possible DX render targets
	MaterialBlendState_t blendStates[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];

	uint32_t blendStateMask;

	// flags to determine how the D3D11_DEPTH_STENCIL_DESC is defined for this material
	uint16_t depthStencilFlags;

	// flags to determine how the D3D11_RASTERIZER_DESC is defined
	uint16_t rasterizerFlags;

	char unk_28[8]; // [rika]: there is data here in newer versions!
};
static_assert(sizeof(MaterialDXState_t) == 0x30);
ENABLE_WARNING();

enum MaterialShaderType_t : uint8_t
{
	RGDU = 0x0,
	RGDP = 0x1,
	RGDC = 0x2,
	SKNU = 0x3,
	SKNP = 0x4,
	SKNC = 0x5,
	WLDU = 0x6,
	WLDC = 0x7,
	PTCU = 0x8,
	PTCS = 0x9,
	RGBS = 0xA,

	_TYPE_COUNT,

	SKN,
	FIX,
	WLD,
	GEN,
	RGD,
	_TYPE_LEGACY,
	_TYPE_LEGACY_COUNT = _TYPE_LEGACY - SKN,
	
	_TYPE_INVAILD = 0xFF,
};

static const char* s_MaterialShaderTypeSuffixes[] =
{
	"_rgdu",
	"_rgdp",
	"_rgdc",

	"_sknu",
	"_sknp",
	"_sknc",

	"_wldu",
	"_wldc",

	"_ptcu",
	"_ptcs",

	"_rgbs",

	"_INVALID_COUNT",

	"_skn",
	"_fix",
	"_wld",
	"_gen",
	"_rgd",

	"_INVALID_LGCY", // titanfall 2
	"_INVALID_LGCY_COUNT",
};

inline const char* GetMaterialShaderSuffixByType(uint8_t type)
{
	if (type >= _TYPE_LEGACY || type == _TYPE_COUNT)
		return "";

	return s_MaterialShaderTypeSuffixes[type];
}

static const char* s_MaterialShaderTypeNames[] = 
{
	"RGDU",
	"RGDP",
	"RGDC",

	"SKNU",
	"SKNP",
	"SKNC",

	"WLDU",
	"WLDC",

	"PTCU",
	"PTCS",

	"RGBS",

	"_COUNT",

	"SKN",
	"FIX",
	"WLD",
	"GEN",
	"RGD",

	"LEGACY", // titanfall 2
	"_LEGACY_COUNT"
};

static const char* s_MaterialShaderTypeHelpText = ""
"This type represents the way that the material is used and rendered by the game.\n\n"
"Possible types:\n"
" - RGDU: Static Props with regular vertices\n"
" - RGDP: Static Props with packed vertex positions\n"
" - RGDC: Static Props with packed vertex positions\n\n"

" - SKNU: Skinned model with regular vertices\n"
" - SKNP: Skinned model with packed vertex positions\n"
" - SKNC: Skinned model with packed vertex positions\n\n"

" - WLDU: World geometry with regular vertices\n"
" - WLDC: World geometry with packed vertex positions\n\n"

" - PTCU: Particles with regular vertices\n"
" - PTCS: Particles\n\n"

" - RGBS: Currently Unknown\n\n"

"Titanfall types:\n"
" - SKN: Skinned model\n"
" - FIX: Skinned model fixup for use as a static prop\n"
" - WLD: World geometry\n"
" - GEN: UI, general use, particles, etc\n";

struct MaterialAssetCPU_t
{
	void* data;
	int dataSize;
	int unk_C; // varies between shaders?
};

struct __declspec(align(16)) MaterialAssetHeader_v12_t
{
	uint64_t vftableReserved; // Gets set to CMaterialGlue vtbl ptr
	char gap_8[0x8]; // unused?
	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.txt)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t colpassMaterial;

	MaterialDXState_v12_t dxStates[2];

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	void* textureHandles; // TextureGUID Map
	void* streamingTextureHandles; // Streamable TextureGUID Map
	short numStreamingTextureHandles; // Number of textures with streamed mip levels.

	char samplers[4]; // 0x503000

	short unk_AE;
	uint64_t unk_B0; // haven't observed anything here.

	// seems to be 0xFBA63181 for loadscreens
	uint32_t unk_B8; // no clue tbh, 0xFBA63181

	uint32_t unk_BC; // this might actually be "Alignment"

	uint32_t glueFlags;
	uint32_t glueFlags2;

	short width;
	short height;
	short depth;
};
static_assert(sizeof(MaterialAssetHeader_v12_t) == 208);

struct __declspec(align(16)) MaterialAssetHeader_v15_t
{
	uint64_t vftableReserved; // reserved for virtual function table pointer (when copied into native CMaterialGlue)

	char gap_8[0x8]; // unused?
	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.rson)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t depthShadowTightMaterial;
	uint64_t colpassMaterial;

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	void* textureHandles; // ptr to array of texture guids
	void* streamingTextureHandles; // ptr to array of streamable texture guids (empty at build time)

	short numStreamingTextureHandles; // number of textures with streamed mip levels.
	short width;
	short height;
	short depth;

	// array of indices into sampler states array. must be set properly to have accurate texture tiling
	// used in CShaderGlue::SetupShader (1403B3C60)
	char samplers[4];// = 0x1D0300;

	uint32_t unk_7C;

	uint32_t unk_80;// = 0x1F5A92BD; // REQUIRED but why?

	uint32_t unk_84;

	uint32_t glueFlags;
	uint32_t glueFlags2;

	MaterialDXState_t dxStates[2]; // seems to be used for setting up some D3D states?

	uint16_t numAnimationFrames; // used in CMaterialGlue::GetNumAnimationFrames (0x1403B4250), which is called from GetSpriteInfo @ 0x1402561FC
	MaterialShaderType_t materialType;
	uint8_t uberBufferFlags; // used for unksections loading in UpdateMaterialAsset

	char pad_00F4[0x4];

	uint64_t textureAnimation;
};
static_assert(sizeof(MaterialAssetHeader_v15_t) == 256);

// [rika]: I think we're on version 23 of matl now and this still works. v22 and v23 are 256 bytes, everything else is below is the same?
struct MaterialAssetHeader_v16_t
{
	uint64_t vftableReserved; // reserved for virtual function table pointer (when copied into native CMaterialGlue)

	char gap_8[0x8]; // unused?
	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.rson)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t depthShadowTightMaterial;
	uint64_t colpassMaterial;

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	void* textureHandles; // ptr to array of texture guids
	void* streamingTextureHandles; // ptr to array of streamable texture guids (empty at build time)

	short numStreamingTextureHandles; // number of textures with streamed mip levels.
	short width;
	short height;
	short depth;

	// array of indices into sampler states array. must be set properly to have accurate texture tiling
	// used in CShaderGlue::SetupShader (1403B3C60)
	char samplers[4];// = 0x1D0300;

	uint32_t unk_7C;

	uint32_t unk_80;// = 0x1F5A92BD; // REQUIRED but why?

	uint32_t unk_84;

	uint32_t glueFlags;
	uint32_t glueFlags2;

	MaterialDXState_t dxStates; // seems to be used for setting up some D3D states?

	int unk_C0[2];

	uint16_t numAnimationFrames; // used in CMaterialGlue::GetNumAnimationFrames (0x1403B4250), which is called from GetSpriteInfo @ 0x1402561FC
	MaterialShaderType_t materialType;
	uint8_t uberBufferFlags; // used for unksections loading in UpdateMaterialAsset

	int unk_CC;

	uint64_t textureAnimation;

	int unk_D8[4];

	float unk_E8;

	int unk_EA;
};
static_assert(sizeof(MaterialAssetHeader_v16_t) == 240);

struct MaterialAssetHeader_v22_t
{
	uint64_t vftableReserved; // reserved for virtual function table pointer (when copied into native CMaterialGlue)

	char gap_8[0x8]; // unused?
	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.rson)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t depthShadowTightMaterial;
	uint64_t colpassMaterial;

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	void* textureHandles; // ptr to array of texture guids
	void* streamingTextureHandles; // ptr to array of streamable texture guids (empty at build time)

	short numStreamingTextureHandles; // number of textures with streamed mip levels.
	short width;
	short height;
	short depth;

	// array of indices into sampler states array. must be set properly to have accurate texture tiling
	// used in CShaderGlue::SetupShader (1403B3C60)
	char samplers[4];// = 0x1D0300;

	uint32_t unk_7C;

	uint32_t unk_80;// = 0x1F5A92BD; // REQUIRED but why?

	uint32_t unk_84;

	uint32_t glueFlags;
	uint32_t glueFlags2;

	MaterialDXState_t dxStates; // seems to be used for setting up some D3D states?

	int unk_C0[2];

	uint16_t numAnimationFrames; // used in CMaterialGlue::GetNumAnimationFrames (0x1403B4250), which is called from GetSpriteInfo @ 0x1402561FC
	MaterialShaderType_t materialType;
	uint8_t uberBufferFlags; // used for unksections loading in UpdateMaterialAsset

	int unk_CC;

	uint64_t textureAnimation;

	int unk_D8[4];

	float unk_E8;

	int unk_EA;

	char unk_F0[16];
};
static_assert(sizeof(MaterialAssetHeader_v22_t) == 256);

struct MaterialAssetHeader_v23_1_t
{
	uint64_t unk_0; // same as previous version (as of v23), does not seem to be vtftableReserved (contains data)

	uint64_t snapshotMaterial; // 'msnp' asset

	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.rson)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t depthShadowTightMaterial;
	uint64_t colpassMaterial;

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	void* textureHandles; // ptr to array of texture guids
	void* streamingTextureHandles; // ptr to array of streamable texture guids (empty at build time)

	short numStreamingTextureHandles; // number of textures with streamed mip levels.
	short width;
	short height;
	short depth;

	// array of indices into sampler states array. must be set properly to have accurate texture tiling
	// used in CShaderGlue::SetupShader (1403B3C60)
	char samplers[4];// = 0x1D0300;

	uint32_t unk_7C;

	uint32_t unk_80;// = 0x1F5A92BD; // REQUIRED but why?


	uint32_t glueFlags;
	uint32_t glueFlags2;

	char unk_8C[16];

	uint16_t numAnimationFrames; // used in CMaterialGlue::GetNumAnimationFrames (0x1403B4250), which is called from GetSpriteInfo @ 0x1402561FC
	MaterialShaderType_t materialType;
	uint8_t uberBufferFlags; // used for unksections loading in UpdateMaterialAsset

	int unk_A0; // unk_CC?

	float unk_A4; // unk_E8?

	uint64_t textureAnimation;

	char unk_B0[8]; // unk_F0 ?

	MaterialAssetCPU_t* cpuDataPtr;
};
static_assert(sizeof(MaterialAssetHeader_v23_1_t) == 192);

struct TextureAssetEntry_t
{
	TextureAssetEntry_t(CPakAsset* assetIn, const uint32_t indexIn) : asset(assetIn), index(indexIn) {};

	CPakAsset* asset;
	const uint32_t index; // index is the position within the guid array.
};

// "flags2" - first set of flags
#define MATERIAL_HAS_EXTRA_BONE_WEIGHTS          (1 <<  0xA) // see 1404521CF
#define MATERIAL_EMISSIVE_USES_VIDEO             (1 <<  0xB) // bink!
#define MATERIAL_PARTICLE_LIGHTING               (1 <<  0xF) // binds PS resource: Texture2D<float4> lightingTexture : register(t75);
#define MATERIAL_NEEDS_POWER_OF_TWO_FRAME_BUFFER (1 << 0x10) // binds PS resource: Texture2D<float4> refractTexture : register(t0);
#define MATERIAL_PARTICLE_DEPTH                  (1 << 0x11) // binds PS resource: Texture2D<float4> depthTexture : register(t73)
#define MATERIAL_COCKPIT_SCREEN                  (1 << 0x13)
#define MATERIAL_USES_LIGHTMAP_TEXTURE           (1 << 0x1D)
#define MATERIAL_USES_ENV_CUBEMAP                (1 << 0x1E)

// material compile flags
#define MATERIALCF_COMPILE_NO_DRAW               (1 << 0)  // 1
#define MATERIALCF_COMPILE_PASS_BULLETS          (1 << 1)  // 2
#define MATERIALCF_COMPILE_DETAIL                (1 << 2)  // 4
#define MATERIALCF_COMPILE_NO_SHADOWS            (1 << 3)  // 8
#define MATERIALCF_NO_TOOL_TEXTURE               (1 << 4)  // 0x10
#define MATERIALCF_COMPILE_NO_LIGHT              (1 << 5)  // 0x20
#define MATERIALCF_COMPILE_NON_SOLID             (1 << 6)  // 0x40
#define MATERIALCF_COMPILE_TRIGGER               (1 << 7)  // 0x80
#define MATERIALCF_COMPILE_CLIP                  (1 << 9)  // 0x200
#define MATERIALCF_OPERATOR_FLOOR                (1 << 10) // 0x400
#define MATERIALCF_COMPILE_SKIP                  (1 << 11) // 0x800
#define MATERIALCF_COMPILE_SKY                   (1 << 12) // 0x1000
#define MATERIALCF_NO_CLIMB                      (1 << 13) // 0x2000
#define MATERIALCF_COMPILE_NPC_CLIP              (1 << 14) // 0x4000
#define MATERIALCF_PLAYER_CLIP                   (1 << 15) // 0x8000
#define MATERIALCF_PLAYER_CLIP_HUMAN             (1 << 16) // 0x10000
#define MATERIALCF_PLAYER_CLIP_TITAN             (1 << 17) // 0x20000
#define MATERIALCF_OCCLUDE_SOUND                 (1 << 18) // 0x40000
#define MATERIALCF_COMPILE_WATER                 (1 << 19) // 0x80000
#define MATERIALCF_NO_GRAPPLE                    (1 << 22) // 0x400000
#define MATERIALCF_COMPILE_UNK                   (1 << 25) // 0x2000000
#define MATERIALCF_SOUND_TRIGGER                 (1 << 26) // 0x4000000
#define MATERIALCF_NO_AIRDROP                    (1 << 27) // 0x8000000

class MaterialAsset
{
public:
	// [rika]: didn't test these so have fun
	MaterialAsset(MaterialAssetHeader_v12_t* const hdr, MaterialAssetCPU_t* const cpu) : snapshotMaterial(0ull), guid(hdr->guid), name(hdr->name), surfaceProp(hdr->surfaceProp), surfaceProp2(hdr->surfaceProp2),
		depthShadowMaterial(hdr->depthShadowMaterial), depthPrepassMaterial(hdr->depthPrepassMaterial), depthVSMMaterial(hdr->depthVSMMaterial), depthShadowTightMaterial(0ull/*[amos]: v12 doesn't have this!*/), colpassMaterial(hdr->colpassMaterial), shaderSet(hdr->shaderSet),
		numAnimationFrames(0), textureAnimation(0), textureHandles(hdr->textureHandles), streamingTextureHandles(hdr->streamingTextureHandles),
		width(hdr->width), height(hdr->height), depth(0), unk(hdr->unk_B8), glueFlags(hdr->glueFlags), glueFlags2(hdr->glueFlags2), materialType(MaterialShaderType_t::_TYPE_LEGACY), shaderSetAsset(nullptr), snapshotAsset(nullptr),
		cpuData(cpu->data), cpuDataSize(cpu->dataSize)
	{
		memcpy_s(samplers, sizeof(samplers), hdr->samplers, sizeof(hdr->samplers));

		for (int i = 0; i < 2; i++)
		{
			dxStates[i] = MaterialDXState_t(hdr->dxStates[i]);
		}

		numRenderTargets = 4;
	};

	MaterialAsset(MaterialAssetHeader_v15_t* const hdr, MaterialAssetCPU_t* const cpu) : snapshotMaterial(0ull), guid(hdr->guid), name(hdr->name), surfaceProp(hdr->surfaceProp), surfaceProp2(hdr->surfaceProp2),
		depthShadowMaterial(hdr->depthShadowMaterial), depthPrepassMaterial(hdr->depthPrepassMaterial), depthVSMMaterial(hdr->depthVSMMaterial), depthShadowTightMaterial(hdr->depthShadowTightMaterial), colpassMaterial(hdr->colpassMaterial), shaderSet(hdr->shaderSet),
		numAnimationFrames(hdr->numAnimationFrames), textureAnimation(hdr->textureAnimation), textureHandles(hdr->textureHandles), streamingTextureHandles(hdr->streamingTextureHandles),
		width(hdr->width), height(hdr->height), depth(hdr->depth), unk(hdr->unk_80), glueFlags(hdr->glueFlags), glueFlags2(hdr->glueFlags2), materialType(hdr->materialType), uberBufferFlags(hdr->uberBufferFlags), shaderSetAsset(nullptr), snapshotAsset(nullptr),
		cpuData(cpu->data), cpuDataSize(cpu->dataSize)
	{
		memcpy_s(samplers, sizeof(samplers), hdr->samplers, sizeof(hdr->samplers));
		memcpy_s(dxStates, sizeof(dxStates), hdr->dxStates, sizeof(hdr->dxStates));

		numRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	};

	MaterialAsset(MaterialAssetHeader_v16_t* const hdr, MaterialAssetCPU_t* const cpu) : snapshotMaterial(0ull), guid(hdr->guid), name(hdr->name), surfaceProp(hdr->surfaceProp), surfaceProp2(hdr->surfaceProp2),
		depthShadowMaterial(hdr->depthShadowMaterial), depthPrepassMaterial(hdr->depthPrepassMaterial), depthVSMMaterial(hdr->depthVSMMaterial), depthShadowTightMaterial(hdr->depthShadowTightMaterial), colpassMaterial(hdr->colpassMaterial), shaderSet(hdr->shaderSet),
		numAnimationFrames(hdr->numAnimationFrames), textureAnimation(hdr->textureAnimation), textureHandles(hdr->textureHandles), streamingTextureHandles(hdr->streamingTextureHandles),
		width(hdr->width), height(hdr->height), depth(hdr->depth), unk(hdr->unk_80), glueFlags(hdr->glueFlags), glueFlags2(hdr->glueFlags2), materialType(hdr->materialType), uberBufferFlags(hdr->uberBufferFlags), shaderSetAsset(nullptr), snapshotAsset(nullptr),
		cpuData(cpu->data), cpuDataSize(cpu->dataSize)
	{
		memcpy_s(samplers, sizeof(samplers), hdr->samplers, sizeof(hdr->samplers));
		memcpy_s(dxStates, sizeof(dxStates), &hdr->dxStates, sizeof(hdr->dxStates));

		numRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	};

	MaterialAsset(MaterialAssetHeader_v22_t* const hdr, MaterialAssetCPU_t* const cpu) : snapshotMaterial(0ull), guid(hdr->guid), name(hdr->name), surfaceProp(hdr->surfaceProp), surfaceProp2(hdr->surfaceProp2),
		depthShadowMaterial(hdr->depthShadowMaterial), depthPrepassMaterial(hdr->depthPrepassMaterial), depthVSMMaterial(hdr->depthVSMMaterial), depthShadowTightMaterial(hdr->depthShadowTightMaterial), colpassMaterial(hdr->colpassMaterial), shaderSet(hdr->shaderSet),
		numAnimationFrames(hdr->numAnimationFrames), textureAnimation(hdr->textureAnimation), textureHandles(hdr->textureHandles), streamingTextureHandles(hdr->streamingTextureHandles),
		width(hdr->width), height(hdr->height), depth(hdr->depth), unk(hdr->unk_80), glueFlags(hdr->glueFlags), glueFlags2(hdr->glueFlags2), materialType(hdr->materialType), uberBufferFlags(hdr->uberBufferFlags), shaderSetAsset(nullptr), snapshotAsset(nullptr),
		cpuData(cpu->data), cpuDataSize(cpu->dataSize)
	{
		memcpy_s(samplers, sizeof(samplers), hdr->samplers, sizeof(hdr->samplers));
		memcpy_s(dxStates, sizeof(dxStates), &hdr->dxStates, sizeof(hdr->dxStates));

		numRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	};

	MaterialAsset(MaterialAssetHeader_v23_1_t* const hdr, MaterialAssetCPU_t* const cpu) : snapshotMaterial(hdr->snapshotMaterial), guid(hdr->guid), name(hdr->name), surfaceProp(hdr->surfaceProp), surfaceProp2(hdr->surfaceProp2),
		depthShadowMaterial(hdr->depthShadowMaterial), depthPrepassMaterial(hdr->depthPrepassMaterial), depthVSMMaterial(hdr->depthVSMMaterial), depthShadowTightMaterial(hdr->depthShadowTightMaterial), colpassMaterial(hdr->colpassMaterial), shaderSet(hdr->shaderSet),
		numAnimationFrames(hdr->numAnimationFrames), textureAnimation(hdr->textureAnimation), textureHandles(hdr->textureHandles), streamingTextureHandles(hdr->streamingTextureHandles),
		width(hdr->width), height(hdr->height), depth(hdr->depth), unk(hdr->unk_80), glueFlags(hdr->glueFlags), glueFlags2(hdr->glueFlags2), materialType(hdr->materialType), uberBufferFlags(hdr->uberBufferFlags), shaderSetAsset(nullptr), snapshotAsset(nullptr),
		cpuData(cpu->data), cpuDataSize(cpu->dataSize),
		dxStates()
	{
		memcpy_s(samplers, sizeof(samplers), hdr->samplers, sizeof(hdr->samplers));

		numRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	};

	uint64_t snapshotMaterial;

	uint64_t guid; // guid of this material asset

	char* name; // pointer to partial asset path
	char* surfaceProp; // pointer to surfaceprop (as defined in surfaceproperties.rson)
	char* surfaceProp2; // pointer to surfaceprop2 

	uint64_t depthShadowMaterial;
	uint64_t depthPrepassMaterial;
	uint64_t depthVSMMaterial;
	uint64_t depthShadowTightMaterial;
	uint64_t colpassMaterial;

	uint64_t shaderSet; // guid of the shaderset asset that this material uses

	uint16_t numAnimationFrames;
	uint64_t textureAnimation;

	void* textureHandles; // ptr to array of texture guids
	void* streamingTextureHandles; // ptr to array of streamable texture guids (empty at build time)

	short width;
	short height;
	short depth;

	uint32_t unk; // 0x1F5A92BD, REQUIRED but why?

	char samplers[4];

	uint32_t glueFlags;
	uint32_t glueFlags2;

	// Disabled alignment warning.
	DISABLE_WARNING(4324);
	MaterialDXState_t dxStates[2]; // seems to be used for setting up some D3D states?
	ENABLE_WARNING();

	MaterialShaderType_t materialType;
	uint8_t uberBufferFlags;
	//

	ID3D11Buffer* uberStaticBuffer;
	void* cpuData;
	int cpuDataSize;
	std::vector<TmpConstBufVar> cpuDataBuf; // should this be done on export?

	CPakAsset* shaderSetAsset;
	CPakAsset* snapshotAsset;
	std::vector<TextureAssetEntry_t> txtrAssets;
	std::map<uint32_t, ShaderResource> resourceBindings; // this is how we get suffixes, could have some reliabity issues

	uint8_t numRenderTargets;

	// parse snapshot
	void ParseSnapshot();
};

void MatPreview_DXState(const MaterialDXState_t& dxState, const uint8_t dxStateId, const uint8_t numRenderTargets);

struct MaterialTextureExportInfo_s
{
	MaterialTextureExportInfo_s() : pTexture(nullptr), isNormal(false) {}
	MaterialTextureExportInfo_s(const std::filesystem::path& path, CPakAsset* const asset) : pTexture(asset), exportPath(path), isNormal(false) {}

	CPakAsset* pTexture;
	std::string exportName;
	std::filesystem::path exportPath;

	bool isNormal;
};

void ParseMaterialTextureExportInfo(std::unordered_map<uint32_t, MaterialTextureExportInfo_s>& textures, const MaterialAsset* materialAsset, const std::filesystem::path& exportPath, const eTextureExportName nameSetting, const bool useFullPaths);
void ExportMaterialTextures(const int setting, const MaterialAsset* materialAsset, const std::unordered_map<uint32_t, MaterialTextureExportInfo_s>& textureInfo);