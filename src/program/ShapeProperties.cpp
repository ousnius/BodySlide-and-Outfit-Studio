/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ShapeProperties.h"

extern ConfigurationManager Config;

using namespace nifly;

wxBEGIN_EVENT_TABLE(ShapeProperties, wxDialog)
	EVT_BUTTON(XRCID("btnMaterialChooser"), ShapeProperties::OnChooseMaterial)
	EVT_BUTTON(XRCID("btnAddShader"), ShapeProperties::OnAddShader)
	EVT_BUTTON(XRCID("btnRemoveShader"), ShapeProperties::OnRemoveShader)
	EVT_BUTTON(XRCID("btnSetTextures"), ShapeProperties::OnSetTextures)
	EVT_BUTTON(XRCID("btnAddTransparency"), ShapeProperties::OnAddTransparency)
	EVT_BUTTON(XRCID("btnRemoveTransparency"), ShapeProperties::OnRemoveTransparency)
	EVT_BUTTON(XRCID("btnCopyShaderFromShape"), ShapeProperties::OnCopyShaderFromShape)
	EVT_BUTTON(XRCID("btnAddExtraData"), ShapeProperties::OnAddExtraData)
	EVT_TEXT(XRCID("textScale"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textX"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textY"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textZ"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRX"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRY"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRZ"), ShapeProperties::OnTransChanged)
	EVT_BUTTON(wxID_OK, ShapeProperties::OnApply)
wxEND_EVENT_TABLE()

ShapeProperties::ShapeProperties(wxWindow* parent, NifFile* refNif, std::vector<NiShape*> refShapes) {
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ShapeProperties.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load ShapeProperties.xrc file!", "Error", wxICON_ERROR);
		return;
	}

	loaded = xrc->LoadDialog(this, parent, "dlgShapeProp");
	if (!loaded) {
		wxMessageBox("Failed to load ShapeProperties dialog!", "Error", wxICON_ERROR);
		return;
	}

	if (refShapes.empty())
		return;

	SetDoubleBuffered(true);
	CenterOnParent();

	os = (OutfitStudioFrame*)parent;
	nif = refNif;
	shapes = refShapes;

	nbProperties = XRCCTRL(*this, "nbProperties", wxNotebook);

	pgShader = XRCCTRL(*this, "pgShader", wxPanel);
	lbShaderName = XRCCTRL(*this, "lbShaderName", wxStaticText);
	shaderName = XRCCTRL(*this, "shaderName", wxTextCtrl);
	btnMaterialChooser = XRCCTRL(*this, "btnMaterialChooser", wxButton);
	shaderType = XRCCTRL(*this, "shaderType", wxChoice);
	lbShadingType = XRCCTRL(*this, "lbShadingType", wxStaticText);
	shadingType = XRCCTRL(*this, "shadingType", wxChoice);
	specularColor = XRCCTRL(*this, "specularColor", wxColourPickerCtrl);
	specularStrength = XRCCTRL(*this, "specularStrength", wxTextCtrl);
	specularPower = XRCCTRL(*this, "specularPower", wxTextCtrl);
	emissiveColor = XRCCTRL(*this, "emissiveColor", wxColourPickerCtrl);
	emissiveMultiple = XRCCTRL(*this, "emissiveMultiple", wxTextCtrl);
	alpha = XRCCTRL(*this, "alpha", wxTextCtrl);
	vertexColors = XRCCTRL(*this, "vertexColors", wxCheckBox);
	doubleSided = XRCCTRL(*this, "doubleSided", wxCheckBox);
	btnAddShader = XRCCTRL(*this, "btnAddShader", wxButton);
	btnRemoveShader = XRCCTRL(*this, "btnRemoveShader", wxButton);
	btnSetTextures = XRCCTRL(*this, "btnSetTextures", wxButton);

	shaderFlagsPane = XRCCTRL(*this, "shaderFlagsPane", wxCollapsiblePane);
	shaderFlags1List = XRCCTRL(*this, "shaderFlags1List", wxCheckListBox);
	shaderFlags2List = XRCCTRL(*this, "shaderFlags2List", wxCheckListBox);

	advancedShaderPane = XRCCTRL(*this, "advancedShaderPane", wxCollapsiblePane);
	uvOffsetU = XRCCTRL(*this, "uvOffsetU", wxTextCtrl);
	uvOffsetV = XRCCTRL(*this, "uvOffsetV", wxTextCtrl);
	uvScaleU = XRCCTRL(*this, "uvScaleU", wxTextCtrl);
	uvScaleV = XRCCTRL(*this, "uvScaleV", wxTextCtrl);
	textureClampMode = XRCCTRL(*this, "textureClampMode", wxChoice);
	environmentMapScale = XRCCTRL(*this, "environmentMapScale", wxTextCtrl);
	refractionStrength = XRCCTRL(*this, "refractionStrength", wxTextCtrl);
	refractionFirePeriod = XRCCTRL(*this, "refractionFirePeriod", wxTextCtrl);
	parallaxMaxPasses = XRCCTRL(*this, "parallaxMaxPasses", wxTextCtrl);
	parallaxScale = XRCCTRL(*this, "parallaxScale", wxTextCtrl);
	lightingEffect1 = XRCCTRL(*this, "lightingEffect1", wxTextCtrl);
	lightingEffect2 = XRCCTRL(*this, "lightingEffect2", wxTextCtrl);
	skinTintColor = XRCCTRL(*this, "skinTintColor", wxColourPickerCtrl);
	hairTintColor = XRCCTRL(*this, "hairTintColor", wxColourPickerCtrl);
	parallaxInnerLayerThickness = XRCCTRL(*this, "parallaxInnerLayerThickness", wxTextCtrl);
	parallaxRefractionScale = XRCCTRL(*this, "parallaxRefractionScale", wxTextCtrl);
	parallaxInnerLayerTexScaleU = XRCCTRL(*this, "parallaxInnerLayerTexScaleU", wxTextCtrl);
	parallaxInnerLayerTexScaleV = XRCCTRL(*this, "parallaxInnerLayerTexScaleV", wxTextCtrl);
	parallaxEnvmapStrength = XRCCTRL(*this, "parallaxEnvmapStrength", wxTextCtrl);
	sparkleParamsR = XRCCTRL(*this, "sparkleParamsR", wxTextCtrl);
	sparkleParamsG = XRCCTRL(*this, "sparkleParamsG", wxTextCtrl);
	sparkleParamsB = XRCCTRL(*this, "sparkleParamsB", wxTextCtrl);
	sparkleParamsA = XRCCTRL(*this, "sparkleParamsA", wxTextCtrl);
	eyeCubemapScale = XRCCTRL(*this, "eyeCubemapScale", wxTextCtrl);
	eyeLeftReflectX = XRCCTRL(*this, "eyeLeftReflectX", wxTextCtrl);
	eyeLeftReflectY = XRCCTRL(*this, "eyeLeftReflectY", wxTextCtrl);
	eyeLeftReflectZ = XRCCTRL(*this, "eyeLeftReflectZ", wxTextCtrl);
	eyeRightReflectX = XRCCTRL(*this, "eyeRightReflectX", wxTextCtrl);
	eyeRightReflectY = XRCCTRL(*this, "eyeRightReflectY", wxTextCtrl);
	eyeRightReflectZ = XRCCTRL(*this, "eyeRightReflectZ", wxTextCtrl);
	wetMaterialPath = XRCCTRL(*this, "wetMaterialPath", wxTextCtrl);
	subsurfaceRolloff = XRCCTRL(*this, "subsurfaceRolloff", wxTextCtrl);
	rimlightPower = XRCCTRL(*this, "rimlightPower", wxTextCtrl);
	backlightPower = XRCCTRL(*this, "backlightPower", wxTextCtrl);
	grayscaleToPaletteScale = XRCCTRL(*this, "grayscaleToPaletteScale", wxTextCtrl);
	fresnelPower = XRCCTRL(*this, "fresnelPower", wxTextCtrl);
	wetnessSpecScale = XRCCTRL(*this, "wetnessSpecScale", wxTextCtrl);
	wetnessSpecPower = XRCCTRL(*this, "wetnessSpecPower", wxTextCtrl);
	wetnessMinVar = XRCCTRL(*this, "wetnessMinVar", wxTextCtrl);
	wetnessEnvMapScale = XRCCTRL(*this, "wetnessEnvMapScale", wxTextCtrl);
	wetnessFresnelPower = XRCCTRL(*this, "wetnessFresnelPower", wxTextCtrl);
	wetnessMetalness = XRCCTRL(*this, "wetnessMetalness", wxTextCtrl);

	alphaThreshold = XRCCTRL(*this, "alphaThreshold", wxTextCtrl);
	vertexAlpha = XRCCTRL(*this, "vertexAlpha", wxCheckBox);
	alphaTest = XRCCTRL(*this, "alphaTest", wxCheckBox);
	alphaBlend = XRCCTRL(*this, "alphaBlend", wxCheckBox);
	alphaSrcBlend = XRCCTRL(*this, "alphaSrcBlend", wxChoice);
	alphaDestBlend = XRCCTRL(*this, "alphaDestBlend", wxChoice);
	alphaTestFunc = XRCCTRL(*this, "alphaTestFunc", wxChoice);
	alphaNoSorter = XRCCTRL(*this, "alphaNoSorter", wxCheckBox);
	btnAddTransparency = XRCCTRL(*this, "btnAddTransparency", wxButton);
	btnRemoveTransparency = XRCCTRL(*this, "btnRemoveTransparency", wxButton);

	transparencyPane = XRCCTRL(*this, "transparencyPane", wxCollapsiblePane);

	// Bind collapsible pane events to update layout
	auto onPaneChanged = [this](wxCollapsiblePaneEvent&) {
		pgShader->Layout();
	};
	shaderFlagsPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, onPaneChanged);
	advancedShaderPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, onPaneChanged);
	transparencyPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, onPaneChanged);

	// Update advanced field visibility when shader type changes
	shaderType->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
		UpdateShaderTypeFields(shaderType->GetSelection());
	});

	// Sync vertex colors/alpha/double-sided checkboxes with shader flags list
	vertexColors->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) {
		if (shaderFlags2List->GetCount() > 5)
			shaderFlags2List->Check(5, evt.IsChecked());
	});
	doubleSided->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) {
		// FO3/NV: bit 4 of shaderFlags2 is "Refraction Tint", not "Double Sided".
		// Double sided is controlled by NiStencilProperty for those games.
		NiShader* s = nif->GetShader(shapes[0]);
		if (s && !s->HasType<BSShaderPPLightingProperty>()) {
			if (shaderFlags2List->GetCount() > 4)
				shaderFlags2List->Check(4, evt.IsChecked());
		}
	});
	vertexAlpha->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt) {
		if (shaderFlags1List->GetCount() > 3)
			shaderFlags1List->Check(3, evt.IsChecked());
		// Enabling vertex alpha also enables vertex colors
		if (evt.IsChecked()) {
			if (shaderFlags2List->GetCount() > 5)
				shaderFlags2List->Check(5, true);
			vertexColors->SetValue(true);
		}
	});

	btnCopyShaderFromShape = XRCCTRL(*this, "btnCopyShaderFromShape", wxButton);

	pgGeometry = XRCCTRL(*this, "pgGeometry", wxPanel);
	fullPrecision = XRCCTRL(*this, "fullPrecision", wxCheckBox);
	subIndex = XRCCTRL(*this, "subIndex", wxCheckBox);
	skinned = XRCCTRL(*this, "skinned", wxCheckBox);
	dynamic = XRCCTRL(*this, "dynamic", wxCheckBox);

	pgExtraData = XRCCTRL(*this, "pgExtraData", wxPanel);
	extraDataGrid = (wxFlexGridSizer*)XRCCTRL(*this, "btnAddExtraData", wxButton)->GetContainingSizer();

	pgCoordinates = XRCCTRL(*this, "pgCoordinates", wxPanel);
	textScale = XRCCTRL(*this, "textScale", wxTextCtrl);
	textX = XRCCTRL(*this, "textX", wxTextCtrl);
	textY = XRCCTRL(*this, "textY", wxTextCtrl);
	textZ = XRCCTRL(*this, "textZ", wxTextCtrl);
	textRX = XRCCTRL(*this, "textRX", wxTextCtrl);
	textRY = XRCCTRL(*this, "textRY", wxTextCtrl);
	textRZ = XRCCTRL(*this, "textRZ", wxTextCtrl);
	cbTransformGeo = XRCCTRL(*this, "cbTransformGeo", wxCheckBox);

	auto& version = nif->GetHeader().GetVersion();
	if (version.Stream() >= 130) {
		lbShaderName->SetLabel(_("Material"));
		btnMaterialChooser->Show();
		pgShader->Layout();
	}

	GetShader();
	GetTransparency();
	GetGeometry();
	GetExtraData();
	GetCoordTrans();

	size_t shapeCount = shapes.size();
	if (shapeCount > 1) {
		SetTitle(wxString::Format("%s - %s", GetTitle(), wxString::Format(_("%zu shapes selected"), shapeCount)));
		nbProperties->SetSelection(1); // Make "Geometry" active
	}
}

