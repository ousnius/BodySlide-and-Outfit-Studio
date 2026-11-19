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

const std::array<wxLanguage, 37> SupportedLangs = {wxLANGUAGE_ENGLISH,	  wxLANGUAGE_AFRIKAANS,		   wxLANGUAGE_ARABIC,  wxLANGUAGE_CATALAN,	  wxLANGUAGE_CZECH,
												   wxLANGUAGE_DANISH,	  wxLANGUAGE_GERMAN,		   wxLANGUAGE_GREEK,   wxLANGUAGE_SPANISH,	  wxLANGUAGE_BASQUE,
												   wxLANGUAGE_FINNISH,	  wxLANGUAGE_FRENCH,		   wxLANGUAGE_HINDI,   wxLANGUAGE_HUNGARIAN,  wxLANGUAGE_INDONESIAN,
												   wxLANGUAGE_ITALIAN,	  wxLANGUAGE_JAPANESE,		   wxLANGUAGE_KOREAN,  wxLANGUAGE_LITHUANIAN, wxLANGUAGE_LATVIAN,
												   wxLANGUAGE_MALAY,	  wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI,  wxLANGUAGE_DUTCH,	  wxLANGUAGE_POLISH,
												   wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN,		   wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK,	  wxLANGUAGE_SLOVENIAN,
												   wxLANGUAGE_ALBANIAN,	  wxLANGUAGE_SWEDISH,		   wxLANGUAGE_TAMIL,   wxLANGUAGE_TURKISH,	  wxLANGUAGE_UKRAINIAN,
												   wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE};

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
