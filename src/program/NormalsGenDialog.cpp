/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#include "NormalsGenDialog.h"
#include "PreviewWindow.h"

NormalsGenDialog::NormalsGenDialog(wxWindow* parent, std::vector<NormalGenLayer>& inLayersRef)
	: wxNormalsGenDlg(parent), refNormalGenLayers(inLayersRef)
{
	pgLayers->Bind(wxEVT_PG_SELECTED, &NormalsGenDialog::doPropSelected, this);

	pgLayers->SetSortFunction([](wxPropertyGrid* WXUNUSED(propGrid), wxPGProperty* p1, wxPGProperty* p2)->int {
		if (p1->GetClientData() >= p2->GetClientData())
			return 1;
		return 0;
	});

	PopulateLayers();

	btnDeleteLayer->Disable();
	btnMoveUp->Disable();
}

void NormalsGenDialog::doShowPresetContext(wxCommandEvent& WXUNUSED(event))
{
	PopupMenu(presetContext);
}

void NormalsGenDialog::doPropertyChanged(wxPropertyGridEvent& WXUNUSED(event))
{
	// Just clear out the whole layers list and rebuild it based on current property grid values.
	refNormalGenLayers.clear();

	NormalGenLayer ngl;
	wxColour clr;
	int propindex = 0;
	for (wxPropertyGridIterator it = pgLayers->GetIterator(wxPG_ITERATE_ALL); !it.AtEnd(); ++it) {
		if (it.GetProperty()->IsCategory()) {
			if (!ngl.layerName.empty()) {
				// layer name not being empty means we've got layer info already, so push the layer onto the list.
				refNormalGenLayers.push_back(ngl);
			}
			ngl.layerName = it.GetProperty()->GetLabel();
			propindex = 0;
		}
		else {
			// pull values based on position in the list.  
			switch (propindex) {
			case 0:
				ngl.sourceFileName = it.GetProperty()->GetValueAsString();
				if (ngl.sourceFileName == "0")
					ngl.sourceFileName.clear();

				break;
			case 1:
				if (ngl.layerName == "Background") {
					//background layer has fillcolor here.
					clr << it.GetProperty()->GetValue();
					ngl.fillColor[0] = clr.Red(); ngl.fillColor[1] = clr.Green(); ngl.fillColor[2] = clr.Blue();
				}
				else {
					ngl.isTangentSpace = it.GetProperty()->GetValue().GetBool();
				}
				break;
			case 2:
				if (ngl.layerName == "Background") {
					//background layer has resolution here
					ngl.resolution = std::stol(it.GetProperty()->GetValueAsString().ToStdString());
				}
				else {
					ngl.maskFileName = it.GetProperty()->GetValueAsString();
					if (ngl.maskFileName == "0")
						ngl.maskFileName.clear();
				}
				break;
			case 3:
				ngl.xOffset = it.GetProperty()->GetValue().GetInteger();
				break;
			case 4:
				ngl.yOffset = it.GetProperty()->GetValue().GetInteger();
				break;
			case 5:
				ngl.scaleToResolution = it.GetProperty()->GetValue().GetBool();
				break;
			}
			propindex++;
		}
	}
}

void NormalsGenDialog::doAddLayer(wxCommandEvent& WXUNUSED(event))
{
	NormalGenLayer blankLayer;

	wxTextEntryDialog entry(this, _("Enter a name for the new layer."), _("Name new layer"), nextLayerName());
	if (entry.ShowModal() == wxID_CANCEL)
		return;

	blankLayer.layerName = entry.GetValue();

	wxPGProperty* p = pgLayers->GetSelectedProperty();
	if (!p) {
		addLayer(blankLayer);
		return;
	}

	wxPropertyGridIterator it;
	for (it = pgLayers->GetIterator(wxPG_ITERATE_CATEGORIES, p); !it.AtEnd(); ++it) {
		if (*it == p)
			continue;
		if ((*it)->GetName() != "Background") {
			addLayer(blankLayer, *it);
			return;
		}
	}

	addLayer(blankLayer);
}

