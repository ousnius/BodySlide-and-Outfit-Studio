/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "FBXImportDialog.h"
#include "../utils/ConfigDialogUtil.h"
#include "../utils/ConfigurationManager.h"
#include "../files/FBXWrangler.h"

#include <wx/valnum.h>

using namespace nifly;

extern ConfigurationManager Config;
extern ConfigurationManager OutfitStudioConfig;


wxBEGIN_EVENT_TABLE(FBXImportDialog, GLDialog)
	EVT_CHECKBOX(XRCID("cbInvertU"), FBXImportDialog::OnInvertU)
	EVT_CHECKBOX(XRCID("cbInvertV"), FBXImportDialog::OnInvertV)
	EVT_TEXT(XRCID("scale"), FBXImportDialog::OnScale)
	EVT_CHOICE(XRCID("rotateX"), FBXImportDialog::OnRotateX)
	EVT_CHOICE(XRCID("rotateY"), FBXImportDialog::OnRotateY)
	EVT_CHOICE(XRCID("rotateZ"), FBXImportDialog::OnRotateZ)
	EVT_LIST_ITEM_SELECTED(XRCID("meshesList"), FBXImportDialog::OnItemSelected)
	EVT_LIST_ITEM_DESELECTED(XRCID("meshesList"), FBXImportDialog::OnItemDeselected)
	EVT_LIST_KEY_DOWN(XRCID("meshesList"), FBXImportDialog::OnListKeyDown)
	EVT_BUTTON(wxID_OK, FBXImportDialog::OnImport)
wxEND_EVENT_TABLE()

FBXImportDialog::FBXImportDialog(wxWindow* parent, const std::string& fileName, size_t vertexLimit, size_t triangleLimit)
	: GLDialog()
	, fileName(fileName)
	, vertexLimit(vertexLimit)
	, triangleLimit(triangleLimit) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/FBXImport.xrc");
	xrc->LoadDialog(this, parent, "dlgFBXImport");

	cbInvertU = XRCCTRL((*this), "cbInvertU", wxCheckBox);
	cbInvertV = XRCCTRL((*this), "cbInvertV", wxCheckBox);

	wxFloatingPointValidator<float> floatValidator(5);
	floatValidator.SetRange(0.00001f, 10000.0f);

	scale = XRCCTRL((*this), "scale", wxTextCtrl);
	scale->SetValidator(floatValidator);

	rotateX = XRCCTRL((*this), "rotateX", wxChoice);
	rotateY = XRCCTRL((*this), "rotateY", wxChoice);
	rotateZ = XRCCTRL((*this), "rotateZ", wxChoice);

	meshesList = XRCCTRL((*this), "meshesList", wxListCtrl);

	ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, (*this), "FBXImport", "cbInvertU");
	ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, (*this), "FBXImport", "cbInvertV");
	ConfigDialogUtil::LoadDialogTextFloat(OutfitStudioConfig, (*this), "FBXImport", "scale");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "FBXImport", "rotateX");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "FBXImport", "rotateY");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "FBXImport", "rotateZ");

	xrc->AttachUnknownControl("glView", CreateCanvas(), this);
}

FBXImportDialog::~FBXImportDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/FBXImport.xrc");
}

