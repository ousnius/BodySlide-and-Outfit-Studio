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
	EVT_TEXT(XRCID("textScale"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textX"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textY"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textZ"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRX"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRY"), ShapeProperties::OnTransChanged)
	EVT_TEXT(XRCID("textRZ"), ShapeProperties::OnTransChanged)
wxEND_EVENT_TABLE()

ShapeProperties::ShapeProperties(wxWindow* parent, NifFile* refNif, NiShape* refShape) {
	wxXmlResource *xrc = wxXmlResource::Get();
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

	SetDoubleBuffered(true);
	CenterOnParent();

	os = (OutfitStudioFrame*)parent;
	nif = refNif;
	shape = refShape;

	pgShader = XRCCTRL(*this, "pgShader", wxPanel);
	lbShaderName = XRCCTRL(*this, "lbShaderName", wxStaticText);
	shaderName = XRCCTRL(*this, "shaderName", wxTextCtrl);
	btnMaterialChooser = XRCCTRL(*this, "btnMaterialChooser", wxButton);
	shaderType = XRCCTRL(*this, "shaderType", wxChoice);
	specularColor = XRCCTRL(*this, "specularColor", wxColourPickerCtrl);
	specularStrength = XRCCTRL(*this, "specularStrength", wxTextCtrl);
	specularPower = XRCCTRL(*this, "specularPower", wxTextCtrl);
	emissiveColor = XRCCTRL(*this, "emissiveColor", wxColourPickerCtrl);
	emissiveMultiple = XRCCTRL(*this, "emissiveMultiple", wxTextCtrl);
	vertexColors = XRCCTRL(*this, "vertexColors", wxCheckBox);
	btnAddShader = XRCCTRL(*this, "btnAddShader", wxButton);
	btnRemoveShader = XRCCTRL(*this, "btnRemoveShader", wxButton);
	btnSetTextures = XRCCTRL(*this, "btnSetTextures", wxButton);

	alphaThreshold = XRCCTRL(*this, "alphaThreshold", wxTextCtrl);
	vertexAlpha = XRCCTRL(*this, "vertexAlpha", wxCheckBox);
	btnAddTransparency = XRCCTRL(*this, "btnAddTransparency", wxButton);
	btnRemoveTransparency = XRCCTRL(*this, "btnRemoveTransparency", wxButton);

	btnCopyShaderFromShape = XRCCTRL(*this, "btnCopyShaderFromShape", wxButton);

	fullPrecision = XRCCTRL(*this, "fullPrecision", wxCheckBox);
	subIndex = XRCCTRL(*this, "subIndex", wxCheckBox);
	skinned = XRCCTRL(*this, "skinned", wxCheckBox);
	dynamic = XRCCTRL(*this, "dynamic", wxCheckBox);

	pgExtraData = XRCCTRL(*this, "pgExtraData", wxPanel);
	extraDataGrid = (wxFlexGridSizer*)XRCCTRL(*this, "btnAddExtraData", wxButton)->GetContainingSizer();

	textScale = XRCCTRL(*this, "textScale", wxTextCtrl);
	textX = XRCCTRL(*this, "textX", wxTextCtrl);
	textY = XRCCTRL(*this, "textY", wxTextCtrl);
	textZ = XRCCTRL(*this, "textZ", wxTextCtrl);
	textRX = XRCCTRL(*this, "textRX", wxTextCtrl);
	textRY = XRCCTRL(*this, "textRY", wxTextCtrl);
	textRZ = XRCCTRL(*this, "textRZ", wxTextCtrl);
	cbTransformGeo = XRCCTRL(*this, "cbTransformGeo", wxCheckBox);

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		lbShaderName->SetLabel(_("Material"));
		btnMaterialChooser->Show();
		pgShader->Layout();
	}

	GetShader();
	GetTransparency();
	GetGeometry();
	GetExtraData();
	GetCoordTrans();
}

ShapeProperties::~ShapeProperties() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ShapeProperties.xrc");
}

