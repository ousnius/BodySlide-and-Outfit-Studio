#include "ConfigurationManager.h"

ConfigurationItem::~ConfigurationItem() {
	vector<ConfigurationItem*>::iterator it;

	for (it = properties.begin(); it != properties.end(); ++it) {
		delete(*it);
	}

	for (it = children.begin(); it != children.end(); ++it) {
		delete(*it);
	}

	properties.clear();
	children.clear();
}

int ConfigurationItem::SettingFromXML(TiXmlElement* xml) {
	string variable;
	ConfigurationItem* newCI;

	Name = xml->Value();			
	if (parent) {
		path = parent->path + "/";
		path += Name;
	} else {
		level = 0;
		path = Name;
	}
	if (xml->GetText() != NULL)
		value = xml->GetText();				

	TiXmlAttribute* attr = xml->FirstAttribute();
	while (attr) {
		newCI = new ConfigurationItem();	
		newCI->parent = this;	
		newCI->level = level+1;
		newCI->isProp = true;
		newCI->Name = attr->Name();		
		newCI->value = attr->Value();	
		newCI->path = path + ".";
		newCI->path += newCI->Name;
		properties.push_back(newCI);		

		attr = attr->Next();
	}

	TiXmlNode* child = xml->FirstChild();
	while (child) {
		if (child->Type() == TiXmlNode::TINYXML_COMMENT) {
			children.push_back(new ConfigurationItem(child->ToComment()->Value(), this, level + 1));
		} else if (child->Type() == TiXmlNode::TINYXML_ELEMENT) {
			children.push_back(new ConfigurationItem(child->ToElement(), this, level + 1));
		}
		child = child->NextSibling();
	}

	return 0;								
}

void ConfigurationItem::ToXML(TiXmlElement* parent) {
	vector<string> delimits;

	TiXmlElement* elem = parent->InsertEndChild(TiXmlElement(Name.c_str()))->ToElement();

	for (auto prop: properties) {
		if (prop->isDefault) continue;
		elem->SetAttribute(prop->Name.c_str(), prop->value.c_str());
	}
	for (auto child: children) {
		if (child->isDefault) continue;
		if (child->isComment) {
			elem->InsertEndChild(TiXmlComment(child->value.c_str()));
		} else {
			child->ToXML(elem);
		}
	}
	elem->InsertEndChild(TiXmlText(value.c_str()));	
}

int ConfigurationItem::EnumerateProperties(vector<ConfigurationItem*>& outList) {
	int count = 0;
	for (auto prop: properties) {
		count++;
		outList.push_back(prop);
	}
	return count;
}

int ConfigurationItem::EnumerateProperties(string& outList) {
	int count = 0;
	outList = "";

	for (auto prop: properties) {
		count++;
		outList += prop->Name + "=\"" + prop->value + "\" ";
	}

	return count;
}

int ConfigurationItem::EnumerateChildren(vector<ConfigurationItem*>& outList, bool withProperties, bool traverse) {
	int count = 0;
	for (auto child: children) {
		if (child->isComment) continue;
		count++;
		outList.push_back(child);
		if (traverse) {
			if (withProperties)
				count += child->EnumerateProperties(outList);
			count += child->EnumerateChildren(outList, withProperties, traverse);
		}
	}
	return count;
}

int ConfigurationItem::EnumerateChildren(const string& nameMatch, vector<string>& outList) {
	for (auto child: children) {
		if (child->isComment) continue;
		if (child->Match(nameMatch)) {
			outList.push_back(child->value);
		}
	}
	return outList.size();
}

int ConfigurationItem::EnumerateChildrenProperty(const string& nameMatch, const string& propertyName, vector<string>& outList) {
	ConfigurationItem* prop;
	for (auto child: children) {
		if (child->isComment) continue;
		if (child->Match(nameMatch)) {
			prop = child->FindProperty(propertyName);
			if (prop)
				outList.push_back(prop->value);
		}
	}
	return outList.size();
}


ConfigurationItem* ConfigurationItem::FindChild(const string& name, bool recurse) {
	string tmpName;
	ConfigurationItem* found = NULL;

	int pos = name.find_first_of("/.");
	if (pos != string::npos) {
		tmpName = name.substr(0, pos);
		for (auto child: children) {
			if (child->isComment) continue;
			if (child->Match(tmpName)) found = child;
		}

		if (!found) return NULL;
		else if (!recurse) return found;
		if (name.at(pos) == '/') {
			found = found->FindChild(name.substr(pos + 1));
		} else if (name.at(pos) == '.') {
			found = found->FindProperty(name.substr(pos + 1));
		}
		if (!found) return NULL;
		
	} else {
		for (auto child: children) {
			if (child->isComment) continue;
			if (child->Match(name)) found = child;
		}
	}
	// return NULL if nothing was found
	return found;
}