void FBXImportDialog::OnShown() {
	GLDialog::OnShown();

	fbxw = std::make_unique<FBXWrangler>();

	bool result = fbxw->ImportScene(fileName);
	if (result) {
		std::vector<std::string> shapes;
		fbxw->GetShapeNames(shapes);

		for (auto& s : shapes) {
			FBXShape* fbxShape = fbxw->GetShape(s);

			size_t vertCount = fbxShape->verts.size();
			size_t triCount = fbxShape->tris.size();

			auto m = new Mesh();
			m->nVerts = static_cast<int>(vertCount);
			m->nTris = static_cast<int>(triCount);

			m->verts = std::make_unique<Vector3[]>(m->nVerts);
			m->norms = std::make_unique<Vector3[]>(m->nVerts);
			m->tangents = std::make_unique<Vector3[]>(m->nVerts);
			m->bitangents = std::make_unique<Vector3[]>(m->nVerts);
			m->texcoord = std::make_unique<Vector2[]>(m->nVerts);
			m->tris = std::make_unique<Triangle[]>(m->nTris);

			for (int v = 0; v < m->nVerts; v++)
				m->verts[v] = Mesh::TransformPosNifToMesh(fbxShape->verts[v]);

			for (int t = 0; t < m->nTris; t++)
				m->tris[t] = fbxShape->tris[t];

			if (vertCount == fbxShape->uvs.size())
				for (int v = 0; v < m->nVerts; v++)
					m->texcoord[v] = fbxShape->uvs[v];

			if (vertCount == fbxShape->normals.size()) {
				for (int v = 0; v < m->nVerts; v++)
					m->norms[v] = Mesh::TransformDirNifToMesh(fbxShape->normals[v]);
			}
			else
				m->SmoothNormals();

			m->shapeName = fbxShape->name;
			m->rendermode = Mesh::RenderMode::Normal;
			m->doublesided = true;
			m->color = Vector3(0.25f, 0.25f, 0.25f);
			m->textured = true;

			std::vector<std::string> textureFiles{Config["AppDir"] + "/res/images/NoImg.png"};
			m->material = GetSurface().AddMaterial(textureFiles, Config["AppDir"] + "/res/shaders/default.vert", Config["AppDir"] + "/res/shaders/default.frag");

			m->CalcTangentSpace();
			m->CreateBuffers();
			GetSurface().UpdateShaders(m);
			GetSurface().AddMesh(m);

			long itemId = meshesList->InsertItem(meshesList->GetItemCount(), wxString::FromUTF8(m->shapeName));

			wxListItem colName{};
			colName.SetId(itemId);
			colName.SetColumn(0);
			colName.SetText(wxString::FromUTF8(m->shapeName));
			meshesList->SetItem(colName);

			wxListItem colVertCount{};
			colVertCount.SetId(itemId);
			colVertCount.SetColumn(1);
			colVertCount.SetText(wxString::Format("%zu", vertCount));
			meshesList->SetItem(colVertCount);

			wxListItem colTriCount{};
			colTriCount.SetId(itemId);
			colTriCount.SetColumn(2);
			colTriCount.SetText(wxString::Format("%zu", triCount));
			meshesList->SetItem(colTriCount);

			if ((vertexLimit > 0 && m->nVerts > vertexLimit) || (triangleLimit > 0 && m->nTris > triangleLimit))
				meshesList->SetItemTextColour(itemId, wxColour("red"));
		}

		UpdateVertexPositions();
		UpdateTextureCoords();
	}
}