void ShapeProperties::GetShader() {
	NiShader* shader = nif->GetShader(shape);

	if (!shader) {
		btnAddShader->Enable();
		btnRemoveShader->Disable();
		btnMaterialChooser->Disable();
		btnSetTextures->Disable();
		shaderName->Disable();
		shaderType->Disable();
		specularColor->Disable();
		specularStrength->Disable();
		specularPower->Disable();
		emissiveColor->Disable();
		emissiveMultiple->Disable();
		vertexColors->Disable();
		vertexAlpha->Disable();
	}
	else {
		btnAddShader->Disable();
		btnRemoveShader->Enable();
		btnMaterialChooser->Enable();
		btnSetTextures->Enable();
		shaderName->Enable();
		shaderType->Enable();
		specularColor->Enable();
		specularStrength->Enable();
		specularPower->Enable();
		emissiveColor->Enable();
		emissiveMultiple->Enable();
		vertexColors->Enable();
		vertexAlpha->Enable();

		currentVertexColors = shader->HasVertexColors();
		currentVertexAlpha = shader->HasVertexAlpha();

		shaderName->SetValue(shader->name.get());
		vertexColors->SetValue(currentVertexColors);
		vertexAlpha->SetValue(currentVertexAlpha);

		Color4 color;
		Vector3 colorVec;
		if (shader->HasType<BSEffectShaderProperty>()) {
			specularColor->Disable();
			specularStrength->Disable();
			specularPower->Disable();

			color = shader->GetEmissiveColor() * 255.0f;
			emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
			emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
		}
		else if (shader->HasType<BSLightingShaderProperty>()) {
			colorVec = shader->GetSpecularColor() * 255.0f;
			specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
			specularStrength->SetValue(wxString::Format("%.4f", shader->GetSpecularStrength()));
			specularPower->SetValue(wxString::Format("%.4f", shader->GetGlossiness()));

			color = shader->GetEmissiveColor() * 255.0f;
			emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
			emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
		}
		else if (shader->HasType<BSShaderPPLightingProperty>()) {
			NiMaterialProperty* material = nif->GetMaterialProperty(shape);
			if (material) {
				colorVec = material->GetSpecularColor() * 255.0f;
				specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
				specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));

				color = material->GetEmissiveColor() * 255.0f;
				emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
				emissiveMultiple->SetValue(wxString::Format("%.4f", material->GetEmissiveMultiple()));
			}
		}
	}

	GetShaderType();
}


void ShapeProperties::GetShaderType() {
	shaderType->Disable();
	shaderType->Clear();

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
		}
	}
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
	AddShader();

	btnAddShader->Disable();
	btnRemoveShader->Enable();
	btnMaterialChooser->Enable();
	btnSetTextures->Enable();
	shaderName->Enable();
	shaderType->Enable();
	specularColor->Enable();
	specularStrength->Enable();
	specularPower->Enable();
	emissiveColor->Enable();
	emissiveMultiple->Enable();
	vertexColors->Enable();

	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
	if (alphaProp) {
		vertexAlpha->Enable();
	}
}

void ShapeProperties::AddShader() {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	std::unique_ptr<NiShader> newShader = nullptr;
	std::unique_ptr<NiMaterialProperty> newMaterial = nullptr;

	switch (targetGame) {
	case FO3:
	case FONV:
		newShader = std::make_unique<BSShaderPPLightingProperty>();
		shape->propertyRefs.AddBlockRef(nif->GetHeader().AddBlock(std::move(newShader)));

		newMaterial = std::make_unique<NiMaterialProperty>();
		shape->propertyRefs.AddBlockRef(nif->GetHeader().AddBlock(std::move(newMaterial)));
		break;

	default:
		newShader = std::make_unique<BSLightingShaderProperty>(nif->GetHeader().GetVersion());
		shape->ShaderPropertyRef()->index = nif->GetHeader().AddBlock(std::move(newShader));
	}

	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		auto nifTexSet = std::make_unique<BSShaderTextureSet>(nif->GetHeader().GetVersion());
		shader->TextureSetRef()->index = nif->GetHeader().AddBlock(std::move(nifTexSet));
	}

	AssignDefaultTexture();
	GetShader();
}

