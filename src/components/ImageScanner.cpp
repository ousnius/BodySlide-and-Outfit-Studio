/*
BodySlide and Outfit Studio
See the included LICENSE file

ImageScanner Implementation
*/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <filesystem>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SOIL2/SOIL2.h>

#include "ImageScanner.h"
#include "../components/UniversalModel.h"

#include <nlohmann/json.hpp>

using namespace univmodel;

// ============== IMAGE PROCESSOR IMPLEMENTATION ==============

std::unique_ptr<ImageData> ImageProcessor::LoadImage(const std::string& filePath) {
	auto image = std::make_unique<ImageData>();
	
	int width, height, channels;
	unsigned char* pixels = SOIL_load_image(filePath.c_str(), &width, &height, &channels, SOIL_LOAD_RGBA);
	
	if (!pixels) {
		return nullptr;
	}
	
	image->width = width;
	image->height = height;
	image->channels = 4;
	image->format = ImageData::Format::RGBA;
	image->pixels.resize(width * height * 4);
	std::memcpy(image->pixels.data(), pixels, image->pixels.size());
	
	SOIL_free_image_data(pixels);
	return image;
}

std::unique_ptr<ImageData> ImageProcessor::ConvertToGreyscale(const ImageData& image) {
	auto grey = std::make_unique<ImageData>();
	grey->width = image.width;
	grey->height = image.height;
	grey->channels = 1;
	grey->format = ImageData::Format::Greyscale;
	grey->pixels.resize(image.width * image.height);
	
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			size_t srcIdx = (y * image.width + x) * image.channels;
			size_t dstIdx = y * image.width + x;
			
			// Luminance formula
			float r = image.pixels[srcIdx];
			float g = image.pixels[srcIdx + 1];
			float b = image.pixels[srcIdx + 2];
			grey->pixels[dstIdx] = static_cast<unsigned char>(0.299f * r + 0.587f * g + 0.114f * b);
		}
	}
	
	return grey;
}

std::unique_ptr<ImageData> ImageProcessor::Resize(const ImageData& image, int targetWidth, int targetHeight) {
	auto resized = std::make_unique<ImageData>();
	resized->width = targetWidth;
	resized->height = targetHeight;
	resized->channels = image.channels;
	resized->format = image.format;
	resized->pixels.resize(targetWidth * targetHeight * image.channels);
	
	// Simple bilinear interpolation
	float scaleX = static_cast<float>(image.width) / targetWidth;
	float scaleY = static_cast<float>(image.height) / targetHeight;
	
	for (int y = 0; y < targetHeight; ++y) {
		for (int x = 0; x < targetWidth; ++x) {
			float srcX = x * scaleX;
			float srcY = y * scaleY;
			
			int x0 = static_cast<int>(srcX);
			int y0 = static_cast<int>(srcY);
			int x1 = std::min(x0 + 1, image.width - 1);
			int y1 = std::min(y0 + 1, image.height - 1);
			
			float fx = srcX - x0;
			float fy = srcY - y0;
			
			for (int c = 0; c < image.channels; ++c) {
				float v00 = image.pixels[(y0 * image.width + x0) * image.channels + c];
				float v10 = image.pixels[(y0 * image.width + x1) * image.channels + c];
				float v01 = image.pixels[(y1 * image.width + x0) * image.channels + c];
				float v11 = image.pixels[(y1 * image.width + x1) * image.channels + c];
				
				float value = v00 * (1 - fx) * (1 - fy) +
							  v10 * fx * (1 - fy) +
							  v01 * (1 - fx) * fy +
							  v11 * fx * fy;
				
				resized->pixels[(y * targetWidth + x) * image.channels + c] = 
					static_cast<unsigned char>(std::clamp(value, 0.0f, 255.0f));
			}
		}
	}
	
	return resized;
}

void ImageProcessor::BilateralFilter(ImageData& image, float spatialSigma, float rangeSigma) {
	// Edge-preserving bilateral filter
	int size = static_cast<int>(std::ceil(spatialSigma * 3.0f));
	size = std::max(size, 1);
	
	auto input = image.pixels;
	image.pixels.resize(image.width * image.height * image.channels);
	
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			float weightSum = 0.0f;
			float colorSum[4] = {0, 0, 0, 0};
			
			for (int ky = -size; ky <= size; ++ky) {
				for (int kx = -size; kx <= size; ++kx) {
					int ny = std::clamp(y + ky, 0, image.height - 1);
					int nx = std::clamp(x + kx, 0, image.width - 1);
					
					float spatialDist = std::sqrt(static_cast<float>(kx * kx + ky * ky));
					float spatialWeight = std::exp(-spatialDist * spatialDist / (2 * spatialSigma * spatialSigma));
					
					size_t srcIdx = (y * image.width + x) * image.channels;
					size_t nbrIdx = (ny * image.width + nx) * image.channels;
					
					float rangeDist = 0.0f;
					for (int c = 0; c < image.channels; ++c) {
						float d = static_cast<float>(input[srcIdx + c]) - static_cast<float>(input[nbrIdx + c]);
						rangeDist += d * d;
					}
					rangeDist = std::sqrt(rangeDist);
					float rangeWeight = std::exp(-rangeDist * rangeDist / (2 * rangeSigma * rangeSigma));
					
					float weight = spatialWeight * rangeWeight;
					weightSum += weight;
					
					for (int c = 0; c < image.channels; ++c) {
						colorSum[c] += weight * static_cast<float>(input[nbrIdx + c]);
					}
				}
			}
			
			size_t dstIdx = (y * image.width + x) * image.channels;
			for (int c = 0; c < image.channels; ++c) {
				image.pixels[dstIdx + c] = static_cast<unsigned char>(
					std::clamp(colorSum[c] / weightSum, 0.0f, 255.0f));
			}
		}
	}
}

void ImageProcessor::MedianFilter(ImageData& image, int kernelSize) {
	int halfSize = kernelSize / 2;
	auto input = image.pixels;
	image.pixels.resize(image.width * image.height * image.channels);
	
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			std::vector<unsigned char> window;
			window.reserve(kernelSize * kernelSize * image.channels);
			
			for (int ky = -halfSize; ky <= halfSize; ++ky) {
				for (int kx = -halfSize; kx <= halfSize; ++kx) {
					int ny = std::clamp(y + ky, 0, image.height - 1);
					int nx = std::clamp(x + kx, 0, image.width - 1);
					size_t idx = (ny * image.width + nx) * image.channels;
					for (int c = 0; c < image.channels; ++c) {
						window.push_back(input[idx + c]);
					}
				}
			}
			
			std::sort(window.begin(), window.end());
			size_t medianIdx = window.size() / 2;
			
			size_t dstIdx = (y * image.width + x) * image.channels;
			for (int c = 0; c < image.channels; ++c) {
				image.pixels[dstIdx + c] = window[medianIdx + c];
			}
		}
	}
}

