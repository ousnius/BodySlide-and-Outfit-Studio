#include "DiffData.h"

#include <algorithm>
#include <unordered_map>

// Set == slider name, target == shape name. 
bool DiffDataSets::TargetMatch(const string& set, const string& target) {
	if(dataTargets.find(set) != dataTargets.end()) {
		return dataTargets[set] == target;
	}
	return false;
}

int DiffDataSets::LoadSet(const string& name, const string& target, unordered_map<int,vec3>& inDiffData) {
	if (namedSet.find(name) != namedSet.end()) {
		namedSet.erase(name);
	}
	namedSet[name] = inDiffData;
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::LoadSet(const string& name, const string& target, const string& fromFile) {
	fstream inFile(fromFile.c_str(), ios_base::in | ios_base::binary);
	if (!inFile.is_open()) {
		return 1;
	}
	int sz;
	inFile.read((char*)&sz, 4);
	
	MAPTYPE<int,vector3> data;
	//data.clear();
	int idx;
	vector3 v;
	for (int i = 0; i < sz; i++) {
		inFile.read((char*)&idx, sizeof(int));
		inFile.read((char*)&v, sizeof(vector3));
		data[idx] = v;
	}
	inFile.close();
	if (namedSet.find(name) != namedSet.end()) {
		namedSet.erase(name);
	}
	namedSet.emplace(name, move(data));
	dataTargets[name] = target;

	return 0;
}

int DiffDataSets::SaveSet(const string& name, const string& target, const string& toFile) {
	MAPTYPE<int,vector3>* data = &namedSet[name];
	//if(namedTargets[name] != target) 
	//	return 1;
	if (!TargetMatch(name, target))
		return 1;
	fstream outFile(toFile.c_str(), ios_base::out | ios_base::binary);
	if (!outFile.is_open()) {
		return 2;
	}
	MAPTYPE<int,vector3>::iterator resultIt;
	int sz = data->size();
	outFile.write((char*)&sz, sizeof(int));
	for (resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		outFile.write((char*)&resultIt->first, sizeof(int));
		outFile.write((char*)&resultIt->second, sizeof(vector3));
	}
	return 0;
}

void DiffDataSets::RenameSet(const string& oldName, const string& newName) {
	if (namedSet.find(oldName) != namedSet.end()) {
		namedSet.emplace(newName, namedSet[oldName]);
		namedSet.erase(oldName);
		dataTargets[newName] = dataTargets[oldName];
		dataTargets.erase(oldName);
	}
}

void DiffDataSets::DeepRename(const string& oldName, const string& newName) {
	vector<string> oldtargets;
	vector<string> newTargets;
	string newDT = "";
	for (auto& dt: dataTargets) {
		if (dt.second == oldName && dt.first.length() >= oldName.length()) {
			oldtargets.push_back(dt.first);
			newDT = dt.first.substr(oldName.length());
			newDT = newName + newDT;
			newTargets.push_back(newDT);
			dt.second = newName;
		}
	}
	for (int i = 0; i < oldtargets.size(); i++) {
		string ot = oldtargets[i];
		string nt = newTargets[i];
		if (dataTargets.find(ot) != dataTargets.end()) {
			dataTargets[nt] = dataTargets[ot];
			dataTargets.erase(ot);
		}
		if (namedSet.find(ot) != namedSet.end()) {
			namedSet[nt] = move(namedSet[ot]);
			namedSet.erase(ot);
		}
	}
}

void DiffDataSets::AddEmptySet(const string &name, const string &target) {
	MAPTYPE<int, vector3> data;
	if (namedSet.find(name) == namedSet.end()) {
		namedSet[name]  = data;
		dataTargets[name] = target;
	}
}

void DiffDataSets::UpdateDiff(const string& name, const string& target, ushort index, vector3 &newdiff) {
	MAPTYPE<int, vector3>* data = &namedSet[name];
	//if(namedTargets[name] != target) 
	if (!TargetMatch(name, target))
		return;

	(*data)[index] = newdiff;
}

void DiffDataSets::SumDiff(const string& name, const string& target, ushort index, vector3 &newdiff) {
	MAPTYPE<int, vector3>* data = &namedSet[name];
	//if(namedTargets[name] != target) 
	if (!TargetMatch(name, target))
		return;
	vec3 v = (*data)[index];
	v += newdiff;
	(*data)[index] = v;
}

void DiffDataSets::ScaleDiff(const string& name, const string& target, float scalevalue) {
	MAPTYPE<int, vector3>* data = &namedSet[name];

	//if (namedTargets[name] != target) 
	if (!TargetMatch(name, target))
		return;
	
	MAPTYPE<int,vector3>::iterator resultIt;
	for (resultIt = data->begin(); resultIt != data->end(); ++resultIt) {
		resultIt->second *= scalevalue;
	}
}

void DiffDataSets::OffsetDiff(const string& name, const string& target, vector3 &offset) {
	MAPTYPE<int, vector3>* data = &namedSet[name];

	//if(namedTargets[name] != target) 
	if(!TargetMatch(name,target))
		return;
	
	MAPTYPE<int,vector3>::iterator resultIt;
	for(resultIt = data->begin(); resultIt!=data->end(); ++resultIt) {
		resultIt->second += offset;
	}
}

void DiffDataSets::ApplyUVDiff( const string& set, const string& target, float percent, vector<vector2>* inOutResult){ 
	//if(namedTargets[set] != target) 
	if((percent == 0)) {
		return;
	}
	
	if(!TargetMatch(set,target))
		return;
	int maxidx = (*inOutResult).size();
	
	MAPTYPE<int,vector3>* data = &namedSet[set];
	
	MAPTYPE<int,vector3>::iterator resultIt;
	for(resultIt = data->begin(); resultIt!=data->end(); ++resultIt) {
		if(resultIt->first >= maxidx) continue;			// prevent crashes.
		(*inOutResult)[resultIt->first].u += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].v += resultIt->second.y * percent;
	}
}
void DiffDataSets::ApplyDiff( const string& set, const string& target, float percent, vector<vector3>* inOutResult){ 
	//if(namedTargets[set] != target) 
	if((percent == 0)) {
		return;
	}
	
	if(!TargetMatch(set,target))
		return;
	int maxidx = (*inOutResult).size();
	
	MAPTYPE<int,vector3>* data = &namedSet[set];
	
	MAPTYPE<int,vector3>::iterator resultIt;
	for(resultIt = data->begin(); resultIt!=data->end(); ++resultIt) {
		if(resultIt->first >= maxidx) continue;			// prevent crashes.
		(*inOutResult)[resultIt->first].x += resultIt->second.x * percent;
		(*inOutResult)[resultIt->first].y += resultIt->second.y * percent;
		(*inOutResult)[resultIt->first].z += resultIt->second.z * percent;
	}
}
void DiffDataSets::ApplyClamp( const string& set, const string& target, vector<vector3>* inOutResult){ 
	MAPTYPE<int,vector3>* data = &namedSet[set];
	//if(namedTargets[set] != target) 
	if(!TargetMatch(set,target))
		return;
	int maxidx = (*inOutResult).size();
	MAPTYPE<int,vector3>::iterator resultIt;
	for(resultIt = data->begin(); resultIt!=data->end(); ++resultIt) {
		if(resultIt->first >= maxidx) continue;			// prevent crashes.
		(*inOutResult)[resultIt->first].x = resultIt->second.x;
		(*inOutResult)[resultIt->first].y = resultIt->second.y;
		(*inOutResult)[resultIt->first].z = resultIt->second.z;
	}
}