ShapeProperties::~ShapeProperties() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ShapeProperties.xrc");
}

bool ShapeProperties::ShowConfirmationDialog() {
	if (confirmationAccepted)
		return true;

	if (shapes.size() > 1) {
		wxMessageDialog dlg(this,
							wxString::Format(_("This action will affect all %zu selected shapes. Are you sure?"), shapes.size()),
							_("Confirmation"),
							wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetYesNoCancelLabels(_("Yes"), _("No"), _("Cancel"));

		int res = dlg.ShowModal();
		if (res != wxID_YES)
			return false;
	}

	confirmationAccepted = true;
	return true;
}

void ShapeProperties::GetShader() {
	bool multipleShapes = shapes.size() > 1;

	bool anyWithShader = false;
	bool anyWithoutShader = false;
	for (auto& shape : shapes) {
		NiShader* shader = nif->GetShader(shape);
		if (shader)
			anyWithShader = true;
		else
			anyWithoutShader = true;
	}

	NiShape* shape = shapes[0];
	NiShader* shader = nif->GetShader(shape);

	if (multipleShapes) {
		btnAddShader->Enable(anyWithoutShader);
		btnRemoveShader->Enable(anyWithShader);
		btnSetTextures->Enable(anyWithShader);

		btnMaterialChooser->Disable();
		shaderName->Disable();
		specularColor->Disable();
		specularStrength->Disable();
		specularPower->Disable();
		emissiveColor->Disable();
		emissiveMultiple->Disable();
		alpha->Disable();
		vertexColors->Disable();
		doubleSided->Disable();
		vertexAlpha->Disable();
		alphaTest->Disable();
		alphaBlend->Disable();
		shaderFlagsPane->Disable();
		advancedShaderPane->Disable();
	}
	else {
		if (!shader) {
			btnAddShader->Enable();

			btnRemoveShader->Disable();
			btnMaterialChooser->Disable();
			btnSetTextures->Disable();
			shaderName->Disable();
			specularColor->Disable();
			specularStrength->Disable();
			specularPower->Disable();
			emissiveColor->Disable();
			emissiveMultiple->Disable();
			alpha->Disable();
			vertexColors->Disable();
			doubleSided->Disable();
			vertexAlpha->Disable();
			alphaTest->Disable();
			alphaBlend->Disable();
			shaderFlagsPane->Disable();
			advancedShaderPane->Disable();

			// Check for standalone NiMaterialProperty (Oblivion/FO3)
			NiMaterialProperty* material = nif->GetMaterialProperty(shape);
			if (material) {
				specularColor->Enable();
				specularPower->Enable();
				emissiveColor->Enable();
				emissiveMultiple->Enable();
				alpha->Enable();

				Vector3 colorVec = material->GetSpecularColor() * 255.0f;
				specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
				specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));

				Color4 color = material->GetEmissiveColor() * 255.0f;
				emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
				emissiveMultiple->SetValue(wxString::Format("%.4f", material->GetEmissiveMultiple()));
				alpha->SetValue(wxString::Format("%.4f", material->GetAlpha()));
			}
		}
		else {
			btnAddShader->Disable();
			alpha->Disable();

			btnRemoveShader->Enable();
			btnMaterialChooser->Enable();
			btnSetTextures->Enable();
			shaderName->Enable();
			specularColor->Enable();
			specularStrength->Enable();
			specularPower->Enable();
			emissiveColor->Enable();
			emissiveMultiple->Enable();
			vertexColors->Enable();
			doubleSided->Enable();
			vertexAlpha->Enable();
			alphaTest->Enable();
			alphaBlend->Enable();
			shaderFlagsPane->Enable();
			advancedShaderPane->Enable();
		}
	}

	// Set values of shader of first shape
	if (shader) {
		bool hasVertexColors = shader->HasVertexColors();
		bool isDoubleSided = shader->IsDoubleSided();
		bool hasVertexAlpha = shader->HasVertexAlpha();
		shaderName->SetValue(shader->name.get());
		vertexColors->SetValue(hasVertexColors);
		vertexAlpha->SetValue(hasVertexAlpha);

		// FO3/NV: Double sided is controlled by NiStencilProperty, not shader flags
		if (shader->HasType<BSShaderPPLightingProperty>()) {
			NiStencilProperty* stencil = nif->GetStencilProperty(shape);
			if (stencil) {
				int drawMode = (stencil->flags & DRAW_MASK) >> DRAW_POS;
				isDoubleSided = (drawMode == DRAW_BOTH);
			}
			else {
				isDoubleSided = false;
			}
		}

		doubleSided->SetValue(isDoubleSided);

		Color4 color;
		Vector3 colorVec;
		if (shader->HasType<BSEffectShaderProperty>()) {
			specularColor->Disable();
			specularStrength->Disable();
			specularPower->Disable();

			color = shader->GetEmissiveColor() * 255.0f;
			emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
			emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
			alpha->SetValue(wxString::Format("%.4f", shader->GetAlpha()));
		}
		else if (shader->HasType<BSLightingShaderProperty>()) {
			if (!multipleShapes)
				alpha->Enable();

			colorVec = shader->GetSpecularColor() * 255.0f;
			specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
			specularStrength->SetValue(wxString::Format("%.4f", shader->GetSpecularStrength()));
			specularPower->SetValue(wxString::Format("%.4f", shader->GetGlossiness()));

			color = shader->GetEmissiveColor() * 255.0f;
			emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
			emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
			alpha->SetValue(wxString::Format("%.4f", shader->GetAlpha()));
		}
		else if (shader->HasType<BSShaderPPLightingProperty>() || shader->HasType<NiMaterialProperty>())
			specularStrength->Disable();

		NiMaterialProperty* material = nif->GetMaterialProperty(shape);
		if (material) {
			if (!multipleShapes)
				alpha->Enable();

			colorVec = material->GetSpecularColor() * 255.0f;
			specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
			specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));

			color = material->GetEmissiveColor() * 255.0f;
			emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
			emissiveMultiple->SetValue(wxString::Format("%.4f", material->GetEmissiveMultiple()));
			alpha->SetValue(wxString::Format("%.4f", material->GetAlpha()));
		}
	}

	GetShaderType();
	GetShaderFlags();
	GetAdvancedShaderProperties();
}


