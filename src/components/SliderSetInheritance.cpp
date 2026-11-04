/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "SliderSetInheritance.h"
#include "../utils/PlatformUtil.h"
#include "../utils/ProjectUtil.h"
#include "../utils/StringStuff.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <wx/intl.h>
#include <wx/string.h>

namespace {
void AddWarning(SliderSetResolveStatus& status, const wxString& warning) {
	status.warnings.push_back(warning.ToUTF8().data());
}
} // namespace

void SliderSet::InheritFrom(const SliderSet& base) {
	SliderSetResolveStatus status = resolveStatus;
	status.baseFound = true;
	status.baseMissing = false;
	status.baseDataFolder = base.datafolder;
	status.baseInputFile = base.inputfile;
	status.baseSliders.clear();
	status.baseData.clear();
	status.baseDataFolders.clear();

	// The geometry of an add-on is the base project's
	if (addOn) {
		datafolder = base.datafolder;
		inputfile = base.inputfile;

		auto shapes = base.shapeAttributes;
		for (auto& shape : shapes) {
			auto ownShape = shapeAttributes.find(shape.first);
			shape.second.vertexCount = ownShape != shapeAttributes.end() ? ownShape->second.vertexCount : 0;
		}
		shapeAttributes = std::move(shapes);

		refProjectFile = base.refProjectFile;
		refProjectName = base.refProjectName;
		refShapeName = base.refShapeName;
	}

	const bool sameDataFolder = StringsEqualInsens(ToOSSlashes(datafolder).c_str(), ToOSSlashes(base.datafolder).c_str());

	std::set<std::string> targets;
	for (auto& shape : shapeAttributes)
		targets.insert(shape.second.targetShape);

	std::map<std::string, const SliderData*> baseSliders;
	for (auto& baseSlider : base.sliders) {
		baseSliders[baseSlider.name] = &baseSlider;
		status.baseSliders.insert(baseSlider.name);
	}

	// Data names are unique across the sliders, the project's own data keeps its names
	std::set<std::string> ownDataNames;
	for (auto& slider : sliders)
		for (auto& df : slider.dataFiles)
			if (!df.bInherited)
				ownDataNames.insert(df.dataName);

	// Own data first takes the place of the base project's data for the same shape
	auto mergeData = [&](const std::string& sliderName, const std::vector<DiffInfo>& ownData, const std::vector<DiffInfo>& baseData) {
		std::vector<DiffInfo> merged;
		std::vector<bool> ownUsed(ownData.size(), false);

		for (auto& baseDataFile : baseData) {
			if (targets.find(baseDataFile.targetName) == targets.end())
				continue;

			// Inherited data uses the data folders the base project finds it in
			DiffInfo inherited = baseDataFile;
			inherited.bInherited = true;

			std::vector<std::string> inheritedFolders;
			if (!inherited.bLocal)
				inheritedFolders = base.GetTargetDataFolders(inherited.targetName);
			else if (!sameDataFolder) {
				inherited.bLocal = false;
				inheritedFolders.push_back(base.datafolder);
			}

			status.baseData[std::make_pair(sliderName, inherited.targetName)] = inherited;
			auto& targetFolders = status.baseDataFolders[inherited.targetName];
			for (auto& dataFolder : inheritedFolders)
				if (std::find(targetFolders.begin(), targetFolders.end(), dataFolder) == targetFolders.end())
					targetFolders.push_back(dataFolder);

			auto own = std::find_if(ownData.begin(), ownData.end(), [&](const DiffInfo& df) { return df.targetName == baseDataFile.targetName; });
			if (own != ownData.end()) {
				size_t ownIndex = own - ownData.begin();
				if (!ownUsed[ownIndex]) {
					merged.push_back(*own);
					ownUsed[ownIndex] = true;
				}
				continue;
			}

			if (ownDataNames.find(baseDataFile.dataName) != ownDataNames.end()) {
				AddWarning(status,
						   wxString::Format(_("Slider data '%s' of slider '%s' in the base project has the name of data of this project, so it's left out."),
											wxString::FromUTF8(baseDataFile.dataName),
											wxString::FromUTF8(sliderName)));
				continue;
			}

			AddInheritedDataFolders(inherited.targetName, inheritedFolders);
			merged.push_back(std::move(inherited));
		}

		for (size_t i = 0; i < ownData.size(); i++)
			if (!ownUsed[i])
				merged.push_back(ownData[i]);

		return merged;
	};

	// Only base sliders that the set doesn't have stay left out
	std::vector<std::string> keptRemovedSliders;
	for (auto& removedSlider : removedSliders)
		if (baseSliders.find(removedSlider) != baseSliders.end() && !SliderExists(removedSlider))
			keptRemovedSliders.push_back(removedSlider);

	std::vector<SliderData> mergedSliders;
	for (auto& slider : sliders) {
		std::vector<DiffInfo> ownData;
		for (auto& df : slider.dataFiles)
			if (!df.bInherited)
				ownData.push_back(df);

		auto baseSlider = baseSliders.find(slider.name);
		if (baseSlider == baseSliders.end()) {
			if (slider.bInherited || slider.HasInheritedData()) {
				AddWarning(status, wxString::Format(_("Slider '%s' isn't in the base project anymore."), wxString::FromUTF8(slider.name)));
				if (ownData.empty())
					continue;
			}

			SliderData ownSlider = slider;
			ownSlider.bInherited = false;
			ownSlider.dataFiles = std::move(ownData);
			mergedSliders.push_back(std::move(ownSlider));
			continue;
		}

		for (auto& df : slider.dataFiles) {
			if (!df.bInherited || targets.find(df.targetName) == targets.end())
				continue;

			auto& baseData = baseSlider->second->dataFiles;
			if (std::none_of(baseData.begin(), baseData.end(), [&](const DiffInfo& baseDf) { return baseDf.targetName == df.targetName; }))
				AddWarning(status,
						   wxString::Format(_("The base project doesn't have data of slider '%s' for shape '%s' anymore."),
											wxString::FromUTF8(slider.name),
											wxString::FromUTF8(TargetToShape(df.targetName))));
		}

		SliderData mergedSlider = slider;
		if (slider.bInherited)
			mergedSlider.CopyPropertiesFrom(*baseSlider->second);

		mergedSlider.dataFiles = mergeData(slider.name, ownData, baseSlider->second->dataFiles);
		mergedSliders.push_back(std::move(mergedSlider));
	}

	// Sliders that were added to the base project since go after the base slider before them
	auto findMerged = [&](const std::string& sliderName) {
		return std::find_if(mergedSliders.begin(), mergedSliders.end(), [&](const SliderData& s) { return s.name == sliderName; });
	};

	for (size_t i = 0; i < base.sliders.size(); i++) {
		auto& baseSlider = base.sliders[i];
		if (IsRemovedSlider(baseSlider.name) || findMerged(baseSlider.name) != mergedSliders.end())
			continue;

		SliderData newSlider(baseSlider.name);
		newSlider.CopyPropertiesFrom(baseSlider);
		newSlider.bInherited = true;
		newSlider.dataFiles = mergeData(baseSlider.name, {}, baseSlider.dataFiles);
		if (newSlider.dataFiles.empty() && !newSlider.KeptWithoutData())
			continue;

		auto insertAt = mergedSliders.end();
		bool foundBefore = false;
		for (size_t j = i; j > 0; j--) {
			auto before = findMerged(base.sliders[j - 1].name);
			if (before != mergedSliders.end()) {
				insertAt = before + 1;
				foundBefore = true;
				break;
			}
		}

		if (!foundBefore) {
			insertAt = std::find_if(mergedSliders.begin(), mergedSliders.end(), [&](const SliderData& s) { return baseSliders.find(s.name) != baseSliders.end(); });
			if (insertAt == mergedSliders.end())
				insertAt = mergedSliders.begin();
		}

		mergedSliders.insert(insertAt, std::move(newSlider));
	}

	// Zaps and toggles can only toggle sliders that are still there
	for (auto& slider : mergedSliders)
		slider.ForgetSliders([&](const std::string& toggle) { return findMerged(toggle) == mergedSliders.end(); });

	sliders = std::move(mergedSliders);
	removedSliders = std::move(keptRemovedSliders);
	resolveStatus = std::move(status);
}

