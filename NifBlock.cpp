#include "NifFile.h"

NifBlock::~NifBlock() { 
}

void NifBlock::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	return;
}
void NifBlock::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	return;
}
void NifBlock::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	return;
}	

void NifBlock::Get(fstream& file, NifBlockHeader& hdr) {
}

void NifBlock::Put(fstream& file, NifBlockHeader& hdr) {
}

int NifBlock::CalcBlockSize() {
	return blockSize;
}

void NifBlock::SetBlockSize(int sz) {
	blockSize=sz;
}



NifString::NifString(bool wantOutputNull) {
	outputNull = wantOutputNull;
}
NifString::NifString(fstream& file, int szSize, bool wantNullOutput) {
	outputNull = wantNullOutput;
	Get(file,szSize);
}
void NifString::Put(fstream& file, int szSize) {	
	unsigned char smSize;
	unsigned short medSize;
	unsigned int bigSize;
	if(szSize == 1) {
		smSize = str.length();
		if(!smSize && outputNull) 
			smSize=1;
		file.write((char*)&smSize,1);
	} else if (szSize==2) {
		medSize = str.length();
		if(!medSize && outputNull) 
			medSize=1;
		file.write((char*)&medSize,2);
	} else if (szSize==4) {
		bigSize = str.length();
		if(!bigSize && outputNull) 
			bigSize=1;
		file.write((char*)&bigSize,4);

	}
	file.write(str.c_str(),str.length());
	if(str.length()==0 && outputNull)
		file.put(0);
}

void NifString::Get(fstream& file, int szSize) {
	char buf[1025];
	unsigned char smSize;
	unsigned short medSize;
	unsigned int bigSize;

	if(szSize == 1) {
		file.read((char*)&smSize,1);	
		file.read(buf,smSize);
		buf[smSize] = 0;
	} else if (szSize == 2) {
		file.read((char*)&medSize,2);
		if(medSize < 1025)
			file.read(buf,medSize);
		buf[medSize] = 0;
	} else if (szSize == 4) {
		file.read((char*)&bigSize,4);
		if(bigSize < 1025)
			file.read(buf,bigSize);
		buf[bigSize] = 0;
	} else 
		return;
	str = buf;
}

void NifBlockNiNode::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	int intData;
	if(hdr.VerCheck(20,1,0,3)) {
		file.read((char*)&nameID,4);
		if(nameID != -1) {
			nodeName = hdr.strings[nameID].str;
		}
	} else {
		nodeName = NifString(file, 4).str;
	}

	file.read((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.read((char*)&intData,4);
		extradata.push_back(intData);
	}
	file.read((char*)&controllerRef,4);
	file.read((char*)&flags,2);
	if(hdr.VerCheck(20,2,0,7)) {
		file.read((char*)&unk,2);
	}
	file.read((char*)&translation.x,4);
	file.read((char*)&translation.y,4);
	file.read((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.read((char*)&rotation[i].x,4);
		file.read((char*)&rotation[i].y,4);
		file.read((char*)&rotation[i].z,4);
	}
	file.read((char*)&scale,4);
	if(hdr.userVer <= 11) {
		file.read((char*)&numProps,4);
		for(i = 0; i<numProps; i++) {
			file.read((char*)&intData,4);
			propRef.push_back(intData);
		}
	} else {
		numProps = 0;
	}
	file.read((char*)&collisionRef,4);

	file.read((char*)&numChildren,4);
	for(i=0;i<numChildren;i++) {
		file.read((char*)&intData,4);
		children.push_back(intData);
	}	
	
	file.read((char*)&numEffects,4);
	for(i=0;i<numEffects;i++) {
		file.read((char*)&intData,4);
		effects.push_back(intData);
	}

}
void NifBlockNiNode::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&nameID,4);
	//shapeName = hdr.strings[nameID].str;
	file.write((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.write((char*)&extradata[i],4);
	}
	file.write((char*)&controllerRef,4);
	file.write((char*)&flags,2);
	file.write((char*)&unk,2);
	file.write((char*)&translation.x,4);
	file.write((char*)&translation.y,4);
	file.write((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.write((char*)&rotation[i].x,4);
		file.write((char*)&rotation[i].y,4);
		file.write((char*)&rotation[i].z,4);
	}
	file.write((char*)&scale,4);
	if(hdr.userVer <= 11) {
		file.write((char*)&numProps,4);
		for(i = 0; i<numProps; i++) {
			file.write((char*)&propRef[i],4);
		}
	}
	file.write((char*)&collisionRef,4);
	
	file.write((char*)&numChildren,4);
	for(i=0;i<numChildren;i++) {
		file.write((char*)&children[i],4);
	}	
	
	file.write((char*)&numEffects,4);
	for(i=0;i<numEffects;i++) {
		file.write((char*)&effects[i],4);
	}

}

void NifBlockNiNode::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	int i;

	for(i=0;i<numExtra;i++) {
		if(extradata[i] == blockID) {
			extradata.erase(extradata.begin()+ i);
			i--;
			numExtra--;
			blockSize -= 4;
		} else if(extradata[i]>blockID) extradata[i]--;
	}

	if(controllerRef == blockID) controllerRef = -1;
	else if(controllerRef > blockID) controllerRef--;

	if(collisionRef == blockID) collisionRef = -1;
	else if(collisionRef > blockID) collisionRef--;

	for(i=0;i<numChildren;i++) {
		if(children[i] == blockID) {
			children.erase(children.begin()+ i);
			i--;
			numChildren--;
			blockSize -= 4;
		} else if(children[i]>blockID) children[i]--;
	}
	for(i=0;i<numEffects;i++) {
		if(effects[i] == blockID) {
			effects.erase(effects.begin()+ i);
			i--;
			numEffects--;
			blockSize -= 4;
		} else if(effects[i]>blockID) effects[i]--;
	}

}

NifBlockNiNode::NifBlockNiNode() {
	nameID = -1;
	numExtra = 0;
	controllerRef = -1;	
	flags = 14;
	unk = 8;
	scale = 1.0f;
	numProps = 0;
	collisionRef = -1;
	numChildren = 0;
	numEffects = 0;
	blockType = NIFBLOCK_NINODE;
	blockSize =  80;
}

NifBlockNiNode::NifBlockNiNode(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_NINODE;
	Get(file, hdr);
}

void NifBlockNiNode::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if(ver1 == 12) blockSize -= 4;
	if(ver1 == 11) blockSize += 4;
}
int NifBlockNiNode::CalcBlockSize() {
	return (blockSize = 80 + (numExtra * 4) + (numChildren * 4) + (numEffects * 4));
}

void NifBlockTriStrips::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	int intData;
	file.read((char*)&nameID,4);
	shapeName = hdr.strings[nameID].str;
	file.read((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.read((char*)&intData,4);
		extradata.push_back(intData);
	}
	file.read((char*)&controllerRef,4);
	file.read((char*)&flags,2);
	file.read((char*)&unk,2);
	file.read((char*)&translation.x,4);
	file.read((char*)&translation.y,4);
	file.read((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.read((char*)&rotation[i].x,4);
		file.read((char*)&rotation[i].y,4);
		file.read((char*)&rotation[i].z,4);
	}
	file.read((char*)&scale,4);
	file.read((char*)&collisionRef,4);
	file.read((char*)&dataRef,4);
	file.read((char*)&skinRef,4);
	file.read((char*)&numMat,4);
	for(i=0;i<numMat;i++) {
		materialNames.push_back(NifString(file,2));
	}
	for(i=0;i<numMat;i++){
		file.read((char*)&intData,4);
		materialExtra.push_back(intData);
	}
	file.read((char*)&activeMat,4);
	file.read((char*)&dirty,1);
	if(hdr.userVer > 11) {
		file.read((char*)&propertiesRef1,4);
		file.read((char*)&propertiesRef2,4);
	}
}


void NifBlockTriStrips::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&nameID,4);
	//shapeName = hdr.strings[nameID].str;
	file.write((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.write((char*)&extradata[i],4);
	}
	file.write((char*)&controllerRef,4);
	file.write((char*)&flags,2);
	file.write((char*)&unk,2);
	file.write((char*)&translation.x,4);
	file.write((char*)&translation.y,4);
	file.write((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.write((char*)&rotation[i].x,4);
		file.write((char*)&rotation[i].y,4);
		file.write((char*)&rotation[i].z,4);
	}
	file.write((char*)&scale,4);
	file.write((char*)&collisionRef,4);
	file.write((char*)&dataRef,4);
	file.write((char*)&skinRef,4);
	file.write((char*)&numMat,4);
	for(i=0;i<numMat;i++) {
		materialNames[i].Put(file,2);
	}
	for(i=0;i<numMat;i++){
		file.write((char*)&materialExtra[i],4);
	}
	file.write((char*)&activeMat,4);
	file.write((char*)&dirty,1);
	if(hdr.userVer > 11) {
		file.write((char*)&propertiesRef1,4);
		file.write((char*)&propertiesRef2,4);
	}
}
void NifBlockTriStrips::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	int i;

	for(i=0;i<numExtra;i++) {
		if(extradata[i] == blockID) {
			extradata.erase(extradata.begin()+ i);
			i--;
			numExtra--;
			blockSize -= 4;
		} else if(extradata[i]>blockID) extradata[i]--;
	}

	if(controllerRef == blockID) controllerRef = -1;
	else if(controllerRef > blockID) controllerRef--;

	if(collisionRef == blockID) collisionRef = -1;
	else if(collisionRef > blockID) collisionRef--;

	if(dataRef == blockID) dataRef = -1;
	else if(dataRef > blockID) dataRef--;

	if(skinRef == blockID) skinRef = -1;
	else if(skinRef > blockID) skinRef--;

	if(propertiesRef1 >= blockID) propertiesRef1 --;
	if(propertiesRef2 >= blockID) propertiesRef2 --;
}


NifBlockTriStrips::NifBlockTriStrips() {
	blockType = NIFBLOCK_TRISHAPE;
	propertiesRef1=propertiesRef2=-1; controllerRef = -1;
	numExtra = 0;
	scale =1.0f;
	flags = 14;
	unk = 8;
	collisionRef = -1;
	dataRef = -1;
	skinRef = -1;
	numMat  = 0;
	activeMat = 0;
	dirty = 0;
	blockSize = 97;

}
NifBlockTriStrips::NifBlockTriStrips(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_TRISTRIPS;
	propertiesRef1=propertiesRef2=-1;
	Get(file,hdr);
}


