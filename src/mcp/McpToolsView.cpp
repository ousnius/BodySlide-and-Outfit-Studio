/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "OutfitStudioMcp.h"
#include "../components/PoseMath.h"
#include "../program/OutfitStudio.h"

#include <wx/dcmemory.h>
#include <wx/mstream.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <optional>
#include <set>

using nlohmann::json;
using namespace nifly;

namespace {

const std::vector<std::string> ViewPresets = {"front", "back", "left", "right", "top", "bottom"};

const char* ModeName(ShapeDisplayMode mode) {
	switch (mode) {
		case ShapeDisplayMode::Hidden: return "hidden";
		case ShapeDisplayMode::Ghost: return "ghost";
		default: return "visible";
	}
}

ShapeDisplayMode ModeFromName(const std::string& name) {
	if (name == "visible")
		return ShapeDisplayMode::Visible;
	if (name == "hidden")
		return ShapeDisplayMode::Hidden;
	if (name == "ghost")
		return ShapeDisplayMode::Ghost;
	throw McpToolError("Display mode '" + name + "' has to be visible, hidden or ghost.");
}

json VectorToJson(const Vector3& v) {
	return json::array({v.x, v.y, v.z});
}

Vector3 VectorFromJson(const json& value, const char* name) {
	if (!value.is_array() || value.size() != 3 || !value[0].is_number() || !value[1].is_number() || !value[2].is_number())
		McpArgs::Wrong(name, "an array of three numbers");
	return Vector3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>());
}

json ViewStateToJson(const ViewState& state) {
	json shapes = json::object();
	for (auto& shape : state.shapes)
		shapes[shape.first] = ModeName(shape.second);

	return {
		{"camera",
		 {{"position", VectorToJson(state.camera.camPos)},
		  {"offset", VectorToJson(state.camera.camOffset)},
		  {"rotation", VectorToJson(state.camera.camRot)},
		  {"rotationOffset", VectorToJson(state.camera.camRotOffset)},
		  {"perspective", state.camera.perspective},
		  {"fov", state.camera.fov}}},
		{"display",
		 {{"wireframe", state.display.wireframe},
		  {"lighting", state.display.lighting},
		  {"textures", state.display.textures},
		  {"mask", state.display.mask},
		  {"weightColors", state.display.weightColors},
		  {"vertexColors", state.display.vertexColors}}},
		{"shapes", shapes},
	};
}

// View overlays that only the menu keeps, by argument name and menu item
const std::pair<const char*, const char*> Overlays[] = {{"bones", "btnShowBones"}, {"nodes", "btnShowNodes"}, {"floor", "btnShowFloor"}};

json OverlaysToJson(OutfitStudioFrame* frame) {
	json overlays = json::object();
	for (auto& overlay : Overlays)
		overlays[overlay.first] = frame->GetMenuBar()->IsChecked(XRCID(overlay.second));
	return overlays;
}

void ApplyOverlays(OutfitStudioFrame* frame, const json& args) {
	for (auto& overlay : Overlays) {
		if (!McpArgs::Has(args, overlay.first))
			continue;

		// The menu handler keeps the menu, the toolbar and the view in step
		wxCommandEvent event(wxEVT_MENU, XRCID(overlay.second));
		event.SetInt(McpArgs::Bool(args, overlay.first, false) ? 1 : 0);
		frame->GetEventHandler()->ProcessEvent(event);
	}
}

const json OverlaysSchema = {
	{"type", "object"},
	{"description", "Overlays to show or hide, the others stay"},
	{"properties",
	 {{"bones", {{"type", "boolean"}, {"description", "Bone lines of the skeleton"}}},
	  {"nodes", {{"type", "boolean"}, {"description", "Bone nodes as points"}}},
	  {"floor", {{"type", "boolean"}, {"description", "Floor grid"}}}}},
};

void ApplyDisplay(GLSurface::DisplaySettings& display, const json& args) {
	display.wireframe = McpArgs::Bool(args, "wireframe", display.wireframe);
	display.lighting = McpArgs::Bool(args, "lighting", display.lighting);
	display.textures = McpArgs::Bool(args, "textures", display.textures);
	display.mask = McpArgs::Bool(args, "mask", display.mask);
	display.weightColors = McpArgs::Bool(args, "weightColors", display.weightColors);
	display.vertexColors = McpArgs::Bool(args, "vertexColors", display.vertexColors);
}

void ApplyCamera(GLSurface::CameraState& camera, const json& args) {
	if (const json* value = McpArgs::Find(args, "position"))
		camera.camPos = VectorFromJson(*value, "position");
	if (const json* value = McpArgs::Find(args, "offset"))
		camera.camOffset = VectorFromJson(*value, "offset");
	if (const json* value = McpArgs::Find(args, "rotation"))
		camera.camRot = VectorFromJson(*value, "rotation");
	if (const json* value = McpArgs::Find(args, "rotationOffset"))
		camera.camRotOffset = VectorFromJson(*value, "rotationOffset");
	camera.perspective = McpArgs::Bool(args, "perspective", camera.perspective);
	camera.fov = McpArgs::Float(args, "fov", camera.fov);
}

const json DisplaySchema = {
	{"type", "object"},
	{"description", "Display toggles to change, the others stay"},
	{"properties",
	 {{"wireframe", {{"type", "boolean"}}},
	  {"lighting", {{"type", "boolean"}}},
	  {"textures", {{"type", "boolean"}}},
	  {"mask", {{"type", "boolean"}, {"description", "Show masked vertices darkened"}}},
	  {"weightColors", {{"type", "boolean"}, {"description", "Show the weights of the active bone as colors"}}},
	  {"vertexColors", {{"type", "boolean"}}}}},
};

const json ShapeModesSchema = {
	{"type", "object"},
	{"additionalProperties", {{"type", "string"}, {"enum", {"visible", "hidden", "ghost"}}}},
	{"description", "Display mode per shape name: visible, hidden or ghost (see-through)"},
};

