/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "../TinyXML-2/tinyxml2.h"

#include <wx/dir.h>
#include <vector>
#include <unordered_map>
#include <set>
#include <unordered_set>

using namespace tinyxml2;

class SliderCategory {
	std::string name;
	std::vector<std::string> sliders;
	std::unordered_map<std::string, std::string> displayNames;
	std::vector<std::string> sourceFiles;
	bool isHidden = false;

public:
	SliderCategory() { }
	SliderCategory(XMLElement* srcCategoryElement) {
		LoadCategory(srcCategoryElement);
	}

	std::string GetName() {
		return name;
	}
	void SetName(const std::string& inName) {
		name = inName;
	}

	int AddSliders(const std::vector<std::string>& inSliders);
	bool HasSlider(const std::string& search);
	int GetSliders(std::vector<std::string>& outSliders);
	int GetSliders(std::unordered_set<std::string>& outSliders);

	std::string GetSliderDisplayName(const std::string& sliderName);

	bool GetHidden();
	void SetHidden(bool hide);

	// Combine the source category's sliders into this one's list. Also merges the source file list.
	void MergeSliders(const SliderCategory& sourceCategory);

	int LoadCategory(XMLElement* srcCategoryElement);
	void WriteCategory(XMLElement* categoryElement, bool append = false);
};


class SliderCategoryCollection {
	std::unordered_map<std::string, SliderCategory> categories;

public:
	// Loads all categories in the specified folder.
	int LoadCategories(const std::string& basePath);

	int GetAllCategories(std::vector<std::string>& outCategories);
	int GetSliderCategory(const std::string& sliderName, std::string& outCategory);
	std::string GetSliderDisplayName(const std::string& categoryName, const std::string& sliderName);

	bool GetCategoryHidden(const std::string& categoryName);
	int SetCategoryHidden(const std::string& categoryName, bool hide);

	int GetCategorySliders(const std::string& categoryName, std::vector<std::string>& outSliders);
	int GetCategorySliders(const std::string& categoryName, std::unordered_set<std::string>& outSliders);
};


class SliderCategoryFile {
	XMLDocument doc;
	XMLElement* root;
	std::unordered_map<std::string, XMLElement*> categoriesInFile;
	int error;

public:
	std::string fileName;
	SliderCategoryFile() :error(0), root(nullptr) { }
	SliderCategoryFile(const std::string& srcFileName);
	~SliderCategoryFile() {};

	bool fail() {
		return error != 0;
	}
	int GetError() {
		return error;
	}

	// Loads the XML document and identifies included category names. On a failure, sets the internal error value.
	void Open(const std::string& srcFileName);

	// Creates a new empty category document structure, ready to add new categories and sliders to.
	void New(const std::string& newFileName);

	// Changes the internal file name. The XML file isn't saved until the Save() function is used.
	// Note the original file name is not changed. This method allows you to save a category as a new file without altering the original.
	void Rename(const std::string& newFileName);

	// Returns a list of all the categories found in the file.
	int GetCategoryNames(std::vector<std::string>& outCategoryNames, bool append = true, bool unique = false);

	// Returns true if the category name exists in the file.
	bool HasCategory(const std::string& queryCategoryName);

	// Adds all of the categories in the file to the supplied categories vector. Does not clear the vector before doing so.
	int GetAllCategories(std::vector<SliderCategory>& outAppendCategories);

	// Gets a single category from the XML document based on the name.
	int GetCategory(const std::string& categoryName, SliderCategory& outCategories);

	// Updates a slider category in the xml document with the provided information.
	// If the category does not already exist in the file (based on name) the category is added.
	int UpdateCategory(SliderCategory& inCategory);

	// Writes the XML file using the internal fileName (use Rename() to change the name).
	bool Save();
};
