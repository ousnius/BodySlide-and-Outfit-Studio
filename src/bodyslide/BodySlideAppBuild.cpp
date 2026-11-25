/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: building outfits, in the window and from the command line

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "PreviewWindow.h"
#include "files/GameDataStream.h"
#include "files/TriFile.h"
#include "components/ClippingFixer.h"
#include "utils/PlatformUtil.h"
#include "utils/ParallelFor.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <regex>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <wx/filename.h>
#include <wx/progdlg.h>
#include <wx/textctrl.h>
#include <wx/treelist.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;

void BodySlideApp::ApplySliders(
	const std::string& targetShape, std::vector<Slider>& sliderSet, DiffDataSets& dataSets, std::vector<Vector3>& verts, std::vector<uint16_t>& ZapIdx, std::vector<Vector2>* uvs) {
	std::vector<float> values = ToggledSliderValues(sliderSet);

	for (size_t i = 0; i < sliderSet.size(); i++) {
		auto& slider = sliderSet[i];
		float val = values[i];
		if (slider.zap && !slider.uv) {
			if (val > 0)
				for (size_t j = 0; j < slider.linkedDataSets.size(); j++)
					dataSets.GetDiffIndices(slider.linkedDataSets[j], targetShape, ZapIdx);
		}
		else {
			if (slider.invert)
				val = 1.0f - val;

			for (size_t j = 0; j < slider.linkedDataSets.size(); j++) {
				if (slider.uv) {
					if (uvs)
						dataSets.ApplyUVDiff(slider.linkedDataSets[j], targetShape, val, uvs);
				}
				else
					dataSets.ApplyDiff(slider.linkedDataSets[j], targetShape, val, &verts);
			}
		}
	}

	for (size_t i = 0; i < sliderSet.size(); i++) {
		auto& slider = sliderSet[i];
		if (slider.clamp && values[i] > 0)
			for (size_t j = 0; j < slider.linkedDataSets.size(); j++)
				dataSets.ApplyClamp(slider.linkedDataSets[j], targetShape, &verts);
	}
}

// Moves the shapes of a built NIF file out of its reference body shape. False if the body shape has no geometry.
static bool FixClippingAgainst(NifFile& nif, NiShape* refShape, const SliderSet& set, const ClippingFixOptions& fixOpts) {
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	nif.GetVertsForShape(refShape, bodyVerts);
	refShape->GetTriangles(bodyTris);

	if (bodyVerts.empty() || bodyTris.empty())
		return false;

	for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
		auto shape = nif.FindBlockByName<NiShape>(it->first);
		if (shape == refShape || !ClippingFixer::IsEligibleForFix(nif, shape))
			continue;

		std::vector<Vector3> outfitVerts;
		std::vector<Triangle> outfitTris;
		if (!nif.GetVertsForShape(shape, outfitVerts))
			continue;
		shape->GetTriangles(outfitTris);

		ClippingFixer::FixClipping(bodyVerts, bodyTris, outfitVerts, outfitTris, fixOpts);
		nif.SetVertsForShape(shape, outfitVerts);
	}

	return true;
}

void BodySlideApp::FixBuildClipping(NifFile& nifBig, NifFile* nifSmall, const SliderSet& set) {
	if (clippingFixStrength <= 0.0f)
		return;

	auto refShape = ClippingFixer::FindReferenceShape(nifBig);
	if (!refShape)
		return;

	ClippingFixOptions fixOpts;
	fixOpts.strength = clippingFixStrength / 100.0f;

	if (!FixClippingAgainst(nifBig, refShape, set, fixOpts) || !nifSmall)
		return;

	// The small/low weight NIF is fixed against the same body shape
	auto refShapeSmall = nifSmall->FindBlockByName<NiShape>(refShape->name.get());
	if (refShapeSmall)
		FixClippingAgainst(*nifSmall, refShapeSmall, set, fixOpts);
}

void BodySlideApp::FinishBuiltShapes(NifFile& nifBig,
									 NifFile* nifSmall,
									 const SliderSet& set,
									 const std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices,
									 bool forceNormals) {
	const bool keepZappedShapes = set.KeepZappedShapes();
	const std::vector<uint16_t> noZaps;
	std::vector<Vector3> verts;

	// False if the shape doesn't exist or has no vertices
	auto finishShape = [&](NifFile& nif, NiShape* shape, const SliderSetShape& shapeData, const std::vector<uint16_t>& zaps) {
		if (!shape)
			return false;

		if (!nif.GetVertsForShape(shape, verts))
			return false;

		if (!shapeData.lockNormals) {
			nif.CalcNormalsForShape(shape, forceNormals, shapeData.smoothSeamNormals);

			if (forceNormals)
				ApplyReferenceNormals(nif);
		}

		nif.CalcTangentsForShape(shape);

		if (keepZappedShapes && zaps.size() == verts.size()) {
			shape->flags |= 1; // Set hidden flag when shape would otherwise be fully zapped
		}
		else {
			if (nif.DeleteVertsForShape(shape, zaps))
				nif.DeleteShape(shape); // Delete fully zapped shape
		}
		return true;
	};

	for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
		auto zapIt = zapIndices.find(it->first);
		auto& shapeZaps = zapIt != zapIndices.end() ? zapIt->second : noZaps;

		if (!finishShape(nifBig, nifBig.FindBlockByName<NiShape>(it->first), it->second, shapeZaps))
			continue;

		if (nifSmall)
			finishShape(*nifSmall, nifSmall->FindBlockByName<NiShape>(it->first), it->second, shapeZaps);
	}

	// BodySlide doesn't write .mesh files, so Starfield geometry goes inside the NIFs, with the meshlets
	// of shapes that need them (zapped or refined triangles)
	GameDataStream::PrepareGeometryForSave(nifBig, true);
	if (nifSmall)
		GameDataStream::PrepareGeometryForSave(*nifSmall, true);
}

