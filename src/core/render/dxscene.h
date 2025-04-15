#pragma once
#include <d3d11.h>

struct HardwareLight
{
	int propertyFlags;             // Offset:    0
	float emitterRadius;           // Offset:    4
	int shdFlags;                  // Offset:    8
	float highlightSize;           // Offset:   12
	XMFLOAT3 pos;                    // Offset:   16
	float rcpMaxRadius;            // Offset:   28
	float rcpMaxRadiusSq;          // Offset:   32
	float attenLinear;             // Offset:   36
	float attenQuadratic;          // Offset:   40
	float specularIntensity;       // Offset:   44
	XMFLOAT3 spotDir;                // Offset:   48
	float spotBias;                // Offset:   60
	XMFLOAT3 spotAxisX;              // Offset:   64
	float spotExpSel;              // Offset:   76
	XMFLOAT3 spotAxisY;              // Offset:   80
	int isShadowed;                // Offset:   92
	XMFLOAT3 color;                  // Offset:   96
	float shadowBias;              // Offset:  108
	XMFLOAT4 shadowMapOffsetScale;   // Offset:  112
};

// Class to hold information related to the general render scene.
// Not directly related to any individual type of preview/render setup.
class CDXScene
{
public:
	CDXScene() = default;

	// Global Lights are used in Pixel Shaders as a StructuredBuffer<HardwareLight> resource in register t62.
	// The data must be bound using a shader resource view.
	ID3D11Buffer* globalLightsBuffer;
	ID3D11ShaderResourceView* globalLightsSRV;

	std::vector<HardwareLight> globalLights;

	void CreateOrUpdateLights(ID3D11Device* device, ID3D11DeviceContext* ctx);

	FORCEINLINE bool NeedsLightingUpdate() const
	{
		return this->invalidatedLighting || this->ShouldRecreateLightBuffer();
	}

	FORCEINLINE void BindLightsSRV(ID3D11DeviceContext* ctx)
	{
		ctx->PSSetShaderResources(62u, 1u, &this->globalLightsSRV);
	}

private:

	// Forces the light buffer to be recreated next frame.
	// This is used when a light is modified but the number of lights doesn't change, as this would
	// usually not result in recreation of the buffer.
	FORCEINLINE void InvalidateLights() { invalidatedLighting = true; };
	FORCEINLINE bool ShouldRecreateLightBuffer() const { return !this->initialLightSetupComplete || this->numLightsInBuffer != globalLights.size(); };

	// Number of lights in the current version of the Global Lights Buffer
	// This is stored separately to just using globalLights.size(), since the globalLights vector
	// may be updated between frames, and we should only have to recreate the buffer whenever there are changes to the size.
	size_t numLightsInBuffer;

	// If lighting data needs to be updated without the vector being resized.
	bool invalidatedLighting : 1;
	// If the initial version of the lighting buffer has been created.
	bool initialLightSetupComplete : 1;
};