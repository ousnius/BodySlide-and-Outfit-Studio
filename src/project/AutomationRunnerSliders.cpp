/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: sliders, slider data and conforming

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "outfitstudio/OutfitStudioFrame.h"
#include "outfitstudio/GLPanel.h"
#include "components/DiffData.h"
#include "components/UndoState.h"
#include "files/TriFile.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <NifFile.hpp>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

using namespace nifly;

int AutomationRunner::ExecuteStepSetSliderValues(const AutomationStep& step) {
	int intVal = static_cast<int>(step.setSliderValue * 100);
	if (step.setSliderNames.empty()) {
		wxLogMessage("Automation: Setting all slider values to %d%%...", intVal);
		for (size_t i = 0; i < project->SliderCount(); i++)
			outfitStudio->SetSliderValue(i, intVal);
	}
	else {
		for (const auto& name : step.setSliderNames) {
			if (!project->ValidSlider(name)) {
				wxLogWarning("Automation: SetSliderValues - slider '%s' not found, skipping.", name);
				continue;
			}
			wxLogMessage("Automation: Setting slider '%s' to %d%%...", name, intVal);
			outfitStudio->SetSliderValue(name, intVal);
		}
	}
	outfitStudio->ApplySliders();
	return 0;
}

int AutomationRunner::ExecuteStepConformSliders(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step, false);
	if (shapes.empty()) {
		wxLogWarning("Automation: ConformSliders - no target shapes found.");
		return 0;
	}

	if (!project->GetBaseShape()) {
		wxLogError("Automation: ConformSliders - no reference shape loaded.");
		return 1;
	}

	ConformOptions options;
	options.proximityRadius = step.conformProximityRadius;
	options.maxResults = step.conformMaxResults;
	options.smoothResultDeltas = step.conformSmoothResults;
	options.smoothIterations = step.conformSmoothIterations;
	options.smoothStrength = step.conformSmoothStrength;
	options.noSqueeze = step.conformNoSqueeze;
	options.solidMode = step.conformSolidMode;
	options.axisX = step.conformAxisX;
	options.axisY = step.conformAxisY;
	options.axisZ = step.conformAxisZ;
	options.fixClipping = step.conformFixClipping;
	options.fixClippingStrength = std::max(0.0f, std::min(1.0f, step.conformFixClippingStrength));
	options.sliderNames = step.conformSliderNames;

	if (options.sliderNames.empty()) {
		wxLogMessage("Automation: ConformSliders - conforming all non-zap/non-UV sliders.");
	}
	else {
		wxString namesList;
		for (const auto& sn : options.sliderNames) {
			if (!namesList.empty())
				namesList += ", ";
			namesList += wxString::FromUTF8(sn);
		}
		wxLogMessage("Automation: ConformSliders - conforming %zu named slider(s): %s",
					 options.sliderNames.size(), namesList);
	}

	OpResult result = ProjectOperations(*project, outfitStudio->editor).ConformSliders(shapes, options);
	if (!result.ok) {
		wxLogError("Automation: ConformSliders - %s", result.error);
		return 1;
	}

	outfitStudio->ApplyRefresh(result.refresh);
	return 0;
}

