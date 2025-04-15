#pragma once

struct FORMATCHUNK
{
	int chunkID = 0x20746d66; // fmt
	long chunkSize = 0x10;
	short formatTag = 3;
	unsigned short channels;
	unsigned long sampleRate;
	unsigned long avgBytesPerSecond;
	unsigned short blockAlign;
	unsigned short bitsPerSample;
};

struct DATACHUNK
{
	int chunkID = 0x61746164; // data
	long chunkSize;
};

struct WAVEHEADER
{
	int groupID = 0x46464952; // RIFF
	long size;
	int riffType = 0x45564157; // WAVE

	FORMATCHUNK fmt;
	DATACHUNK data;
};