/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "TinyXML-2/tinyxml2.h"

#include <vector>
#include <unordered_map>
#include <wx/string.h>

using namespace std;
using namespace tinyxml2;

class ConfigurationItem {
	vector<ConfigurationItem*> children;
	vector<ConfigurationItem*> properties;

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
	ConfigurationItem(const string& text, ConfigurationItem* inParent = nullptr, int inLevel = 0) {
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
	bool isDefault;								// Default values are not saved with config.

	string name;								// Name associated with this node.
	string value;								// Value associated with this node.
	string path;

	int SettingFromXML(XMLElement* xml);

	void ToXML(XMLElement* parent);
	int EnumerateProperties(vector<ConfigurationItem*>& outList);
	int EnumerateProperties(string& outList);
	int EnumerateChildren(vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);
	int EnumerateChildren(const string& inName, vector<string>& outValueList);
	int EnumerateChildrenProperty(const string& inName, const string& propertyName, vector<string>& outValueList);
	ConfigurationItem* FindChild(const string& inName, bool recurse = true);
	ConfigurationItem* AddChild(const string& inName, const string& value, bool isElement = true);

	ConfigurationItem* FindProperty(const string& inName);

	bool Match(const string& otherName) {
		return (!_stricmp(otherName.c_str(), name.c_str()));
	}
};

class ConfigurationManager
{
	vector<ConfigurationItem*> ciList;
	string file;
	ConfigurationItem* FindCI(const string& inName);

public:
	ConfigurationManager();
	~ConfigurationManager();

	void Clear();
	int LoadConfig(const string& pathToFile = "Config.xml", const string& rootElement = "BodySlideConfig");

	int SaveConfig(const string& pathToFile, const string& rootElementName = "BodySlideConfig");

	int EnumerateCIs(vector<ConfigurationItem*>& outList, bool withProperties = true, bool traverse = true);

	int EnumerateChildCIs(vector<ConfigurationItem*>& outList, const string& parentCI, bool withProperties = true, bool traverse = true);

	bool Exists(const string& inName);

	string GetString(const string& inName);

	int GetIntValue(const string& inName, int def = 0);
	float GetFloatValue(const string& inName, float def = 0.0f);

	void SetValue(const string& inName, const string& newValue, bool flagDefault = false);
	void SetValue(const string& inName, int newValue, bool flagDefault = false);
	void SetValue(const string& inName, float newValue, bool flagDefault = false);

	void SetDefaultValue(const string& inName, const string& newValue);
	void SetDefaultValue(const string& inName, int newValue);

	bool MatchValue(const string& inName, const string& val, bool useCase = false);

	void GetFullKey(ConfigurationItem* from, string& outstr);

	int GetValueArray(const string& containerName, const string& arrayName, vector<string>& outValues);
	int GetValueAttributeArray(const string& containerName, const string& arrayName, const string& attributeName, vector<string>& outValues);

	string operator [] (const char* inName)
	{
		return GetString(string(inName));
	}

	string operator [] (const string& inName) {
		return GetString(inName);
	}
	wxString operator [] (const wxString& inName) {
		return GetString(inName.ToStdString());
	}
};

extern ConfigurationManager Config;
