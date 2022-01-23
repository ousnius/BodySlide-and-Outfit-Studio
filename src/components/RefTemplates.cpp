/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "RefTemplates.h"
#include "../utils/PlatformUtil.h"
#include "../utils/StringStuff.h"

int RefTemplateCollection::Load(const std::string& basePath) {
	refTemplates.clear();

	wxArrayString files;
	wxDir::GetAllFiles(basePath, &files, "*.xml");

	for (auto& file : files) {
		RefTemplateFile templateFile(file.ToUTF8().data());
		std::vector<std::string> templates;
		templateFile.GetNames(templates);
		for (auto& t : templates) {
			RefTemplate refTemplate;
			templateFile.Get(t, refTemplate);
			refTemplates.push_back(std::move(refTemplate));
		}
	}

	return 0;
}

int RefTemplateCollection::GetAll(std::vector<RefTemplate>& outAppend) {
	int count = 0;

	for (auto& rt : refTemplates) {
		outAppend.emplace_back(rt);
		count++;
	}

	return count;
}

int RefTemplate::Load(XMLElement* srcElement) {
	if (srcElement == nullptr)
		return 1;

	name = srcElement->GetText();

	if (srcElement->Attribute("sourcefile"))
		source = ToOSSlashes(srcElement->Attribute("sourcefile"));

	if (srcElement->Attribute("set"))
		set = srcElement->Attribute("set");

	if (srcElement->Attribute("shape"))
		shape = srcElement->Attribute("shape");

	if (srcElement->Attribute("loadAll"))
		loadAll = srcElement->BoolAttribute("loadAll");

	std::string* fileName = static_cast<std::string*>(srcElement->GetDocument()->GetUserData());
	sourceFiles.push_back(*fileName);

	return 0;
}

RefTemplateFile::RefTemplateFile(const std::string& srcFileName) {
	root = nullptr;
	error = 0;
	Open(srcFileName);
}

void RefTemplateFile::Open(const std::string& srcFileName) {
	fileName = srcFileName;

	FILE* fp = nullptr;

#ifdef _WINDOWS
	std::wstring winFileName = PlatformUtil::MultiByteToWideUTF8(srcFileName);
	error = _wfopen_s(&fp, winFileName.c_str(), L"rb");
	if (error || !fp)
		return;
#else
	fp = fopen(srcFileName.c_str(), "rb");
	if (!fp) {
		error = errno;
		return;
	}
#endif

	error = doc.LoadFile(fp);
	fclose(fp);

	if (error)
		return;

	doc.SetUserData(&fileName);
	root = doc.FirstChildElement("RefTemplates");
	if (!root) {
		error = 2;
		return;
	}

	XMLElement* element = root->FirstChildElement("Template");
	while (element) {
		auto refTemplate = std::make_pair(element->GetText(), element);
		refTemplatesInFile.push_back(refTemplate);
		element = element->NextSiblingElement("Template");
	}
	if (refTemplatesInFile.empty()) {
		error = 3;
		return;
	}

	error = 0;
}

void RefTemplateFile::Rename(const std::string& newFileName) {
	fileName = newFileName;
}

int RefTemplateFile::GetNames(std::vector<std::string>& outNames, bool append, bool unique) {
	std::unordered_set<std::string> existingNames;
	if (!append)
		outNames.clear();

	if (unique)
		existingNames.insert(outNames.begin(), outNames.end());

	for (auto& rt : refTemplatesInFile) {
		if (unique && existingNames.find(rt.first) != existingNames.end())
			continue;
		else if (unique)
			existingNames.insert(rt.first);

		outNames.push_back(rt.first);
	}
	return 0;
}

bool RefTemplateFile::Has(const std::string& queryName) {
	auto tmpl = std::find_if(refTemplatesInFile.begin(), refTemplatesInFile.end(), [&queryName](const std::pair<std::string, XMLElement*>& rt) { return rt.first == queryName; });
	if (tmpl != refTemplatesInFile.end())
		return true;

	return false;
}

int RefTemplateFile::GetAll(std::vector<RefTemplate>& outAppend) {
	int count = 0;

	for (auto& rt : refTemplatesInFile) {
		RefTemplate refTemplate;
		refTemplate.Load(rt.second);

		outAppend.emplace_back(refTemplate);
		count++;
	}

	return count;
}

int RefTemplateFile::Get(const std::string& templateName, RefTemplate& outTemplates) {
	auto tmpl = std::find_if(refTemplatesInFile.begin(), refTemplatesInFile.end(), [&templateName](const std::pair<std::string, XMLElement*>& rt) {
		return rt.first == templateName;
	});
	if (tmpl != refTemplatesInFile.end())
		outTemplates.Load(tmpl->second);
	else
		return 1;

	return 0;
}
