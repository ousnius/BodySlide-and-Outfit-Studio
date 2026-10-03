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

#pragma once

#include "../components/Anim.h"
#include "../components/BuildSelection.h"
#include "../components/ClippingFixer.h"
#include "../components/PoseData.h"
#include "../components/SliderCategories.h"
#include "../components/SliderData.h"
#include "../components/SliderGroup.h"
#include "../components/SliderManager.h"
#include "../files/TriFile.h"
#include "../physics/Controller.h"
#include "../physics/PumpClock.h"
#include "../utils/ConfigurationManager.h"
#include "../utils/Log.h"
#include "GroupManager.h"
#include "PresetSaveDialog.h"
#include "PreviewWindow.h"
#include "../ui/PreviewPanel.h"
#include "../ui/wxStateButton.h"

#include "FSEngine/FSEngine.h"
#include "FSEngine/FSManager.h"

#include <wx/clrpicker.h>
#include <wx/cmdline.h>
#include <wx/collpane.h>
#include <wx/dcbuffer.h>
#include <wx/dir.h>
#include <wx/html/htmlwin.h>
#include <wx/imagpng.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/treelist.h>
#include <wx/progdlg.h>
#include <wx/splitter.h>
#include <wx/srchctrl.h>
#include <wx/statline.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <wx/wxprec.h>
#include <wx/xrc/xmlres.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>


enum TargetGame { FO3, FONV, SKYRIM, FO4, SKYRIMSE, FO4VR, SKYRIMVR, FO76, OB, SF };

// Marks favorites in the outfit, preset and animation lists and on their buttons
constexpr const char* FavoriteStar = "\xE2\x98\x85";
constexpr const char* FavoriteStarIcon = "/res/images/FavoriteStar.png";
constexpr const char* FavoriteStarEmptyIcon = "/res/images/FavoriteStarEmpty.png";

struct ShapePreviewData {
	std::string name;
	std::vector<nifly::Vector3> verts;
	std::vector<nifly::Vector2> uvs;
	std::vector<uint16_t> zapIdx;
	int projectIdx = 0;
};

class BodySlideFrame;

class BodySlideApp : public wxApp {
	/* UI Managers */
	BodySlideFrame* sliderView = nullptr;
	PreviewPanel* preview = nullptr;
	PreviewWindow* previewWindow = nullptr;
	bool dockPoppedOutOnClose = true;

	/* Command-Line Arguments */
	std::vector<std::string> cmdGroupBuild;
	std::vector<std::string> cmdBuildOutfits; // Outfits to build, given by name.
	std::string cmdBuildFilter;				  // Outfits to build, given as an outfit filter expression.
	bool cmdBuildFilterRegex = false;		  // The filter is a regular expression instead of a substring.
	std::string cmdTargetDir;
	std::string cmdPreset;
	std::string cmdPresetFile;
	bool cmdPresetResolved = false;
	bool cmdPresetPending = false;
	bool cmdTri = false;
	std::vector<std::string> cmdPreviewNifs;
	bool cmdPreviewMode = false;

public:
	/* Clipping Fix */
	float clippingFixStrength = 0.0f; // 0-100 scale, 0 disables clipping fix
private:

	/* Reference shape loaded from external project for clipping fix */
	std::unique_ptr<nifly::NifFile> referenceNif;
	SliderSet referenceSliderSet;
	DiffDataSets referenceDiffData;
	std::string referenceShapeName;

	/* Localization */
	wxLocale* locale = nullptr;
	int language = 0;

	/* Data Managers */
	SliderManager sliderManager;
	Log logger;

	/* Data Items */
	std::map<std::string, std::string, case_insensitive_compare> outfitNameSource; // All currently defined outfits.
	std::vector<std::string> outfitNameOrder;									   // All currently defined outfits, in their order of appearance.
	std::vector<std::string> outfitHasZaps;										   // All currently defined outfits that have visible zaps.
	std::map<std::string, std::vector<std::string>> groupMembers;				   // All currently defined groups.
	std::map<std::string, std::string> groupAlias;								   // Group name aliases.
	std::vector<std::string> ungroupedOutfits;									   // Outfits without a group.
	std::vector<std::string> filteredOutfits;									   // Filtered outfit names.
	std::vector<std::string> favoriteOutfits;
	std::vector<std::string> favoritePresets;
	std::unordered_set<std::string> favoriteOutfitSet;
	std::unordered_set<std::string> favoritePresetSet;
	std::vector<std::string> presetGroups;
	std::vector<std::string> allGroups;
	SliderSetGroupCollection gCollection;

