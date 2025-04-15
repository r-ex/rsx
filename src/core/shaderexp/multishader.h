#pragma once
#include <stdlib.h>
#include <vector> // this will be removed eventuallytm

#undef DOMAIN // go away

// Definitions for the custom MultiShaderWrapper format
// This file is used for exporting and storing all information relating to both Shader and ShaderSet assets
//
// Types of file:
// - Shader
//   - Stores all compiled shader buffers as exported from the game asset CPU data
//   - This uses multiple buffers because shader assets can have different versions of the same shader to support different features 
//     (such as dither fading, instancing, or a combination of them (and maybe others, but this is all i have seen actually being used))
// - ShaderSet
//   - Stores information for both of the shaders referenced by the game asset.
//   - Ultimately points to two Shader data headers, as if the file contained two separate "Shader" MSW files.


// Shader files:
//
// File Header        - MultiShaderWrapper_Header_t       - Defines file-wide information.
// Shader Header      - MultiShaderWrapper_Shader_t       - Defines shader-specific information.
// Shader Descriptors - MultiShaderWrapper_ShaderDesc_t[] - Describes location of individual shader files within the wrapper.
//                                                        - Array size depends on Shader Header variable.

#ifndef MSW_FOURCC
#define MSW_FOURCC(a, b, c, d) (a | (b << 8) | (c << 16) | (d << 24))
#endif

#define MSW_FILE_MAGIC ('M' | ('S' << 8) | ('W' << 16))

// Only updated for breaking changes.
#define MSW_FILE_VER 3

enum class MultiShaderWrapperFileType_e : unsigned char
{
	SHADER = 0,
	SHADERSET = 1,
};

enum class MultiShaderWrapperShaderType_e : unsigned char
{
	PIXEL = 0,
	VERTEX = 1,
	GEOMETRY = 2,
	HULL = 3,
	DOMAIN = 4,
	COMPUTE = 5,
	INVALID = 0xFF
};

#pragma pack(push, 1)
struct MultiShaderWrapper_Header_t
{
	unsigned int magic : 24;    // Identifies this file as being a MSW file.
	unsigned int version : 8; // Used to return early when loading an older (incompatible) version of the file format.
	MultiShaderWrapperFileType_e fileType; // What type of wrapper is this?
};
static_assert(sizeof(MultiShaderWrapper_Header_t) == 5);

struct MultiShaderWrapper_ShaderSet_t
{
	// For R5-exported assets, these should both always be set, but in the event that a shaderset doesn't have one of the shader types,
	// we need to make sure that the parser only tries to read valid data.
	unsigned __int64 pixelShaderGuid;
	unsigned __int64 vertexShaderGuid;

	unsigned short numPixelShaderTextures;
	unsigned short numVertexShaderTextures;

	unsigned short numSamplers;

	unsigned char firstResourceBindPoint;
	unsigned char numResources;

	// These determine the absolute offset to the embedded pixel/vertex shader
	// inside the MSW file. Null if there is no embedded pixel/vertex shader.
	unsigned int pixelShaderOffset;
	unsigned int vertexShaderOffset;
};
static_assert(sizeof(MultiShaderWrapper_ShaderSet_t) == 32);

struct MultiShaderWrapper_Shader_t
{
	// shaderFeatures MUST be the first member of this struct! see WriteShader.
	unsigned __int64 shaderFeatures : 56; // Defines some shader counts.
	unsigned __int64 numShaderDescriptors : 8;  // Number of MultiShaderWrapper_ShaderDesc_t struct instances before file data.

	unsigned int nameLength;
	unsigned int nameOffset;
};
static_assert(sizeof(MultiShaderWrapper_Shader_t) == 16);

struct MultiShaderWrapper_ShaderDesc_t
{
	// Represents a regular shader entry. These shaders have their own file data, pointed to by the "bufferOffset" member.
	// This offset is relative to the start of the MSW_ShaderDesc_t struct instance in the file.
	struct _StandardShader
	{
		unsigned int bufferLength;
		unsigned int bufferOffset;
	};

	// Represents a reference shader entry. These shaders do not have their own file data, and point to a lower index of standard shader
	// to re-use their shader buffer.
	struct _ReferenceShader
	{
		unsigned int bufferIndex;