void ShapeProperties::GetShaderType() {
	shaderType->Disable();
	shaderType->Clear();

	bool multipleShapes = shapes.size() > 1;
	NiShape* shape = shapes[0];

	uint32_t type;
	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		if (shader->HasType<BSLightingShaderProperty>()) {
			type = shader->GetShaderType();
			if (type > BSLSP_LAST)
				type = 0;

			shaderType->Append("Default");
			shaderType->Append("Environment Map");
			shaderType->Append("Glow Shader");
			shaderType->Append("Heightmap");
			shaderType->Append("Face Tint");
			shaderType->Append("Skin Tint");
			shaderType->Append("Hair Tint");
			shaderType->Append("Parallax Occlusion Material");
			shaderType->Append("World Multitexture");
			shaderType->Append("World Map 1");
			shaderType->Append("Unknown 10");
			shaderType->Append("Multi Layer Parallax");
			shaderType->Append("Unknown 12");
			shaderType->Append("World Map 2");
			shaderType->Append("Sparkle Snow");
			shaderType->Append("World Map 3");
			shaderType->Append("Eye Environment Map");
			shaderType->Append("Unknown 17");
			shaderType->Append("World Map 4");
			shaderType->Append("World LOD Multitexture");

			if (!multipleShapes)
				shaderType->Enable();

			shaderType->SetSelection(type);
		}
		else if (shader->HasType<BSShaderPPLightingProperty>()) {
			type = shader->GetShaderType();
			shaderType->Append("Tall Grass");
			shaderType->Append("Default");
			shaderType->Append("Sky");
			shaderType->Append("Skin");
			shaderType->Append("Water");
			shaderType->Append("Lighting 30");
			shaderType->Append("Tile");
			shaderType->Append("No Lighting");

			if (!multipleShapes)
				shaderType->Enable();

			switch (type) {
				case BSShaderType::SHADER_TALL_GRASS: shaderType->SetSelection(0); break;
				case BSShaderType::SHADER_DEFAULT: shaderType->SetSelection(1); break;
				case BSShaderType::SHADER_SKY: shaderType->SetSelection(2); break;
				case BSShaderType::SHADER_SKIN: shaderType->SetSelection(3); break;
				case BSShaderType::SHADER_WATER: shaderType->SetSelection(4); break;
				case BSShaderType::SHADER_LIGHTING30: shaderType->SetSelection(5); break;
				case BSShaderType::SHADER_TILE: shaderType->SetSelection(6); break;
				case BSShaderType::SHADER_NOLIGHTING: shaderType->SetSelection(7); break;
				default: shaderType->SetSelection(1);
			}

			// NiShadeProperty::shadingFlags (FO3/NV)
			auto* bsShaderProp = dynamic_cast<BSShaderProperty*>(shader);
			if (bsShaderProp) {
				lbShadingType->Show();
				shadingType->Show();
				shadingType->SetSelection(bsShaderProp->shadingFlags == SHADING_SMOOTH ? 1 : 0);

				if (!multipleShapes)
					shadingType->Enable();
			}
		}
	}
}

void ShapeProperties::GetShaderFlags() {
	shaderFlags1List->Clear();
	shaderFlags2List->Clear();

	bool multipleShapes = shapes.size() > 1;
	if (multipleShapes)
		return;

	NiShape* shape = shapes[0];
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		return;

	auto* bsShaderProp = dynamic_cast<BSShaderProperty*>(shader);
	if (!bsShaderProp)
		return;

	uint32_t sf1 = bsShaderProp->shaderFlags1;
	uint32_t sf2 = bsShaderProp->shaderFlags2;

	std::vector<ShaderFlagDef> flags1Defs;
	std::vector<ShaderFlagDef> flags2Defs;

	if (shader->HasType<BSShaderPPLightingProperty>()) {
		flags1Defs = GetFO3ShaderFlags1();
		flags2Defs = GetFO3ShaderFlags2();
	}
	else if (nif->GetHeader().GetVersion().Stream() >= 130) {
		flags1Defs = GetFO4ShaderFlags1();
		flags2Defs = GetFO4ShaderFlags2();
	}
	else {
		flags1Defs = GetSkyrimShaderFlags1();
		flags2Defs = GetSkyrimShaderFlags2();
	}

	wxArrayString flagNames1, flagNames2;
	for (auto& def : flags1Defs)
		flagNames1.Add(wxString::Format("%s (Bit %d)", def.name, def.bit));
	for (auto& def : flags2Defs)
		flagNames2.Add(wxString::Format("%s (Bit %d)", def.name, def.bit));

	shaderFlags1List->InsertItems(flagNames1, 0);
	shaderFlags2List->InsertItems(flagNames2, 0);

	for (size_t i = 0; i < flags1Defs.size(); i++) {
		if (sf1 & (static_cast<uint32_t>(1) << flags1Defs[i].bit))
			shaderFlags1List->Check(i, true);
	}

	for (size_t i = 0; i < flags2Defs.size(); i++) {
		if (sf2 & (static_cast<uint32_t>(1) << flags2Defs[i].bit))
			shaderFlags2List->Check(i, true);
	}
}

void ShapeProperties::GetAdvancedShaderProperties() {
	bool multipleShapes = shapes.size() > 1;

	// Helper to show/hide a control and its label in the advanced grid.
	// Handles both controls directly in the flex grid and controls nested in sub-sizers.
	wxSizer* gridSizer = advancedShaderPane->GetPane()->GetSizer();
	auto showControl = [gridSizer](wxWindow* ctrl, bool show) {
		if (!ctrl || !gridSizer)
			return;
		ctrl->Show(show);

		wxSizerItem* prevItem = nullptr;
		for (auto* item : gridSizer->GetChildren()) {
			bool found = false;

			if (item->GetWindow() == ctrl) {
				found = true;
			}
			else if (item->IsSizer()) {
				// Check if the control is inside a sub-sizer (e.g. UV offset box)
				for (auto* subItem : item->GetSizer()->GetChildren()) {
					if (subItem->GetWindow() == ctrl) {
						// Show/hide all sibling controls in the sub-sizer
						for (auto* s : item->GetSizer()->GetChildren())
							if (s->GetWindow())
								s->GetWindow()->Show(show);
						found = true;
						break;
					}
				}
			}

			if (found) {
				item->Show(show);
				if (prevItem) {
					prevItem->Show(show);
					if (prevItem->GetWindow())
						prevItem->GetWindow()->Show(show);
				}
				return;
			}
			prevItem = item;
		}
	};

	// Hide all advanced controls initially
	auto hideAll = [&]() {
		wxWindow* allCtrls[] = {
			uvOffsetU, uvScaleU, textureClampMode, environmentMapScale,
			refractionStrength, refractionFirePeriod, parallaxMaxPasses, parallaxScale,
			lightingEffect1, lightingEffect2, skinTintColor, hairTintColor,
			parallaxInnerLayerThickness, parallaxRefractionScale,
			parallaxInnerLayerTexScaleU,
			parallaxEnvmapStrength, sparkleParamsR,
			eyeCubemapScale, eyeLeftReflectX,
			eyeRightReflectX,
			wetMaterialPath, subsurfaceRolloff, rimlightPower, backlightPower,
			grayscaleToPaletteScale, fresnelPower,
			wetnessSpecScale, wetnessSpecPower, wetnessMinVar, wetnessEnvMapScale,
			wetnessFresnelPower, wetnessMetalness
		};
		for (auto* ctrl : allCtrls)
			showControl(ctrl, false);
	};

	hideAll();

	if (multipleShapes)
		return;

	NiShape* shape = shapes[0];
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		return;

	auto& version = nif->GetHeader().GetVersion();

	if (shader->HasType<BSShaderPPLightingProperty>()) {
		auto bspplp = dynamic_cast<BSShaderPPLightingProperty*>(shader);
		auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
		if (!bspplp || !bssp)
			return;

		showControl(environmentMapScale, true);
		environmentMapScale->SetValue(wxString::Format("%.4f", bssp->environmentMapScale));

		auto* bsslp = dynamic_cast<BSShaderLightingProperty*>(shader);
		if (bsslp) {
			showControl(textureClampMode, true);
			textureClampMode->SetSelection(bsslp->textureClampMode < 4 ? bsslp->textureClampMode : 3);
		}

		showControl(refractionStrength, true);
		refractionStrength->SetValue(wxString::Format("%.4f", bspplp->refractionStrength));

		showControl(refractionFirePeriod, true);
		refractionFirePeriod->SetValue(wxString::Format("%d", bspplp->refractionFirePeriod));

		showControl(parallaxMaxPasses, true);
		parallaxMaxPasses->SetValue(wxString::Format("%.4f", bspplp->parallaxMaxPasses));

		showControl(parallaxScale, true);
		parallaxScale->SetValue(wxString::Format("%.4f", bspplp->parallaxScale));
	}
	else if (shader->HasType<BSLightingShaderProperty>()) {
		auto bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
		auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
		if (!bslsp || !bssp)
			return;

		uint32_t shaderTypeVal = bslsp->GetShaderType();

		// Common properties always shown for BSLightingShaderProperty
		showControl(uvOffsetU, true);
		uvOffsetU->SetValue(wxString::Format("%.4f", bssp->uvOffset.u));
		uvOffsetV->SetValue(wxString::Format("%.4f", bssp->uvOffset.v));

		showControl(uvScaleU, true);
		uvScaleU->SetValue(wxString::Format("%.4f", bssp->uvScale.u));
		uvScaleV->SetValue(wxString::Format("%.4f", bssp->uvScale.v));

		showControl(textureClampMode, true);
		textureClampMode->SetSelection(bslsp->textureClampMode < 4 ? bslsp->textureClampMode : 3);

		showControl(refractionStrength, true);
		refractionStrength->SetValue(wxString::Format("%.4f", bslsp->refractionStrength));

		// Lighting Effect 1/2 (softlighting/rimlightPower) - stream < 130
		if (version.Stream() < 130) {
			showControl(lightingEffect1, true);
			lightingEffect1->SetValue(wxString::Format("%.4f", bslsp->softlighting));

			showControl(lightingEffect2, true);
			lightingEffect2->SetValue(wxString::Format("%.4f", bslsp->rimlightPower));
		}

		// Shader type-specific fields
		UpdateShaderTypeFields(shaderTypeVal);

		// FO4+ properties (stream >= 130)
		if (version.Stream() >= 130) {
			showControl(grayscaleToPaletteScale, true);
			grayscaleToPaletteScale->SetValue(wxString::Format("%.4f", bslsp->grayscaleToPaletteScale));

			showControl(fresnelPower, true);
			fresnelPower->SetValue(wxString::Format("%.4f", bslsp->fresnelPower));

			showControl(wetnessSpecScale, true);
			wetnessSpecScale->SetValue(wxString::Format("%.4f", bslsp->wetnessSpecScale));

			showControl(wetnessSpecPower, true);
			wetnessSpecPower->SetValue(wxString::Format("%.4f", bslsp->wetnessSpecPower));

			showControl(wetnessMinVar, true);
			wetnessMinVar->SetValue(wxString::Format("%.4f", bslsp->wetnessMinVar));

			showControl(wetnessFresnelPower, true);
			wetnessFresnelPower->SetValue(wxString::Format("%.4f", bslsp->wetnessFresnelPower));

			showControl(wetnessMetalness, true);
			wetnessMetalness->SetValue(wxString::Format("%.4f", bslsp->wetnessMetalness));

			showControl(wetMaterialPath, true);
			wetMaterialPath->SetValue(bslsp->GetWetMaterialName());

			showControl(subsurfaceRolloff, true);
			subsurfaceRolloff->SetValue(wxString::Format("%.4f", bslsp->subsurfaceRolloff));

			showControl(rimlightPower, true);
			rimlightPower->SetValue(wxString::Format("%.4f", bslsp->rimlightPower));

			showControl(backlightPower, true);
			backlightPower->SetValue(wxString::Format("%.4f", bslsp->backlightPower));
		}

		// FO4-only (stream 130-139)
		if (version.IsFO4()) {
			showControl(wetnessEnvMapScale, true);
			wetnessEnvMapScale->SetValue(wxString::Format("%.4f", bslsp->wetnessEnvmapScale));
		}
	}

	advancedShaderPane->GetPane()->Layout();
}

