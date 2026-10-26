/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "GameImport.h"
#include "../components/DiffData.h"
#include "../components/SliderSet.h"

#include <wx/filename.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

std::string ToUTF8(const wxString& text) {
	return text.ToUTF8().data();
}

json Vec(const Vector3& v) {
	return {v.x, v.y, v.z};
}

struct ForeignSet {
	SliderSet set;
	std::string nifPath;
	std::string shapeDataPath;
};

// A set of a slider set file of any BodySlide folder, with its NIF and the ShapeData folder its data comes from
OpResult LoadForeignSet(const std::string& path, const std::string& setName, ForeignSet& outSet) {
	SliderSetFile setFile(path);
	if (setFile.fail())
		return OpResult::Fail("'" + path + "' couldn't be read as a slider set file.");

	std::vector<std::string> names;
	setFile.GetSetNames(names);
	std::string name = setName;
	if (name.empty()) {
		if (names.size() != 1)
			return OpResult::Fail("The file has " + std::to_string(names.size()) + " sets, pass one as 'set': " + json(names).dump());
		name = names.front();
	}

	if (setFile.GetSet(name, outSet.set))
		return OpResult::Fail("The file has no set '" + name + "'. It has: " + json(names).dump());

	// SliderSets and ShapeData are next to each other in a BodySlide folder
	wxFileName setPath(wxString::FromUTF8(path));
	setPath.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
	wxFileName shapeData(setPath.GetPath(), "");
	shapeData.RemoveLastDir();
	shapeData.AppendDir("ShapeData");
	outSet.shapeDataPath = ToUTF8(shapeData.GetPath());
	outSet.set.SetBaseDataPath(outSet.shapeDataPath);
	outSet.nifPath = outSet.shapeDataPath + "/" + outSet.set.GetDefaultDataFolder() + "/" + outSet.set.GetInputFile();
	return OpResult();
}

} // namespace

