/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "PhysicsXmlEditor.h"
#include "OutfitStudioFrame.h"
#include "PhysicsPreview.h"
#include "ShapeTreeItems.h"
#include "GLPanel.h"
#include "project/OutfitProject.h"
#include "utils/StringStuff.h"
#include "ui/outfitstudio/PhysicsEditorDialog.h"
#include "utils/ConfigurationManager.h"

#include <fstream>
#include <sstream>
#include <tinyxml2.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/treectrl.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace Physics;

namespace {

const char* const xmlFileFilter = "HDT-SMP physics files (*.xml)|*.xml";

// The XML a row of the pane's list stands for. Every row under a file carries it, so a click anywhere says which.
class XmlItemData : public wxTreeItemData {
public:
	explicit XmlItemData(std::string xmlPath)
		: xmlPath(std::move(xmlPath)) {}

	std::string xmlPath;
};

} // namespace

PhysicsXmlEditor::PhysicsXmlEditor(OutfitStudioFrame& frame)
	: frame(frame) {
	// Read at call time, so the session follows the project the frame has now
	session.SetSourceResolver([&frame](const std::string& xmlPath, GameDataStream::FoundAt* foundAt) -> std::unique_ptr<std::istream> {
		return frame.project ? frame.project->GetPhysicsXmlStream(xmlPath, foundAt) : nullptr;
	});
}

PhysicsXmlEditor::~PhysicsXmlEditor() {
	if (editor)
		editor->Detach();
}

void PhysicsXmlEditor::Init() {
	list = (wxTreeCtrl*)frame.FindWindowByName("physicsSystemTree");
	btnEdit = (wxButton*)frame.FindWindowByName("btnPhysicsEdit");

	frame.Bind(wxEVT_TREE_SEL_CHANGED, &PhysicsXmlEditor::OnListSelect, this, XRCID("physicsSystemTree"));
	frame.Bind(wxEVT_BUTTON, &PhysicsXmlEditor::OnEdit, this, XRCID("btnPhysicsEdit"));
}

void PhysicsXmlEditor::RefreshList() {
	if (!list)
		return;

	list->DeleteAllItems();

	const std::vector<SystemInfo> systems = frame.physicsPreview->ListSystems();
	const wxTreeItemId root = list->AddRoot("physics");

	for (const SystemInfo& system : systems) {
		std::string error;
		XmlDocument* doc = session.Open(system.xmlPath, error);

		wxString label = wxString::FromUTF8(XmlFileName(system.xmlPath));
		if (doc && doc->IsDirty())
			label += " *";

		const wxTreeItemId item = list->AppendItem(root, label, -1, -1, new XmlItemData(system.xmlPath));

		if (doc) {
			const size_t bones = doc->ChildrenOfKind(ElementKind::Bone).size();
			const size_t shapes = doc->ChildrenOfKinds({ElementKind::PerVertexShape, ElementKind::PerTriangleShape}).size();
			const size_t constraints = doc->ChildrenOfKinds({ElementKind::GenericConstraint,
															 ElementKind::StiffSpringConstraint,
															 ElementKind::ConeTwistConstraint,
															 ElementKind::ConstraintGroup})
										   .size();

			list->AppendItem(item, wxString::Format(_("%zu bones, %zu shapes, %zu constraints"), bones, shapes, constraints), -1, -1, new XmlItemData(system.xmlPath));
		}
		else {
			// Either the file isn't there, or only the editor can't read it, in which case it is still simulated
			list->AppendItem(item, wxString::FromUTF8(error), -1, -1, new XmlItemData(system.xmlPath));
		}

		for (const std::string& shapeName : system.shapeNames)
			list->AppendItem(item, wxString::Format(_("Shape: %s"), wxString::FromUTF8(shapeName)), -1, -1, new XmlItemData(system.xmlPath));

		list->Expand(item);
	}

	if (btnEdit)
		btnEdit->Enable(!session.Empty());

	RefreshEditor();
}

void PhysicsXmlEditor::RefreshLinks() {
	// A first link shows the whole pane, and this refills the list either way
	frame.physicsPreview->UpdateControlsVisibility();
	frame.physicsPreview->RequestRebuild();
}

void PhysicsXmlEditor::Shutdown() {
	// A view over the documents that are about to go
	if (editor)
		editor->Close(true);

	if (list)
		list->DeleteAllItems();

	session.Clear();
}

