/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// BodySlide: slider categories, groups and the outfit and preset filters

#include "BodySlideApp.h"
#include "BodySlideFrame.h"
#include "utils/StringStuff.h"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <regex>
#include <set>
#include <unordered_map>
#include <utility>
#include <wx/tokenzr.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

using namespace nifly;

void BodySlideApp::LoadAllCategories() {
	wxLogMessage("Loading all slider categories...");
	cCollection.Clear();
	cCollection.LoadCategories(ProjectUtil::GetProjectPath() + "/SliderCategories");
}

void BodySlideApp::SetPresetGroups(const std::string& setName) {
	presetGroups.clear();
	gCollection.GetOutfitGroups(setName, presetGroups);

	if (presetGroups.empty()) {
		Config.GetValueArray("DefaultGroups", "GroupName", presetGroups);
		if (!presetGroups.empty()) {
			wxString defaultGroups;
			for (auto& group : presetGroups)
				defaultGroups.Append("'" + group + "' ");

			wxLogMessage("Using default group(s): %s", defaultGroups);
		}
		else
			wxLogMessage("No group assigned for set '%s'.", setName);
	}
	else {
		wxString groups;
		for (auto& group : presetGroups)
			groups.Append("'" + group + "' ");

		wxLogMessage("Using group(s): %s", groups);
	}
}

void BodySlideApp::LoadAllGroups() {
	wxLogMessage("Loading all slider groups...");
	gCollection.LoadGroups(ProjectUtil::GetProjectPath() + "/SliderGroups");

	ungroupedOutfits.clear();
	for (auto& o : outfitNameSource) {
		std::vector<std::string> groups;
		gCollection.GetOutfitGroups(o.first, groups);
		if (groups.empty())
			ungroupedOutfits.push_back(o.first);
	}

	std::vector<std::string> aliases;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "alias", aliases);
	std::vector<std::string> groups;
	Config.GetValueAttributeArray("GroupAliases", "GroupAlias", "group", groups);

	if (aliases.size() == groups.size())
		for (size_t i = 0; i < aliases.size(); i++)
			groupAlias[aliases[i]] = groups[i];
}

void BodySlideApp::GetAllGroupNames(std::vector<std::string>& outGroups) {
	std::set<std::string> gNames;
	gCollection.GetAllGroups(gNames);
	outGroups.assign(gNames.begin(), gNames.end());
}

int BodySlideApp::SaveGroupList(const std::string& fileName, const std::string& groupName) {
	if (filteredOutfits.empty())
		return 1;

	std::vector<std::string> existing;
	SliderSetGroupFile outfile(fileName);
	if (outfile.GetError() != 0) {
		if (outfile.GetError() == 1)
			outfile.New(fileName);

		if (outfile.GetError() != 0) {
			wxMessageBox(_("Failed to create group file."), _("Error"), wxICON_ERROR);
			return 5;
		}
	}

	outfile.GetGroupNames(existing);
	bool found = false;
	for (auto& e : existing) {
		if (StringsEqualInsens(e.c_str(), groupName.c_str())) {
			found = true;
			break;
		}
	}
	if (found) {
		int ret = wxMessageBox(_("That group already exists in the specified file, do you wish to overwrite the group?"), _("Group already exists"), wxYES_NO | wxCANCEL);
		if (ret == wxNO)
			return 2;
		else if (ret == wxCANCEL)
			return 3;
	}

	SliderSetGroup ssg;
	ssg.SetName(groupName);
	ssg.AddMembers(filteredOutfits);

	outfile.UpdateGroup(ssg);
	outfile.Save();
	return 0;
}

void BodySlideApp::PopulateFilterData() {
	if (outfitHasZaps.empty())
		LoadSliderSets();
}

