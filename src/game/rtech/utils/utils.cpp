#include <pch.h>
#include <game/rtech/utils/utils.h>
#include <thirdparty/oodle/oodle2.h>
#include <intrin.h>

#if _DEBUG
#pragma comment(lib, "thirdparty/oodle/oo2core_x64D.lib")
#else
#pragma comment(lib, "thirdparty/oodle/oo2core_x64.lib")
#endif

#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define LOW_IND(x,part_type)   LAST_IND(x,part_type)
#  define HIGH_IND(x,part_type)  0
#else
#  define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#  define LOW_IND(x,part_type)   0
#endif

#undef LOBYTE
#undef LOWORD
#undef HIBYTE
#undef HIWORD

#define BYTEn(x, n)   (*((uint8_t*)&(x)+n))
#define WORDn(x, n)   (*((uint16_t*)&(x)+n))
#define DWORDn(x, n)  (*((uint32_t*)&(x)+n))
#define LOBYTE(x)   (*((uint8_t*)&(x)))
#define LOWORD(x)  WORDn(x,LOW_IND(x,uint16_t))
#define LODWORD(x)  (*((uint32_t*)&(x)))
#define HIBYTE(x)  BYTEn(x,HIGH_IND(x,uint8_t))
#define HIWORD(x)  WORDn(x,HIGH_IND(x,uint16_t))
#define HIDWORD(x) DWORDn(x,HIGH_IND(x,uint32_t))
#define BYTE1(x)   BYTEn(x,  1)
#define BYTE2(x)   BYTEn(x,  2)
#define _LONGLONG __int128

size_t RTech::InitPakDecoder(
	PakDecompressContext_t* const context, const uint8_t* const fileBuffer,
	const uint64_t inputMask, const size_t dataSize,
	const size_t dataOffset, const size_t headerSize)
{
	unsigned __int64 v7; // r9
	int v8; // ecx
	unsigned __int64 v9; // rbx
	uint64_t v10; // r9
	unsigned int v11; // er15
	unsigned __int64 v12; // rbx
	unsigned int v13; // ebp
	unsigned __int64 v14; // r8
	uint64_t v15; // r11
	unsigned __int64 v16; // r12
	int v17; // er15
	unsigned __int64 v18; // r12
	unsigned int v19; // ebp
	__int64 v20; // rdx
	__int64 v21; // rsi
	uint64_t result; // rax
	uint64_t v23; // r8
	uint64_t v24; // rdx

	context->m_inputBuf = (uint64_t)fileBuffer;
	context->m_outputBuf = 0i64;
	context->m_outputMask = 0i64;
	context->dword44 = 0;
	context->m_fileSize = dataOffset + dataSize;
	context->m_inputMask = inputMask;
	v7 = *(uint64_t*)&fileBuffer[((uint32_t)dataOffset + headerSize) & inputMask];
	const size_t unkPos = dataOffset + headerSize + 8;
	context->m_fileBytePosition = unkPos;
	v8 = v7 & 0x3F;
	v7 >>= 6;
	context->m_decompBytePosition = headerSize;
	context->m_decompSize = v7 & ((1i64 << v8) - 1) | (1i64 << v8);
	v9 = (v7 >> v8) | (*(uint64_t*)&fileBuffer[((uint32_t)unkPos) & inputMask] << (64 - ((unsigned __int8)v8 + 6)));
	v10 = unkPos + ((unsigned __int64)(unsigned int)(v8 + 6) >> 3);
	LOBYTE(v8) = (v8 + 6) & 7;
	context->m_fileBytePosition = v10;
	v11 = (unsigned __int8)v8 + 13;
	v12 = (0xFFFFFFFFFFFFFFFFui64 >> v8) & v9;
	v13 = (((uint8_t)v12 - 1) & 0x3F) + 1;
	v14 = 0xFFFFFFFFFFFFFFFFui64 >> (64 - (unsigned __int8)v13);
	context->m_inputInvMask = v14;
	v15 = v10 + ((unsigned __int64)v11 >> 3);
	context->m_outputInvMask = 0xFFFFFFFFFFFFFFFFui64 >> (63 - (((v12 >> 6) - 1) & 0x3F));
	v16 = (v12 >> 13) | (*(uint64_t*)&fileBuffer[v10 & inputMask] << (64 - (unsigned __int8)v11));
	context->m_fileBytePosition = v15;
	v17 = v11 & 7;
	v18 = (0xFFFFFFFFFFFFFFFFui64 >> v17) & v16;
	if (v14 == -1i64)
	{
		context->m_headerOffset = 0;
		v21 = dataSize;
	}
	else
	{
		v19 = v13 >> 3;
		context->m_headerOffset = v19 + 1;
		v20 = *(uint64_t*)&fileBuffer[v15 & inputMask];
		context->m_fileBytePosition = v15 + v19 + 1;
		v21 = v20 & ((1i64 << (8 * ((unsigned __int8)v19 + 1))) - 1);
	}
	result = context->m_decompSize;
	v23 = context->m_outputInvMask;
	context->qword70 = context->m_inputInvMask + dataOffset - 6;
	context->m_bufferSizeNeeded = v21 + dataOffset;
	context->m_currentByte = v18;
	context->m_currentByteBit = v17;
	context->dword6C = 0;
	context->m_compressedStreamSize = v21 + dataOffset;
	context->m_decompStreamSize = result;
	if (result - 1 > v23)
	{
		v24 = v21 + dataOffset - context->m_headerOffset;
		context->m_decompStreamSize = v23 + 1;
		context->m_compressedStreamSize = v24;
	}
	return result;
}

bool RTech::DecompressPakFile(RTech::PakDecompressContext_t* context, size_t inLen, size_t outLen)
{
	bool result;                          // al
	uint64_t v5;                          // r15
	uint64_t v6;                          // r11
	uint32_t v7;                          // ebp
	uint64_t v8;                          // rsi
	uint64_t v9;                          // rdi
	uint64_t v10;                         // r12
	uint64_t v11;                         // r13
	uint32_t v12;                         // ecx
	uint64_t v13;                         // rsi
	uint64_t i;                           // rax
	uint64_t v15;                         // r8
	int64_t v16;                          // r9
	int v17;                              // ecx
	uint64_t v18;                         // rax
	uint64_t v19;                         // rsi
	int64_t v20;                          // r14
	int v21;                              // ecx
	uint64_t v22;                         // r11
	int v23;                              // edx
	uint64_t v24;                         // rax
	int v25;                              // er8
	uint32_t v26;                         // er13
	uint64_t v27;                         // r10
	uint64_t v28;                         // rax
	uint64_t* v29;                          // r10
	uint64_t v30;                         // r9
	uint64_t v31;                         // r10
	uint64_t v32;                         // r8
	uint64_t v33;                         // rax
	uint64_t v34;                         // rax
	uint64_t v35;                         // rax
	uint64_t v36;                         // rcx
	int64_t v37;                          // rdx
	uint64_t v38;                         // r14
	uint64_t v39;                         // r11
	char v40;                             // cl
	uint64_t v41;                         // rsi
	int64_t v42;                          // rcx
	uint64_t v43;                         // r8
	int v44;                              // er11
	uint8_t v45;                          // r9
	uint64_t v46;                         // rcx
	uint64_t v47;                         // rcx
	int64_t v48;                          // r9
	int64_t l;                            // r8
	uint32_t v50;                         // er9
	int64_t v51;                          // r8
	int64_t v52;                          // rdx
	int64_t k;                            // r8
	char* v54;                            // r10
	int64_t v55;                          // rdx
	uint32_t v56;                         // er14
	int64_t* v57;                         // rdx
	int64_t* v58;                         // r8
	char v59;                             // al
	uint64_t v60;                         // rsi
	int64_t v61;                          // rax
	uint64_t v62;                         // r9
	int v63;                              // er10
	uint8_t v64;                          // cl
	uint64_t v65;                         // rax
	uint32_t v66;                         // er14
	uint32_t j;                           // ecx
	int64_t v68;                          // rax
	uint64_t v69;                         // rcx
	uint64_t v70;                         // [rsp+0h] [rbp-58h]
	uint32_t v71;                         // [rsp+60h] [rbp+8h]
	uint64_t v74;                         // [rsp+78h] [rbp+20h]

	if (inLen < context->m_bufferSizeNeeded)
		return 0;
	v5 = context->m_decompBytePosition;
	if (outLen < context->m_outputInvMask + (v5 & ~context->m_outputInvMask) + 1 && outLen < context->m_decompSize)
		return 0;
	v6 = context->m_outputBuf;
	v7 = context->m_currentByteBit;
	v8 = context->m_currentByte;
	v9 = context->m_fileBytePosition;
	v10 = context->qword70;
	v11 = context->m_inputBuf;
	if (context->m_compressedStreamSize < v10)
		v10 = context->m_compressedStreamSize;
	v12 = context->dword6C;
	v74 = v11;
	v70 = v6;
	v71 = v12;
	if (!v7)
		goto LABEL_11;
	v13 = (*(uint64_t*)((v9 & context->m_inputMask) + v11) << (64 - (unsigned __int8)v7)) | v8;
	for (i = v7; ; i = v7)
	{
		v7 &= 7u;
		v9 += i >> 3;
		v12 = v71;
		v8 = (0xFFFFFFFFFFFFFFFFui64 >> v7) & v13;
	LABEL_11:
		v15 = (unsigned __int64)v12 << 8;
		v16 = v12;
		v17 = *((unsigned __int8*)&s_PakFileCompressionLUT + (unsigned __int8)v8 + v15 + 512);
		v18 = (unsigned __int8)v8 + v15;
		v7 += v17;
		v19 = v8 >> v17;
		v20 = (unsigned int)*((char*)&s_PakFileCompressionLUT + v18);
		if (*((char*)&s_PakFileCompressionLUT + v18) < 0)
		{
			v56 = -(int)v20;
			v57 = (__int64*)(v11 + (v9 & context->m_inputMask));
			v71 = 1;
			v58 = (__int64*)(v6 + (v5 & context->m_outputMask));
			if (v56 == *((unsigned __int8*)&s_PakFileCompressionLUT + v16 + 1248))
			{
				if ((~v9 & context->m_inputInvMask) < 0xF || (context->m_outputInvMask & ~v5) < 15 || context->m_decompSize - v5 < 0x10)
					v56 = 1;
				v59 = char(v19);
				v60 = v19 >> 3;
				v61 = v59 & 7;
				v62 = v60;
				if (v61)
				{
					v63 = *((unsigned __int8*)&s_PakFileCompressionLUT + v61 + 1232);
					v64 = *((uint8_t*)&s_PakFileCompressionLUT + v61 + 1240);
				}
				else
				{
					v62 = v60 >> 4;
					v65 = v60 & 0xF;
					v7 += 4;
					v63 = *((uint32_t*)&s_PakFileCompressionLUT + v65 + 288);
					v64 = *((uint8_t*)&s_PakFileCompressionLUT + v65 + 1216);
				}
				v7 += v64 + 3;
				v19 = v62 >> v64;
				v66 = v63 + (v62 & ((1 << v64) - 1)) + v56;
				for (j = v66 >> 3; j; --j)
				{
					v68 = *v57++;
					*v58++ = v68;
				}
				if ((v66 & 4) != 0)
				{
					*(uint32_t*)v58 = *(uint32_t*)v57;
					v58 = (__int64*)((char*)v58 + 4);
					v57 = (__int64*)((char*)v57 + 4);
				}
				if ((v66 & 2) != 0)
				{
					*(uint16_t*)v58 = *(uint16_t*)v57;
					v58 = (__int64*)((char*)v58 + 2);
					v57 = (__int64*)((char*)v57 + 2);
				}
				if ((v66 & 1) != 0)
					*(uint8_t*)v58 = *(uint8_t*)v57;
				v9 += v66;
				v5 += v66;
			}
			else
			{
				*v58 = *v57;
				v58[1] = v57[1];
				v9 += v56;
				v5 += v56;
			}
		}
		else
		{
			v21 = v19 & 0xF;
			v71 = 0;
			v22 = ((unsigned __int64)(unsigned int)v19 >> (((unsigned int)(v21 - 31) >> 3) & 6)) & 0x3F;
			v23 = 1 << (v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4)));
			v7 += (((unsigned int)(v21 - 31) >> 3) & 6) + *((unsigned __int8*)&s_PakFileCompressionLUT + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			v24 = context->m_outputMask;
			v25 = 16 * (v23 + ((v23 - 1) & (v19 >> ((((unsigned int)(v21 - 31) >> 3) & 6) + *((uint8_t*)&s_PakFileCompressionLUT + v22 + 1088)))));
			v19 >>= (((unsigned int)(v21 - 31) >> 3) & 6) + *((uint8_t*)&s_PakFileCompressionLUT + v22 + 1088) + v21 + ((v19 >> 4) & ((24 * (((unsigned int)(v21 - 31) >> 3) & 2)) >> 4));
			v26 = v25 + *((unsigned __int8*)&s_PakFileCompressionLUT + v22 + 1024) - 16;
			v27 = v24 & (v5 - v26);
			v28 = v70 + (v5 & v24);
			v29 = (uint64_t*)(v70 + v27);
			if ((uint32_t)v20 == 17)
			{
				v40 = char(v19);
				v41 = v19 >> 3;
				v42 = v40 & 7;
				v43 = v41;
				if (v42)
				{
					v44 = *((unsigned __int8*)&s_PakFileCompressionLUT + v42 + 1232);
					v45 = *((uint8_t*)&s_PakFileCompressionLUT + v42 + 1240);
				}
				else
				{
					v7 += 4;
					v46 = v41 & 0xF;
					v43 = v41 >> 4;
					v44 = *((uint32_t*)&s_PakFileCompressionLUT + v46 + 288);
					v45 = *((uint8_t*)&s_PakFileCompressionLUT + v46 + 1216);
					if (v74 && v7 + v45 >= 61)
					{
						v47 = v9++ & context->m_inputMask;
						v43 |= (unsigned __int64)*(unsigned __int8*)(v47 + v74) << (61 - (unsigned __int8)v7);
						v7 -= 8;
					}
				}
				v7 += v45 + 3;
				v19 = v43 >> v45;
				v48 = ((unsigned int)v43 & ((1 << v45) - 1)) + v44 + 17;
				v5 += v48;
				if (v26 < 8)
				{
					v50 = uint32_t(v48 - 13);
					v5 -= 13i64;
					if (v26 == 1)
					{
						v51 = *(unsigned __int8*)v29;
						//++dword_14D40B2BC;
						v52 = 0i64;
						for (k = 0x101010101010101i64 * v51; (unsigned int)v52 < v50; v52 = (unsigned int)(v52 + 8))
							*(uint64_t*)(v52 + v28) = k;
					}
					else
					{
						//++dword_14D40B2B8;
						if (v50)
						{
							v54 = (char*)v29 - v28;
							v55 = v50;
							do
							{
								*(uint8_t*)v28 = v54[v28];
								++v28;
								--v55;
							} while (v55);
						}
					}
				}
				else
				{
					//++dword_14D40B2AC;
					for (l = 0i64; (unsigned int)l < (unsigned int)v48; l = (unsigned int)(l + 8))
						*(uint64_t*)(l + v28) = *(uint64_t*)((char*)v29 + l);
				}
			}
			else
			{
				v5 += v20;
				*(uint64_t*)v28 = *v29;
				*(uint64_t*)(v28 + 8) = v29[1];
			}
			v11 = v74;
		}
		if (v9 >= v10)
			break;
	LABEL_29:
		v6 = v70;
		v13 = (*(uint64_t*)((v9 & context->m_inputMask) + v11) << (64 - (unsigned __int8)v7)) | v19;
	}
	if (v5 != context->m_decompStreamSize)
		goto LABEL_25;
	v30 = context->m_decompSize;
	if (v5 == v30)
	{
		result = true;
		goto LABEL_69;
	}
	v31 = context->m_inputInvMask;
	v32 = context->m_headerOffset;
	v33 = v31 & -(__int64)v9;
	v19 >>= 1;
	++v7;
	if (v32 > v33)
	{
		v9 += v33;
		v34 = context->qword70;
		if (v9 > v34)
			context->qword70 = v31 + v34 + 1;
	}
	v35 = v9 & context->m_inputMask;
	v9 += v32;
	v36 = v5 + context->m_outputInvMask + 1;
	v37 = *(uint64_t*)(v35 + v11) & ((1i64 << (8 * (unsigned __int8)v32)) - 1);
	v38 = v37 + context->m_bufferSizeNeeded;
	v39 = v37 + context->m_compressedStreamSize;
	context->m_bufferSizeNeeded = v38;
	context->m_compressedStreamSize = v39;
	if (v36 >= v30)
	{
		v36 = v30;
		context->m_compressedStreamSize = v32 + v39;
	}
	context->m_decompStreamSize = v36;
	if (inLen >= v38 && outLen >= v36)
	{
	LABEL_25:
		v10 = context->qword70;
		if (v9 >= v10)
		{
			v9 = ~context->m_inputInvMask & (v9 + 7);
			v10 += context->m_inputInvMask + 1;
			context->qword70 = v10;
		}
		if (context->m_compressedStreamSize < v10)
			v10 = context->m_compressedStreamSize;
		goto LABEL_29;
	}
	v69 = context->qword70;
	if (v9 >= v69)
	{
		v9 = ~v31 & (v9 + 7);
		context->qword70 = v69 + v31 + 1;
	}
	context->dword6C = v71;
	result = false;
	context->m_currentByte = v19;
	context->m_currentByteBit = v7;
