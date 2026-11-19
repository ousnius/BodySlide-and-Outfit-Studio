/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: loading the reference, slider sets and base projects

#include "OutfitProject.h"
#include "ProjectNifHelpers.h"
#include "components/AddOnSliders.h"
#include "components/SliderDataFileUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringStuff.h"
#include "NifUtil.hpp"
#include "utils/ProjectUtil.h"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <filesystem>
#include <limits>
#include <regex>
#include <sstream>
#include <wx/log.h>

using namespace nifly;

void OutfitProject::ClearReference() {
	DeleteShape(baseShape);

	if (activeSet.size() > 0)
		activeSet.Clear();
}

void OutfitProject::ClearOutfit() {
	for (auto& s : workNif.GetShapes()) {
		if (IsBaseShape(s))
			continue;

		DeleteShape(s);
	}
	ClearWorkSliders();
}

int OutfitProject::LoadReferenceTemplate(
	const std::string& sourceFile, const std::string& set, const std::string& shape, bool loadAll, bool mergeSliders, bool mergeZaps, bool appendNewSliders) {
	if (sourceFile.empty() || set.empty()) {
		wxLogError("Template source entries are invalid.");
		host->ShowMessage(HostMessageLevel::Error, _("Template source entries are invalid."), _("Reference Error"));
		return 1;
	}

	if (loadAll) {
		host->StartSubProgress(10, 20);
		return AddFromSliderSet(sourceFile, set, false, appendNewSliders);
	}
	else
		return LoadReference(sourceFile, set, shape, mergeSliders, mergeZaps, appendNewSliders);
}

int OutfitProject::LoadReferenceNif(const std::string& fileName, const std::string& shapeName, bool mergeSliders, bool mergeZaps) {
	if (mergeZaps || mergeSliders) {
		host->DeleteSlidersForMerge(mergeSliders, mergeZaps);
		DeleteShape(baseShape);
	}
	else
		ClearReference();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, fileName, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), fileName, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			host->ShowMessage(HostMessageLevel::Error, errorText, _("Reference Error"));
			return 3;
		}

		wxLogError("Could not load reference NIF file '%s'!", fileName);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load reference NIF file '%s'!"), fileName), _("Reference Error"));
		return 2;
	}

	ValidateNIF(refNif, fileName);
	ResolveTargetConflictsForIncomingShapes({{shapeName, shapeName}});

	std::vector<std::string> deletedShapes;

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shapeName);
	auto refShape = refNif.FindBlockByName<NiShape>(shapeName);
	if (refShapeDup && refShape) {
		// Delete shape with identical name
		DeleteShape(refShapeDup);
		deletedShapes.push_back(shapeName);
	}

	clothData.Capture(refNif, fileName, {refShape}, IsSkeletonBone);

	// Record physics XML links of the reference before it is merged in
	CaptureRootPhysicsData(refNif);
	CapturePhysicsFiles(refNif, {refShape});

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shapeName, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);

		// CloneShape leaves the root behind, so bring its extra data over
		MergeRootExtraData(refNif);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto& s : workNif.GetShapes())
			if (s->name != shapeName)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shapeName);
	activeSet.LoadSetDiffData(baseDiffData);

	// Clear reference source info (NIF-only reference, no OSP available)
	mRefProjectFile.clear();
	mRefProjectName.clear();
	mRefShapeName.clear();

	if (baseShape) {
		mRefShapeName = baseShape->name.get();
		SetShapeReferenceSource(mRefShapeName, "", "");
	}

	if (!deletedShapes.empty()) {
		std::string shapesJoin = JoinStrings(deletedShapes, "; ");
		host->ShowMessage(HostMessageLevel::Warning, wxString::Format("%s\n \n%s", _("The following shapes were deleted. Rename the duplicates yourself beforehand if you wish to keep them."), shapesJoin),
					 _("Deleted Shapes"));
	}

	return 0;
}