std::unique_ptr<ImageData> ImageProcessor::DetectEdges(const ImageData& image) {
	auto edges = std::make_unique<ImageData>();
	edges->width = image.width;
	edges->height = image.height;
	edges->channels = 1;
	edges->format = ImageData::Format::Greyscale;
	edges->pixels.resize(image.width * image.height);
	
	// Sobel operator
	std::vector<unsigned char> grey;
	if (image.channels > 1) {
		auto greyImg = ConvertToGreyscale(image);
		grey = std::move(greyImg->pixels);
	} else {
		grey = image.pixels;
	}
	
	for (int y = 1; y < image.height - 1; ++y) {
		for (int x = 1; x < image.width - 1; ++x) {
			float gx = -grey[(y-1)*image.width+(x-1)] + grey[(y-1)*image.width+(x+1)]
					  -2*grey[y*image.width+(x-1)] + 2*grey[y*image.width+(x+1)]
					  -grey[(y+1)*image.width+(x-1)] + grey[(y+1)*image.width+(x+1)];
			
			float gy = -grey[(y-1)*image.width+(x-1)] - 2*grey[(y-1)*image.width+x] - grey[(y-1)*image.width+(x+1)]
					  +grey[(y+1)*image.width+(x-1)] + 2*grey[(y+1)*image.width+x] + grey[(y+1)*image.width+(x+1)];
			
			float magnitude = std::sqrt(gx*gx + gy*gy);
			edges->pixels[y * image.width + x] = static_cast<unsigned char>(std::clamp(magnitude, 0.0f, 255.0f));
		}
	}
	
	return edges;
}

void ImageProcessor::CalculateGradients(const ImageData& image, 
										std::vector<float>& gradX, 
										std::vector<float>& gradY) {
	std::vector<unsigned char> grey;
	if (image.channels > 1) {
		auto greyImg = ConvertToGreyscale(image);
		grey = std::move(greyImg->pixels);
	} else {
		grey = image.pixels;
	}
	
	gradX.resize(image.width * image.height);
	gradY.resize(image.width * image.height);
	
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			float left = grey[y * image.width + std::max(x - 1, 0)];
			float right = grey[y * image.width + std::min(x + 1, image.width - 1)];
			float top = grey[std::max(y - 1, 0) * image.width + x];
			float bottom = grey[std::min(y + 1, image.height - 1) * image.width + x];
			
			gradX[y * image.width + x] = (right - left) * 0.5f;
			gradY[y * image.width + x] = (bottom - top) * 0.5f;
		}
	}
}

// ============== DEPTH ESTIMATOR IMPLEMENTATION ==============

std::unique_ptr<DepthMap> DepthEstimator::EstimateFromSingleImage(const ImageData& image) {
	ReportProgress(0.1f, "Estimating depth (monocular)...");
	auto result = MonocularEstimation(image);
	ReportProgress(0.5f, "Post-processing depth map...");
	if (config.medianFilter || config.bilateralFilter) {
		PostProcess(*result);
	}
	ReportProgress(1.0f, "Depth estimation complete");
	return result;
}

std::unique_ptr<DepthMap> DepthEstimator::EstimateFromStereo(const ImageData& leftImage, 
															 const ImageData& rightImage,
															 const float baseline) {
	ReportProgress(0.1f, "Estimating depth (stereo)...");
	auto result = StereoMatching(leftImage, rightImage, baseline);
	ReportProgress(0.5f, "Post-processing depth map...");
	if (config.medianFilter || config.bilateralFilter) {
		PostProcess(*result);
	}
	ReportProgress(1.0f, "Depth estimation complete");
	return result;
}

std::unique_ptr<DepthMap> DepthEstimator::EstimateFromMultipleImages(
	const std::vector<ImageData>& images,
	const std::vector<float>& cameraPoses) {
	ReportProgress(0.1f, "Estimating depth (multi-view)...");
	// Multi-view depth fusion would go here
	// For now, fall back to single-view for each image
	std::unique_ptr<DepthMap> result;
	for (size_t i = 0; i < images.size(); ++i) {
		auto single = MonocularEstimation(images[i]);
		if (i == 0) {
			result = std::move(single);
		}
		// Fuse additional views
		ReportProgress(0.1f + 0.8f * static_cast<float>(i) / images.size(), 
					   std::string("Processing view ") + std::to_string(i + 1) + "/" + std::to_string(images.size()));
	}
	ReportProgress(1.0f, "Depth estimation complete");
	return result;
}

void DepthEstimator::ReportProgress(float progress, const std::string& message) {
	if (progressCallback) {
		progressCallback(progress, message);
	}
}

void DepthEstimator::PostProcess(DepthMap& depthMap) {
	// Run median filter if enabled
	if (config.medianFilter) {
		int size = 3;
		std::vector<float> input = depthMap.depth;
		depthMap.depth.resize(depthMap.width * depthMap.height);
		
		for (int y = 0; y < depthMap.height; ++y) {
			for (int x = 0; x < depthMap.width; ++x) {
				std::vector<float> window;
				for (int ky = -size/2; ky <= size/2; ++ky) {
					for (int kx = -size/2; kx <= size/2; ++kx) {
						int ny = std::clamp(y + ky, 0, depthMap.height - 1);
						int nx = std::clamp(x + kx, 0, depthMap.width - 1);
						window.push_back(input[ny * depthMap.width + nx]);
					}
				}
				std::sort(window.begin(), window.end());
				depthMap.depth[y * depthMap.width + x] = window[window.size() / 2];
			}
		}
	}
	
	// Run bilateral filter if enabled
	if (config.bilateralFilter) {
		auto input = depthMap.depth;
		depthMap.depth.resize(depthMap.width * depthMap.height);
		
		int size = static_cast<int>(std::ceil(config.bilateralSpatialSigma * 3.0f));
		size = std::max(size, 1);
		
		for (int y = 0; y < depthMap.height; ++y) {
			for (int x = 0; x < depthMap.width; ++x) {
				float weightSum = 0.0f;
				float depthSum = 0.0f;
				
				for (int ky = -size; ky <= size; ++ky) {
					for (int kx = -size; kx <= size; ++kx) {
						int ny = std::clamp(y + ky, 0, depthMap.height - 1);
						int nx = std::clamp(x + kx, 0, depthMap.width - 1);
						
						float spatialDist = std::sqrt(static_cast<float>(kx*kx + ky*ky));
						float spatialWeight = std::exp(-spatialDist * spatialDist / 
													   (2 * config.bilateralSpatialSigma * config.bilateralSpatialSigma));
						
						float depthDist = input[y * depthMap.width + x] - input[ny * depthMap.width + nx];
						float rangeWeight = std::exp(-depthDist * depthDist / 
													(2 * config.bilateralRangeSigma * config.bilateralRangeSigma));
						
						float weight = spatialWeight * rangeWeight;
						weightSum += weight;
						depthSum += weight * input[ny * depthMap.width + nx];
					}
				}
				
				depthMap.depth[y * depthMap.width + x] = depthSum / weightSum;
			}
		}
	}
	
	// Update depth range
	depthMap.minDepthObserved = *std::min_element(depthMap.depth.begin(), depthMap.depth.end());
	depthMap.maxDepthObserved = *std::max_element(depthMap.depth.begin(), depthMap.depth.end());
}

