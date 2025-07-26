#pragma once

#include <d3d11.h>
#include <game/rtech/utils/utils.h>

inline bool CreateD3DBuffer(
    ID3D11Device* device, ID3D11Buffer** pBuffer,
    UINT byteWidth, D3D11_USAGE usage,
    D3D11_BIND_FLAG bindFlags, int cpuAccessFlags,
    UINT miscFlags, UINT structureByteStride = 0, void* initialData = nullptr)
{
    assert(pBuffer);

    D3D11_BUFFER_DESC desc = {};

    desc.Usage = usage;
    desc.ByteWidth = bindFlags & D3D11_BIND_CONSTANT_BUFFER ? IALIGN(byteWidth, 16) : byteWidth;
    desc.BindFlags = bindFlags;
    desc.CPUAccessFlags = cpuAccessFlags;
    desc.MiscFlags = miscFlags;
    desc.StructureByteStride = structureByteStride;

    HRESULT hr;
    if (initialData)
    {
        D3D11_SUBRESOURCE_DATA resource = { initialData };
        hr = device->CreateBuffer(&desc, &resource, pBuffer);
    }
    else
    {
        hr = device->CreateBuffer(&desc, NULL, pBuffer);
    }

    assert(SUCCEEDED(hr));

    return SUCCEEDED(hr);
}

static const char* D3D11_BLEND_NAMES[] = {
    "D3D11_BLEND_INVALID",
    "D3D11_BLEND_ZERO",
    "D3D11_BLEND_ONE",
    "D3D11_BLEND_SRC_COLOR",
    "D3D11_BLEND_INV_SRC_COLOR",
    "D3D11_BLEND_SRC_ALPHA",
    "D3D11_BLEND_INV_SRC_ALPHA",
    "D3D11_BLEND_DEST_ALPHA",
    "D3D11_BLEND_INV_DEST_ALPHA",
    "D3D11_BLEND_DEST_COLOR",
    "D3D11_BLEND_INV_DEST_COLOR",
    "D3D11_BLEND_SRC_ALPHA_SAT",
    "D3D11_BLEND_BLEND_FACTOR",
    "D3D11_BLEND_INV_BLEND_FACTOR",
    "D3D11_BLEND_SRC1_COLOR",
    "D3D11_BLEND_INV_SRC1_COLOR",
    "D3D11_BLEND_SRC1_ALPHA",
    "D3D11_BLEND_INV_SRC1_ALPHA",
};

static const char* D3D11_BLEND_OP_NAMES[] = {
    "D3D11_BLEND_OP_INVALID",
    "D3D11_BLEND_OP_ADD",
    "D3D11_BLEND_OP_SUBTRACT",
    "D3D11_BLEND_OP_REV_SUBTRACT",
    "D3D11_BLEND_OP_MIN",
    "D3D11_BLEND_OP_MAX",
};