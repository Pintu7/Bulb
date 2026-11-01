// built without the precompiled header: C++/WinRT and MFC headers do not mix, and the test program compiles it alone
#include <windows.h>
#include "ImageOcr.h"
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Media.Ocr.h>

#pragma comment(lib, "windowsapp")

using namespace winrt;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Media::Ocr;

namespace ImageOcr
{
	// a CF_DIB is a .bmp file without its 14 byte file header; put the header back so the WIC decoder can read it
	static std::vector<unsigned char> DibToBmp(const std::vector<unsigned char>& dib)
	{
		if (dib.size() < sizeof(BITMAPINFOHEADER))
		{
			return {};
		}

		const BITMAPINFOHEADER* info = reinterpret_cast<const BITMAPINFOHEADER*>(dib.data());
		DWORD colors = info->biClrUsed;
		if (colors == 0 && info->biBitCount <= 8)
		{
			colors = 1u << info->biBitCount;
		}
		DWORD masks = (info->biSize == sizeof(BITMAPINFOHEADER) && info->biCompression == BI_BITFIELDS) ? 12 : 0;

		BITMAPFILEHEADER file = {};
		file.bfType = 0x4D42; // "BM"
		file.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + dib.size());
		file.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + info->biSize + masks + colors * sizeof(RGBQUAD));

		std::vector<unsigned char> bmp(sizeof(file));
		memcpy(bmp.data(), &file, sizeof(file));
		bmp.insert(bmp.end(), dib.begin(), dib.end());
		return bmp;
	}

	// joins the calling thread to the multithreaded apartment (.get() is not allowed on an STA) and leaves it on every way out
	struct Apartment
	{
		bool joined = false;
		Apartment()
		{
			try
			{
				init_apartment(apartment_type::multi_threaded);
				joined = true;
			}
			catch (...)
			{
				// already in an apartment: use it as it is and leave it alone
			}
		}
		~Apartment()
		{
			if (joined)
			{
				uninit_apartment();
			}
		}
	};

	std::wstring Recognize(const std::vector<unsigned char>& image, bool isDib)
	{
		std::vector<unsigned char> bmp = isDib ? DibToBmp(image) : image;
		if (bmp.empty())
		{
			return L"";
		}

		Apartment apartment;
		std::wstring text;
		try
		{
			InMemoryRandomAccessStream stream;
			DataWriter writer(stream);
			writer.WriteBytes(array_view<const uint8_t>(bmp.data(), bmp.data() + bmp.size()));
			writer.StoreAsync().get();
			writer.DetachStream();
			stream.Seek(0);

			BitmapDecoder decoder = BitmapDecoder::CreateAsync(stream).get();
			if (decoder.PixelWidth() > OcrEngine::MaxImageDimension() || decoder.PixelHeight() > OcrEngine::MaxImageDimension())
			{
				return L""; // shortcut: huge images are skipped instead of scaled down, add scaling if screenshots of 4K+ need it
			}

			SoftwareBitmap bitmap = decoder.GetSoftwareBitmapAsync(BitmapPixelFormat::Bgra8, BitmapAlphaMode::Premultiplied).get();
			OcrEngine engine = OcrEngine::TryCreateFromUserProfileLanguages();
			if (engine == nullptr)
			{
				return L"";
			}

			OcrResult result = engine.RecognizeAsync(bitmap).get();
			for (const OcrLine& line : result.Lines())
			{
				if (text.empty() == false)
				{
					text += L"\n";
				}
				text += line.Text().c_str();
			}
		}
		catch (...)
		{
			text.clear(); // an unreadable image just stays without text
		}

		return text;
	}
}