LABEL_69:
	context->m_decompBytePosition = v5;
	context->m_fileBytePosition = v9;
	return result;
}

// I don't wanna deal with these warnings for now.
#pragma warning(push, 0)
int64_t RTech::sub_7FF7FC23BA70(int64_t param_buffer, int64_t a2)
{
	__int64 v2; // r9
	unsigned int v4; // er11
	char v5; // cl
	char v6; // al
	char v7; // cl
	__int64 v8; // r9
	unsigned int v9; // edi
	unsigned __int16 v10; // cx
	unsigned int v11; // ebp
	int v12; // esi
	unsigned int v13; // ebx
	int v14; // eax
	unsigned int v15; // eax
	unsigned int v16; // edx
	__int64 v17; // rcx
	unsigned int v18; // er11
	unsigned int v19; // ecx
	unsigned int v20; // edx
	__int64 i; // r10
	unsigned int v22; // eax
	unsigned int v23; // eax
	__int64 result; // rax
	int v25; // [rsp+0h] [rbp-A8h]
	__int16 v26[17]; // [rsp+8h] [rbp-A0h]
	__int16 v27; // [rsp+Ch] [rbp-9Ch]
	__int16 v28; // [rsp+Eh] [rbp-9Ah]
	__int16 v29; // [rsp+10h] [rbp-98h]
	__int16 v30; // [rsp+12h] [rbp-96h]
	__int16 v31; // [rsp+14h] [rbp-94h]
	__int16 v32; // [rsp+16h] [rbp-92h]
	__int16 v33; // [rsp+18h] [rbp-90h]
	__int16 v34; // [rsp+1Ah] [rbp-8Eh]
	__int16 v35; // [rsp+1Ch] [rbp-8Ch]
	__int16 v36; // [rsp+1Eh] [rbp-8Ah]
	__int16 v37; // [rsp+20h] [rbp-88h]
	__int16 v38; // [rsp+22h] [rbp-86h]
	__int16 v39; // [rsp+24h] [rbp-84h]
	__int16 v40; // [rsp+26h] [rbp-82h]
	__int16 v41; // [rsp+28h] [rbp-80h]
	int v42; // [rsp+30h] [rbp-78h]
	int v43; // [rsp+34h] [rbp-74h]
	unsigned int v44; // [rsp+38h] [rbp-70h]
	__int64 v45; // [rsp+40h] [rbp-68h]
	__int64 v46; // [rsp+48h] [rbp-60h]
	__int64 v47; // [rsp+50h] [rbp-58h]
	__int64 v48; // [rsp+58h] [rbp-50h]
	__int16 v49; // [rsp+B0h] [rbp+8h]
	__int64 v50; // [rsp+B8h] [rbp+10h]
	unsigned __int16 v51; // [rsp+C0h] [rbp+18h]
	unsigned int v52; // [rsp+C8h] [rbp+20h]

	v50 = a2;
	v2 = a2;
	v44 = *(unsigned __int8*)(param_buffer + 1108) + 1;
	v49 = 0;
	v4 = 0;
	v52 = 0;
	v45 = 0i64;
	do // This below can heavily be cleaned up and made looks nicer, will do when its actually working.
	{
		v41 = 0;
		v47 = 16 * v4;
		v25 = 0;
		v43 = (unsigned __int16)(1 << (15 - *(uint8_t*)(v47 + v2)));
		v26[0] = v43;
		v42 = (unsigned __int16)(1 << (15 - *(uint8_t*)((unsigned int)(v47 + 1) + v2)));
		v26[1] = v42;
		v27 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 2) + v2));
		v26[2] = v27;
		v28 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 3) + v2));
		v26[3] = v28;
		v29 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 4) + v2));
		v26[4] = v29;
		v30 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 5) + v2));
		v26[5] = v30;
		v31 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 6) + v2));
		v26[6] = v31;
		v32 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 7) + v2));
		v26[7] = v32;
		v33 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 8) + v2));
		v26[8] = v33;
		v34 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 9) + v2));
		v26[9] = v34;
		v5 = 15 - *(uint8_t*)((unsigned int)(v47 + 10) + v2);
		v48 = 0i64;
		v46 = 17i64;
		v6 = *(uint8_t*)((unsigned int)(v47 + 11) + v2);
		v35 = 1 << v5;
		v26[10] = v35;
		v7 = 15 - *(uint8_t*)((unsigned int)(v47 + 12) + v2);
		v36 = 1 << (15 - v6);
		v26[11] = v36;
		v37 = 1 << v7;
		v26[12] = v37;
		v38 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 13) + v2));
		v26[13] = v38;
		v39 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 14) + v50));
		v26[14] = v39;
		v40 = 1 << (15 - *(uint8_t*)((unsigned int)(v47 + 15) + v50));
		v26[15] = v40;
		v8 = 0i64;
		v9 = v44;
		v10 = v32 + v33 + v34 + v35 + v36 + (1 << v7) + v38 + v39 + v40 + v43 + v42 + v27 + v28 + v29 + v30 + v31;
		v11 = v10;
		v51 = v10;
		v12 = v10 >> 1;
		v13 = 0;
		do
		{
			v14 = v25 << 15;
			v25 += (unsigned __int16)v26[v8];
			v15 = (v12 + v14) / v11;
			v16 = 0;
			v17 = v15;
			if (v9)
			{
				v18 = v52;
				do
				{
					*(uint16_t*)(param_buffer + 2 * (v8 + 272 * (v52 | (unsigned __int64)(16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 1i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 2i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 3i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 4i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 5i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 6i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 7i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 8i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 9i64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 0xAi64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 0xBi64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 0xCi64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 0xDi64)) + 9816) = v15;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * ((16 * (v52 | (16 * (v16 & *(uint8_t*)(param_buffer + 1108))))) | 0xEi64)) + 9816) = v15;
					v19 = v16++ & *(uint8_t*)(param_buffer + 1108);
					v17 = (16 * (v52 | (16 * v19))) | 0xFi64;
					*(uint16_t*)(param_buffer + 2 * (v8 + 17 * v17) + 9816) = v15;
				} while (v16 < v9);
			}
			else
			{
				v18 = v52;
			}
			++v8;
			--v46;
		} while (v46);
		v20 = 0;
		for (i = v45; v20 < v9; *(uint16_t*)(param_buffer + 2 * v17 + 1112) = v49)
		{
			*(uint16_t*)(param_buffer + 2 * (i + 272i64 * (v20 & *(uint8_t*)(param_buffer + 1108))) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 1i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 2i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 3i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 4i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 5i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 6i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 7i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 8i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 9i64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 0xAi64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 0xBi64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 0xCi64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 0xDi64)) + 1112) = v49;
			*(uint16_t*)(param_buffer + 2 * (i + 17 * ((16 * (v20 & *(uint8_t*)(param_buffer + 1108))) | 0xEi64)) + 1112) = v49;
			v22 = v20++ & *(uint8_t*)(param_buffer + 1108);
			v17 = i + 17 * ((16 * v22) | 0xFi64);
		}
		v4 = v18 + 1;
		v52 = v4;
		v49 += v51;
		v2 = v50;
		v45 = i + 1;
	} while (v4 < 0x10);
	if (!v9)
		return v17;
	do
	{
		*(uint16_t*)(544i64 * (v13 & *(uint8_t*)(param_buffer + 1108)) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 1i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 2i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 3i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 4i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 5i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 6i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 7i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 8i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 9i64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 0xAi64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 0xBi64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 0xCi64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 0xDi64) + param_buffer + 1144) = 0x8000;
		*(uint16_t*)(34 * ((16 * (v13 & *(uint8_t*)(param_buffer + 1108))) | 0xEi64) + param_buffer + 1144) = 0x8000;
		v23 = v13++ & *(uint8_t*)(param_buffer + 1108);
		result = 34 * ((16 * v23) | 0xFi64);
		*(uint16_t*)(result + param_buffer + 1144) = 0x8000;
	} while (v13 < v9);
	return result;
}

__int64 RTech::sub_7FF7FC23C680(__int64 a1, unsigned __int64* a2, unsigned __int8 a3, unsigned int a4, void* a5)
{
	unsigned __int64 v5; // rbx
	void* v7; // r14
	int v9; // er15
	unsigned __int64 v10; // rbx
	unsigned int v11; // esi
	unsigned int v12; // er11
	char v13; // r10
	unsigned __int16 v14; // r10
	unsigned __int16 v15; // r8
	unsigned __int16 i; // ax
	__int16 v17; // r9
	__int16 v18; // dx
	__int64 v19; // rax
	char v20; // bl
	uint64_t* v21; // rdi
	unsigned int v22; // ebx
	unsigned __int64 v23; // rdx
	__int64 v24; // rax
	int v25; // ecx
	unsigned __int64 v26; // rax
	unsigned int v27; // eax
	__int64 v29[66]; // [rsp+20h] [rbp-258h]

	v5 = *a2;
	v7 = a5; // watch this cast fuck everything.
	v9 = a3;
	v29[0] = -65536i64;
	v10 = v5 >> a3;
	v11 = 0;
	v29[1] = -1i64;
	v12 = 0;
	do
	{
		v13 = v10;
		v10 >>= 3;
		v14 = v13 & 7;
		if (v14)
		{
			v15 = *((uint16_t*)v29 + v14);
			*((uint16_t*)v29 + v14) = -1;
			if (v15 == 0xFFFF)
			{
				for (i = v14 - 1; *((uint16_t*)v29 + i) == 0xFFFF; --i)
					;
				v15 = *((uint16_t*)v29 + i);
				*((uint16_t*)v29 + i) = -1;
				v17 = 1 << i;
				do
				{
					++i;
					v18 = v15 + v17;
					v17 *= 2;
					*((uint16_t*)v29 + i) = v18;
				} while (i != v14);
			}
			do
			{
				v19 = v15;
				v15 += 1 << v14;
				*((uint16_t*)&v29[2] + 2 * v19) = v12;
				*((uint16_t*)&v29[2] + 2 * v19 + 1) = v14;
			} while (v15 < 0x80u);
		}
		++v12;
	} while (v12 <= a4);
	v20 = a4 + 1;
	v21 = (unsigned __int64*)((char*)a2 + ((unsigned __int64)(a4 + 1 + v9 + 2 * (a4 + 1)) >> 3));
	v22 = ((uint8_t)v9 + 2 * v20 + v20) & 7;
	if (LOWORD(v29[0]))
	{
		v23 = *v21 >> v22;
		do
		{
			v24 = v23 & 0x7F;
			v25 = *((unsigned __int16*)&v29[2] + 2 * v24 + 1);
			v22 += v25;
			*(uint8_t*)v7 = *((uint8_t*)&v29[2] + 4 * v24);
			v23 >>= v25;
			if ((v11 & 7) == 7)
			{
				v26 = v22;
				v22 &= 7u;
				v21 = (uint64_t*)((char*)v21 + (v26 >> 3));
				v23 = *v21 >> v22;
			}
			++v11;
			v7 = (char*)v7 + 1;
		} while (v11 < 0x100);
	}
	else
	{
		_BitScanReverse((unsigned long*)&v27, 0x100u); // meme cast
		memset(a5, (char)v27, 0x100ui64);
	}
	return v22 + 8 * ((uint32_t)v21 - (uint32_t)a2) - v9;
}