bool BodySlideApp::WriteBuildMorphs(SliderSet& set,
									NifFile& nifBig,
									NifFile* nifSmall,
									const std::string& outFileName,
									const std::string& dataPath,
									bool tri,
									std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices) {
	bool triKeep = set.PreventMorphFile();

	if (targetGame == SF) {
		/* Write Starfield morph.dat file */
		std::string sfMorphPath = set.GetSFMorphPath();
		std::string sfMorphTargetShape = set.GetSFMorphTargetShape();
		if (tri && !triKeep && !sfMorphPath.empty() && !sfMorphTargetShape.empty()) {
			std::string morphFolder = dataPath + sfMorphPath;
			WriteSFMorphFile(morphFolder, set, nifBig, zapIndices);
		}
		return true;
	}

	if (tri && !triKeep) {
		std::string triFilePath = outFileName + ".tri";

		// TRI file already exists but isn't a body TRI file, don't overwrite!
		if (wxFileName::FileExists(wxString::FromUTF8(triFilePath)) && !IsBodyTriFile(triFilePath))
			triKeep = true;
	}

	if (!tri || triKeep) {
		if (!triKeep) {
			wxString triPath = wxString::FromUTF8(outFileName + ".tri");
			if (IsBodyTriFile(triPath.ToUTF8().data()))
				wxRemoveFile(triPath);
		}
		return true;
	}

	/* Add TRI path for in-game morphs */
	std::string triPath = set.GetOutputFilePath() + ".tri";
	std::string triPathTrimmed = triPath;
	// Replace multiple backslashes or forward slashes with one backslash
	triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex("/+|\\\\+"), "\\");

	// Remove everything before and including the meshes path
	triPathTrimmed = std::regex_replace(triPathTrimmed, std::regex(".*meshes\\\\", std::regex_constants::icase), "");

	bool written = WriteMorphTRI(outFileName, set, nifBig, zapIndices);

	bool triToRoot = targetGame == FO4 || targetGame == FO4VR || targetGame == FO76;

	SetTriData(nifBig, triPathTrimmed, triToRoot);
	if (nifSmall)
		SetTriData(*nifSmall, triPathTrimmed, triToRoot);

	// Set all shapes to dynamic/mutable
	for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
		nifBig.SetShapeDynamic(it->first);
		if (nifSmall)
			nifSmall->SetShapeDynamic(it->first);
	}

	// RaceMenu recalculates normals after applying morphs unless they're locked
	if (targetGame == SKYRIMSE || targetGame == SKYRIMVR) {
		SetLockedNormalsData(nifBig, set);
		if (nifSmall)
			SetLockedNormalsData(*nifSmall, set);
	}

	return written;
}

bool BodySlideApp::ChooseBuildOutputs(const std::vector<std::string>& outFileList,
									  std::vector<wxArrayString>& choicesList,
									  BuildSelection& buildSelection,
									  const std::string& datapath,
									  std::vector<std::string>& outfitList) {
	wxXmlResource* rsrc = wxXmlResource::Get();
	wxDialog* dlgBuildOverride = rsrc->LoadDialog(sliderView, "dlgBuildOverride");
	dlgBuildOverride->SetSize(dlgBuildOverride->FromDIP(wxSize(800, 400)));
	dlgBuildOverride->SetSizeHints(dlgBuildOverride->FromDIP(wxSize(400, 400)), dlgBuildOverride->FromDIP(wxSize(-1, -1)));
	dlgBuildOverride->CenterOnParent();

	wxScrolledWindow* scrollOverrides = XRCCTRL(*dlgBuildOverride, "scrollOverrides", wxScrolledWindow);
	wxBoxSizer* choicesSizer = (wxBoxSizer*)scrollOverrides->GetSizer();

	// Create the treelist with checkbox support
	auto treeListCtrl = new wxTreeListCtrl(scrollOverrides, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE | wxTL_CHECKBOX | wxTL_3STATE);
	treeListCtrl->AppendColumn(_("Choice"), wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT);
	treeListCtrl->AppendColumn(_("Source File"), wxCOL_WIDTH_AUTOSIZE, wxALIGN_LEFT);

	// Add root items directly under the (hidden) root
	for (size_t i = 0; i < choicesList.size(); i++) {
		auto& outFile = outFileList[i];
		auto& choices = choicesList[i];
		bool hasDefaultSet = false;

		wxTreeListItem rootItem = treeListCtrl->AppendItem(treeListCtrl->GetRootItem(), wxString::FromUTF8(outFile));
		treeListCtrl->CheckItem(rootItem, wxCheckBoxState::wxCHK_UNDETERMINED);

		// Add children with checkboxes
		for (size_t j = 0; j < choices.size(); j++) {
			wxString choice = choices[j];
			wxString defaultSet;

			// Check previous choices to see if radio button should be checked by default
			std::string outputChoice = GetDefaultOutputChoice(buildSelection, outFile, datapath);
			if (!outputChoice.empty()) {
				wxString c = wxString::FromUTF8(outputChoice);
				if (choices.Index(c) != wxNOT_FOUND) {
					defaultSet = c;
					hasDefaultSet = true;
				}
			}

			if (!hasDefaultSet && j == 0)
				defaultSet = choice;

			wxTreeListItem child = treeListCtrl->AppendItem(rootItem, choice);

			auto outfitSrc = outfitNameSource.find(choice.ToUTF8().data());
			if (outfitSrc != outfitNameSource.end()) {
				wxFileName outfitFileName(wxString::FromUTF8(outfitSrc->second));
				treeListCtrl->SetItemText(child, 1, outfitFileName.GetFullName());
			}
			else
				treeListCtrl->SetItemText(child, 1, _("<no source>"));

			if (!defaultSet.IsEmpty() && choice == defaultSet)
				treeListCtrl->CheckItem(child, wxCheckBoxState::wxCHK_CHECKED);
			else
				treeListCtrl->CheckItem(child, wxCheckBoxState::wxCHK_UNCHECKED);
		}

		treeListCtrl->Expand(rootItem);
	}

	bool checkBoxReverting = false;
	auto handler = [&](wxTreeListEvent& e) {
		if (checkBoxReverting) {
			e.Skip();
			return;
		}

		const wxTreeListItem item = e.GetItem();
		const wxTreeListItem parent = treeListCtrl->GetItemParent(item);

		// Level 1 items are direct children of the (hidden) root: make them non-checkable.
		if (parent == treeListCtrl->GetRootItem()) {
			checkBoxReverting = true;
			treeListCtrl->CheckItem(item, wxCheckBoxState::wxCHK_UNDETERMINED);
			checkBoxReverting = false;
			return;
		}

		// Only enforce the 'radio per level' rule for level-2 items:
		checkBoxReverting = true;

		auto checkedState = treeListCtrl->GetCheckedState(item);
		if (checkedState == wxCheckBoxState::wxCHK_CHECKED) {
			// Uncheck all siblings
			for (wxTreeListItem sib = treeListCtrl->GetFirstChild(parent); sib.IsOk(); sib = treeListCtrl->GetNextSibling(sib)) {
				if (sib != item && treeListCtrl->GetCheckedState(sib) == wxCheckBoxState::wxCHK_CHECKED)
					treeListCtrl->CheckItem(sib, wxCheckBoxState::wxCHK_UNCHECKED);
			}
		}
		else if (checkedState == wxCheckBoxState::wxCHK_UNCHECKED) {
			// Ensure at least one remains checked in this level
			bool anyChecked = false;
			for (wxTreeListItem sib = treeListCtrl->GetFirstChild(parent); sib.IsOk(); sib = treeListCtrl->GetNextSibling(sib)) {
				if (treeListCtrl->GetCheckedState(sib) == wxCheckBoxState::wxCHK_CHECKED) {
					anyChecked = true;
					break;
				}
			}

			if (!anyChecked) {
				// Re-check the one user tried to uncheck
				treeListCtrl->CheckItem(item, wxCheckBoxState::wxCHK_CHECKED);
			}
		}

		checkBoxReverting = false;
		e.Skip();
	};

	treeListCtrl->Bind(wxEVT_TREELIST_ITEM_CHECKED, handler);

	wxTextCtrl* chooseText = XRCCTRL(*dlgBuildOverride, "chooseText", wxTextCtrl);
	if (chooseText) {
		chooseText->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString text = chooseText->GetValue().MakeLower();
			if (!text.IsEmpty()) {
				wxTreeListItem root = treeListCtrl->GetRootItem();

				for (wxTreeListItem level1 = treeListCtrl->GetFirstChild(root); level1.IsOk(); level1 = treeListCtrl->GetNextSibling(level1)) {
					wxTreeListItem firstMatch;

					// Find first matching item
					for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
						wxString label = treeListCtrl->GetItemText(level2).Lower();

						if (label.Contains(text)) {
							firstMatch = level2;
							break;
						}
					}

					if (firstMatch.IsOk()) {
						// Check the matched item
						treeListCtrl->CheckItem(firstMatch, wxCheckBoxState::wxCHK_CHECKED);

						// Uncheck all siblings except the matched one
						for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
							if (level2 != firstMatch)
								treeListCtrl->CheckItem(level2, wxCheckBoxState::wxCHK_UNCHECKED);
						}
					}
					// else: no match found, do nothing / keep existing checks
				}
			}
		});
	}

	choicesSizer->Add(treeListCtrl, 1, wxEXPAND, 0);
	scrollOverrides->FitInside();

	if (ShowBuildOverrideWithPreview(dlgBuildOverride, treeListCtrl) == wxID_CANCEL) {
		delete dlgBuildOverride;
		return false;
	}

	wxTreeListItem root = treeListCtrl->GetRootItem();

	// Iterate level 1 roots (should correspond to choicesList size)
	size_t index = 0;
	for (wxTreeListItem level1 = treeListCtrl->GetFirstChild(root); level1.IsOk() && index < choicesList.size(); level1 = treeListCtrl->GetNextSibling(level1), ++index) {
		wxString checkedItemText;

		// Find the checked child (level 2)
		for (wxTreeListItem level2 = treeListCtrl->GetFirstChild(level1); level2.IsOk(); level2 = treeListCtrl->GetNextSibling(level2)) {
			if (treeListCtrl->GetCheckedState(level2) == wxCheckBoxState::wxCHK_CHECKED) {
				checkedItemText = treeListCtrl->GetItemText(level2);
				break; // assuming only one checked per level 1
			}
		}

		if (!checkedItemText.IsEmpty()) {
			wxString choiceSel = checkedItemText;

			// Add output choice to file
			buildSelection.SetOutputChoice(outFileList[index], choiceSel.ToUTF8().data());

			// Remove the selected choice from choicesList[i]
			choicesList[index].Remove(choiceSel);

			// Remove from outfitList all outfits in choicesList[index]
			for (auto& outfit : choicesList[index]) {
				auto result = std::find(outfitList.begin(), outfitList.end(), outfit.ToStdString());
				if (result != outfitList.end())
					outfitList.erase(result);
			}
		}
	}

	delete dlgBuildOverride;
	return true;
}

