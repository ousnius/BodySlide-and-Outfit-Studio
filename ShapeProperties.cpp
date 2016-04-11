/*
BodySlide and Outfit Studio
Copyright (C) 2016  Caliente & ousnius
See the included LICENSE file
*/

#include "ShapeProperties.h"

wxBEGIN_EVENT_TABLE(ShapeProperties, wxDialog)
	EVT_BUTTON(XRCID("btnAddShader"), ShapeProperties::OnAddShader)
	EVT_BUTTON(XRCID("btnRemoveShader"), ShapeProperties::OnRemoveShader)
	EVT_BUTTON(XRCID("btnSetTextures"), ShapeProperties::OnSetTextures)
	EVT_BUTTON(XRCID("btnAddTransparency"), ShapeProperties::OnAddTransparency)
	EVT_BUTTON(XRCID("btnRemoveTransparency"), ShapeProperties::OnRemoveTransparency)
	EVT_BUTTON(XRCID("btnAddExtraData"), ShapeProperties::OnAddExtraData)
	EVT_BUTTON(wxID_OK, ShapeProperties::OnApply)
wxEND_EVENT_TABLE()

ShapeProperties::ShapeProperties(wxWindow* parent, NifFile* refNif, const string& shapeName) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->LoadDialog(this, parent, "dlgShapeProp");

	SetDoubleBuffered(true);
	CenterOnParent();

	os = (OutfitStudio*)parent;
	nif = refNif;
	shape = shapeName;

	shaderName = XRCCTRL(*this, "shaderName", wxTextCtrl);
	shaderType = XRCCTRL(*this, "shaderType", wxChoice);
	specularColor = XRCCTRL(*this, "specularColor", wxColourPickerCtrl);
	specularStrength = XRCCTRL(*this, "specularStrength", wxTextCtrl);
	specularPower = XRCCTRL(*this, "specularPower", wxTextCtrl);
	emissiveColor = XRCCTRL(*this, "emissiveColor", wxColourPickerCtrl);
	emissiveMultiple = XRCCTRL(*this, "emissiveMultiple", wxTextCtrl);
	btnAddShader = XRCCTRL(*this, "btnAddShader", wxButton);
	btnRemoveShader = XRCCTRL(*this, "btnRemoveShader", wxButton);
	btnSetTextures = XRCCTRL(*this, "btnSetTextures", wxButton);

	alphaThreshold = XRCCTRL(*this, "alphaThreshold", wxTextCtrl);
	btnAddTransparency = XRCCTRL(*this, "btnAddTransparency", wxButton);
	btnRemoveTransparency = XRCCTRL(*this, "btnRemoveTransparency", wxButton);

	fullPrecision = XRCCTRL(*this, "fullPrecision", wxCheckBox);

	pgExtraData = XRCCTRL(*this, "pgExtraData", wxPanel);
	extraDataGrid = (wxFlexGridSizer*)XRCCTRL(*this, "btnAddExtraData", wxButton)->GetContainingSizer();

	if (os->targetGame == FO4)
		XRCCTRL(*this, "lbShaderName", wxStaticText)->SetLabel("Material");

	GetShader();
	GetTransparency();
	GetGeometry();
	GetExtraData();
}

ShapeProperties::~ShapeProperties() {
}

