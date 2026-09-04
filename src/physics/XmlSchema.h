/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <Object3d.hpp>

#include <cstddef>
#include <limits>
#include <string>
#include <variant>
#include <vector>

/*
Declarative description of the HDT-SMP physics XML format (hdtSMP64.xsd, FSMP
2.2.0), used by the physics editor to build its property grid, to validate a
document and to decide whether a change can be pushed into the running
simulation or needs it rebuilt.

Deliberately free of wxWidgets and of Bullet, and compiled whether or not
USE_BULLET is defined, so it can be unit tested on its own.

The defaults recorded here are the ones the simulation actually starts from,
i.e. the member initializers of the template structs in SystemBuilder.h, which
agree with the defaults the XSD documents. PhysicsSchemaTest keeps the two in
step.
*/
namespace Physics {
// One kind of element the editor knows how to show. Elements outside this list
// (and unknown ones) are still kept in the document; they just get no
// generated page.
enum class ElementKind {
	Unknown,
	System,
	Bone,
	BoneDefault,
	PerVertexShape,
	PerVertexShapeDefault,
	PerTriangleShape,
	PerTriangleShapeDefault,
	GenericConstraint,
	GenericConstraintDefault,
	StiffSpringConstraint,
	StiffSpringConstraintDefault,
	ConeTwistConstraint,
	ConeTwistConstraintDefault,
	ConstraintGroup,
	Shape,
	WeightThreshold,
	ShapeChild,
	Pattern,
	PatternDefault,
	Repeat,
	Param,
	PatternBody,
	Patterns,
};

// How a value is spelled in the XML and which editor it deserves. The Ref
// kinds are strings that name something else, and get a picker instead of a
// text field.
enum class ValueKind {
	None,		// element carries no value of its own
	Float,		// flexFloat
	Factor,		// flexFloat documented as [0, 1]
	PosFloat,	// flexFloat documented as >= 0
	Int,
	Bool,
	String,
	Vec3,
	Quaternion,
	AxisAngle,
	Transform,	// choice of basis / basis-axis-angle / origin
	Lerp,		// translationLerp + rotationLerp
	Enum,
	BoneRef,	// names a <bone> (or a skeleton node)
	NiShapeRef, // names a NiShape of the NIF
	ShapeRef,	// names a top level <shape>
	TagRef,		// names a shape tag
};

// Whether the element may appear once or repeatedly under its owner. Note that
// the XSD models bone/bone-default as xsd:all, which caps every child at one,
// while both the game and the port accumulate can-/no-collide-with-bone into
// lists. List wins here: the editor has to be able to read what exists.
enum class Multiplicity { Once, List };

// How far a property gets in this preview.
enum class PreviewSupport {
	Simulated,		  // read by SystemBuilder and acted on
	ParsedNotApplied, // read, then deliberately dropped (see the notes)
	NotParsed,		  // SystemBuilder warns "unknown element"
};

// What changing a property costs.
enum class PatchKind {
	Hot,		// can be written straight into the live simulation
	Structural, // needs the system rebuilt from the XML
};

constexpr float kUnbounded = std::numeric_limits<float>::infinity();

struct AxisAngleValue {
	nifly::Vector3 axis = nifly::Vector3(1.0f, 0.0f, 0.0f);
	float angle = 0.0f;
};

struct LerpValue {
	float translation = 0.0f;
	float rotation = 0.0f;
};

// A property value as the editor passes it around. std::monostate means "the
// element is absent", which is not the same as "present and zero": absent
// means the ambient *-default cascade decides.
using ValueVariant = std::variant<std::monostate,
								  bool,
								  int,
								  float,
								  std::string,
								  nifly::Vector3,
								  nifly::Quaternion,
								  AxisAngleValue,
								  LerpValue,
								  nifly::MatTransform,
								  std::vector<std::string>>;

struct EnumValues {
	const char* const* values = nullptr;
	std::size_t count = 0;
};

// One child element of an editable element.
struct ChildDesc {
	const char* name = nullptr;
	// '|' separated alternate spellings the parser accepts for the same thing
	// (conetwist-constraint has three names for each of its spans), or null.
	const char* aliases = nullptr;
	ValueKind kind = ValueKind::Float;
	Multiplicity mult = Multiplicity::Once;
	// The value used when the element is absent and nothing in the cascade
	// supplies one, spelled as it would appear in the XML. Null when the
	// property has no meaningful default.
	const char* defaultText = nullptr;
	float rangeMin = -kUnbounded;
	float rangeMax = kUnbounded;
	// Unit for the grid to show, e.g. "rad", "units", "kg". Null for
	// dimensionless values.
	const char* unit = nullptr;
	PreviewSupport preview = PreviewSupport::Simulated;
	PatchKind patch = PatchKind::Structural;
	const EnumValues* enumValues = nullptr;
	const char* help = nullptr;
};

struct AttrDesc {
	const char* name = nullptr;
	ValueKind kind = ValueKind::String;
	bool required = false;
	const EnumValues* enumValues = nullptr;
	const char* help = nullptr;
};

struct ElementDesc {
	const char* name = nullptr;
	ElementKind kind = ElementKind::Unknown;
	const AttrDesc* attrs = nullptr;
	std::size_t attrCount = 0;
	const ChildDesc* children = nullptr;
	std::size_t childCount = 0;
	const char* help = nullptr;

	const AttrDesc* Attr(const char* attrName) const;
	// Resolves aliases, so FindChild(coneTwist, "limitZ") returns swingSpan1.
	const ChildDesc* Child(const char* childName) const;
};

// Every element the schema table describes, in a stable order.
const std::vector<const ElementDesc*>& AllElements();

const ElementDesc* FindElement(const char* name);
const ElementDesc* FindElement(ElementKind kind);

ElementKind KindFromName(const char* name);
const char* NameFromKind(ElementKind kind);

// True for the *-default elements, which declare templates rather than objects.
bool IsDefaultKind(ElementKind kind);
// BoneDefault -> Bone, Bone -> Bone, ConstraintGroup -> ConstraintGroup.
ElementKind InstanceKindOf(ElementKind kind);
// Bone -> BoneDefault; Unknown for kinds with no template form.
ElementKind DefaultKindOf(ElementKind kind);

// True when this preview implements the *-default cascade for a kind. The
// system builder reads bone-default and the three *-constraint-default
// elements and nothing else, so a per-*-shape template resolves to nothing
// here however the XSD reads.
bool CascadeApplies(ElementKind instanceKind);

// True for the elements a <system> may contain directly.
bool IsSystemChildKind(ElementKind kind);

// Human readable one-liners for the UI.
const char* PreviewSupportNote(PreviewSupport support);
}