MAPTYPE<int, vector3>* DiffDataSets::GetDiffSet(const string& targetDataName) {
	/*
	string set;
	for(auto dt:dataTargets) {
		if(dt.second == target) {
			set = dt.first;	
		}
	}
	if(set.empty()) 
		return NULL;*/
	if(namedSet.find(targetDataName) == namedSet.end()) 
		return NULL;
	return &namedSet[targetDataName];
}

void DiffDataSets::GetDiffIndices(const string& set, const string& target, vector<ushort>& outIndices, float threshold) {
	//outIndices.clear();
	MAPTYPE<int,vector3>* data = &namedSet[set];
	//if(namedTargets[set] != target) 	
	if(!TargetMatch(set,target))
		return;
	MAPTYPE<int,vector3>::iterator resultIt;
	for(resultIt = data->begin(); resultIt!=data->end(); ++resultIt) {
		if(fabs(resultIt->second.x) > threshold ||
			fabs(resultIt->second.y) > threshold ||
			fabs(resultIt->second.z) > threshold) {
			outIndices.push_back(resultIt->first);
		}
	}
	std::sort(outIndices.begin(), outIndices.end());
	std::unique(outIndices.begin(), outIndices.end());
}

void DiffDataSets::ClearSet( const string& name) {
	namedSet.erase(name);
	dataTargets.erase(name);

}