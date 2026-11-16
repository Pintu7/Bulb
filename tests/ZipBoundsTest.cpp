// Checks the size a .dto file may claim for unzipped data before Bulb allocates it,
// and that the zlib sources kept in src\zlib still zip and unzip.
#include <cstdio>
#include <cstring>
#include <vector>
#include "../src/ZipBounds.h"
#include "../src/zlib/zlib.h"

static int failed = 0;

static void Check(bool ok, const char* what)
{
	if (ok == false)
	{
		printf("FAIL: %s\n", what);
		failed++;
	}
}

int main()
{
	Check(ZipBounds::Plausible(1000, 100), "an ordinary ratio is accepted");
	Check(ZipBounds::Plausible(0, 8), "an empty format is accepted");
	Check(ZipBounds::Plausible(100 * 1032 + 64, 100), "the largest size zlib can produce is accepted");
	Check(ZipBounds::Plausible(100 * 1032 + 65, 100) == false, "one byte more than zlib can produce is refused");
	Check(ZipBounds::Plausible(2147483647, 10) == false, "2 GB claimed for 10 bytes is refused");
	Check(ZipBounds::Plausible(-1, 100) == false, "a negative size is refused");
	Check(ZipBounds::Plausible(10, 0) == false, "no compressed data is refused");

	std::vector<unsigned char> text(200000, 'b');
	for (size_t i = 0; i < text.size(); i += 7) text[i] = (unsigned char)(i * 31);
	uLongf zippedSize = compressBound((uLong)text.size());
	std::vector<unsigned char> zipped(zippedSize);
	Check(compress(zipped.data(), &zippedSize, text.data(), (uLong)text.size()) == Z_OK, "zlib zips");
	uLongf unzippedSize = (uLongf)text.size();
	std::vector<unsigned char> unzipped(unzippedSize);
	Check(uncompress(unzipped.data(), &unzippedSize, zipped.data(), zippedSize) == Z_OK && unzippedSize == text.size()
		&& memcmp(unzipped.data(), text.data(), text.size()) == 0, "zlib unzips what it zipped");
	Check(ZipBounds::Plausible((long long)text.size(), (long long)zippedSize), "a real clip's sizes are accepted");
	Check(strcmp(zlibVersion(), "1.3.2") == 0, "the zlib in src\\zlib is 1.3.2");

	printf("ZipBounds: %s\n", failed ? "FAILED" : "all checks passed");
	return failed;
}
