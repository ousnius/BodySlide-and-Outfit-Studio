/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "AddOnSliders.h"
#include "Automorph.h"
#include "SliderSetIndex.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <set>
#include <wx/dir.h>
#include <wx/intl.h>
#include <wx/string.h>

using namespace nifly;

namespace {
// Paths of the project files compare the same with either slash and in any case
bool SamePathText(const std::string& path1, const std::string& path2) {
	return StringsEqualInsens(ToOSSlashes(path1).c_str(), ToOSSlashes(path2).c_str());
}

void AddWarning(std::vector<std::string>& warnings, const wxString& warning) {
	warnings.push_back(warning.ToUTF8().data());
}

constexpr size_t MaxStackDepth = 8;
} // namespace

void ProjectCatalog::Clear() {
	entries.clear();
	byName.clear();
	byID.clear();
}

bool ProjectCatalog::Add(SliderSetFile& file, const std::string& setName) {
	// BodySlide lists the first set of a name, in any case
	if (byName.find(setName) != byName.end())
		return false;

	ProjectCatalogEntry entry;
	entry.fileName = file.fileName;
	entry.setName = setName;
	entry.projectID = file.GetSetProjectID(setName);
	entry.baseName = file.GetSetBaseProjectName(setName);
	entry.baseID = file.GetSetBaseProjectID(setName);
	entry.addOn = file.IsAddOn(setName);
	file.GetSetSourceFile(setName, entry.dataFolder, entry.sourceFile);
	file.GetSetOutputFilePath(setName, entry.outputFilePath);

	size_t index = entries.size();
	byName[setName] = index;
	if (!entry.projectID.empty() && byID.find(entry.projectID) == byID.end())
		byID[entry.projectID] = index;

	entries.push_back(std::move(entry));
	return true;
}

void ProjectCatalog::Load(const std::string& projectPath) {
	Clear();

	wxString sliderSetsPath = wxString::FromUTF8(projectPath + PathSepStr + "SliderSets");
	if (!wxDir::Exists(sliderSetsPath))
		return;

	wxArrayString files;
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.osp");
	wxDir::GetAllFiles(sliderSetsPath, &files, "*.xml");

	for (auto& file : files) {
		SliderSetFile sliderSetFile(file.ToUTF8().data());
		if (sliderSetFile.fail())
			continue;

		std::vector<std::string> setNames;
		sliderSetFile.GetSetNamesUnsorted(setNames, false);
		for (auto& setName : setNames)
			Add(sliderSetFile, setName);
	}

	Finish();
}

void ProjectCatalog::Finish() {
	for (auto& entry : entries) {
		entry.baseIndex = npos;
		entry.stackable = false;
		if (entry.baseName.empty() && entry.baseID.empty())
			continue;

		// The project of the name if it has the ID too, since copies of a project file can share its ID. Otherwise the
		// ID finds a base project that was renamed.
		auto baseByName = byName.find(entry.baseName);
		auto baseByID = !entry.baseID.empty() ? byID.find(entry.baseID) : byID.end();
		if (baseByName != byName.end()) {
			const std::string& nameID = entries[baseByName->second].projectID;
			if (entry.baseID.empty() || nameID.empty() || nameID == entry.baseID || baseByID == byID.end())
				entry.baseIndex = baseByName->second;
		}

		if (entry.baseIndex == npos && baseByID != byID.end())
			entry.baseIndex = baseByID->second;
	}

	for (auto& entry : entries) {
		if (!entry.addOn || entry.baseIndex == npos || &entries[entry.baseIndex] == &entry)
			continue;

		// An add-on builds the shapes of its base project, but only goes with it if it writes the same file
		auto& base = entries[entry.baseIndex];
		bool sameSource = SamePathText(entry.dataFolder, base.dataFolder) && SamePathText(entry.sourceFile, base.sourceFile);
		bool sameOutput = SamePathText(entry.outputFilePath, base.outputFilePath);
		entry.stackable = sameSource && sameOutput;
	}
}

