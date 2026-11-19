/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// Automation steps: masks

#include "AutomationRunner.h"
#include "OutfitProject.h"
#include "ProjectOperations.h"
#include "StudioSession.h"
#include "components/MaskOps.h"
#include "components/MeshAnalysis.h"
#include "files/MaskFile.h"
#include "utils/StringStuff.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <NifFile.hpp>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

using namespace nifly;

int AutomationRunner::ExecuteStepClearMask(const AutomationStep& step) {
	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: ClearMask - no target shapes found.");
		return 0;
	}

	for (auto* shape : targetShapes) {
		std::string shapeName = shape->name.get();
		Mesh* mesh = session->GetMesh(shapeName);
		if (!mesh)
			continue;

		wxLogMessage("Automation: Clearing mask for shape '%s'...", shapeName);
		mesh->MaskFill(0.0f);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepLoadMask(const AutomationStep& step) {
	if (step.loadMaskFile.empty()) {
		wxLogError("Automation: LoadMask - no mask file specified.");
		return 1;
	}

	if (step.loadMaskName.empty()) {
		wxLogError("Automation: LoadMask - no mask name specified.");
		return 1;
	}

	wxString loadMaskFile = MakeAbsoluteToProject(wxString::FromUTF8(step.loadMaskFile));
	std::string loadMaskFileStd = loadMaskFile.ToUTF8().data();
	MaskFile maskFile;
	int maskErr = maskFile.Load(loadMaskFileStd);
	if (maskErr) {
		wxLogError("Automation: LoadMask - failed to load file '%s' (error %d).", loadMaskFile, maskErr);
		return 1;
	}

	const MaskEntry* entry = maskFile.FindEntry(step.loadMaskName);
	if (!entry) {
		wxLogError("Automation: LoadMask - mask name '%s' not found in file '%s'.",
			step.loadMaskName, loadMaskFile);
		return 1;
	}

	auto targetShapes = ResolveTargetShapes(step);
	if (targetShapes.empty()) {
		wxLogWarning("Automation: LoadMask - no target shapes found.");
		return 0;
	}

	for (auto* shape : targetShapes) {
		std::string shapeName = shape->name.get();
		Mesh* mesh = session->GetMesh(shapeName);
		if (!mesh)
			continue;

		const MaskShapeData* matched = entry->FindMatchingMask(shapeName, mesh->nVerts);
		if (!matched) {
			wxLogWarning("Automation: LoadMask - no matching mask found for shape '%s' (vertex count: %d).",
				shapeName, mesh->nVerts);
			continue;
		}

		wxLogMessage("Automation: Loading mask '%s' onto shape '%s' (matched from '%s')...",
			step.loadMaskName, shapeName, matched->name);
		auto maskCopy = matched->mask;
		session->SetShapeMask(maskCopy, shapeName);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepGrowShrinkMask(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: GrowShrinkMask - no target shapes found.");
		return 0;
	}

	const bool grow = step.maskGrowShrinkMode == 0;
	const int iterations = std::max(1, step.maskGrowShrinkCount);

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		wxLogMessage("Automation: %s the mask of '%s' %d time(s)...", grow ? "Growing" : "Shrinking", shapeName, iterations);

		for (int i = 0; i < iterations; i++)
			MaskOps::ApplyChanges(*m, MaskOps::GrowShrinkChanges(*m, grow));
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskBox(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskBox - no target shapes found.");
		return 0;
	}

	const Vector3 lo(step.maskBoxMinX, step.maskBoxMinY, step.maskBoxMinZ);
	const Vector3 hi(step.maskBoxMaxX, step.maskBoxMaxY, step.maskBoxMaxZ);

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		// Positions as the shape is shown now, in NIF units
		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);

		std::unordered_map<int, float> changes;
		for (int v = 0; v < m->nVerts && v < static_cast<int>(verts.size()); v++) {
			const Vector3& p = verts[v];
			const bool inside = p.x >= lo.x && p.x <= hi.x && p.y >= lo.y && p.y <= hi.y && p.z >= lo.z && p.z <= hi.z;
			if (inside)
				changes[v] = 1.0f;
			else if (!step.maskBoxKeepOthers)
				changes[v] = 0.0f;
		}

		wxLogMessage("Automation: Masking the vertices of '%s' inside the box...", shapeName);
		MaskOps::ApplyChanges(*m, changes);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskCovered(const AutomationStep& step) {
	// Without target meshes, the body under the outfit
	std::vector<NiShape*> shapes;
	if (step.targetMeshes.empty()) {
		if (NiShape* baseShape = project->GetBaseShape())
			shapes.push_back(baseShape);
	}
	else
		shapes = ResolveTargetShapes(step, true, true);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskCovered - no target shapes found.");
		return 0;
	}

	std::vector<NiShape*> covers;
	if (step.maskCoverShapes.empty()) {
		for (auto* shape : project->GetWorkNif()->GetShapes())
			if (!project->IsBaseShape(shape) && std::find(shapes.begin(), shapes.end(), shape) == shapes.end())
				covers.push_back(shape);
	}
	else {
		for (auto& name : step.maskCoverShapes) {
			NiShape* shape = FindShapeByName(name);
			if (!shape) {
				wxLogWarning("Automation: MaskCovered - cover shape '%s' not found, skipping.", name);
				continue;
			}
			covers.push_back(shape);
		}
	}
	if (covers.empty()) {
		wxLogWarning("Automation: MaskCovered - no cover shapes found.");
		return 0;
	}

	// The cover shapes as they are shown now
	std::vector<std::vector<Vector3>> coverVerts(covers.size());
	std::vector<std::vector<Triangle>> coverTris(covers.size());
	for (size_t c = 0; c < covers.size(); c++) {
		project->GetLiveVerts(covers[c], coverVerts[c]);
		covers[c]->GetTriangles(coverTris[c]);
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		std::vector<Vector3> verts;
		project->GetLiveVerts(shape, verts);
		std::vector<Triangle> tris;
		shape->GetTriangles(tris);
		// Covered by any of them
		std::vector<bool> covered(verts.size(), false);
		for (size_t c = 0; c < covers.size(); c++) {
			std::vector<bool> byCover = MeshAnalysis::CoveredVertices(verts, tris, coverVerts[c], coverTris[c], step.maskCoverOutside, step.maskCoverInside);
			for (size_t v = 0; v < covered.size() && v < byCover.size(); v++)
				if (byCover[v])
					covered[v] = true;
		}

		std::unordered_map<int, float> changes;
		size_t count = 0;
		for (int v = 0; v < m->nVerts && v < static_cast<int>(covered.size()); v++) {
			changes[v] = covered[v] ? 1.0f : 0.0f;
			if (covered[v])
				count++;
		}

		wxLogMessage("Automation: Masked %zu covered vertices of '%s'.", count, shapeName);
		MaskOps::ApplyChanges(*m, changes);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepSmoothMask(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SmoothMask - no target shapes found.");
		return 0;
	}

	const int iterations = std::max(1, step.maskSmoothCount);
	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		wxLogMessage("Automation: Smoothing the mask of '%s' %d time(s)...", shapeName, iterations);
		for (int i = 0; i < iterations; i++)
			MaskOps::ApplyChanges(*m, MaskOps::SmoothChanges(*m));
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepInvertMask(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: InvertMask - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		wxLogMessage("Automation: Inverting the mask of '%s'...", shapeName);
		MaskOps::ApplyChanges(*m, MaskOps::InvertChanges(*m));
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskWeighted(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskWeighted - no target shapes found.");
		return 0;
	}

	auto* workAnim = project->GetWorkAnim();
	if (!workAnim) {
		wxLogError("Automation: MaskWeighted - no project loaded.");
		return 1;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		wxLogMessage("Automation: Masking the weighted vertices of '%s'...", shapeName);
		m->MaskFill(0.0f);

		auto boneIt = workAnim->shapeBones.find(shapeName);
		if (boneIt == workAnim->shapeBones.end())
			continue;

		for (const auto& boneName : boneIt->second) {
			auto* weights = workAnim->GetWeightsPtr(shapeName, boneName);
			if (!weights)
				continue;

			for (const auto& bw : *weights)
				if (bw.second > 0.0f && bw.first < m->nVerts)
					m->mask[bw.first] = 1.0f;
		}

		m->QueueUpdate(Mesh::UpdateType::Mask);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskBoneWeighted(const AutomationStep& step) {
	if (step.maskBoneNames.empty()) {
		wxLogError("Automation: MaskBoneWeighted - no bone name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskBoneWeighted - no target shapes found.");
		return 0;
	}

	auto* workAnim = project->GetWorkAnim();
	if (!workAnim) {
		wxLogError("Automation: MaskBoneWeighted - no project loaded.");
		return 1;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		wxLogMessage("Automation: Masking the vertices of '%s' weighted to %s...", shapeName, JoinStrings(step.maskBoneNames, ", "));
		m->MaskFill(0.0f);

		for (const auto& boneName : step.maskBoneNames) {
			auto* weights = workAnim->GetWeightsPtr(shapeName, boneName);
			if (!weights)
				continue;

			for (const auto& bw : *weights)
				if (bw.second > 0.0f && bw.first < m->nVerts)
					m->mask[bw.first] = 1.0f;
		}

		m->QueueUpdate(Mesh::UpdateType::Mask);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskSliderAffected(const AutomationStep& step) {
	if (step.maskSliderName.empty()) {
		wxLogError("Automation: MaskSliderAffected - no slider name specified.");
		return 1;
	}

	if (!project->ValidSlider(step.maskSliderName)) {
		wxLogError("Automation: MaskSliderAffected - slider '%s' not found.", step.maskSliderName);
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskSliderAffected - no target shapes found.");
		return 0;
	}

	for (auto* shape : shapes) {
		wxLogMessage("Automation: Masking the vertices of '%s' affected by slider '%s'...", shape->name.get(), step.maskSliderName);
		project->MaskAffected(step.maskSliderName, shape);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepMaskAsymmetric(const AutomationStep& step) {
	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: MaskAsymmetric - no target shapes found.");
		return 0;
	}

	const bool byTriangles = step.asymMaskMode == 0;

	if (!byTriangles && !step.asymDoPositions && !step.asymDoUnmatched && !step.asymDoSliders && !step.asymDoBones) {
		wxLogWarning("Automation: MaskAsymmetric - no kind of asymmetry selected.");
		return 0;
	}

	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m || !m->mask)
			continue;

		// Both modes leave the asymmetric vertices unmasked and mask everything
		// else, so the following steps operate on the asymmetries. Like the menu
		// items, this only ever adds to the mask.
		std::vector<bool> keepUnmasked;

		if (byTriangles) {
			wxLogMessage("Automation: Masking everything but the asymmetric triangles of '%s'...", shapeName);
			keepUnmasked = project->CalculateAsymmetricTriangleVertexMask(shape, m->weldVerts);
		}
		else {
			wxLogMessage("Automation: Masking everything but the asymmetric vertices of '%s'...", shapeName);

			std::vector<bool> selVerts(m->nVerts);
			for (int i = 0; i < m->nVerts; i++)
				selVerts[i] = m->mask[i] == 0.0f;

			SymmetricVertices symVerts;
			project->MatchSymmetricVertices(shape, m->weldVerts, symVerts);

			VertexAsymmetries asyms;
			project->FindVertexAsymmetries(shape, symVerts, m->weldVerts, asyms);

			VertexAsymmetryTasks tasks;
			tasks.doPos = step.asymDoPositions;
			tasks.doUnmatched = step.asymDoUnmatched;
			tasks.doSliders.resize(asyms.sliders.size(), step.asymDoSliders);
			tasks.doBones.resize(asyms.bones.size(), step.asymDoBones);

			keepUnmasked = CalcVertexListForAsymmetryTasks(symVerts, asyms, tasks, m->nVerts);
			AddWeldedToVertexList(m->weldVerts, keepUnmasked);

			// Vertices that were already masked stay masked
			for (int i = 0; i < m->nVerts; i++)
				if (!selVerts[i])
					keepUnmasked[i] = false;
		}

		for (int i = 0; i < m->nVerts; i++)
			if (i >= static_cast<int>(keepUnmasked.size()) || !keepUnmasked[i])
				m->mask[i] = 1.0f;

		m->QueueUpdate(Mesh::UpdateType::Mask);
	}

	session->Render();
	return 0;
}

int AutomationRunner::ExecuteStepSaveMask(const AutomationStep& step) {
	if (step.saveMaskFile.empty()) {
		wxLogError("Automation: SaveMask - no mask file specified.");
		return 1;
	}

	if (step.saveMaskName.empty()) {
		wxLogError("Automation: SaveMask - no mask name specified.");
		return 1;
	}

	auto shapes = ResolveTargetShapes(step);
	if (shapes.empty()) {
		wxLogWarning("Automation: SaveMask - no target shapes found.");
		return 0;
	}

	std::map<std::string, std::unordered_map<uint16_t, float>> maskData;
	std::map<std::string, int> vertexCounts;
	for (auto* shape : shapes) {
		std::string shapeName = shape->name.get();
		Mesh* m = session->GetMesh(shapeName);
		if (!m)
			continue;

		std::unordered_map<uint16_t, float> mask;
		session->GetShapeMask(mask, shapeName);
		maskData[shapeName] = std::move(mask);
		vertexCounts[shapeName] = m->nVerts;
	}

	if (maskData.empty()) {
		wxLogWarning("Automation: SaveMask - none of the target shapes have a mesh to read a mask from.");
		return 0;
	}

	wxString saveMaskFile = MakeAbsoluteToProject(wxString::FromUTF8(step.saveMaskFile));
	std::string saveMaskFileStd = saveMaskFile.ToUTF8().data();

	MaskFile maskFile;
	if (step.saveMaskMerge && wxFileName::FileExists(saveMaskFile)) {
		int loadErr = maskFile.Load(saveMaskFileStd);
		if (loadErr) {
			wxLogError("Automation: SaveMask - failed to read the existing file '%s' (error %d).", saveMaskFile, loadErr);
			return 1;
		}
	}

	MaskEntry entry;
	entry.name = step.saveMaskName;
	entry.SetFromMaskData(maskData, vertexCounts);

	auto& entries = maskFile.GetEntries();
	auto existing = std::find_if(entries.begin(), entries.end(), [&step](const MaskEntry& e) { return e.name == step.saveMaskName; });
	if (existing != entries.end())
		*existing = std::move(entry);
	else
		entries.push_back(std::move(entry));

	wxString maskFilePath = wxFileName(saveMaskFile).GetPath();
	if (!maskFilePath.IsEmpty())
		wxFileName::Mkdir(maskFilePath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	int err = maskFile.Save(saveMaskFileStd);
	if (err) {
		wxLogError("Automation: SaveMask - failed to write '%s' (error %d).", saveMaskFile, err);
		return 1;
	}

	wxLogMessage("Automation: Saved mask '%s' for %zu shape(s) to '%s'.", step.saveMaskName, maskData.size(), saveMaskFile);
	return 0;
}
