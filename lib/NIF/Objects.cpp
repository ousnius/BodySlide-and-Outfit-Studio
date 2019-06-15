/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#include "Objects.h"

void NiObjectNET::Get(NiStream& stream) {
	NiObject::Get(stream);

	if (bBSLightingShaderProperty && stream.GetVersion().User() >= 12)
		stream >> bslspShaderType;

	name.Get(stream);

	extraDataRefs.Get(stream);
	controllerRef.Get(stream);
}

void NiObjectNET::Put(NiStream& stream) {
	NiObject::Put(stream);

	if (bBSLightingShaderProperty && stream.GetVersion().User() >= 12)
		stream << bslspShaderType;

	name.Put(stream);

	extraDataRefs.Put(stream);
	controllerRef.Put(stream);
}

void NiObjectNET::GetStringRefs(std::set<StringRef*>& refs) {
	NiObject::GetStringRefs(refs);

	refs.insert(&name);
}

void NiObjectNET::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	extraDataRefs.GetIndexPtrs(refs);
	refs.insert(&controllerRef);
}

std::string NiObjectNET::GetName() {
	return name.GetString();
}

void NiObjectNET::SetName(const std::string& str) {
	name.SetString(str);
}

void NiObjectNET::ClearName() {
	name.Clear();
}

int NiObjectNET::GetControllerRef() {
	return controllerRef.GetIndex();
}

void NiObjectNET::SetControllerRef(int ctlrRef) {
	controllerRef.SetIndex(ctlrRef);
}

BlockRefArray<NiExtraData>& NiObjectNET::GetExtraData() {
	return extraDataRefs;
}


void NiAVObject::Get(NiStream& stream) {
	NiObjectNET::Get(stream);

	flags = 0;
	if (stream.GetVersion().Stream() <= 26)
		stream.read((char*)&flags, 2);
	else
		stream >> flags;

	stream >> transform.translation;
	stream >> transform.rotation;
	stream >> transform.scale;

	if (stream.GetVersion().Stream() <= 34)
		propertyRefs.Get(stream);
	
	if (stream.GetVersion().File() >= V10_0_1_0)
		collisionRef.Get(stream);
}

void NiAVObject::Put(NiStream& stream) {
	NiObjectNET::Put(stream);

	if (stream.GetVersion().Stream() <= 26)
		stream.write((char*)&flags, 2);
	else
		stream << flags;

	stream << transform.translation;
	stream << transform.rotation;
	stream << transform.scale;

	if (stream.GetVersion().Stream() <= 34)
		propertyRefs.Put(stream);

	if (stream.GetVersion().File() >= V10_0_1_0)
		collisionRef.Put(stream);
}

void NiAVObject::GetChildRefs(std::set<Ref*>& refs) {
	NiObjectNET::GetChildRefs(refs);

	propertyRefs.GetIndexPtrs(refs);
	refs.insert(&collisionRef);
}

BlockRefArray<NiProperty>& NiAVObject::GetProperties() {
	return propertyRefs;
}


void NiDefaultAVObjectPalette::Get(NiStream& stream) {
	NiAVObjectPalette::Get(stream);

	sceneRef.Get(stream);

	stream >> numObjects;
	objects.resize(numObjects);
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Get(stream, 4);
		objects[i].objectRef.Get(stream);
	}
}

void NiDefaultAVObjectPalette::Put(NiStream& stream) {
	NiAVObjectPalette::Put(stream);

	sceneRef.Put(stream);

	stream << numObjects;
	for (int i = 0; i < numObjects; i++) {
		objects[i].name.Put(stream, 4, false);
		objects[i].objectRef.Put(stream);
	}
}

void NiDefaultAVObjectPalette::GetPtrs(std::set<Ref*>& ptrs) {
	NiAVObjectPalette::GetPtrs(ptrs);

	ptrs.insert(&sceneRef);

	for (int i = 0; i < numObjects; i++)
		ptrs.insert(&objects[i].objectRef);
}


void NiCamera::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	stream >> obsoleteFlags;
	stream >> frustumLeft;
	stream >> frustumRight;
	stream >> frustumTop;
	stream >> frustomBottom;
	stream >> frustumNear;
	stream >> frustumFar;
	stream >> useOrtho;
	stream >> viewportLeft;
	stream >> viewportRight;
	stream >> viewportTop;
	stream >> viewportBottom;
	stream >> lodAdjust;

	sceneRef.Get(stream);
	stream >> numScreenPolygons;
	stream >> numScreenTextures;
}

