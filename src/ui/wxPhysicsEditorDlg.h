/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/treectrl.h>
#include <wx/wx.h>

/*
XRC shell of the physics editor: loads the resource, binds the controls and
turns their events into overridable hooks. All the logic lives in
PhysicsEditorDialog, which derives from this.

A frame rather than a dialog, like EditUV, so it is properly non-modal: the
point of the editor is to change a value and watch the simulation in the
viewport behind it react.
*/
class wxPhysicsEditorDlg : public wxFrame {
public:
	wxPhysicsEditorDlg(wxWindow* parent);
	~wxPhysicsEditorDlg();

	// False when the XRC could not be loaded, in which case the window is
	// unusable and the caller has to throw it away.
	bool IsLoaded() const { return loaded; }

protected:
	wxButton* physXmlNew = nullptr;
	wxButton* physXmlImport = nullptr;
	wxButton* physXmlExport = nullptr;
	wxButton* physXmlSave = nullptr;
	wxButton* physXmlSaveAll = nullptr;
	wxButton* physXmlLink = nullptr;
	wxSplitterWindow* physSplitter = nullptr;
	wxSearchCtrl* physFilter = nullptr;
	wxTreeCtrl* physTree = nullptr;
	wxStaticText* physElementLabel = nullptr;
	wxSplitterWindow* physRightSplitter = nullptr;
	wxPropertyGrid* pgProperties = nullptr;
	wxNotebook* physInfoBook = nullptr;
	wxListCtrl* physCollisionList = nullptr;
	wxListCtrl* physDiagnosticList = nullptr;
	wxStaticText* physStatus = nullptr;
	wxButton* physUndo = nullptr;
	wxButton* physRedo = nullptr;
	wxButton* physClose = nullptr;

	// Virtual event handlers, override them in the derived class
	virtual void doNewXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doImportXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doExportXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doSaveXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doSaveAllXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doLinkXml(wxCommandEvent& event) { event.Skip(); }
	virtual void doTreeSelectionChanged(wxTreeEvent& event) { event.Skip(); }
	virtual void doFilterChanged(wxCommandEvent& event) { event.Skip(); }
	virtual void doPropertyChanged(wxPropertyGridEvent& event) { event.Skip(); }
	virtual void doCollisionActivated(wxListEvent& event) { event.Skip(); }
	virtual void doDiagnosticActivated(wxListEvent& event) { event.Skip(); }
	virtual void doUndo(wxCommandEvent& event) { event.Skip(); }
	virtual void doRedo(wxCommandEvent& event) { event.Skip(); }
	virtual void doCloseButton(wxCommandEvent& event) { event.Skip(); }
	virtual void doCloseWindow(wxCloseEvent& event) { event.Skip(); }

	bool loaded = false;

	wxDECLARE_EVENT_TABLE();
};
