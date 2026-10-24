/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/TextureConvert.h"
#include "../utils/ConfigurationManager.h"

#include "../../lib/SOIL2/stb_image.h"
extern "C" {
#include "../../lib/SOIL2/image_DXT.h"
}

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <wx/filename.h>
#include <wx/utils.h>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace TextureConvert;

namespace {

constexpr uint32_t DdsMagic = 0x20534444;
constexpr uint32_t DdsdCaps = 0x1, DdsdHeight = 0x2, DdsdWidth = 0x4, DdsdPitch = 0x8, DdsdPixelFormat = 0x1000, DdsdMipMapCount = 0x20000, DdsdLinearSize = 0x80000;
constexpr uint32_t DdpfAlphaPixels = 0x1, DdpfFourCC = 0x4, DdpfRGB = 0x40;
constexpr uint32_t DdsCapsComplex = 0x8, DdsCapsTexture = 0x1000, DdsCapsMipMap = 0x400000;

uint32_t FourCC(const char* code) {
	return static_cast<uint32_t>(code[0]) | (static_cast<uint32_t>(code[1]) << 8) | (static_cast<uint32_t>(code[2]) << 16) | (static_cast<uint32_t>(code[3]) << 24);
}

enum class Encoding { Uncompressed, DXT1, DXT5 };

// A DDS file with the image and its mipmaps, uncompressed (BGRA) or DXT1/DXT5
void WriteDds(const std::string& path, const std::vector<Image>& levels, Encoding encoding) {
	std::vector<std::vector<uint8_t>> data;
	for (auto& level : levels) {
		std::vector<uint8_t> rgba = ToBytes(level);
		if (encoding == Encoding::Uncompressed) {
			for (size_t i = 0; i < rgba.size(); i += 4)
				std::swap(rgba[i], rgba[i + 2]);
			data.push_back(std::move(rgba));
			continue;
		}

		int size = 0;
		unsigned char* compressed = encoding == Encoding::DXT1 ? convert_image_to_DXT1(rgba.data(), level.width, level.height, 4, &size)
															   : convert_image_to_DXT5(rgba.data(), level.width, level.height, 4, &size);
		if (!compressed)
			throw McpToolError("Compressing '" + path + "' failed.");
		data.emplace_back(compressed, compressed + size);
		free(compressed);
	}

	uint32_t header[32] = {};
	header[0] = DdsMagic;
	header[1] = 124;
	header[2] = DdsdCaps | DdsdHeight | DdsdWidth | DdsdPixelFormat | DdsdMipMapCount | (encoding == Encoding::Uncompressed ? DdsdPitch : DdsdLinearSize);
	header[3] = static_cast<uint32_t>(levels.front().height);
	header[4] = static_cast<uint32_t>(levels.front().width);
	header[5] = encoding == Encoding::Uncompressed ? static_cast<uint32_t>(levels.front().width * 4) : static_cast<uint32_t>(data.front().size());
	header[7] = static_cast<uint32_t>(levels.size());
	// Pixel format at 76 bytes (index 19)
	header[19] = 32;
	if (encoding == Encoding::Uncompressed) {
		header[20] = DdpfRGB | DdpfAlphaPixels;
		header[22] = 32;
		header[23] = 0x00ff0000;
		header[24] = 0x0000ff00;
		header[25] = 0x000000ff;
		header[26] = 0xff000000;
	}
	else {
		header[20] = DdpfFourCC;
		header[21] = FourCC(encoding == Encoding::DXT1 ? "DXT1" : "DXT5");
	}
	header[27] = DdsCapsTexture | (levels.size() > 1 ? DdsCapsComplex | DdsCapsMipMap : 0);

	std::ofstream file(std::filesystem::u8path(path), std::ios::binary);
	if (!file)
		throw McpToolError("'" + path + "' couldn't be written.");
	file.write(reinterpret_cast<const char*>(header), sizeof(header));
	for (auto& level : data)
		file.write(reinterpret_cast<const char*>(level.data()), static_cast<std::streamsize>(level.size()));
	if (!file)
		throw McpToolError("'" + path + "' couldn't be written.");
}

// An image file at no more than maxSize, a power of two; empty without a path
Image LoadImage(const json& args, const char* name, int maxSize) {
	std::string path = McpArgs::String(args, name, "");
	if (path.empty())
		return Image();
	path = McpFilePath(path);

	FILE* file = nullptr;
#ifdef _WIN32
	file = _wfopen(std::filesystem::u8path(path).wstring().c_str(), L"rb");
#else
	file = fopen(path.c_str(), "rb");
#endif
	if (!file)
		throw McpToolError(std::string("The ") + name + " map '" + path + "' couldn't be opened.");

	int width = 0, height = 0, channels = 0;
	const bool wide = stbi_is_16_bit_from_file(file) != 0;
	fseek(file, 0, SEEK_SET);
	void* pixels = wide ? static_cast<void*>(stbi_load_from_file_16(file, &width, &height, &channels, 0)) : static_cast<void*>(stbi_load_from_file(file, &width, &height, &channels, 0));
	fclose(file);
	if (!pixels)
		throw McpToolError(std::string("The ") + name + " map '" + path + "' couldn't be read: " + (stbi_failure_reason() ? stbi_failure_reason() : "unknown format") + ". PNG, TGA, JPG and BMP work.");

	int outWidth = 0, outHeight = 0;
	FitSize(width, height, maxSize, outWidth, outHeight);
	Image image = FromPixels(static_cast<const uint8_t*>(pixels), wide ? 16 : 8, width, height, channels, outWidth, outHeight);
	stbi_image_free(pixels);
	return image;
}

// Where users get texconv when they don't have it
const std::string TexconvDownload = "https://github.com/Microsoft/DirectXTex/releases/latest/download/texconv.exe";

std::string TexconvPath(const json& args) {
	std::string path = McpArgs::String(args, "texconv", Config["Tools/TexconvPath"]);
	if (!path.empty() && !wxFileName::FileExists(wxString::FromUTF8(path)))
		throw McpToolError("texconv isn't at '" + path + "'. Recommend the user to download it from " + TexconvDownload + ".");
	return path;
}

// Writes a map, compressed to BC7 with texconv when there is one, else to DXT with alpha as DXT5
std::string SaveMap(const std::string& folder, const std::string& fileName, const Image& image, bool normalMap, bool alpha, const std::string& format, const std::string& texconv) {
	const std::string path = folder + "/" + fileName;
	std::vector<Image> levels = MipChain(image, normalMap);

	if (format == "bc7") {
		// texconv takes an uncompressed copy with the mipmaps as they are and writes the file of the same name
		wxString tempDir = wxFileName::CreateTempFileName("bsos");
		wxRemoveFile(tempDir);
		if (!wxFileName::Mkdir(tempDir))
			throw McpToolError("A temporary folder couldn't be made.");
		// Native separators: older texconv builds only split paths at backslashes, and otherwise write nowhere
		const std::string tempPath = wxFileName(tempDir, wxString::FromUTF8(fileName)).GetFullPath().ToUTF8().data();
		WriteDds(tempPath, levels, Encoding::Uncompressed);

		wxString command = wxString::Format("\"%s\" -nologo -y -f BC7_UNORM -o \"%s\" \"%s\"",
											wxFileName(wxString::FromUTF8(texconv)).GetFullPath(),
											wxFileName::DirName(wxString::FromUTF8(folder)).GetPath(),
											wxString::FromUTF8(tempPath));
		wxArrayString output, errors;
		long code = wxExecute(command, output, errors, wxEXEC_SYNC | wxEXEC_NODISABLE | wxEXEC_HIDE_CONSOLE);
		wxRemoveFile(wxString::FromUTF8(tempPath));
		wxFileName::Rmdir(tempDir);
		// Old texconv builds report success when they wrote nothing, so the file is checked too
		if (code != 0 || !wxFileName::FileExists(wxString::FromUTF8(path))) {
			std::string message;
			for (auto& line : errors)
				message += std::string(line.ToUTF8().data()) + " ";
			for (auto& line : output)
				message += std::string(line.ToUTF8().data()) + " ";
			throw McpToolError("texconv failed on '" + fileName + "' (code " + std::to_string(code) + "): " + std::string(command.ToUTF8().data()) + " | " + message);
		}
		return path;
	}

	if (format == "uncompressed")
		WriteDds(path, levels, Encoding::Uncompressed);
	else
		WriteDds(path, levels, alpha ? Encoding::DXT5 : Encoding::DXT1);
	return path;
}

McpToolResult ConvertTextures(const json& args) {
	const std::string preset = McpArgs::String(args, "preset", "skyrim");
	if (preset != "skyrim" && preset != "fallout4")
		McpArgs::Wrong("preset", "\"skyrim\" or \"fallout4\"");

	const std::string folder = McpFilePath(McpArgs::String(args, "outputFolder"));
	const std::string name = McpArgs::String(args, "name");
	if (name.empty() || name.find_first_of("/\\") != std::string::npos)
		throw McpToolError("name is the base file name of the maps, without folders, e.g. \"witch_cloth\".");
	const int maxSize = std::clamp(McpArgs::Int(args, "maxSize", 2048), 4, 16384);

	std::string texconv = TexconvPath(args);
	std::string format = McpArgs::String(args, "format", "auto");
	if (format == "auto")
		format = texconv.empty() ? "dxt" : "bc7";
	if (format != "bc7" && format != "dxt" && format != "uncompressed")
		McpArgs::Wrong("format", "\"auto\", \"bc7\", \"dxt\" or \"uncompressed\"");
	if (format == "bc7" && texconv.empty())
		throw McpToolError("BC7 needs texconv: pass its path in 'texconv'. Without it, recommend the user to download it from " + TexconvDownload +
						   ". format \"dxt\" works without it.");

	Image baseColor = LoadImage(args, "baseColor", maxSize);
	Image normal = LoadImage(args, "normal", maxSize);
	Image roughness = LoadImage(args, "roughness", maxSize);
	Image metallic = LoadImage(args, "metallic", maxSize);
	Image occlusion = LoadImage(args, "ao", maxSize);
	Image opacity = LoadImage(args, "opacity", maxSize);
	if (baseColor.Empty() && normal.Empty())
		throw McpToolError("Give at least the baseColor or normal map.");

	// Glossiness maps are inverted roughness
	if (roughness.Empty() && McpArgs::Has(args, "gloss")) {
		roughness = LoadImage(args, "gloss", maxSize);
		for (size_t i = 0; i < roughness.rgba.size(); i += 4)
			roughness.rgba[i] = 1.0f - roughness.rgba[i];
	}

	SkyrimOptions options;
	options.openGLNormal = McpArgs::String(args, "normalConvention", "opengl") != "directx";
	options.specularScale = McpArgs::Float(args, "specularScale", 1.0f);
	options.metalSpecular = std::clamp(McpArgs::Float(args, "metalSpecular", options.metalSpecular), 0.0f, 1.0f);

	std::error_code ec;
	std::filesystem::create_directories(std::filesystem::u8path(folder), ec);
	if (!wxFileName::DirExists(wxString::FromUTF8(folder)))
		throw McpToolError("The folder '" + folder + "' couldn't be made.");

	const bool overwrite = McpArgs::Bool(args, "overwrite", false);
	auto checkFile = [&](const std::string& fileName) {
		std::string path = folder + "/" + fileName;
		if (!overwrite && wxFileName::FileExists(wxString::FromUTF8(path)))
			throw McpToolError("'" + path + "' exists. Pass overwrite: true to replace it.");
	};

	json files = json::object();
	json notes = json::array();
	json suggested = json::object();
	const float gloss = MeanGloss(roughness);

	if (!baseColor.Empty()) {
		checkFile(name + "_d.dds");
		Image diffuse = Diffuse(baseColor, opacity, occlusion);
		files["diffuse"] = SaveMap(folder, name + "_d.dds", diffuse, false, !opacity.Empty(), format, texconv);
		if (!opacity.Empty())
			notes.push_back("The diffuse alpha is the opacity: give the shapes alpha testing or blending (set_geometry_properties).");
	}

	if (preset == "skyrim") {
		if (!normal.Empty()) {
			checkFile(name + "_n.dds");
			Image n = SkyrimNormal(normal, roughness, metallic, options);
			files["normal"] = SaveMap(folder, name + "_n.dds", n, true, true, format, texconv);
		}
		const bool envMask = McpArgs::Bool(args, "environmentMask", !metallic.Empty());
		if (envMask && !metallic.Empty()) {
			checkFile(name + "_m.dds");
			const Image& sizeOf = !normal.Empty() ? normal : baseColor;
			Image mask = SkyrimEnvironmentMask(metallic, roughness, sizeOf.width, sizeOf.height);
			files["environmentMask"] = SaveMap(folder, name + "_m.dds", mask, false, false, format, texconv);
			// Environment map shader type
			suggested["ShaderType"] = "BSLighting:1";
			suggested["EnvMapScale"] = 1.0f;
			notes.push_back("Environment mapping needs a cube map in slot 4, e.g. the vanilla textures\\cubemaps\\shinybright_e.dds or chrome_e.dds, and the mask in slot 5.");
		}
		suggested["SpecularPower"] = std::round(SpecularPowerFromGloss(gloss));
		suggested["SpecularStrength"] = 1.0f;
		suggested["SpecularColor"] = {1.0f, 1.0f, 1.0f};
		suggested["textureSlots"] = json::object();
		if (files.contains("diffuse"))
			suggested["textureSlots"]["0"] = name + "_d.dds";
		if (files.contains("normal"))
			suggested["textureSlots"]["1"] = name + "_n.dds";
		if (files.contains("environmentMask"))
			suggested["textureSlots"]["5"] = name + "_m.dds";
	}
	else {
		if (!normal.Empty()) {
			checkFile(name + "_n.dds");
			SkyrimOptions noSpecular = options;
			Image n = SkyrimNormal(normal, Image(), Image(), noSpecular);
			for (size_t i = 3; i < n.rgba.size(); i += 4)
				n.rgba[i] = 1.0f;
			files["normal"] = SaveMap(folder, name + "_n.dds", n, true, false, format, texconv);
		}
		const Image& sizeOf = !normal.Empty() ? normal : baseColor;
		checkFile(name + "_s.dds");
		Image s = FalloutSmoothSpec(roughness, metallic, sizeOf.width, sizeOf.height, options);
		files["smoothSpec"] = SaveMap(folder, name + "_s.dds", s, false, false, format, texconv);
		suggested["textures"] = {{"diffuse", name + "_d.dds"}, {"normal", name + "_n.dds"}, {"smoothSpec", name + "_s.dds"}};
		notes.push_back("Write the BGSM with write_material and these textures, relative to the Textures folder.");
	}

	if (format == "dxt")
		notes.push_back("Saved as DXT1/DXT5. Skyrim SE and Fallout 4 also take BC7, which looks better: pass texconv's path to get it. "
						"Without texconv, recommend the user to download it from " + TexconvDownload + ".");

	McpToolResult result;
	result.data["files"] = files;
	result.data["meanGloss"] = std::round(gloss * 100.0f) / 100.0f;
	result.data["size"] = {(!normal.Empty() ? normal : baseColor).width, (!normal.Empty() ? normal : baseColor).height};
	result.data["format"] = format;
	result.data["suggestedShader"] = suggested;
	result.data["notes"] = notes;
	return result;
}

} // namespace

