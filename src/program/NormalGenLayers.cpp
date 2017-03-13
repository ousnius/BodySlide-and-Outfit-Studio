#include "NormalGenLayers.h"
#include <sstream>

void NormalGenLayer::LoadFromXML(tinyxml2::XMLElement * normalGenSource, std::vector<NormalGenLayer>& outLayers)
{
	std::string v;
	const char* c;
	tinyxml2::XMLElement* elem;
	tinyxml2::XMLElement* layer = normalGenSource->FirstChildElement("NormalsLayer");
	if (layer != nullptr) {
		outLayers.clear();
	}
	while (layer != nullptr) {
		NormalGenLayer ngl; 
		ngl.layerName = layer->Attribute("name");
		
		elem = layer->FirstChildElement("SourceFile");
		if (elem) ngl.sourceFileName = (c=elem->GetText())?c:"";

		elem =layer->FirstChildElement("FillColor");
		if (elem) {
			std::stringstream val((c = elem->GetText()) ? c : "");
			std::getline(val, v, ' ');
			ngl.fillColor[0] = std::stoi(v);
			std::getline(val, v, ' ');
			ngl.fillColor[1] = std::stoi(v);
			std::getline(val, v, ' ');
			ngl.fillColor[2] = std::stoi(v);			
		}

		elem = layer->FirstChildElement("Resolution");
		if (elem) {
			ngl.resolution = std::stoi((c = elem->GetText()) ? c : "");
		}

		elem = layer->FirstChildElement("TangentSpace");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.isTangentSpace = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("UseMeshNormalSource");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.useMeshNormalsSource = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("MaskFile");
		if (elem) ngl.maskFileName = (c = elem->GetText()) ? c : "";

		elem = layer->FirstChildElement("XOffset");
		if (elem) {
			ngl.xOffset = std::stoi((c = elem->GetText()) ? c : "");
		}		
		
		elem = layer->FirstChildElement("YOffset");
		if (elem) {
			ngl.yOffset = std::stoi((c = elem->GetText()) ? c : "");
		}

		elem = layer->FirstChildElement("ScaleToFit");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.scaleToResolution = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("SwapRG");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.swapRG = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertRed");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.invertRed = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertGreen");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.invertGreen = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertBlue");
		if (elem) {
			v = (c = elem->GetText()) ? c : "";
			ngl.invertBlue = (v == "true") ? true : false;
		}

		outLayers.push_back(ngl);

		layer = layer->NextSiblingElement("NormalsLayer");


	}

}

void NormalGenLayer::SaveToXML(tinyxml2::XMLElement * container, const std::vector<NormalGenLayer>& layers)
{
	for (auto l : layers) {

		tinyxml2::XMLElement* layerelem = container->GetDocument()->NewElement("NormalsLayer");
		layerelem->SetAttribute("name", l.layerName.c_str());
		
		tinyxml2::XMLElement* elem = container->GetDocument()->NewElement("SourceFile");
		elem->SetText(l.sourceFileName.c_str());

		layerelem->InsertEndChild(elem);

		if (l.layerName == "Background") {			// Background is a Special layer with limited properties.

			elem = container->GetDocument()->NewElement("FillColor");
			std::string s = std::to_string(l.fillColor[0]) + " ";
			s += std::to_string(l.fillColor[1]) + " ";
			s += std::to_string(l.fillColor[2]);
			elem->SetText(s.c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("Resolution");
			elem->SetText(std::to_string(l.resolution).c_str());

			layerelem->InsertEndChild(elem);

		}
		else {

			elem = container->GetDocument()->NewElement("TangentSpace");
			elem->SetText((l.isTangentSpace)?"true":"false");

			layerelem->InsertEndChild(elem);

			if (l.useMeshNormalsSource) {				// not outputting some default values 
				elem = container->GetDocument()->NewElement("UseMeshNormalSource");
				elem->SetText("true");
				layerelem->InsertEndChild(elem);
			}

			tinyxml2::XMLElement* elem = container->GetDocument()->NewElement("MaskFile");
			elem->SetText(l.maskFileName.c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("XOffset");
			elem->SetText(std::to_string(l.xOffset).c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("YOffset");
			elem->SetText(std::to_string(l.yOffset).c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("ScaleToFit");
			elem->SetText((l.scaleToResolution) ? "true" : "false");

			layerelem->InsertEndChild(elem);

			if (l.swapRG) {				// not outputting some default values 
				elem = container->GetDocument()->NewElement("SwapRG");
				elem->SetText("true");
				layerelem->InsertEndChild(elem);
			}			
			if (l.invertRed) {				// not outputting some default values 
				elem = container->GetDocument()->NewElement("InvertRed");
				elem->SetText("true");
				layerelem->InsertEndChild(elem);
			}
			if (l.invertGreen) {				// not outputting some default values 
				elem = container->GetDocument()->NewElement("InvertGreen");
				elem->SetText("true");
				layerelem->InsertEndChild(elem);
			}
			if (l.invertBlue) {				// not outputting some default values 
				elem = container->GetDocument()->NewElement("InvertBlue");
				elem->SetText("true");
				layerelem->InsertEndChild(elem);
			}


		}
		container->InsertEndChild(layerelem);
	}
}