void NifBlockTriStrips::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if(ver1 == 12) blockSize += 4;
	if(ver1 == 11) blockSize -= 4;
}

void NifBlockTriStripsData::Get(fstream& file, NifBlockHeader& hdr) {
	vector3 v;
	color4 c;
	vector2 uv;
	triangle tri;
	matchGroup mg;

	ushort uShort;
	int i;
	file.read((char*)&unknown,4);
	file.read((char*)&numverts,2);
	file.read((char*)&keepflags,1);
	file.read((char*)&compressflags,1);
	file.read((char*)&hasVertices,1);
	for(i=0;i<numverts;i++) {
		file.read((char*)&v.x,4);
		file.read((char*)&v.y,4);
		file.read((char*)&v.z,4);
		vertices.push_back(v);  
	}
	file.read((char*)&numUVs,2);
	if(hdr.userVer == 12) {
		file.read((char*)&unknown2,4);
		myver = 12;
	} else {
		myver = 11;
	}
	file.read((char*)&hasNormals,1);
	if(hasNormals == 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&v.x,4);
			file.read((char*)&v.y,4);
			file.read((char*)&v.z,4);
			normals.push_back(v);  
		}		
		if(numUVs & 0xF000) {
			for(i=0;i<numverts;i++) {
				file.read((char*)&v.x,4);
				file.read((char*)&v.y,4);
				file.read((char*)&v.z,4);
				tangents.push_back(v);  
			}		
			for(i=0;i<numverts;i++) {
				file.read((char*)&v.x,4);
				file.read((char*)&v.y,4);
				file.read((char*)&v.z,4);
				binormals.push_back(v);  
			}
		}
	}
	file.read((char*)&Center.x,4);
	file.read((char*)&Center.y,4);
	file.read((char*)&Center.z,4);
	file.read((char*)&Radius,4);
	file.read((char*)&hasVertColors,1);
	
	if(hasVertColors == 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&c,16);
			vertcolors.push_back(c);  
		}	
	}		
	if(numUVs >= 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&uv.u,4);
			file.read((char*)&uv.v,4);
			uvs.push_back(uv);  
		}	
	}
	file.read((char*)&consistencyflags,2);
	file.read((char*)&additionaldata,4);
	file.read((char*)&numTriangles,2);
	file.read((char*)&numStrips,2);
	for(i = 0;i<numStrips;i++ ) {
		file.read((char*)&uShort,2);
		stripLengths.push_back(uShort);
	}

	file.read((char*)&hasPoints,1);
	if(hasPoints) {
		for(i=0;i<numStrips;i++) {
			points.push_back(vector<ushort>());
			for(int j=0;j<stripLengths[i];j++) {
				file.read((char*)&uShort,2);
				points[i].push_back(uShort);
			}
		}
	}

}


void NifBlockTriStripsData::Put(fstream& file,NifBlockHeader& hdr ) {
	
	int i;
	file.write((char*)&unknown,4);
	file.write((char*)&numverts,2);
	file.write((char*)&keepflags,1);
	file.write((char*)&compressflags,1);
	file.write((char*)&hasVertices,1);
	for(i=0;i<numverts;i++) {
		file.write((char*)&vertices[i].x,4);
		file.write((char*)&vertices[i].y,4);
		file.write((char*)&vertices[i].z,4);
	}
	file.write((char*)&numUVs,2);
	if(hdr.userVer == 12) {
		file.write((char*)&unknown2,4);
	}
	file.write((char*)&hasNormals,1);
	if(hasNormals == 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&normals[i].x,4);
			file.write((char*)&normals[i].y,4);
			file.write((char*)&normals[i].z,4);
		}		
		if(numUVs & 0xF000) {
			for(i=0;i<numverts;i++) {
				file.write((char*)&tangents[i].x,4);
				file.write((char*)&tangents[i].y,4);
				file.write((char*)&tangents[i].z,4);
			}		
			for(i=0;i<numverts;i++) {
				file.write((char*)&binormals[i].x,4);
				file.write((char*)&binormals[i].y,4);
				file.write((char*)&binormals[i].z,4);
			}
		}
	}
	file.write((char*)&Center.x,4);
	file.write((char*)&Center.y,4);
	file.write((char*)&Center.z,4);
	file.write((char*)&Radius,4);
	file.write((char*)&hasVertColors,1);
	
	if(hasVertColors == 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&vertcolors[i],16);
		}	
	}		
	if(numUVs >= 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&uvs[i].u,4);
			file.write((char*)&uvs[i].v,4);
		}	
	}
	file.write((char*)&consistencyflags,2);
	file.write((char*)&additionaldata,4);
	file.write((char*)&numTriangles,2);
	file.write((char*)&numStrips,2);
	for(i=0;i<numStrips;i++) {
		file.write((char*)&stripLengths[i],2);
	}

	file.write((char*)&hasPoints,1);

	if(hasPoints) {
		for(i=0;i<numStrips;i++) {
			for(int j=0;j<stripLengths[i];j++ ){
				file.write((char*)&points[i][j],2);
			}
		}
	}
}

void NifBlockTriStripsData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<ushort>::const_iterator idx;
	vector<int> indexCollapse(vertices.size(), 0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = binormals.size() > 0;
	bool hasCol = vertcolors.size() > 0;
	bool hasUV = uvs.size() > 0;

	int remCount = 0;
	for (int i = 0, j = 0; i < indexCollapse.size(); i++) {
		if (j < vertIndices.size() && vertIndices[j] == i) {  // Found one to remove
			indexCollapse[i] = -1;  // Flag delete
			remCount++;
			j++;
		} else {
			indexCollapse[i] = remCount;
		}
	}

	for (int i = vertices.size()-1; i >= 0; i--) {
		if (indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numverts --;
			blockSize -= sizeof(vector3);
			if (hasNorm) {
				normals.erase(normals.begin() + i);
				blockSize -= sizeof(vector3);
			} 
			if (hasTan){
				tangents.erase(tangents.begin() + i);
				blockSize -= sizeof(vector3);
			} 
			if (hasBin){
				binormals.erase(binormals.begin() + i);
				blockSize -= sizeof(vector3);
			} 
			if (hasCol) {
				vertcolors.erase(vertcolors.begin() + i);
				blockSize -= sizeof(color4);
			}
			if (hasUV) {
				uvs.erase(uvs.begin() + i);
				blockSize -= sizeof(vector2);
			}
		}
	}

	// This is not a healthy way to delete strip data. Probably need to restrip the shape.
	for (int i = 0; i < numStrips; i++) {
		for (int j = 0; j < stripLengths[i]; j++) {
			if (indexCollapse[points[i][j]] == -1) {
				points[i].erase(points[i].begin() + j);
				stripLengths[i]--;
				blockSize -= 2;
			}
		}
	}
}

NifBlockTriStripsData::NifBlockTriStripsData() {
	blockType = NIFBLOCK_TRISTRIPSDATA;
	virtScale = 1.0f;
	scaleFromCenter = true;
	myver = 12;
}
NifBlockTriStripsData::NifBlockTriStripsData(fstream& file, NifBlockHeader &hdr) {
	blockType = NIFBLOCK_TRISTRIPSDATA;
	virtScale = 1.0f;
	scaleFromCenter = true;
	myver = 12;
	Get(file, hdr);
}

int NifBlockTriStripsData::CalcBlockSize() {
	if (myver == 12) 
		blockSize = 44;
	else 
		blockSize = 40;

	blockSize	+= numverts			 * 12;	//verts
	blockSize	+= normals.size()    * 12;	//normals
	blockSize	+= tangents.size()   * 12;	//tangents
	blockSize	+= binormals.size()  * 12;	//binormals
	blockSize	+= vertcolors.size() * 16;  //vertcolors
	blockSize	+= uvs.size()	     * 8;	//uvs
	blockSize	+= stripLengths.size() * 2;	//striplengths
	for (auto pl: points) {
		blockSize += pl.size() * 2;
	}

	return blockSize;
}

void NifBlockTriStripsData::StripsToTris(vector<triangle>* outTris) {		
	triangle t;

	for (int strip = 0; strip<numStrips; strip++) {
		for (int vi = 0; vi < stripLengths[strip] - 2; vi++) {
			if (vi & 1) {
				t.p1 = points[strip][vi];
				t.p2 = points[strip][vi + 2];
				t.p3 = points[strip][vi + 1];
			} else {
				t.p1 = points[strip][vi];
				t.p2 = points[strip][vi + 1];
				t.p3 = points[strip][vi + 2];
			}

			if (t.p1 == t.p2 || t.p2 == t.p3 || t.p3 == t.p1)
				continue;

			outTris->push_back(t);
		}
	}
}

void NifBlockTriStripsData::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if (ver1 == 12) {
		unknown2 = 0;
		blockSize += 4;
		myver = 12;
	} 
	if (ver1 == 11) {
		blockSize -= 4;
		myver = 11;
	}
}

