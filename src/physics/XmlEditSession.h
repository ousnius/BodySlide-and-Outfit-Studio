/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Controller.h"
#include "XmlDocument.h"
#include "files/GameDataStream.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/*
The physics XMLs the preview is running on, held open as editable documents.

Everything the preview resolves goes through Resolver(), which adopts each file
into the session on the way and hands the simulation the session's text from
then on. That is the single injection point the whole editor rests on: the
system builder keeps reading a stream and knows nothing about editing, and every
rebuild after an edit reads what the editor holds instead of what is on disk.

The store belongs to the application frame, not to the editor window: if it died
with the dialog, closing the editor would silently revert the preview to the
file on disk.

Free of wx and Bullet. When to rebuild the simulation after an edit is the
caller's business.
*/
namespace Physics {
class XmlEditSession {
public:
	// Opens a physics XML the way the preview would, reporting where the
	// contents came from. Matches OutfitProject::GetPhysicsXmlStream, which is
	// what this is normally bound to.
	using SourceResolver = std::function<std::unique_ptr<std::istream>(const std::string& xmlPath, GameDataStream::FoundAt* foundAt)>;

	XmlEditSession() = default;

	XmlEditSession(const XmlEditSession&) = delete;
	XmlEditSession& operator=(const XmlEditSession&) = delete;

	void SetSourceResolver(SourceResolver resolver) { sourceResolver = std::move(resolver); }

	// The resolver to hand Controller::BuildFromNif. Serves the session's text
	// for documents it already holds, and adopts anything else it can read. A
	// held document without unsaved edits is read again first when its file
	// changed, so edits made outside the editor still reach the preview.
	//
	// A file tinyxml2 cannot parse is deliberately *not* adopted: the preview
	// gets the original bytes and behaves exactly as it did before the editor
	// existed, because the parser the simulation uses is not this one and must
	// not be held to its standards. Warnings() names those files.
	XmlStreamResolver Resolver();

	// The document for "xmlPath", or nullptr when the session does not hold it.
	// Paths match the way the rest of the physics code matches them: case
	// insensitively, and with either slash.
	XmlDocument* Find(const std::string& xmlPath);
	const XmlDocument* Find(const std::string& xmlPath) const;

	// Reads "xmlPath" through the source resolver and adopts it. Returns the
	// document it already held for that path, read again when its file
	// changed and it has no unsaved edits.
	XmlDocument* Open(const std::string& xmlPath, std::string& outError);

	// Adds an empty <system/> document that exists only in the session until it
	// is exported. Returns nullptr when a document for "xmlPath" already exists.
	XmlDocument* Create(const std::string& xmlPath);

	bool Close(const std::string& xmlPath);

	// Drops every document. Belongs with closing the project they describe, not
	// with stopping the simulation: edits have to outlive turning the preview off
	// and on again.
	void Clear();

	// Open documents in the order they were adopted.
	std::vector<XmlDocument*> Documents();
	std::vector<std::string> Paths() const;
	size_t Count() const { return documents.size(); }
	bool Empty() const { return documents.empty(); }

	// True when any document holds edits that are not on disk.
	bool AnyDirty() const;

	// Files the resolver could not adopt, one message each. Cleared by
	// ClearWarnings, which the caller does before each build.
	const std::vector<std::string>& Warnings() const { return warnings; }
	void ClearWarnings() { warnings.clear(); }

	// Changes whenever a document is adopted, closed or read again from its
	// file, so a view can tell that its tree of the documents is out of date.
	std::uint32_t Revision() const { return revision; }

	// The comparison key the session matches paths by.
	static std::string NormalizeXmlPath(const std::string& xmlPath);

private:
	struct Entry {
		std::string key;
		std::unique_ptr<XmlDocument> doc;
	};

	// Body of the resolver, so Open can share it.
	std::unique_ptr<std::istream> ResolveStream(const std::string& xmlPath, std::string* outError);

	const Entry* FindEntry(const std::string& xmlPath) const;
	// Reads a clean loose document again when its file no longer says what it holds
	void RefreshFromFile(XmlDocument& doc);

	std::vector<Entry> documents;
	SourceResolver sourceResolver;
	std::vector<std::string> warnings;
	std::uint32_t revision = 0;
};
}