void PhysicsXmlEditor::ReloadDocument(const std::string& xmlPath) {
	if (XmlDocument* doc = session.Find(xmlPath))
		if (!doc->IsDirty())
			session.Close(xmlPath);

	// Opens it again if the loaded meshes link it
	RefreshList();
	frame.physicsPreview->RequestRebuild();
}

void PhysicsXmlEditor::RefreshEditor() {
	if (editor)
		editor->RefreshDocuments();
}

std::string PhysicsXmlEditor::SelectedXmlPath() const {
	if (list) {
		const wxTreeItemId selected = list->GetSelection();
		if (selected.IsOk())
			if (auto* data = static_cast<XmlItemData*>(list->GetItemData(selected)))
				return data->xmlPath;
	}

	// Nothing selected, but with a single XML there is nothing to choose
	const std::vector<std::string> paths = session.Paths();
	return paths.size() == 1 ? paths.front() : std::string();
}

void PhysicsXmlEditor::OnListSelect(wxTreeEvent& event) {
	event.Skip();

	const std::string xmlPath = SelectedXmlPath();
	if (editor && !xmlPath.empty())
		editor->SelectDocument(xmlPath);
}

void PhysicsXmlEditor::OnEdit(wxCommandEvent& WXUNUSED(event)) {
	if (!frame.project)
		return;

	if (!editor) {
		auto* dialog = new PhysicsEditorDialog(&frame, *this);
		if (!dialog->IsLoaded()) {
			dialog->Destroy();
			return;
		}

		editor = dialog;

		// The editor's selection is highlighted on the bones, which have to be shown for that
		if (!frame.session.IsOverlayShown(ViewOverlay::Bones)) {
			frame.session.ShowOverlay(ViewOverlay::Bones, true);
			bonesShownForEditor = true;
		}

		editor->CenterOnParent();
		editor->Show();
	}
	else {
		editor->RefreshDocuments();
		editor->Raise();
	}

	const std::string xmlPath = SelectedXmlPath();
	if (!xmlPath.empty())
		editor->SelectDocument(xmlPath);
}

void PhysicsXmlEditor::EditorClosed() {
	editor = nullptr;

	frame.glView->SetPhysicsBones({});

	// Hidden again unless they were shown before or since
	if (bonesShownForEditor && frame.session.IsOverlayShown(ViewOverlay::Bones))
		frame.session.ShowOverlay(ViewOverlay::Bones, false);
	bonesShownForEditor = false;
}

void PhysicsXmlEditor::HighlightBones(const std::set<std::string>& boneNames) {
	std::set<std::string> lowerCase;
	for (auto& name : boneNames)
		lowerCase.insert(ToLower(name));

	frame.glView->SetPhysicsBones(std::move(lowerCase));
}

bool PhysicsXmlEditor::PropertyChanged(XmlDocument& doc, tinyxml2::XMLElement* element, const ChildDesc& desc) {
	bool patched = false;

	// The schema says what is meant to be patchable, the controller what actually could be
	if (element && desc.patch == PatchKind::Hot) {
		PatchRequest request;
		request.xmlPath = doc.XmlPath();
		request.kind = KindFromName(element->Name());
		request.property = desc.name;
		request.value = doc.GetValue(element, desc);

		if (const char* name = element->Attribute("name"))
			request.elementName = name;
		if (const char* bodyA = element->Attribute("bodyA"))
			request.bodyA = bodyA;
		if (const char* bodyB = element->Attribute("bodyB"))
			request.bodyB = bodyB;

		patched = frame.physicsPreview->PatchProperty(request);
	}

	if (!patched)
		frame.physicsPreview->RequestRebuild();

	return patched;
}

void PhysicsXmlEditor::DocumentChanged(const std::string& WXUNUSED(xmlPath)) {
	frame.physicsPreview->RequestRebuild();
}

wxString PhysicsXmlEditor::XmlDirectory() const {
	const wxString gameData = wxString::FromUTF8(Config["GameDataPath"]);
	if (gameData.empty())
		return wxString();

	// Where hdtSMP64 itself looks, and where nearly every mod puts them. Only a place for the dialog to start.
	wxFileName configs(gameData, wxString());
	configs.AppendDir("SKSE");
	configs.AppendDir("Plugins");
	configs.AppendDir("hdtSkinnedMeshConfigs");
	if (configs.DirExists())
		return configs.GetPath();

	return gameData;
}

