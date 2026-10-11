/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ArchiveBrowser.h"

#include "../program/OutfitStudio.h"
#include "../utils/ConfigurationManager.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <wx/xrc/xmlres.h>

extern ConfigurationManager Config;

namespace {
enum TreeImage { ImageFolder, ImageFolderOpen, ImageFile };

// Above this many search matches the tree stays collapsed
constexpr size_t MaxExpandedMatches = 500;

class ArchiveTreeData : public wxClientData {
public:
	explicit ArchiveTreeData(const ArchiveIndex::Node* folder)
		: folder(folder) {}
	explicit ArchiveTreeData(const ArchiveIndex::Entry* file)
		: file(file) {}

	const ArchiveIndex::Node* folder = nullptr;
	const ArchiveIndex::Entry* file = nullptr;
	bool populated = false;
};

// Archive paths are usually ASCII, older tools store other characters as
// Windows-1252. Archive file names come in the system's code page.
wxString ArchiveText(const std::string& text) {
	wxString result = wxString::FromUTF8(text);
	if (result.empty() && !text.empty())
		result = wxString(text.c_str(), wxCSConv(wxFONTENCODING_CP1252));
	return result;
}
}

ArchiveBrowser::ArchiveBrowser(OutfitStudioFrame* parent)
	: frame(parent)
	, searchTimer(this) {
	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ArchiveBrowser.xrc");
	xrc->LoadDialog(this, parent, "dlgArchiveBrowser");

	search = XRCCTRL(*this, "archiveSearch", wxSearchCtrl);
	status = XRCCTRL(*this, "archiveStatus", wxStaticText);
	btnReplace = XRCCTRL(*this, "btnArchiveReplace", wxButton);
	btnImport = XRCCTRL(*this, "btnArchiveImport", wxButton);

	tree = new wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_SINGLE);
	xrc->AttachUnknownControl("archiveTree", tree, this);

	tree->AppendColumn(_("File"), FromDIP(360), wxALIGN_LEFT, wxCOL_RESIZABLE);
	tree->AppendColumn(_("Size"), FromDIP(80), wxALIGN_RIGHT, wxCOL_RESIZABLE);
	tree->AppendColumn(_("Archive"), FromDIP(200), wxALIGN_LEFT, wxCOL_RESIZABLE);

	wxSize iconSize = wxArtProvider::GetSizeHint(wxART_LIST, this);
	auto images = new wxImageList(iconSize.x, iconSize.y);
	images->Add(wxArtProvider::GetIcon(wxART_FOLDER, wxART_LIST, iconSize));
	images->Add(wxArtProvider::GetIcon(wxART_FOLDER_OPEN, wxART_LIST, iconSize));
	images->Add(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_LIST, iconSize));
	tree->AssignImageList(images);

	search->Bind(wxEVT_TEXT, &ArchiveBrowser::OnSearchText, this);
	search->Bind(wxEVT_SEARCH, [&](wxCommandEvent&) {
		searchTimer.Stop();
		ApplyFilter();
	});
	search->Bind(wxEVT_SEARCH_CANCEL, [&](wxCommandEvent&) { search->Clear(); });
	Bind(wxEVT_TIMER, &ArchiveBrowser::OnSearchTimer, this);

	tree->Bind(wxEVT_TREELIST_ITEM_EXPANDING, &ArchiveBrowser::OnItemExpanding, this);
	tree->Bind(wxEVT_TREELIST_ITEM_ACTIVATED, &ArchiveBrowser::OnItemActivated, this);
	tree->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &ArchiveBrowser::OnSelectionChanged, this);
	tree->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU, &ArchiveBrowser::OnItemContextMenu, this);

	btnReplace->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { LoadSelected(false); });
	btnImport->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { LoadSelected(true); });
	XRCCTRL(*this, "btnArchiveRefresh", wxButton)->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { Rebuild(); });
	XRCCTRL(*this, "btnArchiveClose", wxButton)->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { Close(); });
	Bind(wxEVT_CLOSE_WINDOW, &ArchiveBrowser::OnClose, this);

	SetSize(FromDIP(wxSize(720, 640)));
	CenterOnParent();

	Rebuild();
}