int64_t RTech::sub_7FF7FC23B960(int64_t param_buffer, uint32_t unk1)
{
	// dmp location: sig: 8B 49 2C 
	// retail location: direct reference: [actual address in first opcode] E8 ? ? ? ? 8D 4B 09 
	uint32_t v4; // ecx
	int64_t v5; // r8
	char v6; // bl
	int64_t v7; // rbp
	uint32_t v8; // er11
	int64_t v9; // rsi
	int64_t v10; // rdi
	uint64_t v11; // rdx
	uint32_t v12; // er8
	int v13; // er8
	int64_t result; // rax
	uint32_t v15; // edx

	v4 = *(uint32_t*)(param_buffer + 44);
	if (v4 >= unk1)
	{
		v15 = *(uint32_t*)(param_buffer + 40);
		*(uint32_t*)(param_buffer + 44) = v4 - unk1;
		*(uint32_t*)(param_buffer + 40) = v15 >> unk1;
		result = v15 & ((1 << unk1) - 1);
	}
	else
	{
		v5 = *(uint64_t*)(param_buffer + 149184);
		v6 = v4 + 32;
		v7 = *(uint64_t*)(param_buffer + 149168);
		v8 = v4 + 32 - unk1;
		v9 = v5 + 4;
		v10 = *(uint64_t*)(param_buffer + 149192);
		v11 = *(uint32_t*)(param_buffer + 40) | ((uint64_t) * (uint32_t*)((v5 & v10) + v7) << v4);
		*(uint64_t*)(param_buffer + 149184) = v5 + 4;
		*(uint32_t*)(param_buffer + 44) = v4 + 32;
		if (v4 + 32 >= unk1)
		{
			v13 = v11 >> unk1;
		}
		else
		{
			v12 = *(uint32_t*)((v9 & v10) + v7);
			*(uint64_t*)(param_buffer + 149184) = v9 + 4;
			v11 |= (uint64_t)v12 << v6;
			v13 = v12 >> (unk1 - v6);
			v8 += 32;
		}
		*(uint32_t*)(param_buffer + 44) = v8;
		result = v11 & ((1i64 << unk1) - 1);
		*(uint32_t*)(param_buffer + 40) = v13;
	}

	return result;
}

int64_t RTech::sub_7FF7FC23C880(int64_t param_buffer, uint8_t a2, int64_t a3)
{
	// dmp location: direct reference: [actual address in first opcode] E8 ? ? ? ? 44 8D 04 33
	// retail location: direct reference: [actual address in first opcode] E8 ? ? ? ? 8D 0C 33  

	int64_t v4; // rax
	int64_t v5; // rdx
	int64_t v6; // rcx
	uint32_t* v7; // rax
	int64_t v8; // rax
	int64_t v9; // rcx
	int64_t v10; // rcx
	int64_t v11; // rax
	int64_t result; // rax

	v4 = a2;
	v5 = 4i64;
	v6 = param_buffer + 216;
	*(uint8_t*)(param_buffer + 1108) = LUT_Snowflake_0[v4];
	*(uint16_t*)(param_buffer + 1088) = 2048;
	v7 = (std::uint32_t*)(v6 + 130);
	*(uint32_t*)param_buffer = 126353408;
	*(uint32_t*)(param_buffer + 4) = 378998543;
	*(uint32_t*)(param_buffer + 8) = 631643678;
	*(uint32_t*)(param_buffer + 12) = 884288813;
	*(uint32_t*)(param_buffer + 16) = 1136933948;
	*(uint32_t*)(param_buffer + 20) = 1389579083;
	*(uint32_t*)(param_buffer + 24) = 1642224218;
	*(uint32_t*)(param_buffer + 28) = 1894869353;
	*(uint32_t*)(param_buffer + 32) = -2147452808;
	*(uint32_t*)(param_buffer + 36) = 119275520;
	*(uint32_t*)(param_buffer + 40) = 357895737;
	*(uint32_t*)(param_buffer + 44) = 596515954;
	*(uint32_t*)(param_buffer + 48) = 835136171;
	*(uint32_t*)(param_buffer + 52) = 1073756388;
	*(uint32_t*)(param_buffer + 56) = 0x10000000;
	*(uint32_t*)(param_buffer + 60) = 805314560;
	*(uint32_t*)(param_buffer + 64) = 1342193664;
	*(uint32_t*)(param_buffer + 68) = 1879072768;
	*(uint16_t*)(param_buffer + 72) = 0x8000;
	*(uint32_t*)(param_buffer + 74) = 126353408;
	*(uint32_t*)(param_buffer + 78) = 378998543;
	*(uint32_t*)(param_buffer + 82) = 631643678;
	*(uint32_t*)(param_buffer + 86) = 884288813;
	*(uint32_t*)(param_buffer + 90) = 1136933948;
	*(uint32_t*)(param_buffer + 94) = 1389579083;
	*(uint32_t*)(param_buffer + 98) = 1642224218;
	*(uint32_t*)(param_buffer + 102) = 1894869353;
	*(uint32_t*)(param_buffer + 106) = -2147452808;
	*(uint32_t*)(param_buffer + 110) = 63176704;
	*(uint32_t*)(param_buffer + 114) = 189466504;
	*(uint32_t*)(param_buffer + 118) = 315821839;
	*(uint32_t*)(param_buffer + 122) = 442111639;
	*(uint32_t*)(param_buffer + 126) = 568466974;
	*(uint32_t*)(param_buffer + 130) = 694756774;
	*(uint32_t*)(param_buffer + 134) = 821112109;
	*(uint32_t*)(param_buffer + 138) = 947401909;
	*(uint32_t*)(param_buffer + 142) = 1073757244;
	*(uint32_t*)(param_buffer + 146) = 126353408;
	*(uint32_t*)(param_buffer + 150) = 378998543;
	*(uint32_t*)(param_buffer + 154) = 631643678;
	*(uint32_t*)(param_buffer + 158) = 884288813;
	*(uint32_t*)(param_buffer + 162) = 1136933948;
	*(uint32_t*)(param_buffer + 166) = 1389579083;
	*(uint32_t*)(param_buffer + 170) = 1642224218;
	*(uint32_t*)(param_buffer + 174) = 1894869353;
	*(uint32_t*)(param_buffer + 178) = -2147452808;
	*(uint32_t*)(param_buffer + 182) = 0x4000000;
	*(uint32_t*)(param_buffer + 186) = 201328640;
	*(uint32_t*)(param_buffer + 190) = 335548416;
	*(uint32_t*)(param_buffer + 194) = 469768192;
	*(uint32_t*)(param_buffer + 198) = 603987968;
	*(uint32_t*)(param_buffer + 202) = 738207744;
	*(uint32_t*)(param_buffer + 206) = 872427520;
	*(uint32_t*)(param_buffer + 210) = 1006647296;
	*(uint16_t*)(param_buffer + 214) = 0x4000;
	do
	{
		*(v7 - 32) = 67109376;
		*(uint16_t*)v6 = 0;
		v6 += 218;
		*(v7 - 31) = 134219264;
		*(v7 - 30) = 201329152;
		*(v7 - 29) = 268439040;
		*(v7 - 28) = 0x10000000;
		*(v7 - 27) = 805314560;
		*(v7 - 26) = 0x4000;
		*(v7 - 25) = 536875008;
		*(v7 - 24) = 1073754112;
		*(v7 - 23) = 0x10000000;
		*(v7 - 22) = 805314560;
		*(v7 - 21) = 0x4000;
		*(v7 - 20) = 536875008;
		*(v7 - 19) = 1073754112;
		*(v7 - 18) = 0x10000000;
		*(v7 - 17) = 805314560;
		*(v7 - 16) = 0x4000;
		*(v7 - 15) = 536875008;
		*(v7 - 14) = 1073754112;
		*(v7 - 13) = 1610633216;
		*(v7 - 12) = -2147454976;
		*(v7 - 11) = 0x10000000;
		*(v7 - 10) = 805314560;
		*(v7 - 9) = 1342193664;
		*(v7 - 8) = 1879072768;
		*((uint16_t*)v7 - 14) = 0x8000;
		*(v7 - 2) = 76677120;
		*(v7 - 1) = 230099237;
		*v7 = 383455817;
		v7[1] = 536877934;
		v7[2] = 76677120;
		v7[3] = 230099237;
		v7[4] = 383455817;
		v7[5] = 536877934;
		v7[6] = 76677120;
		v7[7] = 230099237;
		v7[8] = 383455817;
		v7[9] = 536877934;
		v7[10] = 76677120;
		v7[11] = 230099237;
		v7[12] = 383455817;
		v7[13] = 536877934;
		v7[14] = 76677120;
		v7[15] = 230099237;
		v7[16] = 383455817;
		v7[17] = 536877934;
		v7[18] = 76677120;
		v7[19] = 230099237;
		v7[20] = 383455817;
		v7[21] = 536877934;
		*(uint32_t*)((char*)v7 - 26) = 0x10000000;
		*(uint32_t*)((char*)v7 - 22) = 805314560;
		*(uint32_t*)((char*)v7 - 18) = 1342193664;
		*(uint32_t*)((char*)v7 - 14) = 1879072768;
		*((uint16_t*)v7 - 5) = 0x8000;
		v7 = (uint32_t*)((char*)v7 + 218);
		--v5;
	} while (v5);

	if (a3)
	{
		sub_7FF7FC23BA70(param_buffer, a3);
	}
	else
	{
		v8 = param_buffer + 1116;
		v9 = 256i64;
		do
		{
			*(uint32_t*)(v8 - 4) = 0x8000000;
			*(uint16_t*)(v8 + 28) = 0x8000;
			*(uint32_t*)v8 = 402657280;
			*(uint32_t*)(v8 + 4) = 671096832;
			*(uint32_t*)(v8 + 8) = 939536384;
			*(uint32_t*)(v8 + 12) = 1207975936;
			*(uint32_t*)(v8 + 16) = 1476415488;
			*(uint32_t*)(v8 + 20) = 1744855040;
			*(uint32_t*)(v8 + 24) = 2013294592;
			v8 += 34i64;
			--v9;
		} while (v9);
		v10 = 4096i64;
		v11 = param_buffer + 9820;
		do
		{
			*(uint32_t*)(v11 - 4) = 0x8000000;
			*(uint16_t*)(v11 + 28) = 0x8000;
			*(uint32_t*)v11 = 402657280;
			*(uint32_t*)(v11 + 4) = 671096832;
			*(uint32_t*)(v11 + 8) = 939536384;
			*(uint32_t*)(v11 + 12) = 1207975936;
			*(uint32_t*)(v11 + 16) = 1476415488;
			*(uint32_t*)(v11 + 20) = 1744855040;
			*(uint32_t*)(v11 + 24) = 2013294592;
			v11 += 34i64;
			--v10;
		} while (v10);
	}
	*(uint32_t*)(param_buffer + 1092) = 96;
	*(uint16_t*)(param_buffer + 1090) = 1024;
	result = 0i64;
	*(uint32_t*)(param_buffer + 1096) = 128;
	*(uint32_t*)(param_buffer + 1100) = 80;
	*(uint32_t*)(param_buffer + 1104) = 112;

	return result;
}

