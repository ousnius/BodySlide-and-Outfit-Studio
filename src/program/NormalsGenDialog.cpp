#include "NormalsGenDialog.h"
NormalsGenDialog::NormalsGenDialog(wxWindow* parent, std::vector<NormalGenLayer>& inLayersRef)
	:
	baseNormGenDlg(parent), refNormalGenLayers(inLayersRef)
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

	PopulateLayers();

	buttonDeleteLayer->Disable();
	buttonMoveUp->Disable();
}

void NormalsGenDialog::doShowPresetContext( wxCommandEvent& event )
{
	PopupMenu(presetContext);
}

void NormalsGenDialog::doPropertyChanged( wxPropertyGridEvent& event )
{
	// Just clear out the whole layers list and rebuild it based on current property grid values.
	refNormalGenLayers.clear();
	NormalGenLayer ngl;
	wxColour clr;
	int propindex;
	for (wxPropertyGridIterator it = layersProperties->GetIterator(wxPG_ITERATE_ALL); !it.AtEnd(); it++) {
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
					ngl.sourceFileName = "";

				break;
			case 1:
				if (ngl.layerName == "Background") {
					//background layer has fillcolor here.
					clr << it.GetProperty()->GetValue();
					ngl.fillColor[0] = clr.Red(); ngl.fillColor[1] = clr.Green(); ngl.fillColor[2] = clr.Blue();
				}
				else {
					ngl.isTangentSpace == it.GetProperty()->GetValue().GetBool();
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
						ngl.maskFileName = "";
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
	PreviewWindow* preview = reinterpret_cast<PreviewWindow*> (GetParent());

	// no file name specified so it only renders a preview.
	preview->RenderNormalMap();
}

void NormalsGenDialog::doGenerateNormalMap( wxCommandEvent& event )
{
	
	PreviewWindow* preview = reinterpret_cast<PreviewWindow*> (GetParent());
	
	wxFileName outfile;
	if (checkUseBackgroundFilename->IsChecked()) {
		outfile = refNormalGenLayers[0].sourceFileName;
	}
	else {
		outfile = outputFileName->GetPath();
	}
	// forcing dds extension for final output. 
	outfile.SetExt("dds");

	if (checkBackup->IsChecked() && wxFileExists(outfile.GetFullPath())) {
		wxFileName bak(outfile);
		bak.SetExt("bak");
		wxCopyFile(outfile.GetFullPath(), bak.GetFullPath(), true);
	}

	// rendering to lossless png (SOIL2 wants to run dxt1 compression on dds which loses lots of quality).
	preview->RenderNormalMap("ngtemp.png");

	if (checkCompress->IsChecked()) {
		// compression using texconv .. pretty slow, but uses direct compute to make it a bit faster.
		wxBusyCursor compressWait;
		wxExecute("texconv.exe -f BC7_UNORM ngtemp.png -o " + outfile.GetFullPath(),wxEXEC_SYNC);
	}
	else {
		// uncompressed 8bpp using texconv just because.
		wxExecute("texconv.exe -f R8G8B8A8_UNORM ngtemp.png -o " + outfile.GetFullPath());
	}
}

void NormalsGenDialog::doLoadPreset( wxCommandEvent& event )
{	
	std::string outfilename;
	wxFileDialog fl(this, _("Choose a normals generation preset file"), "NormalGen", "", "XML Files (*.xml)|*.xml", wxFD_OPEN);
	if (fl.ShowModal() == wxID_CANCEL) {
		return;
	}
	outfilename = fl.GetPath();
	tinyxml2::XMLDocument doc;
	doc.LoadFile(outfilename.c_str());
	tinyxml2::XMLElement* root = doc.FirstChildElement("NormalsGeneration");
	if (root) {
		NormalGenLayer::LoadFromXML(root, refNormalGenLayers);
	}
	PopulateLayers();
}

void NormalsGenDialog::doSavePreset( wxCommandEvent& event )
{
	std::string outfilename;
	wxFileDialog fs(this, _("Choose a file save location for normal generation preset"), "NormalGen", "", "XML Files (*.xml)|*.xml", wxFD_SAVE);
	if (fs.ShowModal() == wxID_CANCEL) {
		return;
	}
	outfilename = fs.GetPath();
	tinyxml2::XMLDocument doc;
	wxFileName fn(outfilename);
	fn.GetName();
	tinyxml2::XMLElement* root = doc.NewElement("NormalsGeneration");
	root->SetAttribute("name", fn.GetName().ToAscii());

	doc.InsertFirstChild(root);

	NormalGenLayer::SaveToXML(root, refNormalGenLayers);

	doc.SaveFile(outfilename.c_str());

	
}


wxString NormalsGenDialog::nextLayerName()
{
	wxString nameStr = _("Layer ") + wxString::Format("%d", layerCount);
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

	newCat = layersProperties->Append(new wxPropertyCategory(newLayer.layerName, "Background"));

	newProp = layersProperties->Append(new wxImageFileProperty(_("Background File"), _("Background File"), newLayer.sourceFileName));
	layersProperties->SetPropertyHelpString(newProp, _("Filename source for this layer.  "));
	newProp = layersProperties->Append(new wxColourProperty(_("Color"), _("Color"), wxColor(newLayer.fillColor[0], newLayer.fillColor[1], newLayer.fillColor[2])));
	layersProperties->SetPropertyHelpString(newProp, _("Solid background color (If File is not set)"));
	newProp = layersProperties->Append(new wxEnumProperty(_("Resolution"), _("Resolution"), resolutions));
	newProp->SetChoiceSelection(selectedRes);
	layersProperties->SetPropertyHelpString(newProp, _("Output texture dimesions.  By default all images will be caled to fit this size."));

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

void NormalsGenDialog::PopulateLayers()
{
	layersProperties->Clear();
	layerCount = 1;
	for (auto l : refNormalGenLayers) {
		if (l.layerName == "Background") {
			addBGLayer(l);
		}
		else {
			addLayer(l);
		}
	}
}

void NormalsGenDialog::SetLayersRef(std::vector<NormalGenLayer>& inLayersRef)
{
	refNormalGenLayers = inLayersRef;
	PopulateLayers();
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