int BodySlideApp::BuildBodies(bool localPath, bool clean, bool tri, bool forceNormals) {
	if (projects.empty())
		return 1;

	auto& activeSet = GetActiveSet();
	std::string inputFileName = activeSet.GetInputFileName();
	NifFile nifSmall;
	NifFile nifBig;

	std::string outFileNameSmall;
	std::string outFileNameBig;

	wxLogMessage("Building set '%s' with options: Local Path = %s, Cleaning = %s, TRI = %s, GenWeights = %s",
				 activeSet.GetName(),
				 localPath ? "True" : "False",
				 clean ? "True" : "False",
				 tri ? "True" : "False",
				 activeSet.GenWeights() ? "True" : "False");

	if (localPath) {
		outFileNameSmall = outFileNameBig = activeSet.GetOutputFile();
	}
	else {
		if (GetOutputDataPath().empty()) {
			if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Would you like to show BodySlide where it is?"),
									   _("Game not found"),
									   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogMessage("Aborted build without data path.");
					return 4;
				}
			}

			wxString response = wxDirSelector(_("Please choose a directory to set as your Data path"), wxGetCwd());
			if (response.IsEmpty()) {
				wxLogMessage("Aborted build without data path.");
				return 4;
			}

			response.Append(PathSepChar);
			Config.SetValue("GameDataPath", response.ToUTF8().data());
		}

		outFileNameSmall = GetOutputDataPath() + activeSet.GetOutputFilePath();
		outFileNameBig = outFileNameSmall;
		wxString path = wxString::FromUTF8(GetOutputDataPath() + activeSet.GetOutputPath());
		wxFileName::Mkdir(path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	// ALT key
	if (clean && !localPath) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folder, potentially causing crashes.\n\nDo you want to continue?"),
							   _("Clean Build"),
							   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning build.");
			return 4;
		}

		wxString removeHigh, removeLow;
		wxString msg = _("Removed the following files:\n");
		bool genWeights = activeSet.GenWeights();

		if (genWeights)
			removeHigh = outFileNameSmall + "_1.nif";
		else
			removeHigh = outFileNameSmall + ".nif";

		bool remHigh = wxRemoveFile(wxString::FromUTF8(removeHigh));
		if (remHigh)
			msg.Append(removeHigh + "\n");
		else
			msg.Append(removeHigh + _(" (no action)\n"));

		std::vector<BuildRecord> noBuilds;
		RecordBuilds(noBuilds, {activeSet.GetOutputFilePath()}, GetOutputDataPath());

		if (!genWeights) {
			wxLogMessage("%s", msg);
			wxMessageBox(msg, _("Process Successful"));
			return 0;
		}

		removeLow = outFileNameSmall + "_0.nif";
		bool remLow = wxRemoveFile(wxString::FromUTF8(removeLow));
		if (remLow)
			msg.Append(removeLow + "\n");
		else
			msg.Append(removeLow + _(" (no action)\n"));

		wxLogMessage("%s", msg);
		wxMessageBox(msg, _("Process Successful"));
		return 0;
	}

	refNormalsCache.clear();

	// Also loads the external geometry of Starfield files, like the preview
	if (!LoadNifFile(inputFileName, nifBig)) {
		wxLogError("Failed to load '%s'!", inputFileName);
		return 1;
	}

	LogBaseProjectStatus(activeSet, &nifBig);
	LogProjectAddOns(activeSet, activeAddOns, &nifBig);

	if (activeSet.GenWeights())
		nifSmall.CopyFrom(nifBig);

	auto& dataSets = GetActiveDataSets();
	dataSets.Clear();
	activeSet.LoadSetDiffData(dataSets);

	std::vector<Vector3> vertsLow;
	std::vector<Vector3> vertsHigh;
	std::vector<Vector2> uvsLow;
	std::vector<Vector2> uvsHigh;
	std::vector<uint16_t> zapIdx;
	std::unordered_map<std::string, std::vector<uint16_t>> zapIdxAll;

	// Phase 1: Apply sliders and set vertices for all shapes
	for (auto it = activeSet.ShapesBegin(); it != activeSet.ShapesEnd(); ++it) {
		auto shape = nifBig.FindBlockByName<NiShape>(it->first);
		if (!nifBig.GetVertsForShape(shape, vertsHigh))
			continue;

		nifBig.GetUvsForShape(shape, uvsHigh);

		if (activeSet.GenWeights()) {
			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
				continue;

			nifSmall.GetUvsForShape(shapeSmall, uvsLow);
		}

		zapIdxAll.emplace(it->first, std::vector<uint16_t>());

		ApplySliders(it->second.targetShape, sliderManager.slidersBig, dataSets, vertsHigh, zapIdx, &uvsHigh);
		nifBig.SetVertsForShape(shape, vertsHigh);
		nifBig.SetUvsForShape(shape, uvsHigh);

		if (activeSet.GenWeights()) {
			zapIdx.clear();
			ApplySliders(it->second.targetShape, sliderManager.slidersSmall, dataSets, vertsLow, zapIdx, &uvsLow);

			auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
			nifSmall.SetVertsForShape(shapeSmall, vertsLow);
			nifSmall.SetUvsForShape(shapeSmall, uvsLow);
		}

		zapIdxAll[it->first] = zapIdx;
		zapIdx.clear();
	}

	NifFile* builtSmall = activeSet.GenWeights() ? &nifSmall : nullptr;
	FixBuildClipping(nifBig, builtSmall, activeSet);
	FinishBuiltShapes(nifBig, builtSmall, activeSet, zapIdxAll, forceNormals);

	if (!WriteBuildMorphs(activeSet, nifBig, builtSmall, outFileNameBig, GetOutputDataPath(), tri, zapIdxAll)) {
		std::string triPath = activeSet.GetOutputFilePath() + ".tri";
		wxLogError("Failed to write TRI file to '%s'!", triPath);
		wxMessageBox(wxString().Format(_("Failed to write TRI file to the following location\n\n%s"), triPath), _("Unable to process"), wxOK | wxICON_ERROR);
	}

	wxString savedLow;
	wxString savedHigh;
	wxString custName;
	bool useCustName = false;

	NifSaveOptions nifOptions;
	nifOptions.optimize = false;

	if (activeSet.GenWeights()) {
		outFileNameSmall += "_0.nif";
		outFileNameBig += "_1.nif";
		custName = wxString::FromUTF8(outFileNameSmall);
		savedLow = custName;

		std::fstream fileSmall;
		PlatformUtil::OpenFileStream(fileSmall, custName.ToUTF8().data(), std::ios::out | std::ios::binary);

		while (nifSmall.Save(fileSmall, nifOptions)) {
			wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
			wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

			custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
			if (custName.IsEmpty()) {
				wxLogMessage("Aborted build when choosing alternate file name.");
				return 4;
			}

			useCustName = true;
			savedLow = custName;

			PlatformUtil::OpenFileStream(fileSmall, custName.ToUTF8().data(), std::ios::out | std::ios::binary);
		}

		// Saving writes the real block sizes into the header last, which only reaches the file once it's closed
		fileSmall.close();

		wxString custEnd;
		if (custName.EndsWith("_0.nif", &custEnd))
			custName = custEnd + "_1.nif";
		else
			custName.Empty();
	}
	else {
		outFileNameBig += ".nif";
		custName = wxString::FromUTF8(outFileNameBig);
	}

	if (!useCustName)
		outFileNameBig = custName.ToUTF8();

	savedHigh = custName;

	std::fstream fileBig;
	PlatformUtil::OpenFileStream(fileBig, custName.ToUTF8().data(), std::ios::out | std::ios::binary);

	while (nifBig.Save(fileBig, nifOptions)) {
		wxLogError("Failed to build set to '%s'! Asking for new location.", custName);
		wxMessageBox(wxString().Format(_("Failed to build set to the following location\n\n%s"), custName), _("Unable to process"), wxOK | wxICON_ERROR);

		custName = wxSaveFileSelector(_("Choose alternate file name"), "*.nif", custName);
		if (custName.IsEmpty()) {
			wxLogMessage("Aborted build when choosing alternate file name.");
			return 4;
		}

		useCustName = true;
		savedHigh = custName;

		PlatformUtil::OpenFileStream(fileBig, custName.ToUTF8().data(), std::ios::out | std::ios::binary);
	}

	// The file is complete before the message below waits for the user
	fileBig.close();

	wxString msg = _("Successfully processed the following files:\n");
	if (!savedLow.IsEmpty())
		msg.Append(savedLow += "\n");

	if (!savedHigh.IsEmpty())
		msg.Append(savedHigh);

	// Builds to the BodySlide folder or to a file chosen after a failure aren't in the output folder
	if (!localPath && !useCustName) {
		std::vector<BuildRecord> built{ActiveSetBuildRecord()};
		RecordBuilds(built, {}, GetOutputDataPath());
	}

	wxLogMessage("%s", msg);
	wxMessageBox(msg, _("Process Successful"));
	return 0;
}

