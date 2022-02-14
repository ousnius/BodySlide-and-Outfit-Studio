/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <memory>
#include <vector>

struct UndoStateProject;

class UndoHistory {
	static constexpr int UH_MAX_UNDO = 40;

	// Note that -1 on the next line is NOT a magic number.  It's the actual
	// index.  The same is true everywhere in UndoHistory.
	int curIndex = -1;
	std::vector<std::unique_ptr<UndoStateProject>> states;

public:
	bool PopState();
	UndoStateProject* PushState(std::unique_ptr<UndoStateProject> uspp = std::make_unique<UndoStateProject>());
	bool BackStepHistory();
	bool ForwardStepHistory();
	void ClearHistory();

	bool CanUndo() const { return curIndex != -1; }

	bool CanRedo() const { return !states.empty() && curIndex + 1 < static_cast<int>(states.size()); }

	UndoStateProject* GetCurState() const {
		if (curIndex == -1)
			return nullptr;
		return states[curIndex].get();
	}

	UndoStateProject* GetBackState() const {
		if (states.empty())
			return nullptr;
		return states.back().get();
	}

	UndoStateProject* GetNextState() const {
		if (curIndex + 1 >= static_cast<int>(states.size()))
			return nullptr;
		return states[curIndex + 1].get();
	}
};
