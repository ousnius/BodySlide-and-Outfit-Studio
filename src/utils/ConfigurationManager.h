/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "../TinyXML-2/tinyxml2.h"

#include <vector>
#include <unordered_map>
#include <wx/string.h>

using namespace tinyxml2;

class ConfigurationItem {
	std::vector<ConfigurationItem*> children;
	std::vector<ConfigurationItem*> properties;

public:
	ConfigurationItem() : parent(nullptr), level(0), isProp(false), isComment(false), isDefault(false) { }
	ConfigurationItem(XMLElement* srcElement, ConfigurationItem* inParent = nullptr, int inLevel = 0) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = false;
		isDefault = false;
		SettingFromXML(srcElement);
	}
	ConfigurationItem(std::string text, ConfigurationItem* inParent = nullptr, int inLevel = 0)
		: value(std::move(text)) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = true;
		isDefault = false;
	}
	~ConfigurationItem();

	ConfigurationItem* parent;
	int level;
	bool isProp;
	bool isComment;
	bool isDefault;								// Default values are not saved with config.

	std::string name;								// Name associated with this node.
	std::string value;								// Value associated with this node.
	std::string path;

	int SettingFromXML(XMLElement* xml);

	void ToXML(XMLElement* parent);
	int EnumerateProperties(std::vector<ConfigurationItem*>& outList);
	int EnumerateProperties(std::string& outList);
	int EnumerateChildren(std::vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);
	int EnumerateChildren(const std::string& inName, std::vector<std::string>& outValueList);
	int EnumerateChildrenProperty(const std::string& inName, const std::string& propertyName, std::vector<std::string>& outValueList);
	ConfigurationItem* FindChild(const std::string& inName, bool recurse = true);
	ConfigurationItem* AddChild(const std::string& inName, const std::string& value, bool isElement = true);

	ConfigurationItem* FindProperty(const std::string& inName);

	bool Match(const std::string& otherName) {
		return (!_stricmp(otherName.c_str(), name.c_str()));
	}
};

class ConfigurationManager {
	std::vector<ConfigurationItem*> ciList;
	std::string file;
	ConfigurationItem* FindCI(const std::string& inName);

public:
	ConfigurationManager();
	~ConfigurationManager();

	void Clear();
	int LoadConfig(const std::string& pathToFile = "Config.xml", const std::string& rootElement = "Config");
	int SaveConfig(const std::string& pathToFile = "Config.xml", const std::string& rootElement = "Config");

	int EnumerateCIs(std::vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);

	int EnumerateChildCIs(std::vector<ConfigurationItem*>& outList, const std::string& parentCI, bool withProperties = true, bool traverse = true);

	bool Exists(const std::string& inName);

	std::string GetString(const std::string& inName);

	int GetIntValue(const std::string& inName, int def = 0);
	float GetFloatValue(const std::string& inName, float def = 0.0f);
	bool GetBoolValue(const std::string& inName, bool def = false);

	void SetValue(const std::string& inName, const std::string& newValue, bool flagDefault = false);
	void SetValue(const std::string& inName, int newValue, bool flagDefault = false);
	void SetValue(const std::string& inName, float newValue, bool flagDefault = false);
	void SetBoolValue(const std::string& inName, bool newValue, bool flagDefault = false);

	void SetDefaultValue(const std::string& inName, const std::string& newValue);
	void SetDefaultValue(const std::string& inName, int newValue);
	void SetDefaultValue(const std::string& inName, float newValue);
	void SetDefaultValue(const std::string& inName, bool newValue);

	bool MatchValue(const std::string& inName, const std::string& val, bool useCase = false);

	void GetFullKey(ConfigurationItem* from, std::string& outstr);

	int GetValueArray(const std::string& containerName, const std::string& arrayName, std::vector<std::string>& outValues);
	int GetValueAttributeArray(const std::string& containerName, const std::string& arrayName, const std::string& attributeName, std::vector<std::string>& outValues);

	std::string operator [] (const char* inName) {
		return GetString(std::string(inName));
	}

	std::string operator [] (const std::string& inName) {
		return GetString(inName);
	}
	wxString operator [] (const wxString& inName) {
		return GetString(inName.ToStdString());
	}

	/* Utility function to replace variables within a string with matching configuration data.  Variables
		are surrounded by %.  EG  :  "%GameDataPath%rest of path"  might become "D:\\Skyrim\\Data\\rest of path.
		a double percent "%%" will be replaced with a single %, while a single % without matching variable will destroy most of the string.
		*/
	void ReplaceVars(std::string& inoutStr);
};
