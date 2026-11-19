/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// The BodySlide window: showing, docking and popping out the preview

#include "BodySlideFrame.h"
#include "ui/bodyslide/PreviewPanel.h"
#include "BodySlideApp.h"

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <algorithm>
#include <utility>

using namespace nifly;

void BodySlideFrame::OnPreview(wxCommandEvent& WXUNUSED(event)) {
	if (app->IsPreviewPoppedOut()) {
		app->ClosePreview();
		previewVisible = false;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", false);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
		UpdatePreviewButtonLabel();
		return;
	}

	if (previewVisible) {
		// Hide preview
		UnsplitPreview();
		previewVisible = false;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", false);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", false);
		UpdatePreviewButtonLabel();
	}
	else {
		bool restorePreviewPoppedOut = BodySlideConfig.GetBoolValue("BodySlideFrame.previewPoppedOut", false)
			|| BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false);
		if (restorePreviewPoppedOut) {
			app->PopOutPreview();
			previewVisible = true;
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
			BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", true);
			UpdatePreviewButtonLabel();

			// The preview may have been hidden while outfits changed; reload current data
			// now that the detached preview window is visible again.
			if (!OutfitIsEmpty()) {
				app->InitPreviewPanel();
				if (app->HasActiveProject())
					app->InitPreview();
			}
			return;
		}

		// Show preview
		SplitPreview();
		previewVisible = true;
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewVisible", true);
		BodySlideConfig.SetBoolValue("BodySlideFrame.previewPoppedOut", false);
		UpdatePreviewButtonLabel();

		// Trigger preview load if outfit is selected
		if (!OutfitIsEmpty()) {
			app->InitPreviewPanel();
			if (app->HasActiveProject()) {
				app->InitPreview();
			}
		}
	}
}

void BodySlideFrame::OnSashPosChanged(wxSplitterEvent& event) {
	if (!IsVisible())
		return;

	int pos = event.GetSashPosition();
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", pos);
	savedSashPosition = pos;
	savedPreviewWidth = previewOnLeft ? pos : splitter->GetSize().GetWidth() - pos;
	if (savedPreviewWidth > 0)
		BodySlideConfig.SetValue("BodySlideFrame.previewWidth", savedPreviewWidth);
}

static constexpr int MinBodySlideLeftPaneWidthDip = 850;

void BodySlideFrame::OnSashPosChanging(wxSplitterEvent& event) {
	if (!splitter || !splitter->IsSplit())
		return;

	const int minMainWidth = FromDIP(MinBodySlideLeftPaneWidthDip);
	if (previewOnLeft) {
		// The main pane is on the right, so the sash may not move too far right
		int maxSashPos = splitter->GetSize().GetWidth() - minMainWidth;
		if (maxSashPos < splitter->GetMinimumPaneSize())
			maxSashPos = splitter->GetMinimumPaneSize();

		if (event.GetSashPosition() > maxSashPos)
			event.SetSashPosition(maxSashPos);
	}
	else if (event.GetSashPosition() < minMainWidth) {
		event.SetSashPosition(minMainWidth);
	}
}

void BodySlideFrame::OnPreviewPopout(wxCommandEvent& WXUNUSED(event)) {
	if (app->IsPreviewPoppedOut()) {
		if (!BodySlideConfig.GetBoolValue("BodySlideFrame.previewAlwaysDetached", false))
			app->DockPreview(true);
		return;
	}

	app->PopOutPreview();
}

void BodySlideFrame::OnPreviewWindowClosed() {
	app->DockPreview(false, true);
}

void BodySlideFrame::UnsplitPreview() {
	if (!splitter || !splitter->IsSplit())
		return;

	savedSashPosition = splitter->GetSashPosition();
	savedPreviewWidth = previewOnLeft ? savedSashPosition : splitter->GetSize().GetWidth() - savedSashPosition;
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", savedSashPosition);
	BodySlideConfig.SetValue("BodySlideFrame.previewWidth", savedPreviewWidth);
	splitter->Unsplit(previewPanel);

	if (savedPreviewWidth > 0) {
		wxSize sz = GetSize();
		sz.SetWidth(sz.GetWidth() - savedPreviewWidth);
		SetSize(sz);
	}
}

