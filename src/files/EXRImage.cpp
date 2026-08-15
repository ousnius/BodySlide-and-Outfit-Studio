/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "EXRImage.h"

#ifdef USE_OPENEXR

#include "../utils/PlatformUtil.h"

// The C core rather than the C++ Imf API on purpose. The C++ one drags in Iex, of which the Autodesk
// FBX SDK statically carries its own copy, and the two can't be linked into Outfit Studio together.
// The core also decodes every compression the format has, DWAA and DWAB included, and needs neither
// Imath nor IlmThread.
#include <openexr.h>

#include <algorithm>
#include <cstring>

namespace {
// Half representation of 1.0, the alpha a file without an alpha channel is taken to mean.
constexpr uint16_t kHalfOne = 0x3C00;

struct MemoryStream {
	std::vector<char> buffer;
};

int64_t StreamRead(exr_const_context_t, void* userData, void* dest, uint64_t size, uint64_t offset, exr_stream_error_func_ptr_t) {
	const auto* stream = static_cast<const MemoryStream*>(userData);
	if (offset >= stream->buffer.size())
		return 0;

	const uint64_t available = std::min<uint64_t>(size, stream->buffer.size() - offset);
	std::memcpy(dest, stream->buffer.data() + offset, static_cast<size_t>(available));
	return static_cast<int64_t>(available);
}

int64_t StreamSize(exr_const_context_t, void* userData) {
	return static_cast<int64_t>(static_cast<const MemoryStream*>(userData)->buffer.size());
}

// The library prints to stderr by default, which in a GUI application goes nowhere useful. The
// result code carries enough to report with.
void SilentErrorHandler(exr_const_context_t, exr_result_t, const char*) {}

bool ReadWholeFile(const std::string& fileName, std::vector<char>& outData) {
	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios_base::in | std::ios_base::binary | std::ios_base::ate);
	if (!file.is_open())
		return false;

	const std::streampos size = file.tellg();
	if (size <= 0)
		return false;

	outData.resize(static_cast<size_t>(size));
	file.seekg(0, std::ios_base::beg);
	file.read(outData.data(), size);
	return file.good() || file.eof();
}

// Which of the four output components a channel writes to, or -1 for one this doesn't care about.
int ChannelIndex(const char* name) {
	if (!name || name[0] == '\0' || name[1] != '\0')
		return -1;

	switch (name[0]) {
		case 'R': return 0;
		case 'G': return 1;
		case 'B': return 2;
		case 'A': return 3;
		default: return -1;
	}
}

// Points the chunk's channels at their place in the output image. Channels the image has no room
// for are left without a destination, which is how the default unpack routine is told to skip them.
bool PointChannelsAtImage(exr_decode_pipeline_t& decode, const exr_chunk_info_t& chunk, uint16_t* pixels, const int width, const int minX, const int minY) {
	const int32_t pixelStride = 4 * sizeof(uint16_t);
	const int32_t lineStride = width * pixelStride;

	bool anyChannel = false;
	for (int16_t c = 0; c < decode.channel_count; c++) {
		exr_coding_channel_info_t& channel = decode.channels[c];

		const int index = ChannelIndex(channel.channel_name);
		// Integer channels have no meaning in a radiance image and aren't converted to half.
		if (index < 0 || channel.data_type == EXR_PIXEL_UINT) {
			channel.decode_to_ptr = nullptr;
			channel.user_pixel_stride = 0;
			channel.user_line_stride = 0;
			continue;
		}

		const size_t offset = (static_cast<size_t>(chunk.start_y - minY) * width + (chunk.start_x - minX)) * 4 + index;

		channel.user_data_type = EXR_PIXEL_HALF;
		channel.user_bytes_per_element = sizeof(uint16_t);
		channel.user_pixel_stride = pixelStride;
		channel.user_line_stride = lineStride;
		channel.decode_to_ptr = reinterpret_cast<uint8_t*>(pixels + offset);
		anyChannel = true;
	}

	return anyChannel;
}
} // namespace

