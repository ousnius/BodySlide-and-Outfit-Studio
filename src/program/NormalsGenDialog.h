/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../ui/wxNormalsGenDlg.h"
#include "../components/NormalGenLayers.h"

class NormalsGenDialog : public wxNormalsGenDlg
{
protected:
	void doShowPresetContext(wxCommandEvent& event);
	void doPropertyChanged(wxPropertyGridEvent& event);
	void doAddLayer(wxCommandEvent& event);
	void doMoveUpLayer(wxCommandEvent& event);
	void doDeleteLayer(wxCommandEvent& event);
	void OnUseBackgroundLayerCheck(wxCommandEvent& event);
	void doSetOutputFileName(wxFileDirPickerEvent& event);
	void doPreviewNormalMap(wxCommandEvent& event);
	void doGenerateNormalMap(wxCommandEvent& event);
	void doLoadPreset(wxCommandEvent& event);
	void doSavePreset(wxCommandEvent& event);

public:
	NormalsGenDialog(wxWindow* parent, std::vector<NormalGenLayer>& inLayersRef);

	void SetLayersRef(std::vector<NormalGenLayer>& inLayersRef);

protected:
	int layerCount = 1;
	std::vector<NormalGenLayer>& refNormalGenLayers;

	void doPropSelected(wxPropertyGridEvent& event);

	wxString nextLayerName();
	void addBGLayer(NormalGenLayer& newLayer);
	void addLayer(NormalGenLayer& newLayer, wxPGProperty* before = nullptr);

	void PopulateLayers();
};
