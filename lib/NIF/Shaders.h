/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Objects.h"

enum BSShaderType : uint {
	SHADER_TALL_GRASS,
	SHADER_DEFAULT,
	SHADER_SKY = 10,
	SHADER_SKIN = 14,
	SHADER_WATER = 17,
	SHADER_LIGHTING30 = 29,
	SHADER_TILE = 32,
	SHADER_NOLIGHTING
};

enum BSLightingShaderPropertyShaderType : uint {
	Default,
	EnvironmentMap,
	GlowShader,
	Heightmap,
	FaceTint,
	SkinTint,
	HairTint,
	ParallaxOccMaterial,
	WorldMultitexture,
	WorldMap1,
	Unknown10,
	MultiLayerParallax,
	Unknown12,
	WorldMap2,
	SparkleSnow,
	WorldMap3,
	EyeEnvmap,
	Unknown17,
	WorldMap4,
	WorldLODMultitexture
};

class NiProperty : public NiObjectNET {
};

class NiShader : public NiProperty {
public:
	virtual bool IsSkinTint() { return false; }
	virtual bool IsSkinned() { return false; }
	virtual void SetSkinned(const bool enable) {}
	virtual bool IsDoubleSided() { return false; }
	virtual bool IsModelSpace() { return false; }
	virtual bool IsEmissive() { return false; }
	virtual bool HasSpecular() { return true; }
	virtual bool HasBacklight() { return false; }
	virtual uint GetType() { return 0xFFFFFFFF; }
	virtual void SetType(const uint type) {}
	virtual Vector2 GetUVOffset() { return Vector2(); }
	virtual Vector2 GetUVScale() { return Vector2(1.0f, 1.0f); }
	virtual Vector3 GetSpecularColor() { return Vector3(); }
	virtual void SetSpecularColor(const Vector3& color) {}
	virtual float GetSpecularStrength() { return 0.0f; }
	virtual void SetSpecularStrength(const float strength) {}
	virtual float GetGlossiness() { return 0.0f; }
	virtual void SetGlossiness(const float gloss) {}
	virtual float GetEnvironmentMapScale() { return 0.0f; }
	virtual int GetTextureSetRef() { return 0xFFFFFFFF; }
	virtual void SetTextureSetRef(const int texSetRef) {}
	virtual Color4 GetEmissiveColor() { return Color4(); }
	virtual void SetEmissiveColor(const Color4& color) {}
	virtual float GetEmissiveMultiple() { return 0.0f; }
	virtual void SetEmissiveMultiple(const float emissive) {}
	virtual float GetAlpha() { return 1.0f; }
	virtual std::string GetWetMaterialName() { return std::string(); }
	virtual void SetWetMaterialName(const std::string& matName) {}
};

class BSShaderProperty : public NiShader {
public:
	ushort shaderFlags;
	BSShaderType shaderType;
	uint shaderFlags1;
	uint shaderFlags2;
	float environmentMapScale;
	Vector2 uvOffset;
	Vector2 uvScale;

	void Init();
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);

	uint GetType();
	void SetType(const uint type);
	bool HasSpecular();
	float GetEnvironmentMapScale();
	Vector2 GetUVOffset();
	Vector2 GetUVScale();
};

class BSShaderTextureSet : public NiObject {
public:
	int numTextures;
	std::vector<NiString> textures;

	BSShaderTextureSet();
	BSShaderTextureSet(NiVersion& version);
	BSShaderTextureSet(NiStream& stream);

	static constexpr const char* BlockName = "BSShaderTextureSet";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSShaderTextureSet* Clone() { return new BSShaderTextureSet(*this); }
};

class BSLightingShaderProperty : public BSShaderProperty {
private:
	StringRef wetMaterialName;
	BlockRef<BSShaderTextureSet> textureSetRef;

public:
	Vector3 emissiveColor;
	float emissiveMultiple;
	uint textureClampMode;
	float alpha;
	float refractionStrength;
	float glossiness;
	Vector3 specularColor;
	float specularStrength;
	float lightingEffect1;					// User Version <= 12, User Version 2 < 130
	float lightingEffect2;					// User Version <= 12, User Version 2 < 130