void ShapeProperties::GetShader() {
	NiShader* shader = nif->GetShader(shape);

	if (!shader) {
		btnAddShader->Enable();
		btnRemoveShader->Disable();
		btnSetTextures->Disable();
		shaderName->Disable();
		shaderType->Disable();
		specularColor->Disable();
		specularStrength->Disable();
		specularPower->Disable();
		emissiveColor->Disable();
		emissiveMultiple->Disable();
	}
	else {
		shaderName->SetValue(shader->GetName());
		
		if (os->targetGame == FO4)
			currentMaterialPath = shader->GetName();

		Color4 color;
		Vector3 colorVec;
		switch (shader->blockType) {
			case BSEFFECTSHADERPROPERTY:
				specularColor->Disable();
				specularStrength->Disable();
				specularPower->Disable();

				color = shader->GetEmissiveColor() * 255.0f;
				emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
				emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
				break;
			case BSLIGHTINGSHADERPROPERTY:
				colorVec = shader->GetSpecularColor() * 255.0f;
				specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
				specularStrength->SetValue(wxString::Format("%.4f", shader->GetSpecularStrength()));
				specularPower->SetValue(wxString::Format("%.4f", shader->GetGlossiness()));

				color = shader->GetEmissiveColor() * 255.0f;
				emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
				emissiveMultiple->SetValue(wxString::Format("%.4f", shader->GetEmissiveMultiple()));
				break;
			case BSSHADERPPLIGHTINGPROPERTY:
				NiMaterialProperty* material = nif->GetMaterialProperty(shape);
				if (!material)
					break;
				
				colorVec = material->GetSpecularColor() * 255.0f;
				specularColor->SetColour(wxColour(colorVec.x, colorVec.y, colorVec.z));
				specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));

				color = material->GetEmissiveColor() * 255.0f;
				emissiveColor->SetColour(wxColour(color.r, color.g, color.b, color.a));
				emissiveMultiple->SetValue(wxString::Format("%.4f", material->GetEmissiveMultiple()));
				break;
		}
	}

	GetShaderType();
}


void ShapeProperties::GetShaderType() {
	shaderType->Disable();
	shaderType->Clear();

	uint type;
	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		switch (shader->blockType) {
			case BSLIGHTINGSHADERPROPERTY:
				type = shader->GetType();
				if (type > BSLightingShaderPropertyShaderType::WorldLODMultitexture)
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
				break;
			case BSSHADERPPLIGHTINGPROPERTY:
				type = shader->GetType();
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
				break;
		}
	}
}

void ShapeProperties::OnAddShader(wxCommandEvent& WXUNUSED(event)) {
	AddShader();

	btnAddShader->Disable();
	btnRemoveShader->Enable();
	btnSetTextures->Enable();
	shaderName->Enable();
	shaderType->Enable();
	specularColor->Enable();
	specularStrength->Enable();
	specularPower->Enable();
	emissiveColor->Enable();
	emissiveMultiple->Enable();
}

void ShapeProperties::AddShader() {
	NiTriBasedGeom* geom = nif->geomForName(shape);
	if (geom) {
		switch (os->targetGame) {
			case FO3:
			case FONV: {
				BSShaderPPLightingProperty* shader = new BSShaderPPLightingProperty(nif->hdr);
				geom->propertiesRef.push_back(nif->AddBlock(shader, "BSShaderPPLightingProperty"));
				geom->numProperties++;

				NiMaterialProperty* material = new NiMaterialProperty(nif->hdr);
				geom->propertiesRef.push_back(nif->AddBlock(material, "NiMaterialProperty"));
				geom->numProperties++;
				break;
			}
			case SKYRIM:
			default: {
				BSLightingShaderProperty* shader = new BSLightingShaderProperty(nif->hdr);
				geom->propertiesRef1 = nif->AddBlock(shader, "BSLightingShaderProperty");
			}
		}
	}
	else {
		BSTriShape* siTriShape = nif->geomForNameF4(shape);
		if (!siTriShape)
			return;

		switch (os->targetGame) {
			case FO4:
			default: {
				BSLightingShaderProperty* shader = new BSLightingShaderProperty(nif->hdr);
				siTriShape->shaderPropertyRef = nif->AddBlock(shader, "BSLightingShaderProperty");
			}
		}
	}


	NiShader* shader = nif->GetShader(shape);
	if (shader) {
		BSShaderTextureSet* nifTexSet = new BSShaderTextureSet(nif->hdr);
		shader->SetTextureSetRef(nif->AddBlock(nifTexSet, "BSShaderTextureSet"));
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
	wxDialog dlg;
	string texPath;
	string oDispPath;
	string nDispPath;
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
			blockType = nif->GetTextureForShape(shape, texPath, i);
			if (!blockType)
				continue;

			stTexGrid->SetCellValue(i, 0, texPath);
		}

		if (blockType == BSEFFECTSHADERPROPERTY) {
			stTexGrid->SetRowLabelValue(0, "Source");
			stTexGrid->SetRowLabelValue(1, "Greyscale");
			stTexGrid->HideRow(2);
			stTexGrid->HideRow(3);
			stTexGrid->HideRow(4);
			stTexGrid->HideRow(5);
			stTexGrid->HideRow(6);
			stTexGrid->HideRow(7);
			stTexGrid->HideRow(8);
			stTexGrid->HideRow(9);
		}

		oDispPath = os->project->GetShapeTexture(shape);
		XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->SetPath(oDispPath);
		XRCCTRL(dlg, "btApplyDiffuse", wxButton)->Bind(wxEVT_BUTTON, &ShapeProperties::OnApplyDiffuse, this);

		if (dlg.ShowModal() == wxID_OK) {
			nDispPath = XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->GetPath();
			if (nDispPath != oDispPath) {
				os->project->SetTexture(shape, nDispPath);
				os->glView->SetMeshTexture(shape, nDispPath, nif->IsShaderSkin(shape));
			}

			for (int i = 0; i < 10; i++) {
				texPath = stTexGrid->GetCellValue(i, 0);
				nif->SetTextureForShape(shape, texPath, i);
			}
			nif->TrimTexturePaths();

			os->glView->Render();
		}
	}
}