// Imports the morphs of a .tri file as slider data, auto-detecting the file type.
// RaceMenu body TRI files store their morphs per shape name, FaceGen head TRI files
// store a mesh with morphs and no shape names, so those are matched by vertex count.
// Returns the number of imported morphs or -1 if the file couldn't be read.
int AutomationRunner::ImportSliderDataFromTRI(const AutomationStep& step, const std::string& filePath) {
	auto sliderSelected = [&step](const std::string& sliderName) {
		if (step.sliderNames.empty())
			return true;

		return std::find(step.sliderNames.begin(), step.sliderNames.end(), sliderName) != step.sliderNames.end();
	};

	int importCount = 0;

	if (IsHeadTriFile(filePath)) {
		TriHeadFile tri;
		if (!tri.Read(filePath))
			return -1;

		// Head TRI files have no shape names, so the shapes are matched by vertex count
		int triVertCount = static_cast<int>(tri.GetVertexCount());
		if (triVertCount > std::numeric_limits<uint16_t>::max()) {
			wxLogWarning("Automation: Head TRI file '%s' has too many vertices (%d).", wxString::FromUTF8(filePath), triVertCount);
			return 0;
		}

		std::vector<NiShape*> shapes;
		for (auto* shape : project->GetWorkNif()->GetShapes())
			if (project->GetVertexCount(shape) == triVertCount)
				shapes.push_back(shape);

		if (shapes.empty()) {
			wxLogWarning("Automation: No shape with %d vertices found for head TRI file '%s'.", triVertCount, wxString::FromUTF8(filePath));
			return 0;
		}

		// Prefer a shape that's named like the file, if there is one
		NiShape* namedShape = nullptr;
		std::string fileBaseName = wxFileName(wxString::FromUTF8(filePath)).GetName().ToUTF8().data();
		for (auto* shape : shapes)
			if (shape->name.get() == fileBaseName)
				namedShape = shape;

		if (namedShape)
			shapes.assign(1, namedShape);

		auto morphs = tri.GetMorphs();
		for (auto* shape : shapes) {
			std::string shapeName = shape->name.get();

			for (auto& morph : morphs) {
				if (!sliderSelected(morph.morphName))
					continue;

				std::unordered_map<uint16_t, Vector3> diff;
				for (size_t i = 0; i < morph.vertices.size(); i++)
					if (!morph.vertices[i].IsZero(true))
						diff.emplace(static_cast<uint16_t>(i), morph.vertices[i]);

				if (diff.empty())
					continue;

				if (!project->ValidSlider(morph.morphName)) {
					if (step.sliderMerge)
						continue;

					project->AddEmptySlider(morph.morphName);
				}

				project->SetSliderFromDiff(morph.morphName, shape, diff);

				importCount++;
				wxLogMessage("Automation: Imported morph '%s' for shape '%s'.", morph.morphName, shapeName);
			}
		}

		return importCount;
	}

	TriFile tri;
	if (!tri.Read(filePath))
		return -1;

	auto morphs = tri.GetMorphs();
	for (auto& [shapeName, morphList] : morphs) {
		auto* shape = project->GetWorkNif()->FindBlockByName<NiShape>(shapeName);
		if (!shape)
			continue;

		for (auto& morphData : morphList) {
			if (!sliderSelected(morphData->name))
				continue;

			if (!project->ValidSlider(morphData->name)) {
				if (step.sliderMerge)
					continue;

				project->AddEmptySlider(morphData->name);
			}

			std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			project->SetSliderFromDiff(morphData->name, shape, diff);

			if (morphData->type == MORPHTYPE_UV) {
				size_t sliderIndex = 0;
				if (project->SliderIndexFromName(morphData->name, sliderIndex))
					project->SetSliderUV(sliderIndex, true);
			}

			importCount++;
			wxLogMessage("Automation: Imported morph '%s' for shape '%s'.", morphData->name, shapeName);
		}
	}

	return importCount;
}