bool LoadEXRImage(const std::string& fileName, std::vector<uint16_t>& outRGBA, int& outWidth, int& outHeight, std::string& outError) {
	MemoryStream stream;
	if (!ReadWholeFile(fileName, stream.buffer)) {
		outError = "File could not be read.";
		return false;
	}

	exr_context_initializer_t init = EXR_DEFAULT_CONTEXT_INITIALIZER;
	init.error_handler_fn = &SilentErrorHandler;
	init.read_fn = &StreamRead;
	init.size_fn = &StreamSize;
	init.user_data = &stream;

	exr_context_t ctxt = nullptr;
	exr_result_t result = exr_start_read(&ctxt, fileName.c_str(), &init);
	if (result != EXR_ERR_SUCCESS) {
		outError = exr_get_default_error_message(result);
		return false;
	}

	// Only the first part is read. Multi-part files are for renders with separate passes, which an
	// environment map isn't, and there would be no way to say which part was meant.
	const int part = 0;

	exr_storage_t storage = EXR_STORAGE_LAST_TYPE;
	exr_attr_box2i_t dataWindow = {};
	if (exr_get_storage(ctxt, part, &storage) != EXR_ERR_SUCCESS || exr_get_data_window(ctxt, part, &dataWindow) != EXR_ERR_SUCCESS) {
		outError = "Image header could not be read.";
		exr_finish(&ctxt);
		return false;
	}

	if (storage != EXR_STORAGE_SCANLINE && storage != EXR_STORAGE_TILED) {
		outError = "Deep images are not supported.";
		exr_finish(&ctxt);
		return false;
	}

	// The data window is what the file actually stores, which needn't start at the origin. Its size
	// is what gets uploaded; the display window only matters for compositing, which an environment
	// map isn't part of.
	const int width = dataWindow.max.x - dataWindow.min.x + 1;
	const int height = dataWindow.max.y - dataWindow.min.y + 1;
	if (width <= 0 || height <= 0) {
		outError = "Image has an empty data window.";
		exr_finish(&ctxt);
		return false;
	}

	// Opaque up front, so a file without an alpha channel doesn't come out fully transparent.
	outRGBA.assign(static_cast<size_t>(width) * height * 4, 0);
	for (size_t i = 3; i < outRGBA.size(); i += 4)
		outRGBA[i] = kHalfOne;

	// Chunks are read in whatever order they are laid out in; each one knows where it belongs.
	std::vector<exr_chunk_info_t> chunks;
	if (storage == EXR_STORAGE_SCANLINE) {
		int32_t scanlinesPerChunk = 1;
		exr_get_scanlines_per_chunk(ctxt, part, &scanlinesPerChunk);

		for (int y = dataWindow.min.y; y <= dataWindow.max.y; y += scanlinesPerChunk) {
			exr_chunk_info_t chunk = {};
			if (exr_read_scanline_chunk_info(ctxt, part, y, &chunk) == EXR_ERR_SUCCESS)
				chunks.push_back(chunk);
		}
	}
	else {
		// exr_get_tile_counts() only exists from OpenEXR 3.2 on, and Ubuntu still ships 3.1. The
		// counts follow from the tile size, which every version has, over the area level 0 covers,
		// which is the data window.
		int32_t tileWidth = 0, tileHeight = 0;
		if (exr_get_tile_sizes(ctxt, part, 0, 0, &tileWidth, &tileHeight) != EXR_ERR_SUCCESS || tileWidth <= 0 || tileHeight <= 0) {
			outError = "Tiled image could not be read.";
			exr_finish(&ctxt);
			return false;
		}

		const int32_t tilesX = (width + tileWidth - 1) / tileWidth;
		const int32_t tilesY = (height + tileHeight - 1) / tileHeight;

		for (int32_t ty = 0; ty < tilesY; ty++) {
			for (int32_t tx = 0; tx < tilesX; tx++) {
				exr_chunk_info_t chunk = {};
				if (exr_read_tile_chunk_info(ctxt, part, tx, ty, 0, 0, &chunk) == EXR_ERR_SUCCESS)
					chunks.push_back(chunk);
			}
		}
	}

	exr_decode_pipeline_t decode = EXR_DECODE_PIPELINE_INITIALIZER;
	bool initialized = false;
	bool decoded = false;

	for (const auto& chunk : chunks) {
		// The pipeline is set up once and then only updated, which keeps its buffers alive across
		// chunks instead of allocating a new set for every one.
		result = initialized ? exr_decoding_update(ctxt, part, &chunk, &decode) : exr_decoding_initialize(ctxt, part, &chunk, &decode);
		if (result != EXR_ERR_SUCCESS)
			break;

		initialized = true;

		if (!PointChannelsAtImage(decode, chunk, outRGBA.data(), width, dataWindow.min.x, dataWindow.min.y))
			continue;

		if (exr_decoding_choose_default_routines(ctxt, part, &decode) != EXR_ERR_SUCCESS)
			break;

		if (exr_decoding_run(ctxt, part, &decode) != EXR_ERR_SUCCESS)
			break;

		decoded = true;
	}

	if (initialized)
		exr_decoding_destroy(ctxt, &decode);

	exr_finish(&ctxt);

	if (!decoded) {
		outError = "Image holds no color channels that could be decoded.";
		return false;
	}

	outWidth = width;
	outHeight = height;
	return true;
}

#else

bool LoadEXRImage(const std::string&, std::vector<uint16_t>&, int&, int&, std::string& outError) {
	outError = "This build has no OpenEXR support.";
	return false;
}

#endif // USE_OPENEXR
