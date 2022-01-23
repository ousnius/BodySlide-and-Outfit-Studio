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
#include "../files/TriFile.h"
#include "../ui/wxBrushSettingsPopup.h"
#include "../utils/ConfigDialogUtil.h"
#include "../utils/PlatformUtil.h"
#include "EditUV.h"
#include "GroupManager.h"
#include "PresetSaveDialog.h"
#include "ShapeProperties.h"
#include "SliderDataImportDialog.h"

#include <sstream>
#include <wx/debugrpt.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "ConvertBodyReferenceDialog.h"

using namespace nifly;

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(OutfitStudioFrame, wxFrame)
	EVT_CLOSE(OutfitStudioFrame::OnClose)
	EVT_MENU(XRCID("fileExit"), OutfitStudioFrame::OnExit)

	EVT_MENU(wxID_ANY, OutfitStudioFrame::OnMenuItem)
	EVT_MENU(XRCID("packProjects"), OutfitStudioFrame::OnPackProjects)
	EVT_MENU(XRCID("fileSettings"), OutfitStudioFrame::OnSettings)
	EVT_MENU(XRCID("btnNewProject"), OutfitStudioFrame::OnNewProject)
	EVT_MENU(XRCID("btnLoadProject"), OutfitStudioFrame::OnLoadProject)
	EVT_MENU(XRCID("btnAddProject"), OutfitStudioFrame::OnAddProject)
	EVT_MENU(XRCID("fileLoadRef"), OutfitStudioFrame::OnLoadReference)
	EVT_MENU(XRCID("fileConvBodyRef"), OutfitStudioFrame::OnConvertBodyReference)
	EVT_MENU(XRCID("fileLoadOutfit"), OutfitStudioFrame::OnLoadOutfit)
	EVT_MENU(XRCID("fileSave"), OutfitStudioFrame::OnSaveSliderSet)
	EVT_MENU(XRCID("fileSaveAs"), OutfitStudioFrame::OnSaveSliderSetAs)
	EVT_MENU(XRCID("fileUnload"), OutfitStudioFrame::OnUnloadProject)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("masksPane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_CHOICE(XRCID("cMaskName"), OutfitStudioFrame::OnSelectMask)
	EVT_BUTTON(XRCID("saveMask"), OutfitStudioFrame::OnSaveMask)
	EVT_BUTTON(XRCID("saveAsMask"), OutfitStudioFrame::OnSaveAsMask)
	EVT_BUTTON(XRCID("deleteMask"), OutfitStudioFrame::OnDeleteMask)

	EVT_COLLAPSIBLEPANE_CHANGED(XRCID("posePane"), OutfitStudioFrame::OnPaneCollapse)
	EVT_CHOICE(XRCID("cPoseBone"), OutfitStudioFrame::OnPoseBoneChanged)
	EVT_COMMAND_SCROLL(XRCID("rxPoseSlider"), OutfitStudioFrame::OnRXPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("ryPoseSlider"), OutfitStudioFrame::OnRYPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("rzPoseSlider"), OutfitStudioFrame::OnRZPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("txPoseSlider"), OutfitStudioFrame::OnTXPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("tyPoseSlider"), OutfitStudioFrame::OnTYPoseSlider)
	EVT_COMMAND_SCROLL(XRCID("tzPoseSlider"), OutfitStudioFrame::OnTZPoseSlider)
	EVT_TEXT(XRCID("rxPoseText"), OutfitStudioFrame::OnRXPoseTextChanged)
	EVT_TEXT(XRCID("ryPoseText"), OutfitStudioFrame::OnRYPoseTextChanged)
	EVT_TEXT(XRCID("rzPoseText"), OutfitStudioFrame::OnRZPoseTextChanged)
	EVT_TEXT(XRCID("txPoseText"), OutfitStudioFrame::OnTXPoseTextChanged)
	EVT_TEXT(XRCID("tyPoseText"), OutfitStudioFrame::OnTYPoseTextChanged)
	EVT_TEXT(XRCID("tzPoseText"), OutfitStudioFrame::OnTZPoseTextChanged)
	EVT_BUTTON(XRCID("resetBonePose"), OutfitStudioFrame::OnResetBonePose)
	EVT_BUTTON(XRCID("resetAllPose"), OutfitStudioFrame::OnResetAllPose)
	EVT_BUTTON(XRCID("poseToMesh"), OutfitStudioFrame::OnPoseToMesh)
	EVT_CHECKBOX(XRCID("cbPose"), OutfitStudioFrame::OnPoseCheckBox)

	EVT_CHOICE(XRCID("cPoseName"), OutfitStudioFrame::OnSelectPose)
	EVT_BUTTON(XRCID("savePose"), OutfitStudioFrame::OnSavePose)
	EVT_BUTTON(XRCID("saveAsPose"), OutfitStudioFrame::OnSaveAsPose)
	EVT_BUTTON(XRCID("deletePose"), OutfitStudioFrame::OnDeletePose)

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
	EVT_MENU(XRCID("sliderImportNIF"), OutfitStudioFrame::OnSliderImportNIF)
	EVT_MENU(XRCID("sliderImportBSD"), OutfitStudioFrame::OnSliderImportBSD)
	EVT_MENU(XRCID("sliderImportOBJ"), OutfitStudioFrame::OnSliderImportOBJ)
	EVT_MENU(XRCID("sliderImportFBX"), OutfitStudioFrame::OnSliderImportFBX)
	EVT_MENU(XRCID("sliderImportOSD"), OutfitStudioFrame::OnSliderImportOSD)
	EVT_MENU(XRCID("sliderImportTRI"), OutfitStudioFrame::OnSliderImportTRI)
	EVT_MENU(XRCID("sliderExportNIF"), OutfitStudioFrame::OnSliderExportNIF)
	EVT_MENU(XRCID("sliderExportBSD"), OutfitStudioFrame::OnSliderExportBSD)
	EVT_MENU(XRCID("sliderExportOBJ"), OutfitStudioFrame::OnSliderExportOBJ)
	EVT_MENU(XRCID("sliderExportOSD"), OutfitStudioFrame::OnSliderExportOSD)
	EVT_MENU(XRCID("sliderExportTRI"), OutfitStudioFrame::OnSliderExportTRI)
	EVT_MENU(XRCID("sliderExportToOBJs"), OutfitStudioFrame::OnSliderExportToOBJs)
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
	EVT_MENU(XRCID("btnUndiffBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnWeightBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnColorBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnAlphaBrush"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnCollapseVertex"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnFlipEdgeTool"), OutfitStudioFrame::OnSelectTool)
	EVT_MENU(XRCID("btnSplitEdgeTool"), OutfitStudioFrame::OnSelectTool)

	EVT_MENU(XRCID("btnViewFront"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewBack"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewLeft"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewRight"), OutfitStudioFrame::OnSetView)
	EVT_MENU(XRCID("btnViewPerspective"), OutfitStudioFrame::OnTogglePerspective)
	EVT_MENU(XRCID("btnToggleRotationCenter"), OutfitStudioFrame::OnToggleRotationCenter)

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
	EVT_MENU(XRCID("btnSmoothSeams"), OutfitStudioFrame::OnSmoothNormalSeams)
	EVT_MENU(XRCID("btnLockNormals"), OutfitStudioFrame::OnLockNormals)

	EVT_MENU(XRCID("btnToggleVisibility"), OutfitStudioFrame::OnToggleVisibility)
	EVT_MENU(XRCID("btnShowWireframe"), OutfitStudioFrame::OnShowWireframe)
	EVT_MENU(XRCID("btnEnableLighting"), OutfitStudioFrame::OnEnableLighting)
	EVT_MENU(XRCID("btnEnableTextures"), OutfitStudioFrame::OnEnableTextures)
	EVT_MENU(XRCID("btnEnableVertexColors"), OutfitStudioFrame::OnEnableVertexColors)

	EVT_MENU(XRCID("uvEdit"), OutfitStudioFrame::OnEditUV)
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
	EVT_MENU(XRCID("copyGeo"), OutfitStudioFrame::OnCopyGeo)
	EVT_MENU(XRCID("copyShape"), OutfitStudioFrame::OnDupeShape)
	EVT_MENU(XRCID("deleteShape"), OutfitStudioFrame::OnDeleteShape)
	EVT_MENU(XRCID("addBone"), OutfitStudioFrame::OnAddBone)
	EVT_MENU(XRCID("addCustomBone"), OutfitStudioFrame::OnAddCustomBone)
	EVT_MENU(XRCID("deleteBone"), OutfitStudioFrame::OnDeleteBone)
	EVT_MENU(XRCID("deleteBoneSelected"), OutfitStudioFrame::OnDeleteBoneFromSelected)
	EVT_MENU(XRCID("editBone"), OutfitStudioFrame::OnEditBone)
	EVT_MENU(XRCID("copyBoneWeight"), OutfitStudioFrame::OnCopyBoneWeight)
	EVT_MENU(XRCID("copySelectedWeight"), OutfitStudioFrame::OnCopySelectedWeight)
	EVT_MENU(XRCID("transferSelectedWeight"), OutfitStudioFrame::OnTransferSelectedWeight)
	EVT_MENU(XRCID("maskWeightedVerts"), OutfitStudioFrame::OnMaskWeighted)
	EVT_MENU(XRCID("maskBoneWeightedVerts"), OutfitStudioFrame::OnMaskBoneWeighted)
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
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("outfitBones"), OutfitStudioFrame::OnBoneTreeContext)

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

	EVT_TREE_SEL_CHANGED(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionSelect)
	EVT_TREE_ITEM_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionContext)
	EVT_COMMAND_RIGHT_CLICK(XRCID("partitionTree"), OutfitStudioFrame::OnPartitionTreeContext)
	EVT_MENU(XRCID("addPartition"), OutfitStudioFrame::OnAddPartition)
	EVT_MENU(XRCID("deletePartition"), OutfitStudioFrame::OnDeletePartition)
	EVT_CHOICE(XRCID("partitionType"), OutfitStudioFrame::OnPartitionTypeChanged)
	EVT_BUTTON(XRCID("partitionApply"), OutfitStudioFrame::OnPartitionApply)
	EVT_BUTTON(XRCID("partitionReset"), OutfitStudioFrame::OnPartitionReset)

	EVT_SLIDER(XRCID("boneScale"), OutfitStudioFrame::OnBoneScaleSlider)

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

	EVT_SLIDER(XRCID("lightAmbientSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightFrontalSlider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional0Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional1Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_SLIDER(XRCID("lightDirectional2Slider"), OutfitStudioFrame::OnUpdateLights)
	EVT_BUTTON(XRCID("lightReset"), OutfitStudioFrame::OnResetLights)

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

const wxString TargetGames[] = {"Fallout3", "FalloutNewVegas", "Skyrim", "Fallout4", "SkyrimSpecialEdition", "Fallout4VR", "SkyrimVR", "Fallout 76", "Oblivion"};
const wxLanguage SupportedLangs[] = {wxLANGUAGE_ENGLISH,	wxLANGUAGE_AFRIKAANS,  wxLANGUAGE_ARABIC,	  wxLANGUAGE_CATALAN,		   wxLANGUAGE_CZECH,	 wxLANGUAGE_DANISH,
									 wxLANGUAGE_GERMAN,		wxLANGUAGE_GREEK,	   wxLANGUAGE_SPANISH,	  wxLANGUAGE_BASQUE,		   wxLANGUAGE_FINNISH,	 wxLANGUAGE_FRENCH,
									 wxLANGUAGE_HINDI,		wxLANGUAGE_HUNGARIAN,  wxLANGUAGE_INDONESIAN, wxLANGUAGE_ITALIAN,		   wxLANGUAGE_JAPANESE,	 wxLANGUAGE_KOREAN,
									 wxLANGUAGE_LITHUANIAN, wxLANGUAGE_LATVIAN,	   wxLANGUAGE_MALAY,	  wxLANGUAGE_NORWEGIAN_BOKMAL, wxLANGUAGE_NEPALI,	 wxLANGUAGE_DUTCH,
									 wxLANGUAGE_POLISH,		wxLANGUAGE_PORTUGUESE, wxLANGUAGE_ROMANIAN,	  wxLANGUAGE_RUSSIAN,		   wxLANGUAGE_SLOVAK,	 wxLANGUAGE_SLOVENIAN,
									 wxLANGUAGE_ALBANIAN,	wxLANGUAGE_SWEDISH,	   wxLANGUAGE_TAMIL,	  wxLANGUAGE_TURKISH,		   wxLANGUAGE_UKRAINIAN, wxLANGUAGE_VIETNAMESE,
									 wxLANGUAGE_CHINESE};

std::string GetProjectPath() {
	std::string res = Config["ProjectPath"];
	return res.empty() ? Config["AppDir"] : res;
}


OutfitStudio::~OutfitStudio() {
	delete locale;
	locale = nullptr;

	FSManager::del();
}

bool OutfitStudio::OnInit() {
	if (!wxApp::OnInit())
		return false;

#ifdef _DEBUG
	std::string dataDir{wxGetCwd().ToUTF8()};
#else
	std::string dataDir{wxStandardPaths::Get().GetDataDir().ToUTF8()};
#endif

	Config.LoadConfig(dataDir + "/Config.xml");
	OutfitStudioConfig.LoadConfig(dataDir + "/OutfitStudio.xml", "OutfitStudioConfig");

	Config.SetDefaultValue("AppDir", dataDir);

	logger.Initialize(Config.GetIntValue("LogLevel", -1), dataDir + "/Log_OS.txt");
	wxLogMessage("Initializing Outfit Studio...");

#ifdef NDEBUG
	wxHandleFatalExceptions();
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

	for (auto& file : cmdFiles) {
		wxFileName loadFile(file);
		if (loadFile.FileExists()) {
			std::string fileName{loadFile.GetFullPath().ToUTF8()};
			wxString fileExt = loadFile.GetExt().MakeLower();
			if (fileExt == "osp") {
				std::string projectName{cmdProject.ToUTF8()};
				frame->LoadProject(fileName, projectName);
				break;
			}
			else if (fileExt == "nif") {
				frame->StartProgress(_("Adding NIF file..."));
				frame->UpdateProgress(1, _("Adding NIF file..."));
				frame->project->ImportNIF(fileName, false);
				frame->project->SetTextures();

				frame->UpdateProgress(60, _("Refreshing GUI..."));
				frame->RefreshGUIFromProj();

				frame->UpdateProgress(100, _("Finished"));
				frame->EndProgress();
			}
			else if (fileExt == "obj") {
				frame->StartProgress("Adding OBJ file...");
				frame->UpdateProgress(1, _("Adding OBJ file..."));
				frame->project->ImportOBJ(fileName);
				frame->project->SetTextures();

				frame->UpdateProgress(60, _("Refreshing GUI..."));
				frame->RefreshGUIFromProj();

				frame->UpdateProgress(100, _("Finished"));
				frame->EndProgress();
			}
			else if (fileExt == "fbx") {
				frame->StartProgress(_("Adding FBX file..."));
				frame->UpdateProgress(1, _("Adding FBX file..."));
				frame->project->ImportFBX(fileName);
				frame->project->SetTextures();

				frame->UpdateProgress(60, _("Refreshing GUI..."));
				frame->RefreshGUIFromProj();

				frame->UpdateProgress(100, _("Finished"));
				frame->EndProgress();
			}
		}
	}

	Bind(wxEVT_CHAR_HOOK, &OutfitStudio::CharHook, this);

	wxLogMessage("Outfit Studio initialized.");
	return true;
}

void OutfitStudio::OnInitCmdLine(wxCmdLineParser& parser) {
	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars("-");
}

bool OutfitStudio::OnCmdLineParsed(wxCmdLineParser& parser) {
	parser.Found("proj", &cmdProject);

	for (size_t i = 0; i < parser.GetParamCount(); i++)
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
	int keyCode = event.GetKeyCode();
	bool letterHack = (keyCode > 0x40 && keyCode < 0x5B) || (keyCode > 0x60 && keyCode < 0x7B);
	if (letterHack && !event.HasModifiers()) {
		if ((!event.ShiftDown() && !wxGetKeyState(wxKeyCode::WXK_CAPITAL)) || (event.ShiftDown() && wxGetKeyState(wxKeyCode::WXK_CAPITAL))) {
			keyCode += 32;
		}

		auto searchCtrl = dynamic_cast<wxTextCtrl*>(w);
		if (searchCtrl) {
			HWND hwndEdit = searchCtrl->GetHandle();
			::SendMessage(hwndEdit, WM_CHAR, keyCode, event.GetRawKeyFlags());
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
	Config.SetDefaultBoolValue("Input/LeftMousePan", false);
	Config.SetDefaultBoolValue("Input/BrushSettingsNearCursor", true);
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
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.width", 1150);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.height", 780);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.x", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.y", 100);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashpos", 768);
	OutfitStudioConfig.SetDefaultValue("OutfitStudioFrame.sashrightpos", 200);

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

	wxString gameKey = Config["GameRegKey/" + TargetGames[targetGame]];
	wxString gameValueKey = Config["GameRegVal/" + TargetGames[targetGame]];

	if (Config["GameDataPath"].empty()) {
#ifdef _WINDOWS
		wxRegKey key(wxRegKey::HKLM, gameKey, wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
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
		setup->SetSize(wxSize(700, -1));
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

		wxFileName dir = GetGameDataPath(OB);
		if (dir.DirExists()) {
			dirOblivion->SetDirName(dir);
			btOblivion->Enable();
		}

		dir = GetGameDataPath(FO3);
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
			}

			Config.SetValue("GameDataPath", dataDir.GetFullPath().ToUTF8().data());
			Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), dataDir.GetFullPath().ToUTF8().data());

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

wxString OutfitStudio::GetGameDataPath(TargetGame targ) {
	wxString dataPath;
	wxString gamestr = TargetGames[targ];
	wxString gkey = "GameRegKey/" + gamestr;
	wxString gval = "GameRegVal/" + gamestr;
	wxString cust = "GameDataPaths/" + gamestr;

	if (!Config[cust].IsEmpty()) {
		dataPath = Config[cust];
	}
#ifdef _WINDOWS
	else {
		wxRegKey key(wxRegKey::HKLM, Config[gkey], wxRegKey::WOW64ViewMode_32);
		if (key.Exists()) {
			if (key.HasValues() && key.QueryValue(Config[gval], dataPath)) {
				dataPath.Append("Data").Append(PathSepChar);
			}
		}
	}
#endif
	return dataPath;
}

void OutfitStudio::InitLanguage() {
	if (locale)
		delete locale;

	int lang = Config.GetIntValue("Language");

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
		f = f.AfterLast('/').AfterLast('\\');
		if (fsearch.find(f) == fsearch.end())
			outList.push_back((dataDir + f).ToUTF8().data());
	}
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
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Slider.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Skeleton.xrc");
	xrc->Load(wxString::FromUTF8(Config["AppDir"]) + "/res/xrc/Settings.xrc");

	int statusWidths[] = {-1, 400, 100};
	statusBar = (wxStatusBar*)FindWindowByName("statusBar");
	if (statusBar) {
		statusBar->SetFieldsCount(3);
		statusBar->SetStatusWidths(3, statusWidths);
		statusBar->SetStatusText(_("Ready!"));
	}

	this->DragAcceptFiles(true);

	menuBar = xrc->LoadMenuBar(this, "menuBar");

	std::vector<std::string> projectHistoryFiles;
	OutfitStudioConfig.GetValueAttributeArray("ProjectHistory", "Project", "file", projectHistoryFiles);
	std::vector<std::string> projectHistoryNames;
	OutfitStudioConfig.GetValueAttributeArray("ProjectHistory", "Project", "name", projectHistoryNames);

	if (projectHistoryFiles.size() == projectHistoryNames.size())
		for (size_t i = projectHistoryFiles.size(); i > 0; --i)
			AddProjectHistory(projectHistoryFiles[i - 1], projectHistoryNames[i - 1]);

	toolBarH = (wxToolBar*)FindWindowByName("toolBarH");
	toolBarV = (wxToolBar*)FindWindowByName("toolBarV");

	if (toolBarH) {
		brushSettings = reinterpret_cast<wxButton*>(toolBarH->FindWindowByName("brushSettings"));
		if (brushSettings)
			brushSettings->Bind(wxEVT_BUTTON, &OutfitStudioFrame::OnBrushSettings, this);

		fovSlider = reinterpret_cast<wxSlider*>(toolBarH->FindWindowByName("fovSlider"));
		if (fovSlider)
			fovSlider->Bind(wxEVT_SLIDER, &OutfitStudioFrame::OnFieldOfViewSlider, this);
	}

	sliderScroll = (wxScrolledWindow*)FindWindowByName("sliderScroll");
	bmpEditSlider = new wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/EditSmall.png", wxBITMAP_TYPE_ANY);
	bmpSliderSettings = new wxBitmap(wxString::FromUTF8(Config["AppDir"]) + "/res/images/Settings.png", wxBITMAP_TYPE_ANY);

	meshTabButton = (wxStateButton*)FindWindowByName("meshTabButton");
	boneTabButton = (wxStateButton*)FindWindowByName("boneTabButton");
	colorsTabButton = (wxStateButton*)FindWindowByName("colorsTabButton");
	segmentTabButton = (wxStateButton*)FindWindowByName("segmentTabButton");
	partitionTabButton = (wxStateButton*)FindWindowByName("partitionTabButton");
	lightsTabButton = (wxStateButton*)FindWindowByName("lightsTabButton");
	masksPane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("masksPane"));
	posePane = dynamic_cast<wxCollapsiblePane*>(FindWindowByName("posePane"));

	if (meshTabButton) {
		meshTabButton->SetCheck();
		currentTabButton = meshTabButton;
	}

	if (wxGetApp().targetGame != FO4 && wxGetApp().targetGame != FO4VR && wxGetApp().targetGame != FO76) {
		if (segmentTabButton)
			segmentTabButton->Show(false);
	}
	else {
		if (partitionTabButton)
			partitionTabButton->Show(false);
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
	}

	outfitBones = (wxTreeCtrl*)FindWindowByName("outfitBones");
	if (outfitBones) {
		wxBitmap noneImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoNone.png", wxBITMAP_TYPE_PNG);
		wxBitmap changeImg(wxString::FromUTF8(Config["AppDir"]) + "/res/images/icoChange.png", wxBITMAP_TYPE_PNG);
		wxImageList* boneStateImages = new wxImageList(16, 16, false, 2);
		if (noneImg.IsOk())
			boneStateImages->Add(noneImg);
		if (changeImg.IsOk())
			boneStateImages->Add(changeImg);
		outfitBones->AssignStateImageList(boneStateImages);
		bonesRoot = outfitBones->AddRoot("Bones");
	}

	colorSettings = (wxPanel*)FindWindowByName("colorSettings");

	segmentTree = (wxTreeCtrl*)FindWindowByName("segmentTree");
	if (segmentTree)
		segmentRoot = segmentTree->AddRoot("Segments");

	partitionTree = (wxTreeCtrl*)FindWindowByName("partitionTree");
	if (partitionTree)
		partitionRoot = partitionTree->AddRoot("Partitions");

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
	}

	auto editPanel = (wxPanel*)FindWindowByName("editPanel");
	if (editPanel)
		editPanel->SetBackgroundColour(wxColour(112, 112, 112));

	boneScale = (wxSlider*)FindWindowByName("boneScale");
	cXMirrorBone = (wxChoice*)FindWindowByName("cXMirrorBone");
	cPoseBone = (wxChoice*)FindWindowByName("cPoseBone");
	rxPoseSlider = (wxSlider*)FindWindowByName("rxPoseSlider");
	ryPoseSlider = (wxSlider*)FindWindowByName("ryPoseSlider");
	rzPoseSlider = (wxSlider*)FindWindowByName("rzPoseSlider");
	txPoseSlider = (wxSlider*)FindWindowByName("txPoseSlider");
	tyPoseSlider = (wxSlider*)FindWindowByName("tyPoseSlider");
	tzPoseSlider = (wxSlider*)FindWindowByName("tzPoseSlider");
	rxPoseText = (wxTextCtrl*)FindWindowByName("rxPoseText");
	ryPoseText = (wxTextCtrl*)FindWindowByName("ryPoseText");
	rzPoseText = (wxTextCtrl*)FindWindowByName("rzPoseText");
	txPoseText = (wxTextCtrl*)FindWindowByName("txPoseText");
	tyPoseText = (wxTextCtrl*)FindWindowByName("tyPoseText");
	tzPoseText = (wxTextCtrl*)FindWindowByName("tzPoseText");
	cbPose = (wxCheckBox*)FindWindowByName("cbPose");

	wxWindow* leftPanel = FindWindowByName("leftSplitPanel");
	if (leftPanel) {
		glView = new wxGLPanel(leftPanel, wxDefaultSize, GLSurface::GetGLAttribs());
		glView->SetNotifyWindow(this);
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

	bEditSlider = false;
	activeItem = nullptr;
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
	if (!CheckPendingChanges())
		return;

	if (editUV)
		editUV->Close();

	if (project) {
		delete project;
		project = nullptr;
	}

	sliderPool.Clear();

	if (glView)
		delete glView;

	if (bmpEditSlider)
		delete bmpEditSlider;
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

	int ret = OutfitStudioConfig.SaveConfig(Config["AppDir"] + "/OutfitStudio.xml", "OutfitStudioConfig");
	if (ret)
		wxLogWarning("Failed to save configuration (%d)!", ret);

	wxLogMessage("Outfit Studio frame closed.");
	Destroy();
}

bool OutfitStudioFrame::CopyStreamData(wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size) {
	wxChar buf[128 * 1024];
	int readSize = 128 * 1024;
	wxFileOffset copiedData = 0;

	for (;;) {
		if (size != -1 && copiedData + readSize > size)
			readSize = size - copiedData;

		inputStream.Read(buf, readSize);

		size_t actuallyRead = inputStream.LastRead();
		outputStream.Write(buf, actuallyRead);
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
		if (projectHistory.size() > id - 1000) {
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
		auto projectFilter = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
		projectFilter->ShowSearchButton(true);
		projectFilter->SetDescriptiveText("Project Filter");
		projectFilter->SetToolTip("Filter project list by name");

		xrc->AttachUnknownControl("projectFilter", projectFilter, packProjects);

		packProjects->SetSize(wxSize(550, 300));
		packProjects->SetMinSize(wxSize(400, 200));
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
		wxDir::GetAllFiles(wxString::FromUTF8(GetProjectPath()) + "/SliderSets", &files, "*.osp");
		wxDir::GetAllFiles(wxString::FromUTF8(GetProjectPath()) + "/SliderSets", &files, "*.xml");

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
		groupFile->SetInitialDirectory(wxString::FromUTF8(GetProjectPath()) + "/SliderGroups");

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
				wxString inputFilePath = wxString::FromUTF8(GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
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
					wxString dataFilePath = wxString::FromUTF8(GetProjectPath() + sep + "ShapeData" + sep + df);
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
				wxLogError("Failed to save merged project file '%s'!", mergedFileName);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFileName), _("Error"), wxICON_ERROR);
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

			EndProgress();
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
				wxString inputFilePath = wxString::FromUTF8(GetProjectPath() + sep + "ShapeData" + sep + set.GetInputFileName());
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
					wxString dataFilePath = wxString::FromUTF8(GetProjectPath() + sep + "ShapeData" + sep + df);
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
				wxLogError("Failed to save merged project file '%s'!", mergedFileName);
				wxMessageBox(wxString::Format(_("Failed to save merged project file '%s'!"), mergedFileName), _("Error"), wxICON_ERROR);
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

			EndProgress();
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
		case FO4:
		case FO4VR:
		case FO76:
		default:
			fpSkeletonFile->SetPath("res/skeleton_fo4.nif");
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
		settings->SetSize(wxSize(525, -1));
		settings->SetMinSize(wxSize(525, -1));
		settings->CenterOnParent();

		wxCollapsiblePane* advancedPane = XRCCTRL(*settings, "advancedPane", wxCollapsiblePane);
		advancedPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [&settings](wxCommandEvent&) { settings->Fit(); });

		wxChoice* choiceTargetGame = XRCCTRL(*settings, "choiceTargetGame", wxChoice);
		choiceTargetGame->Select(Config.GetIntValue("TargetGame"));

		wxDirPickerCtrl* dpGameDataPath = XRCCTRL(*settings, "dpGameDataPath", wxDirPickerCtrl);
		wxString gameDataPath = wxString::FromUTF8(Config["GameDataPath"]);
		dpGameDataPath->SetPath(gameDataPath);

		wxDirPickerCtrl* dpOutputPath = XRCCTRL(*settings, "dpOutputPath", wxDirPickerCtrl);
		wxString outputPath = wxString::FromUTF8(Config["OutputDataPath"]);
		dpOutputPath->SetPath(outputPath);

		wxDirPickerCtrl* dpProjectPath = XRCCTRL(*settings, "dpProjectPath", wxDirPickerCtrl);
		wxString projectPath = wxString::FromUTF8(Config["ProjectPath"]);
		dpProjectPath->SetPath(projectPath);

		wxCheckBox* cbShowForceBodyNormals = XRCCTRL(*settings, "cbShowForceBodyNormals", wxCheckBox);
		cbShowForceBodyNormals->SetValue(Config.GetBoolValue("ShowForceBodyNormals"));

		wxCheckBox* cbBBOverrideWarn = XRCCTRL(*settings, "cbBBOverrideWarn", wxCheckBox);
		cbBBOverrideWarn->SetValue(Config.GetBoolValue("WarnBatchBuildOverride"));

		wxCheckBox* cbBSATextures = XRCCTRL(*settings, "cbBSATextures", wxCheckBox);
		cbBSATextures->SetValue(Config.GetBoolValue("BSATextureScan"));

		wxCheckBox* cbLeftMousePan = XRCCTRL(*settings, "cbLeftMousePan", wxCheckBox);
		cbLeftMousePan->SetValue(Config.GetBoolValue("Input/LeftMousePan"));

		wxCheckBox* cbBrushSettingsNearCursor = XRCCTRL(*settings, "cbBrushSettingsNearCursor", wxCheckBox);
		cbBrushSettingsNearCursor->SetValue(Config.GetBoolValue("Input/BrushSettingsNearCursor"));

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

		wxColourPickerCtrl* cpColorWire = XRCCTRL(*settings, "cpColorWire", wxColourPickerCtrl);
		if (Config.Exists("Rendering/ColorWire")) {
			int colorWireR = Config.GetIntValue("Rendering/ColorWire.r");
			int colorWireG = Config.GetIntValue("Rendering/ColorWire.g");
			int colorWireB = Config.GetIntValue("Rendering/ColorWire.b");
			cpColorWire->SetColour(wxColour(colorWireR, colorWireG, colorWireB));
		}

		wxFilePickerCtrl* fpSkeletonFile = XRCCTRL(*settings, "fpSkeletonFile", wxFilePickerCtrl);
		fpSkeletonFile->SetPath(wxString::FromUTF8(Config["Anim/DefaultSkeletonReference"]));

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
				Config.SetValue("GameDataPath", gameDataDir.GetFullPath().ToUTF8().data());
				Config.SetValue("GameDataPaths/" + TargetGames[targ].ToStdString(), gameDataDir.GetFullPath().ToUTF8().data());
			}

			// set OutputDataPath even if it is empty
			wxFileName outputDataDir = dpOutputPath->GetDirName();
			Config.SetValue("OutputDataPath", outputDataDir.GetFullPath().ToUTF8().data());

			// set ProjectPath even if it is empty
			wxFileName projectDir = dpProjectPath->GetDirName();
			Config.SetValue("ProjectPath", projectDir.GetFullPath().ToUTF8().data());

			wxArrayInt items;
			wxString selectedfiles;
			for (uint32_t i = 0; i < dataFileList->GetCount(); i++)
				if (!dataFileList->IsChecked(i))
					selectedfiles += dataFileList->GetString(i) + "; ";

			selectedfiles = selectedfiles.BeforeLast(';');
			Config.SetValue("GameDataFiles/" + TargetGames[targ].ToStdString(), selectedfiles.ToUTF8().data());

			Config.SetBoolValue("ShowForceBodyNormals", cbShowForceBodyNormals->IsChecked());
			Config.SetBoolValue("WarnBatchBuildOverride", cbBBOverrideWarn->IsChecked());
			Config.SetBoolValue("BSATextureScan", cbBSATextures->IsChecked());
			Config.SetBoolValue("Input/LeftMousePan", cbLeftMousePan->IsChecked());
			Config.SetBoolValue("Input/BrushSettingsNearCursor", cbBrushSettingsNearCursor->IsChecked());

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

			wxColour colorWire = cpColorWire->GetColour();
			Config.SetValue("Rendering/ColorWire.r", colorWire.Red());
			Config.SetValue("Rendering/ColorWire.g", colorWire.Green());
			Config.SetValue("Rendering/ColorWire.b", colorWire.Blue());

			wxFileName skeletonFile = fpSkeletonFile->GetFileName();
			Config.SetValue("Anim/DefaultSkeletonReference", skeletonFile.GetFullPath().ToUTF8().data());
			Config.SetValue("Anim/SkeletonRootName", choiceSkeletonRoot->GetStringSelection().ToUTF8().data());

			Config.SaveConfig(Config["AppDir"] + "/Config.xml");
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
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	std::string error = project->Save(project->mFileName,
									  project->mOutfitName,
									  project->mDataDir,
									  project->mBaseFile,
									  project->mGamePath,
									  project->mGameFile,
									  project->mGenWeights,
									  project->mCopyRef);

	if (!error.empty()) {
		wxLogError(error.c_str());
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		EndProgress();
		return false;
	}

	SetPendingChanges(false);
	EndProgress();
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
		XRCCTRL(dlg, "sssSliderSetFile", wxFilePickerCtrl)->SetInitialDirectory(wxString::FromUTF8(GetProjectPath()) + "/SliderSets");

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
	bool copyRef;
	bool genWeights;

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
		wxString dataFolder(wxString::Format("%s/%s", wxString::FromUTF8(GetProjectPath()), "ShapeData"));
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

	copyRef = XRCCTRL(dlg, "sssAutoCopyRef", wxCheckBox)->GetValue();
	genWeights = XRCCTRL(dlg, "sssGenWeightsTrue", wxRadioButton)->GetValue();

	wxLogMessage("Saving project '%s'...", strOutfitName);
	StartProgress(wxString::Format(_("Saving project '%s'..."), strOutfitName));
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	project->UpdateNifNormals(project->GetWorkNif(), shapeMeshes);

	std::string error = project->Save(sliderSetFile, strOutfitName, strDataDir, strBaseFile, strGamePath, strGameFile, genWeights, copyRef);

	if (error.empty()) {
		SetPendingChanges(false);
		menuBar->Enable(XRCID("fileSave"), true);

		RenameProject(strOutfitName.ToUTF8().data());
		EndProgress();
	}
	else {
		wxLogError(error.c_str());
		wxMessageBox(error, _("Error"), wxOK | wxICON_ERROR);
		EndProgress();
		return false;
	}

	return true;
}