__int64 RTech::sub_7FF7FC23CD20(unsigned __int8* param_buffer, unsigned int a2)
{
	__int64 v2; // rsi
	unsigned __int64 v3; // rbx
	uint8_t* v4; // r14
	unsigned int v5; // er15
	__int64 v6; // r13
	__int64 v7; // r9
	__int64 v8; // r10
	__int64 v9; // rbp
	unsigned __int16 v10; // r8
	unsigned __int64 v11; // r11
	__int64 v12; // r8
	__int64 v13; // r9
	__m128i v14; // xmm7
	__m128i v15; // xmm5
	__m128i v16; // xmm8
	__m128i v17; // xmm9
	__m128i v18; // xmm6
	__m128i v19; // xmm10
	__m128i* v20; // r11
	unsigned __int64 v21; // r10
	__int64 v22; // r8
	__int64 v23; // r9
	int v24; // er8
	unsigned int v25; // er8
	unsigned __int64 v26; // rax
	__m128i v27; // xmm2
	__m128i v28; // xmm4
	unsigned int v29; // er9
	__m128i v30; // xmm0
	__m128i v31; // xmm1
	__m128i v32; // xmm3
	__int64 v33; // rdx
	__int64 result; // rax
	__m128i v35; // xmm4
	__m128i v36; // xmm0
	__m128i v37; // xmm2
	__m128i v38; // xmm3
	__m128i v39; // xmm0
	unsigned __int64 v40; // r10
	__int64 v41; // r8
	__int64 v42; // r9
	char v43; // r12
	unsigned int v44; // ebp
	__m128i* v45; // rsi
	__m128i v46; // xmm4
	__m128i v47; // xmm5
	__m128i v48; // xmm1
	__m128i v49; // xmm1
	__m128i v50; // xmm2
	__m128i v51; // xmm3
	unsigned int v52; // edx
	unsigned int v53; // er11
	__int64 v54; // r9
	unsigned __int64 v55; // r10
	__int64 v56; // r9
	__int64 v57; // r8
	__m128i v58; // xmm7
	__m128i v59; // xmm14
	__m128i v60; // xmm15
	__m128i v61; // xmm12
	__m128i v62; // xmm6
	__m128i v63; // xmm13
	unsigned __int8 v64; // al
	int v65; // edx
	__int64 v66; // r15
	__m128i* v67; // r13
	__m128i v68; // xmm8
	__m128i v69; // xmm9
	__m128i v70; // xmm10
	__m128i v71; // xmm11
	__m128i v72; // xmm1
	__m128i v73; // xmm1
	__m128i v74; // xmm2
	__m128i v75; // xmm3
	__m128i v76; // xmm1
	__m128i v77; // xmm1
	__m128i v78; // xmm4
	__m128i v79; // xmm5
	int v80; // edx
	unsigned int v81; // er10
	__int64 v82; // r9
	unsigned __int64 v83; // r10
	__int64 v84; // r8
	__int64 v85; // r9
	__int64 v86; // r8
	__int64 v87; // r9
	char v88; // dl
	__m128i v89; // xmm4
	__m128i v90; // xmm5
	__m128i v91; // xmm1
	__m128i v92; // xmm1
	__m128i v93; // xmm2
	__m128i v94; // xmm3
	unsigned int v95; // ebx
	__int64 v96; // r9
	unsigned __int64 v97; // r10
	__int64 v98; // r8
	__int64 v99; // r9
	char v100; // [rsp+8h] [rbp-F0h]
	__m128i* v101; // [rsp+10h] [rbp-E8h]
	unsigned __int64 v102; // [rsp+18h] [rbp-E0h]

	__m128i m1 = _mm_set_epi32(0x77106f2, 0x67305f4, 0x57504f6, 0x47703f8);
	__m128i m2 = _mm_set_epi32(0x788f788f, 0x788f788f, 0x788f788f, 0x788f788f);
	__m128i m3 = _mm_set_epi32(0x37902fa, 0x27b01fc, 0x17d00fe, 0x7f0000);
	__m128i m4 = _mm_set_epi32(0x3b10372, 0x33302f4, 0x2b50276, 0x23701f8);
	__m128i m5 = _mm_set_epi32(0x7c4f7c4f, 0x7c4f7c4f, 0x7c4f7c4f, 0x7c4f7c4f);
	__m128i m6 = _mm_set_epi32(0x1b9017a, 0x13b00fc, 0xbd007e, 0x3f0000);
	__m128i m7 = _mm_set_epi32(0xf000e, 0xd000c, 0xb000a, 0x90008);
	__m128i m8 = _mm_set_epi32(0x70006, 0x50004, 0x30002, 0x10000);

	v2 = *((uint64_t*)param_buffer + 18651);
	LODWORD(v3) = param_buffer[1138];
	v4 = (uint8_t*)(v2 + *((uint64_t*)param_buffer + 18650));
	v5 = a2;
	v6 = a2;
	*((uint64_t*)param_buffer + 18651) = v2 + a2;
	if (a2 > 0xF)
	{
		v7 = *((unsigned __int16*)param_buffer + 568);
		v8 = *(uint64_t*)param_buffer >> 12;
		v9 = *(uint64_t*)param_buffer & 0xFFFi64;
		v10 = 4096 - v7;
		if ((*(uint32_t*)param_buffer & 0xFFFu) >= (unsigned int)v7)
		{
			v11 = v9 + v8 * v10 - v7;
			*((uint16_t*)param_buffer + 568) = v7 - ((unsigned __int16)v7 >> 4);
			if (v11 < 0x100000000i64)
			{
				v12 = *((uint64_t*)param_buffer + 3);
				v13 = *(unsigned int*)((v12 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
				*((uint64_t*)param_buffer + 3) = v12 + 4;
				v11 = v13 | (v11 << 32);
			}
			v14 = _mm_load_si128(&m1);
			v15 = _mm_load_si128(&m2);
			v16 = _mm_load_si128(&m3);
			v17 = _mm_load_si128(&m4);
			v18 = _mm_load_si128(&m5);
			v19 = _mm_load_si128(&m6);
			*(uint64_t*)param_buffer = *((uint64_t*)param_buffer + 1);
			*((uint64_t*)param_buffer + 1) = v11;
			do
			{
				LODWORD(v20) = *param_buffer;
				v21 = *(uint64_t*)param_buffer >> 8;
				if (v21 < 0x100000000i64)
				{
					v22 = *((uint64_t*)param_buffer + 3);
					v23 = *(unsigned int*)((v22 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
					*((uint64_t*)param_buffer + 3) = v22 + 4;
					v21 = v23 | (v21 << 32);
				}
				v24 = param_buffer[1156];
				*(uint64_t*)param_buffer = *((uint64_t*)param_buffer + 1);
				v25 = v2 & v24;
				*((uint64_t*)param_buffer + 1) = v21;
				LODWORD(v2) = v2 + 1;
				*v4++ = (uint8_t)v20;
				v25 *= 16;
				v26 = 34 * (v25 | ((unsigned __int64)(unsigned int)v3 >> 4));
				v27 = _mm_loadu_si128((const __m128i*) & param_buffer[v26 + 1160]);
				v28 = _mm_loadu_si128((const __m128i*) & param_buffer[v26 + 1176]);
				v29 = v3 & 0xF | (16 * (v25 | ((unsigned int)v20 >> 4)));
				LODWORD(v3) = (uint32_t)v20;
				v30 = _mm_cvtsi32_si128((unsigned int)v20 >> 4);
				v31 = _mm_shuffle_epi32(_mm_unpacklo_epi16(v30, v30), 0);
				v32 = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(
							_mm_add_epi16(
								_mm_and_si128(_mm_cmpgt_epi16(_mm_load_si128(&m7), v31), v15),
								v14),
							v28),
						7u),
					v28);
				*(__m128i*)& param_buffer[v26 + 1160] = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(
							_mm_add_epi16(
								_mm_and_si128(
									_mm_cmpgt_epi16(
										_mm_load_si128(&m8),
										v31),
									v15),
								v16),
							v27),
						7u),
					v27);
				*(__m128i*)& param_buffer[v26 + 1176] = v32;
				v33 = 34i64 * v29;
				result = (unsigned __int8)v20 & 0xF;
				v35 = _mm_loadu_si128((const __m128i*) & param_buffer[v33 + 9880]);
				v36 = _mm_cvtsi32_si128(result);
				v37 = _mm_shuffle_epi32(_mm_unpacklo_epi16(v36, v36), 0);
				v38 = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(
							_mm_add_epi16(
								_mm_and_si128(_mm_cmpgt_epi16(_mm_load_si128(&m7), v37), v18),
								v17),
							v35),
						6u),
					v35);
				v39 = _mm_loadu_si128((const __m128i*) & param_buffer[v33 + 9864]);
				*(__m128i*)& param_buffer[v33 + 9864] = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(
							_mm_add_epi16(
								_mm_and_si128(
									_mm_cmpgt_epi16(
										_mm_load_si128(&m8),
										v37),
									v18),
								v19),
							v39),
						6u),
					v39);
				*(__m128i*)& param_buffer[v33 + 9880] = v38;
				--v5;
			} while (v5);
			goto LABEL_27;
		}
		v40 = v9 + v7 * v8;
		*((uint16_t*)param_buffer + 568) = v7 + (v10 >> 4);
		if (v40 < 0x100000000i64)
		{
			v41 = *((uint64_t*)param_buffer + 3);
			v42 = *(unsigned int*)((v41 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
			*((uint64_t*)param_buffer + 3) = v41 + 4;
			v40 = v42 | (v40 << 32);
		}
		*(uint64_t*)param_buffer = *((uint64_t*)param_buffer + 1);
		*((uint64_t*)param_buffer + 1) = v40;
	}
	v43 = v2;
	v44 = ((unsigned int)v3 >> 4) | (16 * (unsigned __int8)(v2 & param_buffer[1156]));
	v45 = (__m128i*) & param_buffer[34 * v44 + 1160];
	v46 = _mm_loadu_si128(v45);
	v47 = _mm_loadu_si128(v45 + 1);
	v48 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), 0);
	v49 = _mm_unpacklo_epi16(v48, v48);
	v50 = _mm_cmpgt_epi16(v46, v49);
	v51 = _mm_cmpgt_epi16(v47, v49);
	v52 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v50, v51)));
	v53 = 15 - v52;
	v100 = 15 - v52;
	v54 = v45->m128i_u16[15 - v52];
	v55 = (*(uint64_t*)param_buffer & 0x7FFFi64) + (*(uint64_t*)param_buffer >> 15) * (v45->m128i_u16[16 - v52] - (unsigned int)v54) - v54;
	if (v55 < 0x100000000i64)
	{
		v56 = *((uint64_t*)param_buffer + 3);
		v57 = *(unsigned int*)((v56 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
		*((uint64_t*)param_buffer + 3) = v56 + 4;
		v55 = v57 | (v55 << 32);
	}
	v58 = _mm_load_si128(&m2);
	v59 = _mm_load_si128(&m3);
	v60 = _mm_load_si128(&m1);
	v61 = _mm_load_si128(&m4);
	v62 = _mm_load_si128(&m5);
	v63 = _mm_load_si128(&m6);
	*(uint64_t*)param_buffer = *((uint64_t*)param_buffer + 1);
	*((uint64_t*)param_buffer + 1) = v55;
	*v45 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v50, v58), v59), v46), 7u), v46);
	v45[1] = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v51, v58), v60), v47), 7u), v47);
	v64 = param_buffer[1156];
	LOBYTE(v45) = 15 - v52;
	v65 = 16 * v64;
	if (v6 != 1)
	{
		v66 = v6 - 1;
		LODWORD(v45) = v53;
		while (1)
		{
			v44 = v53 + (v65 & (v44 + 16));
			v67 = (__m128i*) & param_buffer[34 * v44 + 1160];
			v68 = _mm_loadu_si128(v67);
			v69 = _mm_loadu_si128(v67 + 1);
			v102 = v3 & 0xF | (16 * ((unsigned int)v45 | (16 * (unsigned __int8)(param_buffer[1156] & v43))));
			v101 = (__m128i*) & param_buffer[34 * v102 + 9864];
			v70 = _mm_loadu_si128(v101);
			v71 = _mm_loadu_si128(v101 + 1);
			v72 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), 0);
			v73 = _mm_unpacklo_epi16(v72, v72);
			v74 = _mm_cmpgt_epi16(v70, v73);
			v75 = _mm_cmpgt_epi16(v71, v73);
			v76 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*((uint32_t*)param_buffer + 2) & 0x7FFF), 0);
			v77 = _mm_unpacklo_epi16(v76, v76);
			v78 = _mm_cmpgt_epi16(v68, v77);
			v79 = _mm_cmpgt_epi16(v69, v77);
			_BitScanForward((unsigned long*)&v65, _mm_movemask_epi8(_mm_packs_epi16(v74, v75)) | 0x10000); // unsigned int cast before
			LODWORD(v102) = v65;
			_BitScanForward((unsigned long*)&v81, _mm_movemask_epi8(_mm_packs_epi16(v78, v79)) | 0x10000); // unsigned int before cast
			v82 = v101->m128i_u16[v65 - 1];
			v45 = (__m128i*)(v81 - 1);
			v3 = (*(uint64_t*)param_buffer & 0x7FFFi64) + (*(uint64_t*)param_buffer >> 15) * (v101->m128i_u16[v102] - (unsigned int)v82) - v82;
			v83 = (*((uint64_t*)param_buffer + 1) & 0x7FFFi64)
				+ (*((uint64_t*)param_buffer + 1) >> 15)
				* (*(unsigned __int16*)((char*)v67->m128i_u16 + (int)(2 * v81)) - (unsigned int)v67->m128i_u16[(uint64_t)v45])
				- v67->m128i_u16[(uint64_t)v45];
			if (v3 < 0x100000000i64)
			{
				v84 = *((uint64_t*)param_buffer + 3);
				v85 = *(unsigned int*)((v84 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
				*((uint64_t*)param_buffer + 3) = v84 + 4;
				v3 = v85 | (v3 << 32);
			}
			if (v83 < 0x100000000i64)
			{
				v86 = *((uint64_t*)param_buffer + 3);
				v87 = *(unsigned int*)((v86 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
				*((uint64_t*)param_buffer + 3) = v86 + 4;
				v83 = v87 | (v83 << 32);
			}
			*(uint64_t*)param_buffer = v3;
			*((uint64_t*)param_buffer + 1) = v83;
			v88 = (v65 - 1) | (16 * v100);
			v100 = (char)v45;
			LOBYTE(v3) = v88;
			*v101 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v74, v62), v63), v70), 6u), v70);
			++v43;
			v53 = (unsigned int)v45;
			v101[1] = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v75, v62), v61), v71), 6u), v71);
			*v67 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v78, v58), v59), v68), 7u), v68);
			v67[1] = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v79, v58), v60), v69), 7u), v69);
			*v4++ = v88;
			if (!--v66)
				break;
			v65 = 16 * v64;
		}
		v64 = param_buffer[1156];
	}
	v20 = (__m128i*) & param_buffer[34 * (v3 & 0xF | (16 * (v53 | (16 * (unsigned __int8)(v64 & v43))))) + 9864];
	v89 = _mm_loadu_si128(v20);
	v90 = _mm_loadu_si128(v20 + 1);
	v91 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), 0);
	v92 = _mm_unpacklo_epi16(v91, v91);
	v93 = _mm_cmpgt_epi16(v89, v92);
	v94 = _mm_cmpgt_epi16(v90, v92);
	v95 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v93, v94)));
	v96 = v20->m128i_u16[15 - v95];
	v97 = (*(uint64_t*)param_buffer & 0x7FFFi64) + (*(uint64_t*)param_buffer >> 15) * (v20->m128i_u16[16 - v95] - (unsigned int)v96) - v96;
	if (v97 < 0x100000000i64)
	{
		v98 = *((uint64_t*)param_buffer + 3);
		v99 = *(unsigned int*)((v98 & *((uint64_t*)param_buffer + 4)) + *((uint64_t*)param_buffer + 2));
		*((uint64_t*)param_buffer + 3) = v98 + 4;
		v97 = v99 | (v97 << 32);
	}
	result = *((uint64_t*)param_buffer + 1);
	*(uint64_t*)param_buffer = result;
	*((uint64_t*)param_buffer + 1) = v97;
	*v20 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v93, v62), v63), v89), 6u), v89);
	v20[1] = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v94, v62), v61), v90), 6u), v90);
	*v4 = (16 * (uint8_t)v45) | (15 - v95);
	LOBYTE(v20) = (16 * (uint8_t)v45) | (15 - v95);
