/*
BodySlide and Outfit Studio

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
#include "../components/SliderGroup.h"
#include "../components/SliderPresets.h"
#include "../files/HkxFile.h"
#include "../files/MaskFile.h"
#include "../files/TriFile.h"
#include "../files/SFMorphFile.h"
#include "../ui/wxBrushSettingsPopup.h"
#include "../utils/ConfigDialogUtil.h"
#include "../utils/PlatformUtil.h"
#include "EditUV.h"
#include "GroupManager.h"
#include "PhysicsEditorDialog.h"
#include "PresetSaveDialog.h"
#include "PartitionTypeChoices.h"
#include "ShapeProperties.h"
#include "SliderDataDialog.h"
#include "AddProjectDialog.h"
#include "SliderDataImportDialog.h"
#include "AutomationDialog.h"
#include "../components/ClippingFixer.h"
#include "../utils/ProjectUtil.h"
#include "../utils/GameUtil.h"
#include "../utils/StackTrace.h"
#include "../utils/StringStuff.h"
#include "../utils/SettingsDialogShared.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>
#include <wx/debugrpt.h>
#include <wx/display.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "ConvertBodyReferenceDialog.h"

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>

#include <mmsystem.h>
#endif

using namespace nifly;

namespace {

// The playback timer only backs OnAnimIdle up for stretches where no idle events
// arrive. It cannot pace playback on its own: on MSW it is a WM_TIMER, which
// Windows rounds up to a multiple of the ~15.6 ms system clock tick, making 15 ms
// the shortest useful request and 64 Hz the hard ceiling - measured, that holds
// even with the clock resolution already raised.
constexpr int AnimPlaybackTimerIntervalMS = 15;

// Windows quantises waits to the system clock tick, ~15.6 ms by default, which
// caps anything paced by sleeping at 64 Hz. Raising the resolution for the
// duration of playback is what lets OnAnimIdle reach the display's refresh rate.
// Since Windows 10 2004 this affects the calling process, not the whole system.
void SetHighResolutionTimers(bool enable) {
#ifdef __WXMSW__
	if (enable)
		timeBeginPeriod(1);
	else
		timeEndPeriod(1);
#else
	(void)enable;
#endif
}

bool GetPoseHkxFormat(TargetGame targetGame, HKX::Format* outFormat = nullptr);

int GetPreferredPoseFileFilterIndex(TargetGame targetGame) {
	switch (targetGame) {
	case SKYRIMSE:
	case SKYRIMVR:
		return 2;
	case FO4:
	case FO4VR:
		return 1;
	default:
		return 0;
	}
}

wxString GetPoseFileExtensionForFilter(int filterIndex) {
	switch (filterIndex) {
	case 0: return "hkx";
	case 1: return "json";
	case 2: return "yaml";
	default: return wxEmptyString;
	}
}

wxString EnsurePoseFileExtension(const wxString& filePath, int filterIndex) {
	wxFileName fn(filePath);
	if (!fn.HasExt()) {
		wxString ext = GetPoseFileExtensionForFilter(filterIndex);
		if (!ext.empty())
			fn.SetExt(ext);
	}
	return fn.GetFullPath();
}

}

// IPC connection handler for Outfit Studio single-instance checking
class OutfitStudioIPCConnection : public wxConnection {
public:
	OutfitStudioIPCConnection() {}

	virtual bool OnExec(const wxString& WXUNUSED(topic), const wxString& data) override {
		// data is expected to be newline-separated file paths
		wxArrayString files;
		wxStringTokenizer tokenizer(data, "\n");
		while (tokenizer.HasMoreTokens()) {
			wxString token = tokenizer.GetNextToken().Trim();
			if (!token.IsEmpty())
				files.Add(token);
		}

		OutfitStudio* app = dynamic_cast<OutfitStudio*>(wxTheApp);
		if (!files.IsEmpty() && app && app->GetTopWindow()) {
			OutfitStudioFrame* frame = dynamic_cast<OutfitStudioFrame*>(app->GetTopWindow());
			if (frame) {
				// Use wxCallAfter to call OpenFiles on the main GUI thread
				wxTheApp->CallAfter([frame, files]() {
					frame->LoadFiles(files);
				});
				return true;
			}
		}

		return false;
	}
};

class OutfitStudioIPCServer : public wxServer {
public:
	virtual wxConnectionBase* OnAcceptConnection(const wxString& WXUNUSED(topic)) override {
		return new OutfitStudioIPCConnection();
	}
};


// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(OutfitStudioFrame, wxFrame)
	EVT_CLOSE(OutfitStudioFrame::OnClose)
	EVT_MENU(XRCID("fileExit"), OutfitStudioFrame::OnExit)
	EVT_MENU(XRCID("fileAbout"), OutfitStudioFrame::OnAbout)

	EVT_MENU(wxID_ANY, OutfitStudioFrame::OnMenuItem)
	EVT_MENU(XRCID("packProjects"), OutfitStudioFrame::OnPackProjects)
	EVT_MENU(XRCID("fileSettings"), OutfitStudioFrame::OnSettings)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudioFrame::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudioFrame::OnLoadProject)
	EVT_MENU(XRCID("btnAddProject"), OutfitStudioFrame::OnAddProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudioFrame::OnLoadReference)
	EVT_MENU(XRCID("fileConvBodyRef"), OutfitStudioFrame::OnConvertBodyReference)
	EVT_MENU(XRCID("menuRunAutomation"), OutfitStudioFrame::OnRunAutomation)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudioFrame::OnLoadOutfit)
	EVT_MENU(XRCID("fileSave"), OutfitStudioFrame::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudioFrame::OnSaveSliderSetAs)
	EVT_MENU(XRCID("fileUnload"), OutfitStudioFrame::OnUnloadProject)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("masksPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COMBOBOX(XRCID("cMaskName"), OutfitStudioFrame::OnSelectMask)
	EVT_BUTTON(XRCID("saveMask"), OutfitStudioFrame::OnSaveMask)
	EVT_BUTTON(XRCID("deleteMask"), OutfitStudioFrame::OnDeleteMask)
	EVT_BUTTON(XRCID("exportMask"), OutfitStudioFrame::OnExportMask)
	EVT_BUTTON(XRCID("importMask"), OutfitStudioFrame::OnImportMask)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("posePane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("physicsPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("animationPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("notesPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_CHOICE(XRCID("cPoseBone"), OutfitStudioFrame::OnPoseBoneChanged)
	EVT_COMMAND_SCROLL(XRCID("rxPoseSlider"), OutfitStudioFrame::OnRXPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("ryPoseSlider"), OutfitStudioFrame::OnRYPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("rzPoseSlider"), OutfitStudioFrame::OnRZPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("txPoseSlider"), OutfitStudioFrame::OnTXPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("tyPoseSlider"), OutfitStudioFrame::OnTYPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("tzPoseSlider"), OutfitStudioFrame::OnTZPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("scPoseSlider"), OutfitStudioFrame::OnScPoseSlider)
	EVT_TEXT(XRCID("rxPoseText"), OutfitStudioFrame::OnRXPoseTextChanged)
	EVT_TEXT(XRCID("ryPoseText"), OutfitStudioFrame::OnRYPoseTextChanged)
	EVT_TEXT(XRCID("rzPoseText"), OutfitStudioFrame::OnRZPoseTextChanged)
	EVT_TEXT(XRCID("txPoseText"), OutfitStudioFrame::OnTXPoseTextChanged)
	EVT_TEXT(XRCID("tyPoseText"), OutfitStudioFrame::OnTYPoseTextChanged)
	EVT_TEXT(XRCID("tzPoseText"), OutfitStudioFrame::OnTZPoseTextChanged)
	EVT_TEXT(XRCID("scPoseText"), OutfitStudioFrame::OnScPoseTextChanged)
	EVT_BUTTON(XRCID("resetBonePose"), OutfitStudioFrame::OnResetBonePose)
	EVT_BUTTON(XRCID("resetAllPose"), OutfitStudioFrame::OnResetAllPose)
	EVT_BUTTON(XRCID("poseToMesh"), OutfitStudioFrame::OnPoseToMesh)
	EVT_CHECKBOX(XRCID("cbPose"), OutfitStudioFrame::OnPoseCheckBox)
	EVT_CHECKBOX(XRCID("cbPhysics"), OutfitStudioFrame::OnPhysicsCheckBox)
	EVT_CHECKBOX(XRCID("cbPhysicsVis"), OutfitStudioFrame::OnPhysicsVisCheckBox)
	EVT_CHECKBOX(XRCID("cbPhysicsGrab"), OutfitStudioFrame::OnPhysicsGrabCheckBox)
	EVT_CHECKBOX(XRCID("cbPhysicsProbe"), OutfitStudioFrame::OnPhysicsProbeCheckBox)
	EVT_COMMAND_SCROLL(XRCID("physicsProbeX"), OutfitStudioFrame::OnPhysicsProbeXSlider)
	EVT_COMMAND_SCROLL(XRCID("physicsProbeY"), OutfitStudioFrame::OnPhysicsProbeYSlider)
	EVT_COMMAND_SCROLL(XRCID("physicsProbeZ"), OutfitStudioFrame::OnPhysicsProbeZSlider)
	EVT_COMMAND_SCROLL(XRCID("physicsProbeSize"), OutfitStudioFrame::OnPhysicsProbeSizeSlider)
	EVT_TEXT(XRCID("physicsProbeXText"), OutfitStudioFrame::OnPhysicsProbeXText)
	EVT_TEXT(XRCID("physicsProbeYText"), OutfitStudioFrame::OnPhysicsProbeYText)
	EVT_TEXT(XRCID("physicsProbeZText"), OutfitStudioFrame::OnPhysicsProbeZText)
	EVT_TEXT(XRCID("physicsProbeSizeText"), OutfitStudioFrame::OnPhysicsProbeSizeText)
	EVT_COMMAND_SCROLL(XRCID("physicsWindSlider"), OutfitStudioFrame::OnPhysicsWindSlider)
	EVT_CHOICE(XRCID("physicsWindDir"), OutfitStudioFrame::OnPhysicsWindDir)
	EVT_TIMER(PHYSICS_TIMER, OutfitStudioFrame::OnPhysicsTimer)
	EVT_TIMER(PHYSICS_REBUILD_TIMER, OutfitStudioFrame::OnPhysicsRebuildTimer)
	EVT_TREE_SEL_CHANGED(XRCID("physicsSystemTree"), OutfitStudioFrame::OnPhysicsSystemTreeSelect)
	EVT_BUTTON(XRCID("btnPhysicsEdit"), OutfitStudioFrame::OnPhysicsEdit)

	EVT_COMBOBOX(XRCID("cPoseName"), OutfitStudioFrame::OnSelectPose)
	EVT_BUTTON(XRCID("savePose"), OutfitStudioFrame::OnSavePose)
	EVT_BUTTON(XRCID("deletePose"), OutfitStudioFrame::OnDeletePose)
	EVT_BUTTON(XRCID("exportPoseFile"), OutfitStudioFrame::OnSaveHkxPose)
	EVT_BUTTON(XRCID("importPoseFile"), OutfitStudioFrame::OnLoadHkxPose)

	EVT_COMBOBOX(XRCID("cAnimationName"), OutfitStudioFrame::OnSelectAnimation)
	EVT_BUTTON(XRCID("importAnimationFile"), OutfitStudioFrame::OnLoadHkxAnimation)
	EVT_BUTTON(XRCID("animPlayPause"), OutfitStudioFrame::OnAnimPlayPause)
	EVT_COMMAND_SCROLL(XRCID("animFrameSlider"), OutfitStudioFrame::OnAnimFrameSlider)
	EVT_CHOICE(XRCID("animSpeed"), OutfitStudioFrame::OnAnimSpeedChanged)
	EVT_CHECKBOX(XRCID("animInterpolate"), OutfitStudioFrame::OnAnimInterpolateChanged)
	EVT_TIMER(ANIM_PLAYBACK_TIMER, OutfitStudioFrame::OnAnimPlaybackTimer)

	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudioFrame::OnSelectSliders)
	EVT_TEXT_ENTER(XRCID("sliderFilter"), OutfitStudioFrame::OnSliderFilterChanged)
	EVT_TEXT(XRCID("sliderFilter"), OutfitStudioFrame::OnSliderFilterChanged)
	EVT_CHECKBOX(XRCID("cbFixedWeight"), OutfitStudioFrame::OnFixedWeight)
	EVT_CHECKBOX(XRCID("cbNormalizeWeights"), OutfitStudioFrame::OnCBNormalizeWeights)

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

	EVT_MENU(XRCID("importTRIHead"), OutfitStudioFrame::OnImportTRIHead)
	EVT_MENU(XRCID("exportTRIHead"), OutfitStudioFrame::OnExportTRIHead)
	EVT_MENU(XRCID("exportShapeTRIHead"), OutfitStudioFrame::OnExportShapeTRIHead)

	EVT_MENU(XRCID("importPhysicsData"), OutfitStudioFrame::OnImportPhysicsData)
	EVT_MENU(XRCID("exportPhysicsData"), OutfitStudioFrame::OnExportPhysicsData)

	EVT_MENU(XRCID("sliderLoadPreset"), OutfitStudioFrame::OnLoadPreset)
	EVT_MENU(XRCID("sliderSavePreset"), OutfitStudioFrame::OnSavePreset)
	EVT_MENU(XRCID("sliderConform"), OutfitStudioFrame::OnSliderConform)
	EVT_MENU(XRCID("sliderConformAll"), OutfitStudioFrame::OnSliderConformAll)
	EVT_MENU(XRCID("sliderFixClipping"), OutfitStudioFrame::OnSliderFixClipping)
	EVT_MENU(XRCID("sliderImportNIF"), OutfitStudioFrame::OnSliderImportNIF)
	EVT_MENU(XRCID("sliderImportBSD"), OutfitStudioFrame::OnSliderImportBSD)
	EVT_MENU(XRCID("sliderImportOBJ"), OutfitStudioFrame::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderImportFBX"), OutfitStudioFrame::OnSliderImportFBX)
	EVT_MENU(XRCID("sliderImportOSD"), OutfitStudioFrame::OnSliderImportOSD)
	EVT_MENU(XRCID("sliderImportTRI"), OutfitStudioFrame::OnSliderImportTRI)
	EVT_MENU(XRCID("sliderImportMorphsSF"), OutfitStudioFrame::OnSliderImportMorphsSF)
	EVT_MENU(XRCID("sliderExportNIF"), OutfitStudioFrame::OnSliderExportNIF)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudioFrame::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderExportOBJ"), OutfitStudioFrame::OnSliderExportOBJ)
	EVT_MENU(XRCID("sliderExportOSD"), OutfitStudioFrame::OnSliderExportOSD)
	EVT_MENU(XRCID("sliderExportTRI"), OutfitStudioFrame::OnSliderExportTRI)
	EVT_MENU(XRCID("sliderExportMorphsSF"), OutfitStudioFrame::OnSliderExportMorphsSF)
	EVT_MENU(XRCID("sliderExportToOBJs"), OutfitStudioFrame::OnSliderExportToOBJs)
	EVT_MENU(XRCID("sliderNew"), OutfitStudioFrame::OnNewSlider)
	EVT_MENU(XRCID("sliderNewZap"), OutfitStudioFrame::OnNewZapSlider)
	EVT_MENU(XRCID("sliderNewCombined"), OutfitStudioFrame::OnNewCombinedSlider)
	EVT_MENU(XRCID("sliderClone"), OutfitStudioFrame::OnSliderClone)
	EVT_MENU(XRCID("sliderNegate"), OutfitStudioFrame::OnSliderNegate)
	EVT_MENU(XRCID("sliderMask"), OutfitStudioFrame::OnMaskAffected)
	EVT_MENU(XRCID("sliderClear"), OutfitStudioFrame::OnClearSlider)
	EVT_MENU(XRCID("sliderDelete"), OutfitStudioFrame::OnDeleteSlider)
	EVT_MENU(XRCID("sliderProperties"), OutfitStudioFrame::OnSliderProperties)
	EVT_MENU(XRCID("sliderDataLocations"), OutfitStudioFrame::OnSliderDataLocations)

	EVT_MENU(XRCID("btnXMirror"), OutfitStudioFrame::OnXMirror)
	EVT_MENU(XRCID("btnConnected"), OutfitStudioFrame::OnConnectedOnly)
	EVT_MENU(XRCID("btnMerge"), OutfitStudioFrame::OnToolOptionMerge)
	EVT_MENU(XRCID("btnWeld"), OutfitStudioFrame::OnToolOptionWeld)
	EVT_MENU(XRCID("btnRestrictSurface"), OutfitStudioFrame::OnToolOptionRestrictSurface)
	EVT_MENU(XRCID("btnRestrictPlane"), OutfitStudioFrame::OnToolOptionRestrictPlane)
	EVT_MENU(XRCID("btnRestrictNormal"), OutfitStudioFrame::OnToolOptionRestrictNormal)
	EVT_MENU(XRCID("btnEdgeSlide"), OutfitStudioFrame::OnToolOptionEdgeSlide)

	EVT_MENU(XRCID("btnSelect"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnTransform"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnPivot"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnVertexEdit"), OutfitStudioFrame::OnSelectTool)

	EVT_MENU(XRCID("btnMaskBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnInflateBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnDeflateBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnMoveBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnSmoothBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnUndiffBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnColorBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnAlphaBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnCollapseVertex"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnFlipEdgeTool"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnSplitEdgeTool"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnMoveVertexTool"), OutfitStudioFrame::OnSelectTool)

	EVT_MENU(XRCID("btnViewFront"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewBack"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewLeft"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewRight"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewPerspective"), OutfitStudioFrame::OnTogglePerspective)
	EVT_MENU(XRCID("btnToggleRotationCenter"), OutfitStudioFrame::OnToggleRotationCenter)
	EVT_MENU(XRCID("btnFrameSelected"), OutfitStudioFrame::OnFrameSelected)

	EVT_MENU(XRCID("btnShowNodes"), OutfitStudioFrame::OnShowNodes)
	EVT_MENU(XRCID("btnShowBones"), OutfitStudioFrame::OnShowBones)
	EVT_MENU(XRCID("btnShowFloor"), OutfitStudioFrame::OnShowFloor)

	EVT_MENU(XRCID("btnIncreaseSize"), OutfitStudioFrame::OnIncBrush)
	EVT_MENU(XRCID("btnDecreaseSize"), OutfitStudioFrame::OnDecBrush)
	EVT_MENU(XRCID("btnIncreaseStr"), OutfitStudioFrame::OnIncStr)
	EVT_MENU(XRCID("btnDecreaseStr"), OutfitStudioFrame::OnDecStr)
	EVT_MENU(XRCID("btnMaskLess"), OutfitStudioFrame::OnMaskLess)
	EVT_MENU(XRCID("btnMaskMore"), OutfitStudioFrame::OnMaskMore)
	EVT_MENU(XRCID("btnClearMask"), OutfitStudioFrame::OnClearMask)
	EVT_MENU(XRCID("btnInvertMask"), OutfitStudioFrame::OnInvertMask)

	EVT_MENU(XRCID("btnRecalcNormals"), OutfitStudioFrame::OnRecalcNormals)
	EVT_MENU(XRCID("disableNormalsCalc"), OutfitStudioFrame::OnDisableNormalsCalc)
	EVT_MENU(XRCID("btnSmoothSeams"), OutfitStudioFrame::OnSmoothNormalSeams)
	EVT_MENU(XRCID("btnSmoothSeamsAngle"), OutfitStudioFrame::OnSmoothSeamsAngle)
	EVT_MENU(XRCID("btnLockNormals"), OutfitStudioFrame::OnLockNormals)

	EVT_MENU(XRCID("btnToggleVisibility"), OutfitStudioFrame::OnToggleVisibility)
	EVT_MENU(XRCID("btnShowWireframe"), OutfitStudioFrame::OnShowWireframe)
	EVT_MENU(XRCID("btnEnableLighting"), OutfitStudioFrame::OnEnableLighting)
	EVT_MENU(XRCID("btnEnableTextures"), OutfitStudioFrame::OnEnableTextures)
	EVT_MENU(XRCID("btnEnableVertexColors"), OutfitStudioFrame::OnEnableVertexColors)

	EVT_MENU(XRCID("uvEdit"), OutfitStudioFrame::OnEditUV)
	EVT_MENU(XRCID("uvInvertX"), OutfitStudioFrame::OnInvertUV)
	EVT_MENU(XRCID("uvInvertY"), OutfitStudioFrame::OnInvertUV)
	EVT_MENU(XRCID("mirrorShape"), OutfitStudioFrame::OnMirrorShape)

	EVT_MENU(XRCID("moveShape"), OutfitStudioFrame::OnMoveShape)
	EVT_MENU(XRCID("scaleShape"), OutfitStudioFrame::OnScaleShape)
	EVT_MENU(XRCID("rotateShape"), OutfitStudioFrame::OnRotateShape)
	EVT_MENU(XRCID("inflateShape"), OutfitStudioFrame::OnInflateShape)
	EVT_MENU(XRCID("fixClippingShape"), OutfitStudioFrame::OnFixClippingShape)
	EVT_MENU(XRCID("renameShape"), OutfitStudioFrame::OnRenameShape)
	EVT_MENU(XRCID("setReference"), OutfitStudioFrame::OnSetReference)
	EVT_MENU(XRCID("deleteVerts"), OutfitStudioFrame::OnDeleteVerts)
	EVT_MENU(XRCID("separateVerts"), OutfitStudioFrame::OnSeparateVerts)
	EVT_MENU(XRCID("copyGeo"), OutfitStudioFrame::OnCopyGeo)
	EVT_MENU(XRCID("copyShape"), OutfitStudioFrame::OnDupeShape)
	EVT_MENU(XRCID("refineMesh"), OutfitStudioFrame::OnRefineMesh)
	EVT_MENU(XRCID("deleteShape"), OutfitStudioFrame::OnDeleteShape)
	EVT_MENU(XRCID("setBoneSkin"), OutfitStudioFrame::OnSetBoneSkin)
	EVT_MENU(XRCID("setBoneNode"), OutfitStudioFrame::OnSetBoneNode)
	EVT_MENU(XRCID("addBone"), OutfitStudioFrame::OnAddBone)
	EVT_MENU(XRCID("addCustomBone"), OutfitStudioFrame::OnAddCustomBone)
	EVT_MENU(XRCID("deleteBone"), OutfitStudioFrame::OnDeleteBone)
	EVT_MENU(XRCID("deleteBoneSelected"), OutfitStudioFrame::OnDeleteBoneFromSelected)
	EVT_MENU(XRCID("editBone"), OutfitStudioFrame::OnEditBone)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudioFrame::OnCopyBoneWeight)
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudioFrame::OnCopySelectedWeight)
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudioFrame::OnTransferSelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudioFrame::OnMaskWeighted)
	EVT_MENU(XRCID("checkBadBones"), OutfitStudioFrame::OnCheckBadBones)
	EVT_MENU(XRCID("maskBoneWeightedVerts"), OutfitStudioFrame::OnMaskBoneWeighted)
	EVT_MENU(XRCID("copySegPart"), OutfitStudioFrame::OnCopySegPart)
	EVT_MENU(XRCID("maskSymVert"), OutfitStudioFrame::OnMaskSymVert)
	EVT_MENU(XRCID("symVert"), OutfitStudioFrame::OnSymVert)
	EVT_MENU(XRCID("maskSymTri"), OutfitStudioFrame::OnMaskSymTri)
	EVT_MENU(XRCID("resetTransforms"), OutfitStudioFrame::OnResetTransforms)
	EVT_MENU(XRCID("deleteUnreferencedNodes"), OutfitStudioFrame::OnDeleteUnreferencedNodes)
	EVT_MENU(XRCID("removeSkinning"), OutfitStudioFrame::OnRemoveSkinning)
	EVT_MENU(XRCID("shapeProperties"), OutfitStudioFrame::OnShapeProperties)

	EVT_MENU(XRCID("editUndo"), OutfitStudioFrame::OnUndo)
	EVT_MENU(XRCID("editRedo"), OutfitStudioFrame::OnRedo)

	EVT_TREE_STATE_IMAGE_CLICK(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeVisToggle)
	EVT_TREE_SEL_CHANGING(XRCID("outfitShapes"), OutfitStudioFrame::OnCheckTreeSel)
	EVT_TREE_SEL_CHANGED(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeSelect)
	EVT_TREE_ITEM_ACTIVATED(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeContext)
	EVT_TREE_BEGIN_DRAG(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeDrag)
	EVT_TREE_END_DRAG(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeDrop)

	EVT_TEXT_ENTER(XRCID("bonesFilter"), OutfitStudioFrame::OnBonesFilterChanged)
	EVT_TEXT(XRCID("bonesFilter"), OutfitStudioFrame::OnBonesFilterChanged)
	EVT_TREE_STATE_IMAGE_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneStateToggle)
	EVT_TREE_SEL_CHANGED(XRCID("outfitBones"), OutfitStudioFrame::OnBoneSelect)
	EVT_TREE_ITEM_ACTIVATED(XRCID("outfitBones"), OutfitStudioFrame::OnBoneActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneTreeContext)

	EVT_TREE_STATE_IMAGE_CLICK(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentVisToggle)
	EVT_TREE_SEL_CHANGED(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("segmentTree"), OutfitStudioFrame::OnSegmentTreeContext)
	EVT_MENU(XRCID("addSegment"), OutfitStudioFrame::OnAddSegment)
	EVT_MENU(XRCID("addSubSegment"), OutfitStudioFrame::OnAddSubSegment)
	EVT_MENU(XRCID("deleteSegment"), OutfitStudioFrame::OnDeleteSegment)
	EVT_MENU(XRCID("deleteSubSegment"), OutfitStudioFrame::OnDeleteSubSegment)
	EVT_CHOICE(XRCID("segmentSlot"), OutfitStudioFrame::OnSegmentSlotChanged)
	EVT_CHOICE(XRCID("segmentType"), OutfitStudioFrame::OnSegmentTypeChanged)
	EVT_BUTTON(XRCID("segmentApply"), OutfitStudioFrame::OnSegmentApply)
	EVT_BUTTON(XRCID("segmentReset"), OutfitStudioFrame::OnSegmentReset)
	EVT_BUTTON(XRCID("segmentSSFEdit"), OutfitStudioFrame::OnSegmentEditSSF)

	EVT_TREE_STATE_IMAGE_CLICK(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionVisToggle)
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
	EVT_BUTTON(XRCID("colorsTabButton"), OutfitStudioFrame::OnTabButtonClick)
	EVT_BUTTON(XRCID("lightsTabButton"), OutfitStudioFrame::OnTabButtonClick)

	EVT_COLOURPICKER_CHANGED(XRCID("cpBrushColor"), OutfitStudioFrame::OnBrushColorChanged)
	EVT_SLIDER(XRCID("cpClampMaxValueSlider"), OutfitStudioFrame::OnColorClampMaxValueSlider)
	EVT_TEXT_ENTER(XRCID("cpClampMaxValueTxt"), OutfitStudioFrame::OnColorClampMaxValueChanged)
	EVT_TEXT(XRCID("cpClampMaxValueTxt"), OutfitStudioFrame::OnColorClampMaxValueChanged)
	EVT_BUTTON(XRCID("btnSwapBrush"), OutfitStudioFrame::OnSwapBrush)
	EVT_BUTTON(XRCID("btnMaskVertexColor"), OutfitStudioFrame::OnMaskVertexColor)

	EVT_SLIDER(XRCID("lightAmbientSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightFrontalSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional0Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional1Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional2Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_BUTTON(XRCID("lightReset"), OutfitStudioFrame::OnResetLights)
	EVT_BUTTON(XRCID("lightSave"), OutfitStudioFrame::OnSaveLights)
	EVT_CHOICE(XRCID("hdriBackground"), OutfitStudioFrame::OnHDRiBackground)

	EVT_MENU(XRCID("btnDiscord"), OutfitStudioFrame::OnDiscord)
	EVT_MENU(XRCID("btnGitHub"), OutfitStudioFrame::OnGitHub)
	EVT_MENU(XRCID("btnPayPal"), OutfitStudioFrame::OnPayPal)

	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("splitter"), OutfitStudioFrame::OnSashPosChanged)
	EVT_SPLITTER_SASH_POS_CHANGED(XRCID("splitterRight"), OutfitStudioFrame::OnSashPosChanged)
	EVT_MOVE_START(OutfitStudioFrame::OnMoveWindowStart)
	EVT_MOVE_END(OutfitStudioFrame::OnMoveWindowEnd)
	EVT_SIZE(OutfitStudioFrame::OnSetSize)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(OutfitStudio);


ConfigurationManager Config;
ConfigurationManager OutfitStudioConfig;

const std::array<wxLanguage, 37> SupportedLangs = {wxLANGUAGE_ENGLISH,	  wxLANGUAGE_AFRIKAANS,		   wxLANGUAGE_ARABIC,  wxLANGUAGE_CATALAN,	  wxLANGUAGE_CZECH,
												   wxLANGUAGE_DANISH,	  wxLANGUAGE_GERMAN,		   wxLANGUAGE_GREEK,   wxLANGUAGE_SPANISH,	  wxLANGUAGE_BASQUE,
												   wxLANGUAGE_FINNISH,	  wxLANGUAGE_FRENCH,		   wxLANGUAGE_HINDI,   wxLANGUAGE_HUNGARIAN,  wxLANGUAGE_INDONESIAN,
												   wxLANGUAGE_ITALIAN,	  wxLANGUAGE_JAPANESE,		   wxLANGUAGE_KOREAN,  wxLANGUAGE_LITHUANIAN, wxLANGUAGE_LATVIAN,
												   wxLANGUAGE_MALAY,	  wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI,  wxLANGUAGE_DUTCH,	  wxLANGUAGE_POLISH,
												   wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN,		   wxLANGUAGE_RUSSIAN, wxLANGUAGE_SLOVAK,	  wxLANGUAGE_SLOVENIAN,
												   wxLANGUAGE_ALBANIAN,	  wxLANGUAGE_SWEDISH,		   wxLANGUAGE_TAMIL,   wxLANGUAGE_TURKISH,	  wxLANGUAGE_UKRAINIAN,
												   wxLANGUAGE_VIETNAMESE, wxLANGUAGE_CHINESE};



// Load files into the current project
void OutfitStudioFrame::LoadFiles(const wxArrayString& files, const wxString& projectName) {
	for (auto& f : files) {
		wxFileName loadFile(f);
		if (loadFile.FileExists()) {
			std::string fileName{loadFile.GetFullPath().ToUTF8()};
			wxString fileExt = loadFile.GetExt().MakeLower();
			if (fileExt == "osp") {
				std::string loadProjectName{projectName.ToUTF8()};
				LoadProject(fileName, loadProjectName);
				// only open first project file
				break;
			}
			else if (fileExt == "nif") {
				StartProgress(_("Adding NIF file..."));
				UpdateProgress(1, _("Adding NIF file..."));
				project->ImportNIF(fileName, false);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
			}
			else if (fileExt == "obj") {
				StartProgress("Adding OBJ file...");
				UpdateProgress(1, _("Adding OBJ file..."));
				project->ImportOBJ(fileName);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
			}
			else if (fileExt == "fbx") {
#ifdef USE_FBXSDK
				StartProgress(_("Adding FBX file..."));
				UpdateProgress(1, _("Adding FBX file..."));
				project->ImportFBX(fileName);
				project->SetTextures();

				UpdateProgress(60, _("Refreshing GUI..."));
				RefreshGUIFromProj();

				EndProgress();
#endif
			}
		}
	}
}

OutfitStudio::~OutfitStudio() {
	if (ipcServer) {
		delete ipcServer;
		ipcServer = nullptr;
	}

	if (singleChecker) {
		delete singleChecker;
		singleChecker = nullptr;
	}

	if (locale) {
		delete locale;
		locale = nullptr;
	}

	FSManager::del();
}

bool OutfitStudio::OnInit() {
	if (!wxApp::OnInit())
		return false;

#ifdef _DEBUG
	std::string dataDir{wxGetCwd().ToUTF8()};
#else
	std::string dataDir{wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath().ToUTF8()};
#endif

	Config.LoadConfig(dataDir + "/Config.xml");
	OutfitStudioConfig.LoadConfig(dataDir + "/OutfitStudio.xml", "OutfitStudioConfig");

	Config.SetDefaultValue("AppDir", dataDir);

	logger.Initialize(Config.GetIntValue("LogLevel", 2), dataDir + "/Log_OS.txt");
	wxLogMessage("Initializing Outfit Studio...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
#endif

#ifdef __WXMSW__
	SetAppearance(SettingsDialogShared::GetConfiguredAppearance(Config));
#endif

	wxString appDirUri = wxString::FromUTF8(dataDir);
	appDirUri.Replace("#", "%23");
	wxSetEnv("AppDir", appDirUri);

	wxXmlResource* xrc = wxXmlResource::Get();
	xrc->SetFlags(wxXRC_USE_LOCALE | wxXRC_USE_ENVVARS);
	xrc->InitAllHandlers();
	wxInitAllImageHandlers();

	wxLogMessage("Working directory: %s", wxGetCwd());
	wxLogMessage("Executable directory: %s", wxString::FromUTF8(dataDir));
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
		case FO76: gameName.Append("Fallout 76"); break;
		case OB: gameName.Append("Oblivion"); break;
		case SF: gameName.Append("Starfield"); break;
		default: gameName.Append("Invalid");
	}
	wxLogMessage(gameName);

	int x = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.x");
	int y = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.y");
	int w = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.width");
	int h = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.height");
	std::string maximized = OutfitStudioConfig["OutfitStudioFrame.maximized"];

	// create single instance checker
	singleChecker = new wxSingleInstanceChecker(wxString("OutfitStudioInstance"));

	// Headless automation mode: skip single-instance/IPC, do not show the frame,
	// run the named automation script and then close the frame so the app exits
	// cleanly through the normal wx event loop / destructor chain.
	if (!cmdAutomation.IsEmpty()) {
		frame = new OutfitStudioFrame(wxPoint(x, y), wxSize(w, h));
		// Intentionally do not call frame->Show() / Maximize().
		SetTopWindow(frame);

		GameUtil::InitArchives();

		// Defer execution until the main loop is running so wxYield/UI events work.
		CallAfter([this]() {
			automationExitCode = RunAutomationFromCmdLine();
			if (frame) {
				// Suppress the unsaved-changes prompt in headless mode.
				frame->SetPendingChanges(false);
				frame->Close(true);  // fires OnClose -> cleans project/glView, then frame deletion
			}
		});

		return true;
	}

	// If files were passed on the command line, try single-instance IPC via wxWidgets
	if (!cmdFiles.IsEmpty()) {
		if (singleChecker->IsAnotherRunning()) {
			int behavior = OutfitStudioConfig.GetIntValue("SingleInstanceBehavior", 0);

			// Override behavior with command line argument if provided
			if (cmdForceSingleInstanceBehavior >= 0) {
				// cmdForceSingleInstanceBehavior: 0 = force new, 1 = force existing
				behavior = cmdForceSingleInstanceBehavior == 1 ? 1 : 2;  // 1 = Open in Existing, 2 = Open in New
			}

			// 0 = Ask (Message Box), 1 = Open in Existing, 2 = Open in New
			int answer = wxYES;
			if (behavior == 0) {
				wxDialog dlg(nullptr, wxID_ANY, _("Open in existing instance?"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
				wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

				wxBoxSizer* msgSizer = new wxBoxSizer(wxHORIZONTAL);
				msgSizer->Add(new wxStaticBitmap(&dlg, wxID_ANY, wxArtProvider::GetBitmap(wxART_QUESTION, wxART_MESSAGE_BOX)), 0, wxALL | wxALIGN_CENTER_VERTICAL, 10);
				msgSizer->Add(new wxStaticText(&dlg, wxID_ANY, _("An instance of Outfit Studio is already running. Open file(s) in the existing instance?")), 1, wxALL | wxALIGN_CENTER_VERTICAL, 10);
				mainSizer->Add(msgSizer, 1, wxEXPAND);

				wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
				wxButton* btnYes = new wxButton(&dlg, wxID_YES, _("Yes"));
				wxButton* btnYesAlways = new wxButton(&dlg, wxID_YES + 100, _("Yes (always)"));
				wxButton* btnNo = new wxButton(&dlg, wxID_NO, _("No"));
				wxButton* btnNoNever = new wxButton(&dlg, wxID_NO + 100, _("No (never)"));
				btnSizer->Add(btnYes, 0, wxALL, 5);
				btnSizer->Add(btnYesAlways, 0, wxALL, 5);
				btnSizer->Add(btnNo, 0, wxALL, 5);
				btnSizer->Add(btnNoNever, 0, wxALL, 5);
				mainSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxBOTTOM, 5);

				btnYes->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_YES); });
				btnYesAlways->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_YES + 100); });
				btnNo->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_NO); });
				btnNoNever->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) { dlg.EndModal(wxID_NO + 100); });

				dlg.SetSizerAndFit(mainSizer);
				dlg.CenterOnScreen();

				int result = dlg.ShowModal();
				if (result == wxID_YES || result == wxID_YES + 100) {
					answer = wxYES;
					if (result == wxID_YES + 100) {
						OutfitStudioConfig.SetValue("SingleInstanceBehavior", 1);
						OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
					}
				}
				else {
					answer = wxNO;
					if (result == wxID_NO + 100) {
						OutfitStudioConfig.SetValue("SingleInstanceBehavior", 2);
						OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
					}
				}
			}
			else if (behavior == 1) {
				answer = wxYES;  // Open in existing
			}
			else if (behavior == 2) {
				answer = wxNO;   // Open in new
			}

			if (answer == wxYES) {
				// Build newline-separated list
				wxString concat;
				for (size_t i = 0; i < cmdFiles.GetCount(); ++i) {
					if (i)
						concat.Append("\n");
					concat.Append(cmdFiles[i]);
				}

				wxClient client;
				// Try to connect to the server
				wxConnectionBase* conn = client.MakeConnection("localhost", OS_IPC_SERVICE, OS_IPC_SERVICE);
				if (conn) {
					conn->Execute(concat);
					conn->Disconnect();
				}

				// exit this new instance
				return false;
			}
		}
		else {
			// we'll be the server; create it after frame is created
		}
	}

	frame = new OutfitStudioFrame(wxPoint(x, y), wxSize(w, h));
	if (maximized == "true")
		frame->Maximize();

	frame->Show();
	SetTopWindow(frame);

	// If we are the primary instance create IPC server so subsequent launches can connect
	ipcServer = new OutfitStudioIPCServer();
	if (!ipcServer->Create(OS_IPC_SERVICE)) {
		delete ipcServer;
		ipcServer = nullptr;
	}

	GameUtil::InitArchives();

	if (!Config["GameDataPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["GameDataPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["GameDataPath"]);
		if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for game data path!\n\nPlease launch the program with admin elevation and make sure the game data path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!Config["ProjectPath"].empty()) {
		bool dirWritable = wxFileName::IsDirWritable(Config["ProjectPath"]);
		bool dirReadable = wxFileName::IsDirReadable(Config["ProjectPath"]);
		if (!dirWritable || !dirReadable)
			wxMessageBox(
				_("No read/write permission for project path!\n\nPlease launch the program with admin elevation and make sure the project path in the settings is correct."),
				_("Warning"),
				wxICON_WARNING);
	}

	if (!cmdFiles.IsEmpty()) {
		frame->LoadFiles(cmdFiles, cmdProject);
	}

	Bind(wxEVT_CHAR_HOOK, &OutfitStudio::CharHook, this);

	frame->UpdateTitle();
	wxLogMessage("Outfit Studio initialized.");
	return true;
}

void OutfitStudio::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars("-");
}

bool OutfitStudio::OnCmdLineParsed(wxCmdLineParser& parser) {
	parser.Found("proj", &cmdProject);
	parser.Found("automation", &cmdAutomation);

	wxString singleInstanceArg;
	if (parser.Found("single", &singleInstanceArg)) {
		wxString lowerArg = singleInstanceArg.Lower();
		if (lowerArg == "yes") {
			cmdForceSingleInstanceBehavior = 1;  // Force open in existing
		}
		else if (lowerArg == "no") {
			cmdForceSingleInstanceBehavior = 0;  // Force open in new
		}
		// Otherwise leave it as -1 (not set)
	}

	for (size_t i = 0; i < parser.GetParamCount(); i++)
		cmdFiles.Add(parser.GetParam(i));

	return true;
}

int OutfitStudio::RunAutomationFromCmdLine() {
	wxLogMessage("Automation: Running script '%s' in headless mode.", cmdAutomation);

	AutomationDialog* dlg = new AutomationDialog(frame, frame->project);
	// Dialog must remain unshown.

	int code = dlg->RunHeadless(cmdAutomation, cmdFiles);

	// Dialog is parented to the frame and would be auto-deleted on frame teardown,
	// but explicitly destroying it here releases its resources (XRC unload, status
	// bar, etc.) before the frame closes.
	dlg->Destroy();
	return code;
}

int OutfitStudio::OnExit() {
	int base = wxApp::OnExit();
	// In automation mode propagate the script's exit code; otherwise keep wx's.
	return cmdAutomation.IsEmpty() ? base : automationExitCode;
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
	LogStackTraceFromException();

	wxMessageBox(_("Fatal exception has occurred, the program will terminate."), _("Fatal exception"), wxICON_ERROR);

	wxDebugReport report;
	report.AddExceptionContext();
	report.Process();
}

void OutfitStudio::CharHook(wxKeyEvent& event) {
	wxWindow* w = (wxWindow*)event.GetEventObject();
	if (!w) {
		event.Skip();
		return;
	}

	if (!frame->IsDescendant(w)) {
		event.Skip();
		return;
	}

#ifdef _WINDOWS
	bool isTextCtrl = dynamic_cast<wxTextCtrl*>(w) != nullptr || dynamic_cast<wxComboBox*>(w) != nullptr;
	if (isTextCtrl) {
		BYTE keyState[256];
		GetKeyboardState(keyState);
		WCHAR result[4] = {};
		int len = ToUnicode(event.GetRawKeyCode(), MapVirtualKey(event.GetRawKeyCode(), MAPVK_VK_TO_VSC), keyState, result, 4, 0);
		if (len == 1 && result[0] >= 0x20) {
			HWND hwnd = static_cast<HWND>(w->GetHandle());
			::SendMessage(hwnd, WM_CHAR, result[0], event.GetRawKeyFlags());
			return;
		}
	}
#endif

	event.Skip();
}

bool OutfitStudio::SetDefaultConfig() {
	int currentTarget = -1;
	Config.SetDefaultValue("TargetGame", currentTarget);
	currentTarget = Config.GetIntValue("TargetGame");

	Config.SetDefaultBoolValue("WarnMissingGamePath", true);
	Config.SetDefaultBoolValue("BSATextureScan", true);
	Config.SetDefaultValue("LogLevel", "3");
	Config.SetDefaultBoolValue("UseSystemLanguage", false);
	SettingsDialogShared::SetDefaultAppearanceMode(Config);
	Config.SetDefaultValue("Input/SliderMinimum", 0);
	Config.SetDefaultValue("Input/SliderMaximum", 100);
	Config.SetDefaultBoolValue("Input/LeftMousePan", false);
	Config.SetDefaultBoolValue("Input/BrushSettingsNearCursor", true);
	Config.SetDefaultBoolValue("Input/MaskHistory", true);
	Config.SetDefaultBoolValue("Input/ShapeHoverHighlight", true);
	Config.SetDefaultValue("Lights/Ambient", 15);
	Config.SetDefaultValue("Lights/Frontal", 100);
	Config.SetDefaultValue("Lights/Directional0", 0);
	Config.SetDefaultValue("Lights/Directional0.x", -90);
	Config.SetDefaultValue("Lights/Directional0.y", 10);
	Config.SetDefaultValue("Lights/Directional0.z", 100);
	Config.SetDefaultValue("Lights/Directional1", 0);
	Config.SetDefaultValue("Lights/Directional1.x", 70);
	Config.SetDefaultValue("Lights/Directional1.y", 10);
	Config.SetDefaultValue("Lights/Directional1.z", 100);
	Config.SetDefaultValue("Lights/Directional2", 0);
	Config.SetDefaultValue("Lights/Directional2.x", 30);
	Config.SetDefaultValue("Lights/Directional2.y", 20);
	Config.SetDefaultValue("Lights/Directional2.z", -100);
	Config.SetDefaultValue("Rendering/HDRIBackground", "");
	const wxSize outfitStudioFrameSize = wxWindow::FromDIP(wxSize(1360, 900), nullptr);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.width", outfitStudioFrameSize.GetWidth());
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.height", outfitStudioFrameSize.GetHeight());
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.x", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.y", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashpos", 768);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashrightpos", 200);

	constexpr int DEF_PROJECT_HISTORY = 15;
	if (!OutfitStudioConfig.Exists("ProjectHistory.maxcount"))
		OutfitStudioConfig.SetValue("ProjectHistory.maxcount", DEF_PROJECT_HISTORY);

	Config.SetDefaultValue("GameRegKey/Oblivion", "Software\\Bethesda Softworks\\Oblivion");
	Config.SetDefaultValue("GameRegVal/Oblivion", "Installed Path");
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

	if (Config["GameDataPath"].empty()) {
#ifdef _WINDOWS
		wxString gameKey = Config["GameRegKey/" + GameUtil::TargetGames[targetGame]];
		wxString gameValueKey = Config["GameRegVal/" + GameUtil::TargetGames[targetGame]];
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (!gameKey.empty() && key.Exists()) {
			wxString installPath;
			if (key.HasValues() && key.QueryValue(gameValueKey, installPath)) {
				installPath.Append("Data").Append(PathSepChar);
				Config.SetDefaultValue("GameDataPath", installPath.ToUTF8().data());
				wxLogMessage("Registry game data path: %s", installPath);
			}
			else if (Config["WarnMissingGamePath"] == "true") {
				wxLogWarning("Failed to find game install path registry value or GameDataPath in the config.");
				wxMessageBox(_("Failed to find game install path registry value or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
			}
		}
		else
#endif
			if (Config["WarnMissingGamePath"] == "true") {
			wxLogWarning("Failed to find game install path registry key or GameDataPath in the config.");
			wxMessageBox(_("Failed to find game install path registry key or GameDataPath in the config."), _("Warning"), wxICON_WARNING);
		}
	}
	else
		wxLogMessage("Game data path in config: %s", Config["GameDataPath"]);

	if (!Config["OutputDataPath"].empty()) {
		wxLogMessage("Output data path in config: %s", Config["OutputDataPath"]);
	}

	if (!Config["ProjectPath"].empty()) {
		wxLogMessage("Project path in config: %s", Config["ProjectPath"]);
	}

	return true;
}

bool OutfitStudio::ShowSetup() {
	wxXmlResource* xrc = wxXmlResource::Get();
	bool loaded = xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Setup.xrc");
	if (!loaded) {
		wxMessageBox("Failed to load Setup.xrc file!", "Error", wxICON_ERROR);
		return false;
	}

	wxDialog* setup = xrc->LoadDialog(nullptr, "dlgSetup");
	if (setup) {
		setup->SetSize(setup->FromDIP(wxSize(700, -1)));
		setup->CenterOnScreen();

		wxButton* btOblivion = XRCCTRL(*setup, "btOblivion", wxButton);
		btOblivion->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)OB); });

		wxButton* btFallout3 = XRCCTRL(*setup, "btFallout3", wxButton);
		btFallout3->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO3); });

		wxButton* btFalloutNV = XRCCTRL(*setup, "btFalloutNV", wxButton);
		btFalloutNV->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FONV); });

		wxButton* btSkyrim = XRCCTRL(*setup, "btSkyrim", wxButton);
		btSkyrim->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIM); });

		wxButton* btFallout4 = XRCCTRL(*setup, "btFallout4", wxButton);
		btFallout4->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO4); });

		wxButton* btSkyrimSE = XRCCTRL(*setup, "btSkyrimSE", wxButton);
		btSkyrimSE->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIMSE); });

		wxButton* btFallout4VR = XRCCTRL(*setup, "btFallout4VR", wxButton);
		btFallout4VR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)FO4VR); });

		wxButton* btSkyrimVR = XRCCTRL(*setup, "btSkyrimVR", wxButton);
		btSkyrimVR->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SKYRIMVR); });

		wxButton* btStarfield = XRCCTRL(*setup, "btStarfield", wxButton);
		btStarfield->Bind(wxEVT_BUTTON, [&setup](wxCommandEvent&) { setup->EndModal((int)SF); });

		wxDirPickerCtrl* dirOblivion = XRCCTRL(*setup, "dirOblivion", wxDirPickerCtrl);
		dirOblivion->Bind(wxEVT_DIRPICKER_CHANGED, [&dirOblivion, &btOblivion](wxFileDirPickerEvent&) { btOblivion->Enable(dirOblivion->GetDirName().DirExists()); });

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

		wxDirPickerCtrl* dirStarfield = XRCCTRL(*setup, "dirStarfield", wxDirPickerCtrl);
		dirStarfield->Bind(wxEVT_DIRPICKER_CHANGED, [&dirStarfield, &btStarfield](wxFileDirPickerEvent&) { btStarfield->Enable(dirStarfield->GetDirName().DirExists()); });

		wxFileName dir = GameUtil::GetGameDataPath(OB);
		if (dir.DirExists()) {
			dirOblivion->SetDirName(dir);
			btOblivion->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO3);
		if (dir.DirExists()) {
			dirFallout3->SetDirName(dir);
			btFallout3->Enable();
		}

		dir = GameUtil::GetGameDataPath(FONV);
		if (dir.DirExists()) {
			dirFalloutNV->SetDirName(dir);
			btFalloutNV->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIM);
		if (dir.DirExists()) {
			dirSkyrim->SetDirName(dir);
			btSkyrim->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO4);
		if (dir.DirExists()) {
			dirFallout4->SetDirName(dir);
			btFallout4->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIMSE);
		if (dir.DirExists()) {
			dirSkyrimSE->SetDirName(dir);
			btSkyrimSE->Enable();
		}

		dir = GameUtil::GetGameDataPath(FO4VR);
		if (dir.DirExists()) {
			dirFallout4VR->SetDirName(dir);
			btFallout4VR->Enable();
		}

		dir = GameUtil::GetGameDataPath(SKYRIMVR);
		if (dir.DirExists()) {
			dirSkyrimVR->SetDirName(dir);
			btSkyrimVR->Enable();
		}

		dir = GameUtil::GetGameDataPath(SF);
		if (dir.DirExists()) {
			dirStarfield->SetDirName(dir);
			btStarfield->Enable();
		}

		if (setup->ShowModal() != wxID_CANCEL) {
			int targ = setup->GetReturnCode();
			Config.SetValue("TargetGame", targ);

			wxFileName dataDir;
			switch (targ) {
				case OB:
					dataDir = dirOblivion->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_ob.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case FO3:
					dataDir = dirFallout3->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo3nv.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case FONV:
					dataDir = dirFalloutNV->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo3nv.nif");
					Config.SetValue("Anim/SkeletonRootName", "Bip01");
					break;
				case SKYRIM:
					dataDir = dirSkyrim->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sk.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case FO4:
					dataDir = dirFallout4->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo4.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
				case SKYRIMSE:
					dataDir = dirSkyrimSE->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sse.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case FO4VR:
					dataDir = dirFallout4VR->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_fo4.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
				case SKYRIMVR:
					dataDir = dirSkyrimVR->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sse.nif");
					Config.SetValue("Anim/SkeletonRootName", "NPC Root [Root]");
					break;
				case SF:
					dataDir = dirStarfield->GetDirName();
					Config.SetValue("Anim/DefaultSkeletonReference", "res/skeleton_female_sf.nif");
					Config.SetValue("Anim/SkeletonRootName", "Root");
					break;
			}

			Config.SetValue("GameDataPath", dataDir.GetFullPath().ToUTF8().data());
			Config.SetValue("GameDataPaths/" + GameUtil::TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToUTF8().data());

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
			delete setup;
		}
		else {
			delete setup;
			return false;
		}
	}

	return true;
}

void OutfitStudio::InitLanguage() {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");
	if (lang < 0)
		lang = wxLANGUAGE_ENGLISH;

	// Load language if possible, fall back to English otherwise
	if (wxLocale::IsAvailable(lang)) {
		locale = new wxLocale(lang);
		locale->AddCatalogLookupPathPrefix(wxString::FromUTF8(Config["AppDir"]) + "/lang");
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


void ToolBarButtonHider::Init(wxToolBar* tbi) {
	tb = tbi;
	size_t tc = tb->GetToolsCount();
	butdats.resize(tc);
	for (size_t pos = 0; pos < tc; ++pos)
		butdats[pos].id = tb->GetToolByPos(pos)->GetId();
}

void ToolBarButtonHider::Show(int toolId, bool show) {
	size_t hidcount = 0;
	for (size_t pos = 0; pos < butdats.size(); ++pos) {
		ButDat& bd = butdats[pos];
		if (bd.id != toolId) {
			if (bd.but)
				++hidcount;
			continue;
		}
		if (!show && !bd.but)
			bd.but.reset(tb->RemoveTool(toolId));
		if (show && bd.but) {
			tb->InsertTool(pos - hidcount, bd.but.release());
			bd.but = nullptr;
		}
		break;
	}

	tb->Realize();
}


OutfitStudioFrame::OutfitStudioFrame(const wxPoint& pos, const wxSize& size) {
	wxLogMessage("Loading Outfit Studio frame at X:%d Y:%d with W:%d H:%d...", pos.x, pos.y, size.GetWidth(), size.GetHeight());

	wxXmlResource* xrc = wxXmlResource::Get();
	if (!xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/OutfitStudio.xrc")) {
		wxMessageBox(_("Failed to load OutfitStudio.xrc file!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	if (!xrc->LoadFrame(this, nullptr, "outfitStudio")) {
		wxMessageBox(_("Failed to load Outfit Studio frame!"), _("Error"), wxICON_ERROR);
		Close(true);
		return;
	}

	SetIcon(wxIcon(wxString::FromUTF8(Config["AppDir"]) + "/res/images/OutfitStudio.png", wxBITMAP_TYPE_PNG));

	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Project.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Actions.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/WeightCopy.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Slider.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Skeleton.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Settings.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/About.xrc");

	const std::array<int, 3> statusWidths = {-1, 400, 100};
	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	if (statusBar) {
		statusBar->SetFieldsCount(3);
		statusBar->SetStatusWidths(3, statusWidths.data());
		statusBar->SetStatusText(_("Ready!"));
	}

	this->DragAcceptFiles(true);

	menuBar = xrc->LoadMenuBar(this, "menuBar");
	PartitionTypeChoices::Populate(dynamic_cast<wxChoice*>(FindWindowByName("partitionType")));

	std::vector<std::string> projectHistoryFiles;
	OutfitStudioConfig.GetValueAttributeArray("ProjectHistory", "Project", "file", projectHistoryFiles);
	std::vector<std::string> projectHistoryNames;
	OutfitStudioConfig.GetValueAttributeArray("ProjectHistory", "Project", "name", projectHistoryNames);

	if (projectHistoryFiles.size() == projectHistoryNames.size())
		for (size_t i = projectHistoryFiles.size(); i > 0; --i)
			AddProjectHistory(projectHistoryFiles[i - 1], projectHistoryNames[i - 1]);

	bool disableNormalsCalc = OutfitStudioConfig.GetBoolValue("DisableNormalsCalc");
	menuBar->Check(XRCID("disableNormalsCalc"), disableNormalsCalc);

	toolBarH = (wxToolBar*)FindWindowByName("toolBarH");
	toolBarV = (wxToolBar*)FindWindowByName("toolBarV");
	tbvHider.Init(toolBarV);

	if (toolBarH) {
		brushSettings = reinterpret_cast<wxButton*>(toolBarH->FindWindowByName("brushSettings"));
		if (brushSettings)
			brushSettings->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnBrushSettings, this);

		cbEdgeSlideCorrectUV = reinterpret_cast<wxCheckBox*>(toolBarH->FindWindowByName("cbEdgeSlideCorrectUV"));
		if (cbEdgeSlideCorrectUV) {
			cbEdgeSlideCorrectUV->Bind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnEdgeSlideCorrectUV, this);
			cbEdgeSlideCorrectUV->Show(false);
		}

		fovSlider = reinterpret_cast<wxSlider*>(toolBarH->FindWindowByName("fovSlider"));
		if (fovSlider)
			fovSlider->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnFieldOfViewSlider, this);

		cbDepthClip = reinterpret_cast<wxCheckBox*>(toolBarH->FindWindowByName("cbDepthClip"));
		if (cbDepthClip)
			cbDepthClip->Bind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnDepthClip, this);
	}

	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	bmpEditSlider = new wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/EditSmall.png", wxBITMAP_TYPE_ANY);
	wxBitmapHelpers::Rescale(*bmpEditSlider, FromDIP(wxSize(16, 16)));
	bmpEditSliderGreen = new wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/EditSmall_green.png", wxBITMAP_TYPE_ANY);
	wxBitmapHelpers::Rescale(*bmpEditSliderGreen, FromDIP(wxSize(16, 16)));
	bmpSliderSettings = new wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/Settings.png", wxBITMAP_TYPE_ANY);
	wxBitmapHelpers::Rescale(*bmpSliderSettings, FromDIP(wxSize(16, 16)));

	meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
	boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
	colorsTabButton = (wxStateButton*)FindWindowByName("colorsTabButton");
	segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
	partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
	lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");
	toolScroll = dynamic_cast<wxScrolledWindow*>(FindWindowByName("toolScroll"));
	masksPane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("masksPane"));
	posePane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("posePane"));
	physicsPane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("physicsPane"));
	animationPane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("animationPane"));
	notesPane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("notesPane"));
	projectNotes = (wxTextCtrl*)FindWindowByName("projectNotes");

	if (meshTabButton) {
		meshTabButton->SetCheck();
		currentTabButton = meshTabButton;
	}

	if (partitionTabButton) {
		bool showPartitionTab = wxGetApp().targetGame == FO3 || wxGetApp().targetGame == FONV || wxGetApp().targetGame == SKYRIM || wxGetApp().targetGame == SKYRIMSE
								|| wxGetApp().targetGame == SKYRIMVR;
		partitionTabButton->Show(showPartitionTab);
	}

	if (segmentTabButton) {
		bool showSegmentTab = wxGetApp().targetGame == FO4 || wxGetApp().targetGame == FO4VR || wxGetApp().targetGame == FO76;
		segmentTabButton->Show(showSegmentTab);
	}

	outfitShapes = (wxTreeCtrl*)FindWindowByName("outfitShapes");
	if (outfitShapes) {
		wxImageList* visStateImages = new wxImageList(16, 16, false, 2);
		wxBitmap visImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoVisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap invImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoInvisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap wfImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoWireframe.png", wxBITMAP_TYPE_PNG);

		if (visImg.IsOk())
			visStateImages->Add(visImg);
		if (invImg.IsOk())
			visStateImages->Add(invImg);
		if (wfImg.IsOk())
			visStateImages->Add(wfImg);

		outfitShapes->AssignStateImageList(visStateImages);
		shapesRoot = outfitShapes->AddRoot("Shapes");

		outfitShapes->Bind(wxEVT_MOTION, &OutfitStudioFrame::OnShapeTreeMotion, this);
		outfitShapes->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnShapeTreeLeave, this);
	}

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	if (outfitBones) {
		wxBitmap noneImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoNone.png", wxBITMAP_TYPE_PNG);
		wxBitmap changeImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoChange.png", wxBITMAP_TYPE_PNG);
		wxBitmap brokenBoneImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoBrokenBone.png", wxBITMAP_TYPE_PNG);
		wxBitmap brokenBoneChangeImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoBrokenBoneChange.png", wxBITMAP_TYPE_PNG);
		wxImageList* boneStateImages = new wxImageList(16, 16, false, 4);
		if (noneImg.IsOk())
			boneStateImages->Add(noneImg);
		if (changeImg.IsOk())
			boneStateImages->Add(changeImg);
		if (brokenBoneImg.IsOk())
			boneStateImages->Add(brokenBoneImg);
		if (brokenBoneChangeImg.IsOk())
			boneStateImages->Add(brokenBoneChangeImg);
		outfitBones->AssignStateImageList(boneStateImages);
		bonesRoot = outfitBones->AddRoot("Bones");
	}

	colorSettings = (wxPanel*)FindWindowByName("colorSettings");

	segmentTree = (wxTreeCtrl*)FindWindowByName("segmentTree");
	if (segmentTree) {
		wxImageList* segmentStateImages = new wxImageList(16, 16, false, 2);
		wxBitmap visImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoVisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap invImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoInvisible.png", wxBITMAP_TYPE_PNG);

		if (visImg.IsOk())
			segmentStateImages->Add(visImg);
		if (invImg.IsOk())
			segmentStateImages->Add(invImg);

		segmentTree->AssignStateImageList(segmentStateImages);
		segmentRoot = segmentTree->AddRoot("Segments");
	}

	partitionTree = (wxTreeCtrl*)FindWindowByName("partitionTree");
	if (partitionTree) {
		wxImageList* partitionStateImages = new wxImageList(16, 16, false, 2);
		wxBitmap visImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoVisible.png", wxBITMAP_TYPE_PNG);
		wxBitmap invImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoInvisible.png", wxBITMAP_TYPE_PNG);

		if (visImg.IsOk())
			partitionStateImages->Add(visImg);
		if (invImg.IsOk())
			partitionStateImages->Add(invImg);

		partitionTree->AssignStateImageList(partitionStateImages);
		partitionRoot = partitionTree->AddRoot("Partitions");
	}

	int ambient = Config.GetIntValue("Lights/Ambient");
	int frontal = Config.GetIntValue("Lights/Frontal");
	int directional0 = Config.GetIntValue("Lights/Directional0");
	int directional1 = Config.GetIntValue("Lights/Directional1");
	int directional2 = Config.GetIntValue("Lights/Directional2");

	lightSettings = (wxPanel*)FindWindowByName("lightSettings");
	if (lightSettings) {
		auto lightAmbientSlider = (wxSlider*)lightSettings->FindWindowByName("lightAmbientSlider");
		lightAmbientSlider->SetValue(ambient);

		auto lightFrontalSlider = (wxSlider*)lightSettings->FindWindowByName("lightFrontalSlider");
		lightFrontalSlider->SetValue(frontal);

		auto lightDirectional0Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional0Slider");
		lightDirectional0Slider->SetValue(directional0);

		auto lightDirectional1Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional1Slider");
		lightDirectional1Slider->SetValue(directional1);

		auto lightDirectional2Slider = (wxSlider*)lightSettings->FindWindowByName("lightDirectional2Slider");
		lightDirectional2Slider->SetValue(directional2);

		hdriBackground = (wxChoice*)lightSettings->FindWindowByName("hdriBackground");
		PopulateHDRiBackgrounds();
	}

	auto editPanel = (wxPanel*)FindWindowByName("editPanel");
	if (editPanel)
		editPanel->SetBackgroundColour(wxColour(112, 112, 112));

	// Dragging the splitter sash changes how much of the tool area fits.
	if (wxWindow* bottomSplitPanel = FindWindowByName("bottomSplitPanel"))
		bottomSplitPanel->Bind(wxEVT_SIZE, &OutfitStudioFrame::OnBottomPanelResize, this);

	cXMirrorBone = (wxChoice*)FindWindowByName("cXMirrorBone");
	cPoseBone = (wxChoice*)FindWindowByName("cPoseBone");
	rxPoseSlider = (wxSlider*)FindWindowByName("rxPoseSlider");
	ryPoseSlider = (wxSlider*)FindWindowByName("ryPoseSlider");
	rzPoseSlider = (wxSlider*)FindWindowByName("rzPoseSlider");
	txPoseSlider = (wxSlider*)FindWindowByName("txPoseSlider");
	tyPoseSlider = (wxSlider*)FindWindowByName("tyPoseSlider");
	tzPoseSlider = (wxSlider*)FindWindowByName("tzPoseSlider");
	scPoseSlider = (wxSlider*)FindWindowByName("scPoseSlider");
	rxPoseText = (wxTextCtrl*)FindWindowByName("rxPoseText");
	ryPoseText = (wxTextCtrl*)FindWindowByName("ryPoseText");
	rzPoseText = (wxTextCtrl*)FindWindowByName("rzPoseText");
	txPoseText = (wxTextCtrl*)FindWindowByName("txPoseText");
	tyPoseText = (wxTextCtrl*)FindWindowByName("tyPoseText");
	tzPoseText = (wxTextCtrl*)FindWindowByName("tzPoseText");
	scPoseText = (wxTextCtrl*)FindWindowByName("scPoseText");
	cbPose = (wxCheckBox*)FindWindowByName("cbPose");
	cbPhysics = (wxCheckBox*)FindWindowByName("cbPhysics");
	physicsSystemTree = (wxTreeCtrl*)FindWindowByName("physicsSystemTree");
	btnPhysicsEdit = (wxButton*)FindWindowByName("btnPhysicsEdit");
	cbPhysicsVis = (wxCheckBox*)FindWindowByName("cbPhysicsVis");
	cbPhysicsGrab = (wxCheckBox*)FindWindowByName("cbPhysicsGrab");
	cbPhysicsProbe = (wxCheckBox*)FindWindowByName("cbPhysicsProbe");
	physicsProbePanel = (wxPanel*)FindWindowByName("physicsProbePanel");
	physicsProbeX = (wxSlider*)FindWindowByName("physicsProbeX");
	physicsProbeY = (wxSlider*)FindWindowByName("physicsProbeY");
	physicsProbeZ = (wxSlider*)FindWindowByName("physicsProbeZ");
	physicsProbeSize = (wxSlider*)FindWindowByName("physicsProbeSize");
	physicsProbeXText = (wxTextCtrl*)FindWindowByName("physicsProbeXText");
	physicsProbeYText = (wxTextCtrl*)FindWindowByName("physicsProbeYText");
	physicsProbeZText = (wxTextCtrl*)FindWindowByName("physicsProbeZText");
	physicsProbeSizeText = (wxTextCtrl*)FindWindowByName("physicsProbeSizeText");
	physicsWindSlider = (wxSlider*)FindWindowByName("physicsWindSlider");
	physicsWindDir = (wxChoice*)FindWindowByName("physicsWindDir");
	poseToMesh = (wxButton*)FindWindowByName("poseToMesh");

	// Nothing is loaded yet, and only meshes that reference a physics XML can be
	// simulated: UpdatePhysicsControlsVisibility reveals the pane again when one
	// shows up. In a build without Bullet that never happens.
	if (physicsPane)
		physicsPane->Hide();

	cAnimationName = (wxComboBox*)FindWindowByName("cAnimationName");
	animPlayPauseButton = (wxButton*)FindWindowByName("animPlayPause");
	animFrameSlider = (wxSlider*)FindWindowByName("animFrameSlider");
	animFrameText = dynamic_cast<wxStaticText*>(FindWindowByName("animFrameText"));
	animSpeedChoice = (wxChoice*)FindWindowByName("animSpeed");
	animInterpolateCheck = (wxCheckBox*)FindWindowByName("animInterpolate");

	if (cAnimationName) {
		cAnimationName->Append("<None>", (void*)nullptr);
		cAnimationName->SetSelection(0);
	}

	animPlaybackTimer.SetOwner(this, ANIM_PLAYBACK_TIMER);
	physicsTimer.SetOwner(this, PHYSICS_TIMER);
	physicsRebuildTimer.SetOwner(this, PHYSICS_REBUILD_TIMER);

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	if (leftPanel) {
		glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
		glView->SetNotifyWindow(this);

		float brushSize = OutfitStudioConfig.GetFloatValue("BrushSettings.brushsize");
		if (brushSize != 0.0f)
			glView->SetBrushSize(brushSize);
		else
			glView->ResetBrushSize();

		std::vector<std::string> brushNames;
		OutfitStudioConfig.GetValueAttributeArray("BrushSettings", "Brush", "name", brushNames);
		std::vector<std::string> brushStrengthValues;
		OutfitStudioConfig.GetValueAttributeArray("BrushSettings", "Brush", "strength", brushStrengthValues);
		std::vector<std::string> brushFocusValues;
		OutfitStudioConfig.GetValueAttributeArray("BrushSettings", "Brush", "focus", brushFocusValues);
		std::vector<std::string> brushSpacingValues;
		OutfitStudioConfig.GetValueAttributeArray("BrushSettings", "Brush", "spacing", brushSpacingValues);

		auto brushList = glView->GetBrushList();

		for (size_t i = 0; i < brushNames.size(); i++) {
			std::string brushName = brushNames[i];
			auto brushIt = std::find_if(brushList.begin(), brushList.end(), [&brushName](TweakBrush* brush) { return brush->Name() == brushName; });

			if (brushIt != brushList.end()) {
				if (brushStrengthValues.size() > i) {
					float brushStrength = std::atof(brushStrengthValues[i].data());
					(*brushIt)->setStrength(brushStrength);
				}

				if (brushFocusValues.size() > i) {
					float brushFocus = std::atof(brushFocusValues[i].data());
					(*brushIt)->setFocus(brushFocus);
				}

				if (brushSpacingValues.size() > i) {
					float brushSpacing = std::atof(brushSpacingValues[i].data());
					(*brushIt)->setSpacing(brushSpacing);
				}
			}
		}
	}

	wxWindow* rightPanel = FindWindowByName("rightSplitPanel");
	if (rightPanel)
		rightPanel->SetDoubleBuffered(true);

	xrc->AttachUnknownControl("mGLView", glView, this);

	sliderFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), wxTE_PROCESS_ENTER);
	sliderFilter->ShowSearchButton(true);
	sliderFilter->ShowCancelButton(true);
	sliderFilter->SetDescriptiveText("Slider Filter");
	sliderFilter->SetToolTip("Filter slider list by name");

	bonesFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, -1), wxTE_PROCESS_ENTER);
	bonesFilter->ShowSearchButton(true);
	bonesFilter->ShowCancelButton(true);
	bonesFilter->SetDescriptiveText("Filter");
	bonesFilter->SetToolTip("Filter bone list by name");

	xrc->AttachUnknownControl("sliderFilter", sliderFilter, this);
	xrc->AttachUnknownControl("bonesFilter", bonesFilter, this);
	bonesFilter->GetParent()->Hide();

	project = new OutfitProject(this); // Create empty project
	CreateSetSliders();

	selectedItems.clear();

	SetSize(size);
	SetPosition(pos);

	auto splitter = (wxSplitterWindow*)FindWindowByName("splitter");
	if (splitter) {
		int sashPos = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.sashpos");
		splitter->SetSashPosition(sashPos);
	}

	auto splitterRight = (wxSplitterWindow*)FindWindowByName("splitterRight");
	if (splitterRight) {
		int sashRightPos = OutfitStudioConfig.GetIntValue("OutfitStudioFrame.sashrightpos");
		splitterRight->SetSashPosition(sashRightPos);
	}

	if (leftPanel)
		leftPanel->Layout();

	ReEnableToolOptionsUI();

	SetDropTarget(new DnDFile(this));

	// Create initial slider pool
	const size_t minSliderPoolSize = 100;
	sliderPool.CreatePool(minSliderPoolSize, sliderScroll, *bmpEditSlider, *bmpSliderSettings);

	wxLogMessage("Outfit Studio frame loaded.");
}

void OutfitStudioFrame::OnExit(wxCommandEvent& WXUNUSED(event)) {
	Close(true);
}

void OutfitStudioFrame::OnClose(wxCloseEvent& WXUNUSED(event)) {
	PauseAnimationPlayback();

	if (!CheckPendingChanges())
		return;

	if (editUV)
		editUV->Close();

	if (project) {
		ShutdownPhysics();
		delete project;
		project = nullptr;
	}

	sliderPool.Clear();

	OutfitStudioConfig.SetValue("BrushSettings.brushsize", glView->GetBrushSize());

	std::vector<std::map<std::string, std::string>> brushSettingsEntries;

	auto brushList = glView->GetBrushList();
	for (auto& brush : brushList) {
		std::map<std::string, std::string> attributeValues;
		attributeValues["name"] = brush->Name();
		attributeValues["strength"] = std::to_string(brush->getStrength());
		attributeValues["focus"] = std::to_string(brush->getFocus());
		attributeValues["spacing"] = std::to_string(brush->getSpacing());
		brushSettingsEntries.push_back(attributeValues);
	}

	OutfitStudioConfig.ClearValueArray("BrushSettings", "Brush");
	OutfitStudioConfig.AppendValueArray("BrushSettings", "Brush", brushSettingsEntries);

	if (glView) {
		delete glView;
		glView = nullptr;
	}

	if (bmpEditSlider)
		delete bmpEditSlider;
	if (bmpEditSliderGreen)
		delete bmpEditSliderGreen;
	if (bmpSliderSettings)
		delete bmpSliderSettings;

	OutfitStudioConfig.ClearValueArray("ProjectHistory", "Project");

	std::vector<std::map<std::string, std::string>> phArrayEntries;
	for (auto& ph : projectHistory) {
		std::map<std::string, std::string> attributeValues;
		attributeValues["name"] = ph.projectName;
		attributeValues["file"] = ph.fileName;
		phArrayEntries.push_back(attributeValues);
	}

	OutfitStudioConfig.AppendValueArray("ProjectHistory", "Project", phArrayEntries);

	// Reload SingleInstanceBehavior from disk before saving, as it may have been
	// changed by another process via the "Yes (always)" / "No (never)" dialog.
	{
		ConfigurationManager diskConfig;
		std::string configPath = Config["AppDir"] + "/OutfitStudio.xml";
		diskConfig.LoadConfig(configPath, "OutfitStudioConfig");
		if (diskConfig.Exists("SingleInstanceBehavior"))
			OutfitStudioConfig.SetValue("SingleInstanceBehavior", diskConfig.GetIntValue("SingleInstanceBehavior"));
	}

	int ret = OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);

	wxLogMessage("Outfit Studio frame closed.");
	Destroy();
}

bool OutfitStudioFrame::CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size) {
	auto buf = std::make_unique<wxChar[]>(128 * 1024);
	int readSize = 128 * 1024;
	wxFileOffset copiedData = 0;

	for (;;) {
		if (size != -1 && copiedData + readSize > size)
			readSize = size - copiedData;

		inputStream.Read(buf.get(), readSize);

		size_t actuallyRead = inputStream.LastRead();
		outputStream.Write(buf.get(), actuallyRead);
		if (outputStream.LastWrite() != actuallyRead) {
			wxLogError("Failed to output data when copying stream.");
			return false;
		}

		if (size == -1) {
			if (inputStream.Eof())
				break;
		}
		else {
			copiedData += actuallyRead;
			if (copiedData >= size)
				break;
		}
	}

	return true;
}

void OutfitStudioFrame::OnMenuItem(wxCommandEvent& event) {
	int id = event.GetId();
	if (id >= 1000 && id < 2000) {
		// Load project history entry
		if (static_cast<int>(projectHistory.size()) > id - 1000) {
			if (!CheckPendingChanges())
				return;

			auto projectHistoryEntry = projectHistory[id - 1000];
			LoadProject(projectHistoryEntry.fileName, projectHistoryEntry.projectName);
		}
	}
	else
		event.Skip();
}

void OutfitStudioFrame::OnPackProjects(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	wxXmlResource* xrc = wxXmlResource::Get();
	wxDialog* packProjects = xrc->LoadDialog(this, "dlgPackProjects");
	if (packProjects) {
		auto projectFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(200, -1)), wxTE_PROCESS_ENTER);
		projectFilter->ShowSearchButton(true);
		projectFilter->SetDescriptiveText("Project Filter");
		projectFilter->SetToolTip("Filter project list by name");

		xrc->AttachUnknownControl("projectFilter", projectFilter, packProjects);

		packProjects->SetSize(FromDIP(wxSize(550, 300)));
		packProjects->SetMinSize(FromDIP(wxSize(400, 200)));
		packProjects->CenterOnParent();

		std::map<std::string, SliderSet> projectSources;
		std::set<std::string> selectedProjects;

		auto projectList = XRCCTRL(*packProjects, "projectList", wxCheckListBox);
		projectList->Bind(wxEVT_RIGHT_UP, [&](wxMouseEvent& WXUNUSED(event)) {
			wxMenu* menu = wxXmlResource::Get()->LoadMenu("projectListContext");
			if (menu) {
				menu->Bind(wxEVT_MENU, [&](wxCommandEvent& event) {
					if (event.GetId() == XRCID("projectListNone")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};
							projectList->Check(i, false);
							selectedProjects.erase(name);
						}
					}
					else if (event.GetId() == XRCID("projectListAll")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};
							projectList->Check(i);
							selectedProjects.insert(name);
						}
					}
					else if (event.GetId() == XRCID("projectListInvert")) {
						for (uint32_t i = 0; i < projectList->GetCount(); i++) {
							std::string name{projectList->GetString(i).ToUTF8()};

							bool check = !projectList->IsChecked(i);
							projectList->Check(i, check);

							if (check)
								selectedProjects.insert(name);
						}
					}
				});

				PopupMenu(menu);
				delete menu;
			}
		});

		projectList->Bind(wxEVT_CHECKLISTBOX, [&](wxCommandEvent& event) {
			std::string name{event.GetString().ToUTF8()};
			int item = event.GetInt();
			if (projectList->IsChecked(item))
				selectedProjects.insert(name);
			else
				selectedProjects.erase(name);
		});

		projectFilter->Bind(wxEVT_TEXT, [&](wxCommandEvent& event) {
			wxString filterStr = event.GetString();
			filterStr.MakeLower();

			projectList->Clear();

			// Add outfits that are no members to list
			for (auto& project : projectSources) {
				// Filter outfit by name
				wxString projectStr = wxString::FromUTF8(project.first);
				if (projectStr.Lower().Contains(filterStr)) {
					int item = projectList->Append(projectStr);
					if (selectedProjects.find(project.first) != selectedProjects.end())
						projectList->Check(item);
				}
			}
		});

		wxArrayString files;
		wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.osp");
		wxDir::GetAllFiles(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets", &files, "*.xml");

		for (auto& file : files) {
			std::string fileName{file.ToUTF8()};

			SliderSetFile sliderDoc;
			sliderDoc.Open(fileName);
			if (sliderDoc.fail())
				continue;

			std::vector<std::string> setNames;
			sliderDoc.GetSetNamesUnsorted(setNames, false);

			for (auto& setName : setNames) {
				if (projectSources.find(setName) != projectSources.end())
					continue;

				SliderSet set;
				if (sliderDoc.GetSet(setName, set) == 0) {
					projectSources[setName] = set;
					projectList->Append(wxString::FromUTF8(setName));
				}
			}
		}

		std::string sep{wxString(wxFileName::GetPathSeparator()).ToUTF8()};
		wxString baseDir = "Tools" + sep + "BodySlide";

		TargetGame targetGame = wxGetApp().targetGame;
		if (targetGame == SKYRIM || targetGame == SKYRIMSE || targetGame == SKYRIMVR)
			baseDir = "CalienteTools" + sep + "BodySlide";

		auto groupManager = XRCCTRL(*packProjects, "groupManager", wxButton);
		groupManager->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			std::vector<std::string> gmOutfits;
			gmOutfits.reserve(projectSources.size());

			std::transform(std::begin(projectSources), std::end(projectSources), std::back_inserter(gmOutfits), [](auto const& pair) { return pair.first; });

			GroupManager gm(packProjects, gmOutfits);
			gm.ShowModal();
		});

		auto groupFile = XRCCTRL(*packProjects, "groupFile", wxFilePickerCtrl);
		groupFile->SetInitialDirectory(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderGroups");

		auto mergedFileName = XRCCTRL(*packProjects, "mergedFileName", wxTextCtrl);
		auto packFolder = XRCCTRL(*packProjects, "packFolder", wxButton);
		auto packArchive = XRCCTRL(*packProjects, "packArchive", wxButton);

		mergedFileName->Bind(wxEVT_TEXT, [&](wxCommandEvent& WXUNUSED(event)) {
			packFolder->Enable(!mergedFileName->GetValue().IsEmpty());
			packArchive->Enable(!mergedFileName->GetValue().IsEmpty());
		});

		packFolder->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString dir = wxDirSelector(_("Packing projects to folder..."), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, packProjects);
			if (dir.IsEmpty())
				return;

			wxLogMessage("Packing project to folder...");
			StartProgress(_("Packing projects to folder..."));

			std::string mergedFile{wxString(mergedFileName->GetValue() + ".osp").ToUTF8()};
			std::string mergedFilePath{wxFileName::CreateTempFileName("os").ToUTF8()};
			project->ReplaceForbidden(mergedFile);

			SliderSetFile projectFile;
			projectFile.New(mergedFilePath);

			for (auto& setName : selectedProjects) {
				if (projectSources.find(setName) == projectSources.end())
					continue;

				SliderSet set = projectSources[setName];
				projectFile.UpdateSet(set);

				// Add input file to folder
				wxString inputFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileInputStream inputFileStream(inputFilePath);
				if (!inputFileStream.IsOk()) {
					wxLogError("Failed to open input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				// Copy input file to destination folder
				wxString inputFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileName::Mkdir(inputFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

				if (!wxCopyFile(inputFilePath, inputFileDest)) {
					wxLogError("Failed to copy input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to copy input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::set<std::string> dataFiles;

				for (size_t i = 0; i < set.size(); i++) {
					for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
						std::string target = set.ShapeToTarget(it->first);
						std::string targetDataName = set[i].TargetDataName(target);
						if (set[i].IsLocalData(targetDataName)) {
							std::string dataFileName = set[i].DataFileName(targetDataName);
							if (dataFileName.compare(dataFileName.size() - 4, dataFileName.size(), ".bsd") != 0) {
								// Split target file name to get OSD file name
								int split = dataFileName.find_last_of('/');
								if (split < 0)
									split = dataFileName.find_last_of('\\');
								if (split < 0)
									continue;

								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName.substr(0, split));
							}
							else {
								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName);
							}
						}
					}
				}

				// Add data files to folder
				for (auto& df : dataFiles) {
					wxString dataFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + df);
					wxFileInputStream dataFileStream(dataFilePath);
					if (!dataFileStream.IsOk()) {
						wxLogError("Failed to open input file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					// Copy data file to destination folder
					wxString dataFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "ShapeData" + sep + df);
					wxFileName::Mkdir(dataFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

					if (!wxCopyFile(dataFilePath, dataFileDest)) {
						wxLogError("Failed to copy data file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to copy data file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}
				}
			}

			// Save new merged project file
			if (!projectFile.Save()) {
				wxLogError("Failed to save merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Add merged project file to folder
			wxFileInputStream projectFileStream(mergedFilePath);
			if (!projectFileStream.IsOk()) {
				wxLogError("Failed to open project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to open project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Copy merged project file to destination folder
			wxString projectFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "SliderSets" + sep + mergedFile);
			wxFileName::Mkdir(projectFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

			if (!wxCopyFile(mergedFilePath, projectFileDest)) {
				wxLogError("Failed to copy merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to copy merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString groupFilePath = groupFile->GetPath();
			if (!groupFilePath.IsEmpty()) {
				// Add group file to folder
				wxFileInputStream groupFileStream(groupFilePath);
				if (!groupFileStream.IsOk()) {
					wxLogError("Failed to open group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to open group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				// Copy group file to destination folder
				std::string groupFileName{groupFilePath.AfterLast(wxFileName::GetPathSeparator()).ToUTF8()};
				wxString groupFileDest = wxString::FromUTF8(dir + sep + baseDir + sep + "SliderGroups" + sep + groupFileName);
				wxFileName::Mkdir(groupFileDest.BeforeLast(wxFileName::GetPathSeparator()), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

				if (!wxCopyFile(groupFilePath, groupFileDest)) {
					wxLogError("Failed to copy group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to copy group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}
			}

			EndProgress(_("Packing finished."));
		});

		packArchive->Bind(wxEVT_BUTTON, [&](wxCommandEvent& WXUNUSED(event)) {
			wxString fileName = wxFileSelector(_("Packing projects to archive..."), wxEmptyString, wxEmptyString, ".zip", "*.zip", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, packProjects);
			if (fileName.IsEmpty())
				return;

			wxLogMessage("Packing project to archive...");
			StartProgress(_("Packing projects to archive..."));

			std::string mergedFile{wxString(mergedFileName->GetValue() + ".osp").ToUTF8()};
			std::string mergedFilePath{wxFileName::CreateTempFileName("os").ToUTF8()};
			project->ReplaceForbidden(mergedFile);

			SliderSetFile projectFile;
			projectFile.New(mergedFilePath);

			wxFFileOutputStream out(fileName);
			wxZipOutputStream zip(out);

			for (auto& setName : selectedProjects) {
				if (projectSources.find(setName) == projectSources.end())
					continue;

				SliderSet set = projectSources[setName];
				projectFile.UpdateSet(set);

				// Add input file to archive
				wxString inputFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
				wxFileInputStream inputFileStream(inputFilePath);
				if (!inputFileStream.IsOk()) {
					wxLogError("Failed to open input file '%s'!", inputFilePath);
					wxMessageBox(wxString::Format(_("Failed to open input file '%s'!"), inputFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				wxString inputFileEntry = wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + set.GetInputFileName());
				if (!zip.PutNextEntry(inputFileEntry, wxDateTime::Now(), inputFileStream.GetLength())) {
					wxLogError("Failed to put new entry into archive!");
					wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				if (!CopyStreamData(inputFileStream, zip, inputFileStream.GetLength())) {
					wxLogError("Failed to copy file contents to archive!");
					wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::set<std::string> dataFiles;

				for (size_t i = 0; i < set.size(); i++) {
					for (auto it = set.ShapesBegin(); it != set.ShapesEnd(); ++it) {
						std::string target = set.ShapeToTarget(it->first);
						std::string targetDataName = set[i].TargetDataName(target);
						if (set[i].IsLocalData(targetDataName)) {
							std::string dataFileName = set[i].DataFileName(targetDataName);
							if (dataFileName.compare(dataFileName.size() - 4, dataFileName.size(), ".bsd") != 0) {
								// Split target file name to get OSD file name
								int split = dataFileName.find_last_of('/');
								if (split < 0)
									split = dataFileName.find_last_of('\\');
								if (split < 0)
									continue;

								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName.substr(0, split));
							}
							else {
								dataFiles.insert(set.GetDefaultDataFolder() + sep + dataFileName);
							}
						}
					}
				}

				// Add data files to archive
				for (auto& df : dataFiles) {
					wxString dataFilePath = wxString::FromUTF8(ProjectUtil::GetProjectPath() + sep + "ShapeData" + sep + df);
					wxFileInputStream dataFileStream(dataFilePath);
					if (!dataFileStream.IsOk()) {
						wxLogError("Failed to open data file '%s'!", dataFilePath);
						wxMessageBox(wxString::Format(_("Failed to open data file '%s'!"), dataFilePath), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					wxString dataFileEntry = wxString::FromUTF8(baseDir + sep + "ShapeData" + sep + df);
					if (!zip.PutNextEntry(dataFileEntry, wxDateTime::Now(), dataFileStream.GetLength())) {
						wxLogError("Failed to put new entry into archive!");
						wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}

					if (!CopyStreamData(dataFileStream, zip, dataFileStream.GetLength())) {
						wxLogError("Failed to copy file contents to archive!");
						wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
						EndProgress();
						return;
					}
				}
			}

			// Save new merged project file
			if (!projectFile.Save()) {
				wxLogError("Failed to save merged project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			// Add merged project file to archive
			wxFileInputStream projectFileStream(mergedFilePath);
			if (!projectFileStream.IsOk()) {
				wxLogError("Failed to open project file '%s'!", mergedFilePath);
				wxMessageBox(wxString::Format(_("Failed to open project file '%s'!"), mergedFilePath), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString projectFileEntry = wxString::FromUTF8(baseDir + sep + "SliderSets" + sep + mergedFile);
			if (!zip.PutNextEntry(projectFileEntry, wxDateTime::Now(), projectFileStream.GetLength())) {
				wxLogError("Failed to put new entry into archive!");
				wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			if (!CopyStreamData(projectFileStream, zip, projectFileStream.GetLength())) {
				wxLogError("Failed to copy file contents to archive!");
				wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
				EndProgress();
				return;
			}

			wxString groupFilePath = groupFile->GetPath();
			if (!groupFilePath.IsEmpty()) {
				// Add group file to archive
				wxFileInputStream groupFileStream(groupFilePath);
				if (!groupFileStream.IsOk()) {
					wxLogError("Failed to open group file '%s'!", groupFilePath);
					wxMessageBox(wxString::Format(_("Failed to open group file '%s'!"), groupFilePath), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				std::string groupFileName{groupFilePath.AfterLast(wxFileName::GetPathSeparator()).ToUTF8()};
				wxString groupFileEntry = wxString::FromUTF8(baseDir + sep + "SliderGroups" + sep + groupFileName);
				if (!zip.PutNextEntry(groupFileEntry, wxDateTime::Now(), groupFileStream.GetLength())) {
					wxLogError("Failed to put new entry into archive!");
					wxMessageBox(_("Failed to put new entry into archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}

				if (!CopyStreamData(groupFileStream, zip, groupFileStream.GetLength())) {
					wxLogError("Failed to copy file contents to archive!");
					wxMessageBox(_("Failed to copy file contents to archive!"), _("Error"), wxICON_ERROR);
					EndProgress();
					return;
				}
			}

			EndProgress(_("Packing finished."));
		});

		packProjects->ShowModal();
	}
}

void OutfitStudioFrame::OnChooseTargetGame(wxCommandEvent& event) {
	wxChoice* choiceTargetGame = (wxChoice*)event.GetEventObject();
	wxWindow* parent = choiceTargetGame->GetGrandParent();
	wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*parent, "fpSkeletonFile", wxFilePickerCtrl);
	wxChoice* choiceSkeletonRoot = XRCCTRL(*parent, "choiceSkeletonRoot", wxChoice);

	TargetGame targ = (TargetGame)choiceTargetGame->GetSelection();
	switch (targ) {
		case OB:
			fpSkeletonFile->SetPath("res/skeleton_ob.nif");
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case FO3:
		case FONV:
			fpSkeletonFile->SetPath("res/skeleton_fo3nv.nif");
			choiceSkeletonRoot->SetStringSelection("Bip01");
			break;
		case SKYRIM:
			fpSkeletonFile->SetPath("res/skeleton_female_sk.nif");
			choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
			break;
		case SKYRIMSE:
		case SKYRIMVR:
			fpSkeletonFile->SetPath("res/skeleton_female_sse.nif");
			choiceSkeletonRoot->SetStringSelection("NPC Root [Root]");
			break;
		case SF:
			fpSkeletonFile->SetPath("res/skeleton_female_sf.nif");
			choiceSkeletonRoot->SetStringSelection("Root");
			break;
		case FO4:
		case FO4VR:
		case FO76:
		default:
			fpSkeletonFile->SetPath("res/skeleton_fo4.nif");
			choiceSkeletonRoot->SetStringSelection("Root");
			break;
	}

	wxCheckListBox* dataFileList = XRCCTRL(*parent, "DataFileList", wxCheckListBox);
	wxString dataDir = GameUtil::GetGameDataPath(targ);

	wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*parent, "dpGameDataPath", wxDirPickerCtrl);
	dpGameDataPath->SetPath(dataDir);

	SettingsFillDataFiles(dataFileList, dataDir, targ);
}

void OutfitStudioFrame::SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame) {
	dataFileList->Clear();

	wxString cp = "GameDataFiles/" + GameUtil::TargetGames[targetGame];
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
		file = file.AfterLast('/').AfterLast('\\');
		dataFileList->Insert(file, dataFileList->GetCount());

		if (fsearch.find(file.Lower()) == fsearch.end())
			dataFileList->Check(dataFileList->GetCount() - 1);
	}
}

void OutfitStudioFrame::OnSettings(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	wxDialog* settings = wxXmlResource::Get()->LoadDialog(this, "dlgSettings");
	if (settings) {
		settings->SetSize(settings->FromDIP(wxSize(525, -1)));
		settings->SetMinSize(settings->FromDIP(wxSize(525, -1)));
		settings->CenterOnParent();

		wxCollapsiblePane* advancedPane = XRCCTRL(*settings, "advancedPane", wxCollapsiblePane);
		advancedPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&settings](wxCommandEvent&) { settings->Fit(); });

		SettingsDialogShared::CommonSettingsDialogControls commonControls{};
		SettingsDialogShared::InitCommonSettingsDialog(*settings,
			Config,
			OutfitStudioConfig,
			SupportedLangs.data(),
			SupportedLangs.size(),
			commonControls);

		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);

		XRCCTRL(*settings, "cbPreviewAlwaysDetached", wxCheckBox)->Hide();
		XRCCTRL(*settings, "cbPreviewOnLeft", wxCheckBox)->Hide();

		wxChoice* choiceSingleInstanceBehavior = XRCCTRL(*settings, "choiceSingleInstanceBehavior", wxChoice);
		choiceSingleInstanceBehavior->SetSelection(OutfitStudioConfig.GetIntValue("SingleInstanceBehavior", 0));

		SettingsFillDataFiles(commonControls.dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		commonControls.choiceTargetGame->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnChooseTargetGame, this);

		if (settings->ShowModal() == wxID_OK) {
			int targetGameSelection = 0;
			bool needsRestart = false;
			SettingsDialogShared::SaveCommonSettingsDialog(
				Config,
				OutfitStudioConfig,
				GameUtil::TargetGames.data(),
				GameUtil::TargetGames.size(),
				SupportedLangs.data(),
				SupportedLangs.size(),
				commonControls,
				[]() { wxGetApp().InitLanguage(); },
				targetGameSelection,
				needsRestart);

			OutfitStudioConfig.SetValue("SingleInstanceBehavior", choiceSingleInstanceBehavior->GetSelection());
			OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");

			// Apply colors to GL view (SaveCommonSettingsDialog already saved them to config)
			if (glView) {
				wxColour colorBackground = commonControls.cpColorBackground->GetColour();
				glView->gls.SetBackgroundColor(Vector3(colorBackground.Red() / 255.0f, colorBackground.Green() / 255.0f, colorBackground.Blue() / 255.0f));

				wxColour colorWire = commonControls.cpColorWire->GetColour();
				glView->gls.SetWireColor(Vector3(colorWire.Red() / 255.0f, colorWire.Green() / 255.0f, colorWire.Blue() / 255.0f));

				wxColour colorPoints = commonControls.cpColorPoints->GetColour();
				glView->gls.SetPointColor(Vector3(colorPoints.Red() / 255.0f, colorPoints.Green() / 255.0f, colorPoints.Blue() / 255.0f));

				wxColour colorPointsMasked = commonControls.cpColorPointsMasked->GetColour();
				glView->gls.SetMaskedPointColor(Vector3(colorPointsMasked.Red() / 255.0f, colorPointsMasked.Green() / 255.0f, colorPointsMasked.Blue() / 255.0f));

				if (!Config.GetBoolValue("Input/ShapeHoverHighlight"))
					glView->ClearHoverHighlight();

				// Only feeds a uniform, so the meshes and their textures stay as they are
				glView->gls.SetComplexMaterialEnabled(OutfitStudioConfig.GetBoolValue("Rendering/ComplexMaterial", true));

				// True PBR picks a different pair of shader files rather than feeding a uniform, so the
				// shapes have to go back through their material assignment for a change here to show.
				const bool pbrEnabled = OutfitStudioConfig.GetBoolValue("Rendering/TruePBR", true);
				if (pbrEnabled != glView->gls.IsPBREnabled()) {
					glView->gls.SetPBREnabled(pbrEnabled);
					MeshesFromProj();
				}

				glView->Render();
			}

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
		GameUtil::InitArchives();
			if (needsRestart) {
				wxMessageBox(_("Settings changed. Please restart the application for changes to take effect."), _("Settings Changed"), wxOK | wxICON_INFORMATION);
			}
		}

		delete settings;
	}
}

void OutfitStudioFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
	wxDialog* about = wxXmlResource::Get()->LoadDialog(this, "dlgAbout");
	if (about) {
		about->SetSize(about->FromDIP(wxSize(625, 375)));
		about->SetMinSize(about->FromDIP(wxSize(625, 375)));
		about->CenterOnParent();
		about->Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);
		about->Bind(wxEVT_HTML_LINK_CLICKED, &OutfitStudioFrame::OnLinkClicked, this);
		about->ShowModal();
		delete about;
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

void OutfitStudioFrame::OnMoveWindowStart(wxMoveEvent& WXUNUSED(event)) {
	CloseBrushSettings();
}

void OutfitStudioFrame::OnMoveWindowEnd(wxMoveEvent& event) {
	wxPoint p = GetPosition();
	OutfitStudioConfig.SetValue("OutfitStudioFrame.x", p.x);
	OutfitStudioConfig.SetValue("OutfitStudioFrame.y", p.y);
	event.Skip();
}

void OutfitStudioFrame::OnSetSize(wxSizeEvent& event) {
	CloseBrushSettings();

	bool maximized = IsMaximized();
	if (!maximized) {
		wxSize p = event.GetSize();
		OutfitStudioConfig.SetValue("OutfitStudioFrame.width", p.x);
		OutfitStudioConfig.SetValue("OutfitStudioFrame.height", p.y);
	}

	OutfitStudioConfig.SetBoolValue("OutfitStudioFrame.maximized", maximized);
	event.Skip();
}

bool OutfitStudioFrame::SaveProject() {
	if (project->mFileName.empty())
		return SaveProjectAs();

	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return false;
	}

	if (HasUnweightedCheck())
		return false;

	wxLogMessage("Saving project '%s'...", wxString::FromUTF8(project->OutfitName()));
	StartProgress(wxString::Format(_("Saving project '%s'..."), wxString::FromUTF8(project->OutfitName())));

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	if (projectNotes)
		project->activeSet.SetNotes(projectNotes->GetValue().ToUTF8().data());

	std::string error = project->Save(project->mFileName,
									  project->mOutfitName,
									  project->mDataDir,
									  project->mBaseFile,
									  project->mGamePath,
									  project->mGameFile,
									  project->mGenWeights,
									  project->mCopyRef,
									  project->bPreventMorphFile,
									  project->bKeepZappedShapes,
									  project->mSFMorphPath,
									  project->mSFMorphTargetShape);

	if (!error.empty()) {
		wxLogError(error.c_str());
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		EndProgress(_("Saving failed."));
		return false;
	}

	SetPendingChanges(false);
	EndProgress(_("Saved."));
	return true;
}

bool OutfitStudioFrame::SaveProjectAs() {
	wxDialog dlg;
	int result = wxID_CANCEL;

	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return false;
	}

	if (HasUnweightedCheck())
		return false;

	CloseBrushSettings();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgSaveProject", "wxDialog")) {
		XRCCTRL(dlg, "sssNameCopy", wxButton)->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnSSSNameCopy, this);
		XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsTrue, this);
		XRCCTRL(dlg, "sssGenWeightsFalse", wxRadioButton)->Bind(wxEVT_RADIOBUTTON, &OutfitStudioFrame::OnSSSGenWeightsFalse, this);

		XRCCTRL(dlg, "sssOutputDataPathBrowse", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			wxString dataPath = wxString::FromUTF8(Config["GameDataPath"]);
			wxString result = wxDirSelector(_("Select output data path"), dataPath, 0, wxDefaultPosition, &dlg);
			if (!result.empty()) {
				wxFileName relPath(result);
				if (!dataPath.empty())
					relPath.MakeRelativeTo(dataPath);
				XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(relPath.GetFullPath());
			}
		});

		XRCCTRL(dlg, "sssSFMorphPathBrowse", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			wxString dataPath = wxString::FromUTF8(Config["GameDataPath"]);
			wxString result = wxDirSelector(_("Select morph.dat output folder"), dataPath, 0, wxDefaultPosition, &dlg);
			if (!result.empty()) {
				wxFileName relPath(result);
				if (!dataPath.empty())
					relPath.MakeRelativeTo(dataPath);
				XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->ChangeValue(relPath.GetFullPath());
			}
		});

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
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory(wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets");

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
			XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->ChangeValue(wxString::Format("meshes%carmor%c%s", PathSepChar, PathSepChar, sssName));

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

		XRCCTRL(dlg, "sssPreventMorphFile", wxCheckBox)->SetValue(project->bPreventMorphFile);
		XRCCTRL(dlg, "sssKeepZappedShapes", wxCheckBox)->SetValue(project->bKeepZappedShapes);

		auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
		if (targetGame == SF) {
			XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->ChangeValue(project->mSFMorphPath);

			// Populate morph target shape dropdown
			wxChoice* morphShapeChoice = XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice);
			morphShapeChoice->Append("(None)");
			for (auto& s : project->GetWorkNif()->GetShapes())
				morphShapeChoice->Append(wxString::FromUTF8(s->name.get()));

			// Select previously saved shape, or default to (None)
			if (!project->mSFMorphTargetShape.empty()) {
				int sel = morphShapeChoice->FindString(project->mSFMorphTargetShape);
				morphShapeChoice->SetSelection(sel != wxNOT_FOUND ? sel : 0);
			}
			else {
				morphShapeChoice->SetSelection(0);
			}
		}
		else {
			XRCCTRL(dlg, "m_SFMorphPathLabel", wxStaticText)->Hide();
			XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->Hide();
			XRCCTRL(dlg, "sssSFMorphPathBrowse", wxButton)->Hide();
			XRCCTRL(dlg, "m_SFMorphTargetShapeLabel", wxStaticText)->Hide();
			XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice)->Hide();
		}

		if (!project->GetBaseShape()) {
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->Disable();
		}
		else
			XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->SetValue(project->mCopyRef);

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return false;

	wxString strOutfitName;
	wxString strDataDir;
	wxString strBaseFile;
	wxString strGamePath;
	wxString strGameFile;

	wxFileName sliderSetFile = XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->GetFileName();
	if (!sliderSetFile.IsOk()) {
		wxMessageBox(_("Invalid or no slider set file specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (sliderSetFile.GetExt() != "osp")
		sliderSetFile.SetExt("osp");

	strOutfitName = XRCCTRL(dlg, "sssName", wxTextCtrl)->GetValue();
	if (strOutfitName.empty()) {
		wxMessageBox(_("No outfit name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetDirName().GetFullName();
	if (strDataDir.empty()) {
		strDataDir = XRCCTRL(dlg, "sssShapeDataFolder", wxDirPickerCtrl)->GetPath();
		if (strDataDir.empty()) {
			wxMessageBox(_("No data folder specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
			return false;
		}
	}

	wxFileName relativeFolder(strDataDir);
	if (!relativeFolder.IsRelative()) {
		wxString dataFolder(wxString::Format("%s/%s", wxString::FromUTF8(ProjectUtil::GetProjectPath()), "ShapeData"));
		relativeFolder.MakeRelativeTo(dataFolder);
		strDataDir = relativeFolder.GetFullPath();
	}

	strBaseFile = XRCCTRL(dlg, "sssShapeDataFile", wxFilePickerCtrl)->GetFileName().GetFullName();
	if (strBaseFile.length() <= 4) {
		wxMessageBox(_("An invalid or no base outfit .nif file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	if (!strBaseFile.EndsWith(".nif"))
		strBaseFile = strBaseFile.Append(".nif");

	strGamePath = XRCCTRL(dlg, "sssOutputDataPath", wxTextCtrl)->GetValue();
	if (strGamePath.empty()) {
		wxMessageBox(_("No game file path specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	strGameFile = XRCCTRL(dlg, "sssOutputFileName", wxTextCtrl)->GetValue();
	if (strGameFile.empty()) {
		wxMessageBox(_("No game file name specified! Please try again."), _("Error"), wxOK | wxICON_ERROR);
		return false;
	}

	bool copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	bool genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();
	bool preventMorphFile = XRCCTRL(dlg, "sssPreventMorphFile", wxCheckBox)->GetValue();
	bool keepZappedShapes = XRCCTRL(dlg, "sssKeepZappedShapes", wxCheckBox)->GetValue();
	wxString strSFMorphPath = XRCCTRL(dlg, "sssSFMorphPath", wxTextCtrl)->GetValue();
	wxString strSFMorphTargetShape;
	wxChoice* morphShapeChoice = XRCCTRL(dlg, "sssSFMorphTargetShape", wxChoice);
	int sel = morphShapeChoice->GetSelection();
	if (sel > 0) // 0 = "(None)"
		strSFMorphTargetShape = morphShapeChoice->GetString(sel);

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format(_("Saving project '%s'..."), strOutfitName));

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	if (projectNotes)
		project->activeSet.SetNotes(projectNotes->GetValue().ToUTF8().data());

	std::string error = project->Save(sliderSetFile, strOutfitName, strDataDir, strBaseFile, strGamePath, strGameFile, genWeights, copyRef, preventMorphFile, keepZappedShapes, strSFMorphPath, strSFMorphTargetShape);

	if (error.empty()) {
		SetPendingChanges(false);
		menuBar->Enable(XRCID("fileSave"), true);

		RenameProject(strOutfitName.ToUTF8().data());
		EndProgress(_("Saved."));
	}
	else {
		wxLogError(error.c_str());
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		EndProgress(_("Saving failed."));
		return false;
	}

	return true;
}

std::string OutfitStudioFrame::ChooseSliderSetName(const std::string& fileName, const std::string& preferredName) {
	SliderSetFile inFile(fileName);
	if (inFile.fail()) {
		wxLogError("Failed to open '%s' as a slider set file!", fileName);
		wxMessageBox(wxString::Format(_("Failed to open '%s' as a slider set file!"), fileName), _("Slider Set Error"), wxICON_ERROR);
		return "";
	}

	std::vector<std::string> setnames;
	inFile.GetSetNames(setnames);

	if (!preferredName.empty()) {
		auto it = std::find(setnames.begin(), setnames.end(), preferredName);
		if (it != setnames.end())
			return preferredName;
	}

	if (setnames.size() > 1) {
		wxArrayString choices;
		for (auto& s : setnames)
			choices.Add(wxString::FromUTF8(s));

		return wxGetSingleChoice(_("Please choose an outfit to load"), _("Load a slider set"), choices, 0, this).ToUTF8().data();
	}

	if (setnames.size() == 1)
		return setnames.front();

	return "";
}

bool OutfitStudioFrame::LoadProject(const std::string& fileName,
									const std::string& projectName,
									bool clearProject,
									bool newDataLocal,
									bool appendNewSliders,
									bool setAsReference) {
	std::string outfit = ChooseSliderSetName(fileName, projectName);
	if (outfit.empty())
		return false;

	wxLogMessage("Loading project '%s' from file '%s'...", outfit, fileName);
	StartProgress(_("Loading project..."));

	if (clearProject) {
		ClearProject();
		project->ClearReference();
		project->ClearOutfit();

		glView->Cleanup();
		glView->GetUndoHistory()->ClearHistory();

		activeSlider.clear();
		lastActiveSlider.clear();
		bEditSlider = false;
		MenuExitSliderEdit();

		ShutdownPhysics();
		delete project;
		project = new OutfitProject(this);
	}

	wxLogMessage("Loading outfit data...");
	UpdateProgress(10, _("Loading outfit data..."));
	StartSubProgress(10, 40);

	int error = 0;
	std::vector<std::string> origShapeOrder;

	if (clearProject)
		error = project->LoadFromSliderSet(fileName, outfit, &origShapeOrder);
	else
		error = project->AddFromSliderSet(fileName, outfit, newDataLocal, appendNewSliders, setAsReference);

	if (error) {
		EndProgress();
		wxLogError("Failed to create project (%d)!", error);
		wxMessageBox(wxString::Format(_("Failed to create project '%s' from file '%s' (%d)!"), outfit, fileName, error), _("Slider Set Error"), wxICON_ERROR);
		RefreshGUIFromProj();
		return false;
	}

	if (clearProject) {
		NiShape* shape = project->GetBaseShape();
		if (shape) {
			std::string shapeName = shape->name.get();

			// Prevent duplication if valid reference was found
			project->DeleteShape(shape);

			wxLogMessage("Loading reference shape '%s'...", shapeName);
			UpdateProgress(50, wxString::Format(_("Loading reference shape '%s'..."), shapeName));

			error = project->LoadReferenceNif(project->activeSet.GetInputFileName(), shapeName, true, true);
			if (error) {
				EndProgress();
				RefreshGUIFromProj();
				return false;
			}
		}

		project->GetWorkNif()->SetShapeOrder(origShapeOrder);
	}

	wxLogMessage("Loading textures...");
	UpdateProgress(60, _("Loading textures..."));

	project->SetTextures();

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	if (projectNotes && notesPane) {
		wxString notesText = wxString::FromUTF8(project->activeSet.GetNotes());
		projectNotes->SetValue(notesText);
		notesPane->Collapse(notesText.empty());

		UpdateToolScrollLayout();
	}

	UpdateTitle();
	AddProjectHistory(fileName, outfit);

	wxLogMessage("Project loaded.");
	menuBar->Enable(XRCID("fileSave"), true);
	EndProgress();
	return true;
}

void OutfitStudioFrame::CreateSetSliders() {
	wxSizer* rootSz = sliderScroll->GetSizer();

	int inc = 1;
	if (project->SliderCount()) {
		inc = 90 / project->SliderCount();
		StartProgress(_("Creating sliders..."));
	}

	UpdateProgress(0, _("Clearing old sliders..."));

	sliderScroll->Freeze();

	for (auto& sliderPanel : sliderPanels)
		HideSliderPanel(sliderPanel.second);

	sliderPanels.clear();
	sliderFilter->Clear();

	for (size_t i = 0; i < project->SliderCount(); i++) {
		UpdateProgress(inc, _("Loading slider: ") + project->GetSliderName(i));
		if (project->SliderClamp(i)) // clamp sliders are a special case, usually an incorrect scale
			continue;

		createSliderGUI(project->GetSliderName(i), sliderScroll, rootSz);
	}

	if (!sliderScroll->GetDropTarget())
		sliderScroll->SetDropTarget(new DnDSliderFile(this));

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	DoFilterSliders();
	HighlightSliderData();

	EndProgress();
}

void OutfitStudioFrame::createSliderGUI(const std::string& name, wxScrolledWindow* wnd, wxSizer* rootSz) {
	wxString sliderName = wxString::FromUTF8(name);

	wxSliderPanel* sliderPanel = sliderPool.GetNext();
	if (sliderPanel) {
		int minValue = Config.GetIntValue("Input/SliderMinimum");
		int maxValue = Config.GetIntValue("Input/SliderMaximum");

		if (sliderPanel->Create(wnd, sliderName, minValue, maxValue, *bmpEditSlider, *bmpSliderSettings)) {
			// Hover enter
			sliderPanel->btnSliderEdit->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderCheck->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderName->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->slider->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);

			// Hover leave
			sliderPanel->btnSliderEdit->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderCheck->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderName->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->slider->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);

			sliderPanel->btnSliderEdit->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnSliderProp->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnMinus->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->btnPlus->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
			sliderPanel->sliderCheck->Bind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnSliderCheckBox, this);
			sliderPanel->slider->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnSlider, this);
			sliderPanel->sliderReadout->Bind(wxEVT_TEXT, &OutfitStudioFrame::OnReadoutChange, this);

			if (sliderPanel->GetContainingSizer())
				rootSz->Detach(sliderPanel);

			rootSz->Add(sliderPanel, 0, wxALL | wxEXPAND | wxFIXED_MINSIZE, 1);

			if (!sliderPanel->IsShown())
				sliderPanel->Show();

			sliderPanels[name] = sliderPanel;
			ShowSliderEffect(name);
		}
	}
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

	project->AddEmptySlider(sliderName);
	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();

	return sliderName;
}

void OutfitStudioFrame::SetSliderValue(const size_t index, int val) {
	std::string name = project->GetSliderName(index);
	project->SliderValue(index) = val / 100.0f;
	auto it = sliderPanels.find(name);
	if (it != sliderPanels.end() && it->second) {
		it->second->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
		it->second->slider->SetValue(val);
	}
}

void OutfitStudioFrame::SetSliderValue(const std::string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	auto it = sliderPanels.find(name);
	if (it != sliderPanels.end() && it->second) {
		it->second->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
		it->second->slider->SetValue(val);
	}
}

void OutfitStudioFrame::ApplySliders(bool recalcBVH) {
	std::vector<Vector3> verts;
	std::vector<Vector2> uvs;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		project->GetLiveVerts(shape, verts, &uvs);
		glView->UpdateMeshVertices(shape->name.get(), &verts, recalcBVH, true, false, &uvs);
	}

	bool tMode = glView->GetTransformMode();

	if (tMode)
		glView->ShowTransformTool();

	if (!tMode)
		glView->Render();
}

void OutfitStudioFrame::ShowSliderEffect(const std::string& sliderName, bool show) {
	if (project->ValidSlider(sliderName)) {
		project->SliderShow(sliderName) = show;

		wxSliderPanel* sliderPanel = sliderPanels[sliderName];
		if (sliderPanel) {
			if (show)
				sliderPanel->sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
			else
				sliderPanel->sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);
		}
	}
}

void OutfitStudioFrame::UpdateActiveShape() {
	bool smoothSeamNormals = true;
	bool lockNormals = false;
	bool enableSmoothSeamsAngle = true;

	if (!activeItem) {
		if (glView->GetTransformMode())
			glView->ShowTransformTool(false);
		if (glView->GetVertexEdit())
			glView->ShowVertexEdit(false);

		CreateSegmentTree();
		CreatePartitionTree();
		outfitBones->UnselectAll();
		lastSelectedBones.clear();

		menuBar->Enable(XRCID("btnSmoothSeams"), false);
		enableSmoothSeamsAngle = false;
		menuBar->Enable(XRCID("btnLockNormals"), false);
	}
	else {
		Mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
		if (m) {
			smoothSeamNormals = m->smoothSeamNormals;
			lockNormals = m->lockNormals;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
			if (glView->GetVertexEdit())
				glView->ShowVertexEdit();
		}
		else
			enableSmoothSeamsAngle = false;

		menuBar->Enable(XRCID("btnSmoothSeams"), m != nullptr);
		menuBar->Enable(XRCID("btnSmoothSeamsAngle"), m != nullptr);
		menuBar->Enable(XRCID("btnLockNormals"), m != nullptr);

		CreateSegmentTree(activeItem->GetShape());
		CreatePartitionTree(activeItem->GetShape());
	}

	if (!smoothSeamNormals)
		enableSmoothSeamsAngle = false;

	menuBar->Check(XRCID("btnSmoothSeams"), smoothSeamNormals);
	menuBar->Enable(XRCID("btnSmoothSeamsAngle"), enableSmoothSeamsAngle);
	menuBar->Check(XRCID("btnLockNormals"), lockNormals);

	if (autoFrameSelected)
		FrameSelected();
	else if (glView->rotationCenterMode == RotationCenterMode::MeshCenter)
		glView->gls.camRotOffset = glView->gls.GetActiveCenter();

	glView->UpdateBones();
	glView->Render();

	HighlightSliderData();
	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
}

void OutfitStudioFrame::UpdateVertexColors() {
	bool enableVertexColors = menuBar->IsChecked(XRCID("btnEnableVertexColors"));
	glView->SetColorsVisible(enableVertexColors);
	if (enableVertexColors)
		FillVertexColors();
	else if (colorSettings->IsShown())
		glView->ClearColors();
}

void OutfitStudioFrame::UpdateBoneCounts() {
	auto totalBoneCountLabel = reinterpret_cast<wxStaticText*>(FindWindowByName("totalBoneCountLabel"));
	totalBoneCountLabel->SetLabel(wxString::Format(_("Total Bones: %zu"), project->GetActiveBoneCount()));

	std::vector<std::string> boneNames;
	project->GetActiveBones(boneNames);

	size_t selectedBoneCount = 0;
	for (auto& s : selectedItems) {
		for (auto& bone : boneNames) {
			if (project->GetWorkAnim()->HasWeights(s->GetShape()->name.get(), bone)) {
				selectedBoneCount++;
			}
		}
	}

	auto selectedBoneCountLabel = reinterpret_cast<wxStaticText*>(FindWindowByName("selectedBoneCountLabel"));
	selectedBoneCountLabel->SetLabel(wxString::Format(_("Shape Selection Bones: %zu"), selectedBoneCount));
}

void OutfitStudioFrame::HighlightSliderData() {
	for (auto& sliderPanel : sliderPanels) {
		sliderPanel.second->btnSliderEdit->SetBitmap(*bmpEditSlider);

		SliderData& sd = project->activeSet[sliderPanel.first];

		for (auto& i : selectedItems) {
			auto diff = project->GetDiffSet(sd, i->GetShape());
			if (diff && !diff->empty()) {
				sliderPanel.second->btnSliderEdit->SetBitmap(*bmpEditSliderGreen);
				break;
			}
		}
	}
}

void OutfitStudioFrame::HighlightBoneNamesWithWeights() {
	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		outfitBones->SetItemTextColour(item, wxColour(255, 255, 255));

		auto boneName = outfitBones->GetItemText(item).ToStdString();
		for (auto& i : selectedItems) {
			if (project->GetWorkAnim()->HasWeights(i->GetShape()->name.get(), boneName)) {
				outfitBones->SetItemTextColour(item, wxColour(0, 255, 0));
			}
		}

		UpdateBoneItemState(item, boneName);

		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}
}

void OutfitStudioFrame::GetNormalizeBones(std::vector<std::string>* normBones, std::vector<std::string>* notNormBones) {
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	for (auto& boneName : activeBones) {
		if (lastNormalizeBones.find(boneName) != lastNormalizeBones.end()) {
			if (normBones)
				normBones->push_back(boneName);
		}
		else {
			if (notNormBones)
				notNormBones->push_back(boneName);
		}
	}
}

std::vector<std::string> OutfitStudioFrame::GetSelectedBones() {
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	std::vector<std::string> boneList;

	for (auto& selBone : lastSelectedBones) {
		if (std::find(activeBones.begin(), activeBones.end(), selBone) != activeBones.end()) {
			boneList.push_back(selBone);
		}
	}

	return boneList;
}

void OutfitStudioFrame::CalcAutoXMirrorBone() {
	// Note that there is very similar code to this in OutfitProject::MatchSymmetricBoneNames.

	autoXMirrorBone.clear();

	const size_t abLen = activeBone.length();
	std::vector<std::string> bones;
	project->GetActiveBones(bones);

	int bestFlips = 0;
	for (const std::string& b : bones) {
		if (abLen != b.length())
			continue;

		int flips = 0;
		bool nomatch = false;
		for (size_t i = 0; i < abLen && !nomatch; ++i) {
			char abc = ToLower(activeBone[i]);
			char bc = ToLower(b[i]);
			if (abc == 'l') {
				if (bc == 'r')
					++flips;
				else if (bc != abc)
					nomatch = true;
			}
			else if (abc == 'r') {
				if (bc == 'l')
					++flips;
				else if (bc != abc)
					nomatch = true;
			}
			else
				nomatch = bc != abc;
		}

		if (nomatch)
			continue;
		if (flips <= bestFlips)
			continue;

		bestFlips = flips;
		autoXMirrorBone = b;
	}

	if (autoXMirrorBone.empty())
		cXMirrorBone->SetString(1, "Auto: None");
	else
		cXMirrorBone->SetString(1, "Auto: " + autoXMirrorBone);
}

std::string OutfitStudioFrame::GetXMirrorBone() {
	int xMChoice = cXMirrorBone->GetSelection();
	if (xMChoice == 0)
		return std::string();
	else if (xMChoice == 1)
		return autoXMirrorBone;
	else
		return cXMirrorBone->GetString(xMChoice).ToStdString();
}

void OutfitStudioFrame::SelectShape(const std::string& shapeName) {
	if (activeItem && activeItem->GetShape()->name == shapeName)
		return;

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

	if (!outfitRoot.IsOk())
		return shapes;

	wxTreeItemIdValue cookie;

	wxTreeItemId curItem = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (curItem.IsOk()) {
		wxString shapeName = outfitShapes->GetItemText(curItem);
		shapes.push_back(shapeName.ToStdString());
		curItem = outfitShapes->GetNextChild(outfitRoot, cookie);
	}

	return shapes;
}

void OutfitStudioFrame::UpdateShapeSource(NiShape* shape) {
	Mesh* m = glView->GetMesh(shape->name.get());
	if (m) {
		project->UpdateShapeFromMesh(shape, m);
		return;
	}

	// Headless / no GL mesh: bake the live (slider-applied) verts directly
	// into the NIF. This mirrors UpdateShapeFromMesh's non-pose path without
	// requiring a GL mesh round-trip.
	std::vector<Vector3> liveVerts;
	project->GetLiveVerts(shape, liveVerts);
	if (!liveVerts.empty())
		project->GetWorkNif()->SetVertsForShape(shape, liveVerts);
}

void OutfitStudioFrame::ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo) {
	if (!usp->sliderName.empty()) {
		float sliderscale = 1 / usp->sliderscale;
		for (auto& uss : usp->usss) {
			Mesh* m = glView->GetMesh(uss.shapeName);
			if (!m)
				continue;

			auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
			if (!shape)
				continue;

			if (!uss.restDiffs.empty()) {
				// Pose-independent undo/redo: use stored rest-space NIF diffs
				std::unordered_map<uint16_t, Vector3> morphDiff;
				for (auto& rd : uss.restDiffs) {
					Vector3 nifDiff = rd.second;
					if (bIsUndo)
						nifDiff *= -1.0f;
					morphDiff[static_cast<uint16_t>(rd.first)] = Mesh::TransformDiffNifToMesh(nifDiff);
				}
				project->UpdateMorphResult(shape, usp->sliderName, morphDiff);
			}
			else {
				std::unordered_map<uint16_t, Vector3> strokeDiff;

				for (auto& ps : uss.pointStartState) {
					auto pe = uss.pointEndState.find(ps.first);
					if (pe == uss.pointEndState.end())
						continue;
					if (bIsUndo)
						strokeDiff[ps.first] = (ps.second - pe->second) * sliderscale;
					else
						strokeDiff[ps.first] = (pe->second - ps.second) * sliderscale;
				}

				if (project->bPose)
					project->UndoPoseDiffs(shape, strokeDiff);

				// Store rest-space NIF diffs for future undo/redo
				for (auto& sd : strokeDiff)
					uss.restDiffs[sd.first] = Mesh::TransformDiffMeshToNif(sd.second);

				project->UpdateMorphResult(shape, usp->sliderName, strokeDiff);
			}
		}

		HighlightSliderData();
	}
	else {
		if (usp->undoType == UndoType::Weight) {
			for (auto& uss : usp->usss) {
				Mesh* m = glView->GetMesh(uss.shapeName);
				if (!m)
					continue;

				for (auto& bw : uss.boneWeights) {
					if (bw.weights.empty())
						continue;
					project->GetWorkAnim()->AddShapeBone(m->shapeName, bw.boneName);
					auto weights = project->GetWorkAnim()->GetWeightsPtr(m->shapeName, bw.boneName);
					if (!weights)
						continue;
					for (auto& p : bw.weights) {
						float val = bIsUndo ? p.second.startVal : p.second.endVal;
						if (val == 0.0f)
							weights->erase(p.first);
						else
							(*weights)[p.first] = val;
					}
				}
				if (project->bPose) {
					auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
					std::vector<Vector3> verts;
					project->GetLiveVerts(shape, verts);
					glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
				}
			}
		}
		else if (usp->undoType == UndoType::Color) {
			for (auto& uss : usp->usss) {
				Mesh* m = glView->GetMesh(uss.shapeName);
				if (!m)
					continue;

				auto colorPtr = project->GetWorkNif()->GetColorsForShape(m->shapeName);
				if (!colorPtr || colorPtr->empty())
					continue;

				std::vector<Color4> vcolors = (*colorPtr);

				if (bIsUndo) {
					for (auto& p : uss.pointStartState) {
						vcolors[p.first].r = p.second.x;
						vcolors[p.first].g = p.second.y;
						vcolors[p.first].b = p.second.z;
					}
				}
				else {
					for (auto& p : uss.pointEndState) {
						vcolors[p.first].r = p.second.x;
						vcolors[p.first].g = p.second.y;
						vcolors[p.first].b = p.second.z;
					}
				}

				project->GetWorkNif()->SetColorsForShape(m->shapeName, vcolors);
			}
		}
		else if (usp->undoType == UndoType::Alpha) {
			for (auto& uss : usp->usss) {
				Mesh* m = glView->GetMesh(uss.shapeName);
				if (!m)
					continue;

				auto colorPtr = project->GetWorkNif()->GetColorsForShape(m->shapeName);
				if (!colorPtr || colorPtr->empty())
					continue;

				std::vector<Color4> vcolors = (*colorPtr);

				if (bIsUndo) {
					for (auto& p : uss.pointStartState)
						vcolors[p.first].a = p.second.x;
				}
				else {
					for (auto& p : uss.pointEndState)
						vcolors[p.first].a = p.second.x;
				}

				project->GetWorkNif()->SetColorsForShape(m->shapeName, vcolors);
			}
		}
	}

	SetPendingChanges();
}

void OutfitStudioFrame::UpdateShapeReference(NiShape* shape, NiShape* newShape) {
	for (auto& i : selectedItems) {
		if (i->GetShape() == shape) {
			i->SetShape(newShape);
		}
	}

	if (project->IsBaseShape(shape))
		project->SetBaseShape(newShape, false);
}

std::vector<ShapeItemData*>& OutfitStudioFrame::GetSelectedItems() {
	return selectedItems;
}

void OutfitStudioFrame::ClearSelected(NiShape* shape) {
	if (activeItem && activeItem->GetShape() == shape)
		activeItem = nullptr;

	selectedItems.erase(std::remove_if(selectedItems.begin(), selectedItems.end(), [&](ShapeItemData* i) { return i->GetShape() == shape; }), selectedItems.end());
}

bool OutfitStudioFrame::GetShapeReferenceSource(NiShape* shape, std::string& outProjectFile, std::string& outProjectName) {
	outProjectFile.clear();
	outProjectName.clear();

	if (!shape || !outfitRoot.IsOk())
		return false;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (child.IsOk()) {
		auto* itemData = dynamic_cast<ShapeItemData*>(outfitShapes->GetItemData(child));
		if (itemData && itemData->GetShape() == shape) {
			outProjectFile = itemData->GetRefProjectFile();
			outProjectName = itemData->GetRefProjectName();
			return true;
		}

		child = outfitShapes->GetNextChild(outfitRoot, cookie);
	}

	return false;
}

void OutfitStudioFrame::SetShapeReferenceSource(NiShape* shape, const std::string& projectFile, const std::string& projectName) {
	if (!shape || !outfitRoot.IsOk())
		return;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
	while (child.IsOk()) {
		auto* itemData = dynamic_cast<ShapeItemData*>(outfitShapes->GetItemData(child));
		if (itemData && itemData->GetShape() == shape) {
			itemData->SetRefSource(projectFile, projectName);
			return;
		}

		child = outfitShapes->GetNextChild(outfitRoot, cookie);
	}
}

std::string OutfitStudioFrame::GetActiveBone() {
	return activeBone;
}

void OutfitStudioFrame::HideSliderPanel(wxSliderPanel* sliderPanel) {
	if (!sliderPanel)
		return;

	// Hover enter
	sliderPanel->btnSliderEdit->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderName->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->slider->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_ENTER_WINDOW, &OutfitStudioFrame::OnEnterHoverSlider, this);

	// Hover leave
	sliderPanel->btnSliderEdit->Bind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderName->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->slider->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_LEAVE_WINDOW, &OutfitStudioFrame::OnLeaveHoverSlider, this);

	sliderPanel->btnSliderEdit->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnSliderProp->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnMinus->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->btnPlus->Unbind(wxEVT_BUTTON, &OutfitStudioFrame::OnClickSliderButton, this);
	sliderPanel->sliderCheck->Unbind(wxEVT_CHECKBOX, &OutfitStudioFrame::OnSliderCheckBox, this);
	sliderPanel->slider->Unbind(wxEVT_SLIDER, &OutfitStudioFrame::OnSlider, this);
	sliderPanel->sliderReadout->Unbind(wxEVT_TEXT, &OutfitStudioFrame::OnReadoutChange, this);

	sliderPanel->Hide();
}

void OutfitStudioFrame::EnterSliderEdit(const std::string& sliderName) {
	std::string sliderNameEdit = sliderName;

	if (sliderNameEdit.empty()) {
		if (lastActiveSlider.empty())
			sliderNameEdit = project->GetSliderName(0);
		else
			sliderNameEdit = lastActiveSlider;
	}

	if (sliderNameEdit.empty())
		return;

	if (bEditSlider) {
		wxSliderPanel* sliderPanel = sliderPanels[activeSlider];
		if (sliderPanel) {
			sliderPanel->sliderCheck->Enable(true);
			sliderPanel->btnSliderProp->Hide();
			sliderPanel->btnMinus->Hide();
			sliderPanel->btnPlus->Hide();
			sliderPanel->Layout();
		}
	}

	wxSliderPanel* sliderPanel = sliderPanels[sliderNameEdit];
	if (!sliderPanel)
		return;

	activeSlider = sliderNameEdit;
	lastActiveSlider = activeSlider;
	bEditSlider = true;

	sliderPanel->slider->SetValue(100);
	SetSliderValue(activeSlider, 100);
	ShowSliderEffect(activeSlider, true);

	sliderPanel->sliderCheck->Enable(false);
	sliderPanel->btnSliderProp->Show();
	sliderPanel->btnMinus->Show();
	sliderPanel->btnPlus->Show();
	sliderPanel->Layout();
	MenuEnterSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::ExitSliderEdit() {
	if (!activeSlider.empty()) {
		wxSliderPanel* sliderPanel = sliderPanels[activeSlider];
		if (sliderPanel) {
			sliderPanel->sliderCheck->Enable(true);
			sliderPanel->slider->SetValue(0);
			SetSliderValue(activeSlider, 0);
			ShowSliderEffect(activeSlider, true);
			sliderPanel->btnSliderProp->Hide();
			sliderPanel->btnMinus->Hide();
			sliderPanel->btnPlus->Hide();
			sliderPanel->Layout();
		}

		activeSlider.clear();
	}

	bEditSlider = false;
	MenuExitSliderEdit();

	HighlightSlider(activeSlider);
	ApplySliders();
}

void OutfitStudioFrame::MenuEnterSliderEdit() {
	menuBar->Enable(XRCID("menuImportSlider"), true);
	menuBar->Enable(XRCID("menuExportSlider"), true);
	menuBar->Enable(XRCID("sliderClone"), true);
	menuBar->Enable(XRCID("sliderNegate"), true);
	menuBar->Enable(XRCID("sliderMask"), true);
	menuBar->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudioFrame::MenuExitSliderEdit() {
	menuBar->Enable(XRCID("menuImportSlider"), false);
	menuBar->Enable(XRCID("menuExportSlider"), false);
	menuBar->Enable(XRCID("sliderClone"), false);
	menuBar->Enable(XRCID("sliderNegate"), false);
	menuBar->Enable(XRCID("sliderMask"), false);
	menuBar->Enable(XRCID("sliderProperties"), false);
}

void OutfitStudioFrame::ScrollToActiveSlider() {
	if (!activeSlider.empty()) {
		for (auto& sliderPanel : sliderPanels) {
			if (sliderPanel.first == activeSlider) {
				ScrollWindowIntoView(sliderScroll, sliderPanel.second);
			}
		}
	}
}

void OutfitStudioFrame::SelectTool(ToolID tool) {
	UpdateEdgeSlideToolOptionsUI(tool);

	if (tool == ToolID::Select) {
		glView->SetEditMode(false);
		glView->SetBrushMode(false);
		glView->SetActiveTool(tool);

		menuBar->Check(XRCID("btnSelect"), true);
		toolBarH->ToggleTool(XRCID("btnSelect"), true);

		ReEnableToolOptionsUI();
		ReToggleToolOptionsUI();
		return;
	}

	if (tool == ToolID::Transform) {
		int id = XRCID("btnTransform");
		bool state = !glView->GetTransformMode();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetTransformMode(state);
		return;
	}

	if (tool == ToolID::Pivot) {
		int id = XRCID("btnPivot");
		bool state = !glView->GetPivotMode();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetPivotMode(state);
		return;
	}

	if (tool == ToolID::VertexEdit) {
		int id = XRCID("btnVertexEdit");
		bool state = !glView->GetVertexEdit();
		menuBar->Check(id, state);
		toolBarV->ToggleTool(id, state);
		glView->SetVertexEdit(state);
		return;
	}

	glView->SetActiveTool(tool);
	glView->SetCursorType(GLSurface::BrushCursor);

	ReEnableToolOptionsUI();
	ReToggleToolOptionsUI();

	if (tool == ToolID::MaskBrush) {
		menuBar->Check(XRCID("btnMaskBrush"), true);
		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
	}
	else if (tool == ToolID::InflateBrush) {
		menuBar->Check(XRCID("btnInflateBrush"), true);
		toolBarH->ToggleTool(XRCID("btnInflateBrush"), true);
	}
	else if (tool == ToolID::DeflateBrush) {
		menuBar->Check(XRCID("btnDeflateBrush"), true);
		toolBarH->ToggleTool(XRCID("btnDeflateBrush"), true);
	}
	else if (tool == ToolID::MoveBrush) {
		menuBar->Check(XRCID("btnMoveBrush"), true);
		toolBarH->ToggleTool(XRCID("btnMoveBrush"), true);
	}
	else if (tool == ToolID::SmoothBrush) {
		menuBar->Check(XRCID("btnSmoothBrush"), true);
		toolBarH->ToggleTool(XRCID("btnSmoothBrush"), true);
	}
	else if (tool == ToolID::UndiffBrush) {
		menuBar->Check(XRCID("btnUndiffBrush"), true);
		toolBarH->ToggleTool(XRCID("btnUndiffBrush"), true);
	}
	else if (tool == ToolID::WeightBrush) {
		menuBar->Check(XRCID("btnWeightBrush"), true);
		toolBarH->ToggleTool(XRCID("btnWeightBrush"), true);
	}
	else if (tool == ToolID::ColorBrush) {
		menuBar->Check(XRCID("btnColorBrush"), true);
		toolBarH->ToggleTool(XRCID("btnColorBrush"), true);

		FindWindowById(XRCID("colorPalette"), colorSettings)->Show();
		FindWindowById(XRCID("btnMaskVertexColor"), colorSettings)->Show();
		FindWindowById(XRCID("clampMaxValue"), colorSettings)->Hide();
		colorSettings->Layout();
		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Alpha"));
	}
	else if (tool == ToolID::AlphaBrush) {
		menuBar->Check(XRCID("btnAlphaBrush"), true);
		toolBarH->ToggleTool(XRCID("btnAlphaBrush"), true);

		FindWindowById(XRCID("colorPalette"), colorSettings)->Hide();
		FindWindowById(XRCID("btnMaskVertexColor"), colorSettings)->Hide();
		FindWindowById(XRCID("clampMaxValue"), colorSettings)->Show();
		colorSettings->Layout();
		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Color"));
	}
	else if (tool == ToolID::CollapseVertex) {
		menuBar->Check(XRCID("btnCollapseVertex"), true);
		toolBarH->ToggleTool(XRCID("btnCollapseVertex"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::VertexCursor);
		return;
	}
	else if (tool == ToolID::FlipEdge) {
		menuBar->Check(XRCID("btnFlipEdgeTool"), true);
		toolBarH->ToggleTool(XRCID("btnFlipEdgeTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::EdgeCursor);
		return;
	}
	else if (tool == ToolID::SplitEdge) {
		menuBar->Check(XRCID("btnSplitEdgeTool"), true);
		toolBarH->ToggleTool(XRCID("btnSplitEdgeTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::EdgeCursor);
		return;
	}
	else if (tool == ToolID::MoveVertex) {
		menuBar->Check(XRCID("btnMoveVertexTool"), true);
		toolBarH->ToggleTool(XRCID("btnMoveVertexTool"), true);
		glView->SetEditMode();
		glView->SetBrushMode(false);
		glView->SetCursorType(GLSurface::VertexCursor);
		return;
	}
	else {
		glView->SetEditMode(false);
		glView->SetBrushMode(false);
		glView->SetTransformMode(false);
		return;
	}

	// One of the brushes was activated
	glView->SetEditMode();
	glView->SetBrushMode();
	glView->SetBrushSize(glView->GetBrushSize());

	CheckBrushBounds();
	UpdateBrushSettings();
}

void OutfitStudioFrame::UpdateEdgeSlideToolOptionsUI(ToolID tool) {
	bool isEdgeSlide = tool == ToolID::MoveVertex && glView->GetToolOptionEdgeSlide();
	if (!cbEdgeSlideCorrectUV)
		return;

	cbEdgeSlideCorrectUV->Show(isEdgeSlide);
	cbEdgeSlideCorrectUV->SetValue(isEdgeSlide && glView->GetToolOptionEdgeSlideUVCorrection());

	if (toolBarH) {
		toolBarH->Layout();
		toolBarH->Refresh();
	}
}

void OutfitStudioFrame::ReEnableToolOptionsUI() {
	bool isBrush = glView->GetActiveBrush() != nullptr;
	ToolID toolid = glView->GetActiveTool();
	bool isMV = toolid == ToolID::MoveVertex;
	bool isIB = toolid == ToolID::InflateBrush || toolid == ToolID::DeflateBrush;
	bool isMB = toolid == ToolID::MoveBrush;
	bool isSB = toolid == ToolID::SmoothBrush;

	UpdateEdgeSlideToolOptionsUI(toolid);

	menuBar->Enable(XRCID("btnXMirror"), isBrush || isMV);
	tbvHider.Show(XRCID("btnXMirror"), isBrush || isMV);
	menuBar->Enable(XRCID("btnConnected"), isBrush);
	tbvHider.Show(XRCID("btnConnected"), isBrush);
	menuBar->Enable(XRCID("btnMerge"), isMV);
	tbvHider.Show(XRCID("btnMerge"), isMV);
	menuBar->Enable(XRCID("btnWeld"), isMV);
	tbvHider.Show(XRCID("btnWeld"), isMV);
	tbvHider.Show(XRCID("btnEdgeSlide"), isMV);
	menuBar->Enable(XRCID("btnRestrictSurface"), isMV);
	tbvHider.Show(XRCID("btnRestrictSurface"), isMV);
	menuBar->Enable(XRCID("btnRestrictPlane"), isMV || isMB || isSB);
	tbvHider.Show(XRCID("btnRestrictPlane"), isMV || isMB || isSB);
	menuBar->Enable(XRCID("btnRestrictNormal"), isMV || isIB || isMB || isSB);
	tbvHider.Show(XRCID("btnRestrictNormal"), isMV || isIB || isMB || isSB);
}

void OutfitStudioFrame::ReToggleToolOptionsUI() {
	bool isBrush = glView->GetActiveBrush() != nullptr;
	ToolID toolid = glView->GetActiveTool();
	bool isMV = toolid == ToolID::MoveVertex;
	bool isIB = toolid == ToolID::InflateBrush || toolid == ToolID::DeflateBrush;
	bool isMB = toolid == ToolID::MoveBrush;
	bool isSB = toolid == ToolID::SmoothBrush;
	bool transformMode = glView->GetTransformMode();
	bool xMirror = glView->GetToolOptionXMirror();
	bool connOnly = glView->GetToolOptionConnectedOnly();
	bool merge = glView->GetToolOptionMerge();
	bool weld = glView->GetToolOptionWeld();
	bool rsurf = glView->GetToolOptionRestrictSurface();
	bool rplane = glView->GetToolOptionRestrictPlane();
	bool rnormal = glView->GetToolOptionRestrictNormal();
	bool edgeSlide = glView->GetToolOptionEdgeSlide();
	bool isES = isMV && edgeSlide;
	bool edgeSlideUVCorrection = glView->GetToolOptionEdgeSlideUVCorrection();

	if (cbEdgeSlideCorrectUV)
		cbEdgeSlideCorrectUV->SetValue(isES && edgeSlideUVCorrection);

	menuBar->Check(XRCID("btnTransform"), transformMode);
	toolBarV->ToggleTool(XRCID("btnTransform"), transformMode);
	menuBar->Check(XRCID("btnXMirror"), (isBrush || isMV) && xMirror);
	toolBarV->ToggleTool(XRCID("btnXMirror"), (isBrush || isMV) && xMirror);
	menuBar->Check(XRCID("btnConnected"), isBrush && connOnly);
	toolBarV->ToggleTool(XRCID("btnConnected"), isBrush && connOnly);
	menuBar->Check(XRCID("btnMerge"), isMV && merge);
	toolBarV->ToggleTool(XRCID("btnMerge"), isMV && merge);
	menuBar->Check(XRCID("btnWeld"), isMV && weld);
	toolBarV->ToggleTool(XRCID("btnWeld"), isMV && weld);
	toolBarV->ToggleTool(XRCID("btnEdgeSlide"), isES);
	menuBar->Check(XRCID("btnRestrictSurface"), isMV && rsurf);
	toolBarV->ToggleTool(XRCID("btnRestrictSurface"), isMV && rsurf);
	menuBar->Check(XRCID("btnRestrictPlane"), (isMV || isMB || isSB) && rplane);
	toolBarV->ToggleTool(XRCID("btnRestrictPlane"), (isMV || isMB || isSB) && rplane);
	menuBar->Check(XRCID("btnRestrictNormal"), (isMV || isIB || isMB || isSB) && rnormal);
	toolBarV->ToggleTool(XRCID("btnRestrictNormal"), (isMV || isIB || isMB || isSB) && rnormal);
}

void OutfitStudioFrame::CloseBrushSettings() {
	if (brushSettingsPopupTransient) {
		brushSettingsPopupTransient->Destroy();
		brushSettingsPopupTransient = nullptr;
	}
}

void OutfitStudioFrame::PopupBrushSettings(wxWindow* popupAt) {
	CloseBrushSettings();

	if (!glView->GetBrushMode())
		return;

	wxPoint popupPos;
	wxSize popupSize;

	if (popupAt) {
		popupPos = popupAt->GetScreenPosition();
		popupSize.y = popupAt->GetSize().y;
	}
	else {
		popupPos = wxGetMousePosition();
		popupSize.x = 10;
		popupSize.y = 10;
	}

	bool stayOpen = popupAt != nullptr;
	brushSettingsPopupTransient = new wxBrushSettingsPopupTransient(this, stayOpen);
	brushSettingsPopupTransient->Position(popupPos, popupSize);
	brushSettingsPopupTransient->Popup();

	UpdateBrushSettings();
}

void OutfitStudioFrame::UpdateBrushSettings() {
	TweakBrush* brush = glView->GetActiveBrush();
	if (!brush)
		return;

	if (brushSettingsPopupTransient) {
		brushSettingsPopupTransient->SetBrushName(wxString::FromUTF8(brush->Name()));
		brushSettingsPopupTransient->SetBrushSize(glView->GetBrushSize());
		brushSettingsPopupTransient->SetBrushStrength(brush->getStrength());
		brushSettingsPopupTransient->SetBrushFocus(brush->getFocus());
		brushSettingsPopupTransient->SetBrushSpacing(brush->getSpacing());
	}
}

bool OutfitStudioFrame::ConfirmSliderDataLocalForEdit(NiShape* shape, const std::string& sliderName) {
	std::vector<NiShape*> shapes;
	if (shape)
		shapes.push_back(shape);

	std::vector<std::string> sliderNames;
	if (!sliderName.empty())
		sliderNames.push_back(sliderName);

	return ConfirmSliderDataLocalForEdit(shapes, sliderNames);
}

bool OutfitStudioFrame::ConfirmSliderDataLocalForEdit(const std::vector<NiShape*>& shapes, const std::vector<std::string>& sliderNames) {
	if (!project || shapes.empty() || sliderNames.empty())
		return true;

	std::vector<std::pair<NiShape*, std::string>> externalData;
	std::vector<std::string> labels;

	for (auto* shape : shapes) {
		if (!shape)
			continue;

		std::string shapeName = shape->name.get();
		for (const auto& sliderName : sliderNames) {
			if (sliderName.empty() || !project->SliderDataIsExternal(sliderName, shape))
				continue;

			externalData.emplace_back(shape, sliderName);
			std::string label = shapeName + " / " + sliderName;
			if (std::find(labels.begin(), labels.end(), label) == labels.end())
				labels.push_back(label);
		}
	}

	if (externalData.empty())
		return true;

	wxString message = _("The slider data you are about to edit is external. Editing it will make the affected slider data local to this project. Continue and make it local?");
	if (!labels.empty()) {
		message += "\n\n";
		message += _("Affected slider data:");

		size_t maxLabels = std::min<size_t>(labels.size(), 8);
		for (size_t i = 0; i < maxLabels; i++) {
			message += "\n  ";
			message += wxString::FromUTF8(labels[i]);
		}

		if (labels.size() > maxLabels)
			message += "\n  ...";
	}

	int response = wxMessageBox(message, _("External Slider Data"), wxYES_NO | wxICON_WARNING, this);
	if (response != wxYES)
		return false;

	for (auto& data : externalData)
		project->EnsureSliderDataLocal(data.second, data.first);

	SetPendingChanges();
	HighlightSliderData();
	return true;
}

bool OutfitStudioFrame::CheckEditableState() {
	if (!activeItem)
		return false;

	ToolID activeTool = glView->GetActiveTool();

	// These brushes do not modify slider morph data, so they are always
	// editable regardless of slider state.
	if (activeTool == ToolID::MaskBrush || activeTool == ToolID::WeightBrush || activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
		return true;

	if (bEditSlider) {
		if (project->SliderValue(activeSlider) == 0.0) {
			int response = wxMessageBox(_("You are trying to edit a slider's morph with that slider set to zero.  Do you wish to set the slider to one now?"),
										wxMessageBoxCaptionStr,
										wxYES_NO,
										this);

			if (response == wxYES) {
				SetSliderValue(activeSlider, 100);
				ApplySliders();
			}

			return false;
		}

		std::vector<NiShape*> shapes;
		for (auto* item : selectedItems)
			if (item)
				shapes.push_back(item->GetShape());

		if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{activeSlider}))
			return false;

		return true;
	}

	if (activeTool == ToolID::UndiffBrush) {
		wxMessageBox(_("You can only use the undiff brush while editing a slider. Note, use the pencil button next to a slider to enable editing of that slider's morph."),
						wxMessageBoxCaptionStr,
						wxOK,
						this);
		return false;
	}

	if (project->AllSlidersZero())
		return true;

	int response = wxMessageBox(_("You can only edit the base shape when all sliders are zero. Do you wish to set all sliders to zero now?  Note, use the pencil button next to a "
								  "slider to enable editing of that slider's morph."),
								wxMessageBoxCaptionStr,
								wxYES_NO,
								this);

	if (response == wxYES)
		ZeroSliders();

	return false;
}

void OutfitStudioFrame::UpdateTitle() {
	wxString name = wxString::FromUTF8(project->OutfitName());
	if (name.empty() || name == "New Outfit")
		name = project->mBaseFile;

	if (!name.empty()) {
		if (pendingChanges)
			SetTitle(name + "* - Outfit Studio");
		else
			SetTitle(name + " - Outfit Studio");
	}
	else
		SetTitle("Outfit Studio");
}

void OutfitStudioFrame::AddProjectHistory(const std::string& fileName, const std::string& projectName) {
	projectHistory.erase(std::remove_if(projectHistory.begin(),
										projectHistory.end(),
										[&fileName, &projectName](const ProjectHistoryEntry& rt) { return rt.fileName == fileName && rt.projectName == projectName; }),
						 projectHistory.end());

	ProjectHistoryEntry projectHistoryEntry{};
	projectHistoryEntry.fileName = fileName;
	projectHistoryEntry.projectName = projectName;

	constexpr int DEF_PROJECT_HISTORY = 15;
	constexpr int MAX_PROJECT_HISTORY = 100;

	int maxCount = OutfitStudioConfig.GetIntValue("ProjectHistory.maxcount", DEF_PROJECT_HISTORY);
	if (maxCount <= 0)
		maxCount = DEF_PROJECT_HISTORY;
	else if (maxCount > MAX_PROJECT_HISTORY)
		maxCount = MAX_PROJECT_HISTORY;

	if (projectHistory.size() == static_cast<size_t>(maxCount))
		projectHistory.pop_back();

	projectHistory.push_front(projectHistoryEntry);
	UpdateProjectHistory();
}

void OutfitStudioFrame::UpdateProjectHistory() {
	menuBar->Freeze();

	wxMenuItem* menuItemRecentProjects = menuBar->FindItem(XRCID("menuRecentProjects"));
	if (menuItemRecentProjects && menuItemRecentProjects->IsSubMenu()) {
		wxMenu* menuRecentProjects = menuItemRecentProjects->GetSubMenu();
		if (menuRecentProjects) {
			auto menuItemList = menuRecentProjects->GetMenuItems();
			for (auto menuItem : menuItemList)
				menuRecentProjects->Delete(menuItem);

			int itemId = 0;
			for (const auto& projectHistoryEntry : projectHistory) {
				menuRecentProjects->Append(1000 + itemId, wxString::FromUTF8(projectHistoryEntry.projectName), wxString::FromUTF8(projectHistoryEntry.fileName));
				++itemId;
			}

			menuBar->Enable(XRCID("menuRecentProjects"), itemId > 0);
		}
	}

	menuBar->Thaw();
}

void OutfitStudioFrame::SetPendingChanges(bool pending) {
	if (pendingChanges != pending) {
		pendingChanges = pending;
		UpdateTitle();
	}
}

bool OutfitStudioFrame::CheckPendingChanges() {
	// Physics XMLs are separate files that saving the project does not
	// write, so they get their own question rather than being folded into
	// the project one and then quietly dropped.
	if (!CheckPendingPhysicsXml())
		return false;

	if (pendingChanges) {
		wxMessageDialog dlg(this,
							wxString::Format(_("You have unsaved changes to '%s'. Would you like to save them now?"), project->OutfitName()),
							_("Unsaved Changes"),
							wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);

		int res = dlg.ShowModal();
		if (res == wxID_YES) {
			if (!SaveProject())
				return false;
		}
		else if (res == wxID_CANCEL)
			return false;
	}

	return true;
}

void OutfitStudioFrame::UpdateUndoTools() {
	auto undoHistory = glView->GetUndoHistory();
	toolBarH->EnableTool(XRCID("editUndo"), undoHistory->CanUndo());
	toolBarH->EnableTool(XRCID("editRedo"), undoHistory->CanRedo());
}

void OutfitStudioFrame::OnNewProject(wxCommandEvent& WXUNUSED(event)) {
	wxWizard wiz;
	wxWizardPage* pg1;
	bool result = false;

	if (!CheckPendingChanges())
		return;

	CloseBrushSettings();
	UpdateReferenceTemplates();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, wiz, "LoadReference", "npTemplateChoice", refTemplates);

		wiz.FitToPage(pg1);
		wiz.CenterOnParent();

		result = wiz.RunWizard(pg1);
	}
	if (!result)
		return;

	menuBar->Enable(XRCID("fileSave"), false);

	std::string outfitName{XRCCTRL(wiz, "npOutfitName", wxTextCtrl)->GetValue().ToUTF8()};

	wxLogMessage("Creating project '%s'...", outfitName);
	StartProgress(wxString::Format(_("Creating project '%s'..."), outfitName));

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->GetUndoHistory()->ClearHistory();

	activeSlider.clear();
	lastActiveSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	ShutdownPhysics();
	delete project;
	project = new OutfitProject(this);

	UpdateProgress(10, _("Loading reference..."));

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, wiz, "LoadReference", "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(ProjectUtil::GetProjectPath() + PathSepStr + tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll());
			else
				error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll());
		}
		else
			error = 1;
	}
	else if (XRCCTRL(wiz, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(wiz, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(wiz, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...", refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(), sliderSetName.ToUTF8().data(), refShape.ToUTF8().data());
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
#ifdef USE_FBXSDK
		else if (fileName.Lower().EndsWith(".fbx"))
			error = project->ImportFBX(fileName.ToUTF8().data(), outfitName);
#endif
	}

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(80, _("Creating outfit..."));

	if (XRCCTRL(wiz, "npTexAuto", wxRadioButton)->GetValue() == false)
		project->SetTextures({XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data()});
	else
		project->SetTextures();

	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(90, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(90, 99);
	CreateSetSliders();

	if (!outfitName.empty())
		UpdateTitle();

	wxLogMessage("Project created.");
	UpdateProgress(100, _("Finished"));

	EndProgress();
}

void OutfitStudioFrame::OnLoadProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadProjectDialog(this,
								   _("Select a slider set to load"),
								   wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets",
								   wxEmptyString,
								   "Slider Set Files (*.osp;*.xml)|*.osp;*.xml",
								   wxFD_FILE_MUST_EXIST);
	if (loadProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	if (!CheckPendingChanges())
		return;

	std::string fileName{loadProjectDialog.GetPath().ToUTF8()};
	LoadProject(fileName);
}

void OutfitStudioFrame::OnAddProject(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog addProjectDialog(this,
								  _("Select a slider set to add"),
								  wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/SliderSets",
								  wxEmptyString,
								  "Slider Set Files (*.osp;*.xml)|*.osp;*.xml",
								  wxFD_FILE_MUST_EXIST);
	if (addProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	if (!CheckPendingChanges())
		return;

	std::string fileName{addProjectDialog.GetPath().ToUTF8()};

	// Choose the slider set to add before showing the options dialog
	std::string outfit = ChooseSliderSetName(fileName);
	if (outfit.empty())
		return;

	AddProjectDialog optionsDialog(this, OutfitStudioConfig);
	if (optionsDialog.ShowModal() != wxID_OK)
		return;

	const auto& options = optionsDialog.GetOptions();

	LoadProject(fileName, outfit, false, options.sliderDataLocal, options.appendNewSliders, options.setAsReference);
}

void OutfitStudioFrame::OnLoadReference(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	CloseBrushSettings();
	UpdateReferenceTemplates();

	wxDialog dlg;
	int result = wxID_CANCEL;
	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadRef", "wxDialog")) {
		XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(dlg, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, dlg, "LoadReference", "npTemplateChoice", refTemplates);

		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeSliders");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeZaps");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "LoadReference", "chkAppendNewSliders");

		dlg.Fit();
		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress(_("Loading reference..."));

	NiShape* baseShape = project->GetBaseShape();
	if (baseShape)
		glView->DeleteMesh(baseShape->name.get());

	UpdateProgress(10, _("Loading reference set..."));
	bool mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeSliders");
	bool mergeZaps = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkMergeZaps");
	bool appendNewSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "LoadReference", "chkAppendNewSliders");

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, dlg, "LoadReference", "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(ProjectUtil::GetProjectPath() + PathSepStr + tmpl->GetSource(),
													   tmpl->GetSetName(),
													   tmpl->GetShape(),
													   tmpl->GetLoadAll(),
													   mergeSliders,
													   mergeZaps,
													   appendNewSliders);
			else
				error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), mergeSliders, mergeZaps, appendNewSliders);
		}
		else
			error = 1;
	}
	else if (XRCCTRL(dlg, "npRefIsSliderset", wxRadioButton)->GetValue() == true) {
		wxString fileName = XRCCTRL(dlg, "npSliderSetFile", wxFilePickerCtrl)->GetPath();
		wxString refShape = XRCCTRL(dlg, "npRefShapeName", wxChoice)->GetStringSelection();

		if (fileName.EndsWith(".osp") || fileName.EndsWith(".xml")) {
			wxString sliderSetName = XRCCTRL(dlg, "npSliderSetName", wxChoice)->GetStringSelection();
			wxLogMessage("Loading reference '%s' from set '%s' of file '%s'...", refShape, sliderSetName, fileName);

			error = project->LoadReference(fileName.ToUTF8().data(), sliderSetName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders, mergeZaps, appendNewSliders);
		}
		else if (fileName.EndsWith(".nif")) {
			wxLogMessage("Loading reference '%s' from '%s'...", refShape, fileName);
			error = project->LoadReferenceNif(fileName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders, mergeZaps);
		}
	}
	else
		project->ClearReference();

	if (error) {
		EndProgress();
		RefreshGUIFromProj();
		return;
	}

	project->SetTextures(project->GetBaseShape());

	wxLogMessage("Creating reference...");
	UpdateProgress(60, _("Creating reference..."));
	RefreshGUIFromProj();

	wxLogMessage("Creating %zu slider(s)...", project->SliderCount());
	UpdateProgress(70, wxString::Format(_("Creating %zu slider(s)..."), project->SliderCount()));
	StartSubProgress(70, 99);
	CreateSetSliders();

	wxLogMessage("Reference loaded.");
	EndProgress();
}

void OutfitStudioFrame::OnConvertBodyReference(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	auto shapes = project->GetWorkNif()->GetShapes();
	auto baseShape = project->GetBaseShape();
	if (shapes.size() == 0 || (baseShape && shapes.size() == 1)) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	UpdateReferenceTemplates();

	ConvertBodyReferenceDialog dlg(this, project, OutfitStudioConfig, refTemplates);
	if (!dlg.Load())
		return;

	dlg.ConvertBodyReference();
}

void OutfitStudioFrame::OnRunAutomation(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	AutomationDialog dlg(this, project);
	dlg.ShowModal();
}


void OutfitStudioFrame::OnLoadOutfit(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	int result = wxID_CANCEL;

	CloseBrushSettings();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&dlg, this, "dlgLoadOutfit", "wxDialog")) {
		XRCCTRL(dlg, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);
		if (project->GetWorkNif()->IsValid())
			XRCCTRL(dlg, "npWorkAdd", wxCheckBox)->Enable();

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	std::string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue().ToStdString();

	menuBar->Enable(XRCID("fileSave"), false);

	wxLogMessage("Loading outfit...");
	StartProgress(_("Loading outfit..."));

	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			glView->DeleteMesh(s->name.get());
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
#ifdef USE_FBXSDK
		else if (fileName.Lower().EndsWith(".fbx"))
			ret = project->ImportFBX(fileName.ToUTF8().data(), outfitName);
#endif
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
		std::vector<std::string> texVec = {XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data()};
		project->SetTextures(texVec);
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	UpdateTitle();

	wxLogMessage("Outfit loaded.");
	EndProgress();
}

void OutfitStudioFrame::OnUnloadProject(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Unload the project? All unsaved changes will be lost"), _("Unload Project"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Unload"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxLogMessage("Unloading project...");
	menuBar->Enable(XRCID("fileSave"), false);

	ResetProject();

	ShutdownPhysics();
	delete project;
	project = new OutfitProject(this);

	CreateSetSliders();
	RefreshGUIFromProj(false);
	glView->Render();

	statusBar->SetStatusText(_("Ready!"));
}

void OutfitStudioFrame::ResetProject() {
	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->GetUndoHistory()->ClearHistory();

	activeSlider.clear();
	lastActiveSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	CreateSetSliders();
	RefreshGUIFromProj(false);
}

void OutfitStudioFrame::UpdateReferenceTemplates() {
	refTemplates.clear();

	std::string fileName = ProjectUtil::GetProjectPath() + "/RefTemplates.xml";
	if (wxFileName::IsFileReadable(fileName)) {
		RefTemplateFile refTemplateFile(fileName);
		refTemplateFile.GetAll(refTemplates);
	}

	RefTemplateCollection refTemplateCol;
	refTemplateCol.Load(ProjectUtil::GetProjectPath() + "/RefTemplates");
	refTemplateCol.GetAll(refTemplates);
}

void OutfitStudioFrame::ClearProject() {
	if (editUV)
		editUV->Close();

	for (auto& s : project->GetWorkNif()->GetShapeNames())
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

	glView->gls.ClearOverlays();
	activePartition.Unset();
	activeSegment.Unset();

	lastCheckedSlider.clear();
	lastSelectedBones.clear();
	lastNormalizeBones.clear();

	if (currentTabButton)
		currentTabButton->SetPendingChanges(false);

	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	cMaskName->Clear();

	auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	cPoseName->Clear();
	cPoseName->Append("<New>", (void*)nullptr);
	cPoseName->SetStringSelection("<New>");

	ResetAnimationList();

	if (projectNotes && notesPane) {
		projectNotes->Clear();
		notesPane->Collapse();

		UpdateToolScrollLayout();
	}

	project->outfitName.clear();
	pendingChanges = false;
	UpdateTitle();
}

void OutfitStudioFrame::RenameProject(const std::string& projectName) {
	project->outfitName = projectName;
	if (outfitRoot.IsOk())
		outfitShapes->SetItemText(outfitRoot, wxString::FromUTF8(projectName));

	UpdateTitle();
}

void OutfitStudioFrame::LockShapeSelect() {
	selectionLocked = true;
	outfitShapes->Disable();
}

void OutfitStudioFrame::UnlockShapeSelect() {
	selectionLocked = false;
	outfitShapes->Enable();
}

void OutfitStudioFrame::RefreshGUIFromProj(bool render, bool stashMasks) {
	LockShapeSelect();

	selectedItems.clear();
	std::vector<ShapeItemState> prevStates;

	if (outfitRoot.IsOk()) {
		// Collect names from tree items using the tree label text (safe even if NiShape* is dangling)
		wxTreeItemIdValue cookie;
		wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
		while (child.IsOk()) {
			auto itemData = (ShapeItemData*)outfitShapes->GetItemData(child);
			if (itemData) {
				ShapeItemState prevState{};
				prevState.shapeName = outfitShapes->GetItemText(child).ToUTF8().data();
				prevState.state = outfitShapes->GetItemState(child);
				prevState.refProjectFile = itemData->GetRefProjectFile();
				prevState.refProjectName = itemData->GetRefProjectName();

				if (outfitShapes->IsSelected(child))
					prevState.selected = true;

				prevStates.push_back(prevState);
			}

			child = outfitShapes->GetNextChild(outfitRoot, cookie);
		}

		outfitShapes->UnselectAll();
		outfitShapes->DeleteChildren(outfitRoot);
		outfitShapes->Delete(outfitRoot);
		outfitRoot.Unset();
	}

	auto shapes = project->GetWorkNif()->GetShapes();
	if (shapes.size() > 0) {
		if (shapes.size() == 1 && project->IsBaseShape(shapes.front()))
			outfitRoot = outfitShapes->AppendItem(shapesRoot, "Reference Only");
		else
			outfitRoot = outfitShapes->AppendItem(shapesRoot, wxString::FromUTF8(project->OutfitName()));
	}

	wxTreeItemId item;
	wxTreeItemId firstItem;
	wxTreeItemId prevFirstSelItem;
	for (auto& shape : shapes) {
		auto itemData = new ShapeItemData(shape);
		item = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(shape->name.get()));
		outfitShapes->SetItemState(item, 0);
		outfitShapes->SetItemData(item, itemData);

		if (project->IsBaseShape(shape))
			itemData->SetRefSource(project->GetReferenceProjectFile(), project->GetReferenceProjectName());

		if (project->IsBaseShape(shape)) {
			outfitShapes->SetItemBold(item);
			outfitShapes->SetItemTextColour(item, wxColour(0, 255, 0));
		}

		auto it = std::find_if(prevStates.begin(), prevStates.end(), [&shape](const ShapeItemState& state) { return state.shapeName == shape->name.get(); });

		if (it != prevStates.end()) {
			outfitShapes->SetItemState(item, it->state);
			itemData->SetRefSource(it->refProjectFile, it->refProjectName);

			if (it->selected) {
				outfitShapes->SelectItem(item);
				selectedItems.push_back(itemData);

				if (!prevFirstSelItem.IsOk())
					prevFirstSelItem = item;
			}
		}

		if (!firstItem.IsOk())
			firstItem = item;
	}

	UnlockShapeSelect();

	if (prevFirstSelItem.IsOk())
		activeItem = (ShapeItemData*)outfitShapes->GetItemData(prevFirstSelItem);
	else if (firstItem.IsOk())
		outfitShapes->SelectItem(firstItem);
	else
		activeItem = nullptr;

	outfitShapes->ExpandAll();

	if (stashMasks) {
		auto maskStash = glView->StashMasks();
		MeshesFromProj();
		glView->UnstashMasks(maskStash);
	}
	else
		MeshesFromProj();

	UpdateAnimationGUI();

	if (outfitRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
		while (child.IsOk()) {
			bool vis = true;
			bool ghost = false;

			int state = outfitShapes->GetItemState(child);
			switch (state) {
				case 1:
					vis = false;
					ghost = false;
					break;
				case 2:
					vis = true;
					ghost = true;
					break;
				default:
					vis = true;
					ghost = false;
					break;
			}

			std::string shapeName{outfitShapes->GetItemText(child).ToUTF8()};
			glView->SetShapeGhostMode(shapeName, ghost);
			glView->ShowShape(shapeName, vis);
			child = outfitShapes->GetNextChild(outfitRoot, cookie);
		}
	}

	UpdateUndoTools();
	glView->UpdateFloor();

	if (render)
		glView->Render();
}

void OutfitStudioFrame::UpdateAnimationGUI() {
	// Preserve x-mirror and pose bones
	int xMChoice = cXMirrorBone->GetSelection();
	std::string manualXMirrorBone;
	if (xMChoice >= 2)
		manualXMirrorBone = cXMirrorBone->GetString(xMChoice);

	std::string poseBone;
	int poseBoneSel = cPoseBone->GetSelection();
	if (poseBoneSel != wxNOT_FOUND)
		poseBone = cPoseBone->GetString(poseBoneSel).ToStdString();

	if (lastSelectedBones.count(activeBone) == 0)
		activeBone.clear();

	UpdateBoneTree();

	cXMirrorBone->Freeze();
	cXMirrorBone->Clear();
	cXMirrorBone->AppendString("None");
	cXMirrorBone->AppendString("Auto");
	cXMirrorBone->SetSelection(xMChoice == 1 ? 1 : 0);

	cPoseBone->Freeze();
	cPoseBone->Clear();

	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	wxArrayString activeBonesArr;
	for (auto& bone : activeBones)
		activeBonesArr.Add(wxString::FromUTF8(bone));

	cXMirrorBone->Append(activeBonesArr);
	cPoseBone->Append(activeBonesArr);

	// Re-fill mirror and pose bone lists
	for (auto& bone : activeBones) {
		if (xMChoice >= 2 && bone == manualXMirrorBone)
			cXMirrorBone->SetSelection(cXMirrorBone->GetCount() - 1);

		if (poseBone == bone)
			cPoseBone->SetSelection(cPoseBone->GetCount() - 1);
	}

	CalcAutoXMirrorBone();

	if (cPoseBone->GetSelection() == wxNOT_FOUND && cPoseBone->GetCount() > 0)
		cPoseBone->SetSelection(0);

	cXMirrorBone->Thaw();
	cPoseBone->Thaw();

	auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	cPoseName->Clear();

	std::string poseDataPath = ProjectUtil::GetProjectPath() + "/PoseData";
	poseDataCollection.LoadData(poseDataPath);

	// Additionally load community-supplied ScreenArcherMenu/SAF poses if the
	// target game ships them in a known location:
	//   Skyrim SE/VR: Data\SAM\Poses\*.yaml
	//   Fallout 4/VR: Data\F4SE\Plugins\SAF\Poses\*.json
	const TargetGame samGame = wxGetApp().targetGame;
	wxString samRelDir;
	bool samUsesJson = false;
	switch (samGame) {
	case SKYRIMSE:
	case SKYRIMVR:
		samRelDir = wxString("SAM") + PathSepChar + "Poses";
		break;
	case FO4:
	case FO4VR:
		samRelDir = wxString("F4SE") + PathSepChar + "Plugins" + PathSepChar + "SAF" + PathSepChar + "Poses";
		samUsesJson = true;
		break;
	default:
		break;
	}

	if (!samRelDir.IsEmpty()) {
		wxString gameDataPath = GameUtil::GetGameDataPath(samGame);
		if (!gameDataPath.IsEmpty()) {
			if (!gameDataPath.EndsWith(PathSepChar))
				gameDataPath.Append(PathSepChar);
			wxString samDir = gameDataPath + samRelDir;
			if (wxDirExists(samDir)) {
				std::string utf8Dir(samDir.ToUTF8().data());
				if (samUsesJson)
					poseDataCollection.LoadJsonData(utf8Dir, "SAM: ");
				else
					poseDataCollection.LoadYamlData(utf8Dir, "SAM: ");
			}
		}
	}

	for (auto& poseData : poseDataCollection.poseData) {
		wxString poseName = wxString::FromUTF8(poseData.name);
		cPoseName->Append(poseName, &poseData);
	}

	// Dummy sentinel entry representing a clean/empty pose.
	cPoseName->Append("<New>", (void*)nullptr);
	cPoseName->SetStringSelection("<New>");

	// Reflect the read-only state of the initially selected pose (if any).
	UpdatePoseButtonStates();

	RefreshGUIWeightColors();
	PoseToGUI();
	UpdatePhysicsControlsVisibility();

	glView->UpdateNodes();
	glView->UpdateBones();
}

void OutfitStudioFrame::UpdateBoneItemState(const wxTreeItemId& item, const std::string& boneName) {
	bool badBone = false;
	if (activeItem) {
		auto* shape = activeItem->GetShape();
		if (shape && project->GetWorkNif()->IsValid() && project->GetWorkNif()->GetBlockID(shape) != nifly::NIF_NPOS)
			badBone = project->GetWorkAnim()->BoneHasInconsistentTransforms(shape->name.get(), boneName);
	}
	outfitBones->SetItemState(item, (lastNormalizeBones.count(boneName) != 0 ? 1 : 0) + (badBone ? 2 : 0));
}

void OutfitStudioFrame::UpdateBoneTree() {
	bool saveRUI = recursingUI;
	recursingUI = true;

	outfitBones->Freeze();

	// Clear bone tree
	if (outfitBones->GetChildrenCount(bonesRoot) > 0)
		outfitBones->DeleteChildren(bonesRoot);

	wxString filterStr = bonesFilter->GetValue();
	filterStr.MakeLower();

	// Refill bone tree, re-setting normalize state and re-selecting bones
	std::vector<std::string> activeBones;
	project->GetActiveBones(activeBones);

	for (auto& bone : activeBones) {
		// Filter out bone by name
		wxString boneStr = wxString::FromUTF8(bone);
		if (!boneStr.Lower().Contains(filterStr))
			continue;

		wxTreeItemId item = outfitBones->AppendItem(bonesRoot, boneStr);
		UpdateBoneItemState(item, bone);

		if (lastSelectedBones.count(bone) != 0) {
			outfitBones->SelectItem(item);
			if (activeBone.empty())
				activeBone = bone;
		}
	}

	outfitBones->Thaw();

	recursingUI = saveRUI;

	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
}

void OutfitStudioFrame::MeshesFromProj(const bool reloadTextures) {
	glView->gls.DeleteOverlay("refineErrorEdges");

	for (auto& shape : project->GetWorkNif()->GetShapes())
		MeshFromProj(shape, reloadTextures);

	// Every mesh has been through SetMeshTextures by now, so whether any of them is a Complex
	// Material has been decided and the environment can be turned on for them.
	ApplyAutoHDRiBackground();

	if (glView->GetVertexEdit())
		glView->ShowVertexEdit();
}

void OutfitStudioFrame::MeshFromProj(NiShape* shape, const bool reloadTextures) {
	if (editUV)
		editUV->Close();

	if (extInitialized) {
		glView->DeleteMesh(shape->name.get());
		glView->AddMeshFromNif(project->GetWorkNif(), shape->name.get());

		MaterialFile matFile;
		bool hasMatFile = project->GetShapeMaterialFile(shape, matFile);
		glView->SetMeshTextures(shape->name.get(), project->GetShapeTextures(shape), hasMatFile, matFile, reloadTextures);

		UpdateMeshFromSet(shape);
	}

	std::vector<std::string> selShapes;
	for (auto& i : selectedItems)
		selShapes.push_back(i->GetShape()->name.get());

	glView->SetActiveShapes(selShapes);

	if (activeItem)
		glView->SetSelectedShape(activeItem->GetShape()->name.get());
	else
		glView->SetSelectedShape("");
}

void OutfitStudioFrame::UpdateMeshFromSet(NiShape* shape) {
	bool disableNormalsCalc = OutfitStudioConfig.GetBoolValue("DisableNormalsCalc");

	std::string shapeName = shape->name.get();
	Mesh* m = glView->GetMesh(shapeName);
	if (m) {
		m->smoothSeamNormals = project->activeSet.GetSmoothSeamNormals(shapeName);
		m->smoothSeamNormalsAngle = project->activeSet.GetSmoothSeamNormalsAngle(shapeName);
		m->lockNormals = disableNormalsCalc || project->activeSet.GetLockNormals(shapeName);
	}
}

void OutfitStudioFrame::FillVertexColors() {
	std::vector<std::string> shapeNames = project->GetWorkNif()->GetShapeNames();

	for (auto& s : shapeNames) {
		Mesh* m = glView->GetMesh(s);
		if (!m)
			continue;

		m->ColorFill(Vector3(1.0f, 1.0f, 1.0f));
		m->AlphaFill(1.0f);

		const std::vector<Color4>* vcolors = project->GetWorkNif()->GetColorsForShape(s);
		if (vcolors) {
			for (size_t v = 0; v < vcolors->size(); v++) {
				m->vcolors[v].x = vcolors->at(v).r;
				m->vcolors[v].y = vcolors->at(v).g;
				m->vcolors[v].z = vcolors->at(v).b;
				m->valpha[v] = vcolors->at(v).a;
			}
		}
	}
}

bool OutfitStudioFrame::ShapeSelectionCheck() {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return false;
	}
	return true;
}

void OutfitStudioFrame::OnSSSNameCopy(wxCommandEvent& event) {
	wxWindow* win = ((wxButton*)event.GetEventObject())->GetParent();
	std::string copyStr{XRCCTRL(*win, "sssName", wxTextCtrl)->GetValue().ToUTF8()};

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

void OutfitStudioFrame::OnSaveSliderSet(wxCommandEvent& WXUNUSED(event)) {
	SaveProject();
}

void OutfitStudioFrame::OnSaveSliderSetAs(wxCommandEvent& WXUNUSED(event)) {
	SaveProjectAs();
}

void OutfitStudioFrame::OnSetBaseShape(wxCommandEvent& WXUNUSED(event)) {
	wxLogMessage("Setting new base shape.");
	SetBaseShape();
}

void OutfitStudioFrame::SetBaseShape() {
	for (auto& s : project->GetWorkNif()->GetShapes())
		UpdateShapeSource(s);

	ZeroSliders();
	if (!activeSlider.empty()) {
		bEditSlider = false;
		auto it = sliderPanels.find(activeSlider);
		if (it != sliderPanels.end() && it->second)
			it->second->slider->SetFocus();
		HighlightSlider("");
		activeSlider.clear();
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

	for (auto& fileName : fileNames)
		project->ImportNIF(fileName.ToUTF8().data(), false);

	UpdateProgress(60, _("Refreshing GUI..."));
	project->SetTextures();

	SetPendingChanges();
	RefreshGUIFromProj();

	UpdateTitle();
	EndProgress();
}

std::optional<bool> OutfitStudioFrame::PromptStarfieldGeometryMode() {
	auto* nif = project->GetWorkNif();
	if (!nif->GetHeader().GetVersion().IsSF())
		return std::nullopt;

	int result = wxMessageBox(
		_("Starfield supports two modes for mesh geometry data:\n\n"
		  "Internal: Mesh data is embedded directly in the NIF file.\n"
		  "Simpler for modding - single file, no external dependencies.\n\n"
		  "External: Mesh data is stored in separate .mesh files under geometries/.\n"
		  "Can be streamed from BA2 archives for better game performance.\n\n"
		  "Would you like to embed the geometry data in the NIF (internal)?\n"
		  "Choose 'Yes' for internal or 'No' for external."),
		_("Starfield Geometry Mode"),
		wxYES_NO | wxICON_QUESTION,
		this);

	return (result == wxYES);
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

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			Mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	auto useInternalGeom = PromptStarfieldGeometryMode();

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes, false, useInternalGeom);
	if (error) {
		wxLogError("Failed to save NIF file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s'!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportNIFWithRef(wxCommandEvent& event) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (!project->GetBaseShape()) {
		OnExportNIF(event);
		return;
	}

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export project NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project with reference to NIF file '%s'...", fileName);

	std::vector<Mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		Mesh* m = glView->GetMesh(s);
		if (m)
			shapeMeshes.push_back(m);
	}

	auto useInternalGeom = PromptStarfieldGeometryMode();

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes, true, useInternalGeom);
	if (error) {
		wxLogError("Failed to save NIF file '%s' with reference!", fileName);
		wxMessageBox(wxString::Format(_("Failed to save NIF file '%s' with reference!"), fileName), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (HasUnweightedCheck())
		return;

	wxString fileName = wxFileSelector(_("Export selected shapes to NIF"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	std::vector<std::string> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape()->name.get());

	wxLogMessage("Exporting selected shapes to NIF file '%s'.", fileName);

	auto useInternalGeom = PromptStarfieldGeometryMode();

	if (project->ExportShapeNIF(fileName.ToUTF8().data(), shapes, useInternalGeom)) {
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

	for (auto& fileName : fileNames) {
		wxLogMessage("Importing shape(s) from OBJ file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = project->ImportOBJ(fileName.ToUTF8().data(), project->OutfitName(), activeItem->GetShape());
		else
			ret = project->ImportOBJ(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from OBJ file '%s'.", activeItem->GetShape()->name.get(), fileName);
	}

	RefreshGUIFromProj(false);
	SetPendingChanges();

	wxLogMessage("Imported shape(s) from OBJ.");
	glView->Render();
}

void OutfitStudioFrame::OnExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	bool hasSkinTrans = false;
	for (NiShape* shape : project->GetWorkNif()->GetShapes()) {
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the OBJ?  (This is not recommended.)"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	wxString fileName = wxFileSelector(_("Export project as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);

	if (project->ExportOBJ(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
		wxLogError("Failed to export OBJ file '%s'!", fileName);
		wxMessageBox(_("Failed to export OBJ file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the OBJ?"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	if (selectedItems.size() > 1) {
		wxString fileName
			= wxFileSelector(_("Export selected shapes as an .obj file"), wxEmptyString, wxEmptyString, ".obj", "OBJ Files (*.obj)|*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as OBJ file to '%s'.", fileName);

		std::vector<NiShape*> shapes;
		shapes.reserve(selectedItems.size());
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .obj file"),
										   wxEmptyString,
										   wxString(activeItem->GetShape()->name.get() + ".obj"),
										   ".obj",
										   "OBJ Files (*.obj)|*.obj",
										   wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
										   this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as OBJ file to '%s'.", activeItem->GetShape()->name.get(), fileName);

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
			wxLogError("Failed to export OBJ file '%s'!", fileName);
			wxMessageBox(_("Failed to export OBJ file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnImportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	wxFileDialog importDialog(this, _("Import .fbx file for new shape"), wxEmptyString, wxEmptyString, "FBX Files (*.fbx)|*.fbx", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	for (auto& fileName : fileNames) {
		wxLogMessage("Importing shape(s) from FBX file '%s'...", fileName);

		int ret;
		if (activeItem)
			ret = project->ImportFBX(fileName.ToUTF8().data(), project->OutfitName(), activeItem->GetShape());
		else
			ret = project->ImportFBX(fileName.ToUTF8().data(), project->OutfitName());

		if (ret == 101)
			wxLogMessage("Updated shape '%s' from FBX file '%s'.", activeItem->GetShape()->name.get(), fileName);
	}

	RefreshGUIFromProj(false);
	SetPendingChanges();

	wxLogMessage("Imported shape(s) from FBX.");
	glView->Render();
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	bool hasSkinTrans = false;
	for (NiShape* shape : project->GetWorkNif()->GetShapes()) {
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the FBX?  (This is not recommended.)"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	wxString fileName = wxFileSelector(_("Export project as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fileName.IsEmpty())
		return;

	wxLogMessage("Exporting project to OBJ file '%s'...", fileName);

	if (!project->ExportFBX(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal)) {
		wxLogError("Failed to export FBX file '%s'!", fileName);
		wxMessageBox(_("Failed to export FBX file!"), _("Export Error"), wxICON_ERROR);
	}
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnExportShapeFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!ShapeSelectionCheck())
		return;

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->GetTransformGlobalToShape(shape).IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
								 "global coordinates in the FBX?"),
							   _("Transform to global"),
							   wxYES_NO | wxCANCEL);
		if (res == wxCANCEL)
			return;
		transToGlobal = (res == wxYES);
	}

	if (selectedItems.size() > 1) {
		wxString fileName
			= wxFileSelector(_("Export selected shapes as an .fbx file"), wxEmptyString, wxEmptyString, ".fbx", "FBX Files (*.fbx)|*.fbx", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting selected shapes as FBX file to '%s'.", fileName);

		std::vector<NiShape*> shapes;
		shapes.reserve(selectedItems.size());
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes, transToGlobal)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
	else {
		wxString fileName = wxFileSelector(_("Export shape as an .fbx file"),
										   wxEmptyString,
										   wxString(activeItem->GetShape()->name.get() + ".fbx"),
										   ".fbx",
										   "FBX Files (*.fbx)|*.fbx",
										   wxFD_SAVE | wxFD_OVERWRITE_PROMPT,
										   this);
		if (fileName.IsEmpty())
			return;

		wxLogMessage("Exporting shape '%s' as FBX file to '%s'.", activeItem->GetShape()->name.get(), fileName);

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes, transToGlobal)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnImportTRIHead(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog importDialog(this, _("Import .tri morphs"), wxEmptyString, wxEmptyString, "TRI (Head) Files (*.tri)|*.tri", wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (importDialog.ShowModal() == wxID_CANCEL)
		return;

	wxArrayString fileNames;
	importDialog.GetPaths(fileNames);

	sliderScroll->Freeze();
	MenuExitSliderEdit();
	sliderScroll->FitInside();
	activeSlider.clear();

	for (auto& fn : fileNames) {
		wxFileName fileName(fn);
		wxLogMessage("Importing morphs from TRI (head) file '%s'...", fn);

		std::string shapeName{fileName.GetName().ToUTF8()};
		while (project->IsValidShape(shapeName)) {
			std::string result{wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"), shapeName, this).ToUTF8()};
			if (result.empty())
				continue;

			shapeName = std::move(result);
		}

		std::vector<std::string> newSliders;
		if (!project->ImportHeadTRI(fn.ToUTF8().data(), shapeName, true, &newSliders)) {
			wxLogError("Failed to load TRI file '%s'!", fn);
			wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
			return;
		}

		RefreshGUIFromProj(false);

		for (auto& sliderName : newSliders)
			createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();

	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnExportTRIHead(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString dir = wxDirSelector(_("Export .tri morphs"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
	if (dir.IsEmpty())
		return;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::string fn = dir.ToStdString() + PathSepStr + shape->name.get() + ".tri";

		wxLogMessage("Exporting TRI (head) morphs of '%s' to '%s'...", shape->name.get(), fn);
		if (!project->WriteHeadTRI(shape, fn)) {
			wxLogError("Failed to export TRI file to '%s'!", fn);
			wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
		}
	}
}

void OutfitStudioFrame::OnExportShapeTRIHead(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Export .tri morphs"), wxEmptyString, wxEmptyString, ".tri", "*.tri", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting TRI (head) morphs to '%s'...", fn);
	if (!project->WriteHeadTRI(activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Failed to export TRI file to '%s'!", fn);
		wxMessageBox(_("Failed to export TRI file!"), _("Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnImportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	wxString fileName = wxFileSelector(_("Import physics data to project"), wxEmptyString, wxEmptyString, ".hkx", "*.hkx", wxFD_FILE_MUST_EXIST, this);
	if (fileName.IsEmpty())
		return;

	auto physicsBlock = std::make_unique<BSClothExtraData>();
	if (!physicsBlock->FromHKX(fileName.ToUTF8().data())) {
		wxLogError("Failed to import physics data file '%s'!", fileName);
		wxMessageBox(wxString::Format(_("Failed to import physics data file '%s'!"), fileName), _("Import Error"), wxICON_ERROR);
	}

	auto& physicsData = project->GetClothData();
	physicsData[fileName.ToUTF8().data()] = std::move(physicsBlock);

	SetPendingChanges();
}

void OutfitStudioFrame::OnExportPhysicsData(wxCommandEvent& WXUNUSED(event)) {
	auto& physicsData = project->GetClothData();
	if (physicsData.empty()) {
		wxMessageBox(_("There is no physics data loaded!"), _("Info"), wxICON_INFORMATION);
		return;
	}

	wxArrayString fileNames;
	for (auto& data : physicsData)
		fileNames.Add(wxString::FromUTF8(data.first));

	wxSingleChoiceDialog physicsDataChoice(this, _("Please choose the physics data source you want to export."), _("Choose physics data"), fileNames);
	if (physicsDataChoice.ShowModal() == wxID_CANCEL)
		return;

	int sel = physicsDataChoice.GetSelection();
	std::string selString{fileNames[sel].ToUTF8()};

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
	snprintf(thename, 256, "%s", namebase.c_str());
	int count = 1;
	while (sliderPanels.find(thename) != sliderPanels.end())
		snprintf(thename, 256, "%s%d", namebase.c_str(), count++);

	std::string finalName{
		wxGetTextFromUser(_("Create a conversion slider for the current slider settings with the following name: "), _("Create New Conversion Slider"), thename, this).ToUTF8()};
	if (finalName.empty())
		return;

	wxLogMessage("Creating new conversion slider '%s'...", finalName);

	project->AddCombinedSlider(finalName);

	auto baseShape = project->GetBaseShape();
	if (baseShape) {
		Mesh* m = glView->GetMesh(baseShape->name.get());
		if (m)
			project->UpdateShapeFromMesh(baseShape, m);

		project->NegateSlider(finalName, baseShape);
	}

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);
	for (auto& s : sliderList) {
		if (!s.compare(finalName))
			continue;
		project->DeleteSlider(s);
	}

	glView->GetUndoHistory()->ClearHistory();
	UpdateUndoTools();

	activeSlider.clear();
	lastActiveSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	CreateSetSliders();
}

void OutfitStudioFrame::OnSelectSliders(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	for (auto& sliderPanel : sliderPanels)
		ShowSliderEffect(sliderPanel.first, checked);

	ApplySliders();
}

void OutfitStudioFrame::OnSliderFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	DoFilterSliders();
}

void OutfitStudioFrame::DoFilterSliders() {
	wxString filterStr = sliderFilter->GetValue();
	filterStr.MakeLower();

	for (auto& sliderPanel : sliderPanels) {
		if (!sliderPanel.second)
			continue;

		// Filter slider by name
		wxString sliderStr = wxString::FromUTF8(sliderPanel.first);
		if (sliderStr.Lower().Contains(filterStr)) {
			if (!sliderPanel.second->IsShown())
				sliderPanel.second->Show();
		}
		else {
			if (sliderPanel.second->IsShown())
				sliderPanel.second->Hide();
		}
	}

	sliderScroll->FitInside();
}

void OutfitStudioFrame::OnBonesFilterChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateBoneTree();
}

void OutfitStudioFrame::OnFixedWeight(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bFixedWeight = checked;
}

void OutfitStudioFrame::OnCBNormalizeWeights(wxCommandEvent& event) {
	bool checked = event.IsChecked();
	TB_Weight* weightBrush = dynamic_cast<TB_Weight*>(glView->GetActiveBrush());
	if (weightBrush)
		weightBrush->bNormalizeWeights = checked;
}

void OutfitStudioFrame::ToggleVisibility(wxTreeItemId firstItem) {
	bool vis = true;
	bool ghost = false;
	int state = 0;

	if (!firstItem.IsOk()) {
		if (!selectedItems.empty()) {
			firstItem = selectedItems.front()->GetId();
		}
	}

	if (firstItem.IsOk()) {
		state = outfitShapes->GetItemState(firstItem);
		switch (state) {
			case 0:
				vis = false;
				ghost = false;
				state = 1;
				break;
			case 1:
				vis = true;
				ghost = true;
				state = 2;
				break;
			default:
				vis = true;
				ghost = false;
				state = 0;
				break;
		}

		std::string shapeName{outfitShapes->GetItemText(firstItem).ToUTF8()};
		glView->SetShapeGhostMode(shapeName, ghost);
		glView->ShowShape(shapeName, vis);
		outfitShapes->SetItemState(firstItem, state);
	}

	if (selectedItems.size() > 1) {
		for (auto& i : selectedItems) {
			if (i->GetId().GetID() != firstItem.GetID()) {
				std::string shapeName{outfitShapes->GetItemText(i->GetId()).ToUTF8()};
				glView->SetShapeGhostMode(shapeName, ghost);
				glView->ShowShape(shapeName, vis);
				outfitShapes->SetItemState(i->GetId(), state);
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnShapeVisToggle(wxTreeEvent& event) {
	ToggleVisibility(event.GetItem());
	event.Skip();
}

void OutfitStudioFrame::OnShapeSelect(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk()) {
		event.Veto();
		return;
	}

	if (selectionLocked) {
		event.Veto();
		return;
	}

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

	for (auto& i : selected) {
		if (outfitShapes->GetItemParent(i).IsOk()) {
			auto data = (ShapeItemData*)outfitShapes->GetItemData(i);
			if (data) {
				shapeNames.push_back(data->GetShape()->name.get());
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
					shapeNames.push_back(data->GetShape()->name.get());
					selectedItems.push_back(data);

					if (!activeItem)
						activeItem = data;
				}
			}
		}
	}

	glView->SetActiveShapes(shapeNames);

	if (activeItem)
		glView->SetSelectedShape(activeItem->GetShape()->name.get());
	else
		glView->SetSelectedShape("");

	UpdateActiveShape();
}

void OutfitStudioFrame::OnShapeActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitShapes->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnShapeProperties(evt);
}

void OutfitStudioFrame::ToggleBoneState(wxTreeItemId item) {
	if (!item.IsOk())
		return;

	std::string boneName = outfitBones->GetItemText(item).ToStdString();
	if ((outfitBones->GetItemState(item) & 1) != 0)
		lastNormalizeBones.erase(boneName);
	else
		lastNormalizeBones.insert(boneName);
	UpdateBoneItemState(item, boneName);
}

void OutfitStudioFrame::OnBoneStateToggle(wxTreeEvent& event) {
	ToggleBoneState(event.GetItem());
	event.Skip();
}

void OutfitStudioFrame::RefreshGUIWeightColors() {
	// Clear weight color of all shapes
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		Mesh* m = glView->GetMesh(s);
		if (m)
			m->WeightFill(0.0f);
	}

	if (!activeBone.empty()) {
		// Show weights of selected shapes without reference
		for (auto& s : selectedItems) {
			if (!project->IsBaseShape(s->GetShape())) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(s->GetShape()->name.get(), activeBone);

				Mesh* m = glView->GetMesh(s->GetShape()->name.get());
				if (m) {
					m->WeightFill(0.0f);
					if (weights) {
						for (auto& bw : *weights)
							m->weight[bw.first] = bw.second;
					}
				}
			}
		}

		// Always show weights of reference shape
		NiShape* baseShape = project->GetBaseShape();
		if (baseShape) {
			auto weights = project->GetWorkAnim()->GetWeightsPtr(baseShape->name.get(), activeBone);

			Mesh* m = glView->GetMesh(baseShape->name.get());
			if (m) {
				m->WeightFill(0.0f);
				if (weights) {
					for (auto& bw : *weights)
						m->weight[bw.first] = bw.second;
				}
			}
		}
	}

	glView->Refresh();
}

void OutfitStudioFrame::OnBoneSelect(wxTreeEvent& event) {
	if (recursingUI)
		return;

	wxTreeItemId item = event.GetItem();
	if (!activeItem || !item.IsOk())
		return;

	wxArrayTreeItemIds selected;
	outfitBones->GetSelections(selected);

	activeBone.clear();
	std::string selBone = outfitBones->GetItemText(item).ToStdString();

	if (!outfitBones->IsSelected(item)) {
		if (!selected.IsEmpty()) {
			std::string frontBone = outfitBones->GetItemText(selected.front()).ToStdString();
			activeBone = frontBone;
		}
	}
	else {
		activeBone = selBone;
		if (selected.GetCount() == 1)
			lastSelectedBones.clear();
	}

	wxTreeItemIdValue cookie;
	wxTreeItemId itemTree = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (itemTree.IsOk()) {
		std::string boneName = outfitBones->GetItemText(itemTree).ToStdString();
		if (outfitBones->IsSelected(itemTree))
			lastSelectedBones.insert(boneName);
		else
			lastSelectedBones.erase(boneName);

		itemTree = outfitBones->GetNextChild(bonesRoot, cookie);
	}

	// Sync with pose panel choice if the active bone exists there
	if (!activeBone.empty() && cPoseBone) {
		wxString activeBoneStr = wxString::FromUTF8(activeBone);
		for (unsigned int i = 0; i < cPoseBone->GetCount(); ++i) {
			if (cPoseBone->GetString(i) == activeBoneStr) {
				cPoseBone->SetSelection(i);
				PoseToGUI();  // Update sliders and text values for the selected bone
				break;
			}
		}
	}

	glView->UpdateNodeColors();
	RefreshGUIWeightColors();
	CalcAutoXMirrorBone();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
}

void OutfitStudioFrame::OnBoneActivated(wxTreeEvent& event) {
	int hitFlags;
	outfitBones->HitTest(event.GetPoint(), hitFlags);

	if (hitFlags & wxTREE_HITTEST_ONITEMSTATEICON)
		return;

	wxCommandEvent evt;
	OnEditBone(evt);
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

void OutfitStudioFrame::OnShapeTreeMotion(wxMouseEvent& event) {
	event.Skip();

	if (!outfitShapes || !glView)
		return;

	if (!Config.GetBoolValue("Input/ShapeHoverHighlight")) {
		glView->ClearHoverHighlight();
		return;
	}

	const wxPoint mousePos = event.GetPosition();
	int flags = 0;
	wxTreeItemId item = outfitShapes->HitTest(mousePos, flags);

	bool onItemLabelText = false;
	if ((flags & wxTREE_HITTEST_ONITEMLABEL) != 0 && item.IsOk() && outfitShapes->GetItemParent(item).IsOk()) {
		wxRect labelRect;
		onItemLabelText = outfitShapes->GetBoundingRect(item, labelRect, true) && labelRect.Contains(mousePos);
	}

	if (onItemLabelText) {
		auto* data = dynamic_cast<ShapeItemData*>(outfitShapes->GetItemData(item));
		if (data && data->GetShape())
			glView->SetHoverHighlight(data->GetShape()->name.get());
		else
			glView->ClearHoverHighlight();
	}
	else {
		glView->ClearHoverHighlight();
	}
}

void OutfitStudioFrame::OnShapeTreeLeave(wxMouseEvent& event) {
	event.Skip();

	if (glView)
		glView->ClearHoverHighlight();
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
	wxTreeItemId movedItem = outfitShapes->InsertItem(outfitRoot, dropItem, activeItem->GetShape()->name.get());
	if (!movedItem.IsOk())
		return;

	// Set data
	auto dropData = new ShapeItemData(activeItem->GetShape());
	outfitShapes->SetItemState(movedItem, 0);
	outfitShapes->SetItemData(movedItem, dropData);
	if (project->IsBaseShape(dropData->GetShape())) {
		outfitShapes->SetItemBold(movedItem);
		outfitShapes->SetItemTextColour(movedItem, wxColour(0, 255, 0));
	}

	// Delete old item
	outfitShapes->Delete(activeItem->GetId());

	// Select new item
	outfitShapes->UnselectAll();
	outfitShapes->SelectItem(movedItem);

	// Keep NIF shape order in sync with the tree order so future GUI refreshes preserve user reordering.
	project->GetWorkNif()->SetShapeOrder(GetShapeList());
}

void OutfitStudioFrame::OnBoneContext(wxTreeEvent& WXUNUSED(event)) {
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneContext");
	if (menu) {
		if (!activeBone.empty() && activeItem && project->GetWorkAnim()->BoneHasInconsistentTransforms(activeItem->GetShape()->name.get(), activeBone)) {
			if (AnimSkeleton::getInstance().GetBonePtr(activeBone)->isStandardBone)
				menu->Enable(XRCID("setBoneNode"), false);
		}
		else
			menu->Destroy(XRCID("menuBadBone"));

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
}

void OutfitStudioFrame::OnSegmentVisToggle(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || !segmentTree->GetItemParent(item).IsOk()) {
		event.Skip();
		return;
	}

	int newState = segmentTree->GetItemState(item) == 1 ? 0 : 1;
	segmentTree->SetItemState(item, newState);

	if (segmentTree->GetItemParent(item) == segmentRoot) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(item, cookie);
		while (child.IsOk()) {
			segmentTree->SetItemState(child, newState);
			child = segmentTree->GetNextChild(item, cookie);
		}
	}
	else if (newState == 0) {
		wxTreeItemId parent = segmentTree->GetItemParent(item);
		if (parent.IsOk() && parent != segmentRoot)
			segmentTree->SetItemState(parent, 0);
	}

	Mesh* m = nullptr;
	if (activeItem && activeItem->GetShape())
		m = glView->GetMesh(activeItem->GetShape()->name.get());

	if (m) {
		if (m->subMeshes.empty())
			SetSubMeshesForPartitions(m, triSParts);

		ApplySegmentVisibility(m);
		glView->Render();
	}

	event.Skip();
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

int OutfitStudioFrame::CalcMaxSegPartID() {
	int maxid = -1;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData)
			maxid = std::max(maxid, segmentData->partID);
		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
		while (subChild.IsOk()) {
			SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData)
				maxid = std::max(maxid, subSegmentData->partID);
			subChild = segmentTree->GetNextChild(child, subCookie);
		}
		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}
	return maxid;
}

void OutfitStudioFrame::OnAddSegment(wxCommandEvent& WXUNUSED(event)) {
	int newPartID = CalcMaxSegPartID() + 1;
	wxTreeItemId newItem;
	if (!activeSegment.IsOk() || segmentTree->GetChildrenCount(segmentRoot) <= 0) {
		// The new segment is the only partition: assign all triangles.
		for (size_t i = 0; i < triSParts.size(); ++i)
			triSParts[i] = newPartID;

		newItem = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(newPartID));
	}
	else
		newItem = segmentTree->InsertItem(segmentRoot, activeSegment, "Segment", -1, -1, new SegmentItemData(newPartID));

	if (newItem.IsOk()) {
		segmentTree->SetItemState(newItem, 0);
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
	segmentTabButton->SetPendingChanges();
}

void OutfitStudioFrame::OnAddSubSegment(wxCommandEvent& WXUNUSED(event)) {
	int newPartID = CalcMaxSegPartID() + 1;
	wxTreeItemId newItem;
	wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
	if (parent == segmentRoot) {
		if (segmentTree->GetChildrenCount(activeSegment) <= 0) {
			// The new subsegment will be the only child: assign all of
			// the segment's triangles to it.
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData)
				for (size_t i = 0; i < triSParts.size(); ++i)
					if (triSParts[i] == segmentData->partID)
						triSParts[i] = newPartID;
		}

		newItem = segmentTree->PrependItem(activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(newPartID, 0, 0xFFFFFFFF));
	}
	else
		newItem = segmentTree->InsertItem(parent, activeSegment, "Sub Segment", -1, -1, new SubSegmentItemData(newPartID, 0, 0xFFFFFFFF));

	if (newItem.IsOk()) {
		segmentTree->SetItemState(newItem, 0);
		segmentTree->UnselectAll();
		segmentTree->SelectItem(newItem);
	}

	UpdateSegmentNames();
	segmentTabButton->SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteSegment(wxCommandEvent& WXUNUSED(event)) {
	SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (segmentData) {
		// Collect list of partition IDs that will be disappearing.
		std::vector<bool> oldPartIDs(CalcMaxSegPartID() + 1, false);
		oldPartIDs[segmentData->partID] = true;
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, cookie);
		while (child.IsOk()) {
			SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
			if (childData)
				oldPartIDs[childData->partID] = true;
			child = segmentTree->GetNextChild(activeSegment, cookie);
		}

		// Find a new partition to put triangles into.
		int newPartID = -1;
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);
		if (sibling.IsOk()) {
			SegmentItemData* siblingData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				newPartID = siblingData->partID;

			child = segmentTree->GetFirstChild(sibling, cookie);
			if (child.IsOk()) {
				SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childData)
					newPartID = childData->partID;
			}
		}

		// Assign triangles from old partitions to new partition.
		for (size_t i = 0; i < triSParts.size(); ++i)
			if (triSParts[i] >= 0 && triSParts[i] < static_cast<int>(oldPartIDs.size()) && oldPartIDs[triSParts[i]])
				triSParts[i] = newPartID;

		segmentTree->UnselectAll();
		segmentTree->Delete(activeSegment);
		if (sibling.IsOk())
			segmentTree->SelectItem(sibling);
		else
			activeSegment.Unset();
	}

	UpdateSegmentNames();
	segmentTabButton->SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteSubSegment(wxCommandEvent& WXUNUSED(event)) {
	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	wxTreeItemId newSelItem;
	if (subSegmentData) {
		int oldPartID = subSegmentData->partID, newPartID = -1;

		// Find a partition to assign triangles to.
		wxTreeItemId sibling = segmentTree->GetPrevSibling(activeSegment);
		if (!sibling.IsOk())
			sibling = segmentTree->GetNextSibling(activeSegment);

		if (sibling.IsOk()) {
			SubSegmentItemData* siblingData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(sibling));
			if (siblingData)
				newPartID = siblingData->partID;
			newSelItem = sibling;
		}
		else {
			wxTreeItemId parent = segmentTree->GetItemParent(activeSegment);
			if (parent.IsOk()) {
				SegmentItemData* parentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(parent));
				if (parentData)
					newPartID = parentData->partID;
			}
			newSelItem = parent;
		}

		// Assign triangles to new partition.
		for (size_t i = 0; i < triSParts.size(); ++i)
			if (triSParts[i] == oldPartID)
				triSParts[i] = newPartID;

		segmentTree->UnselectAll();
		segmentTree->Delete(activeSegment);
		segmentTree->SelectItem(newSelItem);
	}

	UpdateSegmentNames();
	segmentTabButton->SetPendingChanges();
}

void OutfitStudioFrame::UpdateActiveSlotID() {
	wxChoice* segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			subSegmentData->userSlotID = 0;

			if (segmentSlot->GetSelection() > 0) {
				wxString slotSel = segmentSlot->GetStringSelection().BeforeFirst(' ');
				if (slotSel.length() >= 1) {
					unsigned long slot = 0;
					slotSel.ToULong(&slot);

					if (slot > 0)
						subSegmentData->userSlotID = slot;
				}
			}

			UpdateSegmentNames();
			segmentTabButton->SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnSegmentSlotChanged(wxCommandEvent& WXUNUSED(event)) {
	UpdateActiveSlotID();
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

			subSegmentData->material = type;
			UpdateSegmentNames();
			segmentTabButton->SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnSegmentApply(wxCommandEvent& WXUNUSED(event)) {
	ApplySegments();
}

void OutfitStudioFrame::ApplySegments() {
	NifSegmentationInfo inf;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);

	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		if (segmentData) {
			inf.segs.emplace_back();
			NifSegmentInfo& seg = inf.segs.back();
			seg.partID = segmentData->partID;
			size_t childCount = segmentTree->GetChildrenCount(child);

			if (childCount > 0) {
				seg.subs.resize(childCount);
				int childInd = 0;

				wxTreeItemIdValue subCookie;
				wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
				while (subChild.IsOk()) {
					SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
					if (subSegmentData) {
						NifSubSegmentInfo& sub = seg.subs[childInd++];
						sub.partID = subSegmentData->partID;
						sub.userSlotID = subSegmentData->userSlotID;
						sub.material = subSegmentData->material;
						sub.extraData = subSegmentData->extraData;
					}

					subChild = segmentTree->GetNextChild(child, subCookie);
				}
			}
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	inf.ssfFile = segmentSSF->GetValue().ToStdString();

	project->GetWorkNif()->SetShapeSegments(activeItem->GetShape(), inf, triSParts);
	MeshFromProj(activeItem->GetShape());

	CreateSegmentTree(activeItem->GetShape());
	SetPendingChanges();
}

void OutfitStudioFrame::OnSegmentReset(wxCommandEvent& WXUNUSED(event)) {
	ResetSegments();
}

void OutfitStudioFrame::ResetSegments() {
	if (activeItem)
		CreateSegmentTree(activeItem->GetShape());
	else
		CreateSegmentTree(nullptr);
}

void OutfitStudioFrame::OnSegmentEditSSF(wxCommandEvent& WXUNUSED(event)) {
	auto segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");

	wxString result = wxGetTextFromUser(_("Please enter an SSF file path."), _("SSF File"), segmentSSF->GetValue());
	if (result.empty())
		return;

	segmentSSF->ChangeValue(result);
	segmentTabButton->SetPendingChanges();
}

void OutfitStudioFrame::CreateSegmentTree(NiShape* shape) {
	if (segmentTree->GetChildrenCount(segmentRoot) > 0) {
		triSParts.clear(); // DeleteChildren calls OnSegmentSelect
		segmentTree->DeleteChildren(segmentRoot);
	}

	segmentTabButton->SetPendingChanges(false);

	NifSegmentationInfo inf;
	if (project->GetWorkNif()->GetShapeSegments(shape, inf, triSParts)) {
		for (size_t i = 0; i < inf.segs.size(); i++) {
			wxTreeItemId segID = segmentTree->AppendItem(segmentRoot, "Segment", -1, -1, new SegmentItemData(inf.segs[i].partID));
			if (segID.IsOk()) {
				segmentTree->SetItemState(segID, 0);
				for (size_t j = 0; j < inf.segs[i].subs.size(); j++) {
					NifSubSegmentInfo& sub = inf.segs[i].subs[j];
					wxTreeItemId subID = segmentTree->AppendItem(segID, "Sub Segment", -1, -1, new SubSegmentItemData(sub.partID, sub.userSlotID, sub.material, sub.extraData));
					segmentTree->SetItemState(subID, 0);
				}
			}
		}
	}

	wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
	segmentSSF->ChangeValue(inf.ssfFile);

	UpdateSegmentNames();
	segmentTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	if (child.IsOk())
		segmentTree->SelectItem(child);
}

void OutfitStudioFrame::ApplySegmentVisibility(Mesh* m) {
	if (!m)
		return;

	if (m->subMeshesVisible.size() != m->subMeshes.size())
		m->subMeshesVisible.assign(m->subMeshes.size(), true);

	if (!segmentTree || !segmentRoot.IsOk())
		return;

	const auto setSubMeshVisible = [&](int partID, bool visible) {
		if (partID >= 0 && static_cast<size_t>(partID) < m->subMeshesVisible.size())
			m->subMeshesVisible[partID] = visible;
	};

	wxTreeItemIdValue cookie;
	wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
	while (child.IsOk()) {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(child));
		bool segmentVisible = segmentTree->GetItemState(child) != 1;
		if (segmentData)
			setSubMeshVisible(segmentData->partID, segmentVisible);

		wxTreeItemIdValue subCookie;
		wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
		while (subChild.IsOk()) {
			SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData)
				setSubMeshVisible(subSegmentData->partID, segmentVisible && segmentTree->GetItemState(subChild) != 1);

			subChild = segmentTree->GetNextChild(child, subCookie);
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
	}
}

void OutfitStudioFrame::ResetSegmentVisibility() {
	if (segmentTree && segmentRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = segmentTree->GetFirstChild(segmentRoot, cookie);
		while (child.IsOk()) {
			segmentTree->SetItemState(child, 0);

			wxTreeItemIdValue subCookie;
			wxTreeItemId subChild = segmentTree->GetFirstChild(child, subCookie);
			while (subChild.IsOk()) {
				segmentTree->SetItemState(subChild, 0);
				subChild = segmentTree->GetNextChild(child, subCookie);
			}

			child = segmentTree->GetNextChild(segmentRoot, cookie);
		}
	}

	if (glView) {
		for (auto& m : glView->gls.GetMeshes()) {
			if (m)
				m->subMeshesVisible.assign(m->subMeshes.size(), true);
		}
	}
}

bool OutfitStudioFrame::PaintSegmentPartitionTriangles(Mesh* hitMesh, int hitTri, const Vector3& hitPointModel, float radiusModel) {
	if (!hitMesh || !activeItem || !glView->GetSegmentMode())
		return false;

	auto shape = activeItem->GetShape();
	if (!shape || shape->name.get() != hitMesh->shapeName)
		return false;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	if (tris.empty() || hitTri < 0 || static_cast<size_t>(hitTri) >= tris.size())
		return false;

	std::vector<bool> paintTris(tris.size(), false);
	paintTris[hitTri] = true;

	float radiusMesh = hitMesh->TransformDistModelToMesh(radiusModel);
	if (radiusMesh > 0.0f) {
		Vector3 hitPointMesh = hitMesh->TransformPosModelToMesh(hitPointModel);
		for (size_t ti = 0; ti < tris.size(); ++ti) {
			const Triangle& t = tris[ti];
			Vector3 centroid = (hitMesh->verts[t.p1] + hitMesh->verts[t.p2] + hitMesh->verts[t.p3]) / 3.0f;
			if (centroid.DistanceTo(hitPointMesh) <= radiusMesh)
				paintTris[ti] = true;
		}
	}

	bool changed = false;

	if (triSParts.size() == tris.size() && activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				if (paintTris[ti] && triSParts[ti] != subSegmentData->partID) {
					triSParts[ti] = subSegmentData->partID;
					changed = true;
				}
			}
		}
		else {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData) {
				selPartIDs[segmentData->partID] = true;
				int destPartID = segmentData->partID;
				wxTreeItemIdValue subCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
				while (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						selPartIDs[childData->partID] = true;
						destPartID = childData->partID;
					}
					child = segmentTree->GetNextChild(activeSegment, subCookie);
				}

				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!paintTris[ti])
						continue;
					if (triSParts[ti] >= 0 && triSParts[ti] < static_cast<int>(selPartIDs.size()) && selPartIDs[triSParts[ti]])
						continue;
					if (triSParts[ti] != destPartID) {
						triSParts[ti] = destPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (triParts.size() == tris.size() && activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				if (paintTris[ti] && triParts[ti] != partitionData->index) {
					triParts[ti] = partitionData->index;
					changed = true;
				}
			}
		}
	}

	return changed;
}

bool OutfitStudioFrame::GrowShrinkSegmentPartitionSelection(bool grow) {
	if (!activeItem || !glView->GetSegmentMode())
		return false;

	auto shape = activeItem->GetShape();
	if (!shape)
		return false;

	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	if (tris.empty())
		return false;

	std::vector<std::vector<int>> triNeighbors(tris.size());
	std::unordered_map<uint64_t, int> edgeToTri;
	edgeToTri.reserve(tris.size() * 3);

	auto makeEdgeKey = [](int a, int b) -> uint64_t {
		uint32_t va = static_cast<uint32_t>(std::min(a, b));
		uint32_t vb = static_cast<uint32_t>(std::max(a, b));
		return (static_cast<uint64_t>(va) << 32) | vb;
	};

	auto connectEdge = [&](int triIndex, int v1, int v2) {
		uint64_t edgeKey = makeEdgeKey(v1, v2);
		auto it = edgeToTri.find(edgeKey);
		if (it == edgeToTri.end()) {
			edgeToTri.emplace(edgeKey, triIndex);
		}
		else {
			int otherTri = it->second;
			if (otherTri != triIndex) {
				triNeighbors[triIndex].push_back(otherTri);
				triNeighbors[otherTri].push_back(triIndex);
			}
		}
	};

	for (size_t ti = 0; ti < tris.size(); ++ti) {
		const Triangle& t = tris[ti];
		int triIndex = static_cast<int>(ti);
		connectEdge(triIndex, t.p1, t.p2);
		connectEdge(triIndex, t.p2, t.p3);
		connectEdge(triIndex, t.p3, t.p1);
	}

	bool changed = false;

	if (triSParts.size() == tris.size() && activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);
		int destPartID = -1;

		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			destPartID = subSegmentData->partID;
			selPartIDs[destPartID] = true;
		}
		else {
			SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
			if (segmentData) {
				selPartIDs[segmentData->partID] = true;
				destPartID = segmentData->partID;
				wxTreeItemIdValue subCookie;
				wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
				while (child.IsOk()) {
					SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
					if (childData) {
						selPartIDs[childData->partID] = true;
						destPartID = childData->partID;
					}
					child = segmentTree->GetNextChild(activeSegment, subCookie);
				}
			}
		}

		if (destPartID >= 0) {
			std::vector<bool> triSelected(tris.size(), false);
			for (size_t ti = 0; ti < tris.size(); ++ti) {
				int partID = triSParts[ti];
				if (partID >= 0 && partID < static_cast<int>(selPartIDs.size()) && selPartIDs[partID])
					triSelected[ti] = true;
			}

			if (grow) {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (triSelected[ti])
						continue;

					bool touchesSelection = false;
					for (int nTri : triNeighbors[ti]) {
						if (triSelected[nTri]) {
							touchesSelection = true;
							break;
						}
					}

					if (touchesSelection && triSParts[ti] != destPartID) {
						triSParts[ti] = destPartID;
						changed = true;
					}
				}
			}
			else {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!triSelected[ti])
						continue;

					std::unordered_map<int, int> neighborPartCounts;
					bool boundary = false;
					for (int nTri : triNeighbors[ti]) {
						if (!triSelected[nTri]) {
							boundary = true;
							int nPartID = triSParts[nTri];
							if (nPartID >= 0)
								neighborPartCounts[nPartID]++;
						}
					}

					if (!boundary || neighborPartCounts.empty())
						continue;

					int bestPartID = triSParts[ti];
					int bestCount = -1;
					for (const auto& kv : neighborPartCounts) {
						if (kv.second > bestCount) {
							bestPartID = kv.first;
							bestCount = kv.second;
						}
					}

					if (bestPartID != triSParts[ti]) {
						triSParts[ti] = bestPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (triParts.size() == tris.size() && activePartition.IsOk() && partitionTree->GetItemParent(activePartition).IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
		if (partitionData) {
			const int targetIndex = partitionData->index;
			std::vector<bool> triSelected(tris.size(), false);
			for (size_t ti = 0; ti < tris.size(); ++ti)
				triSelected[ti] = triParts[ti] == targetIndex;

			if (grow) {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (triSelected[ti])
						continue;

					bool touchesSelection = false;
					for (int nTri : triNeighbors[ti]) {
						if (triSelected[nTri]) {
							touchesSelection = true;
							break;
						}
					}

					if (touchesSelection && triParts[ti] != targetIndex) {
						triParts[ti] = targetIndex;
						changed = true;
					}
				}
			}
			else {
				for (size_t ti = 0; ti < tris.size(); ++ti) {
					if (!triSelected[ti])
						continue;

					std::unordered_map<int, int> neighborPartCounts;
					bool boundary = false;
					for (int nTri : triNeighbors[ti]) {
						if (!triSelected[nTri]) {
							boundary = true;
							int nPartID = triParts[nTri];
							if (nPartID >= 0)
								neighborPartCounts[nPartID]++;
						}
					}

					if (!boundary || neighborPartCounts.empty())
						continue;

					int bestPartID = triParts[ti];
					int bestCount = -1;
					for (const auto& kv : neighborPartCounts) {
						if (kv.second > bestCount) {
							bestPartID = kv.first;
							bestCount = kv.second;
						}
					}

					if (bestPartID != triParts[ti]) {
						triParts[ti] = bestPartID;
						changed = true;
					}
				}
			}
		}
	}

	if (changed && currentTabButton)
		currentTabButton->SetPendingChanges();

	return changed;
}

void OutfitStudioFrame::ShowSegment(const wxTreeItemId& item) {
	if (!activeItem || !glView->GetSegmentMode())
		return;

	wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
	segmentType->Disable();
	segmentType->SetSelection(0);

	wxChoice* segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");
	segmentSlot->Disable();
	segmentSlot->SetSelection(0);

	if (item.IsOk())
		activeSegment = item;

	if (!activeSegment.IsOk() || !segmentTree->GetItemParent(activeSegment).IsOk())
		return;

	// Get all triangles of the active shape
	std::vector<Triangle> tris;
	if (activeItem->GetShape())
		activeItem->GetShape()->GetTriangles(tris);
	if (triSParts.size() != tris.size())
		return;

	// selPartIDs will be true for selected item and its children.
	std::vector<bool> selPartIDs(CalcMaxSegPartID() + 1, false);

	SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
	if (subSegmentData) {
		// Active segment is a subsegment
		selPartIDs[subSegmentData->partID] = true;

		if (subSegmentData->material != 0xFFFFFFFF) {
			bool typeFound = false;
			auto typeHash = wxString::Format("0x%08x", subSegmentData->material);
			for (uint32_t i = 0; i < segmentType->GetCount(); i++) {
				auto typeString = segmentType->GetString(i);
				if (typeString.Contains(typeHash)) {
					segmentType->SetSelection((int)i);
					typeFound = true;
					break;
				}
			}

			if (!typeFound)
				segmentType->SetSelection(segmentType->Append(typeHash));
		}

		segmentType->Enable();

		for (uint32_t i = 0; i < segmentSlot->GetCount(); i++) {
			uint32_t userSlotID = subSegmentData->userSlotID;

			// Find matching slot in choice
			wxString slotPrefix = wxString::Format("%d - ", userSlotID);
			auto slotString = segmentSlot->GetString(i);
			if (slotString.StartsWith(slotPrefix)) {
				segmentSlot->SetSelection((int)i);
				break;
			}
		}

		segmentSlot->Enable();
	}
	else {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (segmentData) {
			// Active segment is a normal segment
			// Collect list of partition IDs for segment and children.
			selPartIDs[segmentData->partID] = true;
			wxTreeItemIdValue subCookie;
			wxTreeItemId child = segmentTree->GetFirstChild(activeSegment, subCookie);
			while (child.IsOk()) {
				SubSegmentItemData* childData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(child));
				if (childData)
					selPartIDs[childData->partID] = true;
				child = segmentTree->GetNextChild(activeSegment, subCookie);
			}
		}
	}

	// Display segmentation colors depending on what is selected
	Mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triSParts);
		ApplySegmentVisibility(m);

		// Set colors for segments
		int nsm = m->subMeshes.size();
		m->subMeshesColor.resize(nsm);
		for (int pi = 0; pi < nsm; ++pi) {
			if (selPartIDs[pi]) {
				m->subMeshesColor[pi].x = 1.0f;
				m->subMeshesColor[pi].y = 0.0f;
				m->subMeshesColor[pi].z = 0.0f;
			}
			else {
				float colorValue = (pi + 1.0f) / (nsm + 1);
				m->subMeshesColor[pi] = glView->CreateColorRamp(colorValue);
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::UpdateSegmentNames() {
	auto segmentType = (wxChoice*)FindWindowByName("segmentType");
	auto segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");

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
			std::string subSegmentSlot = "";

			auto subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(subChild));
			if (subSegmentData) {
				if (subSegmentData->material != 0xFFFFFFFF) {
					bool typeFound = false;
					auto typeHash = wxString::Format("0x%08x", subSegmentData->material);
					for (uint32_t i = 0; i < segmentType->GetCount(); i++) {
						auto typeString = segmentType->GetString(i);
						if (typeString.Contains(typeHash)) {
							subSegmentName = typeString.BeforeLast('|');
							typeFound = true;
							break;
						}
					}

					if (!typeFound)
						subSegmentName = typeHash;
				}

				uint32_t userSlotID = subSegmentData->userSlotID;
				if (userSlotID >= 30) {
					wxString slotPrefix = wxString::Format("%d - ", userSlotID);

					// Find matching slot in choice
					for (uint32_t i = 0; i < segmentSlot->GetCount(); i++) {
						auto slotString = segmentSlot->GetString(i);
						if (slotString.StartsWith(slotPrefix)) {
							subSegmentSlot = wxString::Format("(Slot %s)", slotString);
							break;
						}
					}
				}
			}

			segmentTree->SetItemText(subChild, wxString::Format("#%d: %s%s", subSegmentIndex, subSegmentName, subSegmentSlot));

			subChild = segmentTree->GetNextChild(child, subCookie);
			subSegmentIndex++;
		}

		child = segmentTree->GetNextChild(segmentRoot, cookie);
		segmentIndex++;
	}
}

void OutfitStudioFrame::OnPartitionSelect(wxTreeEvent& event) {
	ShowPartition(event.GetItem());
}

void OutfitStudioFrame::OnPartitionVisToggle(wxTreeEvent& event) {
	wxTreeItemId item = event.GetItem();
	if (!item.IsOk() || !partitionTree->GetItemParent(item).IsOk()) {
		event.Skip();
		return;
	}

	partitionTree->SetItemState(item, partitionTree->GetItemState(item) == 1 ? 0 : 1);

	Mesh* m = nullptr;
	if (activeItem && activeItem->GetShape())
		m = glView->GetMesh(activeItem->GetShape()->name.get());

	if (m) {
		if (m->subMeshes.empty())
			SetSubMeshesForPartitions(m, triParts);

		ApplyPartitionVisibility(m);
		glView->Render();
	}

	event.Skip();
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

	// Find an unused partition index
	std::set<int> partInds;
	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData)
			partInds.insert(partitionData->index);
		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	int partInd = 0;
	while (partInds.count(partInd) != 0)
		++partInd;

	// Create partition item
	wxTreeItemId newItem;
	if (!activePartition.IsOk() || partitionTree->GetChildrenCount(partitionRoot) <= 0) {
		auto shape = activeItem->GetShape();
		if (shape && shape->GetNumVertices() > 0) {
			newItem = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(partInd, isSkyrim ? 32 : 0));
		}

		for (int& pi : triParts)
			pi = partInd;
	}
	else
		newItem = partitionTree->InsertItem(partitionRoot, activePartition, "Partition", -1, -1, new PartitionItemData(partInd, isSkyrim ? 32 : 0));

	if (newItem.IsOk()) {
		partitionTree->SetItemState(newItem, 0);
		partitionTree->UnselectAll();
		partitionTree->SelectItem(newItem);
	}

	UpdatePartitionNames();
	partitionTabButton->SetPendingChanges();
}

void OutfitStudioFrame::OnDeletePartition(wxCommandEvent& WXUNUSED(event)) {
	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		wxTreeItemId sibling = partitionTree->GetPrevSibling(activePartition);
		if (!sibling.IsOk())
			sibling = partitionTree->GetNextSibling(activePartition);

		int newIndex = -1;
		if (sibling.IsOk()) {
			PartitionItemData* siblingData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(sibling));
			if (siblingData)
				newIndex = siblingData->index;
		}
		for (size_t i = 0; i < triParts.size(); ++i)
			if (triParts[i] == partitionData->index)
				triParts[i] = newIndex;

		if (sibling.IsOk()) {
			partitionTree->UnselectAll();
			partitionTree->Delete(activePartition);
			partitionTree->SelectItem(sibling);

			ShowPartition(sibling);
		}
		else {
			partitionTree->Delete(activePartition);
			activePartition.Unset();
		}
	}

	UpdatePartitionNames();
	partitionTabButton->SetPendingChanges();
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
	partitionTabButton->SetPendingChanges();
}

void OutfitStudioFrame::OnPartitionApply(wxCommandEvent& WXUNUSED(event)) {
	ApplyPartitions();
}

void OutfitStudioFrame::ApplyPartitions() {
	auto shape = activeItem->GetShape();
	if (!shape)
		return;

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	std::vector<bool> delPartFlags;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);

	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData) {
			const int index = partitionData->index;
			if (index >= static_cast<int64_t>(partitionInfo.size())) {
				partitionInfo.resize(index + 1);
				delPartFlags.resize(index + 1, true);
			}
			partitionInfo[index].flags = PF_EDITOR_VISIBLE;
			partitionInfo[index].partID = partitionData->type;
			delPartFlags[index] = false;
		}

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}

	std::vector<uint32_t> delPartInds;
	for (uint32_t pi = 0; pi < delPartFlags.size(); ++pi)
		if (delPartFlags[pi])
			delPartInds.push_back(pi);

	project->GetWorkNif()->SetShapePartitions(shape, partitionInfo, triParts);
	if (!delPartInds.empty())
		project->GetWorkNif()->DeletePartitions(shape, delPartInds);

	CreatePartitionTree(shape);
	SetPendingChanges();
}

void OutfitStudioFrame::OnPartitionReset(wxCommandEvent& WXUNUSED(event)) {
	ResetPartitions();
}

void OutfitStudioFrame::ResetPartitions() {
	if (activeItem)
		CreatePartitionTree(activeItem->GetShape());
	else
		CreatePartitionTree(nullptr);
}

void OutfitStudioFrame::RefreshActivePartitionTree() {
	if (activeItem)
		CreatePartitionTree(activeItem->GetShape());
	else
		CreatePartitionTree(nullptr);
}

void OutfitStudioFrame::CreatePartitionTree(NiShape* shape) {
	if (partitionTree->GetChildrenCount(partitionRoot) > 0) {
		triParts.clear(); // DeleteChildren calls OnPartitionSelect
		partitionTree->DeleteChildren(partitionRoot);
	}

	partitionTabButton->SetPendingChanges(false);

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	if (project->GetWorkNif()->GetShapePartitions(shape, partitionInfo, triParts)) {
		for (uint32_t i = 0; i < partitionInfo.size(); i++) {
			wxTreeItemId item = partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(static_cast<int>(i), partitionInfo[i].partID));
			partitionTree->SetItemState(item, 0);
		}
	}

	UpdatePartitionNames();
	partitionTree->ExpandAll();

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	if (child.IsOk())
		partitionTree->SelectItem(child);
}

void OutfitStudioFrame::ApplyPartitionVisibility(Mesh* m) {
	if (!m)
		return;

	if (m->subMeshesVisible.size() != m->subMeshes.size())
		m->subMeshesVisible.assign(m->subMeshes.size(), true);

	if (!partitionTree || !partitionRoot.IsOk())
		return;

	wxTreeItemIdValue cookie;
	wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
	while (child.IsOk()) {
		PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(child));
		if (partitionData && partitionData->index >= 0 && static_cast<size_t>(partitionData->index) < m->subMeshesVisible.size())
			m->subMeshesVisible[partitionData->index] = partitionTree->GetItemState(child) != 1;

		child = partitionTree->GetNextChild(partitionRoot, cookie);
	}
}

void OutfitStudioFrame::ResetPartitionVisibility() {
	if (partitionTree && partitionRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = partitionTree->GetFirstChild(partitionRoot, cookie);
		while (child.IsOk()) {
			partitionTree->SetItemState(child, 0);
			child = partitionTree->GetNextChild(partitionRoot, cookie);
		}
	}

	if (glView) {
		for (auto& m : glView->gls.GetMeshes()) {
			if (m)
				m->subMeshesVisible.assign(m->subMeshes.size(), true);
		}
	}
}

void OutfitStudioFrame::ShowPartition(const wxTreeItemId& item) {
	if (!activeItem || !glView->GetSegmentMode())
		return;

	auto shape = activeItem->GetShape();
	if (!shape)
		return;

	wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
	partitionType->Disable();
	partitionType->SetSelection(0);
	wxArrayString partitionStrings = partitionType->GetStrings();

	if (item.IsOk())
		activePartition = item;

	if (!activePartition.IsOk() || !partitionTree->GetItemParent(activePartition).IsOk())
		return;

	// Get all triangles of the active shape
	std::vector<Triangle> allTris;
	shape->GetTriangles(allTris);
	if (allTris.size() != triParts.size())
		return;

	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		for (auto& s : partitionStrings) {
			if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
				// Show correct data in UI
				partitionType->Enable();
				partitionType->SetStringSelection(s);
			}
		}
	}

	// Display partition colors depending on what is selected
	Mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triParts);
		ApplyPartitionVisibility(m);

		// Set colors for non-selected partitions
		int nsm = m->subMeshes.size();
		m->subMeshesColor.resize(nsm);

		for (int pi = 0; pi < nsm; ++pi) {
			float colorValue = (pi + 1.0f) / (nsm + 1);
			m->subMeshesColor[pi] = glView->CreateColorRamp(colorValue);
		}

		// Set color for selected partition
		if (partitionData) {
			if (nsm > partitionData->index) {
				m->subMeshesColor[partitionData->index].x = 1.0f;
				m->subMeshesColor[partitionData->index].y = 0.0f;
				m->subMeshesColor[partitionData->index].z = 0.0f;
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
			for (auto& s : partitionStrings) {
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

void OutfitStudioFrame::SetSubMeshesForPartitions(Mesh* m, const std::vector<int>& tp) {
	uint32_t nTris = static_cast<uint32_t>(tp.size());

	// Sort triangles (via triInds) by partition number, negative partition
	// numbers at the end.
	std::vector<uint32_t> triInds(nTris);
	for (uint32_t ti = 0; ti < nTris; ++ti)
		triInds[ti] = ti;

	std::stable_sort(triInds.begin(), triInds.end(), [&tp](int i, int j) { return tp[j] < 0 || tp[i] < tp[j]; });

	// Re-order triangles
	for (uint32_t ti = 0; ti < nTris; ++ti)
		m->renderTris[ti] = m->tris[triInds[ti]];

	// Find first triangle of each sub-mesh.
	m->subMeshes.clear();
	m->subMeshesColor.clear();
	m->subMeshesVisible.clear();
	m->triSubMeshes.assign(nTris, -1);
	int negativeSubMeshIndex = -1;

	for (uint32_t ti = 0; ti < nTris; ++ti) {
		while (tp[triInds[ti]] >= static_cast<int>(m->subMeshes.size()))
			m->subMeshes.emplace_back(ti, 0);

		if (tp[triInds[ti]] < 0) {
			negativeSubMeshIndex = static_cast<int>(m->subMeshes.size());
			m->subMeshes.emplace_back(ti, 0);
			break;
		}
	}

	// Calculate size of each sub-mesh.
	m->subMeshes.emplace_back(nTris, 0);
	for (size_t si = 0; si + 1 < m->subMeshes.size(); ++si)
		m->subMeshes[si].second = m->subMeshes[si + 1].first - m->subMeshes[si].first;

	m->subMeshes.pop_back();
	m->subMeshesVisible.assign(m->subMeshes.size(), true);

	for (uint32_t ti = 0; ti < nTris; ++ti) {
		int subMeshIndex = tp[ti] >= 0 ? tp[ti] : negativeSubMeshIndex;
		if (subMeshIndex >= 0 && static_cast<size_t>(subMeshIndex) < m->subMeshes.size())
			m->triSubMeshes[ti] = subMeshIndex;
	}

	m->QueueUpdate(Mesh::UpdateType::Indices);
}

void OutfitStudioFrame::SetNoSubMeshes(Mesh* m) {
	if (!m)
		return;

	m->subMeshes.clear();
	m->subMeshesColor.clear();
	m->subMeshesVisible.clear();
	m->triSubMeshes.clear();

	for (int ti = 0; ti < m->nTris; ++ti)
		m->renderTris[ti] = m->tris[ti];

	m->QueueUpdate(Mesh::UpdateType::Indices);
}

void OutfitStudioFrame::SetNoSubMeshes() {
	if (!activeItem)
		return;

	SetNoSubMeshes(glView->GetMesh(activeItem->GetShape()->name.get()));
}

void OutfitStudioFrame::OnSliderCheckBox(wxCommandEvent& event) {
	wxCheckBox* box = (wxCheckBox*)event.GetEventObject();
	if (!box)
		return;

	bool checked = event.IsChecked();

	std::string sliderName = box->GetName().BeforeLast('|').ToStdString();
	ShowSliderEffect(sliderName, checked);

	bool shiftDown = wxGetKeyState(WXK_SHIFT);

	if (!lastCheckedSlider.empty() && shiftDown) {
		wxSliderPanel* sliderPanel = sliderPanels[sliderName];
		wxSliderPanel* lastSliderPanel = sliderPanels[lastCheckedSlider];

		if (sliderPanel && lastSliderPanel) {
			const size_t sliderIndex = sliderPool.FindIndex(sliderPanel);
			const size_t lastSliderIndex = sliderPool.FindIndex(lastSliderPanel);
			const size_t invalidSliderIndex = static_cast<size_t>(-1);

			if (sliderIndex != invalidSliderIndex && lastSliderIndex != invalidSliderIndex && sliderIndex != lastSliderIndex) {
				size_t startIndex, endIndex;
				if (sliderIndex > lastSliderIndex) {
					startIndex = lastSliderIndex;
					endIndex = sliderIndex;
				}
				else {
					startIndex = sliderIndex;
					endIndex = lastSliderIndex;
				}

				for (size_t i = startIndex; i < endIndex; i++) {
					wxSliderPanel* sliderPanelIndex = sliderPool.Get(i);
					if (sliderPanelIndex && sliderPanelIndex->IsCreated()) {
						std::string sliderNameIndex = sliderPanelIndex->sliderCheck->GetName().BeforeLast('|').ToStdString();
						ShowSliderEffect(sliderNameIndex, checked);
					}
				}
			}
		}
	}

	ApplySliders();

	if (!shiftDown)
		lastCheckedSlider = sliderName;
}

void OutfitStudioFrame::OnSelectTool(wxCommandEvent& event) {
	int id = event.GetId();

	if (id == XRCID("btnSelect"))
		SelectTool(ToolID::Select);
	else if (id == XRCID("btnTransform"))
		SelectTool(ToolID::Transform);
	else if (id == XRCID("btnPivot"))
		SelectTool(ToolID::Pivot);
	else if (id == XRCID("btnVertexEdit"))
		SelectTool(ToolID::VertexEdit);
	else if (id == XRCID("btnMaskBrush"))
		SelectTool(ToolID::MaskBrush);
	else if (id == XRCID("btnInflateBrush"))
		SelectTool(ToolID::InflateBrush);
	else if (id == XRCID("btnDeflateBrush"))
		SelectTool(ToolID::DeflateBrush);
	else if (id == XRCID("btnMoveBrush"))
		SelectTool(ToolID::MoveBrush);
	else if (id == XRCID("btnSmoothBrush"))
		SelectTool(ToolID::SmoothBrush);
	else if (id == XRCID("btnUndiffBrush"))
		SelectTool(ToolID::UndiffBrush);
	else if (id == XRCID("btnWeightBrush"))
		SelectTool(ToolID::WeightBrush);
	else if (id == XRCID("btnColorBrush"))
		SelectTool(ToolID::ColorBrush);
	else if (id == XRCID("btnAlphaBrush"))
		SelectTool(ToolID::AlphaBrush);
	else if (id == XRCID("btnCollapseVertex"))
		SelectTool(ToolID::CollapseVertex);
	else if (id == XRCID("btnFlipEdgeTool"))
		SelectTool(ToolID::FlipEdge);
	else if (id == XRCID("btnSplitEdgeTool"))
		SelectTool(ToolID::SplitEdge);
	else if (id == XRCID("btnMoveVertexTool"))
		SelectTool(ToolID::MoveVertex);
	else
		SelectTool(ToolID::Any);

	// Remember last standard tool used in regular tabs
	if (meshTabButton->GetCheck() || lightsTabButton->GetCheck()) {
		auto activeBrush = glView->GetActiveBrush();
		if (activeBrush) {
			TweakBrush::BrushType brushType = activeBrush->Type();
			if (brushType == TweakBrush::BrushType::Inflate || brushType == TweakBrush::BrushType::Mask || brushType == TweakBrush::BrushType::Move) {
				glView->SetLastTool(glView->GetActiveTool());
			}
		}
	}
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
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);

	glView->SetPerspective(enabled);
	OutfitStudioConfig.SetBoolValue("Rendering/PerspectiveView", enabled);
}

void OutfitStudioFrame::OnToggleRotationCenter(wxCommandEvent& WXUNUSED(event)) {
	autoFrameSelected = false;

	if (glView->rotationCenterMode != RotationCenterMode::Zero) {
		glView->rotationCenterMode = RotationCenterMode::Zero;
		glView->gls.camRotOffset.Zero();
	}
	else {
		glView->rotationCenterMode = RotationCenterMode::MeshCenter;
		glView->gls.camRotOffset = glView->gls.GetActiveCenter();
	}

	glView->Render();
}

void OutfitStudioFrame::OnFrameSelected(wxCommandEvent& WXUNUSED(event)) {
	autoFrameSelected = !autoFrameSelected;
	FrameSelected();
}

void OutfitStudioFrame::FrameSelected() {
	auto& gls = glView->gls;
	const auto& meshes = gls.GetActiveMeshes();
	if (meshes.empty())
		return;

	nifly::Vector3 bbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	nifly::Vector3 bbMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
	int count = 0;

	for (auto& m : meshes) {
		for (int i = 0; i < m->nVerts; i++) {
			if (m->mask && m->mask[i] != 0.0f)
				continue;
			nifly::Vector3 v = m->TransformPosMeshToModel(m->verts[i]);
			bbMin.x = std::min(bbMin.x, v.x);
			bbMin.y = std::min(bbMin.y, v.y);
			bbMin.z = std::min(bbMin.z, v.z);
			bbMax.x = std::max(bbMax.x, v.x);
			bbMax.y = std::max(bbMax.y, v.y);
			bbMax.z = std::max(bbMax.z, v.z);
			count++;
		}
	}

	if (count == 0)
		return;

	nifly::Vector3 center = (bbMin + bbMax) / 2.0f;
	float extentX = bbMax.x - bbMin.x;
	float extentY = bbMax.y - bbMin.y;
	float extentZ = bbMax.z - bbMin.z;
	float radius = std::max({extentX, extentY, extentZ}) / 2.0f;
	if (radius < 0.001f)
		radius = 0.001f;

	gls.camRotOffset = center;
	glView->rotationCenterMode = RotationCenterMode::MeshCenter;

	gls.camPos.x = -center.x;
	gls.camPos.y = -center.y;

	if (gls.perspective) {
		float fovRad = gls.mFov * DEG2RAD;
		float distance = radius / std::tan(fovRad / 2.0f);
		gls.camPos.z = -(distance + radius);
	}
	else {
		gls.camPos.z = -(radius * 2.5f);
	}

	glView->Render();
}

void OutfitStudioFrame::OnShowNodes(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowNodes(enabled);
	UpdateBoneTransformToolEnabled();
}

void OutfitStudioFrame::OnShowBones(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowBones(enabled);
	UpdateBoneTransformToolEnabled();
}

void OutfitStudioFrame::OnShowFloor(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowFloor(enabled);
}

void OutfitStudioFrame::OnBrushSettings(wxCommandEvent& WXUNUSED(event)) {
	if (brushSettingsPopupTransient && brushSettingsPopupTransient->IsShown())
		CloseBrushSettings();
	else
		PopupBrushSettings(brushSettings);
}

void OutfitStudioFrame::OnEdgeSlideCorrectUV(wxCommandEvent& event) {
	glView->SetToolOptionEdgeSlideUVCorrection(event.IsChecked());
	ReToggleToolOptionsUI();
}

void OutfitStudioFrame::OnFieldOfViewSlider(wxCommandEvent& WXUNUSED(event)) {
	int fieldOfView = fovSlider->GetValue();

	wxStaticText* fovLabel = (wxStaticText*)toolBarH->FindWindowByName("fovLabel");
	fovLabel->SetLabel(wxString::Format(_("Field of View: %d"), fieldOfView));

	glView->SetFieldOfView(fieldOfView);
}

void OutfitStudioFrame::OnDepthClip(wxCommandEvent& WXUNUSED(event)) {
	float zNear = cbDepthClip->IsChecked() ? 0.001f : 0.1f;
	glView->SetDepthClip(zNear, glView->gls.zFar);
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
	int ambient = 15;
	int frontal = 100;
	int directional0 = 0;
	int directional1 = 0;
	int directional2 = 0;

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

void OutfitStudioFrame::OnSaveLights(wxCommandEvent& event) {
	// Make sure the current slider values are in the configuration before writing it out
	OnUpdateLights(event);

	int ret = Config.SaveConfig(Config["AppDir"] + "/Config.xml");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);
}

void OutfitStudioFrame::PopulateHDRiBackgrounds() {
	if (!hdriBackground)
		return;

	hdriBackground->Clear();

	// Client data carries the file name; an empty one is what turns the environment off.
	hdriBackground->Append(_("No background"), new wxStringClientData(""));

#ifdef USE_OPENEXR
	wxArrayString files;
	wxDir::GetAllFiles(wxString::FromUTF8(Config["AppDir"]) + "/res/hdri", &files, "*.exr", wxDIR_FILES);
	files.Sort();

	for (auto& file : files) {
		const wxFileName fileName(file);

		// The name on disk, not a translated one - these are files the user can add to and rename.
		hdriBackground->Append(fileName.GetName(), new wxStringClientData(fileName.GetFullName()));
	}
#else
	// Nothing can be loaded without an EXR decoder, and a choice with one entry reads as broken.
	hdriBackground->Hide();

	if (lightSettings) {
		if (auto label = lightSettings->FindWindowByName("hdriBackgroundLabel"))
			label->Hide();
	}
#endif

	hdriBackground->SetSelection(0);
}

bool OutfitStudioFrame::SetHDRiBackground(const std::string& fileName, const bool remember) {
	if (!glView->SetHDRiBackground(fileName) && !fileName.empty()) {
		// The environment failed to load and was left off, so the choice has to say so too.
		if (hdriBackground)
			hdriBackground->SetSelection(0);

		return false;
	}

	// Only a background the user picked is worth writing down. The automatic one is a convenience
	// for the project at hand, and remembering it would make the setting say the user chose an HDRi
	// they never asked for - and would rewrite the config on every project that has a Complex
	// Material in it.
	if (remember) {
		Config.SetValue("Rendering/HDRIBackground", fileName);

		int ret = Config.SaveConfig(Config["AppDir"] + "/Config.xml");
		if (ret)
			wxLogWarning("Failed to save configuration (%d)!", ret);
	}

	if (!hdriBackground)
		return true;

	for (unsigned int i = 0; i < hdriBackground->GetCount(); i++) {
		auto data = static_cast<wxStringClientData*>(hdriBackground->GetClientObject(i));
		if (data && data->GetData().ToStdString() == fileName) {
			hdriBackground->SetSelection(i);
			break;
		}
	}

	return true;
}

void OutfitStudioFrame::OnHDRiBackground(wxCommandEvent& WXUNUSED(event)) {
	if (!hdriBackground)
		return;

	auto data = static_cast<wxStringClientData*>(hdriBackground->GetClientObject(hdriBackground->GetSelection()));
	const std::string fileName = data ? data->GetData().ToStdString() : std::string();

	// Turning it off by hand is a decision the automatic activation must not talk the user out of.
	hdriBackgroundCleared = fileName.empty();

	// Only a background the user asked for is worth interrupting them over. The automatic one below
	// leaves its reason in the log and moves on.
	if (!SetHDRiBackground(fileName, true))
		wxMessageBox(wxString::Format(_("Failed to load the HDRi '%s'. See the log for details."), fileName), _("Error"), wxICON_ERROR, this);
}

void OutfitStudioFrame::ApplyAutoHDRiBackground() {
#ifndef USE_OPENEXR
	// Nothing to activate, and trying would log a failure for every project that has a Complex
	// Material in it.
	return;
#else
	if (hdriBackgroundCleared || !hdriBackground || glView->HasHDRiBackground())
		return;

	// Complex Material and True PBR shading both live off what they reflect, so a project full of
	// either is nearly impossible to judge against the flat background. A True PBR shape has the
	// stronger claim of the two: its cube map slot is deliberately empty, so without an HDRi there is
	// no image based lighting for it at all. Anything else is left as it was.
	const auto& meshes = glView->gls.GetMeshes();
	if (std::none_of(meshes.begin(), meshes.end(), [](const Mesh* m) { return m->complexMaterial || m->pbr; }))
		return;

	// Whatever the user last picked by hand, which is the only thing that ever gets written there.
	std::string fileName = Config["Rendering/HDRIBackground"];
	if (fileName.empty() || !PlatformUtil::FileExists(Config["AppDir"] + "/res/hdri/" + fileName))
		fileName = "sunrise.exr";

	SetHDRiBackground(fileName, false);
#endif
}

void OutfitStudioFrame::OnClickSliderButton(wxCommandEvent& event) {
	wxWindow* btn = (wxWindow*)event.GetEventObject();
	if (!btn)
		return;

	wxString buttonName = btn->GetName();
	std::string clickedName{buttonName.BeforeLast('|').ToUTF8()};
	if (clickedName.empty()) {
		event.Skip();
		return;
	}

	wxString buttonNameId = buttonName.AfterLast('|');

	float scale = 0.0f;
	if (buttonNameId == "btnMinus")
		scale = 0.99f;
	else if (buttonNameId == "btnPlus")
		scale = 1.01f;

	if (scale != 0.0f) {
		std::vector<NiShape*> shapes;
		for (auto& i : selectedItems)
			shapes.push_back(i->GetShape());

		if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{activeSlider}))
			return;

		for (auto& i : selectedItems) {
			auto shape = i->GetShape();
			std::vector<Vector3> verts;
			project->ScaleMorphResult(shape, activeSlider, scale);
			project->GetLiveVerts(shape, verts);
			glView->UpdateMeshVertices(shape->name.get(), &verts);
		}
		return;
	}

	if (buttonNameId == "btnSliderProp") {
		ShowSliderProperties(clickedName);
		return;
	}

	if (activeSlider != clickedName)
		EnterSliderEdit(clickedName);
	else
		ExitSliderEdit();
}

void OutfitStudioFrame::OnEnterHoverSlider(wxMouseEvent& event) {
	event.Skip();

	if (!event.ControlDown())
		return;

	if (currentTabButton != meshTabButton)
		return;

	wxWindow* winObj = (wxWindow*)event.GetEventObject();
	if (!winObj)
		return;

	wxString objName = winObj->GetName();
	std::string sliderName{objName.BeforeLast('|').ToUTF8()};
	if (sliderName.empty())
		return;

	SliderData& sd = project->activeSet[sliderName];

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		Mesh* m = glView->GetMesh(shape->name.get());
		if (m && m->weight) {
			// Clear color
			m->WeightFill(0.0f);

			auto diff = project->GetDiffSet(sd, shape);
			if (!diff)
				continue;

			// Get largest diff (length)
			float maxLength = 0.0f;
			for (auto& d : *diff) {
				if (d.second.length() > maxLength)
					maxLength = d.second.length();
			}

			// Apply color gradient to all vertices with diffs
			for (auto& d : *diff) {
				if (m->nVerts > d.first) {
					float gradient = d.second.length() / maxLength;
					m->weight[d.first] = gradient;
				}
			}

			m->QueueUpdate(Mesh::UpdateType::Weight);
		}
	}

	glView->SetWeightVisible();
	glView->Render();
}

void OutfitStudioFrame::OnLeaveHoverSlider(wxMouseEvent& event) {
	event.Skip();

	if (currentTabButton != meshTabButton)
		return;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		Mesh* m = glView->GetMesh(shape->name.get());
		if (m && m->weight) {
			// Clear color
			m->WeightFill(0.0f);
		}
	}

	glView->SetWeightVisible(false);
	glView->Render();
}

void OutfitStudioFrame::OnReadoutChange(wxCommandEvent& event) {
	wxTextCtrl* w = (wxTextCtrl*)event.GetEventObject();
	event.Skip();
	if (!w)
		return;

	wxString sn = w->GetName();
	if (!sn.EndsWith("|readout", &sn))
		return;

	std::string sliderName{sn.ToUTF8()};

	double v;
	wxString val = w->GetValue();
	val.Replace("%", "");
	if (!val.ToDouble(&v))
		return;

	wxSliderPanel* sliderPanel = sliderPanels[sliderName];
	sliderPanel->slider->SetValue(v);

	project->SliderValue(sliderName) = v / 100.0f;

	ApplySliders();
}

void OutfitStudioFrame::OnTabButtonClick(wxCommandEvent& event) {
	int id = event.GetId();

	if (currentTabButton && currentTabButton->HasPendingChanges()) {
		wxMessageDialog dlg(this,
							_("Your changes were not applied yet. Do you want to apply or reset them?"),
							_("Pending Changes"),
							wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetYesNoCancelLabels(_("Apply"), _("Reset"), _("Cancel"));

		int res = dlg.ShowModal();
		if (res == wxID_YES) {
			if (currentTabButton == partitionTabButton)
				ApplyPartitions();
			else if (currentTabButton == segmentTabButton)
				ApplySegments();
		}
		else if (res == wxID_NO) {
			if (currentTabButton == partitionTabButton)
				ResetPartitions();
			else if (currentTabButton == segmentTabButton)
				ResetSegments();
		}
		else {
			event.Skip();
			return;
		}
	}

	if ((id == segmentTabButton->GetId() || id == partitionTabButton->GetId()) && selectedItems.size() != 1) {
		wxMessageBox(_("You must have exactly one mesh selected in order to edit partitions or segments."), _("Info"), wxICON_INFORMATION, this);
		event.Skip();
		return;
	}

	if (id != meshTabButton->GetId())
		masksPane->Hide();

	if (id != segmentTabButton->GetId()) {
		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSlotLabel = (wxStaticText*)FindWindowByName("segmentSlotLabel");
		wxChoice* segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxButton* segmentSSFEdit = (wxButton*)FindWindowByName("segmentSSFEdit");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show(false);
		segmentType->Show(false);
		segmentSlotLabel->Show(false);
		segmentSlot->Show(false);
		segmentSSFLabel->Show(false);
		segmentSSFEdit->Show(false);
		segmentSSF->Show(false);
		segmentApply->Show(false);
		segmentReset->Show(false);

		if (currentTabButton == segmentTabButton)
			ResetSegmentVisibility();

		glView->SetSegmentMode(false);
		glView->SetMaskVisible();

		menuBar->Enable(XRCID("btnSelect"), true);
		menuBar->Enable(XRCID("btnClearMask"), true);
		menuBar->Enable(XRCID("btnInvertMask"), true);
		menuBar->Enable(XRCID("deleteVerts"), true);
		menuBar->Enable(XRCID("refineMesh"), true);

		toolBarH->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != partitionTabButton->GetId()) {
		wxStaticText* partitionTypeLabel = (wxStaticText*)FindWindowByName("partitionTypeLabel");
		wxChoice* partitionType = (wxChoice*)FindWindowByName("partitionType");
		wxButton* partitionApply = (wxButton*)FindWindowByName("partitionApply");
		wxButton* partitionReset = (wxButton*)FindWindowByName("partitionReset");

		partitionTypeLabel->Show(false);
		partitionType->Show(false);
		partitionApply->Show(false);
		partitionReset->Show(false);

		if (currentTabButton == partitionTabButton)
			ResetPartitionVisibility();

		glView->SetSegmentMode(false);
		glView->SetMaskVisible();

		menuBar->Enable(XRCID("btnSelect"), true);
		menuBar->Enable(XRCID("btnClearMask"), true);
		menuBar->Enable(XRCID("btnInvertMask"), true);
		menuBar->Enable(XRCID("deleteVerts"), true);
		menuBar->Enable(XRCID("refineMesh"), true);

		toolBarH->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != boneTabButton->GetId()) {
		cXMirrorBone->Show(false);

		wxStaticText* totalBoneCountLabel = (wxStaticText*)FindWindowByName("totalBoneCountLabel");
		wxStaticText* selectedBoneCountLabel = (wxStaticText*)FindWindowByName("selectedBoneCountLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		wxCheckBox* cbNormalizeWeights = (wxCheckBox*)FindWindowByName("cbNormalizeWeights");
		wxStaticText* xMirrorBoneLabel = (wxStaticText*)FindWindowByName("xMirrorBoneLabel");

		totalBoneCountLabel->Show(false);
		selectedBoneCountLabel->Show(false);
		cbFixedWeight->Show(false);
		cbNormalizeWeights->Show(false);
		xMirrorBoneLabel->Show(false);
		posePane->Show(false);
		if (physicsPane)
			physicsPane->Show(false);
		if (animationPane)
			animationPane->Show(false);
		bonesFilter->GetParent()->Show(false);

		if (project->bPose) {
			project->bPose = false;
			UpdatePhysicsState();
			ApplyPose();
		}

		glView->SetWeightVisible(false);
	}

	if (id != colorsTabButton->GetId()) {
		UpdateVertexColors();
	}

	if (id != boneTabButton->GetId() && id != colorsTabButton->GetId()) {
		menuBar->Check(XRCID("btnInflateBrush"), true);
		menuBar->Enable(XRCID("btnTransform"), true);
		menuBar->Enable(XRCID("btnPivot"), true);
		menuBar->Enable(XRCID("btnVertexEdit"), true);
		menuBar->Enable(XRCID("btnWeightBrush"), false);
		menuBar->Enable(XRCID("btnColorBrush"), false);
		menuBar->Enable(XRCID("btnAlphaBrush"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), true);
		menuBar->Enable(XRCID("btnDeflateBrush"), true);
		menuBar->Enable(XRCID("btnMoveBrush"), true);
		menuBar->Enable(XRCID("btnSmoothBrush"), true);
		menuBar->Enable(XRCID("btnUndiffBrush"), true);
		menuBar->Enable(XRCID("btnCollapseVertex"), true);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), true);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), true);
		menuBar->Enable(XRCID("btnMoveVertexTool"), true);
		menuBar->Enable(XRCID("deleteVerts"), true);
		menuBar->Enable(XRCID("refineMesh"), true);

		toolBarH->ToggleTool(XRCID("btnInflateBrush"), true);
		toolBarH->EnableTool(XRCID("btnWeightBrush"), false);
		toolBarH->EnableTool(XRCID("btnColorBrush"), false);
		toolBarH->EnableTool(XRCID("btnAlphaBrush"), false);
		toolBarV->EnableTool(XRCID("btnTransform"), true);
		toolBarV->EnableTool(XRCID("btnPivot"), true);
		toolBarV->EnableTool(XRCID("btnVertexEdit"), true);
		toolBarH->EnableTool(XRCID("btnInflateBrush"), true);
		toolBarH->EnableTool(XRCID("btnDeflateBrush"), true);
		toolBarH->EnableTool(XRCID("btnMoveBrush"), true);
		toolBarH->EnableTool(XRCID("btnSmoothBrush"), true);
		toolBarH->EnableTool(XRCID("btnUndiffBrush"), true);
		toolBarH->EnableTool(XRCID("btnCollapseVertex"), true);
		toolBarH->EnableTool(XRCID("btnFlipEdgeTool"), true);
		toolBarH->EnableTool(XRCID("btnSplitEdgeTool"), true);
		toolBarH->EnableTool(XRCID("btnMoveVertexTool"), true);
	}

	if (id == meshTabButton->GetId()) {
		currentTabButton = meshTabButton;

		outfitBones->Hide();
		colorSettings->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitShapes->Show();

		boneTabButton->SetCheck(false);
		colorsTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		masksPane->Show();

		SetNoSubMeshes();
		SelectTool(glView->GetLastTool());
	}
	else if (id == boneTabButton->GetId()) {
		currentTabButton = boneTabButton;

		outfitShapes->Hide();
		colorSettings->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		outfitBones->Show();

		meshTabButton->SetCheck(false);
		colorsTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		cXMirrorBone->Show();

		wxStaticText* totalBoneCountLabel = (wxStaticText*)FindWindowByName("totalBoneCountLabel");
		wxStaticText* selectedBoneCountLabel = (wxStaticText*)FindWindowByName("selectedBoneCountLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		wxCheckBox* cbNormalizeWeights = (wxCheckBox*)FindWindowByName("cbNormalizeWeights");
		wxStaticText* xMirrorBoneLabel = (wxStaticText*)FindWindowByName("xMirrorBoneLabel");

		totalBoneCountLabel->Show();
		selectedBoneCountLabel->Show();
		cbFixedWeight->Show();
		cbNormalizeWeights->Show();
		xMirrorBoneLabel->Show();
		posePane->Show();
		if (physicsPane)
			physicsPane->Show(physicsAvailable);
		if (animationPane)
			animationPane->Show();
		bonesFilter->GetParent()->Show();

		UpdateBoneTransformToolEnabled();

		SelectTool(ToolID::WeightBrush);
		glView->SetWeightVisible();

		project->bPose = cbPose->GetValue();

		if (project->bPose)
			ApplyPose();

		menuBar->Check(XRCID("btnWeightBrush"), true);
		menuBar->Enable(XRCID("btnWeightBrush"), true);
		menuBar->Enable(XRCID("btnColorBrush"), false);
		menuBar->Enable(XRCID("btnAlphaBrush"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), true);
		menuBar->Enable(XRCID("btnDeflateBrush"), true);
		menuBar->Enable(XRCID("btnMoveBrush"), true);
		menuBar->Enable(XRCID("btnSmoothBrush"), true);
		menuBar->Enable(XRCID("btnUndiffBrush"), true);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("btnMoveVertexTool"), true);
		menuBar->Enable(XRCID("deleteVerts"), false);
		menuBar->Enable(XRCID("refineMesh"), false);

		toolBarH->ToggleTool(XRCID("btnWeightBrush"), true);
		toolBarH->EnableTool(XRCID("btnWeightBrush"), true);
		toolBarH->EnableTool(XRCID("btnColorBrush"), false);
		toolBarH->EnableTool(XRCID("btnAlphaBrush"), false);
		toolBarV->EnableTool(XRCID("btnPivot"), false);
		toolBarV->EnableTool(XRCID("btnVertexEdit"), false);
		toolBarH->EnableTool(XRCID("btnInflateBrush"), true);
		toolBarH->EnableTool(XRCID("btnDeflateBrush"), true);
		toolBarH->EnableTool(XRCID("btnMoveBrush"), true);
		toolBarH->EnableTool(XRCID("btnSmoothBrush"), true);
		toolBarH->EnableTool(XRCID("btnUndiffBrush"), true);
		toolBarH->EnableTool(XRCID("btnCollapseVertex"), false);
		toolBarH->EnableTool(XRCID("btnFlipEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnSplitEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnMoveVertexTool"), true);

		SetNoSubMeshes();

		ReselectBone();
		UpdateUndoTools();
	}
	else if (id == colorsTabButton->GetId()) {
		currentTabButton = colorsTabButton;

		outfitShapes->Hide();
		outfitBones->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		colorSettings->Show();

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		glView->SetTransformMode(false);
		SelectTool(ToolID::ColorBrush);
		glView->SetColorsVisible();

		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Alpha"));

		FillVertexColors();

		menuBar->Check(XRCID("btnColorBrush"), true);
		menuBar->Enable(XRCID("btnColorBrush"), true);
		menuBar->Enable(XRCID("btnAlphaBrush"), true);
		menuBar->Enable(XRCID("btnWeightBrush"), false);
		menuBar->Enable(XRCID("btnTransform"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);
		menuBar->Enable(XRCID("btnUndiffBrush"), false);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("btnMoveVertexTool"), false);
		menuBar->Enable(XRCID("deleteVerts"), false);
		menuBar->Enable(XRCID("refineMesh"), false);

		toolBarH->ToggleTool(XRCID("btnColorBrush"), true);
		toolBarH->EnableTool(XRCID("btnColorBrush"), true);
		toolBarH->EnableTool(XRCID("btnAlphaBrush"), true);
		toolBarH->EnableTool(XRCID("btnWeightBrush"), false);
		toolBarV->EnableTool(XRCID("btnTransform"), false);
		toolBarV->EnableTool(XRCID("btnPivot"), false);
		toolBarV->EnableTool(XRCID("btnVertexEdit"), false);
		toolBarH->EnableTool(XRCID("btnInflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnDeflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnMoveBrush"), false);
		toolBarH->EnableTool(XRCID("btnSmoothBrush"), false);
		toolBarH->EnableTool(XRCID("btnUndiffBrush"), false);
		toolBarH->EnableTool(XRCID("btnCollapseVertex"), false);
		toolBarH->EnableTool(XRCID("btnFlipEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnSplitEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnMoveVertexTool"), false);

		SetNoSubMeshes();
	}
	else if (id == segmentTabButton->GetId()) {
		currentTabButton = segmentTabButton;

		outfitShapes->Hide();
		outfitBones->Hide();
		colorSettings->Hide();
		partitionTree->Hide();
		lightSettings->Hide();
		segmentTree->Show();

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		colorsTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);
		lightsTabButton->SetCheck(false);

		wxStaticText* segmentTypeLabel = (wxStaticText*)FindWindowByName("segmentTypeLabel");
		wxChoice* segmentType = (wxChoice*)FindWindowByName("segmentType");
		wxStaticText* segmentSlotLabel = (wxStaticText*)FindWindowByName("segmentSlotLabel");
		wxChoice* segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");
		wxStaticText* segmentSSFLabel = (wxStaticText*)FindWindowByName("segmentSSFLabel");
		wxButton* segmentSSFEdit = (wxButton*)FindWindowByName("segmentSSFEdit");
		wxTextCtrl* segmentSSF = (wxTextCtrl*)FindWindowByName("segmentSSF");
		wxButton* segmentApply = (wxButton*)FindWindowByName("segmentApply");
		wxButton* segmentReset = (wxButton*)FindWindowByName("segmentReset");

		segmentTypeLabel->Show();
		segmentType->Show();
		segmentSlotLabel->Show();
		segmentSlot->Show();
		segmentSSFLabel->Show();
		segmentSSFEdit->Show();
		segmentSSF->Show();
		segmentApply->Show();
		segmentReset->Show();

		glView->SetSegmentMode();
		SelectTool(ToolID::MaskBrush); // Use mask brush for segment editing (but with custom painting function 'PaintSegmentPartitionTriangles')
		glView->SetMaskVisible(false);

		menuBar->Check(XRCID("btnMaskBrush"), true);
		menuBar->Enable(XRCID("btnSelect"), false);
		menuBar->Enable(XRCID("btnTransform"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);
		menuBar->Enable(XRCID("btnUndiffBrush"), false);
		menuBar->Enable(XRCID("btnClearMask"), false);
		menuBar->Enable(XRCID("btnInvertMask"), false);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("btnMoveVertexTool"), false);
		menuBar->Enable(XRCID("deleteVerts"), false);
		menuBar->Enable(XRCID("refineMesh"), false);

		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
		toolBarH->EnableTool(XRCID("btnSelect"), false);
		toolBarV->EnableTool(XRCID("btnTransform"), false);
		toolBarV->EnableTool(XRCID("btnPivot"), false);
		toolBarV->EnableTool(XRCID("btnVertexEdit"), false);
		toolBarH->EnableTool(XRCID("btnInflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnDeflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnMoveBrush"), false);
		toolBarH->EnableTool(XRCID("btnSmoothBrush"), false);
		toolBarH->EnableTool(XRCID("btnUndiffBrush"), false);
		toolBarH->EnableTool(XRCID("btnCollapseVertex"), false);
		toolBarH->EnableTool(XRCID("btnFlipEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnSplitEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnMoveVertexTool"), false);

		ResetSegmentVisibility();
		ShowSegment(segmentTree->GetSelection());
	}
	else if (id == partitionTabButton->GetId()) {
		currentTabButton = partitionTabButton;

		outfitShapes->Hide();
		outfitBones->Hide();
		colorSettings->Hide();
		segmentTree->Hide();
		lightSettings->Hide();
		partitionTree->Show();

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		colorsTabButton->SetCheck(false);
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

		glView->SetSegmentMode();
		SelectTool(ToolID::MaskBrush); // Use mask brush for partition editing (but with custom painting function 'PaintSegmentPartitionTriangles')
		glView->SetMaskVisible(false);

		menuBar->Check(XRCID("btnMaskBrush"), true);
		menuBar->Enable(XRCID("btnSelect"), false);
		menuBar->Enable(XRCID("btnTransform"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);
		menuBar->Enable(XRCID("btnUndiffBrush"), false);
		menuBar->Enable(XRCID("btnClearMask"), false);
		menuBar->Enable(XRCID("btnInvertMask"), false);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("btnMoveVertexTool"), false);
		menuBar->Enable(XRCID("deleteVerts"), false);
		menuBar->Enable(XRCID("refineMesh"), false);

		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
		toolBarH->EnableTool(XRCID("btnSelect"), false);
		toolBarV->EnableTool(XRCID("btnTransform"), false);
		toolBarV->EnableTool(XRCID("btnPivot"), false);
		toolBarV->EnableTool(XRCID("btnVertexEdit"), false);
		toolBarH->EnableTool(XRCID("btnInflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnDeflateBrush"), false);
		toolBarH->EnableTool(XRCID("btnMoveBrush"), false);
		toolBarH->EnableTool(XRCID("btnSmoothBrush"), false);
		toolBarH->EnableTool(XRCID("btnUndiffBrush"), false);
		toolBarH->EnableTool(XRCID("btnCollapseVertex"), false);
		toolBarH->EnableTool(XRCID("btnFlipEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnSplitEdgeTool"), false);
		toolBarH->EnableTool(XRCID("btnMoveVertexTool"), false);

		ResetPartitionVisibility();
		ShowPartition(partitionTree->GetSelection());
	}
	else if (id == lightsTabButton->GetId()) {
		currentTabButton = lightsTabButton;

		outfitShapes->Hide();
		outfitBones->Hide();
		colorSettings->Hide();
		segmentTree->Hide();
		partitionTree->Hide();
		lightSettings->Show();

		meshTabButton->SetCheck(false);
		boneTabButton->SetCheck(false);
		colorsTabButton->SetCheck(false);
		segmentTabButton->SetCheck(false);
		partitionTabButton->SetCheck(false);

		SetNoSubMeshes();
		SelectTool(glView->GetLastTool());
	}

	CheckBrushBounds();
	UpdateBrushSettings();

	wxPanel* topSplitPanel = (wxPanel*)FindWindowByName("topSplitPanel");
	topSplitPanel->Layout();

	UpdateToolScrollLayout();

	Refresh();
}

void OutfitStudioFrame::OnBrushColorChanged(wxColourPickerEvent& event) {
	wxColour color = event.GetColour();
	Vector3 brushColor;
	brushColor.x = color.Red() / 255.0f;
	brushColor.y = color.Green() / 255.0f;
	brushColor.z = color.Blue() / 255.0f;
	glView->SetColorBrush(brushColor);
}

void OutfitStudioFrame::OnColorClampMaxValueSlider(wxCommandEvent& WXUNUSED(event)) {
	wxSlider* slider = (wxSlider*)colorSettings->FindWindowByName("cpClampMaxValueSlider");
	wxTextCtrl* txtControl = (wxTextCtrl*)colorSettings->FindWindowByName("cpClampMaxValueTxt");
	int clampMaxValue = slider->GetValue();
	txtControl->SetValue(wxString::Format("%d", clampMaxValue));

	ClampBrush* clampBrush = dynamic_cast<ClampBrush*>(glView->GetActiveBrush());
	if (clampBrush)
		clampBrush->clampMaxValue = clampMaxValue / 255.0f;
}

void OutfitStudioFrame::OnColorClampMaxValueChanged(wxCommandEvent& WXUNUSED(event)) {
	wxTextCtrl* txtControl = (wxTextCtrl*)colorSettings->FindWindowByName("cpClampMaxValueTxt");
	wxSlider* slider = (wxSlider*)colorSettings->FindWindowByName("cpClampMaxValueSlider");
	int clampMaxValue = std::clamp(atoi(txtControl->GetValue().c_str()), 0, 255);
	slider->SetValue(clampMaxValue);

	if (!glView)
		return;
	ClampBrush* clampBrush = dynamic_cast<ClampBrush*>(glView->GetActiveBrush());
	if (clampBrush)
		clampBrush->clampMaxValue = clampMaxValue / 255.0f;
}

void OutfitStudioFrame::OnSwapBrush(wxCommandEvent& WXUNUSED(event)) {
	ToolID activeTool = glView->GetActiveTool();
	if (activeTool == ToolID::ColorBrush)
		SelectTool(ToolID::AlphaBrush);
	else if (activeTool == ToolID::AlphaBrush)
		SelectTool(ToolID::ColorBrush);
}

void OutfitStudioFrame::OnMaskVertexColor(wxCommandEvent& WXUNUSED(event)) {
	auto cpBrushColor = (wxColourPickerCtrl*)FindWindowById(XRCID("cpBrushColor"));
	if (!cpBrushColor)
		return;

	if (!ShapeSelectionCheck())
		return;

	wxColour color = cpBrushColor->GetColour();
	Vector3 brushColor;
	brushColor.x = color.Red() / 255.0f;
	brushColor.y = color.Green() / 255.0f;
	brushColor.z = color.Blue() / 255.0f;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	auto isSimilarColor = [](const Vector3& a,
							 const Vector3& b,
							 float angleEps = 1e-3f,	   // ~cos angle tolerance
							 float chromaThresh = 1e-4f) { // how much colorfulness we require
		if (a == b)
			return true;

		auto getChromaDir = [&](const Vector3& c, Vector3& outDir) {
			float minc = std::min({c.x, c.y, c.z});
			Vector3 chroma{c.x - minc, c.y - minc, c.z - minc}; // strip added white
			float l = chroma.length();
			if (l < chromaThresh)
				return false; // basically gray/white -> no reliable hue

			outDir = {chroma.x / l, chroma.y / l, chroma.z / l};
			return true;
		};

		Vector3 da, db;
		bool okA = getChromaDir(a, da);
		bool okB = getChromaDir(b, db);

		if (!okA && !okB) // both are essentially gray/white
			return false; // treat all near-white/gray as mismatch
		if (okA != okB)
			return false; // one has hue, the other doesn't

		return da.dot(db) > 1.0f - angleEps;
	};

	for (auto& selItem : selectedItems) {
		std::string shapeName = selItem->GetShape()->name.get();
		Mesh* m = glView->GetMesh(shapeName);
		if (!m)
			continue;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];

			if (m->vcolors && isSimilarColor(m->vcolors[i], brushColor))
				uss.pointEndState[i].x = 1.0f;
			else
				uss.pointEndState[i].x = 0.0f;
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::ScrollWindowIntoView(wxScrolledWindow* scrolled, wxWindow* window) {
	// "window" must be an immediate child of "scrolled"
	int scrollRateY = 0;
	scrolled->GetScrollPixelsPerUnit(nullptr, &scrollRateY);

	wxPoint window_pos = scrolled->CalcUnscrolledPosition(window->GetPosition());
	scrolled->Scroll(0, window_pos.y / scrollRateY);
}

void OutfitStudioFrame::HighlightSlider(const std::string& name) {
	for (auto& d : sliderPanels) {
		if (d.first == name) {
			d.second->editing = true;
			d.second->SetBackgroundColour(wxColour(125, 77, 138));
		}
		else {
			d.second->editing = false;
			d.second->SetBackgroundColour(wxColour(64, 64, 64));
		}
	}
	sliderScroll->Refresh();
}

void OutfitStudioFrame::ZeroSliders() {
	if (!project->AllSlidersZero()) {
		for (size_t s = 0; s < project->SliderCount(); s++) {
			if (project->SliderClamp(s))
				continue;

			SetSliderValue(s, 0);
		}
		ApplySliders();
	}
}

void OutfitStudioFrame::OnSlider(wxCommandEvent& event) {
	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	wxString sliderName = s->GetName();
	std::string sn{sliderName.BeforeLast('|').ToUTF8()};
	if (sn.empty())
		return;

	SetSliderValue(sn, event.GetInt());

	ApplySliders();
}

void OutfitStudioFrame::OnLoadPreset(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	PresetCollection presets;
	std::vector<std::string> names;
	wxChoice* presetChoice;

	std::string choice;
	bool hi = true;

	CloseBrushSettings();

	presets.LoadPresets(ProjectUtil::GetProjectPath() + "/SliderPresets", choice, names, true);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto& n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		dlg.SetSize(dlg.FromDIP(wxSize(325, 175)));
		dlg.SetSizeHints(dlg.FromDIP(wxSize(325, 175)), wxSize(-1, -1));
		dlg.CenterOnParent();

		if (dlg.ShowModal() != wxID_OK)
			return;

		choice = presetChoice->GetStringSelection();
		if (XRCCTRL(dlg, "weightLo", wxRadioButton)->GetValue())
			hi = false;

		if (choice == "Zero All") {
			ZeroSliders();
			wxLogMessage("Sliders were reset to zero.");
			return;
		}

		wxLogMessage("Applying preset '%s' with option '%s'.", choice, hi ? "High" : "Low");

		float v;
		bool r;
		for (size_t i = 0; i < project->SliderCount(); i++) {
			if (project->SliderClamp(i))
				continue;

			if (hi)
				r = presets.GetBigPreset(choice, project->GetSliderName(i), v);
			else
				r = presets.GetSmallPreset(choice, project->GetSliderName(i), v);

			// Sliders without a value in the preset fall back to their default for the chosen weight
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

	CloseBrushSettings();

	SliderSetGroupCollection groupCollection;
	groupCollection.LoadGroups(ProjectUtil::GetProjectPath() + "/SliderGroups");

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
	for (auto& s : sliders) {
		size_t index = 0;
		if (!project->SliderIndexFromName(s, index))
			continue;

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

void OutfitStudioFrame::OnSliderImportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .nif file for slider calculation"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from NIF file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromNIF(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("No mesh found in the .nif file that matches currently selected shape!");
		wxMessageBox(_("No mesh found in the .nif file that matches currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	project->SetSliderFromBSD(activeSlider, activeItem->GetShape(), fn.ToUTF8().data());

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .obj file for slider calculation"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromOBJ(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderImportOSD(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .osd file"), wxEmptyString, wxEmptyString, ".osd", "*.osd", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing morphs from OSD file '%s'...", fn);

	OSDataFile osd;
	if (!osd.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import OSD file '%s'!", fn);
		wxMessageBox(_("Failed to import OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;
	auto& diffs = osd.GetDataDiffs();
	const auto& shapes = project->GetWorkNif()->GetShapes();
	for (auto& diff : diffs) {
		std::string bestTargetName;
		for (auto& shape : shapes) {
			std::string shapeName = shape->name.get();
			std::string targetName = project->ShapeToTarget(shapeName);

			// Diff name is supposed to begin with matching shape name
			if (diff.first.substr(0, targetName.size()) != targetName)
				continue;

			if (shapeName.length() > bestTargetName.length())
				bestTargetName = targetName;
		}

		if (bestTargetName.length() == 0)
			continue;

		// Find slider name from data name
		auto sliderName = project->activeSet.SliderFromDataName(bestTargetName, diff.first);
		if (sliderName.empty())
			sliderName = diff.first.substr(bestTargetName.length(), diff.first.length() - bestTargetName.length() + 1);

		shapeToSliders[bestTargetName].emplace(sliderName, diff.first);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("OSD Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		activeSlider.clear();
		lastActiveSlider.clear();
	}

	std::unordered_set<NiShape*> addedShapes;

	for (auto& shape : shapes) {
		// check if the shape is selected
		auto selectedSliders = options.selectedShapesToSliders.find(shape->name.get());
		if (selectedSliders == options.selectedShapesToSliders.end())
			continue;

		addedShapes.emplace(shape);

		for (auto& diff : diffs) {
			auto& sliderNameToDisplayName = selectedSliders->second;

			// check the diff is selected for the specific shape
			auto sliderName = selectedSliders->second.find(diff.first);
			if (sliderName == sliderNameToDisplayName.end())
				continue;

			if (!project->ValidSlider(sliderName->second)) {
				project->AddEmptySlider(sliderName->second);
				createSliderGUI(sliderName->second, sliderScroll, sliderScroll->GetSizer());
			}

			if (!ConfirmSliderDataLocalForEdit(shape, sliderName->second))
				continue;

			project->SetSliderFromDiff(sliderName->second, shape, *diff.second);
		}
	}

	wxString addedDiffs;
	for (auto& addedShape : addedShapes) {
		addedDiffs += addedShape->name.get() + "\n";
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

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

	wxLogMessage("Importing morphs from TRI file '%s'...", fn);

	TriFile tri;
	if (!tri.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to load TRI file '%s'!", fn);
		wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
		return;
	}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;
	auto morphs = tri.GetMorphs();
	for (auto& morph : morphs) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(morph.first);
		if (!shape)
			continue;

		for (auto& morphData : morph.second)
			shapeToSliders[shape->name.get()].emplace(morphData->name, morphData->name);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("TRI Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		activeSlider.clear();
		lastActiveSlider.clear();
	}

	wxString addedMorphs;
	std::unordered_set<NiShape*> addedShapes;

	for (auto& morph : morphs) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(morph.first);
		if (!shape)
			continue;

		// check if the shape is selected
		auto selectedSliders = options.selectedShapesToSliders.find(shape->name.get());
		if (selectedSliders == options.selectedShapesToSliders.end())
			continue;

		addedMorphs += morph.first + "\n";
		for (auto& morphData : morph.second) {
			// check the morph is selected for the specific shape
			if (selectedSliders->second.find(morphData->name) == selectedSliders->second.end())
				continue;

			if (!project->ValidSlider(morphData->name)) {
				project->AddEmptySlider(morphData->name);
				createSliderGUI(morphData->name, sliderScroll, sliderScroll->GetSizer());
			}

			std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
			if (!ConfirmSliderDataLocalForEdit(shape, morphData->name))
				continue;

			project->SetSliderFromDiff(morphData->name, shape, diff);

			if (morphData->type == MORPHTYPE_UV) {
				size_t sliderIndex = 0;
				if (!project->SliderIndexFromName(morphData->name, sliderIndex))
					continue;

				project->SetSliderUV(sliderIndex, true);
			}
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedMorphs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedMorphs), _("TRI Import"));
}

void OutfitStudioFrame::OnSliderImportMorphsSF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Import Starfield morph.dat file"), wxEmptyString, wxEmptyString, ".dat", "*.dat", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing morphs from Starfield morph file '%s'...", fn);

	SFMorphFile morphFile;
	if (!morphFile.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import Starfield morph file '%s'!", fn);
		wxMessageBox(_("Failed to import Starfield morph file!"), _("Error"), wxICON_ERROR);
		return;
	}

	if (!morphFile.FileToCacheData()) {
		wxLogError("Failed to import Starfield morph file '%s'!", fn);
		wxMessageBox(_("Failed to import Starfield morph file!"), _("Error"), wxICON_ERROR);
		return;
	}

	morphFile.UpdateCachedMorphData();
	
	auto shape = activeItem->GetShape();

	std::string shapeName = shape->name.get();
	std::string targetName = project->ShapeToTarget(shapeName);

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> shapeToSliders;

	for (auto& morphName : morphFile.GetMorphNames()) {
		// Find slider name from morph name
		auto sliderName = project->activeSet.SliderFromDataName(shapeName, morphName);
		if (sliderName.empty())
			sliderName = morphName;

		shapeToSliders[shapeName].emplace(sliderName, morphName);
	}

	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(shapeToSliders) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
		wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("Starfield Morph Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK) {
			sliderScroll->Thaw();
			return;
		}

		// Deleting sliders
		std::vector<std::string> erase;
		for (auto& sliderPanel : sliderPanels) {
			sliderPanel.second->slider->SetValue(0);
			SetSliderValue(sliderPanel.first, 0);
			ShowSliderEffect(sliderPanel.first, true);
			sliderPanel.second->slider->SetFocus();
			HideSliderPanel(sliderPanel.second);

			erase.push_back(sliderPanel.first);
			project->DeleteSlider(sliderPanel.first);
		}

		for (auto& e : erase)
			sliderPanels.erase(e);

		MenuExitSliderEdit();
		sliderScroll->FitInside();
		activeSlider.clear();
		lastActiveSlider.clear();
	}

	// Check if the shape is selected
	auto selectedSliders = options.selectedShapesToSliders.find(shapeName);
	if (selectedSliders == options.selectedShapesToSliders.end())
		return;

	for (auto& morphName : morphFile.GetMorphNames()) {
		auto& sliderNameToDisplayName = selectedSliders->second;

		// check the diff is selected for the specific shape
		auto sliderName = selectedSliders->second.find(morphName);
		if (sliderName == sliderNameToDisplayName.end())
			continue;

		if (!project->ValidSlider(sliderName->second)) {
			project->AddEmptySlider(sliderName->second);
			createSliderGUI(sliderName->second, sliderScroll, sliderScroll->GetSizer());
		}

		auto& morphIndex = morphFile.morphNamesCacheMap[morphName];
		auto& morphOffsets = morphFile.morphOffsetsCache[morphIndex];
		if (!ConfirmSliderDataLocalForEdit(shape, sliderName->second))
			continue;

		project->SetSliderFromDiff(sliderName->second, shape, morphOffsets);
	}

	wxString addedDiffs = shapeName + "\n";

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();
	HighlightSliderData();

	wxLogMessage("Added morphs for the following shapes:\n%s", addedDiffs);
	wxMessageBox(wxString::Format(_("Added morphs for the following shapes:\n\n%s"), addedDiffs), _("Starfield Morph Import"));
}

void OutfitStudioFrame::OnSliderImportFBX(wxCommandEvent& WXUNUSED(event)) {
#ifdef USE_FBXSDK
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .fbx file for slider calculation"), wxEmptyString, wxEmptyString, ".fbx", "*.fbx", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	if (!ConfirmSliderDataLocalForEdit(activeItem->GetShape(), activeSlider))
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromFBX(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
#else
	wxMessageBox(_("FBX is only supported in 64-bit builds of Outfit Studio. Start the 64-bit Outfit Studio executable instead."), _("Info"), wxICON_INFORMATION);
#endif
}

void OutfitStudioFrame::OnSliderExportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(_("Export .nif slider data to directory"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto& i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + i->GetShape()->name.get() + "_" + activeSlider + ".nif";
			wxLogMessage("Exporting NIF slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->GetShape()->name.get(), targetFile);
			project->SaveSliderNIF(activeSlider, i->GetShape(), targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(_("Export .nif slider data"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting NIF slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
		if (project->SaveSliderNIF(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
			wxLogError("Failed to export NIF file '%s'!", fn);
			wxMessageBox(_("Failed to export NIF file!"), _("Error"), wxICON_ERROR);
		}
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportBSD(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(_("Export .bsd slider data to directory"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto& i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + i->GetShape()->name.get() + "_" + activeSlider + ".bsd";
			wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->GetShape()->name.get(), targetFile);
			project->SaveSliderBSD(activeSlider, i->GetShape(), targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(_("Export .bsd slider data"), wxEmptyString, wxEmptyString, ".bsd", "*.bsd", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting BSD slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
		project->SaveSliderBSD(activeSlider, activeItem->GetShape(), fn.ToUTF8().data());
	}

	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to export data from!"), _("Error"));
		return;
	}

	if (selectedItems.size() > 1) {
		wxString dir = wxDirSelector(_("Export .obj slider data to directory"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
		if (dir.IsEmpty())
			return;

		for (auto& i : selectedItems) {
			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + i->GetShape()->name.get() + "_" + activeSlider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, i->GetShape()->name.get(), targetFile);
			project->SaveSliderOBJ(activeSlider, i->GetShape(), targetFile);
		}
	}
	else {
		wxString fn = wxFileSelector(_("Export .obj slider data"), wxEmptyString, wxEmptyString, ".obj", "*.obj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
		if (fn.IsEmpty())
			return;

		wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
		if (project->SaveSliderOBJ(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
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

void OutfitStudioFrame::OnSliderExportMorphsSF(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!ShapeSelectionCheck())
		return;

	wxString fn = wxFileSelector(_("Export Starfield morph.dat"), wxEmptyString, wxEmptyString, ".dat", "*.dat", wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Exporting Starfield morph.dat to '%s'...", fn);
	if (!project->WriteSFMorphs(activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Failed to export Starfield morph.dat file to '%s'!", fn);
		wxMessageBox(_("Failed to export Starfield morph.dat file!"), _("Error"), wxICON_ERROR);
		return;
	}
}

void OutfitStudioFrame::OnSliderExportToOBJs(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	wxString dir = wxDirSelector(_("Export .obj slider data to directory"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
	if (dir.IsEmpty())
		return;

	std::vector<std::string> sliderList;
	project->GetSliderList(sliderList);

	wxLogMessage("Exporting sliders to OBJ files in '%s'...", dir);
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		for (auto& slider : sliderList) {
			if (!project->SliderShow(slider))
				continue;

			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + shape->name.get() + "_" + slider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", slider, shape->name.get(), targetFile);
			if (project->SaveSliderOBJ(slider, shape, targetFile, true))
				wxLogError("Failed to export OBJ file '%s'!", targetFile);
		}
	}
}

void OutfitStudioFrame::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	int result;
	if (selectedItems.size() > 1) {
		wxString prompt = _("Are you sure you wish to clear the unmasked slider data for the selected shapes?  This action cannot be undone.");
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}
	else {
		wxString prompt = wxString::Format(_("Are you sure you wish to clear the unmasked slider data for the shape '%s'?  This action cannot be undone."),
										   activeItem->GetShape()->name.get());
		result = wxMessageBox(prompt, _("Confirm data erase"), wxYES_NO | wxICON_WARNING, this);
	}

	if (result != wxYES)
		return;

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	std::vector<std::string> sliderNames;
	if (!bEditSlider) {
		for (auto& sliderPanel : sliderPanels)
			if (sliderPanel.second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED)
				sliderNames.push_back(sliderPanel.first);
	}
	else
		sliderNames.push_back(activeSlider);

	if (!ConfirmSliderDataLocalForEdit(shapes, sliderNames))
		return;

	auto clearSlider = [&](const std::string& sliderName) {
		std::unordered_map<uint16_t, float> mask;
		for (auto& i : selectedItems) {
			mask.clear();
			glView->GetShapeMask(mask, i->GetShape()->name.get());
			if (mask.size() > 0)
				project->ClearUnmaskedDiff(i->GetShape(), sliderName, &mask);
			else
				project->ClearSlider(i->GetShape(), sliderName);
		}
	};

	if (!bEditSlider) {
		wxLogMessage("Clearing slider data of the checked sliders for the selected shapes.");
		for (auto& sliderPanel : sliderPanels)
			if (sliderPanel.second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED)
				clearSlider(sliderPanel.first);
	}
	else {
		wxLogMessage("Clearing slider data of '%s' for the selected shapes.", activeSlider);
		clearSlider(activeSlider);
	}

	SetPendingChanges();
	ApplySliders();
	HighlightSliderData();
}

void OutfitStudioFrame::OnNewSlider(wxCommandEvent& WXUNUSED(event)) {
	NewSlider();
}

void OutfitStudioFrame::OnNewZapSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	std::string baseName = "New Zap";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = wxGetTextFromUser(_("Enter a name for the new zap:"), _("Create New Zap"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating new zap '%s'.", sliderName);

	std::unordered_map<uint16_t, float> unmasked;
	for (auto& i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->GetShape()->name.get());
		project->AddZapSlider(sliderName, unmasked, i->GetShape());
	}

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();
}

void OutfitStudioFrame::OnNewCombinedSlider(wxCommandEvent& WXUNUSED(event)) {
	std::string baseName = "New Slider";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = wxGetTextFromUser(_("Enter a name for the new slider:"), _("Create New Slider"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating new combined slider '%s'.", sliderName);

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	project->AddCombinedSlider(sliderName);
	sliderScroll->FitInside();
	SetPendingChanges();
}

void OutfitStudioFrame::OnSliderClone(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to clone!"), _("Error"));
		return;
	}

	wxLogMessage("Cloning slider '%s'.", activeSlider);

	std::string baseName = "Cloned Slider";

	int count = 1;
	std::string fillName = baseName;

	while (project->ValidSlider(fillName))
		fillName = wxString::Format("%s %d", baseName, ++count).ToUTF8();

	std::string sliderName;
	do {
		sliderName = wxGetTextFromUser(_("Enter a name for the cloned slider:"), _("Clone Slider"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating cloned slider '%s'.", sliderName);

	project->CloneSlider(activeSlider, sliderName);
	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	sliderScroll->FitInside();
	SetPendingChanges();
	HighlightSliderData();
}

void OutfitStudioFrame::OnSliderNegate(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to negate!"), _("Error"));
		return;
	}

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{activeSlider}))
		return;

	wxLogMessage("Negating slider '%s' for the selected shapes.", activeSlider);
	for (auto& i : selectedItems)
		project->NegateSlider(activeSlider, i->GetShape());

	SetPendingChanges();
}

void OutfitStudioFrame::OnMaskAffected(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to create a mask from!"), _("Error"));
		return;
	}

	wxLogMessage("Creating mask for affected vertices of the slider '%s'.", activeSlider);
	for (auto& i : selectedItems)
		project->MaskAffected(activeSlider, i->GetShape());
}

void OutfitStudioFrame::OnDeleteSlider(wxCommandEvent& WXUNUSED(event)) {
	wxString prompt = _("Are you sure you wish to delete the selected slider(s)?");
	int result = wxMessageBox(prompt, _("Confirm slider delete"), wxYES_NO | wxICON_WARNING, this);
	if (result != wxYES)
		return;

	DeleteSliders();
}

void OutfitStudioFrame::DeleteSliders(bool keepSliders, bool keepZaps) {
	if (keepSliders && keepZaps)
		return;

	auto deleteSlider = [&](const std::string& sliderName) {
		wxSliderPanel* sliderPanel = sliderPanels[sliderName];
		sliderPanel->slider->SetValue(0);
		SetSliderValue(sliderName, 0);
		ShowSliderEffect(sliderName, true);
		sliderPanel->slider->SetFocus();
		HideSliderPanel(sliderPanel);

		sliderScroll->FitInside();
		project->DeleteSlider(sliderName);
	};

	if (!bEditSlider) {
		for (auto it = sliderPanels.begin(); it != sliderPanels.end();) {
			if (it->second->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_CHECKED) {
				if ((keepZaps && project->activeSet[it->first].bZap) || (keepSliders && !project->activeSet[it->first].bZap)) {
					++it;
					continue;
				}
				deleteSlider(it->first);
				it = sliderPanels.erase(it);
			}
			else
				++it;
		}
	}
	else {
		wxLogMessage("Deleting slider '%s'.", activeSlider);

		deleteSlider(activeSlider);
		sliderPanels.erase(activeSlider);

		MenuExitSliderEdit();
		activeSlider.clear();
		lastActiveSlider.clear();
		bEditSlider = false;
	}

	SetPendingChanges();
	ApplySliders();
}

void OutfitStudioFrame::ShowSliderProperties(const std::string& sliderName) {
	size_t curSlider = 0;
	if (!project->SliderIndexFromName(sliderName, curSlider))
		return;

	CloseBrushSettings();

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSliderProp")) {
		dlg.SetSize(dlg.FromDIP(wxSize(760, 560)));
		dlg.SetMinSize(dlg.FromDIP(wxSize(500, 480)));

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
		wxListCtrl* sliderDataList = XRCCTRL(dlg, "sliderDataList", wxListCtrl);
		wxButton* btnSliderDataLocal = XRCCTRL(dlg, "btnSliderDataLocal", wxButton);
		wxButton* btnSliderDataExternal = XRCCTRL(dlg, "btnSliderDataExternal", wxButton);
		wxButton* btnSliderDataFolders = XRCCTRL(dlg, "btnSliderDataFolders", wxButton);

		std::vector<SliderDataLocation> sliderDataRows;
		auto updateSliderDataButtons = [&]() {
			std::vector<size_t> selectedRows;
			size_t selectionCount = SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			bool allLocal = SliderDataList::AllHaveSource(sliderDataRows, selectedRows, true);
			bool allExternal = SliderDataList::AllHaveSource(sliderDataRows, selectedRows, false);
			btnSliderDataLocal->Enable(selectionCount > 0 && allExternal);
			btnSliderDataExternal->Enable(selectionCount > 0 && allLocal);
			btnSliderDataFolders->Enable(selectionCount > 0 && allExternal);
		};

		auto refreshSliderData = [&]() {
			project->GetSliderDataLocations(sliderDataRows, sliderName);
			SliderDataList::Populate(sliderDataList, sliderDataRows, nullptr, false);
			updateSliderDataButtons();
		};

		SliderDataList::Configure(sliderDataList, false);

		long loVal = (int)(project->SliderDefault(curSlider, false));
		long hiVal = (int)(project->SliderDefault(curSlider, true));

		edSliderName->SetValue(wxString::FromUTF8(activeSlider));
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

			for (size_t i = 0; i < project->SliderCount(); i++)
				if (i != curSlider && (project->SliderZap(i) || project->SliderHidden(i)))
					zapToggleList->Append(wxString::FromUTF8(project->GetSliderName(i)));

			for (auto& s : project->SliderZapToggles(curSlider)) {
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
				lbValHi->SetLabel(_("Default"));
			}
		}

		if (hiVal > 0 || loVal > 0)
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		else
			cbValZapped->Set3StateValue(wxCheckBoxState::wxCHK_UNCHECKED);

		refreshSliderData();
		sliderDataList->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent&) { updateSliderDataButtons(); });
		sliderDataList->Bind(wxEVT_LIST_ITEM_DESELECTED, [&](wxListEvent&) { updateSliderDataButtons(); });
		SliderDataList::BindSelectAll(&dlg, sliderDataList, updateSliderDataButtons);
		btnSliderDataLocal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::MakeLocal(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});
		btnSliderDataExternal->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::EditFolders(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});
		btnSliderDataFolders->Bind(wxEVT_BUTTON, [&](wxCommandEvent&) {
			std::vector<size_t> selectedRows;
			SliderDataList::GetSelectedRows(sliderDataList, sliderDataRows, selectedRows);
			if (SliderDataList::EditFolders(&dlg, project, sliderDataRows, selectedRows)) {
				SetPendingChanges();
				refreshSliderData();
			}
		});

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
				for (auto& i : toggled)
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

			std::string newSliderName{edSliderName->GetValue().ToUTF8()};
			if (sliderName != newSliderName && !project->ValidSlider(newSliderName)) {
				project->SetSliderName(curSlider, newSliderName);
				wxSliderPanel* d = sliderPanels[sliderName];
				sliderPanels[newSliderName] = d;
				sliderPanels.erase(sliderName);

				wxString sn = wxString::FromUTF8(newSliderName);
				d->slider->SetName(sn + "|slider");
				d->sliderName->SetName(sn + "|lbl");
				d->btnSliderEdit->SetName(sn + "|btn");
				d->btnSliderProp->SetName(sn + "|btnSliderProp");
				d->btnMinus->SetName(sn + "|btnMinus");
				d->btnPlus->SetName(sn + "|btnPlus");
				d->sliderCheck->SetName(sn + "|check");
				d->sliderReadout->SetName(sn + "|readout");
				d->sliderName->SetLabel(sn);

				if (sliderName == activeSlider)
					activeSlider = std::move(newSliderName);
			}

			SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnSliderProperties(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to show properties for!"), _("Error"));
		return;
	}

	ShowSliderProperties(activeSlider);
}

void OutfitStudioFrame::OnSliderDataLocations(wxCommandEvent& WXUNUSED(event)) {
	SliderDataLocationsDialog dlg(this, project);
	dlg.ShowModal();
}

bool OutfitStudioFrame::ShowClippingFixStrength(float& outStrength) {
	int strengthPct = 50;

	wxDialog dlg(this, wxID_ANY, _("Fix Clipping"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
	auto* sizer = new wxBoxSizer(wxVERTICAL);

	auto* label = new wxStaticText(&dlg, wxID_ANY, wxString::Format(_("Strength: %d"), strengthPct));
	auto* slider = new wxSlider(&dlg, wxID_ANY, strengthPct, 1, 100);

	slider->Bind(wxEVT_SLIDER, [&](wxCommandEvent&) {
		strengthPct = slider->GetValue();
		label->SetLabel(wxString::Format(_("Strength: %d"), strengthPct));
	});

	sizer->Add(label, 0, wxALL, 10);
	sizer->Add(slider, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
	dlg.SetSizerAndFit(sizer);
	dlg.CenterOnParent();

	if (dlg.ShowModal() != wxID_OK)
		return false;

	outStrength = strengthPct / 100.0f;
	return true;
}

void OutfitStudioFrame::FixClippingForShape(const std::vector<Vector3>& bodyVerts,
											const std::vector<Triangle>& bodyTris,
											NiShape* shape,
											const std::vector<Vector3>& outfitVerts,
											const ClippingFixOptions& options,
											UndoStateProject* usp,
											const std::unordered_set<uint16_t>* allowedVerts) {
	if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
		return;

	std::vector<Triangle> outfitTris;
	shape->GetTriangles(outfitTris);
	if (outfitTris.empty())
		return;

	std::vector<Vector3> fixedVerts = outfitVerts;
	ClippingFixer::FixClipping(bodyVerts, bodyTris, fixedVerts, outfitTris, options);

	UndoStateShape uss;
	uss.shapeName = shape->name.get();

	for (size_t i = 0; i < outfitVerts.size(); i++) {
		Vector3 diff = fixedVerts[i] - outfitVerts[i];
		if (diff.IsZero(true))
			continue;

		if (allowedVerts && allowedVerts->find(static_cast<uint16_t>(i)) == allowedVerts->end())
			continue;

		uss.pointStartState[i] = Mesh::TransformPosNifToMesh(outfitVerts[i]);
		uss.pointEndState[i] = Mesh::TransformPosNifToMesh(fixedVerts[i]);
	}

	if (!uss.pointStartState.empty())
		usp->usss.push_back(std::move(uss));
}

void OutfitStudioFrame::OnSliderFixClipping(wxCommandEvent& WXUNUSED(event)) {
	if (!bEditSlider) {
		wxMessageBox(_("You must be in slider edit mode to fix clipping for a slider."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	NiShape* refShape = project->GetBaseShape();
	if (!refShape) {
		wxMessageBox(_("No reference shape set."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	float strength = 0.0f;
	if (!ShowClippingFixStrength(strength))
		return;

	ClippingFixOptions options;
	options.strength = strength;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::VertexPosition;

	float sliderscale = project->SliderValue(activeSlider);
	if (sliderscale == 0.0)
		sliderscale = 1.0;

	for (auto& sel : selectedItems) {
		NiShape* shape = sel->GetShape();
		if (project->IsBaseShape(shape))
			continue;

		if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
			continue;

		// Only process shapes that have a diff for the active slider
		size_t sliderIndex = 0;
		if (!project->SliderIndexFromName(activeSlider, sliderIndex))
			continue;

		TargetDataDiffs* diffSet = project->GetDiffSet(project->activeSet[sliderIndex], shape);
		if (!diffSet || diffSet->empty())
			continue;

		std::unordered_map<uint16_t, float> unmasked;
		glView->GetShapeUnmasked(unmasked, shape->name.get());

		std::unordered_set<uint16_t> allowed;
		const std::unordered_set<uint16_t>* allowedVerts = nullptr;
		if (!unmasked.empty()) {
			for (auto& v : unmasked)
				allowed.insert(v.first);

			if (allowed.empty())
				continue;

			allowedVerts = &allowed;
		}

		TargetDataDiffs morphDiffs;
		project->CalcSliderClippingCorrection(shape, activeSlider, options.strength, morphDiffs, allowedVerts);
		if (morphDiffs.empty())
			continue;

		std::vector<Vector3> outfitVerts;
		project->GetLiveVerts(shape, outfitVerts);

		UndoStateShape uss;
		uss.shapeName = shape->name.get();

		for (auto& diffEntry : morphDiffs) {
			uint16_t vertIndex = diffEntry.first;
			if (vertIndex >= outfitVerts.size())
				continue;

			Vector3 start = Mesh::TransformPosNifToMesh(outfitVerts[vertIndex]);
			uss.pointStartState[vertIndex] = start;
			uss.pointEndState[vertIndex] = start + diffEntry.second * sliderscale;
		}

		if (!uss.pointStartState.empty())
			usp->usss.push_back(std::move(uss));
	}

	if (usp->usss.empty()) {
		glView->GetUndoHistory()->PopState();
		return;
	}

	usp->sliderName = activeSlider;
	usp->sliderscale = sliderscale;

	glView->ApplyUndoState(usp, false);

	if (glView->GetTransformMode())
		glView->ShowTransformTool();

	UpdateUndoTools();
	HighlightSliderData();
}

void OutfitStudioFrame::ConformSliders(NiShape* shape, const ConformOptions& options) {
	if (project->IsBaseShape(shape))
		return;

	std::string shapeName = shape->name.get();
	wxLogMessage("Conforming '%s'...", shapeName);
	UpdateProgress(50, _("Conforming: ") + shapeName);

	Mesh* m = glView->GetMesh(shapeName);
	if (m) {
		project->morpher.CopyMeshMask(m, shapeName);
		project->ConformShape(shape, options);
	}

	UpdateProgress(99);
}

void OutfitStudioFrame::OnSliderConform(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetBaseShape())
		return;

	std::vector<NiShape*> shapes;
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape());

	ConformShapes(shapes);
}

void OutfitStudioFrame::OnSliderConformAll(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetBaseShape())
		return;

	auto shapes = project->GetWorkNif()->GetShapes();
	ConformShapes(shapes);
}

int OutfitStudioFrame::ConformShapes(std::vector<NiShape*> shapes, bool silent) {
	if (shapes.empty())
		return 0;

	StartProgress(_("Conforming shapes..."));

	ConformOptions options;
	if (ShowConform(options, silent)) {
		wxLogMessage("Conforming shapes...");

		// Collect shown slider names before zeroing
		for (size_t i = 0; i < project->SliderCount(); i++)
			if (project->SliderShow(i))
				options.sliderNames.push_back(project->GetSliderName(i));

		std::vector<NiShape*> targetShapes;
		for (auto* shape : shapes)
			if (shape && !project->IsBaseShape(shape))
				targetShapes.push_back(shape);

		std::vector<std::string> conformSliderNames;
		project->GetConformSliderNames(options, conformSliderNames);
		if (!ConfirmSliderDataLocalForEdit(targetShapes, conformSliderNames)) {
			EndProgress();
			HighlightSliderData();
			return 0;
		}

		ZeroSliders();

		project->InitConform();

		int inc = 100 / shapes.size() - 1;
		int pos = 0;

		for (auto& shape : shapes) {
			UpdateProgress(pos * inc, _("Conforming: ") + shape->name.get());
			StartSubProgress(pos * inc, pos * inc + inc);
			ConformSliders(shape, options);
			pos++;
			EndProgress();
		}

		project->morpher.ClearProximityCache();
		project->morpher.UnlinkRefDiffData();

		if (statusBar)
			statusBar->SetStatusText(_("All shapes conformed."));

		SetPendingChanges();

		wxLogMessage("All shapes conformed.");
	}

	EndProgress();
	HighlightSliderData();
	return 0;
}

bool OutfitStudioFrame::ShowConform(ConformOptions& options, bool silent) {
	CloseBrushSettings();

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgConforming")) {
		XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			float changed = XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->GetValue() / 1000.0f;
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", changed));
		});

		XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			float changed = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(changed * 1000);
		});

		XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Bind(wxEVT_SLIDER, [&dlg](wxCommandEvent&) {
			int changed = XRCCTRL(dlg, "maxResultsSlider", wxSlider)->GetValue();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue(wxString::Format("%d", changed));
		});

		XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&dlg](wxCommandEvent&) {
			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Enable(!noTargetLimit);
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Enable(!noTargetLimit);

			if (noTargetLimit) {
				XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
				XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
			}
		});

		auto updateSmoothingState = [&dlg]() {
			bool smoothResults = XRCCTRL(dlg, "smoothResults", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Enable(smoothResults);
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Enable(smoothResults);
		};

		XRCCTRL(dlg, "smoothResults", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&updateSmoothingState](wxCommandEvent&) {
			updateSmoothingState();
		});
		updateSmoothingState();

		auto updateFixClippingState = [&dlg]() {
			bool fixClipping = XRCCTRL(dlg, "fixClipping", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Enable(fixClipping);
		};

		XRCCTRL(dlg, "fixClipping", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&updateFixClippingState](wxCommandEvent&) {
			updateFixClippingState();
		});
		updateFixClippingState();

		XRCCTRL(dlg, "presetDefault", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("10.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(10000);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Enable();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->ChangeValue("10");
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->SetValue(10);
		});

		XRCCTRL(dlg, "presetEvenMovement", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
		});

		XRCCTRL(dlg, "presetSmoothClipping", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Enable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
		});

		XRCCTRL(dlg, "presetSolidObject", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "smoothResults", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "fixClipping", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->ChangeValue("50");
			XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("10.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(10000);
		});

		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (silent || dlg.ShowModal() == wxID_OK) {
			options.proximityRadius = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());

			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			if (!noTargetLimit)
				options.maxResults = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			else
				options.maxResults = std::numeric_limits<int>::max();

			options.smoothResultDeltas = XRCCTRL(dlg, "smoothResults", wxCheckBox)->IsChecked();
			options.smoothIterations = atol(XRCCTRL(dlg, "smoothIterationsText", wxTextCtrl)->GetValue().c_str());
			options.smoothStrength = atof(XRCCTRL(dlg, "smoothStrengthText", wxTextCtrl)->GetValue().c_str());
			options.noSqueeze = XRCCTRL(dlg, "noSqueeze", wxCheckBox)->IsChecked();
			options.solidMode = XRCCTRL(dlg, "solidMode", wxCheckBox)->IsChecked();
			options.axisX = XRCCTRL(dlg, "axisX", wxCheckBox)->IsChecked();
			options.axisY = XRCCTRL(dlg, "axisY", wxCheckBox)->IsChecked();
			options.axisZ = XRCCTRL(dlg, "axisZ", wxCheckBox)->IsChecked();
			options.fixClipping = XRCCTRL(dlg, "fixClipping", wxCheckBox)->IsChecked();
			options.fixClippingStrength = std::max(0.0f, std::min(1.0f, static_cast<float>(atof(XRCCTRL(dlg, "fixClippingStrengthText", wxTextCtrl)->GetValue().c_str()) / 100.0f)));
			return true;
		}
	}

	return false;
}

void OutfitStudioFrame::OnInvertUV(wxCommandEvent& event) {
	if (!ShapeSelectionCheck())
		return;

	bool invertX = (event.GetId() == XRCID("uvInvertX"));
	bool invertY = (event.GetId() == XRCID("uvInvertY"));

	for (auto& i : selectedItems) {
		project->GetWorkNif()->InvertUVsForShape(i->GetShape(), invertX, invertY);
	}

	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnMirrorShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMirrorShape")) {
		bool previewMirror = false;

		auto updateMirrorPreview = [&]() {
			if (previewMirror) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
					previewMirror = false;
				}
			}

			bool mirrorAxisX = XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->IsChecked();
			bool mirrorAxisY = XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->IsChecked();
			bool mirrorAxisZ = XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->IsChecked();

			wxCheckBox* swapBonesX = XRCCTRL(dlg, "swapBonesX", wxCheckBox);
			if (mirrorAxisX && !swapBonesX->IsEnabled())
				swapBonesX->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);

			swapBonesX->Enable(mirrorAxisX);

			if (mirrorAxisX || mirrorAxisY || mirrorAxisZ) {
				UndoStateProject* usp = glView->GetUndoHistory()->PushState();
				usp->undoType = UndoType::Mirror;
				usp->mirrorX = mirrorAxisX;
				usp->mirrorY = mirrorAxisY;
				usp->mirrorZ = mirrorAxisZ;
				usp->swapBonesX = swapBonesX->IsChecked();

				for (auto& sel : selectedItems) {
					NiShape* shape = sel->GetShape();

					UndoStateShape uss;
					uss.shapeName = shape->name.get();

					usp->usss.push_back(std::move(uss));
				}

				glView->ApplyUndoState(usp, false);

				previewMirror = true;

				if (glView->GetTransformMode())
					glView->ShowTransformTool();
			}
		};

		auto mirrorAxisChanged = [&](wxCommandEvent&) { updateMirrorPreview(); };

		XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "swapBonesX", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (previewMirror) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
		SetPendingChanges();
	}
}

void OutfitStudioFrame::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	std::string shapeName = activeItem->GetShape()->name.get();
	std::string newShapeName;
	do {
		std::string result{wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"), wxString::FromUTF8(shapeName), this).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	wxLogMessage("Renaming shape '%s' to '%s'.", shapeName, newShapeName);
	project->RenameShape(activeItem->GetShape(), newShapeName);
	glView->RenameShape(shapeName, newShapeName);

	outfitShapes->SetItemText(activeItem->GetId(), wxString::FromUTF8(newShapeName));
	SetPendingChanges();
}

void OutfitStudioFrame::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	auto shape = activeItem->GetShape();
	if (!project->IsBaseShape(shape))
		project->SetBaseShape(shape);
	else
		project->SetBaseShape(nullptr);

	RefreshGUIFromProj();
	SetPendingChanges();
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

void OutfitStudioFrame::OnLinkClicked(wxHtmlLinkEvent& link) {
	wxLaunchDefaultBrowser(link.GetLinkInfo().GetHref());
}

void OutfitStudioFrame::OnMoveShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgMoveShape")) {
		Vector3 previewMove;

		auto updateMovePreview = [&]() {
			std::unordered_map<uint16_t, float> mask;
			std::unordered_map<uint16_t, float>* mptr = nullptr;
			std::vector<Vector3> verts;

			if (!previewMove.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
				}
			}

			Vector3 changed;
			changed.x = atof(XRCCTRL(dlg, "msTextX", wxTextCtrl)->GetValue().c_str());
			changed.y = atof(XRCCTRL(dlg, "msTextY", wxTextCtrl)->GetValue().c_str());
			changed.z = atof(XRCCTRL(dlg, "msTextZ", wxTextCtrl)->GetValue().c_str());

			bool mirrorAxisX = XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->IsChecked();
			bool mirrorAxisY = XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->IsChecked();
			bool mirrorAxisZ = XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->IsChecked();

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UndoType::VertexPosition;

			for (auto& sel : selectedItems) {
				mask.clear();
				mptr = nullptr;

				NiShape* shape = sel->GetShape();
				project->GetLiveVerts(shape, verts);
				glView->GetShapeMask(mask, shape->name.get());

				if (!mask.empty())
					mptr = &mask;

				UndoStateShape uss;
				uss.shapeName = shape->name.get();

				for (size_t i = 0; i < verts.size(); i++) {
					Vector3& vertPos = verts[i];
					Vector3 diff = changed;

					if (mptr)
						diff *= 1.0f - mask[i];

					if (diff.IsZero(true))
						continue;

					if (mirrorAxisX && vertPos.x < 0.0f)
						diff.x = -diff.x;
					if (mirrorAxisY && vertPos.y < 0.0f)
						diff.y = -diff.y;
					if (mirrorAxisZ && vertPos.z < 0.0f)
						diff.z = -diff.z;

					Vector3 newPos = vertPos + diff;
					uss.pointStartState[i] = Mesh::TransformPosNifToMesh(vertPos);
					uss.pointEndState[i] = Mesh::TransformPosNifToMesh(newPos);
				}

				usp->usss.push_back(std::move(uss));
			}

			if (bEditSlider) {
				usp->sliderName = activeSlider;

				float sliderscale = project->SliderValue(activeSlider);
				if (sliderscale == 0.0)
					sliderscale = 1.0;

				usp->sliderscale = sliderscale;
			}

			glView->ApplyUndoState(usp, false);

			previewMove = changed;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
		};

		auto sliderMoved = [&](wxCommandEvent&) {
			Vector3 slider;
			slider.x = XRCCTRL(dlg, "msSliderX", wxSlider)->GetValue() / 1000.0f;
			slider.y = XRCCTRL(dlg, "msSliderY", wxSlider)->GetValue() / 1000.0f;
			slider.z = XRCCTRL(dlg, "msSliderZ", wxSlider)->GetValue() / 1000.0f;

			XRCCTRL(dlg, "msTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.x));
			XRCCTRL(dlg, "msTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.y));
			XRCCTRL(dlg, "msTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", slider.z));

			updateMovePreview();
		};

		auto textChanged = [&](wxCommandEvent&) {
			Vector3 changed;
			changed.x = atof(XRCCTRL(dlg, "msTextX", wxTextCtrl)->GetValue().c_str());
			changed.y = atof(XRCCTRL(dlg, "msTextY", wxTextCtrl)->GetValue().c_str());
			changed.z = atof(XRCCTRL(dlg, "msTextZ", wxTextCtrl)->GetValue().c_str());

			XRCCTRL(dlg, "msSliderX", wxSlider)->SetValue(changed.x * 1000);
			XRCCTRL(dlg, "msSliderY", wxSlider)->SetValue(changed.y * 1000);
			XRCCTRL(dlg, "msSliderZ", wxSlider)->SetValue(changed.z * 1000);

			updateMovePreview();
		};

		auto mirrorAxisChanged = [&](wxCommandEvent&) { updateMovePreview(); };

		XRCCTRL(dlg, "msSliderX", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "msSliderY", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "msSliderZ", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);

		XRCCTRL(dlg, "msTextX", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "msTextY", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "msTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);

		XRCCTRL(dlg, "mirrorAxisX", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisY", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		XRCCTRL(dlg, "mirrorAxisZ", wxCheckBox)->Bind(wxEVT_CHECKBOX, mirrorAxisChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (!previewMove.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
	}
}

void OutfitStudioFrame::OnScaleShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgScaleShape")) {
		Vector3 previewScale(1.0f, 1.0f, 1.0f);

		auto updateScalePreview = [&]() {
			std::unordered_map<uint16_t, float> mask;
			std::unordered_map<uint16_t, float>* mptr = nullptr;
			std::vector<Vector3> verts;

			if (previewScale != Vector3(1.0f, 1.0f, 1.0f)) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
				}
			}

			Vector3 scale;
			scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
			scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
			scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());

			if (scale.x < 0.01f)
				scale.x = 0.01f;
			if (scale.y < 0.01f)
				scale.y = 0.01f;
			if (scale.z < 0.01f)
				scale.z = 0.01f;

			Vector3 origin;
			int originSelection = XRCCTRL(dlg, "origin", wxChoice)->GetCurrentSelection();
			if (originSelection == 1) {
				// Center of selected shape(s), respecting mask
				origin = glView->gls.GetActiveCenter();
				origin = Mesh::TransformPosMeshToNif(origin);
			}

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UndoType::VertexPosition;

			for (auto& sel : selectedItems) {
				mask.clear();
				mptr = nullptr;

				NiShape* shape = sel->GetShape();
				project->GetLiveVerts(shape, verts);
				glView->GetShapeMask(mask, shape->name.get());

				if (!mask.empty())
					mptr = &mask;

				UndoStateShape uss;
				uss.shapeName = shape->name.get();

				Matrix4 xform;
				xform.PushTranslate(origin);
				xform.PushScale(scale.x, scale.y, scale.z);
				xform.PushTranslate(origin * -1.0f);

				for (size_t i = 0; i < verts.size(); i++) {
					Vector3& vertPos = verts[i];
					Vector3 diff = xform * vertPos - vertPos;

					if (mptr)
						diff *= 1.0f - mask[i];

					if (diff.IsZero(true))
						continue;

					Vector3 newPos = vertPos + diff;
					uss.pointStartState[i] = Mesh::TransformPosNifToMesh(vertPos);
					uss.pointEndState[i] = Mesh::TransformPosNifToMesh(newPos);
				}

				usp->usss.push_back(std::move(uss));
			}

			if (bEditSlider) {
				usp->sliderName = activeSlider;

				float sliderscale = project->SliderValue(activeSlider);
				if (sliderscale == 0.0)
					sliderscale = 1.0;

				usp->sliderscale = sliderscale;
			}

			glView->ApplyUndoState(usp, false);

			previewScale = scale;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
		};

		auto sliderMoved = [&](wxCommandEvent& event) {
			Vector3 scale(1.0f, 1.0f, 1.0f);

			bool uniform = XRCCTRL(dlg, "ssUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / 1000.0f;
				scale = Vector3(uniformValue, uniformValue, uniformValue);

				XRCCTRL(dlg, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
				XRCCTRL(dlg, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
				XRCCTRL(dlg, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);
			}
			else {
				scale.x = XRCCTRL(dlg, "ssSliderX", wxSlider)->GetValue() / 1000.0f;
				scale.y = XRCCTRL(dlg, "ssSliderY", wxSlider)->GetValue() / 1000.0f;
				scale.z = XRCCTRL(dlg, "ssSliderZ", wxSlider)->GetValue() / 1000.0f;
			}

			XRCCTRL(dlg, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
			XRCCTRL(dlg, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
			XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));

			updateScalePreview();
		};

		auto textChanged = [&](wxCommandEvent& event) {
			Vector3 scale(1.0f, 1.0f, 1.0f);

			bool uniform = XRCCTRL(dlg, "ssUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
				scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
				if (scale.x != uniformValue)
					XRCCTRL(dlg, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
				if (scale.y != uniformValue)
					XRCCTRL(dlg, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());
				if (scale.z != uniformValue)
					XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				scale = Vector3(uniformValue, uniformValue, uniformValue);
			}
			else {
				scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
				scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
				scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());
			}

			XRCCTRL(dlg, "ssSliderX", wxSlider)->SetValue(scale.x * 1000);
			XRCCTRL(dlg, "ssSliderY", wxSlider)->SetValue(scale.y * 1000);
			XRCCTRL(dlg, "ssSliderZ", wxSlider)->SetValue(scale.z * 1000);

			updateScalePreview();
		};

		auto originChanged = [&](wxCommandEvent&) { updateScalePreview(); };

		XRCCTRL(dlg, "ssSliderX", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "ssSliderY", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "ssSliderZ", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "ssTextX", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "ssTextY", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "origin", wxChoice)->Bind(wxEVT_CHOICE, originChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (previewScale != Vector3(1.0f, 1.0f, 1.0f)) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
	}
}

void OutfitStudioFrame::OnRotateShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgRotateShape")) {
		Vector3 previewRotation;

		auto updateRotationPreview = [&]() {
			std::unordered_map<uint16_t, float> mask;
			std::unordered_map<uint16_t, float>* mptr = nullptr;
			std::vector<Vector3> verts;

			if (!previewRotation.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
				}
			}

			Vector3 angle;
			angle.x = atof(XRCCTRL(dlg, "rsTextX", wxTextCtrl)->GetValue().c_str());
			angle.y = atof(XRCCTRL(dlg, "rsTextY", wxTextCtrl)->GetValue().c_str());
			angle.z = atof(XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->GetValue().c_str());

			Vector3 origin;
			int originSelection = XRCCTRL(dlg, "origin", wxChoice)->GetCurrentSelection();
			if (originSelection == 1) {
				// Center of selected shape(s), respecting mask
				origin = glView->gls.GetActiveCenter();
				origin = Mesh::TransformPosMeshToNif(origin);
			}

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UndoType::VertexPosition;

			for (auto& sel : selectedItems) {
				mask.clear();
				mptr = nullptr;

				NiShape* shape = sel->GetShape();
				project->GetLiveVerts(shape, verts);
				glView->GetShapeMask(mask, shape->name.get());

				if (!mask.empty())
					mptr = &mask;

				UndoStateShape uss;
				uss.shapeName = shape->name.get();

				Matrix4 xform;
				xform.PushTranslate(origin);
				xform.PushRotate(angle.x * DEG2RAD, Vector3(1.0f, 0.0f, 0.0f));
				xform.PushRotate(angle.y * DEG2RAD, Vector3(0.0f, 1.0f, 0.0f));
				xform.PushRotate(angle.z * DEG2RAD, Vector3(0.0f, 0.0f, 1.0f));
				xform.PushTranslate(origin * -1.0f);

				for (size_t i = 0; i < verts.size(); i++) {
					Vector3& vertPos = verts[i];
					Vector3 diff = xform * vertPos - vertPos;

					if (mptr)
						diff *= 1.0f - mask[i];

					if (diff.IsZero(true))
						continue;

					Vector3 newPos = vertPos + diff;
					uss.pointStartState[i] = Mesh::TransformPosNifToMesh(vertPos);
					uss.pointEndState[i] = Mesh::TransformPosNifToMesh(newPos);
				}

				usp->usss.push_back(std::move(uss));
			}

			if (bEditSlider) {
				usp->sliderName = activeSlider;

				float sliderscale = project->SliderValue(activeSlider);
				if (sliderscale == 0.0)
					sliderscale = 1.0;

				usp->sliderscale = sliderscale;
			}

			glView->ApplyUndoState(usp, false);

			previewRotation = angle;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
		};

		auto sliderMoved = [&](wxCommandEvent&) {
			Vector3 angle;
			angle.x = XRCCTRL(dlg, "rsSliderX", wxSlider)->GetValue() / 100.0f;
			angle.y = XRCCTRL(dlg, "rsSliderY", wxSlider)->GetValue() / 100.0f;
			angle.z = XRCCTRL(dlg, "rsSliderZ", wxSlider)->GetValue() / 100.0f;

			XRCCTRL(dlg, "rsTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", angle.x));
			XRCCTRL(dlg, "rsTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", angle.y));
			XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.4f", angle.z));

			updateRotationPreview();
		};

		auto textChanged = [&](wxCommandEvent&) {
			Vector3 angle;
			angle.x = atof(XRCCTRL(dlg, "rsTextX", wxTextCtrl)->GetValue().c_str());
			angle.y = atof(XRCCTRL(dlg, "rsTextY", wxTextCtrl)->GetValue().c_str());
			angle.z = atof(XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->GetValue().c_str());

			XRCCTRL(dlg, "rsSliderX", wxSlider)->SetValue(angle.x * 100);
			XRCCTRL(dlg, "rsSliderY", wxSlider)->SetValue(angle.y * 100);
			XRCCTRL(dlg, "rsSliderZ", wxSlider)->SetValue(angle.z * 100);

			updateRotationPreview();
		};

		auto originChanged = [&](wxCommandEvent&) { updateRotationPreview(); };

		XRCCTRL(dlg, "rsSliderX", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "rsSliderY", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "rsSliderZ", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "rsTextX", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "rsTextY", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "rsTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "origin", wxChoice)->Bind(wxEVT_CHOICE, originChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (!previewRotation.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
	}
}

void OutfitStudioFrame::OnInflateShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgInflateShape")) {
		Vector3 previewInflate;

		auto updateInflatePreview = [&]() {
			std::unordered_map<uint16_t, float> mask;
			std::unordered_map<uint16_t, float>* mptr = nullptr;
			std::vector<Vector3> verts;

			if (!previewInflate.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true, false);
					glView->GetUndoHistory()->PopState();
				}
			}

			Vector3 inflate;
			inflate.x = atof(XRCCTRL(dlg, "isTextX", wxTextCtrl)->GetValue().c_str());
			inflate.y = atof(XRCCTRL(dlg, "isTextY", wxTextCtrl)->GetValue().c_str());
			inflate.z = atof(XRCCTRL(dlg, "isTextZ", wxTextCtrl)->GetValue().c_str());

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UndoType::VertexPosition;

			for (auto& sel : selectedItems) {
				mask.clear();
				mptr = nullptr;

				NiShape* shape = sel->GetShape();

				auto mesh = glView->GetMesh(shape->name.get());
				if (!mesh || !mesh->norms)
					continue;

				project->GetLiveVerts(shape, verts);
				glView->GetShapeMask(mask, shape->name.get());

				if (!mesh->bGotWeldVerts)
					mesh->CalcWeldVerts();

				if (!mask.empty())
					mptr = &mask;

				UndoStateShape uss;
				uss.shapeName = shape->name.get();

				for (size_t i = 0; i < verts.size(); i++) {
					Vector3& vertPos = verts[i];
					Vector3 norm = Mesh::TransformDirMeshToNif(mesh->norms[i]);
					Vector3 diff = norm.ComponentMultiply(inflate);

					if (mptr)
						diff *= 1.0f - mask[i];

					if (diff.IsZero(true))
						continue;

					Vector3 newPos = vertPos + diff;
					uss.pointStartState[i] = Mesh::TransformPosNifToMesh(vertPos);
					uss.pointEndState[i] = Mesh::TransformPosNifToMesh(newPos);
				}

				for (size_t i = 0; i < verts.size(); i++) {
					auto pointEndStateIt = uss.pointEndState.find(i);
					if (pointEndStateIt != uss.pointEndState.end()) {
						mesh->DoForEachWeldedVertex(i, [&](int p) {
							auto pointEndStateWeldIt = uss.pointEndState.find(p);
							if (pointEndStateWeldIt != uss.pointEndState.end())
								pointEndStateWeldIt->second = pointEndStateIt->second;
						});
					}
				}

				usp->usss.push_back(std::move(uss));
			}

			if (bEditSlider) {
				usp->sliderName = activeSlider;

				float sliderscale = project->SliderValue(activeSlider);
				if (sliderscale == 0.0)
					sliderscale = 1.0;

				usp->sliderscale = sliderscale;
			}

			glView->ApplyUndoState(usp, false);

			previewInflate = inflate;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
		};

		auto sliderMoved = [&](wxCommandEvent& event) {
			Vector3 inflate;

			bool uniform = XRCCTRL(dlg, "isUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = ((wxSlider*)event.GetEventObject())->GetValue() / 1000.0f;
				inflate = Vector3(uniformValue, uniformValue, uniformValue);

				XRCCTRL(dlg, "isSliderX", wxSlider)->SetValue(inflate.x * 1000);
				XRCCTRL(dlg, "isSliderY", wxSlider)->SetValue(inflate.y * 1000);
				XRCCTRL(dlg, "isSliderZ", wxSlider)->SetValue(inflate.z * 1000);
			}
			else {
				inflate.x = XRCCTRL(dlg, "isSliderX", wxSlider)->GetValue() / 1000.0f;
				inflate.y = XRCCTRL(dlg, "isSliderY", wxSlider)->GetValue() / 1000.0f;
				inflate.z = XRCCTRL(dlg, "isSliderZ", wxSlider)->GetValue() / 1000.0f;
			}

			XRCCTRL(dlg, "isTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", inflate.x));
			XRCCTRL(dlg, "isTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", inflate.y));
			XRCCTRL(dlg, "isTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", inflate.z));

			updateInflatePreview();
		};

		auto textChanged = [&](wxCommandEvent& event) {
			Vector3 inflate;

			bool uniform = XRCCTRL(dlg, "isUniform", wxCheckBox)->IsChecked();
			if (uniform) {
				float uniformValue = atof(((wxTextCtrl*)event.GetEventObject())->GetValue().c_str());
				inflate.x = atof(XRCCTRL(dlg, "isTextX", wxTextCtrl)->GetValue().c_str());
				if (inflate.x != uniformValue)
					XRCCTRL(dlg, "isTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				inflate.y = atof(XRCCTRL(dlg, "isTextY", wxTextCtrl)->GetValue().c_str());
				if (inflate.y != uniformValue)
					XRCCTRL(dlg, "isTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				inflate.z = atof(XRCCTRL(dlg, "isTextZ", wxTextCtrl)->GetValue().c_str());
				if (inflate.z != uniformValue)
					XRCCTRL(dlg, "isTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", uniformValue));

				inflate = Vector3(uniformValue, uniformValue, uniformValue);
			}
			else {
				inflate.x = atof(XRCCTRL(dlg, "isTextX", wxTextCtrl)->GetValue().c_str());
				inflate.y = atof(XRCCTRL(dlg, "isTextY", wxTextCtrl)->GetValue().c_str());
				inflate.z = atof(XRCCTRL(dlg, "isTextZ", wxTextCtrl)->GetValue().c_str());
			}

			XRCCTRL(dlg, "isSliderX", wxSlider)->SetValue(inflate.x * 1000);
			XRCCTRL(dlg, "isSliderY", wxSlider)->SetValue(inflate.y * 1000);
			XRCCTRL(dlg, "isSliderZ", wxSlider)->SetValue(inflate.z * 1000);

			updateInflatePreview();
		};

		XRCCTRL(dlg, "isSliderX", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "isSliderY", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "isSliderZ", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "isTextX", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "isTextY", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		XRCCTRL(dlg, "isTextZ", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		if (dlg.ShowModal() != wxID_OK) {
			if (!previewInflate.IsZero()) {
				UndoStateProject* curState = glView->GetUndoHistory()->GetBackState();
				if (curState) {
					glView->ApplyUndoState(curState, true);
					glView->GetUndoHistory()->PopState();
				}
			}
		}

		UpdateUndoTools();
	}
}

void OutfitStudioFrame::OnFixClippingShape(wxCommandEvent& event) {
	if (bEditSlider) {
		OnSliderFixClipping(event);
		return;
	}

	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	if (!CheckEditableState())
		return;

	NiShape* refShape = project->GetBaseShape();
	if (!refShape) {
		wxMessageBox(_("No reference shape set."), _("Fix Clipping"), wxICON_WARNING);
		return;
	}

	float strength = 0.0f;
	if (!ShowClippingFixStrength(strength))
		return;

	ClippingFixOptions options;
	options.strength = strength;

	// Get reference shape geometry (unmorphed)
	std::vector<Vector3> bodyVerts;
	std::vector<Triangle> bodyTris;
	project->GetWorkNif()->GetVertsForShape(refShape, bodyVerts);
	refShape->GetTriangles(bodyTris);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::VertexPosition;

	for (auto& sel : selectedItems) {
		NiShape* shape = sel->GetShape();
		if (project->IsBaseShape(shape))
			continue;

		if (!ClippingFixer::IsEligibleForFix(*project->GetWorkNif(), shape))
			continue;

		std::unordered_map<uint16_t, float> unmasked;
		glView->GetShapeUnmasked(unmasked, shape->name.get());

		std::unordered_set<uint16_t> allowed;
		if (!unmasked.empty())
			for (auto& u : unmasked)
				allowed.insert(u.first);

		std::vector<Vector3> outfitVerts;
		project->GetWorkNif()->GetVertsForShape(shape, outfitVerts);

		FixClippingForShape(bodyVerts, bodyTris, shape, outfitVerts, options, usp, allowed.empty() ? nullptr : &allowed);
	}

	if (usp->usss.empty()) {
		glView->GetUndoHistory()->PopState();
		return;
	}

	glView->ApplyUndoState(usp, false);

	if (glView->GetTransformMode())
		glView->ShowTransformTool();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnDeleteVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	// Prepare the undo data and determine if any shapes are being deleted.
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	std::vector<NiShape*> delShapes;
	for (auto& i : selectedItems) {
		if (editUV && editUV->shape == i->GetShape())
			editUV->Close();

		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeUnmasked(mask, i->GetShape()->name.get());
		UndoStateShape uss;
		uss.shapeName = i->GetShape()->name.get();
		if (project->PrepareDeleteVerts(i->GetShape(), mask, uss))
			delShapes.push_back(i->GetShape());
		else
			usp->usss.push_back(std::move(uss));
	}

	// Confirm deleting shapes; then delete them.
	if (!delShapes.empty()) {
		if (wxMessageBox(_("Are you sure you wish to delete parts of the selected shapes?"), _("Confirm Delete"), wxYES_NO) == wxNO)
			return;

		for (NiShape* shape : delShapes) {
			usp->deletedShapes.emplace_back();
			project->CaptureShapeDeleteState(shape, usp->deletedShapes.back());
			project->DeleteShape(shape);
		}
	}

	// Now do the vertex deletion
	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();
	for (auto& uss : usp->usss) {
		NiShape* shape = project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;
		project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, false);
	}

	project->GetWorkAnim()->CleanupBones();

	RefreshGUIFromProj(false, false);
	SetPendingChanges();

	glView->UnstashMasks(maskStash);
	ApplySliders();
}

void OutfitStudioFrame::OnSeparateVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	std::unordered_map<uint16_t, float> masked;
	glView->GetShapeMask(masked, activeItem->GetShape()->name.get());
	if (masked.empty())
		return;

	std::string shapeName = activeItem->GetShape()->name.get();
	std::string newShapeName;
	do {
		std::string result{wxGetTextFromUser(_("Please enter a unique name for the new separated shape."), _("Separate Vertices..."), wxString::FromUTF8(shapeName), this).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	if (editUV && editUV->shape == activeItem->GetShape())
		editUV->Close();

	auto newShape = project->DuplicateShape(activeItem->GetShape(), newShapeName);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(2);
	usp->usss[0].shapeName = shapeName;
	usp->usss[1].shapeName = newShapeName;

	std::unordered_map<uint16_t, float> unmasked = masked;
	glView->InvertMaskTris(unmasked, shapeName);

	project->PrepareDeleteVerts(activeItem->GetShape(), masked, usp->usss[0]);
	project->PrepareDeleteVerts(newShape, unmasked, usp->usss[1]);

	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();

	project->ApplyShapeMeshUndo(activeItem->GetShape(), maskStash[usp->usss[0].shapeName], usp->usss[0], false);
	project->ApplyShapeMeshUndo(newShape, maskStash[usp->usss[1].shapeName], usp->usss[1], false);

	project->SetTextures();
	RefreshGUIFromProj(false, false);
	SetPendingChanges();

	glView->UnstashMasks(maskStash);
	ApplySliders();
}

MergeCheckErrors OutfitStudioFrame::CheckCopyGeo(wxDialog& dlg) {
	wxStaticText* errors = XRCCTRL(dlg, "copyGeometryErrors", wxStaticText);
	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxChoice* targetChoice = XRCCTRL(dlg, "targetChoice", wxChoice);

	std::string source = sourceChoice->GetString(sourceChoice->GetSelection()).ToStdString();
	std::string target = targetChoice->GetString(targetChoice->GetSelection()).ToStdString();

	MergeCheckErrors e;
	project->CheckMerge(source, target, e);
	XRCCTRL(dlg, "wxID_OK", wxButton)->Enable(e.canMerge);
	const bool hasWarnings = e.partitionsMismatch || e.segmentsMismatch || e.textureMismatch || e.transformsMismatch;

	if (e.canMerge && !hasWarnings) {
		errors->SetLabel(_("No errors found!"));
		dlg.SetSize(dlg.GetBestSize());
		return e;
	}

	wxString errorLines;
	if (e.shapesSame)
		errorLines << "\n- " << _("Target must be different from source.");
	if (e.tooManyVertices)
		errorLines << "\n- " << _("Resulting shape would have too many vertices.");
	if (e.tooManyTriangles)
		errorLines << "\n- " << _("Resulting shape would have too many triangles.");
	if (e.shaderMismatch)
		errorLines << "\n- " << _("Shaders do not match. Make sure both shapes either have or don't have a shader and their shader type matches.");
	if (e.alphaPropMismatch)
		errorLines << "\n- " << _("Alpha property mismatch. Make sure both shapes either have or don't have an alpha property and their flags + threshold match.");

	wxString warningLines;
	if (e.partitionsMismatch)
		warningLines << "\n- " << _("Partitions do not match. Merge will auto-reconcile matching slots and create missing partitions.");
	if (e.segmentsMismatch)
		warningLines << "\n- " << _("Segments do not match. Merge will auto-reconcile matching IDs and create missing segments/sub segments.");
	if (e.textureMismatch)
		warningLines << "\n- " << _("Base texture doesn't match. Merge will use texture paths from the target shape.");
	if (e.transformsMismatch)
		warningLines << "\n- " << _("Transforms do not match. Merge will apply the transforms of both shapes to their geometry and clear them, without moving the meshes.");

	wxString msg;
	if (!errorLines.empty())
		msg << _("Errors:") << errorLines;
	if (hasWarnings) {
		if (!msg.empty())
			msg << "\n";
		msg << _("Warnings:") << warningLines;
	}

	errors->SetLabel(msg);
	dlg.SetSize(dlg.GetBestSize());
	return e;
}

void OutfitStudioFrame::OnCopyGeo(wxCommandEvent& WXUNUSED(event)) {
	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyGeometry"))
		return;

	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxChoice* targetChoice = XRCCTRL(dlg, "targetChoice", wxChoice);
	if (!sourceChoice || !targetChoice)
		return;

	std::string sourceShapeName;
	if (activeItem)
		sourceShapeName = activeItem->GetShape()->name.get();

	std::vector<std::string> shapeList = GetShapeList();
	if (shapeList.size() < 2)
		return;

	CloseBrushSettings();

	for (const std::string& shape : shapeList) {
		sourceChoice->AppendString(shape);
		targetChoice->AppendString(shape);
		if (shape == sourceShapeName)
			sourceChoice->SetSelection(sourceChoice->GetCount() - 1);
	}

	if (sourceChoice->GetSelection() == wxNOT_FOUND)
		sourceChoice->SetSelection(0);
	if (sourceChoice->GetSelection() == 0)
		targetChoice->SetSelection(1);
	else
		targetChoice->SetSelection(0);

	MergeCheckErrors mergeErrors;
	sourceChoice->Bind(wxEVT_CHOICE, [this, &dlg, &mergeErrors](wxCommandEvent&) { mergeErrors = CheckCopyGeo(dlg); });
	targetChoice->Bind(wxEVT_CHOICE, [this, &dlg, &mergeErrors](wxCommandEvent&) { mergeErrors = CheckCopyGeo(dlg); });
	mergeErrors = CheckCopyGeo(dlg);

	if (dlg.ShowModal() != wxID_OK)
		return;

	sourceShapeName = sourceChoice->GetString(sourceChoice->GetSelection()).ToStdString();
	std::string targetShapeName = targetChoice->GetString(targetChoice->GetSelection()).ToStdString();

	NiShape* sourceShape = project->GetWorkNif()->FindBlockByName<NiShape>(sourceShapeName);
	if (!sourceShape)
		return;
	NiShape* targetShape = project->GetWorkNif()->FindBlockByName<NiShape>(targetShapeName);
	if (!targetShape)
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(1);
	usp->usss[0].shapeName = targetShapeName;

	project->PrepareCopyGeo(sourceShape, targetShape, usp->usss[0]);

	std::unordered_map<std::string, std::vector<float>> maskStash = glView->StashMasks();

	project->ApplyShapeMeshUndo(targetShape, maskStash[usp->usss[0].shapeName], usp->usss[0], false);

	if (mergeErrors.textureMismatch)
		project->SetTextures(targetShape);

	if (XRCCTRL(dlg, "checkDeleteSource", wxCheckBox)->IsChecked())
		project->DeleteShape(sourceShape);

	RefreshGUIFromProj(false, false);
	SetPendingChanges();
	glView->UnstashMasks(maskStash);
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

		CloseBrushSettings();

		std::string shapeName = activeItem->GetShape()->name.get();
		do {
			std::string result{wxGetTextFromUser(_("Please enter a unique name for the duplicated shape."), _("Duplicate Shape"), wxString::FromUTF8(shapeName), this).ToUTF8()};
			if (result.empty())
				return;

			newName = std::move(result);
		} while (project->IsValidShape(newName));

		wxLogMessage("Duplicating shape '%s' as '%s'.", shapeName, newName);

		auto shape = project->DuplicateShape(activeItem->GetShape(), newName);
		if (shape) {
			glView->AddMeshFromNif(project->GetWorkNif(), newName);
			UpdateMeshFromSet(shape);
			project->SetTextures(shape);

			MaterialFile matFile;
			bool hasMatFile = project->GetShapeMaterialFile(shape, matFile);
			glView->SetMeshTextures(newName, project->GetShapeTextures(shape), hasMatFile, matFile);

			subitem = outfitShapes->AppendItem(outfitRoot, wxString::FromUTF8(newName));
			outfitShapes->SetItemState(subitem, 0);
			outfitShapes->SetItemData(subitem, new ShapeItemData(shape));

			outfitShapes->UnselectAll();
			outfitShapes->SelectItem(subitem);
			SetPendingChanges();
		}
	}
}

void OutfitStudioFrame::OnRefineMesh(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	glView->gls.DeleteOverlay("refineErrorEdges");

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	if (selectedItems.size() != 1) {
		wxMessageBox(_("There is more than one shape selected."), _("Error"));
		return;
	}

	// Determine unmasked vertices
	NiShape* shape = selectedItems[0]->GetShape();
	size_t nverts = shape->GetNumVertices();
	std::unordered_map<uint16_t, float> mask;
	glView->GetShapeUnmasked(mask, shape->name.get());
	std::vector<bool> pincs(nverts, false);
	for (auto& m : mask)
		pincs[m.first] = true;

	// Count unmasked edges
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	size_t nedges = 0;
	for (Triangle& tri : tris) {
		int ntripts = pincs[tri.p1] + pincs[tri.p2] + pincs[tri.p3];
		if (ntripts == 3)
			nedges += 3;
		else if (ntripts == 2)
			nedges += 1;
	}

	// Determine maximum possible vertices and triangles for this Nif type
	auto workNif = project->GetWorkNif();
	if (!workNif)
		return;

	constexpr size_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	size_t maxTriIndex = std::numeric_limits<uint16_t>().max();

	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	// Check if we'll overflow the vertex or triangle arrays.
	// Note that the actual number of vertices added could be as low as
	// nedges/2; nedges is the upper bound (if every edge is welded or
	// boundary).
	if (nverts + nedges > maxVertIndex) {
		wxMessageBox(_("The shape has reached the vertex count limit."), _("Error"), wxICON_ERROR);
		return;
	}

	if (shape->GetNumTriangles() + nedges > maxTriIndex) {
		wxMessageBox(_("The shape has reached the triangle count limit."), _("Error"), wxICON_ERROR);
		return;
	}

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = shape->name.get();
	Mesh* m = glView->GetMesh(shape->name.get());
	std::vector<Edge> badEdges;
	if (!project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, false, &badEdges)) {
		if (!badEdges.empty()) {
			glView->gls.AddVisEdges(m, badEdges, "refineErrorEdges");

			// Add bad edge vertices to the mask
			for (const auto& e : badEdges) {
				m->mask[e.p1] = 1.0f;
				m->mask[e.p2] = 1.0f;
			}
			m->QueueUpdate(Mesh::UpdateType::Mask);
		}

		wxMessageBox(_("Some edges have multiple triangles of the same orientation. They have been highlighted and masked. Correct the orientations before refining."), _("Error"), wxICON_ERROR);
		glView->Render();
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	glView->ApplyUndoState(usp, false);

	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteShape(wxCommandEvent& WXUNUSED(event)) {
	if (currentTabButton == meshTabButton) {
		if (bEditSlider) {
			wxMessageBox(_("Can't delete shape while in slider edit mode.  Use CTRL+Delete to delete sliders instead."), _("Error"));
			return;
		}

		if (!ShapeSelectionCheck())
			return;

		// Delete shape(s) when in meshes tab
		if (wxMessageBox(_("Are you sure you wish to delete the selected shapes?"), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UndoType::ShapeDelete;

		std::vector<ShapeItemData> selected;
		for (auto& i : selectedItems)
			selected.push_back(*i);

		activeItem = nullptr;
		selectedItems.clear();

		for (auto& i : selected) {
			if (editUV && editUV->shape == i.GetShape())
				editUV->Close();

			std::string shapeName = i.GetShape()->name.get();
			wxLogMessage("Deleting shape '%s'.", shapeName);

			usp->deletedShapes.emplace_back();
			project->CaptureShapeDeleteState(i.GetShape(), usp->deletedShapes.back());

			project->DeleteShape(i.GetShape());
			wxTreeItemId item = i.GetId();
			outfitShapes->Delete(item);
		}

		SetPendingChanges();
		UpdateAnimationGUI();
		UpdateUndoTools();
		glView->Render();
	}
	else if (currentTabButton == boneTabButton) {
		// Delete bone(s) when in bones tab
		bool shiftDown = wxGetKeyState(WXK_SHIFT);
		bool ctrlDown = wxGetKeyState(WXK_CONTROL);
		if (shiftDown && ctrlDown) {
			if (wxMessageBox(_("Delete selected bones?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
				return;

			wxCommandEvent evt;
			OnDeleteBone(evt);
		}
		else {
			if (!ShapeSelectionCheck())
				return;

			if (wxMessageBox(_("Delete bones from selected shape(s)?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
				return;

			wxCommandEvent evt;
			OnDeleteBoneFromSelected(evt);
		}
	}
	else if (currentTabButton == partitionTabButton) {
		if (wxMessageBox(_("Delete partition?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		wxCommandEvent evt;
		OnDeletePartition(evt);
	}
	else if (currentTabButton == segmentTabButton) {
		if (wxMessageBox(_("Delete segment?  This action cannot be undone."), _("Confirm Delete"), wxYES_NO | wxICON_WARNING) == wxNO)
			return;

		wxCommandEvent evt;
		OnDeleteSegment(evt);
	}
}

void OutfitStudioFrame::OnSetBoneSkin(wxCommandEvent& WXUNUSED(event)) {
	std::string shape = activeItem->GetShape()->name.get();
	project->GetWorkAnim()->RecalcXFormSkinToBone(shape, activeBone);

	glView->UpdateBones();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnSetBoneNode(wxCommandEvent& WXUNUSED(event)) {
	std::string shape = activeItem->GetShape()->name.get();
	project->GetWorkAnim()->RecalcCustomBoneXFormsFromSkin(shape, activeBone);

	glView->UpdateBones();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnAddBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSkeletonBones"))
		return;

	CloseBrushSettings();

	dlg.SetSize(450, 470);
	dlg.CenterOnParent();

	wxTreeCtrl* boneTree = XRCCTRL(dlg, "boneTree", wxTreeCtrl);

	std::function<void(wxTreeItemId, AnimBone*)> fAddBoneChildren = [&](wxTreeItemId treeParent, AnimBone* boneParent) {
		for (auto& cb : boneParent->children) {
			if (!cb->boneName.empty()) {
				auto newItem = boneTree->AppendItem(treeParent, cb->boneName);
				fAddBoneChildren(newItem, cb);
				if (cb->boneName == activeBone)
					boneTree->SelectItem(newItem);
			}
			else
				fAddBoneChildren(treeParent, cb);
		}
	};

	AnimBone* rb = AnimSkeleton::getInstance().GetRootBonePtr();
	wxTreeItemId rt = boneTree->AddRoot(rb->boneName);
	fAddBoneChildren(rt, rb);

	if (dlg.ShowModal() == wxID_OK) {
		wxArrayTreeItemIds sel;
		boneTree->GetSelections(sel);
		for (size_t i = 0; i < sel.size(); i++) {
			std::string bone = boneTree->GetItemText(sel[i]).ToStdString();
			wxLogMessage("Adding bone '%s' to project.", bone);

			project->AddBoneRef(bone);
			wxTreeItemId item = outfitBones->AppendItem(bonesRoot, bone);
			UpdateBoneItemState(item, bone);
			cXMirrorBone->AppendString(bone);
			cPoseBone->AppendString(bone);
		}

		glView->UpdateBones();
		UpdateBoneCounts();
		SetPendingChanges();
		glView->Render();
	}
}

void OutfitStudioFrame::FillParentBoneChoice(wxDialog& dlg, const std::string& selBone) {
	wxChoice* cParentBone = XRCCTRL(dlg, "cParentBone", wxChoice);
	cParentBone->AppendString("(none)");

	std::set<std::string> boneSet;
	for (auto selItem : selectedItems) {
		const std::vector<std::string>& bones = project->GetWorkAnim()->shapeBones[selItem->GetShape()->name.get()];
		for (const std::string& b : bones)
			boneSet.insert(b);
	}

	for (auto& bone : boneSet) {
		cParentBone->AppendString(bone);
		if (bone == selBone)
			cParentBone->SetSelection(cParentBone->GetCount() - 1);
	}

	if (cParentBone->GetSelection() == wxNOT_FOUND) {
		if (selBone.empty()) {
			cParentBone->SetSelection(0);
		}
		else {
			cParentBone->AppendString(selBone);
			cParentBone->SetSelection(cParentBone->GetCount() - 1);
		}
	}
}

void OutfitStudioFrame::GetBoneDlgData(wxDialog& dlg, MatTransform& xform, std::string& parentBone, int& addCount) {
	xform.translation.x = atof(XRCCTRL(dlg, "textX", wxTextCtrl)->GetValue().c_str());
	xform.translation.y = atof(XRCCTRL(dlg, "textY", wxTextCtrl)->GetValue().c_str());
	xform.translation.z = atof(XRCCTRL(dlg, "textZ", wxTextCtrl)->GetValue().c_str());

	Vector3 rotvec;
	rotvec.x = atof(XRCCTRL(dlg, "textRX", wxTextCtrl)->GetValue().c_str());
	rotvec.y = atof(XRCCTRL(dlg, "textRY", wxTextCtrl)->GetValue().c_str());
	rotvec.z = atof(XRCCTRL(dlg, "textRZ", wxTextCtrl)->GetValue().c_str());
	xform.rotation = RotVecToMat(rotvec);

	wxChoice* cParentBone = XRCCTRL(dlg, "cParentBone", wxChoice);
	int pBChoice = cParentBone->GetSelection();
	if (pBChoice != wxNOT_FOUND)
		parentBone = cParentBone->GetString(pBChoice).ToStdString();

	if (parentBone == "(none)")
		parentBone = std::string();

	wxSpinCtrl* numAddCount = XRCCTRL(dlg, "numAddCount", wxSpinCtrl);
	if (numAddCount)
		addCount = numAddCount->GetValue();
}

void OutfitStudioFrame::OnAddCustomBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone"))
		return;

	CloseBrushSettings();

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);
	FillParentBoneChoice(dlg, activeBone);

	if (dlg.ShowModal() != wxID_OK)
		return;

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

	MatTransform xform;
	std::string parentBone;
	int addCount = 1;
	GetBoneDlgData(dlg, xform, parentBone, addCount);

	wxLogMessage("Adding custom bone '%s' to project (%d/%d).", bone, 1, addCount);
	project->AddCustomBoneRef(bone.ToStdString(), parentBone, xform);

	wxTreeItemId newItem = outfitBones->AppendItem(bonesRoot, bone);
	UpdateBoneItemState(newItem, bone.ToStdString());

	cXMirrorBone->AppendString(bone);
	cPoseBone->AppendString(bone);

	wxString origBone = bone;
	int suffixNumber = 0;

	size_t firstDigit = origBone.find_first_of("0123456789");
	if (firstDigit != std::string::npos) {
		wxString strNumber = origBone.substr(origBone.find_first_of("0123456789"));
		suffixNumber = wxAtoi(strNumber);
	}

	parentBone = bone;

	for (int i = 1; i < addCount; i++) {
		if (firstDigit != std::string::npos)
			bone = origBone.substr(0, firstDigit) + std::to_string(++suffixNumber);
		else
			bone = origBone + std::to_string(++suffixNumber);

		wxTreeItemIdValue cookie2;
		item = outfitBones->GetFirstChild(bonesRoot, cookie2);
		while (item.IsOk()) {
			if (outfitBones->GetItemText(item) == bone) {
				wxMessageBox(wxString::Format(_("Bone '%s' already exists in the project!"), bone), _("Error"), wxICON_INFORMATION, this);
				break;
			}
			item = outfitBones->GetNextChild(bonesRoot, cookie2);
		}

		wxLogMessage("Adding custom bone '%s' to project (%d/%d).", bone, i, addCount);
		project->AddCustomBoneRef(bone.ToStdString(), parentBone, xform);

		newItem = outfitBones->AppendItem(bonesRoot, bone);
		UpdateBoneItemState(newItem, bone.ToStdString());

		cXMirrorBone->AppendString(bone);
		cPoseBone->AppendString(bone);

		parentBone = bone;
	}

	glView->UpdateBones();
	UpdateBoneCounts();
	SetPendingChanges();
	glView->Render();
}

void OutfitStudioFrame::OnEditBone(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
	if (!bPtr)
		return;

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone"))
		return;

	CloseBrushSettings();

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

	if (bPtr->parent)
		FillParentBoneChoice(dlg, bPtr->parent->boneName);
	else
		FillParentBoneChoice(dlg);

	wxTextCtrl* boneNameTC = XRCCTRL(dlg, "boneName", wxTextCtrl);
	boneNameTC->SetValue(bPtr->boneName);
	boneNameTC->Disable();

	Vector3 rotvec = RotMatToVec(bPtr->xformToParent.rotation);
	XRCCTRL(dlg, "textX", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.x);
	XRCCTRL(dlg, "textY", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.y);
	XRCCTRL(dlg, "textZ", wxTextCtrl)->SetValue(wxString() << bPtr->xformToParent.translation.z);
	XRCCTRL(dlg, "textRX", wxTextCtrl)->SetValue(wxString() << rotvec.x);
	XRCCTRL(dlg, "textRY", wxTextCtrl)->SetValue(wxString() << rotvec.y);
	XRCCTRL(dlg, "textRZ", wxTextCtrl)->SetValue(wxString() << rotvec.z);

	XRCCTRL(dlg, "lbAddCount", wxStaticText)->Hide();
	XRCCTRL(dlg, "numAddCount", wxSpinCtrl)->Hide();

	if (bPtr->isStandardBone) {
		dlg.SetLabel(_("View Standard Bone"));
		XRCCTRL(dlg, "textX", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textY", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textZ", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRX", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRY", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "textRZ", wxTextCtrl)->Disable();
		XRCCTRL(dlg, "cParentBone", wxChoice)->Disable();
		XRCCTRL(dlg, "wxID_OK", wxButton)->Disable();
	}
	else {
		dlg.SetLabel(_("Edit Custom Bone"));
	}

	if (dlg.ShowModal() != wxID_OK)
		return;

	MatTransform xform;
	std::string parentBone;
	int addCount = 1;
	GetBoneDlgData(dlg, xform, parentBone, addCount);

	project->ModifyCustomBone(bPtr, parentBone, xform);
	glView->UpdateBones();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteBone(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (size_t i = 0; i < selItems.size(); i++) {
		wxString boneText = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting bone '%s' from project.", boneText);

		std::string bone = boneText.ToStdString();
		project->DeleteBone(bone);
		activeBone.clear();

		outfitBones->Delete(selItems[i]);
		lastSelectedBones.erase(bone);
		lastNormalizeBones.erase(bone);

		int idxMirrorBone = cXMirrorBone->FindString(boneText);
		if (idxMirrorBone != wxNOT_FOUND)
			cXMirrorBone->Delete(idxMirrorBone);

		int idxPoseBone = cPoseBone->FindString(boneText);
		if (idxPoseBone != wxNOT_FOUND)
			cPoseBone->Delete(idxPoseBone);
	}

	if (cXMirrorBone->GetStringSelection().IsEmpty())
		cXMirrorBone->SetSelection(0);

	if (cPoseBone->GetStringSelection().IsEmpty())
		cPoseBone->SetSelection(wxNOT_FOUND);

	glView->UpdateBones();
	ReselectBone();
	CalcAutoXMirrorBone();
	glView->GetUndoHistory()->ClearHistory();
	UpdateBoneCounts();
	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteBoneFromSelected(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (size_t i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]).ToStdString();
		wxLogMessage("Deleting weights of bone '%s' from selected shapes.", bone);

		for (auto& s : selectedItems)
			project->GetWorkAnim()->RemoveShapeBone(s->GetShape()->name.get(), bone);
	}

	glView->UpdateBones();
	ReselectBone();
	glView->GetUndoHistory()->ClearHistory();
	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
	UpdateUndoTools();
	SetPendingChanges();
}

bool OutfitStudioFrame::HasUnweightedCheck() {
	std::vector<std::string> unweighted;
	if (project->HasUnweighted(&unweighted)) {
		std::string shapesJoin = JoinStrings(unweighted, "; ");
		wxLogWarning(wxString::Format("Unweighted vertices found on shapes: %s", shapesJoin));

		int error = wxMessageBox(wxString::Format("%s\n \n%s",
												  _("The following shapes have unweighted vertices, which can cause issues. The affected vertices have been put under a mask. Do "
													"you want to save anyway?"),
												  shapesJoin),
								 _("Unweighted Vertices"),
								 wxYES_NO | wxICON_WARNING,
								 this);
		if (error != wxYES)
			return true;
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

void OutfitStudioFrame::CalcCopySkinTransOption(WeightCopyOptions& options) {
	// This function calculates whether the "copy global-to-skin transform
	// from base shape" checkbox should be shown and what the default value
	// for the "transform-geometry" checkbox should be

	NifFile* nif = project->GetWorkNif();
	NiShape* baseShape = project->GetBaseShape();
	if (!baseShape)
		return;

	AnimInfo& workAnim = *project->GetWorkAnim();

	MatTransform globalToBaseShape = workAnim.GetTransformGlobalToShape(baseShape);

	// Check if any shape's CS is different from the base shape's
	for (size_t i = 0; i < selectedItems.size(); i++) {
		NiShape* shape = selectedItems[i]->GetShape();
		if (shape == baseShape)
			continue;

		MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);
		if (!globalToShape.IsNearlyEqualTo(globalToBaseShape)) {
			options.showSkinTransOption = true;
			break;
		}
	}

	if (!options.showSkinTransOption)
		// They're all the same, so hide the option
		return;

	options.doSkinTransCopy = true;

	// As a first step in calculating a good default for the transform-geometry option,
	// find the average vertex position of the base shape in its own skin coordinates
	Vector3 baseAvg;

	const std::vector<Vector3>& baseVerts = *nif->GetVertsForShape(baseShape);
	for (size_t i = 0; i < baseVerts.size(); ++i)
		baseAvg += baseVerts[i];

	if (baseVerts.size())
		baseAvg /= static_cast<uint32_t>(baseVerts.size());

	// Now check if any shape would be better aligned by changing its global-to-skin transform
	for (size_t i = 0; i < selectedItems.size(); i++) {
		NiShape* shape = selectedItems[i]->GetShape();
		if (shape == baseShape)
			continue;

		MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);
		if (globalToShape.IsNearlyEqualTo(globalToBaseShape))
			continue;

		const std::vector<Vector3>& verts = *nif->GetVertsForShape(shape);
		if (verts.empty())
			continue;

		// Calculate old average and new average.
		MatTransform shapeToGlobal = globalToShape.InverseTransform();
		MatTransform shapeToBaseShape = globalToBaseShape.ComposeTransforms(shapeToGlobal);

		Vector3 oldAvg, newAvg;
		for (size_t j = 0; j < verts.size(); ++j) {
			oldAvg += shapeToBaseShape.ApplyTransform(verts[j]);
			newAvg += verts[j];
		}

		oldAvg /= static_cast<uint32_t>(verts.size());
		newAvg /= static_cast<uint32_t>(verts.size());

		// Check whether old or new is closer to the base shape.
		// If new is farther away, then transforming the geometry would be a good idea
		if (newAvg.DistanceTo(baseAvg) > oldAvg.DistanceTo(baseAvg)) {
			options.doTransformGeo = true;
			break;
		}
	}
}

void OutfitStudioFrame::OnCopyBoneWeight(wxCommandEvent& WXUNUSED(event)) {
	CopyWeightsToSelectedShapes(false);
}

void OutfitStudioFrame::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	CopyWeightsToSelectedShapes(true);
}

void OutfitStudioFrame::CopyWeightsToSelectedShapes(bool selectedBonesOnly) {
	if (!ShapeSelectionCheck())
		return;

	if (!project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	// Bones selected in the bone list are checked in the dialog, all bones otherwise
	std::vector<std::string> preselectedBones;
	if (selectedBonesOnly) {
		preselectedBones = GetSelectedBones();
		if (preselectedBones.empty()) {
			wxMessageBox(_("There are no bones selected!"), _("Error"));
			return;
		}
	}

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems) {
		if (auto shape = s->GetShape(); !project->IsBaseShape(shape))
			selectedShapes.push_back(s->GetShape());
		else
			wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
	}
	CopyBoneWeightForShapes(selectedShapes, false, preselectedBones);
}

int OutfitStudioFrame::CopyBoneWeightForShapes(std::vector<NiShape*> shapes, bool silent, const std::vector<std::string>& preselectedBones) {
	if (shapes.empty())
		return 0;

	CloseBrushSettings();

	WeightCopyOptions options;
	options.preselectedBones = preselectedBones;
	CalcCopySkinTransOption(options);
	AnimInfo& workAnim = *project->GetWorkAnim();

	WeightCopyDialog dlg(this, project, glView, poseDataCollection, lastNormalizeBones, shapes, options, silent);
	if (dlg.GetResult()) {
		StartProgress(_("Copying bone weights..."));

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UndoType::Weight;

		std::vector<std::string> baseBones;
		if (!options.selectedBones.empty())
			baseBones = options.selectedBones;
		else
			baseBones = workAnim.shapeBones[project->GetBaseShape()->name.get()];

		std::sort(baseBones.begin(), baseBones.end());

		int nCopyBones = static_cast<int>(baseBones.size());
		std::vector<std::string> lockedBones;
		bool bSpreadWeight = false;

		// When copying a subset of bones, compute normalization info
		if (!options.selectedBones.empty()) {
			std::unordered_set<std::string> selBones{baseBones.begin(), baseBones.end()};
			std::vector<std::string> normBones, notNormBones;
			GetNormalizeBones(&normBones, &notNormBones);

			for (auto& bone : normBones)
				if (!selBones.count(bone))
					baseBones.push_back(bone);

			bSpreadWeight = static_cast<int>(baseBones.size()) > nCopyBones;

			if (bSpreadWeight) {
				for (auto& bone : notNormBones)
					if (!selBones.count(bone))
						lockedBones.push_back(bone);
			}
			else {
				for (auto& bone : notNormBones)
					if (!selBones.count(bone))
						baseBones.push_back(bone);
			}
		}

		std::unordered_map<uint16_t, float> mask;

		const int inc = 100 / shapes.size() - 1;

		for (size_t i = 0; i < shapes.size(); i++) {
			NiShape* shape = shapes[i];
			wxLogMessage("Copying bone weights to '%s'...", shape->name.get());
			StartSubProgress(i * inc, i * inc + inc);

			if (options.doSkinTransCopy) {
				MatTransform globalToBaseShape = workAnim.GetTransformGlobalToShape(project->GetBaseShape());
				MatTransform globalToShape = workAnim.GetTransformGlobalToShape(shape);

				if (options.doTransformGeo && !globalToBaseShape.IsNearlyEqualTo(globalToShape)) {
					MatTransform shapeToBaseShape = globalToBaseShape.ComposeTransforms(globalToShape.InverseTransform());
					project->ApplyTransformToShapeGeometry(shape, shapeToBaseShape);
				}

				workAnim.SetTransformGlobalToShape(shape, globalToBaseShape);
			}

			usp->usss.resize(usp->usss.size() + 1);
			usp->usss.back().shapeName = shape->name.get();

			mask.clear();
			glView->GetShapeMask(mask, shape->name.get());

			std::vector<std::string> mergedBones = baseBones;

			// For full copy, also add shape-specific bones not already in baseBones
			if (options.selectedBones.empty()) {
				std::vector<std::string> bones = workAnim.shapeBones[shape->name.get()];
				for (auto& b : bones)
					if (!std::binary_search(baseBones.begin(), baseBones.end(), b))
						mergedBones.push_back(b);
			}

			project->CopyBoneWeights(shape, options.proximityRadius, options.maxResults, mask, mergedBones, nCopyBones, lockedBones, usp->usss.back(), bSpreadWeight);
			EndProgress();
		}

		if (options.doSkinTransCopy || options.doTransformGeo)
			RefreshGUIFromProj();

		ActiveShapesUpdated(usp, false);
		project->morpher.ClearProximityCache();

		UpdateUndoTools();

		workAnim.CleanupBones();
		UpdateAnimationGUI();

		EndProgress();
	}

	return 0;
}

void OutfitStudioFrame::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	auto baseShape = project->GetBaseShape();
	if (!baseShape) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	if (project->IsBaseShape(activeItem->GetShape())) {
		wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself."), _("Error"));
		return;
	}

	int baseVertCount = project->GetVertexCount(baseShape);
	int workVertCount = project->GetVertexCount(activeItem->GetShape());
	if (baseVertCount != workVertCount) {
		wxMessageBox(_("The vertex count of the reference and chosen shape is not the same!"), _("Error"));
		return;
	}

	std::vector<std::string> selectedBones = GetSelectedBones();
	if (selectedBones.size() < 1)
		return;

	std::string bonesString;
	for (std::string& boneName : selectedBones)
		bonesString += "'" + boneName + "' ";

	wxLogMessage("Transferring selected bone weights to '%s' for %s...", activeItem->GetShape()->name.get(), bonesString);
	StartProgress(_("Transferring bone weights..."));

	std::unordered_map<uint16_t, float> mask;
	glView->GetActiveMask(mask);
	project->TransferSelectedWeights(activeItem->GetShape(), &mask, &selectedBones);

	UpdateAnimationGUI();

	EndProgress();
	SetPendingChanges();
}

void OutfitStudioFrame::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		std::string shapeName = selItem->GetShape()->name.get();
		Mesh* m = glView->GetMesh(shapeName);
		if (!m)
			continue;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 0.0f;
		}

		auto& bones = project->GetWorkAnim()->shapeBones;
		if (bones.find(shapeName) != bones.end()) {
			for (auto& b : bones[shapeName]) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
				if (weights) {
					for (auto& bw : *weights)
						if (bw.second > 0.0f)
							uss.pointEndState[bw.first].x = 1.0f;
				}
			}
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnMaskBoneWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		std::string shapeName = selItem->GetShape()->name.get();
		Mesh* m = glView->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 0.0f;
		}

		for (auto& b : GetSelectedBones()) {
			auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
			if (weights) {
				for (auto& bw : *weights)
					if (bw.second > 0.0f)
						uss.pointEndState[bw.first].x = 1.0f;
			}
		}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnCheckBadBones(wxCommandEvent& WXUNUSED(event)) {
	if (!project->CheckForBadBones())
		return;

	// CheckForBadBones updates AnimBone transforms in-place but leaves the
	// NIF nodes untouched; push the fixed transforms out so every view
	// (node overlays, bad-bone indicator, gizmo center) is consistent.
	if (auto* nif = project->GetWorkNif()) {
		if (auto* workAnim = project->GetWorkAnim())
			workAnim->WriteNodesToNif(nif);
	}

	glView->UpdateBones();
	glView->UpdateNodes();
	RefreshBoneTreeBadBoneIcons();
	HighlightBoneNamesWithWeights();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnCopySegPart(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	if (!project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems) {
		if (auto shape = s->GetShape(); !project->IsBaseShape(shape))
			selectedShapes.push_back(s->GetShape());
		else
			wxMessageBox(_("Sorry, you can't copy partitions/segments from the reference shape to itself. Skipping this shape."), _("Can't copy segments/partitions"), wxICON_WARNING);
	}
	if (selectedShapes.empty())
		return;

	if (wxMessageBox(_("Triangles will be assigned to the partition/segment of the nearest triangle in the reference.  Existing partitions/segments are cleared.  This action can't be undone."), _("Copy Partitions/Segments"), wxOK | wxCANCEL | wxICON_INFORMATION | wxOK_DEFAULT) != wxOK)
		return;

	CopySegPartForShapes(selectedShapes);
}

int OutfitStudioFrame::CopySegPartForShapes(std::vector<NiShape*> shapes, bool silent) {
	if (shapes.empty())
		return 0;

	int failshapes = 0;

	StartProgress(_("Copying segments/partitions..."));

	const int inc = 100 / shapes.size() - 1;

	for (size_t i = 0; i < shapes.size(); i++) {
		NiShape* shape = shapes[i];
		wxLogMessage("Copying segments/partitions to '%s'...", shape->name.get());
		StartSubProgress(i * inc, i * inc + inc);

		int failcount = project->CopySegPart(shape);
		if (failcount && !silent)
			wxMessageBox(wxString::Format(_("The partitions/segments could not be copied for '%s' because %d triangles could not be matched."), shape->name.get(), failcount), _("Error"));
		if (failcount)
			++failshapes;

		if (!failcount) {
			MeshFromProj(shape);

			if (shape == activeItem->GetShape()) {
				CreateSegmentTree(shape);
				CreatePartitionTree(shape);
			}
		}

		EndProgress();
	}

	SetPendingChanges();
	UpdateUndoTools();

	EndProgress();

	return failshapes;
}

bool OutfitStudioFrame::ShowVertexAsym(Mesh* m, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, VertexAsymmetryTasks& tasks, const std::vector<bool>& selVerts, bool trize) {
	// This dialog has two versions, one for OnMaskSymVert and one for
	// OnSymVert.  trize tells us which version to do.

	CloseBrushSettings();

	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nUnmatched = static_cast<int>(symverts.unmatched.size());

	VertexAsymmetryStats stats;
	CalcVertexAsymmetryStats(symverts, asyms, selVerts, stats);

	// Initialize the portion of tasks that doesn't get initialized by
	// member initialization.
	tasks.doSliders.resize(nSliders, false);
	tasks.doBones.resize(nBones, false);

	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgVertexAsym"))
		return false;

	// Fill in static labels
	XRCCTRL(dlg, "currentUnmaskedText", wxStaticText)->SetLabel(wxString() << stats.unmaskedCount);
	XRCCTRL(dlg, "unmatchedVerticesText", wxStaticText)->SetLabel(wxString() << nUnmatched);
	XRCCTRL(dlg, "posAvgText", wxStaticText)->SetLabel(wxString() << stats.posAvg);
	XRCCTRL(dlg, "positionText", wxStaticText)->SetLabel(wxString() << stats.posCount);
	XRCCTRL(dlg, "anySliderLabel", wxStaticText)->SetLabel(wxString() << nSliders << _(" sliders"));
	XRCCTRL(dlg, "anySliderText", wxStaticText)->SetLabel(wxString() << stats.anySliderCount);
	XRCCTRL(dlg, "anyBoneLabel", wxStaticText)->SetLabel(wxString() << nBones << _(" bones"));
	XRCCTRL(dlg, "anyBoneText", wxStaticText)->SetLabel(wxString() << stats.anyBoneCount);

	// The controls we need later
	wxCheckBox* cbUnmatched = XRCCTRL(dlg, "checkUnmatched", wxCheckBox);
	wxCheckBox* cbPosition = XRCCTRL(dlg, "checkPosition", wxCheckBox);
	wxCheckBox* cbAnySlider = XRCCTRL(dlg, "checkAnySlider", wxCheckBox);
	wxCheckBox* cbAnyBone = XRCCTRL(dlg, "checkAnyBone", wxCheckBox);
	std::vector<wxCheckBox*> cbSliders(nSliders, nullptr);
	std::vector<wxCheckBox*> cbBones;
	wxScrolledWindow* asymScroll = XRCCTRL(dlg, "asymScroll", wxScrolledWindow);
	wxStaticText* stillUnmaskedText = XRCCTRL(dlg, "stillUnmaskedText", wxStaticText);

	// UpdateStillUnmasked: calculates value for the "still unmasked"
	// wxStaticText.
	auto UpdateStillUnmasked = [&]() {
		int stillUnmasked = 0;
		if (tasks.doUnmatched)
			stillUnmasked += nUnmatched;
		std::vector<bool> doVert = CalcVertexListForAsymmetryTasks(symverts, asyms, tasks, m->nVerts);
		for (int vi = 0; vi < m->nVerts; ++vi)
			if (selVerts[vi] && doVert[vi])
				++stillUnmasked;
		stillUnmaskedText->SetLabel(wxString() << stillUnmasked);
	};

	// UpdateChecks: update the individual slider check boxes, the any-slider
	// check box, and the any-bone check box.  Also calls UpdateStillUnmasked.
	auto UpdateChecks = [&]() {
		int sliderCount = 0;
		for (int sai = 0; sai < nSliders; ++sai) {
			cbSliders[sai]->SetValue(tasks.doSliders[sai]);
			sliderCount += tasks.doSliders[sai];
		}
		cbAnySlider->Set3StateValue(sliderCount == nSliders ? wxCHK_CHECKED : sliderCount == 0 ? wxCHK_UNCHECKED : wxCHK_UNDETERMINED);

		int boneCount = 0, totalBones = 0;
		for (int bai = 0; bai < nBones; ++bai) {
			if (!stats.boneCounts[bai])
				continue;
			++totalBones;
			boneCount += tasks.doBones[bai];
		}
		cbAnyBone->Set3StateValue(boneCount == totalBones ? wxCHK_CHECKED : boneCount == 0 ? wxCHK_UNCHECKED : wxCHK_UNDETERMINED);

		UpdateStillUnmasked();
	};

	// Checkbox event handling for the checkboxes that are not in either
	// of the collapsible panes.
	cbUnmatched->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		tasks.doUnmatched = e.IsChecked();
		UpdateStillUnmasked();
	});
	cbPosition->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		tasks.doPos = e.IsChecked();
		UpdateStillUnmasked();
	});
	cbAnySlider->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		bool newval = e.IsChecked();
		for (int sai = 0; sai < nSliders; ++sai)
			tasks.doSliders[sai] = newval;
		UpdateChecks();
	});
	cbAnyBone->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent& e) {
		bool newval = e.IsChecked();
		for (int bai = 0; bai < nBones; ++bai)
			if (stats.boneCounts[bai])
				tasks.doBones[bai] = newval;
		for (size_t bci = 0; bci < cbBones.size(); ++bci)
			cbBones[bci]->SetValue(newval);
		UpdateChecks();
	});

	// Fill in Sliders collapsible pane
	wxCollapsiblePane* slidersCollapse = XRCCTRL(dlg, "slidersCollapse", wxCollapsiblePane);
	wxWindow* slidersPane = slidersCollapse->GetPane();
	wxFlexGridSizer* slidersSz = new wxFlexGridSizer(3);
	slidersSz->AddGrowableCol(0, 1);
	for (int sai = 0; sai < nSliders; ++sai) {
		const auto& sa = asyms.sliders[sai];
		wxCheckBox* cb = new wxCheckBox(slidersPane, wxID_ANY, sa.sliderName);
		slidersSz->Add(cb, 0, wxLEFT|wxRIGHT, 5);
		cbSliders[sai] = cb;
		slidersSz->Add(new wxStaticText(slidersPane, wxID_ANY, wxString() << stats.sliderAvgs[sai]), 0, wxRIGHT|wxALIGN_RIGHT, 20);
		slidersSz->Add(new wxStaticText(slidersPane, wxID_ANY, wxString() << stats.sliderCounts[sai]), 0, wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5);
		cb->Bind(wxEVT_CHECKBOX, [&, sai](wxCommandEvent& e) {
			tasks.doSliders[sai] = e.IsChecked();
			UpdateChecks();
		});
	}
	slidersPane->SetSizerAndFit(slidersSz);
	slidersCollapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&asymScroll](wxCollapsiblePaneEvent&) { asymScroll->FitInside(); });

	// Fill in Bones collapsible pane
	wxCollapsiblePane* bonesCollapse = XRCCTRL(dlg, "bonesCollapse", wxCollapsiblePane);
	wxWindow* bonesPane = bonesCollapse->GetPane();
	wxFlexGridSizer* bonesSz = new wxFlexGridSizer(3);
	bonesSz->AddGrowableCol(0, 1);
	for (int bai = 0; bai < nBones; ++bai) {
		const auto& ba = asyms.bones[bai];
		// If a bone has no asymmetries but its mirror bone does, it's still
		// listed in asyms.bones.  But that doesn't mean we need to show it
		// to the user.
		if (!stats.boneCounts[bai])
			continue;

		wxCheckBox* cb = new wxCheckBox(bonesPane, wxID_ANY, ba.boneName);
		bonesSz->Add(cb, 0, wxLEFT|wxRIGHT, 5);
		cbBones.push_back(cb);
		bonesSz->Add(new wxStaticText(bonesPane, wxID_ANY, wxString() << stats.boneAvgs[bai]), 0, wxRIGHT|wxALIGN_RIGHT, 20);
		bonesSz->Add(new wxStaticText(bonesPane, wxID_ANY, wxString() << stats.boneCounts[bai]), 0, wxLEFT|wxRIGHT|wxALIGN_RIGHT, 5);
		cb->Bind(wxEVT_CHECKBOX, [&, bai](wxCommandEvent& e) {
			tasks.doBones[bai] = e.IsChecked();
			UpdateChecks();
		});
	}
	bonesPane->SetSizerAndFit(bonesSz);
	bonesCollapse->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&asymScroll](wxCollapsiblePaneEvent&) { asymScroll->FitInside(); });

	// Disable irrelevant checkboxes
	auto DisableCheck = [](wxCheckBox* cb) {
		cb->SetValue(false);
		cb->Disable();
	};
	if (!nUnmatched)
		DisableCheck(cbUnmatched);
	if (!stats.posCount)
		DisableCheck(cbPosition);
	if (!stats.anySliderCount)
		DisableCheck(cbAnySlider);
	if (!stats.anyBoneCount)
		DisableCheck(cbAnyBone);

	// Customize the dialog for OnSymVert
	if (trize) {
		dlg.SetTitle(_("Symmetrize Vertices"));
		cbUnmatched->Hide();
		auto desc = _("Eliminates the selected asymmetries from unmasked vertices by adjusting vertex data to be consistent with mirror vertices.");
		if (nBones)
			desc += _("  (Hint: To choose which non-selected bones are adjusted during weight normalization, unlock them in the bones list in the Bones tab.)");
		wxStaticText* descST = XRCCTRL(dlg, "maskSymmetricVerticesDescription", wxStaticText);
		descST->SetLabel(desc);
		descST->Wrap(600);
		XRCCTRL(dlg, "stillUnmaskedLabel", wxStaticText)->SetLabel(_("Vertices that will be symmetrized:"));
		XRCCTRL(dlg, "wxID_OK", wxButton)->SetLabel(_("&Symmetrize"));
	}

	UpdateStillUnmasked();
	dlg.SetSize(dlg.GetBestSize());

	if (dlg.ShowModal() != wxID_OK)
		return false;

	return true;
}

void OutfitStudioFrame::OnMaskSymVert(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	NiShape* s = activeItem->GetShape();
	std::string shapeName = s->name.get();
	Mesh* m = glView->gls.GetMesh(shapeName);
	if (!m)
		return;

	std::vector<bool> selVerts(m->nVerts);
	for (int i = 0; i < m->nVerts; ++i)
		selVerts[i] = m->mask[i] == 0.0f;

	SymmetricVertices r;
	project->MatchSymmetricVertices(s, m->weldVerts, r);

	VertexAsymmetries a;
	project->FindVertexAsymmetries(s, r, m->weldVerts, a);

	VertexAsymmetryTasks tasks;
	if (!ShowVertexAsym(m, r, a, tasks, selVerts, false))
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;
	usp->usss.resize(1);
	UndoStateShape& uss = usp->usss[0];
	uss.shapeName = shapeName;

	// Mask vertices based on selected tasks.
	std::vector<bool> doVert = CalcVertexListForAsymmetryTasks(r, a, tasks, m->nVerts);
	AddWeldedToVertexList(m->weldVerts, doVert);
	for (int i = 0; i < m->nVerts; ++i) {
		if (selVerts[i] && doVert[i])
			continue;
		uss.pointStartState[i].x = m->mask[i];
		uss.pointEndState[i].x = 1.0f;
	};

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnSymVert(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	NiShape* s = activeItem->GetShape();
	std::string shapeName = s->name.get();
	Mesh* m = glView->gls.GetMesh(shapeName);
	if (!m)
		return;

	std::vector<bool> selVerts(m->nVerts);
	for (int i = 0; i < m->nVerts; ++i)
		selVerts[i] = m->mask[i] == 0.0f;

	SymmetricVertices r;
	project->MatchSymmetricVertices(s, m->weldVerts, r);

	VertexAsymmetries a;
	project->FindVertexAsymmetries(s, r, m->weldVerts, a);

	VertexAsymmetryTasks tasks;
	if (!ShowVertexAsym(m, r, a, tasks, selVerts, true))
		return;

	std::vector<std::string> normBones, notNormBones;
	GetNormalizeBones(&normBones, &notNormBones);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.resize(1);
	UndoStateShape& uss = usp->usss[0];
	uss.shapeName = shapeName;

	project->PrepareSymmetrizeVertices(s, uss, r, a, tasks, m->weldVerts, selVerts, normBones, notNormBones);

	glView->ApplyUndoState(usp, false);
	UpdateUndoTools();
}

void OutfitStudioFrame::OnMaskSymTri(wxCommandEvent& WXUNUSED(event)) {
	if (selectedItems.empty())
		return;

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& selItem : selectedItems) {
		NiShape* shape = selItem->GetShape();
		if (!shape)
			continue;
		std::string shapeName = shape->name.get();

		Mesh* m = glView->gls.GetMesh(shapeName);
		if (!m)
			return;

		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = shapeName;

		std::vector<bool> amask = project->CalculateAsymmetricTriangleVertexMask(shape, m->weldVerts);

		// Mask vertices that are _not_ in amask.
		for (int vi = 0; vi < m->nVerts; ++vi)
			if (m->mask[vi] != 1.0f && !amask[vi]) {
				uss.pointStartState[vi].x = m->mask[vi];
				uss.pointEndState[vi].x = 1.0f;
			}
	}

	glView->ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		glView->GetUndoHistory()->PopState();

	UpdateUndoTools();
}

void OutfitStudioFrame::OnResetTransforms(wxCommandEvent& WXUNUSED(event)) {
	project->ResetTransforms();
	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteUnreferencedNodes(wxCommandEvent& WXUNUSED(event)) {
	int deletionCount = 0;
	auto workNif = project->GetWorkNif();
	if (workNif)
		workNif->DeleteUnreferencedNodes(&deletionCount);

	if (deletionCount > 0)
		SetPendingChanges();

	wxString msg = wxString::Format(_("%d unreferenced nodes were deleted."), deletionCount);
	wxMessageBox(msg, _("Delete Unreferenced Nodes"));
}

void OutfitStudioFrame::OnRemoveSkinning(wxCommandEvent& WXUNUSED(event)) {
	project->RemoveSkinning();
	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnShapeProperties(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!ShapeSelectionCheck())
		return;

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems)
		selectedShapes.push_back(s->GetShape());

	auto shape = activeItem->GetShape();
	if (shape) {
		ShapeProperties prop(this, project->GetWorkNif(), selectedShapes);
		prop.ShowModal();
	}
}

void OutfitStudioFrame::OnMaskLess(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	if (glView->GetSegmentMode()) {
		if (GrowShrinkSegmentPartitionSelection(false)) {
			ShowSegment();
			ShowPartition();
		}

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
		return;
	}

	glView->MaskLess();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnMaskMore(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem)
		return;

	if (glView->GetSegmentMode()) {
		if (GrowShrinkSegmentPartitionSelection(true)) {
			ShowSegment();
			ShowPartition();
		}

		if (glView->GetTransformMode())
			glView->ShowTransformTool();
		else
			glView->Render();
		return;
	}

	glView->MaskMore();

	if (glView->GetTransformMode())
		glView->ShowTransformTool();
	else
		glView->Render();
}

void OutfitStudioFrame::OnNPWizChangeSliderSetFile(wxFileDirPickerEvent& event) {
	std::string fn = event.GetPath().ToStdString();
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

		for (auto& sn : setNames)
			setNameChoice->AppendString(sn);

		if (!setNames.empty()) {
			setNameChoice->SetSelection(0);
			ssf.SetShapes(setNames.front(), shapes);
			for (auto& rsn : shapes)
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

		for (auto& rsn : checkFile.GetShapeNames())
			refShapeChoice->AppendString(rsn);

		refShapeChoice->SetSelection(0);
	}
}

void OutfitStudioFrame::OnNPWizChangeSetNameChoice(wxCommandEvent& event) {
	wxWindow* npWiz = ((wxChoice*)event.GetEventObject())->GetParent();
	wxFilePickerCtrl* file = (wxFilePickerCtrl*)XRCCTRL((*npWiz), "npSliderSetFile", wxFilePickerCtrl);
	if (!file)
		return;

	std::string fn = file->GetPath().ToStdString();
	SliderSetFile ssf(fn);
	if (ssf.fail())
		return;

	std::vector<std::string> shapes;
	wxChoice* chooser = (wxChoice*)event.GetEventObject();
	ssf.SetShapes(chooser->GetStringSelection().ToStdString(), shapes);
	wxChoice* refShapeChoice = (wxChoice*)XRCCTRL((*npWiz), "npRefShapeName", wxChoice);
	refShapeChoice->Clear();

	for (auto& rsn : shapes)
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

void OutfitStudioFrame::OnRecalcNormals(wxCommandEvent& WXUNUSED(event)) {
	for (auto& s : selectedItems)
		glView->RecalcNormals(s->GetShape()->name.get(), true);

	glView->Render();
}

void OutfitStudioFrame::OnDisableNormalsCalc(wxCommandEvent& event) {
	OutfitStudioConfig.SetBoolValue("DisableNormalsCalc", event.IsChecked());
}

void OutfitStudioFrame::OnSmoothNormalSeams(wxCommandEvent& event) {
	bool enable = event.IsChecked();
	glView->SetNormalSeamSmoothMode(enable);

	for (auto& s : selectedItems)
		project->activeSet.SetSmoothSeamNormals(s->GetShape()->name.get(), enable);

	menuBar->Enable(XRCID("btnSmoothSeamsAngle"), enable);

	glView->Render();
}

void OutfitStudioFrame::OnSmoothSeamsAngle(wxCommandEvent& WXUNUSED(event)) {
	if (!ShapeSelectionCheck())
		return;

	std::vector<Mesh*> activeMeshes = glView->gls.GetActiveMeshes();
	if (activeMeshes.empty())
		return;

	std::vector<float> oldMeshAngles;
	oldMeshAngles.resize(activeMeshes.size());

	for (size_t i = 0; i < activeMeshes.size(); i++)
		oldMeshAngles[i] = activeMeshes[i]->smoothSeamNormalsAngle;

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgSmoothSeams")) {
		auto updatePreview = [&]() {
			float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());

			for (size_t i = 0; i < activeMeshes.size(); i++) {
				Mesh* m = activeMeshes[i];
				m->smoothSeamNormalsAngle = angle;
				m->SmoothNormals();
			}

			glView->Render();
		};

		auto sliderMoved = [&](wxCommandEvent&) {
			float angle = XRCCTRL(dlg, "angleSlider", wxSlider)->GetValue() / 100.0f;
			XRCCTRL(dlg, "angleText", wxTextCtrl)->ChangeValue(wxString::Format("%0.2f", angle));

			updatePreview();
		};

		auto textChanged = [&](wxCommandEvent&) {
			float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());
			XRCCTRL(dlg, "angleSlider", wxSlider)->SetValue(angle * 100);

			float angleLimited = XRCCTRL(dlg, "angleSlider", wxSlider)->GetValue() / 100.0f;
			if (angle != angleLimited)
				XRCCTRL(dlg, "angleText", wxTextCtrl)->ChangeValue(wxString::Format("%0.2f", angleLimited));

			updatePreview();
		};

		XRCCTRL(dlg, "angleSlider", wxSlider)->Bind(wxEVT_SLIDER, sliderMoved);
		XRCCTRL(dlg, "angleText", wxTextCtrl)->Bind(wxEVT_TEXT, textChanged);
		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		// Set old value of first mesh
		XRCCTRL(dlg, "angleText", wxTextCtrl)->SetValue(wxString::Format("%0.2f", oldMeshAngles.front()));

		if (dlg.ShowModal() != wxID_OK) {
			for (size_t i = 0; i < activeMeshes.size(); i++) {
				Mesh* m = activeMeshes[i];
				m->smoothSeamNormalsAngle = oldMeshAngles[i];
				m->SmoothNormals();
			}

			glView->Render();
			return;
		}

		float angle = atof(XRCCTRL(dlg, "angleText", wxTextCtrl)->GetValue().c_str());

		for (auto& s : selectedItems)
			project->activeSet.SetSmoothSeamNormalsAngle(s->GetShape()->name.get(), angle);
	}
}

void OutfitStudioFrame::OnLockNormals(wxCommandEvent& event) {
	bool enable = event.IsChecked();
	glView->SetLockNormalsMode(enable);

	for (auto& s : selectedItems)
		project->activeSet.SetLockNormals(s->GetShape()->name.get(), enable);
}

void OutfitStudioFrame::OnEditUV(wxCommandEvent& WXUNUSED(event)) {
	if (editUV)
		return;

	if (!ShapeSelectionCheck())
		return;

	auto shape = activeItem->GetShape();
	Mesh* m = glView->GetMesh(shape->name.get());
	if (shape && m) {
		if (bEditSlider && !ConfirmSliderDataLocalForEdit(shape, activeSlider))
			return;

		editUV = new EditUV(this, project->GetWorkNif(), shape, m, activeSlider);

		editUV->Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event) {
			editUV = nullptr;
			event.Skip();
		});

		editUV->CenterOnParent();
		editUV->Show();
	}
}

void OutfitStudioFrame::OnSelectMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(maskSel);
		for (auto mask : (*maskData)) {
			glView->SetShapeMask(mask.second, mask.first);
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnSaveMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");

	wxString maskName = cMaskName->GetValue();
	if (maskName.empty())
		return;

	auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>();

	std::vector<std::string> shapes = GetShapeList();
	for (auto& s : shapes) {
		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeMask(mask, s);
		(*maskData)[s] = std::move(mask);
	}

	int existingSel = cMaskName->FindString(maskName);
	if (existingSel != wxNOT_FOUND) {
		cMaskName->SetClientData(existingSel, maskData);
		cMaskName->SetSelection(existingSel);
	}
	else {
		int maskSel = cMaskName->Append(maskName, maskData);
		cMaskName->SetSelection(maskSel);
	}
}

void OutfitStudioFrame::OnDeleteMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		cMaskName->Delete(maskSel);
		cMaskName->SetValue("");
	}
}

void OutfitStudioFrame::OnExportMask(wxCommandEvent& WXUNUSED(event)) {
	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");
	if (cMaskName->GetCount() == 0) {
		wxMessageBox(_("No masks to export."), _("Export Masks"), wxICON_INFORMATION);
		return;
	}

	wxFileDialog saveDialog(this, _("Export Masks"), wxEmptyString, "masks.xml",
		"XML Files (*.xml)|*.xml", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (saveDialog.ShowModal() == wxID_CANCEL)
		return;

	std::string filePath = saveDialog.GetPath().ToUTF8().data();

	MaskFile maskFile;
	for (unsigned int i = 0; i < cMaskName->GetCount(); i++) {
		auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(i);
		if (!maskData)
			continue;

		MaskEntry entry;
		entry.name = cMaskName->GetString(i).ToUTF8().data();

		std::map<std::string, int> vertexCounts;
		for (auto& [shapeName, mask] : *maskData) {
			Mesh* m = glView->GetMesh(shapeName);
			if (m)
				vertexCounts[shapeName] = m->nVerts;
		}

		entry.SetFromMaskData(*maskData, vertexCounts);
		maskFile.GetEntries().push_back(std::move(entry));
	}

	int err = maskFile.Save(filePath);
	if (err)
		wxMessageBox(_("Failed to save mask file."), _("Export Masks"), wxICON_ERROR);
}

void OutfitStudioFrame::OnImportMask(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog openDialog(this, _("Import Masks"), wxEmptyString, wxEmptyString,
		"XML Files (*.xml)|*.xml", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if (openDialog.ShowModal() == wxID_CANCEL)
		return;

	std::string filePath = openDialog.GetPath().ToUTF8().data();

	MaskFile maskFile;
	int err = maskFile.Load(filePath);
	if (err) {
		wxMessageBox(_("Failed to load mask file."), _("Import Masks"), wxICON_ERROR);
		return;
	}

	auto cMaskName = (wxComboBox*)FindWindowByName("cMaskName");

	for (const auto& entry : maskFile.GetEntries()) {
		wxString maskName = entry.name.empty() ? _("Imported Mask") : wxString::FromUTF8(entry.name);
		auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>(entry.ToMaskData());

		int existing = cMaskName->FindString(maskName);
		if (existing != wxNOT_FOUND) {
			cMaskName->SetClientData(existing, maskData);
		}
		else {
			cMaskName->Append(maskName, maskData);
		}
	}

	// Select and apply the first imported entry
	if (!maskFile.GetEntries().empty()) {
		wxString firstName = wxString::FromUTF8(maskFile.GetEntries().front().name);
		int sel = cMaskName->FindString(firstName);
		if (sel != wxNOT_FOUND) {
			cMaskName->SetSelection(sel);
			auto maskData = (std::map<std::string, std::unordered_map<uint16_t, float>>*)cMaskName->GetClientData(sel);
			if (maskData) {
				for (auto& [shapeName, mask] : *maskData)
					glView->SetShapeMask(mask, shapeName);
			}
		}
	}

	glView->Render();
}

void OutfitStudioFrame::OnPaneCollapse(wxCollapsiblePaneEvent& WXUNUSED(event)) {
	UpdateToolScrollLayout();
}

void OutfitStudioFrame::OnBottomPanelResize(wxSizeEvent& event) {
	event.Skip();
	UpdateToolScrollLayout();
}

void OutfitStudioFrame::UpdateToolScrollLayout() {
	wxWindow* container = FindWindowByName("bottomSplitPanel");
	if (!container)
		return;

	wxSizer* contentSizer = toolScroll ? toolScroll->GetSizer() : nullptr;
	if (contentSizer) {
		// The panes stack up taller than the panel once a few of them are open.
		// Letting the tool area claim its full height would squeeze the slider
		// list to nothing and cut off whatever no longer fits, so cap it here
		// and let the rest scroll.
		int reserved = FromDIP(120);
		if (notesPane && notesPane->IsShown())
			reserved += notesPane->GetEffectiveMinSize().GetHeight();

		const int room = std::max(container->GetClientSize().GetHeight() - reserved, FromDIP(80));
		const int height = std::min(contentSizer->CalcMin().GetHeight(), room);

		if (toolScroll->GetMinHeight() != height)
			toolScroll->SetMinSize(wxSize(-1, height));
	}

	container->Layout();

	// Scrollbars follow the virtual size, which the capped minimum above does
	// not change.
	if (toolScroll)
		toolScroll->FitInside();
}

void OutfitStudioFrame::ApplyPose() {
	// While an animation plays there is no separate physics pump; the
	// simulation advances in lockstep with each displayed frame, using the
	// wall-clock time since the last step.
	if (physicsRunning && animPlaying)
		physics->Step(physicsClock.TakeElapsed());

	// Asking for a BVH update queues the shape with wxGLPanel::OnIdle, which
	// rebuilds a whole AABB tree over every triangle of it - far too expensive
	// to pay once per displayed animation frame. The tree only serves picking
	// and brush hit tests, both locked out during playback, so the rebuild is
	// deferred to the single ApplyPose that PauseAnimationPlayback does.
	const bool updateBVH = !animPlaying && !physicsPumpActive;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);
		glView->UpdateMeshVertices(shape->name.get(), &verts, updateBVH, true, false);
	}

	// Bone overlays cost one GL mesh created and destroyed per bone, not worth
	// paying for every frame while they are not being drawn.
	if (!animPlaying || glView->GetBonesMode() || glView->GetNodesMode())
		glView->UpdateBones();

	glView->Render();
}

void OutfitStudioFrame::UpdateBoneTransformToolEnabled() {
	// The transform tool is generally enabled on every tab except bones (and
	// colors), but on the bones tab we want it enabled when the user has
	// turned on bones-mode or nodes-mode, so they can drag bones/nodes.
	if (!currentTabButton || !boneTabButton || currentTabButton->GetId() != boneTabButton->GetId())
		return;

	const bool enable = glView->GetBonesMode() || glView->GetNodesMode();
	menuBar->Enable(XRCID("btnTransform"), enable);
	toolBarV->EnableTool(XRCID("btnTransform"), enable);
	if (!enable)
		glView->SetTransformMode(false);
}

void OutfitStudioFrame::RefreshBoneTreeBadBoneIcons() {
	// Iterate the bone list and refresh each item's state so that the
	// bad-bones icon reflects any inconsistent transforms introduced by the
	// last edit.
	if (!outfitBones || !bonesRoot.IsOk())
		return;

	wxTreeItemIdValue cookie;
	wxTreeItemId item = outfitBones->GetFirstChild(bonesRoot, cookie);
	while (item.IsOk()) {
		UpdateBoneItemState(item, outfitBones->GetItemText(item).ToStdString());
		item = outfitBones->GetNextChild(bonesRoot, cookie);
	}
}

AnimBone* OutfitStudioFrame::GetPoseBonePtr() {
	int selind = cPoseBone->GetSelection();
	if (selind == wxNOT_FOUND)
		return nullptr;
	std::string poseBone = cPoseBone->GetString(selind).ToStdString();
	return AnimSkeleton::getInstance().GetBonePtr(poseBone);
}

void OutfitStudioFrame::PoseToGUI() {
	AnimBone* bone = GetPoseBonePtr();
	if (bone) {
		rxPoseSlider->SetValue(bone->poseRotVec.x * 100);
		ryPoseSlider->SetValue(bone->poseRotVec.y * 100);
		rzPoseSlider->SetValue(bone->poseRotVec.z * 100);
		txPoseSlider->SetValue(bone->poseTranVec.x * 100);
		tyPoseSlider->SetValue(bone->poseTranVec.y * 100);
		tzPoseSlider->SetValue(bone->poseTranVec.z * 100);
		scPoseSlider->SetValue(std::log(bone->poseScale) / std::log(2) * 500);
		rxPoseText->ChangeValue(wxString() << bone->poseRotVec.x);
		ryPoseText->ChangeValue(wxString() << bone->poseRotVec.y);
		rzPoseText->ChangeValue(wxString() << bone->poseRotVec.z);
		txPoseText->ChangeValue(wxString() << bone->poseTranVec.x);
		tyPoseText->ChangeValue(wxString() << bone->poseTranVec.y);
		tzPoseText->ChangeValue(wxString() << bone->poseTranVec.z);
		scPoseText->ChangeValue(wxString() << bone->poseScale);
	}
	else {
		rxPoseSlider->SetValue(0);
		ryPoseSlider->SetValue(0);
		rzPoseSlider->SetValue(0);
		txPoseSlider->SetValue(0);
		tyPoseSlider->SetValue(0);
		tzPoseSlider->SetValue(0);
		scPoseSlider->SetValue(0);
		rxPoseText->ChangeValue("0");
		ryPoseText->ChangeValue("0");
		rzPoseText->ChangeValue("0");
		txPoseText->ChangeValue("0");
		tyPoseText->ChangeValue("0");
		tzPoseText->ChangeValue("0");
		scPoseText->ChangeValue("1");
	}
	if (project->bPose != cbPose->GetValue())
		cbPose->SetValue(project->bPose);
}

void OutfitStudioFrame::OnPoseBoneChanged(wxCommandEvent& WXUNUSED(event)) {
	PoseToGUI();
}

void OutfitStudioFrame::OnPoseValChanged(int cind, float val) {
	// Called when any pose slider or text control is changed.
	AnimBone* bone = GetPoseBonePtr();
	if (!bone)
		return;
	if (cind < 3)
		bone->poseRotVec[cind] = val;
	else if (cind < 6)
		bone->poseTranVec[cind - 3] = val;
	else
		bone->poseScale = val > 0 ? val : .0001f;

	bone->UpdatePoseTransform();
	ActivatePose(true);
}

void OutfitStudioFrame::OnAnyPoseSlider(wxScrollEvent& e, wxTextCtrl* t, int cind) {
	float val = (cind == 6) ?
		std::exp(e.GetPosition() * 0.002f * std::log(2)) :
		e.GetPosition() * 0.01f;
	t->ChangeValue(wxString() << val);
	OnPoseValChanged(cind, val);
}

void OutfitStudioFrame::OnRXPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, rxPoseText, 0);
}
void OutfitStudioFrame::OnRYPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, ryPoseText, 1);
}
void OutfitStudioFrame::OnRZPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, rzPoseText, 2);
}
void OutfitStudioFrame::OnTXPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, txPoseText, 3);
}
void OutfitStudioFrame::OnTYPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, tyPoseText, 4);
}
void OutfitStudioFrame::OnTZPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, tzPoseText, 5);
}
void OutfitStudioFrame::OnScPoseSlider(wxScrollEvent& e) {
	OnAnyPoseSlider(e, scPoseText, 6);
}

void OutfitStudioFrame::OnAnyPoseTextChanged(wxTextCtrl* t, wxSlider* s, int cind) {
	if (!t || !s)
		return;
	double val;
	if (!t->GetValue().ToDouble(&val))
		return;
	if (cind == 6)
		s->SetValue(std::log(val) / std::log(2) * 500);
	else
		s->SetValue(val * 100);
	OnPoseValChanged(cind, val);
}

void OutfitStudioFrame::OnRXPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(rxPoseText, rxPoseSlider, 0);
}
void OutfitStudioFrame::OnRYPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(ryPoseText, ryPoseSlider, 1);
}
void OutfitStudioFrame::OnRZPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(rzPoseText, rzPoseSlider, 2);
}
void OutfitStudioFrame::OnTXPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(txPoseText, txPoseSlider, 3);
}
void OutfitStudioFrame::OnTYPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(tyPoseText, tyPoseSlider, 4);
}
void OutfitStudioFrame::OnTZPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(tzPoseText, tzPoseSlider, 5);
}
void OutfitStudioFrame::OnScPoseTextChanged(wxCommandEvent& WXUNUSED(event)) {
	OnAnyPoseTextChanged(scPoseText, scPoseSlider, 6);
}

void OutfitStudioFrame::OnResetBonePose(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bone = GetPoseBonePtr();
	if (!bone)
		return;
	bone->poseRotVec = Vector3(0, 0, 0);
	bone->poseTranVec = Vector3(0, 0, 0);
	bone->poseScale = 1.0f;
	bone->UpdatePoseTransform();
	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::OnResetAllPose(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Reset all bone poses?"), _("Reset Pose"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Reset"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	ResetAllPoseBones();
	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::ResetAllPoseBones() {

	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const std::string& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		if (bone->IsUnposed())
			continue;

		bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->poseScale = 1.0f;
		bone->UpdatePoseTransform();
	}
}

void OutfitStudioFrame::OnPoseToMesh(wxCommandEvent& WXUNUSED(event)) {
	if (project->bPose) {
		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		project->ApplyPoseTransformsToAllShapeGeometry(*usp);

		std::vector<std::string> bones;
		AnimSkeleton::getInstance().GetBoneNames(bones);

		for (const std::string& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (!bone)
				continue;

			if (bone->IsUnposed())
				continue;

			bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseScale = 1.0f;
			bone->UpdatePoseTransform();
		}

		PoseToGUI();
		glView->UpdateBones();
		glView->ApplyUndoState(usp, false);
		SetPendingChanges();
	}
}

void OutfitStudioFrame::ActivatePose(bool checked) {
	if (cbPose->IsChecked() != checked)
		cbPose->SetValue(checked);

	project->bPose = checked;
	poseToMesh->Enable(checked);

	UpdatePhysicsState();
	ApplyPose();
}

void OutfitStudioFrame::OnPoseCheckBox(wxCommandEvent& e) {
	ActivatePose(e.IsChecked());
}

void OutfitStudioFrame::OnSelectPose(wxCommandEvent& WXUNUSED(event)) {
	// Selecting a pose takes over the skeleton from any selected animation.
	if (GetSelectedAnimation()) {
		PauseAnimationPlayback();
		animCurrentFrame = 0;
		cAnimationName->SetSelection(0); // "<None>"
		UpdateAnimationPlayerUI();
	}

	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));

		if (!poseData) {
			// "<New>" sentinel: reset all bones to an unposed state.
			ResetAllPoseBones();

			PoseToGUI();
			ApplyPose();
			UpdatePoseButtonStates();
			return;
		}

		poseData->ApplyToSkeleton();

		PoseToGUI();
		ActivatePose(true);

		UpdatePoseButtonStates();
	}
}

void OutfitStudioFrame::UpdatePoseButtonStates() {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	wxWindow* savePose = FindWindow(XRCID("savePose"));
	wxWindow* deletePose = FindWindow(XRCID("deletePose"));
	if (!cPoseName)
		return;

	bool enableSave = true;
	bool enableDelete = true;
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));
		if (!poseData) {
			// "<New>" sentinel: nothing to delete.
			enableDelete = false;
		}
		else if (poseData->readOnly) {
			enableSave = false;
			enableDelete = false;
		}
	}

	if (savePose)
		savePose->Enable(enableSave);
	if (deletePose)
		deletePose->Enable(enableDelete);
}

void OutfitStudioFrame::OnSavePose(wxCommandEvent& WXUNUSED(event)) {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");

	wxString poseName = cPoseName->GetValue();
	poseName.Trim(true).Trim(false);
	if (poseName.empty() || poseName == "<New>") {
		wxMessageBox(_("Please enter a name for the pose."), _("Save Pose"), wxICON_INFORMATION);
		return;
	}

	// If a pose with this name already exists, update it in place;
	// otherwise create a new entry (replacing the old Save As behavior).
	int existingSel = cPoseName->FindString(poseName);
	PoseData* poseData = nullptr;
	if (existingSel != wxNOT_FOUND) {
		poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(existingSel));
		if (poseData) {
			if (poseData->readOnly)
				return;
			poseData->boneData.clear();
		}
		else {
			// Defensive: an existing entry with null client data (shouldn't
			// happen for non-sentinel names). Treat as new.
			poseData = new PoseData(poseName.ToUTF8().data());
			cPoseName->SetClientData(existingSel, poseData);
		}
	}
	else {
		poseData = new PoseData(poseName.ToUTF8().data());
	}

	std::vector<std::string> bones;
	AnimSkeleton::getInstance().GetBoneNames(bones);

	for (const auto& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		if (bone->IsUnposed())
			continue;

		PoseBoneData poseBoneData{};
		poseBoneData.name = bone->boneName;
		poseBoneData.rotation = bone->poseRotVec;
		poseBoneData.translation = bone->poseTranVec;
		poseBoneData.scale = bone->poseScale;
		poseData->boneData.push_back(poseBoneData);
	}

	if (existingSel != wxNOT_FOUND) {
		cPoseName->SetSelection(existingSel);
	}
	else {
		int poseSel = cPoseName->Append(poseName, poseData);
		cPoseName->SetSelection(poseSel);
	}

	wxString dirName = wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/PoseData";
	wxFileName::Mkdir(dirName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxString fileName = dirName + "/" + wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(poseData->name).c_str()) + ".xml";

	PoseDataFile poseDataFile;
	poseDataFile.New(fileName.ToUTF8().data());

	std::vector<PoseData> poses;
	poses.push_back(*poseData);

	poseDataFile.SetData(poses);
	poseDataFile.Save();

	UpdatePoseButtonStates();
}

void OutfitStudioFrame::OnDeletePose(wxCommandEvent& WXUNUSED(event)) {
	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));
		if (!poseData)
			return; // "<New>" sentinel, nothing to delete
		if (poseData->readOnly)
			return;

		wxString prompt = wxString::Format(_("Are you sure you wish to delete the pose '%s'?"), cPoseName->GetStringSelection());
		int result = wxMessageBox(prompt, _("Confirm pose delete"), wxYES_NO | wxICON_WARNING, this);
		if (result != wxYES)
			return;

		wxString fileName = wxString::FromUTF8(ProjectUtil::GetProjectPath()) + "/PoseData/" + wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(poseData->name).c_str()) + ".xml";
		wxRemoveFile(fileName);

		cPoseName->Delete(poseSel);
		cPoseName->SetStringSelection("<New>");
		UpdatePoseButtonStates();
	}
}

namespace {

bool GetPoseHkxFormat(TargetGame targetGame, HKX::Format* outFormat) {
	HKX::Format format = HKX::Format::Unknown;
	switch (targetGame) {
		case SKYRIM: format = HKX::Format::Skyrim32; break;
		case SKYRIMSE:
		case SKYRIMVR: format = HKX::Format::Skyrim64; break;
		case FO4:
		case FO4VR: format = HKX::Format::Fallout64; break;
		default: return false;
	}

	if (outFormat)
		*outFormat = format;
	return true;
}

} // namespace

void OutfitStudioFrame::OnSaveHkxPose(wxCommandEvent& WXUNUSED(event)) {
	wxString defaultFileStem = "pose";
	if (wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName")) {
		wxString poseName = cPoseName->GetValue();
		poseName.Trim(true).Trim(false);
		if (!poseName.empty() && poseName != "<New>")
			defaultFileStem = poseName;
	}
	defaultFileStem = wxString::FromUTF8(PoseDataCollection::SanitizeFileStem(std::string(defaultFileStem.ToUTF8().data())).c_str());

	wxFileDialog saveDlg(this,
					 _("Save pose file"),
					 wxEmptyString,
					 defaultFileStem,
					 "HKX pose files (*.hkx)|*.hkx|SAM JSON pose files (*.json)|*.json|SAM YAML pose files (*.yaml;*.yml)|*.yaml;*.yml",
					 wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	saveDlg.SetFilterIndex(GetPreferredPoseFileFilterIndex(wxGetApp().targetGame));
	if (saveDlg.ShowModal() == wxID_CANCEL)
		return;

	wxString savePath = EnsurePoseFileExtension(saveDlg.GetPath(), saveDlg.GetFilterIndex());
	wxFileName saveFn(savePath);
	PoseFileFormat format = PoseDataCollection::GetPoseFileFormat(std::string(savePath.ToUTF8().data()));
	std::string skeletonHkxPath;
	HKX::Format hkxFormat = HKX::Format::Unknown;

	if (format == PoseFileFormat::Hkx) {
		if (!GetPoseHkxFormat(wxGetApp().targetGame, &hkxFormat)) {
			wxMessageBox(_("Saving HKX poses is currently only supported for Skyrim Legendary Edition, Skyrim Special Edition, Skyrim VR, Fallout 4 and Fallout 4 VR."),
						 _("Save Pose File"),
						 wxOK | wxICON_INFORMATION,
						 this);
			return;
		}

		wxString defSkelNif = wxString::FromUTF8(Config["Anim/DefaultSkeletonReference"]);
		if (defSkelNif.IsEmpty()) {
			wxMessageBox(_("No reference skeleton is configured. Please set a reference skeleton in the application settings before saving an HKX pose."),
						 _("Save Pose File"),
						 wxOK | wxICON_ERROR,
						 this);
			return;
		}

		wxFileName defSkelFn(defSkelNif);
		if (defSkelFn.IsRelative())
			defSkelFn = wxFileName(wxString::FromUTF8(Config["AppDir"]) + PathSepChar + defSkelNif);
		defSkelFn.SetExt("hkx");

		wxString skelHkx = defSkelFn.GetFullPath();
		if (!wxFileExists(skelHkx)) {
			wxMessageBox(wxString::Format(_("No Havok skeleton file was found next to the configured reference skeleton.\n\nExpected file:\n%s\n\nTo save HKX poses, place a matching .hkx skeleton file alongside the .nif reference skeleton."), skelHkx),
						 _("Save Pose File"),
						 wxOK | wxICON_ERROR,
						 this);
			return;
		}

		skeletonHkxPath = std::string(skelHkx.ToUTF8().data());
	}
	else if (format != PoseFileFormat::Json && format != PoseFileFormat::Yaml) {
		wxMessageBox(_("Please save the pose with a .hkx, .json, .yaml or .yml extension."), _("Save Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	PoseData pd;
	PoseDataCollection::CaptureCurrentPose(std::string(saveFn.GetName().ToUTF8().data()), format != PoseFileFormat::Yaml, pd);
	if (pd.boneData.empty()) {
		wxMessageBox(_("No skeleton bones are available to export a pose."), _("Save Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	std::string saveError;
	if (!PoseDataCollection::SavePoseFile(std::string(savePath.ToUTF8().data()), pd, skeletonHkxPath, hkxFormat, &saveError)) {
		wxString message = saveError.empty() ? _("Failed to save the pose file.") : wxString::FromUTF8(saveError);
		wxMessageBox(message, _("Save Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	if (statusBar)
		statusBar->SetStatusText(_("Pose file saved."), 0);
}

void OutfitStudioFrame::OnLoadHkxPose(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadDlg(this,
						 _("Select pose file"),
						 wxEmptyString,
						 wxEmptyString,
						 "HKX pose files (*.hkx)|*.hkx|SAM JSON pose files (*.json)|*.json|SAM YAML pose files (*.yaml;*.yml)|*.yaml;*.yml",
						 wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	loadDlg.SetFilterIndex(GetPreferredPoseFileFilterIndex(wxGetApp().targetGame));
	if (loadDlg.ShowModal() == wxID_CANCEL)
		return;

	wxString srcPath = loadDlg.GetPath();
	PoseFileFormat format = PoseDataCollection::GetPoseFileFormat(std::string(srcPath.ToUTF8().data()));
	std::string skeletonHkxPath;

	if (format == PoseFileFormat::Hkx) {
		if (!GetReferenceSkeletonHkxPath(skeletonHkxPath, _("Load Pose File")))
			return;
	}
	else if (format != PoseFileFormat::Json && format != PoseFileFormat::Yaml) {
		wxMessageBox(_("Please choose a pose file with a .hkx, .json, .yaml or .yml extension."), _("Load Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	PoseData pd;
	std::string loadError;
	if (!PoseDataCollection::LoadPoseFile(std::string(srcPath.ToUTF8().data()), pd, skeletonHkxPath, &loadError)) {
		wxString message = loadError.empty() ? _("Failed to load the pose file.") : wxString::FromUTF8(loadError);
		wxMessageBox(message, _("Load Pose File"), wxOK | wxICON_ERROR, this);
		return;
	}

	wxComboBox* cPoseName = (wxComboBox*)FindWindowByName("cPoseName");
	if (!cPoseName)
		return;

	auto makeUniquePoseName = [cPoseName](const std::string& baseName) {
		wxString uniqueName = wxString::FromUTF8(baseName);
		if (cPoseName->FindString(uniqueName) == wxNOT_FOUND)
			return uniqueName;

		for (int suffix = 1;; ++suffix) {
			wxString candidate = wxString::Format("%s (%d)", uniqueName, suffix);
			if (cPoseName->FindString(candidate) == wxNOT_FOUND)
				return candidate;
		}
	};

	int existingSel = cPoseName->FindString(wxString::FromUTF8(pd.name));
	if (existingSel != wxNOT_FOUND) {
		auto existing = reinterpret_cast<PoseData*>(cPoseName->GetClientData(existingSel));
		if (existing && !existing->readOnly) {
			existing->boneData = std::move(pd.boneData);
			existing->absoluteLocal = pd.absoluteLocal;
			cPoseName->SetSelection(existingSel);
		}
		else {
			pd.name = std::string(makeUniquePoseName(pd.name).ToUTF8().data());
			PoseData* added = poseDataCollection.AddPose(std::move(pd));
			int idx = cPoseName->Append(wxString::FromUTF8(added->name), added);
			cPoseName->SetSelection(idx);
		}
	}
	else {
		PoseData* added = poseDataCollection.AddPose(std::move(pd));
		int idx = cPoseName->Append(wxString::FromUTF8(added->name), added);
		cPoseName->SetSelection(idx);
	}

	wxCommandEvent dummy;
	OnSelectPose(dummy);
	if (statusBar)
		statusBar->SetStatusText(_("Pose file loaded."), 0);
}

bool OutfitStudioFrame::GetReferenceSkeletonHkxPath(std::string& outPath, const wxString& caption) {
	if (!GetPoseHkxFormat(wxGetApp().targetGame, nullptr)) {
		wxMessageBox(_("HKX support is currently only available for Skyrim Legendary Edition, Skyrim Special Edition, Skyrim VR, Fallout 4 and Fallout 4 VR."),
					 caption,
					 wxOK | wxICON_INFORMATION,
					 this);
		return false;
	}

	wxString defSkelNif = wxString::FromUTF8(Config["Anim/DefaultSkeletonReference"]);
	if (defSkelNif.IsEmpty()) {
		wxMessageBox(_("No reference skeleton is configured. Please set a reference skeleton in the application settings first."), caption, wxOK | wxICON_ERROR, this);
		return false;
	}

	wxFileName defSkelFn(defSkelNif);
	if (defSkelFn.IsRelative())
		defSkelFn = wxFileName(wxString::FromUTF8(Config["AppDir"]) + PathSepChar + defSkelNif);
	defSkelFn.SetExt("hkx");

	wxString skelHkx = defSkelFn.GetFullPath();
	if (!wxFileExists(skelHkx)) {
		wxMessageBox(wxString::Format(_("No Havok skeleton file was found next to the configured reference skeleton.\n\nExpected file:\n%s\n\nPlace a matching .hkx skeleton file alongside the .nif reference skeleton."),
						  skelHkx),
					 caption,
					 wxOK | wxICON_ERROR,
					 this);
		return false;
	}

	outPath = std::string(skelHkx.ToUTF8().data());
	return true;
}

AnimationData* OutfitStudioFrame::GetSelectedAnimation() {
	if (!cAnimationName)
		return nullptr;

	int sel = cAnimationName->GetSelection();
	if (sel == wxNOT_FOUND)
		return nullptr;

	// The "<None>" sentinel entry has null client data.
	return reinterpret_cast<AnimationData*>(cAnimationName->GetClientData(sel));
}

double OutfitStudioFrame::GetAnimPlaybackSpeed() {
	// Indices match the "animSpeed" choice contents in the XRC.
	static const double speeds[] = {0.25, 0.5, 1.0, 1.5, 2.0};
	constexpr int defaultSel = 2; // 1x

	int sel = animSpeedChoice ? animSpeedChoice->GetSelection() : defaultSel;
	if (sel < 0 || sel >= static_cast<int>(std::size(speeds)))
		sel = defaultSel;

	return speeds[sel];
}

bool OutfitStudioFrame::IsAnimationInterpolated() const {
	return animInterpolateCheck && animInterpolateCheck->GetValue();
}

void OutfitStudioFrame::ApplyAnimationFrame(double framePos, bool updatePoseGUI) {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty())
		return;

	const size_t numFrames = anim->framePoses.size();
	if (!(framePos >= 0.0) || framePos > double(numFrames))
		framePos = 0.0;

	animCurrentFrame = framePos;

	const size_t frame = std::min(size_t(framePos), numFrames - 1);
	const float blend = float(framePos - double(frame));

	if (IsAnimationInterpolated() && numFrames > 1 && blend > 0.0f) {
		// Playback loops, so the last frame blends back into the first one.
		const size_t nextFrame = (frame + 1) % numFrames;
		PoseData::Interpolate(anim->framePoses[frame], anim->framePoses[nextFrame], blend, animBlendPose);
		animBlendPose.ApplyToSkeleton();
	}
	else {
		anim->framePoses[frame].ApplyToSkeleton();
	}

	// Updating the pose sliders is skipped during playback; they are synced
	// once when playback pauses.
	if (updatePoseGUI)
		PoseToGUI();

	if (!project->bPose)
		ActivatePose(true);
	else
		ApplyPose();

	if (animFrameSlider && animFrameSlider->GetValue() != static_cast<int>(frame))
		animFrameSlider->SetValue(static_cast<int>(frame));

	if (animFrameText)
		animFrameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(frame + 1), static_cast<unsigned long>(numFrames)));
}

void OutfitStudioFrame::UpdateAnimationPlayerUI() {
	AnimationData* anim = GetSelectedAnimation();
	size_t numFrames = anim ? anim->GetNumFrames() : 0;

	if (animPlayPauseButton) {
		animPlayPauseButton->Enable(numFrames > 0);
		animPlayPauseButton->SetLabel(animPlaying ? _("Pause") : _("Play"));
	}

	const size_t currentFrame = numFrames > 0 ? std::min(static_cast<size_t>(animCurrentFrame), numFrames - 1) : 0;

	if (animFrameSlider) {
		animFrameSlider->Enable(numFrames > 1);
		animFrameSlider->SetRange(0, numFrames > 1 ? static_cast<int>(numFrames) - 1 : 1);
		animFrameSlider->SetValue(static_cast<int>(currentFrame));
	}

	if (animSpeedChoice)
		animSpeedChoice->Enable(numFrames > 0);

	if (animInterpolateCheck)
		animInterpolateCheck->Enable(numFrames > 1);

	if (animFrameText) {
		if (numFrames > 0)
			animFrameText->SetLabel(wxString::Format("%lu / %lu", static_cast<unsigned long>(currentFrame + 1), static_cast<unsigned long>(numFrames)));
		else
			animFrameText->SetLabel("0 / 0");
	}
}

int OutfitStudioFrame::GetAnimTargetFps() {
	int refresh = 0;

	const int displayIndex = wxDisplay::GetFromWindow(this);
	if (displayIndex != wxNOT_FOUND) {
		wxVideoMode mode = wxDisplay(static_cast<unsigned int>(displayIndex)).GetCurrentMode();
		refresh = mode.refresh;
	}

	if (refresh <= 0)
		refresh = 60;

	return std::min(refresh, 120);
}

wxLongLong OutfitStudioFrame::GetAnimFrameDueInMicro() {
	const wxLongLong period = 1000000 / std::max(animTargetFps, 1);
	const wxLongLong elapsed = animPlaybackWatch.TimeInMicro() - animLastDrawMicro;
	return elapsed >= period ? wxLongLong(0) : period - elapsed;
}

void OutfitStudioFrame::RestartAnimationClock() {
	animClockBaseFrame = animCurrentFrame;
	animPlaybackWatch.Start();
	animLastDrawMicro = 0;
}

void OutfitStudioFrame::PumpAnimationPlayback() {
	if (!animPlaying)
		return;

	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty()) {
		PauseAnimationPlayback();
		return;
	}

	// Ticks arrive from idle, the timer and mouse motion at wildly different
	// rates, so the drawing rate is decided here rather than by whatever woke us.
	if (GetAnimFrameDueInMicro() > 0)
		return;

	animLastDrawMicro = animPlaybackWatch.TimeInMicro();

	const size_t numFrames = anim->framePoses.size();
	const double frameDuration = anim->frameDuration > 0.0f ? anim->frameDuration : 1.0f / 30.0f;

	// Where the animation is due, from elapsed time rather than a tick count,
	// so a late or dropped tick shifts nothing but the smoothness.
	double framePos = animClockBaseFrame + (animPlaybackWatch.TimeInMicro().ToDouble() / 1000000.0) * GetAnimPlaybackSpeed() / frameDuration;
	framePos = std::fmod(framePos, double(numFrames));
	if (framePos < 0.0)
		framePos = 0.0;

	// Without interpolation only whole frames are ever shown, so skip the work
	// entirely until the position has actually crossed into the next one.
	if (!IsAnimationInterpolated() && size_t(framePos) == size_t(animCurrentFrame))
		return;

	ApplyAnimationFrame(framePos, false);
}

void OutfitStudioFrame::StartAnimationPlayback() {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim || anim->framePoses.empty() || animPlaying)
		return;

	animPlaying = true;
	SetAnimationPlaybackLock(true);
	UpdateAnimationPlayerUI();

	// Resolved once here: the window does not move between displays mid-play.
	animTargetFps = GetAnimTargetFps();

	RestartAnimationClock();
	SetHighResolutionTimers(true);
	Bind(wxEVT_IDLE, &OutfitStudioFrame::OnAnimIdle, this);
	animPlaybackTimer.Start(AnimPlaybackTimerIntervalMS);

	// Physics switches from its own pump to stepping in lockstep with the
	// displayed animation frames (see ApplyPose)
	UpdatePhysicsState();
}

void OutfitStudioFrame::PauseAnimationPlayback() {
	if (!animPlaying)
		return;

	animPlaybackTimer.Stop();
	Unbind(wxEVT_IDLE, &OutfitStudioFrame::OnAnimIdle, this);
	SetHighResolutionTimers(false);
	animPlaying = false;
	SetAnimationPlaybackLock(false);

	// Land on a whole frame, so the pose left behind is the one the player
	// reports rather than a blend between two of them. Unconditional because
	// this is also the ApplyPose that rebuilds the mesh BVHs playback skipped.
	if (GetSelectedAnimation())
		ApplyAnimationFrame(std::floor(animCurrentFrame), false);
	else
		ApplyPose();

	// The frame stays applied, so a paused animation behaves like a pose. Sync
	// the pose sliders that were skipped while playing.
	PoseToGUI();
	UpdateAnimationPlayerUI();

	// Back to the physics pump if physics stays enabled with pose mode on
	UpdatePhysicsState();
}

void OutfitStudioFrame::ResetAnimationList() {
	PauseAnimationPlayback();
	animCurrentFrame = 0;

	poseDataCollection.animationData.clear();

	if (cAnimationName) {
		cAnimationName->Clear();
		cAnimationName->Append("<None>", (void*)nullptr);
		cAnimationName->SetSelection(0);
	}

	UpdateAnimationPlayerUI();
}

void OutfitStudioFrame::SetAnimationPlaybackLock(bool locked) {
	const bool enable = !locked;

	if (menuBar) {
		for (size_t i = 0; i < menuBar->GetMenuCount(); ++i)
			menuBar->EnableTop(i, enable);
	}

	// Disabling a container disables its children without discarding their own
	// enabled state, so whatever the current tab, tool and pose selection had
	// decided (per-tool toolbar entries, the save/delete pose buttons, Pose to
	// Mesh) comes back unchanged on unlock.
	const std::initializer_list<wxWindow*> lockedWindows = {toolBarH,
															toolBarV,
															meshTabButton,
															boneTabButton,
															colorsTabButton,
															segmentTabButton,
															partitionTabButton,
															lightsTabButton,
															outfitShapes,
															outfitBones,
															sliderScroll,
															sliderFilter,
															bonesFilter,
															masksPane,
															notesPane,
															posePane,
															physicsPane,
															FindWindow(XRCID("cbFixedWeight")),
															FindWindow(XRCID("cbNormalizeWeights")),
															// The player itself stays usable; only the
															// controls that would swap the animation out
															// from under it are locked.
															cAnimationName,
															FindWindow(XRCID("importAnimationFile"))};

	for (wxWindow* w : lockedWindows) {
		if (w)
			w->Enable(enable);
	}

	// Swallow every menu, toolbar and accelerator command while playing.
	// Dynamically bound handlers run before the static event table, so this
	// blocks all of them in one place.
	if (locked)
		Bind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
	else
		Unbind(wxEVT_MENU, &OutfitStudioFrame::OnBlockedCommandDuringPlayback, this);
}

void OutfitStudioFrame::OnBlockedCommandDuringPlayback(wxCommandEvent& WXUNUSED(event)) {
	// Intentionally empty: not calling Skip() drops the command.
}

void OutfitStudioFrame::OnSelectAnimation(wxCommandEvent& WXUNUSED(event)) {
	PauseAnimationPlayback();
	animCurrentFrame = 0;

	AnimationData* anim = GetSelectedAnimation();
	if (anim) {
		// The animation takes over the skeleton pose; deselect any pose in
		// the pose list without firing its handler.
		if (auto cPoseName = (wxComboBox*)FindWindowByName("cPoseName"))
			cPoseName->SetStringSelection("<New>");

		ApplyAnimationFrame(0);
	}
	else {
		// "<None>" sentinel: reset all bones like the "<New>" pose entry.
		ResetAllPoseBones();
		PoseToGUI();
		ApplyPose();
	}

	UpdateAnimationPlayerUI();
	UpdatePoseButtonStates();
}

void OutfitStudioFrame::OnLoadHkxAnimation(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog loadDlg(this, _("Select animation file"), wxEmptyString, wxEmptyString, "HKX animation files (*.hkx)|*.hkx", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (loadDlg.ShowModal() == wxID_CANCEL)
		return;

	std::string skeletonHkxPath;
	if (!GetReferenceSkeletonHkxPath(skeletonHkxPath, _("Load Animation File")))
		return;

	wxString srcPath = loadDlg.GetPath();
	wxFileName srcFn(srcPath);

	AnimationData anim;
	anim.name = std::string(srcFn.GetName().ToUTF8().data());

	std::string loadError;
	if (!PoseDataCollection::LoadHkxAnimation(skeletonHkxPath, std::string(srcPath.ToUTF8().data()), anim, &loadError)) {
		wxString message = loadError.empty() ? _("Failed to load the animation file.") : wxString::FromUTF8(loadError);
		wxMessageBox(message, _("Load Animation File"), wxOK | wxICON_ERROR, this);
		return;
	}

	if (!cAnimationName)
		return;

	wxString uniqueName = wxString::FromUTF8(anim.name);
	if (cAnimationName->FindString(uniqueName) != wxNOT_FOUND) {
		for (int suffix = 1;; ++suffix) {
			wxString candidate = wxString::Format("%s (%d)", wxString::FromUTF8(anim.name), suffix);
			if (cAnimationName->FindString(candidate) == wxNOT_FOUND) {
				uniqueName = candidate;
				break;
			}
		}
	}
	anim.name = std::string(uniqueName.ToUTF8().data());

	AnimationData* added = poseDataCollection.AddAnimation(std::move(anim));
	int idx = cAnimationName->Append(wxString::FromUTF8(added->name), added);
	cAnimationName->SetSelection(idx);

	wxCommandEvent dummy;
	OnSelectAnimation(dummy);

	if (statusBar)
		statusBar->SetStatusText(_("Animation file loaded."), 0);
}

void OutfitStudioFrame::OnAnimPlayPause(wxCommandEvent& WXUNUSED(event)) {
	if (animPlaying)
		PauseAnimationPlayback();
	else
		StartAnimationPlayback();
}

void OutfitStudioFrame::OnAnimFrameSlider(wxScrollEvent& event) {
	if (!GetSelectedAnimation())
		return;

	// Seeking teleports the pose; re-seat the physics bodies on it instead of
	// letting them interpret the jump as a huge velocity.
	if (physicsRunning)
		physics->ResetDynamics();

	ApplyAnimationFrame(std::max(event.GetPosition(), 0), !animPlaying);

	// Seeking moves the playhead, so playback has to continue from there.
	if (animPlaying)
		RestartAnimationClock();
}

void OutfitStudioFrame::OnAnimSpeedChanged(wxCommandEvent& WXUNUSED(event)) {
	// The frame position is derived from the elapsed time and the speed, so the
	// clock has to restart at the current frame for the new speed to take over
	// from there rather than jump.
	if (animPlaying)
		RestartAnimationClock();
}

void OutfitStudioFrame::OnAnimInterpolateChanged(wxCommandEvent& WXUNUSED(event)) {
	AnimationData* anim = GetSelectedAnimation();
	if (!anim)
		return;

	// While playing, the next tick picks the new mode up on its own.
	if (!animPlaying)
		ApplyAnimationFrame(animCurrentFrame);
}

void OutfitStudioFrame::OnAnimPlaybackTimer(wxTimerEvent& WXUNUSED(event)) {
	PumpAnimationPlayback();
}

void OutfitStudioFrame::OnAnimIdle(wxIdleEvent& event) {
	if (!animPlaying)
		return;

	// Idle events asked for with RequestMore come back as fast as the event loop
	// can spin, so hand the CPU back while the next frame is still a way off
	// instead of burning a core on the wait. A millisecond at a time stays well
	// inside one frame even at the 120 fps cap, where frames are 8.3 ms apart.
	if (GetAnimFrameDueInMicro() > 1500)
		wxMilliSleep(1);
	else
		PumpAnimationPlayback();

	event.RequestMore();
}

void OutfitStudioFrame::InjectPhysicsCameraYaw(float deltaDegrees) {
	if (physicsRunning)
		physics->InjectCameraYaw(deltaDegrees);
}

void OutfitStudioFrame::OnPhysicsCheckBox(wxCommandEvent& e) {
	// Physics only simulates while posing or playing; make checking the box
	// take effect immediately by enabling pose mode along with it.
	if (e.IsChecked() && !project->bPose && !animPlaying)
		ActivatePose(true);
	else
		UpdatePhysicsState();
}

void OutfitStudioFrame::OnPhysicsVisCheckBox(wxCommandEvent& e) {
	if (physics && physicsRunning) {
		physics->UpdateDebugVis(glView->gls, e.IsChecked());
		glView->Render();
	}
}

void OutfitStudioFrame::OnPhysicsGrabCheckBox(wxCommandEvent& e) {
	// Turning it off mid-drag is only possible with a lost mouse capture, which
	// releases the grab already; this is just to be sure nothing keeps pulling.
	if (!e.IsChecked() && glView)
		glView->EndPhysicsGrab();
}

void OutfitStudioFrame::OnPhysicsProbeCheckBox(wxCommandEvent& WXUNUSED(e)) {
	UpdatePhysicsProbeState();
}

// The sliders count in tenths of a NIF unit, so dragging one lands on a decimal
// rather than jumping a whole unit at a time. The field beside it holds the value
// that is actually used, and can be typed into for anything finer or further out
// than the slider reaches.
static constexpr double physicsProbeSliderScale = 10.0;

void OutfitStudioFrame::OnPhysicsProbeSlider(wxScrollEvent& e, wxTextCtrl* text) {
	if (!text)
		return;

	// "%g" rather than streaming the value in, which would spell 60 as 60.000000
	// and leave nothing of it visible in a field this narrow
	text->ChangeValue(wxString::Format("%g", e.GetPosition() / physicsProbeSliderScale));
	ApplyPhysicsProbe();
}

void OutfitStudioFrame::OnPhysicsProbeXSlider(wxScrollEvent& e) {
	OnPhysicsProbeSlider(e, physicsProbeXText);
}
void OutfitStudioFrame::OnPhysicsProbeYSlider(wxScrollEvent& e) {
	OnPhysicsProbeSlider(e, physicsProbeYText);
}
void OutfitStudioFrame::OnPhysicsProbeZSlider(wxScrollEvent& e) {
	OnPhysicsProbeSlider(e, physicsProbeZText);
}
void OutfitStudioFrame::OnPhysicsProbeSizeSlider(wxScrollEvent& e) {
	OnPhysicsProbeSlider(e, physicsProbeSizeText);
}

void OutfitStudioFrame::OnPhysicsProbeText(wxTextCtrl* text, wxSlider* slider) {
	if (!text || !slider)
		return;

	double val = 0.0;
	if (!text->GetValue().ToDouble(&val))
		return;

	// Only to keep the handle where the value is - the slider clamps to its own
	// range, and a typed value outside it is still the one that gets used.
	slider->SetValue(static_cast<int>(std::lround(val * physicsProbeSliderScale)));
	ApplyPhysicsProbe();
}

void OutfitStudioFrame::OnPhysicsProbeXText(wxCommandEvent& WXUNUSED(e)) {
	OnPhysicsProbeText(physicsProbeXText, physicsProbeX);
}
void OutfitStudioFrame::OnPhysicsProbeYText(wxCommandEvent& WXUNUSED(e)) {
	OnPhysicsProbeText(physicsProbeYText, physicsProbeY);
}
void OutfitStudioFrame::OnPhysicsProbeZText(wxCommandEvent& WXUNUSED(e)) {
	OnPhysicsProbeText(physicsProbeZText, physicsProbeZ);
}
void OutfitStudioFrame::OnPhysicsProbeSizeText(wxCommandEvent& WXUNUSED(e)) {
	OnPhysicsProbeText(physicsProbeSizeText, physicsProbeSize);
}

void OutfitStudioFrame::UpdatePhysicsProbeControl(bool enabled) {
	if (!cbPhysicsProbe)
		return;

	if (!enabled)
		cbPhysicsProbe->SetValue(false);

	cbPhysicsProbe->Enable(enabled);
	UpdatePhysicsProbeState();
}

// Brings the ball, its sliders and the simulation in line with the checkbox.
void OutfitStudioFrame::UpdatePhysicsProbeState() {
	const bool enabled = physicsRunning && cbPhysicsProbe && cbPhysicsProbe->IsChecked();

	// The controls are the only way to place the ball, so they are of no use
	// while there is no ball to place
	if (physicsProbePanel && physicsProbePanel->IsShown() != enabled) {
		physicsProbePanel->Show(enabled);

		// The panel sits inside a collapsible pane, and it is that pane's best
		// size the tool area lays out against, so it has to be recomputed
		// before the pane will make room for the sliders.
		if (physicsPane) {
			physicsPane->GetPane()->Layout();
			physicsPane->InvalidateBestSize();
		}

		UpdateToolScrollLayout();
	}

	if (enabled) {
		ApplyPhysicsProbe();
		return;
	}

	if (physics)
		physics->ClearProbe();

	if (glView)
		glView->HidePhysicsProbe();
}

// Hands the ball's place and size to the simulation and to the mesh that draws it.
// The fields are read rather than the sliders, so a value typed finer than the
// slider steps or beyond the range it covers is the one that counts. Both are in
// NIF units, the space the simulation works in.
void OutfitStudioFrame::ApplyPhysicsProbe() {
	if (!physicsRunning || !physicsProbeXText || !physicsProbeYText || !physicsProbeZText || !physicsProbeSizeText)
		return;

	double x = 0.0, y = 0.0, z = 0.0, size = 0.0;
	if (!physicsProbeXText->GetValue().ToDouble(&x) || !physicsProbeYText->GetValue().ToDouble(&y) || !physicsProbeZText->GetValue().ToDouble(&z)
		|| !physicsProbeSizeText->GetValue().ToDouble(&size))
		return;

	// A ball with no radius collides with nothing, and there would be nothing to draw
	if (size <= 0.0)
		return;

	const Vector3 position(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	const float radius = static_cast<float>(size);

	physics->SetProbe(position, radius);

	if (glView) {
		glView->ShowPhysicsProbe(position, radius);
		glView->Render();
	}
}

void OutfitStudioFrame::UpdatePhysicsGrabControl(bool enabled) {
	if (!cbPhysicsGrab)
		return;

	if (!enabled) {
		cbPhysicsGrab->SetValue(false);
		if (glView)
			glView->EndPhysicsGrab();
	}

	cbPhysicsGrab->Enable(enabled);
}

bool OutfitStudioFrame::IsPhysicsGrabEnabled() const {
	return physicsRunning && cbPhysicsGrab && cbPhysicsGrab->IsChecked();
}

const std::unordered_set<std::string>& OutfitStudioFrame::GetPhysicsAffectedShapes() const {
	static const std::unordered_set<std::string> none;
	return physicsRunning ? physics->AffectedShapes() : none;
}

void OutfitStudioFrame::RefitPhysicsBVH() {
	if (!physicsRunning)
		return;

	for (const auto& shapeName : physics->AffectedShapes()) {
		Mesh* m = glView->GetMesh(shapeName);
		if (m && m->bvh)
			m->bvh->Refit();
	}
}

bool OutfitStudioFrame::BeginPhysicsGrab(const std::vector<Physics::GrabTarget>& targets) {
	return physicsRunning && physics->BeginGrab(targets);
}

void OutfitStudioFrame::UpdatePhysicsGrab(const Vector3& offset) {
	if (physicsRunning)
		physics->UpdateGrab(offset);
}

void OutfitStudioFrame::EndPhysicsGrab() {
	if (physics)
		physics->EndGrab();
}

void OutfitStudioFrame::OnPhysicsWindSlider(wxScrollEvent& WXUNUSED(e)) {
	ApplyPhysicsWind();
}

void OutfitStudioFrame::OnPhysicsWindDir(wxCommandEvent& WXUNUSED(e)) {
	ApplyPhysicsWind();
}

void OutfitStudioFrame::ApplyPhysicsWind() {
	if (!physics)
		return;

	// The choice control lists Physics::WindDirectionNames() in order
	if (physicsWindDir)
		physics->SetWindDirection(Physics::WindDirectionFromIndex(physicsWindDir->GetSelection()));

	if (physicsWindSlider)
		physics->SetWindStrength(physicsWindSlider->GetValue() / 100.0f);
}

void OutfitStudioFrame::UpdatePhysicsControlsVisibility() {
	if (!cbPhysics || !physicsPane)
		return;

	const bool available = project && Physics::HasPhysicsLinks(project->GetWorkNif(), project->shapePhysicsFiles);

	// The tree lists what the loaded meshes reference, so it follows them even
	// when the pane's visibility does not change.
	if (available)
		RefreshPhysicsSystemTree();
	else if (physicsSystemTree)
		physicsSystemTree->DeleteAllItems();

	if (available == physicsAvailable)
		return;

	physicsAvailable = available;

	// Deleting the last shape with physics while it simulates has to stop it,
	// and UpdatePhysicsState only ever starts physics for a checked box
	if (!available)
		cbPhysics->SetValue(false);

	// The pane belongs to the bones tab; leaving it shows nothing regardless.
	physicsPane->Show(available && currentTabButton == boneTabButton);

	if (!available)
		UpdatePhysicsState();

	UpdateToolScrollLayout();
}

void OutfitStudioFrame::ShutdownPhysics() {
	StopPhysicsPump();
	physicsRebuildTimer.Stop();
	physicsRunning = false;

	// It is a view over documents that are about to go with the project
	if (physicsEditor)
		physicsEditor->Close();

	if (physicsSystemTree)
		physicsSystemTree->DeleteAllItems();

	// Every caller of this is about to delete the project the documents were
	// read out of. Stopping the simulation on its own must not clear them:
	// unchecking the box and checking it again has to come back to the edits.
	physicsXml.Clear();

	if (project)
		project->physicsPose = nullptr;

	if (physics) {
		if (glView)
			physics->UpdateDebugVis(glView->gls, false);
		physics->Clear();
	}

	if (cbPhysics)
		cbPhysics->SetValue(false);

	if (cbPhysicsVis) {
		cbPhysicsVis->SetValue(false);
		cbPhysicsVis->Enable(false);
	}

	UpdatePhysicsGrabControl(false);
	UpdatePhysicsProbeControl(false);
}

namespace {
// Which physics XML a row of the compact tree in the Physics pane stands for.
// Every row of a document's subtree carries it, so a click anywhere under a
// file still says which file.
class PhysicsXmlItemData : public wxTreeItemData {
public:
	explicit PhysicsXmlItemData(std::string inXmlPath)
		: xmlPath(std::move(inXmlPath)) {}

	std::string xmlPath;
};

wxString PhysicsXmlFileName(const std::string& xmlPath) {
	return wxString::FromUTF8(Physics::XmlFileName(xmlPath));
}
}

void OutfitStudioFrame::BindPhysicsXmlSource() {
	if (!project)
		return;

	// The session reads the XMLs out of the project, and a project can be
	// replaced without the session noticing, so this is re-done rather than
	// done once.
	OutfitProject* proj = project;
	physicsXml.SetSourceResolver([proj](const std::string& xmlPath, std::string* outSourcePath, bool* outFromArchive) {
		return proj->GetPhysicsXmlStream(xmlPath, outSourcePath, outFromArchive);
	});
}

void OutfitStudioFrame::RefreshPhysicsSystemTree() {
	if (!physicsSystemTree)
		return;

	physicsSystemTree->DeleteAllItems();

	if (!project) {
		if (btnPhysicsEdit)
			btnPhysicsEdit->Enable(false);
		return;
	}

	BindPhysicsXmlSource();

	// While the simulation runs, what it actually built is the truth. Before
	// that, the links in the meshes are - which is what lets the pane and the
	// editor work without simulating anything.
	const std::vector<Physics::SystemInfo> systems = physicsRunning && physics
														 ? physics->Systems()
														 : Physics::CollectPhysicsXmlLinks(project->GetWorkNif(), project->shapePhysicsFiles);

	const wxTreeItemId root = physicsSystemTree->AddRoot("physics");

	for (const Physics::SystemInfo& system : systems) {
		std::string error;
		Physics::XmlDocument* doc = physicsXml.Open(system.xmlPath, error);

		wxString label = PhysicsXmlFileName(system.xmlPath);
		if (doc && doc->IsDirty())
			label += " *";

		const wxTreeItemId item = physicsSystemTree->AppendItem(root, label, -1, -1, new PhysicsXmlItemData(system.xmlPath));

		if (doc) {
			const size_t bones = doc->ChildrenOfKind(Physics::ElementKind::Bone).size();
			const size_t shapes = doc->ChildrenOfKinds({Physics::ElementKind::PerVertexShape, Physics::ElementKind::PerTriangleShape}).size();
			const size_t constraints = doc->ChildrenOfKinds({Physics::ElementKind::GenericConstraint,
															Physics::ElementKind::StiffSpringConstraint,
															Physics::ElementKind::ConeTwistConstraint,
															Physics::ElementKind::ConstraintGroup})
										   .size();

			physicsSystemTree->AppendItem(item,
										  wxString::Format(_("%d bones, %d shapes, %d constraints"),
														   static_cast<int>(bones),
														   static_cast<int>(shapes),
														   static_cast<int>(constraints)),
										  -1,
										  -1,
										  new PhysicsXmlItemData(system.xmlPath));
		}
		else {
			// Says either that the file is not there or that only the editor
			// cannot read it - in the latter case it is still simulated, see
			// XmlEditSession::Resolver.
			physicsSystemTree->AppendItem(item, wxString::FromUTF8(error), -1, -1, new PhysicsXmlItemData(system.xmlPath));
		}

		for (const std::string& shapeName : system.shapeNames) {
			physicsSystemTree->AppendItem(item,
										  wxString::Format(_("Shape: %s"), wxString::FromUTF8(shapeName)),
										  -1,
										  -1,
										  new PhysicsXmlItemData(system.xmlPath));
		}

		physicsSystemTree->Expand(item);
	}

	if (btnPhysicsEdit)
		btnPhysicsEdit->Enable(!physicsXml.Empty());

	// The editor is a view over the same session
	if (physicsEditor)
		physicsEditor->RefreshDocuments();
}

std::string OutfitStudioFrame::SelectedPhysicsXmlPath() const {
	if (!physicsSystemTree)
		return std::string();

	const wxTreeItemId selected = physicsSystemTree->GetSelection();
	if (selected.IsOk()) {
		if (auto* data = static_cast<PhysicsXmlItemData*>(physicsSystemTree->GetItemData(selected)))
			return data->xmlPath;
	}

	// Nothing selected, but with a single XML there is no ambiguity to resolve
	const std::vector<std::string> paths = physicsXml.Paths();
	return paths.size() == 1 ? paths.front() : std::string();
}

void OutfitStudioFrame::OnPhysicsSystemTreeSelect(wxTreeEvent& event) {
	event.Skip();

	if (!physicsEditor)
		return;

	const std::string xmlPath = SelectedPhysicsXmlPath();
	if (!xmlPath.empty())
		physicsEditor->SelectDocument(xmlPath);
}

void OutfitStudioFrame::OnPhysicsEdit(wxCommandEvent& WXUNUSED(event)) {
	if (!project)
		return;

	if (!physicsEditor) {
		auto* editor = new PhysicsEditorDialog(this);
		if (!editor->IsLoaded()) {
			editor->Destroy();
			return;
		}

		physicsEditor = editor;

		// Nulled before the frame's own handler destroys it, the way EditUV
		// does it, so nothing is left pointing at a window on its way out.
		physicsEditor->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& closeEvent) {
			physicsEditor = nullptr;
			closeEvent.Skip();
		});

		physicsEditor->CenterOnParent();
		physicsEditor->Show();
	}
	else {
		physicsEditor->RefreshDocuments();
		physicsEditor->Raise();
	}

	const std::string xmlPath = SelectedPhysicsXmlPath();
	if (!xmlPath.empty())
		physicsEditor->SelectDocument(xmlPath);
}

size_t OutfitStudioFrame::BuildPhysicsFromXml() {
	if (!physics || !project)
		return 0;

	// Everything the builder resolves goes through the edit session, which
	// adopts each XML on the way and serves what the editor holds from then on.
	// Nothing is edited yet the first time round, so this changes nothing about
	// what gets simulated - it only means every later rebuild reads the edits.
	BindPhysicsXmlSource();

	physicsXml.ClearWarnings();

	std::vector<std::string> warnings;
	const size_t systemCount
		= physics->BuildFromNif(project->GetWorkNif(), project->GetWorkAnim(), physicsXml.Resolver(), project->shapePhysicsFiles, warnings);

	// Files the session could not take over come first: they explain why the
	// editor is about to show fewer documents than the simulation is running.
	for (auto& warning : physicsXml.Warnings())
		wxLogWarning("Physics: %s", warning);

	for (auto& warning : warnings)
		wxLogWarning("Physics: %s", warning);

	// Whatever asked for this rebuild has had it
	physicsXml.ClearRebuildRequest();
	physicsRebuildTimer.Stop();

	return systemCount;
}

bool OutfitStudioFrame::PatchOrRebuildPhysics(Physics::XmlDocument& doc, tinyxml2::XMLElement* element, const Physics::ChildDesc& desc) {
	bool patched = false;

	if (element && physicsRunning && physics && desc.patch == Physics::PatchKind::Hot) {
		Physics::PatchRequest request;
		request.xmlPath = doc.XmlPath();
		request.kind = Physics::KindFromName(element->Name());
		request.property = desc.name;
		request.value = doc.GetValue(element, desc);

		if (const char* name = element->Attribute("name"))
			request.elementName = name;
		if (const char* bodyA = element->Attribute("bodyA"))
			request.bodyA = bodyA;
		if (const char* bodyB = element->Attribute("bodyB"))
			request.bodyB = bodyB;

		// The controller has the last word: the schema table says what is
		// meant to be patchable, this says what actually could be.
		patched = physics->PatchProperty(request);
	}

	physicsXml.NotifyChanged(doc.XmlPath(), !patched);

	if (patched) {
		// The change is already in the simulation, but the viewport is only
		// repainted by the pump, which does not run in animation lockstep.
		if (glView && !physicsPumpActive)
			glView->Render();
	}
	else {
		RequestPhysicsRebuild();
	}

	return patched;
}

void OutfitStudioFrame::NotifyPhysicsXmlEdited(const std::string& xmlPath) {
	physicsXml.NotifyChanged(xmlPath, true);
	RequestPhysicsRebuild();
}


void OutfitStudioFrame::RefreshPhysicsLinks() {
	BindPhysicsXmlSource();

	// A first link makes the whole pane appear, and this refills the tree
	// whether or not that changed.
	UpdatePhysicsControlsVisibility();

	if (physicsEditor)
		physicsEditor->RefreshDocuments();

	RequestPhysicsRebuild();
}

wxString OutfitStudioFrame::PhysicsXmlDirectory() const {
	const wxString gameData = wxString::FromUTF8(Config["GameDataPath"]);
	if (gameData.empty())
		return wxString();

	// Where hdtSMP64 itself looks, and where nearly every mod puts them. The
	// folder is only a starting point for the dialog, so it not existing yet is
	// not a problem worth reporting.
	wxFileName configs(gameData, wxString());
	configs.AppendDir("SKSE");
	configs.AppendDir("Plugins");
	configs.AppendDir("hdtSkinnedMeshConfigs");
	if (configs.DirExists())
		return configs.GetPath();

	return gameData;
}

std::string OutfitStudioFrame::GameRelativeXmlPath(const wxString& path) const {
	const wxString gameData = wxString::FromUTF8(Config["GameDataPath"]);
	if (gameData.empty() || path.empty())
		return std::string();

	wxFileName file(path);
	file.MakeAbsolute();

	wxFileName root(gameData, wxString());
	root.MakeAbsolute();

	// MakeRelativeTo happily walks up out of the folder with "..", which is not
	// a path any NIF can carry, so the answer only counts when it stays inside.
	wxFileName relative = file;
	if (!relative.MakeRelativeTo(root.GetPath()))
		return std::string();

	const wxString result = relative.GetFullPath();
	if (result.StartsWith("..") || wxFileName(result).IsAbsolute())
		return std::string();

	// Backslashes, the way the extra data in every shipped model spells it.
	wxString spelled = result;
	spelled.Replace("/", "\\");
	return std::string(spelled.ToUTF8());
}

bool OutfitStudioFrame::SavePhysicsXml(Physics::XmlDocument& doc, bool allowExport) {
	if (doc.Origin() != Physics::XmlOrigin::Loose || doc.SourcePath().empty()) {
		if (!allowExport) {
			wxLogWarning(_("'%s' did not come from a loose file, so there is nothing to save it over. Use Export instead."),
						 wxString::FromUTF8(doc.XmlPath()));
			return false;
		}

		return ExportPhysicsXml(doc);
	}

	std::string error;
	if (!doc.SaveToFile(doc.SourcePath(), error)) {
		wxMessageBox(wxString::Format(_("Could not write '%s':\n\n%s"), wxString::FromUTF8(doc.SourcePath()), wxString::FromUTF8(error)),
					 _("Save physics XML"),
					 wxICON_ERROR);
		return false;
	}

	doc.MarkClean();

	// The editor draws the unsaved marker, and this did not go through it.
	if (physicsEditor)
		physicsEditor->RefreshDocuments();

	return true;
}

bool OutfitStudioFrame::ExportPhysicsXml(Physics::XmlDocument& doc) {
	// The name the NIF knows it by is the name it should be offered under, so
	// exporting a file read out of an archive lands beside where it came from.
	wxFileName suggested(wxString::FromUTF8(doc.SourcePath().empty() ? doc.XmlPath() : doc.SourcePath()));

	wxFileDialog dialog(this,
						_("Export physics XML"),
						doc.SourcePath().empty() ? PhysicsXmlDirectory() : suggested.GetPath(),
						suggested.GetFullName(),
						"HDT-SMP physics files (*.xml)|*.xml",
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	if (dialog.ShowModal() == wxID_CANCEL)
		return false;

	const wxString path = dialog.GetPath();

	std::string error;
	if (!doc.SaveToFile(std::string(path.ToUTF8()), error)) {
		wxMessageBox(wxString::Format(_("Could not write '%s':\n\n%s"), path, wxString::FromUTF8(error)), _("Export physics XML"), wxICON_ERROR);
		return false;
	}

	// Rebound to where it now lives, so Save goes there from here on. What the
	// NIF calls it does not change: exporting a copy elsewhere does not
	// relink the model.
	doc.SetSource(doc.XmlPath(), std::string(path.ToUTF8()), Physics::XmlOrigin::Loose);
	doc.MarkClean();

	if (physicsEditor)
		physicsEditor->RefreshDocuments();

	return true;
}

bool OutfitStudioFrame::ImportPhysicsXml(Physics::XmlDocument& doc, const wxString& path) {
	std::ifstream stream(std::string(path.ToUTF8()), std::ios::binary);
	if (!stream.good()) {
		wxMessageBox(wxString::Format(_("Could not read '%s'."), path), _("Import physics XML"), wxICON_ERROR);
		return false;
	}

	std::ostringstream contents;
	contents << stream.rdbuf();

	// Taken back as one step, like any other edit: importing over the wrong
	// document is exactly the mistake undo exists for.
	doc.BeginEdit("import");

	std::string error;
	if (!doc.ReplaceContent(contents.str(), error)) {
		doc.Undo();
		wxMessageBox(wxString::Format(_("'%s' is not a physics XML this editor can read:\n\n%s"), path, wxString::FromUTF8(error)),
					 _("Import physics XML"),
					 wxICON_ERROR);
		return false;
	}

	NotifyPhysicsXmlEdited(doc.XmlPath());
	return true;
}

bool OutfitStudioFrame::CheckPendingPhysicsXml() {
	if (!physicsXml.AnyDirty())
		return true;

	wxString names;
	for (Physics::XmlDocument* doc : physicsXml.Documents()) {
		if (!doc->IsDirty())
			continue;

		names += "\n    " + wxString::FromUTF8(doc->XmlPath());
	}

	wxMessageDialog dialog(this,
						   wxString::Format(_("These physics XMLs have changes that are not on disk:\n%s\n\nWould you like to save them now?"), names),
						   _("Unsaved physics XML changes"),
						   wxYES_NO | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);

	const int answer = dialog.ShowModal();
	if (answer == wxID_CANCEL)
		return false;
	if (answer == wxID_NO)
		return true;

	// Saving is what was asked for, so a document that cannot be written back
	// over anything asks where to put it rather than being skipped silently.
	for (Physics::XmlDocument* doc : physicsXml.Documents()) {
		if (doc->IsDirty() && !SavePhysicsXml(*doc, true))
			return false;
	}

	return true;
}

void OutfitStudioFrame::RequestPhysicsRebuild() {
	if (!physicsRunning)
		return;

	// Long enough that dragging a slider does not rebuild on the way, short
	// enough that letting go of it looks immediate.
	constexpr int physicsRebuildDelayMS = 250;

	// Restarting the timer is the debounce: a value dragged through a hundred
	// intermediate settings rebuilds once, when the dragging stops.
	physicsRebuildTimer.Start(physicsRebuildDelayMS, wxTIMER_ONE_SHOT);
}

void OutfitStudioFrame::OnPhysicsRebuildTimer(wxTimerEvent& WXUNUSED(event)) {
	RebuildPhysics();
}

void OutfitStudioFrame::RebuildPhysics() {
	physicsRebuildTimer.Stop();

	if (!physicsRunning || !physics || !project)
		return;

	// The grab holds rigid bodies of the systems about to be thrown away, and
	// the marker it drew belongs to a drag that cannot survive this
	if (glView && physics->IsGrabbing())
		glView->EndPhysicsGrab();

	const Physics::DynamicState state = physics->CaptureDynamicState();

	// The map the project reads the overrides out of belongs to the controller
	// and is cleared and refilled by the rebuild
	project->physicsPose = nullptr;

	const size_t systemCount = BuildPhysicsFromXml();
	if (systemCount == 0) {
		// The edited XMLs no longer build anything. Unchecking the box and
		// leaving physicsRunning set is what sends UpdatePhysicsState down its
		// teardown path, which is where the overrides come back off the pose.
		statusBar->SetStatusText(_("Physics stopped: the edited XMLs build no systems"));

		if (cbPhysics)
			cbPhysics->SetValue(false);

		UpdatePhysicsState();
		return;
	}

	project->physicsPose = &physics->PoseOverrides();

	// Puts the cloth back where it was and moving as it was, instead of
	// dropping it onto the pose the fresh systems were built on
	physics->RestoreDynamicState(state);

	// The controller is new and knows none of the current settings
	ApplyPhysicsWind();
	UpdatePhysicsProbeState();
	RefreshPhysicsSystemTree();

	if (glView && cbPhysicsVis)
		physics->UpdateDebugVis(glView->gls, cbPhysicsVis->IsChecked());

	statusBar->SetStatusText(wxString::Format(_("Physics rebuilt: %zu system(s)"), systemCount));
}

void OutfitStudioFrame::UpdatePhysicsState() {
	const bool desired = cbPhysics && cbPhysics->IsChecked() && project && (project->bPose || animPlaying);

	if (desired && !physicsRunning) {
		if (!physics)
			physics = std::make_unique<Physics::Controller>();

		size_t systemCount = BuildPhysicsFromXml();

		if (systemCount == 0) {
			statusBar->SetStatusText(_("No physics XMLs found in loaded meshes"));
			physics->Clear();

			// Nothing is simulating, so the box must not claim otherwise
			cbPhysics->SetValue(false);
			return;
		}

		statusBar->SetStatusText(wxString::Format(_("Physics active: %zu system(s)"), systemCount));
		project->physicsPose = &physics->PoseOverrides();
		physics->ResetDynamics();
		physicsRunning = true;

		// The tree now reports what was built rather than what was linked, and
		// the two can differ: an XML that resolves to nothing builds nothing.
		RefreshPhysicsSystemTree();

		// The step clock must be valid for the lockstep path too, where
		// StartPhysicsPump never runs
		physicsClock.Reset(GetAnimTargetFps());

		// The controller is fresh; re-apply the current wind settings
		ApplyPhysicsWind();

		if (cbPhysicsVis)
			cbPhysicsVis->Enable();

		UpdatePhysicsGrabControl(true);
		UpdatePhysicsProbeControl(true);

		if (!animPlaying)
			StartPhysicsPump();
	}
	else if (!desired && physicsRunning) {
		// Drop the overrides before the pose is applied again, or the frame
		// that is supposed to restore the user pose still shows the simulated
		// one.
		physicsRunning = false;
		project->physicsPose = nullptr;
		physics->UpdateDebugVis(glView->gls, false);
		physics->Clear();
		StopPhysicsPump();

		if (cbPhysicsVis) {
			cbPhysicsVis->SetValue(false);
			cbPhysicsVis->Enable(false);
		}

		UpdatePhysicsGrabControl(false);
		UpdatePhysicsProbeControl(false);

		// Restore the clean user pose without the physics overrides. Also
		// catches the BVH up with what is displayed: the pump skipped the
		// rebuild on every tick.
		ApplyPose();
	}
	else if (physicsRunning) {
		// Animation playback took over or ended: while playing, ApplyPose
		// steps the simulation in lockstep instead of the pump.
		if (animPlaying && physicsPumpActive)
			StopPhysicsPump();
		else if (!animPlaying && !physicsPumpActive)
			StartPhysicsPump();
	}

	// A checked box always means "simulating". Anything that stopped physics -
	// leaving the bones tab, turning off pose mode, ending playback - clears
	// it instead of leaving a checked box that does nothing.
	if (cbPhysics && !physicsRunning)
		cbPhysics->SetValue(false);
}

void OutfitStudioFrame::StartPhysicsPump() {
	if (physicsPumpActive)
		return;

	physicsPumpActive = true;
	physicsClock.Reset(GetAnimTargetFps());
	SetHighResolutionTimers(true);
	Bind(wxEVT_IDLE, &OutfitStudioFrame::OnPhysicsIdle, this);
	physicsTimer.Start(Physics::PumpTimerIntervalMS);
}

void OutfitStudioFrame::StopPhysicsPump() {
	if (!physicsPumpActive)
		return;

	physicsTimer.Stop();
	Unbind(wxEVT_IDLE, &OutfitStudioFrame::OnPhysicsIdle, this);
	SetHighResolutionTimers(false);
	physicsPumpActive = false;
}

void OutfitStudioFrame::PumpPhysics() {
	if (!physicsPumpActive || !physicsRunning)
		return;

	float dtSeconds = 0.0f;
	if (!physicsClock.StepDue(dtSeconds))
		return;

	physics->Step(dtSeconds);

	const auto& affectedShapes = physics->AffectedShapes();
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		if (affectedShapes.count(shape->name.get()) == 0)
			continue;

		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);

		// BVH rebuilds only serve picking and brushes; way too expensive per
		// frame. StopPhysicsPump does one full ApplyPose to catch up.
		glView->UpdateMeshVertices(shape->name.get(), &verts, false, true, false);
	}

	if (cbPhysicsVis && cbPhysicsVis->IsChecked())
		physics->UpdateDebugVis(glView->gls, true);

	glView->Render();
}

void OutfitStudioFrame::OnPhysicsTimer(wxTimerEvent& WXUNUSED(event)) {
	PumpPhysics();
}

void OutfitStudioFrame::OnPhysicsIdle(wxIdleEvent& event) {
	if (!physicsPumpActive)
		return;

	if (physicsClock.UntilDue() > 1500)
		wxMilliSleep(1);
	else
		PumpPhysics();

	event.RequestMore();
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
		int colorR = Config.GetIntValue("Rendering/ColorBackground.r");
		int colorG = Config.GetIntValue("Rendering/ColorBackground.g");
		int colorB = Config.GetIntValue("Rendering/ColorBackground.b");
		gls.SetBackgroundColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorWire")) {
		int colorR = Config.GetIntValue("Rendering/ColorWire.r");
		int colorG = Config.GetIntValue("Rendering/ColorWire.g");
		int colorB = Config.GetIntValue("Rendering/ColorWire.b");
		gls.SetWireColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorPoints")) {
		int colorR = Config.GetIntValue("Rendering/ColorPoints.r");
		int colorG = Config.GetIntValue("Rendering/ColorPoints.g");
		int colorB = Config.GetIntValue("Rendering/ColorPoints.b");
		gls.SetPointColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorPointsMasked")) {
		int colorR = Config.GetIntValue("Rendering/ColorPointsMasked.r");
		int colorG = Config.GetIntValue("Rendering/ColorPointsMasked.g");
		int colorB = Config.GetIntValue("Rendering/ColorPointsMasked.b");
		gls.SetMaskedPointColor(Vector3(colorR / 255.0f, colorG / 255.0f, colorB / 255.0f));
	}

	gls.SetPointSizeParams(
		Config.GetFloatValue("Rendering/PointSizeMin", 4.0f),
		Config.GetFloatValue("Rendering/PointSizeMax", 14.0f),
		Config.GetFloatValue("Rendering/PointSizeScale", 0.6f));

	bool perspectiveView = OutfitStudioConfig.GetBoolValue("Rendering/PerspectiveView", true);
	os->menuBar->Check(XRCID("btnViewPerspective"), perspectiveView);
	os->toolBarV->ToggleTool(XRCID("btnViewPerspective"), perspectiveView);
	gls.SetPerspective(perspectiveView);

	gls.SetComplexMaterialEnabled(OutfitStudioConfig.GetBoolValue("Rendering/ComplexMaterial", true));
	gls.SetPBREnabled(OutfitStudioConfig.GetBoolValue("Rendering/TruePBR", true));

	os->MeshesFromProj();

	UpdateFloor();
	UpdateNodes();
	UpdateBones();

	Render();
}

void wxGLPanel::SetNotifyWindow(wxWindow* win) {
	os = dynamic_cast<OutfitStudioFrame*>(win);
}

void wxGLPanel::AddMeshFromNif(NifFile* nif, const std::string& shapeName) {
	std::vector<std::string> shapeList = nif->GetShapeNames();

	for (size_t i = 0; i < shapeList.size(); i++) {
		if (!shapeName.empty() && shapeList[i] != shapeName)
			continue;

		Mesh* m = gls.AddMeshFromNif(nif, shapeList[i]);
		if (!m)
			continue;

		NiShape* shape = nif->FindBlockByName<NiShape>(shapeList[i]);
		if (shape && shape->IsSkinned()) {
			// Overwrite skin matrix with the one from AnimInfo
			MatTransform globalToShape = os->project->GetWorkAnim()->GetTransformGlobalToShape(shape);
			if (nif->GetHeader().GetVersion().IsSF())
				globalToShape.translation *= sfHavokScale;
			m->SetXformModelToMesh(Mesh::xformNifToMesh.ComposeTransforms(globalToShape.ComposeTransforms(Mesh::xformMeshToNif)));
		}

		m->BuildVertexAdjacency();
		m->BuildEdgeList();
		m->MaskFill(0.0f);
		m->WeightFill(0.0f);

		if (extInitialized) {
			gls.SetContext();
			m->CreateBuffers();
		}
	}
}

void wxGLPanel::SetMeshTextures(
	const std::string& shapeName, const std::vector<std::string>& textureFiles, const bool hasMatFile, const MaterialFile& matFile, const bool reloadTextures) {
	Mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
	std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";
	bool renderAsPBR = false;

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		vShader = Config["AppDir"] + "/res/shaders/fo4_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/fo4_default.frag";
	}
	else if (targetGame == SF) {
		vShader = Config["AppDir"] + "/res/shaders/sf_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/sf_default.frag";
	}
	else if (targetGame == OB) {
		vShader = Config["AppDir"] + "/res/shaders/ob_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/ob_default.frag";
	}
	else if (m->pbr && gls.IsPBREnabled()) {
		// A True PBR shape reads its texture slots differently enough from every other Skyrim shape
		// that it gets its own pair rather than another branch inside the shared one.
		vShader = Config["AppDir"] + "/res/shaders/sk_truepbr.vert";
		fShader = Config["AppDir"] + "/res/shaders/sk_truepbr.frag";
		renderAsPBR = true;
	}

	GLMaterial* mat = gls.AddMaterial(textureFiles, vShader, fShader, reloadTextures, m->hasShader, renderAsPBR);
	if (mat) {
		m->material = mat;

		if (hasMatFile)
			m->UpdateFromMaterialFile(matFile);

		gls.UpdateShaders(m);
	}
}

void wxGLPanel::UpdateMeshVertices(const std::string& shapeName, std::vector<Vector3>* verts, bool updateBVH, bool recalcNormals, bool render, std::vector<Vector2>* uvs) {
	Mesh* m = gls.GetMesh(shapeName);
	if (m) {
		gls.Update(m, verts, uvs);

		if (updateBVH)
			BVHUpdateQueue.insert(m);

		if (recalcNormals)
			m->SmoothNormals();
	}

	if (render)
		gls.RenderOneFrame();
}

void wxGLPanel::RecalculateMeshBVH(const std::string& shapeName) {
	gls.RecalculateMeshBVH(shapeName);
}

void wxGLPanel::ShowShape(const std::string& shapeName, bool show) {
	gls.SetMeshVisibility(shapeName, show);
}

void wxGLPanel::SetActiveShapes(const std::vector<std::string>& shapeNames) {
	gls.SetActiveMeshes(shapeNames);
}

void wxGLPanel::SetSelectedShape(const std::string& shapeName) {
	gls.SetSelectedMesh(shapeName);
}

static const char* kHoverHighlightOverlayName = "_shapehoverhilite";

void wxGLPanel::SetHoverHighlight(const std::string& shapeName) {
	if (shapeName == hoverHighlightName)
		return;

	// Drop any existing overlay first.
	if (!hoverHighlightName.empty()) {
		gls.DeleteOverlay(kHoverHighlightOverlayName);
		hoverHighlightName.clear();
	}

	if (shapeName.empty())
		return;

	Mesh* src = gls.GetMesh(shapeName);
	if (!src || src->nVerts <= 0 || src->nTris <= 0 || !src->verts || !src->tris)
		return;

	// Need an active GL context to create buffers for the overlay mesh.
	if (!gls.SetContext())
		return;

	auto* hov = new Mesh();
	hov->nVerts = src->nVerts;
	hov->nTris = src->nTris;
	hov->verts = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->norms = std::make_unique<nifly::Vector3[]>(hov->nVerts);
	hov->tris = std::make_unique<nifly::Triangle[]>(hov->nTris);

	for (int i = 0; i < hov->nVerts; i++) {
		hov->verts[i] = src->verts[i];
		if (src->norms)
			hov->norms[i] = src->norms[i];
	}
	for (int t = 0; t < hov->nTris; t++)
		hov->tris[t] = src->tris[t];

	// Inherit the source's model-space transform so the overlay sits
	// exactly on top of the original shape (skinned shapes have a
	// non-identity matModel).
	hov->matModel = src->matModel;
	hov->xformMeshToModel = src->xformMeshToModel;
	hov->xformModelToMesh = src->xformModelToMesh;

	hov->shapeName = kHoverHighlightOverlayName;
	hov->color = nifly::Vector3(0.25f, 1.0f, 0.25f); // Light green
	hov->prop.alpha = 0.5f;                           // Semi-transparent blend
	hov->material = gls.GetPrimitiveMaterial();       // Unlit, solid tint
	hov->rendermode = Mesh::RenderMode::UnlitSolid;
	hov->overlayLayer = 100;                          // Draw on top of other overlays
	hov->doublesided = true;
	hov->CreateBuffers();

	gls.AddOverlay(hov);
	hoverHighlightName = shapeName;
	Render();
}

void wxGLPanel::ClearHoverHighlight() {
	if (hoverHighlightName.empty())
		return;

	gls.DeleteOverlay(kHoverHighlightOverlayName);
	hoverHighlightName.clear();
	Render();
}

void wxGLPanel::SetActiveTool(ToolID brushID) {
	activeTool = brushID;

	switch (brushID) {
		case ToolID::MaskBrush: activeBrush = &maskBrush; break;
		case ToolID::InflateBrush: activeBrush = &inflateBrush; break;
		case ToolID::DeflateBrush: activeBrush = &deflateBrush; break;
		case ToolID::MoveBrush: activeBrush = &moveBrush; break;
		case ToolID::SmoothBrush: activeBrush = &smoothBrush; break;
		case ToolID::UndiffBrush: activeBrush = &undiffBrush; break;
		case ToolID::WeightBrush: activeBrush = &weightBrush; break;
		case ToolID::ColorBrush: activeBrush = &colorBrush; break;
		case ToolID::AlphaBrush: activeBrush = &alphaBrush; break;
		default: activeBrush = nullptr; break;
	}

	gls.SetXMirrorCursor(GetToolOptionXMirror());
}

void wxGLPanel::SetLastTool(ToolID tool) {
	lastTool = tool;
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	// No editing shortcuts while an animation is playing.
	if (os && os->IsAnimationPlaying()) {
		event.Skip();
		return;
	}

	if (!event.HasAnyModifiers()) {
		if (event.GetUnicodeKey() == 'V') {
			wxPoint cursorPos(event.GetPosition());

			int vertIndex;
			Mesh* outHitMesh;
			if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex, nullptr, &outHitMesh))
				return;

			if (os->currentTabButton == os->colorsTabButton) {
				auto cpBrushColor = (wxColourPickerCtrl*)os->FindWindowById(XRCID("cpBrushColor"));
				if (!cpBrushColor)
					return;

				if (outHitMesh->vcolors) {
					// Set color for brush to vertex color
					Vector3 vcolor = outHitMesh->vcolors[vertIndex];
					SetColorBrush(vcolor);

					// Set color for picker to vertex color
					wxColour color(vcolor.x * 255.0f, vcolor.y * 255.0f, vcolor.z * 255.0f);
					cpBrushColor->SetColour(color);
				}
			}
			else {
				// Find shape for hit mesh name
				NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(outHitMesh->shapeName);
				if (!shape)
					return;

				os->CloseBrushSettings();

				wxDialog dlg;
				if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(shape, verts);

					Vector3 oldPos = verts[vertIndex];
					XRCCTRL(dlg, "posX", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.x));
					XRCCTRL(dlg, "posY", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.y));
					XRCCTRL(dlg, "posZ", wxTextCtrl)->SetValue(wxString::Format("%0.5f", oldPos.z));

					if (dlg.ShowModal() == wxID_OK) {
						Vector3 newPos;
						newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
						newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
						newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

						// Move vertex in shape directly
						if (!os->bEditSlider)
							os->project->MoveVertex(shape, newPos, vertIndex);

						// To mesh coordinates
						oldPos = Mesh::TransformPosNifToMesh(oldPos);
						newPos = Mesh::TransformPosNifToMesh(newPos);

						UndoStateShape uss;
						uss.shapeName = shape->name.get();
						uss.pointStartState[vertIndex] = oldPos;
						uss.pointEndState[vertIndex] = newPos;

						// Push changes onto undo stack and execute
						UndoStateProject* usp = GetUndoHistory()->PushState();
						usp->undoType = UndoType::VertexPosition;
						usp->usss.push_back(std::move(uss));

						if (os->bEditSlider) {
							usp->sliderName = os->activeSlider;

							float sliderscale = os->project->SliderValue(os->activeSlider);
							if (sliderscale == 0.0)
								sliderscale = 1.0;

							usp->sliderscale = sliderscale;
						}

						ApplyUndoState(usp, false);
						os->UpdateUndoTools();
					}

					if (transformMode)
						ShowTransformTool();
				}
			}
		}
		else if (event.GetUnicodeKey() == '0' && os->menuBar->IsEnabled(XRCID("btnSelect")))
			os->SelectTool(ToolID::Select);
		else if (event.GetUnicodeKey() == '1' && os->menuBar->IsEnabled(XRCID("btnMaskBrush")))
			os->SelectTool(ToolID::MaskBrush);
		else if (event.GetUnicodeKey() == '2' && os->menuBar->IsEnabled(XRCID("btnInflateBrush")))
			os->SelectTool(ToolID::InflateBrush);
		else if (event.GetUnicodeKey() == '3' && os->menuBar->IsEnabled(XRCID("btnDeflateBrush")))
			os->SelectTool(ToolID::DeflateBrush);
		else if (event.GetUnicodeKey() == '4' && os->menuBar->IsEnabled(XRCID("btnMoveBrush")))
			os->SelectTool(ToolID::MoveBrush);
		else if (event.GetUnicodeKey() == '5' && os->menuBar->IsEnabled(XRCID("btnSmoothBrush")))
			os->SelectTool(ToolID::SmoothBrush);
		else if (event.GetUnicodeKey() == '6' && os->menuBar->IsEnabled(XRCID("btnUndiffBrush")))
			os->SelectTool(ToolID::UndiffBrush);
		else if (event.GetUnicodeKey() == '7' && os->menuBar->IsEnabled(XRCID("btnWeightBrush")))
			os->SelectTool(ToolID::WeightBrush);
		else if (event.GetUnicodeKey() == '8' && os->menuBar->IsEnabled(XRCID("btnColorBrush")))
			os->SelectTool(ToolID::ColorBrush);
		else if (event.GetUnicodeKey() == '9' && os->menuBar->IsEnabled(XRCID("btnAlphaBrush")))
			os->SelectTool(ToolID::AlphaBrush);
		else if (event.GetKeyCode() == WXK_SPACE) {
			if (event.ControlDown()) {
				if (!os->activeSlider.empty()) {
					os->ExitSliderEdit();
				}
				else {
					os->EnterSliderEdit();
					os->ScrollToActiveSlider();
				}
			}
			else {
				if (os->brushSettingsPopupTransient && os->brushSettingsPopupTransient->IsShown()) {
					os->CloseBrushSettings();
				}
				else {
					bool brushSettingsNearCursor = Config.GetBoolValue("Input/BrushSettingsNearCursor");
					if (brushSettingsNearCursor)
						os->PopupBrushSettings();
					else
						os->PopupBrushSettings(os->brushSettings);
				}
			}
		}
		else if (event.GetKeyCode() == WXK_ESCAPE) {
			if (isMovingVertex) {
				CancelMoveVertex();
				isMovingVertex = false;
			}
			if (isSlidingEdge) {
				CancelEdgeSlide();
				isSlidingEdge = false;
			}
			os->CloseBrushSettings();
		}
	}

	event.Skip();
}

bool wxGLPanel::StartBrushStroke(const wxPoint& screenPos) {
	// Check if brush strokes are currently allowed
	if (activeBrush == &weightBrush) {
		std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;
	}

	TweakPickInfo tpi;
	Mesh* hitMesh = nullptr;
	int hitTri = -1;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
	if (!hit || !hitMesh)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
	tpi.normal.Normalize();
	tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);

	Vector3 v;
	Vector3 vo;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

	v = v * -1.0f;
	tpi.view = v;

	savedBrush = activeBrush;

	if (wxGetKeyState(WXK_CONTROL)) {
		if (wxGetKeyState(WXK_ALT) && !segmentMode) {
			UnMaskBrush.setStrength(-maskBrush.getStrength());
			UnMaskBrush.setFocus(maskBrush.getFocus());
			UnMaskBrush.setSpacing(maskBrush.getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else {
			activeBrush = &maskBrush;
		}
	}
	else if (activeBrush == &weightBrush) {
		std::vector<std::string> normBones, notNormBones, brushBones, lockedBones;
		os->GetNormalizeBones(&normBones, &notNormBones);

		std::string activeBone = os->GetActiveBone();
		std::string xMirrorBone = os->GetXMirrorBone();
		brushBones.push_back(activeBone);

		if (!xMirrorBone.empty())
			brushBones.push_back(xMirrorBone);

		bool bHasNormBones = false;
		for (auto& bone : normBones) {
			if (bone != activeBone && bone != xMirrorBone) {
				brushBones.push_back(bone);
				bHasNormBones = true;
			}
		}

		if (bHasNormBones) {
			for (auto& bone : notNormBones)
				if (bone != activeBone && bone != xMirrorBone)
					lockedBones.push_back(bone);
		}
		else {
			for (auto& bone : notNormBones)
				if (bone != activeBone && bone != xMirrorBone)
					brushBones.push_back(bone);
		}

		if (wxGetKeyState(WXK_ALT)) {
			unweightBrush.animInfo = os->project->GetWorkAnim();
			unweightBrush.boneNames = brushBones;
			unweightBrush.lockedBoneNames = lockedBones;
			unweightBrush.bSpreadWeight = bHasNormBones;
			unweightBrush.bXMirrorBone = !xMirrorBone.empty();
			unweightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			unweightBrush.setStrength(-weightBrush.getStrength());
			unweightBrush.setFocus(weightBrush.getFocus());
			unweightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &unweightBrush;
		}
		else if (wxGetKeyState(WXK_SHIFT)) {
			smoothWeightBrush.animInfo = os->project->GetWorkAnim();
			smoothWeightBrush.boneNames = brushBones;
			smoothWeightBrush.lockedBoneNames = lockedBones;
			smoothWeightBrush.bSpreadWeight = bHasNormBones;
			smoothWeightBrush.bXMirrorBone = !xMirrorBone.empty();
			smoothWeightBrush.bNormalizeWeights = weightBrush.bNormalizeWeights;
			smoothWeightBrush.setStrength(weightBrush.getStrength() * 15.0f);
			smoothWeightBrush.setFocus(weightBrush.getFocus());
			smoothWeightBrush.setSpacing(weightBrush.getSpacing());
			activeBrush = &smoothWeightBrush;
		}
		else {
			weightBrush.animInfo = os->project->GetWorkAnim();
			weightBrush.boneNames = brushBones;
			weightBrush.lockedBoneNames = lockedBones;
			weightBrush.bSpreadWeight = bHasNormBones;
			weightBrush.bXMirrorBone = !xMirrorBone.empty();
		}
	}
	else if (wxGetKeyState(WXK_ALT) && !segmentMode) {
		if (activeBrush == &inflateBrush) {
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			activeBrush = &inflateBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-activeBrush->getStrength());
			UnMaskBrush.setFocus(activeBrush->getFocus());
			UnMaskBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &UnMaskBrush;
		}
		else if (activeBrush == &colorBrush) {
			uncolorBrush.setStrength(-activeBrush->getStrength());
			uncolorBrush.setFocus(activeBrush->getFocus());
			uncolorBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &uncolorBrush;
		}
		else if (activeBrush == &alphaBrush) {
			unalphaBrush.setStrength(-activeBrush->getStrength());
			unalphaBrush.setFocus(activeBrush->getFocus());
			unalphaBrush.setSpacing(activeBrush->getSpacing());
			activeBrush = &unalphaBrush;
		}
	}
	else if (activeBrush == &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothMaskBrush.setStrength(activeBrush->getStrength());
		smoothMaskBrush.setFocus(activeBrush->getFocus());
		smoothMaskBrush.setSpacing(activeBrush->getSpacing());
		activeBrush = &smoothMaskBrush;
	}
	else if (activeBrush != &weightBrush && activeBrush != &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		activeBrush = &smoothBrush;
	}

	activeBrush->setRadius(brushSize);
	activeBrush->setMirror(GetToolOptionXMirror());
	activeBrush->setConnected(toolOptionConnectedOnly);
	activeBrush->setRestrictPlane(toolOptionRestrictPlane);
	activeBrush->setRestrictNormal(toolOptionRestrictNormal);

	if (activeBrush->Type() == TweakBrush::BrushType::Weight) {
		for (auto& sel : os->GetSelectedItems()) {
			os->project->CreateSkinning(sel->GetShape());

			int boneIndex = os->project->GetWorkAnim()->GetShapeBoneIndex(sel->GetShape()->name.get(), os->GetActiveBone());
			if (boneIndex < 0)
				os->project->AddBoneRef(os->GetActiveBone());
		}
	}

	activeStroke = std::make_unique<TweakStroke>(gls.GetActiveMeshes(), activeBrush, *undoHistory.PushState());

	if (os->bEditSlider) {
		activeStroke->usp.sliderName = os->activeSlider;
		float sliderscale = os->project->SliderValue(os->activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		activeStroke->usp.sliderscale = sliderscale;
	}

	if (activeBrush->Type() == TweakBrush::BrushType::Undiff) {
		std::vector<Mesh*> refMeshes = activeStroke->GetRefMeshes();

		std::vector<std::vector<Vector3>> positionData;
		positionData.resize(refMeshes.size());

		for (size_t i = 0; i < refMeshes.size(); i++) {
			// Get base vertex positions, not current mesh position
			Mesh* m = refMeshes[i];
			std::vector<Vector3> basePosition;

			auto workNif = os->project->GetWorkNif();
			auto shape = workNif->FindBlockByName<NiShape>(m->shapeName);
			if (shape)
				workNif->GetVertsForShape(shape, basePosition);

			for (auto& p : basePosition)
				p = Mesh::TransformPosNifToMesh(p);

			positionData[i] = std::move(basePosition);
		}

		activeStroke->beginStroke(tpi, positionData);
	}
	else
		activeStroke->beginStroke(tpi);

	if (activeBrush->Type() != TweakBrush::BrushType::Move) {
		if (segmentMode) {
			bool changed = os->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->ShowSegment();
				os->ShowPartition();
			}
		}
		else {
			activeStroke->updateStroke(tpi);
		}
	}

	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	TweakPickInfo tpi;

	if (activeStroke) {
		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true);
		Mesh* hitMesh = nullptr;
		int hitTri = -1;

		if (activeBrush->Type() == TweakBrush::BrushType::Move) {
			Vector3 pn;
			float pd;
			((TB_Move*)activeBrush)->GetWorkingPlane(pn, pd);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		}
		else {
			if (!hit)
				return;

			hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, &hitMesh, true, &hitTri);
			if (!hit || !hitMesh)
				return;

			tpi.origin = hitMesh->TransformPosMeshToModel(tpi.origin);
			tpi.normal.Normalize();
			tpi.normal = hitMesh->TransformDirMeshToModel(tpi.normal);
		}

		Vector3 v;
		Vector3 vo;
		gls.GetPickRay(screenPos.x, screenPos.y, nullptr, v, vo);

		v = v * -1.0f;
		tpi.view = v;
		if (segmentMode) {
			bool changed = os->PaintSegmentPartitionTriangles(hitMesh, hitTri, tpi.origin, activeBrush->getRadius());

			if (GetToolOptionXMirror()) {
				Vector3 mirrorHitOrigin;
				Vector3 mirrorHitNormal;
				Mesh* mirrorHitMesh = nullptr;
				int mirrorHitTri = -1;
				if (gls.CollideMeshes(screenPos.x, screenPos.y, mirrorHitOrigin, mirrorHitNormal, true, &mirrorHitMesh, true, &mirrorHitTri) && mirrorHitMesh) {
					mirrorHitOrigin = mirrorHitMesh->TransformPosMeshToModel(mirrorHitOrigin);
					changed = os->PaintSegmentPartitionTriangles(mirrorHitMesh, mirrorHitTri, mirrorHitOrigin, activeBrush->getRadius()) || changed;
				}
			}

			if (changed) {
				os->ShowSegment();
				os->ShowPartition();
			}
		}
		else {
			activeStroke->updateStroke(tpi);
		}

		if (activeBrush->Type() == TweakBrush::BrushType::Weight) {
			std::string selectedBone = os->GetActiveBone();
			if (!selectedBone.empty()) {
				os->ActiveShapesUpdated(undoHistory.GetCurState(), false);
			}
		}

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment();
			os->ShowPartition();
		}
	}
}

void wxGLPanel::EndBrushStroke() {
	if (activeStroke) {
		activeStroke->endStroke();

		TweakBrush::BrushType brushType = activeStroke->BrushType();
		if (brushType != TweakBrush::BrushType::Mask) {
			os->ActiveShapesUpdated(undoHistory.GetCurState());

			if (brushType == TweakBrush::BrushType::Weight) {
				std::string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					os->HighlightBoneNamesWithWeights();
					os->UpdateBoneCounts();
				}
			}

			if (!os->bEditSlider && brushType != TweakBrush::BrushType::Weight && brushType != TweakBrush::BrushType::Color && brushType != TweakBrush::BrushType::Alpha) {
				{
					UndoStateProject* usp = undoHistory.GetCurState();
					for (auto& uss : usp->usss) {
						auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
						if (shape)
							os->project->ComputeUndoRestDiffs(shape, uss);
					}
				}

				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
					os->UpdateShapeSource(s);
					os->project->RefreshMorphShape(s);
				}

				if (os->project->bPose) {
					for (auto& s : os->project->GetWorkNif()->GetShapes()) {
						std::vector<Vector3> verts;
						os->project->GetLiveVerts(s, verts);
						UpdateMeshVertices(s->name.get(), &verts, true, true, false);
					}
				}
			}
			else if (os->bEditSlider && os->project->bPose && brushType != TweakBrush::BrushType::Weight && brushType != TweakBrush::BrushType::Color && brushType != TweakBrush::BrushType::Alpha) {
				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(s, verts);
					UpdateMeshVertices(s->name.get(), &verts, true, true, false);
				}
			}
		}

		if (brushType == TweakBrush::BrushType::Mask) {
			if (!Config.GetBoolValue("Input/MaskHistory"))
				undoHistory.PopState();
		}

		activeStroke = nullptr;
		activeBrush = savedBrush;

		if (transformMode)
			ShowTransformTool();

		if (segmentMode) {
			os->ShowSegment();
			os->ShowPartition();

			if (os->currentTabButton)
				os->currentTabButton->SetPendingChanges();
		}

		os->UpdateUndoTools();
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
	if (!hit)
		return false;

	if (!os->CheckEditableState())
		return false;

	tpi.center = xformCenter;
	xformCenterInitial = xformCenter;

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
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
				//tpi.view = Vector3(0.0f, 1.0f, 0.0f);
				tpi.normal = Vector3(1.0f, 0.0f, 0.0f);
				break;
			case 'Y':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
				//tpi.view = Vector3(-1.0f, 0.0f, 0.0f);
				tpi.normal = Vector3(0.0f, 1.0f, 0.0f);
				break;
			case 'Z':
				gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
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
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(1.0f, 0.0f, 0.0f), tpi.center.x);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 1.0f, 0.0f), tpi.center.y);
			gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, Vector3(0.0f, 0.0f, 1.0f), tpi.center.z);
			tpi.normal = Vector3(0.0f, 0.0f, 1.0f);
		}
	}
	else
		return false;

	if (!nodesMode && !bonesMode) {
		activeStroke = std::make_unique<TweakStroke>(gls.GetActiveMeshes(), &translateBrush, *undoHistory.PushState());

		if (os->bEditSlider) {
			activeStroke->usp.sliderName = os->activeSlider;

			float sliderscale = os->project->SliderValue(os->activeSlider);
			if (sliderscale == 0.0)
				sliderscale = 1.0;

			activeStroke->usp.sliderscale = sliderscale;
		}

		activeStroke->beginStroke(tpi);
	}
	else {
		// Bones/nodes-mode transform: edit the active bone/node directly.
		// The TweakStroke machinery isn't used here because we don't need
		// to modify any mesh vertices through a brush -- we change the
		// bone's local transform and let skinning update the meshes.
		const std::string activeBone = os->GetActiveBone();
		if (activeBone.empty())
			return false;

		// Only translate and rotate are supported for bones/nodes.
		if (mname.find("Move") != std::string::npos)
			boneXformType = 0;
		else if (mname.find("Rotate") != std::string::npos)
			boneXformType = 1;
		else
			return false;

		boneXformPickStart = tpi.origin;
		boneXformPlaneNormalModel = tpi.normal;
		boneXformAxisModel = tpi.view;
		boneXformPlaneDist = tpi.origin.dot(tpi.normal);

		// Pose mode: edit poseTranVec/poseRotVec only (no NIF writes, no
		// undo).  Only AnimBones have pose data; plain NIF nodes do not.
		boneXformPoseMode = false;
		AnimBone* poseBPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (os->project->bPose && poseBPtr) {
			boneXformPoseMode = true;
			xformInitialLocalToParent = poseBPtr->xformToParent;
			// M = parent_pose_to_global ∘ xformToParent
			MatTransform parentPoseToGlobal;
			if (poseBPtr->parent)
				parentPoseToGlobal = poseBPtr->parent->xformPoseToGlobal;
			xformInitialParentToGlobal = parentPoseToGlobal.ComposeTransforms(poseBPtr->xformToParent);
			boneXformPoseInitialTran = poseBPtr->poseTranVec;
			boneXformPoseInitialRot = poseBPtr->poseRotVec;
		}
		else {
			// Capture the initial transforms of the active bone/node.
			xformInitialLocalToParent = MatTransform();
			xformInitialParentToGlobal = MatTransform();

			const AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
			if (bPtr) {
				xformInitialLocalToParent = bPtr->xformToParent;
				if (bPtr->parent)
					xformInitialParentToGlobal = bPtr->parent->xformToGlobal;
			}
			else if (auto* nif = os->project->GetWorkNif()) {
				if (NiNode* node = nif->FindBlockByName<NiNode>(activeBone)) {
					xformInitialLocalToParent = node->GetTransformToParent();
					// Compose the transforms of all ancestor nodes to build
					// parent-to-global.
					NiNode* p = nif->GetParentNode(node);
					while (p) {
						xformInitialParentToGlobal = p->GetTransformToParent().ComposeTransforms(xformInitialParentToGlobal);
						p = nif->GetParentNode(p);
					}
				}
				else {
					// No AnimBone and no NiNode -- nothing to edit.
					return false;
				}
			}
			else
				return false;

			// Push an undo state.  nodeEndXformToParent is filled in in
			// EndTransform; until then it equals nodeStartXformToParent.
			UndoStateProject* usp = undoHistory.PushState();
			usp->undoType = UndoType::NodeTransform;
			usp->boneName = activeBone;
			usp->nodeStartXformToParent = xformInitialLocalToParent;
			usp->nodeEndXformToParent = xformInitialLocalToParent;
		}
	}

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

	if (!nodesMode && !bonesMode) {
		translateBrush.GetWorkingPlane(pn, pd);
		gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);
		activeStroke->updateStroke(tpi);
		ShowTransformTool();
		return;
	}

	// Bones/nodes-mode transform: compute the bone/node's new local transform
	// directly from the drag and apply it.  Meshes update live via skinning.
	const std::string activeBone = os->GetActiveBone();
	if (activeBone.empty()) {
		ShowTransformTool();
		return;
	}

	// Intersect the mouse ray with the pick plane captured in StartTransform.
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, boneXformPlaneNormalModel, boneXformPlaneDist);

	if (boneXformPoseMode) {
		AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
		if (!bPtr) {
			ShowTransformTool();
			return;
		}

		// M = parent_pose_to_global ∘ xformToParent  (captured in
		// xformInitialParentToGlobal).  Edits go into poseTranVec/poseRotVec
		// which are applied in the bone's own CS before xformToParent.
		if (boneXformType == 0) {
			const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
			const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
			const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
			const Vector3 localOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
			bPtr->poseTranVec = boneXformPoseInitialTran + localOffset;
		}
		else if (boneXformType == 1) {
			const Vector3 a = boneXformPickStart - xformCenterInitial;
			const Vector3 b = tpi.origin - xformCenterInitial;
			if (a.length() > EPSILON && b.length() > EPSILON) {
				const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
				const float cosA = a.dot(b);
				const float angle = std::atan2(sinA, cosA);

				Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
				Vector3 axisLocal = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
				const float axisLen = axisLocal.length();
				if (axisLen > EPSILON) {
					axisLocal /= axisLen;
					const Matrix3 rot = RotVecToMat(axisLocal * angle);
					const Matrix3 newPoseRot = rot * RotVecToMat(boneXformPoseInitialRot);
					bPtr->poseRotVec = RotMatToVec(newPoseRot);
				}
			}
		}

		bPtr->UpdatePoseTransform();

		// Live mesh update to show the pose change immediately.
		if (auto* nif = os->project->GetWorkNif()) {
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		// Keep the pose-sliders GUI in sync with the new values.
		os->PoseToGUI();

		UpdateBones();
		UpdateNodes();
		ShowTransformTool();
		return;
	}

	MatTransform newToParent = xformInitialLocalToParent;

	if (boneXformType == 0) {
		// Translate along the gizmo's axis.  Project the drag vector onto
		// the axis in model space, convert to nif distance, then map the
		// resulting global offset into parent space (so scale/rotation of
		// ancestor nodes is handled correctly).
		const float t = (tpi.origin - boneXformPickStart).dot(boneXformAxisModel);
		const Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformAxisModel);
		const Vector3 globalOffset = axisGlobal * Mesh::TransformDistMeshToNif(t);
		const Vector3 parentOffset = xformInitialParentToGlobal.InverseTransform().ApplyTransformToDiff(globalOffset);
		newToParent.translation = xformInitialLocalToParent.translation + parentOffset;
	}
	else if (boneXformType == 1) {
		// Rotate about the gizmo ring's axis.  Angle is the signed angle
		// between the initial and current pick vectors in the rotation plane.
		const Vector3 a = boneXformPickStart - xformCenterInitial;
		const Vector3 b = tpi.origin - xformCenterInitial;
		if (a.length() > EPSILON && b.length() > EPSILON) {
			const float sinA = a.cross(b).dot(boneXformPlaneNormalModel);
			const float cosA = a.dot(b);
			const float angle = std::atan2(sinA, cosA);

			// Convert axis from model to global, then to parent.
			Vector3 axisGlobal = Mesh::TransformDirMeshToNif(boneXformPlaneNormalModel);
			Vector3 axisParent = xformInitialParentToGlobal.rotation.Transpose() * axisGlobal;
			const float axisLen = axisParent.length();
			if (axisLen > EPSILON) {
				axisParent /= axisLen;
				const Matrix3 rot = RotVecToMat(axisParent * angle);
				newToParent.rotation = rot * xformInitialLocalToParent.rotation;
			}
		}
	}

	// Apply the new transform to the AnimBone (if any) and to the NIF node.
	// We deliberately do not recalculate xformSkinToBone: that way the mesh
	// follows the bone through skinning, the active pose updates live, and
	// the "check for bad bones" residual correctly flags the new mismatch.
	auto* nif = os->project->GetWorkNif();
	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(activeBone);
	if (bPtr)
		bPtr->SetTransformBoneToParent(newToParent);
	if (nif)
		nif->SetNodeTransformToParent(activeBone, newToParent);

	// Record the end state on the open undo entry so an in-progress drag is
	// already undo-able if the stroke is canceled.
	if (UndoStateProject* usp = undoHistory.GetCurState()) {
		if (usp->undoType == UndoType::NodeTransform && usp->boneName == activeBone)
			usp->nodeEndXformToParent = newToParent;
	}

	// Live mesh update (so skinned vertices follow the bone).
	if (nif) {
		for (auto& s : nif->GetShapes()) {
			std::vector<Vector3> verts;
			os->project->GetLiveVerts(s, verts);
			UpdateMeshVertices(s->name.get(), &verts, true, true, false);
		}
	}

	UpdateBones();
	UpdateNodes();
	ShowTransformTool();
}

void wxGLPanel::EndTransform() {
	if (!nodesMode && !bonesMode) {
		activeStroke->endStroke();
		activeStroke = nullptr;

		os->ActiveShapesUpdated(undoHistory.GetCurState());
		if (!os->bEditSlider) {
			{
				UndoStateProject* usp = undoHistory.GetCurState();
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						os->project->ComputeUndoRestDiffs(shape, uss);
				}
			}

			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				os->UpdateShapeSource(s);
				os->project->RefreshMorphShape(s);
			}

			if (os->project->bPose) {
				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
					std::vector<Vector3> verts;
					os->project->GetLiveVerts(s, verts);
					UpdateMeshVertices(s->name.get(), &verts, true, true, false);
				}
			}
		}
		else if (os->project->bPose) {
			for (auto& s : os->project->GetWorkNif()->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}
	}
	else {
		// Bones/nodes-mode: sync the AnimInfo's bone transforms out to the
		// NIF, refresh the bone-tree bad-bone icons, and mark the project
		// as modified.  In pose mode we only edited poseTranVec/poseRotVec
		// on the AnimBone -- no NIF writes, no undo, no pending changes.
		if (!boneXformPoseMode) {
			if (auto* nif = os->project->GetWorkNif()) {
				if (auto* workAnim = os->project->GetWorkAnim())
					workAnim->WriteNodesToNif(nif);
			}
			os->RefreshBoneTreeBadBoneIcons();
			os->SetPendingChanges();
		}
		boneXformPoseMode = false;
	}

	ShowTransformTool();
	os->UpdateUndoTools();
}

bool wxGLPanel::StartPivotPosition(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	Mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh);
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

	std::vector<Mesh*> strokeMeshes{hitMesh};
	activeStroke = std::make_unique<TweakStroke>(strokeMeshes, &translateBrush, *undoHistory.PushState());
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
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);

	activeStroke->updateStroke(tpi);
}

void wxGLPanel::EndPivotPosition() {
	activeStroke->endStroke();

	std::vector<Mesh*> refMeshes = activeStroke->GetRefMeshes();
	if (refMeshes.size() > 0) {
		UndoStateShape& uss = activeStroke->usp.usss[0];
		if (uss.pointStartState.size() > 0 && uss.pointEndState.size() > 0) {
			Vector3& pivotStartStatePos = uss.pointStartState[0];
			Vector3& pivotEndStatePos = uss.pointEndState[0];
			Vector3 pivotDiff = pivotEndStatePos - pivotStartStatePos;
			pivotPosition += pivotDiff;
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
		Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
		if (m) {
			double newval = m->mask[vertIndex];
			if (wxGetKeyState(WXK_CONTROL))
				newval = 1.0f;
			else if (!segmentMode)
				newval = 0.0f;

			if (m->mask[vertIndex] != newval) {
				UndoStateProject* usp = GetUndoHistory()->PushState();
				usp->undoType = UndoType::Mask;
				usp->usss.emplace_back();
				UndoStateShape& uss = usp->usss.back();
				uss.shapeName = m->shapeName;
				uss.pointStartState[vertIndex].x = m->mask[vertIndex];
				uss.pointEndState[vertIndex].x = newval;
				ApplyUndoState(usp, false);

				if (!Config.GetBoolValue("Input/MaskHistory"))
					GetUndoHistory()->PopState();

				os->UpdateUndoTools();
			}
		}
	}

	if (transformMode)
		ShowTransformTool();

	return true;
}

bool wxGLPanel::StartPickVertex() {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	return true;
}

void wxGLPanel::UpdatePickVertex(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || hitResult.hoverPoint != mouseDownPoint)
		gls.HidePointCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickVertex() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || lastHitResult.hoverPoint != mouseDownPoint)
		return;

	// Clear PickVertex state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HidePointCursor();

	if (activeTool == ToolID::CollapseVertex)
		ClickCollapseVertex();
}

void wxGLPanel::ClickCollapseVertex() {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Make list of this vertex and its welded vertices.
	std::vector<uint16_t> verts;
	m->GetWeldSet(mouseDownPoint, verts);

	std::sort(verts.begin(), verts.end());

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareCollapseVertex(shape, uss, verts)) {
		wxMessageBox(_("The vertex picked has more than three connections."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

struct EdgeSlideCandidate {
	Edge edge;
	int targetPoint = -1;
	float t = 0.0f;
	float distanceSquared = 0.0f;
};

static float ClampEdgeSlideT(float t) {
	if (t < 0.0f)
		return 0.0f;

	if (t > 1.0f)
		return 1.0f;

	return t;
}

static bool FindEdgeSlideCandidate(GLSurface& gls, Mesh* m, int slidePoint, const Vector3& startPosition, const wxPoint& screenPos, EdgeSlideCandidate& outCandidate) {
	if (!m || slidePoint < 0 || slidePoint >= m->nVerts)
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges)
		return false;

	int startX = 0;
	int startY = 0;
	gls.ProjectPointToScreen(m->TransformPosMeshToModel(startPosition), startX, startY);

	bool foundCandidate = false;
	std::unordered_set<int> seenTargets;
	for (int edgeIndex : m->vertEdges[slidePoint]) {
		if (edgeIndex < 0 || edgeIndex >= m->nEdges)
			continue;

		const Edge& edge = m->edges[edgeIndex];
		int targetPoint = -1;
		if (edge.p1 == slidePoint)
			targetPoint = edge.p2;
		else if (edge.p2 == slidePoint)
			targetPoint = edge.p1;
		else
			continue;

		if (targetPoint < 0 || targetPoint >= m->nVerts || !seenTargets.insert(targetPoint).second)
			continue;

		int endX = 0;
		int endY = 0;
		gls.ProjectPointToScreen(m->TransformPosMeshToModel(m->verts[targetPoint]), endX, endY);

		float dx = static_cast<float>(endX - startX);
		float dy = static_cast<float>(endY - startY);
		float edgeLengthSquared = dx * dx + dy * dy;
		if (edgeLengthSquared <= 0.0001f)
			continue;

		float cursorDx = static_cast<float>(screenPos.x - startX);
		float cursorDy = static_cast<float>(screenPos.y - startY);
		float t = ClampEdgeSlideT((cursorDx * dx + cursorDy * dy) / edgeLengthSquared);
		float closestX = static_cast<float>(startX) + dx * t;
		float closestY = static_cast<float>(startY) + dy * t;
		float distX = static_cast<float>(screenPos.x) - closestX;
		float distY = static_cast<float>(screenPos.y) - closestY;
		float distanceSquared = distX * distX + distY * distY;

		if (!foundCandidate || distanceSquared < outCandidate.distanceSquared) {
			outCandidate.edge = edge;
			outCandidate.targetPoint = targetPoint;
			outCandidate.t = t;
			outCandidate.distanceSquared = distanceSquared;
			foundCandidate = true;
		}
	}

	return foundCandidate;
}

bool wxGLPanel::StartEdgeSlide(const wxPoint& WXUNUSED(screenPos)) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	if (!m->vertEdges || !m->edges)
		m->BuildEdgeList();

	if (!m->vertEdges || !m->edges || m->vertEdges[lastHitResult.hoverPoint].empty())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	edgeSlideStartPosition = lastHitResult.hoverMeshCoord;
	edgeSlideStartUV = Vector2();
	edgeSlideCurrentEdge = Edge();
	edgeSlideTarget = -1;
	edgeSlideHasUV = !os->bEditSlider && GetToolOptionEdgeSlideUVCorrection() && m->texcoord != nullptr;
	if (edgeSlideHasUV)
		edgeSlideStartUV = m->texcoord[mouseDownPoint];

	UndoStateProject* usp = undoHistory.PushState();
	usp->undoType = UndoType::VertexPosition;
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = edgeSlideStartPosition;
	if (edgeSlideHasUV)
		usp->usss[0].uvEndState[mouseDownPoint] = usp->usss[0].uvStartState[mouseDownPoint] = edgeSlideStartUV;

	if (os->bEditSlider) {
		usp->sliderName = os->activeSlider;
		float sliderscale = os->project->SliderValue(os->activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateEdgeSlide(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];

	EdgeSlideCandidate candidate;
	if (!FindEdgeSlideCandidate(gls, m, mouseDownPoint, edgeSlideStartPosition, screenPos, candidate)) {
		gls.HideSegCursor();
		return;
	}

	edgeSlideCurrentEdge = candidate.edge;
	edgeSlideTarget = candidate.targetPoint;

	Vector3 targetPosition = m->verts[edgeSlideTarget];
	Vector3 newPosition = edgeSlideStartPosition + (targetPosition - edgeSlideStartPosition) * candidate.t;
	uss.pointEndState[mouseDownPoint] = newPosition;
	m->verts[mouseDownPoint] = newPosition;
	m->QueueUpdate(Mesh::UpdateType::Position);

	if (edgeSlideHasUV && m->texcoord) {
		Vector2 targetUV = m->texcoord[edgeSlideTarget];
		Vector2 newUV = edgeSlideStartUV + (targetUV - edgeSlideStartUV) * candidate.t;
		uss.uvEndState[mouseDownPoint] = newUV;
		m->texcoord[mouseDownPoint] = newUV;
		m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
	}

	Vector3 newPositionModel = m->TransformPosMeshToModel(newPosition);
	gls.SetPointCursor(newPositionModel);
	gls.SetCenterCursor(newPositionModel);
	gls.AddVisSeg(m->TransformPosMeshToModel(edgeSlideStartPosition), m->TransformPosMeshToModel(targetPosition), "seghilite");
	gls.ShowCursor(true);
}

void wxGLPanel::EndEdgeSlide() {
	isSlidingEdge = false;
	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;

	UndoStateShape& uss = usp->usss[0];
	bool positionChanged = uss.pointEndState[mouseDownPoint] != uss.pointStartState[mouseDownPoint];
	bool uvChanged = false;
	auto uvStart = uss.uvStartState.find(mouseDownPoint);
	auto uvEnd = uss.uvEndState.find(mouseDownPoint);
	if (uvStart != uss.uvStartState.end() && uvEnd != uss.uvEndState.end())
		uvChanged = uvStart->second != uvEnd->second;

	if (!positionChanged && !uvChanged) {
		CancelEdgeSlide();
		return;
	}

	if (!os->bEditSlider) {
		NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		if (shape)
			os->project->ComputeUndoRestDiffs(shape, uss);
	}

	ApplyUndoState(usp, false);
	gls.HideSegCursor();

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelEdgeSlide() {
	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	undoHistory.PopState();
	gls.HideSegCursor();
}

bool wxGLPanel::StartPhysicsGrab(const wxPoint& screenPos) {
	const auto& affectedShapes = os->GetPhysicsAffectedShapes();
	if (affectedShapes.empty())
		return false;

	// The pump skips the BVH update every frame, so the trees still describe
	// the pose the simulation started from. Fit them to where the mesh is now,
	// or the grab misses the cloth that has swung away since.
	os->RefitPhysicsBVH();

	// Any shape the simulation moves can be grabbed, whether or not it is one
	// of the shapes picked for editing: what is being grabbed is the
	// simulation, not the project.
	Vector3 viewDir;
	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, viewDir, viewOrigin);

	Mesh* hitMesh = nullptr;
	Vector3 hitMeshPos;
	float hitDistance = std::numeric_limits<float>::max();

	for (auto* m : gls.GetMeshes()) {
		if (!m->bVisible || !m->bvh || affectedShapes.count(m->shapeName) == 0)
			continue;

		Vector3 rayOrigin = m->TransformPosModelToMesh(viewOrigin);
		Vector3 rayDir = m->TransformDirModelToMesh(viewDir);

		std::vector<IntersectResult> results;
		if (!m->bvh->IntersectRay(rayOrigin, rayDir, &results))
			continue;

		for (auto& result : results) {
			const float distance = result.HitCoord.DistanceTo(rayOrigin);
			if (distance >= hitDistance)
				continue;

			hitDistance = distance;
			hitMesh = m;
			hitMeshPos = result.HitCoord;
		}
	}

	if (!hitMesh)
		return false;

	std::vector<Physics::GrabTarget> targets;
	if (!CollectPhysicsGrabTargets(hitMesh, hitMeshPos, targets))
		return false;

	if (!os->BeginPhysicsGrab(targets))
		return false;

	// The simulation pulls the grabbed patch out from under the cursor right
	// away, so the drag reads against a plane through the grabbed spot facing
	// the camera rather than against the mesh - the move brush freezes a plane
	// for the same reason.
	physicsGrabStart = hitMesh->TransformPosMeshToModel(hitMeshPos);
	physicsGrabPlaneNormal = viewDir * -1.0f;
	physicsGrabPlaneDist = physicsGrabStart.dot(physicsGrabPlaneNormal);

	ShowPhysicsGrabMarker(physicsGrabStart);
	return true;
}

bool wxGLPanel::CollectPhysicsGrabTargets(Mesh* m, const Vector3& meshPos, std::vector<Physics::GrabTarget>& outTargets) {
	AnimInfo* anim = os->project->GetWorkAnim();
	auto skinIt = anim->shapeSkinning.find(m->shapeName);
	if (skinIt == anim->shapeSkinning.end())
		return false;

	// A patch the size of the brush is taken hold of rather than a single
	// vertex, so that a grab catches a piece of cloth instead of whichever
	// bone happens to skin the one vertex under the cursor.
	const float radius = m->TransformDistModelToMesh(brushSize);
	if (!(radius > 0.0f))
		return false;

	Vector3 patchCenter = meshPos;
	std::vector<IntersectResult> results;
	m->bvh->IntersectSphere(patchCenter, radius, &results);

	// Falloff towards the rim of the patch, so the pull stays centered on the
	// cursor instead of dragging the whole bone chain the patch reaches into
	std::unordered_map<uint16_t, float> patch;
	for (auto& result : results) {
		const Triangle& tri = m->tris[result.HitFacet];
		for (const uint16_t point : {tri.p1, tri.p2, tri.p3}) {
			if (patch.count(point) != 0)
				continue;

			const float distance = m->verts[point].DistanceTo(meshPos) / radius;
			if (distance >= 1.0f)
				continue;

			patch[point] = 1.0f - distance * distance * (3.0f - 2.0f * distance);
		}
	}

	if (patch.empty())
		return false;

	// Hand the patch over to the bones skinning it: each one gets the share of
	// it its weights hold, anchored at the center of that share. Anchoring per
	// bone rather than all of them at the cursor keeps the lever arms short,
	// so pulling the hem of a skirt bends it instead of spinning its bones.
	AnimSkin& skin = skinIt->second;
	float strongest = 0.0f;

	for (auto& boneName : skin.boneNames) {
		AnimWeight& boneWeight = skin.boneWeights[boneName.second];

		Vector3 anchor;
		float share = 0.0f;

		for (auto& patchPoint : patch) {
			auto weightIt = boneWeight.weights.find(patchPoint.first);
			if (weightIt == boneWeight.weights.end())
				continue;

			const float weight = patchPoint.second * weightIt->second;
			if (!(weight > 0.0f))
				continue;

			// The simulation works in NIF global space, model space is that
			// same space in render axes and units
			anchor += Mesh::TransformPosMeshToNif(m->TransformPosMeshToModel(m->verts[patchPoint.first])) * weight;
			share += weight;
		}

		if (!(share > 0.0f))
			continue;

		outTargets.push_back({boneName.first, anchor / share, share});
		strongest = std::max(strongest, share);
	}

	if (outTargets.empty())
		return false;

	// Shares are relative to the bone holding the most of the patch: a patch
	// spread thin over many bones must not pull harder than a compact one.
	// Bones barely touching it only add drag, so they are dropped.
	constexpr float minShare = 0.05f;
	for (auto& target : outTargets)
		target.weight /= strongest;

	outTargets.erase(std::remove_if(outTargets.begin(), outTargets.end(), [minShare](const Physics::GrabTarget& target) { return target.weight < minShare; }),
					 outTargets.end());

	return !outTargets.empty();
}

void wxGLPanel::UpdatePhysicsGrab(const wxPoint& screenPos) {
	Vector3 target;
	if (!gls.CollidePlane(screenPos.x, screenPos.y, target, physicsGrabPlaneNormal, physicsGrabPlaneDist))
		return;

	os->UpdatePhysicsGrab(Mesh::TransformDiffMeshToNif(target - physicsGrabStart));
	ShowPhysicsGrabMarker(target);
}

void wxGLPanel::EndPhysicsGrab() {
	isPhysicsGrabbing = false;
	HidePhysicsGrabMarker();
	os->EndPhysicsGrab();
}

void wxGLPanel::ShowPhysicsGrabMarker(const Vector3& modelPos) {
	// Marks where the cursor is pulling to, which is not where the mesh is:
	// how far the mesh trails behind the marker shows how hard its physics
	// setup is resisting the grab. Same circle the brush cursor uses, so the
	// radius it covers reads the same way.
	Mesh* marker = gls.AddVisCircle(modelPos, physicsGrabPlaneNormal, brushSize, "physicsgrabcircle");
	if (marker)
		marker->color = Vector3(1.0f, 0.75f, 0.2f);

	gls.AddVisPoint(modelPos, "physicsgrabcenter")->color = Vector3(1.0f, 0.75f, 0.2f);
}

void wxGLPanel::HidePhysicsGrabMarker() {
	gls.DeleteOverlay("physicsgrabcircle");
	gls.DeleteOverlay("physicsgrabcenter");
}

void wxGLPanel::ShowPhysicsProbe(const Vector3& nifPos, float nifRadius) {
	// A mesh rather than an overlay: overlays are drawn after the depth buffer is
	// cleared, so the ball would float in front of the cloth it is pressed into
	// instead of sinking behind it, and how deep it has gone is the whole point.
	// It carries bPrimitive, so nothing treats it as a shape of the project.
	// Model space is NIF global space in render axes and units.
	Mesh* m = gls.AddVis3dSphere(Mesh::TransformPosNifToMesh(nifPos), Mesh::TransformDistNifToMesh(nifRadius), Vector3(1.0f, 0.75f, 0.2f), "physicsprobeball", true);
	if (!m)
		return;

	// Shaded rather than flat tinted, which the other primitives are. A ball drawn in one
	// colour is a disc: nothing in it says where its near side is, and reading how far it
	// has been pushed into the cloth is the whole reason it is on screen. The sphere is
	// built with per-vertex normals and a tangent space already, so this is all it takes.
	m->rendermode = Mesh::RenderMode::LitSolid;
}

void wxGLPanel::HidePhysicsProbe() {
	gls.DeleteMesh("physicsprobeball");
}

bool wxGLPanel::StartMoveVertex(const wxPoint& screenPos) {
	if (lastHitResult.hitMeshName.empty() || lastHitResult.hoverPoint < 0)
		return false;

	Mesh* m = GetMesh(lastHitResult.hitMeshName);
	if (!m)
		return false;

	if (!os->CheckEditableState())
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownPoint = lastHitResult.hoverPoint;
	mouseHasMovedSinceStart = false;
	mouseDownPointNormal = m->TransformDirMeshToModel(m->norms[mouseDownPoint]);
	moveVertexOperation = MoveVertexOperation::None;

	Vector3 viewOrigin;
	gls.GetPickRay(screenPos.x, screenPos.y, nullptr, mouseDownViewDir, viewOrigin);
	mouseDownViewDir *= -1.0f;

	// snapDistance: shortest edge length of triangle under pointer
	const Triangle& tri = m->tris[lastHitResult.hoverTri];
	Vector3 gtp1 = m->TransformPosMeshToModel(m->verts[tri.p1]);
	Vector3 gtp2 = m->TransformPosMeshToModel(m->verts[tri.p2]);
	Vector3 gtp3 = m->TransformPosMeshToModel(m->verts[tri.p3]);
	snapDistance = gtp1.DistanceTo(gtp2);
	float elen = gtp2.DistanceTo(gtp3);
	if (snapDistance > elen)
		snapDistance = elen;
	elen = gtp3.DistanceTo(gtp1);
	if (snapDistance > elen)
		snapDistance = elen;

	UndoStateProject* usp = undoHistory.PushState();
	usp->usss.emplace_back();
	usp->usss[0].shapeName = mouseDownMeshName;
	usp->usss[0].pointEndState[mouseDownPoint] = usp->usss[0].pointStartState[mouseDownPoint] = lastHitResult.hoverMeshCoord;

	if (mouseDownMirrorPoint != -1)
		usp->usss[0].pointEndState[mouseDownMirrorPoint] = usp->usss[0].pointStartState[mouseDownMirrorPoint] = m->verts[mouseDownMirrorPoint];

	if (os->bEditSlider) {
		usp->sliderName = os->activeSlider;
		float sliderscale = os->project->SliderValue(os->activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		usp->sliderscale = sliderscale;
	}

	wxPoint p;
	gls.ProjectPointToScreen(lastHitResult.hoverRealCoord, p.x, p.y);
	mouseDownOffset = screenPos - p;

	gls.SetPointCursor(lastHitResult.hoverRealCoord);
	gls.SetCenterCursor(lastHitResult.hoverRealCoord);
	gls.ShowCursor(true);

	return true;
}

void wxGLPanel::UpdateMoveVertex(const wxPoint& screenPos) {
	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m || mouseDownPoint < 0)
		return;

	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;
	UndoStateShape& uss = usp->usss[0];

	// Restore original position in m so CollideMeshes and IntersectSphere
	// will work.
	Vector3 mesholdpos = uss.pointStartState[mouseDownPoint];
	m->verts[mouseDownPoint] = mesholdpos;
	if (mouseDownMirrorPoint != -1)
		m->verts[mouseDownMirrorPoint] = uss.pointStartState[mouseDownMirrorPoint];
	Vector3 oldpos = m->TransformPosMeshToModel(mesholdpos);

	// Since the pointer can be offset from screenPos, calculate a point
	// to display at screenPos.  We do this by intersecting the
	// screenPos ray with the plane perpendicular to mouseDownViewDir at oldpos.
	// This will also be our default for newpos if all tool options are off.
	Vector3 pointerPoint;
	gls.CollidePlane(screenPos.x, screenPos.y, pointerPoint, mouseDownViewDir, mouseDownViewDir.dot(oldpos));

	// Determining newpos: 1. intersect screenPos-ray with plane or surface.
	Vector3 newpos = pointerPoint;
	moveVertexOperation = MoveVertexOperation::Move;
	if (toolOptionRestrictSurface) {
		Vector3 hitpt, hitnormal;
		Mesh* hitmesh = nullptr;
		bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, hitpt, hitnormal, false, &hitmesh);
		if (!hit || !hitmesh) {
			newpos = oldpos;
			moveVertexOperation = MoveVertexOperation::None;
		}
		else
			newpos = hitmesh->TransformPosMeshToModel(hitpt);
	}
	else if (toolOptionRestrictPlane) {
		gls.CollidePlane(screenPos.x, screenPos.y, newpos, mouseDownPointNormal, mouseDownPointNormal.dot(oldpos));
	}

	// 2. Project onto normal
	if (toolOptionRestrictNormal && moveVertexOperation != MoveVertexOperation::None) {
		newpos = oldpos + mouseDownPointNormal * mouseDownPointNormal.dot(newpos - oldpos);
	}

	// 3. Snap to nearest valid point
	if ((toolOptionMerge || toolOptionWeld) && moveVertexOperation != MoveVertexOperation::None) {
		// Get adjacent points (not including welded points)
		std::unordered_set<int> adjPts;
		if (m->vertEdges && m->edges) {
			for (int ei : m->vertEdges[mouseDownPoint]) {
				const Edge& edge = m->edges[ei];
				if (edge.p1 != mouseDownPoint)
					adjPts.insert(edge.p1);
				if (edge.p2 != mouseDownPoint)
					adjPts.insert(edge.p2);
			}
		}

		int closestPoint = -1;
		Mesh* closestMesh = nullptr;
		float closestDist = snapDistance;

		for (Mesh* tm : gls.GetActiveMeshes()) {
			Vector3 viewDir, viewOrigin;
			gls.GetPickRay(screenPos.x, screenPos.y, tm, viewDir, viewOrigin);

			Vector3 tmnewpos = tm->TransformPosModelToMesh(newpos);
			float tmSnapDistance = tm->TransformDistModelToMesh(snapDistance);

			std::vector<IntersectResult> iresults;
			if (tm->bvh && tm->bvh->IntersectSphere(tmnewpos, tmSnapDistance, &iresults)) {
				for (const IntersectResult& ir : iresults) {
					const Triangle& t = tm->tris[ir.HitFacet];
					for (int tvi = 0; tvi < 3; ++tvi) {
						int tp = t[tvi];
						if (tm == m && tp == mouseDownPoint)
							continue;
						if (tm == m && adjPts.count(tp) != 0)
							continue;

						// Calculate distance in plane perpendicular to view
						Vector3 diff = tm->verts[tp] - tmnewpos;
						diff -= viewDir * viewDir.dot(diff);
						float d = tm->TransformDistMeshToModel(diff.length());
						if (d >= closestDist)
							continue;

						closestPoint = tp;
						closestMesh = tm;
						closestDist = d;
					}
				}
			}
		}

		if (closestPoint != -1) {
			NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
			NiShape* s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(closestMesh->shapeName);
			bool canWeld = toolOptionWeld && s1 && s2 && os->project->PointsHaveDifferingWeightsOrDiffs(s1, mouseDownPoint, s2, closestPoint);
			bool canMerge = toolOptionMerge && m == closestMesh;
			if (canWeld || canMerge) {
				newpos = closestMesh->TransformPosMeshToModel(closestMesh->verts[closestPoint]);
				if (canWeld)
					moveVertexOperation = MoveVertexOperation::Weld;
				else if (canMerge)
					moveVertexOperation = MoveVertexOperation::Merge;
				moveVertexTarget = closestPoint;
				moveVertexWeldTargetMeshName = closestMesh->shapeName;
			}
		}
	}

	Vector3 meshnewpos = m->TransformPosModelToMesh(newpos);
	uss.pointEndState[mouseDownPoint] = meshnewpos;
	m->verts[mouseDownPoint] = meshnewpos;
	gls.SetPointCursor(newpos);
	gls.SetCenterCursor(pointerPoint);
	m->QueueUpdate(Mesh::UpdateType::Position);
	gls.ShowCursor(true);

	if (mouseDownMirrorPoint != -1) {
		Vector3 mp = newpos;
		mp.x = -mp.x;
		mp = m->TransformPosModelToMesh(mp);
		uss.pointEndState[mouseDownMirrorPoint] = mp;
		m->verts[mouseDownMirrorPoint] = mp;
		gls.ShowMirrorPointCursor(mp, m);
	}
}

void wxGLPanel::EndMoveVertex() {
	isMovingVertex = false;
	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;

	if (moveVertexOperation == MoveVertexOperation::None) {
		CancelMoveVertex();
		return;
	}

	bool isWeld = moveVertexOperation == MoveVertexOperation::Weld;
	bool isMerge = moveVertexOperation == MoveVertexOperation::Merge;

	if (isWeld || isMerge) {
		NiShape* s1 = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
		NiShape* s2 = s1;
		if (isWeld)
			s2 = os->project->GetWorkNif()->FindBlockByName<NiShape>(moveVertexWeldTargetMeshName);

		bool p1b = os->project->IsVertexOnBoundary(s1, mouseDownPoint);
		bool p2b = os->project->IsVertexOnBoundary(s2, moveVertexTarget);

		if (!p1b || !p2b) {
			int response = wxMessageBox(
				!p1b ? !p2b ? _("Neither the selected nor target vertices are on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
							: _("The selected vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?")
					 : _("The target vertex is not on the mesh boundary.  It is recommended that you only weld or merge boundary vertices.  Continue?"),
				_("Weld/Merge Non-Boundary Vertices"),
				wxOK | wxCANCEL | wxCANCEL_DEFAULT,
				os);

			if (response == wxCANCEL) {
				CancelMoveVertex();
				return;
			}
		}

		usp->undoType = UndoType::Mesh;
		usp->sliderName.clear();
		usp->usss[0].pointStartState.clear();
		usp->usss[0].pointEndState.clear();

		if (isWeld)
			os->project->PrepareWeldVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget, s2);

		if (isMerge)
			os->project->PrepareMergeVertex(s1, usp->usss[0], mouseDownPoint, moveVertexTarget);
	}

	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::CancelMoveVertex() {
	UndoStateProject* usp = undoHistory.GetCurState();
	if (!usp)
		return;

	ApplyUndoState(usp, true);
	undoHistory.PopState();
}

bool wxGLPanel::StartPickEdge() {
	if (lastHitResult.hitMeshName.empty() || (lastHitResult.hoverEdge.p1 == 0 && lastHitResult.hoverEdge.p2 == 0))
		return false;

	mouseDownMeshName = lastHitResult.hitMeshName;
	mouseDownEdge = lastHitResult.hoverEdge;
	return true;
}

void wxGLPanel::UpdatePickEdge(const wxPoint& screenPos) {
	GLSurface::CursorHitResult hitResult{};

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, true, &hitResult);
	if (!hit || hitResult.hitMeshName != mouseDownMeshName || !hitResult.hoverEdge.CompareIndices(mouseDownEdge))
		gls.HideSegCursor();

	gls.RenderOneFrame();
}

void wxGLPanel::EndPickEdge() {
	if (lastHitResult.hitMeshName != mouseDownMeshName || !lastHitResult.hoverEdge.CompareIndices(mouseDownEdge))
		return;

	// Clear PickEdge state so no accidents can happen
	lastHitResult.hitMeshName.clear();
	gls.HideSegCursor();

	if (activeTool == ToolID::FlipEdge)
		ClickFlipEdge();
	if (activeTool == ToolID::SplitEdge)
		ClickSplitEdge();
}

void wxGLPanel::ClickFlipEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareFlipEdge(shape, uss, mouseDownEdge)) {
		wxMessageBox(_("The edge picked is on the surface boundary.  Pick an interior edge."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

void wxGLPanel::ClickSplitEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	Mesh* m = GetMesh(mouseDownMeshName);
	if (!m)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	bool shiftDown = wxGetKeyState(WXK_SHIFT);

	constexpr uint16_t maxVertIndex = std::numeric_limits<uint16_t>().max();
	uint32_t maxTriIndex = std::numeric_limits<uint16_t>().max();

	if (workNif->GetHeader().GetVersion().IsFO4() || workNif->GetHeader().GetVersion().IsFO76())
		maxTriIndex = std::numeric_limits<uint32_t>().max();

	if (shape->GetNumVertices() > maxVertIndex - 2) {
		wxMessageBox(_("The shape has reached the vertex count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	if (shape->GetNumTriangles() > maxTriIndex - 2) {
		wxMessageBox(_("The shape has reached the triangle count limit."), _("Error"), wxICON_ERROR, os);
		return;
	}

	std::vector<bool> pincs(shape->GetNumVertices(), false);
	pincs[mouseDownEdge.p1] = true;
	pincs[mouseDownEdge.p2] = true;

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	bool noCurveOffset = shiftDown;
	if (!os->project->PrepareRefineMesh(shape, uss, pincs, m->weldVerts, noCurveOffset)) {
		wxMessageBox(_("The edge picked has multiple triangles of the same orientation.  Correct the orientations before splitting."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mesh;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
	os->SetPendingChanges();
}

std::unordered_map<std::string, std::vector<float>> wxGLPanel::StashMasks() {
	std::unordered_map<std::string, std::vector<float>> stash;
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive || !m->mask)
			continue;
		std::vector<float>& mask = stash[m->shapeName];
		mask.resize(m->nVerts);
		std::copy(m->mask.get(), m->mask.get() + m->nVerts, mask.begin());
	}
	return stash;
}

void wxGLPanel::UnstashMasks(const std::unordered_map<std::string, std::vector<float>>& stash) {
	std::vector<Mesh*> meshes = gls.GetMeshes();
	for (Mesh* m : meshes) {
		if (m->bPrimitive)
			continue;

		auto stit = stash.find(m->shapeName);
		if (stit == stash.end())
			continue;
		const std::vector<float>& mask = stit->second;
		// assert(mask.size() == m->nVerts);
		if (static_cast<int>(mask.size()) != m->nVerts)
			continue;
		std::copy(mask.begin(), mask.end(), m->mask.get());
		m->QueueUpdate(Mesh::UpdateType::Mask);
	}
}

void wxGLPanel::MaskLess() {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;
	for (auto& m : gls.GetActiveMeshes()) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		// First, try to remove welds of unmasked points from the mask.
		bool unmaskedAWeld = false;
		for (int i = 0; i < m->nVerts; ++i) {
			if (m->mask[i] > 0.0f)
				continue;
			m->DoForEachWeldedVertex(i, [&](int wvi){
				if (m->mask[wvi] != 0.0f) {
					uss.pointStartState[wvi].x = m->mask[wvi];
					uss.pointEndState[wvi].x = 0.0f;
					unmaskedAWeld = true;
				}
			});
		}

		// If we unmasked a weld, we've done something useful, so stop.
		if (unmaskedAWeld)
			continue;

		// Get points adjacent to unmasked points, taking welds into account.
		std::unordered_set<int> adjacentPoints;
		for (int i = 0; i < m->nVerts; i++)
			if (m->mask[i] == 0.0f)
				m->GetAdjacentPoints(i, adjacentPoints);

		// Unmask the adjacent points
		for (auto& adj : adjacentPoints)
			if (m->mask[adj] != 0.0f) {
				uss.pointStartState[adj].x = m->mask[adj];
				uss.pointEndState[adj].x = 0.0f;
			}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}


void wxGLPanel::MaskMore() {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;
	for (auto& m : gls.GetActiveMeshes()) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		// First, try to add welds of masked points to the mask.
		bool maskedAWeld = false;
		for (int i = 0; i < m->nVerts; ++i) {
			if (m->mask[i] == 0.0f)
				continue;
			m->DoForEachWeldedVertex(i, [&](int wvi){
				if (m->mask[wvi] != 1.0f) {
					uss.pointStartState[wvi].x = m->mask[wvi];
					uss.pointEndState[wvi].x = 1.0f;
					maskedAWeld = true;
				}
			});
		}

		// If we masked a weld, we've done something useful, so stop.
		if (maskedAWeld)
			continue;

		// Get points adjacent to masked points, taking welds into account.
		std::unordered_set<int> adjacentPoints;
		for (int i = 0; i < m->nVerts; i++)
			if (m->mask[i] > 0.0f)
				m->GetAdjacentPoints(i, adjacentPoints);

		// Mask the adjacent points
		for (auto& adj : adjacentPoints) {
			if (m->mask[adj] != 1.0f) {
				uss.pointStartState[adj].x = m->mask[adj];
				uss.pointEndState[adj].x = 1.0f;
			}
		}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}

void wxGLPanel::InvertMask() {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& m : gls.GetActiveMeshes()) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 1.0f - m->mask[i];
		}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}

void wxGLPanel::ClearMask() {
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UndoType::Mask;

	for (auto& m : gls.GetActiveMeshes()) {
		usp->usss.emplace_back();
		UndoStateShape& uss = usp->usss.back();
		uss.shapeName = m->shapeName;

		for (int i = 0; i < m->nVerts; i++) {
			uss.pointStartState[i].x = m->mask[i];
			uss.pointEndState[i].x = 0.0f;
		}
	}

	ApplyUndoState(usp, false);

	if (!Config.GetBoolValue("Input/MaskHistory"))
		GetUndoHistory()->PopState();

	os->UpdateUndoTools();
}

bool wxGLPanel::RestoreMode(UndoStateProject* usp) {
	bool modeChanged = false;
	UndoType undoType = usp->undoType;
	if (undoType == UndoType::VertexPosition && os->activeSlider != usp->sliderName) {
		os->EnterSliderEdit(usp->sliderName);
		modeChanged = true;
	}
	if ((undoType != UndoType::VertexPosition || usp->sliderName.empty()) && os->bEditSlider) {
		os->ExitSliderEdit();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && !usp->sliderName.empty() && usp->sliderscale != os->project->SliderValue(usp->sliderName)) {
		os->SetSliderValue(usp->sliderName, usp->sliderscale * 100);
		os->ApplySliders();
		modeChanged = true;
	}
	if (undoType == UndoType::VertexPosition && usp->sliderName.empty() && !os->project->AllSlidersZero()) {
		os->ZeroSliders();
		modeChanged = true;
	}
	return modeChanged;
}

void wxGLPanel::ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender) {
	UndoType undoType = usp->undoType;
	std::unordered_map<std::string, std::vector<float>> maskStash;
	if (undoType == UndoType::Weight) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& bw : uss.boneWeights) {
				if (bw.boneName != os->GetActiveBone())
					continue;

				for (auto& wIt : bw.weights)
					m->weight[wIt.first] = bUndo ? wIt.second.startVal : wIt.second.endVal;
			}

			m->QueueUpdate(Mesh::UpdateType::Weight);
		}

		os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UndoType::Mask) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->mask[pit.first] = pit.second.x;

			m->QueueUpdate(Mesh::UpdateType::Mask);
		}
	}
	else if (undoType == UndoType::Color) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->vcolors[pit.first] = pit.second;

			m->QueueUpdate(Mesh::UpdateType::VertexColors);
		}
	}
	else if (undoType == UndoType::Alpha) {
		for (auto& uss : usp->usss) {
			Mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->valpha[pit.first] = pit.second.x;

			m->QueueUpdate(Mesh::UpdateType::VertexAlpha);
		}

		os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UndoType::VertexPosition) {
		auto applyUVState = [&](UndoStateShape& uss) {
			if (!usp->sliderName.empty())
				return;

			const auto& uvState = bUndo ? uss.uvStartState : uss.uvEndState;
			if (uvState.empty())
				return;

			Mesh* m = GetMesh(uss.shapeName);
			if (m && m->texcoord) {
				for (auto& uvIt : uvState) {
					if (uvIt.first >= 0 && uvIt.first < m->nVerts)
						m->texcoord[uvIt.first] = uvIt.second;
				}

				m->QueueUpdate(Mesh::UpdateType::TextureCoordinates);
			}

			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				return;

			std::vector<Vector2> uvs;
			if (!os->project->GetWorkNif()->GetUvsForShape(shape, uvs))
				return;

			for (auto& uvIt : uvState) {
				if (uvIt.first >= 0 && uvIt.first < static_cast<int>(uvs.size()))
					uvs[uvIt.first] = uvIt.second;
			}

			os->project->GetWorkNif()->SetUvsForShape(shape, uvs);
		};

		bool hasRestDiffs = false;
		for (auto& uss : usp->usss) {
			if (!uss.restDiffs.empty()) {
				hasRestDiffs = true;
				break;
			}
		}

		if (hasRestDiffs) {
			// Pose-independent undo/redo: use stored rest-space NIF diffs
			if (usp->sliderName.empty()) {
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (!shape)
						continue;

					std::vector<Vector3> restVerts;
					os->project->GetWorkNif()->GetVertsForShape(shape, restVerts);

					for (auto& rd : uss.restDiffs) {
						if (bUndo)
							restVerts[rd.first] -= rd.second;
						else
							restVerts[rd.first] += rd.second;
					}

					os->project->GetWorkNif()->SetVertsForShape(shape, restVerts);
				}
			}

			os->ActiveShapesUpdated(usp, bUndo);

			for (auto& uss : usp->usss) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
				if (!shape)
					continue;

				std::vector<Vector3> verts;
				os->project->GetLiveVerts(shape, verts);
				UpdateMeshVertices(shape->name.get(), &verts, true, true, false);

				Mesh* m = GetMesh(uss.shapeName);
				if (m)
					m->CalcWeldVerts();

				applyUVState(uss);
			}
		}
		else {
			for (auto& uss : usp->usss) {
				Mesh* m = GetMesh(uss.shapeName);
				if (!m)
					continue;

				for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
					m->verts[pit.first] = pit.second;

				m->CalcWeldVerts();
				m->SmoothNormals();
				BVHUpdateQueue.insert(m);

				m->QueueUpdate(Mesh::UpdateType::Position);

				applyUVState(uss);
			}

			os->ActiveShapesUpdated(usp, bUndo);

			if (usp->sliderName.empty()) {
				for (auto& uss : usp->usss) {
					Mesh* m = GetMesh(uss.shapeName);
					if (!m)
						continue;

					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape)
						os->project->UpdateShapeFromMesh(shape, m);
				}

				if (os->project->bPose) {
					for (auto& uss : usp->usss) {
						auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
						if (shape) {
							std::vector<Vector3> verts;
							os->project->GetLiveVerts(shape, verts);
							UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
						}
					}
				}
			}
			else if (os->project->bPose) {
				for (auto& uss : usp->usss) {
					auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
					if (shape) {
						std::vector<Vector3> verts;
						os->project->GetLiveVerts(shape, verts);
						UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
					}
				}
			}
		}
	}
	else if (undoType == UndoType::Mesh) {
		maskStash = StashMasks();

		// Undo: restore deleted shapes before processing vertex mesh undos
		if (bUndo) {
			for (auto& ds : usp->deletedShapes)
				os->project->RestoreDeletedShape(ds);
		}

		for (auto& uss : usp->usss) {
			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			os->project->ApplyShapeMeshUndo(shape, maskStash[uss.shapeName], uss, bUndo);
		}

		// Redo: re-delete shapes after processing vertex mesh redos
		if (!bUndo) {
			for (auto& ds : usp->deletedShapes) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(ds.shapeName);
				if (shape)
					os->project->DeleteShape(shape);
			}
		}

		os->RefreshGUIFromProj(false, false);
		UnstashMasks(maskStash);
		os->ApplySliders();
	}
	else if (undoType == UndoType::ShapeDelete) {
		if (bUndo) {
			for (auto& ds : usp->deletedShapes)
				os->project->RestoreDeletedShape(ds);
		}
		else {
			for (auto& ds : usp->deletedShapes) {
				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(ds.shapeName);
				if (shape)
					os->project->DeleteShape(shape);
			}
		}

		os->RefreshGUIFromProj(false, false);
		os->ApplySliders();
	}
	else if (undoType == UndoType::Mirror) {
		for (auto& uss : usp->usss) {
			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			os->project->GetWorkNif()->MirrorShape(shape, usp->mirrorX, usp->mirrorY, usp->mirrorZ);
			if (usp->swapBonesX)
				os->project->GetWorkAnim()->SwapBonesLR(uss.shapeName);
		}

		os->RefreshGUIFromProj(false);
	}
	else if (undoType == UndoType::NodeTransform) {
		const MatTransform& target = bUndo ? usp->nodeStartXformToParent : usp->nodeEndXformToParent;

		if (AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(usp->boneName))
			bPtr->SetTransformBoneToParent(target);

		auto* nif = os->project->GetWorkNif();
		if (nif) {
			nif->SetNodeTransformToParent(usp->boneName, target);
			if (auto* workAnim = os->project->GetWorkAnim())
				workAnim->WriteNodesToNif(nif);

			// Re-skin all shapes so poses and bind-pose display both follow
			// the restored bone transform.
			for (auto& s : nif->GetShapes()) {
				std::vector<Vector3> verts;
				os->project->GetLiveVerts(s, verts);
				UpdateMeshVertices(s->name.get(), &verts, true, true, false);
			}
		}

		UpdateBones();
		UpdateNodes();
		os->RefreshBoneTreeBadBoneIcons();
		os->SetPendingChanges();
	}

	if (bRender) {
		if (transformMode)
			ShowTransformTool();
		else
			Render();
	}
}

bool wxGLPanel::UndoStroke() {
	UndoStateProject* curState = undoHistory.GetCurState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!undoHistory.BackStepHistory())
		return false;

	ApplyUndoState(curState, true);

	return true;
}

bool wxGLPanel::RedoStroke() {
	UndoStateProject* curState = undoHistory.GetNextState();
	if (!curState)
		return false;
	if (RestoreMode(curState))
		return true;
	if (!undoHistory.ForwardStepHistory())
		return false;

	ApplyUndoState(curState, false);

	return true;
}

void wxGLPanel::ShowRotationCenter(bool show) {
	if (show) {
		RotationCenterMesh = gls.AddVis3dSphere(gls.camRotOffset, 0.08f, Vector3(1.0f, 0.0f, 1.0f), "RotationCenterMesh");
		RotationCenterMesh->prop.alpha = 0.25f;
		RotationCenterMesh->bVisible = true;

		RotationCenterMeshRingX = gls.AddVis3dRing(gls.camRotOffset, Vector3(1.0f, 0.0f, 0.0f), 0.25f, 0.01f, Vector3(1.0f, 0.0f, 0.0f), "RotationCenterMeshRingX");
		RotationCenterMeshRingX->prop.alpha = 0.25f;
		RotationCenterMeshRingX->bVisible = true;

		RotationCenterMeshRingY = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 1.0f, 0.0f), 0.25f, 0.01f, Vector3(0.0f, 1.0f, 0.0f), "RotationCenterMeshRingY");
		RotationCenterMeshRingY->prop.alpha = 0.25f;
		RotationCenterMeshRingY->bVisible = true;

		RotationCenterMeshRingZ = gls.AddVis3dRing(gls.camRotOffset, Vector3(0.0f, 0.0f, 1.0f), 0.25f, 0.01f, Vector3(0.0f, 0.0f, 1.0f), "RotationCenterMeshRingZ");
		RotationCenterMeshRingZ->prop.alpha = 0.25f;
		RotationCenterMeshRingZ->bVisible = true;
	}
	else {
		if (RotationCenterMesh) {
			RotationCenterMesh->bVisible = false;
			RotationCenterMeshRingX->bVisible = false;
			RotationCenterMeshRingY->bVisible = false;
			RotationCenterMeshRingZ->bVisible = false;
		}
	}
}

void wxGLPanel::ShowTransformTool(bool show) {
	UpdateTransformCenter();

	if (show) {
		XMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(1.0f, 0.0f, 0.0f), "XMoveMesh");
		XMoveMesh->prop.alpha = 0.6f;
		YMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 1.0f, 0.0f), "YMoveMesh");
		YMoveMesh->prop.alpha = 0.6f;
		ZMoveMesh = gls.AddVis3dArrow(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 0.04f, 0.15f, 1.75f, Vector3(0.0f, 0.0f, 1.0f), "ZMoveMesh");
		ZMoveMesh->prop.alpha = 0.6f;

		XRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(1.0f, 0.0f, 0.0f), 1.25f, 0.04f, Vector3(1.0f, 0.0f, 0.0f), "XRotateMesh");
		XRotateMesh->prop.alpha = 0.6f;
		YRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 1.0f, 0.0f), 1.25f, 0.04f, Vector3(0.0f, 1.0f, 0.0f), "YRotateMesh");
		YRotateMesh->prop.alpha = 0.6f;
		ZRotateMesh = gls.AddVis3dRing(xformCenter, Vector3(0.0f, 0.0f, 1.0f), 1.25f, 0.04f, Vector3(0.0f, 0.0f, 1.0f), "ZRotateMesh");
		ZRotateMesh->prop.alpha = 0.6f;

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

		if (nodesMode || bonesMode) {
			// Only move and rotate gizmos are supported for bones/nodes.
			XScaleMesh->bVisible = false;
			YScaleMesh->bVisible = false;
			ZScaleMesh->bVisible = false;
			ScaleUniformMesh->bVisible = false;
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

void wxGLPanel::UpdateTransformCenter() {
	bool centerSet = false;

	if (pivotMode) {
		xformCenter = pivotPosition;
		centerSet = true;
	}
	else if (nodesMode || bonesMode) {
		std::string activeBone = os->GetActiveBone();
		if (!activeBone.empty()) {
			auto boneMesh = gls.GetOverlay("BP_" + activeBone);
			if (boneMesh) {
				xformCenter = boneMesh->verts[0];
				centerSet = true;
			}
			else {
				auto nodeMesh = gls.GetOverlay("P_" + activeBone);
				if (nodeMesh) {
					xformCenter = nodeMesh->verts[0];
					centerSet = true;
				}
			}
		}
	}

	if (!centerSet)
		xformCenter = gls.GetActiveCenter();
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

void wxGLPanel::ShowNodes(bool show) {
	nodesMode = show;

	for (auto& m : nodesPoints)
		m->bVisible = show;

	for (auto& m : nodesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateNodes() {
	for (auto& m : nodesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : nodesLines)
		gls.DeleteOverlay(m);

	nodesPoints.clear();
	nodesLines.clear();

	auto workNif = os->project->GetWorkNif();

	std::function<void(NiNode*, NiNode*, const Vector3&, const Vector3&)> addChildNodes =
		[&](NiNode* node, NiNode* parent, const Vector3& rootPosition, const Vector3& parentPosition) {
			MatTransform ttg = node->GetTransformToParent();
			NiNode* parentNode = parent;
			while (parentNode) {
				ttg = parentNode->GetTransformToParent().ComposeTransforms(ttg);
				parentNode = workNif->GetParentNode(parentNode);
			}

			Vector3 position = ttg.ApplyTransform(rootPosition);

			std::string nodeName = node->name.get();
			if (!nodeName.empty()) {
				Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

				auto pointMesh = gls.AddVisPoint(renderPosition, "P_" + nodeName);
				if (pointMesh) {
					pointMesh->bVisible = nodesMode;
					nodesPoints.push_back(pointMesh);
				}

				if (parent) {
					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "L_" + nodeName);
					if (lineMesh) {
						lineMesh->bVisible = nodesMode;
						nodesLines.push_back(lineMesh);
					}
				}
			}

			for (auto& child : node->childRefs) {
				auto childNode = workNif->GetHeader().GetBlock<NiNode>(child);
				if (childNode)
					addChildNodes(childNode, node, rootPosition, position);
			}
		};

	auto rootNode = workNif->GetRootNode();
	if (rootNode)
		addChildNodes(rootNode, nullptr, rootNode->transform.translation, rootNode->transform.translation);

	UpdateNodeColors();
}

void wxGLPanel::ShowBones(bool show) {
	bonesMode = show;

	for (auto& m : bonesPoints)
		m->bVisible = show;

	for (auto& m : bonesLines)
		m->bVisible = show;

	if (transformMode)
		ShowTransformTool();

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateBones() {
	for (auto& m : bonesPoints)
		gls.DeleteOverlay(m);

	for (auto& m : bonesLines)
		gls.DeleteOverlay(m);

	bonesPoints.clear();
	bonesLines.clear();

	auto workAnim = os->project->GetWorkAnim();
	bool isSF = os->project->GetWorkNif()->GetHeader().GetVersion().IsSF();

	std::function<bool(AnimBone*)> addChildBones = [&](AnimBone* parent) {
		bool anyBoneInSelection = false;

		for (auto& cb : parent->children) {
			bool childBonesInSelection = addChildBones(cb);

			if (!cb->boneName.empty()) {
				bool boneInSelection = false;
				for (auto& si : os->GetSelectedItems()) {
					if (workAnim->GetShapeBoneIndex(si->GetShape()->name.get(), cb->boneName) != -1) {
						boneInSelection = true;
						break;
					}
				}

				if (boneInSelection || childBonesInSelection) {
					const MatTransform& toGlobal = os->project->bPose ? cb->xformPoseToGlobal : cb->xformToGlobal;
					Vector3 position = toGlobal.ApplyTransform(Vector3());
					const MatTransform& parentToGlobal = os->project->bPose ? parent->xformPoseToGlobal : parent->xformToGlobal;
					Vector3 parentPosition = parentToGlobal.ApplyTransform(Vector3());

					if (isSF) {
						position *= sfHavokScale;
						parentPosition *= sfHavokScale;
					}

					bool matchesParent = position.IsNearlyEqualTo(parentPosition);

					Vector3 renderPosition = Mesh::TransformPosNifToMesh(position);

					auto pointMesh = gls.AddVisPoint(renderPosition, "BP_" + cb->boneName);
					if (pointMesh) {
						pointMesh->bVisible = bonesMode;
						bonesPoints.push_back(pointMesh);
					}

					Vector3 renderParentPosition = Mesh::TransformPosNifToMesh(parentPosition);

					if (!matchesParent) {
						auto lineMesh = gls.AddVisSeg(renderParentPosition, renderPosition, "BL_" + cb->boneName);
						if (lineMesh) {
							lineMesh->bVisible = bonesMode;
							bonesLines.push_back(lineMesh);
						}
					}

					anyBoneInSelection = true;
				}
			}
		}

		return anyBoneInSelection;
	};

	AnimBone* rb = AnimSkeleton::getInstance().GetRootBonePtr();
	if (rb)
		addChildBones(rb);

	UpdateNodeColors();
}

void wxGLPanel::UpdateNodeColors() {
	std::string activeBone = os->GetActiveBone();

	for (auto& m : nodesPoints) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color.x = 1.0f;
			m->color.y = 0.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color.x = 0.0f;
			m->color.y = 1.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : nodesLines) {
		auto nodeName = m->shapeName.substr(2, m->shapeName.length() - 2);
		if (nodeName == activeBone) {
			m->color.x = 0.7f;
			m->color.y = 0.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color.x = 0.0f;
			m->color.y = 0.7f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : bonesPoints) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		if (nodeName == activeBone) {
			m->color.x = 1.0f;
			m->color.y = 0.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color.x = 0.0f;
			m->color.y = 1.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::Default;
		}
	}

	for (auto& m : bonesLines) {
		auto nodeName = m->shapeName.substr(3, m->shapeName.length() - 3);
		if (nodeName == activeBone) {
			m->color.x = 0.7f;
			m->color.y = 0.0f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::NodeSelection;
		}
		else {
			m->color.x = 0.0f;
			m->color.y = 0.7f;
			m->color.z = 0.0f;
			m->overlayLayer = OverlayLayer::Default;
		}
	}
}

void wxGLPanel::ShowFloor(bool show) {
	floorMode = show;

	for (auto& m : floorMeshes)
		m->bVisible = show;

	gls.RenderOneFrame();
}

void wxGLPanel::UpdateFloor() {
	for (auto& m : floorMeshes)
		gls.DeleteMesh(m);

	floorMeshes = gls.AddFloor();

	for (auto& m : floorMeshes)
		m->bVisible = floorMode;
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto& m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			Mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
			if (m) {
				m->bShowPoints = true;
				m->ComputeAvgEdgeLength();
				m->QueueUpdate(Mesh::UpdateType::Mask);
			}
		}
	}
}

void wxGLPanel::OnIdle(wxIdleEvent& WXUNUSED(event)) {
	if (wxGetKeyState(wxKeyCode::WXK_SHIFT) || wxGetKeyState(wxKeyCode::WXK_CONTROL) || wxGetKeyState(wxKeyCode::WXK_ALT) || lbuttonDown || rbuttonDown || mbuttonDown)
		return;

	for (auto& m : BVHUpdateQueue)
		m->CreateBVH();

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
	int delt = event.GetWheelRotation();

	// Switching the edited slider and resizing the brush are both locked out
	// during playback; zooming falls through.
	const bool playing = os && os->IsAnimationPlaying();

	if (!playing && event.ControlDown()) {
		std::string sliderName = os->lastActiveSlider;

		if (sliderName.empty())
			sliderName = os->project->GetSliderName(0);

		if (sliderName.empty())
			return;

		if (!os->activeSlider.empty())
			os->ExitSliderEdit();

		size_t sliderCount = os->project->SliderCount();
		size_t sliderIndex = 0;
		if (os->project->SliderIndexFromName(sliderName, sliderIndex)) {
			if (delt < 0) {
				++sliderIndex;

				if (sliderIndex == sliderCount)
					sliderIndex = 0;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
			else {
				if (sliderIndex == 0)
					sliderIndex = sliderCount - 1;
				else
					--sliderIndex;

				os->EnterSliderEdit(os->project->GetSliderName(sliderIndex));
				os->ScrollToActiveSlider();
			}
		}
	}
	else if (!playing && wxGetKeyState(wxKeyCode('S'))) {
		wxPoint p = event.GetPosition();

		if (brushMode) {
			// Adjust brush size
			if (delt < 0)
				DecBrush();
			else
				IncBrush();

			os->CheckBrushBounds();
			os->UpdateBrushSettings();
			gls.UpdateCursor(p.x, p.y, true);
			gls.RenderOneFrame();
		}
	}
	else {
		gls.DollyCamera(delt);
		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}
}

void wxGLPanel::OnMouseMove(wxMouseEvent& event) {
	if (os->IsActive())
		SetFocus();

	// Mouse motion floods the message queue, which starves both the idle events
	// and the WM_TIMER that drive playback and physics, so tick them from here
	// as well.
	os->PumpAnimationPlayback();
	os->PumpPhysics();

	bool cursorExists = false;
	int x;
	int y;
	event.GetPosition(&x, &y);

	mouseHasMovedSinceStart = true;

	ShowRotationCenter(false);

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

		if (wxGetKeyState(WXK_ALT)) {
			if (lastHitResult.hoverPoint != -1) {
				rotationCenterMode = RotationCenterMode::Picked;
				gls.camRotOffset = lastHitResult.hoverRealCoord;
			}
		}

		if (wxGetKeyState(WXK_SHIFT)) {
			gls.PanCamera(x - lastX, y - lastY);
		}
		else {
			float yawDegrees = gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
			os->InjectPhysicsCameraYaw(yawDegrees);
			ShowRotationCenter();
		}

		UpdateTransformTool();
		UpdatePivot();
		gls.RenderOneFrame();
	}

	if (lbuttonDown || isMovingVertex || isSlidingEdge) {
		isLDragging = true;
		if (isPhysicsGrabbing) {
			UpdatePhysicsGrab(event.GetPosition());
		}
		else if (isTransforming) {
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
		else if (isPickingVertex) {
			UpdatePickVertex(event.GetPosition());
		}
		else if (isPickingEdge) {
			UpdatePickEdge(event.GetPosition());
		}
		else if (isMovingVertex) {
			UpdateMoveVertex(event.GetPosition() - mouseDownOffset);
		}
		else if (isSlidingEdge) {
			UpdateEdgeSlide(event.GetPosition());
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

	if (!rbuttonDown && !lbuttonDown && !isMovingVertex && !isSlidingEdge) {
		GLSurface::CursorHitResult hitResult{};

		// The brush cursor is hidden during playback: the brushes are locked out
		// anyway, and it would hit-test against the mesh BVHs that playback
		// leaves stale, reporting vertices that are not where it draws them.
		if (editMode && !(os && os->IsAnimationPlaying())) {
			cursorExists = gls.UpdateCursor(x, y, true, &hitResult);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
		}

		if (activeTool == ToolID::MoveVertex) {
			mouseDownMirrorPoint = -1;
			if (GetToolOptionXMirror()) {
				Vector3 hitPt, hitNrm;
				Mesh* mmesh = nullptr;
				int triInd = 0;
				if (gls.CollideMeshes(x, y, hitPt, hitNrm, true, &mmesh, true, &triInd) && mmesh == hitResult.hitMesh) {
					int hitPti = mmesh->tris[triInd].ClosestVertex(mmesh->verts.get(), hitPt);
					if (hitPti != hitResult.hoverPoint) {
						// Should we also check if the mirror point's location
						// is close to the mirror of the hover point's location?
						mouseDownMirrorPoint = hitPti;
						gls.ShowMirrorPointCursor(mmesh->verts[mouseDownMirrorPoint], mmesh);
					}
				}
			}
		}

		lastHitResult = hitResult;

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
			Mesh* hitMesh = nullptr;
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
				if (activeTool == ToolID::MaskBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", hitResult.hoverPoint, hitResult.hoverMask), 1);
				else if (activeTool == ToolID::WeightBrush) {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hitResult.hoverWeight, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z), 1);
				}
				else if (activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Color: %g, %g, %g, Alpha: %g",
																  hitResult.hoverPoint,
																  hitResult.hoverColor.x,
																  hitResult.hoverColor.y,
																  hitResult.hoverColor.z,
																  hitResult.hoverAlpha),
												 1);
				else {
					Vector3 hoverCoordNif = Mesh::TransformPosMeshToNif(hitResult.hoverMeshCoord);
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, X: %.5f Y: %.5f Z: %.5f", hitResult.hoverPoint, hoverCoordNif.x, hoverCoordNif.y, hoverCoordNif.z),
												 1);
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

	// No tool may start editing while an animation plays; the click still falls
	// through to camera navigation.
	if (os && os->IsAnimationPlaying())
		return;

	// A grab feeds the running simulation and never touches the mesh data, so
	// while it is armed it takes the click ahead of the editing tools. Missing
	// the simulated mesh leaves the click to them as if it were not.
	if (os->IsPhysicsGrabEnabled()) {
		bool meshHit = StartPhysicsGrab(event.GetPosition());
		if (meshHit) {
			isPhysicsGrabbing = true;
			return;
		}
	}

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

	if (brushMode) {
		bool meshHit = StartBrushStroke(event.GetPosition());
		if (meshHit)
			isPainting = true;
	}
	else if (activeTool == ToolID::CollapseVertex) {
		bool meshHit = StartPickVertex();
		if (meshHit)
			isPickingVertex = true;
	}
	else if (activeTool == ToolID::FlipEdge || activeTool == ToolID::SplitEdge) {
		bool meshHit = StartPickEdge();
		if (meshHit)
			isPickingEdge = true;
	}
	else if (activeTool == ToolID::MoveVertex) {
		if (GetToolOptionEdgeSlide()) {
			if (isSlidingEdge) {
				EndEdgeSlide();
				isSlidingEdge = false;
			}
			else {
				bool meshHit = StartEdgeSlide(event.GetPosition());
				if (meshHit)
					isSlidingEdge = true;
			}
		}
		else if (isMovingVertex) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else {
			bool meshHit = StartMoveVertex(event.GetPosition());
			if (meshHit)
				isMovingVertex = true;
		}
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

	// OnLeftDown bails out during playback, so none of the edit states below can
	// be set; only the click-to-select path would still run, and it would pick
	// against the mesh BVHs that playback leaves stale.
	const bool playing = os && os->IsAnimationPlaying();

	if (!playing && !isLDragging && !isPainting && !isPhysicsGrabbing && activeTool == ToolID::Select) {
		int x, y;
		event.GetPosition(&x, &y);

		Mesh* m = gls.PickMesh(x, y);
		if (m)
			os->SelectShape(m->shapeName);
	}

	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		if (mouseHasMovedSinceStart) {
			EndMoveVertex();
			isMovingVertex = false;
		}
		else
			mouseHasMovedSinceStart = true;
	}

	if (isSlidingEdge) {
		if (mouseHasMovedSinceStart) {
			EndEdgeSlide();
			isSlidingEdge = false;
		}
		else
			mouseHasMovedSinceStart = true;
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
	if (isPhysicsGrabbing)
		EndPhysicsGrab();

	if (isPainting) {
		EndBrushStroke();
		isPainting = false;
	}

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
	}

	if (isMovingVertex) {
		CancelMoveVertex();
		isMovingVertex = false;
	}

	if (isSlidingEdge) {
		CancelEdgeSlide();
		isSlidingEdge = false;
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
	if (owner && owner->IsAnimationPlaying())
		return false;

	if (owner) {
		NiShape* mergeShape = nullptr;
		if (owner->activeItem && fileNames.GetCount() == 1)
			mergeShape = owner->activeItem->GetShape();

		for (auto& inputFile : fileNames) {
			wxString dataName = inputFile.AfterLast('/').AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			if (inputFile.Lower().EndsWith(".nif")) {
				owner->StartProgress(_("Adding NIF file..."));
				owner->UpdateProgress(1, _("Adding NIF file..."));
				owner->project->ImportNIF(inputFile.ToUTF8().data(), false);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".obj")) {
				owner->StartProgress("Adding OBJ file...");
				owner->UpdateProgress(1, _("Adding OBJ file..."));
				owner->project->ImportOBJ(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
#ifdef USE_FBXSDK
			else if (inputFile.Lower().EndsWith(".fbx")) {
				owner->StartProgress(_("Adding FBX file..."));
				owner->UpdateProgress(1, _("Adding FBX file..."));
				owner->project->ImportFBX(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->EndProgress();
			}
#endif
		}

		owner->UpdateTitle();
	}
	else
		return false;

	return true;
}

bool DnDSliderFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& fileNames) {
	if (owner && owner->IsAnimationPlaying())
		return false;

	if (owner) {
		bool isMultiple = (fileNames.GetCount() > 1);
		for (size_t i = 0; i < fileNames.GetCount(); i++) {
			wxString inputFile;
			inputFile = fileNames.Item(i);

			wxString dataName = inputFile.AfterLast('/').AfterLast('\\');
			dataName = dataName.BeforeLast('.');

			bool isBSD = inputFile.MakeLower().EndsWith(".bsd");
			bool isOBJ = inputFile.MakeLower().EndsWith(".obj");
#ifdef USE_FBXSDK
			bool isFBX = inputFile.MakeLower().EndsWith(".fbx");
			if (isBSD || isOBJ || isFBX) {
#else
			if (isBSD || isOBJ) {
#endif
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

				if (!owner->ConfirmSliderDataLocalForEdit(owner->activeItem->GetShape(), targetSlider)) {
					owner->EndProgress();
					return false;
				}

				if (isBSD)
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
#ifdef USE_FBXSDK
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
#endif
				else
					return false;


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
		for (auto& child : owner->sliderPanels) {
			if (child.second->GetRect().Contains(x, y)) {
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
