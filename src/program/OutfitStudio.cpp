/*
BodySlide and Outfit Studio
Copyright(C) 2018  Caliente & ousnius

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OutfitStudio.h"
#include "ShapeProperties.h"
#include "PresetSaveDialog.h"
#include "../components/SliderPresets.h"
#include "../components/SliderGroup.h"
#include "../files/TriFile.h"
#include "../utils/PlatformUtil.h"

#include <wx/debugrpt.h>
#include <sstream>

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(OutfitStudioFrame, wxFrame)
	EVT_CLOSE(OutfitStudioFrame::OnClose)
	EVT_MENU(XRCID("fileExit"), OutfitStudioFrame::OnExit)
	EVT_MENU(XRCID("fileSettings"), OutfitStudioFrame::OnSettings)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudioFrame::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudioFrame::OnLoadProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudioFrame::OnLoadReference)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudioFrame::OnLoadOutfit)
	EVT_MENU(XRCID("fileSave"), OutfitStudioFrame::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudioFrame::OnSaveSliderSetAs)
	EVT_MENU(XRCID("fileUnload"), OutfitStudioFrame::OnUnloadProject)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("brushPane"), OutfitStudioFrame::OnBrushPane)
	EVT_COMMAND_SCROLL(XRCID("brushSize"), OutfitStudioFrame::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushStr"), OutfitStudioFrame::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushFocus"), OutfitStudioFrame::OnBrushSettingsSlider)
	EVT_COMMAND_SCROLL(XRCID("brushSpace"), OutfitStudioFrame::OnBrushSettingsSlider)
	
	EVT_COMMAND_SCROLL(wxID_ANY, OutfitStudioFrame::OnSlider)
	EVT_BUTTON(wxID_ANY, OutfitStudioFrame::OnClickSliderButton)
	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudioFrame::OnSelectSliders)
	EVT_CHECKBOX(XRCID("cbFixedWeight"), OutfitStudioFrame::OnFixedWeight)
	EVT_CHECKBOX(wxID_ANY, OutfitStudioFrame::OnCheckBox)

	EVT_MENU(XRCID("saveBaseShape"), OutfitStudioFrame::OnSetBaseShape)
	EVT_MENU(XRCID("makeConvRef"), OutfitStudioFrame::OnMakeConvRef)

	EVT_MENU(XRCID("importNIF"), OutfitStudioFrame::OnImportNIF)
	EVT_MENU(XRCID("exportNIF"), OutfitStudioFrame::OnExportNIF)
	EVT_MENU(XRCID("exportNIFWithRef"), OutfitStudioFrame::OnExportNIFWithRef)
	EVT_MENU(XRCID("exportShapeNIF"), OutfitStudioFrame::OnExportShapeNIF)

	EVT_MENU(XRCID("importOBJ"), OutfitStudioFrame::OnImportOBJ)
	EVT_MENU(XRCID("exportOBJ"), OutfitStudioFrame::OnExportOBJ)
	EVT_MENU(XRCID("exportShapeOBJ"), OutfitStudioFrame::OnExportShapeOBJ)

	EVT_MENU(XRCID("importFBX"), OutfitStudioFrame::OnImportFBX)
	EVT_MENU(XRCID("exportFBX"), OutfitStudioFrame::OnExportFBX)
	EVT_MENU(XRCID("exportShapeFBX"), OutfitStudioFrame::OnExportShapeFBX)

	EVT_MENU(XRCID("importPhysicsData"), OutfitStudioFrame::OnImportPhysicsData)
	EVT_MENU(XRCID("exportPhysicsData"), OutfitStudioFrame::OnExportPhysicsData)
	
	EVT_MENU(XRCID("sliderLoadPreset"), OutfitStudioFrame::OnLoadPreset)
	EVT_MENU(XRCID("sliderSavePreset"), OutfitStudioFrame::OnSavePreset)
	EVT_MENU(XRCID("sliderConform"), OutfitStudioFrame::OnSliderConform)
	EVT_MENU(XRCID("sliderConformAll"), OutfitStudioFrame::OnSliderConformAll)
	EVT_MENU(XRCID("sliderImportBSD"), OutfitStudioFrame::OnSliderImportBSD)
	EVT_MENU(XRCID("sliderImportOBJ"), OutfitStudioFrame::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderImportFBX"), OutfitStudioFrame::OnSliderImportFBX)
	EVT_MENU(XRCID("sliderImportOSD"), OutfitStudioFrame::OnSliderImportOSD)
	EVT_MENU(XRCID("sliderImportTRI"), OutfitStudioFrame::OnSliderImportTRI)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudioFrame::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderExportOBJ"), OutfitStudioFrame::OnSliderExportOBJ)
	EVT_MENU(XRCID("sliderExportOSD"), OutfitStudioFrame::OnSliderExportOSD)
	EVT_MENU(XRCID("sliderExportTRI"), OutfitStudioFrame::OnSliderExportTRI)
	EVT_MENU(XRCID("sliderNew"), OutfitStudioFrame::OnNewSlider)
	EVT_MENU(XRCID("sliderNewZap"), OutfitStudioFrame::OnNewZapSlider)
	EVT_MENU(XRCID("sliderNewCombined"), OutfitStudioFrame::OnNewCombinedSlider)
	EVT_MENU(XRCID("sliderNegate"), OutfitStudioFrame::OnSliderNegate)
	EVT_MENU(XRCID("sliderMask"), OutfitStudioFrame::OnMaskAffected)
	EVT_MENU(XRCID("sliderClear"), OutfitStudioFrame::OnClearSlider)
	EVT_MENU(XRCID("sliderDelete"), OutfitStudioFrame::OnDeleteSlider)
	EVT_MENU(XRCID("sliderProperties"), OutfitStudioFrame::OnSliderProperties)
	
	EVT_MENU(XRCID("btnXMirror"), OutfitStudioFrame::OnXMirror)
	EVT_MENU(XRCID("btnConnected"), OutfitStudioFrame::OnConnectedOnly)
	EVT_MENU(XRCID("btnBrushCollision"), OutfitStudioFrame::OnGlobalBrushCollision)
	
	EVT_MENU(XRCID("btnSelect"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnTransform"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnPivot"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnVertexEdit"), OutfitStudioFrame::OnSelectTool)

	EVT_MENU(XRCID("btnMaskBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnInflateBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnDeflateBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnMoveBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnSmoothBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudioFrame::OnSelectTool)

	EVT_MENU(XRCID("btnViewFront"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewBack"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewLeft"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewRight"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewPerspective"), OutfitStudioFrame::OnTogglePerspective)
	
	EVT_MENU(XRCID("btnIncreaseSize"), OutfitStudioFrame::OnIncBrush)
	EVT_MENU(XRCID("btnDecreaseSize"), OutfitStudioFrame::OnDecBrush)
	EVT_MENU(XRCID("btnIncreaseStr"), OutfitStudioFrame::OnIncStr)
	EVT_MENU(XRCID("btnDecreaseStr"), OutfitStudioFrame::OnDecStr)
	EVT_MENU(XRCID("btnMaskLess"), OutfitStudioFrame::OnMaskLess)
	EVT_MENU(XRCID("btnMaskMore"), OutfitStudioFrame::OnMaskMore)
	EVT_MENU(XRCID("btnClearMask"), OutfitStudioFrame::OnClearMask)
	EVT_MENU(XRCID("btnInvertMask"), OutfitStudioFrame::OnInvertMask)
	EVT_MENU(XRCID("btnShowMask"), OutfitStudioFrame::OnShowMask)

	EVT_MENU(XRCID("btnRecalcNormals"), OutfitStudioFrame::OnRecalcNormals)
	EVT_MENU(XRCID("btnSmoothSeams"), OutfitStudioFrame::OnSmoothNormalSeams)
	EVT_MENU(XRCID("btnLockNormals"), OutfitStudioFrame::OnLockNormals)

	EVT_MENU(XRCID("btnGhostMode"), OutfitStudioFrame::OnGhostMesh)
	EVT_MENU(XRCID("btnShowWireframe"), OutfitStudioFrame::OnShowWireframe)
	EVT_MENU(XRCID("btnEnableLighting"), OutfitStudioFrame::OnEnableLighting)
	EVT_MENU(XRCID("btnEnableTextures"), OutfitStudioFrame::OnEnableTextures)

	EVT_MENU(XRCID("uvInvertX"), OutfitStudioFrame::OnInvertUV)
	EVT_MENU(XRCID("uvInvertY"), OutfitStudioFrame::OnInvertUV)
	EVT_MENU(XRCID("mirrorX"), OutfitStudioFrame::OnMirror)
	EVT_MENU(XRCID("mirrorY"), OutfitStudioFrame::OnMirror)
	EVT_MENU(XRCID("mirrorZ"), OutfitStudioFrame::OnMirror)

	EVT_MENU(XRCID("moveShape"), OutfitStudioFrame::OnMoveShape)
	EVT_MENU(XRCID("scaleShape"), OutfitStudioFrame::OnScaleShape)
	EVT_MENU(XRCID("rotateShape"), OutfitStudioFrame::OnRotateShape)
	EVT_MENU(XRCID("renameShape"), OutfitStudioFrame::OnRenameShape)
	EVT_MENU(XRCID("setReference"), OutfitStudioFrame::OnSetReference)
	EVT_MENU(XRCID("deleteVerts"), OutfitStudioFrame::OnDeleteVerts)
	EVT_MENU(XRCID("separateVerts"), OutfitStudioFrame::OnSeparateVerts)
	EVT_MENU(XRCID("copyShape"), OutfitStudioFrame::OnDupeShape)
	EVT_MENU(XRCID("deleteShape"), OutfitStudioFrame::OnDeleteShape)
	EVT_MENU(XRCID("addBone"), OutfitStudioFrame::OnAddBone)
	EVT_MENU(XRCID("addCustomBone"), OutfitStudioFrame::OnAddCustomBone)
	EVT_MENU(XRCID("deleteBone"), OutfitStudioFrame::OnDeleteBone)
	EVT_MENU(XRCID("deleteBoneSelected"), OutfitStudioFrame::OnDeleteBoneFromSelected)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudioFrame::OnCopyBoneWeight)	
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudioFrame::OnCopySelectedWeight)
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudioFrame::OnTransferSelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudioFrame::OnMaskWeighted)
	EVT_MENU(XRCID("shapeProperties"), OutfitStudioFrame::OnShapeProperties)

	EVT_MENU(XRCID("editUndo"), OutfitStudioFrame::OnUndo)
	EVT_MENU(XRCID("editRedo"), OutfitStudioFrame::OnRedo)

	EVT_TREE_STATE_IMAGE_CLICK(wxID_ANY, OutfitStudioFrame::OnShapeVisToggle)
	EVT_TREE_SEL_CHANGING(XRCID("outfitShapes"), OutfitStudioFrame::OnCheckTreeSel)
	EVT_TREE_SEL_CHANGED(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeSelect)
	EVT_TREE_ITEM_ACTIVATED(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeContext)
	EVT_TREE_BEGIN_DRAG(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeDrag)
	EVT_TREE_END_DRAG(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeDrop)

	EVT_TREE_SEL_CHANGED(XRCID("outfitBones"), OutfitStudioFrame::OnBoneSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneTreeContext)

	EVT_TREE_SEL_CHANGED(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentTreeContext)
	EVT_MENU(XRCID("addSegment"), OutfitStudioFrame::OnAddSegment)
	EVT_MENU(XRCID("addSubSegment"), OutfitStudioFrame::OnAddSubSegment)
	EVT_MENU(XRCID("deleteSegment"), OutfitStudioFrame::OnDeleteSegment)
	EVT_MENU(XRCID("deleteSubSegment"), OutfitStudioFrame::OnDeleteSubSegment)
	EVT_CHOICE(XRCID("segmentType"), OutfitStudioFrame::OnSegmentTypeChanged)
	EVT_BUTTON(XRCID("segmentApply"), OutfitStudioFrame::OnSegmentApply)
	EVT_BUTTON(XRCID("segmentReset"), OutfitStudioFrame::OnSegmentReset)

	EVT_TREE_SEL_CHANGED(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionTreeContext)
	EVT_MENU(XRCID("addPartition"), OutfitStudioFrame::OnAddPartition)
	EVT_MENU(XRCID("deletePartition"), OutfitStudioFrame::OnDeletePartition)
	EVT_CHOICE(XRCID("partitionType"), OutfitStudioFrame::OnPartitionTypeChanged)
	EVT_BUTTON(XRCID("partitionApply"), OutfitStudioFrame::OnPartitionApply)
	EVT_BUTTON(XRCID("partitionReset"), OutfitStudioFrame::OnPartitionReset)
	
	EVT_BUTTON(XRCID("meshTabButton"), OutfitStudioFrame::OnTabButtonClick)
	EVT_BUTTON(XRCID("boneTabButton"), OutfitStudioFrame::OnTabButtonClick)
	EVT_BUTTON(XRCID("segmentTabButton"), OutfitStudioFrame::OnTabButtonClick)
	EVT_BUTTON(XRCID("partitionTabButton"), OutfitStudioFrame::OnTabButtonClick)
	EVT_BUTTON(XRCID("lightsTabButton"), OutfitStudioFrame::OnTabButtonClick)

	EVT_SLIDER(XRCID("lightAmbientSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightFrontalSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional0Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional1Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional2Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_BUTTON(XRCID("lightReset"), OutfitStudioFrame::OnResetLights)
	
	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("splitter"), OutfitStudioFrame::OnSashPosChanged)
	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("splitterRight"), OutfitStudioFrame::OnSashPosChanged)
	EVT_MOVE_END(OutfitStudioFrame::OnMoveWindow)
	EVT_SIZE(OutfitStudioFrame::OnSetSize)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(OutfitStudio);


ConfigurationManager Config;
ConfigurationManager OutfitStudioConfig;

const wxString TargetGames[] = { "Fallout3", "FalloutNewVegas", "Skyrim", "Fallout4", "SkyrimSpecialEdition", "Fallout4VR", "SkyrimVR" };
const wxLanguage SupportedLangs[] = {
	wxLANGUAGE_ENGLISH, wxLANGUAGE_AFRIKAANS, wxLANGUAGE_ARABIC, wxLANGUAGE_CATALAN, wxLANGUAGE_CZECH, wxLANGUAGE_DANISH, wxLANGUAGE_GERMAN,
	wxLANGUAGE_GREEK, wxLANGUAGE_SPANISH, wxLANGUAGE_BASQUE, wxLANGUAGE_FINNISH, wxLANGUAGE_FRENCH, wxLANGUAGE_HINDI,
	wxLANGUAGE_HUNGARIAN, wxLANGUAGE_INDONESIAN, wxLANGUAGE_ITALIAN, wxLANGUAGE_JAPANESE, wxLANGUAGE_KOREAN, wxLANGUAGE_LITHUANIAN,
	wxLANGUAGE_LATVIAN, wxLANGUAGE_MALAY, wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI, wxLANGUAGE_DUTCH, wxLANGUAGE_POLISH,
	wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN, wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK, wxLANGUAGE_SLOVENIAN, wxLANGUAGE_ALBANIAN,
	wxLANGUAGE_SWEDISH, wxLANGUAGE_TAMIL, wxLANGUAGE_TURKISH, wxLANGUAGE_UKRAINIAN, wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE
};

OutfitStudio::~OutfitStudio() {
	delete locale;
	locale = nullptr;

	FSManager::del();
}

bool OutfitStudio::OnInit() {
	if (!wxApp::OnInit())
		return false;

	if (!cmdFiles.IsEmpty()) {
		wxFileName exeFile(wxStandardPaths::Get().GetExecutablePath());
		wxString exePath(exeFile.GetPath());
		wxSetWorkingDirectory(exePath);
	}

	Config.LoadConfig();
	OutfitStudioConfig.LoadConfig("OutfitStudio.xml", "OutfitStudioConfig");

	logger.Initialize(Config.GetIntValue("LogLevel", -1), "Log_OS.txt");
	wxLogMessage("Initializing Outfit Studio...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->InitAllHandlers();
	wxInitAllImageHandlers();

	wxLogMessage("Working directory: %s", wxGetCwd());
	if (!SetDefaultConfig())
		return false;

	InitLanguage();

	wxString gameName = "Target game: ";
	switch (targetGame) {
	case FO3: gameName.Append("Fallout 3"); break;
	case FONV: gameName.Append("Fallout New Vegas"); break;
	case SKYRIM: gameName.Append("Skyrim"); break;
	case FO4: gameName.Append("Fallout 4"); break;
	case SKYRIMSE: gameName.Append("Skyrim Special Edition"); break;
	case FO4VR: gameName.Append("Fallout 4 VR"); break;
	case SKYRIMVR: gameName.Append("Skyrim VR"); break;
	default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	int x = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.x");
	int y = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.y");
	int w = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.width");
	int h = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.height");
	std::string maximized = OutfitStudioConfig["OutfitStudioFrame.maximized"];

	frame = new OutfitStudioFrame(wxPoint(x, y), wxSize(w, h));
	if (maximized == "true")
		frame->Maximize();

	frame->Show();
	SetTopWindow(frame);

	InitArchives();

	if (!Config["GameDataPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["GameDataPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["GameDataPath"]);
		if (!dirWritable || !dirReadable)
			wxMessageBox(_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."), _("Warning"), wxICON_WARNING);
	}

	if (!cmdFiles.IsEmpty()) {
		wxFileName loadFile(cmdFiles.Item(0));
		if (loadFile.FileExists()) {
			std::string fileName = loadFile.GetFullPath().ToUTF8();
			wxString fileExt = loadFile.GetExt().MakeLower();
			if (fileExt == "osp") {
				std::string projectName = cmdProject.ToUTF8();
				frame->LoadProject(fileName, projectName);
			}
			else if (fileExt == "nif")
				frame->LoadNIF(fileName);
		}
	}

	wxLogMessage("Outfit Studio initialized.");
	return true;
}

void OutfitStudio::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars("-");
}

bool OutfitStudio::OnCmdLineParsed(wxCmdLineParser& parser) {
	parser.Found("proj", &cmdProject);

	for (int i = 0; i < parser.GetParamCount(); i++)
		cmdFiles.Add(parser.GetParam(i));

	return true;
}

bool OutfitStudio::OnExceptionInMainLoop() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unexpected exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unexpected exception has occurred: %s, the program will terminate."), error), _("Unexpected exception"), wxICON_ERROR);
	return false;
}

void OutfitStudio::OnUnhandledException() {
	wxString error;
	try {
		throw;
	}
	catch (const std::exception& e) {
		error = e.what();
	}
	catch (...) {
		error = "unknown error";
	}

	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Unhandled exception has occurred: %s, the program will terminate.", error);
	wxMessageBox(wxString::Format(_("Unhandled exception has occurred: %s, the program will terminate."), error), _("Unhandled exception"), wxICON_ERROR);
}

void OutfitStudio::OnFatalException() {
	wxLog::FlushActive();
	logger.SetFormatter(false);

	wxLogError("Fatal exception has occurred, the program will terminate.");
	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);

	wxDebugReport report;
	report.AddExceptionContext();
	report.Process();
}

bool OutfitStudio::SetDefaultConfig() {
	int currentTarget = -1;
	Config.SetDefaultValue("TargetGame", currentTarget);
	currentTarget = Config.GetIntValue("TargetGame");

	Config.SetDefaultValue("ShapeDataPath", wxGetCwd().ToStdString() + "\\ShapeData");
	Config.SetDefaultValue("WarnMissingGamePath", "true");
	Config.SetDefaultValue("BSATextureScan", "true");
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultValue("UseSystemLanguage", "false");
	Config.SetDefaultValue("Input/LeftMousePan", "false");
	Config.SetDefaultValue("Lights/Ambient", 20);
	Config.SetDefaultValue("Lights/Frontal", 20);
	Config.SetDefaultValue("Lights/Directional0", 60);
	Config.SetDefaultValue("Lights/Directional0.x", -90);
	Config.SetDefaultValue("Lights/Directional0.y", 10);
	Config.SetDefaultValue("Lights/Directional0.z", 100);
	Config.SetDefaultValue("Lights/Directional1", 60);
	Config.SetDefaultValue("Lights/Directional1.x", 70);
	Config.SetDefaultValue("Lights/Directional1.y", 10);
	Config.SetDefaultValue("Lights/Directional1.z", 100);
	Config.SetDefaultValue("Lights/Directional2", 85);
	Config.SetDefaultValue("Lights/Directional2.x", 30);
	Config.SetDefaultValue("Lights/Directional2.y", 20);
	Config.SetDefaultValue("Lights/Directional2.z", -100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.width", 990);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.height", 757);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.x", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.y", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashpos", 850);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashrightpos", 200);

	Config.SetDefaultValue("GameRegKey/Fallout3", "Software\\Bethesda Softworks\\Fallout3");
	Config.SetDefaultValue("GameRegVal/Fallout3", "Installed Path");
	Config.SetDefaultValue("GameRegKey/FalloutNewVegas", "Software\\Bethesda Softworks\\FalloutNV");
	Config.SetDefaultValue("GameRegVal/FalloutNewVegas", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Skyrim", "Software\\Bethesda Softworks\\Skyrim");
	Config.SetDefaultValue("GameRegVal/Skyrim", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4", "Software\\Bethesda Softworks\\Fallout4");
	Config.SetDefaultValue("GameRegVal/Fallout4", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimSpecialEdition", "Software\\Bethesda Softworks\\Skyrim Special Edition");
	Config.SetDefaultValue("GameRegVal/SkyrimSpecialEdition", "Installed Path");
	Config.SetDefaultValue("GameRegKey/Fallout4VR", "Software\\Bethesda Softworks\\Fallout 4 VR");
	Config.SetDefaultValue("GameRegVal/Fallout4VR", "Installed Path");
	Config.SetDefaultValue("GameRegKey/SkyrimVR", "Software\\Bethesda Softworks\\Skyrim VR");
	Config.SetDefaultValue("GameRegVal/SkyrimVR", "Installed Path");

	// Target game not set, show setup dialog
	if (currentTarget == -1)
		if (!ShowSetup())
			return false;

	targetGame = (TargetGame)Config.GetIntValue("TargetGame");

	wxString gameKey = Config["GameRegKey/" + TargetGames[targetGame]];
	wxString gameValueKey = Config["GameRegVal/" + TargetGames[targetGame]];

	if (Config["GameDataPath"].empty()) {
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data\\");
				Config.SetDefaultValue("GameDataPath", installPath.ToStdString());
				wxLogMessage("Registry game data path: %s", installPath);
			}
			else if (Config["WarnMissingGamePath"] == "true") {
				wxLogWarning("Failed to find game install path registry value or GameDataPath in the config.");
				wxMessageBox(_("Failed to find game install path registry value or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
			}
		}
		else if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
			wxMessageBox(_("Failed to find game install path registry key or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
		}
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);

	return true;
}

bool OutfitStudio::ShowSetup() {
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load("res\\xrc\\Setup.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load Setup.xrc file!", "Error", wxICON_ERROR);
		return false;
	}

	wxDialog* setup = xrc->LoadDialog(nullptr, "dlgSetup");
	if (setup) {
		setup->SetSize(wxSize(700, -1));
		setup->CenterOnScreen();

		wxButton* btFallout3 = XRCCTRL(*setup, "btFallout3", wxButton);
		btFallout3->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(0); });

		wxButton* btFalloutNV = XRCCTRL(*setup, "btFalloutNV", wxButton);
		btFalloutNV->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(1); });

		wxButton* btSkyrim = XRCCTRL(*setup, "btSkyrim", wxButton);
		btSkyrim->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(2); });

		wxButton* btFallout4 = XRCCTRL(*setup, "btFallout4", wxButton);
		btFallout4->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(3); });

		wxButton* btSkyrimSE = XRCCTRL(*setup, "btSkyrimSE", wxButton);
		btSkyrimSE->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(4); });

		wxButton* btFallout4VR = XRCCTRL(*setup, "btFallout4VR", wxButton);
		btFallout4VR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(5); });

		wxButton* btSkyrimVR = XRCCTRL(*setup, "btSkyrimVR", wxButton);
		btSkyrimVR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal(6); });

		wxDirPickerCtrl* dirFallout3 = XRCCTRL(*setup, "dirFallout3", wxDirPickerCtrl);
		dirFallout3->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout3, &btFallout3](wxFileDirPickerEvent&) { btFallout3->Enable(dirFallout3->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFalloutNV = XRCCTRL(*setup, "dirFalloutNV", wxDirPickerCtrl);
		dirFalloutNV->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFalloutNV, &btFalloutNV](wxFileDirPickerEvent&) { btFalloutNV->Enable(dirFalloutNV->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrim = XRCCTRL(*setup, "dirSkyrim", wxDirPickerCtrl);
		dirSkyrim->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrim, &btSkyrim](wxFileDirPickerEvent&) { btSkyrim->Enable(dirSkyrim->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFallout4 = XRCCTRL(*setup, "dirFallout4", wxDirPickerCtrl);
		dirFallout4->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout4, &btFallout4](wxFileDirPickerEvent&) { btFallout4->Enable(dirFallout4->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrimSE = XRCCTRL(*setup, "dirSkyrimSE", wxDirPickerCtrl);
		dirSkyrimSE->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrimSE, &btSkyrimSE](wxFileDirPickerEvent&) { btSkyrimSE->Enable(dirSkyrimSE->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirFallout4VR = XRCCTRL(*setup, "dirFallout4VR", wxDirPickerCtrl);
		dirFallout4VR->Bind(wxEVT_DIRPICKER_CHANGED, [&dirFallout4VR, &btFallout4VR](wxFileDirPickerEvent&) { btFallout4VR->Enable(dirFallout4VR->GetDirName().DirExists()); });

		wxDirPickerCtrl* dirSkyrimVR = XRCCTRL(*setup, "dirSkyrimVR", wxDirPickerCtrl);
		dirSkyrimVR->Bind(wxEVT_DIRPICKER_CHANGED, [&dirSkyrimVR, &btSkyrimVR](wxFileDirPickerEvent&) { btSkyrimVR->Enable(dirSkyrimVR->GetDirName().DirExists()); });

		wxFileName dir = GetGameDataPath(FO3);
		if (dir.DirExists()) {
			dirFallout3->SetDirName(dir);
			btFallout3->Enable();
		}

		dir = GetGameDataPath(FONV);
		if (dir.DirExists()) {
			dirFalloutNV->SetDirName(dir);
			btFalloutNV->Enable();
		}

		dir = GetGameDataPath(SKYRIM);
		if (dir.DirExists()) {
			dirSkyrim->SetDirName(dir);
			btSkyrim->Enable();
		}

		dir = GetGameDataPath(FO4);
		if (dir.DirExists()) {
			dirFallout4->SetDirName(dir);
			btFallout4->Enable();
		}

		dir = GetGameDataPath(SKYRIMSE);
		if (dir.DirExists()) {
			dirSkyrimSE->SetDirName(dir);
			btSkyrimSE->Enable();
		}

		dir = GetGameDataPath(FO4VR);
		if (dir.DirExists()) {
			dirFallout4VR->SetDirName(dir);
			btFallout4VR->Enable();
		}

		dir = GetGameDataPath(SKYRIMVR);
		if (dir.DirExists()) {
			dirSkyrimVR->SetDirName(dir);
			btSkyrimVR->Enable();
		}

		if (setup->ShowModal() != wxID_CANCEL) {
			int targ = setup->GetReturnCode();
			Config.SetValue("TargetGame", targ);

			wxFileName dataDir;
			switch (targ) {
			case FO3:
				dataDir = dirFallout3->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_fo3nv.nif");
				Config.SetValue("Anim/SkeletonRootName", "Bip01");
				break;
			case FONV:
				dataDir = dirFalloutNV->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_fo3nv.nif");
				Config.SetValue("Anim/SkeletonRootName", "Bip01");
				break;
			case SKYRIM:
				dataDir = dirSkyrim->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_sk.nif");
				Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
				break;
			case FO4:
				dataDir = dirFallout4->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_fo4.nif");
				Config.SetValue("Anim/SkeletonRootName", "Root");
				break;
			case SKYRIMSE:
				dataDir = dirSkyrimSE->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_sse.nif");
				Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
				break;
			case FO4VR:
				dataDir = dirFallout4VR->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_fo4.nif");
				Config.SetValue("Anim/SkeletonRootName", "Root");
				break;
			case SKYRIMVR:
				dataDir = dirSkyrimVR->GetDirName();
				Config.SetValue("Anim/DefaultSkeletonReference", "res\\skeleton_female_sse.nif");
				Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
				break;
			}

			Config.SetValue("GameDataPath", dataDir.GetFullPath().ToStdString());
			Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToStdString());

			Config.SaveConfig();
			delete setup;
		}
		else {
			delete setup;
			return false;
		}
	}

	return true;
}

wxString OutfitStudio::GetGameDataPath(TargetGame targ) {
	wxString dataPath;
	wxString gamestr = TargetGames[targ];
	wxString gkey = "GameRegKey/" + gamestr;
	wxString gval = "GameRegVal/" + gamestr;
	wxString cust = "GameDataPaths/" + gamestr;

	if (!Config[cust].IsEmpty()) {
		dataPath = Config[cust];
	}
	else {
		wxRegKey key(wxRegKey::HKLM, Config[gkey], wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			if (key.HasValues() && key.QueryValue(Config[gval], dataPath)) {
				dataPath.Append("Data\\");
			}
		}
	}
	return dataPath;
}

void OutfitStudio::InitLanguage() {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");

	// Load language if possible, fall back to English otherwise
	if (wxLocale::IsAvailable(lang)) {
		locale = new wxLocale(lang);
		locale->AddCatalogLookupPathPrefix("lang");
		locale->AddCatalog("BodySlide");

		if (!locale->IsOk()) {
			wxLogError("System language '%d' is wrong.", lang);
			wxMessageBox(wxString::Format(_("System language '%d' is wrong."), lang));

			delete locale;
			locale = new wxLocale(wxLANGUAGE_ENGLISH);
			lang = wxLANGUAGE_ENGLISH;
		}
	}
	else {
		wxLogError("The system language '%d' is not supported by your system. Try installing support for this language.", lang);
		wxMessageBox(wxString::Format(_("The system language '%d' is not supported by your system. Try installing support for this language."), lang));

		locale = new wxLocale(wxLANGUAGE_ENGLISH);
		lang = wxLANGUAGE_ENGLISH;
	}

	wxLogMessage("Using language '%s'.", wxLocale::GetLanguageName(lang));
}

void OutfitStudio::InitArchives() {
	// Auto-detect archives
	FSManager::del();

	std::vector<std::string> fileList;
	GetArchiveFiles(fileList);

	FSManager::addArchives(fileList);
}

void OutfitStudio::GetArchiveFiles(std::vector<std::string>& outList) {
	TargetGame targ = (TargetGame)Config.GetIntValue("TargetGame");
	std::string cp = "GameDataFiles/" + TargetGames[targ];
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false);
		val = val.Trim().MakeLower();
		fsearch[val] = true;
	}

	wxString dataDir = Config["GameDataPath"];
	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& f : files) {
		f = f.AfterLast('\\').MakeLower();
		if (fsearch.find(f) == fsearch.end())
			outList.push_back((dataDir + f).ToUTF8().data());
	}
}


OutfitStudioFrame::OutfitStudioFrame(const wxPoint& pos, const wxSize& size) {
	wxLogMessage("Loading Outfit Studio frame at X:%d Y:%d with W:%d H:%d...", pos.x, pos.y, size.GetWidth(), size.GetHeight());

	wxXmlResource *xrc = wxXmlResource::Get();
	if (!xrc->Load("res\\xrc\\OutfitStudio.xrc")) {
		wxMessageBox(_("Failed to load OutfitStudio.xrc file!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	if (!xrc->LoadFrame(this, nullptr, "outfitStudio")) {
		wxMessageBox(_("Failed to load Outfit Studio frame!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	SetIcon(wxIcon("res\\images\\OutfitStudio.png", wxBITMAP_TYPE_PNG));

	xrc->Load("res\\xrc\\Project.xrc");
	xrc->Load("res\\xrc\\Actions.xrc");
	xrc->Load("res\\xrc\\Slider.xrc");
	xrc->Load("res\\xrc\\Skeleton.xrc");
	xrc->Load("res\\xrc\\Settings.xrc");

	int statusWidths[] = { -1, 275, 100 };
	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	statusBar->SetFieldsCount(3);
	statusBar->SetStatusWidths(3, statusWidths);
	statusBar->SetStatusText(_("Ready!"));

	this->DragAcceptFiles(true);

	xrc->LoadMenuBar(this, "menuBar");
	xrc->LoadToolBar(this, "toolBar");

	wxSlider* fovSlider = (wxSlider*)GetToolBar()->FindWindowByName("fovSlider");
	fovSlider->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnFieldOfViewSlider, this);

	visStateImages = new wxImageList(16, 16, false, 2);
	wxBitmap visImg("res\\images\\icoVisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap invImg("res\\images\\icoInvisible.png", wxBITMAP_TYPE_PNG);
	wxBitmap wfImg("res\\images\\icoWireframe.png", wxBITMAP_TYPE_PNG);

	if (visImg.IsOk())
		visStateImages->Add(visImg);
	if (invImg.IsOk())
		visStateImages->Add(invImg);
	if (wfImg.IsOk())
		visStateImages->Add(wfImg);

	wxStateButton* meshTab = (wxStateButton*)FindWindowByName("meshTabButton");
	meshTab->SetCheck();

	if (wxGetApp().targetGame != FO4 && wxGetApp().targetGame != FO4VR) {
		wxStateButton* segmentTab = (wxStateButton*)FindWindowByName("segmentTabButton");
		segmentTab->Show(false);
	}
	else {
		wxStateButton* partitionTab = (wxStateButton*)FindWindowByName("partitionTabButton");
		partitionTab->Show(false);
	}

	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	outfitShapes->AssignStateImageList(visStateImages);
	shapesRoot = outfitShapes->AddRoot("Shapes");

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	bonesRoot = outfitBones->AddRoot("Bones");

	segmentTree = (wxTreeCtrl*)FindWindowByName("segmentTree");
	segmentRoot = segmentTree->AddRoot("Segments");

	partitionTree = (wxTreeCtrl*)FindWindowByName("partitionTree");
	partitionRoot = partitionTree->AddRoot("Partitions");

	wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
	segmentType->Show(false);

	wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
	partitionType->Show(false);

	int ambient = Config.GetIntValue("Lights/Ambient");
	int frontal = Config.GetIntValue("Lights/Frontal");
	int directional0 = Config.GetIntValue("Lights/Directional0");
	int directional1 = Config.GetIntValue("Lights/Directional1");
	int directional2 = Config.GetIntValue("Lights/Directional2");

	lightSettings = (wxPanel*)FindWindowByName("lightSettings");
	wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	lightAmbientSlider->SetValue(ambient);

	wxSlider* lightFrontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	lightFrontalSlider->SetValue(frontal);

	wxSlider* lightDirectional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	lightDirectional0Slider->SetValue(directional0);

	wxSlider* lightDirectional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	lightDirectional1Slider->SetValue(directional1);

	wxSlider* lightDirectional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");
	lightDirectional2Slider->SetValue(directional2);

	boneScale = (wxSlider*)FindWindowByName("boneScale");

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
	glView->SetNotifyWindow(this);

	wxWindow* rightPanel = FindWindowByName("rightSplitPanel");
	rightPanel->SetDoubleBuffered(true);

	xrc->AttachUnknownControl("mGLView", glView, this);

	project = new OutfitProject(this);	// Create empty project
	CreateSetSliders();

	bEditSlider = false;
	activeItem = nullptr;
	selectedItems.clear();

	previousMirror = true;
	previewScale = Vector3(1.0f, 1.0f, 1.0f);

	SetSize(size);
	SetPosition(pos);

	wxSplitterWindow* splitter = (wxSplitterWindow*)FindWindowByName("splitter");
	int sashPos = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.sashpos");
	splitter->SetSashPosition(sashPos);

	wxSplitterWindow* splitterRight = (wxSplitterWindow*)FindWindowByName("splitterRight");
	int sashRightPos = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.sashrightpos");
	splitterRight->SetSashPosition(sashRightPos);

	leftPanel->Layout();

	SetDropTarget(new DnDFile(this));

	wxLogMessage("Outfit Studio frame loaded.");
}

void OutfitStudioFrame::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close(true);
}

void OutfitStudioFrame::OnClose(wxCloseEvent& WXUNUSED(event)) {
	if (project) {
		delete project;
		project = nullptr;
	}

	for (auto &sd : sliderDisplays)
		delete sd.second;

	if (glView)
		delete glView;

	int ret = OutfitStudioConfig.SaveConfig("OutfitStudio.xml", "OutfitStudioConfig");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);

	wxLogMessage("Outfit Studio frame closed.");
	Destroy();
}

void OutfitStudioFrame::OnChooseTargetGame(wxCommandEvent& event) {
	wxChoice* choiceTargetGame = (wxChoice*)event.GetEventObject();
	wxWindow* parent = choiceTargetGame->GetGrandParent();
	wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*parent, "fpSkeletonFile", wxFilePickerCtrl);
	wxChoice* choiceSkeletonRoot = XRCCTRL(*parent, "choiceSkeletonRoot", wxChoice);

	TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
	switch (targ) {
	case FO3:
	case FONV:
		fpSkeletonFile->SetPath("res\\skeleton_fo3nv.nif");
		choiceSkeletonRoot->SetStringSelection("Bip01");
		break;
	case SKYRIM:
		fpSkeletonFile->SetPath("res\\skeleton_female_sk.nif");
		choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
		break;
	case SKYRIMSE:
	case SKYRIMVR:
		fpSkeletonFile->SetPath("res\\skeleton_female_sse.nif");
		choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
		break;
	case FO4:
	case FO4VR:
	default:
		fpSkeletonFile->SetPath("res\\skeleton_fo4.nif");
		choiceSkeletonRoot->SetStringSelection("Root");
		break;
	}

	wxCheckListBox* dataFileList = XRCCTRL(*parent, "DataFileList", wxCheckListBox);
	wxString dataDir = wxGetApp().GetGameDataPath(targ);

	wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*parent, "dpGameDataPath", wxDirPickerCtrl);
	dpGameDataPath->SetPath(dataDir);

	SettingsFillDataFiles(dataFileList, dataDir, targ);
}

void OutfitStudioFrame::SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame) {
	dataFileList->Clear();

	wxString cp = "GameDataFiles/" + TargetGames[targetGame];
	wxString activatedFiles = Config[cp];

	wxStringTokenizer tokenizer(activatedFiles, ";");
	std::map<wxString, bool> fsearch;
	while (tokenizer.HasMoreTokens()) {
		wxString val = tokenizer.GetNextToken().Trim(false).Trim();
		val.MakeLower();
		fsearch[val] = true;
	}

	wxArrayString files;
	wxDir::GetAllFiles(dataDir, &files, "*.ba2", wxDIR_FILES);
	wxDir::GetAllFiles(dataDir, &files, "*.bsa", wxDIR_FILES);
	for (auto& file : files) {
		file = file.AfterLast('\\');
		dataFileList->Insert(file, dataFileList->GetCount());

		if (fsearch.find(file.Lower()) == fsearch.end())
			dataFileList->Check(dataFileList->GetCount() - 1);
	}
}

void OutfitStudioFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(wxSize(525, -1));
		settings->CenterOnParent();

		wxChoice* choiceTargetGame = XRCCTRL(*settings, "choiceTargetGame", wxChoice);
		choiceTargetGame->Select(Config.GetIntValue("TargetGame"));

		wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*settings, "dpGameDataPath", wxDirPickerCtrl);
		wxString gameDataPath = Config["GameDataPath"];
		dpGameDataPath->SetPath(gameDataPath);

		wxCheckBox* cbBBOverrideWarn = XRCCTRL(*settings, "cbBBOverrideWarn", wxCheckBox);
		cbBBOverrideWarn->SetValue(Config["WarnBatchBuildOverride"] != "false");

		wxCheckBox* cbBSATextures = XRCCTRL(*settings, "cbBSATextures", wxCheckBox);
		cbBSATextures->SetValue(Config["BSATextureScan"] != "false");

		wxCheckBox* cbLeftMousePan = XRCCTRL(*settings, "cbLeftMousePan", wxCheckBox);
		cbLeftMousePan->SetValue(Config["Input/LeftMousePan"] != "false");

		wxChoice* choiceLanguage = XRCCTRL(*settings, "choiceLanguage", wxChoice);
		for (int i = 0; i < std::extent<decltype(SupportedLangs)>::value; i++)
			choiceLanguage->AppendString(wxLocale::GetLanguageName(SupportedLangs[i]));

		if (!choiceLanguage->SetStringSelection(wxLocale::GetLanguageName(Config.GetIntValue("Language"))))
			choiceLanguage->SetStringSelection("English");

		wxColourPickerCtrl* cpColorBackground = XRCCTRL(*settings, "cpColorBackground", wxColourPickerCtrl);
		if (Config.Exists("Rendering/ColorBackground")) {
			int colorBackgroundR = Config.GetIntValue("Rendering/ColorBackground.r");
			int colorBackgroundG = Config.GetIntValue("Rendering/ColorBackground.g");
			int colorBackgroundB = Config.GetIntValue("Rendering/ColorBackground.b");
			cpColorBackground->SetColour(wxColour(colorBackgroundR, colorBackgroundG, colorBackgroundB));
		}

		wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*settings, "fpSkeletonFile", wxFilePickerCtrl);
		fpSkeletonFile->SetPath(Config["Anim/DefaultSkeletonReference"]);

		wxChoice* choiceSkeletonRoot = XRCCTRL(*settings, "choiceSkeletonRoot", wxChoice);
		choiceSkeletonRoot->SetStringSelection(Config["Anim/SkeletonRootName"]);

		wxCheckListBox* dataFileList = XRCCTRL(*settings, "DataFileList", wxCheckListBox);
		SettingsFillDataFiles(dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		choiceTargetGame->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
			Config.SetValue("TargetGame", targ);

			if (!dpGameDataPath->GetPath().IsEmpty()) {
				wxFileName gameDataDir = dpGameDataPath->GetDirName();
				Config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToStdString());
				Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), gameDataDir.GetFullPath().ToStdString());
			}

			wxArrayInt items;
			wxString selectedfiles;
			for (int i = 0; i < dataFileList->GetCount(); i++)
				if (!dataFileList->IsChecked(i))
					selectedfiles += dataFileList->GetString(i) + "; ";

			selectedfiles = selectedfiles.BeforeLast(';');
			Config.SetValue("GameDataFiles/" + TargetGames[targ].ToStdString(), selectedfiles.ToStdString());

			Config.SetValue("WarnBatchBuildOverride", cbBBOverrideWarn->IsChecked() ? "true" : "false");
			Config.SetValue("BSATextureScan", cbBSATextures->IsChecked() ? "true" : "false");
			Config.SetValue("Input/LeftMousePan", cbLeftMousePan->IsChecked() ? "true" : "false");

			int oldLang = Config.GetIntValue("Language");
			int newLang = SupportedLangs[choiceLanguage->GetSelection()];
			if (oldLang != newLang) {
				Config.SetValue("Language", newLang);
				wxGetApp().InitLanguage();
			}

			wxColour colorBackground = cpColorBackground->GetColour();
			Config.SetValue("Rendering/ColorBackground.r", colorBackground.Red());
			Config.SetValue("Rendering/ColorBackground.g", colorBackground.Green());
			Config.SetValue("Rendering/ColorBackground.b", colorBackground.Blue());

			wxFileName skeletonFile = fpSkeletonFile->GetFileName();
			Config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToStdString());
			Config.SetValue("Anim/SkeletonRootName", choiceSkeletonRoot->GetStringSelection().ToStdString());

			Config.SaveConfig();
			wxGetApp().InitArchives();
		}

		delete settings;
	}
}

void OutfitStudioFrame::OnSashPosChanged(wxSplitterEvent& event) {
	if (!IsVisible())
		return;

	int pos = event.GetSashPosition();
	if (event.GetId() == XRCID("splitter"))
		OutfitStudioConfig.SetValue("OutfitStudioFrame.sashpos", pos);
	else if (event.GetId() == XRCID("splitterRight"))
		OutfitStudioConfig.SetValue("OutfitStudioFrame.sashrightpos", pos);
}

void OutfitStudioFrame::OnMoveWindow(wxMoveEvent& event) {
	wxPoint p = GetPosition();
	OutfitStudioConfig.SetValue("OutfitStudioFrame.x", p.x);
	OutfitStudioConfig.SetValue("OutfitStudioFrame.y", p.y);
	event.Skip();
}

void OutfitStudioFrame::OnSetSize(wxSizeEvent& event) {
	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		OutfitStudioConfig.SetValue("OutfitStudioFrame.width", p.x);
		OutfitStudioConfig.SetValue("OutfitStudioFrame.height", p.y);
	}

	OutfitStudioConfig.SetValue("OutfitStudioFrame.maximized", maximized ? "true" : "false");
	event.Skip();
}

bool OutfitStudioFrame::LoadProject(const std::string& fileName, const std::string& projectName) {
	std::vector<std::string> setnames;
	SliderSetFile InFile(fileName);
	if (InFile.fail()) {
		wxLogError("Failed to open '%s' as a slider set file!", fileName);
		wxMessageBox(wxString::Format(_("Failed to open '%s' as a slider set file!"), fileName), _("Slider Set Error"), wxICON_ERROR);
		return false;
	}

	std::string outfit;
	InFile.GetSetNames(setnames);

	if (!projectName.empty()) {
		auto it = std::find(setnames.begin(), setnames.end(), projectName);
		if (it != setnames.end())
			outfit = projectName;
	}

	if (outfit.empty()) {
		wxArrayString choices;
		for (auto &s : setnames)
			choices.Add(wxString::FromUTF8(s));

		if (choices.GetCount() > 1) {
			outfit = wxGetSingleChoice(_("Please choose an outfit to load"), _("Load a slider set"), choices, 0, this).ToUTF8();
			if (outfit.empty())
				return false;
		}
		else if (choices.GetCount() == 1)
			outfit = choices.front().ToUTF8();
		else
			return false;
	}

	wxLogMessage("Loading project '%s' from file '%s'...", outfit, fileName);
	StartProgress(_("Loading project..."));

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(this);

	wxLogMessage("Loading outfit data...");
	UpdateProgress(10, _("Loading outfit data..."));
	StartSubProgress(10, 40);

	std::vector<std::string> origShapeOrder;
	int error = project->OutfitFromSliderSet(fileName, outfit, &origShapeOrder);
	if (error) {
		EndProgress();
		wxLogError("Failed to create project (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to create project '%s' from file '%s' (%d)!"), outfit, fileName, error), _("Slider Set Error"), wxICON_ERROR);
		RefreshGUIFromProj();
		return false;
	}

	std::string shape = project->GetBaseShape();
	wxLogMessage("Loading reference shape '%s'...", shape);
	UpdateProgress(50, wxString::Format(_("Loading reference shape '%s'..."), shape));

	if (!shape.empty()) {
		error = project->LoadReferenceNif(project->activeSet.GetInputFileName(), shape, true);
		if (error) {
			EndProgress();
			RefreshGUIFromProj();
			return false;
		}
	}

	project->GetWorkNif()->SetShapeOrder(origShapeOrder);

	wxLogMessage("Loading textures...");
	UpdateProgress(60, _("Loading textures..."));

	project->SetTextures();

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	SetTitle("Outfit Studio - " + wxString::FromUTF8(project->OutfitName()));

	wxLogMessage("Project loaded.");
	UpdateProgress(100, _("Finished"));
	GetMenuBar()->Enable(XRCID("fileSave"), true);
	EndProgress();
	return true;
}

void OutfitStudioFrame::CreateSetSliders() {
	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	if (!sliderScroll)
		return;

	int inc = 1;
	if (project->SliderCount()) {
		inc = 90 / project->SliderCount();
		StartProgress(_("Creating sliders..."));
	}

	UpdateProgress(0, _("Clearing old sliders..."));

	sliderScroll->Freeze();
	sliderScroll->DestroyChildren();
	for (auto &sd : sliderDisplays)
		delete sd.second;

	sliderDisplays.clear();

	wxSizer* rootSz = sliderScroll->GetSizer();

	for (int i = 0; i < project->SliderCount(); i++)  {
		UpdateProgress(inc, _("Loading slider: ") + project->GetSliderName(i));
		if (project->SliderClamp(i))    // clamp sliders are a special case, usually an incorrect scale
			continue;

		createSliderGUI(project->GetSliderName(i), i, sliderScroll, rootSz);
	}

	if (!sliderScroll->GetDropTarget())
		sliderScroll->SetDropTarget(new DnDSliderFile(this));

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	EndProgress();
}

bool OutfitStudioFrame::LoadNIF(const std::string& fileName) {
	StartProgress(_("Importing NIF file..."));
	UpdateProgress(1, _("Importing NIF file..."));

	if (project->ImportNIF(fileName))
		return false;

	UpdateProgress(60, _("Refreshing GUI..."));
	project->SetTextures();
	RefreshGUIFromProj();

	UpdateProgress(100, _("Finished."));
	EndProgress();
	return true;
}

void OutfitStudioFrame::createSliderGUI(const std::string& name, int id, wxScrolledWindow* wnd, wxSizer* rootSz) {
	wxString sn = wxString::FromUTF8(name);

	auto d = new SliderDisplay();
	d->sliderPane = new wxPanel(wnd, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER | wxTAB_TRAVERSAL);
	d->sliderPane->SetBackgroundColour(wxNullColour);
	d->sliderPane->SetMinSize(wxSize(-1, 25));
	d->sliderPane->SetMaxSize(wxSize(-1, 25));

	d->paneSz = new wxBoxSizer(wxHORIZONTAL);

	d->btnSliderEdit = new wxBitmapButton(d->sliderPane, wxID_ANY, wxBitmap("res\\images\\EditSmall.png", wxBITMAP_TYPE_ANY), wxDefaultPosition, wxSize(22, 22), wxBU_AUTODRAW, wxDefaultValidator, sn + "|btn");
	d->btnSliderEdit->SetBitmapDisabled(wxBitmap("res\\images\\EditSmall_d.png", wxBITMAP_TYPE_ANY));
	d->btnSliderEdit->SetToolTip(_("Turn on edit mode for this slider."));
	d->paneSz->Add(d->btnSliderEdit, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnMinus = new wxButton(d->sliderPane, wxID_ANY, "-", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, sn + "|btnMinus");
	d->btnMinus->SetToolTip(_("Weaken slider data by 1%."));
	d->btnMinus->SetForegroundColour(wxTransparentColour);
	d->btnMinus->Hide();
	d->paneSz->Add(d->btnMinus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->btnPlus = new wxButton(d->sliderPane, wxID_ANY, "+", wxDefaultPosition, wxSize(18, 18), 0, wxDefaultValidator, sn + "|btnPlus");
	d->btnPlus->SetToolTip(_("Strengthen slider data by 1%."));
	d->btnPlus->SetForegroundColour(wxTransparentColour);
	d->btnPlus->Hide();
	d->paneSz->Add(d->btnPlus, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->sliderNameCheckID = 1000 + id;
	d->sliderNameCheck = new wxCheckBox(d->sliderPane, d->sliderNameCheckID, "", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, sn + "|check");
	d->sliderNameCheck->SetForegroundColour(wxColour(255, 255, 255));
	d->paneSz->Add(d->sliderNameCheck, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);

	d->sliderName = new wxStaticText(d->sliderPane, wxID_ANY, sn, wxDefaultPosition, wxDefaultSize, 0, sn + "|lbl");
	d->sliderName->SetForegroundColour(wxColour(255, 255, 255));
	d->paneSz->Add(d->sliderName, 0, wxALIGN_CENTER_VERTICAL | wxALL);

	d->sliderID = 2000 + id;
	d->slider = new wxSlider(d->sliderPane, d->sliderID, 0, 0, 100, wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL, wxDefaultValidator, sn + "|slider");
	d->slider->SetMinSize(wxSize(-1, 20));
	d->slider->SetMaxSize(wxSize(-1, 20));

	d->paneSz->Add(d->slider, 1, wxLEFT | wxRIGHT | wxEXPAND, 5);

	d->sliderReadout = new wxTextCtrl(d->sliderPane, wxID_ANY, "0%", wxDefaultPosition, wxSize(40, -1), wxTE_RIGHT | wxTE_PROCESS_ENTER | wxSIMPLE_BORDER, wxDefaultValidator, sn + "|readout");
	d->sliderReadout->SetMaxLength(0);
	d->sliderReadout->SetForegroundColour(wxColour(255, 255, 255));
	d->sliderReadout->SetBackgroundColour(wxColour(48, 48, 48));
	d->sliderReadout->SetMinSize(wxSize(40, 20));
	d->sliderReadout->SetMaxSize(wxSize(40, 20));
	d->sliderReadout->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnReadoutChange, this);
	//d->sliderReadout->Bind(wxEVT_KILL_FOCUS, &OutfitStudioFrame::OnReadoutChange, this);

	d->paneSz->Add(d->sliderReadout, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);

	d->sliderPane->SetSizer(d->paneSz);
	d->sliderPane->Layout();
	d->paneSz->Fit(d->sliderPane);
	rootSz->Add(d->sliderPane, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

	d->hilite = false;

	ShowSliderEffect(id);
	sliderDisplays[name] = d;
}

std::string OutfitStudioFrame::NewSlider(const std::string& suggestedName, bool skipPrompt) {
	std::string baseName = "New Slider";
	if (!suggestedName.empty())
		baseName = suggestedName;

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	if (!skipPrompt) {
		do {
			sliderName = wxGetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), fillName, this).ToUTF8();
			if (sliderName.empty())
				return sliderName;
		} while (project->ValidSlider(sliderName));
	}
	else
		sliderName = fillName;

	wxLogMessage("Creating new slider '%s'.", sliderName);

	createSliderGUI(sliderName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddEmptySlider(sliderName);
	ShowSliderEffect(sliderName);
	sliderScroll->FitInside();

	return sliderName;
}

void OutfitStudioFrame::SetSliderValue(int index, int val) {
	std::string name = project->GetSliderName(index);
	project->SliderValue(index) = val / 100.0f;
	sliderDisplays[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudioFrame::SetSliderValue(const std::string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	sliderDisplays[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderDisplays[name]->slider->SetValue(val);
}

void OutfitStudioFrame::ApplySliders(bool recalcBVH) {
	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;

	for (auto &shape : project->GetWorkNif()->GetShapeNames()) {
		project->GetLiveVerts(shape, verts, &uvs);
		glView->UpdateMeshVertices(shape, &verts, recalcBVH, true, false, &uvs);
	}

	bool tMode = glView->GetTransformMode();
	bool vMode = glView->GetVertexEdit();

	if (tMode)
		glView->ShowTransformTool();
	if (vMode)
		glView->ShowVertexEdit();

	if (!tMode && !vMode)
		glView->Render();
}

void OutfitStudioFrame::ShowSliderEffect(int sliderID, bool show) {
	if (project->ValidSlider(sliderID)) {
		project->SliderShow(sliderID) = show;
		wxCheckBox* sliderState = (wxCheckBox*)FindWindowById(1000 + sliderID);
		if (!sliderState)
			return;

		if (show)
			sliderState->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			sliderState->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudioFrame::ShowSliderEffect(const std::string& sliderName, bool show) {
	if (project->ValidSlider(sliderName)) {
		project->SliderShow(sliderName) = show;
		SliderDisplay* d = sliderDisplays[sliderName];
		if (show)
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
	}
}

void OutfitStudioFrame::UpdateActiveShapeUI() {
	if (!activeItem) {
		if (glView->GetTransformMode())
			glView->ShowTransformTool(false);
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit(false);

		CreateSegmentTree();
		CreatePartitionTree();
		outfitBones->UnselectAll();
	}
	else {
		mesh* m = glView->GetMesh(activeItem->shapeName);
		if (m) {
			if (m->smoothSeamNormals)
				GetMenuBar()->Check(XRCID("btnSmoothSeams"), true);
			else
				GetMenuBar()->Check(XRCID("btnSmoothSeams"), false);

			if (m->lockNormals)
				GetMenuBar()->Check(XRCID("btnLockNormals"), true);
			else
				GetMenuBar()->Check(XRCID("btnLockNormals"), false);

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
			if (glView->GetVertexEdit())
				glView->ShowVertexEdit();
		}
		else {
			GetMenuBar()->Check(XRCID("btnSmoothSeams"), true);
			GetMenuBar()->Check(XRCID("btnLockNormals"), false);
		}

		CreateSegmentTree(activeItem->shapeName);
		CreatePartitionTree(activeItem->shapeName);
		ReselectBone();
	}
}

void OutfitStudioFrame::SelectShape(const std::string& shapeName) {
	wxTreeItemId item;
	wxTreeItemId subitem;
	wxTreeItemIdValue cookie;
	wxTreeItemIdValue subcookie;
	item = outfitShapes->GetFirstChild(shapesRoot, cookie);
	while (item.IsOk()) {
		subitem = outfitShapes->GetFirstChild(item, subcookie);
		while (subitem.IsOk()) {
			if (outfitShapes->GetItemText(subitem) == shapeName) {
				outfitShapes->UnselectAll();
				outfitShapes->SelectItem(subitem);
				outfitShapes->EnsureVisible(subitem);
				return;
			}
			subitem = outfitShapes->GetNextChild(item, subcookie);
		}
		item = outfitShapes->GetNextSibling(item);
	}
}

std::vector<std::string> OutfitStudioFrame::GetShapeList() {
	std::vector<std::string> shapes;
	wxTreeItemIdValue cookie;

	wxTreeItemId curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		wxString shapeName = outfitShapes->GetItemText(curItem);
		shapes.push_back(shapeName.ToStdString());
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
	}

	return shapes;
}

void OutfitStudioFrame::UpdateShapeSource(const std::string& shapeName) {
	project->UpdateShapeFromMesh(shapeName, glView->GetMesh(shapeName));
}

void OutfitStudioFrame::ActiveShapesUpdated(TweakStroke* refStroke, bool bIsUndo) {
	if (bEditSlider) {
		std::vector<mesh*> refMeshes = refStroke->GetRefMeshes();
		for (auto &m : refMeshes) {
			std::unordered_map<ushort, Vector3> strokeDiff;
			if (refStroke->pointStartState.find(m) != refStroke->pointStartState.end()) {
				for (auto &p : refStroke->pointStartState[m]) {
					if (bIsUndo)
						strokeDiff[p.first] = p.second - refStroke->pointEndState[m][p.first];
					else
						strokeDiff[p.first] = refStroke->pointEndState[m][p.first] - p.second;
				}
				project->UpdateMorphResult(m->shapeName, activeSlider, strokeDiff);
			}
		}
	}
	else {
		if (refStroke->BrushType() == TBT_WEIGHT) {
			std::vector<mesh*> refMeshes = refStroke->GetRefMeshes();

			for (auto &m : refMeshes) {
				if (refStroke->pointStartState.find(m) != refStroke->pointStartState.end()) {
					auto weights = project->GetWorkAnim()->GetWeightsPtr(m->shapeName, activeBone);
					if (!weights)
						continue;

					if (bIsUndo) {
						for (auto &p : refStroke->pointStartState[m]) {
							if (p.second.y == 0.0f)
								weights->erase(p.first);
							else
								(*weights)[p.first] = p.second.y;
						}
					}
					else {
						for (auto &p : refStroke->pointEndState[m]) {
							if (p.second.y == 0.0f)
								weights->erase(p.first);
							else
								(*weights)[p.first] = p.second.y;
						}
					}
				}
			}
		}
	}
}

std::vector<ShapeItemData*>& OutfitStudioFrame::GetSelectedItems() {
	return selectedItems;
}

std::string OutfitStudioFrame::GetActiveBone() {
	return activeBone;
}

void OutfitStudioFrame::EnterSliderEdit(const std::string& sliderName) {
	if (sliderName.empty())
		return;

	SliderDisplay* d = sliderDisplays[sliderName];
	if (!d)
		return;

	activeSlider = sliderName;
	bEditSlider = true;

	d->slider->SetValue(100);
	SetSliderValue(activeSlider, 100);

	if (d->sliderNameCheck->Get3StateValue() == wxCheckBoxState::wxCHK_UNCHECKED) {
		d->sliderNameCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		ShowSliderEffect(d->sliderID - 2000, true);
	}

	d->sliderNameCheck->Enable(false);
	d->slider->SetFocus();
	d->btnMinus->Show();
	d->btnPlus->Show();
	d->sliderPane->Layout();
	glView->GetStrokeManager()->PushBVH();
	glView->SetStrokeManager(&d->sliderStrokes);
	MenuEnterSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::ExitSliderEdit() {
	SliderDisplay* d = sliderDisplays[activeSlider];
	if (d) {
		d->sliderNameCheck->Enable(true);
		d->slider->SetValue(0);
		SetSliderValue(activeSlider, 0);
		ShowSliderEffect(activeSlider, true);
		d->slider->SetFocus();
		d->btnMinus->Hide();
		d->btnPlus->Hide();
		d->sliderPane->Layout();
	}

	activeSlider.clear();
	bEditSlider = false;
	glView->GetStrokeManager()->PushBVH();
	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::MenuEnterSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), true);
	menu->Enable(XRCID("menuExportSlider"), true);
	menu->Enable(XRCID("sliderNegate"), true);
	menu->Enable(XRCID("sliderMask"), true);
	menu->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudioFrame::MenuExitSliderEdit() {
	wxMenuBar* menu = GetMenuBar();
	menu->Enable(XRCID("menuImportSlider"), false);
	menu->Enable(XRCID("menuExportSlider"), false);
	menu->Enable(XRCID("sliderNegate"), false);
	menu->Enable(XRCID("sliderMask"), false);
	menu->Enable(XRCID("sliderProperties"), false);
}

bool OutfitStudioFrame::NotifyStrokeStarting() {
	if (!activeItem)
		return false;

	if (bEditSlider || project->AllSlidersZero())
		return true;

	auto activeBrush = glView->GetActiveBrush();
	if (activeBrush) {
		if (activeBrush->Type() == TBT_MASK || activeBrush->Type() == TBT_WEIGHT)
			return true;
	}

	int	response = wxMessageBox(_("You can only edit the base shape when all sliders are zero. Do you wish to set all sliders to zero now?  Note, use the pencil button next to a slider to enable editing of that slider's morph."),
		wxMessageBoxCaptionStr, wxYES_NO, this);

	if (response == wxYES)
		ZeroSliders();

	return false;
}

void OutfitStudioFrame::OnNewProject(wxCommandEvent& WXUNUSED(event)) {
	wxWizard wiz;
	wxWizardPage* pg1;
	wxWizardPage* pg2;
	bool result = false;

	UpdateReferenceTemplates();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		pg2 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj2", wxWizardPageSimple);
		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);

		wxChoice* tmplChoice = XRCCTRL(wiz, "npTemplateChoice", wxChoice);
		for (auto &tmpl : refTemplates)
			tmplChoice->Append(tmpl.name);

		tmplChoice->Select(0);

		wiz.FitToPage(pg1);
		wiz.CenterOnParent();

		result = wiz.RunWizard(pg1);
	}
	if (!result)
		return;

	GetMenuBar()->Enable(XRCID("fileSave"), false);

	std::string outfitName = XRCCTRL(wiz, "npOutfitName", wxTextCtrl)->GetValue().ToUTF8();

	wxLogMessage("Creating project '%s'...", outfitName);
	StartProgress(wxString::Format(_("Creating project '%s'..."), outfitName));

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();
	
	glView->Cleanup();
	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(this);

	UpdateProgress(10, _("Loading reference..."));

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(wiz, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName = refTemplate.ToUTF8();
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const ReferenceTemplate& rt) { return rt.name == tmplName; });
		if (tmpl != refTemplates.end())
			error = project->LoadReferenceTemplate((*tmpl).sourceFile, (*tmpl).set, (*tmpl).shape);
		else
			error = 1;
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(),
				sliderSetName.ToUTF8().data(), false, refShape.ToUTF8().data());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToUTF8().data(), refShape.ToUTF8().data());
		}
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	UpdateProgress(40, _("Loading outfit..."));

	error = 0;
	if (XRCCTRL(wiz, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		wxLogMessage("Loading outfit '%s' from '%s'...", outfitName, fileName);
		if (fileName.Lower().EndsWith(".nif"))
			error = project->ImportNIF(fileName.ToUTF8().data(), true, outfitName);
		else if (fileName.Lower().EndsWith(".obj"))
			error = project->ImportOBJ(fileName.ToUTF8().data(), outfitName);
		else if (fileName.Lower().EndsWith(".fbx"))
			error = project->ImportFBX(fileName.ToUTF8().data(), outfitName);
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == false)
		project->SetTextures({ XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data() });
	else
		project->SetTextures();

	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	if (!outfitName.empty())
		SetTitle("Outfit Studio - " + wxString::FromUTF8(outfitName));

	wxLogMessage("Project created.");
	UpdateProgress(100, _("Finished"));

	EndProgress();
}

void OutfitStudioFrame::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this, _("Select a slider set to load"), "SliderSets", wxEmptyString, "Slider Set Files (*.osp;*.xml)|*.osp;*.xml", wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	std::string fileName = loadProjectDialog.GetPath().ToUTF8();
	LoadProject(fileName);
}

void OutfitStudioFrame::OnLoadReference(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	UpdateReferenceTemplates();

	wxDialog dlg;
	int result = wxID_CANCEL;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadRef", "wxDialog")) {
		XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(dlg, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		wxChoice* tmplChoice = XRCCTRL(dlg, "npTemplateChoice", wxChoice);
		for (auto &tmpl : refTemplates)
			tmplChoice->Append(tmpl.name);

		tmplChoice->Select(0);
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress(_("Loading reference..."));
	glView->DeleteMesh(project->GetBaseShape());

	UpdateProgress(10, _("Loading reference set..."));
	bool mergeSliders = (XRCCTRL(dlg, "chkClearSliders", wxCheckBox)->IsChecked());

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = XRCCTRL(dlg, "npTemplateChoice", wxChoice)->GetStringSelection();
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName = refTemplate.ToUTF8();
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const ReferenceTemplate& rt) { return rt.name == tmplName; });
		if (tmpl != refTemplates.end())
			error = project->LoadReferenceTemplate((*tmpl).sourceFile, (*tmpl).set, (*tmpl).shape, mergeSliders);
		else
			error = 1;
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...",
				refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(),
				sliderSetName.ToUTF8().data(), mergeSliders, refShape.ToUTF8().data());
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders);
		}
	}
	else
		project->ClearReference();

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(project->GetBaseShape());
	if (shape)
		project->SetTextures(shape);

	wxLogMessage("Creating reference...");
	UpdateProgress(60, _("Creating reference..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %d slider(s)...", project->SliderCount());
	UpdateProgress(70, wxString::Format(_("Creating %d slider(s)..."), project->SliderCount()));
	StartSubProgress(70, 99);
	CreateSetSliders();

	ShowSliderEffect(0);

	wxLogMessage("Reference loaded.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);
		if (project->GetWorkNif()->IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	std::string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue();

	GetMenuBar()->Enable(XRCID("fileSave"), false);

	wxLogMessage("Loading outfit...");
	StartProgress(_("Loading outfit..."));

	for (auto &s : project->GetWorkNif()->GetShapeNames()) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s);
		}
	}

	bool keepShapes = XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->IsChecked();
	UpdateProgress(1, _("Loading outfit..."));

	int ret = 0;
	if (XRCCTRL(dlg, "npWorkFile", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->GetPath();
		if (fileName.Lower().EndsWith(".nif")) {
			if (!keepShapes)
				ret = project->ImportNIF(fileName.ToUTF8().data(), true, outfitName);
			else
				ret = project->ImportNIF(fileName.ToUTF8().data(), false);
		}
		else if (fileName.Lower().EndsWith(".obj"))
			ret = project->ImportOBJ(fileName.ToUTF8().data(), outfitName);
		else if (fileName.Lower().EndsWith(".fbx"))
			ret = project->ImportFBX(fileName.ToUTF8().data(), outfitName);
	}
	else
		project->ClearOutfit();

	if (ret) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	if (XRCCTRL(dlg, "npTexAuto", wxRadioButton)->GetValue() == true)
		project->SetTextures();
	else {
		std::vector<std::string> texVec = { XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data() };
		project->SetTextures(texVec);
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	SetTitle("Outfit Studio - " + wxString::FromUTF8(project->OutfitName()));

	wxLogMessage("Outfit loaded.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnUnloadProject(wxCommandEvent& WXUNUSED(event)) {
	int res = wxMessageBox(_("Do you really want to unload the project? All unsaved changes will be lost."), _("Unload Project"), wxYES_NO | wxICON_WARNING, this);
	if (res != wxYES)
		return;

	wxLogMessage("Unloading project...");
	GetMenuBar()->Enable(XRCID("fileSave"), false);

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(this);

	CreateSetSliders();
	RefreshGUIFromProj();
	glView->Render();
}

void OutfitStudioFrame::UpdateReferenceTemplates() {
	refTemplates.clear();

	std::string fileName = "RefTemplates.xml";
	if (!wxFileName::IsFileReadable(fileName))
		return;

	XMLDocument doc;
	XMLElement* root;
	if (doc.LoadFile(fileName.c_str()) == XML_SUCCESS) {
		root = doc.FirstChildElement("RefTemplates");
		if (!root)
			return;

		XMLElement* element = root->FirstChildElement("Template");
		while (element) {
			ReferenceTemplate refTemplate;
			refTemplate.name = element->GetText();
			refTemplate.sourceFile = element->Attribute("sourcefile");
			refTemplate.set = element->Attribute("set");
			refTemplate.shape = element->Attribute("shape");
			refTemplates.push_back(refTemplate);
			element = element->NextSiblingElement("Template");
		}
	}
}

void OutfitStudioFrame::ClearProject() {
	for (auto &s : project->GetWorkNif()->GetShapeNames())
		glView->DeleteMesh(s);

	project->mFileName.clear();
	project->mOutfitName.clear();
	project->mDataDir.clear();
	project->mBaseFile.clear();
	project->mGamePath.clear();
	project->mGameFile.clear();

	if (wxGetApp().targetGame == SKYRIM || wxGetApp().targetGame == SKYRIMSE || wxGetApp().targetGame == SKYRIMVR)
		project->mGenWeights = true;
	else
		project->mGenWeights = false;

	project->mCopyRef = true;

	glView->DestroyOverlays();
	activePartition.Unset();

	SetTitle("Outfit Studio");
}

void OutfitStudioFrame::RenameProject(const std::string& projectName) {
	project->outfitName = projectName;
	if (outfitRoot.IsOk())
		outfitShapes->SetItemText(outfitRoot, wxString::FromUTF8(projectName));

	SetTitle("Outfit Studio - " + wxString::FromUTF8(projectName));
}

void OutfitStudioFrame::RefreshGUIFromProj() {
	if (outfitRoot.IsOk()) {
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	std::vector<std::string> shapes = project->GetWorkNif()->GetShapeNames();

	if (shapes.size() > 0) {
		if (shapes.size() == 1 && project->IsBaseShape(shapes.front()))
			outfitRoot = outfitShapes->AppendItem(shapesRoot, "Reference Only");
		else
			outfitRoot = outfitShapes->AppendItem(shapesRoot, wxString::FromUTF8(project->OutfitName()));
	}

	wxTreeItemId subItem;
	for (auto &shape : shapes) {
		subItem = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(shape));
		outfitShapes->SetItemState(subItem, 0);
		outfitShapes->SetItemData(subItem, new ShapeItemData(shape));

		if (project->IsBaseShape(shape)) {
			outfitShapes->SetItemBold(subItem);
			outfitShapes->SetItemTextColour(subItem, wxColour(0, 255, 0));
		}
	}

	wxTreeItemId itemToSelect;
	wxTreeItemIdValue cookie;
	if (outfitRoot.IsOk())
		itemToSelect = outfitShapes->GetFirstChild(outfitRoot, cookie);

	if (itemToSelect.IsOk()) {
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(itemToSelect);
		outfitShapes->UnselectAll();
		outfitShapes->SelectItem(itemToSelect);
	}
	else
		activeItem = nullptr;

	selectedItems.clear();
	if (activeItem)
		selectedItems.push_back(activeItem);

	outfitShapes->ExpandAll();
	MeshesFromProj();

	AnimationGUIFromProj();
}

void OutfitStudioFrame::AnimationGUIFromProj() {
	std::vector<std::string> activeBones;
	if (outfitBones->GetChildrenCount(bonesRoot) > 0)
		outfitBones->DeleteChildren(bonesRoot);

	activeBone.clear();
	project->GetActiveBones(activeBones);
	for (auto &bone : activeBones)
		outfitBones->AppendItem(bonesRoot, bone);
}

void OutfitStudioFrame::MeshesFromProj(const bool reloadTextures) {
	std::vector<std::string> shapes = project->GetWorkNif()->GetShapeNames();
	for (auto &shape : shapes)
		MeshFromProj(shape, reloadTextures);
}

void OutfitStudioFrame::MeshFromProj(const std::string& shapeName, const bool reloadTextures) {
	if (extInitialized) {
		glView->DeleteMesh(shapeName);
		glView->AddMeshFromNif(project->GetWorkNif(), shapeName);

		MaterialFile matFile;
		bool hasMatFile = project->GetShapeMaterialFile(shapeName, matFile);
		glView->SetMeshTextures(shapeName, project->GetShapeTextures(shapeName), hasMatFile, matFile, reloadTextures);

		UpdateMeshFromSet(shapeName);
		glView->Render();
	}

	std::vector<std::string> selShapes;
	for (auto &i : selectedItems)
		selShapes.push_back(i->shapeName);

	glView->SetActiveShapes(selShapes);

	if (activeItem)
		glView->SetSelectedShape(activeItem->shapeName);
	else
		glView->SetSelectedShape("");

	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::UpdateMeshFromSet(const std::string& shapeName) {
	mesh* m = glView->GetMesh(shapeName);
	if (m) {
		m->smoothSeamNormals = project->activeSet.GetSmoothSeamNormals(shapeName);
		m->lockNormals = project->activeSet.GetLockNormals(shapeName);
	}
}

void OutfitStudioFrame::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();
	std::string copyStr = XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue().ToUTF8();

	project->ReplaceForbidden(copyStr);

	wxString defStr = wxString::FromUTF8(copyStr);
	wxString defSliderSetFile = defStr + ".osp";
	wxString defShapeDataDir = defStr;
	wxString defOutputFile = defStr + ".nif";

	wxFilePickerCtrl* fp = (wxFilePickerCtrl*)win->FindWindowByName("sssSliderSetFile");
	fp->SetPath(defSliderSetFile);

	wxDirPickerCtrl* dp = (wxDirPickerCtrl*)win->FindWindowByName("sssShapeDataFolder");
	dp->SetPath(defShapeDataDir);

	fp = (wxFilePickerCtrl*)win->FindWindowByName("sssShapeDataFile");
	fp->SetPath(defOutputFile);
}

void OutfitStudioFrame::OnSSSGenWeightsTrue(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel("_0/_1.nif");
}

void OutfitStudioFrame::OnSSSGenWeightsFalse(wxCommandEvent& event) {
	wxWindow* win = ((wxRadioButton*)event.GetEventObject())->GetParent();
	XRCCTRL(*win, "m_lowHighInfo", wxStaticText)->SetLabel(".nif");
}

void OutfitStudioFrame::OnSaveSliderSet(wxCommandEvent& event) {
	if (project->mFileName.empty()) {
		OnSaveSliderSetAs(event);
	}
	else {
		if (!project->GetWorkNif()->IsValid()) {
			wxMessageBox(_("There are no valid shapes loaded!"), "Error");
			return;
		}

		if (HasUnweightedCheck())
			return;

		wxLogMessage("Saving project '%s'...", wxString::FromUTF8(project->OutfitName()));
		StartProgress(wxString::Format(_("Saving project '%s'..."), wxString::FromUTF8(project->OutfitName())));
		project->ClearBoneScale();

		std::vector<mesh*> shapeMeshes;
		for (auto &s : project->GetWorkNif()->GetShapeNames())
			if (!project->IsBaseShape(s))
				shapeMeshes.push_back(glView->GetMesh(s));

		project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

		std::string error = project->Save(project->mFileName, project->mOutfitName, project->mDataDir, project->mBaseFile,
			project->mGamePath, project->mGameFile, project->mGenWeights, project->mCopyRef);

		if (!error.empty()) {
			wxLogError(error.c_str());
			wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		}

		EndProgress();
	}
}

void OutfitStudioFrame::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (HasUnweightedCheck())
		return;

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnSSSNameCopy, this);
		XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsTrue, this);
		XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsFalse, this);

		std::string outName;
		if (!project->mOutfitName.empty())
			outName = project->mOutfitName.ToUTF8();
		else if (!project->OutfitName().empty())
			outName = project->OutfitName();
		else
			outName = "New Outfit";

		project->ReplaceForbidden(outName);

		wxString sssName = wxString::FromUTF8(outName);

		XRCCTRL(dlg, "sssName", wxTextCtrl)->SetValue(sssName);
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory("SliderSets");

		if (!project->mFileName.empty())
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(project->mFileName);
		else
			XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetPath(sssName + ".osp");

		if (!project->mDataDir.empty())
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(project->mDataDir);
		else
			XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->SetPath(sssName);

		if (!project->mBaseFile.empty())
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(project->mBaseFile);
		else
			XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->SetPath(sssName + ".nif");

		if (!project->mGamePath.empty())
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(project->mGamePath);
		else
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue("meshes\\armor\\" + sssName);

		if (!project->mGameFile.empty())
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(project->mGameFile);
		else
			XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->ChangeValue(sssName);

		if (project->mGenWeights) {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(true);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(false);
		}
		else {
			XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->SetValue(false);
			XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->SetValue(true);
		}
		if (project->GetBaseShape().empty()) {
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->Disable();
		}
		else
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(project->mCopyRef);

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	wxString strFileName;
	wxString strOutfitName;
	wxString strDataDir;
	wxString strBaseFile;
	wxString strGamePath;
	wxString strGameFile;
	bool copyRef;
	bool genWeights;

	strFileName = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strFileName.length() <= 4) {
		wxMessageBox(_("Invalid or no slider set file specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	if (!strFileName.EndsWith(".osp"))
		strFileName = strFileName.Append(".osp");

	strOutfitName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue();
	if (strOutfitName.empty()) {
		wxMessageBox(_("No outfit name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetDirName().GetFullName();
	if (strDataDir.empty()) {
		strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetPath();
		if (strDataDir.empty()) {
			wxMessageBox(_("No data folder specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
			return;
		}
	}

	wxFileName relativeFolder(strDataDir);
	if (!relativeFolder.IsRelative()) {
		wxString curDir(wxGetCwd());
		wxString dataFolder(wxString::Format("%s/%s", curDir, "ShapeData"));
		relativeFolder.MakeRelativeTo(dataFolder);
		strDataDir = relativeFolder.GetFullPath();
	}

	strBaseFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strBaseFile.length() <= 4) {
		wxMessageBox(_("An invalid or no base outfit .nif file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	if (!strBaseFile.EndsWith(".nif"))
		strBaseFile = strBaseFile.Append(".nif");

	strGamePath = XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->GetValue();
	if (strGamePath.empty()) {
		wxMessageBox(_("No game file path specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	strGameFile = XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->GetValue();
	if (strGameFile.empty()) {
		wxMessageBox(_("No game file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return;
	}

	copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format(_("Saving project '%s'..."), strOutfitName));
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto &s : project->GetWorkNif()->GetShapeNames())
		if (!project->IsBaseShape(s))
			shapeMeshes.push_back(glView->GetMesh(s));

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	std::string error = project->Save(strFileName, strOutfitName, strDataDir, strBaseFile,
		strGamePath, strGameFile, genWeights, copyRef);

	if (error.empty()) {
		GetMenuBar()->Enable(XRCID("fileSave"), true);
		RenameProject(strOutfitName.ToUTF8().data());
	}
	else {
		wxLogError(error.c_str());
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
	}

	EndProgress();
}

void OutfitStudioFrame::OnBrushPane(wxCollapsiblePaneEvent& event) {
	wxCollapsiblePane* brushPane = (wxCollapsiblePane*)event.GetEventObject();
	if (!brushPane)
		return;

	if (!brushPane->IsCollapsed())
		if (!glView->GetEditMode())
			brushPane->Collapse();

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	leftPanel->Layout();
}

void OutfitStudioFrame::OnSetBaseShape(wxCommandEvent& WXUNUSED(event)) {
	wxLogMessage("Setting new base shape.");
	project->ClearBoneScale();

	for (auto &s : project->GetWorkNif()->GetShapeNames())
		UpdateShapeSource(s);

	ZeroSliders();
	if (!activeSlider.empty()) {
		bEditSlider = false;
		SliderDisplay* d = sliderDisplays[activeSlider];
		d->sliderStrokes.Clear(); //InvalidateHistoricalBVH();
		d->slider->SetFocus();
		HighlightSlider("");
		activeSlider.clear();
		glView->SetStrokeManager(nullptr);
	}
}

void OutfitStudioFrame::OnImportNIF(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import NIF file"), wxEmptyString, wxEmptyString, "NIF Files (*.nif)|*.nif", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	StartProgress(_("Importing NIF file..."));
	UpdateProgress(1, _("Importing NIF file..."));

	for (auto &fileName : fileNames)
		project->ImportNIF(fileName.ToUTF8().data(), false);

	UpdateProgress(60, _("Refreshing GUI..."));
	project->SetTextures();
	RefreshGUIFromProj();

	UpdateProgress(100, _("Finished."));
	EndProgress();
}

void OutfitStudioFrame::OnExportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export outfit NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to NIF file '%s'...", fileName);
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto &s : project->GetWorkNif()->GetShapeNames())
		if (!project->IsBaseShape(s))
			shapeMeshes.push_back(glView->GetMesh(s));

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes);
	if (error) {
		wxLogError("Failed to save NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportNIFWithRef(wxCommandEvent& event) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (project->GetBaseShape().empty()) {
		OnExportNIF(event);
		return;
	}

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export project NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project with reference to NIF file '%s'...", fileName);
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto &s : project->GetWorkNif()->GetShapeNames())
		shapeMeshes.push_back(glView->GetMesh(s));

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes, true);
	if (error) {
		wxLogError("Failed to save NIF file '%s' with reference!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s' with reference!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export selected shapes to NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	std::vector<std::string> shapes;
	for (auto &i : selectedItems)
		shapes.push_back(i->shapeName);

	wxLogMessage("Exporting selected shapes to NIF file '%s'.", fileName);
	project->ClearBoneScale();

	if (project->ExportShapeNIF(fileName.ToUTF8().data(), shapes)) {
		wxLogError("Failed to export selected shapes to NIF file '%s'!", fileName);
		wxMessageBox(_("Failed to export selected shapes to NIF file!"), _("Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import .obj file for new shape"), wxEmptyString, wxEmptyString, "OBJ Files (*.obj)|*.obj", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	for (auto &fileName : fileNames) {
		wxLogMessage("Importing shape(s) from OBJ file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = project->ImportOBJ(fileName.ToUTF8().data(), project->OutfitName(), activeItem->shapeName);
		else
			ret = project->ImportOBJ(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from OBJ file '%s'.", activeItem->shapeName, fileName);
	}

	RefreshGUIFromProj();
	wxLogMessage("Imported shape(s) from OBJ.");
	glView->Render();
}

void OutfitStudioFrame::OnExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	wxString fileName = wxFileSelector(_("Export project as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);
	project->ClearBoneScale();

	if (project->ExportOBJ(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapeNames(), Vector3(0.1f, 0.1f, 0.1f))) {
		wxLogError("Failed to export OBJ file '%s'!", fileName);
		wxMessageBox(_("Failed to export OBJ file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString fileName = wxFileSelector(_("Export selected shapes as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "OBJ Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as OBJ file to '%s'.", fileName);
		project->ClearBoneScale();

		std::vector<std::string> shapes;
		shapes.reserve(selectedItems.size());
		for (auto &i : selectedItems)
			shapes.push_back(i->shapeName);

		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .obj file"), wxEmptyString, wxString(activeItem->shapeName + ".obj"), ".obj", "OBJ Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", activeItem->shapeName, fileName);
		project->ClearBoneScale();

		std::vector<std::string> shapes = { activeItem->shapeName };
		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnImportFBX(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import .fbx file for new shape"), wxEmptyString, wxEmptyString, "FBX Files (*.fbx)|*.fbx", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	for (auto &fileName : fileNames) {
		wxLogMessage("Importing shape(s) from FBX file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = project->ImportFBX(fileName.ToUTF8().data(), project->OutfitName(), activeItem->shapeName);
		else
			ret = project->ImportFBX(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from FBX file '%s'.", activeItem->shapeName, fileName);
	}

	RefreshGUIFromProj();
	wxLogMessage("Imported shape(s) from FBX.");
	glView->Render();
}

void OutfitStudioFrame::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export project as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);
	project->ClearBoneScale();

	if (!project->ExportFBX(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapeNames())) {
		wxLogError("Failed to export FBX file '%s'!", fileName);
		wxMessageBox(_("Failed to export FBX file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString fileName = wxFileSelector(_("Export selected shapes as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as FBX file to '%s'.", fileName);
		project->ClearBoneScale();

		std::vector<std::string> shapes;
		shapes.reserve(selectedItems.size());
		for (auto &i : selectedItems)
			shapes.push_back(i->shapeName);

		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .fbx file"), wxEmptyString, wxString(activeItem->shapeName + ".fbx"), ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as FBX file to '%s'.", activeItem->shapeName, fileName);
		project->ClearBoneScale();

		std::vector<std::string> shapes = { activeItem->shapeName };
		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnImportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	wxString fileName = wxFileSelector(_("Import physics data to project"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_FILE_MUST_EXIST, this);
	if (fileName.IsEmpty())
		return;

	auto physicsBlock = new BSClothExtraData();
	if (!physicsBlock->FromHKX(fileName.ToUTF8().data())) {
		wxLogError("Failed to import physics data file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to import physics data file '%s'!"), fileName), _("Import Error"), wxICON_ERROR);
	}

	auto& physicsData = project->GetClothData();
	physicsData[fileName.ToUTF8().data()] = physicsBlock;
}

void OutfitStudioFrame::OnExportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	auto& physicsData = project->GetClothData();
	if (physicsData.empty()) {
		wxMessageBox(_("There is no physics data loaded!"), _("Info"), wxICON_INFORMATION);
		return;
	}

	wxArrayString fileNames;
	for (auto &data : physicsData)
		fileNames.Add(wxString::FromUTF8(data.first));

	wxSingleChoiceDialog physicsDataChoice(this, _("Please choose the physics data source you want to export."), _("Choose physics data"), fileNames);
	if (physicsDataChoice.ShowModal() == wxID_CANCEL)
		return;

	int sel = physicsDataChoice.GetSelection();
	std::string selString = fileNames[sel].ToUTF8();

	if (!selString.empty()) {
		wxString fileName = wxFileSelector(_("Export physics data"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		if (!physicsData[selString]->ToHKX(fileName.ToUTF8().data())) {
			wxLogError("Failed to save physics data file '%s'!", fileName);
			wxMessageBox(wxString::Format(_("Failed to save physics data file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnMakeConvRef(wxCommandEvent& WXUNUSED(event)) {
	if (project->AllSlidersZero()) {
		wxMessageBox(_("This function requires at least one slider position to be non-zero."));
		return;
	}

	std::string namebase = "ConvertToBase";
	char thename[256];
	_snprintf_s(thename, 256, 256, "%s", namebase.c_str());
	int count = 1;
	while (sliderDisplays.find(thename) != sliderDisplays.end())
		_snprintf_s(thename, 256, 256, "%s%d", namebase.c_str(), count++);

	std::string finalName = wxGetTextFromUser(_("Create a conversion slider for the current slider settings with the following name: "), _("Create New Conversion Slider"), thename, this).ToUTF8();
	if (finalName.empty())
		return;

	wxLogMessage("Creating new conversion slider '%s'...", finalName);

	project->ClearBoneScale();
	project->AddCombinedSlider(finalName);

	std::string shape = project->GetBaseShape();
	if (!shape.empty()) {
		project->UpdateShapeFromMesh(shape, glView->GetMesh(shape));
		project->NegateSlider(finalName, shape);
	}

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);
	for (auto &s : sliderList) {
		if (!s.compare(finalName))
			continue;
		project->DeleteSlider(s);
	}

	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();

	activeSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	CreateSetSliders();
}

void OutfitStudioFrame::OnSelectSliders(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	for (auto &sd : sliderDisplays)
		ShowSliderEffect(sd.first, checked);

	ApplySliders();
}

void OutfitStudioFrame::OnFixedWeight(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bFixedWeight = checked;
}

void OutfitStudioFrame::OnShapeVisToggle(wxTreeEvent& event) {
	bool bVis = true;
	bool bGhost = false;

	int state = outfitShapes->GetItemState(event.GetItem());
	if (state == 0) {
		bVis = false;
		bGhost = false;
		state = 1;
	}
	else if (state == 1) {
		bVis = true;
		bGhost = true;
		state = 2;
	}
	else {
		bVis = true;
		bGhost = false;
		state = 0;
	}

	std::string shapeName = outfitShapes->GetItemText(event.GetItem()).ToUTF8();
	glView->SetShapeGhostMode(shapeName, bGhost);
	glView->ShowShape(shapeName, bVis);
	outfitShapes->SetItemState(event.GetItem(), state);

	if (selectedItems.size() > 1) {
		for (auto &i : selectedItems) {
			shapeName = outfitShapes->GetItemText(i->GetId()).ToUTF8();
			glView->SetShapeGhostMode(shapeName, bGhost);
			glView->ShowShape(shapeName, bVis);
			outfitShapes->SetItemState(i->GetId(), state);
		}
	}

	event.Skip();
}

void OutfitStudioFrame::OnShapeSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk())
		return;

	if (outfitShapes->GetItemParent(item).IsOk()) {
		if (outfitShapes->IsSelected(item))
			activeItem = (ShapeItemData*)outfitShapes->GetItemData(item);
		else
			activeItem = nullptr;
	}
	else {
		wxTreeItemIdValue cookie;
		wxTreeItemId subitem = outfitShapes->GetFirstChild(item, cookie);
		if (subitem.IsOk() && outfitShapes->IsSelected(subitem))
			activeItem = (ShapeItemData*)outfitShapes->GetItemData(subitem);
		else
			activeItem = nullptr;
	}

	selectedItems.clear();

	std::vector<std::string> shapeNames;
	wxArrayTreeItemIds selected;
	outfitShapes->GetSelections(selected);

	for (auto &i : selected) {
		if (outfitShapes->GetItemParent(i).IsOk()) {
			auto data = (ShapeItemData*)outfitShapes->GetItemData(i);
			if (data) {
				shapeNames.push_back(data->shapeName);
				selectedItems.push_back(data);

				if (!activeItem)
					activeItem = data;
			}
		}
		else {
			wxTreeItemIdValue cookie;
			wxTreeItemId subitem = outfitShapes->GetFirstChild(i, cookie);
			if (subitem.IsOk()) {
				auto data = (ShapeItemData*)outfitShapes->GetItemData(subitem);
				if (data) {
					shapeNames.push_back(data->shapeName);
					selectedItems.push_back(data);

					if (!activeItem)
						activeItem = data;
				}
			}
		}
	}

	glView->SetActiveShapes(shapeNames);

	if (activeItem)
		glView->SetSelectedShape(activeItem->shapeName);
	else
		glView->SetSelectedShape("");

	UpdateActiveShapeUI();
}

void OutfitStudioFrame::OnShapeActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitShapes->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnShapeProperties(evt);
}

void OutfitStudioFrame::OnBoneSelect(wxTreeEvent& event) {
	project->ClearBoneScale();
	boneScale->SetValue(0);

	wxTreeItemId item = event.GetItem();
	if (!activeItem || !item.IsOk())
		return;

	// Clear history
	glView->GetStrokeManager()->Clear();

	// Clear vcolors of all shapes
	for (auto &s : project->GetWorkNif()->GetShapeNames()) {
		mesh* m = glView->GetMesh(s);
		if (m)
			m->ColorChannelFill(1, 0.0f);
	}

	activeBone.clear();

	if (!outfitBones->IsSelected(item)) {
		wxArrayTreeItemIds selected;
		outfitBones->GetSelections(selected);

		if (!selected.IsEmpty())
			activeBone = outfitBones->GetItemText(selected.front());
	}
	else
		activeBone = outfitBones->GetItemText(item);

	if (!activeBone.empty()) {
		// Show weights of selected shapes without reference
		for (auto &s : selectedItems) {
			if (!project->IsBaseShape(s->shapeName)) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(s->shapeName, activeBone);

				mesh* m = glView->GetMesh(s->shapeName);
				if (m) {
					m->ColorChannelFill(1, 0.0f);
					if (weights) {
						for (auto &bw : *weights)
							m->vcolors[bw.first].y = bw.second;
					}
				}
			}
		}

		// Always show weights of reference shape
		auto weights = project->GetWorkAnim()->GetWeightsPtr(project->GetBaseShape(), activeBone);

		mesh* m = glView->GetMesh(project->GetBaseShape());
		if (m) {
			m->ColorChannelFill(1, 0.0f);
			if (weights) {
				for (auto &bw : *weights)
					m->vcolors[bw.first].y = bw.second;
			}
		}
	}

	glView->Refresh();
}

void OutfitStudioFrame::OnCheckTreeSel(wxTreeEvent& event) {
	int outflags;
	wxPoint p;
	wxGetMousePosition(&p.x, &p.y);
	p = outfitShapes->ScreenToClient(p);

	int mask = wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMSTATEICON;
	outfitShapes->HitTest(p, outflags);
	if ((outflags & mask) != 0)
		event.Veto();
}

void OutfitStudioFrame::OnShapeContext(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (outfitShapes->GetItemParent(item).IsOk()) {
		outfitShapes->SelectItem(item);

		wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuMeshContext");
		if (menu) {
			PopupMenu(menu);
			delete menu;
		}
	}
}

void OutfitStudioFrame::OnShapeDrag(wxTreeEvent& event) {
	if (activeItem) {
		wxPoint p;
		wxGetMousePosition(&p.x, &p.y);
		p = outfitShapes->ScreenToClient(p);

		int outflags;
		outfitShapes->HitTest(p, outflags);

		int mask = wxTREE_HITTEST_ONITEMINDENT | wxTREE_HITTEST_ONITEMSTATEICON;
		if ((outflags & mask) == 0) {
			outfitShapes->SetCursor(wxCURSOR_HAND);
			event.Allow();
		}
		else
			event.Veto();
	}
}

void OutfitStudioFrame::OnShapeDrop(wxTreeEvent& event) {
	outfitShapes->SetCursor(wxNullCursor);
	wxTreeItemId dropItem = event.GetItem();

	if (!dropItem.IsOk() || !activeItem)
		return;

	// Make first child
	if (dropItem == outfitRoot)
		dropItem = 0;
	
	// Duplicate item
	wxTreeItemId movedItem = outfitShapes->InsertItem(outfitRoot, dropItem, activeItem->shapeName);
	if (!movedItem.IsOk())
		return;

	// Set data
	ShapeItemData* dropData = new ShapeItemData(activeItem->shapeName);
	outfitShapes->SetItemState(movedItem, 0);
	outfitShapes->SetItemData(movedItem, dropData);
	if (project->IsBaseShape(dropData->shapeName)) {
		outfitShapes->SetItemBold(movedItem);
		outfitShapes->SetItemTextColour(movedItem, wxColour(0, 255, 0));
	}
	
	// Delete old item
	outfitShapes->Delete(activeItem->GetId());

	// Select new item
	outfitShapes->UnselectAll();
	outfitShapes->SelectItem(movedItem);
}

void OutfitStudioFrame::OnBoneContext(wxTreeEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnBoneTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnSegmentSelect(wxTreeEvent& event) {
	ShowSegment(event.GetItem());

	wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
	segmentApply->Enable();

	wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");
	segmentReset->Enable();
}

void OutfitStudioFrame::OnSegmentContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	segmentTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(event.GetItem()));
	if (subSegmentData)
		menu = wxXmlResource::Get()->LoadMenu("menuSubSegmentContext");
	else
		menu = wxXmlResource::Get()->LoadMenu("menuSegmentContext");

	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnSegmentTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuSegmentTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnAddSegment(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId newItem;
	if (!activeSegment.IsOk() || segmentTree->GetChildrenCount(segmentRoot) <= 0) {
		std::vector<Triangle> shapeTris;
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
		if (shape)
			shape->GetTriangles(shapeTris);

		std::set<uint> tris;
		for (uint id = 0; id < shapeTris.size(); id++)
			tris.insert(id);

		newItem = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(tris));
	}
	else
		newItem = segmentTree->InsertItem(segmentRoot, activeSegment, "Segment", -1, -1, new SegmentItemData(std::set<uint>()));

	if (newItem.IsOk()) {
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
}

void OutfitStudioFrame::OnAddSubSegment(wxCommandEvent& WXUNUSED(event)) {
	wxTreeItemId newItem;
	wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
	if (parent == segmentRoot) {
		std::set<uint> tris;
		if (segmentTree->GetChildrenCount(activeSegment) <= 0) {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData)
				tris = segmentData->tris;
		}

		newItem = segmentTree->PrependItem(activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(tris, 0xFFFFFFFF));
	}
	else
		newItem = segmentTree->InsertItem(parent, activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(std::set<uint>(), 0xFFFFFFFF));

	if (newItem.IsOk()) {
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
}

void OutfitStudioFrame::OnDeleteSegment(wxCommandEvent& WXUNUSED(event)) {
	SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (segmentData) {
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SegmentItemData* siblingData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData) {
				for (auto &id : segmentData->tris)
					siblingData->tris.insert(id);

				wxTreeItemIdValue cookie;
				wxTreeItemId child = segmentTree->GetFirstChild(sibling, cookie);
				if (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						for (auto &id : segmentData->tris)
							childData->tris.insert(id);
					}
				}
			}

			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(sibling);
		}
		else {
			segmentTree->Delete(activeSegment);
			activeSegment.Unset();
		}
	}

	UpdateSegmentNames();
}

void OutfitStudioFrame::OnDeleteSubSegment(wxCommandEvent& WXUNUSED(event)) {
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		std::set<uint> tris = subSegmentData->tris;
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SubSegmentItemData* siblingData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				for (auto &id : tris)
					siblingData->tris.insert(id);

			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(sibling);
		}
		else {
			wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
			segmentTree->UnselectAll();
			segmentTree->Delete(activeSegment);
			segmentTree->SelectItem(parent);
		}
	}

	UpdateSegmentNames();
}

void OutfitStudioFrame::OnSegmentTypeChanged(wxCommandEvent& event) {
	wxChoice* segmentType = (wxChoice*)event.GetEventObject();

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			unsigned long type = 0xFFFFFFFF;

			wxString hashSel = segmentType->GetStringSelection();
			int hashPos = hashSel.First("0x");
			if (hashPos != wxNOT_FOUND)
				hashSel.Mid(hashPos).ToULong(&type, 16);

			subSegmentData->type = type;
			UpdateSegmentNames();
		}
	}
}

void OutfitStudioFrame::OnSegmentApply(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);

	BSSubIndexTriShape::BSSITSSegmentation segmentation;

	uint parentArrayIndex = 0;
	uint segmentIndex = 0;
	uint triangleOffset = 0;

	std::vector<uint> triangles;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData) {
			// Create new segment
			BSSubIndexTriShape::BSSITSSegment segment;
			segment.numPrimitives = segmentData->tris.size();
			segment.startIndex = triangleOffset;

			size_t childCount = segmentTree->GetChildrenCount(child);
			segment.numSubSegments = childCount;

			// Create new segment data record
			BSSubIndexTriShape::BSSITSSubSegmentDataRecord segmentDataRecord;
			segmentDataRecord.segmentUser = segmentIndex;

			segmentation.subSegmentData.arrayIndices.push_back(parentArrayIndex);
			segmentation.subSegmentData.dataRecords.push_back(segmentDataRecord);

			if (childCount > 0) {
				// Add all triangles from the subsegments of the segment
				uint subSegmentNumber = 1;
				wxTreeItemIdValue subCookie;
				wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
				while (subChild.IsOk()) {
					SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
					if (subSegmentData) {
						// Create new subsegment
						BSSubIndexTriShape::BSSITSSubSegment subSegment;
						subSegment.arrayIndex = parentArrayIndex;
						subSegment.numPrimitives = subSegmentData->tris.size();
						subSegment.startIndex = triangleOffset;

						triangleOffset += subSegmentData->tris.size() * 3;
						for (auto &id : subSegmentData->tris)
							triangles.push_back(id);

						segment.subSegments.push_back(subSegment);

						// Create new subsegment data record
						BSSubIndexTriShape::BSSITSSubSegmentDataRecord subSegmentDataRecord;
						subSegmentDataRecord.segmentUser = subSegmentNumber;
						subSegmentDataRecord.unkInt2 = subSegmentData->type;
						subSegmentDataRecord.numData = subSegmentData->extraData.size();
						subSegmentDataRecord.extraData = subSegmentData->extraData;

						segmentation.subSegmentData.dataRecords.push_back(subSegmentDataRecord);
						subSegmentNumber++;
					}

					subChild = segmentTree->GetNextChild(activeSegment, subCookie);
				}
			}
			else {
				// No subsegments, add the triangles of the segment itself
				triangleOffset += segmentData->tris.size() * 3;
				for (auto &id : segmentData->tris)
					triangles.push_back(id);
			}

			segmentation.segments.push_back(segment);

			parentArrayIndex += childCount + 1;
			segmentIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}

	if (!project->GetWorkNif()->ReorderTriangles(activeItem->shapeName, triangles))
		return;

	segmentation.numPrimitives = triangles.size();
	segmentation.numSegments = segmentIndex;
	segmentation.numTotalSegments = parentArrayIndex;

	segmentation.subSegmentData.numSegments = segmentIndex;
	segmentation.subSegmentData.numTotalSegments = parentArrayIndex;

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	segmentation.subSegmentData.ssfFile.SetString(segmentSSF->GetValue().ToStdString());

	project->GetWorkNif()->SetShapeSegments(activeItem->shapeName, segmentation);
	CreateSegmentTree(activeItem->shapeName);
}

void OutfitStudioFrame::OnSegmentReset(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);
	CreateSegmentTree(activeItem->shapeName);
}

void OutfitStudioFrame::CreateSegmentTree(const std::string& shapeName) {
	if (segmentTree->GetChildrenCount(segmentRoot) > 0)
		segmentTree->DeleteChildren(segmentRoot);

	int arrayIndex = 0;
	BSSubIndexTriShape::BSSITSSegmentation segmentation;

	if (project->GetWorkNif()->GetShapeSegments(shapeName, segmentation)) {
		for (int i = 0; i < segmentation.segments.size(); i++) {
			uint startIndex = segmentation.segments[i].startIndex / 3;
			std::set<uint> tris;
			for (int id = startIndex; id < startIndex + segmentation.segments[i].numPrimitives; id++)
				tris.insert(id);

			wxTreeItemId segID = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(tris));
			if (segID.IsOk()) {
				for (int j = 0; j < segmentation.segments[i].subSegments.size(); j++) {
					startIndex = segmentation.segments[i].subSegments[j].startIndex / 3;
					std::set<uint> subTris;
					for (int id = startIndex; id < startIndex + segmentation.segments[i].subSegments[j].numPrimitives; id++)
						subTris.insert(id);

					arrayIndex++;
					segmentTree->AppendItem(segID, "Sub Segment", -1, -1,
						new SubSegmentItemData(subTris, segmentation.subSegmentData.dataRecords[arrayIndex].unkInt2, segmentation.subSegmentData.dataRecords[arrayIndex].extraData));
				}
			}
			arrayIndex++;
		}
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	segmentSSF->ChangeValue(segmentation.subSegmentData.ssfFile.GetString());

	UpdateSegmentNames();
	segmentTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	if (child.IsOk())
		segmentTree->SelectItem(child);
}

void OutfitStudioFrame::ShowSegment(const wxTreeItemId& item, bool updateFromMask) {
	if (!glView->GetSegmentMode())
		return;

	std::unordered_map<ushort, float> mask;
	wxChoice* segmentType = nullptr;
	if (!updateFromMask) {
		segmentType = (wxChoice*)FindWindowByName("segmentType");
		segmentType->Disable();
		segmentType->SetSelection(0);
	}
	else
		glView->GetActiveMask(mask);

	if (item.IsOk())
		activeSegment = item;

	if (!activeSegment.IsOk() || !segmentTree->GetItemParent(activeSegment).IsOk())
		return;

	// Get all triangles of the active shape
	std::vector<Triangle> tris;
	auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
	if (shape)
		shape->GetTriangles(tris);

	wxTreeItemId parent;
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		// Active segment is a subsegment
		parent = segmentTree->GetItemParent(activeSegment);

		if (updateFromMask) {
			subSegmentData->tris.clear();

			// Add triangles from mask
			for (int t = 0; t < tris.size(); t++) {
				if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end())
					subSegmentData->tris.insert(t);
			}

			// Remove new triangles from all other subsegments of the parent segment
			wxTreeItemIdValue cookie;
			wxTreeItemId child = segmentTree->GetFirstChild(parent, cookie);
			while (child.IsOk()) {
				SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childSubSegmentData && childSubSegmentData != subSegmentData)
					for (auto &tri : subSegmentData->tris)
						if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
							childSubSegmentData->tris.erase(tri);

				child = segmentTree->GetNextChild(parent, cookie);
			}

			// Add new triangles to the parent segment as well
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
			if (segmentData) {
				for (auto &tri : subSegmentData->tris)
					segmentData->tris.insert(tri);

				// Remove new triangles from all other segments and their subsegments
				wxTreeItemIdValue segCookie;
				wxTreeItemId segChild = segmentTree->GetFirstChild(segmentRoot, segCookie);
				while (segChild.IsOk()) {
					SegmentItemData* childSegmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(segChild));
					if (childSegmentData && childSegmentData != segmentData) {
						wxTreeItemIdValue subCookie;
						wxTreeItemId subChild = segmentTree->GetFirstChild(segChild, subCookie);
						while (subChild.IsOk()) {
							SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
							if (childSubSegmentData)
								for (auto &tri : segmentData->tris)
									if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
										childSubSegmentData->tris.erase(tri);

							subChild = segmentTree->GetNextChild(segChild, subCookie);
						}

						for (auto &parentTri : segmentData->tris)
							if (childSegmentData->tris.find(parentTri) != childSegmentData->tris.end())
								childSegmentData->tris.erase(parentTri);
					}

					segChild = segmentTree->GetNextChild(segmentRoot, segCookie);
				}
			}
		}
		else {
			if (subSegmentData->type != 0xFFFFFFFF) {
				bool typeFound = false;
				auto typeHash = wxString::Format("0x%08x", subSegmentData->type);
				for (int i = 0; i < segmentType->GetCount(); i++) {
					auto typeString = segmentType->GetString(i);
					if (typeString.Contains(typeHash)) {
						segmentType->SetSelection(i);
						typeFound = true;
						break;
					}
				}

				if (!typeFound)
					segmentType->SetSelection(segmentType->Append(typeHash));
			}

			segmentType->Enable();
		}
	}
	else {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (segmentData) {
			// Active segment is a normal segment
			parent = activeSegment;

			if (updateFromMask) {
				segmentData->tris.clear();

				// Add triangles from mask
				for (int t = 0; t < tris.size(); t++) {
					if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end())
						segmentData->tris.insert(t);
				}

				// Remove new triangles from all other segments and their subsegments
				wxTreeItemIdValue segCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, segCookie);
				while (child.IsOk()) {
					SegmentItemData* childSegmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
					if (childSegmentData && childSegmentData != segmentData) {
						wxTreeItemIdValue subCookie;
						wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
						while (subChild.IsOk()) {
							SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
							if (childSubSegmentData)
								for (auto &tri : segmentData->tris)
									if (childSubSegmentData->tris.find(tri) != childSubSegmentData->tris.end())
										childSubSegmentData->tris.erase(tri);

							subChild = segmentTree->GetNextChild(child, subCookie);
						}

						for (auto &parentTri : segmentData->tris)
							if (childSegmentData->tris.find(parentTri) != childSegmentData->tris.end())
								childSegmentData->tris.erase(parentTri);
					}

					child = segmentTree->GetNextChild(segmentRoot, segCookie);
				}

				// Check if all triangles of the segment are assigned to any subsegment
				for (auto &parentTri : segmentData->tris) {
					bool found = false;
					wxTreeItemIdValue subCookie;
					wxTreeItemId subChild = segmentTree->GetFirstChild(activeSegment, subCookie);
					while (subChild.IsOk()) {
						SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
						if (childSubSegmentData)
							if (childSubSegmentData->tris.find(parentTri) != childSubSegmentData->tris.end())
								found = true;

						if (found)
							break;

						subChild = segmentTree->GetNextChild(activeSegment, subCookie);
					}

					// If not, add it to the last subsegment
					if (!found) {
						wxTreeItemId last = segmentTree->GetLastChild(activeSegment);
						if (last.IsOk()) {
							SubSegmentItemData* lastSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(last));
							if (lastSubSegmentData)
								lastSubSegmentData->tris.insert(parentTri);
						}
					}
				}
			}
		}
	}

	// Display segmentation colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->shapeName);
	if (m) {
		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));

		if (parent.IsOk()) {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
			if (segmentData) {
				size_t childCount = segmentTree->GetChildrenCount(parent);
				if (childCount > 0) {
					// Apply dynamic color to all subsegments of the segment
					float color = 0.0f;
					wxTreeItemIdValue cookie;
					wxTreeItemId child = segmentTree->GetFirstChild(parent, cookie);
					while (child.IsOk()) {
						SubSegmentItemData* childSubSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
						if (childSubSegmentData) {
							color += 1.0f / childCount;

							if (childSubSegmentData != subSegmentData) {
								for (auto &id : childSubSegmentData->tris) {
									if (tris.size() <= id)
										continue;

									m->vcolors[tris[id].p1].y = color;
									m->vcolors[tris[id].p2].y = color;
									m->vcolors[tris[id].p3].y = color;
								}
							}
							else {
								for (auto &id : childSubSegmentData->tris) {
									if (tris.size() <= id)
										continue;

									m->vcolors[tris[id].p1].x = 1.0f;
									m->vcolors[tris[id].p2].x = 1.0f;
									m->vcolors[tris[id].p3].x = 1.0f;
									m->vcolors[tris[id].p1].z = color;
									m->vcolors[tris[id].p2].z = color;
									m->vcolors[tris[id].p3].z = color;
								}
							}
						}

						child = segmentTree->GetNextChild(activeSegment, cookie);
					}
				}
				else {
					// No subsegments, apply fixed color to segment
					for (auto &id : segmentData->tris) {
						if (tris.size() <= id)
							continue;

						m->vcolors[tris[id].p1].x = 1.0f;
						m->vcolors[tris[id].p2].x = 1.0f;
						m->vcolors[tris[id].p3].x = 1.0f;
						m->vcolors[tris[id].p1].z = 1.0f;
						m->vcolors[tris[id].p2].z = 1.0f;
						m->vcolors[tris[id].p3].z = 1.0f;
					}
				}
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::UpdateSegmentNames() {
	auto segmentType = (wxChoice*)FindWindowByName("segmentType");

	int segmentIndex = 0;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		segmentTree->SetItemText(child, wxString::Format("Segment #%d", segmentIndex));

		int subSegmentIndex = 0;
		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);

		while (subChild.IsOk()) {
			std::string subSegmentName = "Default";
			auto subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData && subSegmentData->type != 0xFFFFFFFF) {
				bool typeFound = false;
				auto typeHash = wxString::Format("0x%08x", subSegmentData->type);
				for (int i = 0; i < segmentType->GetCount(); i++) {
					auto typeString = segmentType->GetString(i);
					if (typeString.Contains(typeHash)) {
						subSegmentName = typeString;
						typeFound = true;
						break;
					}
				}

				if (!typeFound)
					subSegmentName = typeHash;
			}

			segmentTree->SetItemText(subChild, wxString::Format("#%d: %s", subSegmentIndex, subSegmentName));

			subChild = segmentTree->GetNextChild(child, subCookie);
			subSegmentIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
		segmentIndex++;
	}
}

void OutfitStudioFrame::OnPartitionSelect(wxTreeEvent& event) {
	ShowPartition(event.GetItem());

	wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
	partitionApply->Enable();

	wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");
	partitionReset->Enable();
}

void OutfitStudioFrame::OnPartitionContext(wxTreeEvent& event) {
	if (!event.GetItem().IsOk())
		return;

	partitionTree->SelectItem(event.GetItem());

	wxMenu* menu = nullptr;
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(event.GetItem()));
	if (partitionData)
		menu = wxXmlResource::Get()->LoadMenu("menuPartitionContext");

	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnPartitionTreeContext(wxCommandEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuPartitionTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnAddPartition(wxCommandEvent& WXUNUSED(event)) {
	bool isSkyrim = wxGetApp().targetGame == SKYRIM || wxGetApp().targetGame == SKYRIMSE || wxGetApp().targetGame == SKYRIMVR;
	
	wxTreeItemId newItem;
	if (!activePartition.IsOk() || partitionTree->GetChildrenCount(partitionRoot) <= 0) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
		if (shape && shape->GetNumVertices() > 0) {
			std::vector<ushort> verts(shape->GetNumVertices());
			for (int id = 0; id < verts.size(); id++)
				verts[id] = id;

			std::vector<Triangle> tris;
			shape->GetTriangles(tris);

			newItem = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1,
				new PartitionItemData(verts, tris, isSkyrim ? 32 : 0));
		}
	}
	else
		newItem = partitionTree->InsertItem(partitionRoot, activePartition, "Partition", -1, -1,
			new PartitionItemData(std::vector<ushort>(), std::vector<Triangle>(), isSkyrim ? 32 : 0));

	if (newItem.IsOk()) {
		partitionTree->UnselectAll();
		partitionTree->SelectItem(newItem);
	}
	
	UpdatePartitionNames();
}

void OutfitStudioFrame::OnDeletePartition(wxCommandEvent& WXUNUSED(event)) {
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		wxTreeItemId sibling = partitionTree->GetPrevSibling(activePartition);
		if (!sibling.IsOk())
			sibling = partitionTree->GetNextSibling(activePartition);

		if (sibling.IsOk()) {
			PartitionItemData* siblingData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(sibling));
			if (siblingData) {
				for (auto &id : partitionData->verts)
					siblingData->verts.push_back(id);
			}

			partitionTree->UnselectAll();
			partitionTree->Delete(activePartition);
			partitionTree->SelectItem(sibling);

			// Force update from mask to fix triangles
			ShowPartition(sibling, true);
		}
		else {
			partitionTree->Delete(activePartition);
			activePartition.Unset();
		}
	}

	UpdatePartitionNames();
}

void OutfitStudioFrame::OnPartitionTypeChanged(wxCommandEvent& event) {
	wxChoice* partitionType = (wxChoice*)event.GetEventObject();

	if (activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			unsigned long type = 0;
			partitionType->GetStringSelection().ToULong(&type);
			partitionData->type = type;
		}
	}

	UpdatePartitionNames();
}

void OutfitStudioFrame::OnPartitionApply(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);

	std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	std::vector<std::vector<ushort>> partitionVerts;
	std::vector<std::vector<Triangle>> partitionTris;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			BSDismemberSkinInstance::PartitionInfo pInfo;
			pInfo.flags = PF_EDITOR_VISIBLE;
			pInfo.partID = partitionData->type;
			partitionInfo.push_back(pInfo);

			partitionVerts.push_back(partitionData->verts);
			partitionTris.push_back(partitionData->tris);
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	project->GetWorkNif()->SetShapePartitions(activeItem->shapeName, partitionInfo, partitionVerts, partitionTris);
	CreatePartitionTree(activeItem->shapeName);
}

void OutfitStudioFrame::OnPartitionReset(wxCommandEvent& event) {
	((wxButton*)event.GetEventObject())->Enable(false);
	CreatePartitionTree(activeItem->shapeName);
}

void OutfitStudioFrame::CreatePartitionTree(const std::string& shapeName) {
	if (partitionTree->GetChildrenCount(partitionRoot) > 0)
		partitionTree->DeleteChildren(partitionRoot);

	std::vector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	std::vector<std::vector<ushort>> partitionVerts;
	std::vector<std::vector<Triangle>> partitionTris;
	if (project->GetWorkNif()->GetShapePartitions(shapeName, partitionInfo, partitionVerts, partitionTris)) {
		partitionInfo.resize(partitionVerts.size());

		for (int i = 0; i < partitionVerts.size(); i++)
			partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(partitionVerts[i], partitionTris[i], partitionInfo[i].partID));
	}

	UpdatePartitionNames();
	partitionTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	if (child.IsOk())
		partitionTree->SelectItem(child);
}

void OutfitStudioFrame::ShowPartition(const wxTreeItemId& item, bool updateFromMask) {
	if (!activeItem || !glView->GetSegmentMode())
		return;

	std::unordered_map<ushort, float> mask;
	wxChoice* partitionType = nullptr;
	wxArrayString partitionStrings;
	if (!updateFromMask) {
		partitionType = (wxChoice*)FindWindowByName("partitionType");
		partitionType->Disable();
		partitionType->SetSelection(0);
		partitionStrings = partitionType->GetStrings();
	}
	else
		glView->GetActiveMask(mask);

	if (item.IsOk())
		activePartition = item;

	if (!activePartition.IsOk() || !partitionTree->GetItemParent(activePartition).IsOk())
		return;

	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		if (!updateFromMask) {
			for (auto &s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					// Show correct data in UI
					partitionType->Enable();
					partitionType->SetStringSelection(s);
				}
			}
		}
		else {
			auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
			if (!shape)
				return;

			// Get all triangles of the active shape
			std::vector<Triangle> tris;
			shape->GetTriangles(tris);

			bool isBSTri = shape->HasType<BSTriShape>();

			// Add vertices and triangles from mask
			std::set<Triangle> realTris;
			partitionData->verts.clear();
			partitionData->tris.clear();
			for (auto &tri : tris) {
				if (mask.find(tri.p1) != mask.end() && mask.find(tri.p2) != mask.end() && mask.find(tri.p3) != mask.end()) {
					Triangle partTri;

					auto p1Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p1);
					if (p1Find == partitionData->verts.end()) {
						partTri.p1 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p1);
					}
					else
						partTri.p1 = p1Find - partitionData->verts.begin();

					auto p2Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p2);
					if (p2Find == partitionData->verts.end()) {
						partTri.p2 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p2);
					}
					else
						partTri.p2 = p2Find - partitionData->verts.begin();

					auto p3Find = find(partitionData->verts.begin(), partitionData->verts.end(), tri.p3);
					if (p3Find == partitionData->verts.end()) {
						partTri.p3 = partitionData->verts.size();
						partitionData->verts.push_back(tri.p3);
					}
					else
						partTri.p3 = p3Find - partitionData->verts.begin();

					tri.rot();
					partTri.rot();

					if (isBSTri)
						partitionData->tris.push_back(tri);
					else
						partitionData->tris.push_back(partTri);

					realTris.insert(tri);
				}
			}

			// Vertex indices that are assigned
			std::set<ushort> vertsToCheck;
			if (!isBSTri) {
				for (auto &tri : partitionData->tris) {
					vertsToCheck.insert(partitionData->verts[tri.p1]);
					vertsToCheck.insert(partitionData->verts[tri.p2]);
					vertsToCheck.insert(partitionData->verts[tri.p3]);
				}
			}
			else {
				for (auto &tri : partitionData->tris) {
					vertsToCheck.insert(tri.p1);
					vertsToCheck.insert(tri.p2);
					vertsToCheck.insert(tri.p3);
				}
			}

			// Remove assigned verts/tris from all other partitions
			wxTreeItemIdValue cookie;
			wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
			while (child.IsOk()) {
				PartitionItemData* childPartition = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
				if (childPartition && childPartition != partitionData) {
					// Move triangles that match to the end
					auto removeTriEnd = childPartition->tris.end();
					if (!isBSTri) {
						removeTriEnd = std::partition(childPartition->tris.begin(), childPartition->tris.end(), [&childPartition, &realTris](const Triangle& tri) {
							Triangle t(childPartition->verts[tri.p1], childPartition->verts[tri.p2], childPartition->verts[tri.p3]);

							if (find_if(realTris.begin(), realTris.end(), [&t](const Triangle& tri) { return tri.CompareIndices(t); }) != realTris.end())
								return true;

							return false;
						});
					}
					else {
						removeTriEnd = std::partition(childPartition->tris.begin(), childPartition->tris.end(), [&childPartition, &realTris](const Triangle& tri) {
							if (find_if(realTris.begin(), realTris.end(), [&tri](const Triangle& t) { return t.CompareIndices(tri); }) != realTris.end())
								return true;

							return false;
						});
					}

					// Find vertices that need to be removed
					std::set<ushort> vertsToRemove;
					auto tri = removeTriEnd;
					auto triEnd = childPartition->tris.end();
					bool removeVert;
					for (auto &v : vertsToCheck) {
						removeVert = true;
						for (tri = removeTriEnd; tri < triEnd; ++tri) {
							const Triangle& t = (*tri);
							if (!isBSTri) {
								if (v == childPartition->verts[t.p1] ||
									v == childPartition->verts[t.p2] ||
									v == childPartition->verts[t.p3]) {
									removeVert = false;
									break;
								}
							}
							else {
								if (v == t.p1 || v == t.p2 || v == t.p3) {
									removeVert = false;
									break;
								}
							}
						}

						if (removeVert)
							vertsToRemove.insert(v);
					}

					// Find vertices that need to be decremented before erasing tris
					std::set<ushort> vertsToDecrement;
					if (!isBSTri) {
						for (auto &v : vertsToRemove) {
							for (auto itTri = childPartition->tris.begin(); itTri < removeTriEnd; ++itTri) {
								const Triangle& t = (*itTri);
								if (v == childPartition->verts[t.p1])
									vertsToDecrement.insert(t.p1);
								else if (v == childPartition->verts[t.p2])
									vertsToDecrement.insert(t.p2);
								else if (v == childPartition->verts[t.p3])
									vertsToDecrement.insert(t.p3);
							}
						}
					}

					// Erase triangles from end
					childPartition->tris.erase(childPartition->tris.begin(), removeTriEnd);

					// Decrement vertex indices in tris
					if (!isBSTri) {
						for (auto &t : childPartition->tris) {
							ushort* p = &t.p1;
							for (int i = 0; i < 3; i++) {
								int pRem = 0;
								for (auto &remPos : vertsToDecrement)
									if (p[i] > remPos)
										pRem++;

								p[i] -= pRem;
							}
						}
					}

					// Erase vertices marked for removal
					auto removeVertEnd = remove_if(childPartition->verts.begin(), childPartition->verts.end(), [&vertsToRemove](const ushort& vert) {
						return (vertsToRemove.find(vert) != vertsToRemove.end());
					});

					childPartition->verts.erase(removeVertEnd, childPartition->verts.end());
				}

				child = partitionTree->GetNextChild(partitionRoot, cookie);
			}
		}
	}

	// Display partition colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->shapeName);
	if (m) {
		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));

		float color = 0.0f;
		wxTreeItemIdValue cookie;
		wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
		size_t childCount = partitionTree->GetChildrenCount(partitionRoot) + 1;
		while (child.IsOk()) {
			PartitionItemData* childPartition = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
			if (childPartition && childPartition != partitionData) {
				color += 1.0f / childCount;

				for (auto &v : childPartition->verts)
					m->vcolors[v].y = color;
			}

			child = partitionTree->GetNextChild(partitionRoot, cookie);
		}

		if (partitionData) {
			for (auto &v : partitionData->verts) {
				m->vcolors[v].x = 1.0f;
				m->vcolors[v].z = 1.0f;
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::UpdatePartitionNames() {
	wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
	wxArrayString partitionStrings = partitionType->GetStrings();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			bool found = false;
			for (auto &s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					partitionTree->SetItemText(child, s);
					found = true;
					break;
				}
			}

			if (!found)
				partitionTree->SetItemText(child, wxString::Format("%d Unknown", partitionData->type));
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}
}

void OutfitStudioFrame::OnCheckBox(wxCommandEvent& event) {
	wxCheckBox* box = (wxCheckBox*)event.GetEventObject();
	if (!box)
		return;

	std::string name = box->GetName().BeforeLast('|');
	ShowSliderEffect(name, event.IsChecked());
	ApplySliders();
}

void OutfitStudioFrame::OnSelectTool(wxCommandEvent& event) {
	int id = event.GetId();
	wxWindow* w = FindFocus();
	wxMenuBar* menuBar = GetMenuBar();
	wxToolBar* toolBar = GetToolBar();

	wxString s = w->GetName();
	if (s.EndsWith("|readout")){
		menuBar->Disable();
		return;
	}

	if (glView->GetActiveBrush() && glView->GetActiveBrush()->Type() == TBT_WEIGHT) {
		glView->SetXMirror(previousMirror);
		menuBar->Check(XRCID("btnXMirror"), previousMirror);
	}

	if (id == XRCID("btnSelect")) {
		glView->SetEditMode(false);
		glView->SetActiveBrush(-1);
		menuBar->Check(XRCID("btnSelect"), true);
		toolBar->ToggleTool(XRCID("btnSelect"), true);
		
		ToggleBrushPane(true);
		return;
	}

	if (id == XRCID("btnTransform")) {
		bool checked = event.IsChecked();
		menuBar->Check(XRCID("btnTransform"), checked);
		toolBar->ToggleTool(XRCID("btnTransform"), checked);
		glView->SetTransformMode(checked);
		return;
	}

	if (id == XRCID("btnPivot")) {
		bool checked = event.IsChecked();
		menuBar->Check(XRCID("btnPivot"), checked);
		toolBar->ToggleTool(XRCID("btnPivot"), checked);
		glView->SetPivotMode(checked);
		return;
	}

	if (id == XRCID("btnVertexEdit")) {
		bool checked = event.IsChecked();
		menuBar->Check(XRCID("btnVertexEdit"), checked);
		toolBar->ToggleTool(XRCID("btnVertexEdit"), checked);
		glView->SetVertexEdit(checked);
		return;
	}

	if (id == XRCID("btnMaskBrush")) {
		glView->SetActiveBrush(0);
		menuBar->Check(XRCID("btnMaskBrush"), true);
		toolBar->ToggleTool(XRCID("btnMaskBrush"), true);
	}
	else if (id == XRCID("btnInflateBrush")) {
		glView->SetActiveBrush(1);
		menuBar->Check(XRCID("btnInflateBrush"), true);
		toolBar->ToggleTool(XRCID("btnInflateBrush"), true);
	}
	else if (id == XRCID("btnDeflateBrush")) {
		glView->SetActiveBrush(2);
		menuBar->Check(XRCID("btnDeflateBrush"), true);
		toolBar->ToggleTool(XRCID("btnDeflateBrush"), true);
	}
	else if (id == XRCID("btnMoveBrush")) {
		glView->SetActiveBrush(3);
		menuBar->Check(XRCID("btnMoveBrush"), true);
		toolBar->ToggleTool(XRCID("btnMoveBrush"), true);
	}
	else if (id == XRCID("btnSmoothBrush")) {
		glView->SetActiveBrush(4);
		menuBar->Check(XRCID("btnSmoothBrush"), true);
		toolBar->ToggleTool(XRCID("btnSmoothBrush"), true);
	}
	else if (id == XRCID("btnWeightBrush")) {
		glView->SetActiveBrush(10);
		menuBar->Check(XRCID("btnWeightBrush"), true);
		toolBar->ToggleTool(XRCID("btnWeightBrush"), true);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		menuBar->Check(XRCID("btnXMirror"), false);
	}
	else {
		glView->SetEditMode(false);
		glView->SetTransformMode(false);
		return;
	}

	// One of the brushes was activated
	glView->SetEditMode();
	glView->SetBrushSize(glView->GetBrushSize());

	CheckBrushBounds();
	UpdateBrushPane();
}

void OutfitStudioFrame::OnSetView(wxCommandEvent& event) {
	int id = event.GetId();

	if (id == XRCID("btnViewFront"))
		glView->SetView('F');
	else if (id == XRCID("btnViewBack"))
		glView->SetView('B');
	else if (id == XRCID("btnViewLeft"))
		glView->SetView('L');
	else if (id == XRCID("btnViewRight"))
		glView->SetView('R');
}

void OutfitStudioFrame::OnTogglePerspective(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	GetMenuBar()->Check(event.GetId(), enabled);
	GetToolBar()->ToggleTool(event.GetId(), enabled);
	glView->SetPerspective(enabled);
}

void OutfitStudioFrame::OnFieldOfViewSlider(wxCommandEvent& event) {
	wxSlider* fovSlider = (wxSlider*)event.GetEventObject();
	int fieldOfView = fovSlider->GetValue();

	wxStaticText* fovLabel = (wxStaticText*)GetToolBar()->FindWindowByName("fovLabel");
	fovLabel->SetLabel(wxString::Format(_("Field of View: %d"), fieldOfView));

	glView->SetFieldOfView(fieldOfView);
}

void OutfitStudioFrame::OnUpdateLights(wxCommandEvent& WXUNUSED(event)) {
	wxSlider* ambientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	wxSlider* frontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	wxSlider* directional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	wxSlider* directional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	wxSlider* directional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");

	int ambient = ambientSlider->GetValue();
	int frontal = frontalSlider->GetValue();
	int directional0 = directional0Slider->GetValue();
	int directional1 = directional1Slider->GetValue();
	int directional2 = directional2Slider->GetValue();
	glView->UpdateLights(ambient, frontal, directional0, directional1, directional2);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Frontal", frontal);
	Config.SetValue("Lights/Directional0", directional0);
	Config.SetValue("Lights/Directional1", directional1);
	Config.SetValue("Lights/Directional2", directional2);
}

void OutfitStudioFrame::OnResetLights(wxCommandEvent& WXUNUSED(event)) {
	int ambient = 20;
	int frontal = 20;
	int directional0 = 60;
	int directional1 = 60;
	int directional2 = 85;

	glView->UpdateLights(ambient, frontal, directional0, directional1, directional2);

	wxSlider* lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
	lightAmbientSlider->SetValue(ambient);

	wxSlider* lightFrontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
	lightFrontalSlider->SetValue(frontal);

	wxSlider* lightDirectional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
	lightDirectional0Slider->SetValue(directional0);

	wxSlider* lightDirectional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
	lightDirectional1Slider->SetValue(directional1);

	wxSlider* lightDirectional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");
	lightDirectional2Slider->SetValue(directional2);

	Config.SetValue("Lights/Ambient", ambient);
	Config.SetValue("Lights/Frontal", frontal);
	Config.SetValue("Lights/Directional0", directional0);
	Config.SetValue("Lights/Directional1", directional1);
	Config.SetValue("Lights/Directional2", directional2);
}

void OutfitStudioFrame::OnClickSliderButton(wxCommandEvent& event) {
	wxWindow* btn = (wxWindow*)event.GetEventObject();
	if (!btn)
		return;

	wxString buttonName = btn->GetName();
	std::string clickedName = buttonName.BeforeLast('|').ToUTF8();
	if (clickedName.empty()) {
		event.Skip();
		return;
	}

	float scale = 0.0f;
	if (buttonName.AfterLast('|') == "btnMinus")
		scale = 0.99f;
	else if (buttonName.AfterLast('|') == "btnPlus")
		scale = 1.01f;

	if (scale != 0.0f) {
		for (auto &i : selectedItems) {
			std::vector<Vector3> verts;
			project->ScaleMorphResult(i->shapeName, activeSlider, scale);
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}
		return;
	}

	if (activeSlider != clickedName)
		EnterSliderEdit(clickedName);
	else
		ExitSliderEdit();
}

void OutfitStudioFrame::OnReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString sn = w->GetName();
	if (!sn.EndsWith("|readout", &sn))
		return;

	std::string sliderName = sn.ToUTF8();

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	SliderDisplay* d = sliderDisplays[sliderName];
	d->slider->SetValue(v);

	int index = project->SliderIndexFromName(sliderName);
	project->SliderValue(index) = v / 100.0f;

	ApplySliders();
}

void OutfitStudioFrame::OnTabButtonClick(wxCommandEvent& event) {
	int id = event.GetId();

	if (id != XRCID("segmentTabButton")) {
		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show(false);
		segmentType->Show(false);
		segmentSSFLabel->Show(false);
		segmentSSF->Show(false);
		segmentApply->Show(false);
		segmentReset->Show(false);

		if (glView->GetSegmentMode())
			glView->ClearActiveColors();

		glView->SetSegmentMode(false);
		glView->SetSegmentsVisible(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		GetMenuBar()->Check(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Check(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Enable(XRCID("btnSelect"), true);
		GetMenuBar()->Enable(XRCID("btnClearMask"), true);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), true);
		GetMenuBar()->Enable(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != XRCID("partitionTabButton")) {
		wxStaticText* partitionTypeLabel = (wxStaticText*)FindWindowByName("partitionTypeLabel");
		wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
		wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
		wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");

		partitionTypeLabel->Show(false);
		partitionType->Show(false);
		partitionApply->Show(false);
		partitionReset->Show(false);

		if (glView->GetSegmentMode())
			glView->ClearActiveColors();

		glView->SetSegmentMode(false);
		glView->SetSegmentsVisible(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		GetMenuBar()->Check(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Check(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), true);
		GetMenuBar()->Enable(XRCID("btnSelect"), true);
		GetMenuBar()->Enable(XRCID("btnClearMask"), true);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), true);
		GetMenuBar()->Enable(XRCID("btnShowMask"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), true);
		GetToolBar()->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != XRCID("boneTabButton")) {
		boneScale->Show(false);

		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");

		boneScaleLabel->Show(false);
		cbFixedWeight->Show(false);

		project->ClearBoneScale();

		glView->SetXMirror(previousMirror);
		glView->SetTransformMode(false);
		glView->SetActiveBrush(1);
		glView->SetEditMode();
		glView->SetWeightVisible(false);

		GetMenuBar()->Check(XRCID("btnXMirror"), previousMirror);
		GetMenuBar()->Check(XRCID("btnInflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnTransform"), true);
		GetMenuBar()->Enable(XRCID("btnPivot"), true);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), true);
		GetMenuBar()->Enable(XRCID("btnWeightBrush"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), true);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), true);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), true);
		GetMenuBar()->Enable(XRCID("deleteVerts"), true);

		GetToolBar()->ToggleTool(XRCID("btnInflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnWeightBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), true);
		GetToolBar()->EnableTool(XRCID("btnPivot"), true);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), true);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), true);
	}

	if (id == XRCID("meshTabButton")) {
		outfitBones->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitShapes->Show();

		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);
	}
	else if (id == XRCID("boneTabButton")) {
		outfitShapes->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitBones->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		boneScale->SetValue(0);
		boneScale->Show();
		
		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");

		boneScaleLabel->Show();
		cbFixedWeight->Show();
		
		glView->SetTransformMode(false);
		glView->SetActiveBrush(10);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetEditMode();
		glView->SetWeightVisible();

		GetMenuBar()->Check(XRCID("btnWeightBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Enable(XRCID("btnWeightBrush"), true);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnPivot"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnWeightBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnWeightBrush"), true);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnPivot"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);

		ReselectBone();
	}
	else if (id == XRCID("segmentTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		segmentTree->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show();
		segmentType->Show();
		segmentSSFLabel->Show();
		segmentSSF->Show();
		segmentApply->Show();
		segmentReset->Show();

		glView->SetActiveBrush(0);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetSegmentMode();
		glView->SetEditMode();
		glView->SetSegmentsVisible();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);
		glView->ClearColors();

		GetMenuBar()->Check(XRCID("btnMaskBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Check(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Check(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("btnSelect"), false);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnPivot"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Enable(XRCID("btnClearMask"), false);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), false);
		GetMenuBar()->Enable(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnMaskBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), false);
		GetToolBar()->EnableTool(XRCID("btnSelect"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnPivot"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), false);

		ShowSegment(segmentTree->GetSelection());
	}
	else if (id == XRCID("partitionTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		segmentTree->Hide();
		lightSettings->Hide();
		partitionTree->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		wxStaticText* partitionTypeLabel = (wxStaticText*)FindWindowByName("partitionTypeLabel");
		wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
		wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
		wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");

		partitionTypeLabel->Show();
		partitionType->Show();
		partitionApply->Show();
		partitionReset->Show();

		glView->SetActiveBrush(0);
		previousMirror = glView->GetXMirror();
		glView->SetXMirror(false);
		glView->SetSegmentMode();
		glView->SetEditMode();
		glView->SetSegmentsVisible();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);
		glView->ClearColors();

		GetMenuBar()->Check(XRCID("btnMaskBrush"), true);
		GetMenuBar()->Check(XRCID("btnXMirror"), false);
		GetMenuBar()->Check(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Check(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("btnSelect"), false);
		GetMenuBar()->Enable(XRCID("btnTransform"), false);
		GetMenuBar()->Enable(XRCID("btnPivot"), false);
		GetMenuBar()->Enable(XRCID("btnVertexEdit"), false);
		GetMenuBar()->Enable(XRCID("btnInflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnDeflateBrush"), false);
		GetMenuBar()->Enable(XRCID("btnMoveBrush"), false);
		GetMenuBar()->Enable(XRCID("btnSmoothBrush"), false);
		GetMenuBar()->Enable(XRCID("btnBrushCollision"), false);
		GetMenuBar()->Enable(XRCID("btnClearMask"), false);
		GetMenuBar()->Enable(XRCID("btnInvertMask"), false);
		GetMenuBar()->Enable(XRCID("btnShowMask"), false);
		GetMenuBar()->Enable(XRCID("deleteVerts"), false);

		GetToolBar()->ToggleTool(XRCID("btnMaskBrush"), true);
		GetToolBar()->ToggleTool(XRCID("btnBrushCollision"), false);
		GetToolBar()->EnableTool(XRCID("btnSelect"), false);
		GetToolBar()->EnableTool(XRCID("btnTransform"), false);
		GetToolBar()->EnableTool(XRCID("btnPivot"), false);
		GetToolBar()->EnableTool(XRCID("btnVertexEdit"), false);
		GetToolBar()->EnableTool(XRCID("btnInflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnDeflateBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnMoveBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnSmoothBrush"), false);
		GetToolBar()->EnableTool(XRCID("btnBrushCollision"), false);

		ShowPartition(partitionTree->GetSelection());
	}
	else if (id == XRCID("lightsTabButton")) {
		outfitShapes->Hide();
		outfitBones->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Show();

		wxStateButton* meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
		wxStateButton* boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
		wxStateButton* segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
		wxStateButton* partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
	}

	CheckBrushBounds();
	UpdateBrushPane();

	wxPanel* topSplitPanel = (wxPanel*)FindWindowByName("topSplitPanel");
	wxPanel* bottomSplitPanel = (wxPanel*)FindWindowByName("bottomSplitPanel");

	topSplitPanel->Layout();
	bottomSplitPanel->Layout();

	Refresh();
}

void OutfitStudioFrame::HighlightSlider(const std::string& name) {
	for (auto &d : sliderDisplays) {
		if (d.first == name) {
			d.second->hilite = true;
			d.second->sliderPane->SetBackgroundColour(wxColour(125, 77, 138));
		}
		else {
			d.second->hilite = false;
			d.second->sliderPane->SetBackgroundColour(wxNullColour);
			d.second->slider->Disable();
			d.second->slider->Enable();
		}
	}
	sliderScroll->Refresh();
}

void OutfitStudioFrame::ZeroSliders() {
	if (!project->AllSlidersZero()) {
		for (int s = 0; s < project->SliderCount(); s++) {
			if (project->SliderClamp(s))
				continue;

			SetSliderValue(s, 0);
			sliderDisplays[project->GetSliderName(s)]->slider->SetValue(0);
		}
		ApplySliders();
	}
}

void OutfitStudioFrame::OnSlider(wxScrollEvent& event) {
	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	wxString sliderName = s->GetName();
	if (sliderName == "brushSize" || sliderName == "brushStrength" || sliderName == "brushFocus" || sliderName == "brushSpacing")
		return;

	if (sliderName != "boneScale") {
		project->ClearBoneScale(false);
		boneScale->SetValue(0);
	}

	if (sliderName == "boneScale") {
		if (!activeBone.empty())
			project->ApplyBoneScale(activeBone, event.GetPosition(), true);

		glView->Render();
		return;
	}

	std::string sn = sliderName.BeforeLast('|').ToUTF8();
	if (sn.empty())
		return;

	SetSliderValue(sn, event.GetPosition());

	if (!bEditSlider && event.GetEventType() == wxEVT_SCROLL_CHANGED)
		ApplySliders(true);
	else
		ApplySliders(false);
}

void OutfitStudioFrame::OnLoadPreset(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	PresetCollection presets;
	std::vector<std::string> names;
	wxChoice* presetChoice;

	std::string choice;
	bool hi = true;

	presets.LoadPresets("SliderPresets", choice, names, true);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto &n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		dlg.SetSize(wxSize(325, 175));
		dlg.SetSizeHints(wxSize(325, 175), wxSize(-1, 175));
		dlg.CenterOnParent();

		if (dlg.ShowModal() != wxID_OK)
			return;

		choice = presetChoice->GetStringSelection();
		if (XRCCTRL(dlg, "weightLo", wxRadioButton)->GetValue())
			hi = false;

		ZeroSliders();

		if (choice == "Zero All") {
			wxLogMessage("Sliders were reset to zero.");
			return;
		}

		wxLogMessage("Applying preset '%s' with option '%s'.", choice, hi ? "High" : "Low");

		float v;
		bool r;
		for (int i = 0; i < project->SliderCount(); i++) {
			if (!presets.GetSliderExists(choice, project->GetSliderName(i)))
				continue;

			if (project->SliderClamp(i))
				continue;

			if (hi)
				r = presets.GetBigPreset(choice, project->GetSliderName(i), v);
			else
				r = presets.GetSmallPreset(choice, project->GetSliderName(i), v);

			if (!r)
				v = project->SliderDefault(i, hi) / 100.0f;
			if (project->SliderInvert(i))
				v = 1.0f - v;

			v *= 100.0f;
			SetSliderValue(i, v);
		}

		ApplySliders();
	}
}

void OutfitStudioFrame::OnSavePreset(wxCommandEvent& WXUNUSED(event)) {
	std::vector<std::string> sliders;
	project->GetSliderList(sliders);
	if (sliders.empty()) {
		wxMessageBox(_("There are no sliders loaded!"), _("Error"), wxICON_ERROR, this);
		return;
	}

	SliderSetGroupCollection groupCollection;
	groupCollection.LoadGroups("SliderGroups");

	std::set<std::string> allGroups;
	groupCollection.GetAllGroups(allGroups);

	PresetSaveDialog psd(this);
	psd.allGroupNames.assign(allGroups.begin(), allGroups.end());
	psd.FilterGroups();

	psd.ShowModal();
	if (psd.outFileName.empty())
		return;

	std::string fileName = psd.outFileName;
	std::string presetName = psd.outPresetName;

	std::vector<std::string> groups;
	groups.assign(psd.outGroups.begin(), psd.outGroups.end());

	bool addedSlider = false;
	PresetCollection presets;
	for (auto &s : sliders) {
		int index = project->SliderIndexFromName(s);
		if (project->SliderZap(index) || project->SliderHidden(index))
			continue;

		float value = project->SliderValue(index);
		if (project->SliderInvert(index))
			value = 1.0f - value;

		if (project->SliderDefault(index, true) == value)
			continue;

		presets.SetSliderPreset(presetName, s, value, -10000.0f);

		if (!addedSlider)
			addedSlider = true;
	}

	if (!addedSlider) {
		wxLogMessage("No changes were made to the sliders, so no preset was saved!");
		wxMessageBox(_("No changes were made to the sliders, so no preset was saved!"), _("Info"), wxICON_INFORMATION, this);
		return;
	}

	int error = presets.SavePreset(fileName, presetName, "OutfitStudioFrame", groups);
	if (error) {
		wxLogError("Failed to save preset (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to save preset (%d)!"), error), _("Error"), wxICON_ERROR, this);
	}
}

void OutfitStudioFrame::OnSliderImportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", activeSlider, activeItem->shapeName, fn);
	project->SetSliderFromBSD(activeSlider, activeItem->shapeName, fn.ToUTF8().data());
	ApplySliders();
}

void OutfitStudioFrame::OnSliderImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .obj file for slider calculation"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromOBJ(activeSlider, activeItem->shapeName, fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderImportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	int result = wxMessageBox(_("This will delete all loaded sliders. Are you sure?"), _("OSD Import"), wxYES_NO | wxCANCEL | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Importing morphs from OSD file '%s'...", fn);

	OSDataFile osd;
	if (!osd.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import OSD file '%s'!", fn);
		wxMessageBox(_("Failed to import OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}

	// Deleting sliders
	sliderScroll->Freeze();
	std::vector<std::string> erase;
	for (auto &sd : sliderDisplays) {
		sd.second->slider->SetValue(0);
		SetSliderValue(sd.first, 0);
		ShowSliderEffect(sd.first, true);
		sd.second->sliderStrokes.Clear();
		sd.second->slider->SetFocus();

		sd.second->btnSliderEdit->Destroy();
		sd.second->slider->Destroy();
		sd.second->sliderName->Destroy();
		sd.second->sliderNameCheck->Destroy();
		sd.second->sliderReadout->Destroy();
		sd.second->sliderPane->Destroy();
		delete sd.second;

		erase.push_back(sd.first);
		project->DeleteSlider(sd.first);
	}

	for (auto &e : erase)
		sliderDisplays.erase(e);

	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();
	sliderScroll->FitInside();
	activeSlider.clear();

	wxString addedDiffs;
	auto diffs = osd.GetDataDiffs();

	for (auto &s : project->GetWorkNif()->GetShapeNames()) {
		bool added = false;
		for (auto &diff : diffs) {
			// Diff name is supposed to begin with matching shape name
			if (diff.first.substr(0, s.size()) != s)
				continue;

			std::string diffName = diff.first.substr(s.length(), diff.first.length() - s.length() + 1);
			if (!project->ValidSlider(diffName)) {
				createSliderGUI(diffName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(diffName);
				ShowSliderEffect(diffName);
			}

			project->SetSliderFromDiff(diffName, s, diff.second);
			added = true;
		}

		if (added)
			addedDiffs += s + "\n";
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();
	ApplySliders();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedDiffs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedDiffs), _("OSD Import"));
}

void OutfitStudioFrame::OnSliderImportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	int result = wxMessageBox(_("This will delete all loaded sliders. Are you sure?"), _("TRI Import"), wxYES_NO | wxCANCEL | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	wxLogMessage("Importing morphs from TRI file '%s'...", fn);

	TriFile tri;
	if (!tri.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to load TRI file '%s'!", fn);
		wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}

	// Deleting sliders
	sliderScroll->Freeze();
	std::vector<std::string> erase;
	for (auto &sd : sliderDisplays) {
		sd.second->slider->SetValue(0);
		SetSliderValue(sd.first, 0);
		ShowSliderEffect(sd.first, true);
		sd.second->sliderStrokes.Clear();
		sd.second->slider->SetFocus();

		sd.second->btnSliderEdit->Destroy();
		sd.second->slider->Destroy();
		sd.second->sliderName->Destroy();
		sd.second->sliderNameCheck->Destroy();
		sd.second->sliderReadout->Destroy();
		sd.second->sliderPane->Destroy();
		delete sd.second;

		erase.push_back(sd.first);
		project->DeleteSlider(sd.first);
	}

	for (auto &e : erase)
		sliderDisplays.erase(e);

	glView->SetStrokeManager(nullptr);
	MenuExitSliderEdit();
	sliderScroll->FitInside();
	activeSlider.clear();

	std::vector<std::string> shapes = project->GetWorkNif()->GetShapeNames();

	wxString addedMorphs;
	auto morphs = tri.GetMorphs();
	for (auto &morph : morphs) {
		if (find(shapes.begin(), shapes.end(), morph.first) == shapes.end())
			continue;

		addedMorphs += morph.first + "\n";
		for (auto &morphData : morph.second) {
			if (!project->ValidSlider(morphData->name)) {
				createSliderGUI(morphData->name, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(morphData->name);
				ShowSliderEffect(morphData->name);
			}

			std::unordered_map<ushort, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			project->SetSliderFromDiff(morphData->name, morph.first, diff);

			if (morphData->type == MORPHTYPE_UV) {
				int sliderIndex = project->SliderIndexFromName(morphData->name);
				project->SetSliderUV(sliderIndex, true);
			}
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();
	ApplySliders();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedMorphs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedMorphs), _("TRI Import"));
}

void OutfitStudioFrame::OnSliderImportFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .fbx file for slider calculation"), wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", activeSlider, activeItem->shapeName, fn);
	if (!project->SetSliderFromFBX(activeSlider, activeItem->shapeName, fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(_("Export .bsd slider data to directory"), wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto &i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + "\\" + i->shapeName + "_" + activeSlider + ".bsd";
			wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderBSD(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(_("Export .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		project->SaveSliderBSD(activeSlider, activeItem->shapeName, fn.ToUTF8().data());
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(_("Export .obj slider data to directory"), wxEmptyString, wxDD_DIR_MUST_EXIST, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto &i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + "\\" + i->shapeName + "_" + activeSlider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->shapeName, targetFile);
			project->SaveSliderOBJ(activeSlider, i->shapeName, targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(_("Export .obj slider data"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->shapeName, fn);
		if (project->SaveSliderOBJ(activeSlider, activeItem->shapeName, fn.ToUTF8().data())) {
			wxLogError("Failed to export OBJ file '%s'!", fn);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Export .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting OSD file to '%s'...", fn);
	if (!project->SaveSliderData(fn.ToUTF8().data())) {
		wxLogError("Failed to export OSD file to '%s'!", fn);
		wxMessageBox(_("Failed to export OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnSliderExportTRI(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Export .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting TRI morphs to '%s'...", fn);
	if (!project->WriteMorphTRI(fn.ToUTF8().data())) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	int result;
	if (selectedItems.size() > 1) {
		std::string prompt = _("Are you sure you wish to clear the unmasked slider data ") + _("for the selected shapes? This cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}
	else {
		std::string prompt = _("Are you sure you wish to clear the unmasked slider data ") + _("for the shape \"") + activeItem->shapeName + _("\"? This cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}

	if (result != wxYES)
		return;

	auto clearSlider = [&](const std::string& sliderName) {
		std::unordered_map<ushort, float> mask;
		for (auto &i : selectedItems) {
			mask.clear();
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				project->ClearUnmaskedDiff(i->shapeName, sliderName, &mask);
			else
				project->ClearSlider(i->shapeName, sliderName);
		}
	};

	if (!bEditSlider) {
		wxLogMessage("Clearing slider data of the checked sliders for the selected shapes.");
		for (auto &sd : sliderDisplays)
			if (sd.second->sliderNameCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED)
				clearSlider(sd.first);
	}
	else {
		wxLogMessage("Clearing slider data of '%s' for the selected shapes.", activeSlider);
		clearSlider(activeSlider);
	}

	ApplySliders();
}

void OutfitStudioFrame::OnNewSlider(wxCommandEvent& WXUNUSED(event)) {
	NewSlider();
}

void OutfitStudioFrame::OnNewZapSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	std::string baseName = "New Zap";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName = wxGetTextFromUser(_("Enter a name for the new zap:"), _("Create New Zap"), fillName, this).ToUTF8();
	if (sliderName.empty())
		return;

	wxLogMessage("Creating new zap '%s'.", sliderName);
	createSliderGUI(sliderName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	std::unordered_map<ushort, float> unmasked;
	for (auto &i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->shapeName);
		project->AddZapSlider(sliderName, unmasked, i->shapeName);
	}

	ShowSliderEffect(sliderName);
	sliderScroll->FitInside();
}

void OutfitStudioFrame::OnNewCombinedSlider(wxCommandEvent& WXUNUSED(event)) {
	std::string baseName = "New Slider";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName = wxGetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), fillName, this).ToUTF8();
	if (sliderName.empty())
		return;

	wxLogMessage("Creating new combined slider '%s'.", sliderName);
	createSliderGUI(sliderName, project->SliderCount(), sliderScroll, sliderScroll->GetSizer());

	project->AddCombinedSlider(sliderName);
	sliderScroll->FitInside();
}

void OutfitStudioFrame::OnSliderNegate(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to negate!"), _("Error"));
		return;
	}

	wxLogMessage("Negating slider '%s' for the selected shapes.", activeSlider);
	for (auto &i : selectedItems)
		project->NegateSlider(activeSlider, i->shapeName);
}

void OutfitStudioFrame::OnMaskAffected(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to create a mask from!"), _("Error"));
		return;
	}

	wxLogMessage("Creating mask for affected vertices of the slider '%s'.", activeSlider);
	for (auto &i : selectedItems)
		project->MaskAffected(activeSlider, i->shapeName);
}

void OutfitStudioFrame::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	std::string prompt = _("Are you sure you wish to delete the selected slider(s)?");
	int result = wxMessageBox(prompt, _("Confirm slider delete"), wxYES_NO | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	auto deleteSlider = [&](const std::string& sliderName) {
		wxLogMessage("Deleting slider '%s'.", sliderName);

		SliderDisplay* sd = sliderDisplays[sliderName];
		sd->slider->SetValue(0);
		SetSliderValue(sliderName, 0);
		ShowSliderEffect(sliderName, true);
		sd->sliderStrokes.Clear();
		sd->slider->SetFocus();

		sd->btnSliderEdit->Destroy();
		sd->slider->Destroy();
		sd->sliderName->Destroy();
		sd->sliderNameCheck->Destroy();
		sd->sliderReadout->Destroy();
		sd->sliderPane->Destroy();

		sliderScroll->FitInside();
		delete sd;
		project->DeleteSlider(sliderName);
	};

	if (!bEditSlider) {
		for (auto it = sliderDisplays.begin(); it != sliderDisplays.end();) {
			if (it->second->sliderNameCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED) {
				deleteSlider(it->first);
				it = sliderDisplays.erase(it);
			}
			else
				++it;
		}
	}
	else {
		deleteSlider(activeSlider);
		sliderDisplays.erase(activeSlider);

		MenuExitSliderEdit();
		glView->SetStrokeManager(nullptr);
		activeSlider.clear();
		bEditSlider = false;
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to show properties for!"), _("Error"));
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		wxTextCtrl* edSliderName = XRCCTRL(dlg, "edSliderName", wxTextCtrl);
		wxStaticText* lbValLo = XRCCTRL(dlg, "lbValLo", wxStaticText);
		wxStaticText* lbValHi = XRCCTRL(dlg, "lbValHi", wxStaticText);
		wxTextCtrl* edValLo = XRCCTRL(dlg, "edValLo", wxTextCtrl);
		wxTextCtrl* edValHi = XRCCTRL(dlg, "edValHi", wxTextCtrl);
		wxCheckBox* cbValZapped = XRCCTRL(dlg, "cbValZapped", wxCheckBox);
		wxCheckBox* chkHidden = XRCCTRL(dlg, "chkHidden", wxCheckBox);
		wxCheckBox* chkInvert = XRCCTRL(dlg, "chkInvert", wxCheckBox);
		wxCheckBox* chkZap = XRCCTRL(dlg, "chkZap", wxCheckBox);
		wxCheckBox* chkUV = XRCCTRL(dlg, "chkUV", wxCheckBox);
		wxCheckListBox* zapToggleList = XRCCTRL(dlg, "zapToggleList", wxCheckListBox);

		int curSlider = project->SliderIndexFromName(activeSlider);
		long loVal = (int)(project->SliderDefault(curSlider, false));
		long hiVal = (int)(project->SliderDefault(curSlider, true));

		edSliderName->SetLabel(wxString::FromUTF8(activeSlider));
		edValLo->SetValue(wxString::Format("%d", loVal));
		edValHi->SetValue(wxString::Format("%d", hiVal));

		if (project->SliderHidden(curSlider))
			chkHidden->SetValue(true);

		if (project->SliderInvert(curSlider))
			chkInvert->SetValue(true);

		if (project->SliderUV(curSlider))
			chkUV->SetValue(true);

		if (project->SliderZap(curSlider)) {
			lbValLo->Hide();
			lbValHi->Hide();
			edValLo->Hide();
			edValHi->Hide();
			cbValZapped->Show();

			chkZap->SetValue(true);
			zapToggleList->Enable();

			for (int i = 0; i < project->SliderCount(); i++)
				if (i != curSlider && (project->SliderZap(i) || project->SliderHidden(i)))
					zapToggleList->Append(wxString::FromUTF8(project->GetSliderName(i)));

			for (auto &s : project->SliderZapToggles(curSlider)) {
				int stringId = zapToggleList->FindString(s, true);
				if (stringId != wxNOT_FOUND)
					zapToggleList->Check(stringId);
			}
		}
		else {
			cbValZapped->Hide();

			if (!project->mGenWeights) {
				lbValLo->Hide();
				edValLo->Hide();
				lbValHi->SetLabel("Default");
			}
		}

		if (hiVal > 0 || loVal > 0)
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);

		chkZap->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& event) {
			bool checked = event.IsChecked();

			lbValLo->Show(!checked);
			lbValHi->Show(!checked);
			edValLo->Show(!checked);
			edValHi->Show(!checked);
			cbValZapped->Show(checked);
			zapToggleList->Enable(checked);

			dlg.Layout();
		});

		XRCCTRL(dlg, "wxID_CANCEL", wxButton)->SetFocus();

		if (dlg.ShowModal() == wxID_OK) {
			if (chkZap->IsChecked()) {
				if (cbValZapped->IsChecked()) {
					loVal = 100;
					hiVal = 100;
				}
				else {
					loVal = 0;
					hiVal = 0;
				}

				wxArrayString zapToggles;
				wxArrayInt toggled;
				zapToggleList->GetCheckedItems(toggled);
				for (auto &i : toggled)
					zapToggles.Add(zapToggleList->GetString(i));

				project->SetSliderZapToggles(curSlider, zapToggles);
			}
			else {
				edValLo->GetValue().ToLong(&loVal);
				edValHi->GetValue().ToLong(&hiVal);
			}

			project->SetSliderZap(curSlider, chkZap->GetValue());
			project->SetSliderInvert(curSlider, chkInvert->GetValue());
			project->SetSliderHidden(curSlider, chkHidden->GetValue());
			project->SetSliderUV(curSlider, chkUV->GetValue());
			project->SetSliderDefault(curSlider, loVal, false);
			project->SetSliderDefault(curSlider, hiVal, true);

			std::string sliderName = edSliderName->GetValue().ToUTF8();
			if (activeSlider != sliderName && !project->ValidSlider(sliderName)) {
				project->SetSliderName(curSlider, sliderName);
				SliderDisplay* d = sliderDisplays[activeSlider];
				sliderDisplays[sliderName] = d;
				sliderDisplays.erase(activeSlider);

				wxString sn = wxString::FromUTF8(sliderName);
				d->slider->SetName(sn + "|slider");
				d->sliderName->SetName(sn + "|lbl");
				d->btnSliderEdit->SetName(sn + "|btn");
				d->sliderNameCheck->SetName(sn + "|check");
				d->sliderReadout->SetName(sn + "|readout");
				d->sliderName->SetLabel(sn);
				activeSlider = std::move(sliderName);
			}
		}
	}
}

void OutfitStudioFrame::OnSliderConformAll(wxCommandEvent& event) {
	std::vector<std::string> shapes = project->GetWorkNif()->GetShapeNames();

	if (shapes.size() - 1 == 0 || project->GetBaseShape().empty())
		return;

	wxLogMessage("Conforming all shapes...");
	StartProgress(_("Conforming all shapes..."));
	int inc = 100 / shapes.size() - 1;
	int pos = 0;

	wxTreeItemId curItem;
	wxTreeItemIdValue cookie;
	auto selectedItemsSave = selectedItems;

	curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		selectedItems.clear();
		selectedItems.push_back((ShapeItemData*)outfitShapes->GetItemData(curItem));
		UpdateProgress(pos * inc, _("Conforming: ") + selectedItems.front()->shapeName);
		StartSubProgress(pos * inc, pos * inc + inc);
		OnSliderConform(event);
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
		pos++;
	}

	selectedItems = std::move(selectedItemsSave);

	if (statusBar)
		statusBar->SetStatusText(_("All shapes conformed."));

	wxLogMessage("All shapes conformed.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	StartProgress(_("Conforming..."));
	if (project->GetBaseShape().empty()) {
		EndProgress();
		return;
	}

	wxLogMessage("Conforming...");
	ZeroSliders();

	for (auto &i : selectedItems) {
		if (project->IsBaseShape(i->shapeName))
			continue;

		wxLogMessage("Conforming '%s'...", i->shapeName);
		UpdateProgress(1, _("Initializing data..."));
		project->InitConform();

		UpdateProgress(50, _("Conforming: ") + i->shapeName);

		project->morpher.CopyMeshMask(glView->GetMesh(i->shapeName), i->shapeName);
		project->ConformShape(i->shapeName);

		UpdateProgress(99);
	}

	project->morpher.ClearProximityCache();

	if (statusBar)
		statusBar->SetStatusText(_("Shape(s) conformed."));

	wxLogMessage("%d shape(s) conformed.", selectedItems.size());
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnInvertUV(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool invertX = (event.GetId() == XRCID("uvInvertX"));
	bool invertY = (event.GetId() == XRCID("uvInvertY"));

	for (auto &i : selectedItems)
		project->GetWorkNif()->InvertUVsForShape(i->shapeName, invertX, invertY);

	MeshesFromProj();
}

void OutfitStudioFrame::OnMirror(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool mirrorX = (event.GetId() == XRCID("mirrorX"));
	bool mirrorY = (event.GetId() == XRCID("mirrorY"));
	bool mirrorZ = (event.GetId() == XRCID("mirrorZ"));

	for (auto &i : selectedItems)
		project->GetWorkNif()->MirrorShape(i->shapeName, mirrorX, mirrorY, mirrorZ);

	MeshesFromProj();
}

void OutfitStudioFrame::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	std::string newShapeName;
	do {
		std::string result = wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape")).ToUTF8();
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	wxLogMessage("Renaming shape '%s' to '%s'.", activeItem->shapeName, newShapeName);
	project->RenameShape(activeItem->shapeName, newShapeName);
	glView->RenameShape(activeItem->shapeName, newShapeName);

	activeItem->shapeName = newShapeName;
	outfitShapes->SetItemText(activeItem->GetId(), wxString::FromUTF8(newShapeName));
}

void OutfitStudioFrame::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (!project->IsBaseShape(activeItem->shapeName))
		project->SetBaseShape(activeItem->shapeName);
	else
		project->SetBaseShape("");

	auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
	if (shape)
		project->SetTextures(shape);

	RefreshGUIFromProj();
}

void OutfitStudioFrame::OnEnterClose(wxKeyEvent& event) {
	if (event.GetKeyCode() == WXK_RETURN) {
		wxDialog* parent = (wxDialog*)((wxWindow*)event.GetEventObject())->GetParent();
		if (!parent)
			return;

		parent->Close();
		parent->SetReturnCode(wxID_OK);
	}
	else
		event.Skip();
}

void OutfitStudioFrame::OnMoveShape(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	Vector3 offs;

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMoveShape")) {
		XRCCTRL(dlg, "msOldOffset", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnMoveShapeOldOffset, this);
		XRCCTRL(dlg, "msSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnMoveShapeSlider, this);
		XRCCTRL(dlg, "msTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnMoveShapeText, this);
		XRCCTRL(dlg, "msTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnMoveShapeText, this);
		XRCCTRL(dlg, "msTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnMoveShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			offs.x = atof(XRCCTRL(dlg, "msTextX", wxTextCtrl)->GetValue().c_str());
			offs.y = atof(XRCCTRL(dlg, "msTextY", wxTextCtrl)->GetValue().c_str());
			offs.z = atof(XRCCTRL(dlg, "msTextZ", wxTextCtrl)->GetValue().c_str());
		}

		std::unordered_map<ushort, float> mask;
		std::unordered_map<ushort, float>* mptr = nullptr;
		std::unordered_map<ushort, Vector3> diff;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			Vector3 d;
			float diffX, diffY, diffZ;
			if (bEditSlider) {
				diff.clear();
				int vertexCount = project->GetVertexCount(i->shapeName);
				for (int j = 0; j < vertexCount; j++) {
					d = (offs - previewMove) * (1.0f - mask[j]);
					diff[j] = d;
					diffX = diff[j].x / -10.0f;
					diffY = diff[j].y / 10.0f;
					diffZ = diff[j].z / 10.0f;
					diff[j].z = diffY;
					diff[j].y = diffZ;
					diff[j].x = diffX;
				}
				project->UpdateMorphResult(i->shapeName, activeSlider, diff);
			}
			else {
				d = offs - previewMove;
				project->OffsetShape(i->shapeName, d, mptr);
			}

			std::vector<Vector3> verts;
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}
		previewMove.Zero();
	}

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::OnMoveShapeOldOffset(wxCommandEvent& event) {
	wxWindow* parent = ((wxButton*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	XRCCTRL(*parent, "msSliderX", wxSlider)->SetValue(0);
	XRCCTRL(*parent, "msSliderY", wxSlider)->SetValue(-2544);
	XRCCTRL(*parent, "msSliderZ", wxSlider)->SetValue(3287);
	XRCCTRL(*parent, "msTextX", wxTextCtrl)->ChangeValue("0.00000");
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->ChangeValue("-2.54431");
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->ChangeValue("3.28790");

	PreviewMove(Vector3(0.00000f, -2.54431f, 3.28790f));
}

void OutfitStudioFrame::OnMoveShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "msSliderX", wxSlider)->GetValue() / 1000.0f;
	slider.y = XRCCTRL(*parent, "msSliderY", wxSlider)->GetValue() / 1000.0f;
	slider.z = XRCCTRL(*parent, "msSliderZ", wxSlider)->GetValue() / 1000.0f;

	XRCCTRL(*parent, "msTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.x));
	XRCCTRL(*parent, "msTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.y));
	XRCCTRL(*parent, "msTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.z));

	PreviewMove(slider);
}

void OutfitStudioFrame::OnMoveShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;
	
	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "msTextX", wxTextCtrl)->GetValue().c_str());
	changed.y = atof(XRCCTRL(*parent, "msTextY", wxTextCtrl)->GetValue().c_str());
	changed.z = atof(XRCCTRL(*parent, "msTextZ", wxTextCtrl)->GetValue().c_str());

	XRCCTRL(*parent, "msSliderX", wxSlider)->SetValue(changed.x * 1000);
	XRCCTRL(*parent, "msSliderY", wxSlider)->SetValue(changed.y * 1000);
	XRCCTRL(*parent, "msSliderZ", wxSlider)->SetValue(changed.z * 1000);

	PreviewMove(changed);
}

void OutfitStudioFrame::PreviewMove(const Vector3& changed) {
	std::unordered_map<ushort, float> mask;
	std::unordered_map<ushort, float>* mptr = nullptr;
	std::unordered_map<ushort, Vector3> diff;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		Vector3 d;
		float diffX, diffY, diffZ;
		if (bEditSlider) {
			int vertexCount = project->GetVertexCount(i->shapeName);
			for (int j = 0; j < vertexCount; j++) {
				d = (changed - previewMove) * (1.0f - mask[j]);
				diff[j] = d;
				diffX = diff[j].x / -10.0f;
				diffY = diff[j].y / 10.0f;
				diffZ = diff[j].z / 10.0f;
				diff[j].z = diffY;
				diff[j].y = diffZ;
				diff[j].x = diffX;
			}
			project->UpdateMorphResult(i->shapeName, activeSlider, diff);
		}
		else {
			d = changed - previewMove;
			project->OffsetShape(i->shapeName, d, mptr);
		}

		std::vector<Vector3> verts;
		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewMove = changed;

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::OnScaleShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		XRCCTRL(dlg, "ssSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnScaleShapeSlider, this);
		XRCCTRL(dlg, "ssTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnScaleShapeText, this);
		XRCCTRL(dlg, "ssTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnScaleShapeText, this);
		XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnScaleShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		Vector3 scale(1.0f, 1.0f, 1.0f);
		if (dlg.ShowModal() == wxID_OK) {
			scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
			scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
			scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());
		}

		Vector3 scaleNew = scale;
		scaleNew.x *= 1.0f / previewScale.x;
		scaleNew.y *= 1.0f / previewScale.y;
		scaleNew.z *= 1.0f / previewScale.z;

		std::unordered_map<ushort, float> mask;
		std::unordered_map<ushort, float>* mptr = nullptr;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			std::vector<Vector3> verts;
			if (bEditSlider) {
				auto& diff = previewDiff[i->shapeName];
				for (auto &d : diff)
					d.second *= -1.0f;

				project->UpdateMorphResult(i->shapeName, activeSlider, diff);
				project->GetLiveVerts(i->shapeName, verts);
				diff.clear();

				Vector3 d;
				float diffX, diffY, diffZ;
				int vertexCount = project->GetVertexCount(i->shapeName);
				for (int j = 0; j < vertexCount; j++) {
					d.x = verts[j].x * scale.x - verts[j].x;
					d.y = verts[j].y * scale.y - verts[j].y;
					d.z = verts[j].z * scale.z - verts[j].z;
					d *= 1.0f - mask[j];
					diff[j] = d;
					diffX = diff[j].x / -10.0f;
					diffY = diff[j].y / 10.0f;
					diffZ = diff[j].z / 10.0f;
					diff[j].z = diffY;
					diff[j].y = diffZ;
					diff[j].x = diffX;
				}
				project->UpdateMorphResult(i->shapeName, activeSlider, diff);
			}
			else
				project->ScaleShape(i->shapeName, scaleNew, mptr);

			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewScale = Vector3(1.0f, 1.0f, 1.0f);
		previewDiff.clear();

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit();
	}
}

void OutfitStudioFrame::OnScaleShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 scale(1.0f, 1.0f, 1.0f);

	bool uniform = XRCCTRL(*parent, "ssUniform", wxCheckBox)->IsChecked();
	if (uniform) {
		float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / 1000.0f;
		scale = Vector3(uniformValue, uniformValue, uniformValue);

		XRCCTRL(*parent, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
		XRCCTRL(*parent, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
		XRCCTRL(*parent, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);
	}
	else {
		scale.x = XRCCTRL(*parent, "ssSliderX", wxSlider)->GetValue() / 1000.0f;
		scale.y = XRCCTRL(*parent, "ssSliderY", wxSlider)->GetValue() / 1000.0f;
		scale.z = XRCCTRL(*parent, "ssSliderZ", wxSlider)->GetValue() / 1000.0f;
	}

	XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
	XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
	XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));

	PreviewScale(scale);
}

void OutfitStudioFrame::OnScaleShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 scale(1.0f, 1.0f, 1.0f);

	bool uniform = XRCCTRL(*parent, "ssUniform", wxCheckBox)->IsChecked();
	if (uniform) {
		float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
		scale = Vector3(uniformValue, uniformValue, uniformValue);

		XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
		XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
		XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
	}
	else {
		scale.x = atof(XRCCTRL(*parent, "ssTextX", wxTextCtrl)->GetValue().c_str());
		scale.y = atof(XRCCTRL(*parent, "ssTextY", wxTextCtrl)->GetValue().c_str());
		scale.z = atof(XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->GetValue().c_str());
	}

	if (scale.x < 0.01f) {
		scale.x = 0.01f;
		XRCCTRL(*parent, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
	}

	if (scale.y < 0.01f) {
		scale.y = 0.01f;
		XRCCTRL(*parent, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
	}

	if (scale.z < 0.01f) {
		scale.z = 0.01f;
		XRCCTRL(*parent, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
	}

	XRCCTRL(*parent, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
	XRCCTRL(*parent, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
	XRCCTRL(*parent, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);

	PreviewScale(scale);
}

void OutfitStudioFrame::PreviewScale(const Vector3& scale) {
	Vector3 scaleNew = scale;
	scaleNew.x *= 1.0f / previewScale.x;
	scaleNew.y *= 1.0f / previewScale.y;
	scaleNew.z *= 1.0f / previewScale.z;

	std::unordered_map<ushort, float> mask;
	std::unordered_map<ushort, float>* mptr = nullptr;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		std::vector<Vector3> verts;
		if (bEditSlider) {
			auto& diff = previewDiff[i->shapeName];
			for (auto &d : diff)
				d.second *= -1.0f;

			project->UpdateMorphResult(i->shapeName, activeSlider, diff);
			project->GetLiveVerts(i->shapeName, verts);
			diff.clear();

			Vector3 d;
			float diffX, diffY, diffZ;
			int vertexCount = project->GetVertexCount(i->shapeName);
			for (int j = 0; j < vertexCount; j++) {
				d.x = verts[j].x * scale.x - verts[j].x;
				d.y = verts[j].y * scale.y - verts[j].y;
				d.z = verts[j].z * scale.z - verts[j].z;
				d *= 1.0f - mask[j];
				diff[j] = d;
				diffX = diff[j].x / -10.0f;
				diffY = diff[j].y / 10.0f;
				diffZ = diff[j].z / 10.0f;
				diff[j].z = diffY;
				diff[j].y = diffZ;
				diff[j].x = diffX;
			}
			project->UpdateMorphResult(i->shapeName, activeSlider, diff);
		}
		else
			project->ScaleShape(i->shapeName, scaleNew, mptr);

		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewScale = scale;

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::OnRotateShape(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("Rotating slider data is currently not possible using the Rotate Shape dialog."), _("Rotate"), wxICON_INFORMATION, this);
		return;
	}

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotateShape")) {
		XRCCTRL(dlg, "rsSliderX", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsSliderY", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsSliderZ", wxSlider)->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnRotateShapeSlider, this);
		XRCCTRL(dlg, "rsTextX", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnRotateShapeText, this);
		XRCCTRL(dlg, "rsTextY", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnRotateShapeText, this);
		XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnRotateShapeText, this);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		Vector3 angle;
		if (dlg.ShowModal() == wxID_OK) {
			angle.x = atof(XRCCTRL(dlg, "rsTextX", wxTextCtrl)->GetValue().c_str());
			angle.y = atof(XRCCTRL(dlg, "rsTextY", wxTextCtrl)->GetValue().c_str());
			angle.z = atof(XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->GetValue().c_str());
		}

		angle -= previewRotation;

		std::unordered_map<ushort, float> mask;
		std::unordered_map<ushort, float>* mptr = nullptr;
		for (auto &i : selectedItems) {
			mask.clear();
			mptr = nullptr;
			glView->GetShapeMask(mask, i->shapeName);
			if (mask.size() > 0)
				mptr = &mask;

			std::vector<Vector3> verts;
			project->RotateShape(i->shapeName, angle, mptr);
			project->GetLiveVerts(i->shapeName, verts);
			glView->UpdateMeshVertices(i->shapeName, &verts);
		}

		previewRotation.Zero();

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit();
	}
}

void OutfitStudioFrame::OnRotateShapeSlider(wxCommandEvent& event) {
	wxWindow* parent = ((wxSlider*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 slider;
	slider.x = XRCCTRL(*parent, "rsSliderX", wxSlider)->GetValue() / 100.0f;
	slider.y = XRCCTRL(*parent, "rsSliderY", wxSlider)->GetValue() / 100.0f;
	slider.z = XRCCTRL(*parent, "rsSliderZ", wxSlider)->GetValue() / 100.0f;

	XRCCTRL(*parent, "rsTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.x));
	XRCCTRL(*parent, "rsTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.y));
	XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", slider.z));

	PreviewRotation(slider);
}

void OutfitStudioFrame::OnRotateShapeText(wxCommandEvent& event) {
	wxWindow* parent = ((wxTextCtrl*)event.GetEventObject())->GetParent();
	if (!parent)
		return;

	Vector3 changed;
	changed.x = atof(XRCCTRL(*parent, "rsTextX", wxTextCtrl)->GetValue().c_str());
	changed.y = atof(XRCCTRL(*parent, "rsTextY", wxTextCtrl)->GetValue().c_str());
	changed.z = atof(XRCCTRL(*parent, "rsTextZ", wxTextCtrl)->GetValue().c_str());

	XRCCTRL(*parent, "rsSliderX", wxSlider)->SetValue(changed.x * 100);
	XRCCTRL(*parent, "rsSliderY", wxSlider)->SetValue(changed.y * 100);
	XRCCTRL(*parent, "rsSliderZ", wxSlider)->SetValue(changed.z * 100);

	PreviewRotation(changed);
}


void OutfitStudioFrame::PreviewRotation(const Vector3& changed) {
	std::unordered_map<ushort, float> mask;
	std::unordered_map<ushort, float>* mptr = nullptr;
	for (auto &i : selectedItems) {
		mask.clear();
		mptr = nullptr;
		glView->GetShapeMask(mask, i->shapeName);
		if (mask.size() > 0)
			mptr = &mask;

		std::vector<Vector3> verts;
		project->RotateShape(i->shapeName, changed - previewRotation, mptr);
		project->GetLiveVerts(i->shapeName, verts);
		glView->UpdateMeshVertices(i->shapeName, &verts);
	}

	previewRotation = changed;

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::OnDeleteVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	if (wxMessageBox(_("Are you sure you wish to delete the unmasked vertices of the selected shapes?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO) == wxNO)
		return;

	bool shapeDeleted = false;
	for (auto &i : selectedItems) {
		std::unordered_map<ushort, float> mask;
		glView->GetShapeUnmasked(mask, i->shapeName);
		if (project->DeleteVerts(i->shapeName, mask))
			shapeDeleted = true;
	}

	project->GetWorkAnim()->CleanupBones();

	if (!shapeDeleted) {
		MeshesFromProj();
		AnimationGUIFromProj();
	}
	else
		RefreshGUIFromProj();

	UpdateActiveShapeUI();

	for (auto &s : sliderDisplays) {
		glView->SetStrokeManager(&s.second->sliderStrokes);
		glView->GetStrokeManager()->Clear();
	}

	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();
	glView->ClearActiveMask();
	ApplySliders();
}

void OutfitStudioFrame::OnSeparateVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	std::unordered_map<ushort, float> masked;
	glView->GetShapeMask(masked, activeItem->shapeName);
	if (masked.empty())
		return;

	std::string newShapeName;
	do {
		std::string result = wxGetTextFromUser(_("Please enter a unique name for the new separated shape."), _("Separate Vertices...")).ToUTF8();
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	project->DuplicateShape(activeItem->shapeName, newShapeName);

	std::unordered_map<ushort, float> unmasked = masked;
	glView->InvertMaskTris(unmasked, activeItem->shapeName);

	project->DeleteVerts(activeItem->shapeName, masked);
	project->DeleteVerts(newShapeName, unmasked);

	project->SetTextures();
	RefreshGUIFromProj();
	UpdateActiveShapeUI();

	for (auto &s : sliderDisplays) {
		glView->SetStrokeManager(&s.second->sliderStrokes);
		glView->GetStrokeManager()->Clear();
	}

	glView->SetStrokeManager(nullptr);
	glView->GetStrokeManager()->Clear();
	glView->ClearActiveMask();
	ApplySliders();
}

void OutfitStudioFrame::OnDupeShape(wxCommandEvent& WXUNUSED(event)) {
	std::string newName;
	wxTreeItemId subitem;
	if (activeItem) {
		if (!outfitRoot.IsOk()) {
			wxMessageBox(_("You can only copy shapes into an outfit, and there is no outfit in the current project. Load one first!"));
			return;
		}

		do {
			std::string result = wxGetTextFromUser(_("Please enter a unique name for the duplicated shape."), _("Duplicate Shape")).ToUTF8();
			if (result.empty())
				return;

			newName = std::move(result);
		} while (project->IsValidShape(newName));

		wxLogMessage("Duplicating shape '%s' as '%s'.", activeItem->shapeName, newName);
		project->ClearBoneScale();
		project->DuplicateShape(activeItem->shapeName, newName);

		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(newName);
		if (shape) {
			glView->AddMeshFromNif(project->GetWorkNif(), newName);
			UpdateMeshFromSet(newName);
			project->SetTextures(shape);

			MaterialFile matFile;
			bool hasMatFile = project->GetShapeMaterialFile(newName, matFile);
			glView->SetMeshTextures(newName, project->GetShapeTextures(newName), hasMatFile, matFile);

			subitem = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(newName));
			outfitShapes->SetItemState(subitem, 0);
			outfitShapes->SetItemData(subitem, new ShapeItemData(newName));

			outfitShapes->UnselectAll();
			outfitShapes->SelectItem(subitem);
		}
	}
}

void OutfitStudioFrame::OnDeleteShape(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxCommandEvent evt;
		OnDeleteSlider(evt);
		return;
	}

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (wxMessageBox(_("Are you sure you wish to delete the selected shapes?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO) == wxNO)
		return;

	std::vector<ShapeItemData> selected;
	for (auto &i : selectedItems)
		selected.push_back(*i);

	for (auto &i : selected) {
		wxLogMessage("Deleting shape '%s'.", i.shapeName);
		project->DeleteShape(i.shapeName);
		glView->DeleteMesh(i.shapeName);
		wxTreeItemId item = i.GetId();
		outfitShapes->Delete(item);
	}

	AnimationGUIFromProj();
	glView->Render();
}

void OutfitStudioFrame::OnAddBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones"))
		return;

	dlg.SetSize(450, 470);
	dlg.CenterOnParent();

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	std::function<void(wxTreeItemId, AnimBone*)> fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
		for (auto &cb : boneParent->children) {
			if (!cb->boneName.empty()) {
				auto newItem = boneTree->AppendItem(treeParent, cb->boneName);
				fAddBoneChildren(newItem, cb);
			}
			else
				fAddBoneChildren(treeParent, cb);
		}
	};

	AnimBone* rb = AnimSkeleton::getInstance().GetBonePtr();
	wxTreeItemId rt = boneTree->AddRoot(rb->boneName);
	fAddBoneChildren(rt, rb);

	if (dlg.ShowModal() == wxID_OK) {
		wxArrayTreeItemIds sel;
		boneTree->GetSelections(sel);
		for (int i = 0; i < sel.size(); i++) {
			std::string bone = boneTree->GetItemText(sel[i]);
			wxLogMessage("Adding bone '%s' to project.", bone);

			project->AddBoneRef(bone);
			outfitBones->AppendItem(bonesRoot, bone);
		}
	}
}

void OutfitStudioFrame::OnAddCustomBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone")) {
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			wxString bone = XRCCTRL(dlg, "boneName", wxTextCtrl)->GetValue();
			if (bone.empty()) {
				wxMessageBox(_("No bone name was entered!"), _("Error"), wxICON_INFORMATION, this);
				return;
			}

			wxTreeItemIdValue cookie;
			wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
			while (item.IsOk()) {
				if (outfitBones->GetItemText(item) == bone) {
					wxMessageBox(wxString::Format(_("Bone '%s' already exists in the project!"), bone), _("Error"), wxICON_INFORMATION, this);
					return;
				}
				item = outfitBones->GetNextChild(bonesRoot, cookie);
			}

			Vector3 translation;
			translation.x = atof(XRCCTRL(dlg, "textX", wxTextCtrl)->GetValue().c_str());
			translation.y = atof(XRCCTRL(dlg, "textY", wxTextCtrl)->GetValue().c_str());
			translation.z = atof(XRCCTRL(dlg, "textZ", wxTextCtrl)->GetValue().c_str());

			wxLogMessage("Adding custom bone '%s' to project.", bone);
			project->AddCustomBoneRef(bone.ToStdString(), translation);
			outfitBones->AppendItem(bonesRoot, bone);
		}
	}
}

void OutfitStudioFrame::OnDeleteBone(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (int i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting bone '%s' from project.", bone);

		project->DeleteBone(bone);
		activeBone.clear();

		outfitBones->Delete(selItems[i]);
	}

	ReselectBone();
}

void OutfitStudioFrame::OnDeleteBoneFromSelected(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (int i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting weights of bone '%s' from selected shapes.", bone);

		for (auto &s : selectedItems)
			project->GetWorkAnim()->RemoveShapeBone(s->shapeName, bone);
	}

	ReselectBone();
}

bool OutfitStudioFrame::HasUnweightedCheck() {
	std::vector<std::string> unweighted;
	if (project->HasUnweighted(&unweighted)) {
		std::string shapesJoin = JoinStrings(unweighted, "; ");
		wxLogWarning(wxString::Format("Unweighted vertices found on shapes: %s", shapesJoin));

		int error = wxMessageBox(wxString::Format("%s\n \n%s", _("The following shapes have unweighted vertices, which can cause issues. The affected vertices have been put under a mask. Do you want to save anyway?"), shapesJoin), _("Unweighted Vertices"), wxYES_NO | wxICON_WARNING, this);
		if (error != wxYES)
			return true;
	}

	return false;
}

bool OutfitStudioFrame::ShowWeightCopy(WeightCopyOptions& options) {
	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyWeights")) {
		XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			float changed = XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->GetValue() / 1000.0f;
			changed = std::min(changed, 15.0f);
			changed = std::max(changed, 0.0f);
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", changed));
		});

		XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			float changed = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			changed = std::min(changed, 15.0f);
			changed = std::max(changed, 0.0f);
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(changed * 1000);
		});

		XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			int changed = XRCCTRL(dlg, "maxResultsSlider", wxSlider)->GetValue();
			changed = std::min(changed, 12);
			changed = std::max(changed, 0);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue(wxString::Format("%d", changed));
		});

		XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			int changed = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			changed = std::min(changed, 12);
			changed = std::max(changed, 0);
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->SetValue(changed);
		});

		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() == wxID_OK) {
			options.proximityRadius = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			options.maxResults = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			return true;
		}
	}

	return false;
}

void OutfitStudioFrame::ReselectBone() {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (!selItems.empty()) {
		wxTreeEvent treeEvent(wxEVT_TREE_SEL_CHANGED, outfitBones, selItems.front());
		OnBoneSelect(treeEvent);
	}
}

void OutfitStudioFrame::OnCopyBoneWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (project->GetBaseShape().empty()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	WeightCopyOptions options;
	if (ShowWeightCopy(options)) {
		StartProgress(_("Copying bone weights..."));

		std::unordered_map<ushort, float> mask;
		for (int i = 0; i < selectedItems.size(); i++) {
			if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
				wxLogMessage("Copying bone weights to '%s'...", selectedItems[i]->shapeName);
				mask.clear();
				glView->GetShapeMask(mask, selectedItems[i]->shapeName);
				project->CopyBoneWeights(selectedItems[i]->shapeName, options.proximityRadius, options.maxResults, &mask);
			}
			else
				wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
		}

		project->morpher.ClearProximityCache();
		ReselectBone();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}

	project->GetWorkAnim()->CleanupBones();
	AnimationGUIFromProj();

	UpdateActiveShapeUI();
}

void OutfitStudioFrame::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (project->GetBaseShape().empty()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	std::vector<std::string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	std::string bonesString;
	for (int i = 0; i < selItems.size(); i++) {
		std::string boneName = outfitBones->GetItemText(selItems[i]).ToStdString();
		bonesString += "'" + boneName + "' ";
		selectedBones.push_back(boneName);
	}

	WeightCopyOptions options;
	if (ShowWeightCopy(options)) {
		StartProgress(_("Copying selected bone weights..."));

		std::unordered_map<ushort, float> mask;
		for (int i = 0; i < selectedItems.size(); i++) {
			if (!project->IsBaseShape(selectedItems[i]->shapeName)) {
				wxLogMessage("Copying selected bone weights to '%s' for %s...", selectedItems[i]->shapeName, bonesString);
				mask.clear();
				glView->GetShapeMask(mask, selectedItems[i]->shapeName);
				project->CopyBoneWeights(selectedItems[i]->shapeName, options.proximityRadius, options.maxResults, &mask, &selectedBones);
			}
			else
				wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
		}

		project->morpher.ClearProximityCache();
		ReselectBone();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}

	project->GetWorkAnim()->CleanupBones();
	AnimationGUIFromProj();

	UpdateActiveShapeUI();
}

void OutfitStudioFrame::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (project->GetBaseShape().empty()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	if (project->IsBaseShape(activeItem->shapeName)) {
		wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself."), _("Error"));
		return;
	}

	int baseVertCount = project->GetVertexCount(project->GetBaseShape());
	int workVertCount = project->GetVertexCount(activeItem->shapeName);
	if (baseVertCount != workVertCount) {
		wxMessageBox(_("The vertex count of the reference and chosen shape is not the same!"), _("Error"));
		return;
	}

	std::vector<std::string> selectedBones;
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	if (selItems.size() < 1)
		return;

	std::string bonesString;
	for (int i = 0; i < selItems.size(); i++) {
		std::string boneName = outfitBones->GetItemText(selItems[i]).ToStdString();
		bonesString += "'" + boneName + "' ";
		selectedBones.push_back(boneName);
	}

	wxLogMessage("Transferring selected bone weights to '%s' for %s...", activeItem->shapeName, bonesString);
	StartProgress(_("Transferring bone weights..."));

	std::unordered_map<ushort, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->shapeName, &mask, &selectedBones);
	ReselectBone();

	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	for (auto &i : selectedItems) {
		mesh* m = glView->GetMesh(i->shapeName);
		if (!m)
			continue;

		m->ColorFill(Vector3(0.0f, 0.0f, 0.0f));

		auto& bones = project->GetWorkAnim()->shapeBones;
		if (bones.find(i->shapeName) != bones.end()) {
			for (auto &b : bones[i->shapeName]) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(i->shapeName, b);
				if (weights) {
					for (auto &bw : *weights)
						if (bw.second > 0.0f)
							m->vcolors[bw.first].x = 1.0f;
				}
			}
		}
	}

	glView->Refresh();
}

void OutfitStudioFrame::OnShapeProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(activeItem->shapeName);
	if (shape) {
		ShapeProperties prop(this, project->GetWorkNif(), shape);
		prop.ShowModal();
	}
}

void OutfitStudioFrame::OnMaskLess(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	glView->MaskLess();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnMaskMore(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	glView->MaskMore();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event) {
	std::string fn = event.GetPath();
	std::vector<std::string> shapes;
	wxWindow* npWiz = ((wxFilePickerCtrl*)event.GetEventObject())->GetParent();
	wxChoice* setNameChoice = (wxChoice*)XRCCTRL((*npWiz), "npSliderSetName", wxChoice);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	XRCCTRL((*npWiz), "npRefIsSliderset", wxRadioButton)->SetValue(true);
	setNameChoice->Clear();
	refShapeChoice->Clear();

	if (fn.rfind(".osp") != std::string::npos || fn.rfind(".xml") != std::string::npos) {
		SliderSetFile ssf(fn);
		if (ssf.fail())
			return;

		std::vector<std::string> setNames;
		ssf.GetSetNames(setNames);

		for (auto &sn : setNames)
			setNameChoice->AppendString(sn);

		if (!setNames.empty()) {
			setNameChoice->SetSelection(0);
			ssf.SetShapes(setNames.front(), shapes);
			for (auto &rsn : shapes)
				refShapeChoice->AppendString(rsn);

			refShapeChoice->SetSelection(0);
		}
	}
	else if (fn.rfind(".nif") != std::string::npos) {
		std::fstream file;
		PlatformUtil::OpenFileStream(file, fn, std::ios::in | std::ios::binary);

		NifFile checkFile;
		if (checkFile.Load(file))
			return;

		for (auto &rsn : checkFile.GetShapeNames())
			refShapeChoice->AppendString(rsn);

		refShapeChoice->SetSelection(0);
	}
}

void OutfitStudioFrame::OnNPWizChangeSetNameChoice(wxCommandEvent& event) {
	wxWindow* npWiz = ((wxChoice*)event.GetEventObject())->GetParent();
	wxFilePickerCtrl* file = (wxFilePickerCtrl*)XRCCTRL((*npWiz), "npSliderSetFile", wxFilePickerCtrl);
	if (!file)
		return;

	std::string fn = file->GetPath();
	SliderSetFile ssf(fn);
	if (ssf.fail())
		return;

	std::vector<std::string> shapes;
	wxChoice* chooser = (wxChoice*)event.GetEventObject();
	ssf.SetShapes(chooser->GetStringSelection().ToStdString(), shapes);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refShapeChoice->Clear();

	for (auto &rsn : shapes)
		refShapeChoice->AppendString(rsn);

	refShapeChoice->SetSelection(0);
}

void OutfitStudioFrame::OnLoadOutfitFP_File(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npWorkFile", wxRadioButton)->SetValue(true);
}

void OutfitStudioFrame::OnLoadOutfitFP_Texture(wxFileDirPickerEvent& event) {
	wxWindow* win = ((wxDialog*)event.GetEventObject())->GetParent();
	XRCCTRL((*win), "npTexFile", wxRadioButton)->SetValue(true);
}

void OutfitStudioFrame::OnBrushSettingsSlider(wxScrollEvent& WXUNUSED(event)) {
	TweakBrush* brush = glView->GetActiveBrush();
	wxCollapsiblePane* parent = (wxCollapsiblePane*)FindWindowByName("brushPane");
	if (!parent)
		return;

	wxStaticText* valSize = (wxStaticText*)XRCCTRL(*parent, "valSize", wxStaticText);
	wxStaticText* valStrength = (wxStaticText*)XRCCTRL(*parent, "valStr", wxStaticText);
	wxStaticText* valFocus = (wxStaticText*)XRCCTRL(*parent, "valFocus", wxStaticText);
	wxStaticText* valSpacing = (wxStaticText*)XRCCTRL(*parent, "valSpace", wxStaticText);

	float slideSize = XRCCTRL(*parent, "brushSize", wxSlider)->GetValue() / 1000.0f;
	float slideStr = XRCCTRL(*parent, "brushStr", wxSlider)->GetValue() / 1000.0f;
	float slideFocus = XRCCTRL(*parent, "brushFocus", wxSlider)->GetValue() / 1000.0f;
	float slideSpace = XRCCTRL(*parent, "brushSpace", wxSlider)->GetValue() / 1000.0f;

	wxString valSizeStr = wxString::Format("%0.3f", slideSize);
	wxString valStrengthStr = wxString::Format("%0.3f", slideStr);
	wxString valFocusStr = wxString::Format("%0.3f", slideFocus);
	wxString valSpacingStr = wxString::Format("%0.3f", slideSpace);

	valSize->SetLabel(valSizeStr);
	valStrength->SetLabel(valStrengthStr);
	valFocus->SetLabel(valFocusStr);
	valSpacing->SetLabel(valSpacingStr);

	if (brush) {
		glView->SetBrushSize(slideSize);
		brush->setStrength(slideStr);
		brush->setFocus(slideFocus);
		brush->setSpacing(slideSpace);
		CheckBrushBounds();
	}
}

void OutfitStudioFrame::OnSmoothNormalSeams(wxCommandEvent& WXUNUSED(event)) {
	glView->ToggleNormalSeamSmoothMode();

	for (auto &s : selectedItems)
		project->activeSet.ToggleSmoothSeamNormals(s->shapeName);

	glView->Render();
}

void OutfitStudioFrame::OnLockNormals(wxCommandEvent& WXUNUSED(event)) {
	glView->ToggleLockNormalsMode();

	for (auto &s : selectedItems)
		project->activeSet.ToggleLockNormals(s->shapeName);
}


wxBEGIN_EVENT_TABLE(wxGLPanel, wxGLCanvas)
	EVT_PAINT(wxGLPanel::OnPaint)
	EVT_SIZE(wxGLPanel::OnSize)
	EVT_MOUSEWHEEL(wxGLPanel::OnMouseWheel)
	EVT_MOTION(wxGLPanel::OnMouseMove)
	EVT_LEFT_DOWN(wxGLPanel::OnLeftDown)
	EVT_LEFT_DCLICK(wxGLPanel::OnLeftDown)
	EVT_LEFT_UP(wxGLPanel::OnLeftUp)
	EVT_MIDDLE_DOWN(wxGLPanel::OnMiddleDown)
	EVT_MIDDLE_UP(wxGLPanel::OnMiddleUp)
	EVT_RIGHT_DOWN(wxGLPanel::OnRightDown)
	EVT_RIGHT_UP(wxGLPanel::OnRightUp)
	EVT_CHAR_HOOK(wxGLPanel::OnKeys)
	EVT_IDLE(wxGLPanel::OnIdle)
	EVT_MOUSE_CAPTURE_LOST(wxGLPanel::OnCaptureLost)
wxEND_EVENT_TABLE()

wxGLPanel::wxGLPanel(wxWindow* parent, const wxSize& size, const wxGLAttributes& attribs)
	: wxGLCanvas(parent, attribs, wxID_ANY, wxDefaultPosition, size, wxFULL_REPAINT_ON_RESIZE) {

	context = std::make_unique<wxGLContext>(this, nullptr, &GLSurface::GetGLContextAttribs());
	rbuttonDown = false;
	lbuttonDown = false;
	mbuttonDown = false;
	isLDragging = false;
	isRDragging = false;
	isMDragging = false;

	lastX = 0;
	lastY = 0;

	brushSize = 0.45f;
	activeBrush = nullptr;
	editMode = false;
	transformMode = false;
	vertexEdit = false;
	segmentMode = false;
	bMaskPaint = false;
	bWeightPaint = false;
	isPainting = false;
	isTransforming = false;
	isMovingPivot = false;
	isSelecting = false;
	bXMirror = true;
	bConnectedEdit = false;
	bGlobalBrushCollision = true;

	lastCenterDistance = 0.0f;

	strokeManager = &baseStrokes;
}

wxGLPanel::~wxGLPanel() {
	Cleanup();
	gls.RenderOneFrame();
}

void wxGLPanel::OnShown() {
	if (!context->IsOK()) {
		wxLogError("Outfit Studio: OpenGL context is not OK.");
		wxMessageBox(_("Outfit Studio: OpenGL context is not OK."), _("OpenGL Error"), wxICON_ERROR, os);
	}

	gls.Initialize(this, context.get());
	auto size = GetSize();
	gls.SetStartingView(Vector3(0.0f, -5.0f, -15.0f), Vector3(15.0f, 0.0f, 0.0f), size.GetWidth(), size.GetHeight());
	gls.SetMaskVisible();

	int ambient = Config.GetIntValue("Lights/Ambient");
	int frontal = Config.GetIntValue("Lights/Frontal");

	int directional0 = Config.GetIntValue("Lights/Directional0");
	int directional0X = Config.GetIntValue("Lights/Directional0.x");
	int directional0Y = Config.GetIntValue("Lights/Directional0.y");
	int directional0Z = Config.GetIntValue("Lights/Directional0.z");

	int directional1 = Config.GetIntValue("Lights/Directional1");
	int directional1X = Config.GetIntValue("Lights/Directional1.x");
	int directional1Y = Config.GetIntValue("Lights/Directional1.y");
	int directional1Z = Config.GetIntValue("Lights/Directional1.z");

	int directional2 = Config.GetIntValue("Lights/Directional2");
	int directional2X = Config.GetIntValue("Lights/Directional2.x");
	int directional2Y = Config.GetIntValue("Lights/Directional2.y");
	int directional2Z = Config.GetIntValue("Lights/Directional2.z");

	Vector3 directional0Dir = Vector3(directional0X / 100.0f, directional0Y / 100.0f, directional0Z / 100.0f);
	Vector3 directional1Dir = Vector3(directional1X / 100.0f, directional1Y / 100.0f, directional1Z / 100.0f);
	Vector3 directional2Dir = Vector3(directional2X / 100.0f, directional2Y / 100.0f, directional2Z / 100.0f);

	UpdateLights(ambient, frontal, directional0, directional1, directional2, directional0Dir, directional1Dir, directional2Dir);

	if (Config.Exists("Rendering/ColorBackground")) {
		int colorBackgroundR = Config.GetIntValue("Rendering/ColorBackground.r");
		int colorBackgroundG = Config.GetIntValue("Rendering/ColorBackground.g");
		int colorBackgroundB = Config.GetIntValue("Rendering/ColorBackground.b");
		gls.SetBackgroundColor(Vector3(colorBackgroundR / 255.0f, colorBackgroundG / 255.0f, colorBackgroundB / 255.0f));
	}

	os->MeshesFromProj();
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	os = dynamic_cast<OutfitStudioFrame*>(win);
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, const std::string& shapeName) {
	std::vector<std::string> shapeList = nif->GetShapeNames();

	for (int i = 0; i < shapeList.size(); i++) {
		mesh* m = nullptr;
		if (!shapeName.empty() && (shapeList[i] == shapeName))
			m = gls.AddMeshFromNif(nif, shapeList[i]);
		else if (!shapeName.empty())
			continue;
		else
			m = gls.AddMeshFromNif(nif, shapeList[i]);

		if (m) {
			m->BuildTriAdjacency();
			m->BuildEdgeList();
			m->ColorFill(Vector3());

			if (extInitialized)
				m->CreateBuffers();
		}
	}
}

void wxGLPanel::SetMeshTextures(const std::string& shapeName, const std::vector<std::string>& textureFiles, const bool hasMatFile, const MaterialFile& matFile, const bool reloadTextures) {
	mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;
	
	std::string vShader = "res\\shaders\\default.vert";
	std::string fShader = "res\\shaders\\default.frag";

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR) {
		vShader = "res\\shaders\\fo4_default.vert";
		fShader = "res\\shaders\\fo4_default.frag";
	}

	GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader, reloadTextures);
	if (mat) {
		m->material = mat;
		
		if (hasMatFile)
			m->UpdateFromMaterialFile(matFile);

		gls.UpdateShaders(m);
	}
}

void wxGLPanel::UpdateMeshVertices(const std::string& shapeName, std::vector<Vector3>* verts, bool updateBVH, bool recalcNormals, bool render, std::vector<Vector2>* uvs) {
	int id = gls.GetMeshID(shapeName);
	gls.Update(id, verts, uvs);

	if (updateBVH)
		BVHUpdateQueue.insert(id);

	if (recalcNormals)
		RecalcNormals(shapeName);

	if (render)
		gls.RenderOneFrame();
}

void wxGLPanel::RecalculateMeshBVH(const std::string& shapeName) {
	gls.RecalculateMeshBVH(gls.GetMeshID(shapeName));
}

void wxGLPanel::ShowShape(const std::string& shapeName, bool show) {
	int id = gls.GetMeshID(shapeName);
	gls.SetMeshVisibility(id, show);
	gls.RenderOneFrame();
}

void wxGLPanel::SetActiveShapes(const std::vector<std::string>& shapeNames) {
	gls.SetActiveMeshesID(shapeNames);
}

void wxGLPanel::SetSelectedShape(const std::string& shapeName) {
	gls.SetSelectedMesh(shapeName);
}

void wxGLPanel::SetActiveBrush(int brushID) {
	bMaskPaint = false;
	bWeightPaint = false;

	switch (brushID) {
	case -1:
		activeBrush = nullptr;
		break;
	case 0:
		activeBrush = &maskBrush;
		bMaskPaint = true;
		break;
	case 1:
		activeBrush = &standardBrush;
		break;
	case 2:
		activeBrush = &deflateBrush;
		break;
	case 3:
		activeBrush = &moveBrush;
		break;
	case 4:
		activeBrush = &smoothBrush;
		break;
	case 10:
		activeBrush = &weightBrush;
		bWeightPaint = true;
		break;
	}
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	if (event.GetUnicodeKey() == 'V') {
		wxDialog dlg;
		wxPoint cursorPos(event.GetPosition());

		std::unordered_map<ushort, Vector3> diff;
		std::vector<Vector3> verts;
		Vector3 newPos;
		int vertIndex;

		if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex))
			return;

		if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
			os->project->GetLiveVerts(os->activeItem->shapeName, verts);
			XRCCTRL(dlg, "posX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].x));
			XRCCTRL(dlg, "posY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].y));
			XRCCTRL(dlg, "posZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", verts[vertIndex].z));

			if (dlg.ShowModal() == wxID_OK) {
				newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
				newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
				newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

				if (os->bEditSlider) {
					diff[vertIndex] = newPos - verts[vertIndex];
					float diffY = diff[vertIndex].y / 10.0f;
					float diffZ = diff[vertIndex].z / 10.0f;
					diff[vertIndex].z = diffY;
					diff[vertIndex].y = diffZ;
					verts[vertIndex] = newPos;
					os->project->UpdateMorphResult(os->activeItem->shapeName, os->activeSlider, diff);
				}
				else {
					os->project->MoveVertex(os->activeItem->shapeName, newPos, vertIndex);
				}
				os->project->GetLiveVerts(os->activeItem->shapeName, verts);
				UpdateMeshVertices(os->activeItem->shapeName, &verts);
			}

			if (transformMode)
				ShowTransformTool();
			if (vertexEdit)
				ShowVertexEdit();
		}
	}
	else if (event.GetKeyCode() == WXK_SPACE)
		os->ToggleBrushPane();

	event.Skip();
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	Vector3 o;
	Vector3 n;
	Vector3 d;
	Vector3 s;

	TweakPickInfo tpi;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, nullptr, bGlobalBrushCollision, &tpi.facet);
	if (!hit)
		return false;

	if (!os->NotifyStrokeStarting())
		return false;

	if (bXMirror) {
		if (!gls.CollideMeshes(screenPos.x, screenPos.y, o, n, true, nullptr, bGlobalBrushCollision, &tpi.facetM))
			tpi.facetM = -1;
	}

	n.Normalize();

	Vector3 v;
	Vector3 vo;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

	v = v * -1.0f;
	tpi.view = v;

	savedBrush = activeBrush;

	if (wxGetKeyState(WXK_CONTROL)) {
		if (wxGetKeyState(WXK_ALT) && !segmentMode) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		if (wxGetKeyState(WXK_ALT)) {
			unweightBrush.refBone = os->GetActiveBone();
			unweightBrush.setStrength(-weightBrush.getStrength());
			activeBrush = &unweightBrush;
		}
		else if (wxGetKeyState(WXK_SHIFT)) {
			smoothWeightBrush.refBone = os->GetActiveBone();
			smoothWeightBrush.setStrength(weightBrush.getStrength() * 15.0f);
			activeBrush = &smoothWeightBrush;
		}
		else {
			weightBrush.refBone = os->GetActiveBone();
		}
	}
	else if (wxGetKeyState(WXK_ALT) && !segmentMode) {
		if (activeBrush == &standardBrush) {
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			activeBrush = &standardBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			activeBrush = &UnMaskBrush;
		}
	}
	else if (activeBrush == &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothMaskBrush.setStrength(maskBrush.getStrength() * 15.0f);
		activeBrush = &smoothMaskBrush;
	}
	else if (activeBrush != &weightBrush && activeBrush != &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		activeBrush = &smoothBrush;
	}

	if (activeBrush->Type() == TBT_WEIGHT) {
		for (auto &sel : os->GetSelectedItems()) {
			int boneIndex = os->project->GetWorkAnim()->GetShapeBoneIndex(sel->shapeName, os->GetActiveBone());
			if (boneIndex < 0)
				os->project->AddBoneRef(os->GetActiveBone());
		}
	}

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMeshes(), activeBrush);

	activeBrush->setConnected(bConnectedEdit);
	activeBrush->setMirror(bXMirror);
	activeBrush->setRadius(brushSize);
	activeStroke->beginStroke(tpi);

	if (activeBrush->Type() != TBT_MOVE)
		activeStroke->updateStroke(tpi);

	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	Vector3 o;
	Vector3 n;
	Vector3 d;	// Mirror pick ray direction.
	Vector3 s;	// Mirror pick ray origin.

	TweakPickInfo tpi;

	if (activeStroke) {
		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision);
		gls.RenderOneFrame();

		if (activeBrush->Type() == TBT_MOVE) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			if (!hit)
				return;

			gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, nullptr, bGlobalBrushCollision, &tpi.facet);
			if (bXMirror) {
				if (!gls.CollideMeshes(screenPos.x, screenPos.y, o, n, true, nullptr, bGlobalBrushCollision, &tpi.facetM))
					tpi.facetM = -1;
			}
			tpi.normal.Normalize();
		}

		Vector3 v;
		Vector3 vo;
		gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

		v = v * -1.0f;
		tpi.view = v;
		activeStroke->updateStroke(tpi);

		if (activeBrush->Type() == TBT_WEIGHT) {
			std::string selectedBone = os->GetActiveBone();
			if (!selectedBone.empty()) {
				os->ActiveShapesUpdated(strokeManager->GetCurStateStroke(), false);

				int boneScalePos = os->boneScale->GetValue();
				if (boneScalePos != 0)
					os->project->ApplyBoneScale(selectedBone, boneScalePos);
			}
		}

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment(nullptr, true);
			os->ShowPartition(nullptr, true);
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (activeStroke) {
		activeStroke->endStroke();

		if (activeStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());

			if (activeStroke->BrushType() == TBT_WEIGHT) {
				std::string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					int boneScalePos = os->boneScale->GetValue();
					os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
				}
			}

			if (!os->bEditSlider && activeStroke->BrushType() != TBT_WEIGHT) {
				for (auto &s : os->project->GetWorkNif()->GetShapeNames()) {
					os->UpdateShapeSource(s);
					os->project->RefreshMorphShape(s);
				}
			}
		}

		activeStroke = nullptr;
		activeBrush = savedBrush;

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment(nullptr, true);
			os->ShowPartition(nullptr, true);
		}
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh, &tpi.facet);
	if (!hit)
		return false;

	tpi.center = xformCenter;

	std::string mname = hitMesh->shapeName;
	if (mname.find("Move") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
		case 'X':
			tpi.view = Vector3(1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = Vector3(0.0f, 0.0f, 1.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		}
	}
	else if (mname.find("Rotate") != std::string::npos) {
		translateBrush.SetXFormType(1);
		switch (mname[0]) {
		case 'X':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), -tpi.center.x);
			//tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		case 'Y':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), -tpi.center.y);
			//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 1.0f, 0.0f);
			break;
		case 'Z':
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), -tpi.center.z);
			//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		}
	}
	else if (mname.find("Scale") != std::string::npos) {
		if (mname.find("Uniform") == std::string::npos) {
			translateBrush.SetXFormType(2);
			switch (mname[0]) {
			case 'X':
				tpi.view = Vector3(1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Y':
				tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
				break;
			case 'Z':
				tpi.view = Vector3(0.0f, 0.0f, 1.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
			}
		}
		else {
			translateBrush.SetXFormType(3);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), -tpi.center.x);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), -tpi.center.y);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), -tpi.center.z);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	else
		return false;

	activeStroke = strokeManager->CreateStroke(gls.GetActiveMeshes(), &translateBrush);

	activeStroke->beginStroke(tpi);

	XMoveMesh->bVisible = false;
	YMoveMesh->bVisible = false;
	ZMoveMesh->bVisible = false;
	XRotateMesh->bVisible = false;
	YRotateMesh->bVisible = false;
	ZRotateMesh->bVisible = false;
	XScaleMesh->bVisible = false;
	YScaleMesh->bVisible = false;
	ZScaleMesh->bVisible = false;
	ScaleUniformMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdateTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	translateBrush.GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, -pd);

	activeStroke->updateStroke(tpi);

	ShowTransformTool();
}

void wxGLPanel::EndTransform() {
	activeStroke->endStroke();
	activeStroke = nullptr;

	os->ActiveShapesUpdated(strokeManager->GetCurStateStroke());
	if (!os->bEditSlider) {
		for (auto &s : os->project->GetWorkNif()->GetShapeNames()) {
			os->UpdateShapeSource(s);
			os->project->RefreshMorphShape(s);
		}
	}

	ShowTransformTool();
}

bool wxGLPanel::StartPivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh, &tpi.facet);
	if (!hit)
		return false;

	tpi.center = pivotPosition;

	std::string mname = hitMesh->shapeName;
	if (mname.find("PivotMesh") != std::string::npos) {
		translateBrush.SetXFormType(0);
		switch (mname[0]) {
		case 'X':
			tpi.view = Vector3(1.0f, 0.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Y':
			tpi.view = Vector3(0.0f, 1.0f, 0.0f);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
			break;
		case 'Z':
			tpi.view = Vector3(0.0f, 0.0f, 1.0f);
			tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
			break;
		}
	}
	else
		return false;

	std::vector<mesh*> strokeMeshes{ hitMesh };
	activeStroke = strokeManager->CreateStroke(strokeMeshes, &translateBrush);
	activeStroke->beginStroke(tpi);

	XPivotMesh->bVisible = false;
	YPivotMesh->bVisible = false;
	ZPivotMesh->bVisible = false;
	PivotCenterMesh->bVisible = false;
	hitMesh->bVisible = true;
	return true;
}

void wxGLPanel::UpdatePivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Vector3 pn;
	float pd;

	translateBrush.GetWorkingPlane(pn, pd);
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, -pd);

	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndPivotPosition() {
	activeStroke->endStroke();

	std::vector<mesh*> refMeshes = activeStroke->GetRefMeshes();
	if (refMeshes.size() > 0) {
		mesh* pivotHitMesh = refMeshes[0];
		auto pivotStartState = activeStroke->pointStartState.find(pivotHitMesh);
		auto pivotEndState = activeStroke->pointEndState.find(pivotHitMesh);

		if (pivotStartState != activeStroke->pointStartState.end() && pivotEndState != activeStroke->pointEndState.end()) {
			if (pivotStartState->second.size() > 0 && pivotEndState->second.size() > 0) {
				Vector3& pivotStartStatePos = pivotStartState->second[0];
				Vector3& pivotEndStatePos = pivotEndState->second[0];
				Vector3 pivotDiff = pivotEndStatePos - pivotStartStatePos;
				pivotPosition += pivotDiff;
			}
		}
	}

	activeStroke = nullptr;
	ShowPivot();
}

bool wxGLPanel::SelectVertex(const wxPoint& screenPos) {
	int vertIndex;
	if (!gls.GetCursorVertex(screenPos.x, screenPos.y, &vertIndex))
		return false;

	if (os->activeItem) {
		mesh* m = GetMesh(os->activeItem->shapeName);
		if (m) {
			if (wxGetKeyState(WXK_CONTROL))
				m->vcolors[vertIndex].x = 1.0f;
			else if (!segmentMode)
				m->vcolors[vertIndex].x = 0.0f;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}
	}

	if (transformMode)
		ShowTransformTool();

	return true;
}

bool wxGLPanel::UndoStroke() {
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();
	bool ret = strokeManager->backStroke(gls.GetActiveMeshes());

	if (ret && curStroke) {
		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke, true);

			if (!os->bEditSlider && curStroke->BrushType() != TBT_WEIGHT) {
				std::vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}
		}

		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				std::string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = os->boneScale->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
			}
		}
	}

	if (transformMode)
		ShowTransformTool();
	else
		Render();

	return ret;
}

bool wxGLPanel::RedoStroke() {
	bool ret = strokeManager->forwardStroke(gls.GetActiveMeshes());
	TweakStroke* curStroke = strokeManager->GetCurStateStroke();

	if (ret && curStroke) {
		if (curStroke->BrushType() != TBT_MASK) {
			os->ActiveShapesUpdated(curStroke);

			if (!os->bEditSlider && curStroke->BrushType() != TBT_WEIGHT) {
				std::vector<mesh*> refMeshes = curStroke->GetRefMeshes();
				for (auto &m : refMeshes)
					os->project->UpdateShapeFromMesh(m->shapeName, m);
			}
		}

		if (curStroke->BrushType() == TBT_WEIGHT) {
			wxArrayTreeItemIds selItems;
			os->outfitBones->GetSelections(selItems);
			if (selItems.size() > 0) {
				std::string selectedBone = os->outfitBones->GetItemText(selItems.front());
				int boneScalePos = os->boneScale->GetValue();
				os->project->ApplyBoneScale(selectedBone, boneScalePos, true);
			}
		}
	}

	if (transformMode)
		ShowTransformTool();
	else
		Render();

	return ret;
}

void wxGLPanel::ShowTransformTool(bool show) {
	if (pivotMode)
		xformCenter = pivotPosition;
	else
		xformCenter = gls.GetActiveCenter();

	if (show) {
		XMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		YMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		ZMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");

		XRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 1.25f, 0.04f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		YRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 1.25f, 0.04f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		ZRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 1.25f, 0.04f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");

		XScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.75f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), 0.12f, Vector3(1.0f, 0.0f, 0.0f), "XScaleMesh");
		YScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.75f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), 0.12f, Vector3(0.0f, 1.0f, 0.0f), "YScaleMesh");
		ZScaleMesh = gls.AddVis3dCube(xformCenter + Vector3(0.0f, 0.0f, 0.75f), Vector3(0.0f, 0.0f, 1.0f), 0.12f, Vector3(0.0f, 0.0f, 1.0f), "ZScaleMesh");
		ScaleUniformMesh = gls.AddVis3dCube(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.15f, Vector3(0.0f, 0.0f, 0.0f), "ScaleUniformMesh");

		lastCenterDistance = 0.0f;

		XMoveMesh->bVisible = true;
		YMoveMesh->bVisible = true;
		ZMoveMesh->bVisible = true;
		XRotateMesh->bVisible = true;
		YRotateMesh->bVisible = true;
		ZRotateMesh->bVisible = true;
		XScaleMesh->bVisible = true;
		YScaleMesh->bVisible = true;
		ZScaleMesh->bVisible = true;
		ScaleUniformMesh->bVisible = true;

		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}
	}
	else {
		if (XMoveMesh) {
			XMoveMesh->bVisible = false;
			YMoveMesh->bVisible = false;
			ZMoveMesh->bVisible = false;
			XRotateMesh->bVisible = false;
			YRotateMesh->bVisible = false;
			ZRotateMesh->bVisible = false;
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
		}

		if (XPivotMesh && pivotMode) {
			XPivotMesh->bVisible = true;
			YPivotMesh->bVisible = true;
			ZPivotMesh->bVisible = true;
			PivotCenterMesh->bVisible = true;
		}
	}

	UpdateTransformTool();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdateTransformTool() {
	if (!transformMode)
		return;

	if (!XMoveMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterDistance != 0.0f) {
		float factor = 1.0f / lastCenterDistance;
		XMoveMesh->ScaleVertices(xformCenter, factor);
		YMoveMesh->ScaleVertices(xformCenter, factor);
		ZMoveMesh->ScaleVertices(xformCenter, factor);

		XRotateMesh->ScaleVertices(xformCenter, factor);
		YRotateMesh->ScaleVertices(xformCenter, factor);
		ZRotateMesh->ScaleVertices(xformCenter, factor);

		XScaleMesh->ScaleVertices(xformCenter, factor);
		YScaleMesh->ScaleVertices(xformCenter, factor);
		ZScaleMesh->ScaleVertices(xformCenter, factor);
		ScaleUniformMesh->ScaleVertices(xformCenter, factor);
	}

	lastCenterDistance = unprojected.DistanceTo(xformCenter) / 15.0f;

	XMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZMoveMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZRotateMesh->ScaleVertices(xformCenter, lastCenterDistance);

	XScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	YScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ZScaleMesh->ScaleVertices(xformCenter, lastCenterDistance);
	ScaleUniformMesh->ScaleVertices(xformCenter, lastCenterDistance);
}

void wxGLPanel::ShowPivot(bool show) {
	if (show) {
		XPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(1.0f, 0.0f, 0.0f), "XPivotMesh");
		YPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 1.0f, 0.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 1.0f, 0.0f), "YPivotMesh");
		ZPivotMesh = gls.AddVis3dArrow(pivotPosition, Vector3(0.0f, 0.0f, 1.0f), 0.03f, 0.1f, 1.0f, Vector3(0.0f, 0.0f, 1.0f), "ZPivotMesh");
		PivotCenterMesh = gls.AddVis3dCube(pivotPosition, Vector3(1.0f, 0.0f, 0.0f), 0.075f, Vector3(0.0f, 0.0f, 0.0f), "PivotCenterMesh");

		XPivotMesh->bVisible = true;
		YPivotMesh->bVisible = true;
		ZPivotMesh->bVisible = true;
		PivotCenterMesh->bVisible = true;

		lastCenterPivotDistance = 0.0f;
	}
	else {
		if (XPivotMesh) {
			XPivotMesh->bVisible = false;
			YPivotMesh->bVisible = false;
			ZPivotMesh->bVisible = false;
			PivotCenterMesh->bVisible = false;
		}
	}

	UpdatePivot();
	gls.RenderOneFrame();
}

void wxGLPanel::UpdatePivot() {
	if (!pivotMode)
		return;

	if (!XPivotMesh)
		return;

	Vector3 unprojected;
	gls.UnprojectCamera(unprojected);

	if (lastCenterPivotDistance != 0.0f) {
		float factor = 1.0f / lastCenterPivotDistance;
		XPivotMesh->ScaleVertices(pivotPosition, factor);
		YPivotMesh->ScaleVertices(pivotPosition, factor);
		ZPivotMesh->ScaleVertices(pivotPosition, factor);
		PivotCenterMesh->ScaleVertices(pivotPosition, factor);
	}

	lastCenterPivotDistance = unprojected.DistanceTo(pivotPosition) / 15.0f;

	XPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	YPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	ZPivotMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
	PivotCenterMesh->ScaleVertices(pivotPosition, lastCenterPivotDistance);
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto &m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			mesh* m = GetMesh(os->activeItem->shapeName);
			if (m) {
				m->bShowPoints = true;
				m->QueueUpdate(mesh::UpdateType::VertexColors);
			}
		}
	}

	gls.RenderOneFrame();
}

void wxGLPanel::OnIdle(wxIdleEvent& WXUNUSED(event)) {
	for (auto &it : BVHUpdateQueue) {
		gls.RecalculateMeshBVH(it);
	}
	BVHUpdateQueue.clear();
}

void wxGLPanel::OnPaint(wxPaintEvent& event) {
	// Initialize OpenGL the first time the window is painted.
	// We unfortunately can't initialize it before the window is shown.
	// We could register for the EVT_SHOW event, but unfortunately it
	// appears to only be called after the first few EVT_PAINT events.
	// It also isn't supported on all platforms.
	if (firstPaint) {
		firstPaint = false;
		OnShown();
	}

	gls.RenderOneFrame();
	event.Skip();
}

void wxGLPanel::OnSize(wxSizeEvent& event) {
	wxSize sz = event.GetSize();
	gls.SetSize(sz.GetX(), sz.GetY());
	gls.RenderOneFrame();
}

void wxGLPanel::OnMouseWheel(wxMouseEvent& event) {
	if (wxGetKeyState(wxKeyCode('S')))  {
		wxPoint p = event.GetPosition();
		int delt = event.GetWheelRotation();

		if (editMode) {
			// Adjust brush size
			if (delt < 0)
				DecBrush();
			else
				IncBrush();

			os->CheckBrushBounds();
			os->UpdateBrushPane();
			gls.UpdateCursor(p.x, p.y, bGlobalBrushCollision);
		}
	}
	else {
		int delt = event.GetWheelRotation();
		gls.DollyCamera(delt);
		UpdateTransformTool();
		UpdatePivot();
	}

	gls.RenderOneFrame();
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	if (os->IsActive())
		SetFocus();

	bool cursorExists = false;
	int x;
	int y;
	int t = 0;
	float w = 0.0f;
	float m = 0.0f;
	event.GetPosition(&x, &y);

	if (mbuttonDown) {
		isMDragging = true;
		if (wxGetKeyState(wxKeyCode::WXK_SHIFT))
			gls.DollyCamera(y - lastY);
		else
			gls.PanCamera(x - lastX, y - lastY);

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (rbuttonDown) {
		isRDragging = true;
		if (wxGetKeyState(WXK_SHIFT)) {
			gls.PanCamera(x - lastX, y - lastY);
		}
		else {
			gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
		}

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (lbuttonDown) {
		isLDragging = true;
		if (isTransforming) {
			UpdateTransform(event.GetPosition());
		}
		else if (isMovingPivot) {
			UpdatePivotPosition(event.GetPosition());
		}
		else if (isPainting) {
			UpdateBrushStroke(event.GetPosition());
		}
		else if (isSelecting) {
			SelectVertex(event.GetPosition());
		}
		else {
			if (Config.MatchValue("Input/LeftMousePan", "true")) {
				gls.PanCamera(x - lastX, y - lastY);
				UpdateTransformTool();
				UpdatePivot();
			}
		}

		gls.RenderOneFrame();
	}

	if (!rbuttonDown && !lbuttonDown) {
		std::string hitMeshName;
		if (editMode) {
			cursorExists = gls.UpdateCursor(x, y, bGlobalBrushCollision, &hitMeshName, &t, &w, &m);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if ((transformMode || pivotMode) && !isTransforming && !isMovingPivot) {
			if (XMoveMesh) {
				XMoveMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YMoveMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZMoveMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XRotateMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YRotateMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZRotateMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				XScaleMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YScaleMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZScaleMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				ScaleUniformMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			if (XPivotMesh) {
				XPivotMesh->color = Vector3(1.0f, 0.0f, 0.0f);
				YPivotMesh->color = Vector3(0.0f, 1.0f, 0.0f);
				ZPivotMesh->color = Vector3(0.0f, 0.0f, 1.0f);
				PivotCenterMesh->color = Vector3(0.0f, 0.0f, 0.0f);
			}

			Vector3 outOrigin, outNormal;
			mesh* hitMesh = nullptr;
			if (gls.CollideOverlay(x, y, outOrigin, outNormal, &hitMesh)) {
				if (hitMesh && hitMesh != PivotCenterMesh) {
					hitMesh->color = Vector3(1.0f, 1.0f, 0.0f);
					gls.ShowCursor(false);
				}
			}
		}

		gls.RenderOneFrame();

		if (os->statusBar) {
			if (cursorExists) {
				if (bWeightPaint)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g", t, w), 1);
				else if (bMaskPaint)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", t, m), 1);
				else {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(hitMeshName, verts);
					if (verts.size() > t)
						os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", t, verts[t].x, verts[t].y, verts[t].z), 1);
				}
			}
			else {
				os->statusBar->SetStatusText("", 1);
			}
		}
	}

	lastX = x;
	lastY = y;
}

void wxGLPanel::OnLeftDown(wxMouseEvent& event) {
	if (!HasCapture())
		CaptureMouse();

	lbuttonDown = true;

	if (transformMode) {
		bool meshHit = StartTransform(event.GetPosition());
		if (meshHit) {
			isTransforming = true;
			return;
		}
	}

	if (pivotMode) {
		bool meshHit = StartPivotPosition(event.GetPosition());
		if (meshHit) {
			isMovingPivot = true;
			return;
		}
	}

	if (editMode) {
		bool meshHit = StartBrushStroke(event.GetPosition());
		if (meshHit)
			isPainting = true;
	}
	else if (vertexEdit) {
		bool meshHit = SelectVertex(event.GetPosition());
		if (meshHit)
			isSelecting = true;
	}
}

void wxGLPanel::OnMiddleDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	mbuttonDown = true;
}

void wxGLPanel::OnMiddleUp(wxMouseEvent& WXUNUSED(event)) {
	if (GetCapture() == this)
		ReleaseMouse();

	isMDragging = false;
	mbuttonDown = false;
}

void wxGLPanel::OnLeftUp(wxMouseEvent& event) {
	if (GetCapture() == this)
		ReleaseMouse();

	if (!isLDragging && !isPainting && !activeBrush) {
		int x, y;
		event.GetPosition(&x, &y);
		wxPoint p = event.GetPosition();

		int meshID = gls.PickMesh(x, y);
		if (meshID > -1)
			os->SelectShape(gls.GetMeshName(meshID));
	}

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isLDragging = false;
	lbuttonDown = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event)) {
	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isTransforming) {
		EndTransform();
		isTransforming = false;
	}

	if (isMovingPivot) {
		EndPivotPosition();
		isMovingPivot = false;
	}

	if (isSelecting)
		isSelecting = false;

	isLDragging = false;
	lbuttonDown = false;

	isMDragging = false;
	mbuttonDown = false;

	rbuttonDown = false;

	gls.RenderOneFrame();
}

void wxGLPanel::OnRightDown(wxMouseEvent& WXUNUSED(event)) {
	if (!HasCapture())
		CaptureMouse();

	rbuttonDown = true;
}

void wxGLPanel::OnRightUp(wxMouseEvent& WXUNUSED(event)) {
	if (HasCapture())
		ReleaseMouse();

	rbuttonDown = false;
}


bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner) {
		std::string mergeShapeName = "";
		if (owner->activeItem && fileNames.GetCount() == 1)
			mergeShapeName = owner->activeItem->shapeName;

		for (auto &inputFile : fileNames) {
			wxString dataName = inputFile.AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			if (inputFile.Lower().EndsWith(".nif")) {
				owner->StartProgress(_("Adding NIF file..."));
				owner->UpdateProgress(1, _("Adding NIF file..."));
				owner->project->ImportNIF(inputFile.ToUTF8().data(), false);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".obj")) {
				owner->StartProgress("Adding OBJ file...");
				owner->UpdateProgress(1, _("Adding OBJ file..."));
				owner->project->ImportOBJ(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShapeName);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".fbx")) {
				owner->StartProgress(_("Adding FBX file..."));
				owner->UpdateProgress(1, _("Adding FBX file..."));
				owner->project->ImportFBX(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShapeName);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
		}
	}
	else
		return false;

	return true;
}

bool DnDSliderFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner) {
		bool isMultiple = (fileNames.GetCount() > 1);
		for (int i = 0; i < fileNames.GetCount(); i++)	{
			wxString inputFile;
			inputFile = fileNames.Item(i);

			wxString dataName = inputFile.AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			bool isBSD = inputFile.MakeLower().EndsWith(".bsd");
			bool isOBJ = inputFile.MakeLower().EndsWith(".obj");
			bool isFBX = inputFile.MakeLower().EndsWith(".fbx");
			if (isBSD || isOBJ) {
				if (!owner->activeItem) {
					wxMessageBox(_("There is no shape selected!"), _("Error"));
					return false;
				}

				if (lastResult == wxDragCopy) {
					targetSlider = owner->NewSlider(dataName.ToUTF8().data(), isMultiple);
				}

				if (targetSlider.empty())
					return false;

				owner->StartProgress(_("Loading slider file..."));
				owner->UpdateProgress(1, _("Loading slider file..."));

				if (isBSD)
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->shapeName, inputFile.ToUTF8().data());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->shapeName, inputFile.ToUTF8().data());
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->shapeName, inputFile.ToUTF8().data());
				else
					return false;


				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
		}
		owner->EnterSliderEdit(targetSlider);
		targetSlider.clear();
	}
	else
		return false;

	return true;
}

wxDragResult DnDSliderFile::OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult) {
	targetSlider.clear();
	lastResult = defResult;

	if (defResult == wxDragCopy)
		return lastResult;

	if (owner) {
		for (auto &child : owner->sliderDisplays) {
			if (child.second->sliderPane->GetRect().Contains(x, y)) {
				targetSlider = child.first;
				lastResult = wxDragMove;
				break;
			}
		}

		if (targetSlider.empty())
			if (owner->sliderScroll->HitTest(x, y) == wxHT_WINDOW_INSIDE)
				lastResult = wxDragCopy;
	}
	else
		lastResult = wxDragCancel;

	return lastResult;
}


std::string JoinStrings(const std::vector<std::string>& elements, const char* const separator) {
	switch (elements.size()) {
	case 0:
		return "";
	case 1:
		return elements[0];
	default:
		std::ostringstream os;
		std::copy(elements.begin(), elements.end() - 1, std::ostream_iterator<std::string>(os, separator));
		os << *elements.rbegin();
		return os.str();
	}
}
