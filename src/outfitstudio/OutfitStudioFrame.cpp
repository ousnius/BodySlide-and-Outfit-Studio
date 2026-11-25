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

#include "OutfitStudioFrame.h"
#include "PosePanel.h"
#include "SegmentPartitionEditor.h"
#include "PhysicsPreview.h"
#include "PhysicsXmlEditor.h"
#include "AnimationPlayer.h"
#include "ui/AppSetup.h"
#include "FrameHelpers.h"
#include "GLPanel.h"
#include "ShapeTreeItems.h"
#include "render/ViewState.h"
#include "utils/ConfigurationManager.h"
#include "ui/wxStateButton.h"
#include "ui/outfitstudio/WeightCopyDialog.h"
#include "FSEngine/FSManager.h"
#include "components/TweakBrush.h"
#include "components/UndoHistory.h"
#include "render/GLSurface.h"
#include "utils/TargetGame.h"
#include "DropTargets.h"
#include "OutfitStudioApp.h"
#include "mcp/OutfitStudioMcp.h"
#include "components/MaskOps.h"
#include "components/SliderGroup.h"
#include "components/SliderPresets.h"
#include "components/SliderSetIndex.h"
#include "files/HkxFile.h"
#include "files/MaskFile.h"
#include "files/TriFile.h"
#include "files/SFMorphFile.h"
#include "files/GameDataStream.h"
#include "ui/outfitstudio/ArchiveBrowser.h"
#include "ui/outfitstudio/wxBrushSettingsPopup.h"
#include "ui/ConfigDialogUtil.h"
#include "utils/PlatformUtil.h"
#include "ui/outfitstudio/EditUV.h"
#include "ui/outfitstudio/FBXImportDialog.h"
#include "ui/GroupManager.h"
#include "ui/outfitstudio/ObjImportDialog.h"
#include "ui/PresetSaveDialog.h"
#include "ui/outfitstudio/PartitionTypeChoices.h"
#include "ui/outfitstudio/ShapeProperties.h"
#include "ui/outfitstudio/SliderDataDialog.h"
#include "ui/outfitstudio/AddProjectDialog.h"
#include "ui/outfitstudio/BadBonesDialog.h"
#include "project/BrushStrokeController.h"
#include "ui/outfitstudio/SliderDataImportDialog.h"
#include "ui/outfitstudio/AutomationDialog.h"
#include "components/ClippingFixer.h"
#include "components/MaskTransfer.h"
#include "utils/ProjectUtil.h"
#include "utils/GameUtil.h"
#include "utils/StackTrace.h"
#include "utils/StringStuff.h"
#include "ui/SettingsDialogShared.h"

#include <cmath>
#include <cstdlib>
#include <iterator>
#include <sstream>
#include <wx/debugrpt.h>
#include <wx/dir.h>
#include <wx/display.h>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "ui/outfitstudio/ConvertBodyReferenceDialog.h"

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>

#include <mmsystem.h>
#endif

