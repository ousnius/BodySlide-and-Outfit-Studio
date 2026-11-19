/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Outfit Studio window: importing and exporting shapes and files

#include "OutfitStudioFrame.h"
#include "FrameHelpers.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "components/UndoHistory.h"
#include "files/HkxFile.h"
#include "ui/outfitstudio/FBXImportDialog.h"
#include "ui/outfitstudio/ObjImportDialog.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

// Load files into the current project
int OutfitStudioFrame::ImportOBJWithDialog(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	wxString warningLabel;
	if (!project->GetBaseShape())
		warningLabel = _("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing OBJ files.");

	NifFile* nif = project->GetWorkNif();
	ObjImportDialog import(this, fileName, nif->GetVertexLimit(), nif->GetTriangleLimit(), warningLabel);
	if (import.ShowModal() != wxID_OK)
		return 1;

	return project->ImportOBJ(fileName, import.GetOptions(), shapeName, mergeShape);
}

#ifdef USE_FBXSDK
int OutfitStudioFrame::ImportFBXWithDialog(const std::string& fileName, const std::string& shapeName, NiShape* mergeShape) {
	wxString warningLabel;
	if (!project->GetBaseShape())
		warningLabel = _("No reference has been loaded.  For correct bone transforms, you might need to load a reference before importing FBX files.");

	NifFile* nif = project->GetWorkNif();
	FBXImportDialog import(this, fileName, nif->GetVertexLimit(), nif->GetTriangleLimit(), warningLabel);
	if (import.ShowModal() != wxID_OK)
		return 1;

	return project->ImportFBX(fileName, import.GetOptions(), shapeName, mergeShape);
}
#endif

void OutfitStudioFrame::OnImportNIF(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import NIF file"), wxEmptyString, wxEmptyString, "NIF Files (*.nif)|*.nif", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	StartProgress(_("Importing NIF file..."));
	UpdateProgress(1, _("Importing NIF file..."));

	for (auto& fileName : fileNames)
		project->ImportNIF(fileName.ToUTF8().data(), false);

	UpdateProgress(60, _("Refreshing GUI..."));
	project->SetTextures();

	SetPendingChanges();
	RefreshGUIFromProj();

	UpdateTitle();
	EndProgress();
}

std::optional<bool> OutfitStudioFrame::PromptStarfieldGeometryMode() {
	auto* nif = project->GetWorkNif();
	if (!nif->GetHeader().GetVersion().IsSF())
		return std::nullopt;

	int result = wxMessageBox(
		_("Starfield supports two modes for mesh geometry data:\n\n"
		  "Internal: Mesh data is embedded directly in the NIF file.\n"
		  "Simpler for modding - single file, no external dependencies.\n\n"
		  "External: Mesh data is stored in separate .mesh files under geometries/.\n"
		  "Can be streamed from BA2 archives for better game performance.\n\n"
		  "Would you like to embed the geometry data in the NIF (internal)?\n"
		  "Choose 'Yes' for internal or 'No' for external."),
		_("Starfield Geometry Mode"),
		wxYES_NO | wxICON_QUESTION,
		this);

	return (result == wxYES);
}