		// This upper DWORD must be 0 on reference shaders so that they can be identified as references.
		// As the buffer offset of standard shaders are relative to the start of the MSW_ShaderDesc_t struct, the value will never be 0.
		// Therefore, if the value is 0, the shader is a reference to another shader's data.
		unsigned int _reserved;
	};

	union
	{
		_StandardShader u_standard;
		_ReferenceShader u_ref;
	};

	unsigned __int64 inputFlags[2];
};
#pragma pack(pop)

// class for handling reading/writing of MSW files from memory structures
class CMultiShaderWrapperIO
{
public:
	struct ShaderEntry_t
	{
		ShaderEntry_t()
		{
			buffer = 0;
			size = 0;
			refIndex = 0;
			deleteBuffer = false;
			flags[0] = 0;
			flags[1] = 0;
		}

		~ShaderEntry_t()
		{
			if (buffer && deleteBuffer)
				delete[] buffer;
		}

		ShaderEntry_t(ShaderEntry_t&& o) noexcept
			: buffer(o.buffer)
			, size(o.size)
			, refIndex(o.refIndex)
			, deleteBuffer(o.deleteBuffer)
		{
			flags[0] = o.flags[0];
			flags[1] = o.flags[1];

			// prevent it from being destroyed.
			o.buffer = nullptr;
		}

		const char* buffer;
		unsigned int size;
		unsigned short refIndex; // if this shader entry is a reference. do not set "buffer" if this is used
		bool deleteBuffer;

		unsigned __int64 flags[2]; // the input flags
	};

	struct Shader_t
	{
		Shader_t() : shaderType(MultiShaderWrapperShaderType_e::INVALID), features{} {};
		Shader_t(MultiShaderWrapperShaderType_e type) : shaderType(type), features{} {};

		std::vector<ShaderEntry_t> entries;
		std::string name;
		MultiShaderWrapperShaderType_e shaderType;
		unsigned char features[7];
	};

	struct ShaderSet_t
	{
		ShaderSet_t()
			: pixelShader(0)
			, vertexShader(0)
			, pixelShaderGuid(0)
			, vertexShaderGuid(0)
			, numPixelShaderTextures(0)
			, numVertexShaderTextures(0)
			, numSamplers(0)
			, firstResourceBindPoint(0)
			, numResources(0)
			, deleteShaders(false)
		{
		}

		~ShaderSet_t()
		{
			if (deleteShaders)
			{
				if (pixelShader)
					delete pixelShader;

				if (vertexShader)
					delete vertexShader;
			}
		}

		Shader_t* pixelShader;
		Shader_t* vertexShader;

		unsigned __int64 pixelShaderGuid;
		unsigned __int64 vertexShaderGuid;

		unsigned short numPixelShaderTextures;
		unsigned short numVertexShaderTextures;

		unsigned short numSamplers;

		unsigned char firstResourceBindPoint;
		unsigned char numResources;

		bool deleteShaders;
	};

	struct ShaderCache_t
	{
		ShaderCache_t()
			: shader(0)
			, type(MultiShaderWrapperFileType_e::SHADER)
		{}

		~ShaderCache_t()
		{
			if (shader && deleteShader)
				delete shader;
		}

		Shader_t* shader;      // Used if type == SHADER
		ShaderSet_t shaderSet; // Used if type == SHADERSET

		MultiShaderWrapperFileType_e type;
		bool deleteShader;
	};
public:
	CMultiShaderWrapperIO() = default;

	inline void SetFileType(MultiShaderWrapperFileType_e type) { _fileType = type; };

	inline void SetShader(Shader_t* shader)
	{
		if (!shader)
			return;

		if (_fileType == MultiShaderWrapperFileType_e::SHADER)
			this->_storedShaders.shader = shader;
		else if (_fileType == MultiShaderWrapperFileType_e::SHADERSET)
		{
			switch (shader->shaderType)
			{
			case MultiShaderWrapperShaderType_e::PIXEL:
			{
				this->_storedShaders.shaderSet.pixelShader = shader;
				break;
			}
			case MultiShaderWrapperShaderType_e::VERTEX:
			{
				this->_storedShaders.shaderSet.vertexShader = shader;
				break;
			}
			default:
				return;
			}
		}

		writtenAnything = true;
	}

