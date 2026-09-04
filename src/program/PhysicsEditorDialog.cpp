/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsEditorDialog.h"
#include "OutfitStudio.h"

#include <wx/filedlg.h>

#include <tinyxml2.h>

#include <algorithm>
#include <deque>
#include <cctype>
#include <unordered_set>

using namespace Physics;

namespace {
wxString FromSchema(const char* text) {
	return text ? wxString::FromUTF8(text) : wxString();
}

std::string ToLowerAscii(const std::string& text) {
	std::string result = text;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return result;
}

wxString FileNameOf(const std::string& xmlPath) {
	return wxString::FromUTF8(XmlFileName(xmlPath));
}

// Grey, for a value that is not in the file and is only being shown so the
// grid is not full of blanks.
wxColour InheritedColour(const wxPropertyGrid* grid) {
	const wxColour fg = grid->GetCellTextColour();
	const wxColour bg = grid->GetCellBackgroundColour();
	return wxColour((fg.Red() + bg.Red()) / 2, (fg.Green() + bg.Green()) / 2, (fg.Blue() + bg.Blue()) / 2);
}

// A transform is a frame of its own with three spellings, and a "None" child
// carries structure rather than a value; both are edited through their own
// nodes rather than in a cell.
// A value that names something else: it gets a picker filled from whatever
// declares those names, rather than a bare text field.
bool IsNameKind(ValueKind kind) {
	switch (kind) {
		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: return true;
		default: return false;
	}
}

bool IsEditableKind(ValueKind kind) {
	return kind != ValueKind::Transform && kind != ValueKind::None;
}

void AppendProperty(wxPropertyGrid* grid, wxPGProperty* property, bool editable) {
	grid->Append(property);
	if (!editable)
		grid->SetPropertyReadOnly(property);
}

// The row a change belongs to: the topmost property under its category, so a
// component of a vector reports the vector.
const wxPGProperty* RootProperty(const wxPGProperty* property) {
	while (property) {
		const wxPGProperty* parent = property->GetParent();
		if (!parent || parent->IsCategory() || parent->IsRoot())
			return property;

		property = parent;
	}

	return nullptr;
}

// Reads a row back out of the grid as the kind the schema says it is.
ValueVariant ReadProperty(const wxPGProperty* property, ValueKind kind, Multiplicity mult) {
	auto component = [property](unsigned index) -> float {
		return index < property->GetChildCount() ? static_cast<float>(property->Item(index)->GetValue().GetDouble()) : 0.0f;
	};

	if (mult == Multiplicity::List) {
		std::vector<std::string> items;
		const wxArrayString strings = property->GetValue().GetArrayString();
		for (const wxString& item : strings)
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

		// The children go in x, y, z, w order; nifly::Quaternion takes w first.
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

// One property for a value of "kind". Composite values become a parent row
// with a child per component, so a Vec3 reads as "1; 1; 1" collapsed and as
// three editable numbers expanded.
wxPGProperty* MakeProperty(const wxString& label,
						   const wxString& name,
						   ValueKind kind,
						   Multiplicity mult,
						   const EnumValues* enums,
						   const wxArrayString& choices,
						   const ValueVariant& value) {
	auto floatOr = [](const ValueVariant& v, float fallback) {
		const float* f = std::get_if<float>(&v);
		return f ? *f : fallback;
	};

	if (mult == Multiplicity::List) {
		wxArrayString items;
		if (const auto* list = std::get_if<std::vector<std::string>>(&value)) {
			for (const std::string& item : *list)
				items.Add(wxString::FromUTF8(item));
		}

		// A list of names is picked from what exists rather than typed. Anything
		// already in the file joins the offered names, so an edit cannot quietly
		// drop a name this machine happens not to know.
		if (IsNameKind(kind)) {
			wxArrayString offered = choices;
			for (const wxString& item : items) {
				if (offered.Index(item, false) == wxNOT_FOUND)
					offered.Add(item);
			}

			return new wxMultiChoiceProperty(label, name, offered, items);
		}

		return new wxArrayStringProperty(label, name, items);
	}

	switch (kind) {
		case ValueKind::Float:
		case ValueKind::Factor:
		case ValueKind::PosFloat: return new wxFloatProperty(label, name, floatOr(value, 0.0f));

		case ValueKind::Int: {
			const int* number = std::get_if<int>(&value);
			return new wxIntProperty(label, name, number ? *number : 0);
		}

		case ValueKind::Bool: {
			const bool* flag = std::get_if<bool>(&value);
			return new wxBoolProperty(label, name, flag && *flag);
		}

		case ValueKind::Enum: {
			wxArrayString listed;
			if (enums) {
				for (std::size_t i = 0; i < enums->count; ++i)
					listed.Add(FromSchema(enums->values[i]));
			}

			const std::string* text = std::get_if<std::string>(&value);
			// Editable, so a value the schema does not list still shows what
			// the file says instead of silently snapping to the first choice.
			return new wxEditEnumProperty(label, name, listed, wxArrayInt(), text ? wxString::FromUTF8(*text) : wxString());
		}

		case ValueKind::Vec3: {
			const nifly::Vector3 vector = std::holds_alternative<nifly::Vector3>(value) ? std::get<nifly::Vector3>(value) : nifly::Vector3();
			auto* parent = new wxStringProperty(label, name);
			parent->ChangeFlag(wxPGFlags::ComposedValue, true);
			parent->AppendChild(new wxFloatProperty("x", name + ".x", vector.x));
			parent->AppendChild(new wxFloatProperty("y", name + ".y", vector.y));
			parent->AppendChild(new wxFloatProperty("z", name + ".z", vector.z));
			return parent;
		}

		case ValueKind::Quaternion: {
			const nifly::Quaternion quaternion
				= std::holds_alternative<nifly::Quaternion>(value) ? std::get<nifly::Quaternion>(value) : nifly::Quaternion();
			auto* parent = new wxStringProperty(label, name);
			parent->ChangeFlag(wxPGFlags::ComposedValue, true);
			parent->AppendChild(new wxFloatProperty("x", name + ".x", quaternion.x));
			parent->AppendChild(new wxFloatProperty("y", name + ".y", quaternion.y));
			parent->AppendChild(new wxFloatProperty("z", name + ".z", quaternion.z));
			parent->AppendChild(new wxFloatProperty("w", name + ".w", quaternion.w));
			return parent;
		}

		case ValueKind::AxisAngle: {
			const AxisAngleValue axisAngle = std::holds_alternative<AxisAngleValue>(value) ? std::get<AxisAngleValue>(value) : AxisAngleValue();
			auto* parent = new wxStringProperty(label, name);
			parent->ChangeFlag(wxPGFlags::ComposedValue, true);
			parent->AppendChild(new wxFloatProperty("x", name + ".x", axisAngle.axis.x));
			parent->AppendChild(new wxFloatProperty("y", name + ".y", axisAngle.axis.y));
			parent->AppendChild(new wxFloatProperty("z", name + ".z", axisAngle.axis.z));
			parent->AppendChild(new wxFloatProperty(_("angle"), name + ".angle", axisAngle.angle));
			return parent;
		}

		case ValueKind::Lerp: {
			const LerpValue lerp = std::holds_alternative<LerpValue>(value) ? std::get<LerpValue>(value) : LerpValue();
			auto* parent = new wxStringProperty(label, name);
			parent->ChangeFlag(wxPGFlags::ComposedValue, true);
			parent->AppendChild(new wxFloatProperty(_("translation"), name + ".translation", lerp.translation));
			parent->AppendChild(new wxFloatProperty(_("rotation"), name + ".rotation", lerp.rotation));
			return parent;
		}

		case ValueKind::Transform:
			// A frame of its own, with a choice of three spellings; it gets a
			// page rather than a row.
			return new wxStringProperty(label, name, _("(edited on its own row)"));

		case ValueKind::None: return new wxStringProperty(label, name, wxString());

		case ValueKind::BoneRef:
		case ValueKind::NiShapeRef:
		case ValueKind::ShapeRef:
		case ValueKind::TagRef: {
			const std::string* named = std::get_if<std::string>(&value);
			const wxString current = named ? wxString::FromUTF8(*named) : wxString();

			wxArrayString offered = choices;
			if (!current.empty() && offered.Index(current, false) == wxNOT_FOUND)
				offered.Insert(current, 0);

			// Editable, so a name nothing loaded here knows about is still a
			// name the file keeps.
			return new wxEditEnumProperty(label, name, offered, wxArrayInt(), current);
		}

		case ValueKind::String: break;
	}

	const std::string* text = std::get_if<std::string>(&value);
	return new wxStringProperty(label, name, text ? wxString::FromUTF8(*text) : wxString());
}
}

PhysicsEditorDialog::PhysicsEditorDialog(OutfitStudioFrame* parent)
	: wxPhysicsEditorDlg(parent)
	, os(parent) {
	if (!IsLoaded())
		return;

	// The two report lists are described here rather than in the XRC, because
	// their columns are decided by what the editor puts in them.
	physCollisionList->AppendColumn(_("Skinned shape"), wxLIST_FORMAT_LEFT, FromDIP(160));
	physCollisionList->AppendColumn(_("File"), wxLIST_FORMAT_LEFT, FromDIP(130));
	physCollisionList->AppendColumn(_("Collides"), wxLIST_FORMAT_LEFT, FromDIP(70));
	physCollisionList->AppendColumn(_("Why"), wxLIST_FORMAT_LEFT, FromDIP(460));

	physDiagnosticList->AppendColumn(_("Severity"), wxLIST_FORMAT_LEFT, FromDIP(70));
	physDiagnosticList->AppendColumn(_("Element"), wxLIST_FORMAT_LEFT, FromDIP(190));
	physDiagnosticList->AppendColumn(_("Detail"), wxLIST_FORMAT_LEFT, FromDIP(640));

	PopulateTree();
	ShowNothing(_("Nothing selected"));
}

XmlEditSession* PhysicsEditorDialog::Session() const {
	return os ? &os->GetPhysicsXmlSession() : nullptr;
}

void PhysicsEditorDialog::RefreshDocuments() {
	if (!IsLoaded())
		return;

	// Remember where the user was, by address rather than by tree item: the
	// tree is about to be thrown away and rebuilt.
	std::string selectedPath;
	NodePath selectedNode;
	if (const wxTreeItemId selected = physTree->GetSelection(); selected.IsOk()) {
		if (auto* data = static_cast<ItemData*>(physTree->GetItemData(selected))) {
			selectedPath = data->xmlPath;
			selectedNode = data->node;
		}
	}

	PopulateTree();

	if (selectedPath.empty()) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	// Nothing found means the element is gone, which is a normal outcome of
	// an edit.
	const wxTreeItemId restored = FindTreeItem(selectedPath, selectedNode);
	if (!restored.IsOk()) {
		ShowNothing(_("The selected element is no longer in the document"));
		return;
	}

	physTree->SelectItem(restored);
	physTree->EnsureVisible(restored);
}

void PhysicsEditorDialog::SelectDocument(const std::string& xmlPath) {
	if (!IsLoaded())
		return;

	const std::string wanted = XmlEditSession::NormalizeXmlPath(xmlPath);

	wxTreeItemIdValue cookie;
	const wxTreeItemId root = physTree->GetRootItem();
	for (wxTreeItemId item = physTree->GetFirstChild(root, cookie); item.IsOk(); item = physTree->GetNextChild(root, cookie)) {
		auto* data = static_cast<ItemData*>(physTree->GetItemData(item));
		if (data && XmlEditSession::NormalizeXmlPath(data->xmlPath) == wanted) {
			physTree->SelectItem(item);
			physTree->EnsureVisible(item);
			return;
		}
	}
}

void PhysicsEditorDialog::PopulateTree() {
	physTree->DeleteAllItems();

	XmlEditSession* session = Session();
	if (!session)
		return;

	const wxTreeItemId root = physTree->AddRoot("physics");

	const std::vector<XmlDocument*> documents = session->Documents();
	if (documents.empty()) {
		physTree->AppendItem(root, _("No physics XMLs are loaded"));
		return;
	}

	for (XmlDocument* doc : documents) {
		wxString label = FileNameOf(doc->XmlPath());
		if (doc->IsDirty())
			label += " *";

		const wxTreeItemId docItem = physTree->AppendItem(root, label, -1, -1, new ItemData(doc->XmlPath(), NodePath()));

		AddCategory(docItem, *doc, _("Bones"), {ElementKind::Bone});

		// Not "Shapes": that string already means the meshes of the outfit
		// everywhere else in this application, and shares its translation.
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

		// Elements the schema table does not describe are kept and written back
		// out untouched, so they have to be visible: a file with something in it
		// this editor cannot explain is exactly what an author wants to know.
		AddCategory(docItem, *doc, _("Not in the schema"), {ElementKind::Unknown});

		physTree->Expand(docItem);
	}
}

void PhysicsEditorDialog::AddCategory(const wxTreeItemId& parent,
									  XmlDocument& doc,
									  const wxString& label,
									  std::initializer_list<ElementKind> kinds,
									  bool nestConstraintGroups) {
	std::vector<tinyxml2::XMLElement*> elements = doc.ChildrenOfKinds(kinds);

	std::vector<tinyxml2::XMLElement*> shown;
	for (tinyxml2::XMLElement* element : elements) {
		if (MatchesFilter(element))
			shown.push_back(element);
	}

	if (shown.empty())
		return;

	// The count is of what is being shown, so a filtered tree does not claim
	// there are more of something than it lists.
	const wxTreeItemId categoryItem
		= physTree->AppendItem(parent, wxString::Format("%s (%d)", label, static_cast<int>(shown.size())), -1, -1, new ItemData(doc.XmlPath(), NodePath()));

	// Under a heading that holds one kind, saying which kind on every row is
	// noise; under one that merges several, it is the only way to tell them
	// apart, and it decides what the element can do.
	const bool showKind = kinds.size() > 1;

	for (tinyxml2::XMLElement* element : shown) {
		AddElementItem(categoryItem, doc, element, showKind);

		if (!nestConstraintGroups || KindFromName(element->Name()) != ElementKind::ConstraintGroup)
			continue;

		// A constraint-group holds its constraints, so they belong under it -
		// which is also where the builder finds them.
		const wxTreeItemId groupItem = physTree->GetLastChild(categoryItem);
		for (tinyxml2::XMLElement* nested :
			 doc.ChildrenOfKinds({ElementKind::GenericConstraint, ElementKind::StiffSpringConstraint, ElementKind::ConeTwistConstraint}, element))
			AddElementItem(groupItem, doc, nested, true);
	}
}

void PhysicsEditorDialog::AddElementItem(const wxTreeItemId& parent, XmlDocument& doc, tinyxml2::XMLElement* element, bool showKind) {
	wxString label = wxString::FromUTF8(XmlDocument::DisplayName(element));

	const ElementKind kind = KindFromName(element->Name());

	// DisplayName falls back to the element's own tag, which would then be
	// repeated: "bone-default [bone-default]".
	if (showKind && kind != ElementKind::Unknown && label != FromSchema(NameFromKind(kind)))
		label += wxString::Format("  [%s]", FromSchema(NameFromKind(kind)));

	physTree->AppendItem(parent, label, -1, -1, new ItemData(doc.XmlPath(), doc.PathOf(element)));
}

bool PhysicsEditorDialog::MatchesFilter(tinyxml2::XMLElement* element) const {
	if (filter.empty())
		return true;

	if (ToLowerAscii(XmlDocument::DisplayName(element)).find(filter) != std::string::npos)
		return true;

	if (element->Name() && ToLowerAscii(element->Name()).find(filter) != std::string::npos)
		return true;

	// A group whose own name says nothing still has to be reachable when what
	// the user typed is inside it.
	for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (MatchesFilter(child))
			return true;
	}

	return false;
}

void PhysicsEditorDialog::ShowNothing(const wxString& reason) {
	pgProperties->Clear();
	physElementLabel->SetLabel(reason);
	physStatus->SetLabel(wxString());
	currentNode = NodePath();

	ShowCollisions(nullptr, nullptr);
	ShowDiagnostics(nullptr);
}

void PhysicsEditorDialog::UpdateEditControls() {
	const XmlDocument* doc = CurrentDocument();

	physUndo->Enable(doc && doc->CanUndo());
	physRedo->Enable(doc && doc->CanRedo());

	// Naming the step makes it obvious what Ctrl+Z is about to take back
	physUndo->SetToolTip(doc && doc->CanUndo() ? wxString::Format(_("Undo %s (Ctrl+Z)"), wxString::FromUTF8(doc->UndoLabel()))
											  : _("Nothing to undo"));
	physRedo->SetToolTip(doc && doc->CanRedo() ? wxString::Format(_("Redo %s (Ctrl+Y)"), wxString::FromUTF8(doc->RedoLabel()))
												  : _("Nothing to redo"));

	UpdateFileControls();

	if (!doc)
		return;

	// The document rows are the tree's top level; only the one that changed
	// needs its unsaved marker brought up to date.
	const wxTreeItemId root = physTree->GetRootItem();
	if (!root.IsOk())
		return;

	wxTreeItemIdValue cookie;
	for (wxTreeItemId item = physTree->GetFirstChild(root, cookie); item.IsOk(); item = physTree->GetNextChild(root, cookie)) {
		auto* data = static_cast<ItemData*>(physTree->GetItemData(item));
		if (!data || data->xmlPath != doc->XmlPath())
			continue;

		wxString label = FileNameOf(doc->XmlPath());
		if (doc->IsDirty())
			label += " *";

		if (physTree->GetItemText(item) != label)
			physTree->SetItemText(item, label);

		break;
	}
}

void PhysicsEditorDialog::ApplyChange(const wxPGProperty* property, const ChildDesc* child, const AttrDesc* attr) {
	XmlDocument* doc = CurrentDocument();
	tinyxml2::XMLElement* element = CurrentElement();
	if (!doc || !element || (!child && !attr))
		return;

	const ValueKind kind = child ? child->kind : attr->kind;
	const Multiplicity mult = child ? child->mult : Multiplicity::Once;
	const ValueVariant value = ReadProperty(property, kind, mult);

	// The descriptor is a static table entry, so its address identifies the
	// property being dragged; changes to the same one within half a second
	// fold into a single undo step.
	doc->BeginEdit(child ? child->name : attr->name, currentNode, child ? static_cast<const void*>(child) : static_cast<const void*>(attr));

	const bool written = child ? doc->SetValue(element, *child, value) : doc->SetAttr(element, *attr, value);
	if (!written) {
		// The document did not take it, so there is no edit to keep
		doc->Undo();
		UpdateEditControls();
		return;
	}

	if (os) {
		// An attribute names something the whole system was built around, so
		// there is no version of it that can be patched in place.
		if (attr)
			os->NotifyPhysicsXmlEdited(doc->XmlPath());
		else
			os->PatchOrRebuildPhysics(*doc, element, *child);
	}

	UpdateEditControls();

	// A tag or a collide list changes the answers on the collisions page, and
	// almost any edit can raise or clear a diagnostic, so both are re-read.
	ShowCollisions(doc, element);
	ShowDiagnostics(doc);
}

void PhysicsEditorDialog::ReloadAfterDocumentChange() {
	// Undo reparses the document, so every pointer into it is stale; the tree
	// items address elements by path for exactly this reason, which is also
	// what lets the selection survive being rebuilt around it.
	RefreshDocuments();

	XmlDocument* doc = CurrentDocument();
	tinyxml2::XMLElement* element = CurrentElement();
	if (doc && element)
		ShowElement(doc, element);

	UpdateEditControls();

	if (os && doc)
		os->NotifyPhysicsXmlEdited(doc->XmlPath());
}

void PhysicsEditorDialog::AppendUnknownElement(tinyxml2::XMLElement* element) {
	pgProperties->Append(new wxPropertyCategory(_("Attributes")));

	// Read only: the schema says nothing about what these mean, so the editor
	// has no business changing them. They are kept and written back as they are.
	for (const tinyxml2::XMLAttribute* attr = element->FirstAttribute(); attr; attr = attr->Next()) {
		AppendProperty(pgProperties,
					   new wxStringProperty(wxString::FromUTF8(attr->Name()), wxString::FromUTF8(attr->Name()), wxString::FromUTF8(attr->Value())),
					   false);
	}

	const char* text = element->GetText();
	if (text && *text) {
		pgProperties->Append(new wxPropertyCategory(_("Text")));
		AppendProperty(pgProperties, new wxStringProperty(_("value"), "text", wxString::FromUTF8(text)), false);
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
	pgProperties->Clear();
	physStatus->SetLabel(wxString());

	if (!doc || !element) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	currentXmlPath = doc->XmlPath();
	currentNode = doc->PathOf(element);

	const wxString displayName = wxString::FromUTF8(XmlDocument::DisplayName(element));
	physElementLabel->SetLabel(wxString::Format("%s  -  %s", wxString::FromUTF8(element->Name() ? element->Name() : ""), displayName));

	const ElementDesc* desc = FindElement(element->Name());
	if (!desc) {
		// Kept in the document and written back out untouched; there is just
		// nothing the schema table can say about it.
		physStatus->SetLabel(_("Not part of the hdtSMP64 schema. It is preserved, but not understood."));
		AppendUnknownElement(element);
		ShowCollisions(doc, element);
		ShowDiagnostics(doc);
		return;
	}

	if (desc->help)
		physStatus->SetLabel(FromSchema(desc->help));

	const wxColour inherited = InheritedColour(pgProperties);

	if (desc->attrCount > 0) {
		pgProperties->Append(new wxPropertyCategory(_("Attributes")));

		for (std::size_t i = 0; i < desc->attrCount; ++i) {
			const AttrDesc& attr = desc->attrs[i];
			const ValueVariant value = doc->GetAttr(element, attr);

			// Attributes are addressed with a leading @, so an attribute and a
			// child of the same name cannot collide in the grid.
			wxPGProperty* property = MakeProperty(FromSchema(attr.name),
												  "@" + FromSchema(attr.name),
												  attr.kind,
												  Multiplicity::Once,
												  attr.enumValues,
												  ChoicesFor(attr.kind, *doc),
												  value);

			wxString help = FromSchema(attr.help);
			if (attr.required)
				help += help.empty() ? _("Required.") : _("\nRequired.");

			// Every attribute here names something - a bone, a shape, a
			// template - so changing one always rebuilds the system.
			help += _("\nChanging this rebuilds the simulation.");

			property->SetHelpString(help);
			AppendProperty(pgProperties, property, IsEditableKind(attr.kind));

			if (std::holds_alternative<std::monostate>(value))
				pgProperties->SetPropertyTextColour(property, inherited);
		}
	}

	// Split by whether this preview does anything with the value, so a setting
	// that will not move the mesh cannot be mistaken for one that will.
	std::vector<const ChildDesc*> simulated;
	std::vector<const ChildDesc*> notSimulated;
	for (std::size_t i = 0; i < desc->childCount; ++i) {
		const ChildDesc& child = desc->children[i];
		(child.preview == PreviewSupport::Simulated ? simulated : notSimulated).push_back(&child);
	}

	auto appendChildren = [&](const std::vector<const ChildDesc*>& children, const wxString& categoryLabel) {
		if (children.empty())
			return;

		pgProperties->Append(new wxPropertyCategory(categoryLabel));

		for (const ChildDesc* child : children) {
			// What the element is really built with, rather than what it says.
			// A property nobody set is decided by the ambient *-default cascade,
			// and showing the schema default in its place would be a different
			// number from the one the simulation is using.
			const EffectiveValue effective = doc->ResolveEffective(element, *child);
			const bool absent = !effective.explicitHere;

			wxPGProperty* property = MakeProperty(FromSchema(child->name),
												  FromSchema(child->name),
												  child->kind,
												  child->mult,
												  child->enumValues,
												  ChoicesFor(child->kind, *doc),
												  effective.value);

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
			AppendProperty(pgProperties, property, IsEditableKind(child->kind));

			if (absent)
				pgProperties->SetPropertyTextColour(property, inherited);
		}
	};

	appendChildren(simulated, _("Properties"));
	appendChildren(notSimulated, _("Not simulated in this preview"));

	ShowCollisions(doc, element);
	ShowDiagnostics(doc);
}

wxArrayString PhysicsEditorDialog::ChoicesFor(ValueKind kind, const XmlDocument& doc) const {
	wxArrayString choices;

	// A skeleton runs to several hundred bones and this is rebuilt for every
	// bone reference on the selected element, so the duplicate check cannot be
	// a scan of what has been added so far.
	std::unordered_set<std::string> seen;
	auto add = [&choices, &seen](const std::string& name) {
		if (name.empty())
			return;

		// Case insensitively, the way the names themselves are matched.
		if (!seen.insert(ToLowerAscii(name)).second)
			return;

		choices.Add(wxString::FromUTF8(name));
	};

	auto addNamed = [&add](const std::vector<tinyxml2::XMLElement*>& elements) {
		for (const tinyxml2::XMLElement* element : elements) {
			if (const char* name = element->Attribute("name"))
				add(name);
		}
	};

	switch (kind) {
		case ValueKind::BoneRef:
			// The bones the file already declares come first: those are the
			// ones the author has decided to simulate, and they are a handful
			// out of the several hundred a skeleton has.
			addNamed(doc.ChildrenOfKind(ElementKind::Bone));
			{
				std::vector<std::string> skeleton;
				AnimSkeleton::getInstance().GetBoneNames(skeleton);
				for (const std::string& name : skeleton)
					add(name);
			}
			break;

		case ValueKind::NiShapeRef:
			if (os && os->project) {
				for (const std::string& name : os->project->GetWorkNif()->GetShapeNames())
					add(name);
			}
			break;

		case ValueKind::ShapeRef:
			// Only a named top level <shape> can be pointed at; an inline one
			// inside a bone has nothing to name it by.
			addNamed(doc.ChildrenOfKind(ElementKind::Shape));
			break;

		case ValueKind::TagRef:
			// Tags cross file boundaries: a shape in one XML routinely names a
			// tag declared in another, which is how body and outfit meet.
			if (XmlEditSession* session = Session()) {
				for (XmlDocument* other : session->Documents()) {
					for (const std::string& tag : other->CollectTags())
						add(tag);
				}
			}
			break;

		default: break;
	}

	return choices;
}

ValidationContext PhysicsEditorDialog::ValidationInputs() const {
	ValidationContext context;

	if (os && os->project && os->project->GetWorkNif()->IsValid())
		context.nifShapeNames = os->project->GetWorkNif()->GetShapeNames();

	// A bone the reference skeleton does not have is still resolved when the
	// NIF carries a node of that name: findAnimBone falls back to
	// LoadCustomBoneFromNif. Reporting those as missing would be wrong.
	AnimSkeleton::getInstance().GetBoneNames(context.skeletonBoneNames);
	if (os && os->project) {
		for (const nifly::NiNode* node : os->project->GetWorkNif()->GetNodes()) {
			if (!node->name.get().empty())
				context.skeletonBoneNames.push_back(node->name.get());
		}
	}

	return context;
}

void PhysicsEditorDialog::ShowCollisions(XmlDocument* doc, tinyxml2::XMLElement* element) {
	physCollisionList->DeleteAllItems();
	collisionRows.clear();

	XmlEditSession* session = Session();
	if (!doc || !element || !session)
		return;

	const ElementKind kind = KindFromName(element->Name());
	if (kind != ElementKind::PerVertexShape && kind != ElementKind::PerTriangleShape) {
		physCollisionList->InsertItem(0, _("Only skinned shapes carry collision filtering."));
		return;
	}

	// Every skinned shape of every open document, because the whole point of
	// the tag lists is to arrange collisions between separate files.
	std::vector<ShapeFacts> everything;
	for (XmlDocument* other : session->Documents()) {
		std::vector<ShapeFacts> facts = CollectShapeFacts(*other);
		everything.insert(everything.end(), facts.begin(), facts.end());
	}

	const NodePath node = doc->PathOf(element);
	const auto self = std::find_if(everything.begin(), everything.end(), [&](const ShapeFacts& facts) {
		return facts.xmlPath == doc->XmlPath() && facts.node == node;
	});

	if (self == everything.end())
		return;

	long row = 0;
	for (const ShapeFacts& other : everything) {
		if (&other == &*self)
			continue;

		const CollisionResult result = EvaluateCollision(*self, other);

		physCollisionList->InsertItem(row, wxString::FromUTF8(other.name));
		physCollisionList->SetItem(row, 1, FileNameOf(other.xmlPath));
		physCollisionList->SetItem(row, 2, result.collides ? _("collides") : _("no"));
		physCollisionList->SetItem(row, 3, wxString::FromUTF8(result.reason));

		// Muted for the pairs that never meet, so the ones that do stand out in
		// a list that is mostly "no".
		if (!result.collides)
			physCollisionList->SetItemTextColour(row, InheritedColour(pgProperties));

		collisionRows.emplace_back(other.xmlPath, other.node);
		row++;
	}

	if (row == 0)
		physCollisionList->InsertItem(0, _("No other skinned shape is loaded."));
}

void PhysicsEditorDialog::ShowDiagnostics(XmlDocument* doc) {
	physDiagnosticList->DeleteAllItems();
	diagnosticRows.clear();

	if (!doc)
		return;

	const std::vector<Diagnostic> diagnostics = doc->Validate(ValidationInputs());

	long row = 0;
	for (const Diagnostic& diagnostic : diagnostics) {
		wxString severity;
		switch (diagnostic.severity) {
			case Diagnostic::Severity::Error: severity = _("Error"); break;
			case Diagnostic::Severity::Warning: severity = _("Warning"); break;
			case Diagnostic::Severity::Info: severity = _("Note"); break;
		}

		physDiagnosticList->InsertItem(row, severity);
		physDiagnosticList->SetItem(row, 1, wxString::FromUTF8(diagnostic.elementLabel));
		physDiagnosticList->SetItem(row, 2, wxString::FromUTF8(diagnostic.message));

		if (diagnostic.severity == Diagnostic::Severity::Info)
			physDiagnosticList->SetItemTextColour(row, InheritedColour(pgProperties));

		diagnosticRows.emplace_back(doc->XmlPath(), diagnostic.node);
		row++;
	}

	// The tab label carries the count, so a problem is visible without opening
	// the page.
	if (physInfoBook) {
		const int page = physInfoBook->FindPage(physDiagnosticList);
		if (page != wxNOT_FOUND)
			physInfoBook->SetPageText(page, row > 0 ? wxString::Format(_("Diagnostics (%ld)"), row) : _("Diagnostics"));
	}

	if (row == 0)
		physDiagnosticList->InsertItem(0, _("Nothing to report."));
}

wxTreeItemId PhysicsEditorDialog::FindTreeItem(const std::string& xmlPath, const NodePath& node) const {
	// Breadth first, in document order, so an address several rows share -
	// a document row and its category headings all carry an empty node path -
	// resolves to the outermost one rather than to whichever the walk
	// happened to reach first.
	std::deque<wxTreeItemId> pending{physTree->GetRootItem()};
	while (!pending.empty()) {
		const wxTreeItemId item = pending.front();
		pending.pop_front();

		if (auto* data = static_cast<ItemData*>(physTree->GetItemData(item))) {
			if (data->xmlPath == xmlPath && data->node == node)
				return item;
		}

		wxTreeItemIdValue cookie;
		for (wxTreeItemId child = physTree->GetFirstChild(item, cookie); child.IsOk(); child = physTree->GetNextChild(item, cookie))
			pending.push_back(child);
	}

	return wxTreeItemId();
}

void PhysicsEditorDialog::SelectNode(const std::string& xmlPath, const NodePath& node) {
	// An empty path stands for a document or a category row, which nothing
	// in the two lists points at.
	if (node.Empty())
		return;

	const wxTreeItemId item = FindTreeItem(xmlPath, node);
	if (!item.IsOk())
		return;

	physTree->SelectItem(item);
	physTree->EnsureVisible(item);
}

void PhysicsEditorDialog::doCollisionActivated(wxListEvent& event) {
	event.Skip();

	const long row = event.GetIndex();
	if (row < 0 || static_cast<std::size_t>(row) >= collisionRows.size())
		return;

	SelectNode(collisionRows[row].first, collisionRows[row].second);
}

void PhysicsEditorDialog::doDiagnosticActivated(wxListEvent& event) {
	event.Skip();

	const long row = event.GetIndex();
	if (row < 0 || static_cast<std::size_t>(row) >= diagnosticRows.size())
		return;

	SelectNode(diagnosticRows[row].first, diagnosticRows[row].second);
}

std::vector<std::string> PhysicsEditorDialog::SelectedShapeNames() const {
	std::vector<std::string> names;
	if (!os)
		return names;

	for (ShapeItemData* item : os->GetSelectedItems()) {
		if (item && item->GetShape())
			names.push_back(item->GetShape()->name.get());
	}

	return names;
}

void PhysicsEditorDialog::LinkToSelectedShapes(XmlDocument& doc) {
	if (!os || !os->project)
		return;

	const std::vector<std::string> shapes = SelectedShapeNames();
	if (shapes.empty()) {
		wxMessageBox(_("Select the shapes to link it to in Outfit Studio first."), _("Link physics XML"), wxICON_INFORMATION);
		return;
	}

	const int linked = os->project->LinkPhysicsFile(doc.XmlPath(), shapes);
	if (linked == 0) {
		wxMessageBox(wxString::Format(_("Every selected shape already links '%s'."), wxString::FromUTF8(doc.XmlPath())),
					 _("Link physics XML"),
					 wxICON_INFORMATION);
		return;
	}

	// The link is stored in the NIF, so what now has unsaved changes is the
	// project, not the XML.
	os->SetPendingChanges();
	os->RefreshPhysicsLinks();
}

void PhysicsEditorDialog::UpdateFileControls() {
	XmlEditSession* session = Session();
	const XmlDocument* doc = CurrentDocument();

	physXmlNew->Enable(session != nullptr && os != nullptr && os->project != nullptr);
	physXmlImport->Enable(doc != nullptr);
	physXmlExport->Enable(doc != nullptr);

	// Enabled whatever is selected in the viewport: what to do about an empty
	// selection is a better answer than a button that is dead for a reason the
	// user cannot see from here.
	physXmlLink->Enable(doc != nullptr);

	// A document read out of an archive, or one that has never been written,
	// has nothing to be saved back over. Say so rather than silently turning
	// Save into Export.
	const bool overwritable = doc && doc->Origin() == XmlOrigin::Loose && !doc->SourcePath().empty();
	physXmlSave->Enable(overwritable && doc->IsDirty());
	physXmlSaveAll->Enable(session && session->AnyDirty());

	if (!doc)
		physXmlSave->SetToolTip(_("Select a physics XML first"));
	else if (!overwritable)
		physXmlSave->SetToolTip(_("This file did not come from a loose file on disk. Use Export instead."));
	else if (!doc->IsDirty())
		physXmlSave->SetToolTip(_("No unsaved changes"));
	else {
		// Worth saying once, where it is about to happen: the values are
		// written back exactly, but the whitespace around them is the
		// printer's rather than the author's.
		physXmlSave->SetToolTip(
			wxString::Format(_("Overwrite %s.\nEverything it says is preserved, but the file is re-indented."), wxString::FromUTF8(doc->SourcePath())));
	}
}

void PhysicsEditorDialog::doNewXml(wxCommandEvent& WXUNUSED(event)) {
	XmlEditSession* session = Session();
	if (!session || !os)
		return;

	wxFileDialog dialog(this,
						_("New physics XML"),
						os->PhysicsXmlDirectory(),
						"physics.xml",
						"HDT-SMP physics files (*.xml)|*.xml",
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_CANCEL)
		return;

	const wxString path = dialog.GetPath();

	// What a NIF carries is a path relative to the game data folder. A file
	// anywhere else can be edited here perfectly well, but nothing in game will
	// ever find it, and that is worth saying at the moment it is decided.
	std::string xmlPath = os->GameRelativeXmlPath(path);
	if (xmlPath.empty()) {
		xmlPath = std::string(wxFileName(path).GetFullName().ToUTF8());
		wxLogWarning(_("'%s' is not inside the game data folder, so a model that links it by name will not find it in game."), path);
	}

	XmlDocument* doc = session->Create(xmlPath);
	if (!doc) {
		wxMessageBox(wxString::Format(_("'%s' is already open in this editor."), wxString::FromUTF8(xmlPath)),
					 _("New physics XML"),
					 wxICON_INFORMATION);
		return;
	}

	doc->SetSource(xmlPath, std::string(path.ToUTF8()), XmlOrigin::Loose);
	if (!os->SavePhysicsXml(*doc, false)) {
		// Nothing on disk and nothing linking it: leaving it in the session
		// would be a document that exists only as a mistake.
		session->Close(xmlPath);
		return;
	}

	// A physics XML nothing links to does nothing at all, so offering the link
	// is part of creating one rather than a separate errand to remember.
	const std::vector<std::string> shapes = SelectedShapeNames();
	if (!shapes.empty()) {
		wxMessageDialog link(this,
							 wxString::Format(_("Link '%s' to the selected shapes now?"), wxString::FromUTF8(xmlPath)),
							 _("New physics XML"),
							 wxYES_NO | wxICON_QUESTION);

		if (link.ShowModal() == wxID_YES)
			LinkToSelectedShapes(*doc);
	}

	RefreshDocuments();
	SelectDocument(xmlPath);
	UpdateEditControls();
}

void PhysicsEditorDialog::doImportXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (!doc || !os)
		return;

	wxFileDialog dialog(this,
						wxString::Format(_("Import over %s"), FileNameOf(doc->XmlPath())),
						os->PhysicsXmlDirectory(),
						wxString(),
						"HDT-SMP physics files (*.xml)|*.xml",
						wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (dialog.ShowModal() == wxID_CANCEL)
		return;

	if (os->ImportPhysicsXml(*doc, dialog.GetPath()))
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::doExportXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && os && os->ExportPhysicsXml(*doc))
		UpdateEditControls();
}

void PhysicsEditorDialog::doSaveXml(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && os && os->SavePhysicsXml(*doc))
		UpdateEditControls();
}

