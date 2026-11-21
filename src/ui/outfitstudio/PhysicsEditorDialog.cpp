/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsEditorDialog.h"
#include "outfitstudio/PhysicsXmlEditor.h"
#include "physics/XmlEditSession.h"
#include "utils/ConfigurationManager.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <tinyxml2.h>
#include <unordered_set>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/propgrid/advprops.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace Physics;

namespace {

wxString ResourcePath() {
	return wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/PhysicsEditor.xrc";
}

wxString FromSchema(const char* text) {
	return text ? wxString::FromUTF8(text) : wxString();
}

wxString FileNameOf(const std::string& xmlPath) {
	return wxString::FromUTF8(XmlFileName(xmlPath));
}

// A document's row in the tree, marked while it has unsaved edits
wxString DocumentLabel(const XmlDocument& doc) {
	wxString label = FileNameOf(doc.XmlPath());
	if (doc.IsDirty())
		label += " *";
	return label;
}

// Grey, for a value that isn't in the file and is only shown so the grid isn't full of blanks
wxColour InheritedColour(const wxPropertyGrid* grid) {
	const wxColour fg = grid->GetCellTextColour();
	const wxColour bg = grid->GetCellBackgroundColour();
	return wxColour((fg.Red() + bg.Red()) / 2, (fg.Green() + bg.Green()) / 2, (fg.Blue() + bg.Blue()) / 2);
}

// A value that names something else: it gets a picker filled from whatever declares those names, rather than a bare
// text field
bool IsNameKind(ValueKind kind) {
	switch (kind) {
		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: return true;
		default: return false;
	}
}

// A transform is a frame of its own with three spellings, and a "None" child carries structure rather than a value.
// Both are edited through their own rows rather than in a cell.
bool IsEditableKind(ValueKind kind) {
	return kind != ValueKind::Transform && kind != ValueKind::None;
}

void AppendProperty(wxPropertyGrid* grid, wxPGProperty* property, bool editable) {
	grid->Append(property);
	if (!editable)
		grid->SetPropertyReadOnly(property);
}

// The row a change belongs to: the topmost property under its category, so a component of a vector reports the vector
const wxPGProperty* RootProperty(const wxPGProperty* property) {
	while (property) {
		const wxPGProperty* parent = property->GetParent();
		if (!parent || parent->IsCategory() || parent->IsRoot())
			return property;

		property = parent;
	}

	return nullptr;
}

// Reads a row back out of the grid as the kind the schema says it is
ValueVariant ReadProperty(const wxPGProperty* property, ValueKind kind, Multiplicity mult) {
	auto component = [property](unsigned index) -> float {
		return index < property->GetChildCount() ? static_cast<float>(property->Item(index)->GetValue().GetDouble()) : 0.0f;
	};

	if (mult == Multiplicity::List) {
		std::vector<std::string> items;
		for (const wxString& item : property->GetValue().GetArrayString())
			items.push_back(std::string(item.ToUTF8()));

		return items;
	}

	switch (kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: return static_cast<float>(property->GetValue().GetDouble());

		case ValueKind::Int: return static_cast<int>(property->GetValue().GetLong());
		case ValueKind::Bool: return property->GetValue().GetBool();

		case ValueKind::Vec3: return nifly::Vector3(component(0), component(1), component(2));

		// The children go in x, y, z, w order; nifly::Quaternion takes w first
		case ValueKind::Quaternion: return nifly::Quaternion(component(3), component(0), component(1), component(2));

		case ValueKind::AxisAngle: {
			AxisAngleValue value;
			value.axis = nifly::Vector3(component(0), component(1), component(2));
			value.angle = component(3);
			return value;
		}

		case ValueKind::Lerp: {
			LerpValue value;
			value.translation = component(0);
			value.rotation = component(1);
			return value;
		}

		case ValueKind::Transform:
		case ValueKind::None: return std::monostate();

		default: break;
	}

	return std::string(property->GetValueAsString().ToUTF8());
}

// A parent row with an editable number per component, so a Vec3 reads as "1; 1; 1" collapsed
wxPGProperty* ComposedProperty(const wxString& label, const wxString& name, std::initializer_list<std::pair<wxString, float>> components) {
	auto* parent = new wxStringProperty(label, name);
#if wxCHECK_VERSION(3, 3, 0)
	parent->ChangeFlag(wxPGFlags::ComposedValue, true);
#else
	// The Linux builds use the wxWidgets 3.2 of the distribution, from before the flags became an enum class
	parent->ChangeFlag(wxPG_PROP_COMPOSED_VALUE, true);
#endif
	for (auto& [componentName, value] : components)
		parent->AppendChild(new wxFloatProperty(componentName, name + "." + componentName, value));
	return parent;
}

template<typename T>
T ValueOr(const ValueVariant& value, T fallback) {
	const T* held = std::get_if<T>(&value);
	return held ? *held : fallback;
}

// One property for a value of "kind"
wxPGProperty* MakeProperty(const wxString& label,
						   const wxString& name,
						   ValueKind kind,
						   Multiplicity mult,
						   const EnumValues* enums,
						   const wxArrayString& choices,
						   const ValueVariant& value) {
	if (mult == Multiplicity::List) {
		wxArrayString items;
		for (const std::string& item : ValueOr(value, std::vector<std::string>()))
			items.Add(wxString::FromUTF8(item));

		if (!IsNameKind(kind))
			return new wxArrayStringProperty(label, name, items);

		// A list of names is picked from what exists. What the file has joins the offered names, so an edit can't
		// quietly drop a name this machine doesn't know.
		wxArrayString offered = choices;
		for (const wxString& item : items)
			if (offered.Index(item, false) == wxNOT_FOUND)
				offered.Add(item);

		return new wxMultiChoiceProperty(label, name, offered, items);
	}

	switch (kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: return new wxFloatProperty(label, name, ValueOr(value, 0.0f));

		case ValueKind::Int: return new wxIntProperty(label, name, ValueOr(value, 0));
		case ValueKind::Bool: return new wxBoolProperty(label, name, ValueOr(value, false));

		case ValueKind::Enum: {
			wxArrayString listed;
			if (enums)
				for (std::size_t i = 0; i < enums->count; ++i)
					listed.Add(FromSchema(enums->values[i]));

			// Editable, so a value the schema doesn't list still shows what the file says
			return new wxEditEnumProperty(label, name, listed, wxArrayInt(), wxString::FromUTF8(ValueOr(value, std::string())));
		}

		case ValueKind::Vec3: {
			const auto v = ValueOr(value, nifly::Vector3());
			return ComposedProperty(label, name, {{"x", v.x}, {"y", v.y}, {"z", v.z}});
		}

		case ValueKind::Quaternion: {
			const auto q = ValueOr(value, nifly::Quaternion());
			return ComposedProperty(label, name, {{"x", q.x}, {"y", q.y}, {"z", q.z}, {"w", q.w}});
		}

		case ValueKind::AxisAngle: {
			const auto a = ValueOr(value, AxisAngleValue());
			return ComposedProperty(label, name, {{"x", a.axis.x}, {"y", a.axis.y}, {"z", a.axis.z}, {"angle", a.angle}});
		}

		case ValueKind::Lerp: {
			const auto l = ValueOr(value, LerpValue());
			return ComposedProperty(label, name, {{"translation", l.translation}, {"rotation", l.rotation}});
		}

		// A frame of its own, with a choice of three spellings
		case ValueKind::Transform: return new wxStringProperty(label, name, _("(edited on its own row)"));
		case ValueKind::None: return new wxStringProperty(label, name, wxString());

		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: {
			const wxString current = wxString::FromUTF8(ValueOr(value, std::string()));

			wxArrayString offered = choices;
			if (!current.empty() && offered.Index(current, false) == wxNOT_FOUND)
				offered.Insert(current, 0);

			// Editable, so a name nothing loaded here knows is still a name the file keeps
			return new wxEditEnumProperty(label, name, offered, wxArrayInt(), current);
		}

		case ValueKind::String: break;
	}

	return new wxStringProperty(label, name, wxString::FromUTF8(ValueOr(value, std::string())));
}

} // namespace

