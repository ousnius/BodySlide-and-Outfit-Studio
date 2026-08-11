/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Decodes an OpenEXR image into interleaved half-float RGBA, the layout GL_RGBA16F expects, so the
//  result uploads without a second conversion. Values are the linear radiance the file stores; no
//  exposure or tone mapping is applied here.
// Returns false and fills outError when the file can't be read, which includes builds made without
//  OpenEXR support. Half is enough for an environment map at the sizes these come in, and it halves
//  what has to cross to the GPU compared to keeping the float the file may have been written in.
bool LoadEXRImage(const std::string& fileName, std::vector<uint16_t>& outRGBA, int& outWidth, int& outHeight, std::string& outError);
