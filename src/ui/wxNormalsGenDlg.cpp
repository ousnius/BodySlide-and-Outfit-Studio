/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#include "wxNormalsGenDlg.h"
#include <wx/xrc/xmlres.h>

wxBEGIN_EVENT_TABLE(wxNormalsGenDlg, wxDialog)
	EVT_RIGHT_DOWN(wxNormalsGenDlg::ShowPresetContextMenu)
	EVT_BUTTON(XRCID("bpPreset"), wxNormalsGenDlg::doShowPresetContext)
	EVT_BUTTON(XRCID("btnAddLayer"), wxNormalsGenDlg::doAddLayer)
	EVT_BUTTON(XRCID("btnMoveUp"), wxNormalsGenDlg::doMoveUpLayer)
	EVT_BUTTON(XRCID("btnDeleteLayer"), wxNormalsGenDlg::doDeleteLayer)
	EVT_CHECKBOX(XRCID("cbSaveToBGLayerFile"), wxNormalsGenDlg::OnUseBackgroundLayerCheck)
	EVT_FILEPICKER_CHANGED(XRCID("fpOutputFile"), wxNormalsGenDlg::doSetOutputFileName)
	EVT_BUTTON(XRCID("btnPreview"), wxNormalsGenDlg::doPreviewNormalMap)
	EVT_BUTTON(XRCID("btnGenerate"), wxNormalsGenDlg::doGenerateNormalMap)
wxEND_EVENT_TABLE()

wxNormalsGenDlg::wxNormalsGenDlg(wxWindow* parent)
{
	wxXmlResource *xrc = wxXmlResource::Get();
	xrc->Load("res\\xrc\\NormalsGenDlg.xrc");
	xrc->LoadDialog(this, parent, "wxNormalsGenDlg");

	bpPreset = XRCCTRL(*this, "bpPreset", wxBitmapButton);
	btnAddLayer = XRCCTRL(*this, "btnAddLayer", wxButton);
	btnMoveUp = XRCCTRL(*this, "btnMoveUp", wxButton);
	btnDeleteLayer = XRCCTRL(*this, "btnDeleteLayer", wxButton);
	cbBackup = XRCCTRL(*this, "cbBackup", wxCheckBox);
	cbCompress = XRCCTRL(*this, "cbCompress", wxCheckBox);
	cbSaveToBGLayerFile = XRCCTRL(*this, "cbSaveToBGLayerFile", wxCheckBox);
	fpOutputFile = XRCCTRL(*this, "fpOutputFile", wxFilePickerCtrl);
	btnPreview = XRCCTRL(*this, "btnPreview", wxButton);
	btnGenerate = XRCCTRL(*this, "btnGenerate", wxButton);

	pgLayers = new wxPropertyGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER);
	pgLayers->SetExtraStyle(wxPG_EX_ENABLE_TLP_TRACKING | wxPG_EX_HELP_AS_TOOLTIPS);
	pgLayers->Bind(wxEVT_PG_CHANGED, &wxNormalsGenDlg::doPropertyChanged, this);
	xrc->AttachUnknownControl("pgLayers", pgLayers, this);

	presetContext = xrc->LoadMenu("presetContext");
	presetContext->Bind(wxEVT_MENU, &wxNormalsGenDlg::doLoadPreset, this, presetContext->FindItem(XRCID("ctxLoadPreset"))->GetId());
	presetContext->Bind(wxEVT_MENU, &wxNormalsGenDlg::doSavePreset, this, presetContext->FindItem(XRCID("ctxSavePreset"))->GetId());

	Fit();
	Layout();
	CenterOnParent();
}

wxNormalsGenDlg::~wxNormalsGenDlg()
{
	delete presetContext;
	wxXmlResource::Get()->Unload("res\\xrc\\NormalsGenDlg.xrc");
}