	/* Cache */
	std::map<std::string, nifly::NifFile, case_insensitive_compare> refNormalsCache; // Cache for reference normals files

	struct ProjectData {
		SliderSet sliderSet;
		DiffDataSets dataSets;
		nifly::NifFile* baseNif = nullptr;
		nifly::NifFile modNif;
		std::string setName;
		std::string inputFileName;

		~ProjectData() { delete baseNif; }
		ProjectData() = default;
		ProjectData(const ProjectData&) = delete;
		ProjectData& operator=(const ProjectData&) = delete;
	};
	std::vector<std::unique_ptr<ProjectData>> projects;
	bool multiProjectMode = false;

	/* Preview skinning, shared by poses, animations and physics */
	// Skinning of each previewed project's NIF (by project index) and of the
	// external reference NIF. Held by pointer because the physics bones keep
	// pointers into them for as long as they exist. Built on demand and dropped
	// whenever the meshes they describe are replaced.
	std::vector<std::unique_ptr<AnimInfo>> previewAnims;
	std::unique_ptr<AnimInfo> previewReferenceAnim;
	bool previewAnimsBuilt = false;
	// NIFs previewed as they are, without a project (--preview with NIF files,
	// or the extra NIFs next to projects). Kept so they can be skinned, with
	// skinning of their own: they are replaced independently of the projects,
	// whose skinning the simulation may be holding on to.
	std::vector<std::unique_ptr<nifly::NifFile>> previewLooseNifs;
	std::vector<std::unique_ptr<AnimInfo>> previewLooseAnims;
	bool previewLooseAnimsBuilt = false;
	// The reference skeleton the application skeleton was loaded from
	std::string previewSkeletonPath;
	// Morphed (but not yet skinned) preview vertices per shape, so a physics or
	// animation tick can re-skin without running every slider again.
	std::unordered_map<std::string, std::vector<nifly::Vector3>> previewMorphedVerts;
	std::vector<nifly::Vector3> previewReferenceMorphedVerts;
	// Paces physics and animation ticks alike
	Physics::PumpClock previewClock;

	/* Physics preview (HDT-SMP), one simulation per previewed project */
	struct PreviewPhysicsProject {
		std::unique_ptr<Physics::Controller> controller;
		size_t projectIdx = 0;
	};
	std::vector<PreviewPhysicsProject> previewPhysics;
	// At least one previewed project references a physics XML, so the preview
	// shows its physics controls.
	bool previewPhysicsAvailable = false;
	bool previewPhysicsRunning = false;

	/* Pose preview */
	// Poses from the PoseData folder and the game's SAM/SAF folder, plus the
	// HKX animations loaded this session. Loaded once, on first use.
	PoseDataCollection previewPoses;
	bool previewPosesLoaded = false;
	// PreviewPoseNone, PreviewPoseRest or PreviewPoseFirst + index into
	// previewPoses.poseData
	int previewPoseIndex = 0;
	// Index into previewPoses.animationData, -1 for none. An animation takes
	// precedence over the pose.
	int previewAnimIndex = -1;
	bool previewAnimPlaying = false;
	bool previewAnimInterpolate = true;
	double previewAnimFrame = 0.0;
	double previewAnimSpeed = 1.0;
	PoseData previewAnimBlendPose;