void ShapeProperties::UpdateShaderTypeFields(uint32_t shaderTypeVal) {
	wxSizer* gridSizer = advancedShaderPane->GetPane()->GetSizer();
	auto showControl = [gridSizer](wxWindow* ctrl, bool show) {
		if (!ctrl || !gridSizer)
			return;
		ctrl->Show(show);

		wxSizerItem* prevItem = nullptr;
		for (auto* item : gridSizer->GetChildren()) {
			bool found = false;
			if (item->GetWindow() == ctrl) {
				found = true;
			}
			else if (item->IsSizer()) {
				for (auto* subItem : item->GetSizer()->GetChildren()) {
					if (subItem->GetWindow() == ctrl) {
						for (auto* s : item->GetSizer()->GetChildren())
							if (s->GetWindow())
								s->GetWindow()->Show(show);
						found = true;
						break;
					}
				}
			}
			if (found) {
				item->Show(show);
				if (prevItem) {
					prevItem->Show(show);
					if (prevItem->GetWindow())
						prevItem->GetWindow()->Show(show);
				}
				return;
			}
			prevItem = item;
		}
	};

	// Hide all shader-type-specific fields
	wxWindow* typeSpecificCtrls[] = {
		environmentMapScale, skinTintColor, hairTintColor,
		parallaxMaxPasses, parallaxScale,
		parallaxInnerLayerThickness, parallaxRefractionScale,
		parallaxInnerLayerTexScaleU, parallaxEnvmapStrength,
		sparkleParamsR,
		eyeCubemapScale, eyeLeftReflectX, eyeRightReflectX
	};
	for (auto* ctrl : typeSpecificCtrls)
		showControl(ctrl, false);

	switch (shaderTypeVal) {
		case BSLightingShaderPropertyShaderType::BSLSP_ENVMAP:
			showControl(environmentMapScale, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_SKINTINT:
			showControl(skinTintColor, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_HAIRTINT:
			showControl(hairTintColor, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_PARALLAXOCC:
			showControl(parallaxMaxPasses, true);
			showControl(parallaxScale, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_MULTILAYERPARALLAX:
			showControl(parallaxInnerLayerThickness, true);
			showControl(parallaxRefractionScale, true);
			showControl(parallaxInnerLayerTexScaleU, true);
			showControl(parallaxEnvmapStrength, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_MULTIINDEXSNOW:
			showControl(sparkleParamsR, true);
			break;
		case BSLightingShaderPropertyShaderType::BSLSP_EYE:
			showControl(eyeCubemapScale, true);
			showControl(eyeLeftReflectX, true);
			showControl(eyeRightReflectX, true);
			break;
	}

	advancedShaderPane->GetPane()->Layout();
	advancedShaderPane->InvalidateBestSize();
	pgShader->InvalidateBestSize();
	nbProperties->InvalidateBestSize();
	InvalidateBestSize();
	pgShader->Layout();

	// Grow the dialog if needed, but don't shrink it
	wxSize cur = GetSize();
	wxSize best = GetBestSize();
	wxSize newSize(std::max(cur.x, best.x), std::max(cur.y, best.y));
	SetMinSize(newSize);
	SetSize(newSize);
}

void ShapeProperties::OnChooseMaterial(wxCommandEvent& WXUNUSED(event)) {
	wxString fileName = wxFileSelector(_("Choose material file"), wxEmptyString, wxEmptyString, ".bgsm", "Material files (*.bgsm;*.bgem)|*.bgsm;*.bgem", wxFD_FILE_MUST_EXIST, this);
	if (fileName.empty())
		return;

	wxString findStr = wxString::Format("%cmaterials%c", PathSepChar, PathSepChar);
	int index = fileName.Lower().Find(findStr);
	if (index != wxNOT_FOUND && fileName.length() - 1 > (size_t)index + 1)
		fileName = fileName.Mid(index + 1);

	shaderName->SetValue(fileName);
}

void ShapeProperties::OnAddShader(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	for (auto& shape : shapes)
		AddShader(shape);

	GetShader();
	GetTransparency();
}

void ShapeProperties::AddShader(NiShape* shape) {
	NiShader* shader = nif->GetShader(shape);
	if (shader)
		return;

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	std::unique_ptr<NiShader> newShader = nullptr;
	std::unique_ptr<NiMaterialProperty> newMaterial = nullptr;

	switch (targetGame) {
		case OB:
			newMaterial = std::make_unique<NiMaterialProperty>();
			shape->propertyRefs.AddBlockRef(nif->GetHeader().AddBlock(std::move(newMaterial)));
			break;

		case FO3:
		case FONV:
			newShader = std::make_unique<BSShaderPPLightingProperty>();
			shape->propertyRefs.AddBlockRef(nif->GetHeader().AddBlock(std::move(newShader)));

			newMaterial = std::make_unique<NiMaterialProperty>();
			shape->propertyRefs.AddBlockRef(nif->GetHeader().AddBlock(std::move(newMaterial)));
			break;

		default:
			newShader = std::make_unique<BSLightingShaderProperty>(nif->GetHeader().GetVersion());
			auto shaderPropertyRef = shape->ShaderPropertyRef();
			if (shaderPropertyRef)
				shaderPropertyRef->index = nif->GetHeader().AddBlock(std::move(newShader));
	}

	shader = nif->GetShader(shape);
	if (shader) {
		auto nifTexSet = std::make_unique<BSShaderTextureSet>(nif->GetHeader().GetVersion());
		auto textureSetRef = shader->TextureSetRef();
		if (textureSetRef)
			textureSetRef->index = nif->GetHeader().AddBlock(std::move(nifTexSet));
	}

	AssignDefaultTexture(shape);
	GetShader();
	os->SetPendingChanges();
}

void ShapeProperties::OnRemoveShader(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	for (auto& shape : shapes)
		RemoveShader(shape);

	GetShader();
	GetTransparency();
}

void ShapeProperties::RemoveShader(NiShape* shape) {
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		return;

	nif->DeleteShader(shape);
	AssignDefaultTexture(shape);
	os->SetPendingChanges();
}

void ShapeProperties::OnSetTextures(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	bool shaderFound = false;
	for (auto& shape : shapes) {
		NiShader* shader = nif->GetShader(shape);
		if (shader) {
			shaderFound = true;
			break;
		}
	}
	if (!shaderFound)
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgShapeTextures")) {
		wxGrid* stTexGrid = XRCCTRL(dlg, "stTexGrid", wxGrid);
		stTexGrid->CreateGrid(10, 1);
		stTexGrid->EnableEditing(true);
		stTexGrid->EnableGridLines(true);
		stTexGrid->EnableDragGridSize(false);
		stTexGrid->SetMargins(0, 0);

		// Columns
		stTexGrid->SetColSize(0, 700);
		stTexGrid->EnableDragColMove(false);
		stTexGrid->EnableDragColSize(false);
		stTexGrid->SetColLabelSize(30);
		stTexGrid->SetColLabelValue(0, "Game Texture Paths");
		stTexGrid->SetColLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

		// Rows
		stTexGrid->AutoSizeRows();
		stTexGrid->EnableDragRowSize(false);
		stTexGrid->SetRowLabelSize(80);
		stTexGrid->SetRowLabelValue(0, "Diffuse");
		stTexGrid->SetRowLabelValue(1, "Normal");
		stTexGrid->SetRowLabelValue(2, "Glow/Skin");
		stTexGrid->SetRowLabelValue(3, "Parallax");
		stTexGrid->SetRowLabelValue(4, "Environment");
		stTexGrid->SetRowLabelValue(5, "Env Mask");
		stTexGrid->SetRowLabelValue(6, "6");
		stTexGrid->SetRowLabelValue(7, "Specular");
		stTexGrid->SetRowLabelValue(8, "8");
		stTexGrid->SetRowLabelValue(9, "9");
		stTexGrid->SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

		// Cell Defaults
		stTexGrid->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_TOP);

		NiShape* firstShape = shapes[0];

		int blockType = 0;
		for (int i = 0; i < 10; i++) {
			std::string texPath;
			blockType = nif->GetTextureSlot(firstShape, texPath, i);
			if (!blockType)
				continue;

			stTexGrid->SetCellValue(i, 0, ToOSSlashes(texPath));
		}

		// BSEffectShaderProperty
		if (blockType == 2) {
			stTexGrid->SetRowLabelValue(0, "Source");
			stTexGrid->SetRowLabelValue(1, "Normal");
			stTexGrid->HideRow(2);
			stTexGrid->SetRowLabelValue(3, "Greyscale");
			stTexGrid->SetRowLabelValue(4, "Environment");
			stTexGrid->SetRowLabelValue(5, "Env Mask");
			stTexGrid->HideRow(6);
			stTexGrid->HideRow(7);
			stTexGrid->HideRow(8);
			stTexGrid->HideRow(9);
		}

		// NiTexturingProperty/NiSourceTexture
		if (blockType == 3) {
			stTexGrid->SetRowLabelValue(0, "Base");
			stTexGrid->SetRowLabelValue(1, "Dark");
			stTexGrid->SetRowLabelValue(2, "Detail");
			stTexGrid->SetRowLabelValue(3, "Gloss");
			stTexGrid->SetRowLabelValue(4, "Glow");
			stTexGrid->SetRowLabelValue(5, "Bump Map");
			stTexGrid->SetRowLabelValue(6, "Decal 0");
			stTexGrid->SetRowLabelValue(7, "Decal 1");
			stTexGrid->SetRowLabelValue(8, "Decal 2");
			stTexGrid->SetRowLabelValue(9, "Decal 3");
		}

		if (dlg.ShowModal() == wxID_OK) {
			auto dataPath = Config["GameDataPath"];
			std::vector<std::string> texFiles(10);
			for (int i = 0; i < 10; i++) {
				std::string texPath = stTexGrid->GetCellValue(i, 0).ToStdString();
				std::string texPath_bs = ToBackslashes(texPath);

				for (auto& shape : shapes)
					nif->SetTextureSlot(shape, texPath_bs, i);

				if (!texPath.empty())
					texFiles[i] = dataPath + texPath;
			}

			nif->TrimTexturePaths();
			os->SetPendingChanges();

			for (auto& shape : shapes) {
				os->project->SetTextures(shape, texFiles);
				os->glView->SetMeshTextures(shape->name.get(), texFiles, false, MaterialFile(), true);
			}

			os->glView->Render();
		}
	}
}

void ShapeProperties::AssignDefaultTexture(NiShape* shape) {
	os->project->SetTextures(shape);
	os->SetPendingChanges();
	os->glView->Render();
}

void ShapeProperties::GetTransparency() {
	bool multipleShapes = shapes.size() > 1;

	bool anyWithTrans = false;
	bool anyWithoutTrans = false;
	for (auto& shape : shapes) {
		NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
		if (alphaProp)
			anyWithTrans = true;
		else
			anyWithoutTrans = true;
	}

	NiShape* shape = shapes[0];
	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);

	if (multipleShapes) {
		btnAddTransparency->Enable(anyWithoutTrans);
		btnRemoveTransparency->Enable(anyWithTrans);

		alphaThreshold->Disable();
		vertexAlpha->Disable();
		alphaTest->Disable();
		alphaBlend->Disable();
		alphaSrcBlend->Disable();
		alphaDestBlend->Disable();
		alphaTestFunc->Disable();
		alphaNoSorter->Disable();
	}
	else {
		if (alphaProp) {
			alphaThreshold->Enable();
			btnAddTransparency->Disable();
			btnRemoveTransparency->Enable();
			alphaTest->Enable();
			alphaBlend->Enable();
			alphaSrcBlend->Enable();
			alphaDestBlend->Enable();
			alphaTestFunc->Enable();
			alphaNoSorter->Enable();

			NiShader* shader = nif->GetShader(shape);
			if (shader)
				vertexAlpha->Enable();
		}
		else {
			alphaThreshold->Disable();
			vertexAlpha->Disable();
			alphaTest->Disable();
			alphaBlend->Disable();
			alphaSrcBlend->Disable();
			alphaDestBlend->Disable();
			alphaTestFunc->Disable();
			alphaNoSorter->Disable();
			btnAddTransparency->Enable();
			btnRemoveTransparency->Disable();
		}
	}

	// Set values of first shape's alpha property
	if (alphaProp) {
		alphaThreshold->SetValue(wxString::Format("%d", alphaProp->threshold));
		alphaTest->SetValue(alphaProp->flags & (1 << 9));
		alphaBlend->SetValue(alphaProp->flags & 1);

		int srcBlend = (alphaProp->flags >> 1) & 0xF;
		int destBlend = (alphaProp->flags >> 5) & 0xF;
		int testFunc = (alphaProp->flags >> 10) & 0x7;
		bool noSorter = (alphaProp->flags >> 13) & 1;

		alphaSrcBlend->SetSelection(srcBlend < static_cast<int>(alphaSrcBlend->GetCount()) ? srcBlend : 0);
		alphaDestBlend->SetSelection(destBlend < static_cast<int>(alphaDestBlend->GetCount()) ? destBlend : 0);
		alphaTestFunc->SetSelection(testFunc < static_cast<int>(alphaTestFunc->GetCount()) ? testFunc : 0);
		alphaNoSorter->SetValue(noSorter);
	}

	// Expand transparency pane if the mesh has an alpha property
	if (anyWithTrans)
		transparencyPane->Collapse(false);
}

void ShapeProperties::OnAddTransparency(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	for (auto& shape : shapes)
		AddTransparency(shape);

	GetTransparency();
}

void ShapeProperties::AddTransparency(NiShape* shape) {
	NiAlphaProperty* alphaPropExists = nif->GetAlphaProperty(shape);
	if (alphaPropExists)
		return;

	auto alphaProp = std::make_unique<NiAlphaProperty>();
	nif->AssignAlphaProperty(shape, std::move(alphaProp));
	os->SetPendingChanges();
}

void ShapeProperties::OnRemoveTransparency(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	for (auto& shape : shapes)
		RemoveTransparency(shape);

	GetTransparency();
}

void ShapeProperties::OnCopyShaderFromShape(wxCommandEvent& WXUNUSED(event)) {
	if (!ShowConfirmationDialog())
		return;

	bool multipleShapes = shapes.size() > 1;
	NiShape* firstShape = shapes[0];
	wxArrayString choices;

	auto nifShapes = nif->GetShapes();
	for (auto& s : nifShapes) {
		// If there's only one selected shape, it can't be the source of the copy
		if (multipleShapes || s != firstShape) {
			choices.Add(wxString::FromUTF8(s->name.get()));
		}
	}

	if (choices.GetCount() > 0) {
		std::string shapeName{wxGetSingleChoice(_("Please choose a shape to copy from"), _("Choose shape"), choices, 0, this).ToUTF8()};
		if (shapeName.empty())
			return;

		auto shapeChoice = nif->FindBlockByName<NiShape>(shapeName);
		if (shapeChoice) {
			auto shapeChoiceShader = nif->GetShader(shapeChoice);
			auto texturePaths = os->project->GetShapeTextures(shapeChoice);

			for (auto& shape : shapes) {
				if (shape != shapeChoice) {
					// Delete old shader and children from destination shapes
					nif->DeleteShader(shape);

					if (shapeChoiceShader) {
						// Clone shader
						auto destShaderS = shapeChoiceShader->Clone();
						auto destShader = destShaderS.get();

						int destShaderId = nif->GetHeader().AddBlock(std::move(destShaderS));

						// Clone shader children
						nif->CloneChildren(destShader);

						// Assign cloned shader to shape
						shape->ShaderPropertyRef()->index = destShaderId;
					}

					if (shapeChoiceShader) {
						// Load same textures
						os->project->SetTextures(shape, texturePaths);
					}
				}
			}

			// Update UI
			GetShader();
			GetTransparency();

			os->SetPendingChanges();
			os->glView->Render();
		}
	}
}

void ShapeProperties::RemoveTransparency(NiShape* shape) {
	NiAlphaProperty* alphaPropExists = nif->GetAlphaProperty(shape);
	if (!alphaPropExists)
		return;

	nif->RemoveAlphaProperty(shape);
	os->SetPendingChanges();
}


void ShapeProperties::GetGeometry() {
	// Initially set checkbox states for first shape
	NiShape* firstShape = shapes[0];
	skinned->SetValue(firstShape->IsSkinned());

	bool hasSubIndex = firstShape->HasType<BSSubIndexTriShape>();
	subIndex->SetValue(hasSubIndex);

	bool hasDynamic = firstShape->HasType<BSDynamicTriShape>();
	dynamic->SetValue(hasDynamic);

	BSTriShape* bsTriShapeFirst = dynamic_cast<BSTriShape*>(firstShape);
	if (bsTriShapeFirst) {
		fullPrecision->SetValue(bsTriShapeFirst->IsFullPrecision());
		fullPrecision->Enable(bsTriShapeFirst->CanChangePrecision());
	}
	else
		fullPrecision->SetValue(true);

	subIndex->Disable();
	dynamic->Disable();
	fullPrecision->Disable();

	auto& version = nif->GetHeader().GetVersion();

	bool subIndexAvail = false;
	if (version.Stream() >= 130)
		subIndexAvail = true;

	bool dynamicAvail = false;
	if (version.Stream() == 100)
		dynamicAvail = true;

	bool fullPrecisionAvail = true;

	for (auto& shape : shapes) {
		BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (!bsTriShape) {
			// Changing these only possible with BSTriShape and co.
			subIndexAvail = false;
			dynamicAvail = false;
			fullPrecisionAvail = false;
			continue;
		}

		if (fullPrecisionAvail) {
			if (nif->GetHeader().GetVersion().Stream() == 100) {
				fullPrecisionAvail = false; // Changing precision only possible in FO4
				continue;
			}
		}

		if (fullPrecisionAvail) {
			if (!bsTriShape->CanChangePrecision()) {
				fullPrecisionAvail = false;  // Changing precision only if shape allows it
				continue;
			}
		}
	}

	if (fullPrecisionAvail)
		fullPrecision->Enable();
	if (subIndexAvail)
		subIndex->Enable();
	if (dynamicAvail)
		dynamic->Enable();

	// Check for undetermined (multiple) skinned states
	for (auto& shape : shapes) {
		bool checked = skinned->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED;
		if (checked != shape->IsSkinned()) {
			skinned->Set3StateValue(wxCheckBoxState::wxCHK_UNDETERMINED);
			break;
		}
	}

	// Check for undetermined (multiple) subindex states
	for (auto& shape : shapes) {
		if (subIndexAvail) {
			bool checked = subIndex->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED;
			if (checked != shape->HasType<BSSubIndexTriShape>()) {
				subIndex->Set3StateValue(wxCheckBoxState::wxCHK_UNDETERMINED);
				break;
			}
		}
	}

	// Check for undetermined (multiple) dynamic states
	for (auto& shape : shapes) {
		if (dynamicAvail) {
			bool checked = dynamic->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED;
			if (checked != shape->HasType<BSDynamicTriShape>()) {
				dynamic->Set3StateValue(wxCheckBoxState::wxCHK_UNDETERMINED);
				break;
			}
		}
	}

	// Check for undetermined (multiple) full precision states
	for (auto& shape : shapes) {
		if (fullPrecisionAvail) {
			BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
			if (bsTriShape) {
				bool checked = fullPrecision->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED;
				if (checked != bsTriShape->IsFullPrecision()) {
					fullPrecision->Set3StateValue(wxCheckBoxState::wxCHK_UNDETERMINED);
					break;
				}
			}
		}
	}
}


void ShapeProperties::GetExtraData() {
	pgExtraData->Enable(shapes.size() == 1);
	NiShape* shape = shapes[0];

	for (size_t i = 0; i < extraDataIndices.size(); i++) {
		wxButton* extraDataBtn = dynamic_cast<wxButton*>(FindWindowById(1000 + i, this));
		wxChoice* extraDataType = dynamic_cast<wxChoice*>(FindWindowById(2000 + i, this));
		wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + i, this));
		wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + i, this));

		if (extraDataBtn)
			extraDataBtn->Destroy();
		if (extraDataType)
			extraDataType->Destroy();
		if (extraDataName)
			extraDataName->Destroy();
		if (extraDataValue)
			extraDataValue->Destroy();

		pgExtraData->FitInside();
		pgExtraData->Layout();
	}

	extraDataIndices.clear();

	for (auto& extraDataRef : shape->extraDataRefs) {
		auto extraData = nif->GetHeader().GetBlock(extraDataRef);
		if (extraData) {
			extraDataIndices.push_back(extraDataRef.index);
			AddExtraData(shape, extraData, true);
		}
	}
}