void SliderSet::MakeDerivedFrom(const std::string& projectFile, const std::string& projectName, bool asAddOn) {
	SliderSetResolveStatus status;
	status.baseFound = true;
	status.baseDataFolder = datafolder;
	status.baseInputFile = inputfile;

	for (auto& slider : sliders) {
		slider.bInherited = true;
		status.baseSliders.insert(slider.name);

		for (auto& df : slider.dataFiles) {
			df.bInherited = true;
			status.baseData[std::make_pair(slider.name, df.targetName)] = df;
			if (!df.bLocal)
				status.baseDataFolders[df.targetName] = GetTargetDataFolders(df.targetName);
		}
	}

	baseProjectFile = projectFile;
	baseProjectName = projectName;
	baseProjectID = projectID;
	projectID.clear();
	addOn = asAddOn;
	sliderDataFile.clear();
	removedSliders.clear();
	notes.clear();
	resolveStatus = std::move(status);
}

bool SliderSet::BaseHasSlider(const std::string& sliderName) {
	if (resolveStatus.baseFound)
		return resolveStatus.baseSliders.find(sliderName) != resolveStatus.baseSliders.end();

	if (!SliderExists(sliderName))
		return false;

	auto& slider = (*this)[sliderName];
	return HasBaseProject() && (slider.bInherited || slider.HasInheritedData());
}