std::unique_ptr<DepthMap> DepthEstimator::MonocularEstimation(const ImageData& image) {
	auto depth = std::make_unique<DepthMap>();
	depth->Allocate(image.width, image.height);
	
	// Calculate gradients
	std::vector<float> gradX, gradY;
	ImageProcessor::CalculateGradients(image, gradX, gradY);
	
	// Simple depth estimation from image gradients
	// This is a fallback - real monocular depth would use a learned model
	// For now, we use edge strength as a proxy for depth discontinuity
	
	for (int y = 0; y < image.height; ++y) {
		for (int x = 0; x < image.width; ++x) {
			size_t idx = y * image.width + x;
			
			// Gradient magnitude (edges are likely at object boundaries)
			float edgeStrength = std::sqrt(gradX[idx] * gradX[idx] + gradY[idx] * gradY[idx]);
			
			// Simple heuristic: smooth areas = likely background (farther)
			// Edges = likely object boundaries
			float normalizedEdge = edgeStrength / 255.0f;
			
			// Inverse depth: edges are closer, smooth areas are farther
			float estimatedDepth = config.minDepth + (config.maxDepth - config.minDepth) * (1.0f - normalizedEdge * 0.5f);
			
			// Add some variation based on intensity
			if (image.channels > 0) {
				float intensity = image.pixels[idx * image.channels] / 255.0f;
				estimatedDepth *= (0.9f + 0.2f * intensity);
			}
			
			depth->depth[idx] = estimatedDepth;
		}
	}
	
	// Normalize if requested
	if (config.normalizeOutput) {
		float minD = *std::min_element(depth->depth.begin(), depth->depth.end());
		float maxD = *std::max_element(depth->depth.begin(), depth->depth.end());
		float range = maxD - minD;
		if (range > 0.0001f) {
			for (auto& d : depth->depth) {
				d = config.minDepth + (d - minD) / range * (config.maxDepth - config.minDepth);
			}
		}
	}
	
	depth->minDepthObserved = *std::min_element(depth->depth.begin(), depth->depth.end());
	depth->maxDepthObserved = *std::max_element(depth->depth.begin(), depth->depth.end());
	
	return depth;
}

std::unique_ptr<DepthMap> DepthEstimator::StereoMatching(const ImageData& leftImage, 
														 const ImageData& rightImage,
														 float baseline) {
	auto depth = std::make_unique<DepthMap>();
	depth->Allocate(leftImage.width, leftImage.height);
	
	int width = leftImage.width;
	int height = leftImage.height;
	
	// Convert to greyscale for matching
	auto leftGrey = ImageProcessor::ConvertToGreyscale(leftImage);
	auto rightGrey = ImageProcessor::ConvertToGreyscale(rightImage);
	
	// Simple block matching for stereo depth estimation
	int blockSize = 7;
	int maxDisparity = 64;
	
	for (int y = blockSize; y < height - blockSize; ++y) {
		for (int x = blockSize; x < width - blockSize; ++x) {
			float bestCost = std::numeric_limits<float>::max();
			int bestDisparity = 0;
			
			for (int d = 0; d < maxDisparity; ++d) {
				if (x - d < blockSize) continue;
				
				float cost = 0.0f;
				int count = 0;
				
				for (int ky = -blockSize/2; ky <= blockSize/2; ++ky) {
					for (int kx = -blockSize/2; kx <= blockSize/2; ++kx) {
						int ly = y + ky;
						int lx = x + kx;
						int rx = lx - d;
						
						float lv = leftGrey->pixels[ly * width + lx] / 255.0f;
						float rv = rightGrey->pixels[ly * width + rx] / 255.0f;
						
						cost += std::abs(lv - rv);
						count++;
					}
				}
				
				cost /= count;
				
				if (cost < bestCost) {
					bestCost = cost;
					bestDisparity = d;
				}
			}
			
			// Convert disparity to depth
			if (bestDisparity > 0) {
				float depthValue = baseline * depth->focalLengthX / bestDisparity;
				depth->depth[y * width + x] = std::clamp(depthValue, config.minDepth, config.maxDepth);
			} else {
				depth->depth[y * width + x] = config.maxDepth;
			}
		}
	}
	
	depth->minDepthObserved = *std::min_element(depth->depth.begin(), depth->depth.end());
	depth->maxDepthObserved = *std::max_element(depth->depth.begin(), depth->depth.end());
	
	return depth;
}

