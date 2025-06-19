#pragma once

static constexpr size_t invalidNoodleIdx = 0xFFFFFFFFFFFFFFFF;

class CRamen
{
public:
	class CNoodle
	{
	public:
		CNoodle() = delete; // no default constructor.
		CNoodle(char* const buf, const size_t compSize, const size_t decompSize, bool isComp) : data(buf), compressedSize(compSize), decompressedSize(decompSize), isCompressed(isComp) {};
		inline ~CNoodle()
		{
			if (data)
			{
				delete[] data;
				data = nullptr;
			}
		}

		char* data;
		size_t compressedSize;
		size_t decompressedSize;
		bool isCompressed;
	};

	inline CRamen() : noodles(nullptr), capacity(0ull), noodleSize(0ull) {};
	inline CRamen(const size_t size) : noodles(nullptr), capacity(0ull), noodleSize(0ull)
	{
		resize(size);
	}

	inline ~CRamen()
	{
		nuke();
	}

	CRamen(const CRamen& ramen) = delete;
	CRamen& operator=(const CRamen&) = delete;
	CRamen& operator=(CRamen&& dataChunks) noexcept
	{
		if (this != &dataChunks)
		{
			clear();
			shrink();

			this->noodles = dataChunks.noodles;
			this->capacity = dataChunks.capacity;
			this->noodleSize = dataChunks.noodleSize;

			dataChunks.noodles = nullptr;
			dataChunks.capacity = 0ull;
			dataChunks.noodleSize = 0ull;
		}

		return *this;
	}

	inline void move(CRamen& raman)
	{
		if (this != &raman)
		{
			clear();
			shrink();

			this->noodles = raman.noodles;
			this->capacity = raman.capacity;
			this->noodleSize = raman.noodleSize;

			raman.noodles = nullptr;
			raman.capacity = 0ull;
			raman.noodleSize = 0ull;
		}
	}

	inline const size_t addBack(char* const buf, const size_t bufSize)
	{ 
		return addIdx(noodleSize, buf, bufSize);
	}

	std::unique_ptr<char[]> getIdx(const size_t index) const;
	inline std::unique_ptr<char[]> getBack() const
	{
		return getIdx(noodleSize - 1ull);
	}

	inline std::unique_ptr<char[]> getFront() const
	{
		return getIdx(0ull);
	}

	inline void clear()
	{
		// only clear the data itself
		if (noodles != nullptr)
		{
			for (size_t i = 0ull; i < noodleSize; ++i)
			{
				if (noodles[i])
					delete noodles[i];
			}
		}

		noodleSize = 0;
	}

	inline void nuke()
	{
		clear();
		if (noodles != nullptr)
		{
			delete[] noodles;
			noodles = nullptr;
		}

		capacity = 0;
	}

	inline void resize(const size_t newSize) // resize with potential data loss
	{
		if (newSize > capacity) // grow
		{
			const size_t newCapacity = std::max(capacity * 2ull, newSize);
			resizeCapacity(newCapacity);
		}
		else if (newSize < noodleSize) // shrink
		{
			for (size_t i = newSize; i < noodleSize; ++i)
			{
				delete noodles[i];
			}

			if (newSize < (capacity / 2ull))
			{
				const size_t newCapacity = std::max(newSize, 1ull);
				resizeCapacity(newCapacity);
			}
		}
	}

	inline const size_t size() const
	{
		return noodleSize;
	}

	inline void shrink()
	{
		resize(noodleSize);
	}

	CNoodle** const begin()
	{
		return noodles;
	}

	CNoodle** const end()
	{
		return (noodles + noodleSize);
	}

private:
	const size_t addIdx(const size_t index, char* const buf, const size_t size);

	inline void ensureCapacity(size_t newCapacity)
	{
		if (newCapacity > capacity)
		{
			resizeCapacity(newCapacity);
		}
	}

	inline void resizeCapacity(const size_t newCapacity)
	{
		CNoodle** const newChunks = new CNoodle*[newCapacity];

		if (noodles && noodleSize > 0ull)
		{
			memcpy(newChunks, noodles, noodleSize * sizeof(CNoodle*));
			delete[] noodles;
		}

		noodles = newChunks;
		capacity = newCapacity;
	}
	CNoodle** noodles;
	size_t capacity;
	size_t noodleSize;
};