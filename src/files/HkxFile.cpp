/*
BodySlide and Outfit Studio
See the included LICENSE file

Native reader for Havok HKX packfile skeletons and spline-compressed
animations. See HkxFile.h for the supported variants.

Spline decompression and the binary layout descriptions are ported from
PyNifly's anim_fo4.py / anim_skyrim.py (GPLv3) by Bad Dog Skyrim,
which is itself derived from Dagobaking's skyrim-fo4-animation-conversion
and PredatorCZ/HavokLib (both GPLv3).
*/

#include "HkxFile.h"

#include "../utils/PlatformUtil.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <vector>

namespace HKX {

namespace {

// ─── Little-endian primitive readers ──────────────────────────────────────

inline uint32_t ReadU32(const uint8_t* d) {
	return uint32_t(d[0]) | (uint32_t(d[1]) << 8) | (uint32_t(d[2]) << 16) | (uint32_t(d[3]) << 24);
}
inline int32_t ReadI32(const uint8_t* d) { return int32_t(ReadU32(d)); }
inline uint16_t ReadU16(const uint8_t* d) { return uint16_t(d[0]) | (uint16_t(d[1]) << 8); }
inline int16_t ReadI16(const uint8_t* d) { return int16_t(ReadU16(d)); }
inline float ReadF32(const uint8_t* d) {
	uint32_t u = ReadU32(d);
	float f;
	std::memcpy(&f, &u, sizeof(f));
	return f;
}

// Bounds-checked variants returning false on overrun.
struct Buf {
	const uint8_t* data = nullptr;
	size_t size = 0;

