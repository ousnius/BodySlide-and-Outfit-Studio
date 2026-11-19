/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "DropTargets.h"
#include "OutfitStudioFrame.h"
#include "ShapeTreeItems.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>

using namespace nifly;

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner && owner->IsAnimationPlaying())
		return false;

	if (owner) {
		NiShape* mergeShape = nullptr;
		if (owner->activeItem && fileNames.GetCount() == 1)
			mergeShape = owner->activeItem->GetShape();

		for (auto& inputFile : fileNames) {
			wxString dataName = inputFile.AfterLast('/').AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			if (inputFile.Lower().EndsWith(".nif")) {
				owner->StartProgress(_("Adding NIF file..."));
				owner->UpdateProgress(1, _("Adding NIF file..."));
				owner->project->ImportNIF(inputFile.ToUTF8().data(), false);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".obj")) {
				owner->StartProgress("Adding OBJ file...");
				owner->UpdateProgress(1, _("Adding OBJ file..."));
				owner->ImportOBJWithDialog(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
#ifdef USE_FBXSDK
			else if (inputFile.Lower().EndsWith(".fbx")) {
				owner->StartProgress(_("Adding FBX file..."));
				owner->UpdateProgress(1, _("Adding FBX file..."));
				owner->ImportFBXWithDialog(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
#endif
		}

		owner->UpdateTitle();
	}
	else
		return false;

	return true;
}

bool DnDSliderFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner && owner->IsAnimationPlaying())
		return false;

	if (owner) {
		bool isMultiple = (fileNames.GetCount() > 1);
		for (size_t i = 0; i < fileNames.GetCount(); i++) {
			wxString inputFile;
			inputFile = fileNames.Item(i);

			wxString dataName = inputFile.AfterLast('/').AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			bool isBSD = inputFile.MakeLower().EndsWith(".bsd");
			bool isOBJ = inputFile.MakeLower().EndsWith(".obj");
#ifdef USE_FBXSDK
			bool isFBX = inputFile.MakeLower().EndsWith(".fbx");
			if (isBSD || isOBJ || isFBX) {
#else
			if (isBSD || isOBJ) {
#endif
				if (!owner->activeItem) {
					wxMessageBox(_("There is no shape selected!"), _("Error"));
					return false;
				}

				if (lastResult == wxDragCopy) {
					targetSlider = owner->NewSlider(dataName.ToUTF8().data(), isMultiple);
				}

				if (targetSlider.empty())
					return false;

				owner->StartProgress(_("Loading slider file..."));
				owner->UpdateProgress(1, _("Loading slider file..."));

				if (!owner->ConfirmSliderDataLocalForEdit(owner->activeItem->GetShape(), targetSlider)) {
					owner->EndProgress();
					return false;
				}

				if (isBSD)
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
#ifdef USE_FBXSDK
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
#endif
				else
					return false;

				owner->EndProgress();
			}
		}
		owner->EnterSliderEdit(targetSlider);
		targetSlider.clear();
	}
	else
		return false;

	return true;
}

wxDragResult DnDSliderFile::OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult) {
	targetSlider.clear();
	lastResult = defResult;

	if (defResult == wxDragCopy)
		return lastResult;

	if (owner) {
		for (auto& child : owner->sliderPanels) {
			if (child.second->GetRect().Contains(x, y)) {
				targetSlider = child.first;
				lastResult = wxDragMove;
				break;
			}
		}

		if (targetSlider.empty())
			if (owner->sliderScroll->HitTest(x, y) == wxHT_WINDOW_INSIDE)
				lastResult = wxDragCopy;
	}
	else
		lastResult = wxDragCancel;

	return lastResult;
}