void AddTextureTools(OutfitStudioMcp& mcp) {
	McpTool convert;
	convert.name = "convert_textures";
	convert.description =
		"Makes a game's textures from the maps other programs export (PBR base color, normal, roughness or gloss, metallic, "
		"ambient occlusion, opacity), for a model brought in with import_file. skyrim: _d (base color times occlusion, opacity in "
		"alpha), _n (normal turned to the game's green-down style when it's OpenGL style, specular mask in alpha from gloss and "
		"metallic) and _m (environment mask from metallic and gloss). fallout4: _d, _n and _s (specular red, smoothness green). "
		"Maps are scaled down to maxSize as a power of two, with mipmaps. BC7 with texconv (texconv argument, or Tools/TexconvPath "
		"in the config), else DXT1/DXT5; users without texconv.exe can download it from "
		"https://github.com/Microsoft/DirectXTex/releases/latest/download/texconv.exe.The result suggests shader values (SpecularPower from the mean gloss) and texture slots "
		"for set_shader_properties and set_texture_paths; texture paths in the game are relative to Data, e.g. "
		"textures\\armor\\mine\\name_d.dds when outputFolder is that folder of the mod.";
	convert.inputSchema = McpSchema(
		{
			{"preset", {{"type", "string"}, {"enum", {"skyrim", "fallout4"}}, {"description", "Default skyrim"}}},
			{"outputFolder", McpProp("string", "Folder to write the maps to (absolute or relative to the BodySlide folder)")},
			{"name", McpProp("string", "Base file name, e.g. \"witch_cloth\" for witch_cloth_d.dds")},
			{"baseColor", McpProp("string", "Base color (albedo) map file")},
			{"normal", McpProp("string", "Tangent space normal map file")},
			{"normalConvention", {{"type", "string"}, {"enum", {"opengl", "directx"}}, {"description", "Which way the normal map's green points: opengl (up, Blender and most PBR exports) or directx. Default opengl."}}},
			{"roughness", McpProp("string", "Roughness map file")},
			{"gloss", McpProp("string", "Glossiness map file, instead of roughness")},
			{"metallic", McpProp("string", "Metallic map file")},
			{"ao", McpProp("string", "Ambient occlusion map file, multiplied into the diffuse")},
			{"opacity", McpProp("string", "Opacity map file, the diffuse alpha")},
			{"environmentMask", McpProp("boolean", "skyrim: write the _m map. Default true with a metallic map.")},
			{"specularScale", McpProp("number", "Scales the specular mask. Default 1.")},
			{"metalSpecular", McpProp("number", "Share of the specular only metal gets, 0 to 1. Default 0.6.")},
			{"maxSize", McpProp("integer", "Largest width or height. Default 2048.")},
			{"format", {{"type", "string"}, {"enum", {"auto", "bc7", "dxt", "uncompressed"}}, {"description", "Default auto: bc7 with texconv, else dxt"}}},
			{"texconv", McpProp("string", "Path of texconv.exe for BC7")},
			{"overwrite", McpProp("boolean", "Replace existing files. Default false.")},
		},
		{"outputFolder", "name"});
	convert.handler = [](const json& args) { return ConvertTextures(args); };
	mcp.AddTool(std::move(convert));
}