void NifBlockTriShape::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	int intData;
	file.read((char*)&nameID,4);
	shapeName = hdr.strings[nameID].str;
	file.read((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.read((char*)&intData,4);
		extradata.push_back(intData);
	}
	file.read((char*)&controllerRef,4);
	file.read((char*)&flags,2);
	file.read((char*)&unk,2);
	file.read((char*)&translation.x,4);
	file.read((char*)&translation.y,4);
	file.read((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.read((char*)&rotation[i].x,4);
		file.read((char*)&rotation[i].y,4);
		file.read((char*)&rotation[i].z,4);
	}
	file.read((char*)&scale,4);
	if(hdr.userVer <= 11) {
		file.read((char*)&numProps,4);
		for(i = 0; i<numProps; i++) {
			file.read((char*)&intData,4);
			propRef.push_back(intData);
		}
	} else {
		numProps = 0;
	}
	file.read((char*)&collisionRef,4);
	file.read((char*)&dataRef,4);
	file.read((char*)&skinRef,4);
	file.read((char*)&numMat,4);
	for(i=0;i<numMat;i++) {
		materialNames.push_back(NifString(file,2));
	}
	for(i=0;i<numMat;i++){
		file.read((char*)&intData,4);
		materialExtra.push_back(intData);
	}
	file.read((char*)&activeMat,4);
	file.read((char*)&dirty,1);
	if(hdr.userVer > 11) {
		file.read((char*)&propertiesRef1,4);
		file.read((char*)&propertiesRef2,4);
	}
}


void NifBlockTriShape::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&nameID,4);
	//shapeName = hdr.strings[nameID].str;
	file.write((char*)&numExtra,4);
	for(i=0;i<numExtra;i++) {
		file.write((char*)&extradata[i],4);
	}
	file.write((char*)&controllerRef,4);
	file.write((char*)&flags,2);
	file.write((char*)&unk,2);
	file.write((char*)&translation.x,4);
	file.write((char*)&translation.y,4);
	file.write((char*)&translation.z,4);
	for(i=0;i<3;i++) {
		file.write((char*)&rotation[i].x,4);
		file.write((char*)&rotation[i].y,4);
		file.write((char*)&rotation[i].z,4);
	}
	file.write((char*)&scale,4);
	if(hdr.userVer <= 11) {
		file.write((char*)&numProps,4);
		for(i = 0; i<numProps; i++) {
			file.write((char*)&propRef[i],4);
		}
	}
	file.write((char*)&collisionRef,4);
	file.write((char*)&dataRef,4);
	file.write((char*)&skinRef,4);
	file.write((char*)&numMat,4);
	for(i=0;i<numMat;i++) {
		materialNames[i].Put(file,2);
	}
	for(i=0;i<numMat;i++){
		file.write((char*)&materialExtra[i],4);
	}
	file.write((char*)&activeMat,4);
	file.write((char*)&dirty,1);
	if(hdr.userVer > 11) {
		file.write((char*)&propertiesRef1,4);
		file.write((char*)&propertiesRef2,4);
	}
}
void NifBlockTriShape::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	int i;

	for(i=0;i<numExtra;i++) {
		if(extradata[i] == blockID) {
			extradata.erase(extradata.begin()+ i);
			i--;
			numExtra--;
			blockSize -= 4;
		} else if(extradata[i]>blockID) extradata[i]--;
	}

	if(controllerRef == blockID) controllerRef = -1;
	else if(controllerRef > blockID) controllerRef--;

	if(collisionRef == blockID) collisionRef = -1;
	else if(collisionRef > blockID) collisionRef--;

	if(dataRef == blockID) dataRef = -1;
	else if(dataRef > blockID) dataRef--;

	if(skinRef == blockID) skinRef = -1;
	else if(skinRef > blockID) skinRef--;

	for(i=0;i<numProps;i++) {
		if(propRef[i] == blockID) {
			propRef.erase (propRef.begin() + i);
			i--;
			numProps--;
			blockSize -= 4;
		} else if(propRef[i] > blockID) propRef[i]--;
	}
	if (propertiesRef1 >= blockID) propertiesRef1--;
	if (propertiesRef2 >= blockID) propertiesRef2--;
}

NifBlockTriShape::NifBlockTriShape() {
	blockType = NIFBLOCK_TRISHAPE;
	propertiesRef1=propertiesRef2=-1; controllerRef = -1;
	numExtra = 0;
	numProps = 0;
	scale = 1.0f;
	flags = 14;
	unk = 8;
	collisionRef = -1;
	dataRef = -1;
	skinRef = -1;
	numMat  = 0;
	activeMat = 0;
	dirty = 0;
	blockSize = 97;
	rotation[0].x = 0.0f;
	rotation[1].y = 0.0f;
	rotation[2].z = 0.0f;
}

NifBlockTriShape::NifBlockTriShape(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_TRISHAPE;
	propertiesRef1 = propertiesRef2 = -1;
	numProps = 0;
	Get(file, hdr);
}

void NifBlockTriShape::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if(ver1 == 12) blockSize += 4;
	if(ver1 == 11) blockSize -= 4;
}

void NifBlockTriShapeData::Get(fstream& file, NifBlockHeader& hdr) {
	vector3 v;
	color4 c;
	vector2 uv;
	triangle tri;
	matchGroup mg;

	ushort uShort;
	int i;
	file.read((char*)&unknown,4);
	file.read((char*)&numverts,2);
	file.read((char*)&keepflags,1);
	file.read((char*)&compressflags,1);
	file.read((char*)&hasVertices,1);
	for(i=0;i<numverts;i++) {
		file.read((char*)&v.x,4);
		file.read((char*)&v.y,4);
		file.read((char*)&v.z,4);
		vertices.push_back(v);  
	}
	file.read((char*)&numUVs,2);
	if(hdr.userVer == 12) {
		file.read((char*)&unknown2,4);
	}
	file.read((char*)&hasNormals,1);
	if(hasNormals == 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&v.x,4);
			file.read((char*)&v.y,4);
			file.read((char*)&v.z,4);
			normals.push_back(v);  
		}		
		if(numUVs & 0xF000) {
			for(i=0;i<numverts;i++) {
				file.read((char*)&v.x,4);
				file.read((char*)&v.y,4);
				file.read((char*)&v.z,4);
				tangents.push_back(v);  
			}		
			for(i=0;i<numverts;i++) {
				file.read((char*)&v.x,4);
				file.read((char*)&v.y,4);
				file.read((char*)&v.z,4);
				binormals.push_back(v);  
			}
		}
	}
	file.read((char*)&Center.x,4);
	file.read((char*)&Center.y,4);
	file.read((char*)&Center.z,4);
	file.read((char*)&Radius,4);
	file.read((char*)&hasVertColors,1);
	
	if(hasVertColors == 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&c,16);
			vertcolors.push_back(c);  
		}	
	}		
	if(numUVs >= 1) {
		for(i=0;i<numverts;i++) {
			file.read((char*)&uv.u,4);
			file.read((char*)&uv.v,4);
			uvs.push_back(uv);  
		}	
	}
	file.read((char*)&consistencyflags,2);
	file.read((char*)&additionaldata,4);
	file.read((char*)&numTriangles,2);
	file.read((char*)&numTriPoints,4);
	file.read((char*)&hasTriangles,1);
	if(hasTriangles == 1) {
		for(i=0;i<numTriangles;i++) {
			file.read((char*)&tri.p1,2);
			file.read((char*)&tri.p2,2);
			file.read((char*)&tri.p3,2);
			tris.push_back(tri);  
		}	
	}	
	file.read((char*)&numMatchGroups,2);
	for(i=0;i<numMatchGroups;i++) {
		file.read((char*)&mg.count,2);
		for(int j =0;j<mg.count;j++) {
			file.read((char*)&uShort,2);
			mg.matches.push_back(uShort);
		}
		matchGroups.push_back(mg);

	}
}


void NifBlockTriShapeData::Put(fstream& file,NifBlockHeader& hdr ) {
	
	int i;
	file.write((char*)&unknown,4);
	file.write((char*)&numverts,2);
	file.write((char*)&keepflags,1);
	file.write((char*)&compressflags,1);
	file.write((char*)&hasVertices,1);
	for(i=0;i<numverts;i++) {
		file.write((char*)&vertices[i].x,4);
		file.write((char*)&vertices[i].y,4);
		file.write((char*)&vertices[i].z,4);
	}
	file.write((char*)&numUVs,2);
	if(hdr.userVer == 12) {
		file.write((char*)&unknown2,4);
	}
	file.write((char*)&hasNormals,1);
	if(hasNormals == 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&normals[i].x,4);
			file.write((char*)&normals[i].y,4);
			file.write((char*)&normals[i].z,4);
		}		
		if(numUVs & 0xF000) {
			for(i=0;i<numverts;i++) {
				file.write((char*)&tangents[i].x,4);
				file.write((char*)&tangents[i].y,4);
				file.write((char*)&tangents[i].z,4);
			}		
			for(i=0;i<numverts;i++) {
				file.write((char*)&binormals[i].x,4);
				file.write((char*)&binormals[i].y,4);
				file.write((char*)&binormals[i].z,4);
			}
		}
	}
	file.write((char*)&Center.x,4);
	file.write((char*)&Center.y,4);
	file.write((char*)&Center.z,4);
	file.write((char*)&Radius,4);
	file.write((char*)&hasVertColors,1);
	
	if(hasVertColors == 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&vertcolors[i],16);
		}	
	}		
	if(numUVs >= 1) {
		for(i=0;i<numverts;i++) {
			file.write((char*)&uvs[i].u,4);
			file.write((char*)&uvs[i].v,4);
		}	
	}
	file.write((char*)&consistencyflags,2);
	file.write((char*)&additionaldata,4);
	file.write((char*)&numTriangles,2);
	file.write((char*)&numTriPoints,4);
	file.write((char*)&hasTriangles,1);
	if(hasTriangles == 1) {
		for(i=0;i<numTriangles;i++) {
			file.write((char*)&tris[i].p1,2);
			file.write((char*)&tris[i].p2,2);
			file.write((char*)&tris[i].p3,2);
		}	
	}	
	file.write((char*)&numMatchGroups,2);
	for(i=0;i<numMatchGroups;i++) {
		file.write((char*)&matchGroups[i].count,2);
		for(int j =0;j<matchGroups[i].count;j++) {
			file.write((char*)&matchGroups[i].matches[j],2);
		}

	}
}

