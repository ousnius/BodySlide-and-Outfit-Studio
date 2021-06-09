/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <vector>
#include <memory>

struct UndoStateProject;

class UndoHistory {
	static constexpr uint32_t UH_NONE = 0xFFFFFFFF;
	static constexpr uint32_t UH_MAX_UNDO = 40;

	uint32_t curIndex = UH_NONE;
	std::vector<std::unique_ptr<UndoStateProject>> states;

public:
	bool PopState();
	UndoStateProject *PushState(std::unique_ptr<UndoStateProject> uspp = std::make_unique<UndoStateProject>());
	bool BackStepHistory();
	bool ForwardStepHistory();
	void ClearHistory();
	
	bool CanUndo() const {
		return curIndex != UH_NONE;
	}

	bool CanRedo() const {
		return !states.empty() && curIndex + 1 < static_cast<int>(states.size());
	}

	UndoStateProject *GetCurState() const {
		if (curIndex == UH_NONE)
			return nullptr;
		return states[curIndex].get();
	}

	UndoStateProject *GetBackState() const {
		if (states.empty())
			return nullptr;
		return states.back().get();
	}

	UndoStateProject *GetNextState() const {
		if (curIndex + 1 >= static_cast<int>(states.size()))
			return nullptr;
		return states[curIndex + 1].get();
	}
};