ConfigurationItem* ConfigurationItem::AddChild(const string& name, const string& value, bool is_element) {
	string tmpName;
	ConfigurationItem* found = NULL;
	ConfigurationItem* newCI;

	int pos = name.find_first_of("/.");			
	tmpName = name.substr(0, pos);				
	for (auto child: children) {
		if (child->isComment) continue;
		if (child->Match(tmpName)) found = child;
	}
	if (!found) {								
		newCI = new ConfigurationItem;			
		newCI->Name = name.substr(0, pos);			
		newCI->parent = this;

		if(is_element)
			newCI->path = path + "/" + newCI->Name;
		else
			newCI->path = path + "." + newCI->Name;

		newCI->level = level + 1;
		if (pos == -1) {
			newCI->value = value;
			found = newCI;
		} else if (name.at(pos) == '/') {
			found = newCI->AddChild(name.substr(pos + 1), value, true);
		} else if (name.at(pos) == '.')
			found = newCI->AddChild(name.substr(pos + 1), value, false);

		if (is_element)							
			children.push_back(newCI);			
		else
			properties.push_back(newCI);	

	} else {
		if (pos == -1) {
			return NULL;
		} else if (name.at(pos) == '/') {
			found = found->AddChild(name.substr(pos + 1), value, true);
		} else if (name.at(pos) == '.')
			found = found->AddChild(name.substr(pos + 1), value, false);
	}

	return found;
}

ConfigurationItem* ConfigurationItem::FindProperty(const string& name) {
	for (auto prop: properties) {
		if (prop->Match(name))
			return prop;
	}
	return NULL;
}

ConfigurationManager::ConfigurationManager(void)
{
}

ConfigurationManager::~ConfigurationManager(void)
{
	Clear();
}

void ConfigurationManager::Clear() {
	for (auto ci: ciList) {
		delete ci;
	}
	ciList.clear();
}

int ConfigurationManager::LoadConfig(const string& pathToFile, const string& rootElement) {
	TiXmlDocument configXML(pathToFile.c_str());
	configXML.LoadFile(pathToFile.c_str());

	if (configXML.Error())
		return 1;

	Clear();

	TiXmlElement* rootElem = configXML.FirstChildElement(rootElement.c_str());
	if (!rootElem)
		return 2;

	TiXmlNode* child = rootElem->FirstChild();
	while (child) {
		if (child->Type() == TiXmlNode::TINYXML_COMMENT) {
			ciList.push_back(new ConfigurationItem(child->ToComment()->Value(), 0, 0));

		} else if(child->Type() == TiXmlNode::TINYXML_ELEMENT)
			ciList.push_back(new ConfigurationItem(child->ToElement()));

		child = child->NextSibling();
	}
	return 0;
}

int ConfigurationManager::EnumerateCIs(vector<ConfigurationItem*>& outList, bool withProperties, bool traverse) {
	int count = 0;
	if (ciList.size() == 0)
		return 0;

	for (auto ci: ciList) {
		outList.push_back(ci);
		count++;
		if (traverse) {
			if (withProperties) count += ci->EnumerateProperties(outList);
			count += ci->EnumerateChildren(outList, withProperties, traverse);
		}
	}
	return count;
}

int ConfigurationManager::EnumerateChildCIs(vector<ConfigurationItem*>& outList, const string& parentCI, bool withProperties, bool traverse) {
	ConfigurationItem* ci = FindCI(parentCI);
	if (!ci) return 0;
	return ci->EnumerateChildren(outList, withProperties, traverse);
}

bool ConfigurationManager::Exists(const string& name) {
	ConfigurationItem* itemFound = FindCI(name);
	if (itemFound)
		return true;
	else
		return false;
}

ConfigurationItem* ConfigurationManager::FindCI(const string& name) {
	string tmpName;
	ConfigurationItem* found = NULL;

	int pos = name.find_first_of("/.");
	if (pos != -1) {
		tmpName = name.substr(0, pos);
		for (auto ci: ciList) {
			if (ci->Match(tmpName))
				found = ci;
		}

		if (!found) return NULL;
		if (name.at(pos) == '/') {
			found = found->FindChild(name.substr(pos + 1));
		} else if (name.at(pos) == '.') {
			found = found->FindProperty(name.substr(pos + 1));
		}
		if (!found) return NULL;
		
	} else {
		for (auto ci: ciList) {
			if (ci->Match(name))
				found = ci;
		}
	}
	return found;
}

const char* ConfigurationManager::GetCString(const string& name, const string& def) {
	ConfigurationItem* itemFound;
	itemFound = FindCI(name);
	if (itemFound)
		return itemFound->value.c_str();

	return def.c_str();
}