LABEL_27:
	param_buffer[1138] = (unsigned __int8)v20;
	return result;
}

int64_t RTech::InitSnowflakeDecompState(int64_t param_buffer, int64_t data_buffer, uint64_t buffer_size)
{
    // dmp location: 49 8B 0F 48 89 9E ? ? ? ? 
    // retail location: sig to containing function: (+0x11) E8 ? ? ? ? 4C 8B 53 38 44 8D 47 10

    uint32_t v2; // eax
    int v3; // edi
    uint64_t v4; // rax
    int32_t v6; // rcx
    int v7; // ebx
    char v8; // al
    uint8_t v9; // r14
    uint32_t v10; // ecx
    int64_t result; // rax
    int v12; // eax
    uint32_t v13; // esi
    int64_t v14; // rdi
    int v15; // ebx
    int64_t v16; // rcx
    int v17; // er8
    char v18[256]; // [rsp+30h] [rbp-128h] BYREF

    *(uint64_t*)(param_buffer + 0x246B0) = data_buffer;
    *(uint64_t*)(param_buffer + 0x246B8) = buffer_size;
    *(uint64_t*)(param_buffer + 0x246C8) = -1;
    *(uint64_t*)(param_buffer + 0x40) = 0;

    v2 = sub_7FF7FC23B960(param_buffer, 6);
    v3 = -1;
    v4 = (sub_7FF7FC23B960(param_buffer, v2) | (1i64 << v2)) - 1;
    *(uint64_t*)(param_buffer + 149144) = v4;
    if (_BitScanReverse64((unsigned long*)&v6, (v4 >> 6) + 100 + v4)) // Had to cast v6 to unsigned long here, it might cause issues. THIS MAY RUIN SMTH KEEP IN MIND.
        v3 = v6;
    *(uint64_t*)(param_buffer + 149160) = sub_7FF7FC23B960(param_buffer, (unsigned int)(v3 + 1));
    v7 = sub_7FF7FC23B960(param_buffer, 4);
    v8 = sub_7FF7FC23B960(param_buffer, 4);
    *(uint32_t*)(param_buffer + 149136) = 1 << ((unsigned __int8)v7 + 9);
    *(uint32_t*)(param_buffer + 149140) = 1 << (v8 + 9);
    *(uint32_t*)(param_buffer + 149132) = v7 + 8;
    v9 = sub_7FF7FC23B960(param_buffer, 2);
    v10 = sub_7FF7FC23B960(param_buffer, 3) | 8;
    if (v10 <= 8)
        return sub_7FF7FC23C880(param_buffer + 48, v9, 0i64);

    v12 = *(uint32_t*)(param_buffer + 44);
    v13 = -v12 & 0x1F;
    v14 = *(uint64_t*)(param_buffer + 149184) - (v12 != 0 ? 4 : 0);
    v15 = sub_7FF7FC23C680(v10, (unsigned __int64*)(*(uint64_t*)(param_buffer + 149168) + v14 + ((unsigned __int64)v13 >> 3)), -(char)v12 & 7, v10, v18);

    result = sub_7FF7FC23C880(param_buffer + 48, v9, (__int64)v18);
    int test = *(uint32_t*)(param_buffer + 0x484);
    v16 = v14 + 4 * ((unsigned __int64)(v15 + v13 + 31) >> 5);
    *(uint64_t*)(param_buffer + 149184) = v16;
    v17 = (v15 + v13) & 0x1F;
    if (v17)
    {
        result = *(uint64_t*)(param_buffer + 149168);
        *(uint32_t*)(param_buffer + 40) = *(uint32_t*)(result + v16 - 4) >> v17;
        *(uint32_t*)(param_buffer + 44) = 32 - v17;
    }
    else
    {
        *(uint64_t*)(param_buffer + 40) = 0i64;
    }

    return result;
}

