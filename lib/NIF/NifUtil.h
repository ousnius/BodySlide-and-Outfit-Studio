/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "utils/Object3d.h"

// ApplyMapToTriangles applies a vertex index renumbering map to p1, p2,
// and p3 of a vector of Triangles "tris".  If a triangle has an index out
// of range of the map or if an index maps to a negative number, the
// triangle is removed.
template<typename IndexType1, typename IndexType2 = int> void ApplyMapToTriangles(std::vector<Triangle> &tris, const std::vector<IndexType1> &map, std::vector<IndexType2> *deletedTris = nullptr) {
	const int mapsz = map.size();
	int di = 0;
	for (int si = 0; si < tris.size(); ++si) {
		const Triangle &stri = tris[si];
		// Triangle's indices are unsigned, but IndexType might be signed.
		if (stri.p1 >= mapsz || stri.p2  >= mapsz || stri.p3 >= mapsz ||
			map[stri.p1] < 0 || map[stri.p2] < 0 || map[stri.p3] < 0) {
			if (deletedTris)
				deletedTris->push_back(si);
			continue;
		}
		Triangle &dtri = tris[di];
		dtri.p1 = map[stri.p1];
		dtri.p2 = map[stri.p2];
		dtri.p3 = map[stri.p3];
		++di;
	}
	tris.resize(di);
}

inline ushort CalcMaxTriangleIndex(const std::vector<Triangle> &v) {
	ushort maxind = 0;
	for (unsigned int i = 0; i < v.size(); ++i) {
		maxind = std::max(maxind, v[i].p1);
		maxind = std::max(maxind, v[i].p2);
		maxind = std::max(maxind, v[i].p3);
	}
	return maxind;
}

// inds must be in sorted ascending order.
template<typename VectorType, typename IndexType> void EraseVectorIndices(VectorType &v, const std::vector<IndexType> &inds) {
	if (inds.empty() || inds[0] >= v.size())
		return;
	int indi = 1;
	int di = inds[0];
	int si = di + 1;
	for (; si < v.size(); ++si) {
		if (indi < inds.size() && si == inds[indi])
			++indi;
		else
			v[di++] = std::move(v[si]);
	}
	v.resize(di);
}

// inds must be in sorted ascending order.
template<typename VectorType, typename IndexType> void InsertVectorIndices(VectorType &v, const std::vector<IndexType> &inds) {
	if (inds.empty() || inds.back() >= v.size() + inds.size())
		return;
	int indi = inds.size() - 1;
	int di = v.size() + inds.size() - 1;
	int si = v.size() - 1;
	v.resize(di + 1);
	while (true) {
		while (indi >= 0 && di == inds[indi])
			--di, --indi;
		if (indi < 0)
			break;
		v[di--] = std::move(v[si--]);
	}
}

// inds must be in sorted ascending order.
template<typename IndexType1, typename IndexType2> std::vector<int> GenerateIndexCollapseMap(const std::vector<IndexType1> &inds, IndexType2 mapSize) {
	std::vector<int> map(mapSize);
	int indi = 0;
	for (int si = 0, di = 0; si < mapSize; ++si) {
		if (indi < inds.size() && si == inds[indi]) {
			map[si] = -1;
			++indi;
		}
		else
			map[si] = di++;
	}
	return map;
}

// inds must be in sorted ascending order.
template<typename IndexType1, typename IndexType2> std::vector<int> GenerateIndexExpandMap(const std::vector<IndexType1> &inds, IndexType2 mapSize) {
	std::vector<int> map(mapSize);
	int indi = 0;
	for (int si = 0, di = 0; si < mapSize; ++si, ++di) {
		while (indi < inds.size() && di == inds[indi])
			++di, ++indi;
		map[si] = di;
	}
	return map;
}

// ApplyIndexMapToMapKeys: MapType is something like
// std::unordered_map<int, Data> or std::map<int, Data>.
// If a MapType-key k is in the indexMap, it is deleted if indexMap[k]
// is negative, or changed to indexMap[k] otherwise.  If k is not in
// indexMap, defaultOffset is added to it.
template<typename MapType> void ApplyIndexMapToMapKeys(MapType &keyMap, const std::vector<int> indexMap, int defaultOffset) {
	MapType copy;
	for (auto &d : keyMap) {
		if (d.first >= indexMap.size())
			copy[d.first + defaultOffset] = std::move(d.second);
		else if (indexMap[d.first] >= 0)
			copy[indexMap[d.first]] = std::move(d.second);
	}
	keyMap = std::move(copy);
}

template<typename IndexType> std::vector<Triangle> GenerateTrianglesFromStrips(const std::vector<std::vector<IndexType>> &strips) {
	std::vector<Triangle> tris;
	for (const std::vector<IndexType> &strip : strips) {
		if (strip.size() < 3)
			continue;
		ushort a = strip[0];
		ushort b = strip[1];
		for (int i = 2; i < strip.size(); ++i) {
			ushort c = strip[i];
			if (a != b && b != c && c != a) {
				if ((i & 1) == 0)
					tris.push_back(Triangle(a, b, c));
				else
					tris.push_back(Triangle(a, c, b));
			}
			a = b;
			b = c;
		}
	}
	return tris;
}
