/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "FrameHelpers.h"

#include <wx/textdlg.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>

#include <mmsystem.h>
#endif


void SetHighResolutionTimers(bool enable) {
#ifdef __WXMSW__
	if (enable)
		timeBeginPeriod(1);
	else
		timeEndPeriod(1);
#else
	(void)enable;
#endif
}

bool GetPoseHkxFormat(TargetGame targetGame, HKX::Format* outFormat) {
	HKX::Format format = HKX::Format::Unknown;
	switch (targetGame) {
		case SKYRIM: format = HKX::Format::Skyrim32; break;
		case SKYRIMSE:
		case SKYRIMVR: format = HKX::Format::Skyrim64; break;
		case FO4:
		case FO4VR: format = HKX::Format::Fallout64; break;
		default: return false;
	}

	if (outFormat)
		*outFormat = format;
	return true;
}

namespace {

// wxTextEntryDialog sets its value on init, which puts the text cursor at the start on MSW
class TextEntryDialogCursorAtEnd : public wxTextEntryDialog {
public:
	using wxTextEntryDialog::wxTextEntryDialog;

	bool TransferDataToWindow() override {
		bool result = wxTextEntryDialog::TransferDataToWindow();
		if (m_textctrl)
			m_textctrl->SetInsertionPointEnd();
		return result;
	}
};

} // namespace

// Same as wxGetTextFromUser, but with the text cursor at the end of the default value
wxString GetTextFromUser(const wxString& message, const wxString& caption, const wxString& defaultValue, wxWindow* parent) {
	TextEntryDialogCursorAtEnd dialog(parent, message, caption, defaultValue);
	if (dialog.ShowModal() != wxID_OK)
		return wxEmptyString;

	return dialog.GetValue();
}
