/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "XmlProfiles.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <tinyxml2.h>

using namespace tinyxml2;

namespace Physics {
namespace {

std::string AttrText(const XMLElement* element, const char* name) {
	const char* raw = element ? element->Attribute(name) : nullptr;
	return raw ? std::string(raw) : std::string();
}

bool IsProfileKind(ElementKind kind) {
	const auto& kinds = ProfileKinds();
	return std::find(kinds.begin(), kinds.end(), kind) != kinds.end();
}

bool IsConstraintKind(ElementKind kind) {
	return kind == ElementKind::GenericConstraint || kind == ElementKind::StiffSpringConstraint || kind == ElementKind::ConeTwistConstraint;
}

// Every element of the system in the order the builder reads them, descending into constraint groups
void ForEachInReadingOrder(const XmlDocument& doc, const std::function<void(XMLElement*)>& visit) {
	std::function<void(XMLElement*)> walk = [&](XMLElement* parent) {
		for (XMLElement* child = parent->FirstChildElement(); child; child = child->NextSiblingElement()) {
			visit(child);
			if (KindFromName(child->Name()) == ElementKind::ConstraintGroup)
				walk(child);
		}
	};

	if (XMLElement* root = doc.Root())
		walk(root);
}

// The bones and constraints, in reading order
std::vector<XMLElement*> ProfileUsersOfAnyKind(const XmlDocument& doc) {
	std::vector<XMLElement*> elements;
	ForEachInReadingOrder(doc, [&](XMLElement* element) {
		const ElementKind kind = KindFromName(element->Name());
		if (kind == ElementKind::Bone || IsConstraintKind(kind))
			elements.push_back(element);
	});
	return elements;
}

// Where the builder creates a bone nothing declares: a skinned shape creates the bones it is weighted to and a
// constraint the two it joins, both from the ambient bone template at that point
std::vector<XMLElement*> ImplicitBonePositions(const XmlDocument& doc) {
	std::vector<XMLElement*> positions;
	ForEachInReadingOrder(doc, [&](XMLElement* element) {
		const ElementKind kind = KindFromName(element->Name());
		if (kind == ElementKind::PerVertexShape || kind == ElementKind::PerTriangleShape || IsConstraintKind(kind))
			positions.push_back(element);
	});
	return positions;
}

const ElementDesc& DescOf(ElementKind instanceKind) {
	return *FindElement(instanceKind);
}

// A property as a template chain supplies it, as XML text: the last default that sets it, or every default's
// entries for a list, which the builder appends one after the other. Empty for the schema default.
std::string ChainText(const XmlDocument& doc, const std::vector<const XMLElement*>& chain, const ChildDesc& desc) {
	std::string text;
	for (const XMLElement* declared : chain) {
		if (!doc.HasValue(declared, desc))
			continue;

		if (desc.mult == Multiplicity::List)
			text += doc.ValueText(declared, desc);
		else
			text = doc.ValueText(declared, desc);
	}
	return text;
}

// Everything a template chain supplies, one entry per property of the kind
using Signature = std::vector<std::string>;

Signature ChainSignature(const XmlDocument& doc, const std::vector<const XMLElement*>& chain, ElementKind instanceKind) {
	const ElementDesc& desc = DescOf(instanceKind);
	Signature signature;
	for (std::size_t i = 0; i < desc.childCount; i++)
		signature.push_back(ChainText(doc, chain, desc.children[i]));
	return signature;
}

bool IsEmpty(const Signature& signature) {
	return std::all_of(signature.begin(), signature.end(), [](const std::string& text) { return text.empty(); });
}

// What an element is built with, per property: its own value, or its own list entries after the template's
Signature ElementSignature(const XmlDocument& doc, const XMLElement* element) {
	const ElementKind kind = KindFromName(element->Name());
	const ElementDesc& desc = DescOf(kind);
	const std::vector<const XMLElement*> chain = doc.TemplateChain(element);

	Signature signature;
	for (std::size_t i = 0; i < desc.childCount; i++) {
		const ChildDesc& child = desc.children[i];
		const std::string own = doc.ValueText(element, child);
		if (child.mult == Multiplicity::List)
			signature.push_back(ChainText(doc, chain, child) + own);
		else
			signature.push_back(doc.HasValue(element, child) ? own : ChainText(doc, chain, child));
	}
	return signature;
}

// Everything the builder would make of the file that the defaults decide: every bone and constraint, and the
// template implicit bones start from at every point one can be created
std::vector<Signature> Snapshot(const XmlDocument& doc) {
	std::vector<Signature> values;
	for (const XMLElement* element : ProfileUsersOfAnyKind(doc))
		values.push_back(ElementSignature(doc, element));
	for (const XMLElement* position : ImplicitBonePositions(doc))
		values.push_back(ChainSignature(doc, doc.TemplateChain(ElementKind::BoneDefault, position, std::string()), ElementKind::Bone));
	return values;
}

// Writes what a chain supplies onto "target", flat
void Flatten(XmlDocument& doc, const std::vector<const XMLElement*>& chain, ElementKind instanceKind, XMLElement* target) {
	const ElementDesc& desc = DescOf(instanceKind);
	for (std::size_t i = 0; i < desc.childCount; i++) {
		const ChildDesc& child = desc.children[i];
		if (child.mult == Multiplicity::List) {
			for (const XMLElement* declared : chain)
				if (doc.HasValue(declared, child))
					doc.CopyValue(declared, target, child, true);
			continue;
		}

		const EffectiveValue supplied = doc.ResolveChain(chain, child);
		if (supplied.source)
			doc.CopyValue(supplied.source, target, child);
	}
}

void SetAttribute(XmlDocument& doc, XMLElement* element, const char* name, const std::string& value) {
	const ElementDesc* desc = FindElement(KindFromName(element->Name()));
	const AttrDesc* attr = desc ? desc->Attr(name) : nullptr;
	if (attr)
		doc.SetAttr(element, *attr, value.empty() ? ValueVariant() : ValueVariant(value));
}

} // namespace

const std::vector<ElementKind>& ProfileKinds() {
	static const std::vector<ElementKind> kinds{ElementKind::BoneDefault,
												ElementKind::GenericConstraintDefault,
												ElementKind::StiffSpringConstraintDefault,
												ElementKind::ConeTwistConstraintDefault};
	return kinds;
}

std::vector<XMLElement*> Profiles(const XmlDocument& doc) {
	std::vector<XMLElement*> profiles;
	ForEachInReadingOrder(doc, [&](XMLElement* element) {
		if (IsProfileKind(KindFromName(element->Name())))
			profiles.push_back(element);
	});
	return profiles;
}

XMLElement* ProfileOf(const XmlDocument& doc, const XMLElement* element) {
	const std::vector<const XMLElement*> chain = doc.TemplateChain(element);
	return chain.empty() ? nullptr : const_cast<XMLElement*>(chain.back());
}

std::vector<XMLElement*> ProfileUsers(const XmlDocument& doc, const XMLElement* profile) {
	std::vector<XMLElement*> users;
	if (!profile)
		return users;

	const ElementKind instanceKind = InstanceKindOf(KindFromName(profile->Name()));
	for (XMLElement* element : ProfileUsersOfAnyKind(doc)) {
		if (KindFromName(element->Name()) != instanceKind)
			continue;

		const std::vector<const XMLElement*> chain = doc.TemplateChain(element);
		if (std::find(chain.begin(), chain.end(), profile) != chain.end())
			users.push_back(element);
	}
	return users;
}

bool IsPositional(const XmlDocument& doc, const XMLElement* profile) {
	if (!profile || !AttrText(profile, "name").empty())
		return false;

	const ElementKind defaultKind = KindFromName(profile->Name());
	const ElementKind instanceKind = InstanceKindOf(defaultKind);

	bool reached = false;
	bool positional = false;
	ForEachInReadingOrder(doc, [&](XMLElement* element) {
		if (element == profile) {
			reached = true;
			return;
		}

		const ElementKind kind = KindFromName(element->Name());
		if (kind == defaultKind && AttrText(element, "name").empty())
			positional = true;

		// Before it, what reads the ambient set reads something else. For bones that includes the places that
		// create bones nothing declares.
		const bool reads = kind == instanceKind
						   || (instanceKind == ElementKind::Bone
							   && (kind == ElementKind::PerVertexShape || kind == ElementKind::PerTriangleShape || IsConstraintKind(kind)));
		if (!reached && reads)
			positional = true;
	});
	return positional;
}

bool NeedsTidy(const XmlDocument& doc) {
	std::map<ElementKind, std::set<std::string>> names;
	for (const XMLElement* profile : Profiles(doc)) {
		if (IsPositional(doc, profile) || !AttrText(profile, "extends").empty())
			return true;

		// A default inside a constraint group looks scoped to it and isn't
		if (profile->Parent() != doc.Root())
			return true;

		const std::string name = ToLower(AttrText(profile, "name"));
		if (!name.empty() && !names[KindFromName(profile->Name())].insert(name).second)
			return true;
	}

	// A template naming nothing declared before it silently falls back to the ambient set
	for (const XMLElement* element : ProfileUsersOfAnyKind(doc)) {
		const std::string wanted = AttrText(element, "template");
		if (wanted.empty())
			continue;

		const XMLElement* profile = ProfileOf(doc, element);
		if (!profile || !StringsEqualInsens(AttrText(profile, "name").c_str(), wanted.c_str()))
			return true;
	}

	return false;
}

bool ApplyToProfile(XmlDocument& doc, XMLElement* element, const ChildDesc& desc) {
	XMLElement* profile = ProfileOf(doc, element);
	if (!profile || !doc.HasValue(element, desc))
		return false;

	doc.BeginEdit("apply to profile");

	// The element's list entries came after the profile's, so they go after them there too
	doc.CopyValue(element, profile, desc, desc.mult == Multiplicity::List);
	doc.ClearValue(element, desc);
	return true;
}

XMLElement* CreateProfileFrom(XmlDocument& doc, XMLElement* element, const std::string& name) {
	const ElementKind kind = KindFromName(element ? element->Name() : nullptr);
	const ElementKind defaultKind = DefaultKindOf(kind);
	if (name.empty() || !IsProfileKind(defaultKind) || IsDefaultKind(kind))
		return nullptr;

	for (const XMLElement* profile : Profiles(doc))
		if (KindFromName(profile->Name()) == defaultKind && StringsEqualInsens(AttrText(profile, "name").c_str(), name.c_str()))
			return nullptr;

	// Right before the element, so the chain it extends is exactly the one the element read
	const std::vector<const XMLElement*> chain = doc.TemplateChain(element);
	const std::string base = chain.empty() ? std::string() : AttrText(chain.back(), "name");

	doc.BeginEdit("new profile");

	XMLElement* created = doc.InsertElementBefore(defaultKind, element);
	if (!created)
		return nullptr;

	SetAttribute(doc, created, "name", name);
	SetAttribute(doc, created, "extends", base);

	const ElementDesc& desc = DescOf(kind);
	for (std::size_t i = 0; i < desc.childCount; i++) {
		const ChildDesc& child = desc.children[i];
		if (!doc.HasValue(element, child))
			continue;

		doc.CopyValue(element, created, child);
		doc.ClearValue(element, child);
	}

	SetAttribute(doc, element, "template", name);
	return created;
}

TidyResult TidyDefaults(XmlDocument& doc) {
	TidyResult result;

	// Rewritten in a copy, so a failed check leaves no trace in the undo history
	XmlDocument work;
	std::string error;
	if (!work.LoadFromString(doc.Serialize(), error)) {
		result.report = error;
		return result;
	}

	XMLElement* root = work.Root();
	const std::vector<Signature> before = Snapshot(work);

	// Bones nothing declares start from the ambient set wherever they are created, and after tidying there is only
	// one ambient set
	std::optional<Signature> implicitBones;
	for (const XMLElement* position : ImplicitBonePositions(work)) {
		const Signature ambient = ChainSignature(work, work.TemplateChain(ElementKind::BoneDefault, position, std::string()), ElementKind::Bone);
		if (!implicitBones)
			implicitBones = ambient;
		else if (ambient != *implicitBones) {
			result.report = "Bones that shapes and constraints create without a <bone> element start from different bone-defaults at '"
							+ XmlDocument::DisplayName(position)
							+ "' than earlier in the file. Declare those bones with <bone> elements first, so tidying can keep their values.";
			return result;
		}
	}

	const std::vector<XMLElement*> oldDefaults = Profiles(work);
	result.defaultsBefore = static_cast<int>(oldDefaults.size());

	struct NewProfile {
		std::vector<const XMLElement*> chain;
		Signature signature;
		std::string name;
		bool ambient = false;
		int users = 0;
	};

	struct KindPlan {
		ElementKind defaultKind = ElementKind::Unknown;
		std::vector<NewProfile> profiles;
		std::map<XMLElement*, std::size_t> profileOfUser;
	};

	std::vector<KindPlan> plans;
	for (ElementKind defaultKind : ProfileKinds()) {
		const ElementKind instanceKind = InstanceKindOf(defaultKind);
		KindPlan plan;
		plan.defaultKind = defaultKind;

		// One profile per distinct set of values under one name. Two names with the same values stay apart, so no
		// name an author chose is lost.
		auto add = [&](const std::vector<const XMLElement*>& chain, const std::string& name) {
			Signature signature = ChainSignature(work, chain, instanceKind);
			for (std::size_t i = 0; i < plan.profiles.size(); i++)
				if (plan.profiles[i].signature == signature && StringsEqualInsens(plan.profiles[i].name.c_str(), name.c_str()))
					return i;

			NewProfile profile;
			profile.chain = chain;
			profile.signature = std::move(signature);
			profile.name = name;
			plan.profiles.push_back(std::move(profile));
			return plan.profiles.size() - 1;
		};

		for (XMLElement* element : ProfileUsersOfAnyKind(work)) {
			if (KindFromName(element->Name()) != instanceKind)
				continue;

			const std::vector<const XMLElement*> chain = work.TemplateChain(element);
			const std::size_t index = add(chain, chain.empty() ? std::string() : AttrText(chain.back(), "name"));
			plan.profileOfUser[element] = index;
			plan.profiles[index].users++;
		}

		// Named profiles nothing uses yet were prepared for something, so they stay
		for (XMLElement* declared : oldDefaults) {
			const std::string name = AttrText(declared, "name");
			if (KindFromName(declared->Name()) != defaultKind || name.empty())
				continue;

			std::vector<const XMLElement*> chain = work.TemplateChain(declared);
			chain.push_back(declared);
			add(chain, name);
		}

		// The ambient set: for bones the one implicit bones need, otherwise the most used unnamed one
		std::optional<std::size_t> ambient;
		if (defaultKind == ElementKind::BoneDefault && implicitBones) {
			for (std::size_t i = 0; i < plan.profiles.size() && !ambient; i++)
				if (plan.profiles[i].name.empty() && plan.profiles[i].signature == *implicitBones)
					ambient = i;

			if (!ambient) {
				NewProfile profile;
				profile.chain = work.TemplateChain(ElementKind::BoneDefault, ImplicitBonePositions(work).front(), std::string());
				profile.signature = *implicitBones;
				plan.profiles.push_back(std::move(profile));
				ambient = plan.profiles.size() - 1;
			}
		}
		else {
			// On a tie the one that sets something, so "Default" is what a simulated element starts from
			auto better = [&](const NewProfile& candidate, const NewProfile& current) {
				if (candidate.users != current.users)
					return candidate.users > current.users;
				return IsEmpty(current.signature) && !IsEmpty(candidate.signature);
			};
			for (std::size_t i = 0; i < plan.profiles.size(); i++)
				if (plan.profiles[i].name.empty() && (!ambient || better(plan.profiles[i], plan.profiles[*ambient])))
					ambient = i;
		}

		// Names: every profile but the ambient one gets one, unique within the kind
		std::set<std::string> taken;
		for (auto& profile : plan.profiles)
			if (!profile.name.empty())
				taken.insert(ToLower(profile.name));

		std::set<std::string> used;
		for (std::size_t i = 0; i < plan.profiles.size(); i++) {
			NewProfile& profile = plan.profiles[i];
			if (ambient && i == *ambient) {
				profile.ambient = true;
				profile.name.clear();
				continue;
			}

			// "Schema defaults" for an unnamed one that sets nothing, "Profile 1" for other unnamed ones, "soft 2" for
			// the second "soft" with other values
			std::string name = profile.name.empty() && IsEmpty(profile.signature) ? "Schema defaults" : profile.name;
			const bool clash = !name.empty() && (used.count(ToLower(name)) || (profile.name.empty() && taken.count(ToLower(name))));
			if (name.empty() || clash) {
				const std::string base = name.empty() ? "Profile" : name;
				int suffix = name.empty() ? 1 : 2;
				do
					name = base + " " + std::to_string(suffix++);
				while (taken.count(ToLower(name)) || used.count(ToLower(name)));
			}

			used.insert(ToLower(name));
			profile.name = name;
		}

		plans.push_back(std::move(plan));
	}

	// The new profiles go first, after any leading comment: per kind the ambient Default, then the named ones as what
	// they change about it. A named profile that can't be said that way (its list doesn't extend the Default's, or it
	// leaves out something the Default sets) comes before the Default instead, with everything it holds.
	XMLElement* anchor = root->FirstChildElement();
	XMLElement* previous = nullptr;
	auto insert = [&](ElementKind defaultKind) {
		XMLElement* created = previous ? work.InsertElement(defaultKind, root, previous)
							  : anchor	 ? work.InsertElementBefore(defaultKind, anchor)
										 : work.InsertElement(defaultKind, root, nullptr);
		previous = created;
		return created;
	};

	int written = 0;
	for (KindPlan& plan : plans) {
		const ElementKind instanceKind = InstanceKindOf(plan.defaultKind);
		const ElementDesc& desc = DescOf(instanceKind);

		// An empty ambient set is the schema defaults, which need nothing written
		const NewProfile* ambient = nullptr;
		for (const NewProfile& profile : plan.profiles)
			if (profile.ambient && !IsEmpty(profile.signature))
				ambient = &profile;

		auto extendsAmbient = [&](const NewProfile& profile) {
			if (!ambient)
				return false;

			for (std::size_t i = 0; i < desc.childCount; i++) {
				const std::string& own = profile.signature[i];
				const std::string& base = ambient->signature[i];
				if (desc.children[i].mult == Multiplicity::List ? own.compare(0, base.size(), base) != 0 : own.empty() && !base.empty())
					return false;
			}
			return true;
		};

		XMLElement* ambientElement = nullptr;
		for (int pass = 0; pass < 3; pass++) {
			for (const NewProfile& profile : plan.profiles) {
				const bool isAmbient = &profile == ambient;
				if (profile.ambient && !isAmbient)
					continue;

				const int profilePass = isAmbient ? 1 : extendsAmbient(profile) ? 2 : 0;
				if (profilePass != pass)
					continue;

				XMLElement* created = insert(plan.defaultKind);
				SetAttribute(work, created, "name", profile.name);
				Flatten(work, profile.chain, instanceKind, created);
				written++;

				if (isAmbient) {
					ambientElement = created;
					continue;
				}

				// Only what it changes about the Default, which it starts from
				for (std::size_t i = 0; i < desc.childCount && pass == 2; i++) {
					const ChildDesc& child = desc.children[i];
					if (child.mult == Multiplicity::List)
						work.RemoveLeadingValues(created, child, work.ValueCount(ambientElement, child));
					else if (!ambient->signature[i].empty() && profile.signature[i] == ambient->signature[i])
						work.ClearValue(created, child);
				}
			}
		}
	}

	for (XMLElement* declared : oldDefaults)
		work.RemoveElement(declared);

	for (KindPlan& plan : plans)
		for (auto& [element, index] : plan.profileOfUser)
			SetAttribute(work, element, "template", plan.profiles[index].name);

	result.profilesAfter = written;

	// Everything has to be built exactly as before. A difference is a bug here, never something to save.
	const std::vector<Signature> after = Snapshot(work);
	if (after != before) {
		std::string where;
		const std::vector<XMLElement*> elements = ProfileUsersOfAnyKind(work);
		for (std::size_t i = 0; i < std::min(before.size(), after.size()) && where.empty(); i++)
			if (before[i] != after[i])
				where = i < elements.size() ? XmlDocument::DisplayName(elements[i]) : "an implicitly created bone";

		result.report = "Tidying would change the values of '" + where + "', so nothing was changed.";
		return result;
	}

	doc.BeginEdit("tidy defaults");
	if (!doc.ReplaceContent(work.Serialize(), error)) {
		doc.Undo();
		result.report = error;
		return result;
	}

	result.ok = true;
	result.report = "Rewrote " + std::to_string(result.defaultsBefore) + " defaults into " + std::to_string(written)
					+ " profiles. Every bone and constraint keeps its values.";
	return result;
}
} // namespace Physics
