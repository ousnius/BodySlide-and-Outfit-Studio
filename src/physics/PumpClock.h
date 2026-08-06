/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <algorithm>

#include <wx/stopwatch.h>

namespace Physics {
// Timer interval driving the physics pump. Well below the frame period, so the
// clock below - not the timer resolution - decides the frame rate.
constexpr int PumpTimerIntervalMS = 15;

/*
Frame pacing for the physics preview, shared by both applications.

Ticks arrive from idle events, a timer and (in Outfit Studio) mouse motion, at
wildly varying rates - mouse movement floods the message queue and starves both
idle events and WM_TIMER, so the pump has to be called from there as well. This
decides which of those calls actually draws a frame and how much wall-clock time
the simulation has to advance by, so the source and rate of the calls do not
matter.
*/
class PumpClock {
public:
	// Begins (or restarts) timing. Call whenever the simulation starts or is
	// re-seated, so a pause does not turn into one huge step.
	void Reset(int targetFps = 60) {
		fps = std::max(targetFps, 1);
		watch.Start();
		lastStepMicro = 0;
		lastDrawMicro = 0;
	}

	// True when the next frame is due, with the seconds since the previous one
	// in "outSeconds". False means this call should do nothing.
	bool StepDue(float& outSeconds) {
		const wxLongLong now = watch.TimeInMicro();
		if (now - lastDrawMicro < Period())
			return false;

		outSeconds = static_cast<float>((now - lastStepMicro).ToDouble() / 1000000.0);
		lastStepMicro = now;
		lastDrawMicro = now;
		return true;
	}

	// Seconds since the last step, for callers driven by something else that
	// already paces itself (Outfit Studio's animation playback).
	float TakeElapsed() {
		const wxLongLong now = watch.TimeInMicro();
		const float seconds = static_cast<float>((now - lastStepMicro).ToDouble() / 1000000.0);
		lastStepMicro = now;
		return seconds;
	}

	// Microseconds until the next frame is due; negative when it is overdue.
	wxLongLong UntilDue() const { return Period() - (watch.TimeInMicro() - lastDrawMicro); }

private:
	wxLongLong Period() const { return 1000000 / fps; }

	wxStopWatch watch;
	wxLongLong lastStepMicro = 0;
	wxLongLong lastDrawMicro = 0;
	int fps = 60;
};
}
