/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "Anim.h"
#include "ConformOptions.h"
#include "DiffData.h"
#include "SliderSet.h"
#include "../utils/StringStuff.h"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// A project as it is saved in its file, without anything from a base project. Cheap to collect for every project.
struct ProjectCatalogEntry {
	std::string fileName; // Full path of the project file
	std::string setName;
	std::string projectID;
	std::string baseName; // Base project as it was saved, empty for a project without one
	std::string baseID;
	bool addOn = false;
	std::string dataFolder; // Add-ons save the data folder and NIF of their base project
	std::string sourceFile;
	std::string outputFilePath; // Output path and file

	// Index of the entry of the base project, or npos if it isn't installed
	size_t baseIndex = static_cast<size_t>(-1);
	// Add-ons that BodySlide builds together with their base project instead of on their own, because
	// they build the same output file from the same NIF
	bool stackable = false;
};

// The projects of a project path, the way BodySlide lists them: the first set of a name counts.
class ProjectCatalog {
	std::vector<ProjectCatalogEntry> entries;
	std::map<std::string, size_t, case_insensitive_compare> byName;
	std::map<std::string, size_t> byID;

public:
	static constexpr size_t npos = static_cast<size_t>(-1);

	void Clear();
	// Adds a set of the file, unless a set of the name was added before. Returns false then.
	bool Add(SliderSetFile& file, const std::string& setName);
	// Adds all sets of all project files in the "SliderSets" folder of a project path
	void Load(const std::string& projectPath);
	// Links the add-ons and variants to their base projects, once all files are added
	void Finish();

	const std::vector<ProjectCatalogEntry>& Entries() const { return entries; }

	// The project with the name if its ID fits, or else the one with the ID, which finds a renamed project
	const ProjectCatalogEntry* Find(const std::string& name, const std::string& id = "") const;
	// True if the project is installed: one with the ID, or with the name if there's no ID
	bool IsInstalled(const std::string& name, const std::string& id) const;

	// Add-ons of the project and of its add-ons, in the order they were added.
	// With 'stackableOnly', only the ones that BodySlide builds together with the project.
	std::vector<const ProjectCatalogEntry*> AddOnsOf(const std::string& name, const std::string& id, bool stackableOnly) const;
	// True if the project is a stackable add-on of another one. 'outBase' gets the project it's built with.
	bool IsStackedAddOn(const std::string& name, std::string* outBase = nullptr) const;
	// The project the set is built with: the project itself, or the base project of a stackable add-on
	std::string StackRoot(const std::string& name) const;
};

// A stacked add-on of a project
struct StackedAddOn {
	std::string setName;
	std::string projectID;
	std::string fileName;
	std::map<std::string, uint32_t> vertexCounts; // By shape, the vertex count its data was made for, 0 if unknown
};

// Adds the sliders and slider data of the stackable add-ons of the set to it, in their order.
// Their changes to sliders the set has replace those, but sliders they leave out stay.
// 'name'/'id' are the project whose add-ons are stacked, the set's own if empty. With 'newSlidersOnly', only
// sliders the set doesn't have are added, for the add-ons of a variant's base project.
// Warnings get what went wrong.
std::vector<StackedAddOn> StackAddOns(SliderSet& set,
									   const ProjectCatalog& catalog,
									   std::vector<std::string>& warnings,
									   const std::string& name = "",
									   const std::string& id = "",
									   bool stackableOnly = true,
									   bool newSlidersOnly = false);

// What stacking the add-ons of a set gave
struct SetStackResult {
	std::vector<StackedAddOn> addOns;	  // Of the set
	std::vector<StackedAddOn> baseAddOns; // Of the base projects of a variant, which FitBaseAddOnsToVariant fits to its shapes
	std::vector<std::string> missingAddOns; // That sliders of the set need, but aren't installed
};

// Stacks the add-ons of the set and, for a variant, the add-ons of its base projects (new sliders only), and removes
// the sliders whose add-on isn't installed. With 'allAddOns', also add-ons that build another output file.
SetStackResult StackSetAddOns(SliderSet& set, const ProjectCatalog& catalog, std::vector<std::string>& warnings, bool allAddOns = false);