void PhysicsEditorDialog::doSaveAllXml(wxCommandEvent& WXUNUSED(event)) {
	XmlEditSession* session = Session();
	if (!session || !os)
		return;

	for (XmlDocument* doc : session->Documents()) {
		if (doc->IsDirty() && !os->SavePhysicsXml(*doc))
			break;
	}

	RefreshDocuments();
	UpdateEditControls();
}

void PhysicsEditorDialog::doLinkXml(wxCommandEvent& WXUNUSED(event)) {
	if (XmlDocument* doc = CurrentDocument())
		LinkToSelectedShapes(*doc);
}

void PhysicsEditorDialog::doTreeSelectionChanged(wxTreeEvent& event) {
	event.Skip();

	XmlEditSession* session = Session();
	if (!session)
		return;

	const wxTreeItemId item = event.GetItem();
	auto* data = item.IsOk() ? static_cast<ItemData*>(physTree->GetItemData(item)) : nullptr;
	if (!data) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	XmlDocument* doc = session->Find(data->xmlPath);
	if (!doc) {
		ShowNothing(_("Nothing selected"));
		return;
	}

	// Which document undo and redo act on follows the selection, including for
	// the rows that stand for the file rather than for an element in it.
	currentXmlPath = data->xmlPath;
	UpdateEditControls();

	if (data->node.Empty()) {
		// A document or category row: say what the file is rather than
		// clearing the grid to nothing. The diagnostics are about the whole
		// file anyway, so this is the natural place to read them.
		pgProperties->Clear();
		ShowCollisions(nullptr, nullptr);
		ShowDiagnostics(doc);
		physElementLabel->SetLabel(wxString::FromUTF8(doc->XmlPath()));

		switch (doc->Origin()) {
			case XmlOrigin::Archive: physStatus->SetLabel(_("From an archive; it can be exported, but not saved over.")); break;
			case XmlOrigin::New: physStatus->SetLabel(_("New; it exists only in this session until it is exported.")); break;
			case XmlOrigin::Loose: physStatus->SetLabel(wxString::FromUTF8(doc->SourcePath())); break;
		}

		return;
	}

	ShowElement(doc, doc->Resolve(data->node));
}

