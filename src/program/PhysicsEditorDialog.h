/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../physics/XmlEditSession.h"
#include "../ui/wxPhysicsEditorDlg.h"

#include <string>
#include <utility>
#include <vector>

namespace tinyxml2 {
class XMLElement;
}

class OutfitStudioFrame;

/*
The physics editor: a tree of the physics XMLs the loaded meshes reference,
their bones, shapes and constraints, and a property grid generated from the
schema table for whatever is selected.

Non-modal on purpose - it edits documents the running simulation is reading, so
the viewport behind it has to stay live and usable.

Owns nothing. The documents belong to the frame's Physics::XmlEditSession, so
closing this window does not throw away edits or revert the preview.
*/
class PhysicsEditorDialog : public wxPhysicsEditorDlg {
public:
	explicit PhysicsEditorDialog(OutfitStudioFrame* parent);

	// Rebuilds the tree from the session, keeping the selected element when it
	// is still there. Call after documents are adopted, closed or reloaded.
	void RefreshDocuments();

	// Selects and reveals the document for "xmlPath", if the tree has it.
	void SelectDocument(const std::string& xmlPath);

private:
	/*
	What a tree item stands for. Documents are addressed by path and elements
	by NodePath, never by pointer: undo reparses a whole document, which
	invalidates every pointer into it, and a stale pointer in a tree item would
	be a use after free.

	An empty node path means the item is a document row or a category heading.
	*/
	class ItemData : public wxTreeItemData {
	public:
		ItemData(std::string inXmlPath, Physics::NodePath inNode)
			: xmlPath(std::move(inXmlPath))
			, node(std::move(inNode)) {}

		std::string xmlPath;
		Physics::NodePath node;
	};

	Physics::XmlEditSession* Session() const;

	void PopulateTree();
	// Adds one heading with the count of what is under it, and a row per
	// element. Adds nothing when there is nothing of that kind.
	void AddCategory(const wxTreeItemId& parent,
					 Physics::XmlDocument& doc,
					 const wxString& label,
					 std::initializer_list<Physics::ElementKind> kinds,
					 bool nestConstraintGroups = false);
	// "showKind" names the element's kind after its name, for the headings
	// that merge several kinds and where the name alone does not say which.
	void AddElementItem(const wxTreeItemId& parent, Physics::XmlDocument& doc, tinyxml2::XMLElement* element, bool showKind);

	// True when "element" passes the filter box, either itself or through one
	// of the elements nested in it.
	bool MatchesFilter(tinyxml2::XMLElement* element) const;

	void ShowElement(Physics::XmlDocument* doc, tinyxml2::XMLElement* element);
	// The tree row standing for an element, or an invalid id when the tree
	// does not have it - which is a normal outcome of an edit that removed
	// the element.
	wxTreeItemId FindTreeItem(const std::string& xmlPath, const Physics::NodePath& node) const;

	// Selects the tree row for an element, so a diagnostic or a collision
	// verdict can be followed back to what it is about.
	void SelectNode(const std::string& xmlPath, const Physics::NodePath& node);

	// The names a value that has to name something else can be picked from:
	// the skeleton for a bone, the loaded meshes for a NiShape, the document
	// for a collision shape, and every open document for a tag. Editable
	// wherever it is offered, so a name none of them knows still round trips.
	wxArrayString ChoicesFor(Physics::ValueKind kind, const Physics::XmlDocument& doc) const;

	// Every other skinned shape, and whether this one would ever be tested
	// against it. Empty for anything that is not a skinned shape.
	void ShowCollisions(Physics::XmlDocument* doc, tinyxml2::XMLElement* element);

	// Revalidates the document and fills the diagnostics page.
	void ShowDiagnostics(Physics::XmlDocument* doc);

	// What validation needs from outside the file: the loaded meshes and the
	// skeleton. Both empty while no project is open, which turns those checks
	// off rather than making everything look wrong.
	Physics::ValidationContext ValidationInputs() const;
	// The shapes selected in Outfit Studio, by name. What linking acts on: the
	// physics link lives on the model, not in the XML.
	std::vector<std::string> SelectedShapeNames() const;

	// Writes the document's link onto the selected shapes and tells the frame
	// the project has changed.
	void LinkToSelectedShapes(Physics::XmlDocument& doc);

	// Enables the file actions for what the selected document can actually do,
	// and says in the tooltip why one of them cannot.
	void UpdateFileControls();


	void ShowNothing(const wxString& reason);
	void AppendUnknownElement(tinyxml2::XMLElement* element);

	// The element the grid is showing. Held as a path rather than a pointer
	// because undo reparses the document out from under every pointer into it.
	Physics::XmlDocument* CurrentDocument() const;
	tinyxml2::XMLElement* CurrentElement() const;

	// Writes one edited value into the document and hands it to the frame,
	// which patches the running simulation or schedules a rebuild.
	void ApplyChange(const wxPGProperty* property, const Physics::ChildDesc* child, const Physics::AttrDesc* attr);

	// Re-reads everything after a change that could have moved elements around
	// - an undo, a redo - and tells the frame the simulation is out of date.
	void ReloadAfterDocumentChange();

	// Enables the undo and redo buttons for what the selected document holds,
	// and marks it in the tree while it has unsaved edits.
	void UpdateEditControls();

	void doTreeSelectionChanged(wxTreeEvent& event) override;
	void doFilterChanged(wxCommandEvent& event) override;
	void doNewXml(wxCommandEvent& event) override;
	void doImportXml(wxCommandEvent& event) override;
	void doExportXml(wxCommandEvent& event) override;
	void doSaveXml(wxCommandEvent& event) override;
	void doSaveAllXml(wxCommandEvent& event) override;
	void doLinkXml(wxCommandEvent& event) override;
	void doPropertyChanged(wxPropertyGridEvent& event) override;
	void doCollisionActivated(wxListEvent& event) override;
	void doDiagnosticActivated(wxListEvent& event) override;
	void doUndo(wxCommandEvent& event) override;
	void doRedo(wxCommandEvent& event) override;
	void doCloseButton(wxCommandEvent& event) override;
	void doCloseWindow(wxCloseEvent& event) override;

	OutfitStudioFrame* os = nullptr;
	// Lower cased, because the box is a convenience and not a query language
	std::string filter;

	std::string currentXmlPath;
	Physics::NodePath currentNode;

	// What each row of the two lists points at, addressed the way the tree
	// items are and for the same reason.
	std::vector<std::pair<std::string, Physics::NodePath>> collisionRows;
	std::vector<std::pair<std::string, Physics::NodePath>> diagnosticRows;
};