PhysicsEditorDialog::PhysicsEditorDialog(wxWindow* parent, PhysicsXmlEditor& owner)
	: owner(&owner) {
	wxXmlResource* xrc = wxXmlResource::Get();
	if (!xrc->Load(ResourcePath())) {
		wxMessageBox(_("Failed to load PhysicsEditor.xrc file!"), _("Error"), wxICON_ERROR);
		return;
	}

	if (!xrc->LoadFrame(this, parent, "dlgPhysicsEditor")) {
		wxMessageBox(_("Failed to load the physics editor frame!"), _("Error"), wxICON_ERROR);
		return;
	}

	btnNew = XRCCTRL(*this, "physXmlNew", wxButton);
	btnImport = XRCCTRL(*this, "physXmlImport", wxButton);
	btnExport = XRCCTRL(*this, "physXmlExport", wxButton);
	btnSave = XRCCTRL(*this, "physXmlSave", wxButton);
	btnSaveAll = XRCCTRL(*this, "physXmlSaveAll", wxButton);
	btnLink = XRCCTRL(*this, "physXmlLink", wxButton);
	filterBox = XRCCTRL(*this, "physFilter", wxSearchCtrl);
	tree = XRCCTRL(*this, "physTree", wxTreeCtrl);
	elementLabel = XRCCTRL(*this, "physElementLabel", wxStaticText);
	infoBook = XRCCTRL(*this, "physInfoBook", wxNotebook);
	collisionList = XRCCTRL(*this, "physCollisionList", wxListCtrl);
	diagnosticList = XRCCTRL(*this, "physDiagnosticList", wxListCtrl);
	status = XRCCTRL(*this, "physStatus", wxStaticText);
	btnUndo = XRCCTRL(*this, "physUndo", wxButton);
	btnRedo = XRCCTRL(*this, "physRedo", wxButton);

	// The grid is generated from the schema table rather than described in the XRC, so it goes in as an unknown control
	wxWindow* gridPanel = XRCCTRL(*this, "physGridPanel", wxPanel);
	grid = new wxPropertyGrid(gridPanel ? gridPanel : this,
							  wxID_ANY,
							  wxDefaultPosition,
							  wxDefaultSize,
							  wxPG_DEFAULT_STYLE | wxPG_SPLITTER_AUTO_CENTER | wxPG_TOOLTIPS);
	grid->SetExtraStyle(wxPG_EX_HELP_AS_TOOLTIPS);
	xrc->AttachUnknownControl("pgProperties", grid, this);

	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnNewXml, this, XRCID("physXmlNew"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnImportXml, this, XRCID("physXmlImport"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnExportXml, this, XRCID("physXmlExport"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnSaveXml, this, XRCID("physXmlSave"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnSaveAllXml, this, XRCID("physXmlSaveAll"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnLinkXml, this, XRCID("physXmlLink"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnUndo, this, XRCID("physUndo"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnRedo, this, XRCID("physRedo"));
	Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); }, XRCID("physClose"));
	Bind(wxEVT_TREE_SEL_CHANGED, &PhysicsEditorDialog::OnTreeSelectionChanged, this, XRCID("physTree"));
	Bind(wxEVT_TEXT, &PhysicsEditorDialog::OnFilterChanged, this, XRCID("physFilter"));
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &PhysicsEditorDialog::OnCollisionActivated, this, XRCID("physCollisionList"));
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &PhysicsEditorDialog::OnDiagnosticActivated, this, XRCID("physDiagnosticList"));
	Bind(wxEVT_CLOSE_WINDOW, &PhysicsEditorDialog::OnCloseWindow, this);
	grid->Bind(wxEVT_PG_CHANGED, &PhysicsEditorDialog::OnPropertyChanged, this);

	// The grid eats plain keys while a cell is edited, so undo and redo go on the frame
	const wxAcceleratorEntry accelerators[]
		= {wxAcceleratorEntry(wxACCEL_CTRL, 'Z', XRCID("physUndo")), wxAcceleratorEntry(wxACCEL_CTRL, 'Y', XRCID("physRedo"))};
	SetAcceleratorTable(wxAcceleratorTable(WXSIZEOF(accelerators), accelerators));

	// Growing the window grows the property grid, instead of handing every extra pixel to the lists below it
	if (auto* rightSplitter = XRCCTRL(*this, "physRightSplitter", wxSplitterWindow))
		rightSplitter->SetSashGravity(1.0);

	// The labels show schema help lines. Without a small minimum they ask for the room of the whole line and push the
	// buttons beside them off the bar instead of ellipsizing.
	status->SetMinSize(FromDIP(wxSize(120, -1)));
	elementLabel->SetMinSize(FromDIP(wxSize(120, -1)));

	// The columns are decided by what the editor puts in the lists
	collisionList->AppendColumn(_("Skinned shape"), wxLIST_FORMAT_LEFT, FromDIP(160));
	collisionList->AppendColumn(_("File"), wxLIST_FORMAT_LEFT, FromDIP(130));
	collisionList->AppendColumn(_("Collides"), wxLIST_FORMAT_LEFT, FromDIP(70));
	collisionList->AppendColumn(_("Why"), wxLIST_FORMAT_LEFT, FromDIP(460));

	diagnosticList->AppendColumn(_("Severity"), wxLIST_FORMAT_LEFT, FromDIP(70));
	diagnosticList->AppendColumn(_("Element"), wxLIST_FORMAT_LEFT, FromDIP(190));
	diagnosticList->AppendColumn(_("Detail"), wxLIST_FORMAT_LEFT, FromDIP(640));

	// A frame from XRC comes up at the minimum of its sizer, far too small for a tree beside a property grid
	SetMinSize(FromDIP(wxSize(600, 400)));
	SetSize(FromDIP(wxSize(980, 640)));
	Layout();

	loaded = true;

	PopulateTree();
	ShowNothing(_("Nothing selected"));
}

PhysicsEditorDialog::~PhysicsEditorDialog() {
	if (owner)
		owner->EditorClosed();

	wxXmlResource::Get()->Unload(ResourcePath());
}

void PhysicsEditorDialog::OnCloseWindow(wxCloseEvent& event) {
	// The documents outlive this window, so there is nothing to save or discard here. The owner forgets the window
	// right away rather than once it is destroyed at the next idle time.
	if (owner)
		owner->EditorClosed();
	owner = nullptr;

	event.Skip();
}

XmlEditSession* PhysicsEditorDialog::Session() const {
	return owner ? &owner->Session() : nullptr;
}

void PhysicsEditorDialog::RefreshDocuments() {
	if (!loaded)
		return;

	// Where the user was, by address: the tree is about to be rebuilt
	std::string selectedPath;
	NodePath selectedNode;
	if (const wxTreeItemId selected = tree->GetSelection(); selected.IsOk()) {
		if (auto* data = static_cast<ItemData*>(tree->GetItemData(selected))) {
			selectedPath = data->xmlPath;
			selectedNode = data->node;
		}
	}

	PopulateTree();

	if (selectedPath.empty()) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	// Not found means the element is gone, a normal outcome of an edit
	const wxTreeItemId restored = FindTreeItem(selectedPath, selectedNode);
	if (!restored.IsOk()) {
		ShowNothing(_("The selected element is no longer in the document"));
		return;
	}

	tree->SelectItem(restored);
	tree->EnsureVisible(restored);
}

void PhysicsEditorDialog::SelectDocument(const std::string& xmlPath) {
	if (!loaded)
		return;

	const wxTreeItemId item = FindDocumentItem(xmlPath);
	if (item.IsOk()) {
		tree->SelectItem(item);
		tree->EnsureVisible(item);
	}
}

wxTreeItemId PhysicsEditorDialog::FindDocumentItem(const std::string& xmlPath) const {
	const wxTreeItemId root = tree->GetRootItem();
	if (!root.IsOk())
		return wxTreeItemId();

	const std::string wanted = XmlEditSession::NormalizeXmlPath(xmlPath);

	wxTreeItemIdValue cookie;
	for (wxTreeItemId item = tree->GetFirstChild(root, cookie); item.IsOk(); item = tree->GetNextChild(root, cookie)) {
		auto* data = static_cast<ItemData*>(tree->GetItemData(item));
		if (data && XmlEditSession::NormalizeXmlPath(data->xmlPath) == wanted)
			return item;
	}

	return wxTreeItemId();
}

void PhysicsEditorDialog::PopulateTree() {
	tree->DeleteAllItems();

	XmlEditSession* session = Session();
	if (!session)
		return;

	const wxTreeItemId root = tree->AddRoot("physics");

	const std::vector<XmlDocument*> documents = session->Documents();
	if (documents.empty()) {
		tree->AppendItem(root, _("No physics XMLs are loaded"));
		return;
	}

	for (XmlDocument* doc : documents) {
		const wxTreeItemId docItem = tree->AppendItem(root, DocumentLabel(*doc), -1, -1, new ItemData(doc->XmlPath(), NodePath()));

		AddCategory(docItem, *doc, _("Bones"), {ElementKind::Bone});

		// Not "Shapes": that means the meshes of the outfit everywhere else in this application
		AddCategory(docItem, *doc, _("Skinned shapes"), {ElementKind::PerVertexShape, ElementKind::PerTriangleShape});
		AddCategory(docItem,
					*doc,
					_("Constraints"),
					{ElementKind::GenericConstraint, ElementKind::StiffSpringConstraint, ElementKind::ConeTwistConstraint, ElementKind::ConstraintGroup},
					true);
		AddCategory(docItem, *doc, _("Collision shapes"), {ElementKind::Shape});
		AddCategory(docItem,
					*doc,
					_("Defaults"),
					{ElementKind::BoneDefault,
					 ElementKind::PerVertexShapeDefault,
					 ElementKind::PerTriangleShapeDefault,
					 ElementKind::GenericConstraintDefault,
					 ElementKind::StiffSpringConstraintDefault,
					 ElementKind::ConeTwistConstraintDefault});

		// Elements the schema table doesn't describe are kept and written back untouched, so they have to be visible
		AddCategory(docItem, *doc, _("Not in the schema"), {ElementKind::Unknown});

		tree->Expand(docItem);
	}
}

void PhysicsEditorDialog::AddCategory(const wxTreeItemId& parent,
									  XmlDocument& doc,
									  const wxString& label,
									  std::initializer_list<ElementKind> kinds,
									  bool nestConstraintGroups) {
	std::vector<tinyxml2::XMLElement*> shown;
	for (tinyxml2::XMLElement* element : doc.ChildrenOfKinds(kinds))
		if (MatchesFilter(element))
			shown.push_back(element);

	if (shown.empty())
		return;

	// The count is of what is shown, so a filtered tree doesn't claim more than it lists
	const wxTreeItemId categoryItem
		= tree->AppendItem(parent, wxString::Format("%s (%zu)", label, shown.size()), -1, -1, new ItemData(doc.XmlPath(), NodePath()));

	// Under a heading of one kind, naming the kind on every row is noise
	const bool showKind = kinds.size() > 1;

	for (tinyxml2::XMLElement* element : shown) {
		AddElementItem(categoryItem, doc, element, showKind);

		if (!nestConstraintGroups || KindFromName(element->Name()) != ElementKind::ConstraintGroup)
			continue;

		// A constraint-group holds its constraints, so they go under it, which is also where the builder finds them
		const wxTreeItemId groupItem = tree->GetLastChild(categoryItem);
		for (tinyxml2::XMLElement* nested :
			 doc.ChildrenOfKinds({ElementKind::GenericConstraint, ElementKind::StiffSpringConstraint, ElementKind::ConeTwistConstraint}, element))
			AddElementItem(groupItem, doc, nested, true);
	}
}

void PhysicsEditorDialog::AddElementItem(const wxTreeItemId& parent, XmlDocument& doc, tinyxml2::XMLElement* element, bool showKind) {
	wxString label = wxString::FromUTF8(XmlDocument::DisplayName(element));

	// DisplayName falls back to the element's tag, which would then be repeated: "bone-default [bone-default]"
	const ElementKind kind = KindFromName(element->Name());
	if (showKind && kind != ElementKind::Unknown && label != FromSchema(NameFromKind(kind)))
		label += wxString::Format("  [%s]", FromSchema(NameFromKind(kind)));

	tree->AppendItem(parent, label, -1, -1, new ItemData(doc.XmlPath(), doc.PathOf(element)));
}

bool PhysicsEditorDialog::MatchesFilter(tinyxml2::XMLElement* element) const {
	if (filter.empty())
		return true;

	if (ToLower(XmlDocument::DisplayName(element)).find(filter) != std::string::npos)
		return true;

	if (element->Name() && ToLower(element->Name()).find(filter) != std::string::npos)
		return true;

	// A group whose own name says nothing is still reachable when what was typed is inside it
	for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		if (MatchesFilter(child))
			return true;

	return false;
}

void PhysicsEditorDialog::ShowNothing(const wxString& reason) {
	grid->Clear();
	elementLabel->SetLabel(reason);
	status->SetLabel(wxString());
	currentNode = NodePath();

	ShowCollisions(nullptr, nullptr);
	ShowDiagnostics(nullptr);
}

void PhysicsEditorDialog::UpdateEditControls() {
	const XmlDocument* doc = CurrentDocument();

	btnUndo->Enable(doc && doc->CanUndo());
	btnRedo->Enable(doc && doc->CanRedo());

	// Naming the step says what Ctrl+Z is about to take back
	btnUndo->SetToolTip(doc && doc->CanUndo() ? wxString::Format(_("Undo %s (Ctrl+Z)"), wxString::FromUTF8(doc->UndoLabel())) : _("Nothing to undo"));
	btnRedo->SetToolTip(doc && doc->CanRedo() ? wxString::Format(_("Redo %s (Ctrl+Y)"), wxString::FromUTF8(doc->RedoLabel())) : _("Nothing to redo"));

	UpdateFileControls();

	if (!doc)
		return;

	const wxTreeItemId item = FindDocumentItem(doc->XmlPath());
	if (item.IsOk() && tree->GetItemText(item) != DocumentLabel(*doc))
		tree->SetItemText(item, DocumentLabel(*doc));
}

void PhysicsEditorDialog::UpdateFileControls() {
	XmlEditSession* session = Session();
	const XmlDocument* doc = CurrentDocument();

	btnNew->Enable(owner && owner->HasProject());
	btnImport->Enable(doc != nullptr);
	btnExport->Enable(doc != nullptr);

	// Enabled whatever is selected in the viewport: being told what to select beats a button that is dead for a
	// reason that can't be seen from here
	btnLink->Enable(doc != nullptr);

	// A document out of an archive, or one that was never written, has nothing to be saved back over
	const bool overwritable = doc && doc->Origin() == XmlOrigin::Loose && !doc->SourcePath().empty();
	btnSave->Enable(overwritable && doc->IsDirty());
	btnSaveAll->Enable(session && session->AnyDirty());

	if (!doc)
		btnSave->SetToolTip(_("Select a physics XML first"));
	else if (!overwritable)
		btnSave->SetToolTip(_("This file did not come from a loose file on disk. Use Export instead."));
	else if (!doc->IsDirty())
		btnSave->SetToolTip(_("No unsaved changes"));
	else
		// The values are written back exactly, but the whitespace around them is the printer's
		btnSave->SetToolTip(wxString::Format(_("Overwrite %s.\nEverything it says is preserved, but the file is re-indented."), wxString::FromUTF8(doc->SourcePath())));
}

void PhysicsEditorDialog::ApplyChange(const wxPGProperty* property, const ChildDesc* child, const AttrDesc* attr) {
	XmlDocument* doc = CurrentDocument();
	tinyxml2::XMLElement* element = CurrentElement();
	if (!doc || !element || (!child && !attr))
		return;

	const ValueKind kind = child ? child->kind : attr->kind;
	const Multiplicity mult = child ? child->mult : Multiplicity::Once;
	const ValueVariant value = ReadProperty(property, kind, mult);

	// The descriptor is a static table entry, so its address identifies the property being dragged; changes to it
	// within half a second fold into one undo step
	doc->BeginEdit(child ? child->name : attr->name, currentNode, child ? static_cast<const void*>(child) : static_cast<const void*>(attr));

	const bool written = child ? doc->SetValue(element, *child, value) : doc->SetAttr(element, *attr, value);
	if (!written) {
		// The document didn't take it, so there is no edit to keep
		doc->Undo();
		UpdateEditControls();
		return;
	}

	if (owner) {
		// An attribute names something the whole system was built around, which can't be patched in place
		if (attr)
			owner->DocumentChanged(doc->XmlPath());
		else
			owner->PropertyChanged(*doc, element, *child);
	}

	UpdateEditControls();

	// A tag or a collide list changes the collisions page, and almost any edit can raise or clear a diagnostic
	ShowCollisions(doc, element);
	ShowDiagnostics(doc);
}

void PhysicsEditorDialog::ReloadAfterDocumentChange() {
	// Undo reparses the document, so every pointer into it is stale. The tree addresses elements by path, which
	// is also what lets the selection survive the rebuild.
	RefreshDocuments();

	XmlDocument* doc = CurrentDocument();
	tinyxml2::XMLElement* element = CurrentElement();
	if (doc && element)
		ShowElement(doc, element);

	UpdateEditControls();

	if (owner && doc)
		owner->DocumentChanged(doc->XmlPath());
}

void PhysicsEditorDialog::AppendUnknownElement(tinyxml2::XMLElement* element) {
	grid->Append(new wxPropertyCategory(_("Attributes")));

	// Read only: the schema says nothing about what these mean. They are kept and written back as they are.
	for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next())
		AppendProperty(grid, new wxStringProperty(wxString::FromUTF8(attr->Name()), wxString::FromUTF8(attr->Name()), wxString::FromUTF8(attr->Value())), false);

	const char* text = element->GetText();
	if (text && *text) {
		grid->Append(new wxPropertyCategory(_("Text")));
		AppendProperty(grid, new wxStringProperty(_("value"), "text", wxString::FromUTF8(text)), false);
	}
}