void FBXImportDialog::UpdateVertexPositions() {
	if (!fbxw)
		return;

	Matrix4 mat;

	float scaleValue = std::atof(scale->GetValue().c_str());
	mat.PushScale(scaleValue, scaleValue, scaleValue);

	int rotateXSel = rotateX->GetSelection();
	int rotateYSel = rotateY->GetSelection();
	int rotateZSel = rotateZ->GetSelection();

	float rotateXDeg = 0.0f;
	float rotateYDeg = 0.0f;
	float rotateZDeg = 0.0f;

	switch (rotateXSel) {
		case 1: rotateXDeg = 90.0f; break;
		case 2: rotateXDeg = 180.0f; break;
		case 3: rotateXDeg = 270.0f; break;
	}

	switch (rotateYSel) {
		case 1: rotateYDeg = 90.0f; break;
		case 2: rotateYDeg = 180.0f; break;
		case 3: rotateYDeg = 270.0f; break;
	}

	switch (rotateZSel) {
		case 1: rotateZDeg = 90.0f; break;
		case 2: rotateZDeg = 180.0f; break;
		case 3: rotateZDeg = 270.0f; break;
	}

	mat.PushRotate(rotateXDeg * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
	mat.PushRotate(rotateYDeg * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	mat.PushRotate(rotateZDeg * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));

	std::vector<std::string> shapes;
	fbxw->GetShapeNames(shapes);

	for (auto& s : shapes) {
		FBXShape* fbxShape = fbxw->GetShape(s);

		auto m = GetSurface().GetMesh(fbxShape->name);
		if (!m)
			continue;

		int vertCount = static_cast<int>(fbxShape->verts.size());
		if (vertCount != m->nVerts)
			continue;

		for (int v = 0; v < m->nVerts; v++)
			m->verts[v] = Mesh::TransformPosNifToMesh(mat * fbxShape->verts[v]);

		m->QueueUpdate(Mesh::UpdateType::Position);
	}

	Render();
}

void FBXImportDialog::UpdateTextureCoords() {
	if (!fbxw)
		return;

	std::vector<std::string> shapes;
	fbxw->GetShapeNames(shapes);

	for (auto& s : shapes) {
		FBXShape* fbxShape = fbxw->GetShape(s);

		auto m = GetSurface().GetMesh(fbxShape->name);
		if (!m)
			continue;

		int uvCount = static_cast<int>(fbxShape->uvs.size());
		if (uvCount != m->nVerts)
			continue;

		for (int v = 0; v < m->nVerts; v++)
			m->texcoord[v] = fbxShape->uvs[v];

		if (cbInvertU->IsChecked())
			for (int v = 0; v < m->nVerts; v++)
				m->texcoord[v].u = 1.0f - m->texcoord[v].u;

		if (cbInvertV->IsChecked())
			for (int v = 0; v < m->nVerts; v++)
				m->texcoord[v].v = 1.0f - m->texcoord[v].v;

		m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	}

	Render();
}

void FBXImportDialog::UpdateItemSelection() {
	for (int item = 0; item < meshesList->GetItemCount(); item++) {
		std::string name = meshesList->GetItemText(item).ToUTF8();

		auto m = GetSurface().GetMesh(name);
		if (m) {
			if (meshesList->GetItemState(item, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED)
				m->color = Vector3(0.1f, 0.25f, 0.1f);
			else
				m->color = Vector3(0.25f, 0.25f, 0.25f);
		}
	}

	Render();
}

void FBXImportDialog::DeleteItemSelection() {
	for (int item = meshesList->GetItemCount() - 1; item >= 0; item--) {
		if (meshesList->GetItemState(item, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED) {
			std::string name = meshesList->GetItemText(item).ToUTF8();
			GetSurface().DeleteMesh(name);
			meshesList->DeleteItem(item);
		}
	}

	Render();
}

void FBXImportDialog::OnInvertU(wxCommandEvent& WXUNUSED(event)) {
	UpdateTextureCoords();
}

void FBXImportDialog::OnInvertV(wxCommandEvent& WXUNUSED(event)) {
	UpdateTextureCoords();
}

void FBXImportDialog::OnScale(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void FBXImportDialog::OnRotateX(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void FBXImportDialog::OnRotateY(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void FBXImportDialog::OnRotateZ(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void FBXImportDialog::OnItemSelected(wxListEvent& WXUNUSED(event)) {
	UpdateItemSelection();
}

void FBXImportDialog::OnItemDeselected(wxListEvent& WXUNUSED(event)) {
	UpdateItemSelection();
}

void FBXImportDialog::OnListKeyDown(wxListEvent& event) {
	if (event.GetKeyCode() == wxKeyCode::WXK_DELETE)
		DeleteItemSelection();
}

void FBXImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {
	options.InvertU = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, (*this), "FBXImport", "cbInvertU");
	options.InvertV = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, (*this), "FBXImport", "cbInvertV");
	options.Scale = ConfigDialogUtil::SetFloatFromDialogTextControl(OutfitStudioConfig, (*this), "FBXImport", "scale");

	int rotateXSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "FBXImport", "rotateX");
	int rotateYSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "FBXImport", "rotateY");
	int rotateZSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "FBXImport", "rotateZ");

	float rotateXDeg = 0.0f;
	float rotateYDeg = 0.0f;
	float rotateZDeg = 0.0f;

	switch (rotateXSel) {
		case 1: rotateXDeg = 90.0f; break;
		case 2: rotateXDeg = 180.0f; break;
		case 3: rotateXDeg = 270.0f; break;
	}

	switch (rotateYSel) {
		case 1: rotateYDeg = 90.0f; break;
		case 2: rotateYDeg = 180.0f; break;
		case 3: rotateYDeg = 270.0f; break;
	}

	switch (rotateZSel) {
		case 1: rotateZDeg = 90.0f; break;
		case 2: rotateZDeg = 180.0f; break;
		case 3: rotateZDeg = 270.0f; break;
	}

	options.RotateX = rotateXDeg;
	options.RotateY = rotateYDeg;
	options.RotateZ = rotateZDeg;
	options.ImportAll = false;

	for (int item = 0; item < meshesList->GetItemCount(); item++) {
		std::string name = meshesList->GetItemText(item).ToUTF8();
		options.ImportShapes.insert(name);
	}

	EndModal(wxID_OK);
}