void NifBlockTriShapeData::Create(vector<vec3>* verts, vector<triangle>* inTris, vector<vec2>* texcoords){
	unknown=0;
	vec3 a(FLT_MAX,FLT_MAX,FLT_MAX);
	vec3 b(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	
	Radius = 0.0f;
	numverts = verts->size();
	keepflags=0; compressflags=0; hasVertices=true; 
	for(auto v : (*verts)) {
		vertices.push_back(v);
			a.x = min(a.x, v.x); a.y = min(a.y, v.y); a.z = min(a.z, v.z);
			b.x = max(b.x, v.x); b.y = max(b.y, v.y); b.z = max(b.z, v.z);
	}	
	if(texcoords->size()>0) {
		numUVs = 4097;
	} else {
		numUVs = 0;
	}

	unknown2 = 0;
	hasNormals = false;
	Center =  (a + b ) / 2.0f;		
	for(auto v : (*verts)) {
		Radius = max(Radius,Center.DistanceTo(v));
	}
	hasVertColors = false;

	for(auto uv: (*texcoords) ){
		uvs.push_back(uv);
	}
	consistencyflags = 16384;
	additionaldata = -1;
	numTriangles = inTris->size();
	numTriPoints = numTriangles*3;
	hasTriangles = true;
	for(auto t: (*inTris)) {
		tris.push_back(t);
	}
	numMatchGroups = 0;

	blockSize =  48;
	blockSize += 12 * numverts;  // vert data size
	if(texcoords->size()>0) {
		blockSize += 8  * numverts;	 // uv data size
	}
	blockSize += 6  * numTriangles; // tri data size
}



void NifBlockTriShapeData::RecalcNormals() {
	if(!hasNormals) return;
	int j;
	vector3 norm;
	for(j=0;j<numverts;j++) {
		normals[j].x = 0;
		normals[j].y = 0;
		normals[j].z = 0;
	}
	for(j =0;j<numTriangles; j++) {
		// calc normal
		tris[j].trinormal(vertices,&norm);
		normals[tris[j].p1].x += norm.x;
		normals[tris[j].p1].y += norm.y;
		normals[tris[j].p1].z += norm.z;
		normals[tris[j].p2].x += norm.x;
		normals[tris[j].p2].y += norm.y;
		normals[tris[j].p2].z += norm.z;
		normals[tris[j].p3].x += norm.x;
		normals[tris[j].p3].y += norm.y;
		normals[tris[j].p3].z += norm.z;
	}
	// normalize all vertex normals to smooth them out.
	for(j=0 ;j<numverts; j++ ){
		normals[j].Normalize();
	}

	vtx* matchverts = new vtx[numverts];

	for(j=0;j<numverts;j++) {
		matchverts[j].x = vertices[j].x;
		matchverts[j].nx = normals[j].x;
		matchverts[j].y = vertices[j].y;
		matchverts[j].ny = normals[j].y;
		matchverts[j].z = vertices[j].z;
		matchverts[j].nz = normals[j].z;
	}
	
	kd_matcher matcher(matchverts, numverts);
	for(j=0; j<matcher.matches.size(); j++ ) {
		vtx* a = matcher.matches[j].first;
		vtx* b = matcher.matches[j].second;
		float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
		if(dot < 1.57079633f) {
			a->nx=((a->nx + b->nx) /2.0f);
			a->ny=((a->ny + b->ny) /2.0f);
			a->nz=((a->nz + b->nz) /2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}

	for(j=0;j<numverts;j++) {
		normals[j].x =matchverts[j].nx;
		normals[j].y =matchverts[j].ny;
		normals[j].z =matchverts[j].nz;
	}
	
	delete []matchverts;
}


void NifBlockTriShapeData::CalcTangentSpace() {
	if (!hasNormals) return;

	numUVs |= 4096;

	blockSize -= tangents.size() * 12;
	blockSize -= binormals.size() * 12;
	tangents.resize(numverts);
	binormals.resize(numverts);
	blockSize += tangents.size() * 12;
	blockSize += binormals.size() * 12;

	// int vertexCount = theMesh.vertexCount;
    // Vector3[] vertices = theMesh.vertices;
    // Vector3[] normals = theMesh.normals;
    // Vector2[] texcoords = theMesh.uv;
    // int[] triangles = theMesh.triangles;
    // int triangleCount = triangles.Length/3; 

    // Vector4[] tangents = new Vector4[vertexCount];
    // Vector3[] tan1 = new Vector3[vertexCount];
    // Vector3[] tan2 = new Vector3[vertexCount]; 
	vector<vec3> tan1;
	vector<vec3> tan2;
	tan1.resize(numverts);
	tan2.resize(numverts);

       // int tri = 0; 

        for (int i = 0; i < (numTriangles); i++) {
            int i1 = tris[i].p1;
            int i2 = tris[i].p2;
            int i3 = tris[i].p3; 

            vec3 v1 = vertices[i1];
            vec3 v2 = vertices[i2];
            vec3 v3 = vertices[i3]; 

            vec2 w1 = uvs[i1];
            vec2 w2 = uvs[i2];
            vec2 w3 = uvs[i3]; 

            float x1 = v2.x - v1.x;
            float x2 = v3.x - v1.x;
            float y1 = v2.y - v1.y;
            float y2 = v3.y - v1.y;
            float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z; 

            float s1 = w2.u - w1.u;
            float s2 = w3.u - w1.u;
            float t1 = w2.v - w1.v;
            float t2 = w3.v - w1.v; 

            float r = 1.0f / (s1 * t2 - s2 * t1);
			r = (r >= 0 ? +1 : -1);

            vec3 sdir = vec3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
            vec3 tdir = vec3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r); 

            tan1[i1] += sdir;
            tan1[i2] += sdir;
            tan1[i3] += sdir; 

            tan2[i1] += tdir;
            tan2[i2] += tdir;
			tan2[i3] += tdir; 

           // tri ++;
        }

        for (int i = 0; i < (numverts); i++) {
            vec3 n = normals[i];
            vec3 t = tan1[i]; 

			// Gram-Schmidt orthogonalize
			// Vector3.OrthoNormalize(ref n, ref t);
            binormals[i].x  = t.x;
            binormals[i].y  = t.y;
            binormals[i].z  = t.z;
			binormals[i].Normalize();

			tangents[i] = binormals[i].cross(normals[i]);
			tangents[i].Normalize();
        }
        //theMesh.tangents = tangents; 
}


void NifBlockTriShapeData::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	vector<ushort>::const_iterator idx;
	vector<int> indexCollapse(vertices.size(),0);
	bool hasNorm = normals.size() > 0;
	bool hasTan = tangents.size() > 0;
	bool hasBin = binormals.size() > 0;
	bool hasCol = vertcolors.size() > 0;
	bool hasUV = uvs.size() > 0;

	int remCount = 0;
	for (int i = 0, j=0; i<indexCollapse.size(); i++ ) {
		if(j < vertIndices.size() && vertIndices[j]==i) {			// found one to remove
			indexCollapse[i] = -1;		// flag delete
			remCount++;
			j++;
		} else {
			indexCollapse[i] = remCount;
		}
	}

	for (int i = vertices.size()-1; i>=0; i--) {
		if(indexCollapse[i] == -1) {
			vertices.erase(vertices.begin() + i);
			numverts --;
			blockSize -= sizeof(vector3);
			if(hasNorm) {
				normals.erase(normals.begin() + i);
			blockSize -= sizeof(vector3);
			} 
			if(hasTan){
				tangents.erase(tangents.begin() + i);
				blockSize -= sizeof(vector3);
			} 
			if(hasBin){
				binormals.erase(binormals.begin() + i);
				blockSize -= sizeof(vector3);
			} 
			if(hasCol) {
				vertcolors.erase(vertcolors.begin() + i);
				blockSize -= sizeof(color4);
			}
			if(hasUV) {
				uvs.erase(uvs.begin() + i);
				blockSize -= sizeof(vector2);
			}
		}

	}
	for (int i = numTriangles-1; i>=0; i--) {

		if(indexCollapse[tris[i].p1] == -1 ||
			indexCollapse[tris[i].p2] == -1 ||
			indexCollapse[tris[i].p3] == -1) {
			tris.erase(tris.begin() + i);
			numTriangles --;
			numTriPoints -=3;
			blockSize -= sizeof(triangle);
		} else {
			tris[i].p1 = tris[i].p1 - indexCollapse[tris[i].p1];			
			tris[i].p2 = tris[i].p2 - indexCollapse[tris[i].p2];
			tris[i].p3 = tris[i].p3 - indexCollapse[tris[i].p3];
		}
	}
}	

int NifBlockTriShapeData::CalcBlockSize() {
	if(myver == 12) 
		blockSize = 48;
	else 
		blockSize = 44;
	blockSize	+= numverts			 * 12;	//verts
	blockSize	+= normals.size()    * 12;	//normals
	blockSize	+= tangents.size()   * 12;	//tangents
	blockSize	+= binormals.size()  * 12;	//binormals
	blockSize	+= vertcolors.size() * 16;  //vertcolors
	blockSize	+= uvs.size()	     * 8;	//uvs 
	blockSize	+= tris.size()		 * 6;	//tris
	for(auto mg : matchGroups) {
		blockSize += 4;
		blockSize += mg.matches.size() * 2;
	}

	return blockSize;
}

NifBlockTriShapeData::NifBlockTriShapeData() {
	blockType = NIFBLOCK_TRISHAPEDATA;
	scaleFromCenter = true;
	virtScale = 1.0f;
	myver = 12;
}
NifBlockTriShapeData::NifBlockTriShapeData(fstream& file, NifBlockHeader &hdr) {
	blockType = NIFBLOCK_TRISHAPEDATA;
	scaleFromCenter = true;
	virtScale = 1.0f;
	myver = 12;
	Get(file, hdr);
}

void NifBlockTriShapeData::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if (ver1 == 12) {
		unknown2 = 0;
		blockSize += 4;
	} 
	if (ver1 == 11) {
		blockSize -= 4;
	}
}
 
void NifBlockTriStripsData::RecalcNormals() {
	if (!hasNormals) return;
	int j;
	vector3 norm;
	for (j = 0; j < numverts; j++) {
		normals[j].x = 0;
		normals[j].y = 0;
		normals[j].z = 0;
	}

	vector<triangle> tris;
	StripsToTris(&tris);

	for(j = 0; j < tris.size(); j++) {
		// calc normal
		tris[j].trinormal(vertices,&norm);
		normals[tris[j].p1].x += norm.x;
		normals[tris[j].p1].y += norm.y;
		normals[tris[j].p1].z += norm.z;
		normals[tris[j].p2].x += norm.x;
		normals[tris[j].p2].y += norm.y;
		normals[tris[j].p2].z += norm.z;
		normals[tris[j].p3].x += norm.x;
		normals[tris[j].p3].y += norm.y;
		normals[tris[j].p3].z += norm.z;
	}
	// normalize all vertex normals to smooth them out.
	for (j = 0; j < numverts; j++) {
		normals[j].Normalize();
	}

	vtx* matchverts = new vtx[numverts];

	for (j = 0; j < numverts; j++) {
		matchverts[j].x = vertices[j].x;
		matchverts[j].nx = normals[j].x;
		matchverts[j].y = vertices[j].y;
		matchverts[j].ny = normals[j].y;
		matchverts[j].z = vertices[j].z;
		matchverts[j].nz = normals[j].z;
	}
	
	kd_matcher matcher(matchverts, numverts);
	for (j = 0; j < matcher.matches.size(); j++) {
		vtx* a = matcher.matches[j].first;
		vtx* b = matcher.matches[j].second;
		float dot = (a->nx * b->nx + a->ny*b->ny + a->nz*b->nz);
		if (dot < 1.57079633f) {
			a->nx=((a->nx + b->nx) /2.0f);
			a->ny=((a->ny + b->ny) /2.0f);
			a->nz=((a->nz + b->nz) /2.0f);
			b->nx = a->nx;
			b->ny = a->ny;
			b->nz = a->nz;
		}
	}

	for (j = 0; j < numverts; j++) {
		normals[j].x =matchverts[j].nx;
		normals[j].y =matchverts[j].ny;
		normals[j].z =matchverts[j].nz;
	}
	
	delete []matchverts;
}

