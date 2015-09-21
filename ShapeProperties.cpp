#include "ShapeProperties.h"

BEGIN_EVENT_TABLE(ShapeProperties, wxDialog)
	EVT_BUTTON(XRCID("btnAddShader"), ShapeProperties::OnAddShader)
	EVT_BUTTON(XRCID("btnRemoveShader"), ShapeProperties::OnRemoveShader)
	EVT_BUTTON(XRCID("btnSetTextures"), ShapeProperties::OnSetTextures)
	EVT_BUTTON(XRCID("btnTransparency"), ShapeProperties::OnTransparency)
	EVT_CLOSE(ShapeProperties::OnClose)
END_EVENT_TABLE();

ShapeProperties::ShapeProperties(wxWindow* parent) {
	wxXmlResource * rsrc = wxXmlResource::Get();
	rsrc->LoadDialog(this, parent, "dlgShapeProp");

	SetSize(535, 335);
	SetDoubleBuffered(true);
	CenterOnParent();

	os = (OutfitStudio*)parent;

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
	if (os->activeItem->bIsOutfitShape)
		shader = os->project->workNif.GetShader(os->activeItem->shapeName);
	else
		shader = os->project->baseNif.GetShader(os->activeItem->shapeName);

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
				NifFile* nif;
				if (os->activeItem->bIsOutfitShape)
					nif = &os->project->workNif;
				else
					nif = &os->project->baseNif;

				if (!nif)
					break;

				NiMaterialProperty* material = nif->GetMaterialProperty(os->activeItem->shapeName);
				if (!material)
					break;
				
				color = material->colorSpecular * 255.0f;
				specularColor->SetColour(wxColour(color.x, color.y, color.z));
				specularPower->SetValue(wxString::Format("%.4f", material->GetGlossiness()));
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
	NifFile* nif;
	if (os->activeItem->bIsOutfitShape)
		nif = &os->project->workNif;
	else
		nif = &os->project->baseNif;

	if (!nif)
		return false;

	NiTriBasedGeom* geom = nif->geomForName(os->activeItem->shapeName);
	if (!geom)
		return false;

	int shaderID;
	BSShaderTextureSet* nifTexSet = new BSShaderTextureSet(nif->hdr);
	switch (os->targetGame) {
		case FO3:
		case FONV:
			shader = new BSShaderPPLightingProperty(nif->hdr);
			shaderID = nif->AddBlock(shader, "BSShaderPPLightingProperty");
			geom->propertiesRef.push_back(shaderID);
			geom->numProperties++;
			break;
		case SKYRIM:
		default:
			shader = new BSLightingShaderProperty(nif->hdr);
			shaderID = nif->AddBlock(shader, "BSLightingShaderProperty");
			geom->propertiesRef1 = shaderID;
	}

	shader->SetTextureSetRef(nif->AddBlock(nifTexSet, "BSShaderTextureSet"));
	AssignDefaultTexture();

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
	NifFile* nif;
	if (os->activeItem->bIsOutfitShape)
		nif = &os->project->workNif;
	else
		nif = &os->project->baseNif;

	if (!nif)
		return;

	nif->DeleteShader(os->activeItem->shapeName);
	AssignDefaultTexture();
}

void ShapeProperties::OnSetTextures(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	string texpath;
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

		int shaderType = 0;
		for (int i = 0; i < 9; i++) {
			if (os->activeItem->bIsOutfitShape) {
				shaderType = os->project->workNif.GetTextureForShape(os->activeItem->shapeName, texpath, i);
				if (!shaderType)
					continue;
			}
			else {
				shaderType = os->project->baseNif.GetTextureForShape(os->activeItem->shapeName, texpath, i);
				if (!shaderType)
					continue;
			}
			stTexGrid->SetCellValue(texpath, i, 0);
		}

		if (shaderType == BSEFFECTSHADERPROPERTY) {
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

		oDispPath = os->project->OutfitTexture(os->activeItem->shapeName);
		XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->SetPath(oDispPath);
		XRCCTRL(dlg, "btApplyDiffuse", wxButton)->Bind(wxEVT_BUTTON, &ShapeProperties::OnApplyDiffuse, this);

		if (dlg.ShowModal() == wxID_OK) {
			nDispPath = XRCCTRL(dlg, "stDisplayTexture", wxFilePickerCtrl)->GetPath();
			if (nDispPath != oDispPath) {
				if (os->activeItem->bIsOutfitShape) {
					os->project->SetOutfitTexture(os->activeItem->shapeName, nDispPath);
					os->glView->SetMeshTexture(os->activeItem->shapeName, nDispPath, os->project->workNif.IsShaderSkin(os->activeItem->shapeName));
				}
				else {
					os->project->SetRefTexture(os->activeItem->shapeName, nDispPath);
					os->glView->SetMeshTexture(os->activeItem->shapeName, nDispPath, os->project->baseNif.IsShaderSkin(os->activeItem->shapeName));
				}
			}

			for (int i = 0; i < 9; i++) {
				texpath = stTexGrid->GetCellValue(i, 0);
				if (os->activeItem->bIsOutfitShape) {
					os->project->workNif.SetTextureForShape(os->activeItem->shapeName, texpath, i);
					os->project->workNif.TrimTexturePaths();
				}
				else {
					os->project->baseNif.SetTextureForShape(os->activeItem->shapeName, texpath, i);
					os->project->baseNif.TrimTexturePaths();
				}
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
		os->project->SetOutfitTexture(os->activeItem->shapeName, "_AUTO_");
		os->glView->SetMeshTexture(os->activeItem->shapeName, texNoImg, os->project->workNif.IsShaderSkin(os->activeItem->shapeName));
	}
	else {
		os->project->SetRefTexture(os->activeItem->shapeName, "_AUTO_");
		os->glView->SetMeshTexture(os->activeItem->shapeName, texNoImg, os->project->baseNif.IsShaderSkin(os->activeItem->shapeName));
	}
	os->glView->Refresh();
}

void ShapeProperties::OnTransparency(wxCommandEvent& WXUNUSED(event)) {

}

void ShapeProperties::OnClose(wxCloseEvent& event) {

}