XmlDocument* PhysicsEditorDialog::CurrentDocument() const {
	XmlEditSession* session = Session();
	return session && !currentXmlPath.empty() ? session->Find(currentXmlPath) : nullptr;
}

tinyxml2::XMLElement* PhysicsEditorDialog::CurrentElement() const {
	XmlDocument* doc = CurrentDocument();
	return doc && !currentNode.Empty() ? doc->Resolve(currentNode) : nullptr;
}

void PhysicsEditorDialog::ShowElement(XmlDocument* doc, tinyxml2::XMLElement* element) {
	grid->Clear();
	status->SetLabel(wxString());

	if (!doc || !element) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	currentXmlPath = doc->XmlPath();
	currentNode = doc->PathOf(element);

	const wxString displayName = wxString::FromUTF8(XmlDocument::DisplayName(element));
	elementLabel->SetLabel(wxString::Format("%s  -  %s", wxString::FromUTF8(element->Name() ? element->Name() : ""), displayName));

	const ElementDesc* desc = FindElement(element->Name());
	if (!desc) {
		status->SetLabel(_("Not part of the hdtSMP64 schema. It is preserved, but not understood."));
		AppendUnknownElement(element);
		ShowCollisions(doc, element);
		ShowDiagnostics(doc);
		return;
	}

	if (desc->help)
		status->SetLabel(FromSchema(desc->help));

	const wxColour inherited = InheritedColour(grid);

	if (desc->attrCount > 0) {
		grid->Append(new wxPropertyCategory(_("Attributes")));

		for (std::size_t i = 0; i < desc->attrCount; ++i) {
			const AttrDesc& attr = desc->attrs[i];
			const ValueVariant value = doc->GetAttr(element, attr);

			// Attributes are named with a leading @, so an attribute and a child of the same name can't collide
			wxPGProperty* property
				= MakeProperty(FromSchema(attr.name), "@" + FromSchema(attr.name), attr.kind, Multiplicity::Once, attr.enumValues, ChoicesFor(attr.kind, *doc), value);

			wxString help = FromSchema(attr.help);
			if (attr.required)
				help += help.empty() ? _("Required.") : _("\nRequired.");

			// Every attribute here names something - a bone, a shape, a template - so changing one rebuilds
			help += _("\nChanging this rebuilds the simulation.");

			property->SetHelpString(help);
			AppendProperty(grid, property, IsEditableKind(attr.kind));

			if (std::holds_alternative<std::monostate>(value))
				grid->SetPropertyTextColour(property, inherited);
		}
	}

	// Split by whether this preview does anything with the value, so a setting that won't move the mesh can't be
	// mistaken for one that will
	std::vector<const ChildDesc*> simulated;
	std::vector<const ChildDesc*> notSimulated;
	for (std::size_t i = 0; i < desc->childCount; ++i) {
		const ChildDesc& child = desc->children[i];
		(child.preview == PreviewSupport::Simulated ? simulated : notSimulated).push_back(&child);
	}

	auto appendChildren = [&](const std::vector<const ChildDesc*>& children, const wxString& categoryLabel) {
		if (children.empty())
			return;

		grid->Append(new wxPropertyCategory(categoryLabel));

		for (const ChildDesc* child : children) {
			// What the element is really built with: a property nobody set is decided by the ambient *-default
			// cascade, and the schema default in its place would be a different number from the simulation's
			const EffectiveValue effective = doc->ResolveEffective(element, *child);
			const bool absent = !effective.explicitHere;

			wxPGProperty* property = MakeProperty(
				FromSchema(child->name), FromSchema(child->name), child->kind, child->mult, child->enumValues, ChoicesFor(child->kind, *doc), effective.value);

			if (child->unit)
				property->SetAttribute(wxPG_ATTR_UNITS, FromSchema(child->unit));
			if (child->rangeMin != -kUnbounded)
				property->SetAttribute(wxPG_ATTR_MIN, child->rangeMin);
			if (child->rangeMax != kUnbounded)
				property->SetAttribute(wxPG_ATTR_MAX, child->rangeMax);

			wxString help = FromSchema(child->help);
			if (child->defaultText)
				help += wxString::Format(_("\nDefault: %s"), FromSchema(child->defaultText));
			if (absent && effective.source)
				help += wxString::Format(_("\nNot set here; the value shown is inherited from %s."), wxString::FromUTF8(effective.sourceLabel));
			else if (absent && CascadeApplies(KindFromName(element->Name())))
				help += _("\nNot set here, and no *-default before it supplies one, so the value shown is the schema default.");
			else if (absent)
				help += _("\nNot set here; the value shown is the schema default.");
			if (child->preview != PreviewSupport::Simulated)
				help += "\n" + FromSchema(PreviewSupportNote(child->preview));
			if (child->patch == PatchKind::Structural)
				help += _("\nChanging this rebuilds the simulation.");

			property->SetHelpString(help);
			AppendProperty(grid, property, IsEditableKind(child->kind));

			if (absent)
				grid->SetPropertyTextColour(property, inherited);
		}
	};

	appendChildren(simulated, _("Properties"));
	appendChildren(notSimulated, _("Not simulated in this preview"));

	ShowCollisions(doc, element);
	ShowDiagnostics(doc);
}