void NifBlockTriStripsData::CalcTangentSpace() {
	if (!hasNormals) return;

	numUVs |= 4096;

	blockSize -= tangents.size() * 12;
	blockSize -= binormals.size() * 12;
	tangents.resize(numverts);
	binormals.resize(numverts);
	blockSize += tangents.size() * 12;
	blockSize += binormals.size() * 12;

	  //  int vertexCount = theMesh.vertexCount;
      //  Vector3[] vertices = theMesh.vertices;
      //  Vector3[] normals = theMesh.normals;
      //  Vector2[] texcoords = theMesh.uv;
      //  int[] triangles = theMesh.triangles;
     //   int triangleCount = triangles.Length/3; 

      //  Vector4[] tangents = new Vector4[vertexCount];
      //  Vector3[] tan1 = new Vector3[vertexCount];
      //  Vector3[] tan2 = new Vector3[vertexCount]; 
	vector<vec3> tan1;
	vector<vec3> tan2;
	tan1.resize(numverts);
	tan2.resize(numverts);

	vector<triangle> tris;
	StripsToTris(&tris);
       // int tri = 0; 

        for (int i = 0; i < (tris.size()); i++) {
            int i1 = tris[i].p1;
            int i2 = tris[i].p2;
            int i3 = tris[i].p3; 

            vec3 v1 = vertices[i1];
            vec3 v2 = vertices[i2];
            vec3 v3 = vertices[i3]; 

            vec2 w1 = uvs[i1];
            vec2 w2 = uvs[i2];
            vec2 w3 = uvs[i3]; 

            float x1 = v2.x - v1.x;
            float x2 = v3.x - v1.x;
            float y1 = v2.y - v1.y;
            float y2 = v3.y - v1.y;
            float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z; 

            float s1 = w2.u - w1.u;
            float s2 = w3.u - w1.u;
            float t1 = w2.v - w1.v;
            float t2 = w3.v - w1.v; 

            float r = 1.0f / (s1 * t2 - s2 * t1);
			r = (r >= 0 ? +1 : -1);

            vec3 sdir = vec3((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
            vec3 tdir = vec3((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r); 

            tan1[i1] += sdir;
            tan1[i2] += sdir;
            tan1[i3] += sdir; 

            tan2[i1] += tdir;
            tan2[i2] += tdir;
			tan2[i3] += tdir; 

           // tri ++;
        }

        for (int i = 0; i < (numverts); i++) {
            vec3 n = normals[i];
            vec3 t = tan1[i]; 

			// Gram-Schmidt orthogonalize
			// Vector3.OrthoNormalize(ref n, ref t);
            binormals[i].x  = t.x;
            binormals[i].y  = t.y;
            binormals[i].z  = t.z;
			binormals[i].Normalize();

			tangents[i] = binormals[i].cross(normals[i]);
			tangents[i].Normalize();
        }
        //theMesh.tangents = tangents; 
}

void NifBlockNiSkinData::Get(fstream& file, NifBlockHeader& hdr) {
	BoneData boneData;
	skin_weight sWt;

	file.read((char*)&rootTransform,sizeof(skin_transform));
	file.read((char*)&numBones,sizeof(uint));
	file.read((char*)&hasVertWeights,sizeof(BYTE));

	for(int i = 0;i<numBones;i++) {
		boneData.skin_weights.clear();
		
		file.read((char*)&boneData.boneTransform, sizeof(skin_transform));
		file.read((char*)&boneData.boundSphereOffset, sizeof(vector3));
		file.read((char*)&boneData.boundSphereRadius, sizeof(float));
		file.read((char*)&boneData.numVerts, sizeof(ushort));
		
		for(int j =0;j<boneData.numVerts;j++) {			
			file.read((char*)&sWt.index, sizeof(ushort));
			file.read((char*)&sWt.weight, sizeof(float));
			boneData.skin_weights.push_back(sWt);		
		}

		Bones.push_back(boneData);
	}
}

void NifBlockNiSkinData::Put(fstream& file, NifBlockHeader& hdr) {
	file.write((char*)&rootTransform,sizeof(skin_transform));
	file.write((char*)&numBones,sizeof(uint));
	file.write((char*)&hasVertWeights,sizeof(BYTE));

	for(int i = 0 ;i <numBones;i++) {
		file.write((char*)&Bones[i].boneTransform,sizeof(skin_transform));
		file.write((char*)&Bones[i].boundSphereOffset,sizeof(vector3));
		file.write((char*)&Bones[i].boundSphereRadius,sizeof(float));
		file.write((char*)&Bones[i].numVerts, sizeof(ushort));

		for(int j =0;j<Bones[i].numVerts;j++) {
			file.write((char*)&Bones[i].skin_weights[j].index,sizeof(ushort));
			file.write((char*)&Bones[i].skin_weights[j].weight,sizeof(float));
		}
	}
}
void NifBlockNiSkinData::notifyVerticesDelete(const vector<ushort>& vertIndices){
	ushort highestRemoved  = vertIndices[vertIndices.size()-1];
	vector<int> indexCollapse(highestRemoved+1,0);
	
//	vector<int> indexCollapse(4000,0);
	
	int remCount = 0;
	for (int i = 0, j=0; i<indexCollapse.size(); i++ ) {
		if(j<vertIndices.size() && vertIndices[j]==i) {			// found one to remove
			indexCollapse[i] = -1;		// flag delete
			remCount++;
			j++;
		} else {
			indexCollapse[i] = remCount;
		}
	}	
	vector<BoneData>::iterator B;
	ushort ival;
	for(B=Bones.begin();B!=Bones.end();++B) {
		for(int i = B->numVerts -1 ; i>=0; i--) {
			ival =B->skin_weights[i].index;
			if(B->skin_weights[i].index > highestRemoved) {
				B->skin_weights[i].index -= remCount;
			} else 
			if(indexCollapse[ival] ==-1) {
				B->skin_weights.erase(B->skin_weights.begin()+i);
				B->numVerts --;
				blockSize -= 6;
			} else {
				B->skin_weights[i].index -= indexCollapse[ival];
			}				
		}
	}
}

NifBlockNiSkinData::NifBlockNiSkinData() {
	blockType = NIFBLOCK_NISKINDATA;
	rootTransform.scale = 1.0f;
	numBones = 0;
	hasVertWeights = 1;
	blockSize = 57;
}

NifBlockNiSkinData::NifBlockNiSkinData(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_NISKINDATA ;
	Get(file, hdr);
}
	
int NifBlockNiSkinData::CalcBlockSize() {
	blockSize = 57;
	for (auto bd: Bones) {
		blockSize += bd.CalcSize();
	}
	return blockSize;
}

int NifBlockNiSkinPartition::CalcBlockSize() {
	int bsize = 4;
	for(auto p : partitionBlocks) {
		bsize += 16;					// plain data size;
		bsize += 2  * p.numBones;		// bonelist size;
		bsize += 2  * p.numVertices;	// vertmap size;
		bsize += 4  * p.numVertices;	// boneindex size;
		bsize += 16 * p.numVertices;	// vertexWeights size;
		bsize += 2  * p.numStrips;		// striplengths size;
		for(int i =0;i<p.numStrips;i++) {
			bsize += 2 * p.stripLengths[i];		// striplist size;
		}
		if(p.numStrips == 0) {
			bsize += 6  * p.numTriangles;	// triangle list size;
		}
	}
	blockSize = bsize;
	return bsize;
}
void NifBlockNiSkinPartition::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	PartitionBlock partBlock;
	ushort uShort;
	vertexWeight wt;
	boneIndices bi;
	triangle tri;

	file.read((char*)&numPartitions,4);
	for(int p = 0;p<numPartitions;p++) {
		partBlock.bones.clear();
		partBlock.vertMap.clear();
		partBlock.vertWeights.clear();
		partBlock.stripLengths.clear();
		partBlock.strips.clear();
		partBlock.tris.clear();
		partBlock.boneindex.clear();

		file.read((char*)&partBlock.numVertices,2);
		file.read((char*)&partBlock.numTriangles,2);
		file.read((char*)&partBlock.numBones,2);
		file.read((char*)&partBlock.numStrips,2);
		file.read((char*)&partBlock.numWeightsPerVert,2);
		for(i=0;i<partBlock.numBones;i++) {
			file.read((char*)&uShort,2);
			partBlock.bones.push_back(uShort);
		}
		file.read((char*)&partBlock.hasVertMap,1);
		for(i=0;(i<partBlock.numVertices) && partBlock.hasVertMap;i++) {
			file.read((char*)&uShort,2);
			partBlock.vertMap.push_back(uShort);
		}
		file.read((char*)&partBlock.hasVertWeights,1);
		for(i=0;(i<partBlock.numVertices) && partBlock.hasVertWeights;i++) {
			file.read((char*)&wt.w1,4);
			file.read((char*)&wt.w2,4);
			file.read((char*)&wt.w3,4);
			file.read((char*)&wt.w4,4);
			partBlock.vertWeights.push_back(wt);
		}
		for(i=0;i<partBlock.numStrips;i++) {
			file.read((char*)&uShort,2);
			partBlock.stripLengths.push_back(uShort);
		}
		file.read((char*)&partBlock.hasFaces,1);
		for(i=0;(i<partBlock.numStrips) && partBlock.hasFaces;i++) {
			partBlock.strips.push_back(vector<ushort>());
			for(int j=0;j<partBlock.stripLengths[i]; j++) {
				file.read((char*)&uShort,2);
				partBlock.strips[i].push_back(uShort);
			}

		}
		if(partBlock.numStrips == 0) {
			for(i=0;(i<partBlock.numTriangles)&&partBlock.hasFaces;i++) {
				file.read((char*)&tri.p1,2);
				file.read((char*)&tri.p2,2);
				file.read((char*)&tri.p3,2);
				partBlock.tris.push_back(tri);
			}
		}
		file.read((char*)&partBlock.hasBoneIndices,1);
		for(i=0;i<partBlock.numVertices  && partBlock.hasBoneIndices; i++ ){
			file.read((char*)&bi.i1,1);
			file.read((char*)&bi.i2,1);
			file.read((char*)&bi.i3,1);
			file.read((char*)&bi.i4,1);
			partBlock.boneindex.push_back(bi);
		}
		if(hdr.userVer == 12) {
			file.read((char*)&partBlock.unknown,2);
		}

		partitionBlocks.push_back(partBlock);
	}
}

void NifBlockNiSkinPartition::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&numPartitions,4);
	for(int p = 0;p<numPartitions;p++) {
		file.write((char*)&partitionBlocks[p].numVertices,2);
		file.write((char*)&partitionBlocks[p].numTriangles,2);
		file.write((char*)&partitionBlocks[p].numBones,2);
		file.write((char*)&partitionBlocks[p].numStrips,2);
		file.write((char*)&partitionBlocks[p].numWeightsPerVert,2);
		for(i=0;i<partitionBlocks[p].numBones;i++) {
			file.write((char*)&partitionBlocks[p].bones[i],2);
		}
		file.write((char*)&partitionBlocks[p].hasVertMap,1);
		for(i=0;(i<partitionBlocks[p].numVertices) && partitionBlocks[p].hasVertMap;i++) {
			file.write((char*)&partitionBlocks[p].vertMap[i],2);
		}
		file.write((char*)&partitionBlocks[p].hasVertWeights,1);
		for(i=0;(i<partitionBlocks[p].numVertices) && partitionBlocks[p].hasVertWeights;i++) {
			file.write((char*)&partitionBlocks[p].vertWeights[i],16);
		}
		for(i=0;i<partitionBlocks[p].numStrips;i++) {
			file.write((char*)&partitionBlocks[p].stripLengths[i],2);
		}
		file.write((char*)&partitionBlocks[p].hasFaces,1);
		for(i=0;(i<partitionBlocks[p].numStrips) && partitionBlocks[p].hasFaces;i++) {
			for(int j=0;j<partitionBlocks[p].stripLengths[i];j++) {
				file.write((char*)&partitionBlocks[p].strips[i][j],2);
			}
		}
		if(partitionBlocks[p].numStrips == 0) {
			for(i=0;(i<partitionBlocks[p].numTriangles)&&partitionBlocks[p].hasFaces;i++) {
				file.write((char*)&partitionBlocks[p].tris[i].p1,6);
			}
		}
		file.write((char*)&partitionBlocks[p].hasBoneIndices,1);
		for(i=0;i<partitionBlocks[p].numVertices  && partitionBlocks[p].hasBoneIndices; i++ ){
			file.write((char*)&partitionBlocks[p].boneindex[i],4);
		}
		if(hdr.userVer == 12) {
			file.write((char*)&partitionBlocks[p].unknown,2);
		}
	}
}