int BodySlideApp::ShowBuildOverrideWithPreview(wxDialog* dlg, wxTreeListCtrl* treeListCtrl) {
	// Save the main app state so the conflicts preview doesn't corrupt it.
	// LoadProjects() -> CleanupPreview() -> AddProjectSliders() all operate on
	// shared app members (projects, sliderManager, multiProjectMode).
	// The preview skinning describes the main projects, so it can't outlive the swap.
	previewAnimPlaying = false;
	ReleasePreviewSkinning();

	PreviewPanel* savedPreview = preview;
	PreviewWindow* savedPreviewWindow = previewWindow;
	auto savedProjects = std::move(projects);
	bool savedMultiProjectMode = multiProjectMode;
	SliderManager savedSliderManager;
	std::swap(sliderManager, savedSliderManager);
	multiProjectMode = false;
	preview = nullptr;
	previewWindow = nullptr;

	PreviewWindow* conflictsPreviewWnd = nullptr;

	auto closeConflictsPreview = [&]() {
		if (!conflictsPreviewWnd)
			return;
		if (previewWindow == conflictsPreviewWnd)
			CleanupPreview();
		auto* wnd = conflictsPreviewWnd;
		conflictsPreviewWnd = nullptr;
		previewWindow = nullptr;
		preview = nullptr;
		wnd->Destroy();
	};

	wxButton* btnPreviewConflicts = XRCCTRL(*dlg, "btnPreviewConflicts", wxButton);
	if (btnPreviewConflicts) {
		btnPreviewConflicts->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxTreeListItem sel = treeListCtrl->GetSelection();
			if (!sel.IsOk())
				return;

			// Walk up to the group root (level 1) if a child is selected
			wxTreeListItem groupItem = sel;
			wxTreeListItem parent = treeListCtrl->GetItemParent(sel);
			if (parent.IsOk() && parent != treeListCtrl->GetRootItem())
				groupItem = parent;

			// Build preview entries from all children in this group
			std::vector<PreviewProjectEntry> entries;
			for (wxTreeListItem child = treeListCtrl->GetFirstChild(groupItem); child.IsOk(); child = treeListCtrl->GetNextSibling(child)) {
				std::string outfitName = treeListCtrl->GetItemText(child).ToUTF8().data();
				auto src = outfitNameSource.find(outfitName);
				if (src != outfitNameSource.end())
					entries.push_back({src->second, outfitName});
			}

			if (entries.empty())
				return;

			closeConflictsPreview();

			// Open a new standalone preview window
			wxSize previewSize = dlg->FromDIP(wxSize(800, 600));
			conflictsPreviewWnd = new PreviewWindow(wxDefaultPosition, previewSize, this);
			previewWindow = conflictsPreviewWnd;
			preview = conflictsPreviewWnd->GetPanel();

			// Handle user closing the preview window via X button.
			// Must cancel the async load thread before the panel is destroyed,
			// otherwise the CallAfter callback could target the wrong panel.
			conflictsPreviewWnd->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent&) {
				closeConflictsPreview();
			});

			wxString title = wxString::Format(_("Preview - %s"), wxString::FromUTF8(treeListCtrl->GetItemText(groupItem)));
			conflictsPreviewWnd->SetTitle(title);

			std::string baseGamePath = Config["GameDataPath"];
			preview->SetBaseDataPath(baseGamePath);
			preview->SetReadOnlyMode(true);
			preview->SetProjectData(entries, false, BodySlideConfig["SelectedPreset"]);
		});
	}

	int result = dlg->ShowModal();

	closeConflictsPreview();

	// Restore main app state
	previewWindow = savedPreviewWindow;
	preview = savedPreview;
	projects = std::move(savedProjects);
	multiProjectMode = savedMultiProjectMode;
	std::swap(sliderManager, savedSliderManager);

	// The pose may have been changed from the conflicts preview
	if (preview) {
		preview->RefreshPoseControls();
		UpdatePreview();
	}

	return result;
}