void DepthEstimator::EdgeAwareUpsample(const DepthMap& input, DepthMap& output, 
										int targetWidth, int targetHeight) {
	output.Allocate(targetWidth, targetHeight);
	
	// Calculate scale factors
	float scaleX = static_cast<float>(input.width) / targetWidth;
	float scaleY = static_cast<float>(input.height) / targetHeight;
	
	// Edge detection threshold - depth differences above this are considered edges
	float edgeThreshold = 0.1f;
	
	// First pass: compute edge map from input depth
	std::vector<float> inputEdges(input.width * input.height, 0.0f);
	for (int y = 1; y < input.height - 1; ++y) {
		for (int x = 1; x < input.width - 1; ++x) {
			float center = input.depth[y * input.width + x];
			float dx = std::abs(input.depth[y * input.width + x + 1] - center) +
					   std::abs(input.depth[y * input.width + x - 1] - center);
			float dy = std::abs(input.depth[(y + 1) * input.width + x] - center) +
					   std::abs(input.depth[(y - 1) * input.width + x] - center);
			
			inputEdges[y * input.width + x] = std::sqrt(dx * dx + dy * dy);
		}
	}
	
	// Bilinear interpolation with edge-aware guidance
	for (int ty = 0; ty < targetHeight; ++ty) {
		for (int tx = 0; tx < targetWidth; ++tx) {
			// Map to input coordinates with bounds clamping
			float srcX = std::min(static_cast<float>(tx) * scaleX, static_cast<float>(input.width - 1));
			float srcY = std::min(static_cast<float>(ty) * scaleY, static_cast<float>(input.height - 1));
			
			int x0 = static_cast<int>(srcX);
			int y0 = static_cast<int>(srcY);
			int x1 = std::min(x0 + 1, input.width - 1);
			int y1 = std::min(y0 + 1, input.height - 1);
			
			float fx = srcX - x0;
			float fy = srcY - y0;
			
			// Get depth values at four corners
			float d00 = input.depth[y0 * input.width + x0];
			float d10 = input.depth[y0 * input.width + x1];
			float d01 = input.depth[y1 * input.width + x0];
			float d11 = input.depth[y1 * input.width + x1];
			
		// Check if there's a significant edge at this location
		float edgeStrength = inputEdges[y0 * input.width + x0];
		
		// Edge threshold scaled by local depth (add epsilon to avoid very small thresholds)
		float localEdgeThreshold = edgeThreshold * (d00 + 0.01f);
		
		if (edgeStrength > localEdgeThreshold) {
				// Near an edge - use nearest neighbor to preserve sharp transitions
				// Find the corner with minimum edge strength (most reliable)
				float e00 = inputEdges[y0 * input.width + x0];
				float e10 = inputEdges[y0 * input.width + x1];
				float e01 = inputEdges[y1 * input.width + x0];
				float e11 = inputEdges[y1 * input.width + x1];
				
				float minEdge = e00;
				float result = d00;
				
				if (e10 < minEdge) { minEdge = e10; result = d10; }
				if (e01 < minEdge) { minEdge = e01; result = d01; }
				if (e11 < minEdge) { minEdge = e11; result = d11; }
				
				output.depth[ty * targetWidth + tx] = result;
			} else {
				// Smooth area - use bilinear interpolation
				float value = d00 * (1 - fx) * (1 - fy) +
							  d10 * fx * (1 - fy) +
							  d01 * (1 - fx) * fy +
							  d11 * fx * fy;
				output.depth[ty * targetWidth + tx] = value;
			}
		}
	}
	
	output.minDepthObserved = *std::min_element(output.depth.begin(), output.depth.end());
	output.maxDepthObserved = *std::max_element(output.depth.begin(), output.depth.end());
	output.focalLengthX = input.focalLengthX;
	output.focalLengthY = input.focalLengthY;
	output.principalPointX = input.principalPointX;
	output.principalPointY = input.principalPointY;
	
	// Upsample confidence map if available
	if (input.confidence.has_value()) {
		auto& inputConf = input.confidence.value();
		std::vector<float> outputConf(targetWidth * targetHeight);
		
		for (int ty = 0; ty < targetHeight; ++ty) {
			for (int tx = 0; tx < targetWidth; ++tx) {
				float srcX = tx * scaleX;
				float srcY = ty * scaleY;
				
				int x0 = static_cast<int>(srcX);
				int y0 = static_cast<int>(srcY);
				int x1 = std::min(x0 + 1, input.width - 1);
				int y1 = std::min(y0 + 1, input.height - 1);
				
				float fx = srcX - x0;
				float fy = srcY - y0;
				
				float c00 = inputConf[y0 * input.width + x0];
				float c10 = inputConf[y0 * input.width + x1];
				float c01 = inputConf[y1 * input.width + x0];
				float c11 = inputConf[y1 * input.width + x1];
				
				float conf = c00 * (1 - fx) * (1 - fy) +
						   c10 * fx * (1 - fy) +
						   c01 * (1 - fx) * fy +
						   c11 * fx * fy;
				
				outputConf[ty * targetWidth + tx] = conf;
			}
		}
		
		output.confidence = std::move(outputConf);
	}
}

// ============== MESH RECONSTRUCTOR IMPLEMENTATION ==============

std::unique_ptr<UniversalMesh> MeshReconstructor::ReconstructFromDepthAndColor(
	const DepthMap& depthMap,
	const ImageData& colorImage,
	const std::string& meshName) {
	ReportProgress(0.1f, "Triangulating depth map...");
	auto mesh = TriangulateDepthMap(depthMap, &colorImage);
	
	ReportProgress(0.5f, "Optimizing mesh...");
	OptimizeMesh(*mesh);
	
	ReportProgress(0.8f, "Generating UV coordinates...");
	if (config.generateUVs) {
		GenerateUVs(*mesh);
	}
	
	if (config.generateTextures && colorImage.IsValid()) {
		ReportProgress(0.9f, "Generating textures...");
		GenerateTextures(*mesh, colorImage);
	}
	
	ReportProgress(1.0f, "Mesh reconstruction complete");
	mesh->name = meshName;
	return mesh;
}

std::unique_ptr<UniversalMesh> MeshReconstructor::ReconstructFromMultipleViews(
	const std::vector<DepthMap>& depthMaps,
	const std::vector<ImageData>& colorImages,
	const std::vector<float>& cameraPoses,
	const std::string& meshName) {
	ReportProgress(0.1f, "Fusing depth maps...");
	std::vector<Vertex> pointCloud = FuseDepthMaps(depthMaps, cameraPoses, 
												   colorImages.empty() ? nullptr : &colorImages);
	
	ReportProgress(0.5f, "Reconstructing from point cloud...");
	auto mesh = PointCloudToMesh(pointCloud);
	
	ReportProgress(0.8f, "Optimizing mesh...");
	OptimizeMesh(*mesh);
	
	ReportProgress(1.0f, "Multi-view reconstruction complete");
	mesh->name = meshName;
	return mesh;
}

