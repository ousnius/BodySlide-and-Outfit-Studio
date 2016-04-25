/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
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
	wxTextCtrl* shaderName = nullptr;
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

	wxCheckBox* fullPrecision = nullptr;
	wxCheckBox* skinned = nullptr;

	vector<int> extraDataIndices;
	wxPanel* pgExtraData = nullptr;
	wxFlexGridSizer* extraDataGrid = nullptr;

	OutfitStudio* os = nullptr;
	NifFile* nif = nullptr;
	string shape;

	string currentMaterialPath;

	void GetShader();
	void GetShaderType();
	void AddShader();
	void RemoveShader();

	void GetTransparency();
	void AddTransparency();
	void RemoveTransparency();

	void GetGeometry();

	void GetExtraData();
	void AddExtraData(const NiExtraData* extraData, bool uiOnly = false);
	void ChangeExtraDataType(int index);
	void RemoveExtraData(int index);

	void AssignDefaultTexture();
	void ApplyChanges();

	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnApplyDiffuse(wxCommandEvent& event);
	void OnAddTransparency(wxCommandEvent& event);
	void OnRemoveTransparency(wxCommandEvent& event);
	void OnAddExtraData(wxCommandEvent& event);
	void OnChangeExtraDataType(wxCommandEvent& event);
	void OnRemoveExtraData(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};
