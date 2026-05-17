/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderDataDialog.h"

#include "OutfitStudio.h"
#include "../utils/StringStuff.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

wxString SliderDataList::JoinDataFolders(const std::vector<std::string>& dataFolders) {
	if (dataFolders.empty())
		return wxString();

	return wxString::FromUTF8(JoinStrings(dataFolders, ";"));
}

void SliderDataList::Configure(wxListCtrl* list, bool includeSlider) {
	list->ClearAll();
	int column = 0;
	if (includeSlider)
		list->InsertColumn(column++, _("Slider"), wxLIST_FORMAT_LEFT, 150);

	list->InsertColumn(column++, _("Shape"), wxLIST_FORMAT_LEFT, includeSlider ? 145 : 130);
	list->InsertColumn(column++, _("Target"), wxLIST_FORMAT_LEFT, includeSlider ? 145 : 130);
	list->InsertColumn(column++, _("Data"), wxLIST_FORMAT_LEFT, includeSlider ? 220 : 190);
	list->InsertColumn(column++, _("Source"), wxLIST_FORMAT_LEFT, 85);
	list->InsertColumn(column++, _("Data Folder(s)"), wxLIST_FORMAT_LEFT, includeSlider ? 245 : 210);
	list->InsertColumn(column++, _("File"), wxLIST_FORMAT_LEFT, includeSlider ? 245 : 210);
	list->InsertColumn(column++, _("Status"), wxLIST_FORMAT_LEFT, 85);
}

void SliderDataList::Populate(wxListCtrl* list, const std::vector<SliderDataLocation>& rows, const std::vector<size_t>* visibleRows, bool includeSlider) {
	list->Freeze();
	list->DeleteAllItems();

	const auto appendRow = [&](size_t rowIndex) {
		if (rowIndex >= rows.size())
			return;

		const auto& row = rows[rowIndex];
		long item = list->InsertItem(list->GetItemCount(), includeSlider ? wxString::FromUTF8(row.sliderName) : wxString::FromUTF8(row.shapeName));
		int column = 1;
		if (includeSlider)
			list->SetItem(item, column++, wxString::FromUTF8(row.shapeName));

		list->SetItem(item, column++, wxString::FromUTF8(row.targetName));
		list->SetItem(item, column++, wxString::FromUTF8(row.dataName));
		list->SetItem(item, column++, row.local ? _("Local") : _("External"));
		list->SetItem(item, column++, JoinDataFolders(row.dataFolders));
		list->SetItem(item, column++, wxString::FromUTF8(row.fileName));
		list->SetItem(item, column++, row.fileName.empty() ? _("No file") : row.resolved ? _("Found") : _("Missing"));
		list->SetItemData(item, static_cast<long>(rowIndex));
	};

	if (visibleRows) {
		for (auto rowIndex : *visibleRows)
			appendRow(rowIndex);
	}
	else {
		for (size_t rowIndex = 0; rowIndex < rows.size(); rowIndex++)
			appendRow(rowIndex);
	}

	list->Thaw();
}

size_t SliderDataList::GetSelected(wxListCtrl* list, const std::vector<SliderDataLocation>& rows, std::vector<SliderDataLocation>& outLocations) {
	outLocations.clear();
	long item = -1;
	while ((item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED)) != -1) {
		size_t rowIndex = static_cast<size_t>(list->GetItemData(item));
		if (rowIndex < rows.size())
			outLocations.push_back(rows[rowIndex]);
	}

	return outLocations.size();
}

bool SliderDataList::AllHaveSource(const std::vector<SliderDataLocation>& locations, bool local) {
	if (locations.empty())
		return false;

	for (auto& location : locations)
		if (location.local != local)
			return false;

	return true;
}

