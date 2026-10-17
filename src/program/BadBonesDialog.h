/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "OutfitProject.h"

class wxWindow;

// Lets the user choose how to fix each bad bone. Returns false if nothing should be fixed.
bool ShowBadBonesDialog(wxWindow* parent, BadBonesReport& report, AnimInfo& anim);
