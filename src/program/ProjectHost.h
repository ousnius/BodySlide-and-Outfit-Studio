/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include <string>
#include <vector>
#include <wx/arrstr.h>
#include <wx/string.h>

class Mesh;
class wxWindow;

namespace nifly {
class NiShape;
}

enum class HostMessageLevel { Info, Warning, Error };
enum class HostAnswer { Yes, No, Cancel };

struct HostMessage {
	HostMessageLevel level = HostMessageLevel::Info;
	std::string title;
	std::string text;
};

// Services an OutfitProject needs from whatever drives it: the Outfit Studio window,
// or an unattended caller such as headless automation or tests.
class IProjectHost {
public:
	virtual ~IProjectHost() = default;

	virtual void StartProgress(const wxString& WXUNUSED(msg) = "") {}
	virtual void StartSubProgress(int WXUNUSED(min), int WXUNUSED(max)) {}
	virtual void UpdateProgress(int WXUNUSED(val), const wxString& WXUNUSED(msg) = "") {}
	virtual void EndProgress(const wxString& WXUNUSED(msg) = "", bool WXUNUSED(forceEmpty) = false) {}

	// False while nobody can answer prompts. Callers then use their default answer
	// instead of asking, and messages are only logged and recorded.
	virtual bool IsInteractive() const = 0;

	virtual void ShowMessage(HostMessageLevel level, const wxString& msg, const wxString& title) = 0;
	virtual HostAnswer Ask(const wxString& msg, const wxString& title, bool withCancel, HostAnswer defaultAnswer) = 0;

	// 'selections' holds the preselected entries on input and the chosen ones on output.
	// Both return false if the choice was cancelled.
	virtual bool ChooseMany(const wxString& msg, const wxString& title, const wxArrayString& choices, wxArrayInt& selections) = 0;
	virtual bool ChooseOne(const wxString& msg, const wxString& title, const wxArrayString& choices, int& selection) = 0;

	// Returns false if cancelled. 'value' holds the default on input.
	virtual bool GetText(const wxString& msg, const wxString& title, wxString& value) = 0;

	// Parent for the dialogs that the project still creates itself.
	virtual wxWindow* GetDialogParent() { return nullptr; }

	// Editable mesh of a shape, or nullptr if there is none.
	virtual Mesh* GetShapeMesh(const std::string& WXUNUSED(shapeName)) { return nullptr; }

	virtual void OnAnimationChanged() {}
	virtual void OnPartitionsChanged() {}
	virtual void OnShapeDeleting(nifly::NiShape* WXUNUSED(shape)) {}

	// Deletes the sliders that are about to be replaced by merged reference sliders.
	virtual void DeleteSlidersForMerge(bool WXUNUSED(keepSliders), bool WXUNUSED(keepZaps)) {}

	// Display order of the project shapes. Empty if the host doesn't track one.
	virtual std::vector<std::string> GetShapeOrder() { return {}; }

	virtual bool GetShapeReferenceSource(nifly::NiShape* WXUNUSED(shape), std::string& WXUNUSED(outProjectFile), std::string& WXUNUSED(outProjectName)) {
		return false;
	}
	virtual void SetShapeReferenceSource(nifly::NiShape* WXUNUSED(shape), const std::string& WXUNUSED(projectFile), const std::string& WXUNUSED(projectName)) {}
};

// Host for unattended use: answers every prompt with its default, logs messages and
// keeps them for the caller to read back.
class NonInteractiveHost : public IProjectHost {
public:
	bool IsInteractive() const override { return false; }

	void ShowMessage(HostMessageLevel level, const wxString& msg, const wxString& title) override;
	HostAnswer Ask(const wxString& msg, const wxString& title, bool withCancel, HostAnswer defaultAnswer) override;
	bool ChooseMany(const wxString& msg, const wxString& title, const wxArrayString& choices, wxArrayInt& selections) override;
	bool ChooseOne(const wxString& msg, const wxString& title, const wxArrayString& choices, int& selection) override;
	bool GetText(const wxString& msg, const wxString& title, wxString& value) override;

	const std::vector<HostMessage>& GetMessages() const { return messages; }
	void ClearMessages() { messages.clear(); }

	// Logs and records a message the way an unattended host reports it.
	void RecordMessage(HostMessageLevel level, const wxString& msg, const wxString& title);

private:
	std::vector<HostMessage> messages;
};