std::string PhysicsXmlEditor::GameRelativeXmlPath(const wxString& path) const {
	const wxString gameData = wxString::FromUTF8(Config["GameDataPath"]);
	if (gameData.empty() || path.empty())
		return std::string();

	wxFileName file(path);
	file.MakeAbsolute();

	wxFileName root(gameData, wxString());
	root.MakeAbsolute();

	// MakeRelativeTo walks up out of the folder with "..", which is no path a NIF can carry
	if (!file.MakeRelativeTo(root.GetPath()))
		return std::string();

	wxString relative = file.GetFullPath();
	if (relative.StartsWith("..") || wxFileName(relative).IsAbsolute())
		return std::string();

	// Backslashes, the way the extra data of every shipped model spells it
	relative.Replace("/", "\\");
	return std::string(relative.ToUTF8());
}

bool PhysicsXmlEditor::Save(XmlDocument& doc, bool allowExport) {
	if (doc.Origin() != XmlOrigin::Loose || doc.SourcePath().empty()) {
		if (allowExport)
			return Export(doc);

		wxLogWarning(_("'%s' did not come from a loose file, so there is nothing to save it over. Use Export instead."), wxString::FromUTF8(doc.XmlPath()));
		return false;
	}

	std::string error;
	if (!doc.SaveToFile(doc.SourcePath(), error)) {
		wxMessageBox(wxString::Format(_("Could not write '%s':\n\n%s"), wxString::FromUTF8(doc.SourcePath()), wxString::FromUTF8(error)),
					 _("Save physics XML"),
					 wxICON_ERROR);
		return false;
	}

	doc.MarkClean();
	RefreshEditor();
	return true;
}

bool PhysicsXmlEditor::Export(XmlDocument& doc) {
	// Offered under the name the NIF knows it by, beside where it came from
	wxFileName suggested(wxString::FromUTF8(doc.SourcePath().empty() ? doc.XmlPath() : doc.SourcePath()));

	wxFileDialog dialog(&frame,
						_("Export physics XML"),
						doc.SourcePath().empty() ? XmlDirectory() : suggested.GetPath(),
						suggested.GetFullName(),
						xmlFileFilter,
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_CANCEL)
		return false;

	const std::string path = std::string(dialog.GetPath().ToUTF8());

	std::string error;
	if (!doc.SaveToFile(path, error)) {
		wxMessageBox(wxString::Format(_("Could not write '%s':\n\n%s"), dialog.GetPath(), wxString::FromUTF8(error)), _("Export physics XML"), wxICON_ERROR);
		return false;
	}

	// Saves go to the new file from now on. Exporting a copy doesn't relink the model, so the link stays.
	doc.SetSource(doc.XmlPath(), path, XmlOrigin::Loose);
	doc.MarkClean();
	RefreshEditor();
	return true;
}

bool PhysicsXmlEditor::Import(XmlDocument& doc, const wxString& path) {
	std::ifstream stream(std::string(path.ToUTF8()), std::ios::binary);
	if (!stream.good()) {
		wxMessageBox(wxString::Format(_("Could not read '%s'."), path), _("Import physics XML"), wxICON_ERROR);
		return false;
	}

	std::ostringstream contents;
	contents << stream.rdbuf();

	// One undo step like any other edit: importing over the wrong document is what undo is for
	doc.BeginEdit("import");

	std::string error;
	if (!doc.ReplaceContent(contents.str(), error)) {
		doc.Undo();
		wxMessageBox(wxString::Format(_("'%s' is not a physics XML this editor can read:\n\n%s"), path, wxString::FromUTF8(error)),
					 _("Import physics XML"),
					 wxICON_ERROR);
		return false;
	}

	DocumentChanged(doc.XmlPath());
	return true;
}