void NiCamera::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	stream << obsoleteFlags;
	stream << frustumLeft;
	stream << frustumRight;
	stream << frustumTop;
	stream << frustomBottom;
	stream << frustumNear;
	stream << frustumFar;
	stream << useOrtho;
	stream << viewportLeft;
	stream << viewportRight;
	stream << viewportTop;
	stream << viewportBottom;
	stream << lodAdjust;

	sceneRef.Put(stream);
	stream << numScreenPolygons;
	stream << numScreenTextures;
}

void NiCamera::GetChildRefs(std::set<Ref*>& refs) {
	NiAVObject::GetChildRefs(refs);

	refs.insert(&sceneRef);
}

int NiCamera::GetSceneRef() {
	return sceneRef.GetIndex();
}

void NiCamera::SetSceneRef(int scRef) {
	sceneRef.SetIndex(scRef);
}


void NiPalette::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> hasAlpha;
	stream >> numEntries;
	palette.resize(numEntries);
	for (int i = 0; i < numEntries; i++)
		stream >> palette[i];
}

void NiPalette::Put(NiStream& stream) {
	NiObject::Put(stream);

	// Size can only be 16 or 256
	if (numEntries != 16 || numEntries != 256) {
		if (numEntries >= 128)
			numEntries = 256;
		else
			numEntries = 16;

		palette.resize(numEntries);
	}

	stream << hasAlpha;
	stream << numEntries;
	for (int i = 0; i < numEntries; i++)
		stream << palette[i];
}


void TextureRenderData::Get(NiStream& stream) {
	NiObject::Get(stream);

	stream >> pixelFormat;
	stream >> bitsPerPixel;
	stream >> unkInt1;
	stream >> unkInt2;
	stream >> flags;
	stream >> unkInt3;

	for (int i = 0; i < 4; i++) {
		stream >> channels[i].type;
		stream >> channels[i].convention;
		stream >> channels[i].bitsPerChannel;
		stream >> channels[i].unkByte1;
	}

	paletteRef.Get(stream);

	stream >> numMipmaps;
	stream >> bytesPerPixel;
	mipmaps.resize(numMipmaps);
	for (int i = 0; i < numMipmaps; i++)
		stream >> mipmaps[i];
}

void TextureRenderData::Put(NiStream& stream) {
	NiObject::Put(stream);

	stream << pixelFormat;
	stream << bitsPerPixel;
	stream << unkInt1;
	stream << unkInt2;
	stream << flags;
	stream << unkInt3;

	for (int i = 0; i < 4; i++) {
		stream << channels[i].type;
		stream << channels[i].convention;
		stream << channels[i].bitsPerChannel;
		stream << channels[i].unkByte1;
	}

	paletteRef.Put(stream);

	stream << numMipmaps;
	stream << bytesPerPixel;
	for (int i = 0; i < numMipmaps; i++)
		stream << mipmaps[i];
}

void TextureRenderData::GetChildRefs(std::set<Ref*>& refs) {
	NiObject::GetChildRefs(refs);

	refs.insert(&paletteRef);
}

int TextureRenderData::GetPaletteRef() {
	return paletteRef.GetIndex();
}

void TextureRenderData::SetPaletteRef(int palRef) {
	paletteRef.SetIndex(palRef);
}


void NiPersistentSrcTextureRendererData::Get(NiStream& stream) {
	TextureRenderData::Get(stream);

	stream >> numPixels;
	stream >> unkInt4;
	stream >> numFaces;
	stream >> unkInt5;

	pixelData.resize(numFaces);
	for (int f = 0; f < numFaces; f++) {
		pixelData[f].resize(numPixels);
		for (int p = 0; p < numPixels; p++)
			stream >> pixelData[f][p];
	}
}

void NiPersistentSrcTextureRendererData::Put(NiStream& stream) {
	TextureRenderData::Put(stream);

	stream >> numPixels;
	stream >> unkInt4;
	stream >> numFaces;
	stream >> unkInt5;

	for (int f = 0; f < numFaces; f++)
		for (int p = 0; p < numPixels; p++)
			stream >> pixelData[f][p];
}


void NiPixelData::Get(NiStream& stream) {
	TextureRenderData::Get(stream);

	stream >> numPixels;
	stream >> numFaces;

	pixelData.resize(numFaces);
	for (int f = 0; f < numFaces; f++) {
		pixelData[f].resize(numPixels);
		for (int p = 0; p < numPixels; p++)
			stream >> pixelData[f][p];
	}
}

void NiPixelData::Put(NiStream& stream) {
	TextureRenderData::Put(stream);

	stream >> numPixels;
	stream >> numFaces;

	for (int f = 0; f < numFaces; f++)
		for (int p = 0; p < numPixels; p++)
			stream >> pixelData[f][p];
}