	// Loads the reference skeleton of the current game into the application
	// skeleton if it isn't already.
	bool EnsurePreviewSkeleton();
	// Loads the poses and lists the favorite animations if it isn't done yet
	void EnsurePreviewPosesLoaded();
	// The game's section in the animation favorites
	std::string GetAnimationFavoritesGame() const;
	// Builds the skinning of all previewed NIFs if it isn't already.
	bool EnsurePreviewSkinning();
	// Tears down the simulation and drops the skinning, without touching the
	// displayed meshes. For when the meshes are about to be replaced.
	void ReleasePreviewSkinning();
	// A pose or animation is selected, so every skinned shape follows the
	// skeleton rather than only the simulated ones
	bool IsPreviewPosed() const;
	// Whether the previewed meshes have to be skinned at all
	bool IsPreviewSkinned() const;
	AnimationData* GetPreviewAnimationData();
	// Puts the application skeleton into the selected pose or animation frame
	void ApplyPreviewSkeletonPose();
	// Moves the skeleton to a different pose or frame in one go and shows it
	void JumpPreviewSkeleton();
	// Skins the shape's morphed vertices with the current skeleton pose, or the
	// simulated bone transforms if a simulation drives the shape.
	void ApplyPreviewSkinning(size_t projectIdx, const std::string& shapeName, std::vector<nifly::Vector3>& verts);
	void ApplyPreviewReferenceSkinning(std::vector<nifly::Vector3>& verts);
	// Shows the loose NIFs posed, or as stored without a pose. Doesn't render.
	void UpdatePreviewLooseMeshes();
	// Skins all cached morphed shapes again, or only those a simulation drives
	void ReskinPreview(bool allShapes);
	// Puts the plain morphed shapes back once nothing needs skinning anymore
	void RestoreUnskinnedPreview();
	// Shows the previewed meshes skinned by whatever currently drives them
	void RefreshPreviewSkinning();
	// Ties the preview's tick timer to whether anything is moving
	void UpdatePreviewPump();

