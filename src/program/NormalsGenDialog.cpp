#include "NormalsGenDialog.h"

NormalsGenDialog::NormalsGenDialog( wxWindow* parent )
:
baseNormGenDlg( parent )
{
	layersProperties->Connect(wxEVT_PG_SELECTED, wxPropertyGridEventHandler(NormalsGenDialog::doPropSelected), NULL, this);

	layersProperties->SetSortFunction([](wxPropertyGrid* propGrid, wxPGProperty* p1, wxPGProperty* p2)->int {
		if (p1->GetClientData() >= p2->GetClientData())
			return 1;
		return 0;
	});

	buttonDeleteLayer->Disable();
	buttonMoveUp->Disable();
}

void NormalsGenDialog::doPropertyChanged( wxPropertyGridEvent& event )
{
int here;
int there;
int everywhere;
}

void NormalsGenDialog::doAddLayer( wxCommandEvent& event )
{
	static int n = 1;
	n++;
	wxString nameStr = _("Layer ") + wxString::Format("%d", n);
	wxString internalName = nameStr;

	wxTextEntryDialog entry(this, _("Enter a name for the new layer."), _("Name new layer"), nameStr);
	if (entry.ShowModal() == wxID_CANCEL) {
		return;
	}
	nameStr = entry.GetValue();

	wxPGProperty* p = layersProperties->GetSelectedProperty();
	if (!p) {
		layersProperties->Append(new wxPropertyCategory(nameStr, internalName));
		return;
	}
	wxPropertyGridIterator it;
	for (it = layersProperties->GetIterator(wxPG_ITERATE_CATEGORIES, p); !it.AtEnd(); it++) {
		if (*it == p)
			continue;
		if((*it)->GetName() != "Background") {	
			layersProperties->Insert(*it, new wxPropertyCategory(nameStr, internalName));
		}
	}
	layersProperties->Append(new wxPropertyCategory(nameStr, internalName));

}

void NormalsGenDialog::doMoveUpLayer( wxCommandEvent& event )
{
	wxPGProperty* prev = nullptr;
	wxPGProperty* me = nullptr;

	wxPGProperty* s = layersProperties->GetSelectedProperty();
	if (!s || !s->IsCategory() || s->GetName() == _("Background")) {
		return;
	}

	int n = 1;
	for (auto it = layersProperties->GetIterator(wxPG_ITERATE_CATEGORIES); !it.AtEnd(); it++) {
		wxPGProperty* p = it.GetProperty();
		p->SetClientData((void*)n++);	
		if (p == s) {
		me = p;
		} else 
		if (!me && p->GetName() != "Background") {
			prev = p;
		}	
	}

	if (!me || !prev)
		return;

	int t = (int)me->GetClientData();
	me->SetClientData(prev->GetClientData());
	prev->SetClientData((void*)t);

	layersProperties->Sort();
	layersProperties->Refresh();
}

void NormalsGenDialog::doDeleteLayer( wxCommandEvent& event )
{
	wxPGProperty* p = layersProperties->GetSelectedProperty();
	if (p && p->IsCategory() && p->GetName() != _("Background")) {
		p->DeleteChildren();
		layersProperties->DeleteProperty(p);
	} 
}

void NormalsGenDialog::doSetOutputFileName( wxFileDirPickerEvent& event )
{
// TODO: Implement doSetOutputFileName
}

void NormalsGenDialog::doPreviewNormalMap( wxCommandEvent& event )
{
// TODO: Implement doPreviewNormalMap
}

void NormalsGenDialog::doGenerateNormalMap( wxCommandEvent& event )
{
// TODO: Implement doGenerateNormalMap
}


void NormalsGenDialog::doPropSelected(wxPropertyGridEvent& event) {
	wxPGProperty* p = layersProperties->GetSelectedProperty();
	if (p && p->IsCategory() && p->GetName() != "Background") {
		buttonDeleteLayer->Enable();
		buttonMoveUp->Enable();
	}
	else {
		buttonDeleteLayer->Disable();
		buttonMoveUp->Disable();
	}
}