XmlDocument* PhysicsXmlEditor::CreateNew(wxWindow* parent) {
	wxFileDialog dialog(parent, _("New physics XML"), XmlDirectory(), "physics.xml", xmlFileFilter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dialog.ShowModal() == wxID_CANCEL)
		return nullptr;

	const wxString path = dialog.GetPath();

	// A NIF links a path relative to the game data folder. A file anywhere else can be edited, but nothing in game
	// will find it, which is worth saying when it is decided.
	std::string xmlPath = GameRelativeXmlPath(path);
	if (xmlPath.empty()) {
		xmlPath = std::string(wxFileName(path).GetFullName().ToUTF8());
		wxLogWarning(_("'%s' is not inside the game data folder, so a model that links it by name will not find it in game."), path);
	}

	XmlDocument* doc = session.Create(xmlPath);
	if (!doc) {
		wxMessageBox(wxString::Format(_("'%s' is already open in this editor."), wxString::FromUTF8(xmlPath)), _("New physics XML"), wxICON_INFORMATION);
		return nullptr;
	}

	doc->SetSource(xmlPath, std::string(path.ToUTF8()), XmlOrigin::Loose);
	if (!Save(*doc, false)) {
		// Nothing on disk and nothing linking it
		session.Close(xmlPath);
		return nullptr;
	}

	return doc;
}

bool PhysicsXmlEditor::CheckPendingChanges() {
	if (!session.AnyDirty())
		return true;

	wxString names;
	for (XmlDocument* doc : session.Documents())
		if (doc->IsDirty())
			names += "\n    " + wxString::FromUTF8(doc->XmlPath());

	wxMessageDialog dialog(&frame,
						   wxString::Format(_("These physics XMLs have changes that are not on disk:\n%s\n\nWould you like to save them now?"), names),
						   _("Unsaved physics XML changes"),
						   wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);

	const int answer = dialog.ShowModal();
	if (answer == wxID_CANCEL)
		return false;
	if (answer == wxID_NO)
		return true;

	// Saving is what was asked for, so a document with nothing to write back over asks where to put it
	for (XmlDocument* doc : session.Documents())
		if (doc->IsDirty() && !Save(*doc, true))
			return false;

	return true;
}

std::vector<std::string> PhysicsXmlEditor::SelectedShapeNames() const {
	std::vector<std::string> names;
	for (ShapeItemData* item : frame.GetSelectedItems())
		if (item && item->GetShape())
			names.push_back(item->GetShape()->name.get());

	return names;
}

void PhysicsXmlEditor::LinkToSelectedShapes(XmlDocument& doc, wxWindow* parent) {
	if (!frame.project)
		return;

	const std::vector<std::string> shapes = SelectedShapeNames();
	if (shapes.empty()) {
		wxMessageBox(_("Select the shapes to link it to in Outfit Studio first."), _("Link physics XML"), wxICON_INFORMATION, parent);
		return;
	}

	if (frame.project->LinkPhysicsFile(doc.XmlPath(), shapes) == 0) {
		wxMessageBox(wxString::Format(_("Every selected shape already links '%s'."), wxString::FromUTF8(doc.XmlPath())),
					 _("Link physics XML"),
					 wxICON_INFORMATION,
					 parent);
		return;
	}

	// The link is stored in the NIF, so it is the project that has unsaved changes
	frame.SetPendingChanges();
	RefreshLinks();
}

bool PhysicsXmlEditor::HasProject() const {
	return frame.project != nullptr;
}

std::vector<std::string> PhysicsXmlEditor::ShapeNames() const {
	if (!frame.project || !frame.project->GetWorkNif()->IsValid())
		return {};

	return frame.project->GetWorkNif()->GetShapeNames();
}

std::vector<std::string> PhysicsXmlEditor::BoneNames() const {
	std::vector<std::string> names;
	frame.skeleton.GetBoneNames(names);

	// A bone the skeleton doesn't have still resolves when the NIF has a node of that name (findAnimBone falls back
	// to LoadCustomBoneFromNif)
	if (frame.project)
		for (const nifly::NiNode* node : frame.project->GetWorkNif()->GetNodes())
			if (!node->name.get().empty())
				names.push_back(node->name.get());

	return names;
}

std::vector<std::string> PhysicsXmlEditor::ShapeBoneNames(const std::string& shapeName) const {
	if (!frame.project)
		return {};

	auto& shapeBones = frame.project->GetWorkAnim()->shapeBones;
	auto bones = shapeBones.find(shapeName);
	return bones != shapeBones.end() ? bones->second : std::vector<std::string>();
}

ValidationContext PhysicsXmlEditor::ValidationInputs() const {
	ValidationContext context;
	context.nifShapeNames = ShapeNames();
	context.skeletonBoneNames = BoneNames();
	return context;
}