// Removes the sliders that need an add-on that isn't installed. Returns the names of those add-ons.
std::vector<std::string> RemoveUninstalledAddOnSliders(SliderSet& set, const ProjectCatalog& catalog);

// The body whose add-on sliders can be conformed to an outfit, and the shape to conform them from
struct ConformBodyRef {
	std::string fileName; // Full path of the project file, empty if no body was found
	std::string setName;
	std::string projectID;
	std::string shapeName;
	bool fromReferenceInfo = false;

	bool Found() const { return !fileName.empty(); }
};

// Finds the body that the outfit was conformed to: from its reference info, or else from a shape whose
// data is in the ShapeData folder of a body project with add-ons, for a shape of that body.
// None for the body itself and for projects that it is the base of.
ConformBodyRef FindConformBody(const SliderSet& outfit, const ProjectCatalog& catalog, const std::string& projectPath);

// A body with its add-ons stacked, loaded to conform its add-on sliders to outfits
class ConformBody {
public:
	struct Slider {
		SliderData slider;		 // With its properties, but no data
		std::string refDataName; // Data of the reference shape
		std::string addOnName;	 // The add-on it comes from
	};

	ConformBodyRef ref;
	std::string targetName;
	nifly::NifFile nif;
	AnimInfo anim;
	DiffDataSets diffs; // Data of the reference shape of the sliders
	std::vector<Slider> sliders;
	std::vector<std::string> addOnNames; // Stacked add-ons that have sliders

	ConformBody() = default;
	ConformBody(const ConformBody&) = delete;
	ConformBody& operator=(const ConformBody&) = delete;

	// Loads the body and the add-on sliders of the reference shape. 'loadNif' loads a NIF file, false if it fails.
	bool Load(const ConformBodyRef& bodyRef,
			  const ProjectCatalog& catalog,
			  const std::string& baseDataPath,
			  const std::function<bool(const std::string&, nifly::NifFile&)>& loadNif,
			  std::vector<std::string>& warnings);

	nifly::NiShape* RefShape() { return nif.FindBlockByName<nifly::NiShape>(ref.shapeName); }
};

// For a variant with the stacked add-ons of its base project: the data of their sliders for shapes whose vertex count
// isn't the one the data was made for is conformed from the base project's shape with 'conform', and else left out.
// Sliders without data are removed then. 'anim' is the skinning of the variant's NIF, only needed with 'conform'.
void FitBaseAddOnsToVariant(SliderSet& set,
							nifly::NifFile& nif,
							const AnimInfo* anim,
							const std::vector<StackedAddOn>& baseAddOns,
							const ProjectCatalog& catalog,
							const std::string& baseDataPath,
							const std::function<bool(const std::string&, nifly::NifFile&)>& loadNif,
							bool conform,
							std::vector<std::string>& warnings,
							const ConformOptions& options = ConformOptions());

// Loads the skinning of all shapes of a NIF file for conforming. Safe to call on several threads at once.
void LoadConformAnim(AnimInfo& anim, nifly::NifFile& nif);

// Conforms the add-on sliders of the body that the set doesn't have to the set's shapes, like Conform in
// Outfit Studio. A shape with the reference shape's target and vertex count gets the reference data as it is.
// The sliders are added to the set with generated data and marked as conformed automatically.
// Returns the names of the add-ons that sliders were conformed from.
std::vector<std::string> ConformAddOnSliders(SliderSet& set, nifly::NifFile& setNif, const AnimInfo& setAnim, ConformBody& body, const ConformOptions& options = ConformOptions());

// Conforms reference data to shapes of a NIF file like OutfitProject::ConformShape does without masks.
// 'sliders' are slider names with the data name of the reference shape. Results are by shape and slider name.
std::map<std::string, std::map<std::string, TargetDataDiffs>> ConformShapesData(nifly::NifFile& refNif,
																				nifly::NiShape* refShape,
																				const AnimInfo& refAnim,
																				DiffDataSets& refDiffs,
																				nifly::NifFile& nif,
																				const AnimInfo& anim,
																				const std::vector<std::string>& shapeNames,
																				const std::vector<std::pair<std::string, std::string>>& sliders,
																				const ConformOptions& options = ConformOptions());
