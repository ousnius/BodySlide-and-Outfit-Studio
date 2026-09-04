/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "wxPhysicsEditorDlg.h"
#include "../utils/ConfigurationManager.h"

#include <wx/filedlg.h>
#include <wx/xrc/xmlres.h>

extern ConfigurationManager Config;

namespace {
wxString ResourcePath() {
	return wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/PhysicsEditor.xrc";
}
}

wxBEGIN_EVENT_TABLE(wxPhysicsEditorDlg, wxFrame)
	EVT_BUTTON(XRCID("physXmlNew"), wxPhysicsEditorDlg::doNewXml)
	EVT_BUTTON(XRCID("physXmlImport"), wxPhysicsEditorDlg::doImportXml)
	EVT_BUTTON(XRCID("physXmlExport"), wxPhysicsEditorDlg::doExportXml)
	EVT_BUTTON(XRCID("physXmlSave"), wxPhysicsEditorDlg::doSaveXml)
	EVT_BUTTON(XRCID("physXmlSaveAll"), wxPhysicsEditorDlg::doSaveAllXml)
	EVT_BUTTON(XRCID("physXmlLink"), wxPhysicsEditorDlg::doLinkXml)
	EVT_TREE_SEL_CHANGED(XRCID("physTree"), wxPhysicsEditorDlg::doTreeSelectionChanged)
	EVT_TEXT(XRCID("physFilter"), wxPhysicsEditorDlg::doFilterChanged)
	EVT_LIST_ITEM_ACTIVATED(XRCID("physCollisionList"), wxPhysicsEditorDlg::doCollisionActivated)
	EVT_LIST_ITEM_ACTIVATED(XRCID("physDiagnosticList"), wxPhysicsEditorDlg::doDiagnosticActivated)
	EVT_BUTTON(XRCID("physUndo"), wxPhysicsEditorDlg::doUndo)
	EVT_BUTTON(XRCID("physRedo"), wxPhysicsEditorDlg::doRedo)
	EVT_BUTTON(XRCID("physClose"), wxPhysicsEditorDlg::doCloseButton)
	EVT_CLOSE(wxPhysicsEditorDlg::doCloseWindow)
wxEND_EVENT_TABLE()

wxPhysicsEditorDlg::wxPhysicsEditorDlg(wxWindow* parent) {
	wxXmlResource* xrc = wxXmlResource::Get();
	if (!xrc->Load(ResourcePath())) {
		wxMessageBox(_("Failed to load PhysicsEditor.xrc file!"), _("Error"), wxICON_ERROR);
		return;
	}

	if (!xrc->LoadFrame(this, parent, "dlgPhysicsEditor")) {
		wxMessageBox(_("Failed to load the physics editor frame!"), _("Error"), wxICON_ERROR);
		return;
	}

	physXmlNew = XRCCTRL(*this, "physXmlNew", wxButton);
	physXmlImport = XRCCTRL(*this, "physXmlImport", wxButton);
	physXmlExport = XRCCTRL(*this, "physXmlExport", wxButton);
	physXmlSave = XRCCTRL(*this, "physXmlSave", wxButton);
	physXmlSaveAll = XRCCTRL(*this, "physXmlSaveAll", wxButton);
	physXmlLink = XRCCTRL(*this, "physXmlLink", wxButton);
	physSplitter = XRCCTRL(*this, "physSplitter", wxSplitterWindow);
	physFilter = XRCCTRL(*this, "physFilter", wxSearchCtrl);
	physTree = XRCCTRL(*this, "physTree", wxTreeCtrl);
	physElementLabel = XRCCTRL(*this, "physElementLabel", wxStaticText);
	physRightSplitter = XRCCTRL(*this, "physRightSplitter", wxSplitterWindow);
	physInfoBook = XRCCTRL(*this, "physInfoBook", wxNotebook);
	physCollisionList = XRCCTRL(*this, "physCollisionList", wxListCtrl);
	physDiagnosticList = XRCCTRL(*this, "physDiagnosticList", wxListCtrl);
	physStatus = XRCCTRL(*this, "physStatus", wxStaticText);
	physUndo = XRCCTRL(*this, "physUndo", wxButton);
	physRedo = XRCCTRL(*this, "physRedo", wxButton);
	physClose = XRCCTRL(*this, "physClose", wxButton);

	// The grid eats plain keys while a cell is being edited, so undo and redo
	// go on the frame rather than on any one control.
	const wxAcceleratorEntry accelerators[]
		= {wxAcceleratorEntry(wxACCEL_CTRL, 'Z', XRCID("physUndo")), wxAcceleratorEntry(wxACCEL_CTRL, 'Y', XRCID("physRedo"))};
	SetAcceleratorTable(wxAcceleratorTable(WXSIZEOF(accelerators), accelerators));

	// The grid is generated from the schema table rather than described in the
	// XRC, so it goes in as an unknown control, the way wxNormalsGenDlg does it.
	wxWindow* gridPanel = XRCCTRL(*this, "physGridPanel", wxPanel);
	pgProperties = new wxPropertyGrid(gridPanel ? gridPanel : this,
									  wxID_ANY,
									  wxDefaultPosition,
									  wxDefaultSize,
									  wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS);
	pgProperties->SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS);
	pgProperties->Bind(wxEVT_PG_CHANGED, &wxPhysicsEditorDlg::doPropertyChanged, this);
	xrc->AttachUnknownControl("pgProperties", pgProperties, this);

	// Growing the window has to grow the property grid. Without this the
	// splitter keeps the top pane at the height it was given and hands every
	// extra pixel to the lists below it, so maximizing the editor leaves the
	// grid four rows tall.
	if (physRightSplitter)
		physRightSplitter->SetSashGravity(1.0);

	// A static text asks its sizer for room for the whole label, and these
	// labels are schema help lines. Without a small minimum of its own it
	// pushes the buttons beside it off the end of the bar instead of
	// ellipsizing, which is what it was given wxST_ELLIPSIZE_END for.
	physStatus->SetMinSize(FromDIP(wxSize(120, -1)));
	physElementLabel->SetMinSize(FromDIP(wxSize(120, -1)));

	// A frame built from XRC comes up at the minimum its sizer asks for, which
	// for a tree beside a property grid is far too small to work in.
	SetMinSize(FromDIP(wxSize(600, 400)));
	SetSize(FromDIP(wxSize(980, 640)));
	Layout();

	loaded = true;
}

wxPhysicsEditorDlg::~wxPhysicsEditorDlg() {
	wxXmlResource::Get()->Unload(ResourcePath());
}
