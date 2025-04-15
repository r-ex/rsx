#pragma once

struct EffectElement_v2
{
    const char* elementName;
    GUID guid; // question mark
    void* unkPtr;
    __int64 unk[3];
    __int64 max_particles;
    const char* material;
    void* unk1[7];
    const char* unkPtr1;
    __int64 bro[35];
};

struct EffectStringDict_v2
{
    const char* stringBuf;
    //RPakPtr string_ptr[data.numStrings];

    //local int stringIdx = 0;
    //for (stringIdx = 0; stringIdx < data.numStrings; stringIdx++)
    //{
    //    GTPTR(string_ptr[stringIdx]);
    //    struct {
    //        string s <fgcolor = 0xee5585>;
    //    } stringentry <read = this.s>;
    //}
};

struct EffectData_v2
{
	const char* pFileName;
    EffectElement_v2* pElements; // unsure
	uint64_t numElements;
    EffectStringDict_v2* pStringDict; // Double ptr to pso name
	uint64_t numStrings;
	//RPakPtr unk4;
};

struct EffectHeader
{
	EffectData_v2* pcfData;
	uint64_t pcfCount;
};