void BodySlideApp::ApplyOutfitFilter() {
	filteredOutfits.clear();

	std::unordered_set<std::string> grpFiltOutfits;
	std::vector<std::string> workFilterList;
	static wxString lastGrps = "";
	static std::set<std::string> grouplist;

	bool showUngrouped = false;
	bool regexFilterOutfits = false;
	bool filterHasZaps = false;
	bool filterBatchBuildSelected = false;

	auto menuOutfitSrchContext = sliderView->outfitsearch->GetMenu();
	if (menuOutfitSrchContext) {
		auto menuRegexOutfits = menuOutfitSrchContext->FindItem(XRCID("menuRegexOutfits"));
		if (menuRegexOutfits)
			regexFilterOutfits = menuRegexOutfits->IsChecked();

		auto menuFilterHasZaps = menuOutfitSrchContext->FindItem(XRCID("menuFilterHasZaps"));
		if (menuFilterHasZaps)
			filterHasZaps = menuFilterHasZaps->IsChecked();

		auto menuFilterOutputWinners = menuOutfitSrchContext->FindItem(XRCID("menuFilterOutputWinners"));
		if (menuFilterOutputWinners)
			filterBatchBuildSelected = menuFilterOutputWinners->IsChecked();
	}

	// Default winners of the conflicting outputs
	std::unordered_map<std::string, std::string> outputWinners;
	if (filterBatchBuildSelected) {
		BuildSelectionFile buildSelFile;
		BuildSelection buildSelection;
		GetBuildSelection(buildSelFile, buildSelection);

		std::string dataFolder = GetOutputDataPath();
		for (auto& outFile : outFileCount) {
			if (outFile.second.size() <= 1)
				continue;

			std::string choice = GetDefaultOutputChoice(buildSelection, outFile.first, dataFolder);
			if (!choice.empty())
				outputWinners[outFile.first] = choice;
		}
	}

	wxString grpSrch = sliderView->search->GetValue();
	std::string outfitSrch{sliderView->outfitsearch->GetValue()};

	if (lastGrps != grpSrch) {
		grouplist.clear();
		if (grpSrch.empty()) {
			gCollection.GetAllGroups(grouplist);
			showUngrouped = true;
		}
		else {
			wxStringTokenizer tokenizer(grpSrch, ",;");
			while (tokenizer.HasMoreTokens()) {
				wxString token = tokenizer.GetNextToken();
				token.Trim();
				token.Trim(false);
				std::string group = token.ToStdString();
				grouplist.insert(group);
			}
		}
		lastGrps = grpSrch;
	}
	else if (lastGrps.empty()) {
		gCollection.GetAllGroups(grouplist);
		showUngrouped = true;
	}

	for (auto& gn : grouplist) {
		if (gn == "Unassigned")
			showUngrouped = true;
		else
			gCollection.GetGroupMembers(gn, grpFiltOutfits);
	}

	if (showUngrouped)
		for (auto& ug : ungroupedOutfits)
			grpFiltOutfits.insert(ug);

	for (auto& no : outfitNameOrder) {
		bool filteredOut = false;

		if (grpFiltOutfits.find(no) == grpFiltOutfits.end())
			filteredOut = true;

		if (!filteredOut && filterHasZaps) {
			if (std::find(outfitHasZaps.cbegin(), outfitHasZaps.cend(), no) == outfitHasZaps.cend())
				filteredOut = true;
		}

		if (!filteredOut && filterBatchBuildSelected) {
			for (auto& winner : outputWinners) {
				if (winner.second == no)
					continue;

				auto& outfits = outFileCount[winner.first];
				if (std::find(outfits.cbegin(), outfits.cend(), no) != outfits.cend()) {
					filteredOut = true;
					break;
				}
			}
		}

		if (!filteredOut)
			workFilterList.push_back(no);
	}

	filteredOutfits = FilterOutfitNames(workFilterList, outfitSrch, regexFilterOutfits);

	SortOutfitNamesForDisplay(filteredOutfits);

	BodySlideConfig.SetValue("LastGroupFilter", grpSrch.ToUTF8().data());
	BodySlideConfig.SetValue("LastOutfitFilter", outfitSrch);
}

std::vector<std::string> BodySlideApp::FilterOutfitNames(const std::vector<std::string>& names, const std::string& filter, bool useRegex, std::string* regexError) const {
	if (filter.empty())
		return names;

	std::vector<std::string> matches;

	if (useRegex) {
		std::regex re;

		try {
			re.assign(filter, std::regex::icase);
		}
		catch (const std::regex_error& e) {
			if (regexError)
				*regexError = e.what();

			return matches;
		}

		for (auto& name : names)
			if (std::regex_search(name, re))
				matches.push_back(name);
	}
	else {
		wxString searchStr = wxString::FromUTF8(filter);
		searchStr.MakeLower();

		for (auto& name : names)
			if (wxString::FromUTF8(name).Lower().Contains(searchStr))
				matches.push_back(name);
	}

	return matches;
}

std::vector<std::string> BodySlideApp::ApplyPresetFilter(const std::vector<std::string>& presetNames) {
	wxString presetSearchStr = sliderView->presetFilter->GetValue();

	std::vector<std::string> filteredPresets;

	if (presetSearchStr.empty()) {
		for (auto& w : presetNames)
			filteredPresets.push_back(w);
	}
	else {
		presetSearchStr.MakeLower();

		for (auto& filterEntry : presetNames) {
			wxString entryStr = wxString::FromUTF8(filterEntry);
			if (entryStr.Lower().Contains(presetSearchStr))
				filteredPresets.push_back(entryStr.ToUTF8().data());
		}
	}

	SortPresetNamesForDisplay(filteredPresets);

	BodySlideConfig.SetValue("LastPresetFilter", presetSearchStr.ToUTF8().data());
	return filteredPresets;
}

int BodySlideApp::GetOutfits(std::vector<std::string>& outList) {
	outList.assign(outfitNameOrder.begin(), outfitNameOrder.end());
	return outList.size();
}

int BodySlideApp::GetFilteredOutfits(std::vector<std::string>& outList) {
	outList.assign(filteredOutfits.begin(), filteredOutfits.end());
	return outList.size();
}