void CheckPreset(const std::string& name) {
	if (std::find(ViewPresets.begin(), ViewPresets.end(), name) == ViewPresets.end())
		throw McpToolError("View '" + name + "' has to be one of front, back, left, right, top, bottom.");
}

GLSurface& Surface(OutfitStudioFrame* frame) {
	if (!frame->glView)
		throw McpToolError("The view isn't available.");
	return frame->glView->gls;
}

void FrameTarget(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	if (McpArgs::Has(args, "bone")) {
		std::string bone = McpArgs::String(args, "bone");
		if (!frame->FrameBone(bone, McpArgs::Float(args, "minWeight", 0.1f)))
			throw McpToolError("No vertices are weighted to the bone '" + bone + "'.");
	}
	else if (McpArgs::Has(args, "vertices")) {
		std::string shape = McpArgs::String(args, "shape");
		mcp.Shape(shape);
		if (!frame->FrameVertices(shape, McpArgs::Ints(args, "vertices")))
			throw McpToolError("None of the vertices exist on '" + shape + "'.");
	}
	else {
		std::vector<std::string> shapes = McpArgs::Strings(args, "shapes");
		if (shapes.empty())
			shapes = mcp.Project().GetShapeOrder();
		for (auto& shape : shapes)
			mcp.Shape(shape);
		if (!frame->FrameShapes(shapes))
			throw McpToolError("The shapes have nothing to frame.");
	}
}

// The point a camera target names in mesh units: a bone where the pose has it, the middle of a shape as shown, or
// a position
Vector3 CameraTarget(OutfitStudioMcp& mcp, const json& target) {
	if (!target.is_string())
		return VectorFromJson(target, "orbit target");

	const std::string name = target.get<std::string>();
	if (AnimBone* bone = AnimSkeleton::getInstance().GetBonePtr(name))
		return (mcp.Project().bPose ? bone->xformPoseToGlobal : bone->xformToGlobal).translation * McpBoneUnits(mcp);

	NiShape* shape = mcp.Project().GetWorkNif()->FindBlockByName<NiShape>(name);
	if (!shape)
		throw McpToolError("The orbit target '" + name + "' is neither a bone of the skeleton nor a shape.");

	std::vector<Vector3> verts = McpGlobalLiveVerts(mcp, shape);
	if (verts.empty())
		throw McpToolError("The shape '" + name + "' has no vertices to look at.");

	Vector3 lo = verts[0], hi = verts[0];
	for (auto& v : verts) {
		lo.x = std::min(lo.x, v.x);
		lo.y = std::min(lo.y, v.y);
		lo.z = std::min(lo.z, v.z);
		hi.x = std::max(hi.x, v.x);
		hi.y = std::max(hi.y, v.y);
		hi.z = std::max(hi.z, v.z);
	}
	return (lo + hi) * 0.5f;
}

// Puts the camera on a sphere around a target: yaw 0 looks at her front, 90 from her right, pitch from above
void ApplyOrbit(OutfitStudioMcp& mcp, GLSurface::CameraState& camera, const json& orbit) {
	if (!orbit.is_object())
		McpArgs::Wrong("orbit", "an object with target, yaw, pitch and distance");

	Vector3 target = Mesh::TransformPosMeshToNif(camera.camOffset * -1.0f);
	if (const json* value = McpArgs::Find(orbit, "target"))
		target = CameraTarget(mcp, *value);

	const float distance = McpArgs::Float(orbit, "distance", -camera.camPos.z * 10.0f);
	if (distance <= 0.0f)
		McpArgs::Wrong("distance", "a distance above 0");

	camera.camOffset = Mesh::TransformPosNifToMesh(target) * -1.0f;
	camera.camPos = Vector3(0.0f, 0.0f, -distance / 10.0f);
	camera.camRot = Vector3(McpArgs::Float(orbit, "pitch", camera.camRot.x), McpArgs::Float(orbit, "yaw", camera.camRot.y), 0.0f);
	camera.camRotOffset = Vector3();
	if (McpArgs::Has(orbit, "fov")) {
		camera.fov = std::clamp(McpArgs::Float(orbit, "fov", camera.fov), 10.0f, 150.0f);
		camera.perspective = true;
	}
}

// The camera as an orbit, when it turns around the point it looks at
json OrbitToJson(const GLSurface::CameraState& camera) {
	if (!camera.camRotOffset.IsZero(true) || std::abs(camera.camPos.x) > 1e-4f || std::abs(camera.camPos.y) > 1e-4f)
		return nullptr;
	return {
		{"target", VectorToJson(Mesh::TransformPosMeshToNif(camera.camOffset * -1.0f))},
		{"yaw", camera.camRot.y},
		{"pitch", camera.camRot.x},
		{"distance", -camera.camPos.z * 10.0f},
		{"fov", camera.fov},
	};
}

const json OrbitSchema = {
	{"type", "object"},
	{"description", "Camera around a point, in the character's terms: yaw 0 looks at her front, 90 from her right, -90 from her "
					"left, 180 from behind; pitch looks from above (positive) or below (negative); distance in mesh units "
					"(a body is about 120 tall). Fields left out keep the camera's."},
	{"properties",
	 {{"target", {{"description", "A bone name (where the pose has it), a shape name (its middle) or a position [x, y, z] in mesh units"}}},
	  {"yaw", McpProp("number", "Degrees around her, 0 in front, 90 at her right")},
	  {"pitch", McpProp("number", "Degrees up (positive, looking down) or down (negative, looking up)")},
	  {"distance", McpProp("number", "From the target in mesh units")},
	  {"fov", McpProp("number", "Field of view in degrees")}}},
};

// Bones drawn over a render: the main bones or a list, each linked to the closest ancestor drawn
struct SkeletonOverlay {
	std::vector<AnimBone*> bones;
	bool labels = true;
	bool only = false;
};