int AutomationRunner::ExecuteStepImportSliderData(const AutomationStep& step) {
	if (step.sliderDataFile.empty()) {
		wxLogError("Automation: ImportSliderData - no file/folder specified.");
		return 1;
	}

	wxString sliderDataPath = MakeAbsoluteToProject(wxString::FromUTF8(step.sliderDataFile));
	std::string sliderDataPathStd = sliderDataPath.ToUTF8().data();

	if (step.sliderDataFromFolder) {
		wxLogMessage("Automation: Importing slider data from folder '%s'...", sliderDataPath);
		wxDir dir(sliderDataPath);
		if (!dir.IsOpened()) {
			wxLogError("Automation: ImportSliderData - cannot open folder '%s'.", sliderDataPath);
			return 1;
		}

		int importCount = 0;
		const auto& shapes = project->GetWorkNif()->GetShapes();
		wxString filename;
		bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (cont) {
			std::string fname = filename.ToUTF8().data();
			wxFileName wxFn(filename);
			std::string extLower = wxFn.GetExt().Lower().ToUTF8().data();
			std::string fullPath = sliderDataPathStd + "/" + fname;

			if (extLower == "osd") {
				// OSD: import all sliders, auto-mapping shapes by target name
				OSDataFile osd;
				if (osd.Read(fullPath)) {
					auto& diffs = osd.GetDataDiffs();
					for (auto& [diffName, diffData] : diffs) {
						std::string bestTargetName;
						NiShape* bestShape = nullptr;

						for (auto* shape : shapes) {
							std::string targetName = project->ShapeToTarget(shape->name.get());
							if (diffName.substr(0, targetName.size()) == targetName) {
								if (targetName.length() > bestTargetName.length()) {
									bestTargetName = targetName;
									bestShape = shape;
								}
							}
						}

						if (!bestShape || bestTargetName.empty())
							continue;

						auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diffName);
						if (sliderName.empty())
							sliderName = diffName.substr(bestTargetName.length());

						if (!step.sliderNames.empty()) {
							bool found = false;
							for (const auto& name : step.sliderNames) {
								if (name == sliderName) {
									found = true;
									break;
								}
							}
							if (!found)
								continue;
						}

						if (!project->ValidSlider(sliderName)) {
							if (step.sliderMerge)
								continue;
							project->AddEmptySlider(sliderName);
						}

						if (diffData)
							project->SetSliderFromDiff(sliderName, bestShape, *diffData);

						importCount++;
						wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, bestShape->name.get());
					}
				}
				else {
					wxLogWarning("Automation: Failed to read OSD file '%s'.", fname);
				}
			}
			else if (extLower == "tri") {
				// TRI: import all morphs, auto-mapping shapes by name or vertex count
				int triImportCount = ImportSliderDataFromTRI(step, fullPath);
				if (triImportCount >= 0)
					importCount += triImportCount;
				else
					wxLogWarning("Automation: Failed to read TRI file '%s'.", fname);
			}
			else if (extLower == "bsd" || extLower == "nif" || extLower == "obj" || extLower == "fbx") {
				// NIF/OBJ/FBX/BSD: use "ShapeName#SliderName.ext" naming pattern
				auto hashPos = fname.find('#');
				if (hashPos != std::string::npos) {
					std::string shapeName = fname.substr(0, hashPos);
					std::string rest = fname.substr(hashPos + 1);
					auto dotPos = rest.rfind('.');
					std::string sliderName = (dotPos != std::string::npos) ? rest.substr(0, dotPos) : rest;

					NiShape* shape = FindShapeByName(shapeName);
					if (shape) {
						if (step.sliderMerge && !project->ValidSlider(sliderName)) {
							cont = dir.GetNext(&filename);
							continue;
						}

						if (!project->ValidSlider(sliderName))
							project->AddEmptySlider(sliderName);

						bool ok = false;
						if (extLower == "bsd") {
							project->SetSliderFromBSD(sliderName, shape, fullPath);
							ok = true;
						}
						else if (extLower == "nif") {
							ok = project->SetSliderFromNIF(sliderName, shape, fullPath);
						}
						else if (extLower == "obj") {
							ok = project->SetSliderFromOBJ(sliderName, shape, fullPath);
						}
#ifdef USE_FBXSDK
						else if (extLower == "fbx") {
							ok = project->SetSliderFromFBX(sliderName, shape, fullPath);
						}
#endif

						if (ok) {
							importCount++;
							wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, shapeName);
						}
						else {
							wxLogWarning("Automation: Failed to import slider '%s' for shape '%s' from '%s'.", sliderName, shapeName, fname);
						}
					}
					else {
						wxLogWarning("Automation: Shape '%s' not found for file '%s'.", shapeName, fname);
					}
				}
			}

			cont = dir.GetNext(&filename);
		}

		wxLogMessage("Automation: Imported %d slider data file(s) from folder.", importCount);
	}
	else {
		// Single file mode
		wxLogMessage("Automation: Importing slider data from '%s'...", sliderDataPath);
		wxFileName fn(sliderDataPath);
		wxString ext = fn.GetExt().Lower();

		if (ext == "osd") {
			// OSD multi-diff import
			OSDataFile osd;
			if (!osd.Read(sliderDataPathStd)) {
				wxLogError("Automation: Failed to read OSD file '%s'.", sliderDataPath);
				return 1;
			}

			auto& diffs = osd.GetDataDiffs();
			const auto& shapes = project->GetWorkNif()->GetShapes();

			for (auto& [diffName, diffData] : diffs) {
				std::string bestTargetName;
				NiShape* bestShape = nullptr;

				for (auto* shape : shapes) {
					std::string shapeName = shape->name.get();
					std::string targetName = project->ShapeToTarget(shapeName);
					if (diffName.substr(0, targetName.size()) == targetName) {
						if (targetName.length() > bestTargetName.length()) {
							bestTargetName = targetName;
							bestShape = shape;
						}
					}
				}

				if (!bestShape || bestTargetName.empty())
					continue;

				auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diffName);
				if (sliderName.empty())
					sliderName = diffName.substr(bestTargetName.length());

				if (!step.sliderNames.empty()) {
					bool found = false;
					for (const auto& name : step.sliderNames) {
						if (name == sliderName) {
							found = true;
							break;
						}
					}
					if (!found)
						continue;
				}

				if (!project->ValidSlider(sliderName)) {
					if (step.sliderMerge)
						continue;
					project->AddEmptySlider(sliderName);
				}

				if (diffData)
					project->SetSliderFromDiff(sliderName, bestShape, *diffData);
				wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, bestShape->name.get());
			}
		}
		else if (ext == "tri") {
			// TRI multi-morph import
			if (ImportSliderDataFromTRI(step, sliderDataPathStd) < 0) {
				wxLogError("Automation: Failed to read TRI file '%s'.", sliderDataPath);
				return 1;
			}
		}
		else if (ext == "nif" || ext == "obj" || ext == "bsd" || ext == "fbx") {
			// Per-shape slider import: compute diff from file mesh vs current shape
			std::string sliderName;
			if (!step.sliderNames.empty())
				sliderName = step.sliderNames[0];
			else
				sliderName = fn.GetName().ToUTF8().data();

			if (!project->ValidSlider(sliderName)) {
				if (step.sliderMerge) {
					wxLogWarning("Automation: Slider '%s' does not exist and merge-only is enabled.", sliderName);
					return 0;
				}
				project->AddEmptySlider(sliderName);
			}

			const auto& shapes = project->GetWorkNif()->GetShapes();
			for (auto* shape : shapes) {
				bool ok = false;
				if (ext == "nif")
					ok = project->SetSliderFromNIF(sliderName, shape, sliderDataPathStd);
				else if (ext == "obj")
					ok = project->SetSliderFromOBJ(sliderName, shape, sliderDataPathStd);
				else if (ext == "bsd") {
					project->SetSliderFromBSD(sliderName, shape, sliderDataPathStd);
					ok = true;
				}
#ifdef USE_FBXSDK
				else if (ext == "fbx")
					ok = project->SetSliderFromFBX(sliderName, shape, sliderDataPathStd);
#endif

				if (ok)
					wxLogMessage("Automation: Imported slider '%s' for shape '%s'.", sliderName, shape->name.get());
			}
		}
		else {
			wxLogError("Automation: ImportSliderData - unsupported file format '.%s'.", ext);
			return 1;
		}
	}
	return 0;
}

