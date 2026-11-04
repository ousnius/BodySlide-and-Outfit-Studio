/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <vector>

// Options of conforming slider data from the reference to other shapes. The defaults are those of the Conform dialog.
struct ConformOptions {
	float proximityRadius = 10.0f;
	int maxResults = 10;
	bool smoothResultDeltas = false;
	int smoothIterations = 2;
	float smoothStrength = 0.5f;
	bool noSqueeze = false;
	bool solidMode = false;
	bool axisX = true;
	bool axisY = true;
	bool axisZ = true;
	bool fixClipping = false;
	float fixClippingStrength = 0.5f;
	std::vector<std::string> sliderNames; // If empty, conform all non-zap/non-UV sliders
};