bool RTech::DecompressSnowflake(int64_t param_buffer, uint64_t data_size, uint64_t buffer_size)
{
	unsigned __int64 v3; // rsi
	size_t v4; // r8
	unsigned __int64 v5; // rax
	int v8; // ebp
	unsigned __int64 v9; // rcx
	__m128i v10; // xmm6
	__m128i v11; // xmm13
	__m128i v12; // xmm7
	__m128i v13; // xmm12
	__m128i v14; // xmm15
	unsigned __int64 v15; // r8
	int v16; // eax
	__int64 v17; // rax
	int v18; // er15
	unsigned __int64 v19; // r10
	unsigned __int64 v20; // rdx
	__m128i v21; // xmm3
	__m128i v22; // xmm4
	__m128i v23; // xmm0
	__m128i v24; // xmm1
	__m128i v25; // xmm2
	unsigned int v26; // ecx
	unsigned int v27; // edi
	__int64 v28; // r8
	unsigned __int64 v29; // r9
	unsigned __int64 v30; // rcx
	unsigned __int64 v31; // r8
	__m128i v32; // xmm2
	__m128i v33; // xmm1
	int v34; // eax
	int v35; // esi
	__int64 v36; // rdx
	unsigned __int64 v37; // r9
	__int64 v38; // rdx
	unsigned __int64 v39; // r8
	__m128i v40; // xmm1
	__m128i v41; // xmm2
	__m128i v42; // xmm1
	int v43; // eax
	unsigned int v44; // edi
	__int64 v45; // rdx
	unsigned __int64 v46; // r9
	__int64 v47; // rdx
	__int64 v48; // r8
	unsigned __int64 v49; // r11
	__m128i v50; // xmm1
	unsigned __int64 v51; // r9
	unsigned __int64 v52; // r10
	__int64 v53; // rdx
	__int64 v54; // r8
	char v55; // al
	unsigned __int64 v56; // r9
	__int64 v57; // rdx
	__int64 v58; // r8
	unsigned int v59; // edx
	__m128i v60; // xmm3
	__m128i v61; // xmm4
	__m128i v62; // xmm0
	__m128i v63; // xmm1
	__m128i v64; // xmm2
	unsigned int v65; // ecx
	unsigned int v66; // er10
	__int64 v67; // r8
	unsigned __int64 v68; // r9
	__int64 v69; // rdx
	__int64 v70; // r8
	unsigned __int64 v71; // r11
	__m128i v72; // xmm14
	unsigned __int64 v73; // rsi
	unsigned __int64 v74; // rdi
	__m128i v75; // xmm3
	__m128i v76; // xmm4
	__m128i v77; // xmm0
	__m128i v78; // xmm1
	__m128i v79; // xmm2
	unsigned int v80; // ecx
	__int64 v81; // rdx
	unsigned __int64 v82; // r9
	__int64 v83; // rdx
	__int64 v84; // r8
	__m128i v85; // xmm2
	__m128i v86; // xmm8
	__m128i v87; // xmm9
	__m128i v88; // xmm10
	__m128i v89; // xmm11
	__m128i v90; // xmm1
	__m128i v91; // xmm1
	__m128i v92; // xmm2
	__m128i v93; // xmm3
	__m128i v94; // xmm1
	__m128i v95; // xmm1
	__m128i v96; // xmm4
	__m128i v97; // xmm5
	unsigned int v98; // edi
	unsigned int v99; // edx
	char v100; // si
	__int64 v101; // rcx
	__int64 v102; // rbp
	__int64 v103; // r8
	unsigned __int64 v104; // r10
	unsigned __int64 v105; // r11
	__int64 v106; // rdx
	__int64 v107; // r8
	__int64 v108; // rdx
	__int64 v109; // r8
	__m128i v110; // xmm4
	__m128i v111; // xmm5
	int v112; // er11
	unsigned __int64 v113; // r10
	__int64 v114; // r8
	__int64 v115; // r9
	unsigned __int8 v116; // cl
	__int64 v117; // rbp
	__int64 v118; // r14
	__m128i v119; // xmm2
	__m128i v120; // xmm1
	int v121; // er10
	__int64 v122; // r8
	unsigned __int64 v123; // r9
	__int64 v124; // rdx
	__int64 v125; // r8
	__m128i v126; // xmm1
	__m128i v127; // xmm3
	__m128i v128; // xmm4
	__m128i v129; // xmm1
	__m128i v130; // xmm2
	__m128i v131; // xmm0
	__m128i v132; // xmm1
	int v133; // er9
	int v134; // er15
	__int64 v135; // r11
	int v136; // eax
	__int64 v137; // rdi
	unsigned __int64 v138; // rbp
	unsigned __int64 v139; // r9
	__int64 v140; // rdx
	__int64 v141; // r10
	__int64 v142; // rdx
	__m128i v143; // xmm1
	__m128i v144; // xmm0
	unsigned int v145; // er8
	__int64 v146; // r11
	__m128i v147; // xmm3
	int v148; // eax
	__int64 v149; // r10
	__int64 v150; // r8
	unsigned __int64 v151; // r9
	__int64 v152; // rdx
	__int64 v153; // r8
	__m128i v154; // xmm0
	__m128i v155; // xmm4
	__int64 v156; // r15
	__m128i v157; // xmm3
	char v158; // bp
	__m128i v159; // xmm1
	__m128i v160; // xmm2
	__m128i v161; // xmm0
	__m128i v162; // xmm1
	int v163; // er10
	__int64 v164; // r13
	__int64 v165; // r9
	int v166; // eax
	__int64 v167; // rdi
	unsigned __int64 v168; // r10
	unsigned __int64 v169; // r9
	__int64 v170; // rdx
	__int64 v171; // r11
	__int64 v172; // rdx
	__m128i v173; // xmm0
	__m128i v174; // xmm1
	unsigned __int64 v175; // r10
	int v176; // er11
	__int64 v177; // r8
	__int64 v178; // r9
	unsigned __int8 v179; // cl
	__m128i v180; // xmm1
	__int64 v181; // rcx
	__int64 v182; // rdx
	__int64 v183; // r10
	__int64 v184; // r9
	unsigned int v185; // eax
	unsigned int v186; // eax
	int v187; // edx
	__int64 v188; // rdx
	unsigned __int64 v189; // rax
	__int64 v190; // r8
	unsigned int v191; // [rsp+20h] [rbp-F8h]
	int v192; // [rsp+20h] [rbp-F8h]
	unsigned __int64 v193; // [rsp+28h] [rbp-F0h]
	signed __int64 v195; // [rsp+38h] [rbp-E0h] BYREF

	__m128i m1 = _mm_set_epi32(0x1000100, 0x1000100, 0x1000100, 0x1000100);
	__m128i m2 = _mm_set_epi32(0x3f003b1, 0x3720333, 0x2f402b5, 0x2760237);
	__m128i m3 = _mm_set_epi32(0x7c107c10, 0x7c107c10, 0x7c107c10, 0x7c107c10);
	__m128i m4 = _mm_set_epi32(0x1f801b9, 0x17a013b, 0xfc00bd, 0x7e003f);
	__m128i m5 = _mm_set_epi32(0xd900ba, 0x9b007c, 0x5d003e, 0x1f0000);

	__m128i m6 = _mm_set_epi32(0xf0e0d0c, 0xb0a0908, 0x7060504, 0x3020100);
	__m128i m7 = _mm_set_epi32(0xf0e0d0c, 0xb0a0908, 0x3020100, 0x7060504);
	__m128i m8 = _mm_set_epi32(0xf0e0d0c, 0x7060504, 0x3020100, 0xb0a0908);
	__m128i m9 = _mm_set_epi32(0xb0a0908, 0x7060504, 0x3020100, 0xf0e0d0c);
	__m128i m10 = _mm_set_epi32(0xb0a0908, 0x7060504, 0x3020100, 0xffffffff);
	__m128i m11 = _mm_set_epi32(0, 0, 0, 0);
	__m128i m_arr[6] = { m6, m7, m8, m9, m10, m11 };

	static int cycle = 0; // This will stay in honor of my 20 hours of total debugging this.

	v3 = buffer_size;
	v4 = *(uint64_t*)(param_buffer + 149144);
	v5 = data_size;
	v193 = data_size;
	if (v4 < 0x40)
	{
		memmove(
			*(void**)(param_buffer + 149200),
			(const void*)(*(uint64_t*)(param_buffer + 149168) + *(uint64_t*)(param_buffer + 149184)),
			v4);
		return 1;
	}
	if (data_size >= *(uint64_t*)(param_buffer + 149160))
		goto LABEL_6;
	if (data_size >= 0x21C)
	{
		v5 = data_size - 540;
		v193 = data_size - 540;
	LABEL_6:
		v8 = *(uint32_t*)(param_buffer + 149128);
		v9 = *(uint64_t*)(param_buffer + 149152);
		if (v3 >= v9)
		{
			v10 = _mm_load_si128((const __m128i*) & m1);
			v11 = _mm_load_si128((const __m128i*) & m2);
			v12 = _mm_load_si128((const __m128i*) & m3);
			v13 = _mm_load_si128((const __m128i*) & m4);
			v14 = _mm_load_si128((const __m128i*) & m5);
			while (!v8)
			{
				v15 = *(uint64_t*)(param_buffer + 149208);
				if (v9 - v15 <= 8)
				{
					if (v15 < v9)
					{
						do
						{
							*(uint8_t*)(v15 + *(uint64_t*)(param_buffer + 149200)) = *(uint8_t*)((*(uint64_t*)(param_buffer + 149192) & *(uint64_t*)(param_buffer + 149184))
								+ *(uint64_t*)(param_buffer + 149168));
							v190 = *(uint64_t*)(param_buffer + 149208);
							++* (uint64_t*)(param_buffer + 149184);
							v15 = v190 + 1;
							*(uint64_t*)(param_buffer + 149208) = v15;
						} while (v15 < *(uint64_t*)(param_buffer + 149152));
					}
					return 1;
				}
				v8 = sub_7FF7FC23B960(param_buffer, *(uint32_t*)(param_buffer + 149132));
				v191 = sub_7FF7FC23B960(param_buffer, 5u) | 0x20;
				v195 = sub_7FF7FC23B960(param_buffer, v191);
				_bittestandset64(&v195, v191);
				v16 = sub_7FF7FC23B960(param_buffer, 5u);
				LOBYTE(v191) = v16 | 0x20;
				v17 = sub_7FF7FC23B960(param_buffer, v16 | 0x20u);
				*(uint64_t*)param_buffer = v195;
				*(uint64_t*)(param_buffer + 24) = *(uint64_t*)(param_buffer + 149184);
				*(uint64_t*)(param_buffer + 8) = v17 | (1i64 << v191);
				*(uint64_t*)(param_buffer + 16) = *(uint64_t*)(param_buffer + 149168);
				*(uint64_t*)(param_buffer + 32) = *(uint64_t*)(param_buffer + 149192);
				if (v8)
				{
					v5 = v193;
					break;
				}
			LABEL_80:
				v187 = *(uint32_t*)(param_buffer + 149152) - *(uint32_t*)(param_buffer + 149208);
				if (v187 != 8)
				{
					sub_7FF7FC23CD20((unsigned __int8*)param_buffer, v187 - 8);
					v10 = _mm_load_si128(&m1);
					v11 = _mm_load_si128(&m2);
					v12 = _mm_load_si128(&m3);
					v13 = _mm_load_si128(&m4);
					v14 = _mm_load_si128(&m5);
				}
				*(uint32_t*)(*(uint64_t*)(param_buffer + 149208) + *(uint64_t*)(param_buffer + 149200)) = *(uint32_t*)(param_buffer + 8);
				*(uint32_t*)(*(uint64_t*)(param_buffer + 149208) + *(uint64_t*)(param_buffer + 149200) + 4i64) = *(uint32_t*)param_buffer;
				v188 = *(uint64_t*)(param_buffer + 149152);
				v189 = *(uint64_t*)(param_buffer + 149144);
				v9 = v188 + *(unsigned int*)(param_buffer + 149136);
				*(uint64_t*)(param_buffer + 149208) = v188;
				*(uint64_t*)(param_buffer + 149152) = v9;
				if (v9 > v189)
				{
					if (v188 == v189)
						return 1;
					*(uint64_t*)(param_buffer + 149152) = v189;
					v9 = v189;
				}
				*(uint64_t*)(param_buffer + 149184) = *(uint64_t*)(param_buffer + 24);
				if (v3 < v9)
					return 0;
				v5 = v193;
			}
		LABEL_12:
			v18 = v8 - 1;
			v192 = v8 - 1;
			while (1)
			{
				v19 = *(uint64_t*)(param_buffer + 24);
				if (v19 > v5)
					break;
				v20 = *(uint64_t*)param_buffer;
				v21 = _mm_loadu_si128((const __m128i*)(param_buffer + 50));
				v22 = _mm_loadu_si128((const __m128i*)(param_buffer + 66));
				v23 = _mm_shuffle_epi8(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), v10);
				v24 = _mm_cmpgt_epi16(v21, v23);
				v25 = _mm_cmpgt_epi16(v22, v23);
				v26 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v24, v25)));
				v27 = 16 - v26;
				v195 = 16 - v26;
				v28 = *(unsigned __int16*)(param_buffer + 2 * v195 + 48);
				v29 = (v20 & 0x7FFF)
					+ (v20 >> 15) * (*(unsigned __int16*)(param_buffer + 2i64 * (17 - v26) + 48) - (unsigned int)v28)
					- v28;
				if (v29 < 0x100000000i64)
				{
					v30 = v19 & *(uint64_t*)(param_buffer + 32);
					v19 += 4i64;
					v29 = *(unsigned int*)(v30 + *(uint64_t*)(param_buffer + 16)) | (v29 << 32);
					*(uint64_t*)(param_buffer + 24) = v19;
				}
				v31 = *(uint64_t*)(param_buffer + 8);
				*(uint64_t*)param_buffer = v31;
				*(uint64_t*)(param_buffer + 8) = v29;
				*(__m128i*)(param_buffer + 50) = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v24, v12), v13), v21),
						6u),
					v21);
				*(__m128i*)(param_buffer + 66) = _mm_add_epi16(
					_mm_srai_epi16(
						_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v25, v12), v11), v22),
						6u),
					v22);
				if (!v27)
					goto LABEL_35;
				if (v27 == 16)
				{
					v32 = _mm_loadu_si128((const __m128i*)(param_buffer + 104));
					v33 = _mm_cmpgt_epi16(v32, _mm_shuffle_epi8(_mm_cvtsi32_si128(v31 & 0x7FFF), v10));
					v34 = (int)__popcnt(_mm_movemask_epi8(v33)) / 2;
					v35 = 7 - v34;
					v36 = *(unsigned __int16*)(param_buffer + 2i64 * (unsigned int)(7 - v34) + 104);
					v37 = (v31 & 0x7FFF)
						+ (v31 >> 15)
						* (*(unsigned __int16*)(param_buffer + 2i64 * (unsigned int)(8 - v34) + 104) - (unsigned int)v36)
						- v36;
					if (v37 < 0x100000000i64)
					{
						v38 = *(unsigned int*)((*(uint64_t*)(param_buffer + 32) & v19) + *(uint64_t*)(param_buffer + 16));
						*(uint64_t*)(param_buffer + 24) = v19 + 4;
						v37 = v38 | (v37 << 32);
					}
					v39 = *(uint64_t*)(param_buffer + 8);
					v40 = _mm_add_epi16(_mm_and_si128(v33, (__m128i)_mm_set_epi32(0x7f277f27, 0x7f277f27, 0x7f277f27, 0x7f270000)), v14);
					*(uint64_t*)(param_buffer + 8) = v37;
					*(uint64_t*)param_buffer = v39;
					*(__m128i*)(param_buffer + 104) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v40, v32), 5u), v32);
					v41 = _mm_loadu_si128((const __m128i*)(param_buffer + 86));
					v42 = _mm_cmpgt_epi16(v41, _mm_shuffle_epi8(_mm_cvtsi32_si128(v39 & 0x3FFF), v10));
					v43 = (int)__popcnt(_mm_movemask_epi8(v42)) / 2;
					v44 = 8 - v43;
					v195 = (unsigned int)(8 - v43);
					v45 = *(unsigned __int16*)(param_buffer + 2 * v195 + 84);
					v46 = (v39 & 0x3FFF)
						+ (v39 >> 14)
						* (*(unsigned __int16*)(param_buffer + 2i64 * (unsigned int)(9 - v43) + 84) - (unsigned int)v45)
						- v45;
					if (v46 < 0x100000000i64)
					{
						v47 = *(uint64_t*)(param_buffer + 24);
						v48 = *(unsigned int*)((v47 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
						*(uint64_t*)(param_buffer + 24) = v47 + 4;
						v46 = v48 | (v46 << 32);
					}
					v49 = *(uint64_t*)(param_buffer + 8);
					v50 = _mm_add_epi16(_mm_and_si128(v42, (__m128i)_mm_set_epi32(0x3e083e08, 0x3e083e08, 0x3e083e08, 0x3e083e08)), v13);
					*(uint64_t*)param_buffer = v49;
					*(uint64_t*)(param_buffer + 8) = v46;
					*(__m128i*)(param_buffer + 86) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v50, v41), 6u), v41);
					if (v44 <= 5)
					{
						v59 = v35 + 8 * (v44 + 2);
						v27 = v59;
					}
					else
					{
						v51 = v49;
						if (v44 == 16)
						{
							v52 = v49 >> 3;
							if (v49 >> 3 < 0x100000000i64)
							{
								v53 = *(uint64_t*)(param_buffer + 24);
								v54 = *(unsigned int*)((v53 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v53 + 4;
								v52 = v54 | (v52 << 32);
							}
							v51 = *(uint64_t*)(param_buffer + 8);
							*(uint64_t*)param_buffer = v51;
							*(uint64_t*)(param_buffer + 8) = v52;
							v55 = (v49 & 7) + 16;
							LODWORD(v49) = v51;
							LOBYTE(v195) = v55;
							LOBYTE(v44) = v55;
						}
						else
						{
							v55 = v195;
						}
						v56 = v51 >> ((unsigned __int8)v44 - 3);
						if (v56 < 0x100000000i64)
						{
							v57 = *(uint64_t*)(param_buffer + 24);
							v58 = *(unsigned int*)((v57 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
							*(uint64_t*)(param_buffer + 24) = v57 + 4;
							v56 = v58 | (v56 << 32);
						}
						*(uint64_t*)param_buffer = *(uint64_t*)(param_buffer + 8);
						v59 = v35 + ((8 * (v49 & ((1 << (v55 - 3)) - 1))) | (1 << v195));
						*(uint64_t*)(param_buffer + 8) = v56;
						v27 = v59;
					}
				}
				else
				{
					v59 = v195;
				}
				sub_7FF7FC23CD20((unsigned __int8*)param_buffer, v59);
				v10 = _mm_load_si128((const __m128i*) & m1);
				v11 = _mm_load_si128((const __m128i*) & m2);
				v12 = _mm_load_si128((const __m128i*) & m3);
				v13 = _mm_load_si128((const __m128i*) & m4);
				v14 = _mm_load_si128((const __m128i*) & m5);
				if (v27 != 511)
				{
				LABEL_35:
					v60 = _mm_loadu_si128((const __m128i*)(param_buffer + 124));
					v61 = _mm_loadu_si128((const __m128i*)(param_buffer + 140));
					v62 = _mm_shuffle_epi8(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), v10);
					v63 = _mm_cmpgt_epi16(v60, v62);
					v64 = _mm_cmpgt_epi16(v61, v62);
					v65 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v63, v64)));
					v66 = 16 - v65;
					v67 = *(unsigned __int16*)(param_buffer + 2i64 * (16 - v65) + 122);
					v68 = (*(uint64_t*)param_buffer & 0x7FFFi64)
						+ (*(uint64_t*)param_buffer >> 15)
						* (*(unsigned __int16*)(param_buffer + 2i64 * (17 - v65) + 122) - (unsigned int)v67)
						- v67;
					if (v68 < 0x100000000i64)
					{
						v69 = *(uint64_t*)(param_buffer + 24);
						v70 = *(unsigned int*)((v69 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
						*(uint64_t*)(param_buffer + 24) = v69 + 4;
						v68 = v70 | (v68 << 32);
					}
					v71 = *(uint64_t*)(param_buffer + 8);
					__m128i unkM = _mm_set_epi32(0x1b9017a, 0x13b00fc, 0xbd007e, 0x3f0000);
					v72 = _mm_load_si128(&unkM);
					v73 = v66 + 3;
					*(uint64_t*)param_buffer = v71;
					*(uint64_t*)(param_buffer + 8) = v68;
					v74 = v71;
					*(__m128i*)(param_buffer + 124) = _mm_add_epi16(
						_mm_srai_epi16(
							_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v63, v12), v13), v60),
							6u),
						v60);
					*(__m128i*)(param_buffer + 140) = _mm_add_epi16(
						_mm_srai_epi16(
							_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v64, v12), v11), v61),
							6u),
						v61);
					if (v66 + 3 >= 0x12)
					{
						if (v66 == 15)
						{
							v75 = _mm_loadu_si128((const __m128i*)(param_buffer + 160));
							v76 = _mm_loadu_si128((const __m128i*)(param_buffer + 176));
							v77 = _mm_shuffle_epi8(_mm_cvtsi32_si128(v71 & 0x3FFF), v10);
							v78 = _mm_cmpgt_epi16(v75, v77);
							v79 = _mm_cmpgt_epi16(v76, v77);
							v80 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v78, v79)));
							v81 = *(unsigned __int16*)(param_buffer + 2i64 * (16 - v80) + 158);
							v82 = (v71 & 0x3FFF)
								+ (v71 >> 14) * (*(unsigned __int16*)(param_buffer + 2i64 * (17 - v80) + 158) - (unsigned int)v81)
								- v81;
							if (v82 < 0x100000000i64)
							{
								v83 = *(uint64_t*)(param_buffer + 24);
								v84 = *(unsigned int*)((v83 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v83 + 4;
								v82 = v84 | (v82 << 32);
							}
							v73 = 16 - v80 + 18;
							v74 = *(uint64_t*)(param_buffer + 8);
							v85 = _mm_add_epi16(
								_mm_srai_epi16(
									_mm_sub_epi16(
										_mm_add_epi16(
											_mm_and_si128(v79, (__m128i)_mm_set_epi32(0x3e103e10, 0x3e103e10, 0x3e103e10, 0x3e103e10)),
											(__m128i)_mm_set_epi32(0x1f001d1, 0x1b20193, 0x1740155, 0x1360117)),
										v76),
									5u),
								v76);
							*(__m128i*)(param_buffer + 160) = _mm_add_epi16(
								_mm_srai_epi16(
									_mm_sub_epi16(
										_mm_add_epi16(
											_mm_and_si128(v78, (__m128i)_mm_set_epi32(0x3e103e10, 0x3e103e10, 0x3e103e10, 0x3e103e10)),
											(__m128i)_mm_set_epi32(0xf800d9, 0xba009b, 0x7c005d, 0x3e001f)),
										v75),
									5u),
								v75);
							*(__m128i*)(param_buffer + 176) = v85;
							*(uint64_t*)(param_buffer + 8) = v82;
						}
						else
						{
							v86 = _mm_loadu_si128((const __m128i*)(param_buffer + 196));
							v87 = _mm_loadu_si128((const __m128i*)(param_buffer + 212));
							v88 = _mm_loadu_si128((const __m128i*)(param_buffer + 230));
							v89 = _mm_loadu_si128((const __m128i*)(param_buffer + 246));
							v90 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(v71 & 0x7FFF), 0);
							v91 = _mm_unpacklo_epi16(v90, v90);
							v92 = _mm_cmpgt_epi16(v86, v91);
							v93 = _mm_cmpgt_epi16(v87, v91);
							v94 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(v68 & 0x3FFF), 0);
							v95 = _mm_unpacklo_epi16(v94, v94);
							v96 = _mm_cmpgt_epi16(v88, v95);
							v97 = _mm_cmpgt_epi16(v89, v95);
							v98 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v92, v93)));
							v99 = __popcnt(_mm_movemask_epi8(_mm_packs_epi16(v96, v97)));
							v100 = 16 - v98;
							v101 = *(unsigned __int16*)(param_buffer + 2i64 * (16 - v98) + 194);
							v102 = 15 - v99;
							v103 = *(unsigned __int16*)(param_buffer + 2 * v102 + 230);
							v104 = (v71 & 0x7FFF)
								+ (v71 >> 15) * (*(unsigned __int16*)(param_buffer + 2i64 * (17 - v98) + 194) - (unsigned int)v101)
								- v101;
							v105 = (v68 & 0x3FFF)
								+ (v68 >> 14) * (*(unsigned __int16*)(param_buffer + 2i64 * (16 - v99) + 230) - (unsigned int)v103)
								- v103;
							if (v104 < 0x100000000i64)
							{
								v106 = *(uint64_t*)(param_buffer + 24);
								v107 = *(unsigned int*)((v106 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v106 + 4;
								v104 = v107 | (v104 << 32);
							}
							if (v105 < 0x100000000i64)
							{
								v108 = *(uint64_t*)(param_buffer + 24);
								v109 = *(unsigned int*)((v108 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v108 + 4;
								v105 = v109 | (v105 << 32);
							}
							v110 = _mm_and_si128(v96, (__m128i)_mm_set_epi32(0x3c4f3c4f, 0x3c4f3c4f, 0x3c4f3c4f, 0x3c4f3c4f));
							v111 = _mm_add_epi16(_mm_and_si128(v97, (__m128i)_mm_set_epi32(0x3c4f3c4f, 0x3c4f3c4f, 0x3c4f3c4f, 0x3c4f3c4f)), (__m128i)_mm_set_epi32(0x3b10372, 0x33302f4, 0x2b50276, 0x23701f8));
							*(uint64_t*)(param_buffer + 8) = v105;
							*(uint64_t*)param_buffer = v104;
							v112 = v104 & ((1 << (17 - v98)) - 1);
							*(__m128i*)(param_buffer + 196) = _mm_add_epi16(
								_mm_srai_epi16(
									_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v92, v12), v13), v86),
									6u),
								v86);
							*(__m128i*)(param_buffer + 212) = _mm_add_epi16(
								_mm_srai_epi16(
									_mm_sub_epi16(_mm_add_epi16(_mm_and_si128(v93, v12), v11), v87),
									6u),
								v87);
							v113 = v104 >> (17 - (unsigned __int8)v98);
							*(__m128i*)(param_buffer + 230) = _mm_add_epi16(
								_mm_srai_epi16(_mm_sub_epi16(_mm_add_epi16(v110, v72), v88), 6u),
								v88);
							*(__m128i*)(param_buffer + 246) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v111, v89), 6u), v89);
							if (v113 < 0x100000000i64)
							{
								v114 = *(uint64_t*)(param_buffer + 24);
								v115 = *(unsigned int*)((v114 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v114 + 4;
								v113 = v115 | (v113 << 32);
							}
							v74 = *(uint64_t*)(param_buffer + 8);
							*(uint64_t*)(param_buffer + 8) = v113;
							v73 = v102 | (unsigned int)(16 * v112) | (unsigned __int64)(unsigned int)(32 << v100);
						}
						LOWORD(v71) = v74;
						*(uint64_t*)param_buffer = v74;
					}
					v116 = 3;
					if ((unsigned int)(v73 - 3) < 3)
						v116 = v73 - 3;
					v117 = 218i64 * v116;
					v118 = param_buffer + v117;
					v119 = _mm_loadu_si128((const __m128i*)(param_buffer + v117 + 264));
					v120 = _mm_cmpgt_epi16(v119, _mm_shuffle_epi8(_mm_cvtsi32_si128(v71 & 0xFFF), v10));
					v121 = (int)__popcnt(_mm_movemask_epi8(v120)) / 2;
					v122 = *(unsigned __int16*)(param_buffer + 2i64 * (unsigned int)(7 - v121) + v117 + 264);
					v123 = (v74 & 0xFFF)
						+ (v74 >> 12)
						* (*(unsigned __int16*)(param_buffer + 2i64 * (unsigned int)(8 - v121) + v117 + 264) - (unsigned int)v122)
						- v122;
					if (v123 < 0x100000000i64)
					{
						v124 = *(uint64_t*)(param_buffer + 24);
						v125 = *(unsigned int*)((v124 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
						*(uint64_t*)(param_buffer + 24) = v124 + 4;
						v123 = v125 | (v123 << 32);
					}
					v126 = _mm_add_epi16(_mm_and_si128(v120, (__m128i)_mm_set_epi32(0xf270f27, 0xf270f27, 0xf270f27, 0xf270000)), v14);
					*(uint64_t*)param_buffer = *(uint64_t*)(param_buffer + 8);
					*(uint64_t*)(param_buffer + 8) = v123;
					*(__m128i*)(v118 + 264) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v126, v119), 5u), v119);
					if ((unsigned int)(7 - v121) > 1)
					{
						v155 = _mm_loadu_si128((const __m128i*)(v118 + 368));
						v156 = v117 + param_buffer + 16i64 * (unsigned int)(5 - v121);
						v157 = _mm_loadu_si128((const __m128i*)(v156 + 386));
						v158 = 3 * (7 - v121);
						v159 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x1FFF), 0);
						v160 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)(param_buffer + 8) & 0x7FFF), 0);
						v161 = _mm_cmpgt_epi16(v157, _mm_unpacklo_epi16(v159, v159));
						v162 = _mm_cmpgt_epi16(v155, _mm_unpacklo_epi16(v160, v160));
						v163 = (int)__popcnt(_mm_movemask_epi8(v161)) / 2;
						v164 = (unsigned int)(7 - v163);
						v165 = *(unsigned __int16*)(v156 + 2 * v164 + 386);
						v166 = (int)__popcnt(_mm_movemask_epi8(v162)) / 2;
						v167 = *(unsigned __int16*)(v118 + 2i64 * (unsigned int)(7 - v166) + 368);
						v168 = (*(uint64_t*)param_buffer & 0x1FFFi64)
							+ (*(uint64_t*)param_buffer >> 13)
							* (*(unsigned __int16*)(v156 + 2i64 * (8 - v163) + 386) - (unsigned int)v165)
							- v165;
						v169 = (*(uint64_t*)(param_buffer + 8) & 0x7FFFi64)
							+ (*(uint64_t*)(param_buffer + 8) >> 15)
							* (*(unsigned __int16*)(v118 + 2i64 * (8 - v166) + 368) - (unsigned int)v167)
							- v167;
						if (v168 >= 0x100000000i64)
						{
							v171 = *(uint64_t*)(param_buffer + 24);
						}
						else
						{
							v170 = *(uint64_t*)(param_buffer + 24);
							v171 = v170 + 4;
							v168 = *(unsigned int*)((v170 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16)) | (v168 << 32);
							*(uint64_t*)(param_buffer + 24) = v170 + 4;
						}
						if (v169 < 0x100000000i64)
						{
							v172 = *(unsigned int*)((v171 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
							*(uint64_t*)(param_buffer + 24) = v171 + 4;
							v169 = v172 | (v169 << 32);
						}
						v173 = _mm_add_epi16(_mm_and_si128(v161, (__m128i)_mm_set_epi32(0x20001e86, 0x1e861e86, 0x1e861e86, 0x1e861e86)), (__m128i)_mm_set_epi32(0x17a, 0x13b00fc, 0xbd007e, 0x3f0000));
						v174 = _mm_add_epi16(_mm_and_si128(v162, (__m128i)_mm_set_epi32(0x7e477e47, 0x7e477e47, 0x7e477e47, 0x7e477e47)), v72);
						*(uint64_t*)param_buffer = v168;
						*(uint64_t*)(param_buffer + 8) = v169;
						*(__m128i*)(v156 + 386) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v173, v157), 6u), v157);
						*(__m128i*)(v118 + 368) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v174, v155), 6u), v155);
						v175 = *(uint64_t*)param_buffer >> (v158 - 3);
						v176 = *(uint32_t*)param_buffer & ((1 << (v158 - 3)) - 1);
						if (v175 < 0x100000000i64)
						{
							v177 = *(uint64_t*)(param_buffer + 24);
							v178 = *(unsigned int*)((v177 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
							*(uint64_t*)(param_buffer + 24) = v177 + 4;
							v175 = v178 | (v175 << 32);
						}
						*(uint64_t*)param_buffer = *(uint64_t*)(param_buffer + 8);
						v179 = *(uint8_t*)(param_buffer + 1139);
						*(uint64_t*)(param_buffer + 8) = v175;
						v145 = ((7 - v166) | (8 * v176) | (((uint32_t)v164 + 1) << v158)) + 1;
						v180 = _mm_slli_si128(_mm_loadu_si128((const __m128i*)(param_buffer + 1140)), 4);
						*(uint8_t*)(param_buffer + 1139) = v179 - (v179 >> 2) + 1;
						*(__m128i*)(param_buffer + 1140) = _mm_or_si128(v180, _mm_cvtsi32_si128(v145));
					LABEL_73:
						v18 = v192;
					}
					else
					{
						if (v121 == 6)
						{
							v127 = _mm_loadu_si128((const __m128i*)(v118 + 332));
							v128 = _mm_loadu_si128((const __m128i*)(v118 + 350));
							v129 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x7FFF), 0);
							v130 = _mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)(param_buffer + 8) & 0x7FFF), 0);
							v131 = _mm_cmpgt_epi16(v127, _mm_unpacklo_epi16(v129, v129));
							v132 = _mm_cmpgt_epi16(v128, _mm_unpacklo_epi16(v130, v130));
							v133 = (int)__popcnt(_mm_movemask_epi8(v131)) / 2;
							v134 = 7 - v133;
							v135 = *(unsigned __int16*)(v118 + 2i64 * (unsigned int)(7 - v133) + 332);
							v136 = (int)__popcnt(_mm_movemask_epi8(v132)) / 2;
							v137 = *(unsigned __int16*)(v118 + 2i64 * (unsigned int)(7 - v136) + 350);
							v138 = (*(uint64_t*)param_buffer & 0x7FFFi64)
								+ (*(uint64_t*)param_buffer >> 15)
								* (*(unsigned __int16*)(v118 + 2i64 * (unsigned int)(8 - v133) + 332) - (unsigned int)v135)
								- v135;
							v139 = (*(uint64_t*)(param_buffer + 8) & 0x7FFFi64)
								+ (*(uint64_t*)(param_buffer + 8) >> 15)
								* (*(unsigned __int16*)(v118 + 2i64 * (unsigned int)(8 - v136) + 350) - (unsigned int)v137)
								- v137;
							if (v138 >= 0x100000000i64)
							{
								v141 = *(uint64_t*)(param_buffer + 24);
							}
							else
							{
								v140 = *(uint64_t*)(param_buffer + 24);
								v141 = v140 + 4;
								v138 = *(unsigned int*)((v140 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16)) | (v138 << 32);
								*(uint64_t*)(param_buffer + 24) = v140 + 4;
							}
							if (v139 < 0x100000000i64)
							{
								v142 = *(unsigned int*)((*(uint64_t*)(param_buffer + 32) & v141) + *(uint64_t*)(param_buffer + 16));
								*(uint64_t*)(param_buffer + 24) = v141 + 4;
								v139 = v142 | (v139 << 32);
							}
							v143 = _mm_add_epi16(_mm_and_si128(v132, (__m128i)_mm_set_epi32(0x7f277f27, 0x7f277f27, 0x7f277f27, 0x7f277f27)), v14);
							v144 = _mm_sub_epi16(
								_mm_add_epi16(_mm_and_si128(v131, (__m128i)_mm_set_epi32(0x7c877c87, 0x7c877c87, 0x7c877c87, 0x7c877c87)), (__m128i)_mm_set_epi32(0x37902fa, 0x27b01fc, 0x17d00fe, 0x7f0000)),
								v127);
							*(uint64_t*)param_buffer = v138;
							*(uint64_t*)(param_buffer + 8) = v139;
							v145 = ((7 - v136) | (8 * v134)) + 1;
							*(__m128i*)(v118 + 332) = _mm_add_epi16(_mm_srai_epi16(v144, 7u), v127);
							*(__m128i*)(v118 + 350) = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(v143, v128), 5u), v128);
							goto LABEL_73;
						}
						v146 = v117 + param_buffer + 10i64 * *(unsigned __int8*)(param_buffer + 1139);
						v147 = _mm_loadl_epi64((const __m128i*)(v146 + 282));
						v148 = (int)__popcnt(
							_mm_movemask_epi8(
								_mm_cmpgt_epi16(
									v147,
									_mm_shufflelo_epi16(_mm_cvtsi32_si128(*(uint32_t*)param_buffer & 0x3FFF), 0))))
							/ 2;
						v149 = (unsigned int)(3 - v148);
						v150 = *(unsigned __int16*)(v146 + 2 * v149 + 282);
						v151 = (*(uint64_t*)param_buffer & 0x3FFFi64)
							+ (*(uint64_t*)param_buffer >> 14)
							* (*(unsigned __int16*)(v146 + 2i64 * (unsigned int)(4 - v148) + 282) - (unsigned int)v150)
							- v150;
						if (v151 < 0x100000000i64)
						{
							v152 = *(uint64_t*)(param_buffer + 24);
							v153 = *(unsigned int*)((v152 & *(uint64_t*)(param_buffer + 32)) + *(uint64_t*)(param_buffer + 16));
							*(uint64_t*)(param_buffer + 24) = v152 + 4;
							v151 = v153 | (v151 << 32);
						}
						*(uint64_t*)param_buffer = *(uint64_t*)(param_buffer + 8);
						*(uint64_t*)(param_buffer + 8) = v151;

						__m128i loadSnowFlake = _mm_loadl_epi64((const __m128i*) & LUT_Snowflake_1[v149]); // Get the current snowflake.
						__m128i subtract16Bits_A_and_B = _mm_sub_epi16(loadSnowFlake, v147);
						__m128i shiftSnowFlake16Bits = _mm_srai_epi16(subtract16Bits_A_and_B, 6u);
						__m128i addPacked16Bits = _mm_add_epi16(shiftSnowFlake16Bits, v147);
						void* ptr = (uint8_t*)v146 + 282; // Cast is needed to ensure we get the right memory address.
						_mm_storel_epi64((__m128i*)ptr, addPacked16Bits);

						v145 = *(uint32_t*)(param_buffer + 4 * v149 + 1140);
						v154 = _mm_loadu_si128((const __m128i*)(param_buffer + 1140));
						*(uint8_t*)(param_buffer + 1139) = 0;

						__m128i packedShuffleBytes = _mm_shuffle_epi8(v154, m_arr[v149]);
						void* ptr2 = (uint8_t*)param_buffer + 1140; // Cast is needed to ensure we get the right memory address.
						_mm_storeu_si128((__m128i*)ptr2, packedShuffleBytes);

					}
					v181 = *(uint64_t*)(param_buffer + 149208);
					v182 = 0i64;
					v183 = v181 + *(uint64_t*)(param_buffer + 149200);
					v184 = v183 - v145;
					*(uint64_t*)(param_buffer + 149208) = v181 + v73;

					cycle++; // This will stay in honor of my 20 hours of total debugging this.

					if (v145 >= 4)
					{
						do
						{
							*(uint32_t*)(v183 + v182) = *(uint32_t*)(v184 + v182); // NO MORE CRASH HERE, DATA GETS WRITTEN HERE NOW.
							v186 = v182 + 4;
							v182 = (unsigned int)(v182 + 4);
						} while (v186 < (unsigned int)v73);
					}
					else
					{
						do
						{
							*(uint8_t*)(v183 + v182) = *(uint8_t*)(v184 + v182);
							v185 = v182 + 1;
							v182 = (unsigned int)(v182 + 1);
						} while (v185 < (unsigned int)v73);
					}
					v8 = v18;
					*(uint8_t*)(param_buffer + 1138) = *(uint8_t*)(v184 + v73 - 1);
					v5 = v193;
					if (!v18)
					{
						v3 = buffer_size;
						v8 = 0;
						goto LABEL_80;
					}
					goto LABEL_12;
				}
				v5 = v193;
			}
			*(uint32_t*)(param_buffer + 149128) = v8;
		}
	}
	return 0;
}
#pragma warning(pop)