	float subsurfaceRolloff;				// User Version == 12, User Version 2 >= 130
	float unkFloat1;						// User Version == 12, User Version 2 >= 130
	float backlightPower;					// User Version == 12, User Version 2 >= 130
	float grayscaleToPaletteScale;			// User Version == 12, User Version 2 >= 130
	float fresnelPower;						// User Version == 12, User Version 2 >= 130
	float wetnessSpecScale;					// User Version == 12, User Version 2 >= 130
	float wetnessSpecPower;					// User Version == 12, User Version 2 >= 130
	float wetnessMinVar;					// User Version == 12, User Version 2 >= 130
	float wetnessEnvmapScale;				// User Version == 12, User Version 2 >= 130
	float wetnessFresnelPower;				// User Version == 12, User Version 2 >= 130
	float wetnessMetalness;					// User Version == 12, User Version 2 >= 130

	ushort unkEnvmap;						// Shader Type == 1, User Version == 12, User Version 2 >= 130
	Vector3 skinTintColor;					// Shader Type == 5
	uint unkSkinTint;						// Shader Type == 5, User Version == 12, User Version 2 >= 130
	Vector3 hairTintColor;					// Shader Type == 6
	float maxPasses;						// Shader Type == 7
	float scale;							// Shader Type == 7
	float parallaxInnerLayerThickness;		// Shader Type == 11
	float parallaxRefractionScale;			// Shader Type == 11
	Vector2 parallaxInnerLayerTextureScale;	// Shader Type == 11
	float parallaxEnvmapStrength;			// Shader Type == 11
	Color4 sparkleParameters;				// Shader Type == 14
	float eyeCubemapScale;					// Shader Type == 16
	Vector3 eyeLeftReflectionCenter;		// Shader Type == 16
	Vector3 eyeRightReflectionCenter;		// Shader Type == 16

	BSLightingShaderProperty();
	BSLightingShaderProperty(NiVersion& version);
	BSLightingShaderProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSLightingShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetStringRefs(std::set<StringRef*>& refs);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	BSLightingShaderProperty* Clone() { return new BSLightingShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	bool IsDoubleSided();
	bool IsModelSpace();
	bool IsEmissive();
	bool HasBacklight();
	uint GetType();
	void SetType(const uint type);
	Vector3 GetSpecularColor();
	void SetSpecularColor(const Vector3& color);
	float GetSpecularStrength();
	void SetSpecularStrength(const float strength);
	float GetGlossiness();
	void SetGlossiness(const float gloss);
	int GetTextureSetRef();
	void SetTextureSetRef(const int texSetRef);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
	float GetAlpha();
	std::string GetWetMaterialName();
	void SetWetMaterialName(const std::string& matName);
};

class BSEffectShaderProperty : public BSShaderProperty {
public:
	NiString sourceTexture;
	uint textureClampMode;
	float falloffStartAngle;
	float falloffStopAngle;
	float falloffStartOpacity;
	float falloffStopOpacity;
	Color4 emissiveColor;
	float emissiveMultiple;
	float softFalloffDepth;
	NiString greyscaleTexture;

	NiString envMapTexture;
	NiString normalTexture;
	NiString envMaskTexture;
	float envMapScale;

	BSEffectShaderProperty();
	BSEffectShaderProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSEffectShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSEffectShaderProperty* Clone() { return new BSEffectShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	bool IsDoubleSided();
	bool IsModelSpace();
	bool IsEmissive();
	bool HasBacklight();
	float GetEnvironmentMapScale();
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
};

class BSWaterShaderProperty : public BSShaderProperty {
private:
	uint waterFlags;

public:
	BSWaterShaderProperty();
	BSWaterShaderProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSWaterShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSWaterShaderProperty* Clone() { return new BSWaterShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	bool IsDoubleSided();
	bool IsModelSpace();
	bool IsEmissive();
	bool HasBacklight();
};

class BSSkyShaderProperty : public BSShaderProperty {
private:
	NiString baseTexture;
	uint skyFlags = 0;

public:
	BSSkyShaderProperty();
	BSSkyShaderProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSSkyShaderProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSSkyShaderProperty* Clone() { return new BSSkyShaderProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	bool IsDoubleSided();
	bool IsModelSpace();
	bool IsEmissive();
	bool HasBacklight();
};

class BSShaderLightingProperty : public BSShaderProperty {
public:
	uint textureClampMode;					// User Version <= 11

