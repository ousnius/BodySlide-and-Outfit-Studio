/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Object3d.hpp"

#include <vector>
#include <wx/event.h>
#include <wx/treebase.h>

class Mesh;
class OutfitStudioFrame;
class wxTreeCtrl;
class wxTreeEvent;

namespace nifly {
class NiShape;
}

// The segments (Fallout 4 and 76) and partitions (Skyrim and older) of the active shape of an Outfit Studio
// window, with the trees and controls of their tabs.
class SegmentPartitionEditor : public wxEvtHandler {
public:
	explicit SegmentPartitionEditor(OutfitStudioFrame& frame);

	// Finds the trees and controls in the window and binds their events
	void Init();

	wxTreeCtrl* GetSegmentTree() const { return segmentTree; }
	wxTreeCtrl* GetPartitionTree() const { return partitionTree; }
	// Forgets the selected segment and partition, for a project that is unloaded
	void ClearActive();

	void CreateSegmentTree(nifly::NiShape* shape = nullptr);
	void CreatePartitionTree(nifly::NiShape* shape = nullptr);
	void RefreshActiveSegmentTree();
	void RefreshActivePartitionTree();

	void ShowSegment(const wxTreeItemId& item = nullptr);
	void ShowPartition(const wxTreeItemId& item = nullptr);
	void ResetSegmentVisibility();
	void ResetPartitionVisibility();
	bool GrowShrinkSegmentPartitionSelection(bool grow);
	bool PaintSegmentPartitionTriangles(Mesh* hitMesh, int hitTri, const nifly::Vector3& hitPointModel, float radiusModel);

	void ApplySegments();
	void ResetSegments();
	void ApplyPartitions();
	void ResetPartitions();

	void SetNoSubMeshes(Mesh* m);
	void SetNoSubMeshes();

	int CopySegPartForShapes(std::vector<nifly::NiShape*> shapes, bool silent = false);

	// The Delete key on the tabs
	void DeleteSelectedSegment();
	void DeleteSelectedPartition();

private:
	void UpdateSegmentNames();
	void ApplySegmentVisibility(Mesh* m);
	void UpdatePartitionNames();
	void ApplyPartitionVisibility(Mesh* m);
	void SetSubMeshesForPartitions(Mesh* m, const std::vector<int>& tp);

	int CalcMaxSegPartID();
	void OnSegmentSelect(wxTreeEvent& event);
	void OnSegmentVisToggle(wxTreeEvent& event);
	void OnSegmentContext(wxTreeEvent& event);
	void OnSegmentTreeContext(wxCommandEvent& event);
	void OnAddSegment(wxCommandEvent& event);
	void OnAddSubSegment(wxCommandEvent& event);
	void OnDeleteSegment(wxCommandEvent& event);
	void OnDeleteSubSegment(wxCommandEvent& event);

	void UpdateActiveSlotID();
	void OnSegmentSlotChanged(wxCommandEvent& event);

	void OnSegmentTypeChanged(wxCommandEvent& event);
	void OnSegmentApply(wxCommandEvent& event);
	void OnSegmentReset(wxCommandEvent& event);
	void OnSegmentEditSSF(wxCommandEvent& event);

	void OnPartitionSelect(wxTreeEvent& event);
	void OnPartitionVisToggle(wxTreeEvent& event);
	void OnPartitionContext(wxTreeEvent& event);
	void OnPartitionTreeContext(wxCommandEvent& event);
	void OnAddPartition(wxCommandEvent& event);
	void OnDeletePartition(wxCommandEvent& event);
	void OnPartitionTypeChanged(wxCommandEvent& event);
	void OnPartitionApply(wxCommandEvent& event);
	void OnPartitionReset(wxCommandEvent& event);

	void OnCopySegPart(wxCommandEvent& event);

	OutfitStudioFrame& frame;

	wxTreeCtrl* segmentTree = nullptr;
	wxTreeCtrl* partitionTree = nullptr;
	wxTreeItemId segmentRoot;
	wxTreeItemId partitionRoot;
	wxTreeItemId activeSegment;
	wxTreeItemId activePartition;
	std::vector<int> triParts;	// the partition index for each triangle, or -1 for none
	std::vector<int> triSParts; // the segment partition index for each triangle, or -1 for none
};