	inline void SetShaderSetHeader(const unsigned __int64 pixelShaderGuid, const unsigned __int64 vertexShaderGuid,
		const unsigned short numPixelShaderTextures, const unsigned short numVertexShaderTextures,
		const unsigned short numSamplers, const unsigned char firstResourceBindPoint, const unsigned char numResources)
	{
		_storedShaders.shaderSet.pixelShaderGuid = pixelShaderGuid;
		_storedShaders.shaderSet.vertexShaderGuid = vertexShaderGuid;

		_storedShaders.shaderSet.numPixelShaderTextures = numPixelShaderTextures;
		_storedShaders.shaderSet.numVertexShaderTextures = numVertexShaderTextures;

		_storedShaders.shaderSet.numSamplers = numSamplers;

		_storedShaders.shaderSet.firstResourceBindPoint = firstResourceBindPoint;
		_storedShaders.shaderSet.numResources = numResources;
	}

	bool ReadFile(const char* filePath, ShaderCache_t* outCache)
	{
		if (!outCache)
			return false;

		FILE* f = NULL;

		if (fopen_s(&f, filePath, "rb") == 0)
		{
			MultiShaderWrapper_Header_t fileHeader = {};

			fread(&fileHeader, sizeof(fileHeader), 1, f);

			//if (fileHeader.magic != MSW_FILE_MAGIC)
			//	Error("Attempted to load an invalid MSW file (expected magic %x, got %x).\n", MSW_FILE_MAGIC, fileHeader.magic);

			//if (fileHeader.version != MSW_FILE_VER)
			//	Error("Attempted to load an unsupported MSW file (expected version %u, got %u).\n", MSW_FILE_VER, fileHeader.version);

			outCache->type = fileHeader.fileType;

			if (fileHeader.fileType == MultiShaderWrapperFileType_e::SHADER)
			{
				outCache->shader = new Shader_t;
				outCache->deleteShader = true;

				ReadShader(f, outCache->shader);
			}
			else if (fileHeader.fileType == MultiShaderWrapperFileType_e::SHADERSET)
			{
				ReadShaderSet(f, outCache);
			}

			return true;
		}
		else
			return false;
	}

	void ReadShaderSet(FILE* const f, ShaderCache_t* const shaderCache)
	{
		MultiShaderWrapper_ShaderSet_t shds;
		fread(&shds, sizeof(shds), 1, f);

		shaderCache->shaderSet.pixelShaderGuid = shds.pixelShaderGuid;
		shaderCache->shaderSet.vertexShaderGuid = shds.vertexShaderGuid;

		shaderCache->shaderSet.numPixelShaderTextures = shds.numPixelShaderTextures;
		shaderCache->shaderSet.numVertexShaderTextures = shds.numVertexShaderTextures;

		shaderCache->shaderSet.numSamplers = shds.numSamplers;

		shaderCache->shaderSet.firstResourceBindPoint = shds.firstResourceBindPoint;
		shaderCache->shaderSet.numResources = shds.numResources;

		if (shds.pixelShaderOffset)
		{
			fseek(f, shds.pixelShaderOffset, SEEK_SET);

			shaderCache->shaderSet.pixelShader = new Shader_t;
			ReadShader(f, shaderCache->shaderSet.pixelShader);
		}

		if (shds.vertexShaderOffset)
		{
			fseek(f, shds.vertexShaderOffset, SEEK_SET);

			shaderCache->shaderSet.vertexShader = new Shader_t;
			ReadShader(f, shaderCache->shaderSet.vertexShader);
		}

		shaderCache->shaderSet.deleteShaders = true;
	};