using namespace nifly;
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
	EVT_MENU(XRCID("fileMcpServer"), OutfitStudioFrame::OnMcpServer)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudioFrame::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudioFrame::OnLoadProject)
	EVT_MENU(XRCID("btnAddProject"), OutfitStudioFrame::OnAddProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudioFrame::OnLoadReference)
	EVT_MENU(XRCID("fileConvBodyRef"), OutfitStudioFrame::OnConvertBodyReference)
	EVT_MENU(XRCID("menuRunAutomation"), OutfitStudioFrame::OnRunAutomation)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudioFrame::OnLoadOutfit)
	EVT_MENU(XRCID("fileBrowseArchives"), OutfitStudioFrame::OnBrowseArchives)
	EVT_MENU(XRCID("fileSave"), OutfitStudioFrame::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudioFrame::OnSaveSliderSetAs)
	EVT_MENU(XRCID("fileUnload"), OutfitStudioFrame::OnUnloadProject)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("masksPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COMBOBOX(XRCID("cMaskName"), OutfitStudioFrame::OnSelectMask)
	EVT_BUTTON(XRCID("saveMask"), OutfitStudioFrame::OnSaveMask)
	EVT_BUTTON(XRCID("deleteMask"), OutfitStudioFrame::OnDeleteMask)
	EVT_BUTTON(XRCID("exportMask"), OutfitStudioFrame::OnExportMask)
	EVT_BUTTON(XRCID("importMask"), OutfitStudioFrame::OnImportMask)
	EVT_BUTTON(XRCID("copyMask"), OutfitStudioFrame::OnCopyMask)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("posePane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("physicsPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("animationPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("notesPane"), OutfitStudioFrame::OnPaneCollapse)



	EVT_CHECKBOX(XRCID("selectSliders"), OutfitStudioFrame::OnSelectSliders)
	EVT_TEXT_ENTER(XRCID("sliderFilter"), OutfitStudioFrame::OnSliderFilterChanged)
	EVT_TEXT(XRCID("sliderFilter"), OutfitStudioFrame::OnSliderFilterChanged)
	EVT_CHECKBOX(XRCID("cbFixedWeight"), OutfitStudioFrame::OnFixedWeight)
	EVT_CHECKBOX(XRCID("cbNormalizeWeights"), OutfitStudioFrame::OnCBNormalizeWeights)
	EVT_BUTTON(XRCID("btnNormalizeWeights"), OutfitStudioFrame::OnNormalizeWeights)

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
	EVT_COMBOBOX(XRCID("presetQuick"), OutfitStudioFrame::OnQuickPreset)
	EVT_COMBOBOX_DROPDOWN(XRCID("presetQuick"), OutfitStudioFrame::OnQuickPresetDropdown)
	EVT_RADIOBUTTON(XRCID("presetWeightLo"), OutfitStudioFrame::OnQuickPresetState)
	EVT_RADIOBUTTON(XRCID("presetWeightHi"), OutfitStudioFrame::OnQuickPresetState)
	EVT_RADIOBUTTON(XRCID("presetOff"), OutfitStudioFrame::OnQuickPresetState)
	EVT_MENU(XRCID("presetToggleAccel"), OutfitStudioFrame::OnQuickPresetToggleAccel)
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
	EVT_MENU(XRCID("sliderRigidify"), OutfitStudioFrame::OnSliderRigidify)
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
	EVT_MENU(XRCID("btnEnableTints"), OutfitStudioFrame::OnEnableTints)

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
	EVT_MENU(XRCID("duplicateBones"), OutfitStudioFrame::OnDuplicateBones)
	EVT_MENU(XRCID("duplicateBonesMirrored"), OutfitStudioFrame::OnDuplicateBonesMirrored)
	EVT_MENU(XRCID("mirrorBonesX"), OutfitStudioFrame::OnMirrorBonesX)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudioFrame::OnCopyBoneWeight)
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudioFrame::OnCopySelectedWeight)
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudioFrame::OnTransferSelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudioFrame::OnMaskWeighted)
	EVT_MENU(XRCID("checkBadBones"), OutfitStudioFrame::OnCheckBadBones)
	EVT_MENU(XRCID("maskBoneWeightedVerts"), OutfitStudioFrame::OnMaskBoneWeighted)
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
	EVT_TREE_ITEM_GETTOOLTIP(XRCID("outfitShapes"), OutfitStudioFrame::OnShapeTooltip)

	EVT_TEXT_ENTER(XRCID("bonesFilter"), OutfitStudioFrame::OnBonesFilterChanged)
	EVT_TEXT(XRCID("bonesFilter"), OutfitStudioFrame::OnBonesFilterChanged)
	EVT_TREE_STATE_IMAGE_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneStateToggle)
	EVT_TREE_SEL_CHANGED(XRCID("outfitBones"), OutfitStudioFrame::OnBoneSelect)
	EVT_TREE_ITEM_ACTIVATED(XRCID("outfitBones"), OutfitStudioFrame::OnBoneActivated)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneTreeContext)



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


OutfitStudioFrame::~OutfitStudioFrame() {}

