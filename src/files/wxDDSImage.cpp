/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxDDSImage.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "gli.hpp"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

wxIMPLEMENT_DYNAMIC_CLASS(wxDDSHandler, wxImageHandler);

bool wxDDSHandler::LoadFile(wxImage* image, wxInputStream& stream, bool WXUNUSED(verbose), int WXUNUSED(index)) {
	size_t datasize = stream.GetSize();
	if (datasize <= 0)
		return false;

	char* buf = new char[datasize];
	if (!stream.ReadAll(buf, datasize)) {
		delete[] buf;
		return false;
	}

	gli::texture intex = gli::load(buf, datasize);
	if (intex.empty()) {
		delete[] buf;
		return false;
	}

	gli::texture2d tex2d(intex);
	gli::extent2d dim = tex2d.extent();
	unsigned char* srcptr = (unsigned char*)tex2d.data();
	image->Destroy();
	image->Create(dim.x, dim.y, false);
	unsigned char* destPtr = image->GetData();

	uint32_t pxcount = dim.x * dim.y * 3;
	if (!gli::is_compressed(intex.format())) {
		for (uint32_t i = 0; i < pxcount; i += 3) {
			destPtr[i] = *srcptr++;
			destPtr[i + 1] = *srcptr++;
			destPtr[i + 2] = *srcptr++;

			// skipping alpha
			srcptr++;
		}
	}
	else {
		for (uint32_t i = 0; i < pxcount; i += 3) {
			destPtr[i] = 66;
			destPtr[i + 1] = 66;
			destPtr[i + 2] = 66;
		}
	}

	delete[] buf;
	/* Compressed image loading leads to heap corruption... :(

	for (int y = 0; y < dim.y; y += 4) {
		for (int x= 0; x < dim.x; x += 4) {
			unsigned char targetcolor[4 * 16];
			DecompressColor(targetcolor, srcptr, true);
			unsigned char* sourcePixel = targetcolor;
			for (int py = 0; py < 4; py++) {
				for (int px = 0; px < 4; px++) {
					int sx = x + px;
					int sy = y + py;
					if (sx < dim.x && sy < dim.y) {
						unsigned char* targetPixel = destPtr + 4 * (dim.x*sy + sx);
						for (int i = 0; i < 4; i++) {
							*targetPixel++ = *sourcePixel++;
						}
					}
					else {
						sourcePixel += 4;
					}

				}
			}
		}
		srcptr += 8;
	}
	*/


	return true;
}

bool wxDDSHandler::SaveFile(wxImage* WXUNUSED(image), wxOutputStream& WXUNUSED(stream), bool WXUNUSED(verbose)) {
	return false;
}


bool wxDDSHandler::DoCanRead(wxInputStream& stream) {
	unsigned char hdr[4];

	if (!stream.Read(hdr, WXSIZEOF(hdr))) // it's ok to modify the stream position here
		return false;

	return memcmp(hdr, "DDS ", WXSIZEOF(hdr)) == 0;
}

/* Decompression functions cribbed from libsquish */
void wxDDSHandler::DecompressColor(unsigned char* outPixels, unsigned char* block, bool dxt1) {
	unsigned char codes[16];
	int a = Unpack565(block, codes);
	int b = Unpack565(block + 2, codes + 4);

	for (int i = 0; i < 3; i++) {
		int c = codes[i];
		int d = codes[4 + i];

		if (dxt1 && a <= b) {
			codes[8 + i] = (c + d) / 2;
			codes[12 + i] = 0;
		}
		else {
			codes[8 + i] = (2 * c + d) / 3;
			codes[12 + i] = (c + 2 * d) / 3;
		}
	}

	codes[8 + 3] = 255;
	codes[12 + 3] = (dxt1 && a <= b) ? 0 : 255;

	unsigned char indices[16];
	for (int i = 0; i < 4; i++) {
		unsigned char* ind = indices + 4 * i;
		unsigned char packed = block[4 + i];
		ind[0] = packed & 0x3;
		ind[1] = (packed >> 2) & 0x3;
		ind[2] = (packed >> 4) & 0x3;
		ind[3] = (packed >> 6) & 0x3;
	}

	for (int i = 0; i < 16; ++i) {
		unsigned char offset = 4 * indices[i];
		for (int j = 0; j < 4; ++j)
			outPixels[4 * i + j] = codes[offset + j];
	}
}

/* Decompression functions cribbed from libsquish */
int wxDDSHandler::Unpack565(unsigned char* bytes, unsigned char* color) {
	int value = (int)bytes[0] | ((int)bytes[1] << 8);

	// get the components in the stored range
	unsigned char red = (unsigned char)((value >> 11) & 0x1f);
	unsigned char green = (unsigned char)((value >> 5) & 0x3f);
	unsigned char blue = (unsigned char)(value & 0x1f);

	// scale up to 8 bits
	color[0] = (red << 3) | (red >> 2);
	color[1] = (green << 2) | (green >> 4);
	color[2] = (blue << 3) | (blue >> 2);
	color[3] = 255;

	// return the value
	return value;
}
