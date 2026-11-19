/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "FrameHelpers.h"

#include <algorithm>
#include <wx/display.h>
#include <wx/scrolbar.h>
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

std::array<wxEventTypeTag<wxScrollEvent>, 9> CommandScrollEventTypes() {
	return {wxEVT_SCROLL_TOP,
			wxEVT_SCROLL_BOTTOM,
			wxEVT_SCROLL_LINEUP,
			wxEVT_SCROLL_LINEDOWN,
			wxEVT_SCROLL_PAGEUP,
			wxEVT_SCROLL_PAGEDOWN,
			wxEVT_SCROLL_THUMBTRACK,
			wxEVT_SCROLL_THUMBRELEASE,
			wxEVT_SCROLL_CHANGED};
}

int GetTargetFps(wxWindow* window) {
	int refresh = 0;

	const int displayIndex = wxDisplay::GetFromWindow(window);
	if (displayIndex != wxNOT_FOUND) {
		wxVideoMode mode = wxDisplay(static_cast<unsigned int>(displayIndex)).GetCurrentMode();
		refresh = mode.refresh;
	}

	if (refresh <= 0)
		refresh = 60;

	return std::min(refresh, 120);
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