int OutfitProject::LoadReference(const std::string& fileName, const std::string& setName, const std::string& shapeName, bool mergeSliders, bool mergeZaps, bool appendNewSliders) {
	if (mergeZaps || mergeSliders) {
		host->DeleteSlidersForMerge(mergeSliders, mergeZaps);
		DeleteShape(baseShape);
	}
	else
		ClearReference();

	SliderSetFile sset(fileName);
	if (sset.fail()) {
		wxLogError("Could not load slider set file '%s'!", fileName);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load slider set file '%s'!"), fileName), _("Reference Error"));
		return 1;
	}

	std::string dataFolder = activeSet.GetDefaultDataFolder();
	std::vector<std::string> dataNames = activeSet.GetLocalData(shapeName);

	SliderSet refSet;
	if (sset.GetSet(setName, refSet, appendNewSliders)) {
		wxLogError("Could not load set '%s' from slider set file '%s'!", setName, fileName);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load set '%s' from slider set file '%s'!"), setName, fileName), _("Reference Error"));
		return 1;
	}

	refSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
	std::string refFile = refSet.GetInputFileName();

	std::fstream file;
	PlatformUtil::OpenFileStream(file, refFile, std::ios::in | std::ios::binary);

	NifFile refNif;
	int error = refNif.Load(file);
	if (error) {
		if (error == 2) {
			wxString errorText = wxString::Format(_("NIF version not supported!\n\nFile: %s\n%s"), refFile, refNif.GetHeader().GetVersion().GetVersionInfo());

			wxLogError(errorText);
			host->ShowMessage(HostMessageLevel::Error, errorText, _("Reference Error"));
			ClearReference();
			return 5;
		}

		ClearReference();
		wxLogError("Could not load reference NIF file '%s'!", refFile);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Could not load reference NIF file '%s'!"), refFile), _("Reference Error"));
		return 2;
	}

	ValidateNIF(refNif, refFile);

	std::vector<std::string> shapes = refNif.GetShapeNames();
	if (shapes.empty()) {
		ClearReference();
		wxLogError("Reference NIF file '%s' does not contain any shapes.", refFile);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Reference NIF file '%s' does not contain any shapes."), refFile), _("Reference Error"));
		return 3;
	}

	std::string shape = shapeName;
	if (shape.empty())
		shape = shapes[0];

	auto refShape = refNif.FindBlockByName<NiShape>(shape);
	if (!refShape) {
		ClearReference();
		wxLogError("Shape '%s' not found in reference NIF file '%s'!", shape, refFile);
		host->ShowMessage(HostMessageLevel::Error, wxString::Format(_("Shape '%s' not found in reference NIF file '%s'!"), shape, refFile), _("Reference Error"));
		return 4;
	}

	ResolveTargetConflictsForIncomingShapes({{shape, refSet.ShapeToTarget(shape)}});
	// Only the reference shape is taken from the set
	sset.GetSet(setName, activeSet, appendNewSliders, shape);
	activeSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

	std::vector<std::string> deletedShapes;

	auto refShapeDup = workNif.FindBlockByName<NiShape>(shape);
	if (refShapeDup) {
		// Delete shape with identical name
		DeleteShape(refShapeDup);
		deletedShapes.push_back(shape);
	}

	clothData.Capture(refNif, refFile, {refShape}, IsSkeletonBone);

	// Record physics XML links of the reference before it is merged in
	CaptureRootPhysicsData(refNif);
	CapturePhysicsFiles(refNif, {refShape});

	if (workNif.IsValid()) {
		// Copy only reference shape
		auto clonedShape = workNif.CloneShape(refShape, shape, &refNif);
		workAnim.LoadFromNif(&workNif, clonedShape);

		// CloneShape leaves the root behind, so bring its extra data over
		MergeRootExtraData(refNif);
	}
	else {
		// Copy the full file
		workNif.CopyFrom(refNif);
		workAnim.LoadFromNif(&workNif);

		// Delete all except for reference
		for (auto& s : workNif.GetShapes())
			if (s->name != shape)
				DeleteShape(s);
	}

	baseShape = workNif.FindBlockByName<NiShape>(shape);

	if (mergeSliders)
		activeSet.LoadSetDiffData(baseDiffData, shape);
	else
		activeSet.LoadSetDiffData(baseDiffData);

	bool hadLocalData = false;
	activeSet.SetReferencedData(shape, false, &hadLocalData);
	for (auto& dn : dataNames)
		activeSet.SetReferencedDataByName(shape, dn, true);

	std::string refDataFolder = activeSet.GetDefaultDataFolder();
	if (hadLocalData && !refDataFolder.empty() && refDataFolder != dataFolder) {
		// Add data folder of reference to list of data folders of new reference target
		std::string targetName = ShapeToTarget(shapeName);
		activeSet.AddTargetDataFolder(targetName, refDataFolder);
	}

	// Keep default data folder from current project if existing
	if (!dataFolder.empty())
		activeSet.SetDataFolder(dataFolder);

	// Remember reference source info for saved projects
	{
		wxFileName refFileName(wxString::FromUTF8(fileName));
		if (refFileName.IsRelative())
			refFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

		if (refFileName.MakeRelativeTo(wxString::FromUTF8(ProjectUtil::GetProjectPath())))
			mRefProjectFile = refFileName.GetFullPath().ToUTF8().data();
		else
			mRefProjectFile = fileName;

		mRefProjectName = setName;
		mRefShapeName = shape;
		SetShapeReferenceSource(shape, mRefProjectFile, mRefProjectName);
	}

	if (!deletedShapes.empty()) {
		std::string shapesJoin = JoinStrings(deletedShapes, "; ");
		host->ShowMessage(HostMessageLevel::Warning, wxString::Format("%s\n \n%s",
									  _("The following shapes were deleted. Rename the duplicates yourself beforehand if you wish to keep them."),
									  shapesJoin),
					 _("Deleted Shapes"));
	}

	return 0;
}