	bool InBounds(size_t off, size_t len) const { return off <= size && len <= size - off; }
	bool U32(size_t off, uint32_t& v) const {
		if (!InBounds(off, 4))
			return false;
		v = ReadU32(data + off);
		return true;
	}
	bool I16(size_t off, int16_t& v) const {
		if (!InBounds(off, 2))
			return false;
		v = ReadI16(data + off);
		return true;
	}
	bool F32(size_t off, float& v) const {
		if (!InBounds(off, 4))
			return false;
		v = ReadF32(data + off);
		return true;
	}
	bool U8(size_t off, uint8_t& v) const {
		if (!InBounds(off, 1))
			return false;
		v = data[off];
		return true;
	}
};

bool ReadAllFile(const std::string& path, std::vector<uint8_t>& out) {
#ifdef _WINDOWS
	std::wstring wpath = PlatformUtil::MultiByteToWideUTF8(path);
	std::ifstream ifs(wpath.c_str(), std::ios::binary);
#else
	std::ifstream ifs(path.c_str(), std::ios::binary);
#endif
	if (!ifs.is_open())
		return false;
	ifs.seekg(0, std::ios::end);
	std::streampos endPos = ifs.tellg();
	if (endPos <= 0) {
		out.clear();
		return false;
	}
	ifs.seekg(0, std::ios::beg);
	out.resize(static_cast<size_t>(endPos));
	const std::streamsize bytesToRead = static_cast<std::streamsize>(out.size());
	ifs.read(reinterpret_cast<char*>(out.data()), bytesToRead);
	return ifs && ifs.gcount() == bytesToRead;
}

bool WriteAllFile(const std::string& path, const std::vector<uint8_t>& data) {
#ifdef _WINDOWS
	std::wstring wpath = PlatformUtil::MultiByteToWideUTF8(path);
	std::ofstream ofs(wpath.c_str(), std::ios::binary | std::ios::trunc);
#else
	std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::trunc);
#endif
	if (!ofs.is_open())
		return false;
	if (!data.empty())
		ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
	return ofs.good();
}

inline uint32_t AlignUp(uint32_t off, uint32_t alignment) {
	if (alignment <= 1)
		return off;
	uint32_t r = off % alignment;
	return r ? off + (alignment - r) : off;
}

void PatchU32(std::vector<uint8_t>& bytes, uint32_t off, uint32_t value) {
	bytes[off + 0] = static_cast<uint8_t>(value & 0xFFu);
	bytes[off + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
	bytes[off + 2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
	bytes[off + 3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

void PatchF32(std::vector<uint8_t>& bytes, uint32_t off, float value) {
	uint32_t raw = 0;
	std::memcpy(&raw, &value, sizeof(raw));
	PatchU32(bytes, off, raw);
}

void PatchU8(std::vector<uint8_t>& bytes, uint32_t off, uint8_t value) {
	bytes[off] = value;
}

void AppendU8(std::vector<uint8_t>& bytes, uint8_t value) {
	bytes.push_back(value);
}

void AppendU32(std::vector<uint8_t>& bytes, uint32_t value) {
	bytes.push_back(static_cast<uint8_t>(value & 0xFFu));
	bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
	bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFFu));
	bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFFu));
}

void AppendF32(std::vector<uint8_t>& bytes, float value) {
	uint32_t raw = 0;
	std::memcpy(&raw, &value, sizeof(raw));
	AppendU32(bytes, raw);
}

void AppendStringZ(std::vector<uint8_t>& bytes, const std::string& value) {
	bytes.insert(bytes.end(), value.begin(), value.end());
	bytes.push_back(0);
}

void AlignBuffer(std::vector<uint8_t>& bytes, uint32_t alignment, uint8_t fill = 0) {
	bytes.resize(AlignUp(static_cast<uint32_t>(bytes.size()), alignment), fill);
}

struct LocalFixup {
	uint32_t src = 0;
	uint32_t dst = 0;
};

struct GlobalFixup {
	uint32_t src = 0;
	uint32_t dst = 0;
};

struct VirtualFixup {
	uint32_t src = 0;
	uint32_t nameOff = 0;
};

struct ClassNameEntry {
	uint32_t signature = 0;
	const char* name = nullptr;
};

struct BuiltClassNames {
	std::vector<uint8_t> bytes;
	std::unordered_map<std::string, uint32_t> nameOffsets;
};

struct FormatTraits {
	Format format = Format::Unknown;
	uint32_t version = 0;
	uint32_t ptrSize = 0;
	uint32_t firstSectionOffset = 0;
	uint32_t sectionHeaderSize = 0;
	uint32_t containerSize = 0;
	uint32_t animationType = 0;
	const uint8_t* headerPrefix = nullptr;
	uint32_t headerPrefixSize = 0;
	const ClassNameEntry* classNames = nullptr;
	size_t classNameCount = 0;
};

void QuatNormalize(float q[4]);

static const uint8_t kHeaderPrefixV8[0x40] = {
	0x57, 0xE0, 0xE0, 0x57, 0x10, 0xC0, 0xC0, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
	0x04, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x4B, 0x00, 0x00, 0x00,
	0x68, 0x6B, 0x5F, 0x32, 0x30, 0x31, 0x30, 0x2E,
	0x32, 0x2E, 0x30, 0x2D, 0x72, 0x31, 0x00, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const uint8_t kHeaderPrefixV11[0x50] = {
	0x57, 0xE0, 0xE0, 0x57, 0x10, 0xC0, 0xC0, 0x10,
	0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x00, 0x00,
	0x08, 0x01, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x4B, 0x00, 0x00, 0x00,
	0x68, 0x6B, 0x5F, 0x32, 0x30, 0x31, 0x34, 0x2E,
	0x31, 0x2E, 0x30, 0x2D, 0x72, 0x31, 0x00, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x10, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const ClassNameEntry kClassNamesV8[] = {
	{0x75585EF6u, "hkClass"},
	{0x5C7EA4C2u, "hkClassMember"},
	{0x8A3609CFu, "hkClassEnum"},
	{0xCE6F8A6Cu, "hkClassEnumItem"},
	{0x2772C11Eu, "hkRootLevelContainer"},
	{0x8DC20333u, "hkaAnimationContainer"},
	{0x792EE0BBu, "hkaSplineCompressedAnimation"},
	{0x66EAC971u, "hkaAnimationBinding"},
};

static const ClassNameEntry kClassNamesV11[] = {
	{0x33D42383u, "hkClass"},
	{0xEFA719B0u, "hkClassMember"},
	{0x8A3609CFu, "hkClassEnum"},
	{0xCE6F8A6Cu, "hkClassEnumItem"},
	{0x2772C11Eu, "hkRootLevelContainer"},
	{0x26859F4Cu, "hkaAnimationContainer"},
	{0xF8E0B860u, "hkaSplineCompressedAnimation"},
	{0x0FAF9150u, "hkaAnimationBinding"},
};

const FormatTraits* GetFormatTraits(Format format) {
	static const FormatTraits kTraits[] = {
		{Format::Skyrim32, 8u, 4u, 0x40u, 0x30u, 0x70u, 5u, kHeaderPrefixV8, sizeof(kHeaderPrefixV8), kClassNamesV8, sizeof(kClassNamesV8) / sizeof(kClassNamesV8[0])},
		{Format::Skyrim64, 8u, 8u, 0x40u, 0x30u, 0x80u, 5u, kHeaderPrefixV8, sizeof(kHeaderPrefixV8), kClassNamesV8, sizeof(kClassNamesV8) / sizeof(kClassNamesV8[0])},
		{Format::Fallout64, 11u, 8u, 0x50u, 0x40u, 0x80u, 3u, kHeaderPrefixV11, sizeof(kHeaderPrefixV11), kClassNamesV11, sizeof(kClassNamesV11) / sizeof(kClassNamesV11[0])},
	};

	for (const auto& traits : kTraits)
		if (traits.format == format)
			return &traits;
	return nullptr;
}

BuiltClassNames BuildClassNames(const FormatTraits& traits) {
	BuiltClassNames built;
	for (size_t i = 0; i < traits.classNameCount; ++i) {
		const ClassNameEntry& entry = traits.classNames[i];
		uint32_t entryStart = static_cast<uint32_t>(built.bytes.size());
		AppendU32(built.bytes, entry.signature);
		AppendU8(built.bytes, 0x09);
		built.nameOffsets[entry.name] = entryStart + 5u;
		AppendStringZ(built.bytes, entry.name);
	}
	AlignBuffer(built.bytes, 16u, 0xFFu);
	return built;
}

void WriteSectionHeader(std::vector<uint8_t>& fileBytes,
						uint32_t off,
						uint32_t headerSize,
						const char* name,
						uint32_t start,
						uint32_t localFixups,
						uint32_t globalFixups,
						uint32_t virtualFixups,
						uint32_t exports,
						uint32_t end) {
	std::fill(fileBytes.begin() + off, fileBytes.begin() + off + headerSize, 0);
	std::memcpy(fileBytes.data() + off, name, std::min<size_t>(std::strlen(name), 16u));
	PatchU32(fileBytes, off + 0x10u, 0xFF000000u);
	PatchU32(fileBytes, off + 0x14u, start);
	PatchU32(fileBytes, off + 0x18u, localFixups);
	PatchU32(fileBytes, off + 0x1Cu, globalFixups);
	PatchU32(fileBytes, off + 0x20u, virtualFixups);
	PatchU32(fileBytes, off + 0x24u, exports);
	PatchU32(fileBytes, off + 0x28u, end);
	PatchU32(fileBytes, off + 0x2Cu, end);
	if (headerSize > 0x30u)
		std::fill(fileBytes.begin() + off + 0x30u, fileBytes.begin() + off + headerSize, 0xFF);
}

bool IsNearlyIdentityFloat(float value, float identity) {
	return std::fabs(value - identity) <= 1e-6f;
}

std::vector<uint8_t> BuildStaticPoseBlob(const std::vector<Transform>& transforms) {
	const uint32_t maskAndQuantSize = AlignUp(static_cast<uint32_t>(transforms.size()) * 4u, 4u);
	std::vector<uint8_t> blob(maskAndQuantSize, 0);

	for (size_t trackIndex = 0; trackIndex < transforms.size(); ++trackIndex) {
		const Transform& transform = transforms[trackIndex];
		uint8_t posFlags = 0;
		for (int axis = 0; axis < 3; ++axis)
			if (!IsNearlyIdentityFloat(transform.translation[axis], 0.0f))
				posFlags |= static_cast<uint8_t>(1u << axis);

		float quat[4] = {transform.rotation[0], transform.rotation[1], transform.rotation[2], transform.rotation[3]};
		QuatNormalize(quat);
		bool hasRotation = !IsNearlyIdentityFloat(quat[0], 0.0f) || !IsNearlyIdentityFloat(quat[1], 0.0f) || !IsNearlyIdentityFloat(quat[2], 0.0f)
						 || !IsNearlyIdentityFloat(quat[3], 1.0f);

		uint8_t scaleFlags = 0;
		for (int axis = 0; axis < 3; ++axis)
			if (!IsNearlyIdentityFloat(transform.scale[axis], 1.0f))
				scaleFlags |= static_cast<uint8_t>(1u << axis);

		const size_t maskOff = trackIndex * 4u;
		blob[maskOff + 0] = static_cast<uint8_t>(5u << 2); // uncompressed quaternion
		blob[maskOff + 1] = posFlags;
		blob[maskOff + 2] = hasRotation ? 1u : 0u;
		blob[maskOff + 3] = scaleFlags;
	}

	AlignBuffer(blob, 4u);
	for (const Transform& transform : transforms) {
		for (int axis = 0; axis < 3; ++axis)
			if (!IsNearlyIdentityFloat(transform.translation[axis], 0.0f))
				AppendF32(blob, transform.translation[axis]);
		AlignBuffer(blob, 4u);

		float quat[4] = {transform.rotation[0], transform.rotation[1], transform.rotation[2], transform.rotation[3]};
		QuatNormalize(quat);
		bool hasRotation = !IsNearlyIdentityFloat(quat[0], 0.0f) || !IsNearlyIdentityFloat(quat[1], 0.0f) || !IsNearlyIdentityFloat(quat[2], 0.0f)
						 || !IsNearlyIdentityFloat(quat[3], 1.0f);
		if (hasRotation) {
			AppendF32(blob, quat[0]);
			AppendF32(blob, quat[1]);
			AppendF32(blob, quat[2]);
			AppendF32(blob, quat[3]);
		}
		AlignBuffer(blob, 4u);

		for (int axis = 0; axis < 3; ++axis)
			if (!IsNearlyIdentityFloat(transform.scale[axis], 1.0f))
				AppendF32(blob, transform.scale[axis]);
		AlignBuffer(blob, 4u);
	}

	return blob;
}

void SetArrayHeader(std::vector<uint8_t>& dataBytes,
					uint32_t fieldRel,
					uint32_t ptrSize,
					uint32_t count,
					uint32_t contentRel,
					std::vector<LocalFixup>& localFixups) {
	if (count > 0)
		localFixups.push_back({fieldRel, contentRel});
	PatchU32(dataBytes, fieldRel + ptrSize, count);
	PatchU32(dataBytes, fieldRel + ptrSize + 4u, count ? (count | 0x80000000u) : 0x80000000u);
}

void SortAndUniqueLocalFixups(std::vector<LocalFixup>& fixups) {
	std::sort(fixups.begin(), fixups.end(), [](const LocalFixup& a, const LocalFixup& b) { return a.src < b.src; });
	fixups.erase(std::unique(fixups.begin(), fixups.end(), [](const LocalFixup& a, const LocalFixup& b) { return a.src == b.src && a.dst == b.dst; }), fixups.end());
}

void SortAndUniqueGlobalFixups(std::vector<GlobalFixup>& fixups) {
	std::sort(fixups.begin(), fixups.end(), [](const GlobalFixup& a, const GlobalFixup& b) { return a.src < b.src; });
	fixups.erase(std::unique(fixups.begin(), fixups.end(), [](const GlobalFixup& a, const GlobalFixup& b) { return a.src == b.src && a.dst == b.dst; }), fixups.end());
}

void SortAndUniqueVirtualFixups(std::vector<VirtualFixup>& fixups) {
	std::sort(fixups.begin(), fixups.end(), [](const VirtualFixup& a, const VirtualFixup& b) { return a.src < b.src; });
	fixups.erase(std::unique(fixups.begin(), fixups.end(), [](const VirtualFixup& a, const VirtualFixup& b) { return a.src == b.src && a.nameOff == b.nameOff; }), fixups.end());
}

// ─── Fixup tables and section descriptors ─────────────────────────────────

struct Section {
	std::string name;
	uint32_t absStart = 0;        // absolute file offset of section data
	uint32_t localFixupAbs = 0;   // absolute offsets to fixup-table starts
	uint32_t globalFixupAbs = 0;
	uint32_t virtualFixupAbs = 0;
	uint32_t exportsAbs = 0;
	uint32_t endAbs = 0;
};

struct VirtualObject {
	uint32_t relOffset; // relative to data section start
	std::string className;
};

// Extract the file format from the 0x40-byte file header (magic already
// validated by caller).
Format DetectFormat(const Buf& buf) {
	if (buf.size < 0x38)
		return Format::Unknown;
	uint32_t version = ReadU32(buf.data + 0x0C);
	uint8_t ptrSize = buf.data[0x10]; // 4 or 8
	if (version == 11 && ptrSize == 8)
		return Format::Fallout64;
	if (version == 8 && ptrSize == 8)
		return Format::Skyrim64;
	if (version == 8 && ptrSize == 4)
		return Format::Skyrim32;
	return Format::Unknown;
}

// Parse the three packfile sections (__classnames__, __types__, __data__).
// FO4 (v11) has a 16-byte padding block before the section headers and uses
// 0x40-byte section headers; Skyrim (v8) uses 0x30-byte section headers and
// no padding.
bool ParseSections(const Buf& buf, Format fmt, Section out[3]) {
	const size_t hdrStart = 0x40;
	const size_t hdrSize = (fmt == Format::Fallout64) ? 0x40 : 0x30;
	const size_t firstSection = (fmt == Format::Fallout64) ? 0x50 : 0x40;

	(void)hdrStart;
	if (!buf.InBounds(firstSection, hdrSize * 3))
		return false;

	for (int i = 0; i < 3; ++i) {
		const uint8_t* h = buf.data + firstSection + i * hdrSize;
		const char* nameStart = reinterpret_cast<const char*>(h);
		const void* nameEnd = std::memchr(nameStart, '\0', 16);
		const size_t nameLen = nameEnd ? static_cast<const char*>(nameEnd) - nameStart : 16;
		out[i].name.assign(nameStart, nameLen);
		uint32_t s = ReadU32(h + 0x14);
		out[i].absStart = s;
		out[i].localFixupAbs = s + ReadU32(h + 0x18);
		out[i].globalFixupAbs = s + ReadU32(h + 0x1C);
		out[i].virtualFixupAbs = s + ReadU32(h + 0x20);
		out[i].exportsAbs = s + ReadU32(h + 0x24);
		out[i].endAbs = s + ReadU32(h + 0x2C);
		if (out[i].endAbs > buf.size)
			return false;
	}
	return true;
}

const Section* FindSection(const Section sec[3], const char* name) {
	for (int i = 0; i < 3; ++i)
		if (sec[i].name == name)
			return &sec[i];
	return nullptr;
}

bool ParseLocalFixups(const Buf& buf, const Section& dataSec, std::unordered_map<uint32_t, uint32_t>& out) {
	uint32_t pos = dataSec.localFixupAbs;
	uint32_t end = dataSec.globalFixupAbs;
	if (end > buf.size)
		return false;
	while (pos + 8 <= end) {
		uint32_t src = ReadU32(buf.data + pos);
		uint32_t dst = ReadU32(buf.data + pos + 4);
		if (src == 0xFFFFFFFFu)
			break;
		out[src] = dst;
		pos += 8;
	}
	return true;
}

bool ParseVirtualFixups(const Buf& buf, const Section& dataSec, uint32_t cnStart, std::vector<VirtualObject>& out) {
	uint32_t pos = dataSec.virtualFixupAbs;
	uint32_t end = dataSec.exportsAbs;
	if (end > buf.size)
		return false;
	while (pos + 12 <= end) {
		uint32_t src = ReadU32(buf.data + pos);
		// section index at +4 unused
		uint32_t nameOff = ReadU32(buf.data + pos + 8);
		if (src == 0xFFFFFFFFu)
			break;
		uint32_t absName = cnStart + nameOff;
		std::string cls;
		// Class names live in the classnames section as null-terminated
		// ASCII; cap the search to keep us bounded.
		if (absName < buf.size) {
			size_t scanEnd = std::min<size_t>(buf.size, absName + 256);
			size_t p = absName;
			while (p < scanEnd && buf.data[p] != 0)
				++p;
			cls.assign(reinterpret_cast<const char*>(buf.data + absName), p - absName);
		}
		out.push_back({src, std::move(cls)});
		pos += 12;
	}
	return true;
}

std::string ReadCString(const Buf& buf, size_t absOff) {
	if (absOff >= buf.size)
		return {};
	size_t scanEnd = std::min<size_t>(buf.size, absOff + 1024);
	size_t p = absOff;
	while (p < scanEnd && buf.data[p] != 0)
		++p;
	return std::string(reinterpret_cast<const char*>(buf.data + absOff), p - absOff);
}

// Resolve a pointer field at relative offset relPtrOff in the data section.
// Returns the relative target offset, or -1 if no fixup exists.
int64_t ResolveLocal(const std::unordered_map<uint32_t, uint32_t>& fixups, uint32_t relPtrOff) {
	auto it = fixups.find(relPtrOff);
	if (it == fixups.end())
		return -1;
	return it->second;
}

// Read an hkArray<T>'s element count (which sits at +ptrSize within the
// hkArray struct) plus its content offset (resolved through local fixups).
struct HkArray {
	uint32_t count = 0;
	int64_t contentRel = -1;
};

HkArray ReadHkArray(const Buf& buf,
					uint32_t dataAbs,
					uint32_t objRel,
					uint32_t fieldOff,
					uint32_t ptrSize,
					const std::unordered_map<uint32_t, uint32_t>& fixups) {
	HkArray a;
	uint32_t arrRel = objRel + fieldOff;
	uint32_t countAbs = dataAbs + arrRel + ptrSize;
	if (countAbs + 4 > buf.size)
		return a;
	a.count = ReadU32(buf.data + countAbs);
	a.contentRel = ResolveLocal(fixups, arrRel);
	return a;
}

inline uint32_t HkArrayStride(uint32_t ptrSize) { return ptrSize + 8; }

// ─── Quaternion helpers ───────────────────────────────────────────────────

void QuatNormalize(float q[4]) {
	float mag = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	if (mag < 1e-10f) {
		q[0] = 0.0f;
		q[1] = 0.0f;
		q[2] = 0.0f;
		q[3] = 1.0f;
		return;
	}
	float inv = 1.0f / mag;
	q[0] *= inv;
	q[1] *= inv;
	q[2] *= inv;
	q[3] *= inv;
}

// ─── Spline-compressed scalar/quat decoders ───────────────────────────────
// Ported from anim_fo4.py.

inline float Read8BitScalar(uint8_t v, float mn, float mx) {
	return mn + (mx - mn) * (float(v) / 255.0f);
}
inline float Read16BitScalar(uint16_t v, float mn, float mx) {
	return mn + (mx - mn) * (float(v) / 65535.0f);
}

bool Read32BitQuat(const uint8_t* d, size_t avail, float out[4]) {
	if (avail < 4)
		return false;
	uint32_t cval = ReadU32(d);
	const uint32_t r_mask = (1u << 10) - 1u;
	const float r_frac = 1.0f / float(r_mask);
	float R = float((cval >> 18) & r_mask) * r_frac;
	R = 1.0f - R * R;
	float phi_theta = float(cval & 0x3FFFFu);
	float phi = std::floor(std::sqrt(phi_theta));
	float theta = 0.0f;
	if (phi > 0.0f) {
		theta = (3.14159265358979323846f / 4.0f) * (phi_theta - phi * phi) / phi;
		phi = (3.14159265358979323846f / 2.0f / 511.0f) * phi;
	}
	float magnitude = std::sqrt(std::max(0.0f, 1.0f - R * R));
	float sp = std::sin(phi), cp = std::cos(phi);
	float st = std::sin(theta), ct = std::cos(theta);
	out[0] = sp * ct * magnitude;
	out[1] = sp * st * magnitude;
	out[2] = cp * magnitude;
	out[3] = R;
	const uint32_t signMasks[4] = {0x10000000u, 0x20000000u, 0x40000000u, 0x80000000u};
	for (int i = 0; i < 4; ++i)
		if (cval & signMasks[i])
			out[i] = -out[i];
	QuatNormalize(out);
	return true;
}

bool Read40BitQuat(const uint8_t* d, size_t avail, float out[4]) {
	if (avail < 5)
		return false;
	const float FRACTAL = 0.000345436f;
	uint64_t raw = 0;
	for (int i = 0; i < 5; ++i)
		raw |= uint64_t(d[i]) << (i * 8);
	uint32_t a = uint32_t(raw & 0xFFF);
	uint32_t b = uint32_t((raw >> 12) & 0xFFF);
	uint32_t c = uint32_t((raw >> 24) & 0xFFF);
	float vals[3] = {(float(a) - 2049.0f) * FRACTAL, (float(b) - 2049.0f) * FRACTAL, (float(c) - 2049.0f) * FRACTAL};
	float sumSq = vals[0] * vals[0] + vals[1] * vals[1] + vals[2] * vals[2];
	float w = std::sqrt(std::max(0.0f, 1.0f - sumSq));
	if ((raw >> 38) & 1)
		w = -w;
	uint32_t shift = uint32_t((raw >> 36) & 3);
	switch (shift) {
		case 0:
			out[0] = w;
			out[1] = vals[0];
			out[2] = vals[1];
			out[3] = vals[2];
			break;
		case 1:
			out[0] = vals[0];
			out[1] = w;
			out[2] = vals[1];
			out[3] = vals[2];
			break;
		case 2:
			out[0] = vals[0];
			out[1] = vals[1];
			out[2] = w;
			out[3] = vals[2];
			break;
		default:
			out[0] = vals[0];
			out[1] = vals[1];
			out[2] = vals[2];
			out[3] = w;
			break;
	}
	QuatNormalize(out);
	return true;
}

bool Read48BitQuat(const uint8_t* d, size_t avail, float out[4]) {
	if (avail < 6)
		return false;
	const float FRACTAL = 0.000043161f;
	const uint32_t MASK = (1u << 15) - 1u;
	const uint32_t HALF = MASK >> 1;
	uint16_t xRaw = ReadU16(d);
	uint16_t yRaw = ReadU16(d + 2);
	uint16_t zRaw = ReadU16(d + 4);
	uint32_t shift = ((uint32_t(yRaw) >> 14) & 2u) | ((uint32_t(xRaw) >> 15) & 1u);
	bool rSign = (zRaw >> 15) != 0;
	float vals[3] = {(float(xRaw & MASK) - float(HALF)) * FRACTAL,
					 (float(yRaw & MASK) - float(HALF)) * FRACTAL,
					 (float(zRaw & MASK) - float(HALF)) * FRACTAL};
	float sumSq = vals[0] * vals[0] + vals[1] * vals[1] + vals[2] * vals[2];
	float w = std::sqrt(std::max(0.0f, 1.0f - sumSq));
	if (rSign)
		w = -w;
	switch (shift) {
		case 0:
			out[0] = w;
			out[1] = vals[0];
			out[2] = vals[1];
			out[3] = vals[2];
			break;
		case 1:
			out[0] = vals[0];
			out[1] = w;
			out[2] = vals[1];
			out[3] = vals[2];
			break;
		case 2:
			out[0] = vals[0];
			out[1] = vals[1];
			out[2] = w;
			out[3] = vals[2];
			break;
		default:
			out[0] = vals[0];
			out[1] = vals[1];
			out[2] = vals[2];
			out[3] = w;
			break;
	}
	QuatNormalize(out);
	return true;
}

bool ReadUncompressedQuat(const uint8_t* d, size_t avail, float out[4]) {
	if (avail < 16)
		return false;
	out[0] = ReadF32(d);
	out[1] = ReadF32(d + 4);
	out[2] = ReadF32(d + 8);
	out[3] = ReadF32(d + 12);
	QuatNormalize(out);
	return true;
}

bool ReadQuat(uint32_t fmt, const uint8_t* d, size_t avail, float out[4], uint32_t& consumed) {
	switch (fmt) {
		case 0:
			consumed = 4;
			return Read32BitQuat(d, avail, out);
		case 1:
			consumed = 5;
			return Read40BitQuat(d, avail, out);
		case 2:
			consumed = 6;
			return Read48BitQuat(d, avail, out);
		case 5:
			consumed = 16;
			return ReadUncompressedQuat(d, avail, out);
		default:
			// Fallback to 40-bit (matches PyNifly behavior).
			consumed = 5;
			return Read40BitQuat(d, avail, out);
	}
}

uint32_t QuatAlign(uint32_t fmt) {
	switch (fmt) {
		case 0: return 4;
		case 1: return 1;
		case 2: return 2;
		case 3: return 1;
		case 4: return 2;
		case 5: return 4;
		default: return 1;
	}
}

inline uint32_t Align(uint32_t off, uint32_t alignment) {
	uint32_t r = off % alignment;
	return r ? off + (alignment - r) : off;
}

// ─── B-spline evaluation (De Boor) ────────────────────────────────────────

uint32_t FindKnotSpan(uint32_t degree, float value, uint32_t numCp, const std::vector<float>& knots) {
	if (numCp == 0)
		return 0;
	if (value >= knots[numCp])
		return numCp - 1;
	uint32_t low = degree;
	uint32_t high = numCp;
	uint32_t mid = (low + high) / 2;
	for (int it = 0; it < 100; ++it) {
		if (value < knots[mid])
			high = mid;
		else if (value >= knots[mid + 1])
			low = mid;
		else
			break;
		mid = (low + high) / 2;
	}
	return mid;
}

// Evaluate B-spline scalar.
float EvalBSplineScalar(uint32_t knotSpan, uint32_t degree, float t, const std::vector<float>& knots, const std::vector<float>& cps) {
	if (cps.empty())
		return 0.0f;
	if (cps.size() == 1)
		return cps[0];
	std::vector<float> N(degree + 1, 0.0f);
	N[0] = 1.0f;
	for (uint32_t i = 1; i <= degree; ++i) {
		for (int j = int(i) - 1; j >= 0; --j) {
			float denom = knots[knotSpan + i - j] - knots[knotSpan - j];
			float A = (denom >= 1e-10f) ? (t - knots[knotSpan - j]) / denom : 0.0f;
			float tmp = N[j] * A;
			if (uint32_t(j) + 1 < N.size())
				N[j + 1] += N[j] - tmp;
			N[j] = tmp;
		}
	}
	float result = 0.0f;
	for (uint32_t i = 0; i <= degree; ++i) {
		int idx = int(knotSpan) - int(i);
		if (idx >= 0 && idx < int(cps.size()))
			result += cps[idx] * N[i];
	}
	return result;
}

// Evaluate B-spline 4-vector (quaternion).
void EvalBSplineQuat(uint32_t knotSpan,
					 uint32_t degree,
					 float t,
					 const std::vector<float>& knots,
					 const std::vector<std::array<float, 4>>& cps,
					 float out[4]) {
	out[0] = out[1] = out[2] = 0.0f;
	out[3] = 1.0f;
	if (cps.empty())
		return;
	if (cps.size() == 1) {
		out[0] = cps[0][0];
		out[1] = cps[0][1];
		out[2] = cps[0][2];
		out[3] = cps[0][3];
		return;
	}
	std::vector<float> N(degree + 1, 0.0f);
	N[0] = 1.0f;
	for (uint32_t i = 1; i <= degree; ++i) {
		for (int j = int(i) - 1; j >= 0; --j) {
			float denom = knots[knotSpan + i - j] - knots[knotSpan - j];
			float A = (denom >= 1e-10f) ? (t - knots[knotSpan - j]) / denom : 0.0f;
			float tmp = N[j] * A;
			if (uint32_t(j) + 1 < N.size())
				N[j + 1] += N[j] - tmp;
			N[j] = tmp;
		}
	}
	float r[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	for (uint32_t i = 0; i <= degree; ++i) {
		int idx = int(knotSpan) - int(i);
		if (idx >= 0 && idx < int(cps.size())) {
			r[0] += cps[idx][0] * N[i];
			r[1] += cps[idx][1] * N[i];
			r[2] += cps[idx][2] * N[i];
			r[3] += cps[idx][3] * N[i];
		}
	}
	out[0] = r[0];
	out[1] = r[1];
	out[2] = r[2];
	out[3] = r[3];
}

// ─── Track mask interpretation ────────────────────────────────────────────

struct TrackMask {
	uint8_t posQuant;   // 0 = 8-bit, 1 = 16-bit
	uint8_t rotQuant;   // quaternion format index (0,1,2,5...)
	uint8_t scaleQuant; // 0 = 8-bit, 1 = 16-bit
	uint8_t posFlags;
	uint8_t rotFlags;
	uint8_t scaleFlags;
	enum Type { Identity, Static, Spline };

	Type PosType(int axis) const {
		if ((posFlags >> (axis + 4)) & 1)
			return Spline;
		if ((posFlags >> axis) & 1)
			return Static;
		return Identity;
	}
	Type ScaleType(int axis) const {
		if ((scaleFlags >> (axis + 4)) & 1)
			return Spline;
		if ((scaleFlags >> axis) & 1)
			return Static;
		return Identity;
	}
	Type RotType() const {
		if ((rotFlags >> 4) & 0x0F)
			return Spline;
		if (rotFlags & 0x0F)
			return Static;
		return Identity;
	}
	bool HasAnyPosSpline() const { return PosType(0) == Spline || PosType(1) == Spline || PosType(2) == Spline; }
	bool HasAnyScaleSpline() const { return ScaleType(0) == Spline || ScaleType(1) == Spline || ScaleType(2) == Spline; }
};

// ─── Spline decompression (block-by-block) ────────────────────────────────

bool DecompressSpline(const uint8_t* blob,
					  size_t blobSize,
					  uint32_t numTracks,
					  uint32_t numFrames,
					  uint32_t numBlocks,
					  uint32_t maxFramesPerBlock,
					  const std::vector<uint32_t>& blockOffsets,
					  uint32_t maskAndQuantSize,
					  std::vector<Transform>& out) {
	out.assign(size_t(numFrames) * numTracks, Transform{});
	if (numTracks == 0 || numFrames == 0 || numBlocks == 0)
		return true;
	if (blockOffsets.size() < numBlocks)
		return false;
	if (maskAndQuantSize == 0)
		maskAndQuantSize = Align(4u * numTracks, 4u);

	std::vector<TrackMask> masks(numTracks);
	std::vector<float> knots;
	std::vector<float> scalarCps[3];
	std::vector<std::array<float, 4>> quatCps;

	auto ensureBytes = [&](uint32_t off, uint32_t need) -> bool { return uint64_t(off) + need <= blobSize; };

	for (uint32_t blockIdx = 0; blockIdx < numBlocks; ++blockIdx) {
		uint32_t blockStart = blockOffsets[blockIdx];
		uint32_t firstFrame = blockIdx * maxFramesPerBlock;
		uint32_t framesInBlock = (blockIdx == numBlocks - 1) ? (numFrames - firstFrame) : maxFramesPerBlock;

		// Parse masks (4 bytes per track).
		uint32_t off = blockStart;
		if (!ensureBytes(off, 4u * numTracks))
			return false;
		for (uint32_t t = 0; t < numTracks; ++t) {
			masks[t].posQuant = blob[off] & 0x03;
			masks[t].rotQuant = (blob[off] >> 2) & 0x0F;
			masks[t].scaleQuant = (blob[off] >> 6) & 0x03;
			masks[t].posFlags = blob[off + 1];
			masks[t].rotFlags = blob[off + 2];
			masks[t].scaleFlags = blob[off + 3];
			off += 4;
		}
		off = blockStart + maskAndQuantSize;

		for (uint32_t trackIdx = 0; trackIdx < numTracks; ++trackIdx) {
			const TrackMask& m = masks[trackIdx];

			// ─── POSITION ───
			std::vector<std::array<float, 3>> posFrames(framesInBlock, {0.0f, 0.0f, 0.0f});
			if (m.HasAnyPosSpline()) {
				if (!ensureBytes(off, 3))
					return false;
				uint32_t numItems = ReadU16(blob + off);
				uint32_t degree = blob[off + 2];
				off += 3;
				uint32_t numKnots = numItems + degree + 2;
				if (!ensureBytes(off, numKnots))
					return false;
				knots.assign(numKnots, 0.0f);
				for (uint32_t k = 0; k < numKnots; ++k)
					knots[k] = float(blob[off + k]);
				off += numKnots;
				off = Align(off, 4);

				struct AxisInfo {
					TrackMask::Type type;
					float mn, mx;
				};
				AxisInfo info[3];
				for (int axis = 0; axis < 3; ++axis) {
					TrackMask::Type t = m.PosType(axis);
					if (t == TrackMask::Spline) {
						if (!ensureBytes(off, 8))
							return false;
						info[axis] = {t, ReadF32(blob + off), ReadF32(blob + off + 4)};
						off += 8;
					}
					else if (t == TrackMask::Static) {
						if (!ensureBytes(off, 4))
							return false;
						float v = ReadF32(blob + off);
						info[axis] = {t, v, v};
						off += 4;
					}
					else {
						info[axis] = {t, 0.0f, 0.0f};
					}
				}
				for (int axis = 0; axis < 3; ++axis)
					scalarCps[axis].clear();
				for (uint32_t i = 0; i < numItems + 1; ++i) {
					for (int axis = 0; axis < 3; ++axis) {
						if (info[axis].type == TrackMask::Spline) {
							if (m.posQuant == 0) {
								if (!ensureBytes(off, 1))
									return false;
								scalarCps[axis].push_back(Read8BitScalar(blob[off], info[axis].mn, info[axis].mx));
								off += 1;
							}
							else {
								if (!ensureBytes(off, 2))
									return false;
								scalarCps[axis].push_back(Read16BitScalar(ReadU16(blob + off), info[axis].mn, info[axis].mx));
								off += 2;
							}
						}
					}
				}
				off = Align(off, 4);

				for (uint32_t f = 0; f < framesInBlock; ++f) {
					float ft = float(f);
					float pos[3] = {0.0f, 0.0f, 0.0f};
					for (int axis = 0; axis < 3; ++axis) {
						if (info[axis].type == TrackMask::Spline) {
							uint32_t span = FindKnotSpan(degree, ft, uint32_t(scalarCps[axis].size()), knots);
							pos[axis] = EvalBSplineScalar(span, degree, ft, knots, scalarCps[axis]);
						}
						else if (info[axis].type == TrackMask::Static) {
							pos[axis] = info[axis].mn;
						}
					}
					posFrames[f] = {pos[0], pos[1], pos[2]};
				}
			}
			else {
				float pos[3] = {0.0f, 0.0f, 0.0f};
				for (int axis = 0; axis < 3; ++axis) {
					if (m.PosType(axis) == TrackMask::Static) {
						if (!ensureBytes(off, 4))
							return false;
						pos[axis] = ReadF32(blob + off);
						off += 4;
					}
				}
				for (uint32_t f = 0; f < framesInBlock; ++f)
					posFrames[f] = {pos[0], pos[1], pos[2]};
			}
			off = Align(off, 4);

			// ─── ROTATION ───
			std::vector<std::array<float, 4>> rotFrames(framesInBlock, {0.0f, 0.0f, 0.0f, 1.0f});
			TrackMask::Type rotType = m.RotType();
			uint32_t qfmt = m.rotQuant;
			uint32_t qalign = QuatAlign(qfmt);

			if (rotType == TrackMask::Spline) {
				if (!ensureBytes(off, 3))
					return false;
				uint32_t numItems = ReadU16(blob + off);
				uint32_t degree = blob[off + 2];
				off += 3;
				uint32_t numKnots = numItems + degree + 2;
				if (!ensureBytes(off, numKnots))
					return false;
				knots.assign(numKnots, 0.0f);
				for (uint32_t k = 0; k < numKnots; ++k)
					knots[k] = float(blob[off + k]);
				off += numKnots;
				if (qalign > 1)
					off = Align(off, qalign);

				quatCps.clear();
				for (uint32_t i = 0; i < numItems + 1; ++i) {
					float q[4];
					uint32_t consumed = 0;
					if (!ReadQuat(qfmt, blob + off, blobSize - off, q, consumed))
						return false;
					off += consumed;
					if (!quatCps.empty()) {
						const auto& prev = quatCps.back();
						float dot = q[0] * prev[0] + q[1] * prev[1] + q[2] * prev[2] + q[3] * prev[3];
						if (dot < 0.0f) {
							q[0] = -q[0];
							q[1] = -q[1];
							q[2] = -q[2];
							q[3] = -q[3];
						}
					}
					quatCps.push_back({q[0], q[1], q[2], q[3]});
				}
				for (uint32_t f = 0; f < framesInBlock; ++f) {
					float ft = float(f);
					uint32_t span = FindKnotSpan(degree, ft, uint32_t(quatCps.size()), knots);
					float q[4];
					EvalBSplineQuat(span, degree, ft, knots, quatCps, q);
					QuatNormalize(q);
					rotFrames[f] = {q[0], q[1], q[2], q[3]};
				}
			}
			else if (rotType == TrackMask::Static) {
				if (qalign > 1)
					off = Align(off, qalign);
				float q[4];
				uint32_t consumed = 0;
				if (!ReadQuat(qfmt, blob + off, blobSize - off, q, consumed))
					return false;
				off += consumed;
				for (uint32_t f = 0; f < framesInBlock; ++f)
					rotFrames[f] = {q[0], q[1], q[2], q[3]};
			}
			off = Align(off, 4);

			// ─── SCALE ───
			std::vector<std::array<float, 3>> scaleFrames(framesInBlock, {1.0f, 1.0f, 1.0f});
			if (m.HasAnyScaleSpline()) {
				if (!ensureBytes(off, 3))
					return false;
				uint32_t numItems = ReadU16(blob + off);
				uint32_t degree = blob[off + 2];
				off += 3;
				uint32_t numKnots = numItems + degree + 2;
				if (!ensureBytes(off, numKnots))
					return false;
				knots.assign(numKnots, 0.0f);
				for (uint32_t k = 0; k < numKnots; ++k)
					knots[k] = float(blob[off + k]);
				off += numKnots;
				off = Align(off, 4);

				struct AxisInfo {
					TrackMask::Type type;
					float mn, mx;
				};
				AxisInfo info[3];
				for (int axis = 0; axis < 3; ++axis) {
					TrackMask::Type t = m.ScaleType(axis);
					if (t == TrackMask::Spline) {
						if (!ensureBytes(off, 8))
							return false;
						info[axis] = {t, ReadF32(blob + off), ReadF32(blob + off + 4)};
						off += 8;
					}
					else if (t == TrackMask::Static) {
						if (!ensureBytes(off, 4))
							return false;
						float v = ReadF32(blob + off);
						info[axis] = {t, v, v};
						off += 4;
					}
					else {
						info[axis] = {t, 1.0f, 1.0f};
					}
				}
				for (int axis = 0; axis < 3; ++axis)
					scalarCps[axis].clear();
				for (uint32_t i = 0; i < numItems + 1; ++i) {
					for (int axis = 0; axis < 3; ++axis) {
						if (info[axis].type == TrackMask::Spline) {
							if (m.scaleQuant == 0) {
								if (!ensureBytes(off, 1))
									return false;
								scalarCps[axis].push_back(Read8BitScalar(blob[off], info[axis].mn, info[axis].mx));
								off += 1;
							}
							else {
								if (!ensureBytes(off, 2))
									return false;
								scalarCps[axis].push_back(Read16BitScalar(ReadU16(blob + off), info[axis].mn, info[axis].mx));
								off += 2;
							}
						}
					}
				}
				off = Align(off, 4);

				for (uint32_t f = 0; f < framesInBlock; ++f) {
					float ft = float(f);
					float s[3] = {1.0f, 1.0f, 1.0f};
					for (int axis = 0; axis < 3; ++axis) {
						if (info[axis].type == TrackMask::Spline) {
							uint32_t span = FindKnotSpan(degree, ft, uint32_t(scalarCps[axis].size()), knots);
							s[axis] = EvalBSplineScalar(span, degree, ft, knots, scalarCps[axis]);
						}
						else if (info[axis].type == TrackMask::Static) {
							s[axis] = info[axis].mn;
						}
					}
					scaleFrames[f] = {s[0], s[1], s[2]};
				}
			}
			else {
				float s[3] = {1.0f, 1.0f, 1.0f};
				for (int axis = 0; axis < 3; ++axis) {
					if (m.ScaleType(axis) == TrackMask::Static) {
						if (!ensureBytes(off, 4))
							return false;
						s[axis] = ReadF32(blob + off);
						off += 4;
					}
				}
				for (uint32_t f = 0; f < framesInBlock; ++f)
					scaleFrames[f] = {s[0], s[1], s[2]};
			}
			off = Align(off, 4);

			// Write into the global output array.
			for (uint32_t f = 0; f < framesInBlock; ++f) {
				Transform& dst = out[size_t(firstFrame + f) * numTracks + trackIdx];
				dst.translation[0] = posFrames[f][0];
				dst.translation[1] = posFrames[f][1];
				dst.translation[2] = posFrames[f][2];
				dst.rotation[0] = rotFrames[f][0];
				dst.rotation[1] = rotFrames[f][1];
				dst.rotation[2] = rotFrames[f][2];
				dst.rotation[3] = rotFrames[f][3];
				dst.scale[0] = scaleFrames[f][0];
				dst.scale[1] = scaleFrames[f][1];
				dst.scale[2] = scaleFrames[f][2];
			}
		}
	}
	return true;
}

// ─── Object parsing ───────────────────────────────────────────────────────

bool ParseSkeleton(const Buf& buf,
				   uint32_t dataAbs,
				   uint32_t skelRel,
				   uint32_t ptrSize,
				   Format fmt,
				   const std::unordered_map<uint32_t, uint32_t>& fixups,
				   Skeleton& outSkel) {
	const uint32_t baseSize = 2u * ptrSize;
	const uint32_t arrSize = HkArrayStride(ptrSize);

	const uint32_t nameOff = baseSize;
	const uint32_t piOff = nameOff + ptrSize;
	const uint32_t bonesOff = piOff + arrSize;
	const uint32_t poseOff = bonesOff + arrSize;

	int64_t nameTarget = ResolveLocal(fixups, skelRel + nameOff);
	if (nameTarget >= 0)
		outSkel.name = ReadCString(buf, dataAbs + uint32_t(nameTarget));

	auto pi = ReadHkArray(buf, dataAbs, skelRel, piOff, ptrSize, fixups);
	std::vector<int16_t> parents;
	if (pi.contentRel >= 0 && pi.count > 0) {
		uint32_t piAbs = dataAbs + uint32_t(pi.contentRel);
		if (!buf.InBounds(piAbs, size_t(pi.count) * 2))
			return false;
		parents.resize(pi.count);
		for (uint32_t i = 0; i < pi.count; ++i)
			parents[i] = ReadI16(buf.data + piAbs + i * 2);
	}

	auto bones = ReadHkArray(buf, dataAbs, skelRel, bonesOff, ptrSize, fixups);
	uint32_t boneStride = (ptrSize == 8) ? 16u : (ptrSize + 4u);
	(void)fmt;
	if (bones.contentRel >= 0 && bones.count > 0) {
		uint32_t boneContentAbs = dataAbs + uint32_t(bones.contentRel);
		if (!buf.InBounds(boneContentAbs, size_t(bones.count) * boneStride))
			return false;
		outSkel.bones.resize(bones.count);
		for (uint32_t i = 0; i < bones.count; ++i) {
			uint32_t boneRel = uint32_t(bones.contentRel) + i * boneStride;
			Bone& b = outSkel.bones[i];
			int64_t strRel = ResolveLocal(fixups, boneRel);
			if (strRel >= 0)
				b.name = ReadCString(buf, dataAbs + uint32_t(strRel));
			b.lockTranslation = (buf.data[dataAbs + boneRel + ptrSize] != 0);
			b.parentIndex = (i < parents.size()) ? parents[i] : -1;
		}
	}

	auto pose = ReadHkArray(buf, dataAbs, skelRel, poseOff, ptrSize, fixups);
	if (pose.contentRel >= 0 && pose.count > 0) {
		const uint32_t POSE_STRIDE = 0x30;
		uint32_t poseContentAbs = dataAbs + uint32_t(pose.contentRel);
		if (!buf.InBounds(poseContentAbs, size_t(pose.count) * POSE_STRIDE))
			return false;
		outSkel.referencePose.resize(pose.count);
		for (uint32_t i = 0; i < pose.count; ++i) {
			const uint8_t* p = buf.data + poseContentAbs + i * POSE_STRIDE;
			Transform& t = outSkel.referencePose[i];
			t.translation[0] = ReadF32(p + 0);
			t.translation[1] = ReadF32(p + 4);
			t.translation[2] = ReadF32(p + 8);
			t.rotation[0] = ReadF32(p + 16);
			t.rotation[1] = ReadF32(p + 20);
			t.rotation[2] = ReadF32(p + 24);
			t.rotation[3] = ReadF32(p + 28);
			t.scale[0] = ReadF32(p + 32);
			t.scale[1] = ReadF32(p + 36);
			t.scale[2] = ReadF32(p + 40);
		}
	}
	return true;
}

bool ParseAnimation(const Buf& buf,
					uint32_t dataAbs,
					uint32_t animRel,
					uint32_t ptrSize,
					const std::unordered_map<uint32_t, uint32_t>& fixups,
					Animation& outAnim,
					std::string* errorOut) {
	const uint32_t base = 2u * ptrSize;
	const uint32_t arrSize = HkArrayStride(ptrSize);

	// hkaAnimation members
	const uint32_t oType = base;
	(void)oType;
	const uint32_t oDuration = base + 4;
	const uint32_t oNumTracks = base + 8;
	const uint32_t oNumFloatTracks = base + 12;
	(void)oNumFloatTracks;
	const uint32_t oExtractedMotion = base + 16;
	const uint32_t oAnnTracks = oExtractedMotion + ptrSize;

	// hkaSplineCompressedAnimation members
	const uint32_t splineBase = oAnnTracks + arrSize;
	const uint32_t oNumFrames = splineBase;
	const uint32_t oNumBlocks = splineBase + 4;
	const uint32_t oMaxFrames = splineBase + 8;
	const uint32_t oMaskQuant = splineBase + 12;
	const uint32_t oBlockDur = splineBase + 16;
	(void)oBlockDur;
	const uint32_t oFrameDur = splineBase + 24;
	uint32_t alignedAfter = splineBase + 28;
	if (ptrSize > 1)
		alignedAfter = (alignedAfter + ptrSize - 1) & ~(ptrSize - 1);
	const uint32_t oBlockOffsets = alignedAfter;
	const uint32_t oFloatBlockOffsets = oBlockOffsets + arrSize;
	(void)oFloatBlockOffsets;
	const uint32_t oTransformOffsets = oFloatBlockOffsets + arrSize;
	(void)oTransformOffsets;
	const uint32_t oFloatOffsets = oTransformOffsets + arrSize;
	(void)oFloatOffsets;
	const uint32_t oData = oFloatOffsets + arrSize;

	uint32_t a = dataAbs + animRel;
	if (!buf.InBounds(a, oData + arrSize)) {
		if (errorOut)
			*errorOut = "Animation object truncated";
		return false;
	}

	outAnim.duration = ReadF32(buf.data + a + oDuration);
	outAnim.numTransformTracks = ReadU32(buf.data + a + oNumTracks);
	outAnim.numFrames = ReadU32(buf.data + a + oNumFrames);
	uint32_t numBlocks = ReadU32(buf.data + a + oNumBlocks);
	uint32_t maxFramesPerBlock = ReadU32(buf.data + a + oMaxFrames);
	uint32_t maskAndQuantSize = ReadU32(buf.data + a + oMaskQuant);
	outAnim.frameDuration = ReadF32(buf.data + a + oFrameDur);

	// Block offsets (hkArray<u32>).
	std::vector<uint32_t> blockOffsets;
	{
		auto ar = ReadHkArray(buf, dataAbs, animRel, oBlockOffsets, ptrSize, fixups);
		if (ar.contentRel >= 0 && ar.count > 0) {
			uint32_t cAbs = dataAbs + uint32_t(ar.contentRel);
			if (!buf.InBounds(cAbs, size_t(ar.count) * 4)) {
				if (errorOut)
					*errorOut = "Block offsets truncated";
				return false;
			}
			blockOffsets.resize(ar.count);
			for (uint32_t i = 0; i < ar.count; ++i)
				blockOffsets[i] = ReadU32(buf.data + cAbs + i * 4);
		}
	}

	// Spline data blob (hkArray<u8>).
	const uint8_t* blob = nullptr;
	uint32_t blobSize = 0;
	{
		auto ar = ReadHkArray(buf, dataAbs, animRel, oData, ptrSize, fixups);
		if (ar.contentRel >= 0 && ar.count > 0) {
			uint32_t cAbs = dataAbs + uint32_t(ar.contentRel);
			if (!buf.InBounds(cAbs, ar.count)) {
				if (errorOut)
					*errorOut = "Spline blob truncated";
				return false;
			}
			blob = buf.data + cAbs;
			blobSize = ar.count;
		}
	}

	if (!blob || blockOffsets.empty()) {
		if (errorOut)
			*errorOut = "Animation has no spline data";
		return false;
	}

	// Annotation tracks: per-track names.
	{
		auto ar = ReadHkArray(buf, dataAbs, animRel, oAnnTracks, ptrSize, fixups);
		if (ar.contentRel >= 0 && ar.count > 0) {
			const uint32_t annStride = ptrSize + arrSize;
			outAnim.trackNames.assign(ar.count, std::string{});
			for (uint32_t i = 0; i < ar.count; ++i) {
				uint32_t trackRel = uint32_t(ar.contentRel) + i * annStride;
				int64_t strRel = ResolveLocal(fixups, trackRel);
				if (strRel >= 0)
					outAnim.trackNames[i] = ReadCString(buf, dataAbs + uint32_t(strRel));
			}
		}
	}

	if (!DecompressSpline(blob, blobSize, outAnim.numTransformTracks, outAnim.numFrames, numBlocks, maxFramesPerBlock, blockOffsets, maskAndQuantSize, outAnim.trackTransforms)) {
		if (errorOut)
			*errorOut = "Spline decompression failed";
		return false;
	}
	return true;
}

bool ParseAnimationBinding(const Buf& buf,
						   uint32_t dataAbs,
						   uint32_t bindRel,
						   uint32_t ptrSize,
						   const std::unordered_map<uint32_t, uint32_t>& fixups,
						   AnimationBinding& outBind) {
	const uint32_t base = 2u * ptrSize;
	const uint32_t arrSize = HkArrayStride(ptrSize);
	const uint32_t bindNameOff = base;
	const uint32_t bindAnimOff = bindNameOff + ptrSize;
	(void)bindAnimOff;
	const uint32_t bindIdxOff = bindAnimOff + ptrSize;

	int64_t skelNameRel = ResolveLocal(fixups, bindRel + bindNameOff);
	if (skelNameRel >= 0)
		outBind.originalSkeletonName = ReadCString(buf, dataAbs + uint32_t(skelNameRel));

	auto idxArr = ReadHkArray(buf, dataAbs, bindRel, bindIdxOff, ptrSize, fixups);
	if (idxArr.contentRel >= 0 && idxArr.count > 0) {
		uint32_t cAbs = dataAbs + uint32_t(idxArr.contentRel);
		if (buf.InBounds(cAbs, size_t(idxArr.count) * 2)) {
			outBind.transformTrackToBoneIndices.resize(idxArr.count);
			for (uint32_t i = 0; i < idxArr.count; ++i)
				outBind.transformTrackToBoneIndices[i] = ReadI16(buf.data + cAbs + i * 2);
		}
	}

	// blendHint follows the two hkArray fields after bindIdxOff.
	uint32_t blendHintOff = bindIdxOff + 2u * arrSize;
	uint32_t blendAbs = dataAbs + bindRel + blendHintOff;
	if (buf.InBounds(blendAbs, 4))
		outBind.blendHint = ReadI32(buf.data + blendAbs);
	return true;
}

} // namespace

bool File::SavePoseAnimation(const std::string& path,
							 Format format,
							 const std::vector<Transform>& transforms,
							 const SaveAnimationOptions& options,
							 std::string* errorOut) {
	const FormatTraits* traits = GetFormatTraits(format);
	if (!traits) {
		if (errorOut)
			*errorOut = "Unsupported HKX variant";
		return false;
	}
	if (transforms.empty()) {
		if (errorOut)
			*errorOut = "No transform tracks to serialize";
		return false;
	}

	const std::string containerName = options.containerName.empty() ? "Merged Animation Container" : options.containerName;
	const std::string skeletonName = options.originalSkeletonName.empty() ? "Skeleton" : options.originalSkeletonName;
	const uint32_t ptrSize = traits->ptrSize;
	const uint32_t arrSize = HkArrayStride(ptrSize);
	const uint32_t baseSize = 2u * ptrSize;
	const uint32_t numTracks = static_cast<uint32_t>(transforms.size());
	const uint32_t maxFramesPerBlock = 256u;
	const uint32_t maskAndQuantSize = AlignUp(4u * numTracks, 4u);

	BuiltClassNames classNames = BuildClassNames(*traits);

	std::vector<uint8_t> dataBytes;
	std::vector<LocalFixup> localFixups;
	std::vector<GlobalFixup> globalFixups;
	std::vector<VirtualFixup> virtualFixups;

	dataBytes.resize(ptrSize + 8u, 0);
	PatchU32(dataBytes, ptrSize, 1u);
	PatchU32(dataBytes, ptrSize + 4u, 0x80000001u);
	AlignBuffer(dataBytes, 16u);

	const uint32_t rootRel = 0u;
	const uint32_t namedVariantRel = static_cast<uint32_t>(dataBytes.size());
	localFixups.push_back({rootRel, namedVariantRel});
	dataBytes.resize(namedVariantRel + 3u * ptrSize, 0);

	const uint32_t rootNameRel = static_cast<uint32_t>(dataBytes.size());
	AppendStringZ(dataBytes, containerName);
	const uint32_t rootClassRel = AlignUp(static_cast<uint32_t>(dataBytes.size()), 16u);
	AlignBuffer(dataBytes, 16u);
	AppendStringZ(dataBytes, "hkaAnimationContainer");
	AlignBuffer(dataBytes, 16u);

	localFixups.push_back({namedVariantRel, rootNameRel});
	localFixups.push_back({namedVariantRel + ptrSize, rootClassRel});

	const uint32_t containerRel = static_cast<uint32_t>(dataBytes.size());
	globalFixups.push_back({namedVariantRel + 2u * ptrSize, containerRel});
	virtualFixups.push_back({rootRel, classNames.nameOffsets.at("hkRootLevelContainer")});

	dataBytes.resize(containerRel + traits->containerSize, 0);
	const uint32_t containerBase = (ptrSize == 4u) ? 8u : 16u;
	const uint32_t animationsArrayField = containerRel + containerBase + arrSize;
	const uint32_t bindingsArrayField = containerRel + containerBase + 2u * arrSize;
	SetArrayHeader(dataBytes, containerRel + containerBase, ptrSize, 0u, 0u, localFixups);
	SetArrayHeader(dataBytes, containerRel + containerBase + 3u * arrSize, ptrSize, 0u, 0u, localFixups);
	SetArrayHeader(dataBytes, containerRel + containerBase + 4u * arrSize, ptrSize, 0u, 0u, localFixups);
	virtualFixups.push_back({containerRel, classNames.nameOffsets.at("hkaAnimationContainer")});

	AlignBuffer(dataBytes, 16u);
	const uint32_t animRel = static_cast<uint32_t>(dataBytes.size());
	const uint32_t oDuration = baseSize + 4u;
	const uint32_t oNumTracks = baseSize + 8u;
	const uint32_t oNumFloatTracks = baseSize + 12u;
	const uint32_t oExtractedMotion = baseSize + 16u;
	const uint32_t oAnnTracks = oExtractedMotion + ptrSize;
	const uint32_t splineBase = oAnnTracks + arrSize;
	const uint32_t oNumFrames = splineBase;
	const uint32_t oNumBlocks = splineBase + 4u;
	const uint32_t oMaxFrames = splineBase + 8u;
	const uint32_t oMaskQuant = splineBase + 12u;
	const uint32_t oBlockDur = splineBase + 16u;
	const uint32_t oFrameDur = splineBase + 24u;
	uint32_t alignedAfter = splineBase + 28u;
	if (ptrSize > 1u)
		alignedAfter = AlignUp(alignedAfter, ptrSize);
	const uint32_t oBlockOffsets = alignedAfter;
	const uint32_t oFloatBlockOffsets = oBlockOffsets + arrSize;
	const uint32_t oTransformOffsets = oFloatBlockOffsets + arrSize;
	const uint32_t oFloatOffsets = oTransformOffsets + arrSize;
	const uint32_t oData = oFloatOffsets + arrSize;
	const uint32_t animHeaderSize = AlignUp(oData + arrSize, 16u);
	dataBytes.resize(animRel + animHeaderSize, 0);

	PatchU32(dataBytes, animRel + baseSize, traits->animationType);
	PatchF32(dataBytes, animRel + oDuration, 1.0f / 30.0f);
	PatchU32(dataBytes, animRel + oNumTracks, numTracks);
	PatchU32(dataBytes, animRel + oNumFloatTracks, 0u);
	SetArrayHeader(dataBytes, animRel + oAnnTracks, ptrSize, 0u, 0u, localFixups);
	PatchU32(dataBytes, animRel + oNumFrames, 1u);
	PatchU32(dataBytes, animRel + oNumBlocks, 1u);
	PatchU32(dataBytes, animRel + oMaxFrames, maxFramesPerBlock);
	PatchU32(dataBytes, animRel + oMaskQuant, maskAndQuantSize);
	PatchF32(dataBytes, animRel + oBlockDur, std::numeric_limits<float>::infinity());
	PatchF32(dataBytes, animRel + oFrameDur, std::numeric_limits<float>::infinity());
	SetArrayHeader(dataBytes, animRel + oTransformOffsets, ptrSize, 0u, 0u, localFixups);
	SetArrayHeader(dataBytes, animRel + oFloatOffsets, ptrSize, 0u, 0u, localFixups);
	virtualFixups.push_back({animRel, classNames.nameOffsets.at("hkaSplineCompressedAnimation")});

	AlignBuffer(dataBytes, 16u);
	const uint32_t bindingRel = static_cast<uint32_t>(dataBytes.size());
	const uint32_t bindNameOff = baseSize;
	const uint32_t bindAnimOff = bindNameOff + ptrSize;
	const uint32_t bindIdxOff = bindAnimOff + ptrSize;
	const uint32_t bindBlendHintOff = bindIdxOff + 2u * arrSize;
	const uint32_t bindingSize = AlignUp(bindBlendHintOff + 4u, 16u);
	dataBytes.resize(bindingRel + bindingSize, 0);
	SetArrayHeader(dataBytes, bindingRel + bindIdxOff, ptrSize, 0u, 0u, localFixups);
	SetArrayHeader(dataBytes, bindingRel + bindIdxOff + arrSize, ptrSize, 0u, 0u, localFixups);
	PatchU32(dataBytes, bindingRel + bindBlendHintOff, 0u);
	globalFixups.push_back({bindingRel + bindAnimOff, animRel});
	virtualFixups.push_back({bindingRel, classNames.nameOffsets.at("hkaAnimationBinding")});

	const std::vector<uint8_t> blob = BuildStaticPoseBlob(transforms);

	AlignBuffer(dataBytes, ptrSize);
	const uint32_t containerAnimationsRel = static_cast<uint32_t>(dataBytes.size());
	dataBytes.resize(containerAnimationsRel + ptrSize, 0);
	AlignBuffer(dataBytes, ptrSize);
	const uint32_t containerBindingsRel = static_cast<uint32_t>(dataBytes.size());
	dataBytes.resize(containerBindingsRel + ptrSize, 0);
	AlignBuffer(dataBytes, 4u);
	const uint32_t blockOffsetsRel = static_cast<uint32_t>(dataBytes.size());
	AppendU32(dataBytes, 0u);
	AlignBuffer(dataBytes, 4u);
	const uint32_t floatBlockOffsetsRel = static_cast<uint32_t>(dataBytes.size());
	AppendU32(dataBytes, 0u);
	AlignBuffer(dataBytes, 4u);
	const uint32_t blobRel = static_cast<uint32_t>(dataBytes.size());
	dataBytes.insert(dataBytes.end(), blob.begin(), blob.end());
	AlignBuffer(dataBytes, 16u);
	const uint32_t bindingNameRel = static_cast<uint32_t>(dataBytes.size());
	AppendStringZ(dataBytes, skeletonName);
	AlignBuffer(dataBytes, 16u);

	SetArrayHeader(dataBytes, animationsArrayField, ptrSize, 1u, containerAnimationsRel, localFixups);
	SetArrayHeader(dataBytes, bindingsArrayField, ptrSize, 1u, containerBindingsRel, localFixups);
	globalFixups.push_back({containerAnimationsRel, animRel});
	globalFixups.push_back({containerBindingsRel, bindingRel});

	SetArrayHeader(dataBytes, animRel + oBlockOffsets, ptrSize, 1u, blockOffsetsRel, localFixups);
	SetArrayHeader(dataBytes, animRel + oFloatBlockOffsets, ptrSize, 1u, floatBlockOffsetsRel, localFixups);
	SetArrayHeader(dataBytes, animRel + oData, ptrSize, static_cast<uint32_t>(blob.size()), blobRel, localFixups);

	localFixups.push_back({bindingRel + bindNameOff, bindingNameRel});

	SortAndUniqueLocalFixups(localFixups);
	SortAndUniqueGlobalFixups(globalFixups);
	SortAndUniqueVirtualFixups(virtualFixups);

	const uint32_t headerBytes = traits->headerPrefixSize + 3u * traits->sectionHeaderSize;
	const uint32_t classnamesStart = headerBytes;
	const uint32_t typesStart = classnamesStart + static_cast<uint32_t>(classNames.bytes.size());
	const uint32_t classnamesEnd = classnamesStart + static_cast<uint32_t>(classNames.bytes.size());
	const uint32_t dataStart = typesStart;

	AlignBuffer(dataBytes, 16u);
	const uint32_t dataSizeBeforeFixups = static_cast<uint32_t>(dataBytes.size());
	const uint32_t localFixupsRel = dataSizeBeforeFixups;
	for (const LocalFixup& fixup : localFixups) {
		AppendU32(dataBytes, fixup.src);
		AppendU32(dataBytes, fixup.dst);
	}
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AlignBuffer(dataBytes, 16u);

	const uint32_t globalFixupsRel = static_cast<uint32_t>(dataBytes.size());
	for (const GlobalFixup& fixup : globalFixups) {
		AppendU32(dataBytes, fixup.src);
		AppendU32(dataBytes, 2u);
		AppendU32(dataBytes, fixup.dst);
	}
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AlignBuffer(dataBytes, 16u);

	const uint32_t virtualFixupsRel = static_cast<uint32_t>(dataBytes.size());
	for (const VirtualFixup& fixup : virtualFixups) {
		AppendU32(dataBytes, fixup.src);
		AppendU32(dataBytes, 0u);
		AppendU32(dataBytes, fixup.nameOff);
	}
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AppendU32(dataBytes, 0xFFFFFFFFu);
	AlignBuffer(dataBytes, 16u);
	const uint32_t exportsRel = static_cast<uint32_t>(dataBytes.size());

	std::vector<uint8_t> fileBytes(traits->headerPrefix, traits->headerPrefix + traits->headerPrefixSize);
	fileBytes.resize(headerBytes, 0);
	if (traits->format != Format::Fallout64)
		PatchU8(fileBytes, 0x10u, static_cast<uint8_t>(ptrSize));
	WriteSectionHeader(fileBytes, traits->firstSectionOffset + 0u * traits->sectionHeaderSize, traits->sectionHeaderSize, "__classnames__", classnamesStart, classnamesEnd - classnamesStart, classnamesEnd - classnamesStart, classnamesEnd - classnamesStart, classnamesEnd - classnamesStart, classnamesEnd - classnamesStart);
	WriteSectionHeader(fileBytes, traits->firstSectionOffset + 1u * traits->sectionHeaderSize, traits->sectionHeaderSize, "__types__", typesStart, 0u, 0u, 0u, 0u, 0u);
	WriteSectionHeader(fileBytes, traits->firstSectionOffset + 2u * traits->sectionHeaderSize, traits->sectionHeaderSize, "__data__", dataStart, localFixupsRel, globalFixupsRel, virtualFixupsRel, exportsRel, static_cast<uint32_t>(dataBytes.size()));
	fileBytes.insert(fileBytes.end(), classNames.bytes.begin(), classNames.bytes.end());
	fileBytes.insert(fileBytes.end(), dataBytes.begin(), dataBytes.end());

	if (!WriteAllFile(path, fileBytes)) {
		if (errorOut)
			*errorOut = "Cannot write HKX file";
		return false;
	}

	return true;
}

bool File::Load(const std::string& path, std::string* errorOut) {
	skeletons.clear();
	animations.clear();
	format = Format::Unknown;

	std::vector<uint8_t> raw;
	if (!ReadAllFile(path, raw)) {
		if (errorOut)
			*errorOut = "Cannot open file";
		return false;
	}

	Buf buf{raw.data(), raw.size()};
	if (buf.size < 0x40) {
		if (errorOut)
			*errorOut = "File too small";
		return false;
	}

	// Validate magic bytes (0x57E0E057, 0x10C0C010 in LE).
	static const uint8_t MAGIC[8] = {0x57, 0xE0, 0xE0, 0x57, 0x10, 0xC0, 0xC0, 0x10};
	if (std::memcmp(buf.data, MAGIC, 8) != 0) {
		if (errorOut)
			*errorOut = "Not a Havok packfile (bad magic)";
		return false;
	}

	format = DetectFormat(buf);
	if (format == Format::Unknown) {
		if (errorOut)
			*errorOut = "Unsupported HKX variant";
		return false;
	}
	uint32_t ptrSize = (format == Format::Skyrim32) ? 4u : 8u;

	Section sec[3];
	if (!ParseSections(buf, format, sec)) {
		if (errorOut)
			*errorOut = "Failed to parse section headers";
		return false;
	}

	const Section* cnSec = FindSection(sec, "__classnames__");
	const Section* dataSec = FindSection(sec, "__data__");
	if (!cnSec || !dataSec) {
		if (errorOut)
			*errorOut = "Missing required HKX sections";
		return false;
	}

	std::unordered_map<uint32_t, uint32_t> localFixups;
	if (!ParseLocalFixups(buf, *dataSec, localFixups)) {
		if (errorOut)
			*errorOut = "Failed to parse local fixups";
		return false;
	}

	std::vector<VirtualObject> objects;
	if (!ParseVirtualFixups(buf, *dataSec, cnSec->absStart, objects)) {
		if (errorOut)
			*errorOut = "Failed to parse virtual fixups";
		return false;
	}

	// Map each animation object to its (optional) binding.
	struct AnimRec {
		uint32_t rel;
	};
	std::vector<AnimRec> animObjs;
	std::vector<uint32_t> bindingObjs;

	for (const auto& obj : objects) {
		if (obj.className == "hkaSkeleton") {
			Skeleton skel;
			if (ParseSkeleton(buf, dataSec->absStart, obj.relOffset, ptrSize, format, localFixups, skel))
				skeletons.push_back(std::move(skel));
		}
		else if (obj.className == "hkaSplineCompressedAnimation") {
			animObjs.push_back({obj.relOffset});
		}
		else if (obj.className == "hkaAnimationBinding") {
			bindingObjs.push_back(obj.relOffset);
		}
	}

	// Parse animations and pair with bindings (positionally — every binding
	// references its corresponding animation; in vanilla files there is at
	// most one of each).
	for (size_t i = 0; i < animObjs.size(); ++i) {
		Animation anim;
		std::string err;
		if (!ParseAnimation(buf, dataSec->absStart, animObjs[i].rel, ptrSize, localFixups, anim, &err)) {
			if (errorOut && errorOut->empty())
				*errorOut = err;
			continue;
		}
		if (i < bindingObjs.size()) {
			ParseAnimationBinding(buf, dataSec->absStart, bindingObjs[i], ptrSize, localFixups, anim.binding);
		}
		animations.push_back(std::move(anim));
	}

	return !skeletons.empty() || !animations.empty();
}

} // namespace HKX
