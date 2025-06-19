#pragma once
#include <game/rtech/cpakfile.h>

struct ParticleScriptAssetHeader_v8_t
{
    uint64_t textures[2];

    // texture input slots ?
    char unk_10[16];

    uint64_t unk_20[4];

    char* name;

    void* uberStaticBuffer;

    uint64_t systemEffectComputeShader;
    uint64_t processComputeShader;
    uint64_t renderVertexShader;
    uint64_t renderGeometryShader;
    uint64_t renderPixelShader;

    uint32_t unk_78;

    uint32_t uberStaticBufferSize;

    char unk_80[8]; // last two null and never referenced in update func (whereas first six are)
};

class ParticleScriptAsset
{
public:
    ParticleScriptAsset(const ParticleScriptAssetHeader_v8_t* const hdr) : name(hdr->name), systemEffectComputeShader(hdr->systemEffectComputeShader), processComputeShader(hdr->processComputeShader), renderVertexShader(hdr->renderVertexShader), renderGeometryShader(hdr->renderGeometryShader),
        renderPixelShader(hdr->renderPixelShader), uberStaticBuffer(hdr->uberStaticBuffer), uberStaticBufferSize(hdr->uberStaticBufferSize)
    {
        memcpy_s(textures, sizeof(textures), hdr->textures, sizeof(textures));
    }

    char* name;

    uint64_t textures[2];

    uint64_t systemEffectComputeShader;
    uint64_t processComputeShader;
    uint64_t renderVertexShader;
    uint64_t renderGeometryShader;
    uint64_t renderPixelShader;

    void* uberStaticBuffer;
    uint32_t uberStaticBufferSize;
};