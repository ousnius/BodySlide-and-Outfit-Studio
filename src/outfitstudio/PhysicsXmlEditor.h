/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "physics/XmlEditSession.h"

#include <set>
#include <string>
#include <vector>
#include <wx/event.h>
#include <wx/string.h>

class OutfitStudioFrame;
class PhysicsEditorDialog;
class wxButton;
class wxTreeCtrl;
class wxTreeEvent;
class wxWindow;

namespace tinyxml2 {
class XMLElement;
}

// The HDT-SMP physics XMLs of the loaded meshes, held open for editing, with the list of them in the physics pane and
// the physics editor window. The preview builds from these documents, so it runs on the edits whether or not the editor
// is open, and turning physics off and on again keeps them. They go with the project (Shutdown).
class PhysicsXmlEditor : public wxEvtHandler {
public:
	explicit PhysicsXmlEditor(OutfitStudioFrame& frame);
	~PhysicsXmlEditor() override;

	// Finds the pane controls in the window and binds their events
	void Init();

	Physics::XmlEditSession& Session() { return session; }

	// Refills the pane's list with the XMLs the loaded meshes reference (or, while physics runs, the systems it built),
	// opening each in the session on the way, so the pane and the editor work before anything is simulated.
	void RefreshList();
	// Picks up a change to which XMLs the loaded meshes link, in the pane, the editor and the running simulation
	void RefreshLinks();
	// Closes the editor and drops the documents, for when the project goes away
	void Shutdown();
	// Reads a document again after something else rewrote its file, and rebuilds the preview. A document with
	// unsaved edits is kept.
	void ReloadDocument(const std::string& xmlPath);

	// Called by the editor window when it closes
	void EditorClosed();

	// Highlights the bones of what is selected in the editor in the viewport, by name in any case
	void HighlightBones(const std::set<std::string>& boneNames);

	// An edited property: written straight into the running simulation when the schema and the controller both allow
	// it, otherwise the simulation is rebuilt a moment later. Returns whether it went in without a rebuild.
	bool PropertyChanged(Physics::XmlDocument& doc, tinyxml2::XMLElement* element, const Physics::ChildDesc& desc);
	// A change too broad to describe as one property (an attribute, an undo, an import), which means a rebuild
	void DocumentChanged(const std::string& xmlPath);

	// Where physics XMLs live for the target game, for file dialogs to open on. Empty without a game data path.
	wxString XmlDirectory() const;
	// The path a NIF would link a file by: relative to the game data folder, with backslashes. Empty when the file is
	// outside of it, where no link would find it in game.
	std::string GameRelativeXmlPath(const wxString& path) const;

	// Writes a document back over the file it was read from. One that came out of an archive or was never on disk has
	// nothing to write back over: with "allowExport" this asks where to put it, without it says so and fails.
	bool Save(Physics::XmlDocument& doc, bool allowExport = true);
	// Save As. Later saves go to the new file, but what the NIF calls the document doesn't change.
	bool Export(Physics::XmlDocument& doc);
	// Loads another file over a document, keeping its link and its place in the session. Undoable.
	bool Import(Physics::XmlDocument& doc, const wxString& path);
	// Asks for a place for a new, empty physics XML and writes it there. Null when that was cancelled or failed.
	Physics::XmlDocument* CreateNew(wxWindow* parent);
	// Offers to save the edits that aren't on disk yet. False when the user cancels what prompted it.
	bool CheckPendingChanges();

	// The shapes selected in the shape list, by name
	std::vector<std::string> SelectedShapeNames() const;
	// Links a document to the selected shapes. The link is part of the project, which then has unsaved changes.
	void LinkToSelectedShapes(Physics::XmlDocument& doc, wxWindow* parent);

	bool HasProject() const;
	// The names of the loaded shapes, and of the bones the simulation can resolve: the skeleton's and the NIF's nodes
	std::vector<std::string> ShapeNames() const;
	std::vector<std::string> BoneNames() const;
	// The bones a loaded shape is skinned to
	std::vector<std::string> ShapeBoneNames(const std::string& shapeName) const;
	// What validating a document needs from outside the file
	Physics::ValidationContext ValidationInputs() const;

private:
	void OnListSelect(wxTreeEvent& event);
	void OnEdit(wxCommandEvent& event);

	// The XML selected in the pane's list, or the only one there is
	std::string SelectedXmlPath() const;
	// Redraws the editor's view of the documents, if it is open
	void RefreshEditor();

	OutfitStudioFrame& frame;
	Physics::XmlEditSession session;

	wxTreeCtrl* list = nullptr;
	wxButton* btnEdit = nullptr;
	PhysicsEditorDialog* editor = nullptr;
	// The bones were hidden when the editor opened and are shown only while it is
	bool bonesShownForEditor = false;
};