ArchiveBrowser::~ArchiveBrowser() {
	wxXmlResource::Get()->Unload(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/ArchiveBrowser.xrc");
}

void ArchiveBrowser::Rebuild() {
	wxBusyCursor busy;

	// The tree points into the index, so clear it first
	tree->DeleteAllItems();
	shown = nullptr;
	filtered = ArchiveIndex::Node();
	index.Clear();

	for (FSArchiveFile* archive : FSManager::archiveList()) {
		if (!archive)
			continue;

		size_t archiveId = index.AddArchive(archive->path(), archive->name());
		archive->forEachFile([&](const std::string& path, wxInt64 size) { index.AddFile(path, size, archiveId); });
	}

	ApplyFilter();
}

void ArchiveBrowser::ApplyFilter() {
	std::string needle = search->GetValue().Trim().Trim(false).ToUTF8().data();

	if (index.Archives().empty()) {
		filtered = ArchiveIndex::Node();
		shown = &filtered;
		status->SetLabel(_("No archives loaded. Check the game data path and archive list in Settings."));
		ShowTree(false);
		return;
	}

	if (needle.empty()) {
		filtered = ArchiveIndex::Node();
		shown = &index.Root();
		status->SetLabel(wxString::Format(_("%lu meshes in %lu archives"), (unsigned long)index.FileCount(), (unsigned long)index.Archives().size()));
		ShowTree(false);
		return;
	}

	size_t matches = 0;
	filtered = index.Filter(needle, &matches);
	shown = &filtered;
	status->SetLabel(wxString::Format(_("%lu of %lu meshes match"), (unsigned long)matches, (unsigned long)index.FileCount()));
	ShowTree(matches <= MaxExpandedMatches);
}

void ArchiveBrowser::ShowTree(bool expandAll) {
	tree->Freeze();
	tree->DeleteAllItems();

	if (shown) {
		PopulateItem(tree->GetRootItem(), *shown);
		if (expandAll) {
			ExpandAll(tree->GetRootItem());
		}
		else {
			// Top level folders like "meshes" hold everything, so open them right away
			for (wxTreeListItem item = tree->GetFirstChild(tree->GetRootItem()); item.IsOk(); item = tree->GetNextSibling(item)) {
				PopulateFolder(item);
				tree->Expand(item);
			}
		}
	}

	tree->Thaw();
	UpdateButtons();
}

void ArchiveBrowser::PopulateItem(wxTreeListItem parent, const ArchiveIndex::Node& node) {
	for (auto& folder : node.folders) {
		wxTreeListItem item = tree->AppendItem(parent, ArchiveText(folder.first), ImageFolder, ImageFolderOpen, new ArchiveTreeData(&folder.second));

		// Placeholder so the folder can be expanded, filled in on demand
		if (!folder.second.Empty())
			tree->AppendItem(item, wxEmptyString);
	}

	const auto& archives = index.Archives();
	for (auto& file : node.files) {
		wxTreeListItem item = tree->AppendItem(parent, ArchiveText(file.first), ImageFile, ImageFile, new ArchiveTreeData(&file.second));
		tree->SetItemText(item, 1, wxString::Format("%lluKB", (unsigned long long)((file.second.size + 1023) / 1024)));
		if (file.second.archive < archives.size())
			tree->SetItemText(item, 2, ArchiveText(archives[file.second.archive].name));
	}
}

void ArchiveBrowser::PopulateFolder(wxTreeListItem item) {
	auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(item));
	if (!data || !data->folder || data->populated)
		return;

	data->populated = true;

	wxTreeListItem placeholder = tree->GetFirstChild(item);
	if (placeholder.IsOk())
		tree->DeleteItem(placeholder);

	PopulateItem(item, *data->folder);
}

void ArchiveBrowser::ExpandAll(wxTreeListItem parent) {
	for (wxTreeListItem item = tree->GetFirstChild(parent); item.IsOk(); item = tree->GetNextSibling(item)) {
		auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(item));
		if (!data || !data->folder)
			continue;

		PopulateFolder(item);
		tree->Expand(item);
		ExpandAll(item);
	}
}

const ArchiveIndex::Entry* ArchiveBrowser::GetSelectedFile() const {
	wxTreeListItem item = tree->GetSelection();
	if (!item.IsOk())
		return nullptr;

	auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(item));
	return data ? data->file : nullptr;
}

void ArchiveBrowser::LoadSelected(bool import) {
	const ArchiveIndex::Entry* file = GetSelectedFile();
	if (!file || file->archive >= index.Archives().size())
		return;

	// Copy, loading may show dialogs that let the tree change
	std::string path = file->path;
	std::string archivePath = index.Archives()[file->archive].path;
	frame->LoadMeshFromArchive(path, archivePath, import);
}

void ArchiveBrowser::UpdateButtons() {
	bool hasFile = GetSelectedFile() != nullptr;
	btnReplace->Enable(hasFile);
	btnImport->Enable(hasFile);
}

void ArchiveBrowser::OnSearchText(wxCommandEvent& WXUNUSED(event)) {
	searchTimer.StartOnce(300);
}

void ArchiveBrowser::OnSearchTimer(wxTimerEvent& WXUNUSED(event)) {
	ApplyFilter();
}

void ArchiveBrowser::OnItemExpanding(wxTreeListEvent& event) {
	PopulateFolder(event.GetItem());
}

void ArchiveBrowser::OnItemActivated(wxTreeListEvent& event) {
	auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(event.GetItem()));
	if (!data)
		return;

	if (data->file) {
		LoadSelected(true);
	}
	else if (data->folder) {
		if (tree->IsExpanded(event.GetItem())) {
			tree->Collapse(event.GetItem());
		}
		else {
			PopulateFolder(event.GetItem());
			tree->Expand(event.GetItem());
		}
	}
}

void ArchiveBrowser::OnSelectionChanged(wxTreeListEvent& WXUNUSED(event)) {
	UpdateButtons();
}

void ArchiveBrowser::OnItemContextMenu(wxTreeListEvent& event) {
	// Act on the row that was clicked, which isn't always the selected one
	if (event.GetItem().IsOk() && event.GetItem() != tree->GetSelection()) {
		tree->Select(event.GetItem());
		UpdateButtons();
	}

	if (!GetSelectedFile())
		return;

	wxMenu menu;
	menu.Append(XRCID("archiveReplaceOutfit"), _("Replace Outfit"));
	menu.Append(XRCID("archiveImport"), _("Import"));

	int id = tree->GetPopupMenuSelectionFromUser(menu);
	if (id == XRCID("archiveReplaceOutfit"))
		LoadSelected(false);
	else if (id == XRCID("archiveImport"))
		LoadSelected(true);
}

void ArchiveBrowser::OnClose(wxCloseEvent& event) {
	searchTimer.Stop();
	event.Skip();
	Destroy();
}