	int CreateSetSliders(const std::string& outfit);
	std::string GetFavoriteConfigKey(const std::string& listName) const;
	std::string SerializeFavoriteNames(const std::vector<std::string>& names) const;
	std::vector<std::string> DeserializeFavoriteNames(const std::string& value) const;
	void SetFavoriteList(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::vector<std::string>& names);
	bool RemoveFavoriteName(std::vector<std::string>& list, std::unordered_set<std::string>& set, const std::string& name);
	void SortFavoritesFirst(std::vector<std::string>& names, const std::unordered_set<std::string>& favorites) const;

public:
	virtual ~BodySlideApp();
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);

	virtual bool OnExceptionInMainLoop();
	virtual void OnUnhandledException();
	virtual void OnFatalException();

	SliderCategoryCollection cCollection;
	TargetGame targetGame = TargetGame::FO3;
	std::map<std::string, std::vector<std::string>, case_insensitive_compare> outFileCount; // Counts how many sets write to the same output file

	bool SetDefaultConfig();
	bool ShowSetup();

	// Pushes the Complex Material render setting to the preview, if one is open.
	void ApplyComplexMaterialSetting();

	// Pushes the True PBR render setting to the preview, if one is open, rebuilding its meshes when
	// the setting actually changed - unlike Complex Material this one selects the shader files.
	void ApplyPBRSetting();

	std::string GetOutputDataPath() const;
	SliderSet& GetActiveSet() { return projects[0]->sliderSet; }
	DiffDataSets& GetActiveDataSets() { return projects[0]->dataSets; }
	bool HasActiveProject() const { return !projects.empty(); }

	int AddProjectSliders(const std::string& projectFile, const std::string& setName);

	void InitLanguage();

	void LoadData();
	void CharHook(wxKeyEvent& event);

	bool OutfitExists(const std::string& name) const { return outfitNameSource.find(name) != outfitNameSource.end(); }
	bool PresetExists(const std::string& name);

	void LoadFavorites();
	void SaveFavorites();
	bool IsFavoriteOutfit(const std::string& name) const;
	bool IsFavoritePreset(const std::string& name) const;
	void SortOutfitNamesForDisplay(std::vector<std::string>& names) const;
	void SortPresetNamesForDisplay(std::vector<std::string>& names) const;
	void ToggleFavoriteOutfit(const std::string& name);
	void ToggleFavoritePreset(const std::string& name);
	void RemoveFavoriteOutfit(const std::string& name);
	void RemoveFavoritePreset(const std::string& name);

	void LoadAllCategories();

	void SetPresetGroups(const std::string& setName);
	void LoadAllGroups();
	void GetAllGroupNames(std::vector<std::string>& outGroups);
	int SaveGroupList(const std::string& filename, const std::string& groupname);

	void PopulateFilterData();
	void ApplyOutfitFilter();
	// Matches outfit names the same way the outfit filter text box does, either as a
	// case insensitive substring or, with useRegex, as a case insensitive regular expression.
	// An invalid regular expression matches nothing and is reported through regexError.
	std::vector<std::string> FilterOutfitNames(const std::vector<std::string>& names, const std::string& filter, bool useRegex, std::string* regexError = nullptr) const;
	std::vector<std::string> ApplyPresetFilter(const std::vector<std::string>& presetNames);
	int GetOutfits(std::vector<std::string>& outList);
	int GetFilteredOutfits(std::vector<std::string>& outList);

	void LoadPresets(const std::string& sliderSet);
	void LoadCmdPresetFile();
	void GetPresetNames(std::vector<std::string>& outNames);
	std::string GetPresetFileName(const std::string& presetName);
	void GetPresetGroups(const std::string& presetName, std::vector<std::string>& outGroups);
	void InitializeSliders(const std::string& presetName = "");
	void RefreshPresetsForCurrentOutfit();
	void PopulatePresetList(const std::string& select);
	void PopulateOutfitList(const std::string& select);
	void DisplayActiveSet();

	void GetBuildSelection(BuildSelectionFile& file, BuildSelection& buildSel);

	void UpdateConflictManager();
	void SetDefaultBuildSelection();

	bool UpdateZapChoices();
	void SetZapChoice(const std::string& zap, bool choice);

	int LoadSliderSets();
	void RefreshOutfitList();

	void RefreshSliders();

	void ActivateOutfit(const std::string& outfitName);
	void ActivatePreset(const std::string& presetName, const bool updatePreview = true);

	std::vector<std::string> GetConflictingOutfits() {
		if (projects.empty())
			return {};
		return outFileCount.find(GetActiveSet().GetOutputFilePath())->second;
	}

	void DeleteOutfit(const std::string& outfitName);
	void DeletePreset(const std::string& presetName);

	void EditProject(const std::string& projectName);
	void LaunchOutfitStudio(const wxString& args = "");

	void ApplySliders(const std::string& targetShape,
					  std::vector<Slider>& sliderSet,
					  DiffDataSets& dataSets,
					  std::vector<nifly::Vector3>& verts,
					  std::vector<uint16_t>& zapidx,
					  std::vector<nifly::Vector2>* uvs = nullptr);
	bool WriteMorphTRI(const std::string& triPath, SliderSet& sliderSet, nifly::NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices);
	bool WriteSFMorphFile(const std::string& morphFolder, SliderSet& sliderSet, nifly::NifFile& nif, std::unordered_map<std::string, std::vector<uint16_t>>& zapIndices);

	void CopySliderValues(bool toHigh);
	void CopyPreviewWeightToSliders();
	void ShowPreview();
	void InitPreviewPanel();
	void BuildPreviewMesh(ProjectData* pp, bool freshLoad);
	void InitPreview();
	bool IsPreviewRenderable() const;
	void CleanupPreview();
	void LoadPreviewNifs(const std::vector<std::string>& nifFilePaths);
	void ClosePreview();
	void PreviewClosed();
	void PopOutPreview();
	void DockPreview(bool attachToMain = true, bool preservePoppedOutState = false);
	bool IsPreviewPoppedOut() const { return previewWindow != nullptr; }
	void SetDockPoppedOutOnClose(bool dock) { dockPoppedOutOnClose = dock; }
	bool ShouldDockPoppedOutOnClose() const { return dockPoppedOutOnClose; }

	/* Async preview loading */
	std::atomic<uint64_t> previewLoadGeneration{0};
	std::thread previewLoadThread;
	bool previewLoading = false;

	void ApplyClippingFix(nifly::NifFile& nif,
						const std::vector<nifly::Vector3>& bodyVerts,
						const std::vector<nifly::Triangle>& bodyTris,
						std::unordered_map<std::string, std::vector<nifly::Vector3>*>& shapeVerts);
	bool LoadExternalReference(const SliderSet& sliderSet);
	void UpdateReferenceCheckboxState();
	void UpdatePreview();
	void RebuildPreviewMeshes();

	/* Physics preview */
	// Rescans the previewed meshes for physics XML links and shows or hides the
	// preview's physics controls accordingly. Always tears the simulation down
	// first, because whatever led here replaced the meshes it was built on; only
	// a rebuild of a preview that was simulating ("restart") starts it again,
	// so a newly opened preview always begins with physics off.
	void UpdatePreviewPhysicsAvailability(bool restart = false);
	bool IsPreviewPhysicsAvailable() const { return previewPhysicsAvailable; }
	// Builds or tears down the simulation for all previewed projects.
	void EnablePreviewPhysics(bool enable);
	bool IsPreviewPhysicsRunning() const { return previewPhysicsRunning; }
	// One animation and simulation tick plus the resulting re-skin and redraw.
	// Internally paced, so it is safe to call from any event source at any rate.
	void PumpPreview();
	// Physics or an animation is moving the preview, so it needs ticks
	bool IsPreviewPumping() const { return previewPhysicsRunning || previewAnimPlaying; }

	/* Pose preview */
	static constexpr int PreviewPoseNone = 0;
	static constexpr int PreviewPoseRest = 1;
	static constexpr int PreviewPoseFirst = 2;
	// Names of the loadable poses, in the order PreviewPoseFirst counts from
	std::vector<std::string> GetPreviewPoseNames();
	int GetPreviewPose() const { return previewPoseIndex; }
	void SetPreviewPose(int poseIndex);
	// Forgets the loaded poses, animations and skeleton, for a change of the
	// target game or reference skeleton
	void ResetPreviewPoses();
	// Takes over the NIFs the preview loaded without a project, replacing the
	// previous ones, and shows them in the current pose
	void SetPreviewLooseNifs(std::vector<std::unique_ptr<nifly::NifFile>> nifs);

	// HKX animations exist for Skyrim and Fallout 4 only
	bool CanLoadPreviewAnimations() const;
	// Loads an HKX animation and selects it. Returns false with a message in
	// errorOut on failure.
	bool LoadPreviewAnimation(const std::string& filePath, std::string& errorOut);
	// Loaded animations and favorites, in the order of their indices
	std::vector<std::string> GetPreviewAnimationNames();
	int GetPreviewAnimation() const { return previewAnimIndex; }
	// Selects an animation (-1 for none), reading it first if it's a favorite
	// that wasn't read yet. On failure the selection stays as it was.
	bool SetPreviewAnimation(int animIndex, std::string* errorOut = nullptr);
	// Favorites are shared with Outfit Studio
	bool IsPreviewAnimationFavorite(int animIndex) const;
	void SetPreviewAnimationFavorite(int animIndex, bool favorite);
	bool IsPreviewAnimationPlaying() const { return previewAnimPlaying; }
	void SetPreviewAnimationPlaying(bool playing);
	size_t GetPreviewAnimationFrameCount();
	int GetPreviewAnimationFrame() const { return static_cast<int>(previewAnimFrame); }
	void SeekPreviewAnimation(int frame);
	void SetPreviewAnimationSpeed(double speed) { previewAnimSpeed = speed; }
	bool IsPreviewAnimationInterpolated() const { return previewAnimInterpolate; }
	void SetPreviewAnimationInterpolate(bool interpolate);
	// Feeds a horizontal camera rotation into the simulation so cloth and hair
	// react as if the character turned under a fixed camera.
	void InjectPreviewCameraYaw(float deltaDegrees);
	// Wind direction as an index into Physics::WindDirectionNames(), strength in
	// percent.
	void SetPreviewWind(int directionIndex, int strengthPercent);
	std::vector<ShapePreviewData> ComputeMorphedShapeData(int weight);
	void PostProcessPreview(std::vector<ShapePreviewData>& shapeData, int weight);
	void UpdateExternalReferenceMesh(int weight, std::vector<nifly::Vector3>* outVerts = nullptr);
	void UpdateMeshesFromSet(SliderSet& set);
	void ApplyReferenceNormals(nifly::NifFile& nif);

	int BuildBodies(bool localPath = false, bool clean = false, bool tri = false, bool forceNormals = false);
	int BuildListBodies(std::vector<std::string>& outfitList,
						std::map<std::string, std::string>& failedOutfits,
						bool remove = false,
						bool tri = false,
						bool forceNormals = false,
						const std::string& custPath = "");
	int ShowBuildOverrideWithPreview(wxDialog* dlg, wxTreeListCtrl* treeListCtrl);

	// True if any of the command-line options that select outfits for a build was given.
	bool HasCmdLineBuild() const { return !cmdGroupBuild.empty() || !cmdBuildOutfits.empty() || !cmdBuildFilter.empty(); }
	// Resolves the outfits selected by the command line, in the order they were loaded in.
	// Outfit names that don't exist and filters that match nothing are reported through failedOutfits.
	std::vector<std::string> GetCmdLineBuildOutfits(std::map<std::string, std::string>& failedOutfits);
	// Builds the outfits selected by the command line and closes the application.
	void CommandLineBuild();

	// Removes all BODYTRI extra data from the file and attaches a single fresh one
	// to the root node (toRoot) or else to the first shape with vertices.
	void SetTriData(nifly::NifFile& nif, const std::string& triPath, bool toRoot = false);

	// Lists every vertex in a LOCKEDNORM extra data block on each shape with locked normals,
	// so RaceMenu doesn't recalculate those normals after applying in-game morphs.
	void SetLockedNormalsData(nifly::NifFile& nif, SliderSet& sliderSet);

	float GetSliderValue(const wxString& sliderName, bool isLo);
	bool IsUVSlider(const wxString& sliderName);
	std::vector<std::string> GetSliderZapToggles(const wxString& sliderName);
	void SetSliderValue(const wxString& sliderName, bool isLo, float val);
	void SetSliderChanged(const wxString& sliderName, bool isLo);

	int UpdateSliderPositions(const std::string& presetName);
	int SaveSliderPositions(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups);
	int SavePresetGroups(const std::string& outputFile, const std::string& presetName, std::vector<std::string>& groups);
};

