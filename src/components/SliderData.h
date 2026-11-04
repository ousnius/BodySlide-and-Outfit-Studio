/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <tinyxml2.h>

#include "DiffData.h"
#include "../utils/StringStuff.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

using namespace tinyxml2;

// Text of an element, empty if it has none
inline std::string ElementText(const XMLElement* element) {
	const char* text = element->GetText();
	return text ? text : "";
}

// Value of an attribute, empty if it's missing
inline std::string AttributeText(const XMLElement* element, const char* name) {
	const char* value = element->Attribute(name);
	return value ? value : "";
}

struct DiffInfo {
	bool bLocal;			// Local files use the slider set's directory for path info. Otherwise, it uses the target's data path.
	bool bInherited;		// Comes from the base project, which owns the data.
	std::string dataName;	// Alias for the data.
	std::string targetName; // Shape affected by the data.
	std::string fileName;	// File name not including path.
	bool bGenerated = false; // BodySlide: made when loading, from the generated data of the set. Never saved.
	DiffInfo(bool l = false, const std::string& dn = "", const std::string& tn = "", const std::string& fn = "", bool inherited = false)
		: bLocal(l)
		, bInherited(inherited)
		, dataName(dn)
		, targetName(tn)
		, fileName(fn) {}
};

// A slider that a toggle sets while it's ticked, with the value in percent
struct ToggleTarget {
	std::string slider;
	float value = 100.0f;

	bool operator==(const ToggleTarget& other) const { return slider == other.slider && value == other.value; }
};

class SliderData {
public:
	std::string name;
	bool bHidden = false;
	bool bInvert = false;
	bool bZap = false;
	bool bClamp = false;
	bool bUV = false;
	float defSmallValue = 0.0f;
	float defBigValue = 0.0f;
	std::vector<std::string> zapToggles;
	bool bToggle = false;				   // BodySlide shows the slider as a check box that applies it at the toggle value.
	float toggleValue = 100.0f;			   // Percent that the toggle applies the slider at when ticked
	std::vector<ToggleTarget> toggleSliders; // Other sliders and zaps that the toggle sets while ticked
	bool bInherited = false; // The properties above come from the base project.

	// The add-on project that the slider was made for. BodySlide leaves the slider out when the add-on isn't installed.
	std::string addOnName;
	std::string addOnID; // Project ID of the add-on, empty if it has none
	bool bAutoConformed = false; // BodySlide: conformed from an add-on of the body when loading. Never saved.
	std::string stackedFrom;	 // BodySlide: the add-on project that the slider was stacked from when loading. Never saved.

	float curValue = 0.0f;
	bool bShow = true;

	std::vector<DiffInfo> dataFiles;

	SliderData(std::string inName = "");
	~SliderData();

	// Gets the slider's data record name for the specified target.
	std::string TargetDataName(const std::string& targetName) {
		for (auto& df : dataFiles)
			if (df.targetName == targetName)
				return df.dataName;

		return "";
	}

	std::string DataFileName(const std::string& targetDataName) {
		for (auto& df : dataFiles)
			if (df.dataName == targetDataName)
				return df.fileName;

		return "";
	}

	DiffInfo* GetDataFile(const std::string& targetDataName) {
		for (auto& df : dataFiles)
			if (df.dataName == targetDataName)
				return &df;

		return nullptr;
	}

	void RenameTarget(const std::string& oldTarget, const std::string& newTarget, const std::string& dataName) {
		for (auto& df : dataFiles) {
			if (df.targetName == oldTarget) {
				df.targetName = newTarget;

				if (df.dataName == oldTarget + dataName) {
					df.dataName = newTarget + dataName;
				}
				else if (StringStartsWith(df.dataName, oldTarget)) {
					std::string dtname = df.dataName.substr(oldTarget.length());
					df.dataName = newTarget + dtname;
				}
				else if (StringEndsWith(df.dataName, name)) {
					df.dataName = newTarget + name;
				}
			}
		}
	}

	void RenameData(const std::string& oldDT, const std::string& newDT) {
		for (auto& df : dataFiles) {
			if (df.dataName == oldDT)
				df.dataName = newDT;
		}
	}

	// Creates a data file record and returns the record index.
	size_t AddDataFile(const std::string& shapeTarget, const std::string& dataAlias, const std::string& fileName, bool localData = true, bool inherited = false) {
		dataFiles.emplace_back(localData, dataAlias, shapeTarget, fileName, inherited);
		return dataFiles.size() - 1;
	}

	// Takes the properties of another slider, but not its name and data.
	void CopyPropertiesFrom(const SliderData& other) {
		bHidden = other.bHidden;
		bInvert = other.bInvert;
		bZap = other.bZap;
		bClamp = other.bClamp;
		bUV = other.bUV;
		defSmallValue = other.defSmallValue;
		defBigValue = other.defBigValue;
		zapToggles = other.zapToggles;
		bToggle = other.bToggle;
		toggleValue = other.toggleValue;
		toggleSliders = other.toggleSliders;
		addOnName = other.addOnName;
		addOnID = other.addOnID;
	}

	// Drops the zaps and sliders this slider toggles that the predicate says are gone.
	template<typename IsGone>
	void ForgetSliders(IsGone isGone) {
		zapToggles.erase(std::remove_if(zapToggles.begin(), zapToggles.end(), isGone), zapToggles.end());
		toggleSliders.erase(std::remove_if(toggleSliders.begin(), toggleSliders.end(), [&isGone](const ToggleTarget& t) { return isGone(t.slider); }),
							toggleSliders.end());
	}

	// Points the zaps and sliders this slider toggles at the new name of a slider.
	void RenameToggledSlider(const std::string& oldName, const std::string& newName) {
		for (auto& toggle : zapToggles)
			if (toggle == oldName)
				toggle = newName;

		for (auto& target : toggleSliders)
			if (target.slider == oldName)
				target.slider = newName;
	}

	// Toggles are kept without data of their own, since they can set other sliders and zaps instead
	bool KeptWithoutData() const { return bToggle && !bZap; }

	bool NeedsAddOn() const { return !addOnName.empty() || !addOnID.empty(); }

	bool HasInheritedData() const {
		for (auto& df : dataFiles)
			if (df.bInherited)
				return true;

		return false;
	}

	// Makes the slider and its data the project's own.
	void ClearInherited() {
		bInherited = false;
		for (auto& df : dataFiles)
			df.bInherited = false;
	}

	bool IsLocalData(const std::string& dataAlias) {
		for (auto& df : dataFiles)
			if (df.dataName == dataAlias)
				return df.bLocal;

		return false;
	}

	bool IsInheritedData(const std::string& dataAlias) {
		for (auto& df : dataFiles)
			if (df.dataName == dataAlias)
				return df.bInherited;

		return false;
	}

	void SetLocalData(const std::string& dataAlias) {
		for (auto& df : dataFiles)
			if (df.dataName == dataAlias)
				df.bLocal = true;
	}

	void Clear() { dataFiles.clear(); }

	int LoadSliderData(XMLElement* srcdata, bool genWeights);
};