std::vector<std::string> OutfitProject::LoadReferenceAddOns() {
	std::vector<std::string> addedSliders;
	if (!baseShape || mRefProjectFile.empty() || mRefProjectName.empty())
		return addedSliders;

	const std::string projectPath = ProjectUtil::GetProjectPath();
	const std::string shapeDataPath = projectPath + PathSepStr + "ShapeData";
	const std::string shapeName = baseShape->name.get();

	wxFileName refFileName(wxString::FromUTF8(mRefProjectFile));
	if (refFileName.IsRelative())
		refFileName.MakeAbsolute(wxString::FromUTF8(projectPath));

	SliderSetFile refFile(refFileName.GetFullPath().ToUTF8().data());
	SliderSet refSet;
	if (refFile.fail() || refFile.GetSet(mRefProjectName, refSet) != 0)
		return addedSliders;

	refSet.SetBaseDataPath(shapeDataPath);

	ProjectCatalog catalog;
	catalog.Load(projectPath);

	std::vector<std::string> warnings;
	const std::string refFilePath = refFileName.GetFullPath().ToUTF8().data();
	SetStackResult stack = StackSetAddOns(refSet, catalog, warnings, true, refFilePath);

	if (!stack.baseAddOns.empty()) {
		NifFile refNif;
		std::fstream file;
		PlatformUtil::OpenFileStream(file, refSet.GetInputFileName(), std::ios::in | std::ios::binary);
		if (refNif.Load(file) == 0) {
			std::vector<StackedAddOn> variantBaseAddOns;
			std::vector<StackedAddOn> otherBaseAddOns;
			for (auto& addOn : stack.baseAddOns)
				(addOn.otherBase ? otherBaseAddOns : variantBaseAddOns).push_back(addOn);

			// The add-ons of a body variant's base body only fit shapes that have the vertices their data was made for
			if (!variantBaseAddOns.empty())
				FitBaseAddOnsToVariant(refSet, refNif, nullptr, variantBaseAddOns, catalog, shapeDataPath, nullptr, false, warnings);

			// Add-ons made for another project of the body are conformed to the reference
			if (!otherBaseAddOns.empty()) {
				AnimInfo refAnim;
				LoadConformAnim(refAnim, refNif);
				auto loadNif = [](const std::string& fileName, NifFile& loadInto) {
					std::fstream nifFile;
					PlatformUtil::OpenFileStream(nifFile, fileName, std::ios::in | std::ios::binary);
					return loadInto.Load(nifFile) == 0;
				};
				FitBaseAddOnsToVariant(refSet, refNif, &refAnim, otherBaseAddOns, catalog, shapeDataPath, loadNif, true, warnings);
			}
		}
	}

	for (auto& warning : warnings)
		wxLogWarning("Reference add-ons: %s", wxString::FromUTF8(warning));

	const std::string refTarget = refSet.ShapeToTarget(shapeName);
	const std::string target = ShapeToTarget(shapeName);

	DiffDataSets refDiffs;
	refSet.LoadSetDiffData(refDiffs, shapeName);

	for (size_t i = 0; i < refSet.size(); i++) {
		const SliderData& refSlider = refSet[i];
		if (refSlider.stackedFrom.empty() || activeSet.SliderExists(refSlider.name))
			continue;

		auto refData = std::find_if(refSlider.dataFiles.begin(), refSlider.dataFiles.end(), [&refTarget](const DiffInfo& df) { return df.targetName == refTarget; });
		if (refData == refSlider.dataFiles.end())
			continue;

		TargetDataDiffs* diffs = refDiffs.GetDiffSet(refData->dataName);
		if (!diffs)
			continue;

		SliderData slider = refSlider;
		slider.dataFiles.clear();
		slider.bInherited = false;
		slider.stackedFrom.clear();
		slider.curValue = 0.0f;

		// Keeping the slider off outfits is up to the body's add-on, not the outfit
		slider.bNoAutoConform = false;

		// The slider needs the add-on to be installed
		slider.addOnName = refSlider.stackedFrom;
		const ProjectCatalogEntry* addOn = catalog.Find(refSlider.stackedFrom);
		slider.addOnID = addOn ? addOn->projectID : "";

		DiffInfo data = *refData;
		if (data.bGenerated) {
			// Data conformed from the add-on's base project has no file, it becomes the project's own
			data = DiffInfo(true, data.dataName, target);
		}
		else {
			// The data stays in the add-on's file, which the reference shape finds in the reference's data folders
			std::vector<std::string> dataFolders;
			if (data.bLocal)
				dataFolders.push_back(refSet.GetDefaultDataFolder());
			else
				dataFolders = refSet.GetTargetDataFolders(refTarget);

			data.bLocal = false;
			data.bInherited = false;
			data.targetName = target;
			for (auto& dataFolder : dataFolders)
				activeSet.AddTargetDataFolder(target, dataFolder);
		}

		slider.dataFiles.push_back(data);
		activeSet.CopySlider(&slider);
		baseDiffData.LoadSet(data.dataName, target, *diffs);

		addedSliders.push_back(slider.name);
	}

	if (!addedSliders.empty())
		wxLogMessage("Loaded %zu slider(s) of add-ons of the reference: %s", addedSliders.size(), JoinStrings(addedSliders, ", "));

	return addedSliders;
}

