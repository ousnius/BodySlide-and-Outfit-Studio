/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "McpHelpers.h"
#include "components/Anim.h"
#include "files/GameDataStream.h"
#include "physics/Controller.h"
#include "physics/XmlEditSession.h"
#include "physics/XmlProfiles.h"
#include "project/OutfitProject.h"
#include "project/StudioSession.h"
#include "utils/StringStuff.h"

#include <tinyxml2.h>

#include <filesystem>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>

using nlohmann::json;
using namespace Physics;

namespace {

// One physics XML a tool works on: the physics editor's copy when it holds one, edits included, else the file
struct XmlSource {
	// As given, or as the NIF links it
	std::string link;
	// The file it was read from; empty for one out of an archive or that only exists in the editor
	std::string path;
	// "editor", "file" or "archive"
	std::string from;
	bool unsaved = false;
	// The loaded shapes that link it
	std::vector<std::string> shapes;
	XmlDocument* doc = nullptr;
	std::unique_ptr<XmlDocument> owned;
	// Why it couldn't be read
	std::string error;
};

XmlSource OpenXml(OutfitStudioMcp& mcp, const std::string& given, std::vector<std::string> shapes) {
	XmlSource source;
	source.link = given;
	source.shapes = std::move(shapes);

	const std::string filePath = McpFilePath(given);
	std::error_code ec;
	const bool isFile = std::filesystem::is_regular_file(std::filesystem::u8path(filePath), ec);

	XmlEditSession* editing = mcp.Session().PhysicsXmlSession();
	if (!isFile && editing) {
		if (XmlDocument* held = editing->Find(given)) {
			source.doc = held;
			source.from = "editor";
			source.unsaved = held->IsDirty();
			source.path = held->Origin() == XmlOrigin::Loose ? held->SourcePath() : std::string();
			return source;
		}
	}

	std::unique_ptr<std::istream> stream;
	if (isFile) {
		stream = std::make_unique<std::ifstream>(std::filesystem::u8path(filePath), std::ios::binary);
		source.path = filePath;
		source.from = "file";
	}
	else {
		GameDataStream::FoundAt foundAt;
		stream = mcp.Project().GetPhysicsXmlStream(given, &foundAt);
		source.path = foundAt.inArchive ? std::string() : foundAt.path;
		source.from = foundAt.inArchive ? "archive" : "file";
	}

	if (!stream || !*stream) {
		source.error = "not found as a file or in the game data";
		return source;
	}

	std::ostringstream text;
	text << stream->rdbuf();
	source.owned = std::make_unique<XmlDocument>();
	if (!source.owned->LoadFromString(text.str(), source.error)) {
		source.owned.reset();
		return source;
	}

	source.doc = source.owned.get();
	return source;
}

// The XMLs of the "xml" argument, or every XML the loaded shapes link
std::vector<XmlSource> OpenXmls(OutfitStudioMcp& mcp, const json& args) {
	std::vector<XmlSource> sources;
	const std::vector<std::string> given = McpArgs::Strings(args, "xml");
	if (!given.empty()) {
		for (auto& link : given) {
			sources.push_back(OpenXml(mcp, link, {}));
			if (!sources.back().doc)
				throw McpToolError("Can't read the physics XML '" + link + "': " + sources.back().error + ".");
		}
		return sources;
	}

	OutfitProject& project = mcp.Project();
	for (auto& system : CollectPhysicsXmlLinks(project.GetWorkNif(), project.shapePhysicsFiles))
		sources.push_back(OpenXml(mcp, system.xmlPath, system.shapeNames));

	if (sources.empty())
		throw McpToolError("No loaded shape links a physics XML. Pass xml.");
	return sources;
}

json SourceJson(const XmlSource& source) {
	json entry = {{"xml", source.link}, {"from", source.from}};
	if (!source.path.empty())
		entry["path"] = source.path;
	if (source.unsaved)
		entry["unsavedEdits"] = true;
	if (!source.shapes.empty())
		entry["linkedBy"] = source.shapes;
	if (!source.error.empty())
		entry["error"] = source.error;
	return entry;
}

json ValueJson(const XmlDocument& doc, const ValueVariant& value, const tinyxml2::XMLElement* source, const ChildDesc& desc) {
	if (const auto* flag = std::get_if<bool>(&value))
		return *flag;
	if (const auto* number = std::get_if<int>(&value))
		return *number;
	if (const auto* number = std::get_if<float>(&value))
		return *number;
	if (const auto* text = std::get_if<std::string>(&value))
		return *text;
	if (const auto* list = std::get_if<std::vector<std::string>>(&value))
		return *list;
	if (const auto* v = std::get_if<nifly::Vector3>(&value))
		return json::array({v->x, v->y, v->z});
	if (const auto* q = std::get_if<nifly::Quaternion>(&value))
		return json::array({q->x, q->y, q->z, q->w});
	if (const auto* a = std::get_if<AxisAngleValue>(&value))
		return {{"axis", json::array({a->axis.x, a->axis.y, a->axis.z})}, {"angle", a->angle}};
	if (const auto* l = std::get_if<LerpValue>(&value))
		return {{"translation", l->translation}, {"rotation", l->rotation}};

	// Structure (a transform, a shape): the XML itself
	return source ? json(doc.ValueText(source, desc)) : json();
}

std::string KindName(const tinyxml2::XMLElement* element) {
	return element && element->Name() ? element->Name() : std::string();
}

std::string ProfileName(const XmlDocument& doc, const tinyxml2::XMLElement* profile) {
	return profile ? doc.ProfileLabel(profile) : "schema defaults";
}

// What a chain of defaults gives every property, in "values"; the profile each comes from in "from"
void AddChainValues(const XmlDocument& doc, const std::vector<const tinyxml2::XMLElement*>& chain, const ElementDesc& desc, bool all, json& values, json* from) {
	for (std::size_t i = 0; i < desc.childCount; i++) {
		const ChildDesc& child = desc.children[i];
		const EffectiveValue supplied = doc.ResolveChain(chain, child);
		if (!supplied.source && !all)
			continue;

		values[child.name] = ValueJson(doc, supplied.value, supplied.source, child);
		if (from && supplied.source)
			(*from)[child.name] = supplied.sourceLabel;
	}
}

// What a bone or constraint is built with. A list is the profile's entries and then the element's own, as the builder
// appends them. Without "all", properties at their schema default are left out. "inherited" names the default a value
// comes from only where that isn't the element's profile itself, e.g. what the profile extends.
json ElementJson(const XmlDocument& doc, const tinyxml2::XMLElement* element, bool all) {
	const ElementDesc& desc = *FindElement(KindFromName(element->Name()));
	const std::vector<const tinyxml2::XMLElement*> chain = doc.TemplateChain(element);
	const std::string profile = ProfileName(doc, chain.empty() ? nullptr : chain.back());

	json values = json::object();
	json inherited = json::object();
	json own = json::array();
	for (std::size_t i = 0; i < desc.childCount; i++) {
		const ChildDesc& child = desc.children[i];
		const bool explicitHere = doc.HasValue(element, child);
		const EffectiveValue supplied = doc.ResolveChain(chain, child);

		if (explicitHere)
			own.push_back(child.name);

		if (child.mult == Multiplicity::List) {
			std::vector<std::string> items;
			if (const auto* list = std::get_if<std::vector<std::string>>(&supplied.value))
				items = *list;
			const ValueVariant ownValue = doc.GetValue(element, child);
			if (const auto* list = std::get_if<std::vector<std::string>>(&ownValue))
				items.insert(items.end(), list->begin(), list->end());

			if (!items.empty() || all)
				values[child.name] = items;
			if (supplied.source && supplied.sourceLabel != profile)
				inherited[child.name] = supplied.sourceLabel;
			continue;
		}

		if (explicitHere) {
			values[child.name] = ValueJson(doc, doc.GetValue(element, child), element, child);
			continue;
		}

		if (!supplied.source && !all)
			continue;

		values[child.name] = ValueJson(doc, supplied.value, supplied.source, child);
		if (supplied.source && supplied.sourceLabel != profile)
			inherited[child.name] = supplied.sourceLabel;
	}

	json entry = {{"profile", profile}, {"values", values}};
	if (!own.empty())
		entry["own"] = own;
	if (!inherited.empty())
		entry["inherited"] = inherited;
	return entry;
}

// Every element of the system in reading order, descending into constraint groups
void ForEachInReadingOrder(const XmlDocument& doc, const std::function<void(tinyxml2::XMLElement*)>& visit) {
	std::function<void(tinyxml2::XMLElement*)> walk = [&](tinyxml2::XMLElement* parent) {
		for (tinyxml2::XMLElement* child = parent->FirstChildElement(); child; child = child->NextSiblingElement()) {
			visit(child);
			if (KindFromName(child->Name()) == ElementKind::ConstraintGroup)
				walk(child);
		}
	};
	walk(doc.Root());
}

bool IsConstraint(ElementKind kind) {
	return kind == ElementKind::GenericConstraint || kind == ElementKind::StiffSpringConstraint || kind == ElementKind::ConeTwistConstraint;
}

// The bones a loaded shape is skinned to, by its name in any case
std::vector<std::string> SkinBones(OutfitProject& project, const std::string& shapeName) {
	for (auto& [name, bones] : project.GetWorkAnim()->shapeBones)
		if (StringsEqualInsens(name.c_str(), shapeName.c_str()))
			return bones;
	return {};
}

bool Wanted(const std::set<std::string>& filter, const std::string& name) {
	return filter.empty() || filter.count(ToLower(name));
}

McpToolResult InspectPhysicsXml(OutfitStudioMcp& mcp, const json& args) {
	OutfitProject& project = mcp.Project();
	const bool all = McpArgs::Bool(args, "allProperties", false);

	std::set<std::string> filter;
	for (auto& name : McpArgs::Strings(args, "bones"))
		filter.insert(ToLower(name));

	// What validation knows from outside the file: the loaded shapes, and the skeleton's bones and the NIF's nodes
	ValidationContext context;
	context.nifShapeNames = project.GetWorkNif()->GetShapeNames();
	mcp.Skeleton().GetBoneNames(context.skeletonBoneNames);
	for (const nifly::NiNode* node : project.GetWorkNif()->GetNodes())
		if (!node->name.get().empty())
			context.skeletonBoneNames.push_back(node->name.get());

	McpToolResult result;
	json files = json::array();
	for (XmlSource& source : OpenXmls(mcp, args)) {
		json file = SourceJson(source);
		if (!source.doc) {
			files.push_back(file);
			continue;
		}
		const XmlDocument& doc = *source.doc;

		json profiles = json::array();
		for (const tinyxml2::XMLElement* profile : Profiles(doc)) {
			std::vector<const tinyxml2::XMLElement*> chain = doc.TemplateChain(profile);
			chain.push_back(profile);

			json values = json::object();
			AddChainValues(doc, chain, *FindElement(InstanceKindOf(KindFromName(profile->Name()))), all, values, nullptr);

			json users = json::array();
			for (const tinyxml2::XMLElement* user : ProfileUsers(doc, profile))
				users.push_back(XmlDocument::DisplayName(user));

			json entry = {{"name", doc.ProfileLabel(profile)}, {"kind", KindName(profile)}, {"usedBy", users}, {"values", values}};
			if (IsPositional(doc, profile))
				entry["dependsOnPosition"] = true;
			profiles.push_back(entry);
		}

		// The builder reads the file top to bottom: a <bone> declares a bone, a skinned shape creates the bones it is
		// weighted to and a constraint the two it joins from the ambient bone-default at that line, unless they
		// exist. A <bone> after the bone was created that way is skipped.
		json bones = json::array();
		json constraints = json::array();
		json implicitBones = json::array();
		json ignoredBones = json::array();
		std::set<std::string> exists;

		auto createImplicit = [&](const std::string& name, const tinyxml2::XMLElement* by) {
			if (name.empty() || !exists.insert(ToLower(name)).second || !Wanted(filter, name))
				return;

			const std::vector<const tinyxml2::XMLElement*> chain = doc.TemplateChain(ElementKind::BoneDefault, by, std::string());
			json values = json::object();
			json from = json::object();
			AddChainValues(doc, chain, *FindElement(ElementKind::Bone), all, values, &from);
			implicitBones.push_back({{"name", name},
									 {"createdBy", KindName(by) + " " + XmlDocument::DisplayName(by)},
									 {"profile", ProfileName(doc, chain.empty() ? nullptr : chain.back())},
									 {"values", values}});
		};

		ForEachInReadingOrder(doc, [&](tinyxml2::XMLElement* element) {
			const ElementKind kind = KindFromName(element->Name());
			const std::string name = element->Attribute("name") ? element->Attribute("name") : "";

			if (kind == ElementKind::Bone) {
				if (!exists.insert(ToLower(name)).second) {
					if (Wanted(filter, name))
						ignoredBones.push_back(name);
					return;
				}
				if (Wanted(filter, name)) {
					json entry = ElementJson(doc, element, all);
					entry["name"] = name;
					bones.push_back(entry);
				}
			}
			else if (kind == ElementKind::PerVertexShape || kind == ElementKind::PerTriangleShape) {
				for (auto& bone : SkinBones(project, name))
					createImplicit(bone, element);
			}
			else if (IsConstraint(kind)) {
				const std::string bodyA = element->Attribute("bodyA") ? element->Attribute("bodyA") : "";
				const std::string bodyB = element->Attribute("bodyB") ? element->Attribute("bodyB") : "";
				createImplicit(bodyA, element);
				createImplicit(bodyB, element);

				if (Wanted(filter, bodyA) || Wanted(filter, bodyB)) {
					json entry = ElementJson(doc, element, all);
					entry["kind"] = KindName(element);
					entry["bodyA"] = bodyA;
					entry["bodyB"] = bodyB;
					if (element->Parent() != doc.Root())
						entry["inConstraintGroup"] = true;
					constraints.push_back(entry);
				}
			}
		});

		json diagnostics = json::array();
		for (const Diagnostic& diagnostic : doc.Validate(context)) {
			const char* severity = diagnostic.severity == Diagnostic::Severity::Error	  ? "error"
								   : diagnostic.severity == Diagnostic::Severity::Warning ? "warning"
																						  : "note";
			diagnostics.push_back({{"severity", severity}, {"element", diagnostic.elementLabel}, {"message", diagnostic.message}});
		}

		file["profiles"] = profiles;
		file["bones"] = bones;
		file["constraints"] = constraints;
		if (!implicitBones.empty())
			file["implicitBones"] = implicitBones;
		if (!ignoredBones.empty())
			file["ignoredBoneElements"] = ignoredBones;
		if (!diagnostics.empty())
			file["diagnostics"] = diagnostics;
		if (NeedsTidy(doc))
			file["needsTidy"] = true;
		files.push_back(file);
	}

	result.data["files"] = files;
	return result;
}

McpToolResult PhysicsCollisions(OutfitStudioMcp& mcp, const json& args) {
	const std::string only = McpArgs::String(args, "shape", "");

	McpToolResult result;
	json files = json::array();
	std::vector<ShapeFacts> facts;
	for (XmlSource& source : OpenXmls(mcp, args)) {
		files.push_back(SourceJson(source));
		if (source.doc) {
			std::vector<ShapeFacts> found = CollectShapeFacts(*source.doc);
			facts.insert(facts.end(), found.begin(), found.end());
		}
	}

	json shapes = json::array();
	for (const ShapeFacts& shape : facts) {
		shapes.push_back({{"name", shape.name},
						  {"xml", shape.xmlPath},
						  {"kind", NameFromKind(shape.kind)},
						  {"shared", shape.shared},
						  {"tags", shape.tags},
						  {"canCollideWithTags", shape.canCollideWithTags},
						  {"noCollideWithTags", shape.noCollideWithTags}});
	}

	json pairs = json::array();
	for (std::size_t a = 0; a < facts.size(); a++) {
		for (std::size_t b = a + 1; b < facts.size(); b++) {
			if (!only.empty() && !StringsEqualInsens(facts[a].name.c_str(), only.c_str()) && !StringsEqualInsens(facts[b].name.c_str(), only.c_str()))
				continue;

			const CollisionResult verdict = EvaluateCollision(facts[a], facts[b]);
			pairs.push_back({{"a", facts[a].name}, {"b", facts[b].name}, {"collides", verdict.collides}, {"reason", verdict.reason}});
		}
	}

	result.data["files"] = files;
	result.data["shapes"] = shapes;
	result.data["pairs"] = pairs;
	result.data["note"] = "Only the filtering in the XMLs: two bodies that are both kinematic (every bone they move with has no mass) never collide either.";
	return result;
}

McpToolResult TidyPhysicsXml(OutfitStudioMcp& mcp, const json& args) {
	std::vector<XmlSource> sources = OpenXmls(mcp, args);
	if (sources.size() != 1) {
		std::vector<std::string> links;
		for (auto& source : sources)
			links.push_back(source.link);
		throw McpToolError("The loaded shapes link several physics XMLs, pass xml with one of: " + JoinStrings(links, ", ") + ".");
	}

	XmlSource& source = sources.front();
	if (!source.doc)
		throw McpToolError("Can't read the physics XML '" + source.link + "': " + source.error + ".");
	if (source.unsaved)
		throw McpToolError("'" + source.link + "' has unsaved edits in the physics editor. Save or undo them there first.");

	XmlDocument work;
	std::string error;
	if (!work.LoadFromString(source.doc->Serialize(), error))
		throw McpToolError(error);

	const TidyResult tidy = TidyDefaults(work);

	McpToolResult result;
	result.data = SourceJson(source);
	result.data["ok"] = tidy.ok;
	result.data["report"] = tidy.report;
	if (!tidy.ok)
		return result;

	result.data["defaultsBefore"] = tidy.defaultsBefore;
	result.data["profilesAfter"] = tidy.profilesAfter;

	json profiles = json::array();
	for (const tinyxml2::XMLElement* profile : Profiles(work))
		profiles.push_back({{"name", work.ProfileLabel(profile)}, {"kind", KindName(profile)}, {"users", ProfileUsers(work, profile).size()}});
	result.data["profiles"] = profiles;

	if (McpArgs::Bool(args, "preview", false)) {
		result.data["tidied"] = work.Serialize();
		return result;
	}

	const std::string target = McpArgs::Has(args, "path") ? McpFilePath(McpArgs::String(args, "path")) : source.path;
	if (target.empty())
		throw McpToolError("'" + source.link + "' doesn't come from a loose file (from " + source.from + "). Pass path to write the tidied file.");

	McpCheckOverwrite(target, args);
	if (!work.SaveToFile(target, error))
		throw McpToolError("Can't write '" + target + "': " + error);

	// What holds the old text reads the file again, and the preview rebuilds from it
	std::error_code ec;
	if (!source.path.empty() && std::filesystem::equivalent(std::filesystem::u8path(target), std::filesystem::u8path(source.path), ec))
		mcp.Session().PhysicsXmlChanged(source.link);

	result.data["path"] = target;
	return result;
}

json XmlArgument() {
	return McpStringList(
		"Physics XMLs: files (absolute or relative to the BodySlide folder) or links as a NIF writes them, found the way the "
		"preview finds them. Default: every XML the loaded shapes link.");
}

} // namespace

