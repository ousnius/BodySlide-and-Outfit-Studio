#pragma once

#include "OutfitStudio.h"
#include <wx/clrpicker.h>

using namespace std;

class ShapeProperties : public wxDialog {
public:
	ShapeProperties(wxWindow*);
	~ShapeProperties();

private:
	wxStaticText* lbName = nullptr;
	wxChoice* shaderType = nullptr;
	wxColourPickerCtrl* specularColor = nullptr;
	wxTextCtrl* specularStrength = nullptr;
	wxTextCtrl* specularPower = nullptr;
	wxButton* btnAddShader = nullptr;
	wxButton* btnRemoveShader = nullptr;
	wxButton* btnSetTextures = nullptr;
	wxButton* btnTransparency = nullptr;

	OutfitStudio* os = nullptr;
	NiShader* shader = nullptr;

	void GetShader();
	bool AddShader();
	void RemoveShader();
	void AssignDefaultTexture();

	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnApplyDiffuse(wxCommandEvent& event);
	void OnTransparency(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
