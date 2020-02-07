/*
BodySlide and Outfit Studio
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

class RefTemplate {
	std::string name;
	std::string source;
	std::string set;
	std::string shape;
	bool loadAll = false;
	std::vector<std::string> sourceFiles;

public:
	RefTemplate() { }
	RefTemplate(XMLElement* srcElement) {
		Load(srcElement);
	}

	std::string GetName() const {
		return name;
	}
	void SetName(const std::string& inName) {
		name = inName;
	}

	std::string GetSource() const {
		return source;
	}
	void SetSource(const std::string& inSource) {
		source = inSource;
	}

	std::string GetSetName() const {
		return set;
	}
	void SetSetName(const std::string& inSetName) {
		set = inSetName;
	}

	std::string GetShape() const {
		return shape;
	}
	void SetShape(const std::string& inShape) {
		shape = inShape;
	}

	bool GetLoadAll() const {
		return loadAll;
	}
	void SetLoadAll(const bool inLoadAll) {
		loadAll = inLoadAll;
	}

	int Load(XMLElement* srcElement);
};


class RefTemplateCollection {
	std::vector<RefTemplate> refTemplates;

public:
	// Loads all in the specified folder.
	int Load(const std::string& basePath);
	int GetAll(std::vector<RefTemplate>& outAppend);
};


class RefTemplateFile {
	XMLDocument doc;
	XMLElement* root;
	std::vector<std::pair<std::string, XMLElement*>> refTemplatesInFile;
	int error;

public:
	std::string fileName;
	RefTemplateFile() :error(0), root(nullptr) { }
	RefTemplateFile(const std::string& srcFileName);
	~RefTemplateFile() {};

	bool fail() {
		return error != 0;
	}
	int GetError() {
		return error;
	}
	
	void Open(const std::string& srcFileName);
	void New(const std::string& newFileName);
	void Rename(const std::string& newFileName);
	int GetNames(std::vector<std::string>& outNames, bool append = true, bool unique = false);
	bool Has(const std::string& queryName);
	int GetAll(std::vector<RefTemplate>& outAppend);
	int Get(const std::string& templateName, RefTemplate& outTemplates);
};
