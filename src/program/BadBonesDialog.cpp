/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BadBonesDialog.h"

#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <wx/collpane.h>
#include <wx/wx.h>

using namespace nifly;

bool ShowBadBonesDialog(wxWindow* parent, BadBonesReport& report, AnimInfo& anim) {
	auto& shapeBBs = report.shapeBBs;
	auto& badCBs = report.badCBs;
	using ShapeBadBones = BadBonesReport::ShapeBadBones;
	using BadCustomBone = BadBonesReport::BadCustomBone;

	// Create dialog window
	wxDialog dlg(parent, -1, _("Bad Bones"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	dlg.SetSize(dlg.FromDIP(wxSize(1000, 700)));
	wxBoxSizer* topBox = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
	// Leaves room for the frame borders and the scroll bar
	const int wrapPixels = dlg.FromDIP(900);
	wxScrolledWindow* wnd = new wxScrolledWindow(&dlg);
	topBox->Add(wnd, wxSizerFlags().Expand().Proportion(1));
	wxBoxSizer* scrollBox = new wxBoxSizer(wxVERTICAL);
	wnd->SetScrollRate(30, 50);

	// A helper class for creating the collapsible panes
	struct CollapsePane {
		wxScrolledWindow* wnd;
		wxCollapsiblePane* collapse;
		wxFlexGridSizer* collPaneBox;
		CollapsePane(wxScrolledWindow* wi, wxSizer* boxSizer, const wxString& label, const wxString& col1Label):
			wnd(wi) {
			wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
			collapse = new wxCollapsiblePane(wnd, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxCP_DEFAULT_STYLE | wxCP_NO_TLW_RESIZE);
			boxSizer->Add(collapse, sizerFlags);
			collPaneBox = new wxFlexGridSizer(4);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, col1Label), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in rotation")), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in translation")), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, _("Error in scale")), sizerFlags);
		}
		void AddRow(const wxString& label, const MatTransform& t) {
			wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
			float rotErr = RotMatToVec(t.rotation).length();
			float trErr = t.translation.length();
			float scErr = std::fabs(t.scale - 1.0f);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, label), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << rotErr), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << trErr), sizerFlags);
			collPaneBox->Add(new wxStaticText(collapse->GetPane(), wxID_ANY, wxString() << scErr), sizerFlags);
		}
		void Finish() {
			collapse->GetPane()->SetSizerAndFit(collPaneBox);
			wxScrolledWindow* wndl = wnd;
			collapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [wndl](wxCollapsiblePaneEvent&) { wndl->FitInside(); });
		}
	};

	// The radio buttons of each frame, so "Set for all" can update them
	struct StandardChoice {
		ShapeBadBones* sbb = nullptr;
		wxRadioButton* update = nullptr;
		wxRadioButton* nothing = nullptr;
	};
	struct CustomChoice {
		BadCustomBone* bcb = nullptr;
		int recFixtype = BadCustomBone::DoNothing;
		std::string recTrustShape;
		wxRadioButton* node = nullptr;
		std::map<std::string, wxRadioButton*> skins;
		wxRadioButton* nothing = nullptr;
	};
	std::vector<StandardChoice> standardChoices;
	std::vector<CustomChoice> customChoices;

	// Add a frame for each shape with bad standard bones
	for (auto& sbbp : shapeBBs) {
		const std::string &shapeName = sbbp.first;
		ShapeBadBones& sbb = sbbp.second;
		auto& bb = sbb.badStandard;
		if (bb.empty())
			continue;

		wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, wnd, wxString::Format(_("Bad standard bones for shape \"%s\""), shapeName));
		scrollBox->Add(boxSizer, sizerFlags);

		wxString label = wxString::Format(_("%zu bones in shape \"%s\" had inconsistencies between their NIF skin transforms and the standard skeleton:\n"), bb.size(), shapeName);

		auto brit = bb.begin();
		label << brit->first;
		++brit;

		while (brit != bb.end()) {
			label << ", " << brit->first;
			++brit;
		}

		wxStaticText* ctrl = new wxStaticText(wnd, -1, label);
		ctrl->Wrap(wrapPixels);
		boxSizer->Add(ctrl, sizerFlags);

		StandardChoice sc;
		sc.sbb = &sbb;

		wxRadioButton* rb = new wxRadioButton(wnd, wxID_ANY, _("Update skin (recommended)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb->SetValue(1);
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = true;
		});
		boxSizer->Add(rb, sizerFlags);
		sbb.fixStanSkin = true;
		sc.update = rb;

		CollapsePane bcp(wnd, boxSizer, _("Details"), _("Bone"));
		for (auto& brp : bb)
			bcp.AddRow(brp.first, brp.second);
		bcp.Finish();

		rb = new wxRadioButton(wnd, wxID_ANY, _("Do nothing"));
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = false;
		});
		boxSizer->Add(rb, sizerFlags);
		sc.nothing = rb;
		standardChoices.push_back(std::move(sc));
	}

	// Add a frame for each bad custom bone
	for (auto& bcbp : badCBs) {
		const std::string& bone = bcbp.first;
		BadCustomBone& bcb = bcbp.second;

		wxStaticBoxSizer* boxSizer = new wxStaticBoxSizer(wxVERTICAL, wnd, wxString::Format(_("Bad Custom Bone \"%s\""), bone));
		scrollBox->Add(boxSizer, sizerFlags);

		wxString label = wxString::Format(_("Custom bone \"%s\" had inconsistent NIF node and skin transforms for the following shapes:\n\""), bone);

		auto bsit = bcb.badShapes.begin();
		label << *bsit;
		++bsit;

		while (bsit != bcb.badShapes.end()) {
			label << "\", \"" << *bsit;
			++bsit;
		}
		label << "\"";

		wxStaticText* ctrl = new wxStaticText(wnd, -1, label);
		ctrl->Wrap(wrapPixels);
		boxSizer->Add(ctrl, sizerFlags);

		CustomChoice cc;
		cc.bcb = &bcb;
		cc.recFixtype = bcb.fixtype;
		cc.recTrustShape = bcb.trustShape;

		wxString tnLabel;
		if (bcb.fixtype == BadCustomBone::TrustNode) {
			tnLabel = "";
			if (bcb.goodShapes.size() > 1)
				tnLabel << _("Trust node and skins \"");
			else
				tnLabel << _("Trust node and skin \"");
			auto gsit = bcb.goodShapes.begin();
			tnLabel << *gsit;
			++gsit;
			while (gsit != bcb.goodShapes.end()) {
				tnLabel << "\", \"" << *gsit;
				++gsit;
			}
			tnLabel << _("\", and update other skins (recommended)");
		}
		else if (bcb.badShapes.size() > 1)
			tnLabel = _("Trust node, and update skins");
		else
			tnLabel = _("Trust node, and update skin");
		wxRadioButton* rb = new wxRadioButton(wnd, wxID_ANY, tnLabel, wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		if (bcb.fixtype == BadCustomBone::TrustNode)
			rb->SetValue(1);
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			bcb.fixtype = BadCustomBone::TrustNode;
		});
		boxSizer->Add(rb, sizerFlags);
		cc.node = rb;

		CollapsePane ncp(wnd, boxSizer, _("Details"), _("Skin"));
		for (const std::string& shapeName : bcb.badShapes) {
			MatTransform t = shapeBBs[shapeName].badCustom[bone];
			ncp.AddRow(shapeName, t);
		}
		ncp.Finish();

		for (const std::string& shapeName : bcb.badShapes) {
			wxString sLabel;
			sLabel << _("Trust skin \"") << shapeName;
			if (bcb.badShapes.size() == 1 && bcb.goodShapes.empty())
				sLabel << _("\", and update node");
			else
				sLabel << _("\", and update node and other skins");
			rb = new wxRadioButton(wnd, wxID_ANY, sLabel);
			rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
				bcb.fixtype = BadCustomBone::TrustSkin;
				bcb.trustShape = shapeName;
			});
			if (bcb.fixtype == BadCustomBone::TrustSkin && bcb.trustShape == shapeName)
				rb->SetValue(1);
			boxSizer->Add(rb, sizerFlags);
			cc.skins[shapeName] = rb;

			CollapsePane scp(wnd, boxSizer, _("Details"), _("With"));
			scp.AddRow(_("Node"), shapeBBs[shapeName].badCustom[bone]);
			for (const std::string& shapeName2 : bcb.badShapes) {
				if (shapeName == shapeName2)
					continue;
				MatTransform skinToBone1, skinToBone2;
				anim.GetXFormSkinToBone(shapeName, bone, skinToBone1);
				anim.GetXFormSkinToBone(shapeName2, bone, skinToBone2);
				MatTransform residual = skinToBone1.ComposeTransforms(skinToBone2.InverseTransform());
				scp.AddRow(shapeName2, residual);
			}
			scp.Finish();
		}

		rb = new wxRadioButton(wnd, wxID_ANY, _("Do nothing"));
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			bcb.fixtype = BadCustomBone::DoNothing;
		});
		if (bcb.fixtype == BadCustomBone::DoNothing)
			rb->SetValue(1);
		boxSizer->Add(rb, sizerFlags);
		cc.nothing = rb;
		customChoices.push_back(std::move(cc));
	}

	// Add buttons that set the fix of every frame at once
	if (standardChoices.size() + customChoices.size() > 1) {
		// Selects the radio buttons that match the fixes in the report
		auto syncRadios = [&standardChoices, &customChoices]() {
			for (auto& sc : standardChoices) {
				if (sc.sbb->fixStanSkin)
					sc.update->SetValue(true);
				else
					sc.nothing->SetValue(true);
			}

			for (auto& cc : customChoices) {
				if (cc.bcb->fixtype == BadCustomBone::TrustNode)
					cc.node->SetValue(true);
				else if (cc.bcb->fixtype == BadCustomBone::TrustSkin) {
					auto it = cc.skins.find(cc.bcb->trustShape);
					if (it != cc.skins.end())
						it->second->SetValue(true);
				}
				else
					cc.nothing->SetValue(true);
			}
		};

		wxStaticBoxSizer* allSizer = new wxStaticBoxSizer(wxVERTICAL, &dlg, _("Set for all"));
		wxWindow* allBox = allSizer->GetStaticBox();
		wxFlexGridSizer* allGrid = new wxFlexGridSizer(2);
		allSizer->Add(allGrid, sizerFlags);

		auto addButton = [&](wxSizer* rowSizer, const wxString& label, std::function<void()> apply) {
			wxButton* btn = new wxButton(allBox, wxID_ANY, label);
			btn->Bind(wxEVT_BUTTON, [apply, syncRadios](wxCommandEvent&) {
				apply();
				syncRadios();
			});
			rowSizer->Add(btn, wxSizerFlags().Border(wxRIGHT, 5));
			return btn;
		};

		if (!standardChoices.empty()) {
			allGrid->Add(new wxStaticText(allBox, wxID_ANY, _("Standard bones:")), wxSizerFlags().CenterVertical().Border(wxALL, 5));
			wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
			allGrid->Add(rowSizer, wxSizerFlags().CenterVertical().Border(wxALL, 5));

			addButton(rowSizer, _("Update skin"), [&standardChoices]() {
				for (auto& sc : standardChoices)
					sc.sbb->fixStanSkin = true;
			});
			addButton(rowSizer, _("Do nothing"), [&standardChoices]() {
				for (auto& sc : standardChoices)
					sc.sbb->fixStanSkin = false;
			});
		}

		if (!customChoices.empty()) {
			allGrid->Add(new wxStaticText(allBox, wxID_ANY, _("Custom bones:")), wxSizerFlags().CenterVertical().Border(wxALL, 5));
			wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
			allGrid->Add(rowSizer, wxSizerFlags().CenterVertical().Border(wxALL, 5));

			addButton(rowSizer, _("Recommended"), [&customChoices]() {
				for (auto& cc : customChoices) {
					cc.bcb->fixtype = cc.recFixtype;
					cc.bcb->trustShape = cc.recTrustShape;
				}
			});
			addButton(rowSizer, _("Trust node"), [&customChoices]() {
				for (auto& cc : customChoices)
					cc.bcb->fixtype = BadCustomBone::TrustNode;
			});
			addButton(rowSizer, _("Do nothing"), [&customChoices]() {
				for (auto& cc : customChoices)
					cc.bcb->fixtype = BadCustomBone::DoNothing;
			});

			// Every shape that can be trusted for at least one bone
			std::set<std::string> skinShapes;
			for (auto& cc : customChoices)
				for (auto& skin : cc.skins)
					skinShapes.insert(skin.first);

			wxChoice* skinChoice = new wxChoice(allBox, wxID_ANY);
			for (const std::string& shapeName : skinShapes)
				skinChoice->Append(wxString::FromUTF8(shapeName));
			skinChoice->SetSelection(0);
			skinChoice->SetToolTip(_("Custom bones that this shape doesn't use keep their fix. Where this shape's skin already matches the node, the node is trusted."));

			wxButton* skinBtn = addButton(rowSizer, _("Trust skin:"), [&customChoices, skinChoice]() {
				std::string shapeName = skinChoice->GetStringSelection().ToUTF8().data();
				for (auto& cc : customChoices) {
					if (cc.bcb->badShapes.count(shapeName)) {
						cc.bcb->fixtype = BadCustomBone::TrustSkin;
						cc.bcb->trustShape = shapeName;
					}
					else if (cc.bcb->goodShapes.count(shapeName))
						cc.bcb->fixtype = BadCustomBone::TrustNode;
				}
			});
			skinBtn->SetToolTip(skinChoice->GetToolTipText());
			rowSizer->Add(skinChoice, wxSizerFlags().CenterVertical());
		}

		topBox->Insert(0, allSizer, sizerFlags);
	}

	// Finish building the dialog window
	wnd->SetSizer(scrollBox);
	wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer;
	buttonSizer->AddButton(new wxButton(&dlg, wxID_OK));
	buttonSizer->AddButton(new wxButton(&dlg, wxID_CANCEL, _("Fix nothing")));
	buttonSizer->Realize();
	topBox->Add(buttonSizer, sizerFlags);
	dlg.SetSizer(topBox);

	return dlg.ShowModal() != wxID_CANCEL;
}