wxArrayString PhysicsEditorDialog::ChoicesFor(ValueKind kind, const XmlDocument& doc) const {
	wxArrayString choices;

	// A skeleton has several hundred bones and this runs for every bone reference of the element, so the duplicate
	// check can't be a scan of what was added. Case insensitive, the way the names are matched.
	std::unordered_set<std::string> seen;
	auto add = [&choices, &seen](const std::string& name) {
		if (!name.empty() && seen.insert(ToLower(name)).second)
			choices.Add(wxString::FromUTF8(name));
	};

	auto addNamed = [&add](const std::vector<tinyxml2::XMLElement*>& elements) {
		for (const tinyxml2::XMLElement* element : elements)
			if (const char* name = element->Attribute("name"))
				add(name);
	};

	switch (kind) {
		case ValueKind::BoneRef:
			// The bones the file declares first: a handful out of the hundreds of the skeleton, the ones the author
			// decided to simulate
			addNamed(doc.ChildrenOfKind(ElementKind::Bone));
			if (owner)
				for (const std::string& name : owner->BoneNames())
					add(name);
			break;

		case ValueKind::NiShapeRef:
			if (owner)
				for (const std::string& name : owner->ShapeNames())
					add(name);
			break;

		case ValueKind::ShapeRef:
			// Only a named top level <shape> can be pointed at
			addNamed(doc.ChildrenOfKind(ElementKind::Shape));
			break;

		case ValueKind::TagRef:
			// Tags cross files: a shape in one XML names a tag declared in another, which is how body and outfit meet
			if (XmlEditSession* session = Session())
				for (XmlDocument* other : session->Documents())
					for (const std::string& tag : other->CollectTags())
						add(tag);
			break;

		default: break;
	}

	return choices;
}