void ShapeProperties::OnAddExtraData(wxCommandEvent& WXUNUSED(event)) {
	NiShape* shape = shapes[0];

	NiStringExtraData extraDataTemp;
	AddExtraData(shape, &extraDataTemp);
}

void ShapeProperties::AddExtraData(NiShape* shape, NiExtraData* extraData, bool uiOnly) {
	if (!uiOnly) {
		auto newExtraData = extraData->Clone();
		int index = nif->AssignExtraData(shape, std::move(newExtraData));
		extraDataIndices.push_back(index);
		os->SetPendingChanges();
	}

	if (extraDataIndices.empty())
		return;

	int id = extraDataIndices.size() - 1;

	wxButton* extraDataBtn = new wxButton(pgExtraData, 1000 + id, "Remove");
	extraDataBtn->Bind(wxEVT_BUTTON, &ShapeProperties::OnRemoveExtraData, this);

	wxArrayString types;
	types.Add("NiStringExtraData");
	types.Add("NiIntegerExtraData");
	types.Add("NiFloatExtraData");
	wxChoice* extraDataType = new wxChoice(pgExtraData, 2000 + id, wxDefaultPosition, wxDefaultSize, types);
	extraDataType->SetSelection(0);
	extraDataType->Bind(wxEVT_CHOICE, &ShapeProperties::OnChangeExtraDataType, this);

	wxTextCtrl* extraDataName = new wxTextCtrl(pgExtraData, 3000 + id);
	wxTextCtrl* extraDataValue = new wxTextCtrl(pgExtraData, 4000 + id);

	if (uiOnly) {
		if (extraData->HasType<NiStringExtraData>()) {
			auto stringExtraData = static_cast<NiStringExtraData*>(extraData);
			extraDataType->SetSelection(0);
			extraDataName->SetValue(stringExtraData->name.get());
			extraDataValue->SetValue(stringExtraData->stringData.get());
		}
		else if (extraData->HasType<NiIntegerExtraData>()) {
			auto intExtraData = static_cast<NiIntegerExtraData*>(extraData);
			extraDataType->SetSelection(1);
			extraDataName->SetValue(intExtraData->name.get());
			extraDataValue->SetValue(wxString::Format("%d", intExtraData->integerData));
		}
		else if (extraData->HasType<NiFloatExtraData>()) {
			auto floatExtraData = static_cast<NiFloatExtraData*>(extraData);
			extraDataType->SetSelection(2);
			extraDataName->SetValue(floatExtraData->name.get());
			extraDataValue->SetValue(wxString::Format("%f", floatExtraData->floatData));
		}
		else {
			extraDataBtn->Destroy();
			extraDataType->Destroy();
			extraDataName->Destroy();
			extraDataValue->Destroy();
			return;
		}
	}

	extraDataGrid->Add(extraDataBtn, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);
	extraDataGrid->Add(extraDataType, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);
	extraDataGrid->Add(extraDataName, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);
	extraDataGrid->Add(extraDataValue, 0, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 5);

	pgExtraData->FitInside();
	pgExtraData->Layout();
}