const ProjectCatalogEntry* ProjectCatalog::Find(const std::string& name, const std::string& id) const {
	// Copies of a project file can share the ID, so the project of the name comes first if the ID doesn't say otherwise
	auto byNameEntry = byName.find(name);
	auto byIDEntry = !id.empty() ? byID.find(id) : byID.end();
	if (byNameEntry != byName.end()) {
		const std::string& nameID = entries[byNameEntry->second].projectID;
		if (id.empty() || nameID.empty() || nameID == id || byIDEntry == byID.end())
			return &entries[byNameEntry->second];
	}

	return byIDEntry != byID.end() ? &entries[byIDEntry->second] : nullptr;
}

bool ProjectCatalog::IsInstalled(const std::string& name, const std::string& id) const {
	if (!id.empty() && byID.find(id) != byID.end())
		return true;

	return !name.empty() && byName.find(name) != byName.end();
}

std::vector<const ProjectCatalogEntry*> ProjectCatalog::AddOnsOf(const std::string& name, const std::string& id, bool stackableOnly) const {
	std::vector<size_t> found;

	// The project of the name, since copies of a project file can share its ID. The ID finds a renamed one.
	const ProjectCatalogEntry* root = Find(name);
	if (!root && !id.empty())
		root = Find("", id);
	if (!root)
		return {};

	std::set<size_t> parents{static_cast<size_t>(root - entries.data())};
	for (size_t depth = 0; depth < MaxStackDepth; depth++) {
		std::set<size_t> next;
		for (size_t i = 0; i < entries.size(); i++) {
			auto& entry = entries[i];
			if (!entry.addOn || entry.baseIndex == npos || parents.find(entry.baseIndex) == parents.end())
				continue;
			if (std::find(found.begin(), found.end(), i) != found.end() || parents.find(i) != parents.end())
				continue;

			// Add-ons use the base project's data folder and NIF
			auto& base = entries[entry.baseIndex];
			if (!SamePathText(entry.dataFolder, base.dataFolder) || !SamePathText(entry.sourceFile, base.sourceFile))
				continue;
			if (stackableOnly && !entry.stackable)
				continue;

			found.push_back(i);
			next.insert(i);
		}

		if (next.empty())
			break;

		parents = std::move(next);
	}

	std::sort(found.begin(), found.end());

	std::vector<const ProjectCatalogEntry*> addOns;
	for (size_t i : found)
		addOns.push_back(&entries[i]);

	return addOns;
}

bool ProjectCatalog::IsStackedAddOn(const std::string& name, std::string* outBase) const {
	const ProjectCatalogEntry* entry = Find(name);
	if (!entry || !entry->stackable)
		return false;

	if (outBase)
		*outBase = StackRoot(name);

	return true;
}

std::string ProjectCatalog::StackRoot(const std::string& name) const {
	const ProjectCatalogEntry* entry = Find(name);
	if (!entry)
		return name;

	for (size_t depth = 0; depth < MaxStackDepth && entry->stackable && entry->baseIndex != npos; depth++)
		entry = &entries[entry->baseIndex];

	return entry->setName;
}