int AutomationRunner::ExecuteStepDeleteSlider(const AutomationStep& step) {
	if (step.deleteSliderNames.empty()) {
		wxLogError("Automation: DeleteSlider - no slider name specified.");
		return 1;
	}

	if (step.deleteSliderRegex) {
		// In regex mode, use first entry as pattern
		std::string pattern = JoinStrings(step.deleteSliderNames, ", ");
		try {
			std::regex re(pattern, std::regex::icase);
			std::vector<std::string> sliderList;
			project->GetSliderList(sliderList);
			int deleted = 0;
			for (const auto& name : sliderList) {
				if (std::regex_search(name, re)) {
					wxLogMessage("Automation: Deleting slider '%s'...", name);
					project->DeleteSlider(name);
					deleted++;
				}
			}
			wxLogMessage("Automation: Deleted %d slider(s) matching '%s'.", deleted, pattern);
		}
		catch (const std::regex_error&) {
			wxLogError("Automation: DeleteSlider - invalid regex '%s'.", pattern);
			return 1;
		}
	}
	else {
		for (const auto& sliderName : step.deleteSliderNames) {
			wxLogMessage("Automation: Deleting slider '%s'...", sliderName);
			project->DeleteSlider(sliderName);
		}
	}
	return 0;
}

int AutomationRunner::ExecuteStepSetSliderProperties(const AutomationStep& step) {
	wxLogMessage("Automation: Setting slider properties...");

	for (size_t i = 0; i < project->SliderCount(); i++) {
		std::string name = project->GetSliderName(i);

		// If specific slider names given, check if this one matches
		if (!step.sliderPropNames.empty()) {
			bool found = false;
			for (const auto& n : step.sliderPropNames) {
				if (n == name) {
					found = true;
					break;
				}
			}
			if (!found)
				continue;
		}

		if (step.sliderPropZap >= 0) {
			project->SetSliderZap(i, step.sliderPropZap != 0);
			wxLogMessage("Automation: Slider '%s' zap = %s.", name, step.sliderPropZap ? "true" : "false");
		}
		if (step.sliderPropHidden >= 0) {
			project->SetSliderHidden(i, step.sliderPropHidden != 0);
			wxLogMessage("Automation: Slider '%s' hidden = %s.", name, step.sliderPropHidden ? "true" : "false");
		}
		if (step.sliderPropDefaultLo >= 0) {
			project->SetSliderDefault(i, step.sliderPropDefaultLo, false);
			wxLogMessage("Automation: Slider '%s' default (small) = %d.", name, step.sliderPropDefaultLo);
		}
		if (step.sliderPropDefaultHi >= 0) {
			project->SetSliderDefault(i, step.sliderPropDefaultHi, true);
			wxLogMessage("Automation: Slider '%s' default (big) = %d.", name, step.sliderPropDefaultHi);
		}
	}

	return 0;
}