void NiSourceTexture::Get(NiStream& stream) {
	NiTexture::Get(stream);

	stream >> useExternal;
	fileName.Get(stream);
	dataRef.Get(stream);
	stream >> pixelLayout;
	stream >> mipMapFormat;
	stream >> alphaFormat;
	stream >> isStatic;

	if (stream.GetVersion().File() >= NiVersion::ToFile(10, 1, 0, 103))
		stream >> directRender;
	if (stream.GetVersion().File() >= NiVersion::ToFile(20, 2, 0, 4))
		stream >> persistentRenderData;
}

void NiSourceTexture::Put(NiStream& stream) {
	NiTexture::Put(stream);

	stream << useExternal;
	fileName.Put(stream);
	dataRef.Put(stream);
	stream << pixelLayout;
	stream << mipMapFormat;
	stream << alphaFormat;
	stream << isStatic;

	if (stream.GetVersion().File() >= NiVersion::ToFile(10, 1, 0, 103))
		stream << directRender;
	if (stream.GetVersion().File() >= NiVersion::ToFile(20, 2, 0, 4))
		stream << persistentRenderData;
}

void NiSourceTexture::GetStringRefs(std::set<StringRef*>& refs) {
	NiTexture::GetStringRefs(refs);

	refs.insert(&fileName);
}

void NiSourceTexture::GetChildRefs(std::set<Ref*>& refs) {
	NiTexture::GetChildRefs(refs);

	refs.insert(&dataRef);
}

int NiSourceTexture::GetDataRef() {
	return dataRef.GetIndex();
}

void NiSourceTexture::SetDataRef(int datRef) {
	dataRef.SetIndex(datRef);
}


void NiDynamicEffect::Get(NiStream& stream) {
	NiAVObject::Get(stream);

	if (stream.GetVersion().Stream() < 130) {
		stream >> switchState;
		affectedNodes.Get(stream);
	}
}

void NiDynamicEffect::Put(NiStream& stream) {
	NiAVObject::Put(stream);

	if (stream.GetVersion().Stream() < 130) {
		stream << switchState;
		affectedNodes.Put(stream);
	}
}

void NiDynamicEffect::GetChildRefs(std::set<Ref*>& refs) {
	NiAVObject::GetChildRefs(refs);

	affectedNodes.GetIndexPtrs(refs);
}

BlockRefArray<NiNode>& NiDynamicEffect::GetAffectedNodes() {
	return affectedNodes;
}


void NiTextureEffect::Get(NiStream& stream) {
	NiDynamicEffect::Get(stream);

	stream >> modelProjectionMatrix;
	stream >> modelProjectionTransform;
	stream >> textureFiltering;
	stream >> textureClamping;
	stream >> textureType;
	stream >> coordinateGenerationType;
	sourceTexture.Get(stream);
	stream >> clippingPlane;
	stream >> unkVector;
	stream >> unkFloat;
}

void NiTextureEffect::Put(NiStream& stream) {
	NiDynamicEffect::Put(stream);

	stream << modelProjectionMatrix;
	stream << modelProjectionTransform;
	stream << textureFiltering;
	stream << textureClamping;
	stream << textureType;
	stream << coordinateGenerationType;
	sourceTexture.Put(stream);
	stream << clippingPlane;
	stream << unkVector;
	stream << unkFloat;
}

void NiTextureEffect::GetChildRefs(std::set<Ref*>& refs) {
	NiDynamicEffect::GetChildRefs(refs);

	refs.insert(&sourceTexture);
}

int NiTextureEffect::GetSourceTextureRef() {
	return sourceTexture.GetIndex();
}

void NiTextureEffect::SetSourceTextureRef(int srcTexRef) {
	sourceTexture.SetIndex(srcTexRef);
}


void NiLight::Get(NiStream& stream) {
	NiDynamicEffect::Get(stream);

	stream >> dimmer;
	stream >> ambientColor;
	stream >> diffuseColor;
	stream >> specularColor;
}

void NiLight::Put(NiStream& stream) {
	NiDynamicEffect::Put(stream);

	stream << dimmer;
	stream << ambientColor;
	stream << diffuseColor;
	stream << specularColor;
}


void NiPointLight::Get(NiStream& stream) {
	NiLight::Get(stream);

	stream >> constantAttenuation;
	stream >> linearAttenuation;
	stream >> quadraticAttenuation;
}

void NiPointLight::Put(NiStream& stream) {
	NiLight::Put(stream);

	stream << constantAttenuation;
	stream << linearAttenuation;
	stream << quadraticAttenuation;
}


void NiSpotLight::Get(NiStream& stream) {
	NiPointLight::Get(stream);

	stream >> cutoffAngle;
	stream >> unkFloat;
	stream >> exponent;
}

void NiSpotLight::Put(NiStream& stream) {
	NiPointLight::Put(stream);

	stream << cutoffAngle;
	stream << unkFloat;
	stream << exponent;
}