std::vector<StackedAddOn> StackAddOns(SliderSet& set,
									   const ProjectCatalog& catalog,
									   std::vector<std::string>& warnings,
									   const std::string& name,
									   const std::string& id,
									   bool stackableOnly,
									   bool newSlidersOnly) {
	std::vector<StackedAddOn> stacked;

	const std::string& rootName = name.empty() ? set.GetName() : name;
	const std::string& rootID = name.empty() ? set.GetProjectID() : id;

	std::map<std::string, std::unique_ptr<SliderSetFile>> files;

	for (auto* addOn : catalog.AddOnsOf(rootName, rootID, stackableOnly)) {
		auto& file = files[addOn->fileName];
		if (!file)
			file = std::make_unique<SliderSetFile>(addOn->fileName);

		SliderSet resolved;
		if (file->fail() || file->GetSet(addOn->setName, resolved) != 0) {
			AddWarning(warnings, wxString::Format(_("Add-on '%s' couldn't be loaded."), wxString::FromUTF8(addOn->setName)));
			continue;
		}

		for (auto& warning : resolved.GetResolveStatus().warnings)
			warnings.push_back(addOn->setName + ": " + warning);

		if (resolved.GetResolveStatus().baseMissing)
			continue;

		if (!resolved.GetRemovedSliders().empty())
			AddWarning(warnings,
					   wxString::Format(_("Add-on '%s' leaves out sliders of its base project, which stay when it's added to it: %s"),
										wxString::FromUTF8(addOn->setName),
										wxString::FromUTF8(JoinStrings(resolved.GetRemovedSliders(), ", "))));

		// Local data of an add-on is in its base project's data folder, which can be another one than the set's
		const bool sameDataFolder = SamePathText(resolved.GetDefaultDataFolder(), set.GetDefaultDataFolder());

		StackedAddOn info;
		info.setName = addOn->setName;
		info.projectID = addOn->projectID;
		info.fileName = addOn->fileName;
		for (auto shape = resolved.ShapesBegin(); shape != resolved.ShapesEnd(); ++shape)
			info.vertexCounts[shape->first] = shape->second.vertexCount;

		for (size_t i = 0; i < resolved.size(); i++) {
			const SliderData& slider = resolved[i];
			if (newSlidersOnly && set.SliderExists(slider.name))
				continue;

			std::vector<DiffInfo> ownData;
			for (auto& df : slider.dataFiles) {
				if (df.bInherited)
					continue;

				DiffInfo data = df;
				if (data.bLocal && !sameDataFolder) {
					data.bLocal = false;
					set.AddInheritedDataFolders(data.targetName, {resolved.GetDefaultDataFolder()});
				}
				else if (!data.bLocal)
					set.AddInheritedDataFolders(data.targetName, resolved.GetTargetDataFolders(data.targetName));

				ownData.push_back(std::move(data));
			}

			const bool ownProperties = !slider.bInherited;
			if (ownData.empty() && !ownProperties)
				continue;

			if (!set.SliderExists(slider.name)) {
				if (ownData.empty())
					continue;

				SliderData newSlider = slider;
				newSlider.bInherited = false;
				newSlider.dataFiles = std::move(ownData);
				newSlider.stackedFrom = addOn->setName;
				set.CopySlider(&newSlider);
				continue;
			}

			// Changes of the add-on to a slider of the set take the place of what the set has
			SliderData& setSlider = set[slider.name];
			if (ownProperties)
				setSlider.CopyPropertiesFrom(slider);

			for (auto& df : ownData) {
				setSlider.dataFiles.erase(std::remove_if(setSlider.dataFiles.begin(),
														 setSlider.dataFiles.end(),
														 [&df](const DiffInfo& setDf) { return setDf.targetName == df.targetName; }),
										  setSlider.dataFiles.end());
				setSlider.AddDataFile(df.targetName, df.dataName, df.fileName, df.bLocal);
			}
		}

		stacked.push_back(std::move(info));
	}

	return stacked;
}

std::vector<std::string> RemoveUninstalledAddOnSliders(SliderSet& set, const ProjectCatalog& catalog) {
	std::vector<std::string> missingAddOns;
	std::vector<std::string> removedSliders;

	for (size_t i = 0; i < set.size(); i++) {
		const SliderData& slider = set[i];
		if (!slider.NeedsAddOn() || catalog.IsInstalled(slider.addOnName, slider.addOnID))
			continue;

		removedSliders.push_back(slider.name);

		const std::string& addOnName = slider.addOnName.empty() ? slider.addOnID : slider.addOnName;
		if (std::find(missingAddOns.begin(), missingAddOns.end(), addOnName) == missingAddOns.end())
			missingAddOns.push_back(addOnName);
	}

	for (auto& sliderName : removedSliders)
		set.DeleteSlider(sliderName);

	if (!removedSliders.empty()) {
		for (size_t i = 0; i < set.size(); i++)
			set[i].ForgetSliders([&removedSliders](const std::string& toggle) {
				return std::find(removedSliders.begin(), removedSliders.end(), toggle) != removedSliders.end();
			});
	}

	return missingAddOns;
}