void OutfitStudioFrame::OnExportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export outfit NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to NIF file '%s'...", fileName);

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	auto useInternalGeom = PromptStarfieldGeometryMode();

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes, false, useInternalGeom);
	if (error) {
		wxLogError("Failed to save NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportNIFWithRef(wxCommandEvent& event) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (!project->GetBaseShape()) {
		OnExportNIF(event);
		return;
	}

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export project NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project with reference to NIF file '%s'...", fileName);

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		Mesh* m = glView->GetMesh(s);
		if (m)
			shapeMeshes.push_back(m);
	}

	auto useInternalGeom = PromptStarfieldGeometryMode();

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes, true, useInternalGeom);
	if (error) {
		wxLogError("Failed to save NIF file '%s' with reference!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s' with reference!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export selected shapes to NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	std::vector<std::string> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape()->name.get());

	wxLogMessage("Exporting selected shapes to NIF file '%s'.", fileName);

	auto useInternalGeom = PromptStarfieldGeometryMode();

	if (project->ExportShapeNIF(fileName.ToUTF8().data(), shapes, useInternalGeom)) {
		wxLogError("Failed to export selected shapes to NIF file '%s'!", fileName);
		wxMessageBox(_("Failed to export selected shapes to NIF file!"), _("Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import .obj file for new shape"), wxEmptyString, wxEmptyString, "OBJ Files (*.obj)|*.obj", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	for (auto& fileName : fileNames) {
		wxLogMessage("Importing shape(s) from OBJ file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = ImportOBJWithDialog(fileName.ToUTF8().data(), project->OutfitName(), activeItem->GetShape());
		else
			ret = ImportOBJWithDialog(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from OBJ file '%s'.", activeItem->GetShape()->name.get(), fileName);
	}

	RefreshGUIFromProj(false);
	SetPendingChanges();

	wxLogMessage("Imported shape(s) from OBJ.");
	glView->Render();
}

void OutfitStudioFrame::OnExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	bool hasSkinTrans = false;
	for (NiShape* shape : project->GetWorkNif()->GetShapes()) {
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the OBJ?  (This is not recommended.)"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	wxString fileName = wxFileSelector(_("Export project as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);

	if (project->ExportOBJ(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
		wxLogError("Failed to export OBJ file '%s'!", fileName);
		wxMessageBox(_("Failed to export OBJ file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the OBJ?"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	if (selectedItems.size() > 1) {
		wxString fileName
			= wxFileSelector(_("Export selected shapes as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "OBJ Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as OBJ file to '%s'.", fileName);

		std::vector<NiShape*> shapes;
		shapes.reserve(selectedItems.size());
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .obj file"),
										   wxEmptyString,
										   wxString(activeItem->GetShape()->name.get() + ".obj"),
										   ".obj",
										   "OBJ Files (*.obj)|*.obj",
										   wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
										   this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", activeItem->GetShape()->name.get(), fileName);

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnImportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	wxFileDialog importDialog(this, _("Import .fbx file for new shape"), wxEmptyString, wxEmptyString, "FBX Files (*.fbx)|*.fbx", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	for (auto& fileName : fileNames) {
		wxLogMessage("Importing shape(s) from FBX file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = ImportFBXWithDialog(fileName.ToUTF8().data(), project->OutfitName(), activeItem->GetShape());
		else
			ret = ImportFBXWithDialog(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from FBX file '%s'.", activeItem->GetShape()->name.get(), fileName);
	}

	RefreshGUIFromProj(false);
	SetPendingChanges();

	wxLogMessage("Imported shape(s) from FBX.");
	glView->Render();
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	bool hasSkinTrans = false;
	for (NiShape* shape : project->GetWorkNif()->GetShapes()) {
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the FBX?  (This is not recommended.)"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	wxString fileName = wxFileSelector(_("Export project as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);

	if (!project->ExportFBX(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal)) {
		wxLogError("Failed to export FBX file '%s'!", fileName);
		wxMessageBox(_("Failed to export FBX file!"), _("Export Error"), wxICON_ERROR);
	}
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnExportShapeFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!ShapeSelectionCheck())
		return;

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the FBX?"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	if (selectedItems.size() > 1) {
		wxString fileName
			= wxFileSelector(_("Export selected shapes as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as FBX file to '%s'.", fileName);

		std::vector<NiShape*> shapes;
		shapes.reserve(selectedItems.size());
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes, transToGlobal)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .fbx file"),
										   wxEmptyString,
										   wxString(activeItem->GetShape()->name.get() + ".fbx"),
										   ".fbx",
										   "FBX Files (*.fbx)|*.fbx",
										   wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
										   this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as FBX file to '%s'.", activeItem->GetShape()->name.get(), fileName);

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes, transToGlobal)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnImportTRIHead(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import .tri morphs"), wxEmptyString, wxEmptyString, "TRI (Head) Files (*.tri)|*.tri", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	sliderScroll->Freeze();
	MenuExitSliderEdit();
	sliderScroll->FitInside();
	editor.activeSlider.clear();

	for (auto& fn : fileNames) {
		wxFileName fileName(fn);
		wxLogMessage("Importing morphs from TRI (head) file '%s'...", fn);

		std::string shapeName{fileName.GetName().ToUTF8()};
		while (project->IsValidShape(shapeName)) {
			std::string result{GetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"), shapeName, this).ToUTF8()};
			if (result.empty())
				continue;

			shapeName = std::move(result);
		}

		std::vector<std::string> newSliders;
		if (!project->ImportHeadTRI(fn.ToUTF8().data(), shapeName, true, &newSliders)) {
			wxLogError("Failed to load TRI file '%s'!", fn);
			wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
			return;
		}

		RefreshGUIFromProj(false);

		for (auto& sliderName : newSliders)
			createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();

	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnExportTRIHead(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString dir = wxDirSelector(_("Export .tri morphs"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
	if (dir.IsEmpty())
		return;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::string fn = dir.ToStdString() + PathSepStr + shape->name.get() + ".tri";

		wxLogMessage("Exporting TRI (head) morphs of '%s' to '%s'...", shape->name.get(), fn);
		if (!project->WriteHeadTRI(shape, fn)) {
			wxLogError("Failed to export TRI file to '%s'!", fn);
			wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnExportShapeTRIHead(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Export .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting TRI (head) morphs to '%s'...", fn);
	if (!project->WriteHeadTRI(activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnImportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	wxString fileName = wxFileSelector(_("Import physics data to project"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_FILE_MUST_EXIST, this);
	if (fileName.IsEmpty())
		return;

	auto physicsBlock = std::make_unique<BSClothExtraData>();
	if (!physicsBlock->FromHKX(fileName.ToUTF8().data())) {
		wxLogError("Failed to import physics data file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to import physics data file '%s'!"), fileName), _("Import Error"), wxICON_ERROR);
		return;
	}

	project->AddClothData(fileName.ToUTF8().data(), std::move(physicsBlock));

	SetPendingChanges();
}

void OutfitStudioFrame::OnExportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	auto& physicsData = project->GetClothData();
	if (physicsData.empty()) {
		wxMessageBox(_("There is no physics data loaded!"), _("Info"), wxICON_INFORMATION);
		return;
	}

	wxArrayString fileNames;
	for (auto& data : physicsData)
		fileNames.Add(wxString::FromUTF8(data.first));

	wxSingleChoiceDialog physicsDataChoice(this, _("Please choose the physics data source you want to export."), _("Choose physics data"), fileNames);
	if (physicsDataChoice.ShowModal() == wxID_CANCEL)
		return;

	int sel = physicsDataChoice.GetSelection();
	std::string selString{fileNames[sel].ToUTF8()};

	if (!selString.empty()) {
		wxString fileName = wxFileSelector(_("Export physics data"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		if (!physicsData.at(selString)->ToHKX(fileName.ToUTF8().data())) {
			wxLogError("Failed to save physics data file '%s'!", fileName);
			wxMessageBox(wxString::Format(_("Failed to save physics data file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnMakeConvRef(wxCommandEvent& WXUNUSED(event)) {
	if (project->AllSlidersZero()) {
		wxMessageBox(_("This function requires at least one slider position to be non-zero."));
		return;
	}

	std::string namebase = "ConvertToBase";
	char thename[256];
	snprintf(thename, 256, "%s", namebase.c_str());
	int count = 1;
	while (sliderPanels.find(thename) != sliderPanels.end())
		snprintf(thename, 256, "%s%d", namebase.c_str(), count++);

	std::string finalName{
		GetTextFromUser(_("Create a conversion slider for the current slider settings with the following name: "), _("Create New Conversion Slider"), thename, this).ToUTF8()};
	if (finalName.empty())
		return;

	wxLogMessage("Creating new conversion slider '%s'...", finalName);

	project->AddCombinedSlider(finalName);

	auto baseShape = project->GetBaseShape();
	if (baseShape) {
		Mesh* m = glView->GetMesh(baseShape->name.get());
		if (m)
			project->UpdateShapeFromMesh(baseShape, m);

		project->NegateSlider(finalName, baseShape);
	}

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);
	for (auto& s : sliderList) {
		if (!s.compare(finalName))
			continue;
		project->DeleteSlider(s);
	}

	glView->GetUndoHistory()->ClearHistory();
	UpdateUndoTools();

	editor.activeSlider.clear();
	editor.lastActiveSlider.clear();
	editor.bEditSlider = false;
	MenuExitSliderEdit();

	CreateSetSliders();
}
