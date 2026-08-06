#ifdef USE_BULLET

#include "XmlReader.h"
#include "hdtStringUtils.h"
#include <charconv>

namespace hdt
{
	static inline float convertFloat(const std::string& str)
	{
		// Trim first (from_chars rejects surrounding spaces), then turn a decimal
		// comma into a point so locale-independent parsing still accepts it.
		std::string s = TrimAsciiWhitespace(str);
		size_t start_pos = s.find(",");
		if (start_pos != std::string::npos)
			s.replace(start_pos, 1, ".");

		float ret{};
		const char* begin = s.data();
		const char* end = begin + s.size();
		// strtof accepted a leading '+'; from_chars doesn't, so skip it.
		if (begin != end && *begin == '+')
			++begin;
		auto [ptr, ec] = std::from_chars(begin, end, ret);
		if (ec != std::errc() || ptr != end)
			throw std::string("not a float value");
		return ret;
	}

	static inline int convertInt(const std::string& str)
	{
		// Trim first: from_chars rejects surrounding spaces and a leading '+'.
		std::string s = TrimAsciiWhitespace(str);
		const char* begin = s.data();
		const char* end = begin + s.size();
		if (begin != end && *begin == '+')
			++begin;

		int radix = 10;
		if (!s.compare(0, 2, "0x")) {
			radix = 16;
			begin += 2;
		} else if (s.length() > 1 && s[0] == '0') {
			begin += 1;
			radix = 8;
		}

		int ret{};
		auto [ptr, ec] = std::from_chars(begin, end, ret, radix);
		if (ec != std::errc() || ptr != end)
			throw std::string("not a int value");
		return ret;
	}

	static inline bool convertBool(const std::string& str)
	{
		const std::string s = TrimAsciiWhitespace(str);
		if (s == "true" || s == "1")
			return true;
		if (s == "false" || s == "0")
			return false;
		throw std::string("not a boolean");
	}

	namespace
	{
		// tinyxml2 only reads UTF-8. Physics XMLs saved as UTF-16 (which some
		// editors do by default) are converted so they keep working; anything
		// else is handed over untouched, BOM included, which tinyxml2 skips.
		bool ConvertUtf16ToUtf8(const uint8_t* data, size_t count, std::string& out)
		{
			const bool littleEndian = count >= 2 && data[0] == 0xFF && data[1] == 0xFE;
			const bool bigEndian = count >= 2 && data[0] == 0xFE && data[1] == 0xFF;
			if (!littleEndian && !bigEndian)
				return false;

			out.clear();
			out.reserve(count / 2);

			for (size_t i = 2; i + 1 < count; i += 2) {
				uint32_t cp = littleEndian ? static_cast<uint32_t>(data[i]) | (static_cast<uint32_t>(data[i + 1]) << 8)
										   : (static_cast<uint32_t>(data[i]) << 8) | static_cast<uint32_t>(data[i + 1]);

				// Combine a surrogate pair into one code point
				if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < count) {
					const uint32_t low = littleEndian
						? static_cast<uint32_t>(data[i + 2]) | (static_cast<uint32_t>(data[i + 3]) << 8)
						: (static_cast<uint32_t>(data[i + 2]) << 8) | static_cast<uint32_t>(data[i + 3]);

					if (low >= 0xDC00 && low <= 0xDFFF) {
						cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
						i += 2;
					}
				}

				if (cp < 0x80) {
					out.push_back(static_cast<char>(cp));
				} else if (cp < 0x800) {
					out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
					out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
				} else if (cp < 0x10000) {
					out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
					out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
					out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
				} else {
					out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
					out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
					out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
					out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
				}
			}

			return true;
		}