void NormalsGenDialog::doMoveUpLayer(wxCommandEvent& WXUNUSED(event))
{
	wxPGProperty* prev = nullptr;
	wxPGProperty* me = nullptr;

	wxPGProperty* s = pgLayers->GetSelectedProperty();
	if (!s || !s->IsCategory() || s->GetName() == "Background")
		return;

	int n = 1;
	for (auto it = pgLayers->GetIterator(wxPG_ITERATE_CATEGORIES); !it.AtEnd(); ++it) {
		wxPGProperty* p = it.GetProperty();
		p->SetClientData((void*)n++);
		if (p == s) {
			me = p;
		}
		else
			if (!me && p->GetName() != "Background") {
				prev = p;
			}
	}

	if (!me || !prev)
		return;

	int t = (int)me->GetClientData();
	me->SetClientData(prev->GetClientData());
	prev->SetClientData((void*)t);

	pgLayers->Sort();
	pgLayers->Refresh();
}

void NormalsGenDialog::doDeleteLayer(wxCommandEvent& WXUNUSED(event))
{
	wxPGProperty* p = pgLayers->GetSelectedProperty();
	if (p && p->IsCategory() && p->GetName() != "Background") {
		p->DeleteChildren();
		pgLayers->DeleteProperty(p);
	}
}

void NormalsGenDialog::OnUseBackgroundLayerCheck(wxCommandEvent& event)
{
	if (event.IsChecked())
		fpOutputFile->Disable();
	else
		fpOutputFile->Enable();
}

void NormalsGenDialog::doSetOutputFileName(wxFileDirPickerEvent& WXUNUSED(event))
{
	// TODO: Implement doSetOutputFileName
}

void NormalsGenDialog::doPreviewNormalMap(wxCommandEvent& WXUNUSED(event))
{
	PreviewWindow* preview = reinterpret_cast<PreviewWindow*>(GetParent());

	// no file name specified so it only renders a preview.
	preview->RenderNormalMap();
}

void NormalsGenDialog::doGenerateNormalMap(wxCommandEvent& WXUNUSED(event))
{
	PreviewWindow* preview = reinterpret_cast<PreviewWindow*>(GetParent());

	wxFileName outfile;
	if (cbSaveToBGLayerFile->IsChecked())
		outfile = refNormalGenLayers[0].sourceFileName;
	else
		outfile = fpOutputFile->GetPath();

	// forcing dds extension for final output. 
	outfile.SetExt("dds");

	if (cbBackup->IsChecked() && wxFileExists(outfile.GetFullPath())) {
		wxFileName bak(outfile);
		bak.SetExt("bak");
		wxCopyFile(outfile.GetFullPath(), bak.GetFullPath(), true);
	}

	// rendering to lossless png (SOIL2 wants to run dxt1 compression on dds which loses lots of quality).
	preview->RenderNormalMap("ngtemp.png");

	if (cbCompress->IsChecked()) {
		// compression using texconv .. pretty slow, but uses direct compute to make it a bit faster.
		wxBusyCursor compressWait;
		wxExecute("texconv.exe -f BC7_UNORM ngtemp.png -o " + outfile.GetFullPath(), wxEXEC_SYNC);
	}
	else {
		// uncompressed 8bpp using texconv just because.
		wxExecute("texconv.exe -f R8G8B8A8_UNORM ngtemp.png -o " + outfile.GetFullPath());
	}
}

void NormalsGenDialog::doLoadPreset(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fl(this, _("Choose a normals generator preset file..."), "NormalGen", wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_OPEN);
	if (fl.ShowModal() == wxID_CANCEL)
		return;

	wxFileName fn(fl.GetPath());

	tinyxml2::XMLDocument doc;
	doc.LoadFile(fn.GetFullPath());

	tinyxml2::XMLElement* root = doc.FirstChildElement("NormalsGeneration");
	if (root)
		NormalGenLayer::LoadFromXML(root, refNormalGenLayers);

	PopulateLayers();
}

void NormalsGenDialog::doSavePreset(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fs(this, _("Save normals generator preset to..."), "NormalGen", wxEmptyString, "XML Files (*.xml)|*.xml", wxFD_SAVE);
	if (fs.ShowModal() == wxID_CANCEL)
		return;

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* root = doc.NewElement("NormalsGeneration");

	wxFileName fn(fs.GetPath());
	root->SetAttribute("name", fn.GetName());

	doc.InsertFirstChild(root);

	NormalGenLayer::SaveToXML(root, refNormalGenLayers);

	doc.SaveFile(fn.GetFullPath());
}

wxString NormalsGenDialog::nextLayerName()
{
	wxString nameStr = _("Layer") + wxString::Format(" %d", layerCount);
	return nameStr;
}