static const wxCmdLineEntryDesc g_cmdLineDesc[] = {{wxCMD_LINE_OPTION, "gbuild", "groupbuild", "builds the specified group on launch", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"b",
													"build",
													"builds the specified outfits on launch, a single outfit name or a list of them separated by ',', ';' or '|'",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"f",
													"filter",
													"builds all outfits matching the specified filter on launch, works like the outfit filter box",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_SWITCH, "regex", "regexfilter", "treats the value of the filter option as a regular expression"},
												   {wxCMD_LINE_OPTION, "t", "targetdir", "build target directory, defaults to game data path", wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_OPTION,
													"p",
													"preset",
													"preset to load on launch, use for the build or apply in preview mode, either a preset name or the "
													"path to a preset XML file optionally followed by '?' and a preset name, defaults to last used preset",
													wxCMD_LINE_VAL_STRING},
												   {wxCMD_LINE_SWITCH, "tri", "trimorphs", "enables tri morph output for the specified build"},
												   {wxCMD_LINE_OPTION, "preview", "preview", "open the specified nif files in preview mode", wxCMD_LINE_VAL_STRING},
												   wxCMD_LINE_DESC_END};

#define DELAYLOAD_TIMER 299
#define SLIDER_LO 1
#define SLIDER_HI 2