	// Allocate a shader before calling this
	void ReadShader(FILE* const f, Shader_t* const shader)
	{
		MultiShaderWrapper_Shader_t shdr = {};

		fread(&shdr, sizeof(shdr), 1, f);

		const size_t descStartOffset = ftell(f);

		MultiShaderWrapper_ShaderDesc_t* const descriptors = new MultiShaderWrapper_ShaderDesc_t[shdr.numShaderDescriptors];
		for (int i = 0; i < shdr.numShaderDescriptors; ++i)
		{
			const size_t thisDescOffset = descStartOffset + (i * sizeof(MultiShaderWrapper_ShaderDesc_t));

			fseek(f, static_cast<long>(thisDescOffset), SEEK_SET);

			MultiShaderWrapper_ShaderDesc_t* desc = &descriptors[i];
			fread(desc, sizeof(MultiShaderWrapper_ShaderDesc_t), 1, f);

			ShaderEntry_t& entry = shader->entries.emplace_back();

			if (desc->u_ref.bufferIndex == UINT32_MAX && desc->u_ref._reserved == UINT32_MAX)
			{
				// null shader entry
				entry.refIndex = UINT16_MAX;
			}
			else if (desc->u_ref._reserved == 0 && desc->u_ref.bufferIndex != UINT32_MAX)
			{
				entry.refIndex = static_cast<unsigned short>(desc->u_ref.bufferIndex);
			}
			else
			{
				// regular shader - get buffer and allocate it some new space to go into the shader entry vector
				fseek(f, static_cast<long>(thisDescOffset + desc->u_standard.bufferOffset), SEEK_SET);

				char* buffer = new char[desc->u_standard.bufferLength];

				fread(buffer, sizeof(char), desc->u_standard.bufferLength, f);

				entry.buffer = buffer;
				entry.size = desc->u_standard.bufferLength;
				entry.refIndex = UINT16_MAX;
				entry.deleteBuffer = true;
			}
			
			entry.flags[0] = desc->inputFlags[0];
			entry.flags[1] = desc->inputFlags[1];
		}

		delete[] descriptors;

		// shader type isn't saved, so it has to be found from the shader bytecode separately
		shader->shaderType = MultiShaderWrapperShaderType_e::INVALID;
		memcpy(shader->features, &shdr, sizeof(shader->features));

		if (shdr.nameLength > 0)
		{
			shader->name.reserve(shdr.nameLength);
			shader->name.resize(shdr.nameLength - 1);

			// The name comes last in the shader block.
			fseek(f, shdr.nameOffset, SEEK_SET);
			fread(shader->name.data(), shdr.nameLength, 1, f);
		}
	}

	__forceinline bool WriteFile(const char* filePath)
	{
		if (!writtenAnything && (_fileType == MultiShaderWrapperFileType_e::SHADER))
		{
			assert(0); // exported a shader without setting the data, null shader.
			return false;
		}

		FILE* f = NULL;

		if (fopen_s(&f, filePath, "wb") == 0)
		{
			MultiShaderWrapper_Header_t fileHeader =
			{
				.magic = MSW_FILE_MAGIC,
				.version = MSW_FILE_VER,
				.fileType = this->_fileType
			};

			fwrite(&fileHeader, sizeof(MultiShaderWrapper_Header_t), 1, f);

			if (_fileType == MultiShaderWrapperFileType_e::SHADER)
				WriteShader(f, _storedShaders.shader);
			else if (_fileType == MultiShaderWrapperFileType_e::SHADERSET)
				WriteShaderSet(f);

			fclose(f);
			return true;
		}
		return false;
	}

private:
	inline void WriteShaderSet(FILE* f)
	{
		MultiShaderWrapper_ShaderSet_t shaderSet =
		{
			.pixelShaderGuid = _storedShaders.shaderSet.pixelShaderGuid,
			.vertexShaderGuid = _storedShaders.shaderSet.vertexShaderGuid,

			.numPixelShaderTextures = _storedShaders.shaderSet.numPixelShaderTextures,
			.numVertexShaderTextures = _storedShaders.shaderSet.numVertexShaderTextures,

			.numSamplers = _storedShaders.shaderSet.numSamplers,

			.firstResourceBindPoint = _storedShaders.shaderSet.firstResourceBindPoint,
			.numResources = _storedShaders.shaderSet.numResources,

			.pixelShaderOffset = 0,
			.vertexShaderOffset = 0
		};

		// Skip MSW header and shader set header for later.
		const int curPos = ftell(f);
		fseek(f, curPos + sizeof(shaderSet), SEEK_SET);

		// note: we can write shader sets without the shaders them selfs.
		// shader sets simply just reverence the pixel and vertex shader
		// creating the set. if we write the shaders them selfs here too,
		// repak will add it into the pak if its not already added.
		if (_storedShaders.shaderSet.pixelShader)
		{
			shaderSet.pixelShaderOffset = ftell(f);
			this->WriteShader(f, _storedShaders.shaderSet.pixelShader);
		}
		if (_storedShaders.shaderSet.vertexShader)
		{
			shaderSet.vertexShaderOffset = ftell(f);
			this->WriteShader(f, _storedShaders.shaderSet.vertexShader);
		}

		// Store the header now.
		fseek(f, curPos, SEEK_SET);
		fwrite(&shaderSet, sizeof(shaderSet), 1, f);
	}