void ShapeProperties::OnApplyDiffuse(wxCommandEvent& event) {
	wxDialog* dlg = (wxDialog*)((wxWindow*)event.GetEventObject())->GetParent();
	if (!dlg)
		return;

	wxFilePickerCtrl* dispPath = (wxFilePickerCtrl*)dlg->FindWindow("stDisplayTexture");
	wxGrid* texGrid = (wxGrid*)dlg->FindWindow("stTexGrid");
	if (!dispPath || !texGrid)
		return;

	string tex = texGrid->GetCellValue(0, 0);
	if (!tex.empty()) {
		string newTex = os->appConfig["GameDataPath"] + tex;
		dispPath->SetPath(newTex);
	}
}

void ShapeProperties::AssignDefaultTexture() {
	string texNoImg = os->appConfig["GameDataPath"] + "noimg.dds";
	os->project->SetTexture(shape, "_AUTO_");
	os->glView->SetMeshTexture(shape, texNoImg, nif->IsShaderSkin(shape));
	os->glView->Render();
}

void ShapeProperties::GetTransparency() {
	ushort flags;
	byte threshold;

	if (nif->GetAlphaForShape(shape, flags, threshold)) {
		alphaThreshold->SetValue(wxString::Format("%d", threshold));
		alphaThreshold->Enable();
		btnAddTransparency->Disable();
		btnRemoveTransparency->Enable();
	}
	else {
		alphaThreshold->Disable();
		btnAddTransparency->Enable();
		btnRemoveTransparency->Disable();
	}
}

void ShapeProperties::OnAddTransparency(wxCommandEvent& WXUNUSED(event)) {
	AddTransparency();
}

void ShapeProperties::AddTransparency() {
	nif->SetAlphaForShape(shape);
	GetTransparency();
}

void ShapeProperties::OnRemoveTransparency(wxCommandEvent& WXUNUSED(event)) {
	RemoveTransparency();
}

void ShapeProperties::RemoveTransparency() {
	nif->DeleteAlpha(shape);
	GetTransparency();
}


void ShapeProperties::GetGeometry() {
	NiTriBasedGeom* geom = nif->geomForName(shape);
	if (geom) {
		// No properties yet
	}
	else {
		BSTriShape* bsGeom = nif->geomForNameF4(shape);
		if (!bsGeom)
			return;

		fullPrecision->SetValue(bsGeom->IsFullPrecision());
		fullPrecision->Enable(bsGeom->CanChangePrecision());
	}
}