class SliderCategoryUI {
	bool isCreated = false;

public:
	bool isShown = false;
	bool isEnabled = false;
	bool oneSize = false;

	std::string categoryName;
	std::vector<std::string> sliderNames;

	wxPanel* dummyPanel1 = nullptr;
	wxCheckBox* check = nullptr;
	wxStaticText* label = nullptr;
	wxPanel* dummyPanel2 = nullptr;
	wxStateButton* tabButton = nullptr;

	SliderCategoryUI();

	bool IsCreated() { return isCreated; }

	bool Create(wxScrolledWindow* scrollWindow,
				wxSizer* sliderLayout,
				wxSizer* categoryTabSizer,
				const std::string& name,
				const std::vector<std::string>& sliders,
				bool pEnabled = true,
				bool pOneSize = false);
	void Show(bool show = true);
	void Destroy();
};

class SliderDisplay {
	bool isCreated = false;

public:
	bool isShown = false;
	bool isZap = false;
	bool oneSize = false;

	std::string sliderName;
	std::string displayName;
	std::string categoryName;

	wxStaticText* lblSliderLo = nullptr;
	wxSlider* sliderLo = nullptr;
	wxTextCtrl* sliderReadoutLo = nullptr;
	wxStaticText* lblSliderHi = nullptr;
	wxSlider* sliderHi = nullptr;
	wxTextCtrl* sliderReadoutHi = nullptr;
	wxCheckBox* zapCheckHi = nullptr;
	wxCheckBox* zapCheckLo = nullptr;