void PhysicsEditorDialog::ShowCollisions(XmlDocument* doc, tinyxml2::XMLElement* element) {
	collisionList->DeleteAllItems();
	collisionRows.clear();

	XmlEditSession* session = Session();
	if (!doc || !element || !session)
		return;

	const ElementKind kind = KindFromName(element->Name());
	if (kind != ElementKind::PerVertexShape && kind != ElementKind::PerTriangleShape) {
		collisionList->InsertItem(0, _("Only skinned shapes carry collision filtering."));
		return;
	}

	// Every skinned shape of every open document: the tag lists exist to arrange collisions between files
	std::vector<ShapeFacts> everything;
	for (XmlDocument* other : session->Documents()) {
		std::vector<ShapeFacts> facts = CollectShapeFacts(*other);
		everything.insert(everything.end(), facts.begin(), facts.end());
	}

	const NodePath node = doc->PathOf(element);
	const auto self = std::find_if(everything.begin(), everything.end(), [&](const ShapeFacts& facts) { return facts.xmlPath == doc->XmlPath() && facts.node == node; });
	if (self == everything.end())
		return;

	long row = 0;
	for (const ShapeFacts& other : everything) {
		if (&other == &*self)
			continue;

		const CollisionResult result = EvaluateCollision(*self, other);

		collisionList->InsertItem(row, wxString::FromUTF8(other.name));
		collisionList->SetItem(row, 1, FileNameOf(other.xmlPath));
		collisionList->SetItem(row, 2, result.collides ? _("collides") : _("no"));
		collisionList->SetItem(row, 3, wxString::FromUTF8(result.reason));

		// Muted for the pairs that never meet, so the ones that do stand out in a list that is mostly "no"
		if (!result.collides)
			collisionList->SetItemTextColour(row, InheritedColour(grid));

		collisionRows.emplace_back(other.xmlPath, other.node);
		row++;
	}

	if (row == 0)
		collisionList->InsertItem(0, _("No other skinned shape is loaded."));
}