void ShapeProperties::OnChangeExtraDataType(wxCommandEvent& event) {
	NiShape* shape = shapes[0];
	ChangeExtraDataType(shape, event.GetId() - 2000);
}

void ShapeProperties::ChangeExtraDataType(NiShape* shape, int id) {
	wxChoice* extraDataType = dynamic_cast<wxChoice*>(FindWindowById(2000 + id, this));
	int selection = extraDataType->GetSelection();

	int index = extraDataIndices[id];
	nif->GetHeader().DeleteBlock(index);

	for (size_t i = 0; i < extraDataIndices.size(); i++)
		if (extraDataIndices[i] > index)
			extraDataIndices[i]--;

	extraDataIndices[id] = 0xFFFFFFFF;

	wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + id, this));
	wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + id, this));

	std::unique_ptr<NiExtraData> extraDataResult = nullptr;
	switch (selection) {
		case 0: {
			auto strExtraData = std::make_unique<NiStringExtraData>();
			strExtraData->name.get() = extraDataName->GetValue().ToStdString();
			strExtraData->stringData.get() = extraDataValue->GetValue().ToStdString();
			extraDataResult = std::move(strExtraData);
			break;
		}
		case 1: {
			auto intExtraData = std::make_unique<NiIntegerExtraData>();
			intExtraData->name.get() = extraDataName->GetValue().ToStdString();
			intExtraData->integerData = 0;
			extraDataResult = std::move(intExtraData);
			break;
		}
		case 2: {
			auto floatExtraData = std::make_unique<NiFloatExtraData>();
			floatExtraData->name.get() = extraDataName->GetValue().ToStdString();
			floatExtraData->floatData = 0.0f;
			extraDataResult = std::move(floatExtraData);
			break;
		}
	}

	if (extraDataResult) {
		extraDataIndices[id] = nif->AssignExtraData(shape, std::move(extraDataResult));
		os->SetPendingChanges();
	}
}

void ShapeProperties::OnRemoveExtraData(wxCommandEvent& event) {
	RemoveExtraData(event.GetId() - 1000);
}

void ShapeProperties::RemoveExtraData(int id) {
	wxButton* extraDataBtn = dynamic_cast<wxButton*>(FindWindowById(1000 + id, this));
	wxChoice* extraDataType = dynamic_cast<wxChoice*>(FindWindowById(2000 + id, this));
	wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + id, this));
	wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + id, this));

	extraDataBtn->Destroy();
	extraDataType->Destroy();
	extraDataName->Destroy();
	extraDataValue->Destroy();

	int index = extraDataIndices[id];
	nif->GetHeader().DeleteBlock(index);

	for (size_t i = 0; i < extraDataIndices.size(); i++)
		if (extraDataIndices[i] > index)
			extraDataIndices[i]--;

	extraDataIndices[id] = 0xFFFFFFFF;

	pgExtraData->FitInside();
	pgExtraData->Layout();
	os->SetPendingChanges();
}