void NifBlockNiSkinPartition::notifyVerticesDelete(const vector<ushort>& vertIndices) {
	ushort highestRemoved  = vertIndices.back();
	vector<int> indexCollapse(highestRemoved+1,0);
	int remCount = 0;
	for (int i = 0, j=0; i<indexCollapse.size(); i++ ) {
		if(j<vertIndices.size() && vertIndices[j]==i) {			// found one to remove
			indexCollapse[i] = -1;		// flag delete
			remCount++;
			j++;
		} else {
			indexCollapse[i] = remCount;
		}
	}	

	ushort ival;
	vector<PartitionBlock>::iterator P;
		size_t maxVertMapSz = 0;
	for(P=partitionBlocks.begin();P!=partitionBlocks.end();++P) {
		if(P->vertMap.size() >  maxVertMapSz) maxVertMapSz = P->vertMap.size();
	}
	vector<int> mapCollapse (maxVertMapSz,0);
	int mapRemCount = 0;
	int mapHighestRemoved = maxVertMapSz;


	for(P=partitionBlocks.begin();P!=partitionBlocks.end();++P) {
		mapRemCount = 0;
		mapHighestRemoved = maxVertMapSz;
		for (int i = 0, j=0; i<P->vertMap.size(); i++ ) {
			if(P->vertMap[i] < indexCollapse.size() && indexCollapse[P->vertMap[i]] == -1) {
				mapRemCount++;
				mapCollapse[i] = -1;
				mapHighestRemoved = i;
			} else {
				mapCollapse[i] = mapRemCount;
			}
		}
		for(int i = P->vertMap.size()-1;i>=0; i--) {
			ival = P->vertMap[i];
			if(ival>highestRemoved) {
				P->vertMap[i] -= remCount;
			} else 
			if(indexCollapse[ival] == -1) {
				P->vertMap.erase(P->vertMap.begin() + i);
				P->numVertices--;
				this->blockSize -= sizeof(ushort);
				if(P->hasVertWeights) {
					P->vertWeights.erase(P->vertWeights.begin() + i);
					blockSize -= sizeof(vertexWeight);
				} if(P->hasBoneIndices)
					P->boneindex.erase(P->boneindex.begin() + i);
					blockSize -= sizeof(boneIndices);
			} else {
				P->vertMap[i] -= indexCollapse[ival];
			}
		}
		
		for (int i = P->numTriangles-1; i>=0; i--) {
			if(P->tris[i].p1 > mapHighestRemoved) {
				P->tris[i].p1 -= mapRemCount;
			} else 
			if(mapCollapse[P->tris[i].p1] == -1) {
				P->tris.erase(P->tris.begin() + i);
				P->numTriangles --;
				blockSize -= sizeof(triangle);
				continue;
			} else {
				P->tris[i].p1 -= mapCollapse[P->tris[i].p1];
			}
			if(P->tris[i].p2 > mapHighestRemoved) {
				P->tris[i].p2 -= mapRemCount;
			} else 
			if(mapCollapse[P->tris[i].p2] == -1) {
				P->tris.erase(P->tris.begin() + i);
				P->numTriangles --;
				blockSize -= sizeof(triangle);
				continue;
			} else {
				P->tris[i].p2 -= mapCollapse[P->tris[i].p2];
			}				
			if(P->tris[i].p3 > mapHighestRemoved) {
				P->tris[i].p3 -= mapRemCount;
			} else 
			if(mapCollapse[P->tris[i].p3] == -1) {
				P->tris.erase(P->tris.begin() + i);
				P->numTriangles --;
				blockSize -= sizeof(triangle);
				continue;
			} else {
				P->tris[i].p3 -= mapCollapse[P->tris[i].p3];
			}
		}
	}
}

int NifBlockNiSkinPartition::RemoveEmptyPartitions(vector<int>& outDeletedIndices) {
	outDeletedIndices.clear();
	for (int i = partitionBlocks.size() - 1; i >= 0; i--) {
		if (partitionBlocks[i].numVertices == 0) {
			blockSize -= (16 + 2 * partitionBlocks[i].numBones);
			outDeletedIndices.insert(outDeletedIndices.begin(), i);
			partitionBlocks.erase(partitionBlocks.begin() + i);
			numPartitions--;
		}
	}
	return outDeletedIndices.size();
}

NifBlockNiSkinPartition::NifBlockNiSkinPartition() {
	blockType = NIFBLOCK_NISKINPARTITION;
	numPartitions = 0;
	blockSize = 4;
	needsBuild = true;
}

NifBlockNiSkinPartition::NifBlockNiSkinPartition(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_NISKINPARTITION;
	Get(file, hdr);
	needsBuild = false;
}

void NifBlockNiSkinPartition::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	if (ver1 == 12)  {
		for (int i = 0; i < numPartitions; i++) {
			partitionBlocks[i].unknown = 0;
			blockSize += 2;
		}
	}
	if (ver1 == 11) {
		for (int i = 0; i < numPartitions; i++) {
			//partitionBlocks[i].unknown = 0;
			blockSize -= 2;
		}
	}
}

void NifBlockNiSkinInstance::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	unsigned int uInt;
	file.read((char*)&dataRef, 4);
	file.read((char*)&skinRef, 4);
	file.read((char*)&skeletonRoot, 4);
	file.read((char*)&numBones, 4);
	for (i = 0; i < numBones; i++) {		
		file.read((char*)&uInt, 4);
		bonePtrs.push_back(uInt);
	}
}

void NifBlockNiSkinInstance::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&dataRef, 4);
	file.write((char*)&skinRef, 4);
	file.write((char*)&skeletonRoot, 4);
	file.write((char*)&numBones, 4);
	for (i = 0; i < numBones; i++) {
		file.write((char*)&bonePtrs[i], 4);
	}
}

NifBlockNiSkinInstance::NifBlockNiSkinInstance() {
	blockType = NIFBLOCK_NISKININSTANCE;
	dataRef = -1; skinRef = -1; skeletonRoot = -1; 
	numBones = 0;
	blockSize = 16;
}

NifBlockNiSkinInstance::NifBlockNiSkinInstance(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_NISKININSTANCE;
	Get(file, hdr);
}

int NifBlockNiSkinInstance::CalcBlockSize() {
	blockSize = 16;
	blockSize += numBones * 4;
	return blockSize;
}

void NifBlockNiSkinInstance::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	int boneIndex = -1;
	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinRef == blockID)
		skinRef = -1;
	else if (skinRef > blockID)
		skinRef--;

	if (skeletonRoot == blockID)
		skeletonRoot = -1;
	else if (skeletonRoot > blockID)
		skeletonRoot--;

	for (int i = 0; i < numBones; i++) {
		if (bonePtrs[i] == blockID) {
			bonePtrs.erase(bonePtrs.begin() + i);
			i--;
			boneIndex = i;
			numBones--;
			blockSize -= 4;
		}
		else if (bonePtrs[i] > blockID)
			bonePtrs[i]--;
	}
	if (boneIndex >= 0 && dataRef != -1) { // bone was removed, clear out the skinning data for it.
		if (hdr.blocks == NULL)
			return;

		NifBlockNiSkinData* skinData = dynamic_cast<NifBlockNiSkinData*>((*hdr.blocks)[dataRef]);
		if (!skinData)
			return;

		skinData->Bones.erase(skinData->Bones.begin() + boneIndex);
		skinData->CalcBlockSize();
	}
}

void NifBlockBSDismemberment::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	unsigned int uInt;
	Partition Part;
	file.read((char*)&dataRef, 4);
	file.read((char*)&skinRef, 4);
	file.read((char*)&skeletonRoot, 4);
	file.read((char*)&numBones, 4);
	for (i = 0; i < numBones; i++) {		
		file.read((char*)&uInt, 4);
		bonePtrs.push_back(uInt);
	}
	file.read((char*)&numPartitions,4);
	for (i = 0; i < numPartitions; i++) {
		file.read((char*)&Part.flags, 2);
		file.read((char*)&Part.partID, 2);
		partitions.push_back(Part);
	}
}

void NifBlockBSDismemberment::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&dataRef, 4);
	file.write((char*)&skinRef, 4);
	file.write((char*)&skeletonRoot, 4);
	file.write((char*)&numBones, 4);
	for(i=0;i<numBones;i++) {
		file.write((char*)&bonePtrs[i], 4);
	}
	file.write((char*)&numPartitions, 4);
	for (i = 0; i < numPartitions; i++) {
		file.write((char*)&partitions[i].flags, 2);		
		file.write((char*)&partitions[i].partID, 2);
	}
}

