/*
BodySlide and Outfit Studio
Copyright (C) 2018  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "OutfitStudio.h"
#include <wx/clrpicker.h>

class ShapeProperties : public wxDialog {
public:
	ShapeProperties(wxWindow*, NifFile*, NiShape*);
	~ShapeProperties();

private:
	wxPanel* pgShader = nullptr;
	wxStaticText* lbShaderName = nullptr;
	wxTextCtrl* shaderName = nullptr;
	wxButton* btnMaterialChooser = nullptr;
	wxChoice* shaderType = nullptr;
	wxColourPickerCtrl* specularColor = nullptr;
	wxTextCtrl* specularStrength = nullptr;
	wxTextCtrl* specularPower = nullptr;
	wxColourPickerCtrl* emissiveColor = nullptr;
	wxTextCtrl* emissiveMultiple = nullptr;
	wxCheckBox* vertexColors = nullptr;
	wxButton* btnAddShader = nullptr;
	wxButton* btnRemoveShader = nullptr;
	wxButton* btnSetTextures = nullptr;

	wxTextCtrl* alphaThreshold = nullptr;
	wxCheckBox* vertexAlpha = nullptr;
	wxButton* btnAddTransparency = nullptr;
	wxButton* btnRemoveTransparency = nullptr;

	wxCheckBox* fullPrecision = nullptr;
	wxCheckBox* subIndex = nullptr;
	wxCheckBox* skinned = nullptr;

	std::vector<int> extraDataIndices;
	wxPanel* pgExtraData = nullptr;
	wxFlexGridSizer* extraDataGrid = nullptr;

	OutfitStudioFrame* os = nullptr;
	NifFile* nif = nullptr;
	NiShape* shape = nullptr;

	bool currentSubIndex = false;
	bool currentVertexColors = false;
	bool currentVertexAlpha = false;

	void GetShader();
	void GetShaderType();
	void AddShader();
	void RemoveShader();

	void GetTransparency();
	void AddTransparency();
	void RemoveTransparency();

	void GetGeometry();

	void GetExtraData();
	void AddExtraData(NiExtraData* extraData, bool uiOnly = false);
	void ChangeExtraDataType(int index);
	void RemoveExtraData(int index);

	void AssignDefaultTexture();
	void RefreshMesh();
	void ApplyChanges();

	void OnChooseMaterial(wxCommandEvent& event);
	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnAddTransparency(wxCommandEvent& event);
	void OnRemoveTransparency(wxCommandEvent& event);
	void OnAddExtraData(wxCommandEvent& event);
	void OnChangeExtraDataType(wxCommandEvent& event);
	void OnRemoveExtraData(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};
