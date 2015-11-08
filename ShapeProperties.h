/*
BodySlide and Outfit Studio
Copyright (C) 2015  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "OutfitStudio.h"
#include <wx/clrpicker.h>

using namespace std;

class ShapeProperties : public wxDialog {
public:
	ShapeProperties(wxWindow*, NifFile*, const string&);
	~ShapeProperties();

private:
	wxChoice* shaderType = nullptr;
	wxColourPickerCtrl* specularColor = nullptr;
	wxTextCtrl* specularStrength = nullptr;
	wxTextCtrl* specularPower = nullptr;
	wxColourPickerCtrl* emissiveColor = nullptr;
	wxTextCtrl* emissiveMultiple = nullptr;
	wxButton* btnAddShader = nullptr;
	wxButton* btnRemoveShader = nullptr;
	wxButton* btnSetTextures = nullptr;

	wxTextCtrl* alphaThreshold = nullptr;
	wxButton* btnAddTransparency = nullptr;
	wxButton* btnRemoveTransparency = nullptr;

	OutfitStudio* os = nullptr;
	NifFile* nif = nullptr;
	string shape;

	void GetShader();
	void GetShaderType();
	void AddShader();
	void RemoveShader();

	void GetTransparency();
	void AddTransparency();
	void RemoveTransparency();

	void AssignDefaultTexture();
	void ApplyChanges();

	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnApplyDiffuse(wxCommandEvent& event);
	void OnAddTransparency(wxCommandEvent& event);
	void OnRemoveTransparency(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};