void ShapeProperties::GetExtraData() {
	for (int i = 0; i < extraDataIndices.size(); i++) {
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

	NiTriBasedGeom* geom = nif->geomForName(shape);
	if (geom) {
		for (int i = 0; i < geom->numExtraData; i++) {
			NiExtraData* extraData = dynamic_cast<NiExtraData*>(nif->GetBlock(geom->extraDataRef[i]));
			if (extraData) {
				extraDataIndices.push_back(geom->extraDataRef[i]);
				AddExtraData(extraData, true);
			}
		}
	}
	else {
		BSTriShape* siTriShape = nif->geomForNameF4(shape);
		if (!siTriShape)
			return;

		for (int i = 0; i < siTriShape->numExtraData; i++) {
			NiExtraData* extraData = dynamic_cast<NiExtraData*>(nif->GetBlock(siTriShape->extraDataRef[i]));
			if (extraData) {
				extraDataIndices.push_back(siTriShape->extraDataRef[i]);
				AddExtraData(extraData, true);
			}
		}
	}
}

void ShapeProperties::OnAddExtraData(wxCommandEvent& WXUNUSED(event)) {
	NiStringExtraData extraDataTemp(nif->hdr);
	AddExtraData(&extraDataTemp);
}

void ShapeProperties::AddExtraData(const NiExtraData* extraData, bool uiOnly) {
	if (!uiOnly) {
		if (extraData->blockType == NISTRINGEXTRADATA) {
			NiStringExtraData* stringExtraData = (NiStringExtraData*)extraData;
			int index = nif->AddStringExtraData(shape, stringExtraData->GetName(), stringExtraData->GetStringData());
			if (index != -1)
				extraDataIndices.push_back(index);
		}
		else if (extraData->blockType == NIINTEGEREXTRADATA) {
			NiIntegerExtraData* intExtraData = (NiIntegerExtraData*)extraData;
			int index = nif->AddIntegerExtraData(shape, intExtraData->GetName(), intExtraData->GetIntegerData());
			if (index != -1)
				extraDataIndices.push_back(index);
		}
	}

	if (extraDataIndices.empty())
		return;

	int id = extraDataIndices.size() - 1;

	wxButton* extraDataBtn = new wxButton(pgExtraData, 1000 + id, "Remove");
	extraDataBtn->Bind(wxEVT_BUTTON, &ShapeProperties::OnRemoveExtraData, this);

	wxArrayString types;
	types.Add("NiStringExtraData");
	types.Add("NiIntegerExtraData");
	wxChoice* extraDataType = new wxChoice(pgExtraData, 2000 + id, wxDefaultPosition, wxDefaultSize, types);
	extraDataType->SetSelection(0);
	extraDataType->Bind(wxEVT_CHOICE, &ShapeProperties::OnChangeExtraDataType, this);

	wxTextCtrl* extraDataName = new wxTextCtrl(pgExtraData, 3000 + id);
	wxTextCtrl* extraDataValue = new wxTextCtrl(pgExtraData, 4000 + id);

	if (uiOnly) {
		if (extraData->blockType == NISTRINGEXTRADATA) {
			NiStringExtraData* stringExtraData = (NiStringExtraData*)extraData;
			extraDataType->SetSelection(0);
			extraDataName->SetValue(stringExtraData->GetName());
			extraDataValue->SetValue(stringExtraData->GetStringData());
		}
		else if (extraData->blockType == NIINTEGEREXTRADATA) {
			NiIntegerExtraData* intExtraData = (NiIntegerExtraData*)extraData;
			extraDataType->SetSelection(1);
			extraDataName->SetValue(intExtraData->GetName());
			extraDataValue->SetValue(wxString::Format("%d", intExtraData->GetIntegerData()));
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
	nif->DeleteBlock(index);

	for (int i = 0; i < extraDataIndices.size(); i++)
		if (extraDataIndices[i] > index)
			extraDataIndices[i]--;

	extraDataIndices[id] = -1;

	wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + id, this));
	wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + id, this));

	switch (selection) {
	case 0:
		extraDataIndices[id] = nif->AddStringExtraData(shape, extraDataName->GetValue().ToStdString(), extraDataValue->GetValue().ToStdString());
		break;
	case 1:
		extraDataIndices[id] = nif->AddIntegerExtraData(shape, extraDataName->GetValue().ToStdString(), 0);
		break;
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
	nif->DeleteBlock(index);

	for (int i = 0; i < extraDataIndices.size(); i++)
		if (extraDataIndices[i] > index)
			extraDataIndices[i]--;

	extraDataIndices[id] = -1;

	pgExtraData->FitInside();
	pgExtraData->Layout();
}


