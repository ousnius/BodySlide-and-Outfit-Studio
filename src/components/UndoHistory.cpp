/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "UndoHistory.h"
#include "UndoState.h"

void UndoHistory::ClearHistory() {
	states.clear();
	curIndex = UH_NONE;
}

bool UndoHistory::PopState() {
	if (states.empty())
		return false;

	if (curIndex + 1 == states.size())
		curIndex--;

	states.pop_back();
	return true;
}

UndoStateProject *UndoHistory::PushState(std::unique_ptr<UndoStateProject> uspp) {
	size_t nStrokes = states.size();
	if (curIndex + 1 < nStrokes)
		states.resize(curIndex + 1);
	else if (nStrokes == UH_MAX_UNDO)
		states.erase(states.begin());
	states.push_back(std::move(uspp));
	curIndex = static_cast<uint32_t>(states.size() - 1);
	return states.back().get();
}

bool UndoHistory::BackStepHistory() {
	if (curIndex != UH_NONE) {
		curIndex--;
		return true;
	}
	return false;
}

bool UndoHistory::ForwardStepHistory() {
	if (states.empty())
		return false;

	size_t nStrokes = states.size();
	if (curIndex == UH_NONE || curIndex < nStrokes - 1) {
		++curIndex;
		return true;
	}
	return false;
}
