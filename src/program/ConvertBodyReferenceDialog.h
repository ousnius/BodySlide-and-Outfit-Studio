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

class ConvertBodyReferenceDialog : public wxWizard {
public:
	ConvertBodyReferenceDialog(OutfitStudioFrame* outfitStudio, OutfitProject* project, ConfigurationManager& config, const std::vector<RefTemplate>& refTemplates);
	~ConvertBodyReferenceDialog() override;

	bool Load();
	void ConvertBodyReference() const;
private:
	OutfitStudioFrame* outfitStudio;
	OutfitProject* project;
	ConfigurationManager& config;
	const std::vector<RefTemplate>& refTemplates;
	wxWizardPage* pg1;
	wxWizardPage* pg2;

	int LoadReferenceTemplate(const wxString& refTemplate, bool mergeSliders, bool mergeZaps) const;
	bool AlertProgressError(int error, const wxString& title, const wxString& message) const;

	wxDECLARE_EVENT_TABLE();
};