NifBlockBSDismemberment::NifBlockBSDismemberment() {
	blockType = NIFBLOCK_BSDISMEMBER;
	dataRef = -1; skinRef = -1; skeletonRoot = -1; 
	numBones = 0;
	numPartitions = 0;
	blockSize = 20;
}

NifBlockBSDismemberment::NifBlockBSDismemberment(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_BSDISMEMBER;
	Get(file, hdr);
}

int NifBlockBSDismemberment::CalcBlockSize() {
	blockSize = 20;
	blockSize += numBones * 4;
	blockSize += numPartitions * 4;
	return blockSize;
}

void NifBlockBSDismemberment::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	int boneIndex = -1;
	if (dataRef == blockID)
		dataRef = -1;
	else if (dataRef > blockID)
		dataRef--;

	if (skinRef == blockID)
		skinRef = -1;
	else if (skinRef > blockID)
		skinRef--;

	if (skeletonRoot == blockID)
		skeletonRoot = -1;
	else if (skeletonRoot > blockID)
		skeletonRoot--;

	for (int i = 0; i < numBones; i++) {
		if (bonePtrs[i] == blockID) {
			bonePtrs.erase(bonePtrs.begin() + i);
			boneIndex = i;
			i--;
			numBones--;
			blockSize -= 4;
		}
		else if (bonePtrs[i] > blockID)
			bonePtrs[i]--;
	}
	if (boneIndex >= 0 && dataRef != -1) { // bone was removed, clear out the skinning data for it.
		if (hdr.blocks == NULL)
			return;

		NifBlockNiSkinData* skinData = dynamic_cast<NifBlockNiSkinData*>((*hdr.blocks)[dataRef]);
		if (!skinData)
			return;

		skinData->Bones.erase(skinData->Bones.begin() + boneIndex);
		skinData->CalcBlockSize();
	}
}
	
void NifBlockBSDismemberment::notifyVersionChange(unsigned int ver1, unsigned int ver2) {
	for (int i = 0; i < numPartitions; i++) {
		if (ver1 == 12) {
			if (partitions[i].partID == 0) partitions[i].partID = 32;
			if (partitions[i].partID == 7) partitions[i].partID = 38;   /// HACK: 38 is both legs...
			if (partitions[i].partID == 3) partitions[i].partID = 34;   /// HACK: 34 is both arms...
		} 
	}
}

NifBlockHeader::NifBlockHeader() {
	blockSize = 0;
	numBlocks = 0;
	numStrings = 0;
	blocks = NULL;
	blockType = NIFBLOCK_HEADER;
}
void NifBlockHeader::Clear() {
	numBlockTypes = 0;
	numStrings = 0;
	numBlocks = 0;
	blocks = NULL;
	blockTypes.clear();
	blockIndex.clear();
	blockSizes.clear();
	strings.clear();
}

void NifBlockHeader::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	unsigned short uShort;
	unsigned int uInt;
	file.read(verStr, 38);
	if (_strnicmp(verStr, "Gamebryo", 8) != 0) {
		verStr[0] = 0;
		return;
	}

	//file >> unk1; 
	unk1 = 10;
	file >> ver1 >> ver2 >> ver3 >> ver4;
	file >> endian;
	file.read((char*)&userVer, 4);
	file.read((char*)&numBlocks, 4);
	file.read((char*)&userVer2, 4);
	creator.Get(file, 1);
	exportInfo1.Get(file, 1);
	exportInfo2.Get(file, 1);
	file.read((char*)&numBlockTypes, 2);
	for (i = 0; i < numBlockTypes; i++) {
		blockTypes.push_back(NifString(file, 4));
	}
	for (i = 0; i < numBlocks; i++) {
		file.read((char*)&uShort,2);
		blockIndex.push_back(uShort);
	}
	if (VerCheck(20, 2, 0, 7)) {
		for (i = 0; i < numBlocks; i++) {
			file.read((char*)&uInt, 4);
			blockSizes.push_back(uInt);
		}
	} else {
		for (i = 0; i < numBlocks; i++) {
			blockSizes.push_back(0);
		}
	}

	if (VerCheck(20, 1, 0, 3)) {
		file.read((char*)&numStrings, 4);
		file.read((char*)&maxStringLen, 4);
		for (i = 0; i < numStrings; i++) {
			strings.push_back(NifString(file, 4));
		}
	}
	file.read((char*)&unk2, 4);
}

void NifBlockHeader::Put(fstream& file, NifBlockHeader& hdr) {
	int i;

	file.write(verStr, 0x26);
	file << unk1;
	file << ver1 << ver2 << ver3 << ver4;
	file << endian;
	file.write((char*)&userVer, 4);
	file.write((char*)&numBlocks, 4);
	file.write((char*)&userVer2, 4);
	creator.Put(file, 1);
	exportInfo1.Put(file, 1);
	exportInfo2.Put(file, 1);
	file.write((char*)&numBlockTypes, 2);
	for (i = 0; i < numBlockTypes; i++) {
		blockTypes[i].Put(file, 4);
	}
	for (i = 0; i < numBlocks; i++) {
		file.write((char*)&blockIndex[i], 2);
	}
	for (i = 0; i < numBlocks; i++) {
		file.write((char*)&blockSizes[i], 4);
	}
	file.write((char*)&numStrings, 4);
	file.write((char*)&maxStringLen, 4);
	for (i = 0; i < numStrings; i++) {
		strings[i].Put(file, 4);
	}
	file.write((char*)&unk2, 4);
}

bool NifBlockHeader::VerCheck(int v1, int v2, int v3, int v4) {
	if (ver4 >= v1 && ver3 >= v2 && ver2 >= v3 && ver1 >= v4) {
		return true;
	}
	return false;
}

NifBlockUnknown::NifBlockUnknown() { blockType = NIFBLOCK_UNKNOWN; data = NULL; }
NifBlockUnknown::NifBlockUnknown(fstream& file, unsigned int size) {
	blockType = NIFBLOCK_UNKNOWN;
	blockSize = size;
	data = new char[size];	
	Get(file, NifBlockHeader());
}

NifBlockUnknown::NifBlockUnknown(unsigned int size) {
	blockType = NIFBLOCK_UNKNOWN;
	blockSize = size;
	data = new char[size];
}

NifBlockUnknown::~NifBlockUnknown() {
	if (data) 
		delete [] data;
}

void NifBlockUnknown::Clone(NifBlockUnknown* other) {
	if (blockSize == other->blockSize)
		memcpy(data, other->data, blockSize);
}

void NifBlockUnknown::Get(fstream& file, NifBlockHeader& hdr) {
	if (!data) return;
	file.read(data, blockSize);
}

void NifBlockUnknown::Put(fstream& file, NifBlockHeader& hdr) {
	if (!data) return;
	file.write(data, blockSize);
}

void NifBlockBSLightShadeProp::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	int intVal;
	file.read((char*)&shaderType, 4);
	file.read((char*)&nameID, 4);
	if (nameID != 0xffffffff)
		shaderName = hdr.strings[nameID].str;
	else shaderName = "";
	file.read((char*)&numExtraData, 4);
	for (i = 0; i < numExtraData; i++) {
		file.read((char*)&intVal, 4);
		extraData.push_back(intVal);
	}
	file.read((char*)&controllerRef, 4);
	file.read((char*)&shaderFlags1, 4);
	file.read((char*)&shaderFlags2, 4);
	file.read((char*)&uvOffset.u, 4);
	file.read((char*)&uvOffset.v, 4);
	file.read((char*)&uvScale.u, 4);
	file.read((char*)&uvScale.v, 4);
	file.read((char*)&texsetRef, 4);

	file.read((char*)&emissiveClr.x, 4); file.read((char*)&emissiveClr.y, 4); file.read((char*)&emissiveClr.z, 4);
	file.read((char*)&emissivleMult, 4);
	file.read((char*)&texClampMode, 4);
	file.read((char*)&alpha, 4);
	file.read((char*)&unk, 4);
	file.read((char*)&glossiness, 4);
	file.read((char*)&specClr.x, 4); file.read((char*)&specClr.y, 4); file.read((char*)&specClr.z, 4);
	file.read((char*)&specStr, 4);
	file.read((char*)&lightFX1, 4);
	file.read((char*)&lightFX2, 4);
	if (shaderType == 1) {
		file.read((char*)&envMapScale, 4);
	}

	if (shaderType == 5) {
		file.read((char*)&skinTintClr.x, 4);	
		file.read((char*)&skinTintClr.y, 4);	
		file.read((char*)&skinTintClr.z, 4);
	}
	if (shaderType == 6) {
		file.read((char*)&hairTintClr.x, 4);	
		file.read((char*)&hairTintClr.y, 4);	
		file.read((char*)&hairTintClr.z, 4);		
	}
	if (shaderType == 7) {
		file.read((char*)&maxPasses, 4);
		file.read((char*)&scale, 4);
	}
	if (shaderType == 11) {
		file.read((char*)&parallaxThickness, 4);
		file.read((char*)&parallaxRefrScale, 4);
		file.read((char*)&parallaxTexScale.u, 4);
		file.read((char*)&parallaxTexScale.v, 4);
		file.read((char*)&parallaxEnvMapStr, 4);
	}
	if (shaderType == 14) {
		file.read((char*)&sparkleParams.r, 4);	
		file.read((char*)&sparkleParams.g, 4);	
		file.read((char*)&sparkleParams.b, 4);	
		file.read((char*)&sparkleParams.a, 4);
	}
	if (shaderType == 16) {
		file.read((char*)&eyeCubeScale, 4);	
		file.read((char*)&eyeLeftReflectCenter.x, 4);	
		file.read((char*)&eyeLeftReflectCenter.y, 4);
		file.read((char*)&eyeLeftReflectCenter.z, 4);
		file.read((char*)&eyeRightReflectCenter.x, 4);
		file.read((char*)&eyeRightReflectCenter.y, 4);
		file.read((char*)&eyeRightReflectCenter.z, 4);
	}
} 