void PhysicsEditorDialog::ShowDiagnostics(XmlDocument* doc) {
	diagnosticList->DeleteAllItems();
	diagnosticRows.clear();

	if (!doc || !owner)
		return;

	long row = 0;
	for (const Diagnostic& diagnostic : doc->Validate(owner->ValidationInputs())) {
		wxString severity;
		switch (diagnostic.severity) {
			case Diagnostic::Severity::Error: severity = _("Error"); break;
			case Diagnostic::Severity::Warning: severity = _("Warning"); break;
			case Diagnostic::Severity::Info: severity = _("Note"); break;
		}

		diagnosticList->InsertItem(row, severity);
		diagnosticList->SetItem(row, 1, wxString::FromUTF8(diagnostic.elementLabel));
		diagnosticList->SetItem(row, 2, wxString::FromUTF8(diagnostic.message));

		if (diagnostic.severity == Diagnostic::Severity::Info)
			diagnosticList->SetItemTextColour(row, InheritedColour(grid));

		diagnosticRows.emplace_back(doc->XmlPath(), diagnostic.node);
		row++;
	}

	// The tab carries the count, so a problem is visible without opening the page
	const int page = infoBook ? infoBook->FindPage(diagnosticList) : wxNOT_FOUND;
	if (page != wxNOT_FOUND)
		infoBook->SetPageText(page, row > 0 ? wxString::Format(_("Diagnostics (%ld)"), row) : _("Diagnostics"));

	if (row == 0)
		diagnosticList->InsertItem(0, _("Nothing to report."));
}

