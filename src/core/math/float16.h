#pragma once

// VALVE'S float16 class

//=========================================================
// 16 bit float
//=========================================================


const int float32bias = 127;
const int float16bias = 15;

const float maxfloat16bits = 65504.0f;

class float16
{
public:
	//float16() {}
	//float16( float f ) { m_storage.rawWord = ConvertFloatTo16bits(f); }

	void Init() { m_storage.rawWord = 0; }
	//float16& operator=(const float16 &other) { m_storage.rawWord = other.m_storage.rawWord; return *this; }
	//float16& operator=(const float &other) { m_storage.rawWord = ConvertFloatTo16bits(other); return *this; }
	//operator uint16_t () { return m_storage.rawWord; }
	//operator float () { return Convert16bitFloatTo32bits( m_storage.rawWord ); }
	uint16_t GetBits() const { return m_storage.rawWord; }
	
	float GetFloat() const { return Convert16bitFloatTo32bits(m_storage.rawWord); }
	void SetFloat(float in) { m_storage.rawWord = ConvertFloatTo16bits(in); }

	bool IsInfinity() const { return m_storage.bits.biased_exponent == 31 && m_storage.bits.mantissa == 0; }
	bool IsNaN() const { return m_storage.bits.biased_exponent == 31 && m_storage.bits.mantissa != 0; }

	bool operator==(const float16 other) const { return m_storage.rawWord == other.m_storage.rawWord; }
	bool operator!=(const float16 other) const { return m_storage.rawWord != other.m_storage.rawWord; }

	//	bool operator< (const float other) const	   { return GetFloat() < other; }
	//	bool operator> (const float other) const	   { return GetFloat() > other; }

protected:
	union float32bits
	{
		float rawFloat;
		struct
		{
			uint32_t mantissa : 23;
			uint32_t biased_exponent : 8;
			uint32_t sign : 1;
		} bits;
	};

	union float16bits
	{
		uint16_t rawWord;
		struct
		{
			uint16_t mantissa : 10;
			uint16_t biased_exponent : 5;
			uint16_t sign : 1;
		} bits;
	};

	static bool IsNaN(float16bits in) { return in.bits.biased_exponent == 31 && in.bits.mantissa != 0; }
	static bool IsInfinity(float16bits in) { return in.bits.biased_exponent == 31 && in.bits.mantissa == 0; }

	// 0x0001 - 0x03ff
	static uint16_t ConvertFloatTo16bits(float input)
	{
		if (input > maxfloat16bits)
			input = maxfloat16bits;
		else if (input < -maxfloat16bits)
			input = -maxfloat16bits;

		float16bits output{};
		float32bits inFloat{};

		inFloat.rawFloat = input;

		output.bits.sign = inFloat.bits.sign;

		if ((inFloat.bits.biased_exponent == 0) && (inFloat.bits.mantissa == 0))
		{
			// zero
			output.bits.mantissa = 0;
			output.bits.biased_exponent = 0;
		}
		else if ((inFloat.bits.biased_exponent == 0) && (inFloat.bits.mantissa != 0))
		{
			// denorm -- denorm float maps to 0 half
			output.bits.mantissa = 0;
			output.bits.biased_exponent = 0;
		}
		else if ((inFloat.bits.biased_exponent == 0xff) && (inFloat.bits.mantissa == 0))
		{
#if 0
			// infinity
			output.bits.mantissa = 0;
			output.bits.biased_exponent = 31;
#else
			// infinity maps to maxfloat
			output.bits.mantissa = 0x3ff;
			output.bits.biased_exponent = 0x1e;
#endif
		}
		else if ((inFloat.bits.biased_exponent == 0xff) && (inFloat.bits.mantissa != 0))
		{
#if 0
			// NaN
			output.bits.mantissa = 1;
			output.bits.biased_exponent = 31;
#else
			// NaN maps to zero
			output.bits.mantissa = 0;
			output.bits.biased_exponent = 0;
#endif
		}
		else
		{
			// regular number
			int new_exp = inFloat.bits.biased_exponent - 127;

			if (new_exp < -24)
			{
				// this maps to 0
				output.bits.mantissa = 0;
				output.bits.biased_exponent = 0;
			}

			if (new_exp < -14)
			{
				// this maps to a denorm
				output.bits.biased_exponent = 0;
				uint32_t exp_val = static_cast<uint32_t>(-14 - (inFloat.bits.biased_exponent - float32bias));
				if (exp_val > 0 && exp_val < 11)
				{
					output.bits.mantissa = (1 << (10 - exp_val)) + (inFloat.bits.mantissa >> (13 + exp_val));
				}
			}
			else if (new_exp > 15)
			{
#if 0
				// map this value to infinity
				output.bits.mantissa = 0;
				output.bits.biased_exponent = 31;
#else
				// to big. . . maps to maxfloat
				output.bits.mantissa = 0x3ff;
				output.bits.biased_exponent = 0x1e;
#endif
			}
			else
			{
				output.bits.biased_exponent = new_exp + 15;
				output.bits.mantissa = (inFloat.bits.mantissa >> 13);
			}
		}
		return output.rawWord;
	}

	static float Convert16bitFloatTo32bits(uint16_t input)
	{
		float32bits output{};
		const float16bits& inFloat = *((float16bits*)&input);

		if (IsInfinity(inFloat))
		{
			return maxfloat16bits * ((inFloat.bits.sign == 1) ? -1.0f : 1.0f);
		}
		if (IsNaN(inFloat))
		{
			return 0.0;
		}
		if (inFloat.bits.biased_exponent == 0 && inFloat.bits.mantissa != 0)
		{
			// denorm
			const float half_denorm = (1.0f / 16384.0f); // 2^-14
			float mantissa = (static_cast<float>(inFloat.bits.mantissa)) / 1024.0f;
			float sgn = (inFloat.bits.sign) ? -1.0f : 1.0f;
			output.rawFloat = sgn * mantissa * half_denorm;
		}
		else
		{
			// regular number
			uint32_t mantissa = inFloat.bits.mantissa;
			uint32_t biased_exponent = inFloat.bits.biased_exponent;
			uint32_t sign = static_cast<uint32_t>(inFloat.bits.sign) << 31;
			biased_exponent = ((biased_exponent - float16bias + float32bias) * (biased_exponent != 0)) << 23;
			mantissa <<= (23 - 10);

			*reinterpret_cast<uint32_t*>(&output) = (mantissa | biased_exponent | sign);
		}

		return output.rawFloat;
	}


	float16bits m_storage;
};