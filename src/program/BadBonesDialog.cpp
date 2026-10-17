/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "BadBonesDialog.h"

#include <cmath>
#include <wx/collpane.h>
#include <wx/wx.h>

using namespace nifly;

bool ShowBadBonesDialog(wxWindow* parent, BadBonesReport& report, AnimInfo& anim) {
	auto& shapeBBs = report.shapeBBs;
	auto& badCBs = report.badCBs;
	using ShapeBadBones = BadBonesReport::ShapeBadBones;
	using BadCustomBone = BadBonesReport::BadCustomBone;

	// Create dialog window
	wxDialog dlg(parent, -1, _("Bad Bones"), wxDefaultPosition, wxSize(800,600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	wxBoxSizer* topBox = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags sizerFlags = wxSizerFlags().Expand().Border(wxALL, 5);
	constexpr int wrapPixels = 800;
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

		wxRadioButton* rb = new wxRadioButton(wnd, wxID_ANY, _("Update skin (recommended)"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb->SetValue(1);
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = true;
		});
		boxSizer->Add(rb, sizerFlags);
		sbb.fixStanSkin = true;

		CollapsePane bcp(wnd, boxSizer, _("Details"), _("Bone"));
		for (auto& brp : bb)
			bcp.AddRow(brp.first, brp.second);
		bcp.Finish();

		rb = new wxRadioButton(wnd, wxID_ANY, _("Do nothing"));
		rb->Bind(wxEVT_RADIOBUTTON, [&](wxCommandEvent&) {
			sbb.fixStanSkin = false;
		});
		boxSizer->Add(rb, sizerFlags);
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
