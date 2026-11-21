/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "XmlDocument.h"

#include <string>
#include <vector>

namespace tinyxml2 {
class XMLElement;
}

/*
The *-default elements of a physics XML seen as profiles: named sets of values that bones and constraints start
from, which their own values override.

In the file they are order-dependent. An unnamed default replaces the ambient set for everything after it, a named
one is a snapshot of what it @extends at the point it is declared, and an element reads its @template, or the
ambient set, at its own position. A profile is one *-default, and the elements using it are the ones whose template
chain contains it (XmlDocument::TemplateChain), which hides that bookkeeping.

TidyDefaults rewrites a file so that it no longer depends on the order: flat named profiles at the top, an unnamed
"Default" for the ambient set, and a @template on every element that uses another profile.

Only bone-default and the three *-constraint-default elements are profiles; this preview doesn't read the per-*-shape
defaults (see CascadeApplies).
*/
namespace Physics {
// The kinds of the *-defaults that are profiles: bone, generic, stiffspring and conetwist
const std::vector<ElementKind>& ProfileKinds();

// Every profile in reading order, those inside constraint groups included
std::vector<tinyxml2::XMLElement*> Profiles(const XmlDocument& doc);

// The profile a bone or constraint starts from: the last default of its template chain. Null when it starts from the
// schema defaults.
tinyxml2::XMLElement* ProfileOf(const XmlDocument& doc, const tinyxml2::XMLElement* element);

// The bones and constraints whose template chain contains "profile": everything a change to it reaches
std::vector<tinyxml2::XMLElement*> ProfileUsers(const XmlDocument& doc, const tinyxml2::XMLElement* profile);

// Whether what "profile" applies to depends on where it is in the file: it is unnamed, and another unnamed default of
// its kind exists or an element reading it comes before it. Such a profile can't be picked by name.
bool IsPositional(const XmlDocument& doc, const tinyxml2::XMLElement* profile);

// Whether TidyDefaults would change the layout: positional profiles, @extends chains, defaults inside constraint
// groups, names declared twice, or templates naming nothing declared before them
bool NeedsTidy(const XmlDocument& doc);

// Moves the element's own value of "desc" onto its profile, as one undo step. False when the element has no value of
// its own or no profile.
bool ApplyToProfile(XmlDocument& doc, tinyxml2::XMLElement* element, const ChildDesc& desc);

// Creates a profile named "name" from the element's own values, on top of the profile it had, and points the element
// at it, as one undo step. The element keeps every value it had. Null when the name is empty or a profile of the kind
// already has it.
tinyxml2::XMLElement* CreateProfileFrom(XmlDocument& doc, tinyxml2::XMLElement* element, const std::string& name);

struct TidyResult {
	bool ok = false;
	// What changed, or why nothing did
	std::string report;
	// *-defaults before and profiles after
	int defaultsBefore = 0;
	int profilesAfter = 0;
};

// Rewrites the *-defaults into profiles that don't depend on the order of the file, as one undo step. Every bone,
// constraint and implicitly created bone keeps exactly the values it had, which is verified before anything changes.
// Refuses when bones that shapes or constraints create without a <bone> would start from different defaults at
// different places, which only declaring those bones can preserve.
TidyResult TidyDefaults(XmlDocument& doc);
} // namespace Physics
