/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "StackTrace.h"

#include <wx/log.h>
#include <wx/stackwalk.h>

namespace {
constexpr size_t MaxStackDepth = 64;

#if wxUSE_STACKWALKER
class LogStackWalker : public wxStackWalker {
protected:
	void OnStackFrame(const wxStackFrame& frame) override {
		wxString message = wxString::Format("  #%lu", static_cast<unsigned long>(frame.GetLevel()));

		wxString name = frame.GetName();
		if (!name.empty())
			message += " " + name;
		else
			message += " <unknown>";

		void* address = frame.GetAddress();
		if (address)
			message += wxString::Format(" [%p]", address);

		wxString module = frame.GetModule();
		if (!module.empty())
			message += " in " + module;

		if (frame.HasSourceLocation())
			message += wxString::Format(" at %s:%lu", frame.GetFileName(), static_cast<unsigned long>(frame.GetLine()));

		wxLogError("%s", message);
	}
};
#endif
}

void LogStackTraceFromException() {
#if wxUSE_STACKWALKER
	wxLogError("Stack trace:");

	LogStackWalker walker;
#if wxUSE_ON_FATAL_EXCEPTION
	walker.WalkFromException(MaxStackDepth);
#else
	walker.Walk(1, MaxStackDepth);
#endif
#else
	wxLogError("Stack trace unavailable: wxWidgets was built without wxStackWalker.");
#endif
}