void BodySlideFrame::SplitPreview(wxPanel* panel) {
	if (!splitter || splitter->IsSplit())
		return;

	wxPanel* panelToSplit = panel ? panel : previewPanel;
	if (!panelToSplit)
		return;

	int previewWidth = savedPreviewWidth;
	if (previewWidth <= 0)
		previewWidth = BodySlideConfig.GetIntValue("BodySlideFrame.previewWidth");
	if (previewWidth <= 0)
		previewWidth = 400;

	// With the preview on the left, the sash position is the preview width itself
	int sashPos = previewWidth;
	if (!previewOnLeft) {
		sashPos = savedSashPosition;
		if (sashPos <= 0)
			sashPos = BodySlideConfig.GetIntValue("BodySlideFrame.sashpos");
		if (sashPos <= 0)
			sashPos = GetClientSize().GetWidth();
	}

	wxSize sz = GetSize();
	sz.SetWidth(sz.GetWidth() + previewWidth);
	SetSize(sz);

	panelToSplit->Show();

	if (previewOnLeft)
		splitter->SplitVertically(panelToSplit, leftPanel, sashPos);
	else
		splitter->SplitVertically(leftPanel, panelToSplit, sashPos);

	savedSashPosition = sashPos;
	savedPreviewWidth = previewWidth;
}

void BodySlideFrame::SetPreviewOnLeft(bool onLeft) {
	if (previewOnLeft == onLeft)
		return;

	if (!splitter || !splitter->IsSplit()) {
		previewOnLeft = onLeft;
		return;
	}

	// Keep the current preview width and swap the panes in place, without resizing the frame
	const int splitterWidth = splitter->GetSize().GetWidth();
	const int sashPos = splitter->GetSashPosition();

	int previewWidth = previewOnLeft ? sashPos : splitterWidth - sashPos;
	if (previewWidth <= 0)
		previewWidth = savedPreviewWidth;
	if (previewWidth <= 0)
		previewWidth = FromDIP(400);

	wxWindow* previewPane = splitter->GetWindow1() == leftPanel ? splitter->GetWindow2() : splitter->GetWindow1();
	if (!previewPane) {
		previewOnLeft = onLeft;
		return;
	}

	previewOnLeft = onLeft;

	splitter->Unsplit(previewPane);
	previewPane->Show();

	int newSashPos = onLeft ? previewWidth : splitterWidth - previewWidth;
	if (newSashPos < splitter->GetMinimumPaneSize())
		newSashPos = splitter->GetMinimumPaneSize();

	if (onLeft)
		splitter->SplitVertically(previewPane, leftPanel, newSashPos);
	else
		splitter->SplitVertically(leftPanel, previewPane, newSashPos);

	savedSashPosition = newSashPos;
	savedPreviewWidth = previewWidth;
	BodySlideConfig.SetValue("BodySlideFrame.sashpos", newSashPos);
	BodySlideConfig.SetValue("BodySlideFrame.previewWidth", previewWidth);
}

void BodySlideFrame::UpdatePreviewButtonLabel() {
	auto btn = (wxButton*)FindWindowByName("btnPreview", this);
	if (btn)
		btn->SetLabel(previewVisible ? _("Hide Preview") : _("Show Preview"));
}

void BodySlideFrame::OnClippingStrengthChanged(wxCommandEvent& WXUNUSED(event)) {
	auto sliderClippingStrength = XRCCTRL(*this, "sliderClippingStrength", wxSlider);
	if (!sliderClippingStrength)
		return;

	app->clippingFixStrength = static_cast<float>(sliderClippingStrength->GetValue());
	app->UpdateReferenceCheckboxState();
	app->UpdatePreview();
}
