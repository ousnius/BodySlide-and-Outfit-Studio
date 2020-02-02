/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <vector>
#include <memory>

struct UndoStateProject;

class UndoHistory {
	static constexpr int UH_MAX_UNDO = 40;

	int curIndex = -1;
	std::vector<std::unique_ptr<UndoStateProject>> states;

public:
	UndoStateProject *PushState(std::unique_ptr<UndoStateProject> uspp = std::make_unique<UndoStateProject>());
	bool BackStepHistory();
	bool ForwardStepHistory();
	void ClearHistory();

	UndoStateProject *GetCurState() {
		if (curIndex == -1)
			return nullptr;
		return states[curIndex].get();
	}

	UndoStateProject *GetNextState() {
		if (curIndex + 1 >= static_cast<int>(states.size()))
			return nullptr;
		return states[curIndex + 1].get();
	}
};