OutfitStudioFrame::OutfitStudioFrame(const wxPoint& pos, const wxSize& size) {
	// Other threads wake the dispatcher through the event queue, which wx makes thread safe
	dispatcher = std::make_unique<MainThreadDispatcher>([this]() { CallAfter([this]() { dispatcher->ProcessPending(); }); });
	animation = std::make_unique<AnimationPlayer>(*this);
	physicsPreview = std::make_unique<PhysicsPreview>(*this);
	physicsXml = std::make_unique<PhysicsXmlEditor>(*this);
	segPartEditor = std::make_unique<SegmentPartitionEditor>(*this);
	posePanel = std::make_unique<PosePanel>(*this);

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

	segPartEditor->Init();

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
	posePanel->Init();

	// Nothing is loaded yet, and only meshes that reference a physics XML can be
	// simulated: UpdatePhysicsControlsVisibility reveals the pane again when one
	// shows up. In a build without Bullet that never happens.
	if (physicsPane)
		physicsPane->Hide();

	animation->Init();
	physicsPreview->Init();
	physicsXml->Init();
	animation->ResetList();

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	if (leftPanel) {
		glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
		glView->SetNotifyWindow(this);
		glView->SetColorsVisible(menuBar->IsChecked(XRCID("btnEnableVertexColors")));

		float brushSize = OutfitStudioConfig.GetFloatValue("BrushSettings.brushsize");
		if (brushSize != 0.0f)
			glView->SetBrushSize(brushSize);
		else
			glView->ResetBrushSize();

		glView->SetToolOptionAccumulateSmoothWeight(OutfitStudioConfig.GetBoolValue("BrushSettings.accumulateSmoothWeight", true));
		glView->SetToolOptionAccumulateSmoothMask(OutfitStudioConfig.GetBoolValue("BrushSettings.accumulateSmoothMask", true));

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

	presetQuick = (wxFavoriteChoice*)FindWindowByName("presetQuick");
	presetWeightLo = (wxRadioButton*)FindWindowByName("presetWeightLo");
	presetWeightHi = (wxRadioButton*)FindWindowByName("presetWeightHi");
	presetOff = (wxRadioButton*)FindWindowByName("presetOff");
	// Long preset names would otherwise widen the whole panel
	if (presetQuick)
		presetQuick->SetMinSize(wxSize(FromDIP(60), -1));
	RefreshQuickPresets();

	// Ctrl+P has no menu item, the menu bar keeps its own accelerators next to this table
	wxAcceleratorEntry accelEntries[1];
	accelEntries[0].Set(wxACCEL_CTRL, (int)'P', XRCID("presetToggleAccel"));
	wxAcceleratorTable accelTable(1, accelEntries);
	SetAcceleratorTable(accelTable);

	project = new OutfitProject(skeleton, &projectHost); // Create empty project
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
	animation->Pause();

	if (!CheckPendingChanges())
		return;

	if (editUV)
		editUV->Close();

	if (archiveBrowser)
		archiveBrowser->Close();

	// Before the project goes. Calls from agents that wait for this thread are let through first.
	StopMcpServer();
	dispatcher->Shutdown();

	if (project) {
		physicsPreview->Shutdown();
		delete project;
		project = nullptr;
	}

	sliderPool.Clear();

	OutfitStudioConfig.SetValue("BrushSettings.brushsize", glView->GetBrushSize());
	OutfitStudioConfig.SetBoolValue("BrushSettings.accumulateSmoothWeight", glView->GetToolOptionAccumulateSmoothWeight());
	OutfitStudioConfig.SetBoolValue("BrushSettings.accumulateSmoothMask", glView->GetToolOptionAccumulateSmoothMask());

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

bool OutfitStudioFrame::StartMcpServer(int port, bool showError) {
	if (!mcp)
		mcp = std::make_unique<OutfitStudioMcp>(session);

	if (!mcp->IsRunning()) {
		if (port <= 0)
			port = OutfitStudioConfig.GetIntValue("MCP/Port", McpDefaultPort);

		std::string error;
		if (!mcp->Start(port, OutfitStudioConfig["MCP/Token"], error)) {
			if (showError) {
				wxLogError("MCP server: %s", error);
				wxMessageBox(wxString::Format(_("The MCP server couldn't start:\n%s"), error), _("MCP Server"), wxOK | wxICON_ERROR, this);
			}
			else {
				wxLogWarning("MCP server: %s", error);
				if (statusBar)
					statusBar->SetStatusText(_("The MCP server couldn't start, its port is in use."));
			}
			UpdateMcpMenu();
			return false;
		}

		if (statusBar)
			statusBar->SetStatusText(wxString::Format(_("MCP server: %s"), mcp->GetUrl()));
	}

	UpdateMcpMenu();
	return true;
}

void OutfitStudioFrame::StopMcpServer() {
	if (mcp && mcp->IsRunning()) {
		mcp->Stop();
		if (statusBar)
			statusBar->SetStatusText(_("MCP server stopped."));
	}

	UpdateMcpMenu();
}

void OutfitStudioFrame::UpdateMcpMenu() {
	if (menuBar && menuBar->FindItem(XRCID("fileMcpServer")))
		menuBar->Check(XRCID("fileMcpServer"), mcp && mcp->IsRunning());
}

void OutfitStudioFrame::OnMcpServer(wxCommandEvent& event) {
	if (event.IsChecked())
		StartMcpServer();
	else
		StopMcpServer();
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
			AppSetup::SupportedLangs.data(),
			AppSetup::SupportedLangs.size(),
			commonControls);

		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);

		XRCCTRL(*settings, "cbPreviewAlwaysDetached", wxCheckBox)->Hide();
		XRCCTRL(*settings, "cbPreviewOnLeft", wxCheckBox)->Hide();
		XRCCTRL(*settings, "cbConformAddOnSliders", wxCheckBox)->Hide();

		wxChoice* choiceSingleInstanceBehavior = XRCCTRL(*settings, "choiceSingleInstanceBehavior", wxChoice);
		choiceSingleInstanceBehavior->SetSelection(OutfitStudioConfig.GetIntValue("SingleInstanceBehavior", 0));

		wxCheckBox* cbMcpServer = XRCCTRL(*settings, "cbMcpServer", wxCheckBox);
		wxSpinCtrl* spinMcpPort = XRCCTRL(*settings, "spinMcpPort", wxSpinCtrl);
		const int mcpPort = OutfitStudioConfig.GetIntValue("MCP/Port", McpDefaultPort);
		const bool mcpEnabled = OutfitStudioConfig.GetBoolValue("MCP/Enabled");
		cbMcpServer->SetValue(mcpEnabled);
		spinMcpPort->SetValue(mcpPort);

		AppSetup::FillDataFileList(commonControls.dataFileList, gameDataPath, Config.GetIntValue("TargetGame"));

		commonControls.choiceTargetGame->Bind(wxEVT_CHOICE, &AppSetup::OnChooseTargetGame);

		const std::vector<std::string> extraFoldersBefore = GameUtil::GetExtraDataFolders(Config.GetIntValue("TargetGame"));
		if (settings->ShowModal() == wxID_OK) {
			int targetGameSelection = 0;
			bool needsRestart = false;
			SettingsDialogShared::SaveCommonSettingsDialog(
				Config,
				OutfitStudioConfig,
				GameUtil::TargetGames.data(),
				GameUtil::TargetGames.size(),
				AppSetup::SupportedLangs.data(),
				AppSetup::SupportedLangs.size(),
				commonControls,
				[]() { wxGetApp().InitLanguage(); },
				targetGameSelection,
				needsRestart);

			OutfitStudioConfig.SetValue("SingleInstanceBehavior", choiceSingleInstanceBehavior->GetSelection());
			OutfitStudioConfig.SetBoolValue("MCP/Enabled", cbMcpServer->IsChecked());
			OutfitStudioConfig.SetValue("MCP/Port", spinMcpPort->GetValue());
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

			// Textures and materials may be found in other folders now
			glView->gls.GetResourceLoader()->RetryMissingTextures();
			if (GameUtil::GetExtraDataFolders(Config.GetIntValue("TargetGame")) != extraFoldersBefore) {
				project->SetTextures();
				MeshesFromProj(true);
				glView->Render();
			}

			// Turning the server off stops it, a running server moves to the new port and turning it on starts it right away.
			// A server started from the command line while the setting is off keeps running.
			const bool mcpRunning = mcp && mcp->IsRunning();
			if (mcpRunning && mcpEnabled && !cbMcpServer->IsChecked())
				StopMcpServer();
			else if (mcpRunning && spinMcpPort->GetValue() != mcpPort) {
				StopMcpServer();
				StartMcpServer();
			}
			else if (!mcpRunning && cbMcpServer->IsChecked())
				StartMcpServer();

			// The browser refers to the archives that were just reloaded
			if (archiveBrowser)
				archiveBrowser->Rebuild();

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

std::string OutfitStudioFrame::GetBusyReason() {
#ifdef __WXMSW__
	// Modal dialogs and native message boxes disable the window they belong to
	if (!::IsWindowEnabled(static_cast<HWND>(GetHandle())))
		return "A dialog or message box is open.";
#endif

	for (wxWindow* window : wxTopLevelWindows) {
		auto dialog = dynamic_cast<wxDialog*>(window);
		if (dialog && dialog->IsModal())
			return "The dialog '" + std::string(dialog->GetTitle().ToUTF8().data()) + "' is open.";
	}

	if (glView && glView->IsStrokeActive())
		return "A brush stroke or transform is in progress.";

	if (animation->IsPlaying())
		return "An animation is playing.";

	if (!progressStack.empty())
		return "Another operation is running.";

	return {};
}

void OutfitStudioFrame::RestoreEditSnapshot(const EditSnapshot& snapshot) {
	if (editor.bEditSlider)
		ExitSliderEdit();

	// Meshes and selection refer to shapes of the work NIF that is about to be replaced
	for (auto& shape : project->GetWorkNif()->GetShapes())
		glView->DeleteMesh(shape->name.get());

	activeItem = nullptr;
	selectedItems.clear();
	editor.undoHistory.ClearHistory();
	UpdateUndoTools();

	ProjectOperations ops(*project, editor);
	ApplyRefresh(ops.RestoreSnapshot(snapshot));
	ops.RestoreMasks(snapshot);
	glView->Render();
}

void OutfitStudioFrame::ApplyRefresh(RefreshFlags flags) {
	if (HasRefresh(flags, RefreshFlags::ShapeList))
		RefreshGUIFromProj();

	if (HasRefresh(flags, RefreshFlags::SliderSet))
		CreateSetSliders();

	if (HasRefresh(flags, RefreshFlags::SliderValues))
		SyncSliderPanels();

	if (HasRefresh(flags, RefreshFlags::Meshes))
		ApplySliders(true);

	if (HasRefresh(flags, RefreshFlags::SliderData))
		HighlightSliderData();

	if (HasRefresh(flags, RefreshFlags::Anim))
		UpdateAnimationGUI();

	if (HasRefresh(flags, RefreshFlags::Shaders))
		glView->gls.SetDisplaySettings(glView->gls.GetDisplaySettings());

	if (HasRefresh(flags, RefreshFlags::WeightDisplay)) {
		HighlightBoneNamesWithWeights();
		UpdateBoneCounts();
	}

	if (HasRefresh(flags, RefreshFlags::Pose)) {
		posePanel->ShowProjectPoseState();

		posePanel->PoseToGUI();
		physicsPreview->UpdateState();
		ApplyPose();
	}

	if (HasRefresh(flags, RefreshFlags::PendingChanges))
		SetPendingChanges();
}

bool OutfitStudioFrame::CheckEditableState() {
	if (!activeItem)
		return false;

	ToolID activeTool = glView->GetActiveTool();

	// These brushes do not modify slider morph data, so they are always
	// editable regardless of slider state.
	if (activeTool == ToolID::MaskBrush || activeTool == ToolID::WeightBrush || activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
		return true;

	if (editor.bEditSlider) {
		if (project->SliderValue(editor.activeSlider) == 0.0) {
			int response = wxMessageBox(_("You are trying to edit a slider's morph with that slider set to zero.  Do you wish to set the slider to one now?"),
										wxMessageBoxCaptionStr,
										wxYES_NO,
										this);

			if (response == wxYES) {
				SetSliderValue(editor.activeSlider, 100);
				ApplySliders();
			}

			return false;
		}

		std::vector<NiShape*> shapes;
		for (auto* item : selectedItems)
			if (item)
				shapes.push_back(item->GetShape());

		if (!ConfirmSliderDataLocalForEdit(shapes, std::vector<std::string>{editor.activeSlider}))
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

void OutfitStudioFrame::StartProgress(const wxString& msg) {
	if (progressStack.empty()) {
		progressVal = 0;
		progressStack.emplace_back(0, 10000);

		wxRect rect;
		statusBar->GetFieldRect(1, rect);

		wxBeginBusyCursor();
		progressBar = new wxGauge(statusBar, wxID_ANY, 10000, rect.GetPosition(), rect.GetSize());

		if (msg.IsEmpty())
			statusBar->SetStatusText(_("Starting..."));
		else
			statusBar->SetStatusText(msg);

		if (progressListener)
			progressListener(0, msg);
	}
}

void OutfitStudioFrame::StartSubProgress(int min, int max) {
	if (progressStack.empty())
		return;

	int range = progressStack.back().second - progressStack.back().first;
	float mindiv = min / 100.0f;
	float maxdiv = max / 100.0f;
	int minoff = mindiv * range + 1;
	int maxoff = maxdiv * range + 1;
	progressStack.emplace_back(minoff + progressStack.back().first, maxoff + progressStack.back().first);
}

void OutfitStudioFrame::EndProgress(const wxString& msg, bool forceEmpty) {
	if (progressStack.empty())
		return;

	if (forceEmpty) {
		progressBar->SetValue(progressStack.front().second);
		progressStack.clear();
	}
	else {
		progressBar->SetValue(progressStack.back().second);
		progressStack.pop_back();
	}

	if (progressStack.empty()) {
		if (msg.IsEmpty())
			statusBar->SetStatusText(_("Ready!"));
		else
			statusBar->SetStatusText(msg);

		if (progressListener)
			progressListener(10000, msg);

		delete progressBar;
		progressBar = nullptr;
		wxEndBusyCursor();
	}
}

void OutfitStudioFrame::UpdateProgress(int val, const wxString& msg) {
	if (progressStack.empty())
		return;

	int range = progressStack.back().second - progressStack.back().first;
	float div = val / 100.0f;
	int offset = range * div + 1;

	progressVal = progressStack.back().first + offset;
	if (progressVal > 10000)
		progressVal = 10000;

	statusBar->SetStatusText(msg);
	progressBar->SetValue(progressVal);

	if (progressListener)
		progressListener(progressVal, msg);
}

void OutfitStudioFrame::UpdateUndoTools() {
	auto undoHistory = glView->GetUndoHistory();
	toolBarH->EnableTool(XRCID("editUndo"), undoHistory->CanUndo());
	toolBarH->EnableTool(XRCID("editRedo"), undoHistory->CanRedo());
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

void OutfitStudioFrame::OnUndo(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetSegmentMode())
		return;

	glView->UndoStroke();
	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnRedo(wxCommandEvent& WXUNUSED(event)) {
	if (glView->GetSegmentMode())
		return;

	glView->RedoStroke();
	UpdateUndoTools();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDiscord(wxCommandEvent& WXUNUSED(event)) {
	wxString url = "https://discordapp.com/invite/2qg2Rzw";
	wxLaunchDefaultBrowser(url);
}

void OutfitStudioFrame::OnGitHub(wxCommandEvent& WXUNUSED(event)) {
	wxString url = "https://github.com/ousnius/BodySlide-and-Outfit-Studio";
	wxLaunchDefaultBrowser(url);
}

void OutfitStudioFrame::OnPayPal(wxCommandEvent& WXUNUSED(event)) {
	wxString url = "https://www.paypal.me/ousnius";
	wxLaunchDefaultBrowser(url);
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
