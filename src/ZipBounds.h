#pragma once

// A .dto file states how large each clip format is once unzipped, and Bulb allocates that much before unzipping.
// zlib cannot expand data more than about 1032 times, so a stated size beyond that comes from a damaged or hostile file.
namespace ZipBounds
{
	inline bool Plausible(long long unzipped, long long zipped)
	{
		return zipped > 0 && unzipped >= 0 && unzipped <= zipped * 1032 + 64;
	}
}
