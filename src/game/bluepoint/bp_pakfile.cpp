#include <pch.h>
#include <core/utils/utils_general.h>
#include <game/bluepoint/bp_pakfile.h>

#if defined(XB_XCOMPRESS)
#include <thirdparty/xcompress/xcompress.h>

#pragma comment(lib, "thirdparty/xcompress/xcompress.lib")
#endif

extern ExportSettings_t g_ExportSettings;

bool CBluepointPakfile::ParseFromFile()
{
	if (m_filePath.empty())
		return false;

	StreamIO in(m_filePath, eStreamIOMode::Read);
	size_t fileSize = std::filesystem::file_size(m_filePath);

	m_Buf = std::make_shared<char[]>(fileSize);
	in.R()->read(m_Buf.get(), fileSize);

	in.close();

	const bpkhdr_short_t* const tmp = reinterpret_cast<const bpkhdr_short_t* const>(m_Buf.get());

	if (tmp->id != BP_PAK_ID)
		return false;

	const int version = SWAP32(tmp->version);

	switch (version)
	{
	case BP_PAK_VER_R1:
	{
		bpkhdr_v6_t* const hdr = reinterpret_cast<bpkhdr_v6_t* const>(m_Buf.get());

		hdr->swap();

		m_version = hdr->version;

		m_fileCount = hdr->fileCount;
		m_patchCount = hdr->patchCount;

		m_files = hdr->pFile(0);
		m_patches = hdr->pPatch(0);

		m_chunkSize = hdr->chunkSize;
		m_chunks.reserve(hdr->chunkCount);

		char* ptrToCurChunk = hdr->pChunkData();

		for (int i = 0; i < hdr->chunkCount; i++)
		{
			const int chunkSize = *hdr->pChunkSize(i);
			const Chunk_t chunk{ .data = ptrToCurChunk, .dataSize = chunkSize, .pad = 0 };

			m_chunks.emplace_back(chunk);

			ptrToCurChunk += chunkSize;
		}

		return true;
	}
	default:
	{
		assertm(false, "invalid version");
		return false;
	}
	}
}

void LoadBluepointWrappedFileAsset(CAssetContainer* container, CAsset* asset)
{
	UNUSED(container);

	CBluepointWrappedFile* const file = static_cast<CBluepointWrappedFile* const>(asset);

	switch (file->GetAssetVersion().majorVer)
	{
	case 0:
	{
		assertm(false, "version 0 currently unsupported");
		return;
	}
	case 6:
	{
		bpkfile_v6_t* const bpkfile = reinterpret_cast<bpkfile_v6_t* const>(file->GetAssetData());
		file->ParseFromBpkfile(bpkfile);

		break;
	}
	default:
	{
		assertm(false, "unaccounted asset version, will cause major issues!");
		return;
	}
	}

	file->SetAssetNameFromCache();
}

bool ExportBluepointWrappedFileAsset(CAsset* const asset, const int setting)
{
	UNUSED(setting);

	const CBluepointWrappedFile* const file = static_cast<const CBluepointWrappedFile* const>(asset);
	const CBluepointPakfile* const pakfile = file->GetContainerFile<CBluepointPakfile>();

	std::filesystem::path exportPath = std::filesystem::current_path().append(EXPORT_DIRECTORY_NAME);
	const std::filesystem::path filePath(file->GetAssetName());

	// truncate paths?
	if (g_ExportSettings.exportPathsFull)
	{
		exportPath.append(filePath.parent_path().string());
	}
	else
	{
		exportPath.append("bpkfile");
		exportPath.append(filePath.stem().string());
	}

	if (!CreateDirectories(exportPath))
	{
		assertm(false, "Failed to create asset directory.");
		return false;
	}

	exportPath.append(filePath.filename().string());
	if (!filePath.has_extension())
		exportPath.replace_extension(".bin");

	std::unique_ptr<char[]> fileBuf = std::make_unique<char[]>(file->GetDecompSize());
	char* curpos = fileBuf.get();

	const CBluepointPakfile::Chunk_t* const firstChunk = pakfile->GetChunk(file->GetFirstChunkIndex());

#ifdef XB_XCOMPRESS
	XMEMCOMPRESSION_CONTEXT ctx = nullptr;

	// [rika]: only create context if we need it
	if (file->IsCompressed())
	{
		XMEMCODEC_PARAMETERS_LZX params;
		params.Flags = 0;
		params.WindowSize = pakfile->GetMaxChunkSize();
		params.CompressionPartitionSize = 524288;

		XMemCreateDecompressionContext(XMEMCODEC_LZX, &params, 0, &ctx);
	}
#endif

	for (int i = 0; i < file->GetChunkCount(); i++)
	{
		const CBluepointPakfile::Chunk_t* const chunk = firstChunk + i;

#ifdef XB_XCOMPRESS
		if (file->IsCompressed() && ctx != nullptr)
		{
			SIZE_T decompSize = 0;
			XMemDecompress(ctx, curpos, &decompSize, chunk->data, chunk->dataSize);

			curpos += decompSize;

			continue;
		}
#endif
		
		memcpy_s(curpos, chunk->dataSize, chunk->data, chunk->dataSize);
		curpos += firstChunk[i].dataSize;
	}

	StreamIO out(exportPath, eStreamIOMode::Write);
	out.write(fileBuf.get(), file->GetDecompSize());

	return true;
}

void InitBluepointWrappedFileAssetType()
{
	AssetTypeBinding_t type =
	{
		.type = 'fwpb',
		.headerAlignment = 4,
		.loadFunc = LoadBluepointWrappedFileAsset,
		.postLoadFunc = nullptr,
		.previewFunc = nullptr,
		.e = { ExportBluepointWrappedFileAsset, 0, nullptr, 0ull },
	};

	REGISTER_TYPE(type);
}