ConformBodyRef FindConformBody(const SliderSet& outfit, const ProjectCatalog& catalog, const std::string& projectPath) {
	auto& entries = catalog.Entries();

	// The body of an add-on goes up to the project that the add-ons are of
	auto bodyOf = [&](const ProjectCatalogEntry* entry) {
		for (size_t depth = 0; depth < MaxStackDepth && entry && entry->addOn && entry->baseIndex != ProjectCatalog::npos; depth++)
			entry = &entries[entry->baseIndex];
		return entry;
	};

	// The body, its add-ons and variants have the body's sliders
	const ProjectCatalogEntry* outfitEntry = catalog.Find(outfit.GetName());
	auto isOfBody = [&](const ProjectCatalogEntry* body) {
		if (outfit.GetName() == body->setName)
			return true;

		const ProjectCatalogEntry* entry = outfitEntry;
		for (size_t depth = 0; depth < MaxStackDepth && entry; depth++) {
			if (entry == body)
				return true;

			entry = entry->baseIndex != ProjectCatalog::npos ? &entries[entry->baseIndex] : nullptr;
		}

		return outfit.HasBaseProject() && outfit.GetBaseProjectName() == body->setName;
	};

	auto makeRef = [&](const ProjectCatalogEntry* body, const std::string& shapeName, bool fromReferenceInfo) {
		ConformBodyRef ref;
		if (!body || isOfBody(body))
			return ref;

		ref.fileName = body->fileName;
		ref.setName = body->setName;
		ref.projectID = body->projectID;
		ref.shapeName = shapeName;
		ref.fromReferenceInfo = fromReferenceInfo;
		return ref;
	};

	if (outfit.HasExternalReferenceInfo()) {
		const ProjectCatalogEntry* reference = catalog.Find(outfit.GetReferenceProjectName());

		// A set of the same name in the file the outfit names comes first
		std::filesystem::path refFile = std::filesystem::u8path(outfit.GetReferenceProjectFile());
		if (refFile.is_relative())
			refFile = std::filesystem::u8path(projectPath) / refFile;

		if (reference && !SliderSetIndex::SameFile(reference->fileName, refFile.u8string())) {
			SliderSetFile file(refFile.u8string());
			if (!file.fail() && file.HasSet(outfit.GetReferenceProjectName())) {
				// Not the set BodySlide lists, so its add-ons can't be found
				return {};
			}
		}

		return makeRef(bodyOf(reference), outfit.GetReferenceShapeName(), true);
	}

	// Without reference info, a shape whose data folders have a body's data folder was made with that body
	std::vector<size_t> bodies;
	for (auto& entry : entries) {
		if (!entry.addOn || entry.baseIndex == ProjectCatalog::npos)
			continue;

		const ProjectCatalogEntry* body = bodyOf(&entry);
		size_t bodyIndex = body - entries.data();
		if (!body->addOn && std::find(bodies.begin(), bodies.end(), bodyIndex) == bodies.end())
			bodies.push_back(bodyIndex);
	}

	// Variants of those bodies get their add-on sliders
	const size_t bodiesWithAddOns = bodies.size();
	for (size_t i = 0; i < entries.size(); i++) {
		const ProjectCatalogEntry* entry = &entries[i];
		if (entry->addOn || entry->baseIndex == ProjectCatalog::npos || std::find(bodies.begin(), bodies.end(), i) != bodies.end())
			continue;

		for (size_t depth = 0; depth < MaxStackDepth && entry->baseIndex != ProjectCatalog::npos; depth++) {
			entry = &entries[entry->baseIndex];
			if (std::find(bodies.begin(), bodies.begin() + bodiesWithAddOns, static_cast<size_t>(entry - entries.data())) != bodies.begin() + bodiesWithAddOns) {
				bodies.push_back(i);
				break;
			}
		}
	}

	for (size_t bodyIndex : bodies) {
		const ProjectCatalogEntry* body = &entries[bodyIndex];
		if (isOfBody(body))
			continue;

		SliderSetFile file(body->fileName);
		SliderSet bodySet;
		if (file.fail() || file.GetSet(body->setName, bodySet, true, "", false) != 0)
			continue;

		for (auto shape = outfit.ShapesBegin(); shape != outfit.ShapesEnd(); ++shape) {
			auto& folders = shape->second.dataFolders;
			bool inBodyFolder = std::any_of(folders.begin(), folders.end(), [&](const std::string& folder) {
				return SamePathText(folder, body->dataFolder) && !SamePathText(folder, outfit.GetDefaultDataFolder());
			});
			if (!inBodyFolder)
				continue;

			std::string bodyShape = bodySet.TargetToShape(shape->second.targetShape);
			if (!bodyShape.empty())
				return makeRef(body, bodyShape, false);
		}
	}

	return {};
}

