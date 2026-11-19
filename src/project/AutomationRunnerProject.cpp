/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: projects, references, importing, exporting and saving

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "outfitstudio/OutfitStudioFrame.h"
#include "outfitstudio/GLPanel.h"
#include "GameImport.h"
#include "utils/ConfigurationManager.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include <NifFile.hpp>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

using namespace nifly;

int AutomationRunner::ImportOBJFile(const std::string& fileName) {
	if (headlessMode)
		return project->ImportOBJ(fileName, objImportOptions);

	return outfitStudio->ImportOBJWithDialog(fileName);
}
#ifdef USE_FBXSDK
int AutomationRunner::ImportFBXFile(const std::string& fileName) {
	if (headlessMode)
		return project->ImportFBX(fileName, FBXImportOptions());

	return outfitStudio->ImportFBXWithDialog(fileName);
}
#endif

int AutomationRunner::ExecuteStepClearProject(const AutomationStep&) {
	wxLogMessage("Automation: Clearing project...");
	ResetAndClearProject();
	return 0;
}

int AutomationRunner::ExecuteStepLoadReference(const AutomationStep& step) {
	if (step.refSourceFile.empty()) {
		wxLogError("Automation: LoadReference - no source file specified.");
		return 1;
	}

	ReferenceSource source;
	source.file = MakeAbsoluteToProject(wxString::FromUTF8(step.refSourceFile)).ToUTF8().data();
	source.set = step.refSet;
	source.shape = step.refShape;
	source.loadAll = step.refLoadAll;
	source.mergeSliders = step.refMergeSliders;
	source.mergeZaps = step.refMergeZaps;
	source.appendNewSliders = step.refAppendNewSliders;
	source.loadAddOns = step.refLoadAddOns;

	OpResult result = ProjectOperations(*project, outfitStudio->editor).LoadReference(source);
	if (!result.ok) {
		wxLogError("Automation: LoadReference - %s", result.error);
		return 1;
	}

	return 0;
}

int AutomationRunner::ExecuteStepAddProject(const AutomationStep& step) {
	if (step.refSourceFile.empty()) {
		wxLogError("Automation: AddProject - no source file specified.");
		return 1;
	}

	wxString refSourceFile = MakeAbsoluteToProject(wxString::FromUTF8(step.refSourceFile));
	wxLogMessage("Automation: Adding project from '%s' (set: '%s')...", refSourceFile, step.refSet);

	OpResult result = ProjectOperations(*project, outfitStudio->editor).AddProject(refSourceFile.ToUTF8().data(), step.refSet, step.refAppendNewSliders);
	if (!result.ok) {
		wxLogError("Automation: AddProject - %s", result.error);
		return 1;
	}

	return 0;
}

int AutomationRunner::ExecuteStepSetBaseShape(const AutomationStep&) {
	wxLogMessage("Automation: Setting base shape (baking slider values)...");
	outfitStudio->SetBaseShape();
	return 0;
}

int AutomationRunner::ExecuteStepClearReference(const AutomationStep&) {
	wxLogMessage("Automation: Clearing reference...");
	project->DeleteShape(project->GetBaseShape());
	return 0;
}