void SliderDataList::BindSelectAll(wxWindow* window, wxListCtrl* list, const std::function<void()>& updateButtons) {
	window->Bind(wxEVT_CHAR_HOOK, [list, updateButtons](wxKeyEvent& event) {
		int keyCode = event.GetKeyCode();
		bool selectAll = event.ControlDown() && !event.AltDown() && (keyCode == 'A' || keyCode == 'a' || keyCode == 1);
		if (!selectAll) {
			event.Skip();
			return;
		}

		wxWindow* focus = wxWindow::FindFocus();
		if (focus && focus->IsKindOf(wxCLASSINFO(wxTextCtrl))) {
			event.Skip();
			return;
		}

		list->Freeze();
		for (long item = 0; item < list->GetItemCount(); item++)
			list->SetItemState(item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
		list->Thaw();
		updateButtons();
	});
}

bool SliderDataList::MakeLocal(wxWindow* parent, OutfitProject* project, const std::vector<SliderDataLocation>& locations) {
	bool changed = false;
	for (auto& location : locations) {
		if (location.local)
			continue;

		std::string errorMessage;
		if (!project->SetSliderDataLocal(location.sliderIndex, location.dataIndex, &errorMessage)) {
			wxMessageBox(wxString::FromUTF8(errorMessage), _("Slider Data"), wxOK | wxICON_WARNING, parent);
			return changed;
		}

		changed = true;
	}

	return changed;
}

bool SliderDataList::EditFolders(wxWindow* parent, OutfitProject* project, const std::vector<SliderDataLocation>& locations) {
	if (locations.empty())
		return false;

	bool makeExternal = locations.front().local;
	if (!AllHaveSource(locations, makeExternal)) {
		wxMessageBox(_("Select either local or external slider data entries, not both."), _("Slider Data"), wxOK | wxICON_WARNING, parent);
		return false;
	}

	wxDialog dlg(parent, wxID_ANY, makeExternal ? _("Make Slider Data External") : _("Edit Shape Data Folders"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	auto* mainSizer = new wxBoxSizer(wxVERTICAL);

	wxString message = makeExternal ?
		_("Enter one or more shape data folders separated by semicolons. These folders will be used to find the selected slider data.") :
		_("Enter one or more shape data folders separated by semicolons. These folders will be shared by all external slider data for each selected shape.");
	auto* messageText = new wxStaticText(&dlg, wxID_ANY, message);
	messageText->Wrap(480);
	mainSizer->Add(messageText, 0, wxALL | wxEXPAND, 10);

	wxString commonFolders;
	if (!makeExternal) {
		commonFolders = JoinDataFolders(locations.front().dataFolders);
		for (size_t locationIndex = 1; locationIndex < locations.size(); locationIndex++) {
			if (JoinDataFolders(locations[locationIndex].dataFolders) != commonFolders) {
				commonFolders.clear();
				break;
			}
		}
	}

	auto* folderLabel = new wxStaticText(&dlg, wxID_ANY, _("Data Folder(s):"));
	auto* folderText = new wxTextCtrl(&dlg, wxID_ANY, commonFolders);
	mainSizer->Add(folderLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
	mainSizer->Add(folderText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

	wxString commonOSDFile;
	bool hasOSD = false;
	bool hasOSDValue = false;
	for (auto& location : locations) {
		if (location.isBSD)
			continue;

		hasOSD = true;
		wxString osdFile = wxString::FromUTF8(location.dataFileName);
		if (!hasOSDValue) {
			commonOSDFile = osdFile;
			hasOSDValue = true;
		}
		else if (osdFile != commonOSDFile) {
			commonOSDFile.clear();
			break;
		}
	}

	auto* osdLabel = new wxStaticText(&dlg, wxID_ANY, _("OSD File:"));
	auto* osdText = new wxTextCtrl(&dlg, wxID_ANY, commonOSDFile);
	if (!hasOSD) {
		osdLabel->Enable(false);
		osdText->Enable(false);
	}
	mainSizer->Add(osdLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);
	mainSizer->Add(osdText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

	mainSizer->Add(dlg.CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
	dlg.SetSizerAndFit(mainSizer);
	dlg.SetMinSize(wxSize(520, -1));
	dlg.CenterOnParent();
	folderText->SetFocus();
	folderText->SetSelection(-1, -1);

	if (dlg.ShowModal() != wxID_OK)
		return false;

	std::vector<std::pair<size_t, size_t>> dataEntries;
	for (auto& location : locations)
		dataEntries.emplace_back(location.sliderIndex, location.dataIndex);

	std::vector<std::string> dataFolders;
	wxStringTokenizer tokenizer(folderText->GetValue(), ";");
	while (tokenizer.HasMoreTokens()) {
		wxString token = tokenizer.GetNextToken();
		token.Trim(true);
		token.Trim(false);
		if (!token.IsEmpty())
			dataFolders.push_back(ToOSSlashes(std::string(token.ToUTF8().data())));
	}

	wxString osdValue = osdText->GetValue();
	osdValue.Trim(true);
	osdValue.Trim(false);
	std::string osdFileName = osdValue.IsEmpty() ? std::string() : ToOSSlashes(std::string(osdValue.ToUTF8().data()));

	std::string errorMessage;
	if (!project->SetSliderDataExternal(dataEntries, dataFolders, osdFileName, &errorMessage)) {
		wxMessageBox(wxString::FromUTF8(errorMessage), _("Slider Data"), wxOK | wxICON_WARNING, parent);
		return false;
	}

	return true;
}

SliderDataLocationsDialog::SliderDataLocationsDialog(OutfitStudioFrame* owner, OutfitProject* project)
	: wxDialog(owner, wxID_ANY, _("Slider Data"), wxDefaultPosition, wxSize(1200, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, owner(owner)
	, project(project) {
	SetMinSize(wxSize(980, 440));

	auto* mainSizer = new wxBoxSizer(wxVERTICAL);
	auto* filterSizer = new wxBoxSizer(wxHORIZONTAL);

	auto* filterLabel = new wxStaticText(this, wxID_ANY, _("Filter:"));
	filterText = new wxTextCtrl(this, wxID_ANY);
	auto* sourceLabel = new wxStaticText(this, wxID_ANY, _("Source:"));
	sourceChoice = new wxChoice(this, wxID_ANY);
	sourceChoice->Append(_("All"));
	sourceChoice->Append(_("Local"));
	sourceChoice->Append(_("External"));
	sourceChoice->SetSelection(0);
	auto* statusLabel = new wxStaticText(this, wxID_ANY, _("Status:"));
	statusChoice = new wxChoice(this, wxID_ANY);
	statusChoice->Append(_("All"));
	statusChoice->Append(_("Found"));
	statusChoice->Append(_("Missing"));
	statusChoice->SetSelection(0);

	filterSizer->Add(filterLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	filterSizer->Add(filterText, 1, wxRIGHT | wxEXPAND, 10);
	filterSizer->Add(sourceLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	filterSizer->Add(sourceChoice, 0, wxRIGHT, 10);
	filterSizer->Add(statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	filterSizer->Add(statusChoice, 0);
	mainSizer->Add(filterSizer, 0, wxALL | wxEXPAND, 8);

	sliderDataList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_HRULES | wxLC_VRULES);
	SliderDataList::Configure(sliderDataList, true);
	mainSizer->Add(sliderDataList, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

	auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	btnMakeLocal = new wxButton(this, wxID_ANY, _("Make Local"));
	btnMakeExternal = new wxButton(this, wxID_ANY, _("Make External"));
	btnEditFolders = new wxButton(this, wxID_ANY, _("Edit Folders..."));
	auto* btnClose = new wxButton(this, wxID_CLOSE, _("Close"));
	buttonSizer->Add(btnMakeLocal, 0, wxRIGHT, 5);
	buttonSizer->Add(btnMakeExternal, 0, wxRIGHT, 5);
	buttonSizer->Add(btnEditFolders, 0, wxRIGHT, 5);
	buttonSizer->AddStretchSpacer(1);
	buttonSizer->Add(btnClose, 0);
	mainSizer->Add(buttonSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 8);

	SetSizer(mainSizer);

	filterText->Bind(wxEVT_TEXT, [&](wxCommandEvent&) { ApplyFilters(); });
	sourceChoice->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) { ApplyFilters(); });
	statusChoice->Bind(wxEVT_CHOICE, [&](wxCommandEvent&) { ApplyFilters(); });
	sliderDataList->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent&) { UpdateButtons(); });
	sliderDataList->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent&) { UpdateButtons(); });
	SliderDataList::BindSelectAll(this, sliderDataList, [this]() { UpdateButtons(); });
	btnMakeLocal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
		std::vector<SliderDataLocation> locations;
		SliderDataList::GetSelected(sliderDataList, rows, locations);
		if (SliderDataList::MakeLocal(this, project, locations))
			MarkChangedAndRefresh();
	});
	btnMakeExternal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
		std::vector<SliderDataLocation> locations;
		SliderDataList::GetSelected(sliderDataList, rows, locations);
		if (SliderDataList::EditFolders(this, project, locations))
			MarkChangedAndRefresh();
	});
	btnEditFolders->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
		std::vector<SliderDataLocation> locations;
		SliderDataList::GetSelected(sliderDataList, rows, locations);
		if (SliderDataList::EditFolders(this, project, locations))
			MarkChangedAndRefresh();
	});
	btnClose->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { EndModal(wxID_CLOSE); });

	RefreshRows();
	CenterOnParent();
}