std::optional<SkeletonOverlay> SkeletonArg(const json& args) {
	const json* value = McpArgs::Find(args, "skeleton");
	if (!value || (value->is_boolean() && !value->get<bool>()))
		return std::nullopt;

	SkeletonOverlay overlay;
	std::string mode = "main";
	std::vector<std::string> names;
	if (value->is_object()) {
		overlay.labels = McpArgs::Bool(*value, "labels", true);
		overlay.only = McpArgs::Bool(*value, "only", false);
		if (const json* bones = McpArgs::Find(*value, "bones")) {
			if (bones->is_string())
				mode = bones->get<std::string>();
			else {
				mode = "list";
				names = McpArgs::Strings(*value, "bones");
			}
		}
	}
	else if (!value->is_boolean())
		McpArgs::Wrong("skeleton", "true or {bones, labels, only}");

	AnimSkeleton& skeleton = AnimSkeleton::getInstance();
	if (mode == "list") {
		for (auto& name : names) {
			AnimBone* bone = skeleton.GetBonePtr(name);
			if (!bone)
				throw McpToolError("There is no bone '" + name + "' in the skeleton.");
			overlay.bones.push_back(bone);
		}
	}
	else if (mode == "main" || mode == "all") {
		std::vector<std::string> all;
		skeleton.GetBoneNames(all);
		for (auto& name : all)
			if (AnimBone* bone = skeleton.GetBonePtr(name))
				overlay.bones.push_back(bone);
		if (mode == "main")
			overlay.bones = PoseMath::MainBones(overlay.bones);
	}
	else
		McpArgs::Wrong("skeleton.bones", "\"main\", \"all\" or a list of bone names");

	if (overlay.bones.empty())
		throw McpToolError("The skeleton has no bones to draw.");
	return overlay;
}

// Draws the bones over a rendered image as seen by the camera: lines to the parent drawn, joints as dots, and the
// short names. Red is her right side, blue her left, green the middle.
void DrawSkeleton(wxImage& image, const GLSurface& surface, const SkeletonOverlay& overlay, bool posed, float units) {
	const int width = image.GetWidth();
	const int height = image.GetHeight();

	auto project = [&](const AnimBone* bone, wxPoint& out) {
		const Vector3 position = (posed ? bone->xformPoseToGlobal : bone->xformToGlobal).translation * units;
		float x = 0.0f, y = 0.0f;
		if (!surface.ProjectPointToImage(Mesh::TransformPosNifToMesh(position), static_cast<uint32_t>(width), static_cast<uint32_t>(height), x, y))
			return false;
		out = wxPoint(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)));
		return true;
	};
	auto colour = [&](const AnimBone* bone) {
		// The side the bone reaches to: a clavicle can start across the middle (Starfield's)
		float x = bone->xformToGlobal.translation.x;
		if (AnimBone* reach = PoseMath::ReachChild(bone))
			x += reach->xformToGlobal.translation.x;
		x *= units;
		if (x > 0.5f)
			return wxColour(230, 60, 60);
		if (x < -0.5f)
			return wxColour(60, 130, 255);
		return wxColour(40, 200, 80);
	};

	std::set<const AnimBone*> drawn(overlay.bones.begin(), overlay.bones.end());
	auto drawnParent = [&](const AnimBone* bone) -> const AnimBone* {
		for (const AnimBone* p = bone->parent; p; p = p->parent)
			if (drawn.count(p))
				return p;
		return nullptr;
	};

	wxBitmap bitmap(image);
	{
		wxMemoryDC dc(bitmap);
		const int lineWidth = std::max(2, height / 300);
		const int radius = std::max(3, height / 200);

		for (const AnimBone* bone : overlay.bones) {
			const AnimBone* parent = drawnParent(bone);
			wxPoint from, to;
			if (!parent || !project(bone, to) || !project(parent, from))
				continue;
			dc.SetPen(wxPen(colour(bone), lineWidth));
			dc.DrawLine(from, to);
		}

		dc.SetFont(wxFont(wxFontInfo(std::max(8, height / 75))));
		dc.SetTextForeground(*wxWHITE);
		dc.SetTextBackground(wxColour(0, 0, 0));
		dc.SetBackgroundMode(wxBRUSHSTYLE_SOLID);
		for (const AnimBone* bone : overlay.bones) {
			wxPoint at;
			if (!project(bone, at))
				continue;
			dc.SetPen(wxPen(*wxBLACK, 1));
			dc.SetBrush(wxBrush(colour(bone)));
			dc.DrawCircle(at, radius);
			if (overlay.labels)
				dc.DrawText(wxString::FromUTF8(PoseMath::ShortBoneName(bone->boneName)), at.x + radius + 1, at.y - radius - 1);
		}
	}
	image = bitmap.ConvertToImage();
}

std::vector<uint8_t> EncodePNG(const wxImage& image) {
	wxMemoryOutputStream stream;
	if (!image.SaveFile(stream, wxBITMAP_TYPE_PNG))
		throw McpToolError("The image couldn't be encoded.");

	std::vector<uint8_t> png(stream.GetSize());
	stream.CopyTo(png.data(), png.size());
	return png;
}

// Puts the view and slider values back the way they were after a render, also on errors
class RenderRestore {
public:
	RenderRestore(OutfitStudioFrame* inFrame)
		: frame(inFrame)
		, view(inFrame->GetViewState()) {
		OutfitProject* project = frame->project;
		for (size_t i = 0; i < project->SliderCount(); i++)
			sliderValues.push_back(project->SliderValue(i));
	}