void ShapeProperties::OnRemoveShader(wxCommandEvent& WXUNUSED(event)) {
	RemoveShader();
}

void ShapeProperties::RemoveShader() {
	nif->DeleteShader(shape);
	AssignDefaultTexture();
	GetShader();
	GetTransparency();
}

void ShapeProperties::OnSetTextures(wxCommandEvent& WXUNUSED(event)) {
	NiShader* shader = nif->GetShader(shape);
	if (!shader)
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
		stTexGrid->SetColSize(0, 350);
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

		int blockType = 0;
		for (int i = 0; i < 10; i++) {
			std::string texPath;
			blockType = nif->GetTextureSlot(shader, texPath, i);
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

		if (dlg.ShowModal() == wxID_OK) {
			auto dataPath = Config["GameDataPath"];
			std::vector<std::string> texFiles(10);
			for (int i = 0; i < 10; i++) {
				std::string texPath = stTexGrid->GetCellValue(i, 0);
				std::string texPath_bs = ToBackslashes(texPath);
				nif->SetTextureSlot(shader, texPath_bs, i);

				if (!texPath.empty())
					texFiles[i] = dataPath + texPath;
			}

			nif->TrimTexturePaths();

			os->project->SetTextures(shape, texFiles);
			os->glView->SetMeshTextures(shape->name.get(), texFiles, false, MaterialFile(), true);
			os->glView->Render();
		}
	}
}

void ShapeProperties::AssignDefaultTexture() {
	os->project->SetTextures(shape);
	os->glView->Render();
}

void ShapeProperties::GetTransparency() {
	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
	if (alphaProp) {
		alphaThreshold->SetValue(wxString::Format("%d", alphaProp->threshold));
		alphaThreshold->Enable();
		btnAddTransparency->Disable();
		btnRemoveTransparency->Enable();

		NiShader* shader = nif->GetShader(shape);
		if (shader) {
			vertexAlpha->Enable();
		}
	}
	else {
		alphaThreshold->Disable();
		vertexAlpha->Disable();
		btnAddTransparency->Enable();
		btnRemoveTransparency->Disable();
	}
}

void ShapeProperties::OnAddTransparency(wxCommandEvent& WXUNUSED(event)) {
	AddTransparency();
}

void ShapeProperties::AddTransparency() {
	auto alphaProp = std::make_unique<NiAlphaProperty>();
	nif->AssignAlphaProperty(shape, std::move(alphaProp));
	GetTransparency();
}

void ShapeProperties::OnRemoveTransparency(wxCommandEvent& WXUNUSED(event)) {
	RemoveTransparency();
}

void ShapeProperties::OnCopyShaderFromShape(wxCommandEvent& WXUNUSED(event)) {
	wxArrayString choices;

	auto shapes = nif->GetShapes();
	for (auto &s : shapes) {
		if (s != shape) {
			choices.Add(wxString::FromUTF8(s->name.get()));
		}
	}

	if (choices.GetCount() > 0) {
		std::string shapeName = wxGetSingleChoice(_("Please choose a shape to copy from"), _("Choose shape"), choices, 0, this).ToUTF8();
		if (shapeName.empty())
			return;

		auto shapeChoice = nif->FindBlockByName<NiShape>(shapeName);
		if (shapeChoice) {
			auto shapeShader = nif->GetShader(shape);
			auto shapeChoiceShader = nif->GetShader(shapeChoice);

			if (!shapeShader && shapeChoiceShader) {
				AddShader();
				shapeShader = nif->GetShader(shape);

				auto shapeAlphaProp = nif->GetAlphaProperty(shape);
				auto shapeChoiceAlphaProp = nif->GetAlphaProperty(shapeChoice);
				if (!shapeAlphaProp && shapeChoiceAlphaProp)
					AddTransparency();
			}
			else if (shapeShader && !shapeChoiceShader)
				RemoveShader();

			auto oldShape = shape;
			shape = shapeChoice;

			GetShader();
			GetTransparency();

			shape = oldShape;

			if (shapeChoiceShader) {
				// Copy texture paths
				for (int i = 0; i < 10; i++) {
					std::string texPath;
					nif->GetTextureSlot(shapeChoiceShader, texPath, i);
					nif->SetTextureSlot(shapeShader, texPath, i);
				}

				auto texturePaths = os->project->GetShapeTextures(shapeChoice);
				os->project->SetTextures(shape, texturePaths);
				os->glView->Render();
			}
		}
	}
}