std::vector<Vertex> MeshReconstructor::FuseDepthMaps(
	const std::vector<DepthMap>& depthMaps,
	const std::vector<float>& cameraPoses,
	const std::vector<ImageData>* colorImages) {
	std::vector<Vertex> pointCloud;
	
	for (size_t viewIdx = 0; viewIdx < depthMaps.size(); ++viewIdx) {
		const auto& depth = depthMaps[viewIdx];
		const float* pose = cameraPoses.empty() ? nullptr : &cameraPoses[viewIdx * 16];
		
		const ImageData* color = nullptr;
		if (colorImages && viewIdx < colorImages->size()) {
			color = &(*colorImages)[viewIdx];
		}
		
		for (int y = 0; y < depth.height; ++y) {
			for (int x = 0; x < depth.width; ++x) {
				const float* d = depth.GetDepthPtr(x, y);
				if (!d || *d <= 0) continue;
				
				Vertex v;
				
				// Back-project to 3D
				float fx = depth.focalLengthX > 0 ? depth.focalLengthX : 1.0f;
				float fy = depth.focalLengthY > 0 ? depth.focalLengthY : 1.0f;
				float cx = depth.principalPointX > 0 ? depth.principalPointX : depth.width / 2.0f;
				float cy = depth.principalPointY > 0 ? depth.principalPointY : depth.height / 2.0f;
				
				float rayX = (x - cx) / fx * (*d);
				float rayY = (y - cy) / fy * (*d);
				float rayZ = *d;
				
				// Apply camera transform if available
				if (pose) {
					glm::mat4 camMat(
						pose[0], pose[4], pose[8], pose[12],
						pose[1], pose[5], pose[9], pose[13],
						pose[2], pose[6], pose[10], pose[14],
						pose[3], pose[7], pose[11], pose[15]
					);
					glm::vec4 pt(rayX, rayY, rayZ, 1.0f);
					auto result = camMat * pt;
					v.x = result.x;
					v.y = result.y;
					v.z = result.z;
				} else {
					v.x = rayX;
					v.y = rayY;
					v.z = rayZ;
				}
				
				// Copy color if available
				if (color && y < color->height && x < color->width) {
					size_t idx = (y * color->width + x) * color->channels;
					v.r = color->pixels[idx] / 255.0f;
					v.g = color->pixels[idx + 1] / 255.0f;
					v.b = color->pixels[idx + 2] / 255.0f;
					v.a = color->channels > 3 ? color->pixels[idx + 3] / 255.0f : 1.0f;
				} else {
					v.r = v.g = v.b = 1.0f;
					v.a = 1.0f;
				}
				
				v.nx = 0;
				v.ny = 1;
				v.nz = 0;
				v.u = static_cast<float>(x) / depth.width;
				v.v = static_cast<float>(y) / depth.height;
				v.id = static_cast<uint32_t>(pointCloud.size());
				
				pointCloud.push_back(v);
			}
		}
		
		ReportProgress(0.3f + 0.6f * static_cast<float>(viewIdx) / depthMaps.size(), 
					   std::string("Fusing view ") + std::to_string(viewIdx + 1) + "/" + std::to_string(depthMaps.size()));
	}
	
	return pointCloud;
}

std::unique_ptr<UniversalMesh> MeshReconstructor::PointCloudToMesh(
	const std::vector<Vertex>& pointCloud,
	int octreeDepth,
	float interpolationWeight) {
	// Simple implementation - creates mesh from organized point cloud
	// A full implementation would use Poisson surface reconstruction
	
	auto mesh = std::make_unique<UniversalMesh>();
	
	if (pointCloud.empty()) {
		return mesh;
	}
	
	// For organized point clouds (from depth maps), we can triangulate directly
	// Check if points have grid structure
	bool isGrid = true;
	for (size_t i = 1; i < pointCloud.size(); ++i) {
		// Simple heuristic: if points are in a grid, neighbors should be nearby
		// This works for depth map outputs
	}
	
	// As a fallback, create vertices and use marching cubes or similar
	// For now, just create vertices and simple connectivity
	mesh->vertices = pointCloud;
	
	// Generate indices for a simple grid if we have enough points
	int width = static_cast<int>(std::sqrt(pointCloud.size()));
	int height = width;
	
	if (width * height == static_cast<int>(pointCloud.size()) && width > 1 && height > 1) {
		// We have an organized point cloud - create triangle mesh
		for (int y = 0; y < height - 1; ++y) {
			for (int x = 0; x < width - 1; ++x) {
				int i = y * width + x;
				
				Triangle t1;
				t1.v1 = i;
				t1.v2 = i + 1;
				t1.v3 = i + width;
				t1.submeshIndex = 0;
				mesh->triangles.push_back(t1);
				
				Triangle t2;
				t2.v1 = i + 1;
				t2.v2 = i + width + 1;
				t2.v3 = i + width;
				t2.submeshIndex = 0;
				mesh->triangles.push_back(t2);
			}
		}
		
		// Calculate normals
		MeshUtils::GenerateSmoothNormals(*mesh);
	}
	
	mesh->ComputeBounds();
	
	// Add default submesh
	Submesh submesh;
	submesh.startIndex = 0;
	submesh.triangleCount = static_cast<uint32_t>(mesh->triangles.size());
	submesh.materialName = "DefaultMaterial";
	submesh.color[0] = 1;
	submesh.color[1] = 1;
	submesh.color[2] = 1;
	submesh.color[3] = 1;
	submesh.visible = true;
	mesh->submeshes.push_back(submesh);
	
	return mesh;
}