void ShapeProperties::OnApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyChanges();
	EndModal(wxID_OK);
}

void ShapeProperties::ApplyChanges() {
	NiShader* shader = nif->GetShader(shape);

	if (shader) {
		string name = shaderName->GetValue();
		uint type = shaderType->GetSelection();
		wxColour color = specularColor->GetColour();
		Vector3 specColor(color.Red(), color.Green(), color.Blue());
		specColor /= 255.0f;
		float specStrength = atof(specularStrength->GetValue().c_str());
		float specPower = atof(specularPower->GetValue().c_str());

		color = emissiveColor->GetColour();
		Color4 emisColor(color.Red(), color.Green(), color.Blue(), color.Alpha());
		emisColor /= 255.0f;
		float emisMultiple = atof(emissiveMultiple->GetValue().c_str());

		shader->SetName(name);

		switch (shader->blockType) {
			case BSEFFECTSHADERPROPERTY: {
				shader->SetEmissiveColor(emisColor);
				shader->SetEmissiveMultiple(emisMultiple);
				break;
			}
			case BSLIGHTINGSHADERPROPERTY: {
				shader->SetType(type);

				shader->SetSpecularColor(specColor);
				shader->SetSpecularStrength(specStrength);
				shader->SetGlossiness(specPower);

				shader->SetEmissiveColor(emisColor);
				shader->SetEmissiveMultiple(emisMultiple);
				break;
			}
			case BSSHADERPPLIGHTINGPROPERTY: {
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

				shader->SetType(type);

				NiMaterialProperty* material = nif->GetMaterialProperty(shape);
				if (!material)
					break;

				material->SetSpecularColor(specColor);
				material->SetGlossiness(specPower);

				material->SetEmissiveColor(emisColor);
				material->SetEmissiveMultiple(emisMultiple);
				break;
			}
		}

		if (os->targetGame == FO4 && currentMaterialPath != name) {
			os->project->SetTexture(shape, "_AUTO_");
			os->RefreshGUIFromProj();
		}
	}

	ushort flags;
	byte threshold;
	if (nif->GetAlphaForShape(shape, flags, threshold)) {
		threshold = atoi(alphaThreshold->GetValue().c_str());
		nif->SetAlphaForShape(shape, flags, threshold);
	}

	if (os->targetGame == FO4) {
		BSTriShape* siTriShape = nif->geomForNameF4(shape);
		if (siTriShape)
			siTriShape->SetFullPrecision(fullPrecision->IsChecked());
	}

	for (int i = 0; i < extraDataIndices.size(); i++) {
		wxTextCtrl* extraDataName = dynamic_cast<wxTextCtrl*>(FindWindowById(3000 + i, this));
		wxTextCtrl* extraDataValue = dynamic_cast<wxTextCtrl*>(FindWindowById(4000 + i, this));
		if (!extraDataName || !extraDataValue)
			continue;

		NiExtraData* extraData = dynamic_cast<NiExtraData*>(nif->GetBlock(extraDataIndices[i]));
		if (extraData) {
			extraData->SetName(extraDataName->GetValue().ToStdString());

			if (extraData->blockType == NISTRINGEXTRADATA) {
				NiStringExtraData* stringExtraData = (NiStringExtraData*)extraData;
				stringExtraData->SetStringData(extraDataValue->GetValue().ToStdString());
			}
			else if (extraData->blockType == NIINTEGEREXTRADATA) {
				NiIntegerExtraData* intExtraData = (NiIntegerExtraData*)extraData;
				unsigned long val = 0;
				if (extraDataValue->GetValue().ToULong(&val))
					intExtraData->SetIntegerData(val);
			}
		}
	}
}