bool ConformBody::Load(const ConformBodyRef& bodyRef,
					   const ProjectCatalog& catalog,
					   const std::string& baseDataPath,
					   const std::function<bool(const std::string&, nifly::NifFile&)>& loadNif,
					   std::vector<std::string>& warnings) {
	ref = bodyRef;
	sliders.clear();
	addOnNames.clear();
	diffs.Clear();

	SliderSetFile file(bodyRef.fileName);
	SliderSet bodySet;
	if (file.fail() || file.GetSet(bodyRef.setName, bodySet) != 0) {
		AddWarning(warnings, wxString::Format(_("Body '%s' couldn't be loaded to conform its add-on sliders."), wxString::FromUTF8(bodyRef.setName)));
		return false;
	}

	bodySet.SetBaseDataPath(baseDataPath);

	// All add-ons of the body, even ones that build elsewhere, add sliders to its shapes. A body variant
	// gets the add-on sliders of its base body as well.
	SetStackResult stack = StackSetAddOns(bodySet, catalog, warnings, true);

	targetName = bodySet.ShapeToTarget(bodyRef.shapeName);
	if (targetName.empty())
		targetName = bodyRef.shapeName;

	auto hasAddOnSliders = [&]() {
		for (size_t i = 0; i < bodySet.size(); i++)
			if (!bodySet[i].stackedFrom.empty())
				return true;
		return false;
	};

	if (!hasAddOnSliders())
		return false;

	if (!loadNif(bodySet.GetInputFileName(), nif)) {
		AddWarning(warnings, wxString::Format(_("The NIF file of body '%s' couldn't be loaded: %s"), wxString::FromUTF8(bodyRef.setName), wxString::FromUTF8(bodySet.GetInputFileName())));
		return false;
	}

	if (!RefShape()) {
		AddWarning(warnings, wxString::Format(_("Shape '%s' of body '%s' wasn't found."), wxString::FromUTF8(bodyRef.shapeName), wxString::FromUTF8(bodyRef.setName)));
		return false;
	}

	LoadConformAnim(anim, nif);

	// The edited shapes of a body variant get the sliders conformed from the base body first
	if (!stack.baseAddOns.empty())
		FitBaseAddOnsToVariant(bodySet, nif, &anim, stack.baseAddOns, catalog, baseDataPath, loadNif, true, warnings);

	for (size_t i = 0; i < bodySet.size(); i++) {
		const SliderData& slider = bodySet[i];
		if (slider.stackedFrom.empty() || slider.bZap || slider.bUV)
			continue;

		std::string refDataName;
		for (auto& df : slider.dataFiles)
			if (df.targetName == targetName)
				refDataName = df.dataName;

		if (refDataName.empty())
			continue;

		Slider bodySlider;
		bodySlider.slider = slider;
		bodySlider.slider.dataFiles.clear();
		bodySlider.slider.curValue = 0.0f;
		bodySlider.slider.bAutoConformed = false;
		bodySlider.refDataName = refDataName;
		bodySlider.addOnName = slider.stackedFrom;
		sliders.push_back(std::move(bodySlider));

		if (std::find(addOnNames.begin(), addOnNames.end(), slider.stackedFrom) == addOnNames.end())
			addOnNames.push_back(slider.stackedFrom);
	}

	if (sliders.empty())
		return false;

	bodySet.LoadSetDiffData(diffs, bodyRef.shapeName);

	// Data is converted when it's first used, which mustn't happen on several threads
	for (auto& slider : sliders)
		diffs.GetDiffSet(slider.refDataName);

	return true;
}

