/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "ProjectHost.h"

#include <wx/log.h>

void NonInteractiveHost::RecordMessage(HostMessageLevel level, const wxString& msg, const wxString& title) {
	switch (level) {
		case HostMessageLevel::Error: wxLogError("%s: %s", title, msg); break;
		case HostMessageLevel::Warning: wxLogWarning("%s: %s", title, msg); break;
		default: wxLogMessage("%s: %s", title, msg); break;
	}

	messages.push_back({level, title.ToUTF8().data(), msg.ToUTF8().data()});
}

void NonInteractiveHost::ShowMessage(HostMessageLevel level, const wxString& msg, const wxString& title) {
	RecordMessage(level, msg, title);
}

HostAnswer NonInteractiveHost::Ask(const wxString& msg, const wxString& title, bool WXUNUSED(withCancel), HostAnswer defaultAnswer) {
	wxString answer = defaultAnswer == HostAnswer::Yes ? "yes" : (defaultAnswer == HostAnswer::No ? "no" : "cancel");
	RecordMessage(HostMessageLevel::Info, wxString::Format("%s [answered: %s]", msg, answer), title);
	return defaultAnswer;
}

bool NonInteractiveHost::ChooseMany(const wxString& msg, const wxString& title, const wxArrayString& WXUNUSED(choices), wxArrayInt& selections) {
	RecordMessage(HostMessageLevel::Info, wxString::Format("%s [kept %zu preselected]", msg, selections.GetCount()), title);
	return true;
}

bool NonInteractiveHost::ChooseOne(const wxString& msg, const wxString& title, const wxArrayString& choices, int& selection) {
	if (selection < 0 || selection >= static_cast<int>(choices.GetCount())) {
		RecordMessage(HostMessageLevel::Warning, wxString::Format("%s [no default choice]", msg), title);
		return false;
	}

	RecordMessage(HostMessageLevel::Info, wxString::Format("%s [chose: %s]", msg, choices[selection]), title);
	return true;
}

bool NonInteractiveHost::GetText(const wxString& msg, const wxString& title, wxString& value) {
	RecordMessage(HostMessageLevel::Info, wxString::Format("%s [answered: %s]", msg, value), title);
	return !value.empty();
}