void ShapeProperties::RemoveTransparency() {
	nif->RemoveAlphaProperty(shape);
	GetTransparency();
}


void ShapeProperties::GetGeometry() {
	skinned->SetValue(shape->IsSkinned());

	currentSubIndex = shape->HasType<BSSubIndexTriShape>();
	subIndex->SetValue(currentSubIndex);

	currentDynamic = shape->HasType<BSDynamicTriShape>();
	dynamic->SetValue(currentDynamic);

	BSTriShape* bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		if (nif->GetHeader().GetVersion().Stream() == 100) {
			fullPrecision->SetValue(true);
			fullPrecision->Enable(false);
		}
		else {
			fullPrecision->SetValue(bsTriShape->IsFullPrecision());
			fullPrecision->Enable(bsTriShape->CanChangePrecision());
		}

		auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
		
		subIndex->Enable(targetGame == FO4 || targetGame == FO4VR || targetGame == FO76);
		dynamic->Enable(targetGame == SKYRIMSE || targetGame == SKYRIMVR);
	}
}


void ShapeProperties::GetExtraData() {
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
			AddExtraData(extraData, true);
		}
	}
}

void ShapeProperties::OnAddExtraData(wxCommandEvent& WXUNUSED(event)) {
	NiStringExtraData extraDataTemp;
	AddExtraData(&extraDataTemp);
}

void ShapeProperties::AddExtraData(NiExtraData* extraData, bool uiOnly) {
	if (!uiOnly) {
		auto newExtraData = extraData->Clone();
		int index = nif->AssignExtraData(shape, std::move(newExtraData));
		extraDataIndices.push_back(index);
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
	ChangeExtraDataType(event.GetId() - 2000);
}

void ShapeProperties::ChangeExtraDataType(int id) {
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

	if (extraDataResult)
		extraDataIndices[id] = nif->AssignExtraData(shape, std::move(extraDataResult));
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
}

void ShapeProperties::GetCoordTrans() {
	oldXformGlobalToSkin = os->project->GetWorkAnim()->shapeSkinning[shape->name.get()].xformGlobalToSkin;
	newXformGlobalToSkin = oldXformGlobalToSkin;
	Vector3 rotvec = RotMatToVec(newXformGlobalToSkin.rotation);

	textScale->ChangeValue(wxString::Format("%.10f", newXformGlobalToSkin.scale));
	textX->ChangeValue(wxString::Format("%.10f", newXformGlobalToSkin.translation.x));
	textY->ChangeValue(wxString::Format("%.10f", newXformGlobalToSkin.translation.y));
	textZ->ChangeValue(wxString::Format("%.10f", newXformGlobalToSkin.translation.z));
	textRX->ChangeValue(wxString::Format("%.10f", rotvec.x));
	textRY->ChangeValue(wxString::Format("%.10f", rotvec.y));
	textRZ->ChangeValue(wxString::Format("%.10f", rotvec.z));

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

	newXformGlobalToSkin.scale = scale;
	newXformGlobalToSkin.translation.x = x;
	newXformGlobalToSkin.translation.y = y;
	newXformGlobalToSkin.translation.z = z;
	newXformGlobalToSkin.rotation = RotVecToMat(Vector3(rx, ry, rz));
	cbTransformGeo->Enable(!newXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin));
}

void ShapeProperties::RefreshMesh() {
	os->project->SetTextures(shape);
	os->MeshFromProj(shape, true);
	os->UpdateActiveShapeUI();
}

void ShapeProperties::OnApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyChanges();
	RefreshMesh();
	EndModal(wxID_OK);
}

