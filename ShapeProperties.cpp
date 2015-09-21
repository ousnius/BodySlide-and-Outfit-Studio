#include "ShapeProperties.h"

BEGIN_EVENT_TABLE(ShapeProperties, wxDialog)
	EVT_BUTTON(XRCID("btnAddShader"), ShapeProperties::OnAddShader)
	EVT_BUTTON(XRCID("btnRemoveShader"), ShapeProperties::OnRemoveShader)
	EVT_BUTTON(XRCID("btnSetTextures"), ShapeProperties::OnSetTextures)
	EVT_BUTTON(XRCID("btnTransparency"), ShapeProperties::OnTransparency)
	EVT_BUTTON(wxID_OK, ShapeProperties::OnApply)
END_EVENT_TABLE();

ShapeProperties::ShapeProperties(wxWindow* parent, NifFile* refNif, const string& shapeName) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->LoadDialog(this, parent, "dlgShapeProp");

	SetSize(535, 335);
	SetDoubleBuffered(true);
	CenterOnParent();

	os = (OutfitStudio*)parent;
	nif = refNif;
	shape = shapeName;

	lbName = XRCCTRL(*this, "lbName", wxStaticText);
	shaderType = XRCCTRL(*this, "shaderType", wxChoice);
	specularColor = XRCCTRL(*this, "specularColor", wxColourPickerCtrl);
	specularStrength = XRCCTRL(*this, "specularStrength", wxTextCtrl);
	specularPower = XRCCTRL(*this, "specularPower", wxTextCtrl);
	btnAddShader = XRCCTRL(*this, "btnAddShader", wxButton);
	btnRemoveShader = XRCCTRL(*this, "btnRemoveShader", wxButton);
	btnSetTextures = XRCCTRL(*this, "btnSetTextures", wxButton);
	btnTransparency = XRCCTRL(*this, "btnTransparency", wxButton);

	lbName->SetLabel("Name: " + os->activeItem->shapeName);
	GetShader();
}

ShapeProperties::~ShapeProperties() {
}

void ShapeProperties::GetShader() {
	NiShader* shader = nif->GetShader(shape);

	if (!shader) {
		btnAddShader->Enable();
		btnRemoveShader->Disable();
		btnSetTextures->Disable();
		shaderType->Disable();
		specularColor->Disable();
		specularStrength->Disable();
		specularPower->Disable();
	}
	else {
		Vector3 color;
		switch (shader->blockType) {
			case BSEFFECTSHADERPROPERTY:
				specularColor->Disable();
				specularStrength->Disable();
				specularPower->Disable();
				break;
			case BSLIGHTINGSHADERPROPERTY:
				color = shader->GetSpecularColor() * 255.0f;
				specularColor->SetColour(wxColour(color.x, color.y, color.z));
				specularStrength->SetValue(wxString::Format("%.4f", shader->GetSpecularStrength()));
				specularPower->SetValue(wxString::Format("%.4f", shader->GetGlossiness()));
				break;
			case BSSHADERPPLIGHTINGPROPERTY:
				NiMaterialProperty* material = nif->GetMaterialProperty(shape);
				if (!material)
					break;
				
				color = material->colorSpecular * 255.0f;
				specularColor->SetColour(wxColour(color.x, color.y, color.z));
				specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));
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
				shaderType->Append("Default");
				shaderType->Append("Environment Map");
				shaderType->Append("Glow Shader");
				shaderType->Append("Heightmap");
				shaderType->Append("Face Tint");
				shaderType->Append("Skin Tint");
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

				switch (type) {
					case BSLightingShaderPropertyShaderType::Default: shaderType->SetSelection(0); break;
					case BSLightingShaderPropertyShaderType::EnvironmentMap: shaderType->SetSelection(1); break;
					case BSLightingShaderPropertyShaderType::GlowShader: shaderType->SetSelection(2); break;
					case BSLightingShaderPropertyShaderType::Heightmap: shaderType->SetSelection(3); break;
					case BSLightingShaderPropertyShaderType::FaceTint: shaderType->SetSelection(4); break;
					case BSLightingShaderPropertyShaderType::SkinTint: shaderType->SetSelection(5); break;
					case BSLightingShaderPropertyShaderType::ParallaxOccMaterial: shaderType->SetSelection(6); break;
					case BSLightingShaderPropertyShaderType::WorldMultitexture: shaderType->SetSelection(7); break;
					case BSLightingShaderPropertyShaderType::WorldMap1: shaderType->SetSelection(8); break;
					case BSLightingShaderPropertyShaderType::Unknown10: shaderType->SetSelection(9); break;
					case BSLightingShaderPropertyShaderType::MultiLayerParallax: shaderType->SetSelection(10); break;
					case BSLightingShaderPropertyShaderType::Unknown12: shaderType->SetSelection(11); break;
					case BSLightingShaderPropertyShaderType::WorldMap2: shaderType->SetSelection(12); break;
					case BSLightingShaderPropertyShaderType::SparkleSnow: shaderType->SetSelection(13); break;
					case BSLightingShaderPropertyShaderType::WorldMap3: shaderType->SetSelection(14); break;
					case BSLightingShaderPropertyShaderType::EyeEnvmap: shaderType->SetSelection(15); break;
					case BSLightingShaderPropertyShaderType::Unknown17: shaderType->SetSelection(16); break;
					case BSLightingShaderPropertyShaderType::WorldMap4: shaderType->SetSelection(17); break;
					case BSLightingShaderPropertyShaderType::WorldLODMultitexture: shaderType->SetSelection(18); break;
				}
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
				}
				break;
		}
	}
}

