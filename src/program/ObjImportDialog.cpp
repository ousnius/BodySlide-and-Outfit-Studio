/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ObjImportDialog.h"
#include "../utils/ConfigDialogUtil.h"
#include "../utils/ConfigurationManager.h"

#include <wx/valnum.h>

using namespace nifly;

extern ConfigurationManager Config;
extern ConfigurationManager OutfitStudioConfig;


wxBEGIN_EVENT_TABLE(ObjImportDialog, GLDialog)
	EVT_CHECKBOX(XRCID("cbInvertU"), ObjImportDialog::OnInvertU)
	EVT_CHECKBOX(XRCID("cbInvertV"), ObjImportDialog::OnInvertV)
	EVT_TEXT(XRCID("scale"), ObjImportDialog::OnScale)
	EVT_CHOICE(XRCID("rotateX"), ObjImportDialog::OnRotateX)
	EVT_CHOICE(XRCID("rotateY"), ObjImportDialog::OnRotateY)
	EVT_CHOICE(XRCID("rotateZ"), ObjImportDialog::OnRotateZ)
	EVT_LIST_ITEM_SELECTED(XRCID("meshesList"), ObjImportDialog::OnItemSelected)
	EVT_LIST_ITEM_DESELECTED(XRCID("meshesList"), ObjImportDialog::OnItemDeselected)
	EVT_LIST_KEY_DOWN(XRCID("meshesList"), ObjImportDialog::OnListKeyDown)
	EVT_BUTTON(wxID_OK, ObjImportDialog::OnImport)
wxEND_EVENT_TABLE()

ObjImportDialog::ObjImportDialog(wxWindow* parent, const std::string& fileName, size_t vertexLimit, size_t triangleLimit, const wxString& warningLabel)
	: GLDialog()
	, fileName(fileName)
	, vertexLimit(vertexLimit)
	, triangleLimit(triangleLimit) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ImportDialog.xrc");
	xrc->LoadDialog(this, parent, "importDialog");

	lbWarning = XRCCTRL((*this), "lbWarning", wxStaticText);

	if (!warningLabel.IsEmpty()) {
		lbWarning->SetLabel(warningLabel);
		lbWarning->Wrap(GetClientSize().x - 25);
		lbWarning->Show();
	}

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

	ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, (*this), "OBJImport", "cbInvertU");
	ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, (*this), "OBJImport", "cbInvertV");
	ConfigDialogUtil::LoadDialogTextFloat(OutfitStudioConfig, (*this), "OBJImport", "scale");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "OBJImport", "rotateX");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "OBJImport", "rotateY");
	ConfigDialogUtil::LoadDialogChoiceIndex(OutfitStudioConfig, (*this), "OBJImport", "rotateZ");

	xrc->AttachUnknownControl("glView", CreateCanvas(), this);
	canvas->MSWDisableComposited(); // Fix stuttering from composited flag?
}

ObjImportDialog::~ObjImportDialog() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ImportDialog.xrc");
}

void ObjImportDialog::OnShown() {
	GLDialog::OnShown();

	obj.SetScale(Vector3(10.0f, 10.0f, 10.0f));

	int result = obj.LoadForNif(fileName);
	if (result == 0) {
		auto shapes = obj.GetGroupList();

		for (auto& s : shapes) {
			std::vector<Vector3> verts;
			std::vector<Triangle> tris;
			std::vector<Vector2> uvs;
			std::vector<Vector3> norms;

			if (!obj.CopyDataForGroup(s, &verts, &tris, &uvs, &norms))
				continue;

			size_t vertCount = verts.size();
			size_t triCount = tris.size();

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
				m->verts[v] = Mesh::TransformPosNifToMesh(verts[v]);

			for (int t = 0; t < m->nTris; t++)
				m->tris[t] = tris[t];

			if (vertCount == uvs.size())
				for (int v = 0; v < m->nVerts; v++)
					m->texcoord[v] = uvs[v];

			if (vertCount == norms.size()) {
				for (int v = 0; v < m->nVerts; v++)
					m->norms[v] = Mesh::TransformDirNifToMesh(norms[v]);
			}
			else
				m->SmoothNormals();

			m->shapeName = s;
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

			wxListItem colInfo{};
			colInfo.SetId(itemId);
			colInfo.SetColumn(3);

			if (vertexLimit > 0 && m->nVerts > vertexLimit) {
				colInfo.SetText(_("The shape has reached the vertex count limit."));
				meshesList->SetItemTextColour(itemId, wxColour("red"));
			}
			else if (triangleLimit > 0 && m->nTris > triangleLimit) {
				colInfo.SetText(_("The shape has reached the triangle count limit."));
				meshesList->SetItemTextColour(itemId, wxColour("red"));
			}

			meshesList->SetItem(colInfo);
		}

		UpdateVertexPositions();
		UpdateTextureCoords();
	}
}