	void Init();
	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
};

class BSShaderNoLightingProperty : public BSShaderLightingProperty {
public:
	NiString baseTexture;
	float falloffStartAngle;				// User Version 2 > 26
	float falloffStopAngle;					// User Version 2 > 26
	float falloffStartOpacity;				// User Version 2 > 26
	float falloffStopOpacity;				// User Version 2 > 26

	BSShaderNoLightingProperty();
	BSShaderNoLightingProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSShaderNoLightingProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	BSShaderNoLightingProperty* Clone() { return new BSShaderNoLightingProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
};

class BSShaderPPLightingProperty : public BSShaderLightingProperty {
private:
	BlockRef<BSShaderTextureSet> textureSetRef;

public:
	float refractionStrength;				// User Version == 11 && User Version 2 > 14
	int refractionFirePeriod;				// User Version == 11 && User Version 2 > 14
	float parallaxMaxPasses;				// User Version == 11 && User Version 2 > 24
	float parallaxScale;					// User Version == 11 && User Version 2 > 24
	Color4 emissiveColor;					// User Version >= 12

	BSShaderPPLightingProperty();
	BSShaderPPLightingProperty(NiStream& stream);

	static constexpr const char* BlockName = "BSShaderPPLightingProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	void GetChildRefs(std::set<int*>& refs);
	int CalcBlockSize(NiVersion& version);
	BSShaderPPLightingProperty* Clone() { return new BSShaderPPLightingProperty(*this); }

	bool IsSkinTint();
	bool IsSkinned();
	void SetSkinned(const bool enable);
	int GetTextureSetRef();
	void SetTextureSetRef(const int texSetRef);
};

class NiAlphaProperty : public NiProperty {
public:
	ushort flags;
	byte threshold;

	NiAlphaProperty();
	NiAlphaProperty(NiStream& stream);

	static constexpr const char* BlockName = "NiAlphaProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	NiAlphaProperty* Clone() { return new NiAlphaProperty(*this); }
};


class NiMaterialProperty : public NiProperty {
private:
	Vector3 colorSpecular;
	Vector3 colorEmissive;
	float glossiness;
	float alpha;
	float emitMulti;						// Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21

public:
	Vector3 colorAmbient;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)
	Vector3 colorDiffuse;					// !(Version == 20.2.0.7 && User Version >= 11 && User Version 2 > 21)

	NiMaterialProperty();
	NiMaterialProperty(NiStream& stream);

	static constexpr const char* BlockName = "NiMaterialProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	NiMaterialProperty* Clone() { return new NiMaterialProperty(*this); }

	bool IsEmissive();
	Vector3 GetSpecularColor();
	void SetSpecularColor(const Vector3& color);
	float GetGlossiness();
	void SetGlossiness(const float gloss);
	Color4 GetEmissiveColor();
	void SetEmissiveColor(const Color4& color);
	float GetEmissiveMultiple();
	void SetEmissiveMultiple(const float emissive);
	float GetAlpha();
};

class NiStencilProperty : public NiProperty {
public:
	ushort flags;
	uint stencilRef;
	uint stencilMask;

	NiStencilProperty();
	NiStencilProperty(NiStream& stream);

	static constexpr const char* BlockName = "NiStencilProperty";
	virtual const char* GetBlockName() { return BlockName; }

	void Get(NiStream& stream);
	void Put(NiStream& stream);
	int CalcBlockSize(NiVersion& version);
	NiStencilProperty* Clone() { return new NiStencilProperty(*this); }
};
