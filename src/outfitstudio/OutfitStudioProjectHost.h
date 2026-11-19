/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "project/ProjectHost.h"

#include <algorithm>
#include <vector>

class OutfitStudioFrame;

// Connects the project to the Outfit Studio window: progress bar, message boxes and GUI refreshes.
class OutfitStudioProjectHost : public NonInteractiveHost {
public:
	explicit OutfitStudioProjectHost(OutfitStudioFrame* inFrame)
		: frame(inFrame) {}

	// While disabled, prompts are answered with their defaults like NonInteractiveHost does.
	void SetInteractive(bool enable) { interactive = enable; }
	bool IsInteractive() const override { return interactive; }

	void StartProgress(const wxString& msg = "") override;
	void StartSubProgress(int min, int max) override;
	void UpdateProgress(int val, const wxString& msg = "") override;
	void EndProgress(const wxString& msg = "", bool forceEmpty = false) override;

	void ShowMessage(HostMessageLevel level, const wxString& msg, const wxString& title) override;
	HostAnswer Ask(const wxString& msg, const wxString& title, bool withCancel, HostAnswer defaultAnswer) override;
	bool ChooseMany(const wxString& msg, const wxString& title, const wxArrayString& choices, wxArrayInt& selections) override;
	bool ChooseOne(const wxString& msg, const wxString& title, const wxArrayString& choices, int& selection) override;
	bool GetText(const wxString& msg, const wxString& title, wxString& value) override;
	Mesh* GetShapeMesh(const std::string& shapeName) override;

	void UpdateShapeMesh(const std::string& shapeName) override;
	void OnAnimationChanged() override;
	void OnPartitionsChanged() override;
	void OnShapeDeleting(nifly::NiShape* shape) override;
	void DeleteSlidersForMerge(bool keepSliders, bool keepZaps) override;

private:
	OutfitStudioFrame* frame = nullptr;
	bool interactive = true;
};

// While it exists, the project host answers prompts with their defaults. Collects the messages
// of that time, so a caller can report what happened during an operation.
class UnattendedScope {
public:
	explicit UnattendedScope(OutfitStudioProjectHost& inHost)
		: host(inHost)
		, wasInteractive(inHost.IsInteractive())
		, firstMessage(inHost.GetMessages().size()) {
		host.SetInteractive(false);
	}

	~UnattendedScope() { host.SetInteractive(wasInteractive); }

	std::vector<HostMessage> Messages() const {
		const auto& all = host.GetMessages();
		return std::vector<HostMessage>(all.begin() + std::min(firstMessage, all.size()), all.end());
	}

private:
	OutfitStudioProjectHost& host;
	bool wasInteractive = true;
	size_t firstMessage = 0;
};
