#pragma once

#include "hdtBulletHelper.h"

#include <tinyxml2.h>

#include <cstdint>
#include <string>

namespace hdt
{
	// Pull-style reader over a parsed XML document, with the interface the
	// physics XML parsing was written against upstream (where it was a thin
	// wrapper around the XmlInspector streaming parser). Keeping the interface
	// means the parsing code stays comparable with upstream; the document
	// itself is parsed by the tinyxml2 the rest of the project already uses.
	//
	// Elements are reported as a StartTag, then their content, then an EndTag,
	// including empty elements such as <foo/>. Comments, declarations and
	// processing instructions are skipped.
	class XMLReader
	{
	public:
		enum class Inspected
		{
			None,
			StartTag,
			EndTag,
			Text
		};

		XMLReader(const uint8_t* data, size_t count);

		// Advances to the next item, false at the end of the document
		bool Inspect();
		Inspected GetInspected() const { return m_inspected; }

		// Element name at a StartTag or EndTag, empty otherwise
		std::string GetName() const;
		// Character data at Text, empty otherwise
		std::string GetValue() const;

		bool HasError() const;
		std::string GetErrorMessage() const;

		void skipCurrentElement();
		void nextStartElement();

		bool hasAttribute(const std::string& name);
		std::string getAttribute(const std::string& name);
		std::string getAttribute(const std::string& name, const std::string& def);

		float getAttributeAsFloat(const std::string& name);
		int getAttributeAsInt(const std::string& name);
		bool getAttributeAsBool(const std::string& name);

		std::string readText();
		float readFloat();
		int readInt();
		bool readBool();

		btVector3 readVector3();
		btQuaternion readQuaternion();
		btQuaternion readAxisAngle();
		btTransform readTransform();

	private:
		bool moveTo(tinyxml2::XMLNode* node);

		tinyxml2::XMLDocument m_doc;
		// Owns the text when the input had to be converted to UTF-8, since
		// tinyxml2 parses in place
		std::string m_converted;
		tinyxml2::XMLNode* m_node = nullptr;
		Inspected m_inspected = Inspected::None;
		bool m_started = false;
	};
}
