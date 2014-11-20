#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include <string>
#include <map>
#include <hash_map>
#include <unordered_map>
#include <vector>
#include "niffile.h"

using namespace std;

#define MAPTYPE unordered_map

class DiffDataSets {
	map<string,MAPTYPE<int,vector3>> namedSet;
	map<string,string> dataTargets;
public:
	inline bool TargetMatch(const string& set, const string& target);
	int LoadSet( const string& name, const string& target, unordered_map<int, vec3>& inDiffData);
	int LoadSet( const string& name, const string& target, const string& fromFile);
	int SaveSet( const string& name, const string& target, const string& toFile);
	void RenameSet( const string& oldName, const string& newName);
	void DeepRename(const string& oldName, const string& newName);
	void AddEmptySet( const string& name, const string& target);
	void UpdateDiff( const string& name, const string& target, ushort index, vector3& newdiff);
	void SumDiff( const string& name, const string& target, ushort index, vector3& newdiff);
	void ScaleDiff( const string& name, const string& target, float scalevalue);	
	void OffsetDiff(const string& name, const string& target, vector3 &offset);
	void ApplyDiff( const string& set,const string& target, float percent, vector<vector3>* inOutResult);
	void ApplyUVDiff( const string& set,const string& target, float percent, vector<vector2>* inOutResult);
	void ApplyClamp( const string& set,const string& target, vector<vector3>* inOutResult);
	MAPTYPE<int,vector3>* GetDiffSet(const string& targetDataName);
	void GetDiffIndices (const string& set, const string& target, vector<ushort>& outIndices, float threshold = 0.0f);
	
	void ClearSet( const string& name );
	void EmptySet( const string& set, const string& target ) {
		
		if(!TargetMatch(set,target))
			return;
		namedSet[set].clear();
	}

	
	void ZeroVertDiff(const string& set, const string& target,  vec3* vcolorMask) {
		for(auto ns : namedSet[set]) {
			float f = vcolorMask[ns.first].x;
			if(f == 1.0f) {
				continue;
			} else if(f==0.0f) {	
				namedSet[set][ns.first] *= 0.0f;
			} else {
				namedSet[set][ns.first] *= f;
			}
		}
	}

	// zeroes diffs for the specified verts (or all verts in set if vertSet is null), with an optional mask value.  a partially masked vertex will have it's diff brought closer to 0, 
	// a fully masked vertex will have it's diff remain the same, and a fully unmasked vert will have its diff erased.
	void ZeroVertDiff(const string& set, const string& target, vector<int>* vertSet, unordered_map<int, float>* mask) {
		if(!TargetMatch(set, target)) 
			return;
		vector<int> v;
		if(vertSet != NULL) {
			v = (*vertSet);
		} else {
			for(auto diff : namedSet[set]) {
				v.push_back(diff.first);
			}
		}
		float f;
		for(auto i : v) {
			auto d = namedSet[set].find(i);
			if(d == namedSet[set].end())
				continue;
			f=0.0f;
			if(mask) {
				auto m = mask->find(i);
				if(m!=mask->end()) {
					f = m->second;
				}		
			}
			if(f == 1.0f) 
				continue;
			if(f == 0.0f) {
				namedSet[set].erase(i);
				continue;
			}
			namedSet[set][i] *= f;
		}

	}


	void Clear() {
		namedSet.clear();
		dataTargets.clear();
	}
};
