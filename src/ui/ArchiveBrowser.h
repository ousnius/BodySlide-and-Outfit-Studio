/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/ArchiveIndex.h"

#include <wx/srchctrl.h>
#include <wx/timer.h>
#include <wx/treelist.h>
#include <wx/wx.h>

class OutfitStudioFrame;

// Modeless tree of the files in all loaded BSA/BA2 archives, filtered by file
// extension (.nif by default), to load meshes as the outfit or into the project
// beside it, or to export files to a folder.
class ArchiveBrowser : public wxDialog {
public:
	ArchiveBrowser(OutfitStudioFrame* parent);
	~ArchiveBrowser();

	// Re-reads the file lists of the loaded archives. Must be called whenever
	// the archives were reloaded, since the tree refers to them by path.
	void Rebuild();

private:
	OutfitStudioFrame* frame = nullptr;

	wxSearchCtrl* search = nullptr;
	wxComboBox* extension = nullptr;
	wxTreeListCtrl* tree = nullptr;
	wxStaticText* status = nullptr;
	wxButton* btnReplace = nullptr;
	wxButton* btnLoad = nullptr;
	wxButton* btnExport = nullptr;
	wxTimer searchTimer;
	wxString exportDir; // Folder of the last export, where the next one starts

	ArchiveIndex index;
	ArchiveIndex::Node filtered;
	const ArchiveIndex::Node* shown = nullptr;

	void FillExtensions();
	void ApplyFilter();
	void ShowTree(bool expandAll);
	void PopulateItem(wxTreeListItem parent, const ArchiveIndex::Node& node);
	void PopulateFolder(wxTreeListItem item);
	void ExpandAll(wxTreeListItem parent);

	// The selected file when exactly one item is selected and it's a file
	const ArchiveIndex::Entry* GetSelectedFile() const;
	// All selected files, folders left out
	std::vector<const ArchiveIndex::Entry*> GetSelectedFiles() const;
	void LoadSelected(bool add);
	void ExportSelected();
	bool ExportFile(const ArchiveIndex::Entry& file, const wxString& target, wxString& error) const;
	void UpdateButtons();

	void OnSearchText(wxCommandEvent& event);
	void OnSearchTimer(wxTimerEvent& event);
	void OnItemExpanding(wxTreeListEvent& event);
	void OnItemActivated(wxTreeListEvent& event);
	void OnSelectionChanged(wxTreeListEvent& event);
	void OnItemContextMenu(wxTreeListEvent& event);
	void OnClose(wxCloseEvent& event);
};