	SliderDisplay();

	bool IsCreated() { return isCreated; }

	bool Create(
		wxScrolledWindow* scrollWindow, wxSizer* sliderLayout, const std::string& name, const std::string& category, const std::string& display, int minValue, int maxValue, bool pIsZap, bool pOneSize = false);
	void Show(bool show = true);
};

class SliderDisplayPool {
	std::vector<SliderDisplay*> pool;

	const size_t MaxPoolSize = 500;

public:
	SliderDisplay* Push();
	void CreatePool(size_t poolSize, wxScrolledWindow* scrollWindow, wxSizer* sliderLayout);
	SliderDisplay* Get(size_t index);
	SliderDisplay* GetNext();
	void Clear();
};

class BodySlideFrame : public wxFrame {
public:
	SliderDisplayPool sliderPool;
	std::unordered_map<std::string, SliderDisplay*> sliderDisplays;
	std::unordered_map<std::string, SliderCategoryUI*> sliderCategories;

	wxTimer delayLoad;

	wxChoice* outfitChoice = nullptr;
	wxChoice* presetChoice = nullptr;
	wxButton* btnFavoriteOutfit = nullptr;
	wxButton* btnFavoritePreset = nullptr;
	wxButton* btnSavePreset = nullptr;
	wxSearchCtrl* search = nullptr;
	wxSearchCtrl* outfitsearch = nullptr;
	wxSearchCtrl* sliderFilter = nullptr;
	wxSearchCtrl* presetFilter = nullptr;
	wxSizer* categoryTabSizer = nullptr;

	wxScrolledWindow* sliderScroll = nullptr;
	wxFlexGridSizer* sliderLayout = nullptr;

	wxCheckListBox* batchBuildList = nullptr;
	wxMenu* fileCollisionMenu = nullptr;
	std::vector<std::string> outfitChoiceNames;
	std::vector<std::string> presetChoiceNames;
	bool populatingChoices = false;

	// Splitter and embedded preview
	wxSplitterWindow* splitter = nullptr;
	wxPanel* leftPanel = nullptr;
	PreviewPanel* previewPanel = nullptr;
	bool previewVisible = true;
	bool previewOnLeft = false;
	int savedSashPosition = -1;
	int savedPreviewWidth = 0;

	// Helpers for preview docking/undocking
	void UnsplitPreview();
	void SplitPreview(wxPanel* panel = nullptr);
	void SetPreviewOnLeft(bool onLeft);
	void UpdatePreviewButtonLabel();

	BodySlideFrame(BodySlideApp* app, const wxSize& size);
	~BodySlideFrame() { delete fileCollisionMenu; }

	void HideSlider(SliderDisplay* slider);
	void ShowLowColumn(bool show);
	void AddCategorySliderUI(const std::string& name, const std::vector<std::string>& sliders, bool enabled, bool oneSize);
	void AddSliderGUI(const std::string& name, const std::string& display, const std::string& categoryName, bool isZap, bool oneSize = false);

	SliderDisplay* GetSliderDisplay(const std::string& name) {
		if (sliderDisplays.find(name) != sliderDisplays.end())
			return sliderDisplays[name];

		return nullptr;
	}

	SliderCategoryUI* GetSliderCategory(const std::string& name) {
		if (sliderCategories.find(name) != sliderCategories.end())
			return sliderCategories[name];

		return nullptr;
	}

	void ClearPresetList();
	void ClearOutfitList();
	void ClearSliderGUI();
	void SetPresetChanged(bool changed = true);

	void PopulateOutfitList(const wxArrayString& items, const wxString& selectItem);
	void PopulatePresetList(const wxArrayString& items, const wxString& selectItem);
	std::string GetSelectedOutfitName() const;
	std::string GetSelectedPresetName() const;