int OutfitProject::LoadFromSliderSet(const std::string& fileName,
									 const std::string& sliderSetName,
									 std::vector<std::string>* origShapeOrder,
									 const std::string& shapeDataPath) {
	host->StartProgress(_("Loading slider set..."));
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		host->EndProgress();
		return 1;
	}

	UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, activeSet)) {
		host->EndProgress();
		return 3;
	}

	activeSet.SetBaseDataPath(shapeDataPath.empty() ? ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData" : shapeDataPath);

	std::string inputNif = activeSet.GetInputFileName();

	UpdateProgress(30, _("Loading outfit shapes..."));
	if (ImportNIF(inputNif, true, sliderSetName)) {
		host->EndProgress();
		return 4;
	}

	if (origShapeOrder)
		*origShapeOrder = workNif.GetShapeNames();

	NiShape* newBaseShape = nullptr;

	if (activeSet.HasReferenceShapeName()) {
		std::string refShape = activeSet.GetReferenceShapeName();
		if (!refShape.empty())
			newBaseShape = workNif.FindBlockByName<NiShape>(refShape);
	}

	// First external target with skin shader becomes reference
	std::vector<std::string> refTargets;
	if (!newBaseShape) {
		activeSet.GetReferencedTargets(refTargets);
		for (auto& target : refTargets) {
			std::string shapeName = activeSet.TargetToShape(target);
			auto shape = workNif.FindBlockByName<NiShape>(shapeName);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}
	}

	// No external target found, first skin shaded shape becomes reference
	if (!newBaseShape && refTargets.empty()) {
		for (auto shapeTarget = activeSet.ShapesBegin(); shapeTarget != activeSet.ShapesEnd(); ++shapeTarget) {
			auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}
	}

	// Store base shape for later deletion
	baseShape = newBaseShape;

	UpdateProgress(90, _("Updating slider data..."));
	morpher.LoadResultDiffs(activeSet);

	wxString rest;
	mFileName = wxString::FromUTF8(fileName);
	if (mFileName.EndsWith(".xml", &rest))
		mFileName = rest.Append(".osp");

	mOutfitName = wxString::FromUTF8(sliderSetName);
	mDataDir = wxString::FromUTF8(activeSet.GetDefaultDataFolder());
	mBaseFile = wxString::FromUTF8(activeSet.GetInputFileName());
	mBaseFile = mBaseFile.AfterLast('/').AfterLast('\\');

	mGamePath = wxString::FromUTF8(activeSet.GetOutputPath());
	mGameFile = wxString::FromUTF8(activeSet.GetOutputFile());
	mCopyRef = true;
	mGenWeights = activeSet.GenWeights();
	bPreventMorphFile = activeSet.PreventMorphFile();
	bKeepZappedShapes = activeSet.KeepZappedShapes();
	mSFMorphPath = wxString::FromUTF8(activeSet.GetSFMorphPath());
	mSFMorphTargetShape = wxString::FromUTF8(activeSet.GetSFMorphTargetShape());

	// Preserve reference info from the loaded project so it gets saved again
	if (!activeSet.GetReferenceProjectFile().empty())
		mRefProjectFile = activeSet.GetReferenceProjectFile();
	if (!activeSet.GetReferenceProjectName().empty())
		mRefProjectName = activeSet.GetReferenceProjectName();
	if (!activeSet.GetReferenceShapeName().empty())
		mRefShapeName = activeSet.GetReferenceShapeName();

	if (mRefShapeName.empty() && baseShape) {
		mRefShapeName = baseShape->name.get();
	}

	// An add-on can't save changes to the shapes of its base project
	RecordAddOnGeometry(activeSet.IsAddOn());

	host->EndProgress();
	ReportResolveStatus();
	return 0;
}

