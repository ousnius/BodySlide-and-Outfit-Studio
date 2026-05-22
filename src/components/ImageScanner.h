/*
BodySlide and Outfit Studio
See the included LICENSE file

Image Scanning Module - Photogrammetry and Depth Map Processing
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>

#include "UniversalModel.h"

namespace univmodel {

// ============== DEPTH ESTIMATION ==============

struct DepthEstimationConfig {
	// Model type: monocular, stereoscopic, or multi-view
	enum class ModelType { Monocular, Stereo, MultiView };
	ModelType modelType = ModelType::Monocular;
	
	// Output settings
	int outputWidth = 512;
	int outputHeight = 512;
	
	// Quality settings
	int downscaleFactor = 2;  // Higher = faster but less accurate
	float confidenceThreshold = 0.5f;
	
	// Depth range (in world units)
	float minDepth = 0.1f;
	float maxDepth = 100.0f;
	
	// Normalization
	bool normalizeOutput = true;
	
	// Post-processing
	bool medianFilter = true;
	bool bilateralFilter = true;
	float bilateralSpatialSigma = 2.0f;
	float bilateralRangeSigma = 0.1f;
	
	// Colorize output
	bool colorizeDepth = false;
};

struct DepthMap {
	std::vector<float> depth;  // Linear depth values
	int width = 0;
	int height = 0;
	
	// Optional confidence map
	std::optional<std::vector<float>> confidence;
	
	// Camera parameters (if available)
	float focalLengthX = 0;
	float focalLengthY = 0;
	float principalPointX = 0;
	float principalPointY = 0;
	
	float minDepthObserved = 0;
	float maxDepthObserved = 0;
	
	float* GetDepthPtr(int x, int y) {
		if (x < 0 || x >= width || y < 0 || y >= height) return nullptr;
		return &depth[y * width + x];
	}
	
	const float* GetDepthPtr(int x, int y) const {
		if (x < 0 || x >= width || y < 0 || y >= height) return nullptr;
		return &depth[y * width + x];
	}
	
	void Allocate(int w, int h) {
		width = w;
		height = h;
		depth.resize(w * h);
	}
};

// ============== IMAGE PROCESSING ==============

struct ImageData {
	std::vector<unsigned char> pixels;  // RGBA
	int width = 0;
	int height = 0;
	int channels = 4;  // Usually 4 for RGBA
	
	enum class Format { RGBA, RGB, Greyscale, BGRA };
	Format format = Format::RGBA;
	
	bool IsValid() const { return width > 0 && height > 0 && !pixels.empty(); }
};

class ImageProcessor {
public:
	// Load image from file
	static std::unique_ptr<ImageData> LoadImage(const std::string& filePath);
	
	// Convert to greyscale
	static std::unique_ptr<ImageData> ConvertToGreyscale(const ImageData& image);
	
	// Resize image
	static std::unique_ptr<ImageData> Resize(const ImageData& image, int targetWidth, int targetHeight);
	
	// Apply bilateral filter for edge-preserving smoothing
	static void BilateralFilter(ImageData& image, float spatialSigma, float rangeSigma);
	
	// Apply median filter
	static void MedianFilter(ImageData& image, int kernelSize = 3);
	
	// Detect edges using Sobel operator
	static std::unique_ptr<ImageData> DetectEdges(const ImageData& image);
	
	// Calculate image gradients (for depth estimation)
	static void CalculateGradients(const ImageData& image, 
								   std::vector<float>& gradX, 
								   std::vector<float>& gradY);
};

// ============== DEPTH ESTIMATOR ==============

class DepthEstimator {
public:
	DepthEstimator() = default;
	explicit DepthEstimator(const DepthEstimationConfig& config) : config(config) {}
	
	// Estimate depth from single image (monocular depth estimation)
	// Uses learned model or geometric heuristics
	std::unique_ptr<DepthMap> EstimateFromSingleImage(const ImageData& image);
	
	// Estimate depth from stereo pair
	std::unique_ptr<DepthMap> EstimateFromStereo(const ImageData& leftImage, 
												 const ImageData& rightImage,
												 const float baseline = 0.1f);
	
	// Estimate depth from multiple images (structure from motion style)
	std::unique_ptr<DepthMap> EstimateFromMultipleImages(
		const std::vector<ImageData>& images,
		const std::vector<float>& cameraPoses);  // Camera positions as 4x4 matrices
	
	// Post-process depth map
	void PostProcess(DepthMap& depthMap);
	
	// Set configuration
	void SetConfig(const DepthEstimationConfig& config) { this->config = config; }
	const DepthEstimationConfig& GetConfig() const { return config; }
	
	// Set progress callback
	void SetProgressCallback(std::function<void(float, const std::string&)> callback) {
		progressCallback = std::move(callback);
	}

private:
	void ReportProgress(float progress, const std::string& message);
	DepthEstimationConfig config;
	std::function<void(float, const std::string&)> progressCallback;
	
	// Internal methods for different estimation strategies
	std::unique_ptr<DepthMap> MonocularEstimation(const ImageData& image);
	std::unique_ptr<DepthMap> StereoMatching(const ImageData& leftImage, 
											 const ImageData& rightImage,
											 float baseline);
	
	// Edge-aware upsampling
	void EdgeAwareUpsample(const DepthMap& input, DepthMap& output, int targetWidth, int targetHeight);
	
	// Confidence estimation
	std::vector<float> EstimateConfidence(const DepthMap& depthMap);
};

// ============== MESH RECONSTRUCTION ==============

struct MeshReconstructionConfig {
	// Input settings
	bool useColorImages = true;
	bool generateUVs = true;
	
	// Mesh generation settings
	float depthThreshold = 0.01f;  // Depth difference threshold for triangulation
	float normalThreshold = 0.8f;  // Cosine threshold for smooth normals
	int minTriangles = 100;
	
	// Decimation options
	bool decimateMesh = false;
	int targetTriangleCount = 10000;
	float decimationErrorThreshold = 0.01f;
	
	// Mesh optimization
	bool removeDuplicates = true;
	bool removeDegenerates = true;
	bool weldVertices = true;
	float weldTolerance = 0.001f;
	
	// Texture generation
	bool generateTextures = true;
	int textureWidth = 2048;
	int textureHeight = 2048;
	
	// Post-processing
	bool smoothMesh = false;
	int smoothIterations = 2;
	float smoothStrength = 0.5f;
	
	// Fill holes
	bool fillHoles = false;
	int maxHoleSize = 100;  // Max vertices to fill
};

class MeshReconstructor {
public:
	MeshReconstructor() = default;
	explicit MeshReconstructor(const MeshReconstructionConfig& config) : config(config) {}
	
	// Reconstruct mesh from depth map and color image
	// Returns a UniversalMesh ready for further processing
	std::unique_ptr<UniversalMesh> ReconstructFromDepthAndColor(
		const DepthMap& depthMap,
		const ImageData& colorImage,
		const std::string& meshName = u8"ScannedMesh");
	
	// Reconstruct mesh from multiple depth maps (multi-view)
	std::unique_ptr<UniversalMesh> ReconstructFromMultipleViews(
		const std::vector<DepthMap>& depthMaps,
		const std::vector<ImageData>& colorImages,
		const std::vector<float>& cameraPoses,
		const std::string& meshName = u8"MultiViewMesh");
	
	// Fuse multiple depth maps into single point cloud
	std::vector<Vertex> FuseDepthMaps(
		const std::vector<DepthMap>& depthMaps,
		const std::vector<float>& cameraPoses,
		const std::vector<ImageData>* colorImages = nullptr);
	
	// Convert point cloud to mesh using Poisson reconstruction or similar
	std::unique_ptr<UniversalMesh> PointCloudToMesh(
		const std::vector<Vertex>& pointCloud,
		int octreeDepth = 8,
		float interpolationWeight = 1.0f);
	
	// Set configuration
	void SetConfig(const MeshReconstructionConfig& config) { this->config = config; }
	const MeshReconstructionConfig& GetConfig() const { return config; }
	
	// Set progress callback
	void SetProgressCallback(std::function<void(float, const std::string&)> callback) {
		progressCallback = std::move(callback);
	}

private:
	void ReportProgress(float progress, const std::string& message);
	MeshReconstructionConfig config;
	std::function<void(float, const std::string&)> progressCallback;
	
	// Triangulate depth map to mesh
	std::unique_ptr<UniversalMesh> TriangulateDepthMap(
		const DepthMap& depthMap,
		const ImageData* colorImage);
	
	// Merge multiple meshes
	std::unique_ptr<UniversalMesh> MergeMeshes(const std::vector<std::unique_ptr<UniversalMesh>>& meshes);
	
	// Optimize mesh (decimation, welding, etc.)
	void OptimizeMesh(UniversalMesh& mesh);
	
	// Generate UV coordinates using planar or spherical projection
	void GenerateUVs(UniversalMesh& mesh);
	
	// Generate textures from color images
	void GenerateTextures(UniversalMesh& mesh, const ImageData& colorImage);
	
	// Remove degenerate triangles
	void RemoveDegenerates(UniversalMesh& mesh);
	
	// Fill holes in mesh
	void FillHoles(UniversalMesh& mesh);
};

// ============== SCANNER PIPELINE ==============

struct ScanProject {
	std::string name;
	std::string outputDirectory;
	
	// Input images
	std::vector<std::string> imagePaths;
	std::vector<std::string> cameraPoses;  // Optional: camera pose files
	
	// Processing state
	enum class State { Ready, Capturing, Processing, Completed, Error };
	State state = State::Ready;
	
	// Results
	std::unique_ptr<UniversalMesh> reconstructedMesh;
	std::string errorMessage;
};

class ImageScanner {
public:
	ImageScanner() = default;
	
	// Create new scan project
	std::unique_ptr<ScanProject> CreateProject(const std::string& name, 
												const std::string& outputDirectory);
	
	// Add images to project
	bool AddImages(ScanProject& project, const std::vector<std::string>& imagePaths);
	
	// Set camera poses (from external source like photogrammetry software)
	bool SetCameraPoses(ScanProject& project, const std::vector<std::string>& poseFiles);
	
	// Run full reconstruction pipeline
	bool Reconstruct(ScanProject& project, 
					 const DepthEstimationConfig& depthConfig = DepthEstimationConfig(),
					 const MeshReconstructionConfig& meshConfig = MeshReconstructionConfig());
	
	// Individual pipeline steps
	bool EstimateDepths(ScanProject& project, const DepthEstimationConfig& config);
	bool ReconstructMesh(ScanProject& project, const MeshReconstructionConfig& config);
	
	// Save results
	bool SaveMesh(const ScanProject& project, const std::string& filePath, FormatType format);
	
	// Get/set configuration
	void SetDepthConfig(const DepthEstimationConfig& config) { depthConfig = config; }
	const DepthEstimationConfig& GetDepthConfig() const { return depthConfig; }
	
	void SetMeshConfig(const MeshReconstructionConfig& config) { meshConfig = config; }
	const MeshReconstructionConfig& GetMeshConfig() const { return meshConfig; }
	
	// Progress reporting
	void SetProgressCallback(std::function<void(float, const std::string&)> callback) {
		progressCallback = std::move(callback);
	}

private:
	DepthEstimationConfig depthConfig;
	MeshReconstructionConfig meshConfig;
	std::function<void(float, const std::string&)> progressCallback;
	
	// Internal pipeline state
	DepthEstimator depthEstimator;
	MeshReconstructor meshReconstructor;
	std::vector<std::unique_ptr<DepthMap>> estimatedDepths;
	
	// Load and preprocess images
	std::vector<std::unique_ptr<ImageData>> LoadImages(const std::vector<std::string>& paths);
	
	// Load camera poses if available
	std::vector<float> LoadCameraPoses(const std::vector<std::string>& poseFiles);
	
	// Parse camera pose from JSON string
	static std::vector<float> ParseCameraPoseJSON(const std::string& json);
	
	// Report progress
	void ReportProgress(float progress, const std::string& message);
};

} // namespace univmodel