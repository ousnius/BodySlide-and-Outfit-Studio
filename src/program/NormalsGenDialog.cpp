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
	wxPGProperty* categoryBackground;
	wxPGProperty* m_propertyGridItem2;
	wxPGProperty* m_propertyGridItem7;
	wxPGProperty* m_propertyGridItem6;


	categoryBackground = layersProperties->Append(new wxPropertyCategory(_("Background"), _("Background")));
	m_propertyGridItem2 = layersProperties->Append(new wxImageFileProperty(_("Background File"), _("Background File")));
	layersProperties->SetPropertyHelpString(m_propertyGridItem2, _("Filename source for this layer.  "));
	m_propertyGridItem7 = layersProperties->Append(new wxColourProperty(_("Color"), _("Color")));
	layersProperties->SetPropertyHelpString(m_propertyGridItem7, _("Solid background color (If File is not set)"));
	m_propertyGridItem6 = layersProperties->Append(new wxEnumProperty(_("Resolution"), _("Resolution")));
	layersProperties->SetPropertyHelpString(m_propertyGridItem6, _("Output texture dimesions.  By default all images will be caled to fit this size."));

	buttonDeleteLayer->Disable();
	buttonMoveUp->Disable();
}

void NormalsGenDialog::doShowPresetContext( wxCommandEvent& event )
{
	PopupMenu(presetContext);
}

void NormalsGenDialog::doPropertyChanged( wxPropertyGridEvent& event )
{
int here;
int there;
int everywhere;
}

void NormalsGenDialog::doAddLayer( wxCommandEvent& event )
{

NormalGenLayer blankLayer;

wxTextEntryDialog entry(this, _("Enter a name for the new layer."), _("Name new layer"), nextLayerName());
if (entry.ShowModal() == wxID_CANCEL) {
return;
}
blankLayer.layerName = entry.GetValue();

wxPGProperty* p = layersProperties->GetSelectedProperty();
if (!p) {
addLayer(blankLayer);
return;
}
wxPropertyGridIterator it;
for (it = layersProperties->GetIterator(wxPG_ITERATE_CATEGORIES, p); !it.AtEnd(); it++) {
if (*it == p)
continue;
if((*it)->GetName() != "Background") {
addLayer(blankLayer, *it);
return;
}
}
addLayer(blankLayer);

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

void NormalsGenDialog::OnUseBackgroundLayerCheck( wxCommandEvent& event )
{
if (event.IsChecked()) {
outputFileName->Disable();
}
else {
outputFileName->Enable();
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

void NormalsGenDialog::doLoadPreset( wxCommandEvent& event )
{
}

void NormalsGenDialog::doSavePreset( wxCommandEvent& event )
{
}


wxString NormalsGenDialog::nextLayerName()
{
	wxString nameStr = _("Layer ") + wxString::Format("%d", layerCount);
	return nameStr;
}

void NormalsGenDialog::addLayer(NormalGenLayer & newLayer, wxPGProperty * before)
{
	wxString internalName = nextLayerName();
	layerCount++;

	wxPGProperty* newCat;
	wxPGProperty* newProp;

	if (before == nullptr) {
		newCat = layersProperties->Append(new wxPropertyCategory(newLayer.layerName, internalName));
	}
	else {
		newCat = layersProperties->Insert(before, new wxPropertyCategory(newLayer.layerName, internalName));
	}

	newProp = newCat->AppendChild(new wxImageFileProperty(_("File"), internalName + "File", newLayer.sourceFileName));
	newProp->SetAttribute("Wildcard", "PNG files (*.png)|*.png");
	layersProperties->SetPropertyHelpString(newProp, _("A file containing normals data to combine.  Note this file should fit the mesh uvs"));

	newProp = newCat->AppendChild(new wxBoolProperty(_("Is Tangent Space?"), internalName +"IsTangent" , newLayer.isTangentSpace));
	layersProperties->SetPropertyHelpString(newProp, _("True if the normals data in the layer file is in tangent space, false if they are in model space (msn). "));

	newProp = newCat->AppendChild(new wxImageFileProperty(_("Mask"), internalName + "Mask", newLayer.maskFileName));
	layersProperties->SetPropertyHelpString(newProp, _("A greyscale image used to mask updates to destination image"));

	newProp = newCat->AppendChild(new wxUIntProperty(_("X Offset"), internalName + "XOffset", newLayer.xOffset));
	layersProperties->SetPropertyHelpString(newProp, _("Offset to apply to image position "));

	newProp = newCat->AppendChild(new wxUIntProperty(_("Y Offset"), internalName +"YOffset" , newLayer.yOffset));
	layersProperties->SetPropertyHelpString(newProp, _("Y Offset to apply to image position "));

	newProp = newCat->AppendChild(new wxBoolProperty(_("Scale"), internalName + "Scale", newLayer.scaleToResolution));
	layersProperties->SetPropertyHelpString(newProp, _("if true, scale image to match background resolution."));

}

void NormalsGenDialog::doPropSelected(wxPropertyGridEvent& event) 
{
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