	~RenderRestore() {
		// The pose first, the meshes are rebuilt with the slider values below
		if (pose) {
			pose.reset();
			frame->ApplyRefresh(RefreshFlags::Pose | RefreshFlags::Meshes);
		}

		OutfitProject* project = frame->project;
		bool changed = false;
		for (size_t i = 0; i < sliderValues.size() && i < project->SliderCount(); i++) {
			if (project->SliderValue(i) != sliderValues[i]) {
				project->SliderValue(i) = sliderValues[i];
				changed = true;
			}
		}

		if (changed)
			frame->ApplyRefresh(RefreshFlags::SliderValues | RefreshFlags::Meshes);

		if (vertexColorsChanged)
			frame->FillVertexColors();
		frame->SetViewState(view);
		if (weightColorsChanged)
			frame->RefreshGUIWeightColors();
	}

	const ViewState& View() const { return view; }
	bool weightColorsChanged = false;
	// The shapes' own vertex colors are put back, e.g. after highlighting vertices
	bool vertexColorsChanged = false;
	// Set while a render poses the skeleton, puts the pose back when it goes
	std::optional<McpPoseScope> pose;

private:
	OutfitStudioFrame* frame = nullptr;
	ViewState view;
	std::vector<float> sliderValues;
};

McpToolResult RenderView(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	Surface(frame);

	const int width = std::clamp(McpArgs::Int(args, "width", 768), 64, 2048);
	const int height = std::clamp(McpArgs::Int(args, "height", 768), 64, 2048);

	std::vector<std::string> views = McpArgs::Strings(args, "views");
	if (views.empty())
		views.push_back("current");
	if (views.size() > 12)
		throw McpToolError("Render at most 12 views per call.");
	for (auto& view : views)
		if (view != "current")
			CheckPreset(view);

	std::vector<std::string> frameShapes = McpArgs::Strings(args, "frame");
	for (auto& shape : frameShapes)
		mcp.Shape(shape);

	RenderRestore restore(frame);
	ViewState state = restore.View();

	if (const json* display = McpArgs::Find(args, "display"))
		ApplyDisplay(state.display, *display);

	// Only the listed shapes, the others are hidden
	std::vector<std::string> show = McpArgs::Strings(args, "show");
	if (!show.empty()) {
		for (auto& shape : state.shapes)
			shape.second = ShapeDisplayMode::Hidden;
		for (auto& name : show) {
			mcp.Shape(name);
			state.shapes[name] = ShapeDisplayMode::Visible;
		}
	}
	for (auto& name : McpArgs::Strings(args, "ghost")) {
		mcp.Shape(name);
		state.shapes[name] = ShapeDisplayMode::Ghost;
	}

	if (const json* orbit = McpArgs::Find(args, "orbit"))
		ApplyOrbit(mcp, state.camera, *orbit);

	std::optional<SkeletonOverlay> skeleton = SkeletonArg(args);
	if (skeleton && skeleton->only)
		for (auto& shape : state.shapes)
			shape.second = ShapeDisplayMode::Hidden;

	std::string weightBone = McpArgs::String(args, "weightBone", "");
	if (!weightBone.empty()) {
		std::vector<NiShape*> shapes;
		bool found = false;
		for (auto* shape : project.GetWorkNif()->GetShapes()) {
			shapes.push_back(shape);
			if (project.GetWorkAnim()->GetShapeBoneIndex(shape->name.get(), weightBone) >= 0)
				found = true;
		}
		if (!found)
			throw McpToolError("No shape has the bone '" + weightBone + "'.");

		state.display.weightColors = true;
		mcp.Ops().FillWeightColors(weightBone, shapes);
		restore.weightColorsChanged = true;
	}

	auto sliderValues = McpSliderValuesArg(mcp, args);
	if (McpArgs::Bool(args, "zeroOthers", false)) {
		// Only the given values show, not what an earlier apply_preset or set_slider_values left set
		std::map<std::string, float> zero;
		for (size_t i = 0; i < project.SliderCount(); i++)
			if (!project.SliderClamp(i))
				zero[project.GetSliderName(i)] = 0.0f;
		for (auto& value : sliderValues)
			zero[value.first] = value.second;
		sliderValues = zero;
	}
	if (!sliderValues.empty())
		mcp.Apply(mcp.Ops().SetSliderValues(sliderValues));

	// Other sliders that are set show in every image, which is easy to forget after apply_preset
	json otherSliders = json::array();
	for (size_t i = 0; i < project.SliderCount(); i++) {
		const std::string name = project.GetSliderName(i);
		if (project.SliderValue(i) != 0.0f && !project.SliderZap(i) && !project.SliderClamp(i) && !sliderValues.count(name))
			otherSliders.push_back(name);
	}

	// Vertices to fit into each image: listed ones, or the masked or unmasked vertices of a shape
	std::string focusShape;
	std::vector<int> focusVertices;
	if (const json* focus = McpArgs::Find(args, "focus")) {
		if (!focus->is_object())
			McpArgs::Wrong("focus", "an object with shape and vertices, masked or unmasked");
		focusShape = McpArgs::String(*focus, "shape");
		mcp.Shape(focusShape);
		focusVertices = McpArgs::Ints(*focus, "vertices");
		const bool masked = McpArgs::Bool(*focus, "masked", false);
		const bool unmasked = McpArgs::Bool(*focus, "unmasked", false);
		if (masked || unmasked) {
			Mesh* m = frame->glView->GetMesh(focusShape);
			for (int v = 0; m && v < m->nVerts; v++) {
				const bool isMasked = m->mask && m->mask[v] > 0.0f;
				if (isMasked == masked)
					focusVertices.push_back(v);
			}
		}
		if (focusVertices.empty())
			throw McpToolError("focus has no vertices on '" + focusShape + "'. Give vertices, masked: true or unmasked: true.");

		// Listed vertices show red on a grey shape, so they can be found in the image
		if (McpArgs::Bool(*focus, "highlight", McpArgs::Has(*focus, "vertices"))) {
			NiShape* shape = mcp.Shape(focusShape);
			std::vector<float> values(shape->GetNumVertices(), FLT_MAX);
			for (int v : focusVertices)
				if (v >= 0 && v < static_cast<int>(values.size()))
					values[v] = 1.0f;
			mcp.Apply(mcp.Ops().ShowHeatmap(shape, values, 0.0f, 1.0f));
			state.display.vertexColors = true;
			restore.vertexColorsChanged = true;
			// The mask display hides vertex colors, unless the mask is what the images are for
			const json* display = McpArgs::Find(args, "display");
			if (!display || !McpArgs::Has(*display, "mask"))
				state.display.mask = false;
		}
	}

	// Focus vertices facing away from a view are behind the shape or the body there, and the image shows nothing useful
	json facingAway = json::array();
	if (!focusShape.empty()) {
		const std::map<std::string, Vector3> viewFacing = {{"front", Vector3(0.0f, 1.0f, 0.0f)}, {"back", Vector3(0.0f, -1.0f, 0.0f)}, {"top", Vector3(0.0f, 0.0f, 1.0f)}, {"bottom", Vector3(0.0f, 0.0f, -1.0f)}};
		Vector3 mean;
		if (auto normals = project.GetWorkNif()->GetNormalsForShape(mcp.Shape(focusShape)))
			for (int v : focusVertices)
				if (v >= 0 && v < static_cast<int>(normals->size()))
					mean += (*normals)[v];
		if (!mean.IsZero(true)) {
			mean.Normalize();
			for (auto& view : views) {
				auto facing = viewFacing.find(view);
				if (facing != viewFacing.end() && mean.dot(facing->second) < -0.3f)
					facingAway.push_back(view);
			}
		}
	}

	// Each view is rendered for each variant: different slider values or poses side by side
	struct Variant {
		std::string label;
		std::map<std::string, float> values;
		std::string pose;
	};
	std::vector<Variant> variants;
	if (const json* list = McpArgs::Find(args, "variants")) {
		if (!list->is_array() || list->empty())
			McpArgs::Wrong("variants", "a list of {label, sliderValues, pose}");
		for (auto& item : *list) {
			if (!item.is_object())
				McpArgs::Wrong("variants", "a list of {label, sliderValues, pose}");
			Variant v;
			v.values = McpSliderValuesArg(mcp, item);
			v.pose = McpArgs::String(item, "pose", "");
			v.label = McpArgs::String(item, "label", "");
			// A preset sets every slider it has, the others are 0 like after apply_preset
			const std::string preset = McpArgs::String(item, "preset", "");
			if (!preset.empty()) {
				for (size_t i = 0; i < project.SliderCount(); i++)
					if (!project.SliderClamp(i))
						v.values.emplace(project.GetSliderName(i), 0.0f);
				if (v.label.empty())
					v.label = preset + " (" + McpArgs::String(item, "presetWeight", "high") + ")";
			}
			if (v.label.empty()) {
				for (auto& value : v.values)
					v.label += (v.label.empty() ? "" : ", ") + value.first + " " + wxString::Format("%g", value.second).ToStdString();
				if (!v.pose.empty())
					v.label += (v.label.empty() ? "" : " @ ") + v.pose;
				if (v.label.empty())
					v.label = "as is";
			}
			variants.push_back(v);
		}
	}
	else
		variants.push_back({});

	if (views.size() * variants.size() > 24)
		throw McpToolError("That's " + std::to_string(views.size() * variants.size()) + " images, render at most 24 per call.");

	const bool grid = McpArgs::Bool(args, "grid", false);
	if (grid && static_cast<size_t>(width) * views.size() * static_cast<size_t>(height) * variants.size() > 4096u * 4096u)
		throw McpToolError("The grid would be larger than 4096 x 4096 pixels. Use a smaller width and height.");

	// Slider values the variants start from
	std::map<std::string, float> baseValues;
	for (size_t i = 0; i < project.SliderCount(); i++)
		baseValues[project.GetSliderName(i)] = project.SliderValue(i);

	frame->SetViewState(state);

	McpToolResult result;
	std::vector<wxImage> images;
	std::vector<std::string> labels;
	for (auto& variant : variants) {
		if (!variant.values.empty() || variants.size() > 1) {
			std::map<std::string, float> values = baseValues;
			for (auto& value : variant.values) {
				if (!project.ValidSlider(value.first))
					throw McpToolError("There is no slider '" + value.first + "'.");
				values[value.first] = value.second;
			}
			mcp.Apply(mcp.Ops().SetSliderValues(values));
		}

		if (!variant.pose.empty()) {
			if (!restore.pose)
				restore.pose.emplace(mcp);
			restore.pose->Set(variant.pose);
			frame->ApplyRefresh(RefreshFlags::Pose | RefreshFlags::Meshes);
		}
		else if (restore.pose) {
			// A variant without a pose after one with a pose shows the pose from before the render
			restore.pose->Restore();
			frame->ApplyRefresh(RefreshFlags::Pose | RefreshFlags::Meshes);
		}

		for (auto& view : views) {
			// Every view starts from the camera of the render, not from the preset or framing of the image before
			Surface(frame).SetCamera(state.camera);
			if (view != "current")
				Surface(frame).SetViewPreset(view);
			if (!frameShapes.empty())
				frame->FrameShapes(frameShapes);
			if (!focusShape.empty())
				frame->FrameVertices(focusShape, focusVertices);

			wxImage image;
			if (!frame->glView->RenderViewImage(width, height, image))
				throw McpToolError("Rendering failed. The window may be minimized or the graphics driver doesn't support offscreen rendering.");
			if (skeleton)
				DrawSkeleton(image, Surface(frame), *skeleton, project.bPose, McpBoneUnits(mcp));

			images.push_back(image);
			labels.push_back(variant.label.empty() ? view : view + " - " + variant.label);
		}
	}

	if (grid && images.size() > 1) {
		// One image, a row per variant and a column per view, each labeled
		const int columns = static_cast<int>(views.size());
		const int rows = static_cast<int>(variants.size());
		wxImage combined(width * columns, height * rows);
		for (size_t i = 0; i < images.size(); i++) {
			wxImage cell = images[i];
			if (cell.HasAlpha())
				cell.ClearAlpha();
			combined.Paste(cell, static_cast<int>(i % columns) * width, static_cast<int>(i / columns) * height);
		}

		wxBitmap bitmap(combined);
		{
			wxMemoryDC dc(bitmap);
			dc.SetFont(wxFont(wxFontInfo(std::max(10, height / 40))));
			dc.SetTextForeground(*wxWHITE);
			dc.SetTextBackground(wxColour(0, 0, 0));
			dc.SetBackgroundMode(wxBRUSHSTYLE_SOLID);
			for (size_t i = 0; i < labels.size(); i++)
				dc.DrawText(wxString::FromUTF8(" " + labels[i] + " "), static_cast<int>(i % columns) * width + 4, static_cast<int>(i / columns) * height + 4);
		}
		result.images.push_back({EncodePNG(bitmap.ConvertToImage())});
	}
	else {
		for (auto& image : images)
			result.images.push_back({EncodePNG(image)});
	}

	result.data["views"] = views;
	result.data["width"] = width;
	result.data["height"] = height;
	result.data["images"] = labels;
	if (skeleton)
		result.data["skeleton"] = {{"bones", skeleton->bones.size()}, {"colors", "red: her right side, blue: her left side, green: the middle"}};
	if (!facingAway.empty())
		result.data["focusFacesAway"] = {{"views", facingAway}, {"note", "The focus vertices face away from these views, so they are hidden there. Pick a view from the side they face (get_shape_geometry normals)."}};
	if (!otherSliders.empty()) {
		json sample = json::array();
		for (size_t i = 0; i < otherSliders.size() && i < 10; i++)
			sample.push_back(otherSliders[i]);
		result.data["otherSlidersSet"] = {{"count", otherSliders.size()}, {"sliders", sample}, {"note", "These were set before the render and show in it too. zeroOthers: true leaves them out."}};
	}
	return result;
}