SetStackResult StackSetAddOns(SliderSet& set, const ProjectCatalog& catalog, std::vector<std::string>& warnings, bool allAddOns) {
	SetStackResult result;
	result.addOns = StackAddOns(set, catalog, warnings, "", "", !allAddOns);

	// A variant has the sliders of its base projects' add-ons too, which are fit to its shapes later
	if (set.HasBaseProject() && !set.IsAddOn()) {
		const ProjectCatalogEntry* base = catalog.Find(set.GetBaseProjectName(), set.GetBaseProjectID());
		for (size_t depth = 0; depth < MaxStackDepth && base; depth++) {
			if (!base->addOn) {
				auto baseAddOns = StackAddOns(set, catalog, warnings, base->setName, base->projectID, !allAddOns, true);
				result.baseAddOns.insert(result.baseAddOns.end(), baseAddOns.begin(), baseAddOns.end());
			}

			base = base->baseIndex != ProjectCatalog::npos ? &catalog.Entries()[base->baseIndex] : nullptr;
		}
	}

	result.missingAddOns = RemoveUninstalledAddOnSliders(set, catalog);
	return result;
}

void LoadConformAnim(AnimInfo& anim, nifly::NifFile& nif) {
	// Loading skinning adds bones to the shared skeleton
	static std::mutex skeletonMutex;
	std::lock_guard<std::mutex> lock(skeletonMutex);
	anim.LoadFromNif(&nif);
}

std::map<std::string, std::map<std::string, TargetDataDiffs>> ConformShapesData(nifly::NifFile& refNif,
																				nifly::NiShape* refShape,
																				const AnimInfo& refAnim,
																				DiffDataSets& refDiffs,
																				nifly::NifFile& nif,
																				const AnimInfo& anim,
																				const std::vector<std::string>& shapeNames,
																				const std::vector<std::pair<std::string, std::string>>& sliders,
																				const ConformOptions& options) {
	std::map<std::string, std::map<std::string, TargetDataDiffs>> results;
	if (!refShape || shapeNames.empty() || sliders.empty())
		return results;

	Automorph morpher;
	morpher.SetRef(refNif, refShape, &refAnim);
	morpher.LinkRefDiffData(&refDiffs);
	morpher.SourceShapesFromNif(nif, &anim);

	for (auto& shapeName : shapeNames) {
		morpher.BuildProximityCache(shapeName, options.proximityRadius);

		for (auto& slider : sliders) {
			morpher.GenerateResultDiff(shapeName,
									   slider.first,
									   slider.second,
									   true,
									   options.maxResults,
									   options.noSqueeze,
									   options.solidMode,
									   options.axisX,
									   options.axisY,
									   options.axisZ,
									   shapeName);
			if (options.smoothResultDeltas)
				morpher.SmoothResultDiff(shapeName, slider.first, shapeName, options.smoothIterations, options.smoothStrength);

			TargetDataDiffs diff;
			morpher.GetRawResultDiff(shapeName, slider.first, diff);
			if (!diff.empty())
				results[shapeName][slider.first] = std::move(diff);
		}

		morpher.ClearProximityCache();
	}

	morpher.UnlinkRefDiffData();
	return results;
}