int ConfigurationManager::GetIntValue(const string& name, int def) {
	int res = def;
	ConfigurationItem* itemFound;

	itemFound = FindCI(name);
	if (itemFound) {
		if (!itemFound->value.empty()) {
			res = atoi(itemFound->value.c_str());
		}
	}
	return res;
}
float ConfigurationManager::GetFloatValue(const string& name, float def) {
	float res = def;
	ConfigurationItem* itemFound;

	itemFound = FindCI(name);
	if (itemFound) {
		if (!itemFound->value.empty()) {
			res = (float)atof(itemFound->value.c_str());
		}
	}
	return res;
}

string ConfigurationManager::GetString(const string& name) {
	ConfigurationItem* itemFound;
	itemFound = FindCI(name);
	if (itemFound)
		return itemFound->value;

	return "";
}

void ConfigurationManager::SetDefaultValue(const string& name, const string& newValue) {
	if (FindCI(name))
		return;

	SetValue(name, newValue, true);
}

void ConfigurationManager::SetValue(const string& name, const string& newValue, bool flagDefault) {
	ConfigurationItem* itemFound;
	ConfigurationItem* newCI;
	string search = name;
	string tmpName;
	int pos;

	itemFound = FindCI(search);
	if (itemFound) {
		itemFound->value = newValue;
		itemFound->isDefault = flagDefault;
	} else {
		pos = search.find_first_of("/.");
		tmpName = search.substr(0, pos);
		for (auto ci: ciList) {
			if (ci->Match(tmpName))
				itemFound = ci;
		}

		if (!itemFound) {
			newCI = new ConfigurationItem();
			newCI->Name = tmpName;
			newCI->path = tmpName;
			ciList.push_back(newCI);
			itemFound = newCI;
			if (pos == -1) {
				newCI->value = newValue;
				newCI->isDefault = flagDefault;
				return;
			}
		}

		if (search.at(pos) == '/')
			itemFound->AddChild(search.substr(pos + 1), newValue)->isDefault = flagDefault;
		else
			itemFound->AddChild(search.substr(pos + 1), newValue, false)->isDefault = flagDefault;
	}
}

void ConfigurationManager::SetValue(const string& name, int newValue, bool flagDefault) {
	char intStr[24];
	_snprintf_s(intStr, 24, 24, "%d", newValue);
	SetValue(name, string(intStr), flagDefault);
}

void ConfigurationManager::SetValue(const string& name, float newValue, bool flagDefault) {
	char intStr[24];
	_snprintf_s(intStr, 24, 24, "%0.5f", newValue);
	SetValue(name, string(intStr), flagDefault);
}

bool ConfigurationManager::MatchValue(const string& name, const string& val, bool useCase) {
	ConfigurationItem* itemFound = FindCI(name);
	if (itemFound) {
		if (!useCase) {
			if (!_stricmp(itemFound->value.c_str(), val.c_str()))
				return true;
		}
		else {
			if (!strcmp(itemFound->value.c_str(), val.c_str()))
				return true;
		}
	}
	return false;
}

void ConfigurationManager::GetFullKey(ConfigurationItem* from, string& outStr) {
	vector<string> stringStack;
	outStr = "";

	while (from) {
		stringStack.push_back(from->Name);
		from = from->parent;
	}

	for (int i = stringStack.size() - 1; i >= 0; --i) {
		if (i != stringStack.size() - 1) outStr += "/";
		outStr += stringStack[i];
	}
}

int ConfigurationManager::GetValueArray(const string& containerName, const string& arrayName, vector<string>& outValues) {
	int count = 0;

	ConfigurationItem* container = FindCI(containerName);
	if (container) {
		count = container->EnumerateChildren(arrayName, outValues);
	}

	return count;
}

int ConfigurationManager::GetValueAttributeArray(const string& containerName, const string& arrayName, const string& attributeName, vector<string>& outValues) {
	int count = 0;

	ConfigurationItem* container = FindCI(containerName);
	if (container) {
		count = container->EnumerateChildrenProperty(arrayName, attributeName, outValues);
	}

	return count;
}

int ConfigurationManager::SaveConfig(const string& pathToFile, const string& rootElementName) {
	if (rootElementName.empty()) return 1;

	TiXmlDocument doc(pathToFile.c_str());
	if (doc.Error())
		return 2;

	doc.Clear();

	TiXmlElement* rootElem = (TiXmlElement*)doc.InsertEndChild(TiXmlElement(rootElementName.c_str()));

	for (auto ci: ciList) {
		if (ci->isDefault) continue;
		if (ci->isComment) {
			 rootElem->InsertEndChild(TiXmlComment(ci->value.c_str()));
		} else
			ci->ToXML(rootElem);

	}
	doc.SaveFile();
	return 0;
}