McpToolResult ListShapes(OutfitStudioMcp& mcp, const json& args) {
	OutfitStudioFrame* frame = mcp.GetFrame();
	OutfitProject& project = mcp.Project();
	const bool withBones = McpArgs::Bool(args, "withBones", false);

	std::vector<std::string> selected = frame->GetSelectedShapeNames();

	json shapes = json::array();
	for (auto& name : project.GetShapeOrder()) {
		NiShape* shape = project.GetWorkNif()->FindBlockByName<NiShape>(name);
		if (!shape)
			continue;

		json entry = {
			{"name", name},
			{"reference", project.IsBaseShape(shape)},
			{"vertices", shape->GetNumVertices()},
			{"triangles", shape->GetNumTriangles()},
			{"skinned", shape->IsSkinned()},
			{"selected", std::find(selected.begin(), selected.end(), name) != selected.end()},
		};

		// Where the shape is as shown, with the current sliders and pose
		std::vector<Vector3> verts;
		project.GetLiveVerts(shape, verts);
		if (!verts.empty()) {
			Vector3 lo = verts[0], hi = verts[0];
			for (auto& v : verts) {
				lo.x = std::min(lo.x, v.x);
				lo.y = std::min(lo.y, v.y);
				lo.z = std::min(lo.z, v.z);
				hi.x = std::max(hi.x, v.x);
				hi.y = std::max(hi.y, v.y);
				hi.z = std::max(hi.z, v.z);
			}
			entry["bounds"] = {{"min", VectorToJson(lo)}, {"max", VectorToJson(hi)}};
		}

		ShapeDisplayMode mode;
		if (frame->GetShapeDisplayMode(name, mode))
			entry["display"] = ModeName(mode);

		auto& shapeBones = project.GetWorkAnim()->shapeBones;
		auto bones = shapeBones.find(name);
		entry["boneCount"] = bones != shapeBones.end() ? bones->second.size() : 0;
		if (withBones)
			entry["bones"] = bones != shapeBones.end() ? json(bones->second) : json::array();

		std::string refFile, refName;
		if (project.GetShapeReferenceSource(name, refFile, refName))
			entry["source"] = {{"projectFile", refFile}, {"projectName", refName}};

		shapes.push_back(entry);
	}

	McpToolResult result;
	result.data["shapes"] = shapes;
	return result;
}

} // namespace

