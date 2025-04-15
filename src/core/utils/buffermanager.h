#pragma once

#include <pch.h>

static constexpr size_t managedBufferSize = (1024 * 1024 * 16);

// do it this way as allocating big memory blocks over and over is expensive
struct CManagedBuffer
{
public:
	CManagedBuffer() : isOpen(true) { buf = new char[managedBufferSize]; };
	~CManagedBuffer() { delete[] buf; };

	const bool GetStatus() const { return isOpen; };
	char* const Buffer() const { return buf; };

	void SetStatus(const bool status) { isOpen = status; };

private:
	char* buf;
	bool isOpen;
};

class CBufferManager
{
public:
	CBufferManager() : bufferCount((CThread::GetConCurrentThreads() << 1) + 2)
	{
		buffers = new CManagedBuffer[bufferCount];

		for (uint8_t i = 0; i < bufferCount; i++)
			openSlots.push(i);
	};

	~CBufferManager() { delete[] buffers; };

	CManagedBuffer* ClaimBuffer()
	{
#if defined(ASSERTS)
		assertm(openSlots.empty() == false, "at least one slot should always be open");
#endif
		if (openSlots.empty())
			return nullptr;

		std::unique_lock<std::mutex> lock(bufferMutex);
		const uint8_t index = openSlots.top();
		openSlots.pop();

#if defined(ASSERTS)
		assertm(index >= 0 || index < bufferCount, "index was out of bounds");
		assertm(buffers[index].GetStatus(), "buffer was not open");
#endif

		buffers[index].SetStatus(false);
		return &buffers[index];
	}

	void RelieveBuffer(CManagedBuffer* buffer)
	{
		std::unique_lock<std::mutex> lock(bufferMutex);
		const uint8_t index = static_cast<uint8_t>(buffer - buffers);

		openSlots.push(index);
		buffer->SetStatus(true);
	}

	static inline const size_t MaxBufferSize() { return managedBufferSize; };

private:
	std::mutex bufferMutex;
	CManagedBuffer* buffers;
	const uint32_t bufferCount;
	std::stack<uint8_t> openSlots;

};