#include <pch.h>

#include <core/utils/ramen.h>
#include <thirdparty/oodle/oodle2.h>
#include <game/rtech/utils/utils.h>
 
constexpr OodleLZ_Compressor noodleCompressor = OodleLZ_Compressor_Kraken;
constexpr OodleLZ_CompressionLevel noodleCompressionLevel = OodleLZ_CompressionLevel_VeryFast;

const size_t CRamen::addIdx(const size_t index, char* const buf, const size_t bufSize)
{
	if (index > noodleSize)
	{
		Log(__FUNCTION__ " tried to add chunk non sequentially, not supported so an invalid index is returned...\n");
		return invalidNoodleIdx;
	}

	ensureCapacity(noodleSize + 1);

	const size_t compSizeRequired = OodleLZ_GetCompressedBufferSizeNeeded(noodleCompressor, bufSize); // this will not be the actual compressed size which is not ideal
	char* const compBuf = new char[compSizeRequired];
	const size_t compSize = OodleLZ_Compress(noodleCompressor, buf, bufSize, compBuf, noodleCompressionLevel);
	if (compSize == OODLELZ_FAILED)
	{
		assert(false); // odd, report in debug

		delete[] compBuf;
		noodles[index] = new CNoodle(buf, 0ull, bufSize, false);

		noodleSize++;

		return index;
	}

	// oodle demands more memory than it actually uses, fix up.
	char* const compBufShrink = new char[compSize];
	memcpy(compBufShrink, compBuf, compSize);
	delete[] compBuf;

	noodles[index] = new CNoodle(compBufShrink, compSize, bufSize, true);

	noodleSize++;

	return index;
}

std::unique_ptr<char[]> CRamen::getIdx(const size_t index) const
{
	if (capacity == 0)
		return nullptr;

	CNoodle* const thisChunk = noodles[index];
	std::unique_ptr<char[]> out = std::make_unique<char[]>(thisChunk->decompressedSize);
	if (thisChunk->isCompressed)
	{
		const size_t decompSize = OodleLZ_Decompress(thisChunk->data, thisChunk->compressedSize, out.get(), thisChunk->decompressedSize);
		if (decompSize == OODLELZ_FAILED)
		{
			assert(false);
			return nullptr;
		}

		assert(decompSize == thisChunk->decompressedSize);
		return out;
	}
	else
	{
		memcpy(out.get(), thisChunk->data, thisChunk->decompressedSize);
		return out;
	}

	unreachable();
}