int OutfitProject::ReloadReferenceShape() {
	NiShape* shape = GetBaseShape();
	if (!shape)
		return 0;

	std::string shapeName = shape->name.get();

	// The reference template the set names, which deleting and reloading the shape clears. It's saved again.
	const std::string refProjectFile = mRefProjectFile;
	const std::string refProjectName = mRefProjectName;

	// Prevent duplication if valid reference was found
	DeleteShape(shape);

	wxLogMessage("Loading reference shape '%s'...", shapeName);
	UpdateProgress(50, wxString::Format(_("Loading reference shape '%s'..."), shapeName));

	int error = LoadReferenceNif(activeSet.GetInputFileName(), shapeName, true, true);
	if (error)
		return error;

	if (!refProjectFile.empty() && GetBaseShape()) {
		mRefProjectFile = refProjectFile;
		mRefProjectName = refProjectName;
		SetShapeReferenceSource(shapeName, refProjectFile, refProjectName);
	}

	RecordAddOnGeometry(activeSet.IsAddOn());

	return 0;
}

int OutfitProject::CreateFromBase(const std::string& fileName, const std::string& sliderSetName, bool addOn, const std::string& projectName) {
	std::vector<std::string> shapeOrder;
	int error = LoadFromSliderSet(fileName, sliderSetName, &shapeOrder);
	if (!error)
		error = ReloadReferenceShape();
	if (error)
		return error;

	workNif.SetShapeOrder(shapeOrder);
	MakeDerivedProject(fileName, sliderSetName, addOn, projectName);
	return 0;
}

namespace {
// Project file path the way projects refer to other projects, relative to the project path if it can be
std::string ProjectRelativeFile(const std::string& fileName) {
	wxFileName projectFileName(wxString::FromUTF8(fileName));
	if (projectFileName.IsRelative())
		projectFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

	if (projectFileName.MakeRelativeTo(wxString::FromUTF8(ProjectUtil::GetProjectPath())))
		return projectFileName.GetFullPath().ToUTF8().data();

	return fileName;
}

bool NearlyEqual(const Vector3& a, const Vector3& b, float epsilon) {
	return (a - b).length2() <= epsilon * epsilon;
}

bool DiffsEqual(const TargetDataDiffs* a, const TargetDataDiffs* b) {
	constexpr float epsilon = 1e-5f;
	auto contains = [epsilon](const TargetDataDiffs& from, const TargetDataDiffs& in) {
		for (auto& diff : from) {
			auto other = in.find(diff.first);
			if (other == in.end()) {
				if (!NearlyEqual(diff.second, Vector3(), epsilon))
					return false;
			}
			else if (!NearlyEqual(diff.second, other->second, epsilon))
				return false;
		}
		return true;
	};

	TargetDataDiffs empty;
	const TargetDataDiffs& first = a ? *a : empty;
	const TargetDataDiffs& second = b ? *b : empty;
	return contains(first, second) && contains(second, first);
}
} // namespace

bool OutfitProject::BaseGeometryMatches(const std::string& fileName, const std::string& sliderSetName) {
	SliderSetFile file(fileName);
	SliderSet base;
	if (file.fail() || file.GetSet(sliderSetName, base) != 0)
		return false;

	base.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");

	std::fstream nifFile;
	PlatformUtil::OpenFileStream(nifFile, base.GetInputFileName(), std::ios::in | std::ios::binary);

	NifFile baseNif;
	if (baseNif.Load(nifFile) != 0)
		return false;

	auto shapes = workNif.GetShapes();
	if (shapes.size() != baseNif.GetShapes().size())
		return false;

	for (auto* shape : shapes) {
		auto baseNifShape = baseNif.FindBlockByName<NiShape>(shape->name.get());
		if (!baseNifShape)
			return false;

		std::vector<Vector3> verts, baseVerts;
		workNif.GetVertsForShape(shape, verts);
		baseNif.GetVertsForShape(baseNifShape, baseVerts);
		if (verts.size() != baseVerts.size())
			return false;

		for (size_t i = 0; i < verts.size(); i++)
			if (!NearlyEqual(verts[i], baseVerts[i], 1e-4f))
				return false;

		std::vector<Triangle> tris, baseTris;
		shape->GetTriangles(tris);
		baseNifShape->GetTriangles(baseTris);
		if (tris.size() != baseTris.size())
			return false;

		for (size_t i = 0; i < tris.size(); i++)
			if (tris[i].p1 != baseTris[i].p1 || tris[i].p2 != baseTris[i].p2 || tris[i].p3 != baseTris[i].p3)
				return false;
	}

	return true;
}

