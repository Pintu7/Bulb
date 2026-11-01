#pragma once

#include <string>
#include <vector>

// Text in images, read by the OCR engine built into Windows 10/11: local, nothing leaves the PC.
namespace ImageOcr
{
	// image: the bytes of a CF_DIB clipboard image (isDib true), or of an encoded image file such as the "PNG"
	// clipboard format (isDib false). Returns the recognized lines joined by newlines, or "" when there is no text,
	// the image is too large for the engine, or no OCR language is installed.
	// Blocks for a moment: call it from a worker thread, never from the UI thread.
	std::wstring Recognize(const std::vector<unsigned char>& image, bool isDib = true);
}