std::unique_ptr<char[]> RTech::DecompressStreamedBuffer(std::unique_ptr<char[]> buf, uint64_t& bufSize, const eCompressionType compType)
{
    switch (compType)
    {
    case eCompressionType::OODLE:
    {
        OodleLZDecoder* const decoder = OodleLZDecoder_Create(OodleLZ_Compressor::OodleLZ_Compressor_Invalid, bufSize, nullptr, 0);

        int outPos = 0;
        int bufPos = 0;

        // Check if we are compressed first.
        OodleLZ_DecodeSome_Out decodeOut = {};
        std::unique_ptr<char[]> outBuf = std::make_unique<char[]>(bufSize);
        if (!OodleLZDecoder_DecodeSome(decoder, &decodeOut, outBuf.get(), outPos, bufSize, bufSize - outPos, buf.get() + bufPos, bufSize - bufPos, OodleLZ_FuzzSafe_No, OodleLZ_CheckCRC_No, OodleLZ_Verbosity::OodleLZ_Verbosity_None, OodleLZ_Decode_ThreadPhaseAll))
        {
            // Not decompressed.
            OodleLZDecoder_Destroy(decoder);
            return std::move(buf);
        }

        // We already have an initial amount of decompressed data due to the first run.
        while (true)
        {
            outPos += decodeOut.decodedCount;
            bufPos += decodeOut.compBufUsed;

            // Are we done with decompressing?
            if (decodeOut.compBufUsed + decodeOut.decodedCount == 0)
                break;

            // We shouldn't ever exceed our initial bufSize..
            if (outPos >= bufSize)
                break;

            // Continue decompressing.
            OodleLZDecoder_DecodeSome(decoder, &decodeOut, outBuf.get(), outPos, bufSize, bufSize - outPos, buf.get() + bufPos, bufSize - bufPos, OodleLZ_FuzzSafe_No, OodleLZ_CheckCRC_No, OodleLZ_Verbosity::OodleLZ_Verbosity_None, OodleLZ_Decode_ThreadPhaseAll);
        }

        OodleLZDecoder_Destroy(decoder);
        return std::move(outBuf);
    }
	case eCompressionType::PAKFILE:
	{
		RTech::PakDecompressContext_t context = {};
		const uint64_t decodeSize = RTech::InitPakDecoder(&context, reinterpret_cast<uint8_t*>(buf.get()), PAK_DECODE_MASK, bufSize, 0, 0); // We don't want to skip any data here, hence why no headerSize.

		std::unique_ptr<char[]> outBuf = std::make_unique<char[]>(decodeSize);
		context.m_outputMask = PAK_DECODE_MASK;
		context.m_outputBuf = uint64_t(outBuf.get());

		DecompressPakFile(&context, bufSize, decodeSize);
		assertm(decodeSize == context.m_decompSize, "Mismatch on decomp size.");
		bufSize = context.m_decompSize;
		return std::move(outBuf);
	}
	case eCompressionType::SNOWFLAKE: // Snowflake will be made prettier when it's working.
	{
		std::unique_ptr<char[]> decompState = std::make_unique<char[]>(0x25000);
		InitSnowflakeDecompState((long long)&decompState.get()[0], reinterpret_cast<int64_t>(buf.get()), bufSize);

		__int64* editDecompState = (__int64*)&decompState.get()[0];
		__int64 decodeSize = editDecompState[0x48D3];

		unsigned int v15 = *((unsigned int*)editDecompState + 0x91A4); // decomp pos?
		*((uint32_t*)editDecompState + 0x91A2) = 0;
		if (v15 < decodeSize)
			decodeSize = v15;

		std::unique_ptr<char[]> outBuf = std::make_unique<char[]>(decodeSize);

		editDecompState[0x48D4] = decodeSize;
		editDecompState[0x48DA] = (__int64)outBuf.get(); // output buffer.
		editDecompState[0x48DB] = 0;

		DecompressSnowflake((long long)&decompState.get()[0], bufSize, decodeSize);
		bufSize = editDecompState[0x48db];

		return std::move(outBuf);
	}
    default:
    {
        assertm(false, "Unhandled compression type.");
        return nullptr;
    }
    }

    unreachable();
}

