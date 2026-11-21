/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsEditorDialog.h"
#include "outfitstudio/PhysicsXmlEditor.h"
#include "physics/XmlEditSession.h"
#include "physics/XmlProfiles.h"
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

// "3 bones", "1 constraint": what uses a profile of this kind
wxString UserCount(ElementKind profileKind, std::size_t count) {
	if (profileKind == ElementKind::BoneDefault)
		return wxString::Format(wxPLURAL("%zu bone", "%zu bones", count), count);
	return wxString::Format(wxPLURAL("%zu constraint", "%zu constraints", count), count);
}

// What a profile is for, in the tree
wxString ProfileKindName(ElementKind profileKind) {
	switch (profileKind) {
		case ElementKind::BoneDefault: return _("bone");
		case ElementKind::GenericConstraintDefault: return _("generic constraint");
		case ElementKind::StiffSpringConstraintDefault: return _("stiff spring constraint");
		case ElementKind::ConeTwistConstraintDefault: return _("cone twist constraint");
		default: return wxString();
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
	btnTidy = XRCCTRL(*this, "physXmlTidy", wxButton);
	filterBox = XRCCTRL(*this, "physFilter", wxSearchCtrl);
	tree = XRCCTRL(*this, "physTree", wxTreeCtrl);
	elementLabel = XRCCTRL(*this, "physElementLabel", wxStaticText);
	infoBook = XRCCTRL(*this, "physInfoBook", wxNotebook);
	collisionList = XRCCTRL(*this, "physCollisionList", wxListCtrl);
	diagnosticList = XRCCTRL(*this, "physDiagnosticList", wxListCtrl);
	usedByList = XRCCTRL(*this, "physUsedByList", wxListCtrl);
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
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnTidy, this, XRCID("physXmlTidy"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnUndo, this, XRCID("physUndo"));
	Bind(wxEVT_BUTTON, &PhysicsEditorDialog::OnRedo, this, XRCID("physRedo"));
	Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); }, XRCID("physClose"));
	Bind(wxEVT_TREE_SEL_CHANGED, &PhysicsEditorDialog::OnTreeSelectionChanged, this, XRCID("physTree"));
	Bind(wxEVT_TEXT, &PhysicsEditorDialog::OnFilterChanged, this, XRCID("physFilter"));
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &PhysicsEditorDialog::OnCollisionActivated, this, XRCID("physCollisionList"));
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &PhysicsEditorDialog::OnDiagnosticActivated, this, XRCID("physDiagnosticList"));
	Bind(wxEVT_LIST_ITEM_ACTIVATED, &PhysicsEditorDialog::OnUsedByActivated, this, XRCID("physUsedByList"));
	Bind(wxEVT_CLOSE_WINDOW, &PhysicsEditorDialog::OnCloseWindow, this);
	grid->Bind(wxEVT_PG_CHANGED, &PhysicsEditorDialog::OnPropertyChanged, this);
	grid->Bind(wxEVT_PG_RIGHT_CLICK, &PhysicsEditorDialog::OnPropertyRightClick, this);

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

	usedByList->AppendColumn(_("Element"), wxLIST_FORMAT_LEFT, FromDIP(220));
	usedByList->AppendColumn(_("Kind"), wxLIST_FORMAT_LEFT, FromDIP(180));

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

	// Where the user was, by address: the tree is about to be rebuilt. The element shown, whose address an edit
	// may have just moved, else the row selected (a document or a heading).
	std::string selectedPath = currentXmlPath;
	NodePath selectedNode = currentNode;
	if (const wxTreeItemId selected = tree->GetSelection(); selectedNode.Empty() && selected.IsOk()) {
		if (auto* data = static_cast<ItemData*>(tree->GetItemData(selected))) {
			selectedPath = data->xmlPath;
			selectedNode = data->node;
		}
	}

	PopulateTree();

	if (selectedPath.empty()) {
		ShowNothing(_("Nothing selected"));
		HighlightSelection();
		return;
	}

	// Not found means the element is gone, a normal outcome of an edit
	const wxTreeItemId restored = FindTreeItem(selectedPath, selectedNode);
	if (!restored.IsOk()) {
		ShowNothing(_("The selected element is no longer in the document"));
		HighlightSelection();
		return;
	}

	tree->SelectItem(restored);
	tree->EnsureVisible(restored);
	HighlightSelection();
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
		AddProfiles(docItem, *doc);
		AddCategory(docItem, *doc, _("Shape defaults"), {ElementKind::PerVertexShapeDefault, ElementKind::PerTriangleShapeDefault});

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

	if (IsProfileUserKind(kind)) {
		const tinyxml2::XMLElement* profile = ProfileOf(doc, element);
		label += wxString::Format("  (%s)", profile ? wxString::FromUTF8(doc.ProfileLabel(profile)) : _("schema defaults"));
	}

	tree->AppendItem(parent, label, -1, -1, new ItemData(doc.XmlPath(), doc.PathOf(element)));
}

