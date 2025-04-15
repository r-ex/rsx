#include <pch.h>
#include <game/rtech/utils/studio/studio.h>
#include <game/rtech/utils/studio/studio_r1.h>

const char* studiohdr_short_t::pszName() const
{
	if (!UseHdr2())
	{
		const int offset = reinterpret_cast<const int* const>(this)[3];

		return reinterpret_cast<const char*>(this) + offset;
	}

	// very bad very not good. however these vars have not changed since HL2.
	const r1::studiohdr_t* const pStudioHdr = reinterpret_cast<const r1::studiohdr_t* const>(this);

	return pStudioHdr->pszName();
}