void ShapeProperties::OnAddShader(wxCommandEvent& WXUNUSED(event)) {
	if (!AddShader())
		return;

	btnAddShader->Disable();
	btnRemoveShader->Enable();
	btnSetTextures->Enable();
	shaderType->Enable();
	specularColor->Enable();
	specularStrength->Enable();
	specularPower->Enable();
}

bool ShapeProperties::AddShader() {
	NiTriBasedGeom* geom = nif->geomForName(shape);
	if (!geom)
		return false;

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

	NiShader* shader = nif->GetShader(shape);
	if (!shader)
		wxMessageBox("Could not add successfully add shader.", "Error");

	BSShaderTextureSet* nifTexSet = new BSShaderTextureSet(nif->hdr);
	shader->SetTextureSetRef(nif->AddBlock(nifTexSet, "BSShaderTextureSet"));
	AssignDefaultTexture();

	GetShaderType();

	return true;
}

void ShapeProperties::OnRemoveShader(wxCommandEvent& WXUNUSED(event)) {
	RemoveShader();

	btnAddShader->Enable();
	btnRemoveShader->Disable();
	btnSetTextures->Disable();
	shaderType->Disable();
	specularColor->Disable();
	specularStrength->Disable();
	specularPower->Disable();
}

void ShapeProperties::RemoveShader() {
	nif->DeleteShader(shape);
	AssignDefaultTexture();
	GetShaderType();
}

void ShapeProperties::OnSetTextures(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	string texPath;
	string oDispPath;
	string nDispPath;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgShapeTextures")) {
		wxGrid* stTexGrid = XRCCTRL(dlg, "stTexGrid", wxGrid);
		stTexGrid->CreateGrid(9, 1);
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
		for (int i = 0; i < 9; i++) {
			blockType = nif->GetTextureForShape(shape, texPath, i);
			if (!blockType)
				continue;

			stTexGrid->SetCellValue(texPath, i, 0);
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
		}

		oDispPath = os->project->OutfitTexture(shape);
		XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->SetPath(oDispPath);
		XRCCTRL(dlg, "btApplyDiffuse", wxButton)->Bind(wxEVT_BUTTON, &ShapeProperties::OnApplyDiffuse, this);

		if (dlg.ShowModal() == wxID_OK) {
			nDispPath = XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->GetPath();
			if (nDispPath != oDispPath) {
				if (os->activeItem->bIsOutfitShape) {
					os->project->SetOutfitTexture(shape, nDispPath);
					os->glView->SetMeshTexture(shape, nDispPath, nif->IsShaderSkin(shape));
				}
				else {
					os->project->SetRefTexture(shape, nDispPath);
					os->glView->SetMeshTexture(shape, nDispPath, nif->IsShaderSkin(shape));
				}
			}

			for (int i = 0; i < 9; i++) {
				texPath = stTexGrid->GetCellValue(i, 0);
				nif->SetTextureForShape(shape, texPath, i);
				nif->TrimTexturePaths();
			}
			os->glView->Refresh();
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
	if (os->activeItem->bIsOutfitShape) {
		os->project->SetOutfitTexture(shape, "_AUTO_");
		os->glView->SetMeshTexture(shape, texNoImg, nif->IsShaderSkin(shape));
	}
	else {
		os->project->SetRefTexture(shape, "_AUTO_");
		os->glView->SetMeshTexture(shape, texNoImg, nif->IsShaderSkin(shape));
	}
	os->glView->Refresh();
}

void ShapeProperties::OnTransparency(wxCommandEvent& WXUNUSED(event)) {

}

void ShapeProperties::OnApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyChanges();
	EndModal(wxID_OK);
}