bool OutfitProject::SetBaseProject(const std::string& fileName, const std::string& sliderSetName, bool addOn, std::string* errorMessage) {
	SliderSetFile file(fileName);
	SliderSet base;
	if (file.fail() || file.GetSet(sliderSetName, base) != 0) {
		if (errorMessage)
			*errorMessage = "The base project wasn't found.";
		return false;
	}

	if (wxString::FromUTF8(sliderSetName) == mOutfitName && wxFileName(wxString::FromUTF8(fileName)).SameAs(mFileName)) {
		if (errorMessage)
			*errorMessage = "A project can't be its own base project.";
		return false;
	}

	base.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
	DiffDataSets baseDiffs;
	base.LoadSetDiffData(baseDiffs);

	auto dataFileOf = [](const DiffInfo& dataFile) {
		std::string osdFileName;
		std::string dataNameInFile;
		if (SplitSliderDataFileName(dataFile.fileName, osdFileName, dataNameInFile))
			return std::make_pair(ToLower(osdFileName), dataNameInFile);
		return std::make_pair(ToLower(dataFile.fileName), std::string());
	};

	// What equals the base project becomes inherited, the rest stays the project's own
	std::vector<std::string> removedSliders;
	for (size_t b = 0; b < base.size(); b++) {
		const SliderData& baseSlider = base[b];
		if (!activeSet.SliderExists(baseSlider.name)) {
			removedSliders.push_back(baseSlider.name);
			continue;
		}

		SliderData& slider = activeSet[baseSlider.name];
		slider.bInherited = slider.bHidden == baseSlider.bHidden && slider.bInvert == baseSlider.bInvert && slider.bZap == baseSlider.bZap
							&& slider.bClamp == baseSlider.bClamp && slider.bUV == baseSlider.bUV && slider.defSmallValue == baseSlider.defSmallValue
							&& slider.defBigValue == baseSlider.defBigValue && slider.zapToggles == baseSlider.zapToggles;

		for (auto& dataFile : slider.dataFiles) {
			auto baseDataFile = std::find_if(baseSlider.dataFiles.begin(), baseSlider.dataFiles.end(), [&](const DiffInfo& df) { return df.targetName == dataFile.targetName; });
			if (baseDataFile == baseSlider.dataFiles.end())
				continue;

			// Data that already is the base project's needs no comparing
			bool sameData = dataFileOf(dataFile) == dataFileOf(*baseDataFile);
			if (!sameData) {
				std::string shapeName = activeSet.TargetToShape(dataFile.targetName);
				NiShape* shape = workNif.FindBlockByName<NiShape>(shapeName);
				TargetDataDiffs* ownDiff = shape && IsBaseShape(shape) ? baseDiffData.GetDiffSet(dataFile.dataName) : morpher.GetDiffSet(dataFile.dataName);
				sameData = DiffsEqual(ownDiff, baseDiffs.GetDiffSet(baseDataFile->dataName));
			}

			if (sameData)
				dataFile.bInherited = true;
		}
	}

	std::map<std::pair<std::string, std::string>, std::string> inheritedDataNames;
	for (size_t i = 0; i < activeSet.size(); i++)
		for (auto& dataFile : activeSet[i].dataFiles)
			if (dataFile.bInherited)
				inheritedDataNames[std::make_pair(activeSet[i].name, dataFile.targetName)] = dataFile.dataName;

	// Other base projects are other projects of the same body, which another base project doesn't have
	const bool sameBase = StringsEqualInsens(activeSet.GetBaseProjectName().c_str(), sliderSetName.c_str()) && activeSet.GetBaseProjectID() == base.GetProjectID();
	std::vector<BaseProjectRef> otherBases = sameBase ? activeSet.GetOtherBaseProjects() : std::vector<BaseProjectRef>();

	activeSet.SetBaseProject(ProjectRelativeFile(fileName), sliderSetName, base.GetProjectID());
	activeSet.SetOtherBaseProjects(otherBases);
	activeSet.SetRemovedSliders(removedSliders);

	// The project's own data goes into an .osd file of the add-on's own
	if (addOn) {
		std::string name = mOutfitName.empty() ? activeSet.GetName() : std::string(mOutfitName.ToUTF8());
		ReplaceForbidden(name);

		activeSet.SetAddOn(true);
		activeSet.SetSliderDataFile(name + ".osd");
		mBaseFile = wxString::FromUTF8(base.GetInputFile());
		mDataDir = wxString::FromUTF8(base.GetDefaultDataFolder());
		activeSet.SetDataFolder(base.GetDefaultDataFolder());
		activeSet.SetInputFile(base.GetInputFile());

		for (size_t i = 0; i < activeSet.size(); i++)
			for (auto& dataFile : activeSet[i].dataFiles)
				if (!dataFile.bInherited)
					UnlinkSliderData(dataFile);
	}

	// Merging the base project in the way loading does brings its folders and status along
	activeSet.InheritFrom(base);

	// Inherited data has the base project's data names, which the loaded data takes over
	for (size_t i = 0; i < activeSet.size(); i++) {
		for (auto& dataFile : activeSet[i].dataFiles) {
			auto oldName = inheritedDataNames.find(std::make_pair(activeSet[i].name, dataFile.targetName));
			if (!dataFile.bInherited || oldName == inheritedDataNames.end() || oldName->second == dataFile.dataName)
				continue;

			NiShape* shape = workNif.FindBlockByName<NiShape>(activeSet.TargetToShape(dataFile.targetName));
			if (shape && IsBaseShape(shape))
				baseDiffData.RenameSet(oldName->second, dataFile.dataName);
			else {
				morpher.RenameSet(oldName->second, dataFile.dataName);
				morpher.SetResultDataName(dataFile.targetName, activeSet[i].name, dataFile.dataName);
			}
		}
	}

	RecordAddOnGeometry(addOn);
	return true;
}