void ObjImportDialog::UpdateVertexPositions() {
	if (!scale || !rotateX || !rotateY || !rotateZ)
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

	Matrix4 matRot;
	matRot.PushRotate(rotateXDeg * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
	matRot.PushRotate(rotateYDeg * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
	matRot.PushRotate(rotateZDeg * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));

	auto shapes = obj.GetGroupList();

	for (auto& s : shapes) {
		auto m = GetSurface().GetMesh(s);
		if (!m)
			continue;

		std::vector<Vector3> verts;
		std::vector<Vector3> norms;

		if (!obj.CopyDataForGroup(s, &verts, nullptr, nullptr, &norms))
			continue;

		int vertCount = static_cast<int>(verts.size());
		if (vertCount != m->nVerts)
			continue;
		
		for (int v = 0; v < m->nVerts; v++)
			m->verts[v] = Mesh::TransformPosNifToMesh(mat * verts[v]);

		m->QueueUpdate(Mesh::UpdateType::Position);

		int normCount = static_cast<int>(norms.size());
		if (normCount == m->nVerts) {
			for (int v = 0; v < m->nVerts; v++)
				m->norms[v] = Mesh::TransformDirNifToMesh(matRot * norms[v]);

			m->QueueUpdate(Mesh::UpdateType::Normals);
			m->CalcTangentSpace();
		}
	}

	Render();
}

void ObjImportDialog::UpdateTextureCoords() {
	if (!cbInvertU || !cbInvertV)
		return;

	auto shapes = obj.GetGroupList();

	for (auto& s : shapes) {
		auto m = GetSurface().GetMesh(s);
		if (!m)
			continue;

		std::vector<Vector2> uvs;

		if (!obj.CopyDataForGroup(s, nullptr, nullptr, &uvs, nullptr))
			continue;

		int uvCount = static_cast<int>(uvs.size());
		if (uvCount != m->nVerts)
			continue;

		for (int v = 0; v < m->nVerts; v++)
			m->texcoord[v] = uvs[v];

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

void ObjImportDialog::UpdateItemSelection() {
	if (!meshesList)
		return;

	for (int item = 0; item < meshesList->GetItemCount(); item++) {
		std::string name{meshesList->GetItemText(item).ToUTF8()};

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

void ObjImportDialog::DeleteItemSelection() {
	if (!meshesList)
		return;

	for (int item = meshesList->GetItemCount() - 1; item >= 0; item--) {
		if (meshesList->GetItemState(item, wxLIST_STATE_SELECTED) == wxLIST_STATE_SELECTED) {
			std::string name{meshesList->GetItemText(item).ToUTF8()};
			GetSurface().DeleteMesh(name);
			meshesList->DeleteItem(item);
		}
	}

	Render();
}

void ObjImportDialog::OnInvertU(wxCommandEvent& WXUNUSED(event)) {
	UpdateTextureCoords();
}

void ObjImportDialog::OnInvertV(wxCommandEvent& WXUNUSED(event)) {
	UpdateTextureCoords();
}

void ObjImportDialog::OnScale(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void ObjImportDialog::OnRotateX(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void ObjImportDialog::OnRotateY(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void ObjImportDialog::OnRotateZ(wxCommandEvent& WXUNUSED(event)) {
	UpdateVertexPositions();
}

void ObjImportDialog::OnItemSelected(wxListEvent& WXUNUSED(event)) {
	UpdateItemSelection();
}

void ObjImportDialog::OnItemDeselected(wxListEvent& WXUNUSED(event)) {
	UpdateItemSelection();
}

void ObjImportDialog::OnListKeyDown(wxListEvent& event) {
	if (event.GetKeyCode() == wxKeyCode::WXK_DELETE)
		DeleteItemSelection();
}

void ObjImportDialog::OnImport(wxCommandEvent& WXUNUSED(event)) {
	options.InvertU = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, (*this), "OBJImport", "cbInvertU");
	options.InvertV = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, (*this), "OBJImport", "cbInvertV");
	options.Scale = ConfigDialogUtil::SetFloatFromDialogTextControl(OutfitStudioConfig, (*this), "OBJImport", "scale");

	int rotateXSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "OBJImport", "rotateX");
	int rotateYSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "OBJImport", "rotateY");
	int rotateZSel = ConfigDialogUtil::SetIntegerFromDialogChoice(OutfitStudioConfig, (*this), "OBJImport", "rotateZ");

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
		std::string name{meshesList->GetItemText(item).ToUTF8()};
		options.ImportShapes.insert(name);
	}

	EndModal(wxID_OK);
}
