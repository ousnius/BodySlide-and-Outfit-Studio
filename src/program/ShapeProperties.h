/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitStudio.h"
#include <wx/clrpicker.h>
#include <wx/notebook.h>

class ShapeProperties : public wxDialog {
public:
	ShapeProperties(wxWindow*, nifly::NifFile*, std::vector<nifly::NiShape*>);
	~ShapeProperties();

private:
	wxNotebook* nbProperties = nullptr;

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
	wxTextCtrl* alpha = nullptr;
	wxCheckBox* vertexColors = nullptr;
	wxCheckBox* doubleSided = nullptr;
	wxButton* btnAddShader = nullptr;
	wxButton* btnRemoveShader = nullptr;
	wxButton* btnSetTextures = nullptr;

	wxTextCtrl* alphaThreshold = nullptr;
	wxCheckBox* vertexAlpha = nullptr;
	wxCheckBox* alphaTest = nullptr;
	wxCheckBox* alphaBlend = nullptr;
	wxButton* btnAddTransparency = nullptr;
	wxButton* btnRemoveTransparency = nullptr;

	wxButton* btnCopyShaderFromShape = nullptr;

	wxPanel* pgGeometry = nullptr;
	wxCheckBox* fullPrecision = nullptr;
	wxCheckBox* subIndex = nullptr;
	wxCheckBox* skinned = nullptr;
	wxCheckBox* dynamic = nullptr;

	std::vector<int> extraDataIndices;
	wxPanel* pgExtraData = nullptr;
	wxFlexGridSizer* extraDataGrid = nullptr;

	wxPanel* pgCoordinates = nullptr;
	wxTextCtrl* textScale = nullptr;
	wxTextCtrl* textX = nullptr;
	wxTextCtrl* textY = nullptr;
	wxTextCtrl* textZ = nullptr;
	wxTextCtrl* textRX = nullptr;
	wxTextCtrl* textRY = nullptr;
	wxTextCtrl* textRZ = nullptr;
	wxCheckBox* cbTransformGeo = nullptr;
	nifly::MatTransform oldTransform;
	nifly::MatTransform newTransform;

	OutfitStudioFrame* os = nullptr;
	nifly::NifFile* nif = nullptr;
	std::vector<nifly::NiShape*> shapes;

	bool confirmationAccepted = false;

	bool ShowConfirmationDialog();

	void GetShader();
	void GetShaderType();
	void AddShader(nifly::NiShape* shape);
	void RemoveShader(nifly::NiShape* shape);

	void GetTransparency();
	void AddTransparency(nifly::NiShape* shape);
	void RemoveTransparency(nifly::NiShape* shape);

	void GetGeometry();

	void GetExtraData();
	void AddExtraData(nifly::NiShape* shape, nifly::NiExtraData* extraData, bool uiOnly = false);
	void ChangeExtraDataType(nifly::NiShape* shape, int index);
	void RemoveExtraData(int index);

	void GetCoordTrans();
	void OnTransChanged(wxCommandEvent&);

	void AssignDefaultTexture(nifly::NiShape* shape);
	void RefreshMesh();
	void ApplyChanges();

	void OnChooseMaterial(wxCommandEvent& event);
	void OnAddShader(wxCommandEvent& event);
	void OnRemoveShader(wxCommandEvent& event);
	void OnSetTextures(wxCommandEvent& event);
	void OnAddTransparency(wxCommandEvent& event);
	void OnRemoveTransparency(wxCommandEvent& event);
	void OnCopyShaderFromShape(wxCommandEvent& event);
	void OnAddExtraData(wxCommandEvent& event);
	void OnChangeExtraDataType(wxCommandEvent& event);
	void OnRemoveExtraData(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};
