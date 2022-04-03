/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/listctrl.h>

#include "FBXImportOptions.h"
#include "../render/GLDialog.h"
#include "../render/GLSurface.h"

class FBXWrangler;

class FBXImportDialog : public GLDialog {
public:
	FBXImportDialog(wxWindow* parent, const std::string& fileName, size_t vertexLimit = 0, size_t triangleLimit = 0);
	~FBXImportDialog();

	virtual void OnShown() override;
	void OnImport(wxCommandEvent& event);

	FBXImportOptions GetOptions() { return options; }

	wxDECLARE_EVENT_TABLE();

private:
	std::string fileName;
	size_t vertexLimit = 0;
	size_t triangleLimit = 0;

	std::unique_ptr<FBXWrangler> fbxw;

	FBXImportOptions options;
	wxCheckBox* cbInvertU;
	wxCheckBox* cbInvertV;
	wxTextCtrl* scale;
	wxChoice* rotateX;
	wxChoice* rotateY;
	wxChoice* rotateZ;
	wxListCtrl* meshesList;

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
