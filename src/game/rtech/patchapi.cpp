#include <pch.h>
#include <game/rtech/patchapi.h>

#if defined(PAKLOAD_PATCHING_ANY)
void CPakFile::SetPatchCommand(const int8_t cmd)
{
    assertm(cmd >= 0 && cmd < ePakPatchCommand_t::_CMD_COUNT, "CPakFile::SetPatchCommand: invalid patch command index.");
    this->p.patchFunc = g_pakPatchApi[cmd];
}

const int CPakFile::ResolvePointers()
{
    int pointerIdx = 0;
    for (pointerIdx = this->numResolvedPointers; pointerIdx < this->pointerCount(); ++pointerIdx)
    {
        PagePtr_t* const curPointer = &this->m_pPointerHeaders[pointerIdx];
        int curCount = curPointer->index - this->firstPageIdx;
        if (curCount < 0)  // get the index of the page in relation to the order that it will be loaded
            curCount += this->pageCount();

        if (curCount >= this->numProcessedPages)
            break;

        PagePtr_t* const ptr = reinterpret_cast<PagePtr_t*>(this->pageBuffers[curPointer->index] + curPointer->offset);
        assertm(ptr->ptr, "uh oh something went very wrong!!!! (shifted pointers are most likely wrong)");
        ptr->ptr = this->pageBuffers[ptr->index] + ptr->offset;
    }

    return pointerIdx;
}

bool PatchCmd_0(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    size_t numBytes = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);

    if (pak->p.numBytesToSkip > 0)
    {
        if (numBytes <= pak->p.numBytesToSkip)
        {
            pak->p.offsetInFileBuffer += numBytes;
            pak->p.numBytesToSkip -= numBytes;
            pak->p.numBytesToPatch -= numBytes;
            *numRemainingFileBufferBytes -= numBytes;

            return pak->p.numBytesToPatch == 0;
        }

        pak->p.offsetInFileBuffer += pak->p.numBytesToSkip;
        pak->p.numBytesToPatch -= pak->p.numBytesToSkip;

        *numRemainingFileBufferBytes -= pak->p.numBytesToSkip;
        numBytes -= pak->p.numBytesToSkip;

        pak->p.numBytesToSkip = 0ull;

        if (numBytes == 0)
            return pak->p.numBytesToPatch == 0;
    }

    const size_t patchSrcSize = std::min(numBytes, pak->p.patchDestinationSize);
    memcpy(
        pak->p.patchDestination,
        pak->GetDecompressedBuffer().get() + pak->p.offsetInFileBuffer,
        patchSrcSize
    );

    pak->p.patchDestination += patchSrcSize;
    pak->p.offsetInFileBuffer += patchSrcSize;
    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;
    *numRemainingFileBufferBytes -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_1(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t numBytes = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);
    const bool ret = *numRemainingFileBufferBytes > pak->p.numBytesToPatch;

    *numRemainingFileBufferBytes -= numBytes;
    pak->p.offsetInFileBuffer += numBytes;
    pak->p.numBytesToPatch -= numBytes;

    return ret;
}

bool PatchCmd_2(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    UNUSED(numRemainingFileBufferBytes);

    if (pak->p.numBytesToSkip > 0)
    {
        if (pak->p.numBytesToPatch <= pak->p.numBytesToSkip)
        {
            pak->p.patchReplacementData += pak->p.numBytesToPatch;
            pak->p.numBytesToSkip -= pak->p.numBytesToPatch;
            pak->p.numBytesToPatch = 0ull;

            return true;
        }

        pak->p.patchReplacementData += pak->p.numBytesToSkip;
        pak->p.numBytesToPatch -= pak->p.numBytesToSkip;
        pak->p.numBytesToSkip = 0ull;
    }

    const size_t patchSrcSize = std::min(pak->p.numBytesToPatch, pak->p.patchDestinationSize);
    memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);

    pak->p.patchReplacementData += patchSrcSize;
    pak->p.patchDestination += patchSrcSize;
    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_3(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t v9 = std::min(*numRemainingFileBufferBytes, pak->p.numBytesToPatch);
    const size_t patchSrcSize = std::min(v9, pak->p.patchDestinationSize);

    memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);
    pak->p.patchReplacementData += patchSrcSize;
    pak->p.patchDestination += patchSrcSize;
    pak->p.offsetInFileBuffer += patchSrcSize;

    pak->p.patchDestinationSize -= patchSrcSize;
    pak->p.numBytesToPatch -= patchSrcSize;
    *numRemainingFileBufferBytes -= patchSrcSize;

    return pak->p.numBytesToPatch == 0;
}

bool PatchCmd_4_5(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    if (!*numRemainingFileBufferBytes)
        return false;

    *pak->p.patchDestination = *(char*)pak->p.patchReplacementData++;
    ++pak->p.offsetInFileBuffer;
    --pak->p.patchDestinationSize;
    ++pak->p.patchDestination;

    pak->SetPatchCommand(ePakPatchCommand_t::CMD_0);

    *numRemainingFileBufferBytes -= 1;

    return PatchCmd_0(pak, numRemainingFileBufferBytes);
}

bool PatchCmd_6(CPakFile* const pak, size_t* const numRemainingFileBufferBytes)
{
    const size_t v2 = *numRemainingFileBufferBytes;
    size_t v3 = 2;

    if (*numRemainingFileBufferBytes < 2)
    {
        if (!*numRemainingFileBufferBytes)
            return false;

        v3 = *numRemainingFileBufferBytes;
    }

    const size_t patchSrcSize = pak->p.patchDestinationSize;
    if (v3 > patchSrcSize)
    {
        memcpy(pak->p.patchDestination, pak->p.patchReplacementData, patchSrcSize);
        pak->p.patchReplacementData += patchSrcSize;
        pak->p.offsetInFileBuffer += patchSrcSize;
        pak->p.patchDestinationSize -= patchSrcSize;
        pak->p.patchDestination += patchSrcSize;
        pak->SetPatchCommand(ePakPatchCommand_t::CMD_4);
        *numRemainingFileBufferBytes -= patchSrcSize;
    }
    else
    {
        memcpy(pak->p.patchDestination, pak->p.patchReplacementData, v3);
        pak->p.patchReplacementData += v3;
        pak->p.offsetInFileBuffer += v3;
        pak->p.patchDestinationSize -= v3;
        pak->p.patchDestination += v3;

        if (v2 >= 2)
        {
            pak->SetPatchCommand(ePakPatchCommand_t::CMD_0);
            *numRemainingFileBufferBytes -= v3;

            return PatchCmd_0(pak, numRemainingFileBufferBytes);
        }

        pak->SetPatchCommand(ePakPatchCommand_t::CMD_4);
        *numRemainingFileBufferBytes = 0;
    }

    return false;
}

const CPakFile::PatchFunc_t g_pakPatchApi[] = {
	PatchCmd_0,
    PatchCmd_1,
    PatchCmd_2,
    PatchCmd_3,
    PatchCmd_4_5,
    PatchCmd_4_5,
    PatchCmd_6
};
static_assert(sizeof(g_pakPatchApi) == 56);
#endif // #if defined(PAKLOAD_PATCHING_ANY)