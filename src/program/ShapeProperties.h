/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitStudio.h"
#include "ShaderFlagDefs.h"
#include <wx/checklst.h>
#include <wx/clrpicker.h>
#include <wx/collpane.h>
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
	wxStaticText* lbShadingType = nullptr;
	wxChoice* shadingType = nullptr;
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

	// Shader flags
	wxCollapsiblePane* shaderFlagsPane = nullptr;
	wxCheckListBox* shaderFlags1List = nullptr;
	wxCheckListBox* shaderFlags2List = nullptr;

	// Advanced shader properties
	wxCollapsiblePane* advancedShaderPane = nullptr;
	wxTextCtrl* uvOffsetU = nullptr;
	wxTextCtrl* uvOffsetV = nullptr;
	wxTextCtrl* uvScaleU = nullptr;
	wxTextCtrl* uvScaleV = nullptr;
	wxChoice* textureClampMode = nullptr;
	wxTextCtrl* environmentMapScale = nullptr;
	wxTextCtrl* refractionStrength = nullptr;
	wxTextCtrl* refractionFirePeriod = nullptr;
	wxTextCtrl* parallaxMaxPasses = nullptr;
	wxTextCtrl* parallaxScale = nullptr;
	wxTextCtrl* lightingEffect1 = nullptr;
	wxTextCtrl* lightingEffect2 = nullptr;
	wxColourPickerCtrl* skinTintColor = nullptr;
	wxColourPickerCtrl* hairTintColor = nullptr;
	wxTextCtrl* parallaxInnerLayerThickness = nullptr;
	wxTextCtrl* parallaxRefractionScale = nullptr;
	wxTextCtrl* parallaxInnerLayerTexScaleU = nullptr;
	wxTextCtrl* parallaxInnerLayerTexScaleV = nullptr;
	wxTextCtrl* parallaxEnvmapStrength = nullptr;
	wxTextCtrl* sparkleParamsR = nullptr;
	wxTextCtrl* sparkleParamsG = nullptr;
	wxTextCtrl* sparkleParamsB = nullptr;
	wxTextCtrl* sparkleParamsA = nullptr;
	wxTextCtrl* eyeCubemapScale = nullptr;
	wxTextCtrl* eyeLeftReflectX = nullptr;
	wxTextCtrl* eyeLeftReflectY = nullptr;
	wxTextCtrl* eyeLeftReflectZ = nullptr;
	wxTextCtrl* eyeRightReflectX = nullptr;
	wxTextCtrl* eyeRightReflectY = nullptr;
	wxTextCtrl* eyeRightReflectZ = nullptr;
	wxTextCtrl* wetMaterialPath = nullptr;
	wxTextCtrl* subsurfaceRolloff = nullptr;
	wxTextCtrl* rimlightPower = nullptr;
	wxTextCtrl* backlightPower = nullptr;
	wxTextCtrl* grayscaleToPaletteScale = nullptr;
	wxTextCtrl* fresnelPower = nullptr;
	wxTextCtrl* wetnessSpecScale = nullptr;
	wxTextCtrl* wetnessSpecPower = nullptr;
	wxTextCtrl* wetnessMinVar = nullptr;
	wxTextCtrl* wetnessEnvMapScale = nullptr;
	wxTextCtrl* wetnessFresnelPower = nullptr;
	wxTextCtrl* wetnessMetalness = nullptr;

	wxCollapsiblePane* transparencyPane = nullptr;
	wxTextCtrl* alphaThreshold = nullptr;
	wxCheckBox* vertexAlpha = nullptr;
	wxCheckBox* alphaTest = nullptr;
	wxCheckBox* alphaBlend = nullptr;
	wxChoice* alphaSrcBlend = nullptr;
	wxChoice* alphaDestBlend = nullptr;
	wxChoice* alphaTestFunc = nullptr;
	wxCheckBox* alphaNoSorter = nullptr;
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
	void GetShaderFlags();
	void GetAdvancedShaderProperties();
	void UpdateShaderTypeFields(uint32_t shaderTypeVal);
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
