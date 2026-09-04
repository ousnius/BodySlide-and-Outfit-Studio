/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "XmlSchema.h"

#include <cstdint>
#include <deque>
#include <initializer_list>
#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
}

/*
One physics XML file, held open for editing as a tinyxml2 document.

The document is the source of truth: the editor mutates it in place and the
preview is rebuilt from what it serializes, so comments, unknown elements,
unknown attributes and the order everything appears in all survive a round
trip. Only whitespace is reflowed, because that is as much as tinyxml2's
printer preserves.

Order matters here beyond tidiness. An unnamed *-default sets the ambient
defaults for everything after it in the file, so the editor never rewrites a
document into a flat, default free form; every edit is a value change, or a
single insertion or removal, leaving every other element where it was.
*/
namespace Physics {
// Where the file was found, which decides whether saving can write back over
// it. Archive means it came out of a BSA/BA2 and there is nothing to overwrite.
enum class XmlOrigin { Loose, Archive, New };

/*
Address of an element inside a document, as the chain of child indices leading
to it from the root. Used instead of a tinyxml2::XMLElement* wherever the
address has to outlive a single call: undo reparses the whole document, which
invalidates every pointer into it, and a stale pointer in a tree item would be
a use after free.
*/
struct NodePath {
	std::vector<std::uint16_t> index;

	bool Empty() const { return index.empty(); }
	bool operator==(const NodePath& other) const { return index == other.index; }
	bool operator!=(const NodePath& other) const { return index != other.index; }
};
/*
Where a property's value actually comes from once the ambient *-default cascade
has had its say.

The cascade is not a tidy inheritance tree: an unnamed *-default replaces the
ambient defaults for everything after it in reading order, a named one is
selected by @template, @extends chains one onto another, and a name that
resolves to nothing silently falls back to the ambient set - all exactly as
SystemBuilder::getBoneTemplate and friends behave. Resolving it is what lets the
grid show what a bone will really be built with instead of the schema default.
*/
struct EffectiveValue {
	ValueVariant value;
	// The element the value was read from: the instance itself, or the
	// *-default that last supplied it. Null when nothing in the file does and
	// the value is the schema's own default.
	const tinyxml2::XMLElement* source = nullptr;
	// True when the instance carries the property itself, so editing it is a
	// change to this element rather than an override of something inherited.
	bool explicitHere = false;
	// How to name the source in the UI, e.g. "bone-default #2" or
	// "generic-constraint-default \"soft\"". Empty when source is null.
	std::string sourceLabel;
};

// One thing wrong with a document, or worth knowing about it.
struct Diagnostic {
	enum class Severity { Error, Warning, Info };

	Severity severity = Severity::Warning;
	// The element it is about, so the UI can jump to it.
	NodePath node;
	std::string elementLabel;
	std::string message;
};

// What Validate needs from outside the file to check the names in it. An empty
// list means "not known here", and the checks that would use it are skipped
// rather than reported as failures.
struct ValidationContext {
	std::vector<std::string> nifShapeNames;
	std::vector<std::string> skeletonBoneNames;
};


class XmlDocument {
public:
	XmlDocument();
	~XmlDocument();

	XmlDocument(const XmlDocument&) = delete;
	XmlDocument& operator=(const XmlDocument&) = delete;

	// Movable so a document store can keep them in a container.
	XmlDocument(XmlDocument&&) noexcept;
	XmlDocument& operator=(XmlDocument&&) noexcept;

	// Replaces the content. "outError" gets tinyxml2's message, or a note that
	// the root element is not <system>, which is the only root the simulation
	// accepts.
	bool LoadFromString(const std::string& xml, std::string& outError);
	bool LoadFromStream(std::istream& stream, std::string& outError);

	// Swaps in different content without becoming a different document: the
	// undo history and the record of what is on disk both survive, because
	// importing a file over a document is an edit to it and has to be
	// undoable like one.
	bool ReplaceContent(const std::string& xml, std::string& outError);

	// Starts a new document holding nothing but an empty <system/>.
	void CreateEmpty();

	std::string Serialize() const;
	bool SaveToFile(const std::string& path, std::string& outError) const;

	tinyxml2::XMLElement* Root() const;
	tinyxml2::XMLElement* Resolve(const NodePath& path) const;
	NodePath PathOf(const tinyxml2::XMLElement* element) const;