bool OutfitProject::SetOtherBaseProjects(const std::vector<std::pair<std::string, std::string>>& fileAndSetNames, std::string* errorMessage) {
	auto fail = [errorMessage](const std::string& message) {
		if (errorMessage)
			*errorMessage = message;
		return false;
	};

	if (!activeSet.IsAddOn())
		return fail("Only add-ons can have other base projects.");

	std::vector<BaseProjectRef> otherBases;
	for (auto& fileAndSet : fileAndSetNames) {
		wxFileName baseFileName(wxString::FromUTF8(fileAndSet.first));
		if (baseFileName.IsRelative())
			baseFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

		SliderSetFile file(baseFileName.GetFullPath().ToUTF8().data());
		if (file.fail() || !file.HasSet(fileAndSet.second))
			return fail("The base project '" + fileAndSet.second + "' wasn't found in '" + fileAndSet.first + "'.");

		if (wxString::FromUTF8(fileAndSet.second) == mOutfitName && baseFileName.SameAs(mFileName))
			return fail("A project can't be its own base project.");

		otherBases.push_back({ProjectRelativeFile(baseFileName.GetFullPath().ToUTF8().data()), fileAndSet.second, file.GetSetProjectID(fileAndSet.second)});
	}

	activeSet.SetOtherBaseProjects(otherBases);
	return true;
}

void OutfitProject::SetAddOnDataFile(const std::string& fileName) {
	if (!activeSet.IsAddOn() || fileName == activeSet.GetSliderDataFile())
		return;

	// The add-on's own data moves to the new file when saving
	activeSet.SetSliderDataFile(fileName);
	for (size_t i = 0; i < activeSet.size(); i++)
		for (auto& dataFile : activeSet[i].dataFiles)
			if (!dataFile.bInherited && dataFile.bLocal)
				UnlinkSliderData(dataFile);
}

void OutfitProject::ClearBaseProject() {
	if (!activeSet.HasBaseProject())
		return;

	// Without a base project, an add-on needs a NIF file of its own, named like its .osd file
	if (activeSet.IsAddOn()) {
		std::string dataFile = activeSet.GetSliderDataFile();
		std::string name = dataFile.empty() ? std::string(mOutfitName.ToUTF8()) : dataFile.substr(0, dataFile.find_last_of('.'));
		ReplaceForbidden(name);
		mBaseFile = wxString::FromUTF8(name + ".nif");
		activeSet.SetAddOn(false);
		activeSet.SetSliderDataFile("");
	}

	for (size_t i = 0; i < activeSet.size(); i++)
		for (auto& dataFile : activeSet[i].dataFiles)
			DetachSliderData(dataFile);

	activeSet.ClearBaseProject();
	activeSet.SetResolveStatus(SliderSetResolveStatus());
	RecordAddOnGeometry(false);
}

void OutfitProject::MakeDerivedProject(const std::string& fileName, const std::string& sliderSetName, bool addOn, const std::string& projectName) {
	// The base project is referred to like a reference project
	activeSet.MakeDerivedFrom(ProjectRelativeFile(fileName), sliderSetName, addOn);

	std::string name = projectName;
	ReplaceForbidden(name);
	activeSet.SetName(name);
	outfitName = name;
	mOutfitName = wxString::FromUTF8(name);
	mFileName.clear();

	// A variant gets a NIF file of its own, an add-on keeps the base project's
	if (addOn)
		activeSet.SetSliderDataFile(name + ".osd");
	else
		mBaseFile.clear();

	RecordAddOnGeometry(addOn);
}