std::unique_ptr<UniversalMesh> MeshReconstructor::TriangulateDepthMap(
	const DepthMap& depthMap,
	const ImageData* colorImage) {
	auto mesh = std::make_unique<UniversalMesh>();
	
	int width = depthMap.width;
	int height = depthMap.height;
	
	mesh->vertices.reserve(width * height);
	
	// Create vertices from depth map
	float fx = depthMap.focalLengthX > 0 ? depthMap.focalLengthX : 1.0f;
	float fy = depthMap.focalLengthY > 0 ? depthMap.focalLengthY : 1.0f;
	float cx = depthMap.principalPointX > 0 ? depthMap.principalPointX : width / 2.0f;
	float cy = depthMap.principalPointY > 0 ? depthMap.principalPointY : height / 2.0f;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float depth = *depthMap.GetDepthPtr(x, y);
			if (depth <= 0) {
				// Use default depth for invalid pixels
				depth = (depthMap.minDepthObserved + depthMap.maxDepthObserved) * 0.5f;
			}
			
			Vertex v;
			
			// Back-project
			v.x = (x - cx) / fx * depth;
			v.y = (y - cy) / fy * depth;
			v.z = depth;
			
			// UV coordinates
			v.u = static_cast<float>(x) / width;
			v.v = static_cast<float>(y) / height;
			
			// Color from image
			if (colorImage && y < colorImage->height && x < colorImage->width) {
				size_t idx = (y * colorImage->width + x) * colorImage->channels;
				v.r = colorImage->pixels[idx] / 255.0f;
				v.g = colorImage->pixels[idx + 1] / 255.0f;
				v.b = colorImage->pixels[idx + 2] / 255.0f;
				v.a = colorImage->channels > 3 ? colorImage->pixels[idx + 3] / 255.0f : 1.0f;
			} else {
				v.r = v.g = v.b = 1.0f;
				v.a = 1.0f;
			}
			
			v.nx = 0;
			v.ny = 0;
			v.nz = 1;
			v.id = static_cast<uint32_t>(y * width + x);
			
			mesh->vertices.push_back(v);
		}
	}
	
	// Create triangles
	for (int y = 0; y < height - 1; ++y) {
		for (int x = 0; x < width - 1; ++x) {
			int i = y * width + x;
			
			// Skip invalid vertices
			float d00 = *depthMap.GetDepthPtr(x, y);
			float d10 = *depthMap.GetDepthPtr(x + 1, y);
			float d01 = *depthMap.GetDepthPtr(x, y + 1);
			float d11 = *depthMap.GetDepthPtr(x + 1, y + 1);
			
			if (d00 <= 0 || d10 <= 0 || d01 <= 0 || d11 <= 0) continue;
			
			Triangle t1;
			t1.v1 = i;
			t1.v2 = i + 1;
			t1.v3 = i + width;
			t1.submeshIndex = 0;
			mesh->triangles.push_back(t1);
			
			Triangle t2;
			t2.v1 = i + 1;
			t2.v2 = i + width + 1;
			t2.v3 = i + width;
			t2.submeshIndex = 0;
			mesh->triangles.push_back(t2);
		}
	}
	
	// Generate normals
	MeshUtils::GenerateSmoothNormals(*mesh);
	
	// Add submesh
	Submesh submesh;
	submesh.startIndex = 0;
	submesh.triangleCount = static_cast<uint32_t>(mesh->triangles.size());
	submesh.materialName = "DefaultMaterial";
	submesh.color[0] = 1;
	submesh.color[1] = 1;
	submesh.color[2] = 1;
	submesh.color[3] = 1;
	submesh.visible = true;
	mesh->submeshes.push_back(submesh);
	
	mesh->ComputeBounds();
	return mesh;
}

void MeshReconstructor::OptimizeMesh(UniversalMesh& mesh) {
	if (config.removeDegenerates) {
		RemoveDegenerates(mesh);
	}
	
	if (config.weldVertices) {
		auto weldMap = MeshUtils::CreateWeldMap(mesh, config.weldTolerance);
		mesh.ApplyWeld(weldMap);
	}
	
	if (config.smoothMesh && mesh.vertices.size() > 2) {
		// Build adjacency information ONCE before the smoothing loop
		std::map<uint32_t, std::vector<uint32_t>> vertexNeighbors;
		for (const auto& tri : mesh.triangles) {
			if (tri.v1 >= mesh.vertices.size() || tri.v2 >= mesh.vertices.size() || tri.v3 >= mesh.vertices.size())
				continue;
			vertexNeighbors[tri.v1].push_back(tri.v2);
			vertexNeighbors[tri.v1].push_back(tri.v3);
			vertexNeighbors[tri.v2].push_back(tri.v1);
			vertexNeighbors[tri.v2].push_back(tri.v3);
			vertexNeighbors[tri.v3].push_back(tri.v1);
			vertexNeighbors[tri.v3].push_back(tri.v2);
		}
		
		// Remove duplicate neighbors for each vertex
		for (auto& [vid, neighbors] : vertexNeighbors) {
			std::sort(neighbors.begin(), neighbors.end());
			neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
		}
		
		for (int iter = 0; iter < config.smoothIterations; ++iter) {
			// Simple Laplacian smooth - compute neighbor average
			std::vector<Vertex> smoothed = mesh.vertices;
			
			for (auto& [vid, neighbors] : vertexNeighbors) {
				if (vid >= smoothed.size() || neighbors.empty()) continue;
				
				float avgX = 0, avgY = 0, avgZ = 0;
				for (uint32_t nid : neighbors) {
					if (nid < mesh.vertices.size()) {
						avgX += mesh.vertices[nid].x;
						avgY += mesh.vertices[nid].y;
						avgZ += mesh.vertices[nid].z;
					}
				}
				float count = static_cast<float>(neighbors.size());
				smoothed[vid].x = mesh.vertices[vid].x * 0.5f + (avgX / count) * 0.5f;
				smoothed[vid].y = mesh.vertices[vid].y * 0.5f + (avgY / count) * 0.5f;
				smoothed[vid].z = mesh.vertices[vid].z * 0.5f + (avgZ / count) * 0.5f;
			}
			
			mesh.vertices = std::move(smoothed);
			
			// Recompute normals after smoothing
			MeshUtils::GenerateSmoothNormals(mesh);
		}
	}
	
	if (config.fillHoles) {
		FillHoles(mesh);
	}
}

void MeshReconstructor::RemoveDegenerates(UniversalMesh& mesh) {
	std::vector<Triangle> validTris;
	validTris.reserve(mesh.triangles.size());
	
	for (const auto& t : mesh.triangles) {
		// Check for duplicate vertices
		if (t.v1 == t.v2 || t.v2 == t.v3 || t.v1 == t.v3) {
			continue;
		}
		
		// Check for degenerate area
		const Vertex& v1 = mesh.vertices[t.v1];
		const Vertex& v2 = mesh.vertices[t.v2];
		const Vertex& v3 = mesh.vertices[t.v3];
		
		float ax = v2.x - v1.x, ay = v2.y - v1.y, az = v2.z - v1.z;
		float bx = v3.x - v1.x, by = v3.y - v1.y, bz = v3.z - v1.z;
		
		float cx = ay * bz - az * by;
		float cy = az * bx - ax * bz;
		float cz = ax * by - ay * bx;
		float area = std::sqrt(cx*cx + cy*cy + cz*cz);
		
		if (area < 1e-6f) {
			continue;
		}
		
		validTris.push_back(t);
	}
	
	mesh.triangles = std::move(validTris);
}

