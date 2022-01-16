#pragma once
#include <vector>
#include <type_traits>
#include "ConfigDialogUtil.h"
#include "../components/RefTemplates.h"

#include "ConfigurationManager.h"
#include <wx/dialog.h>
#include <wx/xrc/xmlres.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>

namespace ConfigDialogUtil {
	template<typename T, std::enable_if_t<std::is_base_of_v<NamedValue, T>, bool> = true>
	void LoadDialogChoices(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty, const std::vector<T> possibleChoices) {
		wxChoice* choice = XRCCTRL(dlg, dlgProperty, wxChoice);
		for (auto& tmpl : possibleChoices)
			choice->Append(tmpl.GetName());

		const std::string& lastValue = configManager[dlgProperty];
		if (!choice->SetStringSelection(wxString::FromUTF8(lastValue)))
			choice->Select(0);
	}

	inline void LoadDialogText(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty) {
		wxTextCtrl* tmplChoice = XRCCTRL(dlg, dlgProperty, wxTextCtrl);
		const std::string& lastValue = configManager[dlgProperty];
		tmplChoice->SetValue(lastValue);
	}

	inline void LoadDialogCheckBox(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty) {
		wxCheckBox* tmplChoice = XRCCTRL(dlg, dlgProperty, wxCheckBox);
		bool lastValue = configManager.GetBoolValue(dlgProperty);
		tmplChoice->SetValue(lastValue);
	}

	inline bool SetBoolFromDialogCheckbox(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty)
	{
		const bool value = (XRCCTRL(dlg, dlgProperty, wxCheckBox)->IsChecked());
		configManager.SetBoolValue(dlgProperty, value);
		return value;
	}

	inline wxString SetStringFromDialogChoice(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty)
	{
		const wxString str = XRCCTRL(dlg, dlgProperty, wxChoice)->GetStringSelection();
		configManager.SetValue(dlgProperty, str.ToStdString());
		return str;
	}

	inline wxString SetStringFromDialogTextControl(ConfigurationManager& configManager, const wxDialog& dlg, const char* dlgProperty)
	{
		const wxString str = XRCCTRL(dlg, dlgProperty, wxTextCtrl)->GetValue();
		configManager.SetValue(dlgProperty, str.ToStdString());
		return str;
	}
};