std::vector<std::string> ConformAddOnSliders(SliderSet& set, nifly::NifFile& setNif, const AnimInfo& setAnim, ConformBody& body, const ConformOptions& options) {
	std::vector<std::string> usedAddOns;

	NiShape* refShape = body.RefShape();
	if (!refShape)
		return usedAddOns;

	// Each conform has data of its own, so that outfits can be conformed on several threads
	DiffDataSets refDiffs = body.diffs;

	std::vector<const ConformBody::Slider*> conformSliders;
	std::vector<std::pair<std::string, std::string>> sliderData;
	for (auto& slider : body.sliders) {
		if (set.SliderExists(slider.slider.name) || !refDiffs.GetDiffSet(slider.refDataName))
			continue;

		conformSliders.push_back(&slider);
		sliderData.emplace_back(slider.slider.name, slider.refDataName);
	}

	if (conformSliders.empty())
		return usedAddOns;

	// The body shape of an outfit takes the body's data as it is
	const size_t refVertexCount = refShape->GetNumVertices();
	std::vector<std::pair<std::string, std::string>> directShapes; // Shape and target
	std::vector<std::string> conformShapes;
	std::map<std::string, std::string> shapeTargets;
	for (auto shape = set.ShapesBegin(); shape != set.ShapesEnd(); ++shape) {
		NiShape* nifShape = setNif.FindBlockByName<NiShape>(shape->first);
		if (!nifShape)
			continue;

		shapeTargets[shape->first] = shape->second.targetShape;
		if (shape->second.targetShape == body.targetName && nifShape->GetNumVertices() == refVertexCount)
			directShapes.emplace_back(shape->first, shape->second.targetShape);
		else
			conformShapes.push_back(shape->first);
	}

	auto results = ConformShapesData(body.nif, refShape, body.anim, refDiffs, setNif, setAnim, conformShapes, sliderData, options);

	auto generated = std::make_shared<GeneratedSliderData>();
	if (set.GetGeneratedData())
		*generated = *set.GetGeneratedData();

	std::set<std::string> dataNames;
	for (size_t i = 0; i < set.size(); i++)
		for (auto& df : set[i].dataFiles)
			dataNames.insert(df.dataName);
	for (auto& data : generated->data)
		dataNames.insert(data.first);

	auto uniqueDataName = [&dataNames](const std::string& name) {
		std::string dataName = name;
		for (int i = 2; dataNames.find(dataName) != dataNames.end(); i++)
			dataName = name + "_addon" + std::to_string(i);

		dataNames.insert(dataName);
		return dataName;
	};

	for (auto* slider : conformSliders) {
		SliderData newSlider = slider->slider;
		newSlider.dataFiles.clear();
		newSlider.bInherited = false;
		newSlider.bAutoConformed = true;
		newSlider.stackedFrom = slider->addOnName;

		auto addData = [&](const std::string& targetName, const TargetDataDiffs& diffs) {
			std::string dataName = uniqueDataName(targetName + newSlider.name);
			generated->data[dataName] = {targetName, diffs};
			newSlider.AddDataFile(targetName, dataName, "", false);
			newSlider.dataFiles.back().bGenerated = true;
		};

		for (auto& shape : directShapes)
			if (TargetDataDiffs* diffs = refDiffs.GetDiffSet(slider->refDataName))
				addData(shape.second, *diffs);

		for (auto& shapeResults : results) {
			auto result = shapeResults.second.find(newSlider.name);
			if (result != shapeResults.second.end())
				addData(shapeTargets[shapeResults.first], result->second);
		}

		if (newSlider.dataFiles.empty())
			continue;

		set.CopySlider(&newSlider);
		if (std::find(usedAddOns.begin(), usedAddOns.end(), slider->addOnName) == usedAddOns.end())
			usedAddOns.push_back(slider->addOnName);
	}

	set.SetGeneratedData(generated);
	return usedAddOns;
}

