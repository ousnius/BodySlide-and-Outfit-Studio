/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/listctrl.h>

#include "../render/GLDialog.h"
#include "../render/GLSurface.h"
#include "../files/ObjFile.h"

class ObjImportDialog : public GLDialog {
public:
	ObjImportDialog(wxWindow* parent, const std::string& fileName, size_t vertexLimit = 0, size_t triangleLimit = 0, const wxString& warningLabel = wxString());
	~ObjImportDialog();

	virtual void OnShown() override;
	void OnImport(wxCommandEvent& event);

	ObjImportOptions GetOptions() { return options; }

	wxDECLARE_EVENT_TABLE();

private:
	std::string fileName;
	size_t vertexLimit = 0;
	size_t triangleLimit = 0;

	ObjFile obj;

	ObjImportOptions options;
	wxStaticText* lbWarning;
	wxCheckBox* cbInvertU = nullptr;
	wxCheckBox* cbInvertV = nullptr;
	wxTextCtrl* scale = nullptr;
	wxChoice* rotateX = nullptr;
	wxChoice* rotateY = nullptr;
	wxChoice* rotateZ = nullptr;
	wxListCtrl* meshesList = nullptr;

	void UpdateVertexPositions();
	void UpdateTextureCoords();
	void UpdateItemSelection();
	void DeleteItemSelection();
	void OnInvertU(wxCommandEvent& event);
	void OnInvertV(wxCommandEvent& event);
	void OnScale(wxCommandEvent& event);
	void OnRotateX(wxCommandEvent& event);
	void OnRotateY(wxCommandEvent& event);
	void OnRotateZ(wxCommandEvent& event);
	void OnItemSelected(wxListEvent& event);
	void OnItemDeselected(wxListEvent& event);
	void OnListKeyDown(wxListEvent& event);
};
