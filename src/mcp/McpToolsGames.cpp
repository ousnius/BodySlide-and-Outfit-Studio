/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/DiffData.h"
#include "../components/SliderSet.h"
#include "../files/MaterialFile.h"
#include "../program/OutfitStudio.h"
#include "../utils/ConfigurationManager.h"

#include <wx/filename.h>

#include <algorithm>
#include <cmath>
#include <set>

extern ConfigurationManager Config;

using nlohmann::json;
using namespace nifly;

namespace {

std::string ToUTF8(const wxString& text) {
	return text.ToUTF8().data();
}

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

std::string GameName(const NiVersion& version) {
	if (version.IsSF())
		return "Starfield";
	if (version.IsFO76())
		return "Fallout 76";
	if (version.IsFO4())
		return "Fallout 4";
	if (version.IsSSE())
		return "Skyrim Special Edition";
	if (version.IsSK())
		return "Skyrim";
	if (version.IsFO3())
		return "Fallout 3";
	if (version.IsOB())
		return "Oblivion";
	return version.GetVersionInfo();
}

// A material path of a shader, found in one of the data folders, with or without the leading "materials"
std::string FindMaterialFile(const std::string& materialPath, const std::vector<std::string>& dataFolders) {
	if (materialPath.empty())
		return {};

	wxString path = wxString::FromUTF8(materialPath);
	path.Replace("\\", "/");
	while (path.StartsWith("/"))
		path.Remove(0, 1);

	for (auto& folder : dataFolders) {
		wxString root = wxString::FromUTF8(McpFilePath(folder));
		for (const wxString& candidate : {root + "/" + path, root + "/materials/" + path}) {
			if (wxFileName::FileExists(candidate))
				return ToUTF8(candidate);
		}
	}
	return {};
}

// Material strings keep the terminating null of the file
std::string MaterialString(const std::string& text) {
	return text.substr(0, text.find('\0'));
}

json MaterialJson(MaterialFile& mat) {
	json data = {
		{"type", mat.signature == MaterialFile::BGEM ? "BGEM" : "BGSM"},
		{"version", mat.version},
		{"tileU", mat.tileU},
		{"tileV", mat.tileV},
		{"uvOffset", {mat.uvOffset.u, mat.uvOffset.v}},
		{"uvScale", {mat.uvScale.u, mat.uvScale.v}},
		{"alpha", mat.alpha},
		{"alphaBlendMode", static_cast<int>(mat.alphaBlendMode)},
		{"alphaTest", mat.alphaTest},
		{"alphaTestRef", static_cast<int>(static_cast<uint8_t>(mat.alphaTestRef))},
		{"zBufferWrite", mat.zBufferWrite},
		{"zBufferTest", mat.zBufferTest},
		{"decal", mat.decal},
		{"twoSided", mat.twoSided},
		{"environmentMapping", mat.environmentMapping},
		{"environmentMappingMaskScale", mat.environmentMappingMaskScale},
		{"grayscaleToPaletteColor", mat.grayscaleToPaletteColor},
	};

	if (mat.signature == MaterialFile::BGEM) {
		data["textures"] = {
			{"base", MaterialString(mat.baseTexture)},
			{"grayscale", MaterialString(mat.grayscaleTexture)},
			{"envmap", MaterialString(mat.fxEnvmapTexture)},
			{"normal", MaterialString(mat.fxNormalTexture)},
			{"envmapMask", MaterialString(mat.envmapMaskTexture)},
		};
		data["baseColor"] = Vec(mat.baseColor);
		data["baseColorScale"] = mat.baseColorScale;
		data["falloffEnabled"] = mat.falloffEnabled;
		data["lightingInfluence"] = mat.lightingInfluence;
		return data;
	}

	data["textures"] = {
		{"diffuse", MaterialString(mat.diffuseTexture)},
		{"normal", MaterialString(mat.normalTexture)},
		{"smoothSpec", MaterialString(mat.smoothSpecTexture)},
		{"greyscale", MaterialString(mat.greyscaleTexture)},
		{"glow", MaterialString(mat.glowTexture)},
		{"wrinkles", MaterialString(mat.wrinklesTexture)},
		{"specular", MaterialString(mat.specularTexture)},
		{"lighting", MaterialString(mat.lightingTexture)},
		{"flow", MaterialString(mat.flowTexture)},
		{"envmap", MaterialString(mat.envmapTexture)},
		{"innerLayer", MaterialString(mat.innerLayerTexture)},
		{"displacement", MaterialString(mat.displacementTexture)},
	};
	data["specularEnabled"] = mat.specularEnabled;
	data["specularColor"] = Vec(mat.specularColor);
	data["specularMult"] = mat.specularMult;
	data["smoothness"] = mat.smoothness;
	data["fresnelPower"] = mat.fresnelPower;
	data["rimLighting"] = mat.rimLighting;
	data["rimPower"] = mat.rimPower;
	data["backLighting"] = mat.backLighting;
	data["backLightPower"] = mat.backLightPower;
	data["subsurfaceLighting"] = mat.subsurfaceLighting;
	data["subsurfaceLightingRolloff"] = mat.subsurfaceLightingRolloff;
	data["emitEnabled"] = mat.emitEnabled;
	data["emittanceColor"] = Vec(mat.emittanceColor);
	data["emittanceMult"] = mat.emittanceMult;
	data["glowMap"] = mat.glowMap;
	data["modelSpaceNormals"] = mat.modelSpaceNormals;
	data["receiveShadows"] = mat.receiveShadows;
	data["castShadows"] = mat.castShadows;
	data["skinTint"] = mat.skinTint;
	data["hair"] = mat.hair;
	data["pbr"] = mat.pbr;
	return data;
}

// Specular power (Skyrim glossiness) for a Fallout 4 smoothness. Fallout 4 scales the gloss map with the smoothness,
// Skyrim has one specular power for the whole shape: 0.5 gives 22, 0.7 gives 59, 1 gives 256.
float GlossinessFromSmoothness(float smoothness) {
	return std::clamp(std::exp2(smoothness * 7.0f + 1.0f), 2.0f, 256.0f);
}

// Sets up a Skyrim lighting shader from a Fallout 4 material. Returns what was taken over and what has no place in
// the target shader.
json ApplyMaterial(NifFile& nif, NiShape* shape, MaterialFile& mat) {
	json applied = json::object();
	json notes = json::array();

	auto* shader = dynamic_cast<BSLightingShaderProperty*>(nif.GetShader(shape));
	if (!shader) {
		notes.push_back("The shape has no lighting shader, nothing was set.");
		return {{"notes", notes}};
	}

	auto setTexture = [&](const std::string& path, uint32_t slot) {
		std::string texture = MaterialString(path);
		if (texture.empty())
			return;
		// Material texture paths are relative to the textures folder
		wxString lower = wxString::FromUTF8(texture).Lower();
		if (!lower.StartsWith("textures\\") && !lower.StartsWith("textures/"))
			texture = "textures\\" + texture;
		nif.SetTextureSlot(shape, texture, slot);
		applied["textures"][std::to_string(slot)] = texture;
	};

	if (mat.signature == MaterialFile::BGEM) {
		setTexture(mat.baseTexture, 0);
		setTexture(mat.fxNormalTexture, 1);
		notes.push_back("BGEM effect materials need a BSEffectShaderProperty in the target game; only the textures were set on the lighting shader.");
		return {{"applied", applied}, {"notes", notes}};
	}

	setTexture(mat.diffuseTexture, 0);
	setTexture(mat.normalTexture, 1);

	uint32_t type = BSLSP_DEFAULT;
	if (mat.glowMap && !MaterialString(mat.glowTexture).empty()) {
		setTexture(mat.glowTexture, 2);
		type = BSLSP_GLOWMAP;
	}
	else if (mat.skinTint)
		type = BSLSP_SKINTINT;
	else if (mat.hair)
		type = BSLSP_HAIRTINT;
	shader->SetShaderType(type);
	applied["shaderType"] = type;

	uint32_t& f1 = shader->shaderFlags1;
	uint32_t& f2 = shader->shaderFlags2;
	auto setFlag = [](uint32_t& flags, uint32_t flag, bool on) {
		flags = on ? (flags | flag) : (flags & ~flag);
	};

	setFlag(f1, SLSF1_SKINNED, shape->IsSkinned());
	setFlag(f1, SLSF1_SPECULAR, mat.specularEnabled);
	setFlag(f1, SLSF1_MODEL_SPACE_NORMALS, mat.modelSpaceNormals);
	// Fallout 4 materials leave shadows to the renderer, Skyrim armor casts and receives them
	setFlag(f1, SLSF1_RECEIVE_SHADOWS, true);
	setFlag(f1, SLSF1_CAST_SHADOWS, true);
	setFlag(f1, SLSF1_DECAL, mat.decal);
	setFlag(f1, SLSF1_ZBUFFER_TEST, mat.zBufferTest);
	setFlag(f1, SLSF1_OWN_EMIT, mat.emitEnabled || mat.glowMap);
	setFlag(f1, SLSF1_ENVIRONMENT_MAPPING, false);
	setFlag(f2, SLSF2_ZBUFFER_WRITE, mat.zBufferWrite);
	setFlag(f2, SLSF2_DOUBLE_SIDED, mat.twoSided);
	setFlag(f2, SLSF2_GLOW_MAP, type == BSLSP_GLOWMAP);
	setFlag(f2, SLSF2_RIM_LIGHTING, mat.rimLighting);
	setFlag(f2, SLSF2_BACK_LIGHTING, mat.backLighting);
	setFlag(f2, SLSF2_SOFT_LIGHTING, mat.subsurfaceLighting);
	setFlag(f2, SLSF2_VERTEX_COLORS, shape->HasVertexColors());

	shader->specularColor = mat.specularColor;
	shader->specularStrength = mat.specularMult;
	shader->glossiness = GlossinessFromSmoothness(mat.smoothness);
	shader->alpha = mat.alpha;
	shader->uvOffset = mat.uvOffset;
	shader->uvScale = mat.uvScale;
	shader->textureClampMode = static_cast<TexClampMode>((mat.tileU ? 2 : 0) | (mat.tileV ? 1 : 0));
	if (mat.emitEnabled) {
		shader->emissiveColor = mat.emittanceColor;
		shader->emissiveMultiple = mat.emittanceMult;
	}
	else {
		shader->emissiveColor = Vector3();
		shader->emissiveMultiple = 1.0f;
	}
	if (mat.rimLighting)
		shader->rimlightPower = mat.rimPower;
	if (mat.backLighting)
		shader->backlightPower = mat.backLightPower;
	if (mat.subsurfaceLighting)
		shader->softlighting = mat.subsurfaceLightingRolloff;

	applied["specular"] = mat.specularEnabled;
	applied["specularColor"] = Vec(shader->specularColor);
	applied["specularStrength"] = shader->specularStrength;
	applied["glossiness"] = shader->glossiness;
	applied["doubleSided"] = mat.twoSided;
	applied["alpha"] = shader->alpha;

	// Alpha blending and testing go into an alpha property, which Fallout 4 keeps in the material
	const bool blend = mat.alphaBlendMode != MaterialFile::AlphaBlendModeType::None && mat.alphaBlendMode != MaterialFile::AlphaBlendModeType::Unknown;
	if (blend || mat.alphaTest) {
		uint8_t mode0 = 0;
		uint32_t src = 6, dst = 7;
		if (blend)
			mat.ConvertAlphaBlendMode(mat.alphaBlendMode, mode0, src, dst);

		auto alphaProp = std::make_unique<NiAlphaProperty>();
		alphaProp->flags = static_cast<uint16_t>((blend ? 1 : 0) | ((src & 0xF) << 1) | ((dst & 0xF) << 5) | (mat.alphaTest ? (1 << 9) : 0) | (4 << 10));
		alphaProp->threshold = static_cast<uint8_t>(mat.alphaTestRef);
		applied["alphaProperty"] = {{"flags", alphaProp->flags}, {"threshold", alphaProp->threshold}};
		nif.AssignAlphaProperty(shape, std::move(alphaProp));
	}
	else
		nif.RemoveAlphaProperty(shape);

	const std::string smoothSpec = MaterialString(mat.smoothSpecTexture);
	if (!smoothSpec.empty())
		notes.push_back("Smoothness/specular map '" + smoothSpec
						+ "' has no slot in the Skyrim shader: Skyrim reads specular from the alpha of the normal map, so "
						  "build a normal map with the specular map's red channel as alpha. Glossiness came from the smoothness "
						  "alone, which Fallout 4 multiplies with the gloss map (green channel): lower SpecularPower when "
						  "that map is darker.");
	if (mat.environmentMapping || !MaterialString(mat.envmapTexture).empty())
		notes.push_back("Environment mapping wasn't set up: Skyrim needs its own cube map (slot 4) and an environment mask (slot 5).");
	if (!MaterialString(mat.greyscaleTexture).empty() || mat.grayscaleToPaletteColor)
		notes.push_back("Greyscale-to-palette coloring has no Skyrim equivalent.");
	if (mat.pbr)
		notes.push_back("The material is PBR; Skyrim's lighting shader isn't.");

	return {{"applied", applied}, {"notes", notes}};
}

struct ForeignSet {
	SliderSet set;
	std::string nifPath;
	std::string shapeDataPath;
};

// A set of a slider set file of any BodySlide folder, with its NIF and the ShapeData folder its data comes from
ForeignSet LoadForeignSet(const std::string& file, const std::string& setName) {
	std::string path = McpFilePath(file);
	SliderSetFile setFile(path);
	if (setFile.fail())
		throw McpToolError("'" + path + "' couldn't be read as a slider set file.");

	std::vector<std::string> names;
	setFile.GetSetNames(names);
	std::string name = setName;
	if (name.empty()) {
		if (names.size() != 1)
			throw McpToolError("The file has " + std::to_string(names.size()) + " sets, pass one as 'set': " + json(names).dump());
		name = names.front();
	}

	ForeignSet result;
	if (setFile.GetSet(name, result.set))
		throw McpToolError("The file has no set '" + name + "'. It has: " + json(names).dump());

	// SliderSets and ShapeData are next to each other in a BodySlide folder
	wxFileName setPath(wxString::FromUTF8(path));
	setPath.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	wxFileName shapeData(setPath.GetPath(), "");
	shapeData.RemoveLastDir();
	shapeData.AppendDir("ShapeData");
	result.shapeDataPath = ToUTF8(shapeData.GetPath());
	result.set.SetBaseDataPath(result.shapeDataPath);
	result.nifPath = result.shapeDataPath + "/" + result.set.GetDefaultDataFolder() + "/" + result.set.GetInputFile();
	return result;
}

McpToolResult ImportFromGame(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();

	std::string file = McpFilePath(McpArgs::String(args, "file"));
	const bool isSet = wxFileName(wxString::FromUTF8(file)).GetExt().Lower() == "osp";

	std::unique_ptr<ForeignSet> foreign;
	std::string nifPath = file;
	if (isSet) {
		foreign = std::make_unique<ForeignSet>(LoadForeignSet(file, McpArgs::String(args, "set", "")));
		nifPath = foreign->nifPath;
	}

	NifFile nif;
	if (nif.Load(nifPath) != 0)
		throw McpToolError("'" + nifPath + "' couldn't be loaded as a NIF file.");

	std::vector<std::string> wanted = McpArgs::Strings(args, "shapes");
	const std::vector<std::string> skipped = McpArgs::Strings(args, "skipShapes");
	const std::string sliderMode = McpArgs::String(args, "sliders", "local");
	if (sliderMode != "local" && sliderMode != "all" && sliderMode != "zaps" && sliderMode != "none")
		McpArgs::Wrong("sliders", "\"local\", \"all\", \"zaps\" or \"none\"");

	std::vector<std::string> dataFolders = McpArgs::Strings(args, "dataFolders");

	std::vector<NiShape*> sourceShapes;
	for (NiShape* shape : nif.GetShapes()) {
		std::string name = shape->name.get();
		if (!wanted.empty() && std::find(wanted.begin(), wanted.end(), name) == wanted.end())
			continue;
		if (std::find(skipped.begin(), skipped.end(), name) != skipped.end())
			continue;
		sourceShapes.push_back(shape);
	}
	if (sourceShapes.empty())
		throw McpToolError("None of the shapes of '" + nifPath + "' were picked. It has: " + json(nif.GetShapeNames()).dump());

	// Slider data of the set, in the vertex order of its shapes
	DiffDataSets sourceDiffs;
	if (foreign && sliderMode != "none")
		foreign->set.LoadSetDiffData(sourceDiffs);

	if (!project.GetWorkAnim()->GetRefNif())
		project.GetWorkAnim()->SetRefNif(project.GetWorkNif());

	json shapesOut = json::array();
	json slidersAdded = json::array();
	std::set<std::string> sliderNames;

	for (NiShape* source : sourceShapes) {
		const std::string sourceName = source->name.get();
		std::string newName = sourceName;
		for (int n = 2; project.IsValidShape(newName); n++)
			newName = sourceName + "_" + std::to_string(n);

		std::vector<Vector3> verts;
		std::vector<Vector3> normals;
		std::vector<Vector2> uvs;
		std::vector<Color4> colors;
		std::vector<Triangle> tris;
		nif.GetVertsForShape(source, verts);
		nif.GetUvsForShape(source, uvs);
		nif.GetColorsForShape(source, colors);
		if (auto* n = nif.GetNormalsForShape(source))
			normals = *n;
		source->GetTriangles(tris);

		// Positions in global space: skinned shapes are stored in their skin space, others under their nodes
		MatTransform toGlobal;
		std::string space = "global";
		MatTransform globalToSkin;
		if (source->IsSkinned() && nif.CalcShapeTransformGlobalToSkin(source, globalToSkin)) {
			toGlobal = globalToSkin.InverseTransform();
			space = "skin";
		}
		else if (!source->IsSkinned()) {
			toGlobal = source->GetTransformToParent();
			for (NiNode* parent = nif.GetParentNode(source); parent; parent = nif.GetParentNode(parent))
				toGlobal = parent->GetTransformToParent().ComposeTransforms(toGlobal);
			space = "node";
		}

		for (auto& v : verts)
			v = toGlobal.ApplyTransform(v);
		for (auto& n : normals)
			n = toGlobal.ApplyTransformToDir(n);

		NiShape* shape = project.CreateNifShapeFromData(newName, &verts, &tris, uvs.size() == verts.size() ? &uvs : nullptr, normals.size() == verts.size() ? &normals : nullptr);
		if (!shape)
			throw McpToolError("The shape '" + sourceName + "' couldn't be created.");

		NifFile& work = *project.GetWorkNif();
		if (colors.size() == verts.size()) {
			shape->SetVertexColors(true);
			work.SetColorsForShape(shape, colors);
		}
		work.CalcTangentsForShape(shape);

		std::vector<std::string> sourceBones;
		nif.GetShapeBoneList(source, sourceBones);
		if (source->IsSkinned())
			project.CreateSkinning(shape);

		json shapeOut = {
			{"name", newName},
			{"source", sourceName},
			{"vertices", shape->GetNumVertices()},
			{"triangles", shape->GetNumTriangles()},
			{"sourceSpace", space},
			{"translationToGlobal", Vec(toGlobal.translation)},
			{"vertexColors", colors.size() == verts.size()},
			{"sourceBones", sourceBones.size()},
		};
		if (shape->GetNumVertices() < verts.size() || shape->GetNumTriangles() < tris.size())
			shapeOut["warning"] = "The target game's vertex or triangle limit was exceeded, the rest was dropped. Slider data was not imported.";

		// Shader: the source material file if there is one, else the source shader's textures
		NiShader* sourceShader = nif.GetShader(source);
		const std::string materialPath = sourceShader ? sourceShader->name.get() : std::string();
		std::string materialFile = FindMaterialFile(materialPath, dataFolders);
		if (!materialFile.empty()) {
			MaterialFile mat(materialFile);
			if (!mat.Failed()) {
				json mapped = ApplyMaterial(work, shape, mat);
				shapeOut["material"] = {{"file", materialFile}, {"source", MaterialJson(mat)}, {"target", mapped}};
			}
			else
				shapeOut["material"] = {{"file", materialFile}, {"error", "The material file couldn't be read."}};
		}
		else {
			json textures = json::object();
			for (uint32_t slot = 0; slot < 10; slot++) {
				std::string texture;
				if (nif.GetTextureSlot(source, texture, slot) && !texture.empty()) {
					work.SetTextureSlot(shape, texture, slot);
					textures[std::to_string(slot)] = texture;
				}
			}
			shapeOut["textures"] = textures;
			if (!materialPath.empty())
				shapeOut["material"] = {{"path", materialPath}, {"error", "Not found in dataFolders, only the shader's texture paths were copied."}};
		}
		project.SetTextures(shape);

		// Slider data, turned like the positions
		if (foreign && sliderMode != "none" && !shapeOut.contains("warning")) {
			SliderSet& set = foreign->set;
			for (size_t i = 0; i < set.size(); i++) {
				SliderData& slider = set[i];
				if (sliderMode == "zaps" && !slider.bZap)
					continue;

				for (auto& df : slider.dataFiles) {
					if (set.TargetToShape(df.targetName) != sourceName)
						continue;
					if (sliderMode != "all" && !df.bLocal && !slider.bZap)
						continue;

					TargetDataDiffs* diffs = sourceDiffs.GetDiffSet(df.dataName);
					if (!diffs)
						continue;

					TargetDataDiffs turned;
					for (auto& d : *diffs)
						if (d.first < shape->GetNumVertices())
							turned[d.first] = toGlobal.ApplyTransformToDiff(d.second);

					if (!project.ValidSlider(slider.name)) {
						project.AddEmptySlider(slider.name);
						size_t index = 0;
						project.SliderIndexFromName(slider.name, index);
						project.SetSliderZap(index, slider.bZap);
						project.SetSliderHidden(index, slider.bHidden);
						project.SetSliderInvert(index, slider.bInvert);
						project.SetSliderUV(index, slider.bUV);
						project.SetSliderDefault(index, static_cast<int>(slider.defSmallValue), false);
						project.SetSliderDefault(index, static_cast<int>(slider.defBigValue), true);
						if (!slider.zapToggles.empty()) {
							wxArrayString toggles;
							for (auto& t : slider.zapToggles)
								toggles.Add(wxString::FromUTF8(t));
							project.SetSliderZapToggles(index, toggles);
						}
						project.activeSet[slider.name].bClamp = slider.bClamp;
						slidersAdded.push_back(slider.name);
					}

					project.SetSliderFromDiff(slider.name, shape, turned);
					sliderNames.insert(slider.name);
				}
			}
		}

		shapesOut.push_back(shapeOut);
	}

	if (foreign) {
		if (project.mGamePath.empty())
			project.mGamePath = wxString::FromUTF8(foreign->set.GetOutputPath());
		if (project.mGameFile.empty())
			project.mGameFile = wxString::FromUTF8(foreign->set.GetOutputFile());
	}

	frame->RefreshGUIFromProj();
	frame->CreateSetSliders();
	frame->ApplySliders();
	frame->SetPendingChanges();

	McpToolResult result;
	result.data["sourceGame"] = GameName(nif.GetHeader().GetVersion());
	result.data["sourceFile"] = nifPath;
	result.data["shapes"] = shapesOut;
	result.data["slidersWithData"] = std::vector<std::string>(sliderNames.begin(), sliderNames.end());
	result.data["slidersAdded"] = slidersAdded;
	result.data["next"] =
		"Bone weights of another game's skeleton aren't kept: the shapes are skinned without weights. Load the target "
		"body (or a conversion reference from the source body), conform, then copy_bone_weights. Set partitions or "
		"segments for the target game, and check the shader.";
	return result;
}

} // namespace