int AutomationRunner::ExecuteStepFixClipping(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: FixClipping - no target shapes found.");
		return 0;
	}

	ClippingFixMode mode = step.fixClipMode == 1 ? ClippingFixMode::Sliders : ClippingFixMode::Shapes;
	wxLogMessage("Automation: FixClipping (%s mode, strength=%.0f%%)...", mode == ClippingFixMode::Sliders ? "Sliders" : "Shapes", step.fixClipStrength * 100.0f);

	OpResult result = ProjectOperations(*project, outfitStudio->editor).FixClipping(shapes, mode, step.fixClipStrength, step.fixClipSliderNames);
	if (!result.ok) {
		wxLogError("Automation: FixClipping - %s", result.error);
		return 1;
	}

	outfitStudio->ApplyRefresh(result.refresh);
	return 0;
}

int AutomationRunner::ExecuteStepClearSliderData(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: ClearSliderData - no target shapes found.");
		return 0;
	}

	std::vector<std::string> sliderNames = step.clearSliderNames;
	if (sliderNames.empty())
		project->GetSliderList(sliderNames);

	if (sliderNames.empty()) {
		wxLogWarning("Automation: ClearSliderData - the project has no sliders.");
		return 0;
	}

	for (const auto& sliderName : sliderNames) {
		if (!project->ValidSlider(sliderName)) {
			wxLogWarning("Automation: ClearSliderData - slider '%s' not found, skipping.", sliderName);
			continue;
		}

		for (auto* shape : shapes) {
			std::string shapeName = shape->name.get();

			// Masked vertices keep their slider data, like the menu item
			std::unordered_map<uint16_t, float> mask;
			if (step.clearSliderUseMask)
				outfitStudio->glView->GetShapeMask(mask, shapeName);

			wxLogMessage("Automation: Clearing the data of slider '%s' for '%s'...", sliderName, shapeName);
			if (!mask.empty())
				project->ClearUnmaskedDiff(shape, sliderName, &mask);
			else
				project->ClearSlider(shape, sliderName);
		}
	}

	outfitStudio->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepCloneSlider(const AutomationStep& step) {
	if (step.cloneSliderSource.empty() || step.newSliderName.empty()) {
		wxLogError("Automation: CloneSlider - both a source slider and a new name are required.");
		return 1;
	}

	if (!project->ValidSlider(step.cloneSliderSource)) {
		wxLogError("Automation: CloneSlider - slider '%s' not found.", step.cloneSliderSource);
		return 1;
	}

	if (project->ValidSlider(step.newSliderName)) {
		wxLogError("Automation: CloneSlider - slider '%s' already exists.", step.newSliderName);
		return 1;
	}

	wxLogMessage("Automation: Cloning slider '%s' as '%s'...", step.cloneSliderSource, step.newSliderName);
	project->CloneSlider(step.cloneSliderSource, step.newSliderName);
	outfitStudio->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepNegateSlider(const AutomationStep& step) {
	if (step.negateSliderNames.empty()) {
		wxLogError("Automation: NegateSlider - no slider name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: NegateSlider - no target shapes found.");
		return 0;
	}

	for (const auto& sliderName : step.negateSliderNames) {
		if (!project->ValidSlider(sliderName)) {
			wxLogWarning("Automation: NegateSlider - slider '%s' not found, skipping.", sliderName);
			continue;
		}

		for (auto* shape : shapes) {
			wxLogMessage("Automation: Negating slider '%s' for '%s'...", sliderName, shape->name.get());
			project->NegateSlider(sliderName, shape);
		}
	}

	outfitStudio->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepRigidifySlider(const AutomationStep& step) {
	// The reference keeps its morphs, rigidify only changes the outfit
	std::vector<NiShape*> shapes;
	for (auto* shape : ResolveTargetShapes(step, false))
		if (!project->IsBaseShape(shape))
			shapes.push_back(shape);

	if (shapes.empty()) {
		wxLogWarning("Automation: RigidifySlider - no target shapes found.");
		return 0;
	}

	// Named sliders, or else every morph slider with data on the target shapes
	std::vector<std::string> sliderNames;
	if (!step.rigidifySliderNames.empty()) {
		for (const auto& sliderName : step.rigidifySliderNames) {
			size_t sliderIndex = 0;
			if (!project->SliderIndexFromName(sliderName, sliderIndex))
				wxLogWarning("Automation: RigidifySlider - slider '%s' not found, skipping.", sliderName);
			else if (project->SliderZap(sliderIndex) || project->SliderUV(sliderIndex))
				wxLogWarning("Automation: RigidifySlider - slider '%s' is a zap or UV slider, skipping.", sliderName);
			else
				sliderNames.push_back(sliderName);
		}
	}
	else {
		for (size_t i = 0; i < project->SliderCount(); i++) {
			if (project->SliderZap(i) || project->SliderUV(i))
				continue;
			for (auto* shape : shapes) {
				TargetDataDiffs* diff = project->GetDiffSet(project->activeSet[i], shape);
				if (diff && !diff->empty()) {
					sliderNames.push_back(project->GetSliderName(i));
					break;
				}
			}
		}
	}

	if (sliderNames.empty()) {
		wxLogWarning("Automation: RigidifySlider - no sliders to rigidify.");
		return 0;
	}

	RigidifyOptions options;
	options.allowScale = step.rigidifyMode == 1;
	options.perPart = step.rigidifyMode != 2;
	options.clearance = step.rigidifyClearance;
	options.maxPartVertices = std::max(0, step.rigidifyMaxPartVertices);
	options.maxPartLength = std::max(0.0f, step.rigidifyMaxPartLength);

	// Rigidify reads the project's slider data, not the meshes, so they are refreshed once at the end
	ProjectOperations ops(*project, outfitStudio->editor);
	RefreshFlags refresh = RefreshFlags::None;
	for (const auto& sliderName : sliderNames) {
		wxLogMessage("Automation: Rigidifying the parts of slider '%s' for %zu shape(s)...", sliderName, shapes.size());

		// One slider per undo state
		UndoStateProject usp;
		size_t parts = 0;
		OpResult result = ops.RigidifySlider(shapes, sliderName, options, usp, &parts);
		refresh |= result.refresh;
		if (!result.ok) {
			outfitStudio->ApplyRefresh(refresh);
			wxLogError("Automation: RigidifySlider - %s", result.error);
			return 1;
		}
		wxLogMessage("Automation: Rigidified %zu part(s), %zu shape(s) changed.", parts, usp.usss.size());
	}

	outfitStudio->ApplyRefresh(refresh);
	return 0;
}

int AutomationRunner::ExecuteStepNewCombinedSlider(const AutomationStep& step) {
	if (step.newSliderName.empty()) {
		wxLogError("Automation: NewCombinedSlider - no slider name specified.");
		return 1;
	}

	if (project->ValidSlider(step.newSliderName)) {
		wxLogError("Automation: NewCombinedSlider - slider '%s' already exists.", step.newSliderName);
		return 1;
	}

	wxLogMessage("Automation: Creating combined slider '%s'...", step.newSliderName);
	project->AddCombinedSlider(step.newSliderName);
	outfitStudio->SetPendingChanges();
	return 0;
}

int AutomationRunner::ExecuteStepNewZapSlider(const AutomationStep& step) {
	if (step.newSliderName.empty()) {
		wxLogError("Automation: NewZapSlider - no slider name specified.");
		return 1;
	}

	if (project->ValidSlider(step.newSliderName)) {
		wxLogError("Automation: NewZapSlider - slider '%s' already exists.", step.newSliderName);
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: NewZapSlider - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();

		// The zap removes the unmasked vertices, so a mask marks what to keep
		std::unordered_map<uint16_t, float> unmasked;
		outfitStudio->glView->GetShapeUnmasked(unmasked, shapeName);
		if (unmasked.empty()) {
			wxLogWarning("Automation: NewZapSlider - '%s' is fully masked, skipping.", shapeName);
			continue;
		}

		wxLogMessage("Automation: Adding zap '%s' for %zu vertices of '%s'...", step.newSliderName, unmasked.size(), shapeName);
		project->AddZapSlider(step.newSliderName, unmasked, shape);
	}

	outfitStudio->SetPendingChanges();
	return 0;
}