wxTreeItemId PhysicsEditorDialog::FindTreeItem(const std::string& xmlPath, const NodePath& node) const {
	// Breadth first, so an address several rows share (a document row and its headings all carry an empty node path)
	// resolves to the outermost one
	std::deque<wxTreeItemId> pending{tree->GetRootItem()};
	while (!pending.empty()) {
		const wxTreeItemId item = pending.front();
		pending.pop_front();
		if (!item.IsOk())
			continue;

		if (auto* data = static_cast<ItemData*>(tree->GetItemData(item)))
			if (data->xmlPath == xmlPath && data->node == node)
				return item;

		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = tree->GetFirstChild(item, cookie); child.IsOk(); child = tree->GetNextChild(item, cookie))
			pending.push_back(child);
	}

	return wxTreeItemId();
}

void PhysicsEditorDialog::SelectNode(const std::string& xmlPath, const NodePath& node) {
	// An empty path stands for a document or heading row, which nothing in the two lists points at
	if (node.Empty())
		return;

	const wxTreeItemId item = FindTreeItem(xmlPath, node);
	if (!item.IsOk())
		return;

	tree->SelectItem(item);
	tree->EnsureVisible(item);
}

void PhysicsEditorDialog::OnCollisionActivated(wxListEvent& event) {
	event.Skip();

	const long row = event.GetIndex();
	if (row >= 0 && static_cast<std::size_t>(row) < collisionRows.size())
		SelectNode(collisionRows[row].first, collisionRows[row].second);
}

