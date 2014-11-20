#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "tinyxml.h"

using namespace std;

class ConfigurationItem {
	vector<ConfigurationItem*> children;
	vector<ConfigurationItem*> properties;

public:
	ConfigurationItem(): parent(NULL), level(0), isProp(false), isComment(false), isDefault(false) {}
	ConfigurationItem(TiXmlElement* srcElement, ConfigurationItem* inParent = NULL, int inLevel = 0) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = false;
		isDefault = false;
		SettingFromXML(srcElement);
	}
	ConfigurationItem(const string& text, ConfigurationItem* inParent = NULL, int inLevel = 0) {
		parent = inParent;
		level = inLevel;
		isProp = false;
		isComment = true;
		isDefault = false;
		value = text;
	}

	~ConfigurationItem();
	ConfigurationItem* parent;
	int level;
	bool isProp;
	bool isComment;
	bool isDefault;								// Default values are not saved with config

	string Name;								// Name associated with this node 
	string value;								// Value associated with this node 
	string path; 

	int SettingFromXML(TiXmlElement* xml);

	void ToXML(TiXmlElement* parent);
	int EnumerateProperties(vector<ConfigurationItem*>& outList);
	int EnumerateProperties(string& outList);
	int EnumerateChildren(vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);
	int EnumerateChildren(const string& name, vector<string>& outValueList);
	int EnumerateChildrenProperty(const string& name, const string& propertyName, vector<string>& outValueList);
	ConfigurationItem* FindChild(const string& name, bool recurse = true);
	ConfigurationItem* AddChild(const string& name, const string& value, bool is_element = true);

	ConfigurationItem* FindProperty(const string& name);

	bool Match(const string& name) {
		bool isMatch = _stricmp(name.c_str(), Name.c_str());
		if (!isMatch) return true;
		return false;
	}
};

class ConfigurationManager
{
	vector<ConfigurationItem*> ciList;
	string file;
	ConfigurationItem* FindCI(const string& name);

public:
	ConfigurationManager(void);
	~ConfigurationManager(void);

	void Clear();
	int LoadConfig(const string& pathToFile = "Config.xml", const string& rootElement = "BodySlideConfig");

	int SaveConfig(const string& pathToFile, const string& rootElementName = "BodySlideConfig");

	int EnumerateCIs (vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);

	int EnumerateChildCIs(vector<ConfigurationItem*>& outList, const string& parentCI, bool withProperties = true, bool traverse = true);

	bool Exists(const string& name);

	const char* GetCString(const string& name, const string& def = NULL);

	string GetString(const string& name);

	int GetIntValue(const string& name, int def = 0);
	float GetFloatValue(const string& name, float def = 0.0f);

	void SetValue(const string& name, const string& newValue, bool flagDefault = false);
	void SetValue(const string& name, int newValue, bool flagDefault = false);
	void SetValue(const string& name, float newValue, bool flagDefault = false);

	void SetDefaultValue(const string& name, const string& newValue);

	bool MatchValue(const string& name, const string& val, bool useCase = false);

	void GetFullKey(ConfigurationItem* from, string& outstr);

	int GetValueArray(const string& containerName, const string& arrayName, vector<string>& outValues);
	int GetValueAttributeArray(const string& containerName, const string& arrayName, const string& attributeName, vector<string>& outValues);

	string operator [] (const string& name) {
		return GetString(name);
	}
};

extern ConfigurationManager Config;