bool OutfitStudioFrame::LoadProject(const std::string& fileName, const std::string& projectName, bool clearProject) {
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
		for (auto& s : setnames)
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
		error = project->AddFromSliderSet(fileName, outfit);

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

	UpdateTitle();
	AddProjectHistory(fileName, outfit);

	wxLogMessage("Project loaded.");
	UpdateProgress(100, _("Finished"));
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

	EndProgress();
}

void OutfitStudioFrame::createSliderGUI(const std::string& name, wxScrolledWindow* wnd, wxSizer* rootSz) {
	wxString sliderName = wxString::FromUTF8(name);

	wxSliderPanel* sliderPanel = sliderPool.GetNext();
	if (sliderPanel) {
		if (sliderPanel->Create(wnd, sliderName, *bmpEditSlider, *bmpSliderSettings)) {
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

			ShowSliderEffect(name);
			sliderPanels[name] = sliderPanel;
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

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	project->AddEmptySlider(sliderName);
	ShowSliderEffect(sliderName);
	sliderScroll->FitInside();
	SetPendingChanges();

	return sliderName;
}

void OutfitStudioFrame::SetSliderValue(const size_t index, int val) {
	std::string name = project->GetSliderName(index);
	project->SliderValue(index) = val / 100.0f;
	sliderPanels[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderPanels[name]->slider->SetValue(val);
}

void OutfitStudioFrame::SetSliderValue(const std::string& name, int val) {
	project->SliderValue(name) = val / 100.0f;
	sliderPanels[name]->sliderReadout->ChangeValue(wxString::Format("%d%%", val));
	sliderPanels[name]->slider->SetValue(val);
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
		menuBar->Enable(XRCID("btnLockNormals"), false);
	}
	else {
		mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
		if (m) {
			smoothSeamNormals = m->smoothSeamNormals;
			lockNormals = m->lockNormals;

			if (glView->GetTransformMode())
				glView->ShowTransformTool();
			if (glView->GetVertexEdit())
				glView->ShowVertexEdit();
		}

		menuBar->Enable(XRCID("btnSmoothSeams"), m != nullptr);
		menuBar->Enable(XRCID("btnLockNormals"), m != nullptr);

		CreateSegmentTree(activeItem->GetShape());
		CreatePartitionTree(activeItem->GetShape());
	}

	menuBar->Check(XRCID("btnSmoothSeams"), smoothSeamNormals);
	menuBar->Check(XRCID("btnLockNormals"), lockNormals);

	if (glView->rotationCenterMode == RotationCenterMode::MeshCenter)
		glView->gls.camRotOffset = glView->gls.GetActiveCenter();

	glView->UpdateBones();
	glView->Render();

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
			char abc = std::tolower(activeBone[i]);
			char bc = std::tolower(b[i]);
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
	mesh* m = glView->GetMesh(shape->name.get());
	if (m)
		project->UpdateShapeFromMesh(shape, m);
}

void OutfitStudioFrame::ActiveShapesUpdated(UndoStateProject* usp, bool bIsUndo) {
	if (!usp->sliderName.empty()) {
		float sliderscale = 1 / usp->sliderscale;
		for (auto& uss : usp->usss) {
			mesh* m = glView->GetMesh(uss.shapeName);
			if (!m)
				continue;

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
			auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(m->shapeName);
			if (shape)
				project->UpdateMorphResult(shape, usp->sliderName, strokeDiff);
		}
	}
	else {
		if (usp->undoType == UT_WEIGHT) {
			for (auto& uss : usp->usss) {
				mesh* m = glView->GetMesh(uss.shapeName);
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
		else if (usp->undoType == UT_COLOR) {
			for (auto& uss : usp->usss) {
				mesh* m = glView->GetMesh(uss.shapeName);
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
		else if (usp->undoType == UT_ALPHA) {
			for (auto& uss : usp->usss) {
				mesh* m = glView->GetMesh(uss.shapeName);
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
	selectedItems.erase(std::remove_if(selectedItems.begin(), selectedItems.end(), [&](ShapeItemData* i) { return i->GetShape() == shape; }), selectedItems.end());
}

std::string OutfitStudioFrame::GetActiveBone() {
	return activeBone;
}

void OutfitStudioFrame::HideSliderPanel(wxSliderPanel* sliderPanel) {
	if (!sliderPanel)
		return;

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

	if (sliderPanel->sliderCheck->Get3StateValue() == wxCheckBoxState::wxCHK_UNCHECKED) {
		sliderPanel->sliderCheck->Set3StateValue(wxCheckBoxState::wxCHK_CHECKED);
		ShowSliderEffect(activeSlider, true);
	}

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
	menuBar->Enable(XRCID("sliderNegate"), true);
	menuBar->Enable(XRCID("sliderMask"), true);
	menuBar->Enable(XRCID("sliderProperties"), true);
}

void OutfitStudioFrame::MenuExitSliderEdit() {
	menuBar->Enable(XRCID("menuImportSlider"), false);
	menuBar->Enable(XRCID("menuExportSlider"), false);
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
	if (tool == ToolID::Select) {
		glView->SetEditMode(false);
		glView->SetBrushMode(false);
		glView->SetActiveTool(tool);

		menuBar->Check(XRCID("btnSelect"), true);
		toolBarH->ToggleTool(XRCID("btnSelect"), true);

		menuBar->Check(XRCID("btnXMirror"), false);
		menuBar->Enable(XRCID("btnXMirror"), false);
		toolBarV->ToggleTool(XRCID("btnXMirror"), false);
		toolBarV->EnableTool(XRCID("btnXMirror"), false);
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

	auto activeBrush = glView->GetActiveBrush();
	if (activeBrush) {
		menuBar->Enable(XRCID("btnXMirror"), true);
		menuBar->Check(XRCID("btnXMirror"), activeBrush->isMirrored());
		toolBarV->EnableTool(XRCID("btnXMirror"), true);
		toolBarV->ToggleTool(XRCID("btnXMirror"), activeBrush->isMirrored());
	}
	else {
		menuBar->Enable(XRCID("btnXMirror"), false);
		toolBarV->EnableTool(XRCID("btnXMirror"), false);
	}

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
		FindWindowById(XRCID("clampMaxValue"), colorSettings)->Hide();
		colorSettings->Layout();
		wxButton* btnSwapBrush = (wxButton*)FindWindowById(XRCID("btnSwapBrush"), colorSettings);
		btnSwapBrush->SetLabel(_("Edit Alpha"));
	}
	else if (tool == ToolID::AlphaBrush) {
		menuBar->Check(XRCID("btnAlphaBrush"), true);
		toolBarH->ToggleTool(XRCID("btnAlphaBrush"), true);

		FindWindowById(XRCID("colorPalette"), colorSettings)->Hide();
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
		brushSettingsPopupTransient->SetBrushSize(glView->GetBrushSize());
		brushSettingsPopupTransient->SetBrushStrength(brush->getStrength());
		brushSettingsPopupTransient->SetBrushFocus(brush->getFocus());
		brushSettingsPopupTransient->SetBrushSpacing(brush->getSpacing());
	}
}

bool OutfitStudioFrame::CheckEditableState() {
	if (!activeItem)
		return false;

	ToolID activeTool = glView->GetActiveTool();
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

		return true;
	}
	else {
		if (activeTool == ToolID::UndiffBrush) {
			wxMessageBox(_("You can only use the undiff brush while editing a slider. Note, use the pencil button next to a slider to enable editing of that slider's morph."),
						 wxMessageBoxCaptionStr,
						 wxOK,
						 this);
			return false;
		}
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
	std::string outfitName = project->OutfitName();
	if (!outfitName.empty()) {
		if (pendingChanges)
			SetTitle(wxString::FromUTF8(outfitName) + "* - Outfit Studio");
		else
			SetTitle(wxString::FromUTF8(outfitName) + " - Outfit Studio");
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

	constexpr int MAX_PROJECT_HISTORY = 15;
	if (projectHistory.size() == MAX_PROJECT_HISTORY)
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
	wxWizardPage* pg2;
	bool result = false;

	if (!CheckPendingChanges())
		return;

	CloseBrushSettings();
	UpdateReferenceTemplates();

	if (wxXmlResource::Get()->LoadObject((wxObject*)&wiz, this, "wizNewProject", "wxWizard")) {
		pg1 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj1", wxWizardPageSimple);
		XRCCTRL(wiz, "npSliderSetFile", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnNPWizChangeSliderSetFile, this);
		XRCCTRL(wiz, "npSliderSetName", wxChoice)->Bind(wxEVT_CHOICE, &OutfitStudioFrame::OnNPWizChangeSetNameChoice, this);

		pg2 = (wxWizardPage*)XRCCTRL(wiz, "wizpgNewProj2", wxWizardPageSimple);
		XRCCTRL(wiz, "npWorkFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_File, this);
		XRCCTRL(wiz, "npTexFilename", wxFilePickerCtrl)->Bind(wxEVT_FILEPICKER_CHANGED, &OutfitStudioFrame::OnLoadOutfitFP_Texture, this);

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, wiz, "npTemplateChoice", refTemplates);

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

	delete project;
	project = new OutfitProject(this);

	UpdateProgress(10, _("Loading reference..."));

	int error = 0;
	if (XRCCTRL(wiz, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, wiz, "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(GetProjectPath() + PathSepStr + tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll());
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
								   wxString::FromUTF8(GetProjectPath()) + "/SliderSets",
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
								  wxString::FromUTF8(GetProjectPath()) + "/SliderSets",
								  wxEmptyString,
								  "Slider Set Files (*.osp;*.xml)|*.osp;*.xml",
								  wxFD_FILE_MUST_EXIST);
	if (addProjectDialog.ShowModal() == wxID_CANCEL)
		return;

	if (!CheckPendingChanges())
		return;

	std::string fileName{addProjectDialog.GetPath().ToUTF8()};
	LoadProject(fileName, "", false);
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

		ConfigDialogUtil::LoadDialogChoices(OutfitStudioConfig, dlg, "npTemplateChoice", refTemplates);

		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "chkMergeSliders");
		ConfigDialogUtil::LoadDialogCheckBox(OutfitStudioConfig, dlg, "chkMergeZaps");

		result = dlg.ShowModal();
	}
	if (result == wxID_CANCEL)
		return;

	StartProgress(_("Loading reference..."));

	NiShape* baseShape = project->GetBaseShape();
	if (baseShape)
		glView->DeleteMesh(baseShape->name.get());

	UpdateProgress(10, _("Loading reference set..."));
	bool mergeSliders = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "chkMergeSliders");
	bool mergeZaps = ConfigDialogUtil::SetBoolFromDialogCheckbox(OutfitStudioConfig, dlg, "chkMergeZaps");

	int error = 0;
	if (XRCCTRL(dlg, "npRefIsTemplate", wxRadioButton)->GetValue() == true) {
		wxString refTemplate = ConfigDialogUtil::SetStringFromDialogChoice(OutfitStudioConfig, dlg, "npTemplateChoice");
		wxLogMessage("Loading reference template '%s'...", refTemplate);

		std::string tmplName{refTemplate.ToUTF8()};
		auto tmpl = find_if(refTemplates.begin(), refTemplates.end(), [&tmplName](const RefTemplate& rt) { return rt.GetName() == tmplName; });
		if (tmpl != refTemplates.end()) {
			if (wxFileName(wxString::FromUTF8(tmpl->GetSource())).IsRelative())
				error = project->LoadReferenceTemplate(GetProjectPath() + PathSepStr + tmpl->GetSource(),
													   tmpl->GetSetName(),
													   tmpl->GetShape(),
													   tmpl->GetLoadAll(),
													   mergeSliders,
													   mergeZaps);
			else
				error = project->LoadReferenceTemplate(tmpl->GetSource(), tmpl->GetSetName(), tmpl->GetShape(), tmpl->GetLoadAll(), mergeSliders, mergeZaps);
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

			error = project->LoadReference(fileName.ToUTF8().data(), sliderSetName.ToUTF8().data(), refShape.ToUTF8().data(), mergeSliders, mergeZaps);
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
	UpdateProgress(100, _("Finished"));
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

	std::string outfitName = XRCCTRL(dlg, "npOutfitName", wxTextCtrl)->GetValue();

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
		std::vector<std::string> texVec = {XRCCTRL(dlg, "npTexFilename", wxFilePickerCtrl)->GetPath().ToUTF8().data()};
		project->SetTextures(texVec);
	}

	wxLogMessage("Creating outfit...");
	UpdateProgress(50, _("Creating outfit..."));
	RefreshGUIFromProj();

	UpdateTitle();

	wxLogMessage("Outfit loaded.");
	UpdateProgress(100, _("Finished"));
	EndProgress();
}

void OutfitStudioFrame::OnUnloadProject(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Unload the project? All unsaved changes will be lost"), _("Unload Project"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Unload"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	wxLogMessage("Unloading project...");
	menuBar->Enable(XRCID("fileSave"), false);

	ClearProject();
	project->ClearReference();
	project->ClearOutfit();

	glView->Cleanup();
	glView->GetUndoHistory()->ClearHistory();

	activeSlider.clear();
	lastActiveSlider.clear();
	bEditSlider = false;
	MenuExitSliderEdit();

	delete project;
	project = new OutfitProject(this);

	CreateSetSliders();
	RefreshGUIFromProj(false);
	glView->Render();
}

void OutfitStudioFrame::UpdateReferenceTemplates() {
	refTemplates.clear();

	std::string fileName = GetProjectPath() + "/RefTemplates.xml";
	if (wxFileName::IsFileReadable(fileName)) {
		RefTemplateFile refTemplateFile(fileName);
		refTemplateFile.GetAll(refTemplates);
	}

	RefTemplateCollection refTemplateCol;
	refTemplateCol.Load(GetProjectPath() + "/RefTemplates");
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

	glView->ClearOverlays();
	activePartition.Unset();
	activeSegment.Unset();

	lastSelectedBones.clear();
	lastNormalizeBones.clear();

	if (currentTabButton)
		currentTabButton->SetPendingChanges(false);

	auto cMaskName = (wxChoice*)FindWindowByName("cMaskName");
	cMaskName->Clear();

	auto cPoseName = (wxChoice*)FindWindowByName("cPoseName");
	cPoseName->Clear();

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

void OutfitStudioFrame::RefreshGUIFromProj(bool render) {
	LockShapeSelect();

	selectedItems.clear();
	std::vector<ShapeItemState> prevStates;

	if (outfitRoot.IsOk()) {
		wxTreeItemIdValue cookie;
		wxTreeItemId child = outfitShapes->GetFirstChild(outfitRoot, cookie);
		while (child.IsOk()) {
			auto itemData = (ShapeItemData*)outfitShapes->GetItemData(child);
			if (itemData) {
				ShapeItemState prevState;
				prevState.shape = itemData->GetShape();
				prevState.state = outfitShapes->GetItemState(child);

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

		if (project->IsBaseShape(shape)) {
			outfitShapes->SetItemBold(item);
			outfitShapes->SetItemTextColour(item, wxColour(0, 255, 0));
		}

		auto it = std::find_if(prevStates.begin(), prevStates.end(), [&shape](const ShapeItemState& state) { return state.shape == shape; });

		if (it != prevStates.end()) {
			outfitShapes->SetItemState(item, it->state);

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

	auto cPoseName = (wxChoice*)FindWindowByName("cPoseName");
	cPoseName->Clear();

	std::string poseDataPath = GetProjectPath() + "/PoseData";
	poseDataCollection.LoadData(poseDataPath);

	for (auto& poseData : poseDataCollection.poseData) {
		wxString poseName = wxString::FromUTF8(poseData.name);
		cPoseName->Append(poseName, &poseData);
	}

	RefreshGUIWeightColors();
	PoseToGUI();

	glView->UpdateNodes();
	glView->UpdateBones();
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
		outfitBones->SetItemState(item, lastNormalizeBones.count(bone) != 0 ? 1 : 0);

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
	for (auto& shape : project->GetWorkNif()->GetShapes())
		MeshFromProj(shape, reloadTextures);

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
	std::string shapeName = shape->name.get();
	mesh* m = glView->GetMesh(shapeName);
	if (m) {
		m->smoothSeamNormals = project->activeSet.GetSmoothSeamNormals(shapeName);
		m->lockNormals = project->activeSet.GetLockNormals(shapeName);
	}
}

void OutfitStudioFrame::FillVertexColors() {
	std::vector<std::string> shapeNames = project->GetWorkNif()->GetShapeNames();

	for (auto& s : shapeNames) {
		mesh* m = glView->GetMesh(s);
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
	project->ClearBoneScale();

	for (auto& s : project->GetWorkNif()->GetShapes())
		UpdateShapeSource(s);

	ZeroSliders();
	if (!activeSlider.empty()) {
		bEditSlider = false;
		wxSliderPanel* sliderPanel = sliderPanels[activeSlider];
		sliderPanel->slider->SetFocus();
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
	for (auto& s : project->GetWorkNif()->GetShapes()) {
		if (!project->IsBaseShape(s)) {
			mesh* m = glView->GetMesh(s->name.get());
			if (m)
				shapeMeshes.push_back(m);
		}
	}

	int error = project->ExportNIF(fileName.ToUTF8().data(), shapeMeshes);
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
	project->ClearBoneScale();

	std::vector<mesh*> shapeMeshes;
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		mesh* m = glView->GetMesh(s);
		if (m)
			shapeMeshes.push_back(m);
	}

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
	for (auto& i : selectedItems)
		shapes.push_back(i->GetShape()->name.get());

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
		if (!project->GetWorkAnim()->shapeSkinning[shape->name.get()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform()))
			hasSkinTrans = true;
	}
	bool transToGlobal = false;
	if (hasSkinTrans) {
		int res = wxMessageBox(_("Some of the shapes have skin coordinate systems that are not the same as the global coordinate system.  Should the geometry be transformed to "
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
	project->ClearBoneScale();

	if (project->ExportOBJ(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
		wxLogError("Failed to export OBJ file '%s'!", fileName);
		wxMessageBox(_("Failed to export OBJ file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeOBJ(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->shapeSkinning[shape->name.get()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform()))
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
		project->ClearBoneScale();

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
		project->ClearBoneScale();

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (project->ExportOBJ(fileName.ToUTF8().data(), shapes, transToGlobal, Vector3(0.1f, 0.1f, 0.1f))) {
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
}

void OutfitStudioFrame::OnExportFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid())
		return;

	if (HasUnweightedCheck())
		return;

	bool hasSkinTrans = false;
	for (NiShape* shape : project->GetWorkNif()->GetShapes()) {
		if (!project->GetWorkAnim()->shapeSkinning[shape->name.get()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform()))
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
	project->ClearBoneScale();

	if (!project->ExportFBX(fileName.ToUTF8().data(), project->GetWorkNif()->GetShapes(), transToGlobal)) {
		wxLogError("Failed to export FBX file '%s'!", fileName);
		wxMessageBox(_("Failed to export FBX file!"), _("Export Error"), wxICON_ERROR);
	}
}

void OutfitStudioFrame::OnExportShapeFBX(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool hasSkinTrans = false;
	for (auto& i : selectedItems) {
		NiShape* shape = i->GetShape();
		if (!project->GetWorkAnim()->shapeSkinning[shape->name.get()].xformGlobalToSkin.IsNearlyEqualTo(MatTransform()))
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
		project->ClearBoneScale();

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
		project->ClearBoneScale();

		std::vector<NiShape*> shapes = {activeItem->GetShape()};
		if (!project->ExportFBX(fileName.ToUTF8().data(), shapes, transToGlobal)) {
			wxLogError("Failed to export FBX file '%s'!", fileName);
			wxMessageBox(_("Failed to export FBX file!"), _("Error"), wxICON_ERROR);
		}
	}
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

		TriHeadFile tri;
		if (!tri.Read(fn.ToUTF8().data())) {
			wxLogError("Failed to load TRI file '%s'!", fn);
			wxMessageBox(_("Failed to load TRI file!"), _("Error"), wxICON_ERROR);
			return;
		}

		std::string shapeName{fileName.GetName().ToUTF8()};
		while (project->IsValidShape(shapeName)) {
			std::string result{wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape"), shapeName, this).ToUTF8()};
			if (result.empty())
				continue;

			shapeName = std::move(result);
		}

		auto verts = tri.GetVertices();
		auto tris = tri.GetTriangles();
		auto uvs = tri.GetUV();
		auto shape = project->CreateNifShapeFromData(shapeName, &verts, &tris, &uvs);
		if (!shape)
			return;

		RefreshGUIFromProj(false);

		auto morphs = tri.GetMorphs();
		for (auto& morph : morphs) {
			if (!project->ValidSlider(morph.morphName)) {
				createSliderGUI(morph.morphName, sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(morph.morphName);
				ShowSliderEffect(morph.morphName);
			}

			std::unordered_map<uint16_t, Vector3> diff;
			diff.reserve(morph.vertices.size());

			for (size_t i = 0; i < morph.vertices.size(); i++)
				diff[i] = morph.vertices[i];

			project->SetSliderFromDiff(morph.morphName, shape, diff);
		}
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();

	ApplySliders();
	DoFilterSliders();
}

void OutfitStudioFrame::OnExportTRIHead(wxCommandEvent& WXUNUSED(event)) {
	if (!project->GetWorkNif()->IsValid()) {
		wxMessageBox(_("There are no valid shapes loaded!"), _("Error"));
		return;
	}

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	wxString dir = wxDirSelector(_("Export .tri morphs"), wxEmptyString, wxDD_DEFAULT_STYLE, wxDefaultPosition, this);
	if (dir.IsEmpty())
		return;

	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::string fn = dir + PathSepStr + shape->name.get() + ".tri";

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

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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

	project->ClearBoneScale();
	project->AddCombinedSlider(finalName);

	auto baseShape = project->GetBaseShape();
	if (baseShape) {
		mesh* m = glView->GetMesh(baseShape->name.get());
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
	glView->GetUndoHistory()->ClearHistory();
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

	std::string text = outfitBones->GetItemText(item).ToStdString();
	int state = outfitBones->GetItemState(item);

	switch (state) {
		case 0:
			outfitBones->SetItemState(item, 1);
			lastNormalizeBones.insert(text);
			break;
		default:
			outfitBones->SetItemState(item, 0);
			lastNormalizeBones.erase(text);
			break;
	}
}

void OutfitStudioFrame::OnBoneStateToggle(wxTreeEvent& event) {
	ToggleBoneState(event.GetItem());
	event.Skip();
}

void OutfitStudioFrame::RefreshGUIWeightColors() {
	// Clear weight color of all shapes
	for (auto& s : project->GetWorkNif()->GetShapeNames()) {
		mesh* m = glView->GetMesh(s);
		if (m)
			m->WeightFill(0.0f);
	}

	if (!activeBone.empty()) {
		// Show weights of selected shapes without reference
		for (auto& s : selectedItems) {
			if (!project->IsBaseShape(s->GetShape())) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(s->GetShape()->name.get(), activeBone);

				mesh* m = glView->GetMesh(s->GetShape()->name.get());
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

			mesh* m = glView->GetMesh(baseShape->name.get());
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

	project->ClearBoneScale();
	boneScale->SetValue(0);

	wxTreeItemId item = event.GetItem();
	if (!activeItem || !item.IsOk())
		return;

	wxArrayTreeItemIds selected;
	outfitBones->GetSelections(selected);

	activeBone.clear();
	std::string selBone = outfitBones->GetItemText(item);

	if (!outfitBones->IsSelected(item)) {
		if (!selected.IsEmpty()) {
			std::string frontBone = outfitBones->GetItemText(selected.front());
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

	glView->UpdateNodeColors();
	RefreshGUIWeightColors();
	CalcAutoXMirrorBone();
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
}

void OutfitStudioFrame::OnBoneContext(wxTreeEvent& event) {
	contextBone.clear();
	wxTreeItemId itemId = event.GetItem();
	if (itemId.IsOk())
		contextBone = outfitBones->GetItemText(itemId);
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnBoneTreeContext(wxCommandEvent& WXUNUSED(event)) {
	contextBone.clear();
	wxMenu* menu = wxXmlResource::Get()->LoadMenu("menuBoneTreeContext");
	if (menu) {
		PopupMenu(menu);
		delete menu;
	}
}

void OutfitStudioFrame::OnSegmentSelect(wxTreeEvent& event) {
	ShowSegment(event.GetItem());
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

void OutfitStudioFrame::OnSegmentSlotChanged(wxCommandEvent& event) {
	wxChoice* segmentSlot = (wxChoice*)event.GetEventObject();

	if (activeSegment.IsOk() && segmentTree->GetItemParent(activeSegment).IsOk()) {
		SubSegmentItemData* subSegmentData = dynamic_cast<SubSegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (subSegmentData) {
			subSegmentData->userSlotID = 0;

			if (segmentSlot->GetSelection() > 0) {
				wxString slotSel = segmentSlot->GetStringSelection();
				if (slotSel.length() >= 2) {
					unsigned long slot = 0;
					slotSel.Mid(0, 2).ToULong(&slot);

					if (slot > 0)
						subSegmentData->userSlotID = slot;
				}
			}

			UpdateSegmentNames();
			segmentTabButton->SetPendingChanges();
		}
	}
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
				for (size_t j = 0; j < inf.segs[i].subs.size(); j++) {
					NifSubSegmentInfo& sub = inf.segs[i].subs[j];
					segmentTree->AppendItem(segID, "Sub Segment", -1, -1, new SubSegmentItemData(sub.partID, sub.userSlotID, sub.material, sub.extraData));
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

void OutfitStudioFrame::ShowSegment(const wxTreeItemId& item, bool updateFromMask) {
	if (!activeItem || !glView->GetSegmentMode())
		return;

	std::unordered_map<uint16_t, float> mask;
	wxChoice* segmentType = nullptr;
	wxChoice* segmentSlot = nullptr;
	if (!updateFromMask) {
		segmentType = (wxChoice*)FindWindowByName("segmentType");
		segmentType->Disable();
		segmentType->SetSelection(0);

		segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");
		segmentSlot->Disable();
		segmentSlot->SetSelection(0);
	}
	else
		glView->GetActiveMask(mask);

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

		if (updateFromMask) {
			// Add triangles from mask
			for (size_t t = 0; t < tris.size(); t++) {
				if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end())
					triSParts[t] = subSegmentData->partID;
			}
		}
		else {
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
				wxString slotPrefix = wxString::Format("%d - ", subSegmentData->userSlotID);
				auto slotString = segmentSlot->GetString(i);
				if (slotString.StartsWith(slotPrefix)) {
					segmentSlot->SetSelection((int)i);
					break;
				}
			}

			segmentSlot->Enable();
		}
	}
	else {
		SegmentItemData* segmentData = dynamic_cast<SegmentItemData*>(segmentTree->GetItemData(activeSegment));
		if (segmentData) {
			// Active segment is a normal segment
			// Collect list of partition IDs for segment and children.
			// Also find partition ID of last child, or segment if none.
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

			if (updateFromMask) {
				// Add triangles from mask
				for (size_t t = 0; t < tris.size(); t++) {
					if (mask.find(tris[t].p1) != mask.end() && mask.find(tris[t].p2) != mask.end() && mask.find(tris[t].p3) != mask.end()
						&& (triSParts[t] < 0 || !selPartIDs[triSParts[t]]))
						triSParts[t] = destPartID;
				}
			}
		}
	}

	// Display segmentation colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triSParts);

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

		// Set mask
		m->MaskFill(0.0f);

		for (size_t i = 0; i < triSParts.size(); ++i) {
			if (triSParts[i] < 0 || !selPartIDs[triSParts[i]])
				continue;

			m->mask[tris[i].p1] = 1.0f;
			m->mask[tris[i].p2] = 1.0f;
			m->mask[tris[i].p3] = 1.0f;
		}
	}

	glView->Render();
}

void OutfitStudioFrame::UpdateSegmentNames() {
	auto segmentSlot = (wxChoice*)FindWindowByName("segmentSlot");
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

				if (subSegmentData->userSlotID >= 30) {
					wxString slotPrefix = wxString::Format("%d - ", subSegmentData->userSlotID);

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
			if (index >= partitionInfo.size()) {
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

void OutfitStudioFrame::CreatePartitionTree(NiShape* shape) {
	if (partitionTree->GetChildrenCount(partitionRoot) > 0) {
		triParts.clear(); // DeleteChildren calls OnPartitionSelect
		partitionTree->DeleteChildren(partitionRoot);
	}

	partitionTabButton->SetPendingChanges(false);

	NiVector<BSDismemberSkinInstance::PartitionInfo> partitionInfo;
	if (project->GetWorkNif()->GetShapePartitions(shape, partitionInfo, triParts)) {
		for (int i = 0; i < partitionInfo.size(); i++) {
			partitionTree->AppendItem(partitionRoot, "Partition", -1, -1, new PartitionItemData(i, partitionInfo[i].partID));
		}
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
	auto shape = activeItem->GetShape();
	if (!shape)
		return;

	std::unordered_map<uint16_t, float> mask;
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

	// Get all triangles of the active shape
	std::vector<Triangle> allTris;
	shape->GetTriangles(allTris);
	if (allTris.size() != triParts.size())
		return;

	PartitionItemData* partitionData = dynamic_cast<PartitionItemData*>(partitionTree->GetItemData(activePartition));
	if (partitionData) {
		if (!updateFromMask) {
			for (auto& s : partitionStrings) {
				if (s.StartsWith(wxString::Format("%d", partitionData->type))) {
					// Show correct data in UI
					partitionType->Enable();
					partitionType->SetStringSelection(s);
				}
			}
		}
		else {
			// Add triangles from mask
			for (size_t triInd = 0; triInd < allTris.size(); ++triInd) {
				const Triangle& tri = allTris[triInd];
				if (mask.find(tri.p1) != mask.end() && mask.find(tri.p2) != mask.end() && mask.find(tri.p3) != mask.end())
					triParts[triInd] = partitionData->index;
			}
		}
	}

	// Display partition colors depending on what is selected
	mesh* m = glView->GetMesh(activeItem->GetShape()->name.get());
	if (m) {
		SetSubMeshesForPartitions(m, triParts);

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

		// Set mask
		m->MaskFill(0.0f);

		if (partitionData) {
			for (size_t i = 0; i < allTris.size(); ++i) {
				if (triParts[i] != partitionData->index)
					continue;

				const Triangle& t = allTris[i];
				m->mask[t.p1] = 1.0f;
				m->mask[t.p2] = 1.0f;
				m->mask[t.p3] = 1.0f;
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

void OutfitStudioFrame::SetSubMeshesForPartitions(mesh* m, const std::vector<int>& tp) {
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

	for (uint32_t ti = 0; ti < nTris; ++ti) {
		while (tp[triInds[ti]] >= static_cast<int>(m->subMeshes.size()))
			m->subMeshes.emplace_back(ti, 0);

		if (tp[triInds[ti]] < 0) {
			m->subMeshes.emplace_back(ti, 0);
			break;
		}
	}

	// Calculate size of each sub-mesh.
	m->subMeshes.emplace_back(nTris, 0);
	for (size_t si = 0; si + 1 < m->subMeshes.size(); ++si)
		m->subMeshes[si].second = m->subMeshes[si + 1].first - m->subMeshes[si].first;

	m->subMeshes.pop_back();
	m->QueueUpdate(mesh::UpdateType::Indices);
}

void OutfitStudioFrame::SetNoSubMeshes(mesh* m) {
	if (!m)
		return;

	m->subMeshes.clear();
	m->subMeshesColor.clear();

	for (int ti = 0; ti < m->nTris; ++ti)
		m->renderTris[ti] = m->tris[ti];

	m->QueueUpdate(mesh::UpdateType::Indices);
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

	std::string name = box->GetName().BeforeLast('|');
	ShowSliderEffect(name, event.IsChecked());
	ApplySliders();
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
	else
		SelectTool(ToolID::Any);

	// Remember last standard tool used in regular tabs
	if (meshTabButton->GetCheck() || lightsTabButton->GetCheck()) {
		auto activeBrush = glView->GetActiveBrush();
		if (activeBrush) {
			int brushType = activeBrush->Type();
			if (brushType == TBT_STANDARD || brushType == TBT_MASK || brushType == TBT_MOVE) {
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
}

void OutfitStudioFrame::OnToggleRotationCenter(wxCommandEvent& WXUNUSED(event)) {
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

void OutfitStudioFrame::OnShowNodes(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowNodes(enabled);
}

void OutfitStudioFrame::OnShowBones(wxCommandEvent& event) {
	bool enabled = event.IsChecked();
	menuBar->Check(event.GetId(), enabled);
	toolBarV->ToggleTool(event.GetId(), enabled);
	glView->ShowBones(enabled);
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

void OutfitStudioFrame::OnFieldOfViewSlider(wxCommandEvent& WXUNUSED(event)) {
	int fieldOfView = fovSlider->GetValue();

	wxStaticText* fovLabel = (wxStaticText*)toolBarH->FindWindowByName("fovLabel");
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

void OutfitStudioFrame::OnBoneScaleSlider(wxCommandEvent& event) {
	if (!activeBone.empty())
		project->ApplyBoneScale(activeBone, event.GetInt(), true);

	glView->Render();
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

		if (glView->GetSegmentMode())
			glView->ClearActiveMask();

		glView->SetSegmentMode(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		menuBar->Check(XRCID("btnBrushCollision"), true);
		menuBar->Enable(XRCID("btnBrushCollision"), true);
		menuBar->Enable(XRCID("btnSelect"), true);
		menuBar->Enable(XRCID("btnClearMask"), true);
		menuBar->Enable(XRCID("btnInvertMask"), true);
		menuBar->Enable(XRCID("deleteVerts"), true);

		toolBarV->ToggleTool(XRCID("btnBrushCollision"), true);
		toolBarV->EnableTool(XRCID("btnBrushCollision"), true);
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

		if (glView->GetSegmentMode())
			glView->ClearActiveMask();

		glView->SetSegmentMode(false);
		glView->SetMaskVisible();
		glView->SetGlobalBrushCollision();

		menuBar->Check(XRCID("btnBrushCollision"), true);
		menuBar->Enable(XRCID("btnBrushCollision"), true);
		menuBar->Enable(XRCID("btnSelect"), true);
		menuBar->Enable(XRCID("btnClearMask"), true);
		menuBar->Enable(XRCID("btnInvertMask"), true);
		menuBar->Enable(XRCID("deleteVerts"), true);

		toolBarV->ToggleTool(XRCID("btnBrushCollision"), true);
		toolBarV->EnableTool(XRCID("btnBrushCollision"), true);
		toolBarH->EnableTool(XRCID("btnSelect"), true);
	}

	if (id != boneTabButton->GetId()) {
		boneScale->Show(false);
		cXMirrorBone->Show(false);

		wxStaticText* totalBoneCountLabel = (wxStaticText*)FindWindowByName("totalBoneCountLabel");
		wxStaticText* selectedBoneCountLabel = (wxStaticText*)FindWindowByName("selectedBoneCountLabel");
		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		wxCheckBox* cbNormalizeWeights = (wxCheckBox*)FindWindowByName("cbNormalizeWeights");
		wxStaticText* xMirrorBoneLabel = (wxStaticText*)FindWindowByName("xMirrorBoneLabel");

		totalBoneCountLabel->Show(false);
		selectedBoneCountLabel->Show(false);
		boneScaleLabel->Show(false);
		cbFixedWeight->Show(false);
		cbNormalizeWeights->Show(false);
		xMirrorBoneLabel->Show(false);
		posePane->Show(false);
		bonesFilter->GetParent()->Show(false);

		project->ClearBoneScale();

		if (project->bPose) {
			project->bPose = false;
			ApplyPose();
		}

		glView->SetWeightVisible(false);
	}

	if (id != colorsTabButton->GetId()) {
		UpdateVertexColors();
	}

	if (id != boneTabButton->GetId() || id != colorsTabButton->GetId()) {
		glView->SetTransformMode(false);

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
		menuBar->Enable(XRCID("deleteVerts"), true);

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

		boneScale->SetValue(0);
		boneScale->Show();
		cXMirrorBone->Show();

		wxStaticText* totalBoneCountLabel = (wxStaticText*)FindWindowByName("totalBoneCountLabel");
		wxStaticText* selectedBoneCountLabel = (wxStaticText*)FindWindowByName("selectedBoneCountLabel");
		wxStaticText* boneScaleLabel = (wxStaticText*)FindWindowByName("boneScaleLabel");
		wxCheckBox* cbFixedWeight = (wxCheckBox*)FindWindowByName("cbFixedWeight");
		wxCheckBox* cbNormalizeWeights = (wxCheckBox*)FindWindowByName("cbNormalizeWeights");
		wxStaticText* xMirrorBoneLabel = (wxStaticText*)FindWindowByName("xMirrorBoneLabel");

		totalBoneCountLabel->Show();
		selectedBoneCountLabel->Show();
		boneScaleLabel->Show();
		cbFixedWeight->Show();
		cbNormalizeWeights->Show();
		xMirrorBoneLabel->Show();
		posePane->Show();
		bonesFilter->GetParent()->Show();

		glView->SetTransformMode(false);
		SelectTool(ToolID::WeightBrush);
		glView->SetWeightVisible();

		project->bPose = cbPose->GetValue();

		if (project->bPose)
			ApplyPose();

		menuBar->Check(XRCID("btnWeightBrush"), true);
		menuBar->Enable(XRCID("btnWeightBrush"), true);
		menuBar->Enable(XRCID("btnColorBrush"), false);
		menuBar->Enable(XRCID("btnAlphaBrush"), false);
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
		menuBar->Enable(XRCID("deleteVerts"), false);

		toolBarH->ToggleTool(XRCID("btnWeightBrush"), true);
		toolBarH->EnableTool(XRCID("btnWeightBrush"), true);
		toolBarH->EnableTool(XRCID("btnColorBrush"), false);
		toolBarH->EnableTool(XRCID("btnAlphaBrush"), false);
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

		SetNoSubMeshes();

		ReselectBone();
		glView->GetUndoHistory()->ClearHistory();
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
		menuBar->Enable(XRCID("deleteVerts"), false);

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

		SelectTool(ToolID::MaskBrush);
		glView->SetSegmentMode();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);
		glView->ClearMasks();

		menuBar->Check(XRCID("btnMaskBrush"), true);
		menuBar->Check(XRCID("btnBrushCollision"), false);
		menuBar->Enable(XRCID("btnSelect"), false);
		menuBar->Enable(XRCID("btnTransform"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);
		menuBar->Enable(XRCID("btnUndiffBrush"), false);
		menuBar->Enable(XRCID("btnBrushCollision"), false);
		menuBar->Enable(XRCID("btnClearMask"), false);
		menuBar->Enable(XRCID("btnInvertMask"), false);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("deleteVerts"), false);

		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
		toolBarV->ToggleTool(XRCID("btnBrushCollision"), false);
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
		toolBarV->EnableTool(XRCID("btnBrushCollision"), false);

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

		SelectTool(ToolID::MaskBrush);
		glView->SetSegmentMode();
		glView->SetMaskVisible(false);
		glView->SetGlobalBrushCollision(false);
		glView->ClearMasks();

		menuBar->Check(XRCID("btnMaskBrush"), true);
		menuBar->Check(XRCID("btnBrushCollision"), false);
		menuBar->Enable(XRCID("btnSelect"), false);
		menuBar->Enable(XRCID("btnTransform"), false);
		menuBar->Enable(XRCID("btnPivot"), false);
		menuBar->Enable(XRCID("btnVertexEdit"), false);
		menuBar->Enable(XRCID("btnInflateBrush"), false);
		menuBar->Enable(XRCID("btnDeflateBrush"), false);
		menuBar->Enable(XRCID("btnMoveBrush"), false);
		menuBar->Enable(XRCID("btnSmoothBrush"), false);
		menuBar->Enable(XRCID("btnUndiffBrush"), false);
		menuBar->Enable(XRCID("btnBrushCollision"), false);
		menuBar->Enable(XRCID("btnClearMask"), false);
		menuBar->Enable(XRCID("btnInvertMask"), false);
		menuBar->Enable(XRCID("btnCollapseVertex"), false);
		menuBar->Enable(XRCID("btnFlipEdgeTool"), false);
		menuBar->Enable(XRCID("btnSplitEdgeTool"), false);
		menuBar->Enable(XRCID("deleteVerts"), false);

		toolBarH->ToggleTool(XRCID("btnMaskBrush"), true);
		toolBarV->ToggleTool(XRCID("btnBrushCollision"), false);
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
		toolBarV->EnableTool(XRCID("btnBrushCollision"), false);

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
	wxPanel* bottomSplitPanel = (wxPanel*)FindWindowByName("bottomSplitPanel");

	topSplitPanel->Layout();
	bottomSplitPanel->Layout();

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
			sliderPanels[project->GetSliderName(s)]->slider->SetValue(0);
		}
		ApplySliders();
	}
}

void OutfitStudioFrame::OnSlider(wxCommandEvent& event) {
	wxSlider* s = ((wxSlider*)event.GetEventObject());
	if (!s)
		return;

	project->ClearBoneScale(false);
	boneScale->SetValue(0);

	wxString sliderName = s->GetName();
	std::string sn{sliderName.BeforeLast('|').ToUTF8()};
	if (sn.empty())
		return;

	SetSliderValue(sn, event.GetInt());

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

	CloseBrushSettings();

	presets.LoadPresets(GetProjectPath() + "/SliderPresets", choice, names, true);
	presets.GetPresetNames(names);

	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgChoosePreset")) {
		presetChoice = XRCCTRL(dlg, "choicePreset", wxChoice);
		presetChoice->AppendString("Zero All");
		for (auto& n : names)
			presetChoice->AppendString(n);

		presetChoice->SetSelection(0);

		dlg.SetSize(wxSize(325, 175));
		dlg.SetSizeHints(wxSize(325, 175), wxSize(-1, -1));
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
		for (size_t i = 0; i < project->SliderCount(); i++) {
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

	CloseBrushSettings();

	SliderSetGroupCollection groupCollection;
	groupCollection.LoadGroups(GetProjectPath() + "/SliderGroups");

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
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
	if (!bEditSlider) {
		wxMessageBox(_("There is no slider in edit mode to import data to!"), _("Error"));
		return;
	}

	wxString fn = wxFileSelector(_("Import .nif file for slider calculation"), wxEmptyString, wxEmptyString, ".nif", "*.nif", wxFD_FILE_MUST_EXIST, this);
	if (fn.IsEmpty())
		return;

	wxLogMessage("Importing slider to '%s' for shape '%s' from NIF file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromNIF(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("No mesh found in the .nif file that matches currently selected shape!");
		wxMessageBox(_("No mesh found in the .nif file that matches currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
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

	wxLogMessage("Importing slider to '%s' for shape '%s' from BSD file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	project->SetSliderFromBSD(activeSlider, activeItem->GetShape(), fn.ToUTF8().data());

	SetPendingChanges();
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

	wxLogMessage("Importing slider to '%s' for shape '%s' from OBJ file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromOBJ(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
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

	wxLogMessage("Importing morphs from OSD file '%s'...", fn);

	OSDataFile osd;
	if (!osd.Read(fn.ToUTF8().data())) {
		wxLogError("Failed to import OSD file '%s'!", fn);
		wxMessageBox(_("Failed to import OSD file!"), _("Error"), wxICON_ERROR);
		return;
	}

	std::unordered_map<std::string, std::tuple<std::string, nifly::NiShape*>> newNameToDiffNameAndShape;
	auto diffs = osd.GetDataDiffs();
	auto& shapes = project->GetWorkNif()->GetShapes();
	for (auto& diff : diffs) {
		std::string bestShapeName;
		nifly::NiShape* bestShape = nullptr;
		for (auto& shape : shapes) {
			std::string shapeName = shape->name.get();
			// Diff name is supposed to begin with matching shape name
			if (diff.first.substr(0, shapeName.size()) != shapeName)
				continue;
			if (shapeName.length() > bestShapeName.length()) {
				bestShapeName = shapeName;
				bestShape = shape;
			}
		}
		if (!bestShape)
			continue;
		auto newName = diff.first.substr(bestShapeName.length(), diff.first.length() - bestShapeName.length() + 1);
		newNameToDiffNameAndShape[newName] = std::make_tuple(diff.first, bestShape);
	}

	std::vector<std::string> sliderNames(newNameToDiffNameAndShape.size());
	std::transform(newNameToDiffNameAndShape.begin(), newNameToDiffNameAndShape.end(), sliderNames.begin(), [](auto pair) { return pair.first; });
	SliderDataImportDialog import(this, project, OutfitStudioConfig);
	if (import.ShowModal(sliderNames) != wxID_OK)
		return;

	const auto& options = import.GetOptions();

	sliderScroll->Freeze();
	if (!options.mergeSliders) {
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

	wxString addedDiffs;
	std::unordered_set<NiShape*> addedShapes;

	for (auto& sliderName : options.selectedSliderNames) {
		auto& nameAndShape = newNameToDiffNameAndShape[sliderName];
		auto& originalSliderName = std::get<0>(nameAndShape);
		auto& diff = diffs[originalSliderName];
		auto& shape = std::get<1>(nameAndShape);

		if (!project->ValidSlider(sliderName)) {
			createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());
			project->AddEmptySlider(sliderName);
			ShowSliderEffect(sliderName);
		}

		project->SetSliderFromDiff(sliderName, shape, diff);
		addedShapes.emplace(shape);
	}

	for (auto& addedShape : addedShapes) {
		addedDiffs += addedShape->name.get() + "\n";
	}

	sliderScroll->FitInside();
	sliderScroll->Thaw();

	SetPendingChanges();
	ApplySliders();
	DoFilterSliders();

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

	wxMessageDialog dlg(this, _("This will delete all loaded sliders. Are you sure?"), _("TRI Import"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Import"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
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

	wxString addedMorphs;
	auto morphs = tri.GetMorphs();
	for (auto& morph : morphs) {
		auto shape = project->GetWorkNif()->FindBlockByName<NiShape>(morph.first);
		if (!shape)
			continue;

		addedMorphs += morph.first + "\n";
		for (auto& morphData : morph.second) {
			if (!project->ValidSlider(morphData->name)) {
				createSliderGUI(morphData->name, sliderScroll, sliderScroll->GetSizer());
				project->AddEmptySlider(morphData->name);
				ShowSliderEffect(morphData->name);
			}

			std::unordered_map<uint16_t, Vector3> diff(morphData->offsets.begin(), morphData->offsets.end());
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

	wxLogMessage("Importing slider to '%s' for shape '%s' from FBX file '%s'...", activeSlider, activeItem->GetShape()->name.get(), fn);
	if (!project->SetSliderFromFBX(activeSlider, activeItem->GetShape(), fn.ToUTF8().data())) {
		wxLogError("Vertex count of .obj file mesh does not match currently selected shape!");
		wxMessageBox(_("Vertex count of .obj file mesh does not match currently selected shape!"), _("Error"), wxICON_ERROR);
		return;
	}

	SetPendingChanges();
	ApplySliders();
}

void OutfitStudioFrame::OnSliderExportNIF(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
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
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
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
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}
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
			std::string targetFile = std::string(dir.ToUTF8()) + PathSepStr + shape->name.get() + "_" + slider + ".obj";
			wxLogMessage("Exporting OBJ slider data of '%s' for shape '%s' to '%s'...", slider, shape->name.get(), targetFile);
			if (project->SaveSliderOBJ(slider, shape, targetFile, true))
				wxLogError("Failed to export OBJ file '%s'!", targetFile);
		}
	}
}

void OutfitStudioFrame::OnClearSlider(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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

	std::string sliderName;
	do {
		sliderName = wxGetTextFromUser(_("Enter a name for the new zap:"), _("Create New Zap"), fillName, this).ToUTF8();
		if (sliderName.empty())
			return;
	} while (project->ValidSlider(sliderName));

	wxLogMessage("Creating new zap '%s'.", sliderName);

	createSliderGUI(sliderName, sliderScroll, sliderScroll->GetSizer());

	std::unordered_map<uint16_t, float> unmasked;
	for (auto& i : selectedItems) {
		unmasked.clear();
		glView->GetShapeUnmasked(unmasked, i->GetShape()->name.get());
		project->AddZapSlider(sliderName, unmasked, i->GetShape());
	}

	ShowSliderEffect(sliderName);
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
	for (auto& i : selectedItems)
		project->NegateSlider(activeSlider, i->GetShape());

	SetPendingChanges();
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
		wxLogMessage("Deleting slider '%s'.", sliderName);

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

void OutfitStudioFrame::ConformSliders(NiShape* shape, const ConformOptions& options) {
	if (project->IsBaseShape(shape))
		return;

	std::string shapeName = shape->name.get();
	wxLogMessage("Conforming '%s'...", shapeName);
	UpdateProgress(50, _("Conforming: ") + shapeName);

	mesh* m = glView->GetMesh(shapeName);
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
	ConformOptions options;
	if (ShowConform(options, silent)) {
		wxLogMessage("Conforming shapes...");
		ZeroSliders();

		StartProgress(_("Conforming shapes..."));
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
		UpdateProgress(100, _("Finished"));
		EndProgress();

		return 0;
	}
	return 1;
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

		XRCCTRL(dlg, "presetDefault", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(false);
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
			XRCCTRL(dlg, "noSqueeze", wxCheckBox)->SetValue(true);
			XRCCTRL(dlg, "solidMode", wxCheckBox)->SetValue(false);
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Disable();
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Disable();
			XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->ChangeValue("5.00000");
			XRCCTRL(dlg, "proximityRadiusSlider", wxSlider)->SetValue(5000);
		});

		XRCCTRL(dlg, "presetSolidObject", wxButton)->Bind(wxEVT_BUTTON, [&dlg](wxCommandEvent&) {
			XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->SetValue(true);
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

			options.noSqueeze = XRCCTRL(dlg, "noSqueeze", wxCheckBox)->IsChecked();
			options.solidMode = XRCCTRL(dlg, "solidMode", wxCheckBox)->IsChecked();
			options.axisX = XRCCTRL(dlg, "axisX", wxCheckBox)->IsChecked();
			options.axisY = XRCCTRL(dlg, "axisY", wxCheckBox)->IsChecked();
			options.axisZ = XRCCTRL(dlg, "axisZ", wxCheckBox)->IsChecked();
			return true;
		}
	}

	return false;
}

void OutfitStudioFrame::OnInvertUV(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool invertX = (event.GetId() == XRCID("uvInvertX"));
	bool invertY = (event.GetId() == XRCID("uvInvertY"));

	for (auto& i : selectedItems) {
		project->GetWorkNif()->InvertUVsForShape(i->GetShape(), invertX, invertY);
	}

	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnMirror(wxCommandEvent& event) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	bool mirrorX = (event.GetId() == XRCID("mirrorX"));
	bool mirrorY = (event.GetId() == XRCID("mirrorY"));
	bool mirrorZ = (event.GetId() == XRCID("mirrorZ"));

	for (auto& i : selectedItems) {
		project->GetWorkNif()->MirrorShape(i->GetShape(), mirrorX, mirrorY, mirrorZ);
	}

	RefreshGUIFromProj();
	SetPendingChanges();
}

void OutfitStudioFrame::OnRenameShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	std::string newShapeName;
	do {
		std::string result{wxGetTextFromUser(_("Please enter a new unique name for the shape."), _("Rename Shape")).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	std::string shapeName = activeItem->GetShape()->name.get();
	wxLogMessage("Renaming shape '%s' to '%s'.", shapeName, newShapeName);
	project->RenameShape(activeItem->GetShape(), newShapeName);
	glView->RenameShape(shapeName, newShapeName);

	outfitShapes->SetItemText(activeItem->GetId(), wxString::FromUTF8(newShapeName));
	SetPendingChanges();
}

void OutfitStudioFrame::OnSetReference(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	auto shape = activeItem->GetShape();
	if (!project->IsBaseShape(shape))
		project->SetBaseShape(shape);
	else
		project->SetBaseShape(nullptr);

	if (shape)
		project->SetTextures(shape);

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

void OutfitStudioFrame::OnMoveShape(wxCommandEvent& WXUNUSED(event)) {
	CloseBrushSettings();

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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
			usp->undoType = UT_VERTPOS;

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
					uss.pointStartState[i] = mesh::VecToMeshCoords(vertPos);
					uss.pointEndState[i] = mesh::VecToMeshCoords(newPos);
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

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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

			Vector3 origin;
			int originSelection = XRCCTRL(dlg, "origin", wxChoice)->GetCurrentSelection();
			if (originSelection == 1) {
				// Center of selected shape(s), respecting mask
				origin = glView->gls.GetActiveCenter();
				origin = mesh::VecToNifCoords(origin);
			}

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UT_VERTPOS;

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
					uss.pointStartState[i] = mesh::VecToMeshCoords(vertPos);
					uss.pointEndState[i] = mesh::VecToMeshCoords(newPos);
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
				scale = Vector3(uniformValue, uniformValue, uniformValue);

				XRCCTRL(dlg, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
				XRCCTRL(dlg, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
				XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
			}
			else {
				scale.x = atof(XRCCTRL(dlg, "ssTextX", wxTextCtrl)->GetValue().c_str());
				scale.y = atof(XRCCTRL(dlg, "ssTextY", wxTextCtrl)->GetValue().c_str());
				scale.z = atof(XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->GetValue().c_str());
			}

			if (scale.x < 0.01f) {
				scale.x = 0.01f;
				XRCCTRL(dlg, "ssTextX", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.x));
			}

			if (scale.y < 0.01f) {
				scale.y = 0.01f;
				XRCCTRL(dlg, "ssTextY", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.y));
			}

			if (scale.z < 0.01f) {
				scale.z = 0.01f;
				XRCCTRL(dlg, "ssTextZ", wxTextCtrl)->ChangeValue(wxString::Format("%0.5f", scale.z));
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

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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
				origin = mesh::VecToNifCoords(origin);
			}

			UndoStateProject* usp = glView->GetUndoHistory()->PushState();
			usp->undoType = UT_VERTPOS;

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
					uss.pointStartState[i] = mesh::VecToMeshCoords(vertPos);
					uss.pointEndState[i] = mesh::VecToMeshCoords(newPos);
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

void OutfitStudioFrame::OnDeleteVerts(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (bEditSlider) {
		wxMessageBox(_("You're currently editing slider data, please exit the slider's edit mode (pencil button) and try again."));
		return;
	}

	// Prepare the undo data and determine if any shapes are being deleted.
	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UT_MESH;
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
		for (NiShape* shape : delShapes)
			project->DeleteShape(shape);
	}

	// Now do the vertex deletion
	for (auto& uss : usp->usss) {
		NiShape* shape = project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
		if (!shape)
			continue;
		project->ApplyShapeMeshUndo(shape, uss, false);
	}

	project->GetWorkAnim()->CleanupBones();

	RefreshGUIFromProj(false);
	SetPendingChanges();

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

	std::unordered_map<uint16_t, float> masked;
	glView->GetShapeMask(masked, activeItem->GetShape()->name.get());
	if (masked.empty())
		return;

	std::string newShapeName;
	do {
		std::string result{wxGetTextFromUser(_("Please enter a unique name for the new separated shape."), _("Separate Vertices...")).ToUTF8()};
		if (result.empty())
			return;

		newShapeName = std::move(result);
	} while (project->IsValidShape(newShapeName));

	if (editUV && editUV->shape == activeItem->GetShape())
		editUV->Close();

	auto newShape = project->DuplicateShape(activeItem->GetShape(), newShapeName);

	UndoStateProject* usp = glView->GetUndoHistory()->PushState();
	usp->undoType = UT_MESH;
	usp->usss.resize(2);
	usp->usss[0].shapeName = activeItem->GetShape()->name.get();
	usp->usss[1].shapeName = newShapeName;

	std::unordered_map<uint16_t, float> unmasked = masked;
	glView->InvertMaskTris(unmasked, activeItem->GetShape()->name.get());

	project->PrepareDeleteVerts(activeItem->GetShape(), masked, usp->usss[0]);
	project->PrepareDeleteVerts(newShape, unmasked, usp->usss[1]);

	project->ApplyShapeMeshUndo(activeItem->GetShape(), usp->usss[0], false);
	project->ApplyShapeMeshUndo(newShape, usp->usss[1], false);

	project->SetTextures();
	RefreshGUIFromProj(false);
	SetPendingChanges();

	glView->ClearActiveMask();
	ApplySliders();
}

void OutfitStudioFrame::CheckCopyGeo(wxDialog& dlg) {
	wxStaticText* errors = XRCCTRL(dlg, "copyGeometryErrors", wxStaticText);
	wxChoice* sourceChoice = XRCCTRL(dlg, "sourceChoice", wxChoice);
	wxChoice* targetChoice = XRCCTRL(dlg, "targetChoice", wxChoice);

	std::string source = sourceChoice->GetString(sourceChoice->GetSelection()).ToStdString();
	std::string target = targetChoice->GetString(targetChoice->GetSelection()).ToStdString();

	MergeCheckErrors e;
	project->CheckMerge(source, target, e);
	XRCCTRL(dlg, "wxID_OK", wxButton)->Enable(e.canMerge);

	if (e.canMerge) {
		errors->SetLabel(_("No errors found!"));
		dlg.SetSize(dlg.GetBestSize());
		return;
	}

	wxString msg;
	msg << _("Errors:");
	if (e.shapesSame)
		msg << "\n- " << _("Target must be different from source.");
	if (e.partitionsMismatch)
		msg << "\n- " << _("Partitions do not match. Make sure the amount of partitions and their slots match up.");
	if (e.segmentsMismatch)
		msg << "\n- " << _("Segments do not match. Make sure the amount of segments, sub segments and their info as well as the segmentation file match.");
	if (e.tooManyVertices)
		msg << "\n- " << _("Resulting shape would have too many vertices.");
	if (e.tooManyTriangles)
		msg << "\n- " << _("Resulting shape would have too many triangles.");
	if (e.shaderMismatch)
		msg << "\n- " << _("Shaders do not match. Make sure both shapes either have or don't have a shader and their shader type matches.");
	if (e.textureMismatch)
		msg << "\n- " << _("Base texture doesn't match. Make sure both shapes have the same base/diffuse texture path.");
	if (e.alphaPropMismatch)
		msg << "\n- " << _("Alpha property mismatch. Make sure both shapes either have or don't have an alpha property and their flags + threshold match.");

	errors->SetLabel(msg);
	dlg.SetSize(dlg.GetBestSize());
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

	sourceChoice->Bind(wxEVT_CHOICE, [this, &dlg](wxCommandEvent&) { CheckCopyGeo(dlg); });
	targetChoice->Bind(wxEVT_CHOICE, [this, &dlg](wxCommandEvent&) { CheckCopyGeo(dlg); });
	CheckCopyGeo(dlg);

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
	usp->undoType = UT_MESH;
	usp->usss.resize(1);
	usp->usss[0].shapeName = targetShapeName;

	project->PrepareCopyGeo(sourceShape, targetShape, usp->usss[0]);

	project->ApplyShapeMeshUndo(targetShape, usp->usss[0], false);

	if (XRCCTRL(dlg, "checkDeleteSource", wxCheckBox)->IsChecked())
		project->DeleteShape(sourceShape);

	RefreshGUIFromProj(false);
	SetPendingChanges();
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

		do {
			std::string result{wxGetTextFromUser(_("Please enter a unique name for the duplicated shape."), _("Duplicate Shape")).ToUTF8()};
			if (result.empty())
				return;

			newName = std::move(result);
		} while (project->IsValidShape(newName));

		wxLogMessage("Duplicating shape '%s' as '%s'.", activeItem->GetShape()->name.get(), newName);
		project->ClearBoneScale();

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
	for (auto& i : selectedItems)
		selected.push_back(*i);

	activeItem = nullptr;
	selectedItems.clear();

	for (auto& i : selected) {
		if (editUV && editUV->shape == i.GetShape())
			editUV->Close();

		std::string shapeName = i.GetShape()->name.get();
		wxLogMessage("Deleting shape '%s'.", shapeName);
		project->DeleteShape(i.GetShape());
		glView->DeleteMesh(shapeName);
		wxTreeItemId item = i.GetId();
		outfitShapes->Delete(item);
	}

	SetPendingChanges();
	UpdateAnimationGUI();
	glView->Render();
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
				if (cb->boneName == contextBone)
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
			std::string bone = boneTree->GetItemText(sel[i]);
			wxLogMessage("Adding bone '%s' to project.", bone);

			project->AddBoneRef(bone);
			wxTreeItemId item = outfitBones->AppendItem(bonesRoot, bone);
			outfitBones->SetItemState(item, 0);
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

void OutfitStudioFrame::GetBoneDlgData(wxDialog& dlg, MatTransform& xform, std::string& parentBone) {
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
}

void OutfitStudioFrame::OnAddCustomBone(wxCommandEvent& WXUNUSED(event)) {
	wxDialog dlg;
	if (!wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCustomBone"))
		return;

	CloseBrushSettings();

	dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);
	FillParentBoneChoice(dlg, contextBone);

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
	GetBoneDlgData(dlg, xform, parentBone);

	wxLogMessage("Adding custom bone '%s' to project.", bone);
	project->AddCustomBoneRef(bone.ToStdString(), parentBone, xform);
	wxTreeItemId newItem = outfitBones->AppendItem(bonesRoot, bone);
	outfitBones->SetItemState(newItem, 0);
	cXMirrorBone->AppendString(bone);
	cPoseBone->AppendString(bone);

	glView->UpdateBones();
	UpdateBoneCounts();
	SetPendingChanges();
	glView->Render();
}

void OutfitStudioFrame::OnEditBone(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bPtr = AnimSkeleton::getInstance().GetBonePtr(contextBone);
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

	if (bPtr->isStandardBone) {
		dlg.SetLabel("View Standard Bone");
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
		dlg.SetLabel("Edit Custom Bone");
	}

	if (dlg.ShowModal() != wxID_OK)
		return;

	MatTransform xform;
	std::string parentBone;
	GetBoneDlgData(dlg, xform, parentBone);

	project->ModifyCustomBone(bPtr, parentBone, xform);
	glView->UpdateBones();
	ApplyPose();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteBone(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (size_t i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting bone '%s' from project.", bone);

		project->DeleteBone(bone);
		activeBone.clear();

		outfitBones->Delete(selItems[i]);
		lastSelectedBones.erase(bone);
		lastNormalizeBones.erase(bone);
	}

	glView->UpdateBones();
	ReselectBone();
	CalcAutoXMirrorBone();
	glView->GetUndoHistory()->ClearHistory();
	UpdateBoneCounts();
	SetPendingChanges();
}

void OutfitStudioFrame::OnDeleteBoneFromSelected(wxCommandEvent& WXUNUSED(event)) {
	wxArrayTreeItemIds selItems;
	outfitBones->GetSelections(selItems);
	for (size_t i = 0; i < selItems.size(); i++) {
		std::string bone = outfitBones->GetItemText(selItems[i]);
		wxLogMessage("Deleting weights of bone '%s' from selected shapes.", bone);

		for (auto& s : selectedItems)
			project->GetWorkAnim()->RemoveShapeBone(s->GetShape()->name.get(), bone);
	}

	glView->UpdateBones();
	ReselectBone();
	glView->GetUndoHistory()->ClearHistory();
	HighlightBoneNamesWithWeights();
	UpdateBoneCounts();
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

bool OutfitStudioFrame::ShowWeightCopy(WeightCopyOptions& options, bool silent) {
	CloseBrushSettings();

	wxDialog dlg;
	if (wxXmlResource::Get()->LoadDialog(&dlg, this, "dlgCopyWeights")) {
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

		XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Bind(wxEVT_TEXT, [&dlg](wxCommandEvent&) {
			int changed = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->SetValue(changed);
		});

		XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->Bind(wxEVT_CHECKBOX, [&dlg](wxCommandEvent&) {
			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->Enable(!noTargetLimit);
			XRCCTRL(dlg, "maxResultsSlider", wxSlider)->Enable(!noTargetLimit);
		});

		wxCheckBox* cbCopySkinTrans = XRCCTRL(dlg, "cbCopySkinTrans", wxCheckBox);
		wxCheckBox* cbTransformGeo = XRCCTRL(dlg, "cbTransformGeo", wxCheckBox);
		if (options.showSkinTransOption) {
			cbCopySkinTrans->SetValue(options.doSkinTransCopy);
			cbTransformGeo->SetValue(options.doTransformGeo);
			cbCopySkinTrans->Show();
			cbTransformGeo->Show();
			XRCCTRL(dlg, "copyTransDescription", wxStaticText)->Show();
		}

		dlg.Bind(wxEVT_CHAR_HOOK, &OutfitStudioFrame::OnEnterClose, this);

		dlg.SetSize(dlg.GetBestSize());

		if (silent || dlg.ShowModal() == wxID_OK) {
			options.proximityRadius = atof(XRCCTRL(dlg, "proximityRadiusText", wxTextCtrl)->GetValue().c_str());

			bool noTargetLimit = XRCCTRL(dlg, "noTargetLimit", wxCheckBox)->IsChecked();
			if (!noTargetLimit)
				options.maxResults = atol(XRCCTRL(dlg, "maxResultsText", wxTextCtrl)->GetValue().c_str());
			else
				options.maxResults = std::numeric_limits<int>::max();

			if (options.showSkinTransOption) {
				options.doSkinTransCopy = cbCopySkinTrans->IsChecked();
				options.doTransformGeo = cbTransformGeo->IsChecked();
			}

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

void OutfitStudioFrame::CalcCopySkinTransOption(WeightCopyOptions& options) {
	// This function calculates whether the "copy global-to-skin transform
	// from base shape" checkbox should be shown and what the default value
	// for the "transform-geometry" checkbox should be

	NifFile* nif = project->GetWorkNif();
	NiShape* baseShape = project->GetBaseShape();
	if (!baseShape)
		return;

	AnimInfo& workAnim = *project->GetWorkAnim();

	if (!workAnim.HasSkinnedShape(baseShape))
		return;

	const MatTransform& baseXformGlobalToSkin = workAnim.shapeSkinning[baseShape->name.get()].xformGlobalToSkin;

	// Check if any shape's skin CS is different from the base shape's
	for (size_t i = 0; i < selectedItems.size(); i++) {
		NiShape* shape = selectedItems[i]->GetShape();
		if (shape == baseShape)
			continue;

		if (!workAnim.HasSkinnedShape(shape))
			continue;

		if (!workAnim.shapeSkinning[shape->name.get()].xformGlobalToSkin.IsNearlyEqualTo(baseXformGlobalToSkin)) {
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

		const MatTransform& globalToSkin = workAnim.shapeSkinning[shape->name.get()].xformGlobalToSkin;
		if (globalToSkin.IsNearlyEqualTo(baseXformGlobalToSkin))
			continue;

		const std::vector<Vector3>& verts = *nif->GetVertsForShape(shape);
		if (verts.empty())
			continue;

		// Calculate old average and new average.
		MatTransform skinToGlobal = globalToSkin.InverseTransform();
		MatTransform skinToBaseSkin = baseXformGlobalToSkin.ComposeTransforms(skinToGlobal);

		Vector3 oldAvg, newAvg;
		for (size_t j = 0; j < verts.size(); ++j) {
			oldAvg += skinToBaseSkin.ApplyTransform(verts[j]);
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
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (!project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	std::vector<NiShape*> selectedShapes;
	for (auto& s : selectedItems) {
		if (auto shape = s->GetShape(); !project->IsBaseShape(shape))
			selectedShapes.push_back(s->GetShape());
		else
			wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
	}
	CopyBoneWeightForShapes(selectedShapes);
}

int OutfitStudioFrame::CopyBoneWeightForShapes(std::vector<NiShape*> shapes, bool silent) {
	WeightCopyOptions options;
	CalcCopySkinTransOption(options);
	AnimInfo& workAnim = *project->GetWorkAnim();

	if (ShowWeightCopy(options, silent)) {
		StartProgress(_("Copying bone weights..."));

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UT_WEIGHT;

		std::vector<std::string> baseBones = workAnim.shapeBones[project->GetBaseShape()->name.get()];
		std::sort(baseBones.begin(), baseBones.end());
		std::unordered_map<uint16_t, float> mask;

		const int inc = 100 / shapes.size() - 1;

		for (size_t i = 0; i < shapes.size(); i++) {
			NiShape* shape = shapes[i];
			wxLogMessage("Copying bone weights to '%s'...", shape->name.get());
			StartSubProgress(i * inc, i * inc + inc);

			if (options.doSkinTransCopy) {
				const MatTransform& baseXformGlobalToSkin = workAnim.shapeSkinning[project->GetBaseShape()->name.get()].xformGlobalToSkin;
				const MatTransform& oldXformGlobalToSkin = workAnim.shapeSkinning[shape->name.get()].xformGlobalToSkin;

				if (options.doTransformGeo && !baseXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin))
					project->ApplyTransformToShapeGeometry(shape, baseXformGlobalToSkin.ComposeTransforms(oldXformGlobalToSkin.InverseTransform()));

				workAnim.ChangeGlobalToSkinTransform(shape->name.get(), baseXformGlobalToSkin);
				project->GetWorkNif()->SetShapeTransformGlobalToSkin(shape, baseXformGlobalToSkin);
			}

			usp->usss.resize(usp->usss.size() + 1);
			usp->usss.back().shapeName = shape->name.get();

			mask.clear();
			glView->GetShapeMask(mask, shape->name.get());

			std::vector<std::string> bones = workAnim.shapeBones[shape->name.get()];
			std::vector<std::string> mergedBones = baseBones;

			for (auto b : bones)
				if (!std::binary_search(baseBones.begin(), baseBones.end(), b))
					mergedBones.push_back(b);

			std::vector<std::string> lockedBones;

			project->CopyBoneWeights(shape, options.proximityRadius, options.maxResults, mask, mergedBones, baseBones.size(), lockedBones, usp->usss.back(), false);
			EndProgress();
		}

		if (options.doSkinTransCopy || options.doTransformGeo)
			RefreshGUIFromProj();

		ActiveShapesUpdated(usp, false);
		project->morpher.ClearProximityCache();

		UpdateUndoTools();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}

	workAnim.CleanupBones();
	UpdateAnimationGUI();
	return 0;
}

void OutfitStudioFrame::OnCopySelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	if (!project->GetBaseShape()) {
		wxMessageBox(_("There is no reference shape!"), _("Error"));
		return;
	}

	std::vector<std::string> boneList = GetSelectedBones();
	int nSelBones = boneList.size();
	if (nSelBones < 1)
		return;

	std::unordered_set<std::string> selBones{boneList.begin(), boneList.end()};

	std::string bonesString;
	for (std::string& boneName : boneList)
		bonesString += "'" + boneName + "' ";

	std::vector<std::string> normBones, notNormBones, lockedBones;
	GetNormalizeBones(&normBones, &notNormBones);
	for (auto& bone : normBones)
		if (!selBones.count(bone))
			boneList.push_back(bone);

	bool bHasNormBones = static_cast<int>(boneList.size()) > nSelBones;
	if (bHasNormBones) {
		for (auto& bone : notNormBones)
			if (!selBones.count(bone))
				lockedBones.push_back(bone);
	}
	else {
		for (auto& bone : notNormBones)
			if (!selBones.count(bone))
				boneList.push_back(bone);
	}

	WeightCopyOptions options;
	CalcCopySkinTransOption(options);
	AnimInfo& workAnim = *project->GetWorkAnim();

	if (ShowWeightCopy(options)) {
		StartProgress(_("Copying selected bone weights..."));

		UndoStateProject* usp = glView->GetUndoHistory()->PushState();
		usp->undoType = UT_WEIGHT;
		std::unordered_map<uint16_t, float> mask;
		for (size_t i = 0; i < selectedItems.size(); i++) {
			NiShape* shape = selectedItems[i]->GetShape();
			if (!project->IsBaseShape(shape)) {
				wxLogMessage("Copying selected bone weights to '%s' for %s...", shape->name.get(), bonesString);
				if (options.doSkinTransCopy) {
					const MatTransform& baseXformGlobalToSkin = workAnim.shapeSkinning[project->GetBaseShape()->name.get()].xformGlobalToSkin;
					const MatTransform& oldXformGlobalToSkin = workAnim.shapeSkinning[shape->name.get()].xformGlobalToSkin;

					if (options.doTransformGeo && !baseXformGlobalToSkin.IsNearlyEqualTo(oldXformGlobalToSkin))
						project->ApplyTransformToShapeGeometry(shape, baseXformGlobalToSkin.ComposeTransforms(oldXformGlobalToSkin.InverseTransform()));

					workAnim.ChangeGlobalToSkinTransform(shape->name.get(), baseXformGlobalToSkin);
					project->GetWorkNif()->SetShapeTransformGlobalToSkin(shape, baseXformGlobalToSkin);
				}

				usp->usss.resize(usp->usss.size() + 1);
				usp->usss.back().shapeName = shape->name.get();

				mask.clear();
				glView->GetShapeMask(mask, shape->name.get());

				project->CopyBoneWeights(shape, options.proximityRadius, options.maxResults, mask, boneList, nSelBones, lockedBones, usp->usss.back(), bHasNormBones);
			}
			else
				wxMessageBox(_("Sorry, you can't copy weights from the reference shape to itself. Skipping this shape."), _("Can't copy weights"), wxICON_WARNING);
		}

		if (options.doSkinTransCopy || options.doTransformGeo)
			RefreshGUIFromProj();

		ActiveShapesUpdated(usp, false);
		project->morpher.ClearProximityCache();

		UpdateUndoTools();

		UpdateProgress(100, _("Finished"));
		EndProgress();
	}

	workAnim.CleanupBones();
	UpdateAnimationGUI();
}

void OutfitStudioFrame::OnTransferSelectedWeight(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

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

	UpdateProgress(100, _("Finished"));
	EndProgress();

	SetPendingChanges();
}

void OutfitStudioFrame::OnMaskWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	for (auto& i : selectedItems) {
		std::string shapeName = i->GetShape()->name.get();
		mesh* m = glView->GetMesh(shapeName);
		if (!m)
			continue;

		m->MaskFill(0.0f);

		auto& bones = project->GetWorkAnim()->shapeBones;
		if (bones.find(shapeName) != bones.end()) {
			for (auto& b : bones[shapeName]) {
				auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
				if (weights) {
					for (auto& bw : *weights)
						if (bw.second > 0.0f)
							m->mask[bw.first] = 1.0f;
				}
			}
		}
	}

	glView->Refresh();
}

void OutfitStudioFrame::OnMaskBoneWeighted(wxCommandEvent& WXUNUSED(event)) {
	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	for (auto& i : selectedItems) {
		std::string shapeName = i->GetShape()->name.get();
		mesh* m = glView->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		m->MaskFill(0.0f);

		for (auto& b : GetSelectedBones()) {
			auto weights = project->GetWorkAnim()->GetWeightsPtr(shapeName, b);
			if (weights) {
				for (auto& bw : *weights)
					if (bw.second > 0.0f)
						m->mask[bw.first] = 1.0f;
			}
		}
	}

	glView->Refresh();
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

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	auto shape = activeItem->GetShape();
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

	std::string fn = file->GetPath();
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
		glView->RecalcNormals(s->GetShape()->name.get());

	glView->Render();
}

void OutfitStudioFrame::OnSmoothNormalSeams(wxCommandEvent& event) {
	bool enable = event.IsChecked();
	glView->SetNormalSeamSmoothMode(enable);

	for (auto& s : selectedItems)
		project->activeSet.SetSmoothSeamNormals(s->GetShape()->name.get(), enable);

	glView->Render();
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

	if (!activeItem) {
		wxMessageBox(_("There is no shape selected!"), _("Error"));
		return;
	}

	auto shape = activeItem->GetShape();
	mesh* m = glView->GetMesh(shape->name.get());
	if (shape && m) {
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
	wxChoice* cMaskName = (wxChoice*)FindWindowByName("cMaskName");
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
	wxChoice* cMaskName = (wxChoice*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>();

		std::vector<std::string> shapes = GetShapeList();
		for (auto& s : shapes) {
			std::unordered_map<uint16_t, float> mask;
			glView->GetShapeMask(mask, s);
			(*maskData)[s] = std::move(mask);
		}

		cMaskName->SetClientData(maskSel, maskData);
	}
	else {
		wxCommandEvent evt;
		OnSaveAsMask(evt);
	}
}

void OutfitStudioFrame::OnSaveAsMask(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cMaskName = (wxChoice*)FindWindowByName("cMaskName");

	wxString maskName;
	do {
		maskName = wxGetTextFromUser(_("Please enter a new unique name for the mask."), _("New Mask"));
		if (maskName.empty())
			return;

	} while (cMaskName->FindString(maskName) != wxNOT_FOUND);

	auto maskData = new std::map<std::string, std::unordered_map<uint16_t, float>>();

	std::vector<std::string> shapes = GetShapeList();
	for (auto& s : shapes) {
		std::unordered_map<uint16_t, float> mask;
		glView->GetShapeMask(mask, s);
		(*maskData)[s] = std::move(mask);
	}

	int maskSel = cMaskName->Append(maskName, maskData);
	cMaskName->SetSelection(maskSel);
}

void OutfitStudioFrame::OnDeleteMask(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cMaskName = (wxChoice*)FindWindowByName("cMaskName");
	int maskSel = cMaskName->GetSelection();
	if (maskSel != wxNOT_FOUND) {
		cMaskName->Delete(maskSel);
	}
}

void OutfitStudioFrame::OnPaneCollapse(wxCollapsiblePaneEvent& WXUNUSED(event)) {
	wxWindow* parentPanel = FindWindowByName("bottomSplitPanel");
	parentPanel->Layout();
}

void OutfitStudioFrame::ApplyPose() {
	for (auto& shape : project->GetWorkNif()->GetShapes()) {
		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);
		glView->UpdateMeshVertices(shape->name.get(), &verts, true, true, false);
	}
	if (!activeBone.empty()) {
		int boneScalePos = boneScale->GetValue();
		if (boneScalePos != 0)
			project->ApplyBoneScale(activeBone, boneScalePos);
	}
	glView->Render();
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
		rxPoseText->ChangeValue(wxString() << bone->poseRotVec.x);
		ryPoseText->ChangeValue(wxString() << bone->poseRotVec.y);
		rzPoseText->ChangeValue(wxString() << bone->poseRotVec.z);
		txPoseText->ChangeValue(wxString() << bone->poseTranVec.x);
		tyPoseText->ChangeValue(wxString() << bone->poseTranVec.y);
		tzPoseText->ChangeValue(wxString() << bone->poseTranVec.z);
	}
	else {
		rxPoseSlider->SetValue(0);
		ryPoseSlider->SetValue(0);
		rzPoseSlider->SetValue(0);
		txPoseSlider->SetValue(0);
		tyPoseSlider->SetValue(0);
		tzPoseSlider->SetValue(0);
		rxPoseText->ChangeValue("0");
		ryPoseText->ChangeValue("0");
		rzPoseText->ChangeValue("0");
		txPoseText->ChangeValue("0");
		tyPoseText->ChangeValue("0");
		tzPoseText->ChangeValue("0");
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
	else
		bone->poseTranVec[cind - 3] = val;
	bone->UpdatePoseTransform();
	ApplyPose();
}

void OutfitStudioFrame::OnAnyPoseSlider(wxScrollEvent& e, wxTextCtrl* t, int cind) {
	float val = e.GetPosition() * 0.01f;
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

void OutfitStudioFrame::OnAnyPoseTextChanged(wxTextCtrl* t, wxSlider* s, int cind) {
	if (!t || !s)
		return;
	double val;
	if (!t->GetValue().ToDouble(&val))
		return;
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

void OutfitStudioFrame::OnResetBonePose(wxCommandEvent& WXUNUSED(event)) {
	AnimBone* bone = GetPoseBonePtr();
	if (!bone)
		return;
	bone->poseRotVec = Vector3(0, 0, 0);
	bone->poseTranVec = Vector3(0, 0, 0);
	bone->UpdatePoseTransform();
	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::OnResetAllPose(wxCommandEvent& WXUNUSED(event)) {
	wxMessageDialog dlg(this, _("Reset all bone poses?"), _("Reset Pose"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
	dlg.SetOKCancelLabels(_("Reset"), _("Cancel"));
	if (dlg.ShowModal() != wxID_OK)
		return;

	std::vector<std::string> bones;
	project->GetActiveBones(bones);

	for (const std::string& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		if (bone->poseRotVec.IsZero() && bone->poseTranVec.IsZero())
			continue;

		bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
		bone->UpdatePoseTransform();
	}

	PoseToGUI();
	ApplyPose();
}

void OutfitStudioFrame::OnPoseToMesh(wxCommandEvent& WXUNUSED(event)) {
	if (project->bPose) {
		wxMessageDialog dlg(this, _("Permanently apply the pose to the mesh?"), _("Apply Pose to Mesh"), wxOK | wxCANCEL | wxICON_WARNING | wxCANCEL_DEFAULT);
		dlg.SetOKCancelLabels(_("Apply"), _("Cancel"));
		if (dlg.ShowModal() != wxID_OK)
			return;

		for (auto& s : project->GetWorkNif()->GetShapes()) {
			UpdateShapeSource(s);
			project->RefreshMorphShape(s);
		}

		project->InvalidateBoneScaleCache();
		boneScale->SetValue(0);

		std::vector<std::string> bones;
		project->GetActiveBones(bones);

		for (const std::string& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (!bone)
				continue;

			if (bone->poseRotVec.IsZero() && bone->poseTranVec.IsZero())
				continue;

			bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			bone->UpdatePoseTransform();
		}

		PoseToGUI();
		ApplyPose();
		SetPendingChanges();
	}
}

void OutfitStudioFrame::OnPoseCheckBox(wxCommandEvent& e) {
	project->bPose = e.IsChecked();

	auto poseToMesh = (wxButton*)FindWindowByName("poseToMesh");
	poseToMesh->Enable(project->bPose);

	ApplyPose();
}

void OutfitStudioFrame::OnSelectPose(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cPoseName = (wxChoice*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));

		std::vector<std::string> bones;
		project->GetActiveBones(bones);

		for (const auto& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (!bone)
				continue;

			auto poseBoneData = std::find_if(poseData->boneData.begin(), poseData->boneData.end(), [&boneName](const PoseBoneData& rt) { return rt.name == boneName; });
			if (poseBoneData != poseData->boneData.end()) {
				bone->poseRotVec = poseBoneData->rotation;
				bone->poseTranVec = poseBoneData->translation;
			}
			else {
				bone->poseRotVec = Vector3(0.0f, 0.0f, 0.0f);
				bone->poseTranVec = Vector3(0.0f, 0.0f, 0.0f);
			}

			bone->UpdatePoseTransform();
		}

		PoseToGUI();
		ApplyPose();
	}
}

void OutfitStudioFrame::OnSavePose(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cPoseName = (wxChoice*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));
		poseData->boneData.clear();

		std::vector<std::string> bones;
		project->GetActiveBones(bones);

		for (const auto& boneName : bones) {
			AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
			if (!bone)
				continue;

			if (bone->poseRotVec.IsZero() && bone->poseTranVec.IsZero())
				continue;

			PoseBoneData poseBoneData{};
			poseBoneData.name = bone->boneName;
			poseBoneData.rotation = bone->poseRotVec;
			poseBoneData.translation = bone->poseTranVec;
			poseData->boneData.push_back(poseBoneData);
		}

		cPoseName->SetClientData(poseSel, poseData);

		wxString dirName = wxString::FromUTF8(GetProjectPath()) + "/PoseData";
		wxFileName::Mkdir(dirName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

		wxString fileName = dirName + "/" + wxString::FromUTF8(poseData->name) + ".xml";

		PoseDataFile poseDataFile;
		poseDataFile.New(fileName.ToUTF8().data());

		std::vector<PoseData> poses;
		poses.push_back(*poseData);

		poseDataFile.SetData(poses);
		poseDataFile.Save();
	}
	else {
		wxCommandEvent evt;
		OnSaveAsPose(evt);
	}
}

void OutfitStudioFrame::OnSaveAsPose(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cPoseName = (wxChoice*)FindWindowByName("cPoseName");

	wxString poseName;
	do {
		poseName = wxGetTextFromUser(_("Please enter a new unique name for the pose."), _("New Pose"));
		if (poseName.empty())
			return;

	} while (cPoseName->FindString(poseName) != wxNOT_FOUND);

	auto poseData = new PoseData(poseName.ToUTF8().data());

	std::vector<std::string> bones;
	project->GetActiveBones(bones);

	for (const auto& boneName : bones) {
		AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(boneName);
		if (!bone)
			continue;

		if (bone->poseRotVec.IsZero() && bone->poseTranVec.IsZero())
			continue;

		PoseBoneData poseBoneData{};
		poseBoneData.name = bone->boneName;
		poseBoneData.rotation = bone->poseRotVec;
		poseBoneData.translation = bone->poseTranVec;
		poseData->boneData.push_back(poseBoneData);
	}

	int poseSel = cPoseName->Append(poseName, poseData);
	cPoseName->SetSelection(poseSel);

	wxString dirName = wxString::FromUTF8(GetProjectPath()) + "/PoseData";
	wxFileName::Mkdir(dirName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	wxString fileName = dirName + "/" + wxString::FromUTF8(poseData->name) + ".xml";

	PoseDataFile poseDataFile;
	poseDataFile.New(fileName.ToUTF8().data());

	std::vector<PoseData> poses;
	poses.push_back(*poseData);

	poseDataFile.SetData(poses);
	poseDataFile.Save();
}

void OutfitStudioFrame::OnDeletePose(wxCommandEvent& WXUNUSED(event)) {
	wxChoice* cPoseName = (wxChoice*)FindWindowByName("cPoseName");
	int poseSel = cPoseName->GetSelection();
	if (poseSel != wxNOT_FOUND) {
		auto poseData = reinterpret_cast<PoseData*>(cPoseName->GetClientData(poseSel));

		wxString prompt = wxString::Format(_("Are you sure you wish to delete the pose '%s'?"), cPoseName->GetStringSelection());
		int result = wxMessageBox(prompt, _("Confirm pose delete"), wxYES_NO | wxICON_WARNING, this);
		if (result != wxYES)
			return;

		wxString fileName = wxString::FromUTF8(GetProjectPath()) + "/PoseData/" + wxString::FromUTF8(poseData->name) + ".xml";
		wxRemoveFile(fileName);

		cPoseName->Delete(poseSel);
	}
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
		int colorBackgroundR = Config.GetIntValue("Rendering/ColorBackground.r");
		int colorBackgroundG = Config.GetIntValue("Rendering/ColorBackground.g");
		int colorBackgroundB = Config.GetIntValue("Rendering/ColorBackground.b");
		gls.SetBackgroundColor(Vector3(colorBackgroundR / 255.0f, colorBackgroundG / 255.0f, colorBackgroundB / 255.0f));
	}

	if (Config.Exists("Rendering/ColorWire")) {
		int colorWireR = Config.GetIntValue("Rendering/ColorWire.r");
		int colorWireG = Config.GetIntValue("Rendering/ColorWire.g");
		int colorWireB = Config.GetIntValue("Rendering/ColorWire.b");
		gls.SetWireColor(Vector3(colorWireR / 255.0f, colorWireG / 255.0f, colorWireB / 255.0f));
	}

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

		mesh* m = gls.AddMeshFromNif(nif, shapeList[i]);
		if (!m)
			continue;

		NiShape* shape = nif->FindBlockByName<NiShape>(shapeList[i]);
		if (shape && shape->IsSkinned()) {
			// Overwrite skin matrix with the one from AnimInfo
			const auto skinning = os->project->GetWorkAnim()->shapeSkinning.find(shapeList[i]);
			if (skinning != os->project->GetWorkAnim()->shapeSkinning.end()) {
				MatTransform gts = skinning->second.xformGlobalToSkin;
				gts.translation = mesh::VecToMeshCoords(gts.translation);
				m->matModel = glm::inverse(mesh::TransformToMatrix4(gts));
			}
		}

		m->BuildTriAdjacency();
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
	mesh* m = gls.GetMesh(shapeName);
	if (!m)
		return;

	std::string vShader = Config["AppDir"] + "/res/shaders/default.vert";
	std::string fShader = Config["AppDir"] + "/res/shaders/default.frag";

	auto targetGame = (TargetGame)Config.GetIntValue("TargetGame");
	if (targetGame == FO4 || targetGame == FO4VR || targetGame == FO76) {
		vShader = Config["AppDir"] + "/res/shaders/fo4_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/fo4_default.frag";
	}
	else if (targetGame == OB) {
		vShader = Config["AppDir"] + "/res/shaders/ob_default.vert";
		fShader = Config["AppDir"] + "/res/shaders/ob_default.frag";
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
	mesh* m = gls.GetMesh(shapeName);
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

void wxGLPanel::SetActiveTool(ToolID brushID) {
	activeTool = brushID;

	switch (brushID) {
		case ToolID::MaskBrush: activeBrush = &maskBrush; break;
		case ToolID::InflateBrush: activeBrush = &standardBrush; break;
		case ToolID::DeflateBrush: activeBrush = &deflateBrush; break;
		case ToolID::MoveBrush: activeBrush = &moveBrush; break;
		case ToolID::SmoothBrush: activeBrush = &smoothBrush; break;
		case ToolID::UndiffBrush: activeBrush = &undiffBrush; break;
		case ToolID::WeightBrush: activeBrush = &weightBrush; break;
		case ToolID::ColorBrush: activeBrush = &colorBrush; break;
		case ToolID::AlphaBrush: activeBrush = &alphaBrush; break;
		default: activeBrush = nullptr; break;
	}
}

void wxGLPanel::SetLastTool(ToolID tool) {
	lastTool = tool;
}

void wxGLPanel::OnKeys(wxKeyEvent& event) {
	if (!event.HasAnyModifiers()) {
		if (event.GetUnicodeKey() == 'V') {
			wxPoint cursorPos(event.GetPosition());

			int vertIndex;
			if (!gls.GetCursorVertex(cursorPos.x, cursorPos.y, &vertIndex))
				return;

			os->CloseBrushSettings();

			wxDialog dlg;
			if (wxXmlResource::Get()->LoadDialog(&dlg, os, "dlgMoveVertex")) {
				NiShape* shape = os->activeItem->GetShape();

				std::vector<Vector3> verts;
				os->project->GetLiveVerts(shape, verts);

				Vector3 oldPos = verts[vertIndex];
				XRCCTRL(dlg, "posX", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", oldPos.x));
				XRCCTRL(dlg, "posY", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", oldPos.y));
				XRCCTRL(dlg, "posZ", wxTextCtrl)->SetLabel(wxString::Format("%0.5f", oldPos.z));

				if (dlg.ShowModal() == wxID_OK) {
					Vector3 newPos;
					newPos.x = atof(XRCCTRL(dlg, "posX", wxTextCtrl)->GetValue().c_str());
					newPos.y = atof(XRCCTRL(dlg, "posY", wxTextCtrl)->GetValue().c_str());
					newPos.z = atof(XRCCTRL(dlg, "posZ", wxTextCtrl)->GetValue().c_str());

					// Move vertex in shape directly
					if (!os->bEditSlider)
						os->project->MoveVertex(shape, newPos, vertIndex);

					// To mesh coordinates
					oldPos = mesh::VecToMeshCoords(oldPos);
					newPos = mesh::VecToMeshCoords(newPos);

					UndoStateShape uss;
					uss.shapeName = shape->name.get();
					uss.pointStartState[vertIndex] = oldPos;
					uss.pointEndState[vertIndex] = newPos;

					// Push changes onto undo stack and execute
					UndoStateProject* usp = GetUndoHistory()->PushState();
					usp->undoType = UT_VERTPOS;
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
		else if (event.GetUnicodeKey() == '0')
			os->SelectTool(ToolID::Select);
		else if (event.GetUnicodeKey() == '1')
			os->SelectTool(ToolID::MaskBrush);
		else if (event.GetUnicodeKey() == '2')
			os->SelectTool(ToolID::InflateBrush);
		else if (event.GetUnicodeKey() == '3')
			os->SelectTool(ToolID::DeflateBrush);
		else if (event.GetUnicodeKey() == '4')
			os->SelectTool(ToolID::MoveBrush);
		else if (event.GetUnicodeKey() == '5')
			os->SelectTool(ToolID::SmoothBrush);
		else if (event.GetUnicodeKey() == '6')
			os->SelectTool(ToolID::UndiffBrush);
		else if (event.GetUnicodeKey() == '7')
			os->SelectTool(ToolID::WeightBrush);
		else if (event.GetUnicodeKey() == '8')
			os->SelectTool(ToolID::ColorBrush);
		else if (event.GetUnicodeKey() == '9')
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
		else if (event.GetKeyCode() == WXK_ESCAPE)
			os->CloseBrushSettings();
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

	Vector3 o;
	Vector3 n;
	Vector3 d;
	Vector3 s;

	TweakPickInfo tpi;
	bool hit = gls.CollideMeshes(screenPos.x, screenPos.y, tpi.origin, tpi.normal, false, nullptr, bGlobalBrushCollision, &tpi.facet);
	if (!hit)
		return false;

	if (!os->CheckEditableState())
		return false;

	if (activeBrush->isMirrored()) {
		if (!gls.CollideMeshes(screenPos.x, screenPos.y, o, n, true, nullptr, bGlobalBrushCollision, &tpi.facetM))
			tpi.facetM = -1;
	}

	tpi.normal.Normalize();

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
			unweightBrush.setMirror(weightBrush.isMirrored());
			unweightBrush.setConnected(weightBrush.isConnected());
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
			smoothWeightBrush.setMirror(weightBrush.isMirrored());
			smoothWeightBrush.setConnected(weightBrush.isConnected());
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
		if (activeBrush == &standardBrush) {
			deflateBrush.setMirror(activeBrush->isMirrored());
			deflateBrush.setConnected(activeBrush->isConnected());
			activeBrush = &deflateBrush;
		}
		else if (activeBrush == &deflateBrush) {
			standardBrush.setMirror(activeBrush->isMirrored());
			standardBrush.setConnected(activeBrush->isConnected());
			activeBrush = &standardBrush;
		}
		else if (activeBrush == &maskBrush) {
			UnMaskBrush.setStrength(-activeBrush->getStrength());
			UnMaskBrush.setMirror(activeBrush->isMirrored());
			UnMaskBrush.setConnected(activeBrush->isConnected());
			activeBrush = &UnMaskBrush;
		}
		else if (activeBrush == &colorBrush) {
			uncolorBrush.setStrength(-activeBrush->getStrength());
			uncolorBrush.setMirror(activeBrush->isMirrored());
			uncolorBrush.setConnected(activeBrush->isConnected());
			activeBrush = &uncolorBrush;
		}
		else if (activeBrush == &alphaBrush) {
			unalphaBrush.setStrength(-activeBrush->getStrength());
			unalphaBrush.setMirror(activeBrush->isMirrored());
			unalphaBrush.setConnected(activeBrush->isConnected());
			activeBrush = &unalphaBrush;
		}
	}
	else if (activeBrush == &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothMaskBrush.setStrength(activeBrush->getStrength() * 15.0f);
		smoothMaskBrush.setMirror(activeBrush->isMirrored());
		smoothMaskBrush.setConnected(activeBrush->isConnected());
		activeBrush = &smoothMaskBrush;
	}
	else if (activeBrush != &weightBrush && activeBrush != &maskBrush && wxGetKeyState(WXK_SHIFT)) {
		smoothBrush.setMirror(activeBrush->isMirrored());
		smoothBrush.setConnected(activeBrush->isConnected());
		activeBrush = &smoothBrush;
	}

	activeBrush->setRadius(brushSize);

	if (activeBrush->Type() == TBT_WEIGHT) {
		for (auto& sel : os->GetSelectedItems()) {
			os->project->GetWorkNif()->CreateSkinning(sel->GetShape());

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

	if (activeBrush->Type() == TBT_UNDIFF) {
		std::vector<mesh*> refMeshes = activeStroke->GetRefMeshes();

		std::vector<std::vector<Vector3>> positionData;
		positionData.resize(refMeshes.size());

		for (size_t i = 0; i < refMeshes.size(); i++) {
			// Get base vertex positions, not current mesh position
			mesh* m = refMeshes[i];
			std::vector<Vector3> basePosition;

			auto workNif = os->project->GetWorkNif();
			auto shape = workNif->FindBlockByName<NiShape>(m->shapeName);
			if (shape)
				workNif->GetVertsForShape(shape, basePosition);

			for (auto& p : basePosition)
				p = mesh::VecToMeshCoords(p);

			positionData[i] = std::move(basePosition);
		}

		activeStroke->beginStroke(tpi, positionData);
	}
	else
		activeStroke->beginStroke(tpi);

	if (activeBrush->Type() != TBT_MOVE)
		activeStroke->updateStroke(tpi);

	return true;
}

void wxGLPanel::UpdateBrushStroke(const wxPoint& screenPos) {
	Vector3 o;
	Vector3 n;
	Vector3 d; // Mirror pick ray direction.
	Vector3 s; // Mirror pick ray origin.

	TweakPickInfo tpi;

	if (activeStroke) {
		bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision);

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
			if (activeBrush->isMirrored()) {
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
				os->ActiveShapesUpdated(undoHistory.GetCurState(), false);

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

		int brushType = activeStroke->BrushType();
		if (brushType != TBT_MASK) {
			os->ActiveShapesUpdated(undoHistory.GetCurState());

			if (brushType == TBT_WEIGHT) {
				std::string selectedBone = os->GetActiveBone();
				if (!selectedBone.empty()) {
					int boneScalePos = os->boneScale->GetValue();
					if (boneScalePos != 0)
						os->project->ApplyBoneScale(selectedBone, boneScalePos, true);

					os->HighlightBoneNamesWithWeights();
					os->UpdateBoneCounts();
				}
			}

			if (!os->bEditSlider && brushType != TBT_WEIGHT && brushType != TBT_COLOR && brushType != TBT_ALPHA) {
				for (auto& s : os->project->GetWorkNif()->GetShapes()) {
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

			if (os->currentTabButton)
				os->currentTabButton->SetPendingChanges();
		}

		os->UpdateUndoTools();
	}
}

bool wxGLPanel::StartTransform(const wxPoint& screenPos) {
	TweakPickInfo tpi;
	mesh* hitMesh;
	bool hit = gls.CollideOverlay(screenPos.x, screenPos.y, tpi.origin, tpi.normal, &hitMesh, &tpi.facet);
	if (!hit)
		return false;

	if (!os->CheckEditableState())
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

	activeStroke = std::make_unique<TweakStroke>(gls.GetActiveMeshes(), &translateBrush, *undoHistory.PushState());

	if (os->bEditSlider) {
		activeStroke->usp.sliderName = os->activeSlider;

		float sliderscale = os->project->SliderValue(os->activeSlider);
		if (sliderscale == 0.0)
			sliderscale = 1.0;

		activeStroke->usp.sliderscale = sliderscale;
	}

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
	gls.CollidePlane(screenPos.x, screenPos.y, tpi.origin, pn, pd);

	activeStroke->updateStroke(tpi);

	ShowTransformTool();
}

void wxGLPanel::EndTransform() {
	activeStroke->endStroke();
	activeStroke = nullptr;

	os->ActiveShapesUpdated(undoHistory.GetCurState());
	if (!os->bEditSlider) {
		for (auto& s : os->project->GetWorkNif()->GetShapes()) {
			os->UpdateShapeSource(s);
			os->project->RefreshMorphShape(s);
		}
	}

	ShowTransformTool();
	os->UpdateUndoTools();
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

	std::vector<mesh*> strokeMeshes{hitMesh};
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

	std::vector<mesh*> refMeshes = activeStroke->GetRefMeshes();
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
		mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
		if (m) {
			if (wxGetKeyState(WXK_CONTROL))
				m->mask[vertIndex] = 1.0f;
			else if (!segmentMode)
				m->mask[vertIndex] = 0.0f;

			m->QueueUpdate(mesh::UpdateType::Mask);
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

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision, &hitResult);
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
	mesh* m = GetMesh(mouseDownMeshName);
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
	verts.push_back(mouseDownPoint);
	if (m->weldVerts.find(mouseDownPoint) != m->weldVerts.end())
		for (int wv : m->weldVerts[mouseDownPoint])
			verts.push_back(wv);

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
	usp->undoType = UT_MESH;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
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

	bool hit = gls.UpdateCursor(screenPos.x, screenPos.y, bGlobalBrushCollision, &hitResult);
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
	usp->undoType = UT_MESH;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
}

void wxGLPanel::ClickSplitEdge() {
	if (mouseDownMeshName.empty() || mouseDownEdge.p1 < 0 || mouseDownEdge.p1 == mouseDownEdge.p2)
		return;

	mesh* m = GetMesh(mouseDownMeshName);
	if (!m)
		return;

	auto workNif = os->project->GetWorkNif();
	if (!workNif)
		return;

	NiShape* shape = workNif->FindBlockByName<NiShape>(mouseDownMeshName);
	if (!shape)
		return;

	uint16_t maxVertIndex = std::numeric_limits<uint16_t>().max();
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

	// Collect welded vertices
	uint16_t p1 = mouseDownEdge.p1;
	uint16_t p2 = mouseDownEdge.p2;
	std::vector<uint16_t> p1s(1, p1);
	std::vector<uint16_t> p2s(1, p2);

	auto wvit = m->weldVerts.find(p1);
	if (wvit != m->weldVerts.end())
		std::copy(wvit->second.begin(), wvit->second.end(), std::back_inserter(p1s));

	wvit = m->weldVerts.find(p2);
	if (wvit != m->weldVerts.end())
		std::copy(wvit->second.begin(), wvit->second.end(), std::back_inserter(p2s));

	// Prepare list of changes
	UndoStateShape uss;
	uss.shapeName = mouseDownMeshName;
	if (!os->project->PrepareSplitEdge(shape, uss, p1s, p2s)) {
		wxMessageBox(_("The edge picked has multiple triangles of the same orientation.  Correct the orientations before splitting."), _("Error"), wxICON_ERROR, os);
		return;
	}

	// Push changes onto undo stack and execute.
	UndoStateProject* usp = GetUndoHistory()->PushState();
	usp->undoType = UT_MESH;
	usp->usss.push_back(std::move(uss));
	ApplyUndoState(usp, false);

	os->UpdateUndoTools();
}

bool wxGLPanel::RestoreMode(UndoStateProject* usp) {
	bool modeChanged = false;
	int undoType = usp->undoType;
	if (undoType == UT_VERTPOS && os->activeSlider != usp->sliderName) {
		os->EnterSliderEdit(usp->sliderName);
		modeChanged = true;
	}
	if ((undoType != UT_VERTPOS || usp->sliderName.empty()) && os->bEditSlider) {
		os->ExitSliderEdit();
		modeChanged = true;
	}
	if (undoType == UT_VERTPOS && !usp->sliderName.empty() && usp->sliderscale != os->project->SliderValue(usp->sliderName)) {
		os->SetSliderValue(usp->sliderName, usp->sliderscale * 100);
		os->ApplySliders();
		modeChanged = true;
	}
	if (undoType == UT_VERTPOS && usp->sliderName.empty() && !os->project->AllSlidersZero()) {
		os->ZeroSliders();
		modeChanged = true;
	}
	return modeChanged;
}

void wxGLPanel::ApplyUndoState(UndoStateProject* usp, bool bUndo, bool bRender) {
	int undoType = usp->undoType;
	if (undoType == UT_WEIGHT) {
		for (auto& uss : usp->usss) {
			mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& bw : uss.boneWeights) {
				if (bw.boneName != os->GetActiveBone())
					continue;

				for (auto& wIt : bw.weights)
					m->weight[wIt.first] = bUndo ? wIt.second.startVal : wIt.second.endVal;
			}

			m->QueueUpdate(mesh::UpdateType::Weight);
		}

		os->ActiveShapesUpdated(usp, bUndo);

		std::string activeBone = os->GetActiveBone();
		if (!activeBone.empty()) {
			int boneScalePos = os->boneScale->GetValue();
			if (boneScalePos != 0)
				os->project->ApplyBoneScale(activeBone, boneScalePos, true);
		}
	}
	else if (undoType == UT_MASK) {
		for (auto& uss : usp->usss) {
			mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->mask[pit.first] = pit.second.x;

			m->QueueUpdate(mesh::UpdateType::Mask);
		}

		if (undoType != UT_MASK)
			os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UT_COLOR) {
		for (auto& uss : usp->usss) {
			mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->vcolors[pit.first] = pit.second;

			m->QueueUpdate(mesh::UpdateType::VertexColors);
		}

		if (undoType != UT_COLOR)
			os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UT_ALPHA) {
		for (auto& uss : usp->usss) {
			mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->valpha[pit.first] = pit.second.x;

			m->QueueUpdate(mesh::UpdateType::VertexAlpha);
		}

		os->ActiveShapesUpdated(usp, bUndo);
	}
	else if (undoType == UT_VERTPOS) {
		for (auto& uss : usp->usss) {
			mesh* m = GetMesh(uss.shapeName);
			if (!m)
				continue;

			for (auto& pit : (bUndo ? uss.pointStartState : uss.pointEndState))
				m->verts[pit.first] = pit.second;

			m->SmoothNormals();
			BVHUpdateQueue.insert(m);

			m->QueueUpdate(mesh::UpdateType::Position);
		}

		os->ActiveShapesUpdated(usp, bUndo);

		if (usp->sliderName.empty()) {
			for (auto& uss : usp->usss) {
				mesh* m = GetMesh(uss.shapeName);
				if (!m)
					continue;

				auto shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
				if (shape)
					os->project->UpdateShapeFromMesh(shape, m);
			}
		}
	}
	else if (undoType == UT_MESH) {
		for (auto& uss : usp->usss) {
			NiShape* shape = os->project->GetWorkNif()->FindBlockByName<NiShape>(uss.shapeName);
			if (!shape)
				continue;

			os->project->ApplyShapeMeshUndo(shape, uss, bUndo);
		}

		os->RefreshGUIFromProj(false);
		ClearActiveMask();
		os->ApplySliders();
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
	if (pivotMode)
		xformCenter = pivotPosition;
	else
		xformCenter = gls.GetActiveCenter();

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

void wxGLPanel::ShowNodes(bool show) {
	nodesMode = show;

	for (auto& m : nodesPoints)
		m->bVisible = show;

	for (auto& m : nodesLines)
		m->bVisible = show;

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
				Vector3 renderPosition = mesh::VecToMeshCoords(position);

				auto pointMesh = gls.AddVisPoint(renderPosition, "P_" + nodeName);
				if (pointMesh) {
					pointMesh->bVisible = nodesMode;
					nodesPoints.push_back(pointMesh);
				}

				if (parent) {
					Vector3 renderParentPosition = mesh::VecToMeshCoords(parentPosition);

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

	std::function<bool(AnimBone*, const Vector3&)> addChildBones = [&](AnimBone* parent, const Vector3& rootPosition) {
		bool anyBoneInSelection = false;

		for (auto& cb : parent->children) {
			bool childBonesInSelection = addChildBones(cb, rootPosition);

			if (!cb->boneName.empty()) {
				bool boneInSelection = false;
				for (auto& si : os->GetSelectedItems()) {
					if (workAnim->GetShapeBoneIndex(si->GetShape()->name.get(), cb->boneName) != -1) {
						boneInSelection = true;
						break;
					}
				}

				if (boneInSelection || childBonesInSelection) {
					Vector3 position = cb->xformToGlobal.ApplyTransform(rootPosition);
					Vector3 parentPosition = parent->xformToGlobal.ApplyTransform(rootPosition);
					bool matchesParent = position.IsNearlyEqualTo(parentPosition);

					Vector3 renderPosition = mesh::VecToMeshCoords(position);

					auto pointMesh = gls.AddVisPoint(renderPosition, "BP_" + cb->boneName);
					if (pointMesh) {
						pointMesh->bVisible = bonesMode;
						bonesPoints.push_back(pointMesh);
					}

					Vector3 renderParentPosition = mesh::VecToMeshCoords(parentPosition);

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
		addChildBones(rb, rb->xformToParent.translation);

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

	floorMeshes.clear();

	const float floorWidth = 100.0f;
	const float floorWidthHalf = floorWidth / 2.0f;
	const float floorGridStepBig = 5.0f;
	const float floorGridStepSmall = 1.0f;
	const int numLinesBig = (int)(floorWidth / floorGridStepBig) + 1;
	const int numLinesSmall = (int)(floorWidth / floorGridStepSmall) + 1;

	Matrix4 floorMat;
	floorMat.Rotate(90.0f * DEG2RAD, 1.0f, 0.0f, 0.0f);

	Vector3 floorColor(0.0f, 0.0f, 1.0f);
	auto floorMesh = gls.AddVisPlane(floorMat, Vector2(floorWidth, floorWidth), 1.0f, 0.0f, "", &floorColor, true);
	if (floorMesh) {
		floorMesh->prop.alpha = 0.05f;
		floorMesh->bVisible = floorMode;
		floorMeshes.push_back(floorMesh);
	}

	float nextLinePos = floorWidth - floorWidthHalf;

	// Floor with width on X and Y axis (big grid)
	for (int i = 0; i < numLinesBig; i++) {
		Vector3 startPos(floorWidthHalf, 0.0f, nextLinePos);
		Vector3 endPos(-floorWidthHalf, 0.0f, nextLinePos);

		auto lineMesh = gls.AddVisSeg(startPos, endPos, "", true);
		if (lineMesh) {
			lineMesh->color.x = 0.0f;
			lineMesh->color.y = 1.0f;
			lineMesh->color.z = 0.0f;
			lineMesh->bVisible = floorMode;
			floorMeshes.push_back(lineMesh);
		}

		startPos = Vector3(nextLinePos, 0.0f, floorWidthHalf);
		endPos = Vector3(nextLinePos, 0.0f, -floorWidthHalf);

		lineMesh = gls.AddVisSeg(startPos, endPos, "", true);
		if (lineMesh) {
			lineMesh->color.x = 0.0f;
			lineMesh->color.y = 1.0f;
			lineMesh->color.z = 0.0f;
			lineMesh->bVisible = floorMode;
			floorMeshes.push_back(lineMesh);
		}

		nextLinePos -= floorGridStepBig;
	}

	nextLinePos = floorWidth - floorWidthHalf;

	// Floor with width on X and Y axis (small grid)
	for (int i = 0; i < numLinesSmall; i++) {
		Vector3 startPos(floorWidthHalf, 0.0f, nextLinePos);
		Vector3 endPos(-floorWidthHalf, 0.0f, nextLinePos);

		auto lineMesh = gls.AddVisSeg(startPos, endPos, "", true);
		if (lineMesh) {
			lineMesh->color.x = 0.0f;
			lineMesh->color.y = 1.0f;
			lineMesh->color.z = 0.0f;
			lineMesh->prop.alpha = 0.7f;
			lineMesh->bVisible = floorMode;
			floorMeshes.push_back(lineMesh);
		}

		startPos = Vector3(nextLinePos, 0.0f, floorWidthHalf);
		endPos = Vector3(nextLinePos, 0.0f, -floorWidthHalf);

		lineMesh = gls.AddVisSeg(startPos, endPos, "", true);
		if (lineMesh) {
			lineMesh->color.x = 0.0f;
			lineMesh->color.y = 1.0f;
			lineMesh->color.z = 0.0f;
			lineMesh->prop.alpha = 0.7f;
			lineMesh->bVisible = floorMode;
			floorMeshes.push_back(lineMesh);
		}

		nextLinePos -= floorGridStepSmall;
	}
}

void wxGLPanel::ShowVertexEdit(bool show) {
	for (auto& m : gls.GetMeshes())
		if (m)
			m->bShowPoints = false;

	if (show) {
		if (os->activeItem) {
			mesh* m = GetMesh(os->activeItem->GetShape()->name.get());
			if (m) {
				m->bShowPoints = true;
				m->QueueUpdate(mesh::UpdateType::Mask);
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

	if (event.ControlDown()) {
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
	else if (wxGetKeyState(wxKeyCode('S'))) {
		wxPoint p = event.GetPosition();

		if (brushMode) {
			// Adjust brush size
			if (delt < 0)
				DecBrush();
			else
				IncBrush();

			os->CheckBrushBounds();
			os->UpdateBrushSettings();
			gls.UpdateCursor(p.x, p.y, bGlobalBrushCollision);
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

	bool cursorExists = false;
	int x;
	int y;
	event.GetPosition(&x, &y);

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
			gls.TurnTableCamera(x - lastX);
			gls.PitchCamera(y - lastY);
			ShowRotationCenter();
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
		else if (isPickingVertex) {
			UpdatePickVertex(event.GetPosition());
		}
		else if (isPickingEdge) {
			UpdatePickEdge(event.GetPosition());
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
		GLSurface::CursorHitResult hitResult{};

		if (editMode) {
			cursorExists = gls.UpdateCursor(x, y, bGlobalBrushCollision, &hitResult);
		}
		else {
			cursorExists = false;
			gls.ShowCursor(false);
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
				if (activeTool == ToolID::MaskBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Mask: %g", hitResult.hoverPoint, hitResult.hoverColor.x), 1);
				else if (activeTool == ToolID::WeightBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Weight: %g", hitResult.hoverPoint, hitResult.hoverColor.y), 1);
				else if (activeTool == ToolID::ColorBrush || activeTool == ToolID::AlphaBrush)
					os->statusBar->SetStatusText(wxString::Format("Vertex: %d, Color: %g, %g, %g, Alpha: %g",
																  hitResult.hoverPoint,
																  hitResult.hoverColor.x,
																  hitResult.hoverColor.y,
																  hitResult.hoverColor.z,
																  hitResult.hoverAlpha),
												 1);
				else {
					Vector3 hoverCoordNif = mesh::VecToNifCoords(hitResult.hoverMeshCoord);
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

	if (!isLDragging && !isPainting && activeTool == ToolID::Select) {
		int x, y;
		event.GetPosition(&x, &y);
		wxPoint p = event.GetPosition();

		mesh* m = gls.PickMesh(x, y);
		if (m)
			os->SelectShape(m->shapeName);
	}

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

	if (isPickingVertex) {
		EndPickVertex();
		isPickingVertex = false;
	}

	if (isPickingEdge) {
		EndPickEdge();
		isPickingEdge = false;
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

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".obj")) {
				owner->StartProgress("Adding OBJ file...");
				owner->UpdateProgress(1, _("Adding OBJ file..."));
				owner->project->ImportOBJ(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
				owner->project->SetTextures();

				owner->UpdateProgress(60, _("Refreshing GUI..."));
				owner->RefreshGUIFromProj();

				owner->UpdateProgress(100, _("Finished"));
				owner->EndProgress();
			}
			else if (inputFile.Lower().EndsWith(".fbx")) {
				owner->StartProgress(_("Adding FBX file..."));
				owner->UpdateProgress(1, _("Adding FBX file..."));
				owner->project->ImportFBX(inputFile.ToUTF8().data(), dataName.ToUTF8().data(), mergeShape);
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
		for (size_t i = 0; i < fileNames.GetCount(); i++) {
			wxString inputFile;
			inputFile = fileNames.Item(i);

			wxString dataName = inputFile.AfterLast('/').AfterLast('\\');
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
					owner->project->SetSliderFromBSD(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
				else if (isOBJ)
					owner->project->SetSliderFromOBJ(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
				else if (isFBX)
					owner->project->SetSliderFromFBX(targetSlider, owner->activeItem->GetShape(), inputFile.ToUTF8().data());
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


std::string JoinStrings(const std::vector<std::string>& elements, const char* const separator) {
	switch (elements.size()) {
		case 0: return "";
		case 1: return elements[0];
		default:
			std::ostringstream os;
			std::copy(elements.begin(), elements.end() - 1, std::ostream_iterator<std::string>(os, separator));
			os << *elements.rbegin();
			return os.str();
	}
}
