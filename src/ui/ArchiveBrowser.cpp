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
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/imaglist.h>
#include <wx/xrc/xmlres.h>

#include <algorithm>
#include <set>

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
	extension = XRCCTRL(*this, "archiveExtension", wxComboBox);
	status = XRCCTRL(*this, "archiveStatus", wxStaticText);
	btnReplace = XRCCTRL(*this, "btnArchiveReplace", wxButton);
	btnLoad = XRCCTRL(*this, "btnArchiveLoad", wxButton);
	btnExport = XRCCTRL(*this, "btnArchiveExport", wxButton);

	tree = new wxTreeListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTL_MULTIPLE);
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
	extension->Bind(wxEVT_TEXT, &ArchiveBrowser::OnSearchText, this);
	extension->Bind(wxEVT_COMBOBOX, [&](wxCommandEvent&) {
		searchTimer.Stop();
		ApplyFilter();
	});
	extension->Bind(wxEVT_TEXT_ENTER, [&](wxCommandEvent&) {
		searchTimer.Stop();
		ApplyFilter();
	});
	Bind(wxEVT_TIMER, &ArchiveBrowser::OnSearchTimer, this);

	tree->Bind(wxEVT_TREELIST_ITEM_EXPANDING, &ArchiveBrowser::OnItemExpanding, this);
	tree->Bind(wxEVT_TREELIST_ITEM_ACTIVATED, &ArchiveBrowser::OnItemActivated, this);
	tree->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &ArchiveBrowser::OnSelectionChanged, this);
	tree->Bind(wxEVT_TREELIST_ITEM_CONTEXT_MENU, &ArchiveBrowser::OnItemContextMenu, this);

	btnReplace->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { LoadSelected(false); });
	btnLoad->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { LoadSelected(true); });
	btnExport->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { ExportSelected(); });
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

	FillExtensions();
	ApplyFilter();
}

void ArchiveBrowser::FillExtensions() {
	// .nif first, then the extensions of the loaded archives by file count, * for all files last
	std::vector<std::pair<size_t, std::string>> found;
	for (auto& ext : index.ExtensionCounts())
		if (!ext.first.empty() && ext.first != ".nif")
			found.emplace_back(ext.second, ext.first);
	std::sort(found.begin(), found.end(), [](auto& a, auto& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });

	wxArrayString choices;
	choices.Add(".nif");
	for (auto& ext : found)
		choices.Add(wxString::FromUTF8(ext.second));
	choices.Add("*");

	// Set() would clear the text, which is the current filter
	wxString current = extension->GetValue();
	extension->Set(choices);
	extension->ChangeValue(current.empty() ? wxString(".nif") : current);
}

void ArchiveBrowser::ApplyFilter() {
	std::string needle = search->GetValue().Trim().Trim(false).ToUTF8().data();
	std::string ext = extension->GetValue().Trim().Trim(false).ToUTF8().data();
	wxString extLabel = ext.empty() || ext == "*" ? wxString(_("files")) : wxString::Format(_("%s files"), wxString::FromUTF8(ext));

	if (index.Archives().empty()) {
		filtered = ArchiveIndex::Node();
		shown = &filtered;
		status->SetLabel(_("No archives loaded. Check the game data path and archive list in Settings."));
		ShowTree(false);
		return;
	}

	if (needle.empty() && ArchiveIndex::HasExtension("", ext)) {
		// No filter at all, show the whole index without copying it
		filtered = ArchiveIndex::Node();
		shown = &index.Root();
		status->SetLabel(wxString::Format(_("%lu %s in %lu archives"), (unsigned long)index.FileCount(), extLabel, (unsigned long)index.Archives().size()));
		ShowTree(false);
		return;
	}

	size_t matches = 0;
	filtered = index.Filter(needle, ext, &matches);
	shown = &filtered;
	if (needle.empty()) {
		status->SetLabel(wxString::Format(_("%lu %s in %lu archives"), (unsigned long)matches, extLabel, (unsigned long)index.Archives().size()));
		ShowTree(false);
	}
	else {
		status->SetLabel(wxString::Format(_("%lu of %lu %s match"), (unsigned long)matches, (unsigned long)index.FileCount(ext), extLabel));
		ShowTree(matches <= MaxExpandedMatches);
	}
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
	wxTreeListItems items;
	if (tree->GetSelections(items) != 1)
		return nullptr;

	auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(items.front()));
	return data ? data->file : nullptr;
}

std::vector<const ArchiveIndex::Entry*> ArchiveBrowser::GetSelectedFiles() const {
	wxTreeListItems items;
	tree->GetSelections(items);

	std::vector<const ArchiveIndex::Entry*> files;
	for (auto& item : items) {
		auto data = static_cast<ArchiveTreeData*>(tree->GetItemData(item));
		if (data && data->file)
			files.push_back(data->file);
	}
	return files;
}

void ArchiveBrowser::LoadSelected(bool add) {
	const ArchiveIndex::Entry* file = GetSelectedFile();
	if (!file || file->archive >= index.Archives().size() || !ArchiveIndex::IsMeshFile(file->path))
		return;

	// Copy, loading may show dialogs that let the tree change
	std::string path = file->path;
	std::string archivePath = index.Archives()[file->archive].path;
	frame->LoadMeshFromArchive(path, archivePath, add);
}