void ShapeProperties::GetCoordTrans() {
	pgCoordinates->Enable(shapes.size() == 1);

	NiShape* shape = shapes[0];
	Vector3 rotationVec;

	oldTransform = os->project->GetWorkAnim()->GetTransformShapeToGlobal(shape);
	newTransform = oldTransform;
	rotationVec = RotMatToVec(newTransform.rotation);

	textScale->ChangeValue(wxString::Format("%.10f", newTransform.scale));
	textX->ChangeValue(wxString::Format("%.10f", newTransform.translation.x));
	textY->ChangeValue(wxString::Format("%.10f", newTransform.translation.y));
	textZ->ChangeValue(wxString::Format("%.10f", newTransform.translation.z));
	textRX->ChangeValue(wxString::Format("%.10f", rotationVec.x));
	textRY->ChangeValue(wxString::Format("%.10f", rotationVec.y));
	textRZ->ChangeValue(wxString::Format("%.10f", rotationVec.z));

	cbTransformGeo->Disable();
}

void ShapeProperties::OnTransChanged(wxCommandEvent&) {
	if (!textScale || !textX || !textY || !textZ || !textRX || !textRY || !textRZ)
		return;

	double scale, x, y, z, rx, ry, rz;
	if (!textScale->GetValue().ToDouble(&scale))
		return;
	if (scale <= 0)
		return;
	if (!textX->GetValue().ToDouble(&x))
		return;
	if (!textY->GetValue().ToDouble(&y))
		return;
	if (!textZ->GetValue().ToDouble(&z))
		return;
	if (!textRX->GetValue().ToDouble(&rx))
		return;
	if (!textRY->GetValue().ToDouble(&ry))
		return;
	if (!textRZ->GetValue().ToDouble(&rz))
		return;

	newTransform.scale = scale;
	newTransform.translation.x = x;
	newTransform.translation.y = y;
	newTransform.translation.z = z;
	newTransform.rotation = RotVecToMat(Vector3(rx, ry, rz));
	cbTransformGeo->Enable(!newTransform.IsNearlyEqualTo(oldTransform));
}

void ShapeProperties::RefreshMesh() {
	for (auto& shape : shapes) {
		os->project->SetTextures(shape);
		os->MeshFromProj(shape, true);
	}
	os->UpdateActiveShape();
}

void ShapeProperties::OnApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyChanges();
	RefreshMesh();
	EndModal(wxID_OK);
}

