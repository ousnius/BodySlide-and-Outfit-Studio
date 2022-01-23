/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/filepicker.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/wx.h>

class wxNormalsGenDlg : public wxDialog {
protected:
	void ShowPresetContextMenu(wxMouseEvent& event) { PopupMenu(presetContext, event.GetPosition()); }

	wxBitmapButton* bpPreset = nullptr;
	wxPropertyGrid* pgLayers = nullptr;
	wxButton* btnAddLayer = nullptr;
	wxButton* btnMoveUp = nullptr;
	wxButton* btnDeleteLayer = nullptr;
	wxCheckBox* cbBackup = nullptr;
	wxCheckBox* cbCompress = nullptr;
	wxCheckBox* cbSaveToBGLayerFile = nullptr;
	wxFilePickerCtrl* fpOutputFile = nullptr;
	wxButton* btnPreview = nullptr;
	wxButton* btnGenerate = nullptr;
	wxMenu* presetContext = nullptr;

	// Virtual event handlers, override them in your derived class
	virtual void doShowPresetContext(wxCommandEvent& event) { event.Skip(); }
	virtual void doPropertyChanged(wxPropertyGridEvent& event) { event.Skip(); }
	virtual void doAddLayer(wxCommandEvent& event) { event.Skip(); }
	virtual void doMoveUpLayer(wxCommandEvent& event) { event.Skip(); }
	virtual void doDeleteLayer(wxCommandEvent& event) { event.Skip(); }
	virtual void OnUseBackgroundLayerCheck(wxCommandEvent& event) { event.Skip(); }
	virtual void doSetOutputFileName(wxFileDirPickerEvent& event) { event.Skip(); }
	virtual void doPreviewNormalMap(wxCommandEvent& event) { event.Skip(); }
	virtual void doGenerateNormalMap(wxCommandEvent& event) { event.Skip(); }
	virtual void doLoadPreset(wxCommandEvent& event) { event.Skip(); }
	virtual void doSavePreset(wxCommandEvent& event) { event.Skip(); }

	wxDECLARE_EVENT_TABLE();

public:
	wxNormalsGenDlg(wxWindow* parent);
	~wxNormalsGenDlg();
};