void AddPhysicsXmlTools(OutfitStudioMcp& mcp) {
	McpTool inspect;
	inspect.name = "inspect_physics_xml";
	inspect.description =
		"Reads SMP physics XMLs the way the builder does and reports what every bone and constraint is built with: its "
		"profile (the *-default it starts from) and its values, which of them it sets itself (own), and for values from "
		"another default than the profile (what it extends) which one (inherited). Resolves what is easy to misread in the XML: an unnamed *-default applies to what "
		"follows it, a named one is a snapshot of what it extends when declared, template names match in any case, and "
		"list entries (can-collide-with-bone) add up along the chain. Also lists the bones the loaded shapes' skins and "
		"the constraints create without a <bone> element (implicitBones, from the bone-default at that line), <bone> "
		"elements the builder skips because the bone exists by then (ignoredBoneElements), each profile with what uses "
		"it, the diagnostics, and needsTidy when defaults depend on their position (see tidy_physics_xml). Reads what "
		"the physics editor holds, unsaved edits included. Properties at their schema default are left out unless "
		"allProperties.";
	inspect.inputSchema = McpSchema({
		{"xml", XmlArgument()},
		{"bones", McpStringList("Only these bones, the constraints joining them and the implicit bones among them")},
		{"allProperties", McpProp("boolean", "Also list properties at their schema default. Default false.")},
	});
	inspect.readOnly = true;
	inspect.handler = [&mcp](const json& args) { return InspectPhysicsXml(mcp, args); };
	mcp.AddTool(std::move(inspect));

	McpTool collisions;
	collisions.name = "physics_collisions";
	collisions.description =
		"Which skinned shapes (per-vertex-shape, per-triangle-shape) of SMP physics XMLs are tested against each other and "
		"why, across files: shared (public, internal, external, private), tags, can-collide-with-tag and "
		"no-collide-with-tag, both ways. The filtering only: two bodies that are both kinematic never collide either.";
	collisions.inputSchema = McpSchema({
		{"xml", XmlArgument()},
		{"shape", McpProp("string", "Only the pairs with this skinned shape")},
	});
	collisions.readOnly = true;
	collisions.handler = [&mcp](const json& args) { return PhysicsCollisions(mcp, args); };
	mcp.AddTool(std::move(collisions));

	McpTool tidy;
	tidy.name = "tidy_physics_xml";
	tidy.description =
		"Rewrites the *-default elements of an SMP physics XML into profiles that don't depend on where they are: an "
		"unnamed Default per kind at the top, then named profiles that hold what they change about it, and a template on "
		"every bone and constraint that uses another. Every bone, constraint and implicitly created bone keeps exactly its "
		"values, which is checked before anything is written; it refuses when bones created without a <bone> would start "
		"from different defaults at different lines. Comments inside the defaults are lost. preview returns the result "
		"without writing. Writes over the source file (overwrite: true) or to path; the physics editor and the preview "
		"pick the new file up.";
	tidy.inputSchema = McpSchema({
		{"xml", McpProp("string", "The XML, as a file or a link. Default: the one the loaded shapes link.")},
		{"path", McpProp("string", "File to write. Default: the file the XML was read from.")},
		{"preview", McpProp("boolean", "Only report, and return the tidied XML. Default false.")},
		{"overwrite", McpProp("boolean", "Replace an existing file. Default false.")},
	});
	tidy.destructive = true;
	tidy.handler = [&mcp](const json& args) { return TidyPhysicsXml(mcp, args); };
	mcp.AddTool(std::move(tidy));
}