void PhysicsEditorDialog::AddProfiles(const wxTreeItemId& parent, XmlDocument& doc) {
	std::vector<tinyxml2::XMLElement*> shown;
	for (tinyxml2::XMLElement* profile : Profiles(doc))
		if (MatchesFilter(profile))
			shown.push_back(profile);

	if (shown.empty())
		return;

	const wxTreeItemId heading
		= tree->AppendItem(parent, wxString::Format("%s (%zu)", _("Profiles"), shown.size()), -1, -1, new ItemData(doc.XmlPath(), NodePath()));

	for (tinyxml2::XMLElement* profile : shown) {
		const ElementKind kind = KindFromName(profile->Name());
		wxString label = wxString::Format("%s (%s) - %s",
										  wxString::FromUTF8(doc.ProfileLabel(profile)),
										  ProfileKindName(kind),
										  UserCount(kind, ProfileUsers(doc, profile).size()));
		if (IsPositional(doc, profile))
			label += _(", depends on position");

		tree->AppendItem(heading, label, -1, -1, new ItemData(doc.XmlPath(), doc.PathOf(profile)));
	}
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
	ShowUsers(nullptr, nullptr);
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

	btnTidy->Enable(NeedsTidy(*doc));
	btnTidy->SetToolTip(btnTidy->IsEnabled() ? _("Rewrite the defaults into profiles that don't depend on where they are in the file")
											 : _("The defaults of this file don't depend on where they are"));

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
	ValueVariant value = ReadProperty(property, kind, mult);
	const std::string before = doc->Serialize();

	// The profile picker: its first entry is the profile the element has by position, which takes no @template
	if (attr && std::strcmp(attr->name, "template") == 0 && IsProfileUserKind(KindFromName(element->Name()))) {
		const auto* picked = std::get_if<std::string>(&value);
		if (!picked || wxString::FromUTF8(*picked) == AmbientProfileLabel(*doc, element))
			value = std::monostate();
	}

	// The descriptor is a static table entry, so its address identifies the property being dragged; changes to it
	// within half a second fold into one undo step
	const bool added
		= doc->BeginEdit(child ? child->name : attr->name, currentNode, child ? static_cast<const void*>(child) : static_cast<const void*>(attr));

	const bool written = child ? doc->SetValue(element, *child, value) : doc->SetAttr(element, *attr, value);
	if (!written || doc->Serialize() == before) {
		// Nothing to keep: the document didn't take it, or it already said this
		doc->AbandonEdit(before, added);
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

	// A renamed bone or a constraint joining other bones
	HighlightSelection();
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

			if (std::strcmp(attr.name, "template") == 0 && IsProfileUserKind(KindFromName(element->Name()))) {
				AppendProperty(grid, MakeProfilePicker(*doc, element), true);
				continue;
			}

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
				help += wxString::Format(_("\nNot set here; the value shown comes from profile %s. Right click to set it here or there."),
										 wxString::FromUTF8(effective.sourceLabel));
			else if (absent && CascadeApplies(KindFromName(element->Name())))
				help += _("\nNot set here, and no profile supplies it, so the value shown is the schema default.");
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

	const ElementKind kind = KindFromName(element->Name());
	if (owner && kind == ElementKind::Bone && owner->IsIgnoredBoneElement(*doc, element))
		status->SetLabel(_("SMP skips this <bone>: the bone exists by then, made by a skinned shape or a constraint above it, or "
						   "declared before. Move it up to have it count."));

	if (IsProfileKind(kind)) {
		wxString what = wxString::Format(_("Profile %s: %s start from it, and their own values override it."),
										 wxString::FromUTF8(doc->ProfileLabel(element)),
										 UserCount(kind, ProfileUsers(*doc, element).size()));
		if (IsPositional(*doc, element))
			what += _(" Which ones depends on where it is in the file; Tidy Defaults makes it a profile that doesn't.");
		status->SetLabel(what);
	}

	ShowCollisions(doc, element);
	ShowDiagnostics(doc);
	ShowUsers(doc, element);
}

wxString PhysicsEditorDialog::AmbientProfileLabel(const XmlDocument& doc, const tinyxml2::XMLElement* element) const {
	const std::vector<const tinyxml2::XMLElement*> chain = doc.TemplateChain(DefaultKindOf(KindFromName(element->Name())), element, std::string());
	return chain.empty() ? _("(schema defaults)") : wxString::FromUTF8(doc.ProfileLabel(chain.back()));
}

wxPGProperty* PhysicsEditorDialog::MakeProfilePicker(const XmlDocument& doc, const tinyxml2::XMLElement* element) const {
	const ElementKind profileKind = DefaultKindOf(KindFromName(element->Name()));
	const wxString ambient = AmbientProfileLabel(doc, element);

	// A name declared again later is still one entry; what it means here is what was declared before the element
	wxArrayString labels;
	labels.Add(ambient);
	for (const tinyxml2::XMLElement* profile : Profiles(doc)) {
		const std::string name = profile->Attribute("name") ? profile->Attribute("name") : "";
		if (KindFromName(profile->Name()) != profileKind || name.empty())
			continue;

		const std::vector<const tinyxml2::XMLElement*> chain = doc.TemplateChain(profileKind, element, name);
		if (!chain.empty() && chain.back() == profile && labels.Index(wxString::FromUTF8(name), false) == wxNOT_FOUND)
			labels.Add(wxString::FromUTF8(name));
	}

	const tinyxml2::XMLElement* current = ProfileOf(doc, element);
	const int selected = current && current->Attribute("name") ? labels.Index(wxString::FromUTF8(current->Attribute("name")), false) : 0;

	auto* picker = new wxEnumProperty(_("profile"), "@template", labels, wxArrayInt(), std::max(selected, 0));
	picker->SetHelpString(_("The profile this element starts from; its own values override the profile's. Unnamed defaults "
							"that depend on their position can't be picked here; Tidy Defaults makes them profiles.\n"
							"Changing this rebuilds the simulation."));
	return picker;
}

void PhysicsEditorDialog::ShowUsers(XmlDocument* doc, tinyxml2::XMLElement* element) {
	usedByList->DeleteAllItems();
	usedByRows.clear();

	if (!doc || !element || !IsProfileKind(KindFromName(element->Name()))) {
		usedByList->InsertItem(0, _("Select a profile to see what starts from it."));
		return;
	}

	long row = 0;
	for (tinyxml2::XMLElement* user : ProfileUsers(*doc, element)) {
		usedByList->InsertItem(row, wxString::FromUTF8(XmlDocument::DisplayName(user)));
		usedByList->SetItem(row, 1, FromSchema(user->Name()));
		usedByRows.emplace_back(doc->XmlPath(), doc->PathOf(user));
		row++;
	}

	if (row == 0)
		usedByList->InsertItem(0, _("Nothing starts from this profile."));
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

std::set<std::string> PhysicsEditorDialog::BonesOf(const XmlDocument& doc, const tinyxml2::XMLElement* element) const {
	std::set<std::string> bones;
	auto addAttribute = [&bones, element](const char* name) {
		const char* value = element->Attribute(name);
		if (value && *value)
			bones.insert(value);
	};

	switch (KindFromName(element->Name())) {
		case ElementKind::Bone: addAttribute("name"); break;

		case ElementKind::GenericConstraint:
		case ElementKind::StiffSpringConstraint:
		case ElementKind::ConeTwistConstraint:
			addAttribute("bodyA");
			addAttribute("bodyB");
			break;

		case ElementKind::ConstraintGroup:
			for (const tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
				bones.merge(BonesOf(doc, child));
			break;

		case ElementKind::BoneDefault:
		case ElementKind::GenericConstraintDefault:
		case ElementKind::StiffSpringConstraintDefault:
		case ElementKind::ConeTwistConstraintDefault:
			for (const tinyxml2::XMLElement* user : ProfileUsers(doc, element))
				bones.merge(BonesOf(doc, user));
			break;

		case ElementKind::PerVertexShape:
		case ElementKind::PerTriangleShape:
			if (const char* name = element->Attribute("name"); name && owner)
				for (auto& bone : owner->ShapeBoneNames(name))
					bones.insert(bone);
			break;

		default: break;
	}

	return bones;
}

std::set<std::string> PhysicsEditorDialog::BonesOfItem(const wxTreeItemId& item) const {
	auto* data = item.IsOk() ? static_cast<ItemData*>(tree->GetItemData(item)) : nullptr;
	XmlEditSession* session = Session();
	XmlDocument* doc = data && session ? session->Find(data->xmlPath) : nullptr;
	if (!doc)
		return {};

	if (!data->node.Empty()) {
		const tinyxml2::XMLElement* element = doc->Resolve(data->node);
		return element ? BonesOf(*doc, element) : std::set<std::string>();
	}

	std::set<std::string> bones;
	wxTreeItemIdValue cookie;
	for (wxTreeItemId child = tree->GetFirstChild(item, cookie); child.IsOk(); child = tree->GetNextChild(item, cookie))
		bones.merge(BonesOfItem(child));

	return bones;
}

void PhysicsEditorDialog::HighlightSelection() {
	if (owner)
		owner->HighlightBones(BonesOfItem(tree->GetSelection()));
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

void PhysicsEditorDialog::OnUsedByActivated(wxListEvent& event) {
	event.Skip();

	const long row = event.GetIndex();
	if (row >= 0 && static_cast<std::size_t>(row) < usedByRows.size())
		SelectNode(usedByRows[row].first, usedByRows[row].second);
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

	// The viewport shows the bones of what is selected here
	HighlightSelection();

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
	ShowUsers(nullptr, nullptr);
	elementLabel->SetLabel(wxString::FromUTF8(doc->XmlPath()));

	switch (doc->Origin()) {
		case XmlOrigin::Archive: status->SetLabel(_("From an archive; it can be exported, but not saved over.")); break;
		case XmlOrigin::New: status->SetLabel(_("New; it exists only in this session until it is exported.")); break;
		case XmlOrigin::Loose: status->SetLabel(wxString::FromUTF8(doc->SourcePath())); break;
	}
}

void PhysicsEditorDialog::OnPropertyRightClick(wxPropertyGridEvent& event) {
	XmlDocument* doc = CurrentDocument();
	tinyxml2::XMLElement* element = CurrentElement();
	const wxPGProperty* property = RootProperty(event.GetProperty());
	if (!doc || !element || !property || !IsProfileUserKind(KindFromName(element->Name())))
		return;

	const ElementKind kind = KindFromName(element->Name());
	const std::string name = std::string(property->GetName().ToUTF8());
	const ChildDesc* child = name.empty() || name[0] == '@' ? nullptr : FindElement(kind)->Child(name.c_str());
	tinyxml2::XMLElement* profile = ProfileOf(*doc, element);
	const bool own = child && HasOwnValue(*doc, element, *child);

	enum { idReset = wxID_HIGHEST + 1, idApply, idNew };
	wxMenu menu;
	menu.Append(idReset, profile ? _("Reset to Profile Value") : _("Reset to Schema Default"))->Enable(own);
	menu.Append(idApply,
				profile ? wxString::Format(_("Apply to Profile %s (%s)"),
										   wxString::FromUTF8(doc->ProfileLabel(profile)),
										   UserCount(DefaultKindOf(kind), ProfileUsers(*doc, profile).size()))
						: _("Apply to Profile"))
		->Enable(own && profile);
	menu.AppendSeparator();
	menu.Append(idNew, _("New Profile from This Element..."));

	switch (GetPopupMenuSelectionFromUser(menu)) {
		case idReset: ResetToProfile(*doc, element, *child); break;

		case idApply: ApplyToProfile(*doc, element, *child); break;

		case idNew: {
			const wxString profileName = wxGetTextFromUser(_("Name of the new profile. It takes this element's own values, and the element starts from it."),
														   _("New Profile"),
														   wxString(),
														   this);
			if (profileName.empty())
				return;

			if (!CreateProfileFrom(*doc, element, std::string(profileName.ToUTF8()))) {
				wxMessageBox(wxString::Format(_("There already is a profile named '%s'."), profileName), _("New Profile"), wxICON_INFORMATION, this);
				return;
			}
			break;
		}

		default: return;
	}

	// A new profile goes in before the element, which moves its address
	currentNode = doc->PathOf(element);
	ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::OnTidy(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (!doc)
		return;

	// Tried on a copy first, to say what it would do, or why it can't
	XmlDocument trial;
	std::string error;
	if (!trial.LoadFromString(doc->Serialize(), error))
		return;

	const TidyResult preview = TidyDefaults(trial);
	if (!preview.ok) {
		wxMessageBox(wxString::FromUTF8(preview.report), _("Tidy Defaults"), wxICON_INFORMATION, this);
		return;
	}

	const wxString question = wxString::Format(
		_("This rewrites the %d defaults of '%s' into %d profiles at the top of the file: a Default for each kind, and named "
		  "profiles that say what they change about it. Every bone and constraint keeps its values, and it can be undone.\n\n"
		  "Comments inside the defaults are lost. Continue?"),
		preview.defaultsBefore,
		FileNameOf(doc->XmlPath()),
		preview.profilesAfter);
	if (wxMessageBox(question, _("Tidy Defaults"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
		return;

	const std::string xmlPath = doc->XmlPath();
	if (!TidyDefaults(*doc).ok)
		return;

	// Elements moved, so addresses into the document don't mean what they did
	currentNode = NodePath();
	RefreshDocuments();
	SelectDocument(xmlPath);
	UpdateEditControls();
	if (owner)
		owner->DocumentChanged(xmlPath);
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