const DiffInfo* SliderSet::BaseData(const std::string& sliderName, const std::string& targetName) const {
	auto data = resolveStatus.baseData.find(std::make_pair(sliderName, targetName));
	return data != resolveStatus.baseData.end() ? &data->second : nullptr;
}

void SliderSet::AddInheritedDataFolders(const std::string& targetName, const std::vector<std::string>& dataFolders) {
	if (dataFolders.empty())
		return;

	for (auto& shape : shapeAttributes) {
		if (shape.second.targetShape != targetName)
			continue;

		// No data folders of its own stands for the default one, which comes first
		if (shape.second.dataFolders.empty())
			shape.second.dataFolders.push_back(datafolder);

		for (auto& dataFolder : dataFolders)
			if (std::find(shape.second.dataFolders.begin(), shape.second.dataFolders.end(), dataFolder) == shape.second.dataFolders.end())
				shape.second.dataFolders.push_back(dataFolder);
	}
}

SliderSetResolver::SliderSetResolver(const std::string& inProjectPath)
	: projectPath(inProjectPath) {}

std::string SliderSetResolver::ProjectPathOfFile(const std::string& fileName) {
	std::error_code error;
	std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(fileName), error);
	if (!error) {
		for (auto dir = path.parent_path(); dir.has_relative_path(); dir = dir.parent_path())
			if (StringsEqualInsens(dir.filename().u8string().c_str(), "SliderSets"))
				return dir.parent_path().u8string();
	}

	return ProjectUtil::GetProjectPath();
}

SliderSetFile* SliderSetResolver::OpenFile(const std::string& fileName) {
	auto openFile = openFiles.find(fileName);
	if (openFile != openFiles.end())
		return openFile->second.get();

	auto file = std::make_unique<SliderSetFile>(fileName);
	if (file->fail())
		file.reset();

	return (openFiles[fileName] = std::move(file)).get();
}

const SliderSetIndex& SliderSetResolver::Index() {
	if (!index)
		index = std::make_unique<SliderSetIndex>(projectPath);

	return *index;
}

