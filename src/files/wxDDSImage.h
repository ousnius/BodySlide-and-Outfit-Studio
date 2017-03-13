#pragma once

#include "wx/defs.h"
#include "wx/image.h"


class WXDLLIMPEXP_CORE wxDDSHandler : public wxImageHandler
{
public:
	inline wxDDSHandler()
	{
		m_name = wxT("DDS file");
		m_extension = wxT("dds");
		m_type = wxBITMAP_TYPE_ANY;
		m_mime = wxT("image/dds");
	}

#if wxUSE_STREAMS
	virtual bool LoadFile(wxImage *image, wxInputStream& stream, bool verbose = true, int index = -1) wxOVERRIDE;
	virtual bool SaveFile(wxImage *image, wxOutputStream& stream, bool verbose = true) wxOVERRIDE;
protected:
	virtual bool DoCanRead(wxInputStream& stream) wxOVERRIDE;
#endif

private:
	wxDECLARE_DYNAMIC_CLASS(wxDDSHandler);

	/* Decompression functions cribbed from libsquish */
	void DecompressColor(unsigned char* outPixels, unsigned char* block, bool dxt1);
	int Unpack565(unsigned char* bytes, unsigned char* color);
};