void AddViewTools(OutfitStudioMcp& mcp) {
	OutfitStudioFrame* frame = mcp.GetFrame();

	McpTool listShapes;
	listShapes.name = "list_shapes";
	listShapes.description =
		"Lists the shapes of the project in order, the reference shape included, with vertex and triangle counts, "
		"bounds (min and max in NIF units, as shown), skinning, bone count, display mode, selection and the project "
		"each shape came from.";
	listShapes.inputSchema = McpSchema({{"withBones", McpProp("boolean", "Also list the bones of each shape. Default false.")}});
	listShapes.readOnly = true;
	listShapes.handler = [&mcp](const json& args) { return ListShapes(mcp, args); };
	mcp.AddTool(std::move(listShapes));

	McpTool select;
	select.name = "select_shapes";
	select.description = "Selects shapes in the window, the first one becomes the active shape. Tools that take shapes use the selected outfit shapes when they are given none.";
	select.inputSchema = McpSchema({{"shapes", McpStringList("Shape names")}}, {"shapes"});
	select.handler = [&mcp, frame](const json& args) {
		std::vector<std::string> names = McpArgs::Strings(args, "shapes");
		for (auto& name : names)
			mcp.Shape(name);
		if (names.empty() || !frame->SelectShapes(names))
			throw McpToolError("The shapes couldn't be selected.");

		McpToolResult result;
		result.data["selectedShapes"] = frame->GetSelectedShapeNames();
		return result;
	};
	mcp.AddTool(std::move(select));

	McpTool display;
	display.name = "set_shape_display";
	display.description = "Shows, hides or ghosts (see-through) shapes in the view.";
	display.inputSchema = McpSchema({{"shapes", ShapeModesSchema}}, {"shapes"});
	display.handler = [&mcp, frame](const json& args) {
		const json& shapes = args.at("shapes");
		if (!shapes.is_object())
			McpArgs::Wrong("shapes", "an object of shape names to display modes");

		for (auto& item : shapes.items()) {
			mcp.Shape(item.key());
			if (!item.value().is_string())
				McpArgs::Wrong("shapes", "an object of shape names to display modes");
			frame->SetShapeDisplayMode(item.key(), ModeFromName(item.value().get<std::string>()));
		}

		frame->glView->Render();
		McpToolResult result;
		result.data["shapes"] = ViewStateToJson(frame->GetViewState())["shapes"];
		return result;
	};
	mcp.AddTool(std::move(display));

	McpTool getView;
	getView.name = "get_view_state";
	getView.description = "Returns the camera, the display toggles, the overlays and the display mode of each shape, to restore later with set_view_state.";
	getView.readOnly = true;
	getView.handler = [frame](const json&) {
		Surface(frame);
		McpToolResult result;
		result.data = ViewStateToJson(frame->GetViewState());
		result.data["camera"]["orbit"] = OrbitToJson(frame->GetViewState().camera);
		result.data["overlays"] = OverlaysToJson(frame);
		return result;
	};
	mcp.AddTool(std::move(getView));

	McpTool setView;
	setView.name = "set_view_state";
	setView.description =
		"Changes the view the user sees: a camera preset, the camera as get_view_state returns it, an orbit around a "
		"bone, shape or point (yaw and pitch in the character's terms), display toggles, overlays (bones, nodes, floor) "
		"and shape display modes. Only what's given changes.";
	setView.inputSchema = McpSchema({
		{"preset", {{"type", "string"}, {"enum", ViewPresets}, {"description", "Turn the camera to look from this side"}}},
		{"camera", {{"type", "object"}, {"description", "Camera fields as get_view_state returns them"}}},
		{"orbit", OrbitSchema},
		{"display", DisplaySchema},
		{"overlays", OverlaysSchema},
		{"shapes", ShapeModesSchema},
	});
	setView.handler = [&mcp, frame](const json& args) {
		GLSurface& surface = Surface(frame);
		ViewState state = frame->GetViewState();

		if (const json* camera = McpArgs::Find(args, "camera"))
			ApplyCamera(state.camera, *camera);
		if (const json* orbit = McpArgs::Find(args, "orbit"))
			ApplyOrbit(mcp, state.camera, *orbit);
		if (const json* display = McpArgs::Find(args, "display"))
			ApplyDisplay(state.display, *display);
		if (const json* shapes = McpArgs::Find(args, "shapes")) {
			if (!shapes->is_object())
				McpArgs::Wrong("shapes", "an object of shape names to display modes");
			for (auto& item : shapes->items()) {
				mcp.Shape(item.key());
				state.shapes[item.key()] = ModeFromName(item.value().get<std::string>());
			}
		}

		frame->SetViewState(state);
		if (const json* overlays = McpArgs::Find(args, "overlays"))
			ApplyOverlays(frame, *overlays);

		if (McpArgs::Has(args, "preset")) {
			std::string preset = McpArgs::String(args, "preset");
			CheckPreset(preset);
			surface.SetViewPreset(preset);
			frame->glView->Render();
		}

		McpToolResult result;
		result.data = ViewStateToJson(frame->GetViewState());
		result.data["camera"]["orbit"] = OrbitToJson(frame->GetViewState().camera);
		result.data["overlays"] = OverlaysToJson(frame);
		return result;
	};
	mcp.AddTool(std::move(setView));

	McpTool toggleDisplay;
	toggleDisplay.name = "toggle_display";
	toggleDisplay.description =
		"Turns display toggles of the view the user sees on or off, like the toolbar buttons: textures (off shows plain "
		"grey shapes, e.g. when textures are missing), wireframe (shows the triangles, for folds and mesh density), "
		"lighting, mask, vertexColors and weightColors. Each named toggle flips, or takes the value given in set. Returns "
		"all toggles after the change. render_view display does the same for one render only.";
	toggleDisplay.inputSchema = McpSchema({
		{"toggle", {{"type", "array"}, {"items", {{"type", "string"}, {"enum", {"textures", "wireframe", "lighting", "mask", "vertexColors", "weightColors"}}}}, {"description", "Toggles to flip"}}},
		{"set", DisplaySchema},
	});
	toggleDisplay.handler = [frame](const json& args) {
		Surface(frame);
		ViewState state = frame->GetViewState();
		GLSurface::DisplaySettings& display = state.display;
		for (auto& name : McpArgs::Strings(args, "toggle")) {
			if (name == "textures")
				display.textures = !display.textures;
			else if (name == "wireframe")
				display.wireframe = !display.wireframe;
			else if (name == "lighting")
				display.lighting = !display.lighting;
			else if (name == "mask")
				display.mask = !display.mask;
			else if (name == "vertexColors")
				display.vertexColors = !display.vertexColors;
			else if (name == "weightColors")
				display.weightColors = !display.weightColors;
			else
				McpArgs::Wrong("toggle", "a list of textures, wireframe, lighting, mask, vertexColors, weightColors");
		}
		if (const json* set = McpArgs::Find(args, "set"))
			ApplyDisplay(display, *set);
		frame->SetViewState(state);

		McpToolResult result;
		result.data = ViewStateToJson(frame->GetViewState())["display"];
		return result;
	};
	mcp.AddTool(std::move(toggleDisplay));

	McpTool frameView;
	frameView.name = "frame_view";
	frameView.description =
		"Points the camera at shapes (all by default), at vertices of a shape, or at the area weighted to a bone, "
		"optionally from a preset side.";
	frameView.inputSchema = McpSchema({
		{"shapes", McpStringList("Shapes to frame")},
		{"shape", McpProp("string", "Shape of the vertices")},
		{"vertices", McpIntList("Vertex indices of the shape to frame")},
		{"bone", McpProp("string", "Frame the vertices weighted to this bone")},
		{"minWeight", McpProp("number", "With bone: lowest weight that counts. Default 0.1.")},
		{"preset", {{"type", "string"}, {"enum", ViewPresets}, {"description", "Look from this side"}}},
	});
	frameView.handler = [&mcp, frame](const json& args) {
		GLSurface& surface = Surface(frame);
		if (McpArgs::Has(args, "preset")) {
			std::string preset = McpArgs::String(args, "preset");
			CheckPreset(preset);
			surface.SetViewPreset(preset);
		}

		FrameTarget(mcp, args);
		frame->glView->Render();

		McpToolResult result;
		result.data["camera"] = ViewStateToJson(frame->GetViewState())["camera"];
		result.data["camera"]["orbit"] = OrbitToJson(frame->GetViewState().camera);
		return result;
	};
	mcp.AddTool(std::move(frameView));

	McpTool render;
	render.name = "render_view";
	render.description =
		"Renders the view to PNG images to look at the outfit, e.g. from several sides, with slider values, "
		"wireframe, the mask or the weights of a bone. focus zooms in on vertices (a problem area), variants renders the "
		"views for several slider values or poses, and grid puts everything into one labeled image to compare. "
		"orbit places the camera around a bone or shape by yaw and pitch, skeleton draws the posed bones over the images. "
		"Everything given only applies to the images, the view the user sees, the pose and the slider values stay as they were.";
	render.inputSchema = McpSchema({
		{"width", McpProp("integer", "Image width in pixels, 64 to 2048. Default 768.")},
		{"height", McpProp("integer", "Image height in pixels, 64 to 2048. Default 768.")},
		{"views",
		 {{"type", "array"},
		  {"items", {{"type", "string"}, {"enum", {"current", "front", "back", "left", "right", "top", "bottom"}}}},
		  {"description", "One image per view: the current camera or a preset side. Default [\"current\"]."}}},
		{"frame", McpStringList("Fit these shapes into each image")},
		{"show", McpStringList("Only show these shapes")},
		{"ghost", McpStringList("Show these shapes see-through")},
		{"display", DisplaySchema},
		{"weightBone", McpProp("string", "Color the shapes by their weights for this bone")},
		{"sliderValues", McpNumberMap("Slider values for the images, 0 to 1 (e.g. {\"Breasts\": 1}), on top of the values set now")},
		{"zeroOthers", McpProp("boolean", "Set all other sliders to 0 for the images, e.g. after apply_preset. Default false; the result lists other sliders that were set (otherSlidersSet).")},
		{"focus",
		 {{"type", "object"},
		  {"properties",
		   {{"shape", McpProp("string", "Shape of the vertices")},
			{"vertices", McpIntList("Vertices to fit into each image, e.g. the deepest of analyze_clipping or the flippedVertices of analyze_slider")},
			{"masked", McpProp("boolean", "Fit the masked vertices, e.g. after mask_ops from_analysis with target protect")},
			{"unmasked", McpProp("boolean", "Fit the unmasked vertices, e.g. after mask_ops from_analysis with target fix")},
			{"highlight", McpProp("boolean", "Color the focus vertices red on a grey shape. Default true when vertices are listed.")}}},
		  {"description", "Zoom each image in on vertices of a shape, after the view preset"}}},
		{"variants",
		 {{"type", "array"},
		  {"items",
		   {{"type", "object"},
			{"properties",
			 {{"label", McpProp("string", "Caption, made from the values by default")},
			  {"sliderValues", McpNumberMap("Slider values on top of the others, 0 to 1")},
			  {"preset", McpProp("string", "A body preset: its values with all other sliders at 0, sliderValues on top")},
			  {"presetWeight", {{"type", "string"}, {"enum", {"low", "high"}}, {"description", "Default high"}}},
			  {"pose", McpProp("string", "A pose of list_poses, or \"none\" for unposed")}}}}},
		  {"description", "Render each view once per variant, e.g. [{\"sliderValues\": {\"Breasts\": 0}}, {\"sliderValues\": {\"Breasts\": 1}}] or [{}, {\"preset\": \"UBE Necoco V1\"}] to compare"}}},
		{"grid", McpProp("boolean", "Put all images into one, a row per variant and a column per view, each labeled. Easier to compare. Default false.")},
		{"orbit", OrbitSchema},
		{"skeleton",
		 {{"anyOf",
		   {{{"type", "boolean"}},
			{{"type", "object"},
			 {"properties",
			  {{"bones", {{"description", "\"main\" (default: spine, neck, head, arms, legs), \"all\", or a list of bone names"}}},
			   {"labels", McpProp("boolean", "Label the joints with their short names (LUar for NPC L UpperArm [LUar]). Default true.")},
			   {"only", McpProp("boolean", "Hide all shapes and draw only the skeleton, like a stick figure. Default false.")}}}}}},
		  {"description", "Draw the skeleton with the pose over the images: lines to the parent, joints as dots, red on her right "
						  "side, blue on her left, green in the middle. Compare it line by line with a reference picture."}}},
	});
	const json presetProperties = McpPresetValueProperties();
	for (auto& property : presetProperties.items())
		render.inputSchema["properties"][property.key()] = property.value();
	render.readOnly = true;
	render.handler = [&mcp](const json& args) { return RenderView(mcp, args); };
	mcp.AddTool(std::move(render));
}
