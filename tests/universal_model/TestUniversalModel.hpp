/*
BodySlide and Outfit Studio - Universal Model Tests
*/

#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cmath>
#include <algorithm>

// Simple test framework
#define REQUIRE(condition) do { if (!(condition)) { std::cerr << __FILE__ << ':' << __LINE__ <<: FAILED: Requirement not met: #condition\n'; return false; } } while(0)
#define CHECK_CLOSE(a, b, eps) do { if (std::abs((a) - (b)) > (eps)) { std::cerr << __FILE__ << ':' << __LINE__ << FAILED: ' << a << \" != \" << b << \" (diff=\" << std::abs((a)-(b)) << \")\n'; return false; } } while(0)

#include <UniversalModel.h>

namespace univmodel {
namespace test {

// Test helper functions
bool TestVertexCreation();
bool TestTriangleCreation();
bool TestMeshBounds();
bool TestMeshApplyWeld();
bool TestFormatRegistry();
bool TestFormatInfo();
bool TestMeshUtilsMerge();
bool TestMeshUtilsSplit();
bool TestSkeletonCreation();
bool TestDepthMapAccess();
bool TestImageDataValidation();
bool TestMeshReconstruction();

} // namespace test
} // namespace univmodel