int AutomationRunner::ExecuteStepImportFile(const AutomationStep& step) {
	if (step.importFilePath.empty()) {
		wxLogError("Automation: ImportFile - no file/folder specified.");
		return 1;
	}

	wxString importFilePath = MakeAbsoluteToProject(wxString::FromUTF8(step.importFilePath));
	std::string importFilePathStd = importFilePath.ToUTF8().data();

	if (step.importFromFolder) {
		// Folder mode: import all NIF/OBJ/FBX/TRI files from the folder
		wxLogMessage("Automation: Importing all files from folder '%s'...", importFilePath);
		wxDir dir(importFilePath);
		if (!dir.IsOpened()) {
			wxLogError("Automation: ImportFile - cannot open folder '%s'.", importFilePath);
			return 1;
		}

		int importCount = 0;
		wxString filename;
		bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
		while (cont) {
			wxFileName fn(filename);
			wxString ext = fn.GetExt().Lower();
			if (ext == "nif" || ext == "obj" || ext == "fbx" || ext == "tri") {
				std::string fullPath = importFilePathStd + "/" + filename.ToUTF8().data();
				int err = 0;
				if (ext == "nif")
					err = project->ImportNIF(fullPath, false);
				else if (ext == "obj")
					err = ImportOBJFile(fullPath);
				else if (ext == "tri")
					// Only head TRI files carry a mesh, body TRI files are handled by the ImportSliderData step
					err = project->ImportHeadTRI(fullPath, "", step.importTriSliders) ? 0 : 1;
				else if (ext == "fbx")
#ifdef USE_FBXSDK
					err = ImportFBXFile(fullPath);
#else
					wxLogError("Automation: FBX import is not available (FBX SDK not compiled in).");
#endif

				if (err)
					wxLogWarning("Automation: Failed to import '%s' (error %d).", fullPath, err);
				else
					importCount++;
			}
			cont = dir.GetNext(&filename);
		}

		wxLogMessage("Automation: Imported %d file(s) from folder.", importCount);
	}
	else {
		// Single file mode
		wxFileName fn(importFilePath);
		wxString ext = fn.GetExt().Lower();
		int err = 0;

		if (ext == "nif")
			err = project->ImportNIF(importFilePathStd, false);
		else if (ext == "obj")
			err = ImportOBJFile(importFilePathStd);
		else if (ext == "tri")
			// Only head TRI files carry a mesh, body TRI files are handled by the ImportSliderData step
			err = project->ImportHeadTRI(importFilePathStd, "", step.importTriSliders) ? 0 : 1;
		else if (ext == "fbx")
#ifdef USE_FBXSDK
			err = ImportFBXFile(importFilePathStd);
#else
			wxLogError("Automation: FBX import is not available (FBX SDK not compiled in).");
#endif
		else {
			wxLogError("Automation: ImportFile - unsupported file extension '%s'.", ext);
			return 1;
		}

		if (err) {
			wxLogError("Automation: ImportFile '%s' failed with error %d.", importFilePath, err);
			return err;
		}
	}
	return 0;
}

int AutomationRunner::ExecuteStepImportFromGame(const AutomationStep& step) {
	if (step.gameImportFile.empty()) {
		wxLogError("Automation: ImportFromGame - no file specified.");
		return 1;
	}

	GameImport::Options options;
	options.file = MakeAbsoluteToProject(wxString::FromUTF8(step.gameImportFile)).ToUTF8().data();
	options.set = step.gameImportSet;
	options.shapes = step.gameImportShapes;
	options.skipShapes = step.gameImportSkipShapes;
	if (!step.gameImportSliders.empty())
		options.sliders = step.gameImportSliders;
	for (auto& folder : step.gameImportDataFolders)
		options.dataFolders.push_back(MakeAbsoluteToProject(wxString::FromUTF8(folder)).ToUTF8().data());

	wxLogMessage("Automation: Importing '%s' from another game...", options.file);

	GameImport::Report report;
	OpResult result = GameImport::Import(*project, options, report);
	if (!result.ok) {
		wxLogError("Automation: ImportFromGame - %s", result.error);
		return 1;
	}

	// Each material note once, they repeat for every shape with the same material
	std::vector<std::string> notes;
	for (auto& shape : report.shapes) {
		const std::string name = shape["name"].get<std::string>();
		wxLogMessage("Automation: ImportFromGame - imported '%s' from %s.", name, report.sourceGame);
		if (shape.contains("warning"))
			wxLogWarning("Automation: ImportFromGame - '%s': %s", name, shape["warning"].get<std::string>());

		if (!shape.contains("material"))
			continue;

		const auto& material = shape["material"];
		if (material.contains("error"))
			wxLogWarning("Automation: ImportFromGame - '%s': %s", name, material["error"].get<std::string>());
		if (material.contains("target") && material["target"].contains("notes"))
			for (auto& note : material["target"]["notes"])
				if (std::find(notes.begin(), notes.end(), note.get<std::string>()) == notes.end())
					notes.push_back(note.get<std::string>());
	}
	for (auto& note : notes)
		wxLogMessage("Automation: ImportFromGame - %s", note);

	return 0;
}