namespace GameImport {

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

std::string FindMaterialFile(const std::string& materialPath, const std::vector<std::string>& dataFolders) {
	if (materialPath.empty())
		return {};

	wxString path = wxString::FromUTF8(materialPath);
	path.Replace("\\", "/");
	while (path.StartsWith("/"))
		path.Remove(0, 1);

	for (auto& folder : dataFolders) {
		wxString root = wxString::FromUTF8(folder);
		for (const wxString& candidate : {root + "/" + path, root + "/materials/" + path}) {
			if (wxFileName::FileExists(candidate))
				return ToUTF8(candidate);
		}
	}
	return {};
}

// Fallout 4 scales the gloss map with the smoothness, Skyrim has one specular power for the whole shape: 0.5 gives 22,
// 0.7 gives 59, 1 gives 256.
float GlossinessFromSmoothness(float smoothness) {
	return std::clamp(std::exp2(smoothness * 7.0f + 1.0f), 2.0f, 256.0f);
}

json ApplyMaterial(NifFile& nif, NiShape* shape, MaterialFile& mat) {
	json applied = json::object();
	json notes = json::array();

	auto* shader = dynamic_cast<BSLightingShaderProperty*>(nif.GetShader(shape));
	if (!shader) {
		notes.push_back("The shape has no lighting shader, nothing was set.");
		return {{"notes", notes}};
	}

	auto setTexture = [&](const std::string& path, uint32_t slot) {
		std::string texture = path;
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
	if (mat.glowMap && !mat.glowTexture.empty()) {
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

	const std::string smoothSpec = mat.smoothSpecTexture;
	if (!smoothSpec.empty())
		notes.push_back("Smoothness/specular map '" + smoothSpec
						+ "' has no slot in the Skyrim shader: Skyrim reads specular from the alpha of the normal map, so "
						  "build a normal map with the specular map's red channel as alpha. Glossiness came from the smoothness "
						  "alone, which Fallout 4 multiplies with the gloss map (green channel): lower SpecularPower when "
						  "that map is darker.");
	if (mat.environmentMapping || !mat.envmapTexture.empty())
		notes.push_back("Environment mapping wasn't set up: Skyrim needs its own cube map (slot 4) and an environment mask (slot 5).");
	if (!mat.greyscaleTexture.empty() || mat.grayscaleToPaletteColor)
		notes.push_back("Greyscale-to-palette coloring has no Skyrim equivalent.");
	if (mat.pbr)
		notes.push_back("The material is PBR; Skyrim's lighting shader isn't.");

	return {{"applied", applied}, {"notes", notes}};
}

OpResult Import(OutfitProject& project, const Options& options, Report& outReport) {
	const std::string& file = options.file;
	const bool isSet = wxFileName(wxString::FromUTF8(file)).GetExt().Lower() == "osp";

	std::unique_ptr<ForeignSet> foreign;
	std::string nifPath = file;
	if (isSet) {
		foreign = std::make_unique<ForeignSet>();
		OpResult loaded = LoadForeignSet(file, options.set, *foreign);
		if (!loaded.ok)
			return loaded;
		nifPath = foreign->nifPath;
	}

	NifFile nif;
	if (nif.Load(nifPath) != 0)
		return OpResult::Fail("'" + nifPath + "' couldn't be loaded as a NIF file.");

	const std::string& sliderMode = options.sliders;
	if (sliderMode != "local" && sliderMode != "all" && sliderMode != "zaps" && sliderMode != "none")
		return OpResult::Fail("The slider data to take over has to be \"local\", \"all\", \"zaps\" or \"none\", not '" + sliderMode + "'.");

	std::vector<NiShape*> sourceShapes;
	for (NiShape* shape : nif.GetShapes()) {
		std::string name = shape->name.get();
		if (!options.shapes.empty() && std::find(options.shapes.begin(), options.shapes.end(), name) == options.shapes.end())
			continue;
		if (std::find(options.skipShapes.begin(), options.skipShapes.end(), name) != options.skipShapes.end())
			continue;
		sourceShapes.push_back(shape);
	}
	if (sourceShapes.empty())
		return OpResult::Fail("None of the shapes of '" + nifPath + "' were picked. It has: " + json(nif.GetShapeNames()).dump());

	// Slider data of the set, in the vertex order of its shapes
	DiffDataSets sourceDiffs;
	if (foreign && sliderMode != "none")
		foreign->set.LoadSetDiffData(sourceDiffs);

	if (!project.GetWorkAnim()->GetRefNif())
		project.GetWorkAnim()->SetRefNif(project.GetWorkNif());

	outReport.sourceGame = GameName(nif.GetHeader().GetVersion());
	outReport.sourceFile = nifPath;
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
			return OpResult::Fail("The shape '" + sourceName + "' couldn't be created.");

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
		std::string materialFile = FindMaterialFile(materialPath, options.dataFolders);
		if (!materialFile.empty()) {
			MaterialFile mat(materialFile);
			if (!mat.Failed())
				shapeOut["material"] = {{"file", materialFile}, {"target", ApplyMaterial(work, shape, mat)}};
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
				shapeOut["material"] = {{"path", materialPath}, {"error", "Not found in the data folders, only the shader's texture paths were copied."}};
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
						outReport.slidersAdded.push_back(slider.name);
					}

					project.SetSliderFromDiff(slider.name, shape, turned);
					sliderNames.insert(slider.name);
				}
			}
		}

		outReport.shapes.push_back(shapeOut);
	}

	if (foreign) {
		if (project.mGamePath.empty())
			project.mGamePath = wxString::FromUTF8(foreign->set.GetOutputPath());
		if (project.mGameFile.empty())
			project.mGameFile = wxString::FromUTF8(foreign->set.GetOutputFile());
	}

	outReport.slidersWithData.assign(sliderNames.begin(), sliderNames.end());
	return OpResult();
}

} // namespace GameImport
