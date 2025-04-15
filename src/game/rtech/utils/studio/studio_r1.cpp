#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>

namespace r1
{
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2)
	{
		if (!panimvalue)
		{
			v1 = v2 = 0;
			return;
		}

		// Avoids a crash reading off the end of the data
		// There is probably a better long-term solution; Ken is going to look into it.
		if ((panimvalue->num.total == 1) && (panimvalue->num.valid == 1))
		{
			v1 = v2 = panimvalue[1].value * scale;
			return;
		}

		int k = frame;

		// find the data list that has the frame
		while (panimvalue->num.total <= k)
		{
			k -= panimvalue->num.total;
			panimvalue += panimvalue->num.valid + 1;
			if (panimvalue->num.total == 0)
			{
				//assert(0); // running off the end of the animation stream is bad
				v1 = v2 = 0;
				return;
			}
		}
		if (panimvalue->num.valid > k)
		{
			// has valid animation data
			v1 = panimvalue[k + 1].value * scale;

			if (panimvalue->num.valid > k + 1)
			{
				// has valid animation blend data
				v2 = panimvalue[k + 2].value * scale;
			}
			else
			{
				if (panimvalue->num.total > k + 1)
				{
					// data repeats, no blend
					v2 = v1;
				}
				else
				{
					// pull blend from first data block in next list
					v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
				}
			}
		}
		else
		{
			// get last valid data block
			v1 = panimvalue[panimvalue->num.valid].value * scale;
			if (panimvalue->num.total > k + 1)
			{
				// data repeats, no blend
				v2 = v1;
			}
			else
			{
				// pull blend from first data block in next list
				v2 = panimvalue[panimvalue->num.valid + 2].value * scale;
			}
		}
	}

	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1)
	{
		if (!panimvalue)
		{
			v1 = 0;
			return;
		}

		int k = frame;

		while (panimvalue->num.total <= k)
		{
			k -= panimvalue->num.total;
			panimvalue += panimvalue->num.valid + 1;
			if (panimvalue->num.total == 0)
			{
				//assert(0); // running off the end of the animation stream is bad
				v1 = 0;
				return;
			}
		}
		if (panimvalue->num.valid > k)
		{
			v1 = panimvalue[k + 1].value * scale;
		}
		else
		{
			// get last valid data block
			v1 = panimvalue[panimvalue->num.valid].value * scale;
		}
	}
}