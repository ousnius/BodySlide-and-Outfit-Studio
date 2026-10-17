/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../components/UndoHistory.h"

#include <string>
#include <unordered_set>

// Editing state that isn't part of the project data and doesn't belong to any widget.
struct EditorState {
	// Slider whose morph is being edited, and the one edited before it
	std::string activeSlider;
	std::string lastActiveSlider;
	bool bEditSlider = false;

	// Bone for weight painting, and its counterpart on the other side for mirrored painting
	std::string activeBone;
	std::string autoXMirrorBone;
	std::unordered_set<std::string> lastSelectedBones;
	std::unordered_set<std::string> lastNormalizeBones;

	UndoHistory undoHistory;
};