	void SetSliderPosition(const wxString& name, float newValue, short HiLo);
	void DoFilterSliders();

	int lastScroll = 0;

private:
	void OnExit(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnActivateFrame(wxActivateEvent& event);
	void OnIconizeFrame(wxIconizeEvent& event);
	void PostIconizeFrame();

	void OnLinkClicked(wxHtmlLinkEvent& link);
	void OnEnterClose(wxKeyEvent& event);

	void OnEnterSliderWindow(wxMouseEvent& event);
	wxString FavoriteChoiceLabel(const std::string& name, bool favorite) const;
	bool SelectChoiceName(wxChoice* choice, const std::vector<std::string>& names, const std::string& selectItem) const;
	void SetFavoriteButtonBitmap(wxButton* button, bool favorite) const;
	void UpdateFavoriteButtons();
	void RebuildOutfitChoice(const std::string& selectItem);
	void RebuildPresetChoice(const std::string& selectItem);
	void OnSliderChange(wxScrollEvent& event);
	void OnSliderReadoutChange(wxCommandEvent& event);
	void OnSearchChange(wxCommandEvent& event);
	void OnOutfitSearchChange(wxCommandEvent& event);

	void OnSliderFilterChanged(wxCommandEvent&);
	void OnPresetFilterChanged(wxCommandEvent&);

	void OnZapCheckChanged(wxCommandEvent& event);
	void OnCategoryCheckChanged(wxCommandEvent& event);
	void OnCategoryTabButton(wxCommandEvent& event);

	void OnEraseBackground(wxEraseEvent& event);

	void OnDelayLoad(wxTimerEvent& event);

	void OnChooseGroups(wxCommandEvent& event);
	void OnRefreshGroups(wxCommandEvent& event);
	void OnBrowseOutfitFolder(wxCommandEvent& event);
	void OnSaveGroups(wxCommandEvent& event);
	void OnRefreshOutfits(wxCommandEvent& event);
	void OnRegexOutfits(wxCommandEvent& event);
	void OnFilterHasZaps(wxCommandEvent& event);
	void OnFilterOutputWinners(wxCommandEvent& event);

	void OnChooseOutfit(wxCommandEvent& event);
	void OnChoosePreset(wxCommandEvent& event);
	void OnFavoriteOutfit(wxCommandEvent& event);
	void OnFavoritePreset(wxCommandEvent& event);

	void OnDeleteProject(wxCommandEvent& event);
	void OnDeletePreset(wxCommandEvent& event);

	void OnEditPreset(wxCommandEvent& event);
	void OnSavePreset(wxCommandEvent& event);
	void OnSavePresetAs(wxCommandEvent& event);
	void OnGroupManager(wxCommandEvent& event);
	void OnConflictPopup(wxMouseEvent& event);
	void OnOutfitChoiceSelect(wxCommandEvent& event);

	void OnPreview(wxCommandEvent& event);
	void OnSashPosChanging(wxSplitterEvent& event);
	void OnSashPosChanged(wxSplitterEvent& event);
	void OnPreviewPopout(wxCommandEvent& event);
	void OnPreviewWindowClosed();

	void OnHighToLow(wxCommandEvent& event);
	void OnLowToHigh(wxCommandEvent& event);
	void OnBuildBodies(wxCommandEvent& event);
	void OnBatchBuild(wxCommandEvent& event);
	void OnBatchBuildContext(wxMouseEvent& event);
	void OnBatchBuildSelect(wxCommandEvent& event);
	void OnOutfitStudio(wxCommandEvent& event);
	void SettingsFillDataFiles(wxCheckListBox* dataFileList, wxString& dataDir, int targetGame);
	void OnSettings(wxCommandEvent& event);
	void OnChooseTargetGame(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnMoveWindow(wxMoveEvent& event);
	void OnSetSize(wxSizeEvent& event);

	void OnEditProject(wxCommandEvent& event);

	void OnClippingStrengthChanged(wxCommandEvent& event);

	bool OutfitIsEmpty() {
		if (!GetSelectedOutfitName().empty())
			return false;

		return true;
	}

	void RefreshTargetGameState();

	BodySlideApp* app;

	wxDECLARE_EVENT_TABLE();
};