void ArchiveBrowser::ExportSelected() {
	std::vector<const ArchiveIndex::Entry*> files = GetSelectedFiles();
	if (files.empty())
		return;

	// Copies, the dialogs below let the tree change
	std::vector<ArchiveIndex::Entry> entries;
	for (auto file : files)
		entries.push_back(*file);

	// The file name of an archive path, e.g. "foo_1.nif"
	auto fileName = [](const ArchiveIndex::Entry& entry) {
		size_t sep = entry.path.rfind('/');
		return ArchiveText(sep == std::string::npos ? entry.path : entry.path.substr(sep + 1));
	};

	std::vector<wxString> targets;
	size_t skipped = 0;
	if (entries.size() == 1) {
		// One file can be renamed on the way out
		wxString name = fileName(entries.front());
		wxString ext = wxFileName(name).GetExt();
		wxString wildcard = ext.empty() ? wxString("*.*") : wxString::Format("*.%s|*.%s|%s|*.*", ext, ext, _("All Files"));
		wxString target = wxFileSelector(_("Export file from archive"), exportDir, name, ext, wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (target.empty())
			return;

		targets.push_back(target);
		exportDir = wxFileName(target).GetPath();
	}
	else {
		wxString dir = wxDirSelector(wxString::Format(_("Export %zu files from archives to folder"), entries.size()), exportDir, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
		if (dir.empty())
			return;
		exportDir = dir;

		// Files go straight into the folder, unless two have the same name: then all keep their archive paths
		std::set<wxString> names;
		bool sameNames = false;
		for (auto& entry : entries)
			sameNames |= !names.insert(fileName(entry).Lower()).second;

		size_t existing = 0;
		for (auto& entry : entries) {
			wxFileName target(dir + wxFILE_SEP_PATH + (sameNames ? ArchiveText(entry.path) : fileName(entry)));
			target.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
			if (target.FileExists())
				existing++;
			targets.push_back(target.GetFullPath());
		}

		if (existing > 0) {
			int answer = wxMessageBox(wxString::Format(_("%zu of the files already exist in the folder. Replace them?"), existing),
									  _("Export files"), wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
			if (answer == wxCANCEL)
				return;
			if (answer == wxNO) {
				// Skip the existing ones
				for (size_t i = 0; i < entries.size();) {
					if (wxFileExists(targets[i])) {
						entries.erase(entries.begin() + i);
						targets.erase(targets.begin() + i);
						skipped++;
					}
					else
						i++;
				}
			}
		}
	}

	if (entries.empty()) {
		status->SetLabel(wxString::Format(_("Nothing exported, the %zu files already exist in %s"), skipped, exportDir));
		return;
	}

	wxBusyCursor busy;
	size_t exported = 0;
	wxString errors;
	for (size_t i = 0; i < entries.size(); i++) {
		wxString error;
		if (ExportFile(entries[i], targets[i], error))
			exported++;
		else
			errors += ArchiveText(entries[i].path) + ": " + error + "\n";
	}

	wxString done = wxString::Format(_("Exported %zu of %zu files to %s"), exported, entries.size(), exportDir);
	if (skipped > 0)
		done += wxString::Format(_(", skipped %zu that already exist"), skipped);
	status->SetLabel(done);
	if (!errors.empty())
		wxMessageBox(_("These files couldn't be exported:") + "\n\n" + errors, _("Export files"), wxOK | wxICON_ERROR, this);
}

bool ArchiveBrowser::ExportFile(const ArchiveIndex::Entry& file, const wxString& target, wxString& error) const {
	if (file.archive >= index.Archives().size()) {
		error = _("unknown archive");
		return false;
	}

	const std::string& archivePath = index.Archives()[file.archive].path;
	FSArchiveFile* archive = nullptr;
	for (FSArchiveFile* candidate : FSManager::archiveList())
		if (candidate && candidate->path() == archivePath)
			archive = candidate;

	wxMemoryBuffer data;
	if (!archive || !archive->fileContents(file.path, data) || data.IsEmpty()) {
		error = _("couldn't be read from the archive");
		return false;
	}

	wxFileName targetName(target);
	if (!targetName.DirExists() && !wxFileName::Mkdir(targetName.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		error = _("the folder couldn't be created");
		return false;
	}

	wxFile out;
	if (!out.Create(target, true) || out.Write(data.GetData(), data.GetDataLen()) != data.GetDataLen()) {
		error = _("couldn't be written");
		return false;
	}
	return true;
}

void ArchiveBrowser::UpdateButtons() {
	const ArchiveIndex::Entry* file = GetSelectedFile();
	bool hasMesh = file && ArchiveIndex::IsMeshFile(file->path);
	btnReplace->Enable(hasMesh);
	btnLoad->Enable(hasMesh);
	btnExport->Enable(!GetSelectedFiles().empty());
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
	// Act on the row that was clicked, or on the selection it belongs to
	if (event.GetItem().IsOk() && !tree->IsSelected(event.GetItem())) {
		tree->UnselectAll();
		tree->Select(event.GetItem());
		UpdateButtons();
	}

	if (GetSelectedFiles().empty())
		return;

	wxMenu menu;
	const ArchiveIndex::Entry* file = GetSelectedFile();
	if (file && ArchiveIndex::IsMeshFile(file->path)) {
		menu.Append(XRCID("archiveReplaceOutfit"), _("Replace Outfit"));
		menu.Append(XRCID("archiveLoad"), _("Load"));
		menu.AppendSeparator();
	}
	menu.Append(XRCID("archiveExport"), _("Export..."));

	int id = tree->GetPopupMenuSelectionFromUser(menu);
	if (id == XRCID("archiveReplaceOutfit"))
		LoadSelected(false);
	else if (id == XRCID("archiveLoad"))
		LoadSelected(true);
	else if (id == XRCID("archiveExport"))
		ExportSelected();
}

void ArchiveBrowser::OnClose(wxCloseEvent& event) {
	searchTimer.Stop();
	event.Skip();
	Destroy();
}