void NifBlockBSLightShadeProp::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&shaderType, 4);
	file.write((char*)&nameID, 4);
	file.write((char*)&numExtraData, 4);
	for (i = 0; i < numExtraData; i++) {
		file.write((char*)&extraData[i], 4);
	}
	file.write((char*)&controllerRef, 4);	
	file.write((char*)&shaderFlags1, 4);
	file.write((char*)&shaderFlags2, 4);
	file.write((char*)&uvOffset.u, 4);
	file.write((char*)&uvOffset.v, 4);
	file.write((char*)&uvScale.u, 4);
	file.write((char*)&uvScale.v, 4);
	/*
	file.write((char*)&flags, 2);
	file.write((char*)&shaderType, 4);
	file.write((char*)&shaderFlags, 4);
	file.write((char*)&unkShort1, 2);
	file.write((char*)&unkInt2, 4);
	file.write((char*)&envmapScale, 4);
	file.write((char*)&unkInt3, 4);
	*/
	file.write((char*)&texsetRef, 4);
	file.write((char*)&emissiveClr.x, 4);
	file.write((char*)&emissiveClr.y, 4);
	file.write((char*)&emissiveClr.z, 4);
	file.write((char*)&emissivleMult, 4);
	file.write((char*)&texClampMode, 4);
	file.write((char*)&alpha, 4);
	file.write((char*)&unk, 4);
	file.write((char*)&glossiness, 4);
	file.write((char*)&specClr.x, 4);
	file.write((char*)&specClr.y, 4);
	file.write((char*)&specClr.z, 4);
	file.write((char*)&specStr, 4);
	file.write((char*)&lightFX1, 4);
	file.write((char*)&lightFX2, 4);

	if (shaderType == 1) {
		file.write((char*)&envMapScale, 4);
	}

	if (shaderType == 5) {
		file.write((char*)&skinTintClr.x, 4);	
		file.write((char*)&skinTintClr.y, 4);	
		file.write((char*)&skinTintClr.z, 4);
	}
	if (shaderType == 6) {
		file.write((char*)&hairTintClr.x, 4);	
		file.write((char*)&hairTintClr.y, 4);	
		file.write((char*)&hairTintClr.z, 4);		
	}
	if (shaderType == 7) {
		file.write((char*)&maxPasses, 4);
		file.write((char*)&scale, 4);
	}
	if (shaderType == 11) {
		file.write((char*)&parallaxThickness, 4);
		file.write((char*)&parallaxRefrScale, 4);
		file.write((char*)&parallaxTexScale.u, 4);
		file.write((char*)&parallaxTexScale.v, 4);
		file.write((char*)&parallaxEnvMapStr, 4);
	}
	if (shaderType == 14) {
		file.write((char*)&sparkleParams.r, 4);	
		file.write((char*)&sparkleParams.g, 4);	
		file.write((char*)&sparkleParams.b, 4);	
		file.write((char*)&sparkleParams.a, 4);
	}
	if (shaderType == 16) {
		file.write((char*)&eyeCubeScale, 4);	
		file.write((char*)&eyeLeftReflectCenter.x, 4);	
		file.write((char*)&eyeLeftReflectCenter.y, 4);
		file.write((char*)&eyeLeftReflectCenter.z, 4);
		file.write((char*)&eyeRightReflectCenter.x, 4);
		file.write((char*)&eyeRightReflectCenter.y, 4);
		file.write((char*)&eyeRightReflectCenter.z, 4);
	}
} 

void NifBlockBSLightShadeProp::Clone(NifBlockBSLightShadeProp* Other) {
	(*this) = (*Other);
	//uint argFlag = Other->argFlag;
	//string shaderName = Other->shaderName;
	//uint nameID = Other->nameID;
	//uint numExtraData = Other->numExtraData;
	//vector<int> extraData = Other->extraData;
	//int controllerRef = Other->controllerRef;
	//ushort flags = Other->flags;
	//uint shaderType = Other->shaderType;
	//ushort shaderFlags = Other->shaderFlags;
	//ushort unkShort1 = Other->unkShort1;
	//int unkInt2 = Other->unkInt2;
	//float envmapScale = Other->envmapScale;
	//int unkInt3 = Other->unkInt3;
	//int texsetRef = Other->texsetRef;
	//float unkFloat2 = Other->unkFloat2;
	//int refractionPeriod = Other->refractionPeriod;
	//float unkFloat4 = Other->unkFloat4;
	//float unkFloat5 = Other->unkFloat5;
	//int unkInt1 = Other->unkInt1;
	//memcpy(unkFloats,Other->unkFloats,sizeof(float) * 9);
	//vector3 unkArg_5 = Other->unkArg_5;
	//float unkArg_1 = Other->unkArg_1;	
	//memcpy(unkArg_16,Other->unkArg_16,sizeof(float) * 7);
	//vector3 unkArg_6 = Other->unkArg_6;
	//memcpy(unkArg_11,Other->unkArg_11,sizeof(float) * 5);
	//color4 unkArg_14 = Other->unkArg_14;
	//memcpy(unkArg_7,Other->unkArg_7,sizeof(float) * 2);
}

NifBlockBSLightShadeProp::NifBlockBSLightShadeProp() {
	blockType = NIFBLOCK_BSLGTSHADEPROP;
	shaderType = 0;
	nameID = -1;
	numExtraData = 0;
	controllerRef = -1;
	shaderFlags1 = 0x82400303;
	shaderFlags2 = 0x8001;
	uvOffset.u = 0.0f; uvOffset.v = 0.0f;
	uvScale.u = 1.0f; uvScale.v = 1.0f;
	texsetRef = -1;
	emissiveClr.x = 0.0f; emissiveClr.y = 0.0f; emissiveClr.z = 0.0f;;
	emissivleMult = 1.0f;
	texClampMode = 3;
	alpha = 1.0f;
	unk = 0.0f;
	glossiness = 20.0f;
	specClr.x = 1.0f; specClr.y = 1.0f; specClr.z = 1.0f;
	specStr = 1.0f;
	lightFX1 = 0.3f;
	lightFX2 = 2.0f;
//	for(int i=0;i<2;i++) { unkArg_7[i] = 0.0f; }

	blockSize = 100;
}

NifBlockBSLightShadeProp::NifBlockBSLightShadeProp(fstream& file, NifBlockHeader& hdr){
	blockType = NIFBLOCK_BSLGTSHADEPROP;
	Get(file, hdr);
}
	
void NifBlockBSLightShadeProp::notifyBlockDelete(int blockID, NifBlockHeader& hdr) {
	if (texsetRef == blockID) texsetRef = -1;
	else if (texsetRef > blockID) texsetRef--;
}

void NifBlockBSLightShadeProp::SetTexSetBlockID(int blockId) {
	texsetRef = blockId;
}

bool NifBlockBSLightShadeProp::IsSkinShader() {
	return (shaderFlags1 & (1 << 21)) != 0;
}

bool NifBlockBSLightShadeProp::IsDoubleSided() {
	return (shaderFlags2 & (1 << 4)) == 16;
}

NifBlockBSShaderTextureSet::NifBlockBSShaderTextureSet() { 
	blockType = NIFBLOCK_BSSHADERTEXSET;
	numTex = 9;
	for (int i = 0; i < 9; i++) {
		textures.push_back(NifString(false));
	}
	blockSize = 40;
}

NifBlockBSShaderTextureSet::NifBlockBSShaderTextureSet(fstream& file, NifBlockHeader& hdr) { 
	blockType = NIFBLOCK_BSSHADERTEXSET;
	Get(file, hdr);
}
	
void NifBlockBSShaderTextureSet::Get(fstream& file, NifBlockHeader& hdr) {		
	file.read((char*)&numTex, 4);
	for (int i = 0; i < numTex; i++) {
		textures.push_back(NifString(file, 4, false));
	}
}
	
void NifBlockBSShaderTextureSet::Put(fstream& file, NifBlockHeader& hdr) {
	file.write((char*)&numTex, 4);
	for (int i = 0; i < numTex; i++) {
		textures[i].Put(file, 4);
	}
}

int NifBlockBSShaderTextureSet::CalcBlockSize() {
	blockSize = 4;
	for (auto tex: textures) {
		blockSize += 4;
		blockSize += tex.str.length();
	}

	return blockSize;
}

NifBlockAlphaProperty::NifBlockAlphaProperty() {
	blockType = NIFBLOCK_ALPHAPROPERTY;
	blockSize = 15;
	alphaName = "";
	nameID = -1;
	numExtraData = 0;
	controllerRef = -1;
	flags = 4844;
	threshold = 128;
}

void NifBlockAlphaProperty::Get(fstream& file, NifBlockHeader& hdr) {
	int i;
	int intVal;
	file.read((char*)&nameID, 4);
	file.read((char*)&numExtraData, 4);
	for (i = 0; i < numExtraData; i++) {
		file.read((char*)&intVal, 4);
		extraData.push_back(intVal);
	}
	file.read((char*)&controllerRef, 4);
	file.read((char*)&flags, 2);
	file.read((char*)&threshold, 1);
}

void NifBlockAlphaProperty::Put(fstream& file, NifBlockHeader& hdr) {
	int i;
	file.write((char*)&nameID, 4);
	file.write((char*)&numExtraData, 4);
	for (i = 0; i < numExtraData; i++) {
		file.write((char*)&extraData[i], 4);
	}
	file.write((char*)&controllerRef, 4);
	file.write((char*)&flags, 2);
	file.write((char*)&threshold, 1);
}

void NifBlockStringExtraData::Get(fstream& file, NifBlockHeader& hdr) {
	file.read((char*)&nameID, 4);
	if (nameID != 0xffffffff)
		name = hdr.strings[nameID].str;
	else name = "";
	//file.read((char*)&nextExtraData, 4);
	//file.read((char*)&bytesRemaining, 4);
	file.read((char*)&stringDataId, 4);
	if (stringDataId != 0xffffffff)
		stringData = hdr.strings[stringDataId].str;
	else stringData = "";
}

void NifBlockStringExtraData::Put(fstream& file, NifBlockHeader& hdr) {
	file.write((char*)&nameID, 4);
	//file.write((char*)&nextExtraData, 4);
	//file.write((char*)&bytesRemaining, 4);
	file.write((char*)&stringDataId, 4);
}

NifBlockStringExtraData::NifBlockStringExtraData() {
	blockType = NIFBLOCK_STRINGEXTRADATA;
	blockSize = 8;
	nameID = (uint)-1;
	stringDataId = (uint)-1;
	name = "";
	//nextExtraData = -1;
	//bytesRemaining = 4;
	stringData = "";
}

NifBlockStringExtraData::NifBlockStringExtraData(fstream& file, NifBlockHeader& hdr) {
	blockType = NIFBLOCK_STRINGEXTRADATA;
	Get(file, hdr);
}