void MeshReconstructor::FillHoles(UniversalMesh& mesh) {
	if (mesh.triangles.empty() || mesh.vertices.size() < 3) {
		return;
	}
	
	// Build edge connectivity map
	std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> edgeToTriangles;
	for (uint32_t tIdx = 0; tIdx < mesh.triangles.size(); ++tIdx) {
		const auto& tri = mesh.triangles[tIdx];
		auto addEdge = [&](uint32_t a, uint32_t b) {
			uint32_t lo = std::min(a, b);
			uint32_t hi = std::max(a, b);
			auto edge = std::make_pair(lo, hi);
			edgeToTriangles[edge].push_back(tIdx);
		};
		addEdge(tri.v1, tri.v2);
		addEdge(tri.v2, tri.v3);
		addEdge(tri.v3, tri.v1);
	}
	
	// Find boundary edges (edges used by only one triangle)
	std::vector<std::pair<uint32_t, uint32_t>> boundaryEdges;
	for (const auto& [edge, tris] : edgeToTriangles) {
		if (tris.size() == 1) {
			boundaryEdges.push_back(edge);
		}
	}
	
	if (boundaryEdges.empty()) {
		return;  // No holes found
	}		// Group boundary edges into loops
	std::vector<std::vector<uint32_t>> holes;
	std::set<uint32_t> usedEdges;
	
	for (const auto& edge : boundaryEdges) {
		// Use bitwise combine to avoid overflow (shift 16 bits, safe for vertex indices < 65535)
		uint32_t edgeHash = (edge.first << 16) | edge.second;
		if (usedEdges.find(edgeHash) != usedEdges.end()) continue;
		
		std::vector<uint32_t> hole;
		uint32_t start = edge.first;
		uint32_t current = edge.second;
		
		while (true) {
			hole.push_back(start);
			edgeHash = (start << 16) | current;
			usedEdges.insert(edgeHash);
			
			// Find the next boundary edge connected to current vertex
			bool foundNext = false;
			for (const auto& nextEdge : boundaryEdges) {
				uint32_t nextHash = (nextEdge.first << 16) | nextEdge.second;
				if (usedEdges.find(nextHash) != usedEdges.end()) continue;
				
				uint32_t nextStart = nextEdge.first;
				uint32_t nextEnd = nextEdge.second;
				
				if (nextStart == current) {
					hole.push_back(current);
					usedEdges.insert(nextHash);
					start = current;
					current = nextEnd;
					foundNext = true;
					break;
				} else if (nextEnd == current) {
					hole.push_back(current);
					usedEdges.insert(nextHash);
					start = current;
					current = nextStart;
					foundNext = true;
					break;
				}
			}
			
			if (!foundNext || hole.size() > config.maxHoleSize) break;  // Configurable safety limit
			if (current == edge.first) break;  // Loop closed
		}
		
		if (hole.size() >= 3) {
			holes.push_back(hole);
		}
	}
	
	// Fill each hole with triangles using ear clipping
	for (const auto& hole : holes) {
		if (hole.size() < 3) continue;
		
		// Simple approach: triangulate from first vertex
		// More robust would use ear clipping algorithm
		for (size_t i = 1; i + 1 < hole.size(); ++i) {
			Triangle tri;
			tri.v1 = hole[0];
			tri.v2 = hole[i];
			tri.v3 = hole[i + 1];
			tri.submeshIndex = 0;
			
			// Check for degeneracy before adding
			const Vertex& v1 = mesh.vertices[tri.v1];
			const Vertex& v2 = mesh.vertices[tri.v2];
			const Vertex& v3 = mesh.vertices[tri.v3];
			
			float ax = v2.x - v1.x, ay = v2.y - v1.y, az = v2.z - v1.z;
			float bx = v3.x - v1.x, by = v3.y - v1.y, bz = v3.z - v1.z;
			float cx = ay * bz - az * by;
			float cy = az * bx - ax * bz;
			float cz = ax * by - ay * bx;
			float area = std::sqrt(cx*cx + cy*cy + cz*cz);
			
			if (area > 1e-6f) {
				mesh.triangles.push_back(tri);
			}
		}
	}
}

void MeshReconstructor::GenerateUVs(UniversalMesh& mesh) {
	// Simple planar UV projection
	for (auto& v : mesh.vertices) {
		// Normalize to 0-1 range based on bounds
		v.u = (v.x - mesh.boundsMin[0]) / (mesh.boundsMax[0] - mesh.boundsMin[0] + 0.0001f);
		v.v = (v.y - mesh.boundsMin[1]) / (mesh.boundsMax[1] - mesh.boundsMin[1] + 0.0001f);
	}
}

void MeshReconstructor::GenerateTextures(UniversalMesh& mesh, const ImageData& colorImage) {
	// Texture generation would create an image from the vertex colors
	// and store the path in mesh.texturePaths
	// For now, just reserve the path
	mesh.texturePaths.push_back("scan_texture.png");
}

void MeshReconstructor::ReportProgress(float progress, const std::string& message) {
	if (progressCallback) {
		progressCallback(progress, message);
	}
}

// ============== IMAGE SCANNER IMPLEMENTATION ==============

std::unique_ptr<ScanProject> ImageScanner::CreateProject(const std::string& name, 
														  const std::string& outputDirectory) {
	auto project = std::make_unique<ScanProject>();
	project->name = name;
	project->outputDirectory = outputDirectory;
	project->state = ScanProject::State::Ready;
	return project;
}

bool ImageScanner::AddImages(ScanProject& project, const std::vector<std::string>& imagePaths) {
	for (const auto& path : imagePaths) {
		if (std::filesystem::exists(path)) {
			project.imagePaths.push_back(path);
		}
	}
	return !project.imagePaths.empty();
}

bool ImageScanner::SetCameraPoses(ScanProject& project, const std::vector<std::string>& poseFiles) {
	project.cameraPoses = poseFiles;
	return true;
}

bool ImageScanner::Reconstruct(ScanProject& project, 
							   const DepthEstimationConfig& depthConfig,
							   const MeshReconstructionConfig& meshConfig) {
	this->depthConfig = depthConfig;
	this->meshConfig = meshConfig;
	
	project.state = ScanProject::State::Processing;
	
	try {
		if (!EstimateDepths(project, depthConfig)) {
			project.state = ScanProject::State::Error;
			return false;
		}
		
		if (!ReconstructMesh(project, meshConfig)) {
			project.state = ScanProject::State::Error;
			return false;
		}
		
		project.state = ScanProject::State::Completed;
		return true;
	} catch (const std::exception& e) {
		project.errorMessage = e.what();
		project.state = ScanProject::State::Error;
		return false;
	}
}