void PhysicsEditorDialog::OnDiagnosticActivated(wxListEvent& event) {
	event.Skip();

	const long row = event.GetIndex();
	if (row >= 0 && static_cast<std::size_t>(row) < diagnosticRows.size())
		SelectNode(diagnosticRows[row].first, diagnosticRows[row].second);
}

void PhysicsEditorDialog::OnNewXml(wxCommandEvent& WXUNUSED(event)) {
	if (!owner)
		return;

	XmlDocument* doc = owner->CreateNew(this);
	if (!doc)
		return;

	const std::string xmlPath = doc->XmlPath();

	// A physics XML nothing links does nothing, so linking it is offered as part of creating it
	if (!owner->SelectedShapeNames().empty()) {
		wxMessageDialog link(this, wxString::Format(_("Link '%s' to the selected shapes now?"), wxString::FromUTF8(xmlPath)), _("New physics XML"), wxYES_NO | wxICON_QUESTION);
		if (link.ShowModal() == wxID_YES)
			owner->LinkToSelectedShapes(*doc, this);
	}

	RefreshDocuments();
	SelectDocument(xmlPath);
	UpdateEditControls();
}

void PhysicsEditorDialog::OnImportXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (!doc || !owner)
		return;

	wxFileDialog dialog(this,
						wxString::Format(_("Import over %s"), FileNameOf(doc->XmlPath())),
						owner->XmlDirectory(),
						wxString(),
						"HDT-SMP physics files (*.xml)|*.xml",
						wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialog.ShowModal() == wxID_CANCEL)
		return;

	if (owner->Import(*doc, dialog.GetPath()))
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::OnExportXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && owner && owner->Export(*doc))
		UpdateEditControls();
}

void PhysicsEditorDialog::OnSaveXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && owner && owner->Save(*doc))
		UpdateEditControls();
}

void PhysicsEditorDialog::OnSaveAllXml(wxCommandEvent& WXUNUSED(event)) {
	XmlEditSession* session = Session();
	if (!session)
		return;

	for (XmlDocument* doc : session->Documents())
		if (doc->IsDirty() && !owner->Save(*doc))
			break;

	RefreshDocuments();
	UpdateEditControls();
}

void PhysicsEditorDialog::OnLinkXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && owner)
		owner->LinkToSelectedShapes(*doc, this);
}

void PhysicsEditorDialog::OnTreeSelectionChanged(wxTreeEvent& event) {
	event.Skip();

	XmlEditSession* session = Session();
	const wxTreeItemId item = event.GetItem();
	auto* data = item.IsOk() ? static_cast<ItemData*>(tree->GetItemData(item)) : nullptr;
	XmlDocument* doc = session && data ? session->Find(data->xmlPath) : nullptr;
	if (!doc) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	// Undo and redo act on the selected document, also from the rows that stand for the file
	currentXmlPath = data->xmlPath;
	UpdateEditControls();

	if (!data->node.Empty()) {
		ShowElement(doc, doc->Resolve(data->node));
		return;
	}

	// A document or heading row says what the file is. The diagnostics are about the whole file, so this is the
	// natural place to read them.
	grid->Clear();
	currentNode = NodePath();
	ShowCollisions(nullptr, nullptr);
	ShowDiagnostics(doc);
	elementLabel->SetLabel(wxString::FromUTF8(doc->XmlPath()));

	switch (doc->Origin()) {
		case XmlOrigin::Archive: status->SetLabel(_("From an archive; it can be exported, but not saved over.")); break;
		case XmlOrigin::New: status->SetLabel(_("New; it exists only in this session until it is exported.")); break;
		case XmlOrigin::Loose: status->SetLabel(wxString::FromUTF8(doc->SourcePath())); break;
	}
}

void PhysicsEditorDialog::OnPropertyChanged(wxPropertyGridEvent& event) {
	event.Skip();

	const wxPGProperty* property = RootProperty(event.GetProperty());
	tinyxml2::XMLElement* element = CurrentElement();
	const ElementDesc* desc = element ? FindElement(element->Name()) : nullptr;
	if (!property || !desc)
		return;

	const std::string name = std::string(property->GetName().ToUTF8());
	if (name.empty())
		return;

	if (name[0] == '@')
		ApplyChange(property, nullptr, desc->Attr(name.c_str() + 1));
	else
		ApplyChange(property, desc->Child(name.c_str()), nullptr);
}

void PhysicsEditorDialog::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && doc->Undo())
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && doc->Redo())
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::OnFilterChanged(wxCommandEvent& event) {
	event.Skip();

	const std::string typed = ToLower(std::string(filterBox->GetValue().ToUTF8()));
	if (typed == filter)
		return;

	filter = typed;
	RefreshDocuments();
}
