#include "../src/physics/XmlEditSession.h"

#include <catch2/catch_test_macros.hpp>

#include <tinyxml2.h>

#include <map>
#include <sstream>
#include <string>

using namespace Physics;

namespace {
// Stands in for OutfitProject::GetPhysicsXmlStream: a fixed set of files, and a
// record of what was asked for, so a test can tell an adopted document from one
// that was read again.
class FakeSource {
public:
	void Add(const std::string& xmlPath, std::string contents, std::string sourcePath, bool fromArchive = false) {
		File file;
		file.contents = std::move(contents);
		file.sourcePath = std::move(sourcePath);
		file.fromArchive = fromArchive;
		files[xmlPath] = std::move(file);
	}

	XmlEditSession::SourceResolver Resolver() {
		return [this](const std::string& xmlPath, std::string* outSourcePath, bool* outFromArchive) -> std::unique_ptr<std::istream> {
			++reads;

			auto it = files.find(xmlPath);
			if (it == files.end())
				return nullptr;

			if (outSourcePath)
				*outSourcePath = it->second.sourcePath;
			if (outFromArchive)
				*outFromArchive = it->second.fromArchive;

			return std::make_unique<std::istringstream>(it->second.contents);
		};
	}

	int reads = 0;

private:
	struct File {
		std::string contents;
		std::string sourcePath;
		bool fromArchive = false;
	};

	std::map<std::string, File> files;
};

std::string ReadAll(const std::unique_ptr<std::istream>& stream) {
	REQUIRE(stream != nullptr);
	std::ostringstream contents;
	contents << stream->rdbuf();
	return contents.str();
}

const char* const kSystemXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
							   "<system>\n"
							   "\t<bone name=\"NPC L Breast01\">\n"
							   "\t\t<mass>1.25</mass>\n"
							   "\t</bone>\n"
							   "</system>\n";
}

TEST_CASE("The edit session adopts what the preview resolves", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml", kSystemXml, "D:/Data/SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());
	REQUIRE(session.Empty());

	auto resolver = session.Resolver();
	const std::string served = ReadAll(resolver("SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml"));

	REQUIRE(session.Count() == 1);
	REQUIRE(source.reads == 1);
	REQUIRE(served.find("NPC L Breast01") != std::string::npos);

	XmlDocument* doc = session.Find("SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml");
	REQUIRE(doc != nullptr);

	SECTION("the source it was found at comes with it") {
		REQUIRE(doc->XmlPath() == "SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml");
		REQUIRE(doc->SourcePath() == "D:/Data/SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml");
		REQUIRE(doc->Origin() == XmlOrigin::Loose);
		REQUIRE_FALSE(doc->IsDirty());
	}

	SECTION("a second resolve is served from the session, not read again") {
		ReadAll(resolver("SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml"));
		REQUIRE(source.reads == 1);
		REQUIRE(session.Count() == 1);
	}

	SECTION("paths match however the link spells them") {
		REQUIRE(session.Find("SKSE\\Plugins\\hdtSkinnedMeshConfigs\\outfit.xml") == doc);
		REQUIRE(session.Find("skse/plugins/hdtskinnedmeshconfigs/OUTFIT.XML") == doc);

		// ...including through the resolver, which must not adopt the same file
		// twice under two spellings
		ReadAll(resolver("SKSE\\Plugins\\hdtSkinnedMeshConfigs\\OUTFIT.xml"));
		REQUIRE(session.Count() == 1);
		REQUIRE(source.reads == 1);
	}

	SECTION("Paths reports the spelling the link used") {
		const std::vector<std::string> paths = session.Paths();
		const std::vector<std::string> expected{"SKSE/Plugins/hdtSkinnedMeshConfigs/outfit.xml"};
		REQUIRE(paths == expected);
	}
}