int AutomationRunner::ExecuteStepSaveProject(const AutomationStep& step) {
	wxLogMessage("Automation: Saving project '%s'...", step.saveName);

	SaveProjectOptions options;
	options.displayName = step.saveName;
	options.sliderSetFile = step.saveSliderSetFile;
	options.shapeDataFolder = step.saveShapeDataFolder;
	options.shapeDataFile = step.saveShapeDataFile;
	options.outputDataPath = step.saveOutputDataPath;
	options.outputFileName = step.saveOutputFileName;
	options.genWeights = step.saveGenWeights;
	options.copyReference = step.saveAutoCopyRef;
	options.sfMorphPath = step.saveSFMorphPath;
	options.sfMorphTargetShape = step.saveSFMorphTargetShape;

	OpResult result = ProjectOperations(*project, outfitStudio->editor).SaveProject(options);
	if (!result.ok) {
		wxLogError("Automation: SaveProject - %s", result.error);
		return 1;
	}

	outfitStudio->SetPendingChanges(false);
	return 0;
}

int AutomationRunner::ExecuteStepExportFile(const AutomationStep& step) {
	if (step.exportFilePath.empty()) {
		wxLogError("Automation: ExportFile - no file path specified.");
		return 1;
	}

	// Apply prefix/suffix to filename
	std::string exportPath = step.exportFilePath;
	if (!step.exportPrefix.empty() || !step.exportSuffix.empty()) {
		wxFileName fn(wxString::FromUTF8(exportPath));
		wxString name = fn.GetName();
		if (!step.exportPrefix.empty())
			name = wxString::FromUTF8(step.exportPrefix) + name;
		if (!step.exportSuffix.empty())
			name = name + wxString::FromUTF8(step.exportSuffix);
		fn.SetName(name);
		exportPath = fn.GetFullPath().ToUTF8().data();
	}

	wxLogMessage("Automation: Exporting to '%s'...", exportPath);

	// OBJ and FBX exports from automation have always written every shape, including the reference
	ExportOptions options;
	wxString ext = wxFileName(wxString::FromUTF8(exportPath)).GetExt().Lower();
	options.withReference = step.exportWithRef || ext == "obj" || ext == "fbx";

	// A head TRI file holds the morphs of a single shape. Without target meshes, that's the only non-reference shape.
	if (ext == "tri" && step.exportHeadTri) {
		auto shapes = ResolveTargetShapes(step, false);
		if (shapes.size() != 1) {
			wxLogError("Automation: ExportFile - a head TRI file needs exactly one target shape, found %zu. Specify the shape in Target Meshes.", shapes.size());
			return 1;
		}

		options.headTri = true;
		options.shapes.push_back(shapes.front()->name.get());
	}

	OpResult result = ProjectOperations(*project, outfitStudio->editor).ExportFile(exportPath, options);
	if (!result.ok) {
		wxLogError("Automation: ExportFile - %s", result.error);
		return 1;
	}

	return 0;
}

