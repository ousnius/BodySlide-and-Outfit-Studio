/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "UndoHistory.h"
#include "UndoState.h"

void UndoHistory::ClearHistory() {
	states.clear();
	curIndex = -1;
}

UndoStateProject *UndoHistory::PushState(std::unique_ptr<UndoStateProject> uspp) {
	int nStrokes = states.size();
	if (curIndex + 1 < nStrokes)
		states.resize(curIndex + 1);
	else if (nStrokes == UH_MAX_UNDO)
		states.erase(states.begin());
	states.push_back(std::move(uspp));
	curIndex = states.size() - 1;
	return states.back().get();
}

bool UndoHistory::BackStepHistory() {
	if (curIndex > -1) {
		curIndex--;
		return true;
	}
	return false;
}

bool UndoHistory::ForwardStepHistory() {
	int nStrokes = states.size();
	if (curIndex < nStrokes - 1) {
		++curIndex;
		return true;
	}
	return false;
}