void SliderDataLocationsDialog::UpdateButtons() {
	std::vector<SliderDataLocation> locations;
	size_t selectionCount = SliderDataList::GetSelected(sliderDataList, rows, locations);
	bool allLocal = SliderDataList::AllHaveSource(locations, true);
	bool allExternal = SliderDataList::AllHaveSource(locations, false);
	btnMakeLocal->Enable(selectionCount > 0 && allExternal);
	btnMakeExternal->Enable(selectionCount > 0 && allLocal);
	btnEditFolders->Enable(selectionCount > 0 && allExternal);
}

void SliderDataLocationsDialog::ApplyFilters() {
	visibleRows.clear();
	wxString filter = filterText->GetValue().Lower();
	int sourceFilter = sourceChoice->GetSelection();
	int statusFilter = statusChoice->GetSelection();

	for (size_t rowIndex = 0; rowIndex < rows.size(); rowIndex++) {
		const auto& row = rows[rowIndex];
		if (sourceFilter == 1 && !row.local)
			continue;
		if (sourceFilter == 2 && row.local)
			continue;
		if (statusFilter == 1 && !row.resolved)
			continue;
		if (statusFilter == 2 && row.resolved)
			continue;

		if (!filter.IsEmpty()) {
			wxString haystack = wxString::FromUTF8(row.sliderName + " " + row.shapeName + " " + row.targetName + " " + row.dataName + " " + row.fileName);
			haystack += " ";
			haystack += SliderDataList::JoinDataFolders(row.dataFolders);
			if (!haystack.Lower().Contains(filter))
				continue;
		}

		visibleRows.push_back(rowIndex);
	}

	SliderDataList::Populate(sliderDataList, rows, &visibleRows, true);
	UpdateButtons();
}

void SliderDataLocationsDialog::RefreshRows() {
	if (project)
		project->GetSliderDataLocations(rows);
	else
		rows.clear();

	ApplyFilters();
}

void SliderDataLocationsDialog::MarkChangedAndRefresh() {
	if (owner)
		owner->SetPendingChanges();

	RefreshRows();
}