int AutomationRunner::ExecuteStepExportSFMorphs(const AutomationStep& step) {
	if (step.exportSFMorphsFolder.empty()) {
		wxLogError("Automation: ExportSFMorphs - no output folder specified.");
		return 1;
	}

	// A morph.dat holds the morphs of a single shape. Without target meshes, fall back
	// to the project's morph target shape, then to the only non-reference shape.
	std::vector<NiShape*> shapes;
	if (step.targetMeshes.empty() && !project->mSFMorphTargetShape.empty()) {
		NiShape* shape = FindShapeByName(project->mSFMorphTargetShape.ToUTF8().data());
		if (shape)
			shapes.push_back(shape);
		else
			wxLogWarning("Automation: ExportSFMorphs - project morph target shape '%s' not found.", project->mSFMorphTargetShape);
	}

	if (shapes.empty())
		shapes = ResolveTargetShapes(step, false);

	if (shapes.size() != 1) {
		wxLogError("Automation: ExportSFMorphs - expected exactly one target shape, found %zu. Specify the morph target shape in Target Meshes.", shapes.size());
		return 1;
	}

	// Relative folders are resolved against the game data path, like the project's morph.dat path
	wxFileName morphFolder = wxFileName::DirName(wxString::FromUTF8(step.exportSFMorphsFolder));
	if (morphFolder.IsRelative()) {
		wxString dataPath = wxString::FromUTF8(Config["GameDataPath"]);
		if (dataPath.empty()) {
			wxLogError("Automation: ExportSFMorphs - relative folder '%s' needs a game data path to be set.", morphFolder.GetPath());
			return 1;
		}
		morphFolder.MakeAbsolute(dataPath);
	}

	wxFileName::Mkdir(morphFolder.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxFileName morphFile(morphFolder.GetPath(), "morph.dat");
	wxString morphFilePath = morphFile.GetFullPath();

	NiShape* shape = shapes.front();
	wxLogMessage("Automation: Exporting Starfield morph.dat for shape '%s' to '%s'...", shape->name.get(), morphFilePath);
	if (!project->WriteSFMorphs(shape, morphFilePath.ToUTF8().data())) {
		wxLogError("Automation: ExportSFMorphs - failed to write '%s'.", morphFilePath);
		return 1;
	}
	return 0;
}

int AutomationRunner::ExecuteStepSetReferenceShape(const AutomationStep& step) {
	if (step.setRefUnset) {
		wxLogMessage("Automation: Unsetting reference shape...");
		project->SetBaseShape(nullptr);
		return 0;
	}

	if (step.setRefShapeName.empty()) {
		wxLogError("Automation: SetReferenceShape - no shape name specified.");
		return 1;
	}

	NiShape* shape = FindShapeByName(step.setRefShapeName);
	if (!shape) {
		if (step.setRefSkipIfMissing) {
			wxLogMessage("Automation: SetReferenceShape - shape '%s' not found, skipping.", step.setRefShapeName);
			return 0;
		}

		wxLogError("Automation: SetReferenceShape - shape '%s' not found.", step.setRefShapeName);
		return 1;
	}

	wxLogMessage("Automation: Setting reference shape to '%s'...", step.setRefShapeName);
	project->SetBaseShape(shape);
	return 0;
}

int AutomationRunner::ExecuteStepMakeConversionRef(const AutomationStep& step) {
	if (step.convRefSliderName.empty()) {
		wxLogError("Automation: MakeConversionRef - no slider name specified.");
		return 1;
	}

	if (project->AllSlidersZero()) {
		wxLogError("Automation: MakeConversionRef - at least one slider value has to be non-zero.");
		return 1;
	}

	if (project->ValidSlider(step.convRefSliderName)) {
		wxLogError("Automation: MakeConversionRef - slider '%s' already exists.", step.convRefSliderName);
		return 1;
	}

	wxLogMessage("Automation: Creating conversion slider '%s'...", step.convRefSliderName);
	project->AddCombinedSlider(step.convRefSliderName);

	auto* baseShape = project->GetBaseShape();
	if (baseShape) {
		// The reference mesh carries the current slider result; make that the new
		// base geometry and let the conversion slider undo it.
		Mesh* m = outfitStudio->glView->GetMesh(baseShape->name.get());
		if (m)
			project->UpdateShapeFromMesh(baseShape, m);

		project->NegateSlider(step.convRefSliderName, baseShape);
	}

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);
	for (const auto& sliderName : sliderList) {
		if (sliderName == step.convRefSliderName)
			continue;

		wxLogMessage("Automation: Deleting slider '%s'...", sliderName);
		project->DeleteSlider(sliderName);
	}

	outfitStudio->glView->GetUndoHistory()->ClearHistory();
	outfitStudio->SetPendingChanges();
	return 0;
}
