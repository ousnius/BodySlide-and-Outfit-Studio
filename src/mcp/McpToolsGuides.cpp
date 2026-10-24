/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../utils/ConfigurationManager.h"

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/tokenzr.h>

extern ConfigurationManager Config;

using nlohmann::json;

namespace {

// Guides on how to do tasks with the tools are Markdown files in res/mcp/guides, one per topic, named after it.
// The first line is the title, the next line that isn't empty the summary.
struct Guide {
	std::string topic;
	std::string title;
	std::string summary;
	std::string text;
};

wxString GuidesFolder() {
	return wxString::FromUTF8(Config["AppDir"]) + "/res/mcp/guides";
}

bool ReadGuide(const wxString& path, Guide& guide) {
	wxFile file;
	wxString text;
	if (!wxFileName::FileExists(path) || !file.Open(path) || !file.ReadAll(&text, wxConvUTF8))
		return false;
	text.Replace("\r\n", "\n");

	wxStringTokenizer lines(text, "\n");
	wxString title = lines.GetNextToken();
	if (title.StartsWith("#"))
		title = title.AfterFirst(' ');
	wxString summary;
	while (lines.HasMoreTokens() && summary.empty())
		summary = lines.GetNextToken().Trim().Trim(false);

	guide.topic = wxFileName(path).GetName().ToUTF8().data();
	guide.title = title.Trim().ToUTF8().data();
	guide.summary = summary.ToUTF8().data();
	guide.text = text.ToUTF8().data();
	return true;
}

// Read each time, so guides can be changed without restarting
std::vector<Guide> ReadGuides() {
	std::vector<Guide> guides;
	const wxString folder = GuidesFolder();
	if (!wxDir::Exists(folder))
		return guides;

	wxArrayString files;
	wxDir::GetAllFiles(folder, &files, "*.md", wxDIR_FILES);
	files.Sort();
	for (auto& file : files) {
		Guide guide;
		if (ReadGuide(file, guide))
			guides.push_back(std::move(guide));
	}
	return guides;
}

json TopicList(const std::vector<Guide>& guides) {
	json list = json::array();
	for (auto& guide : guides)
		list.push_back({{"topic", guide.topic}, {"title", guide.title}, {"summary", guide.summary}});
	return list;
}

} // namespace

void AddGuideTools(OutfitStudioMcp& mcp) {
	McpTool getGuide;
	getGuide.name = "get_guide";
	getGuide.description =
		"Returns a guide on how to do a task with these tools: which tools to use in which order, what to check, and "
		"the pitfalls. Read the guide of a task before starting it, and \"basics\" first. Without a topic it lists the "
		"topics with a summary each.";

	json topicProp = McpProp("string", "Topic of the guide. Leave out to list the topics.");
	json topics = json::array();
	for (auto& guide : ReadGuides())
		topics.push_back(guide.topic);
	if (!topics.empty())
		topicProp["enum"] = topics;

	getGuide.inputSchema = McpSchema({{"topic", topicProp}});
	getGuide.readOnly = true;
	getGuide.handler = [](const json& args) {
		std::vector<Guide> guides = ReadGuides();
		if (guides.empty())
			throw McpToolError("No guides were found in '" + std::string(GuidesFolder().ToUTF8().data()) + "'.");

		McpToolResult result;
		const std::string topic = McpArgs::String(args, "topic", "");
		if (topic.empty()) {
			result.data["topics"] = TopicList(guides);
			return result;
		}

		for (auto& guide : guides) {
			if (guide.topic == topic) {
				result.data["topic"] = guide.topic;
				result.data["title"] = guide.title;
				result.data["guide"] = guide.text;
				return result;
			}
		}

		result = McpToolResult::Error("There is no guide '" + topic + "'.");
		result.data["topics"] = TopicList(guides);
		return result;
	};
	mcp.AddTool(std::move(getGuide));
}
