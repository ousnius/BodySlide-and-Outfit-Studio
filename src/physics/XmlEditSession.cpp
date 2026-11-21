/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "XmlEditSession.h"

#include <cctype>
#include <sstream>

namespace Physics {
std::string XmlEditSession::NormalizeXmlPath(const std::string& xmlPath) {
	// The same two liberties the rest of the physics code takes with these
	// paths: bone and file names compare case insensitively (hdt::IDStr), and
	// the links in the wild are written with either separator.
	std::string key;
	key.reserve(xmlPath.size());

	for (const char c : xmlPath) {
		if (c == '\\')
			key.push_back('/');
		else
			key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
	}

	return key;
}

const XmlEditSession::Entry* XmlEditSession::FindEntry(const std::string& xmlPath) const {
	const std::string key = NormalizeXmlPath(xmlPath);

	for (const auto& entry : documents) {
		if (entry.key == key)
			return &entry;
	}

	return nullptr;
}

XmlDocument* XmlEditSession::Find(const std::string& xmlPath) {
	const Entry* entry = FindEntry(xmlPath);
	return entry ? entry->doc.get() : nullptr;
}

const XmlDocument* XmlEditSession::Find(const std::string& xmlPath) const {
	const Entry* entry = FindEntry(xmlPath);
	return entry ? entry->doc.get() : nullptr;
}

std::unique_ptr<std::istream> XmlEditSession::ResolveStream(const std::string& xmlPath, std::string* outError) {
	if (outError)
		outError->clear();

	if (XmlDocument* doc = Find(xmlPath)) {
		RefreshFromFile(*doc);
		return std::make_unique<std::istringstream>(doc->Serialize());
	}

	if (!sourceResolver) {
		if (outError)
			*outError = "no physics XML source is bound to the session";
		return nullptr;
	}

	GameDataStream::FoundAt foundAt;
	auto stream = sourceResolver(xmlPath, &foundAt);
	if (!stream) {
		if (outError)
			*outError = "not found";
		return nullptr;
	}

	std::ostringstream contents;
	contents << stream->rdbuf();
	std::string text = contents.str();

	auto doc = std::make_unique<XmlDocument>();
	std::string error;
	if (!doc->LoadFromString(text, error)) {
		// The simulation reads these files with its own parser, which is not
		// this one, so a file only this one rejects has to keep working: hand
		// the preview the original bytes and leave the file alone.
		warnings.push_back(xmlPath + ": not editable (" + error + ")");
		if (outError)
			*outError = "not editable: " + error;

		return std::make_unique<std::istringstream>(std::move(text));
	}

	// The path inside an archive is no file to write back to
	doc->SetSource(xmlPath, foundAt.inArchive ? std::string() : foundAt.path, foundAt.inArchive ? XmlOrigin::Archive : XmlOrigin::Loose);

	// Serialize before the document moves into the store, so the stream does
	// not depend on where it landed.
	std::string serialized = doc->Serialize();

	Entry entry;
	entry.key = NormalizeXmlPath(xmlPath);
	entry.doc = std::move(doc);
	documents.push_back(std::move(entry));
	++revision;

	return std::make_unique<std::istringstream>(std::move(serialized));
}

void XmlEditSession::RefreshFromFile(XmlDocument& doc) {
	// Edits that aren't saved are the editor's, and the other origins have no file of their own to change
	if (doc.IsDirty() || doc.Origin() != XmlOrigin::Loose || doc.SourcePath().empty())
		return;

	auto stream = GameDataStream::OpenLoose(doc.SourcePath());
	if (!stream)
		return;

	std::ostringstream contents;
	contents << stream->rdbuf();

	XmlDocument fresh;
	std::string error;
	if (!fresh.LoadFromString(contents.str(), error) || fresh.Serialize() == doc.Serialize())
		return;

	// Changed on disk since it was read, by something other than this editor
	const std::string xmlPath = doc.XmlPath();
	const std::string sourcePath = doc.SourcePath();
	doc.LoadFromString(contents.str(), error);
	doc.SetSource(xmlPath, sourcePath, XmlOrigin::Loose);
	++revision;
}

XmlStreamResolver XmlEditSession::Resolver() {
	return [this](const std::string& xmlPath) { return ResolveStream(xmlPath, nullptr); };
}

XmlDocument* XmlEditSession::Open(const std::string& xmlPath, std::string& outError) {
	if (XmlDocument* existing = Find(xmlPath)) {
		RefreshFromFile(*existing);
		return existing;
	}

	ResolveStream(xmlPath, &outError);

	// Null when the file could not be read at all, and also when it was read
	// but not parsed - in which case ResolveStream left the reason in outError.
	return Find(xmlPath);
}

XmlDocument* XmlEditSession::Create(const std::string& xmlPath) {
	if (Find(xmlPath))
		return nullptr;

	auto doc = std::make_unique<XmlDocument>();
	doc->CreateEmpty();
	doc->SetSource(xmlPath, std::string(), XmlOrigin::New);

	Entry entry;
	entry.key = NormalizeXmlPath(xmlPath);
	entry.doc = std::move(doc);
	documents.push_back(std::move(entry));
	++revision;

	return documents.back().doc.get();
}

bool XmlEditSession::Close(const std::string& xmlPath) {
	const std::string key = NormalizeXmlPath(xmlPath);

	for (auto it = documents.begin(); it != documents.end(); ++it) {
		if (it->key != key)
			continue;

		documents.erase(it);
		++revision;
		return true;
	}

	return false;
}

void XmlEditSession::Clear() {
	documents.clear();
	warnings.clear();
	++revision;
}

std::vector<XmlDocument*> XmlEditSession::Documents() {
	std::vector<XmlDocument*> result;
	result.reserve(documents.size());

	for (auto& entry : documents)
		result.push_back(entry.doc.get());

	return result;
}

std::vector<std::string> XmlEditSession::Paths() const {
	std::vector<std::string> result;
	result.reserve(documents.size());

	// The spelling the link used, not the comparison key
	for (const auto& entry : documents)
		result.push_back(entry.doc->XmlPath());

	return result;
}

bool XmlEditSession::AnyDirty() const {
	for (const auto& entry : documents) {
		if (entry.doc->IsDirty())
			return true;
	}

	return false;
}
}