int OutfitProject::AddFromSliderSet(const std::string& fileName,
									const std::string& sliderSetName,
									const bool newDataLocal,
									const bool appendNewSliders,
									const bool setAsReference) {
	host->StartProgress(_("Adding slider set..."));
	const bool hadBaseShape = baseShape != nullptr;
	SliderSetFile InSS(fileName);
	if (InSS.fail()) {
		host->EndProgress();
		return 1;
	}

	SliderSet addSet;
	UpdateProgress(20, _("Retrieving sliders..."));
	if (InSS.GetSet(sliderSetName, addSet)) {
		host->EndProgress();
		return 2;
	}

	if (!addSet.GetReferenceProjectFile().empty())
		mRefProjectFile = addSet.GetReferenceProjectFile();
	if (!addSet.GetReferenceProjectName().empty())
		mRefProjectName = addSet.GetReferenceProjectName();
	if (!addSet.GetReferenceShapeName().empty())
		mRefShapeName = addSet.GetReferenceShapeName();

	addSet.SetBaseDataPath(ProjectUtil::GetProjectPath() + PathSepStr + "ShapeData");
	std::string inputNif = addSet.GetInputFileName();

	std::map<std::string, std::string> renamedShapes;
	UpdateProgress(30, _("Adding outfit shapes..."));
	if (ImportNIF(inputNif, false, "", &renamedShapes)) {
		host->EndProgress();
		return 3;
	}

	// Shapes renamed on import take their slider data along under the new name.
	// It would otherwise be merged into the data of the shapes they clashed with.
	std::set<std::string> renamedTargets;
	if (!renamedShapes.empty()) {
		std::vector<std::string> renamedShapesInfo;
		renamedShapesInfo.reserve(renamedShapes.size());
		for (auto& rs : renamedShapes) {
			renamedShapesInfo.push_back(rs.first + " -> " + rs.second);

			if (addSet.HasShape(rs.first)) {
				addSet.RenameShape(rs.first, rs.second);
				renamedTargets.insert(addSet.ShapeToTarget(rs.second));
			}
		}

		std::string shapesJoin = JoinStrings(renamedShapesInfo, "; ");
		host->ShowMessage(HostMessageLevel::Warning,
						  wxString::Format("%s\n \n%s", _("The following shapes were renamed because the project already has shapes with the same names."), shapesJoin),
						  _("Renamed Shapes"));
	}

	if (setAsReference && !baseShape) {
		NiShape* newBaseShape = nullptr;

		// First external target with skin shader becomes reference
		std::vector<std::string> refTargets;
		addSet.GetReferencedTargets(refTargets);
		for (auto& target : refTargets) {
			std::string shapeName = addSet.TargetToShape(target);
			auto shape = workNif.FindBlockByName<NiShape>(shapeName);
			if (shape) {
				NiShader* shader = workNif.GetShader(shape);
				if (shader && shader->IsSkinTinted()) {
					newBaseShape = shape;
					break;
				}
			}
		}

		// No external target found, first skin shaded shape becomes reference
		if (refTargets.empty()) {
			for (auto shapeTarget = addSet.ShapesBegin(); shapeTarget != addSet.ShapesEnd(); ++shapeTarget) {
				auto shape = workNif.FindBlockByName<NiShape>(shapeTarget->first);
				if (shape) {
					NiShader* shader = workNif.GetShader(shape);
					if (shader && shader->IsSkinTinted()) {
						newBaseShape = shape;
						break;
					}
				}
			}
		}

		baseShape = newBaseShape;
	}

	std::vector<std::pair<std::string, std::string>> incomingShapeTargets;
	for (auto shapeTarget = addSet.ShapesBegin(); shapeTarget != addSet.ShapesEnd(); ++shapeTarget) {
		if (workNif.FindBlockByName<NiShape>(shapeTarget->first))
			incomingShapeTargets.emplace_back(shapeTarget->first, shapeTarget->second.targetShape);
	}
	ResolveTargetConflictsForIncomingShapes(incomingShapeTargets);

	UpdateProgress(70, _("Updating slider data..."));
	morpher.MergeResultDiffs(activeSet, addSet, baseDiffData, baseShape ? baseShape->name.get() : "", newDataLocal, appendNewSliders);

	// The renamed data doesn't exist under its new name in the added project's files, so it becomes the project's own
	for (size_t i = 0; i < activeSet.size(); i++) {
		for (auto& dataFile : activeSet[i].dataFiles) {
			if (renamedTargets.find(dataFile.targetName) == renamedTargets.end())
				continue;

			dataFile.fileName = dataFile.dataName;
			dataFile.bLocal = true;
			dataFile.bInherited = false;
		}
	}

	// Set reference info if not already present and a new base shape was found
	if (baseShape && !hadBaseShape && mRefProjectFile.empty() && mRefProjectName.empty() && mRefShapeName.empty()) {
		wxFileName refFileName(wxString::FromUTF8(fileName));
		if (refFileName.IsRelative())
			refFileName.MakeAbsolute(wxString::FromUTF8(ProjectUtil::GetProjectPath()));

		if (refFileName.MakeRelativeTo(wxString::FromUTF8(ProjectUtil::GetProjectPath())))
			mRefProjectFile = refFileName.GetFullPath().ToUTF8().data();
		else
			mRefProjectFile = fileName;

		mRefProjectName = sliderSetName;
		if (baseShape)
			mRefShapeName = baseShape->name.get();
	}

	host->EndProgress();
	return 0;
}
