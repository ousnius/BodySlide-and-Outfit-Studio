/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "OutfitProject.h"

class OutfitStudioFrame;
class ConfigurationManager;
class RefTemplate;

class ConvertBodyReferenceDialog : public wxDialog {
public:
	ConvertBodyReferenceDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project, ConfigurationManager& config, const std::vector<RefTemplate>& refTemplates);
	~ConvertBodyReferenceDialog() override;

	void ConvertBodyReference() const;
	wxDECLARE_EVENT_TABLE();
private:
	OutfitStudioFrame* outfitStudio;
	OutfitProject* project;
	ConfigurationManager& config;
	const std::vector<RefTemplate>& refTemplates;

	int LoadReferenceTemplate(const wxString& refTemplate, bool keepZapSliders) const;
	bool AlertProgressError(int error, const wxString& title, const wxString& message) const;
};