unsigned char byte_18002D840[] =
{
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90,
  0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8,
  0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8,
  0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
  0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C,
  0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC,
  0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2,
  0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A,
  0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6,
  0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6,
  0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81,
  0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1,
  0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9,
  0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95,
  0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD,
  0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD,
  0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
  0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B,
  0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB,
  0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7,
  0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F,
  0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#pragma warning( push, 0 )
#pragma warning( disable: 6297 )
__int64 PakPatch_DecodeData(char* buffer, int arrayBitDepth, char* out_1, char* out_2)
{
    int v5; // er14
    int v8; // eax
    char v11; // al MAPDST
    char v12; // r8
    unsigned int offset; // edi
    unsigned int arraySize; // ebx
    int v15; // er12
    int v17; // er9
    char i; // r8 MAPDST
    char v20; // r13
    __int64 j; // rax

    // in_data - PES (patch edit stream) buffer
    // arrayBitDepth - number of bits used for array indexes
    // (array is size 1 << arrayBitDepth)

    // a3 - array of numbers: [0 1 2 3 4 5 6]
    // out_1 - output array
    // out_2 - output array
    v5 = 0;
    if (!_BitScanReverse((unsigned long*)&v8, arrayBitDepth - 2))
        v11 = 0;
    else
        v11 = v8 + 1;

    v12 = *buffer;
    offset = 0;
    arraySize = 1 << arrayBitDepth;
    v15 = (1 << v11) - 1;
    v17 = 1 << arrayBitDepth;
    for (i = (v15 & v12) + 1; ; v5 = (v5 + 1) << (i - v20))
    {
        v20 = i;
        for (j = byte_18002D840[v5 << (8 - i)]; j < arraySize; j = ((1 << i) + j))
        {
            --v17;
            out_1[j] = ((unsigned __int8)buffer[offset] >> v11);
            out_2[j] = i;
        }
        offset++;
        if (!v17)
            break;
        i = (v15 & buffer[offset]) + 1;
    }
    return offset;
}
#pragma warning( pop )

static uint64_t Pak_StringToGuidAligned(const char* string)
{
	uint64_t         v1; // r9
	int               i; // r11d
	uint32_t         v4; // edi
	int              v5; // ebp
	int              v6; // r10d
	uint32_t         v7; // ecx
	uint32_t         v8; // edx
	uint32_t         v9; // eax
	uint32_t        v10; // r8d
	int64_t         v11; // r10
	uint64_t        v12; // r8
	int             v13; // eax
	int             v15; // ecx

	v1 = 0ull;
	for (i = 0; ; i += 4)
	{
		v4 = ~*(uint32_t*)string & (*(uint32_t*)string - 0x1010101) & 0x80808080;
		v5 = v4 ^ (v4 - 1);
		v6 = v5 & *(uint32_t*)string ^ 0x5C5C5C5C;
		v7 = ~v6 & (v6 - 0x1010101) & 0x80808080;
		v8 = v7 & -(int32_t)v7;
		if (v7 != v8)
		{
			v9 = 0xFF000000;
			do
			{
				v10 = v9;
				if ((v9 & v6) == 0)
					v8 |= v9 & 0x80808080;
				v9 >>= 8;
			} while (v10 >= 0x100);
		}
		v11 = 0x633D5F1 * v1;
		v12 = (0xFB8C4D96501i64 * (uint64_t)(((v5 & *(uint32_t*)string) - 45 * (v8 >> 7)) & 0xDFDFDFDF)) >> 24;
		if (v4)
			break;
		string += 4;
		v1 = ((v11 + v12) >> 61) ^ (v11 + v12);
	}
	v13 = -1;
	if (_BitScanReverse((unsigned long*)&v15, v5))
		v13 = v15;
	return v12 + v11 - 0xAE502812AA7333i64 * (uint32_t)(i + v13 / 8);
}

static uint64_t Pak_StringToGuidUnaligned(const char* string)
{
	uint64_t        v1; // rbx
	uint64_t        v2; // r10
	int              i; // esi
	int             v4; // edx
	uint32_t        v5; // edi
	int             v6; // ebp
	int             v7; // edx
	uint32_t        v8; // ecx
	uint32_t        v9; // r8d
	uint32_t       v10; // eax
	uint32_t       v11; // r9d
	int64_t        v12; // r9
	uint64_t       v13; // r8
	int            v14; // eax
	int            v16; // ecx

	v1 = 0ull;
	v2 = (uint64_t)(string + 3);
	for (i = 0; ; i += 4)
	{
		if ((v2 ^ (v2 - 3)) >= 0x1000)
		{
			v4 = *(uint8_t*)(v2 - 3);
			if ((uint8_t)v4)
			{
				v4 = *(uint16_t*)(v2 - 3);
				if (*(uint8_t*)(v2 - 2))
				{
					v4 |= *(uint8_t*)(v2 - 1) << 16;
					if (*(uint8_t*)(v2 - 1))
						v4 |= *(uint8_t*)v2 << 24;
				}
			}
		}
		else
		{
			v4 = *(uint32_t*)(v2 - 3);
		}
		v5 = ~v4 & (v4 - 0x1010101) & 0x80808080;
		v6 = v5 ^ (v5 - 1);
		v7 = v6 & v4;
		v8 = ~(v7 ^ 0x5C5C5C5C) & ((v7 ^ 0x5C5C5C5C) - 0x1010101) & 0x80808080;
		v9 = v8 & -(int32_t)v8;
		if (v8 != v9)
		{
			v10 = 0xFF000000;
			do
			{
				v11 = v10;
				if ((v10 & (v7 ^ 0x5C5C5C5C)) == 0)
					v9 |= v10 & 0x80808080;
				v10 >>= 8;
			} while (v11 >= 0x100);
		}
		v12 = 0x633D5F1 * v1;
		v13 = (0xFB8C4D96501i64 * (uint64_t)((v7 - 45 * (v9 >> 7)) & 0xDFDFDFDF)) >> 24;
		if (v5)
			break;
		v2 += 4i64;
		v1 = ((v12 + v13) >> 61) ^ (v12 + v13);
	}
	v14 = -1;
	if (_BitScanReverse((unsigned long*)&v16, v6))
		v14 = v16;
	return v13 + v12 - 0xAE502812AA7333i64 * (uint32_t)(i + v14 / 8);
}

uint64_t __fastcall RTech::StringToGuid(const char* str)
{
	return ((uintptr_t)str & 3)
		? Pak_StringToGuidUnaligned(str)
		: Pak_StringToGuidAligned(str);
}

uint32_t __fastcall RTech::StringToUIMGHash(const char* str)
{
    std::uint64_t r = StringToGuid(str);
    unsigned int l = (r & 0xFFFFFFFF);
    unsigned int h = (r >> 32);

    unsigned int x = l ^ h;

    return x;
}