void FitBaseAddOnsToVariant(SliderSet& set,
							nifly::NifFile& nif,
							const AnimInfo* anim,
							const std::vector<StackedAddOn>& baseAddOns,
							const ProjectCatalog& catalog,
							const std::string& baseDataPath,
							const std::function<bool(const std::string&, nifly::NifFile&)>& loadNif,
							bool conform,
							std::vector<std::string>& warnings,
							const ConformOptions& options) {
	std::shared_ptr<GeneratedSliderData> generated;

	std::set<std::string> dataNames;
	for (size_t i = 0; i < set.size(); i++)
		for (auto& df : set[i].dataFiles)
			dataNames.insert(df.dataName);

	for (auto& addOn : baseAddOns) {
		std::vector<std::string> mismatchedShapes;
		for (auto& vertexCount : addOn.vertexCounts) {
			NiShape* shape = nif.FindBlockByName<NiShape>(vertexCount.first);
			if (shape && vertexCount.second > 0 && vertexCount.second != shape->GetNumVertices())
				mismatchedShapes.push_back(vertexCount.first);
		}

		if (mismatchedShapes.empty())
			continue;

		// The base project with its add-ons stacked has the data the shapes are conformed from
		const ProjectCatalogEntry* addOnEntry = catalog.Find(addOn.setName, addOn.projectID);
		const ProjectCatalogEntry* baseEntry = addOnEntry && addOnEntry->baseIndex != ProjectCatalog::npos ? &catalog.Entries()[addOnEntry->baseIndex] : nullptr;

		SliderSet baseSet;
		DiffDataSets baseDiffs;
		NifFile baseNif;
		AnimInfo baseAnim;
		bool canConform = conform && anim && baseEntry;
		if (canConform) {
			SliderSetFile baseFile(baseEntry->fileName);
			canConform = !baseFile.fail() && baseFile.GetSet(baseEntry->setName, baseSet) == 0;
			if (canConform) {
				baseSet.SetBaseDataPath(baseDataPath);
				StackAddOns(baseSet, catalog, warnings, baseEntry->setName, baseEntry->projectID);
				for (auto& shapeName : mismatchedShapes)
					baseSet.LoadSetDiffData(baseDiffs, shapeName);
				canConform = loadNif(baseSet.GetInputFileName(), baseNif);
			}

			if (canConform)
				LoadConformAnim(baseAnim, baseNif);
		}

		for (auto& shapeName : mismatchedShapes) {
			const std::string target = set.ShapeToTarget(shapeName);
			NiShape* refShape = canConform ? baseNif.FindBlockByName<NiShape>(shapeName) : nullptr;
			const std::string refTarget = baseSet.ShapeToTarget(shapeName);

			std::vector<std::pair<std::string, std::string>> sliderData;
			for (size_t i = 0; i < set.size(); i++) {
				SliderData& slider = set[i];
				if (slider.stackedFrom != addOn.setName)
					continue;

				slider.dataFiles.erase(std::remove_if(slider.dataFiles.begin(), slider.dataFiles.end(), [&target](const DiffInfo& df) { return df.targetName == target; }),
									   slider.dataFiles.end());

				if (refShape && baseSet.SliderExists(slider.name)) {
					std::string refDataName = baseSet[slider.name].TargetDataName(refTarget);
					if (!refDataName.empty() && baseDiffs.GetDiffSet(refDataName))
						sliderData.emplace_back(slider.name, refDataName);
				}
			}

			if (!refShape) {
				AddWarning(warnings,
						   wxString::Format(_("Shape '%s' has another vertex count than add-on '%s' was made for, so its sliders leave it out."),
											wxString::FromUTF8(shapeName),
											wxString::FromUTF8(addOn.setName)));
				continue;
			}

			auto results = ConformShapesData(baseNif, refShape, baseAnim, baseDiffs, nif, *anim, {shapeName}, sliderData, options);
			auto shapeResults = results.find(shapeName);
			if (shapeResults == results.end())
				continue;

			if (!generated) {
				generated = std::make_shared<GeneratedSliderData>();
				if (set.GetGeneratedData())
					*generated = *set.GetGeneratedData();
				for (auto& data : generated->data)
					dataNames.insert(data.first);
			}

			for (auto& result : shapeResults->second) {
				std::string dataName = target + result.first;
				for (int n = 2; dataNames.find(dataName) != dataNames.end(); n++)
					dataName = target + result.first + "_addon" + std::to_string(n);
				dataNames.insert(dataName);

				generated->data[dataName] = {target, std::move(result.second)};

				SliderData& slider = set[result.first];
				slider.AddDataFile(target, dataName, "", false);
				slider.dataFiles.back().bGenerated = true;
				slider.bAutoConformed = true;
			}
		}
	}

	if (generated)
		set.SetGeneratedData(generated);

	std::vector<std::string> emptySliders;
	for (size_t i = 0; i < set.size(); i++)
		if (!set[i].stackedFrom.empty() && set[i].dataFiles.empty() && !set[i].KeptWithoutData())
			emptySliders.push_back(set[i].name);

	for (auto& sliderName : emptySliders)
		set.DeleteSlider(sliderName);
}
