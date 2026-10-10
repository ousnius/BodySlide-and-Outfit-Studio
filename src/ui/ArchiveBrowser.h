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

// Modeless tree of the mesh files in all loaded BSA/BA2 archives, to load them
// as the outfit or import them into the project.
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
	wxTreeListCtrl* tree = nullptr;
	wxStaticText* status = nullptr;
	wxButton* btnReplace = nullptr;
	wxButton* btnImport = nullptr;
	wxTimer searchTimer;

	ArchiveIndex index;
	ArchiveIndex::Node filtered;
	const ArchiveIndex::Node* shown = nullptr;

	void ApplyFilter();
	void ShowTree(bool expandAll);
	void PopulateItem(wxTreeListItem parent, const ArchiveIndex::Node& node);
	void PopulateFolder(wxTreeListItem item);
	void ExpandAll(wxTreeListItem parent);

	const ArchiveIndex::Entry* GetSelectedFile() const;
	void LoadSelected(bool import);
	void UpdateButtons();

	void OnSearchText(wxCommandEvent& event);
	void OnSearchTimer(wxTimerEvent& event);
	void OnItemExpanding(wxTreeListEvent& event);
	void OnItemActivated(wxTreeListEvent& event);
	void OnSelectionChanged(wxTreeListEvent& event);
	void OnItemContextMenu(wxTreeListEvent& event);
	void OnClose(wxCloseEvent& event);
};