void PhysicsEditorDialog::doPropertyChanged(wxPropertyGridEvent& event) {
	event.Skip();

	const wxPGProperty* property = RootProperty(event.GetProperty());
	tinyxml2::XMLElement* element = CurrentElement();
	if (!property || !element)
		return;

	const ElementDesc* desc = FindElement(element->Name());
	if (!desc)
		return;

	const std::string name = std::string(property->GetName().ToUTF8());
	if (name.empty())
		return;

	if (name[0] == '@')
		ApplyChange(property, nullptr, desc->Attr(name.c_str() + 1));
	else
		ApplyChange(property, desc->Child(name.c_str()), nullptr);
}

void PhysicsEditorDialog::doUndo(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && doc->Undo())
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::doRedo(wxCommandEvent& WXUNUSED(event)) {
	XmlDocument* doc = CurrentDocument();
	if (doc && doc->Redo())
		ReloadAfterDocumentChange();
}

void PhysicsEditorDialog::doFilterChanged(wxCommandEvent& event) {
	event.Skip();

	const std::string typed = ToLowerAscii(std::string(physFilter->GetValue().ToUTF8()));
	if (typed == filter)
		return;

	filter = typed;
	RefreshDocuments();
}

void PhysicsEditorDialog::doCloseButton(wxCommandEvent& WXUNUSED(event)) {
	Close();
}

void PhysicsEditorDialog::doCloseWindow(wxCloseEvent& event) {
	// The documents belong to the frame's session and outlive this window, so
	// there is nothing to save or discard here yet.
	event.Skip();
	Destroy();
}