TEST_CASE("The preview reads what the session holds, not what is on disk", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("outfit.xml", kSystemXml, "D:/outfit.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());

	auto resolver = session.Resolver();
	REQUIRE(ReadAll(resolver("outfit.xml")).find("1.25") != std::string::npos);

	XmlDocument* doc = session.Find("outfit.xml");
	REQUIRE(doc != nullptr);

	const ElementDesc* boneDesc = FindElement(ElementKind::Bone);
	REQUIRE(boneDesc != nullptr);
	const ChildDesc* massDesc = boneDesc->Child("mass");
	REQUIRE(massDesc != nullptr);

	tinyxml2::XMLElement* bone = doc->Root()->FirstChildElement("bone");
	REQUIRE(bone != nullptr);
	REQUIRE(doc->SetValue(bone, *massDesc, ValueVariant(4.5f)));
	REQUIRE(doc->IsDirty());

	// This is the whole point of routing the builder through the session: the
	// next rebuild simulates the edit without anything being written to disk.
	const std::string served = ReadAll(resolver("outfit.xml"));
	REQUIRE(served.find("4.5") != std::string::npos);
	REQUIRE(served.find("1.25") == std::string::npos);
	REQUIRE(source.reads == 1);
}

TEST_CASE("A file the editor cannot parse is left to the simulation", "[PhysicsEditSession]") {
	// Truncated on purpose: the parser the simulation uses is not this one, so
	// a file only this one rejects has to keep being previewable.
	const std::string broken = "<system><bone name=\"a\"><mass>1</mass>";

	FakeSource source;
	source.Add("broken.xml", broken, "D:/broken.xml");
	source.Add("notasystem.xml", "<hdtSkinnedMesh/>", "D:/notasystem.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());
	auto resolver = session.Resolver();

	SECTION("malformed XML comes through byte for byte") {
		REQUIRE(ReadAll(resolver("broken.xml")) == broken);
		REQUIRE(session.Empty());
		REQUIRE(session.Find("broken.xml") == nullptr);
		REQUIRE(session.Warnings().size() == 1);
		REQUIRE(session.Warnings()[0].find("broken.xml") != std::string::npos);
	}

	SECTION("so does a root element the editor does not recognize") {
		REQUIRE(ReadAll(resolver("notasystem.xml")) == "<hdtSkinnedMesh/>");
		REQUIRE(session.Empty());
	}

	SECTION("a file that is not there resolves to nothing and warns nobody") {
		REQUIRE(resolver("missing.xml") == nullptr);
		REQUIRE(session.Empty());

		// BuildFromNif already reports what it could not resolve; saying it
		// twice would be noise.
		REQUIRE(session.Warnings().empty());
	}

	SECTION("warnings belong to one build pass") {
		ReadAll(resolver("broken.xml"));
		REQUIRE(session.Warnings().size() == 1);
		session.ClearWarnings();
		REQUIRE(session.Warnings().empty());
	}
}

TEST_CASE("A document out of an archive knows it cannot be saved over", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("packed.xml", kSystemXml, "SKSE/Plugins/hdtSkinnedMeshConfigs/packed.xml", true);

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());

	std::string error;
	XmlDocument* doc = session.Open("packed.xml", error);
	REQUIRE(doc != nullptr);
	REQUIRE(doc->Origin() == XmlOrigin::Archive);

	// Not a writable location, but still worth carrying: it names the file and
	// gives the export dialog somewhere to start.
	REQUIRE(doc->SourcePath() == "SKSE/Plugins/hdtSkinnedMeshConfigs/packed.xml");
}

TEST_CASE("Opening a document twice returns the one already held", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("outfit.xml", kSystemXml, "D:/outfit.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());

	std::string error;
	XmlDocument* first = session.Open("outfit.xml", error);
	REQUIRE(first != nullptr);
	REQUIRE(error.empty());

	XmlDocument* second = session.Open("outfit.xml", error);
	REQUIRE(second == first);
	REQUIRE(source.reads == 1);

	SECTION("and a document that cannot be opened reports why") {
		XmlDocument* missing = session.Open("missing.xml", error);
		REQUIRE(missing == nullptr);
		REQUIRE_FALSE(error.empty());
	}
}

TEST_CASE("A new document exists only in the session", "[PhysicsEditSession]") {
	XmlEditSession session;

	XmlDocument* doc = session.Create("SKSE/Plugins/hdtSkinnedMeshConfigs/new.xml");
	REQUIRE(doc != nullptr);
	REQUIRE(doc->Origin() == XmlOrigin::New);
	REQUIRE(doc->SourcePath().empty());
	REQUIRE(doc->Root() != nullptr);
	REQUIRE(std::string(doc->Root()->Name()) == "system");

	// One document per path, whichever way it got there
	REQUIRE(session.Create("skse\\plugins\\hdtskinnedmeshconfigs\\new.xml") == nullptr);
	REQUIRE(session.Count() == 1);

	SECTION("and the preview can be built from it without any file existing") {
		auto resolver = session.Resolver();
		REQUIRE(ReadAll(resolver("SKSE/Plugins/hdtSkinnedMeshConfigs/new.xml")).find("<system") != std::string::npos);
	}
}

TEST_CASE("The session tracks what a rebuild still owes the simulation", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("outfit.xml", kSystemXml, "D:/outfit.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());

	std::string error;
	REQUIRE(session.Open("outfit.xml", error) != nullptr);
	REQUIRE_FALSE(session.RebuildPending());

	const std::uint32_t revision = session.Revision();

	SECTION("a change that can be patched into the running simulation does not ask for one") {
		session.NotifyChanged("outfit.xml", false);
		REQUIRE_FALSE(session.RebuildPending());

		// It is still a change: a view showing the document is out of date
		REQUIRE(session.Revision() != revision);
	}

	SECTION("a structural change asks for one until it has had it") {
		session.NotifyChanged("outfit.xml", true);
		REQUIRE(session.RebuildPending());

		session.ClearRebuildRequest();
		REQUIRE_FALSE(session.RebuildPending());
	}

	SECTION("a change to a document the session does not hold is not its business") {
		session.NotifyChanged("elsewhere.xml", true);
		REQUIRE_FALSE(session.RebuildPending());
		REQUIRE(session.Revision() == revision);
	}

	SECTION("closing a document the preview is running on asks for one") {
		REQUIRE(session.Close("OUTFIT.XML"));
		REQUIRE(session.Empty());
		REQUIRE(session.RebuildPending());
		REQUIRE_FALSE(session.Close("outfit.xml"));
	}
}

TEST_CASE("Unsaved edits are visible across the whole session", "[PhysicsEditSession]") {
	FakeSource source;
	source.Add("a.xml", kSystemXml, "D:/a.xml");
	source.Add("b.xml", kSystemXml, "D:/b.xml");

	XmlEditSession session;
	session.SetSourceResolver(source.Resolver());

	std::string error;
	REQUIRE(session.Open("a.xml", error) != nullptr);
	XmlDocument* b = session.Open("b.xml", error);
	REQUIRE(b != nullptr);
	REQUIRE_FALSE(session.AnyDirty());

	const ChildDesc* massDesc = FindElement(ElementKind::Bone)->Child("mass");
	REQUIRE(massDesc != nullptr);
	REQUIRE(b->SetValue(b->Root()->FirstChildElement("bone"), *massDesc, ValueVariant(9.0f)));
	REQUIRE(session.AnyDirty());

	SECTION("documents come back in the order they were adopted") {
		const std::vector<XmlDocument*> docs = session.Documents();
		REQUIRE(docs.size() == 2);
		REQUIRE(docs[0]->XmlPath() == "a.xml");
		REQUIRE(docs[1] == b);
	}

	SECTION("clearing forgets the edits along with everything else") {
		session.Clear();
		REQUIRE(session.Empty());
		REQUIRE_FALSE(session.AnyDirty());

		// The binding went with them, so nothing can be read back by accident
		REQUIRE(session.Resolver()("a.xml") == nullptr);
	}
}
