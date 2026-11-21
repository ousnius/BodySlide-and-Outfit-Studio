/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "physics/XmlDocument.h"

#include <set>
#include <string>
#include <utility>
#include <vector>
#include <wx/frame.h>
#include <wx/treectrl.h>

class PhysicsXmlEditor;
class wxButton;
class wxListCtrl;
class wxListEvent;
class wxNotebook;
class wxPGProperty;
class wxPropertyGrid;
class wxPropertyGridEvent;
class wxSearchCtrl;
class wxSplitterWindow;
class wxStaticText;

namespace Physics {
class XmlEditSession;
}

namespace tinyxml2 {
class XMLElement;
}

/*
The physics editor: a tree of the physics XMLs the loaded meshes reference, with their bones, shapes and
constraints, and a property grid generated from the schema table for whatever is selected.

A frame rather than a dialog, like EditUV, so it is non-modal: it edits documents the running simulation reads, so
the viewport behind it has to stay live.

Owns nothing. The documents belong to the PhysicsXmlEditor of the window, so closing this doesn't throw away edits or
revert the preview.
*/
class PhysicsEditorDialog : public wxFrame {
public:
	PhysicsEditorDialog(wxWindow* parent, PhysicsXmlEditor& owner);
	~PhysicsEditorDialog() override;

	// False when the XRC couldn't be loaded, in which case the caller has to throw the window away
	bool IsLoaded() const { return loaded; }

	// Rebuilds the tree from the session, keeping the selected element when it is still there. Call after documents
	// are adopted, closed or reloaded.
	void RefreshDocuments();
	// Selects and reveals the document for "xmlPath", if the tree has it
	void SelectDocument(const std::string& xmlPath);
	// For the owner going away first: nothing is reported to it anymore
	void Detach() { owner = nullptr; }

private:
	/*
	What a tree item stands for. Documents are addressed by path and elements by NodePath, never by pointer: undo
	reparses a whole document, which invalidates every pointer into it.

	An empty node path means the item is a document row or a category heading.
	*/
	class ItemData : public wxTreeItemData {
	public:
		ItemData(std::string xmlPath, Physics::NodePath node)
			: xmlPath(std::move(xmlPath))
			, node(std::move(node)) {}

		std::string xmlPath;
		Physics::NodePath node;
	};

	// The documents of the owner. Null after Detach.
	Physics::XmlEditSession* Session() const;

	void PopulateTree();
	// Adds one heading with the count of what is under it, and a row per element. Nothing when there is nothing of
	// that kind.
	void AddCategory(const wxTreeItemId& parent,
					 Physics::XmlDocument& doc,
					 const wxString& label,
					 std::initializer_list<Physics::ElementKind> kinds,
					 bool nestConstraintGroups = false);
	// "showKind" names the element's kind after its name, for the headings that merge several kinds
	void AddElementItem(const wxTreeItemId& parent, Physics::XmlDocument& doc, tinyxml2::XMLElement* element, bool showKind);

	// True when "element" passes the filter box, itself or through one of the elements nested in it
	bool MatchesFilter(tinyxml2::XMLElement* element) const;

	void ShowElement(Physics::XmlDocument* doc, tinyxml2::XMLElement* element);
	void ShowNothing(const wxString& reason);
	void AppendUnknownElement(tinyxml2::XMLElement* element);

	// The top level row of a document, or an invalid id
	wxTreeItemId FindDocumentItem(const std::string& xmlPath) const;
	// The row of an element, or an invalid id, which is a normal outcome of an edit that removed it
	wxTreeItemId FindTreeItem(const std::string& xmlPath, const Physics::NodePath& node) const;
	// Selects the row of an element, so a diagnostic or a collision verdict can be followed to what it is about
	void SelectNode(const std::string& xmlPath, const Physics::NodePath& node);

	// The names a value that names something else can be picked from: the skeleton for a bone, the loaded meshes for
	// a NiShape, the document for a collision shape, and every open document for a tag. Editable wherever offered,
	// so a name none of them knows still round trips.
	wxArrayString ChoicesFor(Physics::ValueKind kind, const Physics::XmlDocument& doc) const;

	// The bones an element stands for: a bone's own, the two a constraint joins, those of the constraints of a group,
	// and the ones a skinned shape is skinned to
	std::set<std::string> BonesOf(const tinyxml2::XMLElement* element) const;
	// The same for a tree row. A document or heading row stands for everything under it.
	std::set<std::string> BonesOfItem(const wxTreeItemId& item) const;
	// Highlights the bones of the selected row in the viewport
	void HighlightSelection();

	// Every other skinned shape, and whether this one would ever be tested against it. Only for skinned shapes.
	void ShowCollisions(Physics::XmlDocument* doc, tinyxml2::XMLElement* element);
	// Revalidates the document and fills the diagnostics page
	void ShowDiagnostics(Physics::XmlDocument* doc);

	// The element the grid shows, kept as a path because undo reparses the document under every pointer into it
	Physics::XmlDocument* CurrentDocument() const;
	tinyxml2::XMLElement* CurrentElement() const;

	// Writes one edited value into the document and hands it to the owner, which patches the running simulation or
	// rebuilds it
	void ApplyChange(const wxPGProperty* property, const Physics::ChildDesc* child, const Physics::AttrDesc* attr);
	// Re-reads everything after a change that could have moved elements around (undo, redo, import)
	void ReloadAfterDocumentChange();

	// Enables undo, redo and the file actions for what the selected document can do, and marks it in the tree while
	// it has unsaved edits
	void UpdateEditControls();
	void UpdateFileControls();

	void OnTreeSelectionChanged(wxTreeEvent& event);
	void OnFilterChanged(wxCommandEvent& event);
	void OnNewXml(wxCommandEvent& event);
	void OnImportXml(wxCommandEvent& event);
	void OnExportXml(wxCommandEvent& event);
	void OnSaveXml(wxCommandEvent& event);
	void OnSaveAllXml(wxCommandEvent& event);
	void OnLinkXml(wxCommandEvent& event);
	void OnPropertyChanged(wxPropertyGridEvent& event);
	void OnCollisionActivated(wxListEvent& event);
	void OnDiagnosticActivated(wxListEvent& event);
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);
	void OnCloseWindow(wxCloseEvent& event);

	PhysicsXmlEditor* owner = nullptr;
	bool loaded = false;

	wxButton* btnNew = nullptr;
	wxButton* btnImport = nullptr;
	wxButton* btnExport = nullptr;
	wxButton* btnSave = nullptr;
	wxButton* btnSaveAll = nullptr;
	wxButton* btnLink = nullptr;
	wxSearchCtrl* filterBox = nullptr;
	wxTreeCtrl* tree = nullptr;
	wxStaticText* elementLabel = nullptr;
	wxPropertyGrid* grid = nullptr;
	wxNotebook* infoBook = nullptr;
	wxListCtrl* collisionList = nullptr;
	wxListCtrl* diagnosticList = nullptr;
	wxStaticText* status = nullptr;
	wxButton* btnUndo = nullptr;
	wxButton* btnRedo = nullptr;

	// Lower case, because the box is a convenience and not a query language
	std::string filter;

	std::string currentXmlPath;
	Physics::NodePath currentNode;

	// What each row of the two lists points at, addressed like the tree items and for the same reason
	std::vector<std::pair<std::string, Physics::NodePath>> collisionRows;
	std::vector<std::pair<std::string, Physics::NodePath>> diagnosticRows;
};
