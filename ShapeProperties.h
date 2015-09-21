#pragma once

#include "OutfitStudio.h"
#include <wx/clrpicker.h>

using namespace std;

class ShapeProperties : public wxDialog {
public:
	ShapeProperties(wxWindow*, NifFile*, const string&);
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
	NifFile* nif = nullptr;
	string shape;

	void GetShader();
	void GetShaderType();
	bool AddShader();
	void RemoveShader();

	void AssignDefaultTexture();
	void ApplyChanges();

	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnApplyDiffuse(wxCommandEvent& event);
	void OnTransparency(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
