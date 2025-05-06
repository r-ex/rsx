#pragma once
#include <game/rtech/cpakfile.h>
#include <game/rtech/assets/material.h>

struct  MaterialSnapshotAssetHeader_v1_t
{
    MaterialDXState_t dxStates;

    uint64_t shaderSet; // guid for shaderset

    uint64_t unk_38;
};

class MaterialSnapshotAsset
{
public:
    MaterialSnapshotAsset(const MaterialSnapshotAssetHeader_v1_t* const hdr) : shaderSet(hdr->shaderSet), shaderSetAsset(nullptr), unk_38(hdr->unk_38)
    {
        memcpy_s(&dxStates, sizeof(MaterialDXState_t), &hdr->dxStates, sizeof(MaterialDXState_t));
    }

    MaterialDXState_t dxStates;

    uint64_t shaderSet; // guid for shaderset
    CPakAsset* shaderSetAsset; // ptr to shaderset asset

    uint64_t unk_38;
};