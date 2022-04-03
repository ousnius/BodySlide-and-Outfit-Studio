#pragma once
#include "../components/RefTemplates.h"
#include "ConfigDialogUtil.h"
#include <type_traits>
#include <vector>

#include "ConfigurationManager.h"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/xrc/xmlres.h>

namespace ConfigDialogUtil {
inline std::string GetConfigKey(const char* configPrefix, const char* dlgProperty) {
	std::string configKey = configPrefix;
	configKey += "_";
	configKey += dlgProperty;
	return configKey;
}

inline void LoadDialogChoice(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxChoice* choice = XRCCTRL(dlg, dlgProperty, wxChoice);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	const std::string& lastValue = configManager[configKey];
	if (!choice->SetStringSelection(wxString::FromUTF8(lastValue))) {
		if (choice->GetSelection() < 0)
			choice->Select(0);
	}
}

inline void LoadDialogChoiceIndex(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxChoice* choice = XRCCTRL(dlg, dlgProperty, wxChoice);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	int lastSel = configManager.GetIntValue(configKey, choice->GetSelection());
	choice->SetSelection(lastSel);
}

template<typename T, std::enable_if_t<std::is_base_of_v<NamedValue, T>, bool> = true>
void LoadDialogChoices(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty, const std::vector<T> possibleChoices) {
	wxChoice* choice = XRCCTRL(dlg, dlgProperty, wxChoice);
	for (auto& entry : possibleChoices)
		choice->Append(entry.GetName());

	LoadDialogChoice(configManager, dlg, configPrefix, dlgProperty);
}

inline void LoadDialogText(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxTextCtrl* textCtrl = XRCCTRL(dlg, dlgProperty, wxTextCtrl);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	if (configManager.Exists(configKey)) {
		std::string lastValue = configManager[configKey];
		textCtrl->SetValue(lastValue);
	}
}

inline void LoadDialogTextInteger(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxTextCtrl* textCtrl = XRCCTRL(dlg, dlgProperty, wxTextCtrl);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	if (configManager.Exists(configKey)) {
		int lastValue = configManager.GetIntValue(configKey);
		textCtrl->SetValue(wxString::Format("%d", lastValue));
	}
}

inline void LoadDialogTextFloat(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxTextCtrl* textCtrl = XRCCTRL(dlg, dlgProperty, wxTextCtrl);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	if (configManager.Exists(configKey)) {
		float lastValue = configManager.GetFloatValue(configKey);
		textCtrl->SetValue(wxString::Format("%f", lastValue));
	}
}

inline void LoadDialogCheckBox(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	wxCheckBox* check = XRCCTRL(dlg, dlgProperty, wxCheckBox);

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	bool lastValue = configManager.GetBoolValue(configKey, check->GetValue());
	check->SetValue(lastValue);
}

inline bool SetBoolFromDialogCheckbox(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	const bool value = (XRCCTRL(dlg, dlgProperty, wxCheckBox)->IsChecked());

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetBoolValue(configKey, value);
	return value;
}

inline int SetIntegerFromDialogChoice(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	int sel = XRCCTRL(dlg, dlgProperty, wxChoice)->GetSelection();

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetValue(configKey, sel);
	return sel;
}

inline wxString SetStringFromDialogChoice(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	const wxString str = XRCCTRL(dlg, dlgProperty, wxChoice)->GetStringSelection();

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetValue(configKey, str.ToStdString());
	return str;
}

inline int SetIntegerFromDialogTextControl(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	int val = std::atoi(XRCCTRL(dlg, dlgProperty, wxTextCtrl)->GetValue().c_str());

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetValue(configKey, val);
	return val;
}

inline float SetFloatFromDialogTextControl(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	float val = std::atof(XRCCTRL(dlg, dlgProperty, wxTextCtrl)->GetValue().c_str());

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetValue(configKey, val);
	return val;
}

inline wxString SetStringFromDialogTextControl(ConfigurationManager& configManager, const wxDialog& dlg, const char* configPrefix, const char* dlgProperty) {
	const wxString str = XRCCTRL(dlg, dlgProperty, wxTextCtrl)->GetValue();

	std::string configKey = GetConfigKey(configPrefix, dlgProperty);
	configManager.SetValue(configKey, str.ToStdString());
	return str;
}
}; // namespace ConfigDialogUtil