void NormalsGenDialog::addBGLayer(NormalGenLayer & newLayer)
{
	wxPGProperty* newCat;
	wxPGProperty* newProp;
	wxArrayString resolutions;
	resolutions.Add("256");
	resolutions.Add("512");
	resolutions.Add("1024");
	resolutions.Add("2048");
	resolutions.Add("4096");

	int selectedRes = 0;
	for (int i = 0; i < resolutions.size(); i++) {
		if (resolutions[i] == std::to_string(newLayer.resolution)) {
			selectedRes = i;
			break;
		}
	}

	newCat = pgLayers->Append(new wxPropertyCategory(newLayer.layerName, "Background"));

	newProp = pgLayers->Append(new wxImageFileProperty(_("Background File"), _("Background File"), newLayer.sourceFileName));
	pgLayers->SetPropertyHelpString(newProp, _("File source for this layer."));
	newProp = pgLayers->Append(new wxColourProperty(_("Color"), _("Color"), wxColour(newLayer.fillColor[0], newLayer.fillColor[1], newLayer.fillColor[2])));
	pgLayers->SetPropertyHelpString(newProp, _("Solid background color (if file is not set)."));
	newProp = pgLayers->Append(new wxEnumProperty(_("Resolution"), _("Resolution"), resolutions));
	newProp->SetChoiceSelection(selectedRes);
	pgLayers->SetPropertyHelpString(newProp, _("Output texture dimensions. By default all images will be scaled to fit this size."));
}

void NormalsGenDialog::addLayer(NormalGenLayer & newLayer, wxPGProperty * before)
{
	wxString internalName = nextLayerName();
	layerCount++;

	wxPGProperty* newCat;
	wxPGProperty* newProp;

	if (before)
		newCat = pgLayers->Insert(before, new wxPropertyCategory(newLayer.layerName, internalName));
	else
		newCat = pgLayers->Append(new wxPropertyCategory(newLayer.layerName, internalName));

	newProp = newCat->AppendChild(new wxImageFileProperty(_("File"), internalName + "File", newLayer.sourceFileName));
	newProp->SetAttribute("Wildcard", "PNG files (*.png)|*.png");
	pgLayers->SetPropertyHelpString(newProp, _("File containing normals data to combine. Note this file should fit the mesh UVs."));

	newProp = newCat->AppendChild(new wxBoolProperty(_("Is Tangent Space?"), internalName + "IsTangent", newLayer.isTangentSpace));
	pgLayers->SetPropertyHelpString(newProp, _("True if the normals data in the layer file is in tangent space, false if they are in model space (msn)."));

	newProp = newCat->AppendChild(new wxImageFileProperty(_("Mask"), internalName + "Mask", newLayer.maskFileName));
	pgLayers->SetPropertyHelpString(newProp, _("A greyscale image used to mask updates to destination image."));

	newProp = newCat->AppendChild(new wxUIntProperty(_("X Offset"), internalName + "XOffset", newLayer.xOffset));
	pgLayers->SetPropertyHelpString(newProp, _("Offset to apply to image position."));

	newProp = newCat->AppendChild(new wxUIntProperty(_("Y Offset"), internalName + "YOffset", newLayer.yOffset));
	pgLayers->SetPropertyHelpString(newProp, _("Offset to apply to image position."));

	newProp = newCat->AppendChild(new wxBoolProperty(_("Scale"), internalName + "Scale", newLayer.scaleToResolution));
	pgLayers->SetPropertyHelpString(newProp, _("If true, scale image to match background resolution."));
}

void NormalsGenDialog::PopulateLayers()
{
	pgLayers->Clear();
	layerCount = 1;
	for (auto l : refNormalGenLayers) {
		if (l.layerName == "Background")
			addBGLayer(l);
		else
			addLayer(l);
	}
}

void NormalsGenDialog::SetLayersRef(std::vector<NormalGenLayer>& inLayersRef)
{
	refNormalGenLayers = inLayersRef;
	PopulateLayers();
}

void NormalsGenDialog::doPropSelected(wxPropertyGridEvent& WXUNUSED(event))
{
	wxPGProperty* p = pgLayers->GetSelectedProperty();
	if (p && p->IsCategory() && p->GetName() != "Background") {
		btnDeleteLayer->Enable();
		btnMoveUp->Enable();
	}
	else {
		btnDeleteLayer->Disable();
		btnMoveUp->Disable();
	}
}