	inline void WriteShader(FILE* f, const Shader_t* shader)
	{
		const unsigned int nameLen = static_cast<unsigned int>(shader->name.length());

		MultiShaderWrapper_Shader_t shdr =
		{
			.numShaderDescriptors = shader->entries.size(),
			.nameLength = nameLen ? nameLen+1 : 0,
			.nameOffset = 0
		};

		// Copy to the start of the struct, since we can't copy directly to a bitfield
		memcpy_s(&shdr, sizeof(shader->features), shader->features, sizeof(shader->features));

		const int headerHeaderPos = ftell(f);
		fseek(f, sizeof(shdr), SEEK_CUR);

		const size_t descStartOffset = ftell(f);
		size_t bufferWriteOffset = descStartOffset + (shader->entries.size() * sizeof(MultiShaderWrapper_ShaderDesc_t));

		// Skip the description structs for now, since we can write those at the same time as the buffers.
		// Buffer writing has to come back here to set the offset, so we might as well do it all in one go!
		fseek(f, static_cast<long>(bufferWriteOffset), SEEK_SET);

		size_t entryIndex = 0;
		for (auto& entry : shader->entries)
		{
			const size_t thisDescOffset = descStartOffset + (entryIndex * sizeof(MultiShaderWrapper_ShaderDesc_t));

			MultiShaderWrapper_ShaderDesc_t desc = {};

			// If there is a valid buffer pointer, this entry is standard.
			if (entry.buffer)
			{
				// Seek to the position of the next shader buffer to write.
				fseek(f, static_cast<long>(bufferWriteOffset), SEEK_SET);
				fwrite(entry.buffer, sizeof(char), entry.size, f);

				// Buffer offsets are relative to the start of the desc structure, so subtract one from the other.
				desc.u_standard.bufferOffset = static_cast<unsigned int>(bufferWriteOffset - thisDescOffset);
				desc.u_standard.bufferLength = entry.size;

				bufferWriteOffset += entry.size;
			}
			else if (entry.refIndex != UINT16_MAX) // If the ref index is not 0xFFFF, this entry is a reference.
			{
				desc.u_ref.bufferIndex = entry.refIndex;
			}
			else // If there is no valid buffer and no valid reference, this is a null entry.
			{
				desc.u_ref.bufferIndex = UINT32_MAX;
				desc.u_ref._reserved = UINT32_MAX;
			}

			desc.inputFlags[0] = entry.flags[0];
			desc.inputFlags[1] = entry.flags[1];

			const int bufPos = ftell(f);

			// Seek back to the desc location so we can write it now that the buffer is in place 
			fseek(f, static_cast<long>(thisDescOffset), SEEK_SET);
			fwrite(&desc, sizeof(desc), 1, f);

			// Seek back to the end of the stream
			fseek(f, bufPos, SEEK_SET);

			entryIndex++;
		}

		if (nameLen)
		{
			shdr.nameOffset = ftell(f);
			fwrite(shader->name.c_str(), nameLen + 1, 1, f);
		}

		const int curPos = ftell(f);

		fseek(f, headerHeaderPos, SEEK_SET);
		fwrite(&shdr, sizeof(shdr), 1, f);

		fseek(f, curPos, SEEK_SET);
	}

private:
	ShaderCache_t _storedShaders;

	MultiShaderWrapperFileType_e _fileType;
	bool writtenAnything;
};

static inline const char* MSW_TypeToString(const MultiShaderWrapperFileType_e expectType)
{
	switch (expectType)
	{
	case MultiShaderWrapperFileType_e::SHADER: return "shader";
	case MultiShaderWrapperFileType_e::SHADERSET: return "shader set";
	}

	return "unknown";
}