void ShapeProperties::ApplyChanges() {
	NiShader* shader = nif->GetShader(shape);

	if (shader) {
		uint type = 0xFFFFFFFF;
		uint typeSelection = shaderType->GetSelection();
		wxColour color = specularColor->GetColour();
		Vector3 specColor(color.Red(), color.Green(), color.Blue());
		specColor /= 255.0f;
		float specStrength = atof(specularStrength->GetValue().ToAscii().data());
		float specPower = atof(specularPower->GetValue().ToAscii().data());

		switch (shader->blockType) {
			case BSEFFECTSHADERPROPERTY: {
				break;
			}
			case BSLIGHTINGSHADERPROPERTY: {
				switch (typeSelection) {
					case 0: type = BSLightingShaderPropertyShaderType::Default; break;
					case 1: type = BSLightingShaderPropertyShaderType::EnvironmentMap; break;
					case 2: type = BSLightingShaderPropertyShaderType::GlowShader; break;
					case 3: type = BSLightingShaderPropertyShaderType::Heightmap; break;
					case 4: type = BSLightingShaderPropertyShaderType::FaceTint; break;
					case 5: type = BSLightingShaderPropertyShaderType::SkinTint; break;
					case 6: type = BSLightingShaderPropertyShaderType::ParallaxOccMaterial; break;
					case 7: type = BSLightingShaderPropertyShaderType::WorldMultitexture; break;
					case 8: type = BSLightingShaderPropertyShaderType::WorldMap1; break;
					case 9: type = BSLightingShaderPropertyShaderType::Unknown10; break;
					case 10: type = BSLightingShaderPropertyShaderType::MultiLayerParallax; break;
					case 11: type = BSLightingShaderPropertyShaderType::Unknown12; break;
					case 12: type = BSLightingShaderPropertyShaderType::WorldMap2; break;
					case 13: type = BSLightingShaderPropertyShaderType::SparkleSnow; break;
					case 14: type = BSLightingShaderPropertyShaderType::WorldMap3; break;
					case 15: type = BSLightingShaderPropertyShaderType::EyeEnvmap; break;
					case 16: type = BSLightingShaderPropertyShaderType::Unknown17; break;
					case 17: type = BSLightingShaderPropertyShaderType::WorldMap4; break;
					case 18: type = BSLightingShaderPropertyShaderType::WorldLODMultitexture; break;
				}

				if (type != 0xFFFFFFFF)
					shader->SetType(type);

				shader->SetSpecularColor(specColor);
				shader->SetSpecularStrength(specStrength);
				shader->SetGlossiness(specPower);
				break;
			}
			case BSSHADERPPLIGHTINGPROPERTY: {
				switch (typeSelection) {
					case 0: type = BSShaderType::SHADER_TALL_GRASS; break;
					case 1: type = BSShaderType::SHADER_DEFAULT; break;
					case 2: type = BSShaderType::SHADER_SKY; break;
					case 3: type = BSShaderType::SHADER_SKIN; break;
					case 4: type = BSShaderType::SHADER_WATER; break;
					case 5: type = BSShaderType::SHADER_LIGHTING30; break;
					case 6: type = BSShaderType::SHADER_TILE; break;
					case 7: type = BSShaderType::SHADER_NOLIGHTING; break;
				}

				if (type != 0xFFFFFFFF)
					shader->SetType(type);

				NiMaterialProperty* material = nif->GetMaterialProperty(shape);
				if (!material)
					break;

				material->SetSpecularColor(specColor);
				material->SetGlossiness(specPower);
				break;
			}
		}
	}
}
