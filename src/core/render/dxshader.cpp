#include <pch.h>
#include <core/render/dx.h>
#include <core/render/dxshader.h>
#include <d3dcompiler.h>

extern CDXParentHandler* g_dxHandler;

CShader* CDXShaderManager::GetShaderByPath(const std::string& path)
{
	if (m_shaders.count(path) != 0)
		return m_shaders.at(path);

	return nullptr;
}

CShader* CDXShaderManager::LoadShaderFromString(const std::string& path, const std::string& sourceString, eShaderType type)
{
	if (CShader* shader = GetShaderByPath(path))
		return shader;

	Log("* loading %s shader %s from string\n", GetShaderTypeName(type), path.c_str());

	const std::string shortName = GetShaderTypeShortName(type);
	const std::string entrypoint = shortName + "_main";
	const std::string shaderTarget = shortName + "_5_0";

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompile(sourceString.c_str(), sourceString.length(), path.c_str(),
		nullptr, nullptr,
		entrypoint.c_str(), shaderTarget.c_str(),
		0u, 0u,
		&shaderBlob,
		&errorBlob
	);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			Log("** failed to compile shader '%s'\n%s", path.c_str(), static_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		else
			Log("** failed to compile shader '%s'", path.c_str());

		return nullptr;
	}

	CShader* const shader = new CShader();
	ID3D11Device* const device = g_dxHandler->GetDevice();

	switch (type)
	{
	case eShaderType::Pixel:
	{
		hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11PixelShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Vertex:
	{
		hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11VertexShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Geometry:
	{
		hr = device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11GeometryShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Hull:
	{
		hr = device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11HullShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Domain:
	{
		hr = device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11DomainShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Compute:
	{
		hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11ComputeShader**>(&shader->m_dxShader));
		break;
	}
	default:
	{
		unreachable();
	}
	}

	shader->m_name = path;
	shader->m_type = type;

	if (FAILED(hr))
	{
		shaderBlob->Release();
		Log("** failed to create %s shader '%s'. HRESULT = 0x%08x", GetShaderTypeName(type), path.c_str(), hr);

		return nullptr;
	}

	// do better
	if (type == eShaderType::Vertex && !shader->m_inputLayout)
	{
		D3D11_INPUT_ELEMENT_DESC desc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		const HRESULT h = g_dxHandler->GetDevice()->CreateInputLayout(desc, ARRSIZE(desc), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &shader->m_inputLayout);
		if (FAILED(h))
			return nullptr;
	}

	m_shaders.emplace(path, shader);
	return shader;
}

CShader* CDXShaderManager::LoadShader(const std::string& path, eShaderType type)
{
	if (CShader* shader = GetShaderByPath(path))
		return shader;

	Log("* loading %s shader %s from file\n", GetShaderTypeName(type), path.c_str());

	if (!std::filesystem::exists(path + ".hlsl"))
	{
		Log("** error: shader file not found\n");
		return nullptr;
	}

	const std::string shortName = GetShaderTypeShortName(type);
	const std::string entrypoint = shortName + "_main";
	const std::string shaderTarget = shortName + "_5_0";

	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
		std::filesystem::path(path + ".hlsl").wstring().c_str(),
		nullptr, nullptr,
		entrypoint.c_str(),
		shaderTarget.c_str(),
		0u, 0u,
		&shaderBlob,
		&errorBlob
	);

	if (FAILED(hr))
	{
		if (errorBlob)
		{
			Log("** failed to compile shader '%s'\n%s", path.c_str(), static_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		else
			Log("** failed to compile shader '%s'", path.c_str());

		return nullptr;
	}

	CShader* const shader = new CShader();
	ID3D11Device* const device = g_dxHandler->GetDevice();

	switch (type)
	{
	case eShaderType::Pixel:
	{
		hr = device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11PixelShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Vertex:
	{
		hr = device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11VertexShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Geometry:
	{
		hr = device->CreateGeometryShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11GeometryShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Hull:
	{
		hr = device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11HullShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Domain:
	{
		hr = device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11DomainShader**>(&shader->m_dxShader));
		break;
	}
	case eShaderType::Compute:
	{
		hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, reinterpret_cast<ID3D11ComputeShader**>(&shader->m_dxShader));
		break;
	}
	default:
	{
		unreachable();
	}
	}

	shader->m_name = path;
	shader->m_type = type;

	if (FAILED(hr))
	{
		shaderBlob->Release();
		Log("** failed to create %s shader '%s'. HRESULT = 0x%08x", GetShaderTypeName(type), path.c_str(), hr);

		return nullptr;
	}

	// do better
	if (type == eShaderType::Vertex && !shader->m_inputLayout)
	{
		D3D11_INPUT_ELEMENT_DESC desc[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "NORMAL", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		const HRESULT h = g_dxHandler->GetDevice()->CreateInputLayout(desc, ARRSIZE(desc), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &shader->m_inputLayout);
		if (FAILED(h))
			return nullptr;
	}

	m_shaders.emplace(path, shader);
	return shader;
}