	// The children of "parent" - the root when it is null - whose kind is one
	// of "kinds", in document order. Does not descend: the constraints inside
	// a constraint-group are children of the group, not of the system.
	std::vector<tinyxml2::XMLElement*> ChildrenOfKinds(std::initializer_list<ElementKind> kinds, const tinyxml2::XMLElement* parent = nullptr) const;
	std::vector<tinyxml2::XMLElement*> ChildrenOfKind(ElementKind kind, const tinyxml2::XMLElement* parent = nullptr) const;

	// What to call an element in a tree: its name, the pair of bodies a
	// constraint joins, or the element's own tag for the ones that carry
	// neither.
	static std::string DisplayName(const tinyxml2::XMLElement* element);

	// Reads the value of "owner"'s child described by "desc". Returns
	// std::monostate when the child is absent, which is not the same as
	// present and zero: absent means the *-default cascade decides.
	//
	// Elements holding structure rather than a value - Transform, None, and
	// every List of anything but plain text - come back as monostate too;
	// those are edited through their own nodes.
	ValueVariant GetValue(const tinyxml2::XMLElement* owner, const ChildDesc& desc) const;
	ValueVariant GetAttr(const tinyxml2::XMLElement* owner, const AttrDesc& desc) const;

	// Whether the child is in the document at all. Not the same as GetValue
	// returning something: the ones that carry structure rather than a value
	// (a transform, a list) are present without having a value to hand back.
	bool HasValue(const tinyxml2::XMLElement* owner, const ChildDesc& desc) const;
	// Resolves what "element" is really built with for one property: its own
	// value, or the one the *-default cascade hands it, or the schema default.
	// Mirrors this preview, so for the kinds whose defaults SystemBuilder does
	// not implement it goes straight to the schema default; see CascadeApplies.
	EffectiveValue ResolveEffective(const tinyxml2::XMLElement* element, const ChildDesc& desc) const;

	// Every tag declared by a per-*-shape in this document, deduplicated, in the
	// order they appear. What the collide list pickers offer.
	std::vector<std::string> CollectTags() const;

	// Everything wrong with the document that can be found without running it,
	// plus the traps the XSD cannot express: the identity constraints, the
	// ranges, and the handful of places where this preview is stricter or
	// looser than the schema. "context" supplies the names from outside the
	// file; leaving a list empty skips the checks that would need it.
	std::vector<Diagnostic> Validate(const ValidationContext& context = ValidationContext()) const;


	// Writes a value, creating the child if it is missing. A new child is
	// inserted among its siblings in the order the schema lists them, so a
	// generated file stays readable, and no existing element moves.
	bool SetValue(tinyxml2::XMLElement* owner, const ChildDesc& desc, const ValueVariant& value);
	bool SetAttr(tinyxml2::XMLElement* owner, const AttrDesc& desc, const ValueVariant& value);

	// Removes the child again, handing the property back to the cascade.
	void ClearValue(tinyxml2::XMLElement* owner, const ChildDesc& desc);

	// Creates an element of "kind" with its required attributes present but
	// empty, inserted after "after" or, when that is null, at the end of
	// "parent".
	tinyxml2::XMLElement* InsertElement(ElementKind kind, tinyxml2::XMLElement* parent, tinyxml2::XMLElement* after);
	void RemoveElement(tinyxml2::XMLElement* element);

	// Records the state before a change so it can be undone. "coalesceKey" and
	// "coalesceDesc" identify what is being edited: repeated changes to the
	// same property within half a second fold into one undo step, so dragging
	// a spin control does not bury the history.
	void BeginEdit(const std::string& label, const NodePath& coalesceKey = {}, const void* coalesceDesc = nullptr);

	bool CanUndo() const { return !undoStack.empty(); }
	bool CanRedo() const { return !redoStack.empty(); }
	const std::string& UndoLabel() const;
	const std::string& RedoLabel() const;
	bool Undo();
	bool Redo();

	// Whether the document differs from the last text that was loaded or
	// saved. Undoing back to that text clears it again: an editor that keeps
	// claiming unsaved work after the work has been taken back teaches people
	// to ignore the marker.
	bool IsDirty() const { return dirty; }

	// Declares the current text to be what is on disk. Called after loading
	// and after saving; nothing else has any business calling it.
	void MarkClean();