void ShapeProperties::ApplyChanges() {
	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		std::string name = shaderName->GetValue();
		uint32_t type = shaderType->GetSelection();
		wxColour color = specularColor->GetColour();
		Vector3 specColor(color.Red(), color.Green(), color.Blue());
		specColor /= 255.0f;
		float specStrength = atof(specularStrength->GetValue().c_str());
		float specPower = atof(specularPower->GetValue().c_str());

		color = emissiveColor->GetColour();
		Color4 emisColor(color.Red(), color.Green(), color.Blue(), color.Alpha());
		emisColor /= 255.0f;
		float emisMultiple = atof(emissiveMultiple->GetValue().c_str());

		shader->name.get() = name;
		shader->SetVertexColors(vertexColors->IsChecked());

		if (vertexColors->IsChecked() && currentVertexColors != vertexColors->IsChecked())
			shape->SetVertexColors(true);

		if (shader->HasType<BSEffectShaderProperty>()) {
			shader->SetEmissiveColor(emisColor);
			shader->SetEmissiveMultiple(emisMultiple);
		}
		else if (shader->HasType<BSLightingShaderProperty>()) {
			shader->SetShaderType(type);

			shader->SetSpecularColor(specColor);
			shader->SetSpecularStrength(specStrength);
			shader->SetGlossiness(specPower);

			shader->SetEmissiveColor(emisColor);
			shader->SetEmissiveMultiple(emisMultiple);
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

			NiMaterialProperty* material = nif->GetMaterialProperty(shape);
			if (material) {
				material->SetSpecularColor(specColor);
				material->SetGlossiness(specPower);

				material->SetEmissiveColor(emisColor);
				material->SetEmissiveMultiple(emisMultiple);
			}
		}
	}

	NiAlphaProperty* alphaProp = nif->GetAlphaProperty(shape);
	if (alphaProp) {
		alphaProp->threshold = atoi(alphaThreshold->GetValue().c_str());

		if (shader) {
			shader->SetVertexAlpha(vertexAlpha->IsChecked());

			if (vertexAlpha->IsChecked() && currentVertexAlpha != vertexAlpha->IsChecked()) {
				shader->SetVertexColors(true);
				shape->SetVertexColors(true);
			}
		}
	}

	auto bsTriShape = dynamic_cast<BSTriShape*>(shape);
	if (bsTriShape) {
		if (nif->GetHeader().GetVersion().Stream() != 100)
			bsTriShape->SetFullPrecision(fullPrecision->IsChecked());

		if ((targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) && currentSubIndex != subIndex->IsChecked()) {
			if (subIndex->IsChecked()) {
				auto bsSITS = std::make_unique<BSSubIndexTriShape>();
				*static_cast<BSTriShape*>(bsSITS.get()) = *bsTriShape;
				bsSITS->SetDefaultSegments();
				bsSITS->name.get() = bsTriShape->name.get();

				shape = bsSITS.get();
				os->UpdateShapeReference(bsTriShape, shape);
				nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsSITS));
			}
			else {
				auto bsTS = std::make_unique<BSTriShape>(*bsTriShape);
				bsTS->name.get() = bsTriShape->name.get();

				shape = bsTS.get();
				os->UpdateShapeReference(bsTriShape, shape);
				nif->GetHeader().ReplaceBlock(nif->GetBlockID(bsTriShape), std::move(bsTS));
			}
		}

		if (targetGame == SKYRIMSE && currentDynamic != dynamic->IsChecked()) {
			if (dynamic->IsChecked()) {
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
			else {
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

	if (skinned->IsChecked()) {
		nif->CreateSkinning(shape);
	}
	else {
		nif->DeleteSkinning(shape);
		os->project->GetWorkAnim()->ClearShape(shape->name.get());
		os->AnimationGUIFromProj();
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

	if (!newXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin)) {
		if (cbTransformGeo->IsChecked())
			os->project->ApplyTransformToShapeGeometry(shape, newXformGlobalToSkin.ComposeTransforms(oldXformGlobalToSkin.InverseTransform()));

		os->project->GetWorkAnim()->ChangeGlobalToSkinTransform(shape->name.get(), newXformGlobalToSkin);
		nif->SetShapeTransformGlobalToSkin(shape, newXformGlobalToSkin);
	}
}