		// Only elements and character data are reported; everything else
		// (comments, declarations, processing instructions) is stepped over.
		tinyxml2::XMLNode* NextReportable(tinyxml2::XMLNode* node)
		{
			for (; node; node = node->NextSibling()) {
				if (node->ToElement() || node->ToText())
					return node;
			}
			return nullptr;
		}
	}

	XMLReader::XMLReader(const uint8_t* data, size_t count)
	{
		if (ConvertUtf16ToUtf8(data, count, m_converted))
			m_doc.Parse(m_converted.data(), m_converted.size());
		else
			m_doc.Parse(reinterpret_cast<const char*>(data), count);
	}

	bool XMLReader::moveTo(tinyxml2::XMLNode* node)
	{
		if (!node) {
			m_inspected = Inspected::None;
			return false;
		}

		m_node = node;
		m_inspected = node->ToElement() ? Inspected::StartTag : Inspected::Text;
		return true;
	}

	bool XMLReader::Inspect()
	{
		if (!m_started) {
			m_started = true;
			return moveTo(NextReportable(m_doc.FirstChild()));
		}

		if (!m_node) {
			m_inspected = Inspected::None;
			return false;
		}

		// Entering an element: its content comes next, or it ends right away
		if (m_inspected == Inspected::StartTag) {
			if (auto* child = NextReportable(m_node->FirstChild()))
				return moveTo(child);

			m_inspected = Inspected::EndTag;
			return true;
		}

		// Done with this item: on to the next one, or out of the parent
		if (auto* sibling = NextReportable(m_node->NextSibling()))
			return moveTo(sibling);

		auto* parent = m_node->Parent();
		if (!parent || parent->ToDocument()) {
			m_node = nullptr;
			m_inspected = Inspected::None;
			return false;
		}

		m_node = parent;
		m_inspected = Inspected::EndTag;
		return true;
	}

	std::string XMLReader::GetName() const
	{
		auto* element = m_node ? m_node->ToElement() : nullptr;
		if (!element || m_inspected == Inspected::None)
			return std::string();

		const char* name = element->Name();
		return name ? name : std::string();
	}

	std::string XMLReader::GetValue() const
	{
		if (m_inspected != Inspected::Text || !m_node)
			return std::string();

		const char* value = m_node->Value();
		return value ? value : std::string();
	}

	bool XMLReader::HasError() const
	{
		return m_doc.Error();
	}

	std::string XMLReader::GetErrorMessage() const
	{
		if (!m_doc.Error())
			return std::string();

		std::string message = m_doc.ErrorStr() ? m_doc.ErrorStr() : "unknown error";
		if (m_doc.ErrorLineNum() > 0)
			message += " (line " + std::to_string(m_doc.ErrorLineNum()) + ")";

		return message;
	}

	void XMLReader::skipCurrentElement()
	{
		if (GetInspected() == Inspected::EndTag)
			return;

		int currentDepth = 1;
		while (currentDepth && Inspect()) {
			switch (GetInspected()) {
			case Inspected::StartTag:
				++currentDepth;
				break;
			case Inspected::EndTag:
				--currentDepth;
				break;
			}
		}
	}

	void XMLReader::nextStartElement()
	{
		while (Inspect() && GetInspected() != Inspected::StartTag);
	}

	bool XMLReader::hasAttribute(const std::string& a_name)
	{
		auto* element = m_node ? m_node->ToElement() : nullptr;
		return element && element->Attribute(a_name.c_str());
	}

	std::string XMLReader::getAttribute(const std::string& a_name)
	{
		auto* element = m_node ? m_node->ToElement() : nullptr;
		const char* value = element ? element->Attribute(a_name.c_str()) : nullptr;
		if (!value)
			throw std::string("missing attribute : " + a_name);

		return value;
	}

	std::string XMLReader::getAttribute(const std::string& a_name, const std::string& def)
	{
		auto* element = m_node ? m_node->ToElement() : nullptr;
		const char* value = element ? element->Attribute(a_name.c_str()) : nullptr;
		return value ? value : def;
	}

	float XMLReader::getAttributeAsFloat(const std::string& a_name)
	{
		return convertFloat(getAttribute(a_name));
	}

	int XMLReader::getAttributeAsInt(const std::string& a_name)
	{
		return convertInt(getAttribute(a_name));
	}

	bool XMLReader::getAttributeAsBool(const std::string& a_name)
	{
		return convertBool(getAttribute(a_name));
	}

	std::string XMLReader::readText()
	{
		Inspect();
		auto ret = GetValue();
		skipCurrentElement();
		return ret;
	}

	float XMLReader::readFloat()
	{
		Inspect();
		auto ret = convertFloat(GetValue());
		skipCurrentElement();
		return ret;
	}

	int XMLReader::readInt()
	{
		Inspect();
		auto ret = convertInt(GetValue());
		skipCurrentElement();
		return ret;
	}

	bool XMLReader::readBool()
	{
		Inspect();
		auto ret = convertBool(GetValue());
		skipCurrentElement();
		return ret;
	}

	btVector3 XMLReader::readVector3()
	{
		float x = getAttributeAsFloat("x");
		float y = getAttributeAsFloat("y");
		float z = getAttributeAsFloat("z");
		skipCurrentElement();
		return btVector3(x, y, z);
	}

	btQuaternion XMLReader::readQuaternion()
	{
		float x = getAttributeAsFloat("x");
		float y = getAttributeAsFloat("y");
		float z = getAttributeAsFloat("z");
		float w = getAttributeAsFloat("w");
		skipCurrentElement();
		btQuaternion q(x, y, z, w);
		if (btFuzzyZero(q.length2()))
			q = btQuaternion::getIdentity();
		else
			q.normalize();
		return q;
	}

	btQuaternion XMLReader::readAxisAngle()
	{
		float x = getAttributeAsFloat("x");
		float y = getAttributeAsFloat("y");
		float z = getAttributeAsFloat("z");
		float w = getAttributeAsFloat("angle");
		skipCurrentElement();
		btQuaternion q;
		btVector3 axis(x, y, z);
		if (axis.fuzzyZero()) {
			axis.setX(1);
			w = 0;
		} else
			axis.normalize();
		q.setRotation(axis, w);
		return q;
	}

	btTransform XMLReader::readTransform()
	{
		btTransform ret(btTransform::getIdentity());
		while (Inspect()) {
			switch (GetInspected()) {
			case Inspected::StartTag:
				if (GetName() == "basis")
					ret.setRotation(readQuaternion());
				else if (GetName() == "basis-axis-angle")
					ret.setRotation(readAxisAngle());
				else if (GetName() == "origin")
					ret.setOrigin(readVector3());
				break;
			case Inspected::EndTag:
				return ret;
			}
		}
		return ret;
	}
}

#endif  // USE_BULLET