SliderSetFile* SliderSetResolver::FindBaseProject(const SliderSet& set, SliderSetFile& fromFile, std::string& outSetName) {
	const std::string& baseID = set.GetBaseProjectID();
	const std::string& baseName = set.GetBaseProjectName();

	std::vector<SliderSetFile*> likelyFiles;
	if (!set.GetBaseProjectFile().empty()) {
		std::filesystem::path hint = std::filesystem::u8path(set.GetBaseProjectFile());
		if (hint.is_relative())
			hint = std::filesystem::u8path(projectPath) / hint;

		if (SliderSetFile* hintFile = OpenFile(hint.u8string()))
			likelyFiles.push_back(hintFile);
	}
	likelyFiles.push_back(&fromFile);

	// The ID still finds a base project that was renamed
	if (!baseID.empty()) {
		for (auto* file : likelyFiles) {
			std::string setName = file->FindSetByID(baseID);
			if (!setName.empty()) {
				outSetName = setName;
				return file;
			}
		}

		for (auto& entry : Index().Entries()) {
			if (entry.set.GetProjectID() == baseID) {
				if (SliderSetFile* file = OpenFile(entry.fileName)) {
					outSetName = entry.setName;
					return file;
				}
			}
		}
	}

	for (auto* file : likelyFiles) {
		if (file->HasSet(baseName)) {
			outSetName = baseName;
			return file;
		}
	}

	if (const SliderSetIndexEntry* entry = Index().Find(baseName)) {
		if (SliderSetFile* file = OpenFile(entry->fileName)) {
			outSetName = baseName;
			return file;
		}
	}

	return nullptr;
}

int SliderSetResolver::Resolve(SliderSetFile& file, const std::string& setName, SliderSet& outSet) {
	return Resolve(file, setName, outSet, 0);
}

int SliderSetResolver::Resolve(SliderSetFile& file, const std::string& setName, SliderSet& outSet, size_t depth) {
	SliderSet stored;
	int error = file.GetSet(setName, stored, true, "", false);
	if (error)
		return error;

	if (!stored.HasBaseProject()) {
		outSet = std::move(stored);
		return 0;
	}

	SliderSetResolveStatus status;

	auto key = std::make_pair(file.fileName, setName);
	if (depth >= MaxDepth || std::find(resolving.begin(), resolving.end(), key) != resolving.end()) {
		status.baseMissing = true;
		AddWarning(status, wxString::Format(_("The base projects of '%s' name each other, so it's used as it was saved."), wxString::FromUTF8(setName)));
		outSet = std::move(stored);
		outSet.SetResolveStatus(status);
		return 0;
	}

	std::string baseSetName;
	SliderSetFile* baseFile = FindBaseProject(stored, file, baseSetName);
	if (!baseFile) {
		status.baseMissing = true;
		AddWarning(status,
				   wxString::Format(_("Base project '%s' of '%s' wasn't found, so it's used as it was saved."),
									wxString::FromUTF8(stored.GetBaseProjectName()),
									wxString::FromUTF8(setName)));
		outSet = std::move(stored);
		outSet.SetResolveStatus(status);
		return 0;
	}

	resolving.push_back(key);
	SliderSet base;
	error = Resolve(*baseFile, baseSetName, base, depth + 1);
	resolving.pop_back();

	if (error) {
		status.baseMissing = true;
		AddWarning(status, wxString::Format(_("Base project '%s' couldn't be loaded, so it's used as it was saved."), wxString::FromUTF8(baseSetName)));
		outSet = std::move(stored);
		outSet.SetResolveStatus(status);
		return 0;
	}

	outSet = std::move(stored);
	outSet.InheritFrom(base);

	// What happened further down the chain matters as well
	status = outSet.GetResolveStatus();
	for (auto& warning : base.GetResolveStatus().warnings)
		status.warnings.push_back(baseSetName + ": " + warning);
	outSet.SetResolveStatus(status);

	return 0;
}
