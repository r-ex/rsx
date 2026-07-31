#include <pch.h>
#include <core/render/dxscene.h>
#include <DirectXMath.h>
#include <core/render/dxutils.h>
#include <d3d11.h>

void CDXScene::UpdateHardwareLights()
{
	if (this->globalLights.size() == 0)
	{
		this->globalLights.resize(16384);

		HardwareLight::SetStaticDataInVector(this->globalLights);
	}
}

void CDXScene::UpdateCubemapSamples()
{
	if (this->cubemapSamples.size() == 0)
	{
		this->cubemapSamples.resize(64);

		// wooo magic numbers!!
		this->cubemapSamples[0] = 999.99994f;
		this->cubemapSamples[1] = 535.95728f;
	}
}

void CDXScene::MapAndUpdateLightBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx)
{
#if (ADVANCED_MODEL_PREVIEW)
	// This should be checked before calling, but we don't want to accidentally forget
	// since unnecessarily mapping and copying data is wasteful.
	assert(NeedsLightingUpdate());

	// check if we have to (re)create the buffer
	if (ShouldRecreateLightBuffer())
	{
		// Buffer must be released for any update to the size.
		DX_RELEASE_PTR(this->globalLightsBuffer);
		DX_RELEASE_PTR(this->globalLightsSRV);

		if (this->globalLights.size() > 0 && CreateD3DBuffer(
			device, &this->globalLightsBuffer,
			sizeof(HardwareLight) * static_cast<UINT>(globalLights.size()),
			D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
			D3D11_CPU_ACCESS_WRITE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			sizeof(HardwareLight)
		))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = static_cast<UINT>(globalLights.size());

			HRESULT hr = device->CreateShaderResourceView(this->globalLightsBuffer, &desc, &this->globalLightsSRV);

			assert(SUCCEEDED(hr));

			if (FAILED(hr))
				return;
		}
	}

	if (this->globalLightsBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		assert(SUCCEEDED(ctx->Map(this->globalLightsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)));

		memcpy(resource.pData, this->globalLights.data(), this->globalLights.size() * sizeof(HardwareLight));

		ctx->Unmap(this->globalLightsBuffer, 0);
	}

	if (!this->initialLightSetupComplete)
		this->initialLightSetupComplete = true;
#else
	UNUSED(device); UNUSED(ctx);
	assertm(false, "CDXScene::CreateOrUpdateLights called without ADVANCED_MODEL_PREVIEW defined");
#endif
}


void CDXScene::MapAndUpdateCubemapSamplesBuffer(ID3D11Device* device, ID3D11DeviceContext* ctx)
{
#if (ADVANCED_MODEL_PREVIEW)
	// This should be checked before calling, but we don't want to accidentally forget
	// since unnecessarily mapping and copying data is wasteful.
	// check if we have to (re)create the buffer
	if (ShouldRecreateCubemapSmpBuffer())
	{
		// Buffer must be released for any update to the size.
		DX_RELEASE_PTR(this->cubemapSamplesBuffer);
		DX_RELEASE_PTR(this->cubemapSamplesSRV);

		if (this->cubemapSamples.size() > 0 && CreateD3DBuffer(
			device, &this->cubemapSamplesBuffer,
			sizeof(float) * static_cast<UINT>(cubemapSamples.size()),
			D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE,
			D3D11_CPU_ACCESS_WRITE, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			sizeof(float)
		))
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
			desc.Buffer.FirstElement = 0;
			desc.Buffer.NumElements = static_cast<UINT>(cubemapSamples.size());

			HRESULT hr = device->CreateShaderResourceView(this->cubemapSamplesBuffer, &desc, &this->cubemapSamplesSRV);

			assert(SUCCEEDED(hr));

			if (FAILED(hr))
				return;
		}
	}

	if (this->cubemapSamplesBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		assert(SUCCEEDED(ctx->Map(this->cubemapSamplesBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)));

		memcpy(resource.pData, this->cubemapSamples.data(), this->cubemapSamples.size() * sizeof(float));

		ctx->Unmap(this->cubemapSamplesBuffer, 0);
	}

	if (!this->initialCubemapSmpSetupComplete)
		this->initialCubemapSmpSetupComplete = true;
#else
	UNUSED(device); UNUSED(ctx);
#endif
}