void ShapeProperties::ApplyChanges() {
	bool multipleShapes = shapes.size() > 1;
	auto& version = nif->GetHeader().GetVersion();

	wxColour color = specularColor->GetColour();
	Vector3 specColor(color.Red(), color.Green(), color.Blue());
	specColor /= 255.0f;
	float specStrength = atof(specularStrength->GetValue().c_str());
	float specPower = atof(specularPower->GetValue().c_str());

	color = emissiveColor->GetColour();
	Color4 emisColor(color.Red(), color.Green(), color.Blue(), color.Alpha());
	emisColor /= 255.0f;
	float emisMultiple = atof(emissiveMultiple->GetValue().c_str());
	float alphaValue = atof(alpha->GetValue().c_str());

	if (!multipleShapes) {
		NiShape* shape = shapes[0];
		NiShader* shader = nif->GetShader(shape);
		if (shader) {
			std::string name = shaderName->GetValue().ToStdString();
			uint32_t type = shaderType->GetSelection();
			uint32_t oldType = shader->GetShaderType();

			shader->name.get() = name;

			bool hadVertexColors = shader->HasVertexColors();
			shader->SetVertexColors(vertexColors->IsChecked());

			if (vertexColors->IsChecked() && !hadVertexColors)
				shape->SetVertexColors(true);

			// FO3/NV: Double sided is controlled by NiStencilProperty, not shader flags
			if (shader->HasType<BSShaderPPLightingProperty>()) {
				bool wantDoubleSided = doubleSided->IsChecked();
				NiStencilProperty* stencil = nif->GetStencilProperty(shape);
				if (wantDoubleSided) {
					if (!stencil) {
						auto stencilProp = std::make_unique<NiStencilProperty>();
						int stencilRef = nif->GetHeader().AddBlock(std::move(stencilProp));
						shape->propertyRefs.AddBlockRef(stencilRef);
					}
					else {
						stencil->flags = (stencil->flags & ~DRAW_MASK) | (DRAW_BOTH << DRAW_POS);
					}
				}
				else if (stencil) {
					stencil->flags = (stencil->flags & ~DRAW_MASK) | (DRAW_CCW << DRAW_POS);
				}
			}
			else {
				shader->SetDoubleSided(doubleSided->IsChecked());
			}

			if (shader->HasType<BSEffectShaderProperty>()) {
				shader->SetEmissiveColor(emisColor);
				shader->SetEmissiveMultiple(emisMultiple);
			}
			else if (shader->HasType<BSLightingShaderProperty>()) {
				auto bslsp = dynamic_cast<BSLightingShaderProperty*>(shader);
				if (bslsp) {
					bslsp->SetShaderType(type);

					if (oldType != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && type == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP) {
						// Shader type was changed to environment mapping, enable flag as well
						bslsp->SetEnvironmentMapping(true);
					}
					else if (oldType == BSLightingShaderPropertyShaderType::BSLSP_ENVMAP && type != BSLightingShaderPropertyShaderType::BSLSP_ENVMAP) {
						// Shader type was changed away from environment mapping, disable flag as well
						bslsp->SetEnvironmentMapping(false);
					}

					bslsp->SetSpecularColor(specColor);
					bslsp->SetSpecularStrength(specStrength);
					bslsp->SetGlossiness(specPower);

					bslsp->SetEmissiveColor(emisColor);
					bslsp->SetEmissiveMultiple(emisMultiple);

					bslsp->SetAlpha(alphaValue);

					// Advanced properties
					auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
					if (bssp) {
						bssp->uvOffset.u = atof(uvOffsetU->GetValue().c_str());
						bssp->uvOffset.v = atof(uvOffsetV->GetValue().c_str());
						bssp->uvScale.u = atof(uvScaleU->GetValue().c_str());
						bssp->uvScale.v = atof(uvScaleV->GetValue().c_str());
						bssp->environmentMapScale = atof(environmentMapScale->GetValue().c_str());
					}

					bslsp->textureClampMode = static_cast<TexClampMode>(textureClampMode->GetSelection());
					bslsp->refractionStrength = atof(refractionStrength->GetValue().c_str());
					bslsp->softlighting = atof(lightingEffect1->GetValue().c_str());
					bslsp->rimlightPower = atof(lightingEffect2->GetValue().c_str());

					wxColour stc = skinTintColor->GetColour();
					bslsp->skinTintColor = Vector3(stc.Red() / 255.0f, stc.Green() / 255.0f, stc.Blue() / 255.0f);

					wxColour htc = hairTintColor->GetColour();
					bslsp->hairTintColor = Vector3(htc.Red() / 255.0f, htc.Green() / 255.0f, htc.Blue() / 255.0f);

					bslsp->maxPasses = atof(parallaxMaxPasses->GetValue().c_str());
					bslsp->scale = atof(parallaxScale->GetValue().c_str());
					bslsp->parallaxInnerLayerThickness = atof(parallaxInnerLayerThickness->GetValue().c_str());
					bslsp->parallaxRefractionScale = atof(parallaxRefractionScale->GetValue().c_str());
					bslsp->parallaxInnerLayerTextureScale.u = atof(parallaxInnerLayerTexScaleU->GetValue().c_str());
					bslsp->parallaxInnerLayerTextureScale.v = atof(parallaxInnerLayerTexScaleV->GetValue().c_str());
					bslsp->parallaxEnvmapStrength = atof(parallaxEnvmapStrength->GetValue().c_str());

					bslsp->sparkleParameters.r = atof(sparkleParamsR->GetValue().c_str());
					bslsp->sparkleParameters.g = atof(sparkleParamsG->GetValue().c_str());
					bslsp->sparkleParameters.b = atof(sparkleParamsB->GetValue().c_str());
					bslsp->sparkleParameters.a = atof(sparkleParamsA->GetValue().c_str());

					bslsp->eyeCubemapScale = atof(eyeCubemapScale->GetValue().c_str());
					bslsp->eyeLeftReflectionCenter.x = atof(eyeLeftReflectX->GetValue().c_str());
					bslsp->eyeLeftReflectionCenter.y = atof(eyeLeftReflectY->GetValue().c_str());
					bslsp->eyeLeftReflectionCenter.z = atof(eyeLeftReflectZ->GetValue().c_str());
					bslsp->eyeRightReflectionCenter.x = atof(eyeRightReflectX->GetValue().c_str());
					bslsp->eyeRightReflectionCenter.y = atof(eyeRightReflectY->GetValue().c_str());
					bslsp->eyeRightReflectionCenter.z = atof(eyeRightReflectZ->GetValue().c_str());

					if (version.Stream() >= 130) {
						bslsp->SetWetMaterialName(wetMaterialPath->GetValue().ToStdString());
						bslsp->subsurfaceRolloff = atof(subsurfaceRolloff->GetValue().c_str());
						bslsp->backlightPower = atof(backlightPower->GetValue().c_str());
						bslsp->grayscaleToPaletteScale = atof(grayscaleToPaletteScale->GetValue().c_str());
						bslsp->fresnelPower = atof(fresnelPower->GetValue().c_str());
						bslsp->wetnessSpecScale = atof(wetnessSpecScale->GetValue().c_str());
						bslsp->wetnessSpecPower = atof(wetnessSpecPower->GetValue().c_str());
						bslsp->wetnessMinVar = atof(wetnessMinVar->GetValue().c_str());
						bslsp->wetnessEnvmapScale = atof(wetnessEnvMapScale->GetValue().c_str());
						bslsp->wetnessFresnelPower = atof(wetnessFresnelPower->GetValue().c_str());
						bslsp->wetnessMetalness = atof(wetnessMetalness->GetValue().c_str());
					}
				}
			}
			else if (shader->HasType<BSShaderPPLightingProperty>()) {
				switch (type) {
					case 0: type = BSShaderType::SHADER_TALL_GRASS; break;
					case 1: type = BSShaderType::SHADER_DEFAULT; break;
					case 2: type = BSShaderType::SHADER_SKY; break;
					case 3: type = BSShaderType::SHADER_SKIN; break;
					case 4: type = BSShaderType::SHADER_WATER; break;
					case 5: type = BSShaderType::SHADER_LIGHTING30; break;
					case 6: type = BSShaderType::SHADER_TILE; break;
					case 7: type = BSShaderType::SHADER_NOLIGHTING; break;
				}

				shader->SetShaderType(type);

				auto bspplp = dynamic_cast<BSShaderPPLightingProperty*>(shader);
				if (bspplp) {
					auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
					if (bssp) {
						bssp->environmentMapScale = atof(environmentMapScale->GetValue().c_str());
						bssp->shadingFlags = shadingType->GetSelection() == 1 ? SHADING_SMOOTH : SHADING_HARD;
					}

					auto* bsslp = dynamic_cast<BSShaderLightingProperty*>(shader);
					if (bsslp)
						bsslp->textureClampMode = static_cast<TexClampMode>(textureClampMode->GetSelection());

					bspplp->refractionStrength = atof(refractionStrength->GetValue().c_str());
					bspplp->refractionFirePeriod = atoi(refractionFirePeriod->GetValue().c_str());
					bspplp->parallaxMaxPasses = atof(parallaxMaxPasses->GetValue().c_str());
					bspplp->parallaxScale = atof(parallaxScale->GetValue().c_str());
				}
			}

			// Save shader flags
			auto* bssp = dynamic_cast<BSShaderProperty*>(shader);
			if (bssp) {
				uint32_t sf1 = 0;
				uint32_t sf2 = 0;
				for (unsigned int i = 0; i < shaderFlags1List->GetCount(); i++) {
					if (shaderFlags1List->IsChecked(i))
						sf1 |= (static_cast<uint32_t>(1) << i);
				}
				for (unsigned int i = 0; i < shaderFlags2List->GetCount(); i++) {
					if (shaderFlags2List->IsChecked(i))
						sf2 |= (static_cast<uint32_t>(1) << i);
				}
				bssp->shaderFlags1 = sf1;
				bssp->shaderFlags2 = sf2;
			}
		}

		NiMaterialProperty* material = nif->GetMaterialProperty(shape);
		if (material) {
			material->SetSpecularColor(specColor);
			material->SetGlossiness(specPower);

			material->SetEmissiveColor(emisColor);
			material->SetEmissiveMultiple(emisMultiple);
			material->SetAlpha(alphaValue);
		}

		NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
		if (alphaProp) {
			alphaProp->threshold = atoi(alphaThreshold->GetValue().c_str());

			// Rebuild flags from all controls
			uint16_t flags = 0;

			if (alphaBlend->IsChecked())
				flags |= 1;

			int srcBlend = alphaSrcBlend->GetSelection();
			if (srcBlend >= 0)
				flags |= (static_cast<uint16_t>(srcBlend) & 0xF) << 1;

			int destBlend = alphaDestBlend->GetSelection();
			if (destBlend >= 0)
				flags |= (static_cast<uint16_t>(destBlend) & 0xF) << 5;

			if (alphaTest->IsChecked())
				flags |= 1 << 9;

			int testFunc = alphaTestFunc->GetSelection();
			if (testFunc >= 0)
				flags |= (static_cast<uint16_t>(testFunc) & 0x7) << 10;

			if (alphaNoSorter->IsChecked())
				flags |= 1 << 13;

			alphaProp->flags = flags;

			if (shader) {
				bool hadVertexAlpha = shader->HasVertexAlpha();
				shader->SetVertexAlpha(vertexAlpha->IsChecked());

				if (vertexAlpha->IsChecked() && !hadVertexAlpha) {
					shader->SetVertexColors(true);
					shape->SetVertexColors(true);
				}
			}
		}

		for (size_t i = 0; i < extraDataIndices.size(); i++) {
			wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + i, this));
			wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + i, this));
			if (!extraDataName || !extraDataValue)
				continue;

			auto extraData = nif->GetHeader().GetBlock<NiExtraData>(extraDataIndices[i]);
			if (extraData) {
				extraData->name.get() = extraDataName->GetValue().ToStdString();

				if (extraData->HasType<NiStringExtraData>()) {
					auto stringExtraData = static_cast<NiStringExtraData*>(extraData);
					stringExtraData->stringData.get() = extraDataValue->GetValue().ToStdString();
				}
				else if (extraData->HasType<NiIntegerExtraData>()) {
					auto intExtraData = static_cast<NiIntegerExtraData*>(extraData);
					unsigned long val = 0;
					if (extraDataValue->GetValue().ToULong(&val))
						intExtraData->integerData = val;
				}
				else if (extraData->HasType<NiFloatExtraData>()) {
					auto floatExtraData = static_cast<NiFloatExtraData*>(extraData);
					double val = 0.0;
					if (extraDataValue->GetValue().ToDouble(&val))
						floatExtraData->floatData = (float)val;
				}
			}
		}
	}

	for (auto& shape : shapes) {
		auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
		if (bsTriShape) {
			wxCheckBoxState fullPrecisionState = fullPrecision->Get3StateValue();
			if (fullPrecision->IsEnabled() && fullPrecisionState != wxCheckBoxState::wxCHK_UNDETERMINED) {
				if (nif->GetHeader().GetVersion().Stream() != 100)
					bsTriShape->SetFullPrecision(fullPrecisionState == wxCheckBoxState::wxCHK_CHECKED);
			}

			wxCheckBoxState subIndexState = subIndex->Get3StateValue();
			if (subIndex->IsEnabled() && subIndexState != wxCheckBoxState::wxCHK_UNDETERMINED) {
				bool hasSubIndex = shape->HasType<BSSubIndexTriShape>();

				if (version.Stream() >= 130) {
					if (subIndexState == wxCheckBoxState::wxCHK_CHECKED && !hasSubIndex) {
						auto bsSITS = std::make_unique<BSSubIndexTriShape>();
						*static_cast<BSTriShape*>(bsSITS.get()) = *bsTriShape;
						bsSITS->SetDefaultSegments();
						bsSITS->name.get() = bsTriShape->name.get();

						shape = bsSITS.get();
						os->UpdateShapeReference(bsTriShape, shape);
						nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsSITS));
					}
					else if (subIndexState == wxCheckBoxState::wxCHK_UNCHECKED && hasSubIndex) {
						auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
						bsTS->name.get() = bsTriShape->name.get();

						shape = bsTS.get();
						os->UpdateShapeReference(bsTriShape, shape);
						nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
					}
				}
			}

			wxCheckBoxState dynamicState = dynamic->Get3StateValue();
			if (dynamic->IsEnabled() && dynamicState != wxCheckBoxState::wxCHK_UNDETERMINED) {
				bool hasDynamic = shape->HasType<BSDynamicTriShape>();

				if (version.Stream() == 100) {
					if (dynamicState == wxCheckBoxState::wxCHK_CHECKED && !hasDynamic) {
						auto bsDTS = std::make_unique<BSDynamicTriShape>();
						*static_cast<BSTriShape*>(bsDTS.get()) = *bsTriShape;
						bsDTS->name.get() = bsTriShape->name.get();

						bsDTS->vertexDesc.RemoveFlag(VF_VERTEX);
						bsDTS->vertexDesc.SetFlag(VF_FULLPREC);

						bsDTS->CalcDynamicData();
						bsDTS->CalcDataSizes(nif->GetHeader().GetVersion());

						shape = bsDTS.get();
						os->UpdateShapeReference(bsTriShape, shape);
						nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsDTS));
					}
					else if (dynamicState == wxCheckBoxState::wxCHK_UNCHECKED && hasDynamic) {
						auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
						bsTS->name.get() = bsTriShape->name.get();

						bsTS->vertexDesc.SetFlag(VF_VERTEX);
						bsTS->vertexDesc.RemoveFlag(VF_FULLPREC);

						bsTS->CalcDataSizes(nif->GetHeader().GetVersion());

						shape = bsTS.get();
						os->UpdateShapeReference(bsTriShape, shape);
						nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
					}
				}
			}
		}

		wxCheckBoxState skinnedState = skinned->Get3StateValue();
		if (skinned->IsEnabled() && skinnedState != wxCheckBoxState::wxCHK_UNDETERMINED) {
			if (skinnedState == wxCheckBoxState::wxCHK_CHECKED) {
				os->project->CreateSkinning(shape);
			}
			else {
				os->project->RemoveSkinning(shape);
				os->UpdateAnimationGUI();
			}
		}

		if (!multipleShapes) {
			if (!newTransform.IsNearlyEqualTo(oldTransform)) {
				if (cbTransformGeo->IsChecked())
					os->project->ApplyTransformToShapeGeometry(shape, newTransform.InverseTransform().ComposeTransforms(oldTransform));

				os->project->GetWorkAnim()->SetTransformShapeToGlobal(shape, newTransform);
			}
		}
	}

	os->SetPendingChanges();
}
