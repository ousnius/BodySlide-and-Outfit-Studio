/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "NormalGenLayers.h"
#include "../utils/ConfigurationManager.h"
#include <sstream>

extern ConfigurationManager Config;

void NormalGenLayer::LoadFromXML(tinyxml2::XMLElement* normalGenSource, std::vector<NormalGenLayer>& outLayers) {
	tinyxml2::XMLElement* layer = normalGenSource->FirstChildElement("NormalsLayer");
	if (layer)
		outLayers.clear();

	while (layer) {
		NormalGenLayer ngl;
		ngl.layerName = layer->Attribute("name");

		const char* c = nullptr;
		tinyxml2::XMLElement* elem = layer->FirstChildElement("SourceFile");
		if (elem) {
			c = elem->GetText();
			ngl.sourceFileName = c ? c : "";
			Config.ReplaceVars(ngl.sourceFileName);
		}

		elem = layer->FirstChildElement("FillColor");
		if (elem) {
			std::string v;
			c = elem->GetText();
			std::stringstream val(c ? c : "");
			std::getline(val, v, ' ');
			ngl.fillColor[0] = std::stoi(v);
			std::getline(val, v, ' ');
			ngl.fillColor[1] = std::stoi(v);
			std::getline(val, v, ' ');
			ngl.fillColor[2] = std::stoi(v);
		}

		elem = layer->FirstChildElement("Resolution");
		if (elem) {
			c = elem->GetText();
			ngl.resolution = std::stoi(c ? c : "");
		}

		elem = layer->FirstChildElement("TangentSpace");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.isTangentSpace = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("UseMeshNormalSource");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.useMeshNormalsSource = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("MaskFile");
		if (elem) {
			c = elem->GetText();
			ngl.maskFileName = c ? c : "";
		}

		Config.ReplaceVars(ngl.maskFileName);

		elem = layer->FirstChildElement("XOffset");
		if (elem) {
			c = elem->GetText();
			ngl.xOffset = std::stoi(c ? c : "");
		}

		elem = layer->FirstChildElement("YOffset");
		if (elem) {
			c = elem->GetText();
			ngl.yOffset = std::stoi(c ? c : "");
		}

		elem = layer->FirstChildElement("ScaleToFit");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.scaleToResolution = (v == "true") ? true : false;
		}

		elem = layer->FirstChildElement("SwapRG");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.swapRG = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertRed");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.invertRed = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertGreen");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.invertGreen = (v == "true") ? true : false;
		}
		elem = layer->FirstChildElement("InvertBlue");
		if (elem) {
			c = elem->GetText();
			std::string v = c ? c : "";
			ngl.invertBlue = (v == "true") ? true : false;
		}

		outLayers.push_back(ngl);

		layer = layer->NextSiblingElement("NormalsLayer");
	}
}

void NormalGenLayer::SaveToXML(tinyxml2::XMLElement* container, const std::vector<NormalGenLayer>& layers) {
	std::string gamepath = Config["GameDataPath"];
	std::string relfn;
	for (auto& l : layers) {
		tinyxml2::XMLElement* layerelem = container->GetDocument()->NewElement("NormalsLayer");
		layerelem->SetAttribute("name", l.layerName.c_str());

		tinyxml2::XMLElement* elem = container->GetDocument()->NewElement("SourceFile");

		relfn = l.sourceFileName;
		if (relfn.find(gamepath) != std::string::npos)
			relfn.replace(relfn.find(gamepath), gamepath.length(), "%GameDataPath%");

		elem->SetText(relfn.c_str());
		layerelem->InsertEndChild(elem);

		if (l.layerName == "Background") { // Background is a Special layer with limited properties.

			elem = container->GetDocument()->NewElement("FillColor");
			std::string s = std::to_string((unsigned char)l.fillColor[0]) + " ";
			s += std::to_string((unsigned char)l.fillColor[1]) + " ";
			s += std::to_string((unsigned char)l.fillColor[2]);
			elem->SetText(s.c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("Resolution");
			elem->SetText(std::to_string(l.resolution).c_str());

			layerelem->InsertEndChild(elem);
		}
		else {
			elem = container->GetDocument()->NewElement("TangentSpace");
			elem->SetText(l.isTangentSpace);

			layerelem->InsertEndChild(elem);

			if (l.useMeshNormalsSource) { // not outputting some default values
				elem = container->GetDocument()->NewElement("UseMeshNormalSource");
				elem->SetText(true);
				layerelem->InsertEndChild(elem);
			}

			elem = container->GetDocument()->NewElement("MaskFile");
			relfn = l.maskFileName;
			if (relfn.find(gamepath) != std::string::npos)
				relfn.replace(relfn.find(gamepath), gamepath.length(), "%GameDataPath%");

			elem->SetText(relfn.c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("XOffset");
			elem->SetText(std::to_string(l.xOffset).c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("YOffset");
			elem->SetText(std::to_string(l.yOffset).c_str());

			layerelem->InsertEndChild(elem);

			elem = container->GetDocument()->NewElement("ScaleToFit");
			elem->SetText(l.scaleToResolution);

			layerelem->InsertEndChild(elem);

			if (l.swapRG) { // not outputting some default values
				elem = container->GetDocument()->NewElement("SwapRG");
				elem->SetText(true);
				layerelem->InsertEndChild(elem);
			}
			if (l.invertRed) { // not outputting some default values
				elem = container->GetDocument()->NewElement("InvertRed");
				elem->SetText(true);
				layerelem->InsertEndChild(elem);
			}
			if (l.invertGreen) { // not outputting some default values
				elem = container->GetDocument()->NewElement("InvertGreen");
				elem->SetText(true);
				layerelem->InsertEndChild(elem);
			}
			if (l.invertBlue) { // not outputting some default values
				elem = container->GetDocument()->NewElement("InvertBlue");
				elem->SetText(true);
				layerelem->InsertEndChild(elem);
			}
		}
		container->InsertEndChild(layerelem);
	}
}