void AddGameTools(OutfitStudioMcp& mcp) {
	McpTool importGame;
	importGame.name = "import_from_game";
	importGame.description =
		"Imports shapes of another game's NIF, or of a set of another game's BodySlide folder (.osp, with its "
		"ShapeData), as new shapes of the open project's game, e.g. Fallout 4 outfits into a Skyrim SE project. "
		"Geometry, UVs, normals and vertex colors are taken over in global space (skin space and node transforms are "
		"applied). With a set, the outfit's own slider data (morphs and zaps) comes along with the slider flags. "
		"Bone weights aren't kept, since the skeletons differ. The shader is set up from the source's material "
		"file (BGSM) when it's found in dataFolders: textures, specular, glossiness from smoothness, two-sided, "
		"alpha, glow, rim and back lighting; the result lists the source material and notes on what doesn't map.";
	importGame.inputSchema = McpSchema(
		{
			{"file", McpProp("string", "NIF file, or slider set file (.osp) of another BodySlide folder (absolute or relative to this BodySlide folder)")},
			{"set", McpProp("string", "Set of the slider set file, needed when it has more than one")},
			{"shapes", McpStringList("Only these shapes. Default: all.")},
			{"skipShapes", McpStringList("Leave these shapes out, e.g. the source game's body")},
			{"sliders",
			 {{"type", "string"},
			  {"enum", {"local", "all", "zaps", "none"}},
			  {"description",
			   "Slider data to take over from a set: local (the outfit's own morphs and all zaps, default), all (also data "
			   "from the body's shared files), zaps or none"}}},
			{"dataFolders", McpStringList("Data folders of the source game or mod to find material files in, e.g. \"D:/Mods/MyOutfit\" that has Materials/...")},
		},
		{"file"});
	importGame.handler = [&mcp](const json& args) { return ImportFromGame(mcp, args); };
	mcp.AddTool(std::move(importGame));

	McpTool readMaterial;
	readMaterial.name = "read_material";
	readMaterial.description = "Reads a Fallout 4 or 76 material file (BGSM or BGEM) and returns its properties and textures.";
	readMaterial.inputSchema = McpSchema(
		{
			{"path", McpProp("string", "Material file, or a material path as a shader names it (e.g. Materials\\\\Outfit\\\\a.bgsm) with dataFolders")},
			{"dataFolders", McpStringList("Data folders to find a material path in")},
		},
		{"path"});
	readMaterial.readOnly = true;
	readMaterial.handler = [](const json& args) {
		std::string path = McpArgs::String(args, "path");
		std::string file = McpFilePath(path);
		if (!wxFileName::FileExists(wxString::FromUTF8(file)))
			file = FindMaterialFile(path, McpArgs::Strings(args, "dataFolders"));
		if (file.empty())
			throw McpToolError("There is no material file '" + path + "'.");

		MaterialFile mat(file);
		if (mat.Failed())
			throw McpToolError("'" + file + "' couldn't be read as a BGSM or BGEM file.");

		McpToolResult result;
		result.data = MaterialJson(mat);
		result.data["file"] = file;
		return result;
	};
	mcp.AddTool(std::move(readMaterial));
}
