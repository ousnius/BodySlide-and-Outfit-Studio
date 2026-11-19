/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "files/HkxFile.h"
#include "utils/TargetGame.h"

#include <array>
#include <wx/event.h>
#include <wx/intl.h>
#include <wx/string.h>

class wxWindow;

// Helpers the Outfit Studio window files share

// Windows quantises waits to the system clock tick, ~15.6 ms by default, which
// caps anything paced by sleeping at 64 Hz. Raising the resolution for the
// duration of playback is what lets AnimationPlayer::OnIdle reach the display's refresh rate.
// Since Windows 10 2004 this affects the calling process, not the whole system.
void SetHighResolutionTimers(bool enable);

// The HKX format poses and animations of the target game use. False if the game has none.
bool GetPoseHkxFormat(TargetGame targetGame, HKX::Format* outFormat = nullptr);

// The event types that EVT_COMMAND_SCROLL handles, for binding them. A function, because wx
// creates the event types at run time.
std::array<wxEventTypeTag<wxScrollEvent>, 9> CommandScrollEventTypes();

// Refresh rate of the display the window is on, capped at 120 and falling
// back to 60 when the display does not report one.
int GetTargetFps(wxWindow* window);

// Same as wxGetTextFromUser, but with the text cursor at the end of the default value
wxString GetTextFromUser(const wxString& message, const wxString& caption, const wxString& defaultValue, wxWindow* parent);
