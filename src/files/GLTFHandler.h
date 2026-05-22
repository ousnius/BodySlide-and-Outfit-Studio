/*
BodySlide and Outfit Studio
See the included LICENSE file

glTF Format Handler - Universal 3D format for game engines
*/

#pragma once

#ifdef USE_FBXSDK

#include <string>
#include <memory>
#include <vector>
#include <filesystem>

#include <UniversalModel.h>

struct aiMesh;
struct aiNode;
struct aiScene;

namespace univmodel {

class GLTFHandler : public IFormatHandler {
public:
	GLTFHandler();
	virtual ~GLTFHandler();

	FormatInfo GetFormatInfo() const override;
	bool CanImport() const override { return true; }
	bool CanExport() const override { return true; }

	std::unique_ptr<UniversalModel> ImportModel(const std::string& filePath) override;
	bool ExportModel(const UniversalModel& model, const std::string& filePath) override;

	std::unique_ptr<UniversalAnimation> ImportAnimation(const std::string& filePath) override;
	bool ExportAnimation(const UniversalAnimation& anim, const std::string& filePath) override;

	bool CanHandleFile(const std::string& filePath) const override;
	FormatType GetFormatType() const override { return FormatType::GLTF; }

	// glTF-specific options
	struct ExportOptions {
		bool embedTextures = false;
		bool exportSkinning = true;
		bool exportAnimations = true;
		bool useBinaryFormat = false;  // .glb vs .gltf
		float scaleFactor = 1.0f;
		int materialTextureSize = 2048;
	};

	void SetExportOptions(const ExportOptions& options) { exportOptions = options; }
	const ExportOptions& GetExportOptions() const { return exportOptions; }

private:
	ExportOptions exportOptions;

	// Internal conversion methods
	void UniversalMeshToAssimp(const UniversalMesh& mesh, aiMesh* outMesh);
	UniversalMesh AssimpMeshToUniversal(const aiMesh* mesh);

	// Write glTF JSON
	void WriteGLTF(const UniversalModel& model, const std::string& filePath);
	void WriteGLB(const UniversalModel& model, const std::string& filePath);

	// Read glTF JSON
	std::unique_ptr<UniversalModel> ReadGLTF(const std::string& filePath);
	std::unique_ptr<UniversalModel> ReadGLB(const std::string& filePath);
};

} // namespace univmodel

#endif // USE_FBXSDK