bool ImageScanner::EstimateDepths(ScanProject& project, const DepthEstimationConfig& config) {
	depthEstimator.SetConfig(config);
	depthEstimator.SetProgressCallback([this](float p, const std::string& m) { 
		ReportProgress(p * 0.5f, m); 
	});
	
	auto images = LoadImages(project.imagePaths);
	if (images.empty()) {
		project.errorMessage = "No valid images found";
		return false;
	}
	
	ReportProgress(0.1f, "Estimating depth...");
	estimatedDepths.clear();
	
	if (images.size() == 1) {
		estimatedDepths.push_back(depthEstimator.EstimateFromSingleImage(*images[0]));
	} else if (images.size() == 2) {
		estimatedDepths.push_back(depthEstimator.EstimateFromStereo(*images[0], *images[1]));
	} else {
		auto poses = LoadCameraPoses(project.cameraPoses);
		std::vector<ImageData> imageDataVec;
		for (const auto& img : images) {
			imageDataVec.push_back(*img);
		}
		estimatedDepths.push_back(depthEstimator.EstimateFromMultipleImages(imageDataVec, poses));
	}
	
	return !estimatedDepths.empty();
}

bool ImageScanner::ReconstructMesh(ScanProject& project, const MeshReconstructionConfig& config) {
	meshReconstructor.SetConfig(config);
	meshReconstructor.SetProgressCallback([this](float p, const std::string& m) {
		ReportProgress(0.5f + p * 0.5f, m);
	});
	
	if (estimatedDepths.empty()) {
		project.errorMessage = "No depth maps available";
		return false;
	}
	
	auto images = LoadImages(project.imagePaths);
	
	ReportProgress(0.5f, "Reconstructing mesh...");
	
	if (estimatedDepths.size() == 1 && !images.empty()) {
		project.reconstructedMesh = meshReconstructor.ReconstructFromDepthAndColor(
			*estimatedDepths[0], *images[0]);
	} else {
		auto poses = LoadCameraPoses(project.cameraPoses);
		std::vector<ImageData> imageData;
		for (const auto& img : images) {
			imageData.push_back(*img);
		}
		std::vector<DepthMap> depthMapsRef;
		for (const auto& d : estimatedDepths) {
			depthMapsRef.push_back(*d);
		}
		project.reconstructedMesh = meshReconstructor.ReconstructFromMultipleViews(
			depthMapsRef, imageData, poses);
	}
	
	return project.reconstructedMesh != nullptr;
}

bool ImageScanner::SaveMesh(const ScanProject& project, const std::string& filePath, FormatType format) {
	if (!project.reconstructedMesh) {
		return false;
	}
	
	// Get handler for format and save
	FormatRegistry& registry = FormatRegistry::GetInstance();
	auto handler = registry.GetHandler(format);
	
	if (!handler) {
		return false;
	}
	
	UniversalModel model;
	model.AddMesh(*project.reconstructedMesh);
	
	return handler->ExportModel(model, filePath);
}

std::vector<std::unique_ptr<ImageData>> ImageScanner::LoadImages(const std::vector<std::string>& paths) {
	std::vector<std::unique_ptr<ImageData>> images;
	for (const auto& path : paths) {
		auto img = ImageProcessor::LoadImage(path);
		if (img) {
			images.push_back(std::move(img));
		}
	}
	return images;
}

std::vector<float> ImageScanner::LoadCameraPoses(const std::vector<std::string>& poseFiles) {
	std::vector<float> poses;
	
	if (poseFiles.empty()) {
		return poses;
	}
	
	for (const auto& poseFile : poseFiles) {
		if (!std::filesystem::exists(poseFile)) {
			continue;
		}
		
		try {
			std::ifstream file(poseFile);
			if (!file.is_open()) {
				continue;
			}
			
			// Try to parse as JSON camera pose file
			file.seekg(0, std::ios::end);
			size_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);
			
			std::string content(fileSize, '\0');
			file.read(content.data(), fileSize);
			file.close();
			
			// Simple JSON parsing for camera pose
			// Expected format: {"camera_pose": [16 numbers]} or [[16 numbers]]
			// Also supports legacy format: {"rotation": [...], "translation": [...]}
			
			std::vector<float> cameraPose = ParseCameraPoseJSON(content);
			
			if (!cameraPose.empty()) {
				poses.insert(poses.end(), cameraPose.begin(), cameraPose.end());
			} else {
				// Default to identity matrix if parsing fails
				float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
				poses.insert(poses.end(), identity, identity + 16);
			}
		} catch (const std::exception& e) {
			// On error, use identity transform
			float identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
			poses.insert(poses.end(), identity, identity + 16);
		}
	}
	
	return poses;
}

std::vector<float> ImageScanner::ParseCameraPoseJSON(const std::string& json) {
	std::vector<float> pose;
	
	if (json.empty()) {
		return pose;
	}
	
	try {
		auto j = nlohmann::json::parse(json);
		
		// Format 1: {"camera_pose": [16 numbers]}
		if (j.contains("camera_pose") && j["camera_pose"].is_array()) {
			auto& arr = j["camera_pose"];
			if (arr.size() >= 16) {
				pose.resize(16);
				for (int i = 0; i < 16; ++i) {
					pose[i] = arr[i].get<float>();
				}
				return pose;
			}
		}
		
		// Format 2: Direct array [[16 numbers]] or [16 numbers]
		if (j.is_array() && !j.empty()) {
			// Check if it's a nested array (Format 2: [[...]])
			if (j[0].is_array()) {
				auto& inner = j[0];
				if (inner.size() >= 16) {
					pose.resize(16);
					for (int i = 0; i < 16; ++i) {
						pose[i] = inner[i].get<float>();
					}
					return pose;
				}
			} else if (j.size() >= 16) {
				// Flat array format
				pose.resize(16);
				for (int i = 0; i < 16; ++i) {
					pose[i] = j[i].get<float>();
				}
				return pose;
			}
		}
		
		// Format 3: {"rotation": [9], "translation": [3]}
		if (j.contains("rotation") && j.contains("translation")) {
			auto& rot = j["rotation"];
			auto& trans = j["translation"];
			if (rot.is_array() && trans.is_array() && rot.size() >= 9 && trans.size() >= 3) {
				pose.resize(16, 0.0f);
				pose[15] = 1.0f;  // homogeneous coordinate
				
				// Row-major 3x3 rotation matrix
				for (int i = 0; i < 9; ++i) {
					pose[i] = rot[i].get<float>();
				}
				
				// Translation
				pose[12] = trans[0].get<float>();
				pose[13] = trans[1].get<float>();
				pose[14] = trans[2].get<float>();
				
				return pose;
			}
		}
	} catch (const nlohmann::json::parse_error& e) {
		// JSON parsing failed - LoadCameraPoses will use identity matrix as fallback
	}
	
	return pose;
}

void ImageScanner::ReportProgress(float progress, const std::string& message) {
	if (progressCallback) {
		progressCallback(progress, message);
	}
}