	// The path as the NIF spells it, e.g. "SKSE\Plugins\hdtSkinnedMeshConfigs\hair.xml".
	const std::string& XmlPath() const { return xmlPath; }
	// Absolute path of the loose file it was read from; empty for Archive.
	const std::string& SourcePath() const { return sourcePath; }
	XmlOrigin Origin() const { return origin; }
	void SetSource(const std::string& inXmlPath, const std::string& inSourcePath, XmlOrigin inOrigin);

private:
	struct Snapshot {
		std::string text;
		std::string label;
		NodePath coalesceKey;
		const void* coalesceDesc = nullptr;
		long long stamp = 0;
	};

	// How many snapshots to keep. Physics XMLs run from a few kB to a few
	// hundred, so this is worth at most a handful of megabytes.
	static constexpr std::size_t maxUndoDepth = 64;

	std::unique_ptr<tinyxml2::XMLDocument> doc;
	std::string xmlPath;
	std::string sourcePath;
	XmlOrigin origin = XmlOrigin::New;
	bool dirty = false;

	// The text as it was last loaded or saved, so undo can tell that it has
	// arrived back at it. A physics XML runs to a few hundred kilobytes at
	// the very worst, and the undo ring already holds sixty of them.
	std::string cleanText;

	std::deque<Snapshot> undoStack;
	std::deque<Snapshot> redoStack;

	void PushSnapshot(std::deque<Snapshot>& stack, const std::string& label, const NodePath& key, const void* desc);
	bool RestoreFrom(std::deque<Snapshot>& from, std::deque<Snapshot>& to);

	// Parses and checks the root element. Null on failure, with outError set.
	std::unique_ptr<tinyxml2::XMLDocument> ParseSystem(const std::string& xml, std::string& outError) const;

	tinyxml2::XMLElement* FindChild(const tinyxml2::XMLElement* owner, const ChildDesc& desc) const;
	tinyxml2::XMLElement* EnsureChild(tinyxml2::XMLElement* owner, const ChildDesc& desc);
};

/*
The value conversions the simulation itself uses, so the editor reads a file
exactly the way SystemBuilder will. Ported from the converters in
hdt/XmlReader.cpp: a decimal comma, a leading '+', hexadecimal and octal
integers and "true"/"1" all have to keep working.

Writing always produces a '.' decimal separator.
*/
// The value a property falls back to when neither the element nor the ambient
// *-default cascade supplies one, parsed out of the schema's defaultText.
// monostate when the property has no meaningful default, which is not the same
// as a default of zero.
ValueVariant DefaultValue(const ChildDesc& desc);

// The file name part of a physics XML link, for the rows that show one and
// keep the whole path somewhere else. The links in the wild are written with
// either separator.
std::string XmlFileName(const std::string& xmlPath);
/*
The collision filtering of one skinned shape, lifted out of the document so the
editor can answer "does this collide with that?" without building a system.

The rules live in SkinnedMeshBody::canCollideWith and PreviewBody::canCollideWith
and are quietly asymmetric: each side asks the question of the other and both
answers have to be yes, a non-empty whitelist makes the blacklist irrelevant,
and the sharing mode is decided by which system a shape belongs to rather than
by anything in the shape itself.
*/
struct ShapeFacts {
	std::string xmlPath;
	std::string name;
	ElementKind kind = ElementKind::Unknown;
	NodePath node;
	// "public", "private", "internal" or "external"; whatever the file says.
	std::string shared = "public";
	std::vector<std::string> tags;
	std::vector<std::string> canCollideWithTags;
	std::vector<std::string> noCollideWithTags;
};

struct CollisionResult {
	bool collides = false;
	// Which rule decided, in the words the editor shows.
	std::string reason;
};

// The per-*-shapes of a document, with the paths that address them.
std::vector<ShapeFacts> CollectShapeFacts(const XmlDocument& doc);

// Whether the two shapes would ever be tested against each other, and why.
// Tag names are matched case insensitively, as IDStr does.
//
// It cannot see one thing the simulation can: two bodies that are both
// kinematic never collide, and whether a shape is kinematic depends on the
// masses of the bones the NIF happens to skin it to. A pair this reports as
// colliding may still be inert for that reason.
CollisionResult EvaluateCollision(const ShapeFacts& a, const ShapeFacts& b);


namespace XmlValue {
bool ParseFloat(const std::string& text, float& out);
bool ParseInt(const std::string& text, int& out);
bool ParseBool(const std::string& text, bool& out);

std::string FormatFloat(float value);
std::string FormatInt(int value);
std::string FormatBool(bool value);
}
}