int BodySlideApp::BuildListBodies(
	std::vector<std::string>& outfitList, std::map<std::string, std::string>& failedOutfits, bool clean, bool tri, bool forceNormals, const std::string& custPath) {
	std::string datapath = custPath;

	wxLogMessage("Started batch build with options: Custom Path = %s, Cleaning = %s, TRI = %s",
				 custPath.empty() ? "False" : custPath,
				 clean ? "True" : "False",
				 tri ? "True" : "False");

	if (clean) {
		int ret = wxMessageBox(_("WARNING: This will delete the output files from the output folders, potentially causing crashes.\n\nDo you want to continue?"),
							   _("Clean Batch Build"),
							   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
		if (ret != wxYES) {
			wxLogMessage("Aborted cleaning batch build.");
			return 1;
		}
	}

	std::string activePreset = BodySlideConfig["SelectedPreset"];

	if (datapath.empty()) {
		if (GetOutputDataPath().empty()) {
			if (clean) {
				wxLogError("Aborted batch clean with unconfigured data path. Files can't be removed that way.");
				wxMessageBox(_("WARNING: Game data path not configured. Files can't be removed that way."), _("Game not found"), wxOK | wxICON_ERROR);
				return 1;
			}
			if (cmdQuiet) {
				wxLogWarning("Game data path not configured. Saving files to the working directory '%s'.", Config["AppDir"]);
			}
			else if (Config["WarnMissingGamePath"] == "true") {
				int ret = wxMessageBox(_("WARNING: Game data path not configured. Continue saving files to the working directory?"),
									   _("Game not found"),
									   wxYES_NO | wxCANCEL | wxICON_EXCLAMATION);
				if (ret != wxYES) {
					wxLogError("Aborted batch build with unconfigured data path.");
					return 1;
				}
			}

			Config.SetValue("GameDataPath", Config["AppDir"] + PathSepStr);
		}

		datapath = GetOutputDataPath();
	}

	std::vector<std::string> outFileList;
	std::vector<wxArrayString> choicesList;
	std::vector<std::pair<std::string, std::string>> implicitChoices;
	for (auto& outFile : outFileCount) {
		if (outFile.second.size() > 1) {
			wxArrayString selOutfits;
			for (auto& outfit : outFile.second) {
				// Only if it's going to be batch built
				if (std::find(outfitList.begin(), outfitList.end(), outfit) != outfitList.end())
					selOutfits.Add(wxString::FromUTF8(outfit));
			}

			// Same file would not be written more than once, but the only selected outfit becomes the output choice
			if (selOutfits.size() <= 1) {
				if (selOutfits.size() == 1 && !clean)
					implicitChoices.emplace_back(outFile.first, selOutfits[0].ToUTF8().data());
				continue;
			}

			outFileList.push_back(outFile.first);
			choicesList.push_back(selOutfits);
		}
	}

	// Load BuildSelection file or create new one
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	if (!choicesList.empty() || !implicitChoices.empty())
		GetBuildSelection(buildSelFile, buildSelection);

	for (auto& implicitChoice : implicitChoices)
		buildSelection.SetOutputChoice(implicitChoice.first, implicitChoice.second);

	// Quiet builds take the saved choice or the first one instead of asking
	if (!choicesList.empty() && cmdQuiet) {
		for (size_t i = 0; i < choicesList.size(); i++) {
			std::string choice = GetDefaultOutputChoice(buildSelection, outFileList[i], datapath);
			if (choice.empty() || choicesList[i].Index(wxString::FromUTF8(choice)) == wxNOT_FOUND)
				choice = choicesList[i][0].ToUTF8().data();
			wxLogWarning("Outfits '%s' all write '%s', building '%s'.", wxJoin(choicesList[i], ','), outFileList[i], choice);

			for (auto& outfit : choicesList[i]) {
				if (outfit.ToUTF8().data() == choice)
					continue;
				auto result = std::find(outfitList.begin(), outfitList.end(), outfit.ToUTF8().data());
				if (result != outfitList.end())
					outfitList.erase(result);
			}
		}
		choicesList.clear();
	}

	if (!choicesList.empty() && !ChooseBuildOutputs(outFileList, choicesList, buildSelection, datapath, outfitList)) {
		wxLogMessage("Aborted batch build by not choosing a file override.");
		return 1;
	}

	if ((!choicesList.empty() || !implicitChoices.empty()) && !cmdQuiet) {
		// Save output choices to file
		buildSelFile.UpdateOutputChoices(buildSelection);
		buildSelFile.Save();
	}

	refNormalsCache.clear();

	wxProgressDialog progWnd(_("Processing Outfits"), _("Starting..."), 1000, sliderView, wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
	progWnd.SetSize(400, 150);
	float progstep = 1000.0f / outfitList.size();
	std::atomic<int> count = 0;
	std::unordered_map<std::string, std::string> failedOutfitsCon;
	std::mutex batchBuildMutex;
	std::mutex outputDirectoryMutex;

	// Only builds to the output folder are recorded, not ones to a custom folder
	bool recordBuilds = custPath.empty() || BuildRecord::SameFolder(BuildRecord::NormalizeFolder(custPath), BuildRecord::NormalizeFolder(GetOutputDataPath()));
	std::vector<BuildRecord> builtRecords;
	std::vector<std::string> removedOutputs;

	auto recordFailure = [&](const std::string& outfit, const auto& message) {
		std::lock_guard<std::mutex> lock(batchBuildMutex);
		failedOutfitsCon[outfit] = message;
	};

	auto recordBuilt = [&](BuildRecord&& record) {
		std::lock_guard<std::mutex> lock(batchBuildMutex);
		builtRecords.push_back(std::move(record));
	};

	auto recordRemoved = [&](const std::string& outputPath) {
		std::lock_guard<std::mutex> lock(batchBuildMutex);
		removedOutputs.push_back(outputPath);
	};

	auto outputDirectoryExists = [](const wxString& dir) {
		wxFileName dirName;
		dirName.AssignDir(dir);
		return dirName.DirExists();
	};

	auto ensureOutputDirectory = [&](const wxString& dir) {
		if (outputDirectoryExists(dir))
			return true;

		std::lock_guard<std::mutex> lock(outputDirectoryMutex);
		if (outputDirectoryExists(dir))
			return true;

		if (wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
			return true;

		return outputDirectoryExists(dir);
	};

	// Conforming uses the skinning of the shapes, which needs the skeleton. It's loaded here and not on the threads.
	if (conformAddOnSliders)
		EnsurePreviewSkeleton();

	auto buildOutfit = [&](const std::string& outfit) {
		int processedCount = ++count;
		wxString progMsg = wxString::Format(_("Processing '%s' (%d of %d)..."), wxString::FromUTF8(outfit), processedCount, (int)outfitList.size());
		{
			std::lock_guard<std::mutex> lock(batchBuildMutex);
			progWnd.Update((int)(processedCount * progstep) - 1, progMsg);
			progWnd.Fit();
			wxLogMessage(progMsg);
		}

		/* Load set */
		if (outfitNameSource.find(outfit) == outfitNameSource.end()) {
			recordFailure(outfit, _("No recorded outfit name source"));
			return;
		}

		SliderSet currentSet;
		DiffDataSets currentDiffs;

		SliderSetFile sliderDoc;
		sliderDoc.Open(outfitNameSource[outfit]);
		if (!sliderDoc.fail()) {
			if (sliderDoc.GetSet(outfit, currentSet)) {
				recordFailure(outfit, _("Unable to get slider set from file: ") + outfitNameSource[outfit]);
				return;
			}
		}
		else {
			recordFailure(outfit, _("Unable to open slider set file: ") + outfitNameSource[outfit]);
			return;
		}

		currentSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

		// ALT key
		if (clean && custPath.empty()) {
			recordRemoved(currentSet.GetOutputFilePath());

			bool genWeights = currentSet.GenWeights();

			wxString removePath = wxString::FromUTF8(datapath + currentSet.GetOutputFilePath());
			wxString removeHigh = removePath + ".nif";
			if (genWeights)
				removeHigh = removePath + "_1.nif";

			if (wxFileName::FileExists(removeHigh))
				wxRemoveFile(removeHigh);

			if (!genWeights)
				return;

			wxString removeLow = removePath + "_0.nif";
			if (wxFileName::FileExists(removeLow))
				wxRemoveFile(removeLow);

			return;
		}

		/* Load input NIFs */
		NifFile nifBig;
		NifFile nifSmall;
		if (!LoadNifFile(currentSet.GetInputFileName(), nifBig)) {
			recordFailure(outfit, _("Unable to load input nif: ") + currentSet.GetInputFileName());
			return;
		}

		ProjectAddOns addOns;
		AddProjectAddOns(currentSet, conformAddOnSliders, addOns, &nifBig);

		LogBaseProjectStatus(currentSet, &nifBig);
		LogProjectAddOns(currentSet, addOns);
		LogProjectAddOns(currentSet, addOns, &nifBig);

		if (currentSet.GenWeights())
			nifSmall.CopyFrom(nifBig);

		currentSet.LoadSetDiffData(currentDiffs);

		// Load BuildSelection file for zap choices
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		/* Shape the NIF files */
		std::vector<Vector3> vertsLow;
		std::vector<Vector3> vertsHigh;
		std::vector<Vector2> uvsLow;
		std::vector<Vector2> uvsHigh;
		std::vector<int> clamps;
		std::vector<uint16_t> zapIdx;
		std::unordered_map<std::string, std::vector<uint16_t>> zapIdxAll;

		for (size_t s = 0; s < currentSet.size(); s++) {
			std::string name = currentSet[s].name;

			if (currentSet[s].bClamp) {
				clamps.push_back(s);
				continue;
			}

			if (currentSet[s].bZap && !currentSet[s].bUV) {
				float vbig = sliderManager.GetBigPresetValue(activePreset, name, currentSet[s].defBigValue / 100.0f);
				
				/*
				* Note: skip applying sliderManager values on zaps in batch mode. 
				* If a set has a zap with the same name as the set loaded in the UI,
				* its value could be overwritten by the UI value.
				* Only BuildSelection is a reliable source for zap values in batch mode.
				*/

				if (!currentSet[s].bHidden) {
					// Apply stored zap choice for zaps visible to the user
					if (buildSelection.HasZapChoice(currentSet.GetName(), name)) {
						bool zapChoice = buildSelection.GetZapChoice(currentSet.GetName(), name);
						vbig = zapChoice ? 1.0f : 0.0f;
					}
				}

				// Apply zap toggles if zap is not in default state
				if (vbig != currentSet[s].defBigValue / 100.0f) {
					for (auto& zapToggle : currentSet[s].zapToggles) {
						// Toggled zap default values are read in later code if no preset overwrites it
						auto& slider = currentSet[zapToggle];
						slider.defBigValue = 100.0f - slider.defBigValue;
						slider.defSmallValue = 100.0f - slider.defSmallValue;
					}
				}
			}
		}

		// Values of the sliders from the preset or the changed sliders of the UI, then with the toggles applied
		std::vector<Slider> batchSlidersBig;
		std::vector<Slider> batchSlidersSmall;
		batchSlidersBig.reserve(currentSet.size());
		batchSlidersSmall.reserve(currentSet.size());
		for (size_t s = 0; s < currentSet.size(); s++) {
			std::string name = currentSet[s].name;
			Slider sliderValue = Slider::FromData(currentSet[s]);
			sliderValue.linkedDataSets.clear();

			sliderValue.value = sliderManager.GetBigPresetValue(activePreset, name, currentSet[s].defBigValue / 100.0f);
			for (auto& sliderBig : sliderManager.slidersBig) {
				if (sliderBig.name == name && sliderBig.changed && !sliderBig.clamp && !currentSet[s].bZap) {
					sliderValue.value = sliderBig.value;
					break;
				}
			}
			batchSlidersBig.push_back(sliderValue);

			sliderValue.value = 0.0f;
			if (currentSet.GenWeights()) {
				sliderValue.value = sliderManager.GetSmallPresetValue(activePreset, name, currentSet[s].defSmallValue / 100.0f);
				for (auto& sliderSmall : sliderManager.slidersSmall) {
					if (sliderSmall.name == name && sliderSmall.changed && !sliderSmall.clamp && !currentSet[s].bZap) {
						sliderValue.value = sliderSmall.value;
						break;
					}
				}
			}
			batchSlidersSmall.push_back(std::move(sliderValue));
		}

		// The values the outfit is built with that differ from the preset, before toggles hold other sliders
		BuildRecord record;
		record.path = currentSet.GetOutputFilePath();
		record.set = outfit;
		for (size_t s = 0; s < currentSet.size(); s++) {
			if (currentSet[s].bClamp)
				continue;

			const std::string& name = currentSet[s].name;
			bool zap = currentSet[s].bZap;
			float vbig = batchSlidersBig[s].value;

			if (zap && !currentSet[s].bUV && !currentSet[s].bHidden && buildSelection.HasZapChoice(currentSet.GetName(), name))
				vbig = buildSelection.GetZapChoice(currentSet.GetName(), name) ? 1.0f : 0.0f;

			record.AddPresetChange(name, false, zap, vbig, sliderManager.GetBigPresetValue(activePreset, name, currentSet[s].defBigValue / 100.0f));

			if (currentSet.GenWeights() && !zap)
				record.AddPresetChange(name,
									   true,
									   false,
									   batchSlidersSmall[s].value,
									   sliderManager.GetSmallPresetValue(activePreset, name, currentSet[s].defSmallValue / 100.0f));
		}

		std::vector<int> forcedBig;
		std::vector<float> valuesBig = ToggledSliderValues(batchSlidersBig, &forcedBig);
		std::vector<float> valuesSmall = ToggledSliderValues(batchSlidersSmall);

		// Phase 1: Apply sliders and set vertices for all shapes
		for (auto it = currentSet.ShapesBegin(); it != currentSet.ShapesEnd(); ++it) {
			auto shape = nifBig.FindBlockByName<NiShape>(it->first);
			if (!nifBig.GetVertsForShape(shape, vertsHigh))
				continue;

			nifBig.GetUvsForShape(shape, uvsHigh);

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				if (!nifSmall.GetVertsForShape(shapeSmall, vertsLow))
					continue;

				nifSmall.GetUvsForShape(shapeSmall, uvsLow);
			}

			float vbig = 0.0f;
			float vsmall = 0.0f;
			zapIdxAll.emplace(it->first, std::vector<uint16_t>());

			for (size_t s = 0; s < currentSet.size(); s++) {
				std::string name = currentSet[s].name;
				std::string target = it->second.targetShape;
				std::string dn = currentSet[s].TargetDataName(target);
				if (dn.empty())
					continue;

				vbig = valuesBig[s];
				if (currentSet.GenWeights())
					vsmall = valuesSmall[s];

				if (currentSet[s].bInvert) {
					vbig = 1.0f - vbig;
					if (currentSet.GenWeights())
						vsmall = 1.0f - vsmall;
				}

				if (currentSet[s].bZap && !currentSet[s].bUV) {
					// Apply stored zap choice for zaps visible to the user, unless a ticked toggle holds the zap
					if (!currentSet[s].bHidden && forcedBig[s] < 0) {
						if (buildSelection.HasZapChoice(currentSet.GetName(), name)) {
							bool zapChoice = buildSelection.GetZapChoice(currentSet.GetName(), name);
							vbig = zapChoice ? 1.0f : 0.0f;
						}
					}

					if (vbig > 0.0f) {
						currentDiffs.GetDiffIndices(dn, target, zapIdx);
						zapIdxAll[it->first] = zapIdx;
					}
					continue;
				}

				if (currentSet[s].bUV)
					currentDiffs.ApplyUVDiff(dn, target, vbig, &uvsHigh);
				else
					currentDiffs.ApplyDiff(dn, target, vbig, &vertsHigh);

				if (currentSet.GenWeights()) {
					if (currentSet[s].bUV)
						currentDiffs.ApplyUVDiff(dn, target, vsmall, &uvsLow);
					else
						currentDiffs.ApplyDiff(dn, target, vsmall, &vertsLow);
				}
			}

			if (!clamps.empty()) {
				for (auto& c : clamps) {
					std::string dn = currentSet[c].TargetDataName(it->second.targetShape);
					std::string target = it->second.targetShape;
					if (currentSet[c].defBigValue > 0)
						currentDiffs.ApplyClamp(dn, target, &vertsHigh);

					if (currentSet.GenWeights())
						if (currentSet[c].defSmallValue > 0)
							currentDiffs.ApplyClamp(dn, target, &vertsLow);
				}
			}

			nifBig.SetVertsForShape(shape, vertsHigh);
			nifBig.SetUvsForShape(shape, uvsHigh);

			if (currentSet.GenWeights()) {
				auto shapeSmall = nifSmall.FindBlockByName<NiShape>(it->first);
				nifSmall.SetVertsForShape(shapeSmall, vertsLow);
				nifSmall.SetUvsForShape(shapeSmall, uvsLow);
			}

			zapIdx.clear();
		}

		NifFile* builtSmall = currentSet.GenWeights() ? &nifSmall : nullptr;
		FixBuildClipping(nifBig, builtSmall, currentSet);
		FinishBuiltShapes(nifBig, builtSmall, currentSet, zapIdxAll, forceNormals);

		currentDiffs.Clear();

		/* Create directory for the outfit */
		wxString dir = wxString::FromUTF8(datapath + currentSet.GetOutputPath());
		bool success = ensureOutputDirectory(dir);

		if (!success) {
			recordFailure(outfit, _("Unable to create destination directory: ") + dir.ToUTF8().data());
			return;
		}

		std::string outFileNameSmall = datapath + currentSet.GetOutputFilePath();
		std::string outFileNameBig = outFileNameSmall;

		if (!WriteBuildMorphs(currentSet, nifBig, builtSmall, outFileNameBig, datapath, tri, zapIdxAll))
			wxLogError("Failed to create TRI file to '%s'!", currentSet.GetOutputFilePath() + ".tri");

		NifSaveOptions nifOptions;
		nifOptions.optimize = false;

		/* Set filenames for the outfit */
		if (currentSet.GenWeights()) {
			outFileNameSmall += "_0.nif";
			outFileNameBig += "_1.nif";

			std::fstream fileBig;
			PlatformUtil::OpenFileStream(fileBig, outFileNameBig, std::ios::out | std::ios::binary);

			if (nifBig.Save(fileBig, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameBig);
				return;
			}

			std::fstream fileSmall;
			PlatformUtil::OpenFileStream(fileSmall, outFileNameSmall, std::ios::out | std::ios::binary);

			if (nifSmall.Save(fileSmall, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameSmall);
				return;
			}
		}
		else {
			outFileNameBig += ".nif";

			std::fstream fileBig;
			PlatformUtil::OpenFileStream(fileBig, outFileNameBig, std::ios::out | std::ios::binary);

			if (nifBig.Save(fileBig, nifOptions)) {
				recordFailure(outfit, _("Unable to save nif file: ") + outFileNameBig);
				return;
			}
		}

		recordBuilt(std::move(record));
	};

	// Multi-threading for 64-bit only due to memory limits of 32-bit builds
	if (sizeof(void*) >= 8 && outfitList.size() > 1) {
		std::atomic<bool> buildDone = false;
		std::thread buildTask([&] {
			ParallelForDynamic(outfitList.size(), 1, 1, [&](size_t startIndex, size_t endIndex) {
				for (size_t outfitIndex = startIndex; outfitIndex < endIndex; outfitIndex++)
					buildOutfit(outfitList[outfitIndex]);
			});
			buildDone = true;
		});

		while (!buildDone) {
			Yield();
			wxMilliSleep(100);
		}

		buildTask.join();
	}
	else {
		for (auto& outfit : outfitList)
			buildOutfit(outfit);
	}

	progWnd.Update(1000);

	if (recordBuilds)
		RecordBuilds(builtRecords, removedOutputs, datapath);

	failedOutfits.insert(failedOutfitsCon.begin(), failedOutfitsCon.end());

	if (failedOutfits.size() > 0)
		return 3;

	return 0;
}

std::vector<std::string> BodySlideApp::GetCmdLineBuildOutfits(std::map<std::string, std::string>& failedOutfits) {
	// Collected case insensitively, so the same outfit selected by several options is only built once.
	std::set<std::string, case_insensitive_compare> selected;

	// The groups and the filter narrow the outfit list down together, the same way the group filter
	// and the outfit filter box do in the GUI. Outfits named with the build option are added on top.
	if (!cmdGroupBuild.empty() || !cmdBuildFilter.empty()) {
		std::vector<std::string> matches = outfitNameOrder;

		if (!cmdGroupBuild.empty()) {
			matches.clear();

			for (auto& no : outfitNameOrder) {
				std::vector<std::string> groups;
				gCollection.GetOutfitGroups(no, groups);

				for (auto& g : groups) {
					if (std::find(cmdGroupBuild.begin(), cmdGroupBuild.end(), g) != cmdGroupBuild.end()) {
						matches.push_back(no);
						break;
					}
				}
			}

			wxLogMessage("Command-line build: %d outfit(s) belong to the specified group(s).", (int)matches.size());

			if (matches.empty())
				failedOutfits["--groupbuild"] = _("No outfit belongs to any of the specified groups.").ToUTF8().data();
		}

		if (!cmdBuildFilter.empty() && !matches.empty()) {
			std::string regexError;
			matches = FilterOutfitNames(matches, cmdBuildFilter, cmdBuildFilterRegex, &regexError);

			if (!regexError.empty()) {
				wxLogError("Invalid regular expression '%s' from the command line: %s", cmdBuildFilter, regexError);
				failedOutfits[cmdBuildFilter] = wxString::Format(_("Invalid regular expression: %s"), regexError).ToUTF8().data();
			}
			else {
				wxLogMessage("Command-line build: %d outfit(s) match the filter '%s'.", (int)matches.size(), cmdBuildFilter);

				if (matches.empty())
					failedOutfits[cmdBuildFilter] = _("Filter doesn't match any outfit.").ToUTF8().data();
			}
		}

		selected.insert(matches.begin(), matches.end());
	}

	for (auto& cmdOutfitName : cmdBuildOutfits) {
		// An add-on that is built with its base project builds that
		std::string outfitName = cmdOutfitName;
		std::string stackBase;
		if (projectCatalog.IsStackedAddOn(outfitName, &stackBase)) {
			wxLogMessage("Command-line build: '%s' is an add-on of '%s' and is built with it.", outfitName, stackBase);
			outfitName = stackBase;
		}

		auto sourceIt = outfitNameSource.find(outfitName);
		if (sourceIt == outfitNameSource.end()) {
			wxLogError("Outfit '%s' from the command line doesn't exist.", outfitName);
			failedOutfits[outfitName] = _("Outfit doesn't exist.").ToUTF8().data();
			continue;
		}

		// Insert the name as it's defined in the slider set, not as it was spelled on the command line.
		selected.insert(sourceIt->first);
	}

	// Built in the order the outfits were loaded in, no matter which option selected them.
	std::vector<std::string> outfits;
	for (auto& no : outfitNameOrder)
		if (selected.find(no) != selected.end())
			outfits.push_back(no);

	return outfits;
}

void BodySlideApp::CommandLineBuild() {
	std::map<std::string, std::string> failedOutfits;
	std::vector<std::string> outfits = GetCmdLineBuildOutfits(failedOutfits);

	// Loaded before the preset folder so that a preset file given on the command line wins on name conflicts.
	LoadCmdPresetFile();

	std::string preset;
	bool presetOverride = !cmdPreset.empty();
	if (presetOverride) {
		preset = BodySlideConfig["SelectedPreset"];
		BodySlideConfig.SetValue("SelectedPreset", cmdPreset);
	}

	std::vector<std::string> groups;
	sliderManager.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", "", groups, true);

	// Apply saved build selections for command-line builds before entering batch build conflict handling.
	BuildSelectionFile buildSelFile;
	BuildSelection buildSelection;
	GetBuildSelection(buildSelFile, buildSelection);

	for (auto& outFile : outFileCount) {
		if (outFile.second.size() <= 1)
			continue;

		std::vector<std::string> outfitsInBuild;
		for (auto& outfit : outFile.second) {
			if (std::find(outfits.begin(), outfits.end(), outfit) != outfits.end())
				outfitsInBuild.push_back(outfit);
		}

		if (outfitsInBuild.size() <= 1)
			continue;

		std::string outputChoice = GetDefaultOutputChoice(buildSelection, outFile.first, cmdTargetDir.empty() ? GetOutputDataPath() : cmdTargetDir);
		if (outputChoice.empty())
			continue;

		if (std::find(outfitsInBuild.begin(), outfitsInBuild.end(), outputChoice) == outfitsInBuild.end())
			continue;

		int removedChoices = 0;

		for (auto& outfit : outfitsInBuild) {
			if (outfit == outputChoice)
				continue;

			auto result = std::find(outfits.begin(), outfits.end(), outfit);
			if (result != outfits.end()) {
				outfits.erase(result);
				removedChoices++;
			}
		}

		if (removedChoices > 0) {
			wxLogMessage("Command-line build applied saved BuildSelection for output '%s': selected '%s', skipped %d conflicting choice(s).",
						 outFile.first,
						 outputChoice,
						 removedChoices);
		}
	}

	// Only with the option, so that tools calling BodySlide get the same output whatever the settings are
	bool conformAddOns = conformAddOnSliders;
	conformAddOnSliders = cmdConformAddOns;
	int ret = BuildListBodies(outfits, failedOutfits, false, cmdTri, false, cmdTargetDir);
	conformAddOnSliders = conformAddOns;

	if (presetOverride)
		BodySlideConfig.SetValue("SelectedPreset", preset);

	wxLog::FlushActive();

	if (ret == 0) {
		wxLogMessage("All command-line build sets processed successfully!");
	}
	else if (ret == 3) {
		wxArrayString errlist;
		for (auto& e : failedOutfits) {
			wxString ename = wxString::FromUTF8(e.first);
			wxLogError("Failed to build '%s': %s", ename, e.second);
			errlist.Add(ename + ":" + e.second);
		}

		if (!cmdQuiet) {
			wxSingleChoiceDialog errdisplay(sliderView, _("The following sets failed"), _("Failed"), errlist, nullptr, wxDEFAULT_DIALOG_STYLE | wxOK | wxRESIZE_BORDER);
			errdisplay.ShowModal();
		}
	}

	sliderView->Close(true);
}
