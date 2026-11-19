/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

// OutfitProject: symmetric vertices, asymmetries and symmetrizing

#include "OutfitProject.h"
#include "components/WeightNorm.h"
#include "NifUtil.hpp"

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <filesystem>
#include <limits>
#include <regex>
#include <sstream>
#include <wx/log.h>

using namespace nifly;

void OutfitProject::MatchSymmetricVertices(NiShape* shape, const Mesh::WeldVertsType& weldVerts, SymmetricVertices& r) {
	// Gather shape's vertices
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	int origNVerts = static_cast<int>(verts.size());

	// Dealing with welded vertices is potentially very messy.  So we
	// remove all but one vertex of each welded set from the vertex list,
	// producing a reduced vertex list.  The surviving vertex of each
	// welded set is the one with the lowest index.
	std::vector<int> indRedToOrig(origNVerts);
	int redNVerts = 0;
	for (int vi = 0; vi < origNVerts; ++vi) {
		if (Mesh::LeastWeldedVertexIndex(weldVerts, vi) == vi)
			indRedToOrig[redNVerts++] = vi;
	}

	// Delete welded vertices from our copy of the vertex list.
	for (int vi = 0; vi < redNVerts; ++vi)
		verts[vi] = verts[indRedToOrig[vi]];

	// Build proximity cache for vertices.
	// Note that kd_tree keeps pointers into verts.
	kd_tree<uint16_t> vertTree(&verts[0], redNVerts);

	struct VertData {
		// bad: no match or a bad match
		bool bad = false;
		// mi: matched vertex's index.  The value of -1 is not meaningful,
		// except it's not a valid vertex index.
		int mi = -1;
		int matchcount = 0;
	};
	std::vector<VertData> vdata(redNVerts);

	// Main loop through the reduced vertex set.
	for (int vi = 0; vi < redNVerts; ++vi) {
		// Construct mirror coordinates
		Vector3 mc = verts[vi];
		mc.x = -mc.x;

		// Match
		vertTree.kd_nn(&mc, 0);

		// mi: matched vertex's index
		int mi = vertTree.queryResult[0].vertex_index;
		vdata[vi].mi = mi;
		++vdata[mi].matchcount;
	}

	// Mark a vertex bad if two vertices matched to it, if its matched
	// vertex didn't match to it, or if its match is bad.
	for (int vi = 0; vi < redNVerts; ++vi)
		if (vdata[vi].matchcount != 1 || vdata[vdata[vi].mi].mi != vi) {
			vdata[vi].bad = true;
			vdata[vdata[vi].mi].bad = true;
		}

	// Put results in r.
	for (int vi = 0; vi < redNVerts; ++vi) {
		const VertData& vd = vdata[vi];
		if (vd.bad)
			r.unmatched.push_back(indRedToOrig[vi]);
		else if (vi <= vd.mi)
			r.matches.emplace_back(indRedToOrig[vi], indRedToOrig[vd.mi]);
	}
}

void OutfitProject::MatchSymmetricBoneNames(std::vector<std::pair<std::string, std::string>>& pairs, std::vector<std::string>& singles) {
	// Note that there is very similar code to this in OutfitStudioFrame::CalcAutoXMirrorBone.
	std::vector<std::string> bones;
	GetActiveBones(bones);

	AnimInfo::MatchSymmetricBoneNames(bones, pairs, singles);
}

void OutfitProject::FindVertexAsymmetries(NiShape* shape, const SymmetricVertices& symverts, const Mesh::WeldVertsType& weldVerts, VertexAsymmetries& r) {
	// Get shape's vertices
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	int nVerts = static_cast<int>(shape->GetNumVertices());

	// Create welded-vertex set for every vertex
	std::vector<std::vector<int>> weldSets(nVerts);
	for (int vi = 0; vi < nVerts; ++vi)
		Mesh::GetWeldSet(weldVerts, vi, weldSets[vi]);

	// Initialize global result arrays
	int nMatches = static_cast<int>(symverts.matches.size());
	r.positions.resize(nMatches, false);
	r.anyslider.resize(nMatches, false);
	r.anybone.resize(nMatches, false);
	r.poserr.resize(nMatches, 0.0f);

	// Find position asymmetries
	for (int mpi = 0; mpi < nMatches; ++mpi) {
		bool asym = false;
		float err = 0.0f;
		int errCount = 0;
		for (int p1 : weldSets[symverts.matches[mpi].first]) {
			Vector3 v1 = verts[p1];
			v1.x = -v1.x;
			for (int p2 : weldSets[symverts.matches[mpi].second])
				if (v1 != verts[p2]) {
					asym = true;
					err += (v1 - verts[p2]).length();
					++errCount;
				}
		}
		if (asym) {
			r.positions[mpi] = true;
			r.poserr[mpi] = err / errCount;
		}
	}

	// Find position slider asymmetries
	for (size_t si = 0; si < activeSet.size(); ++si) {
		SliderData& sd = activeSet[si];
		if (sd.bUV || sd.bClamp || sd.bZap)
			continue;

		// Get the diffs for this slider
		const std::unordered_map<uint16_t, Vector3>* diffSet = GetDiffSet(sd, shape);
		if (!diffSet)
			continue;

		// Initialize all the data we'll be collecting for this slider
		std::vector<bool> aflags(nMatches, false);
		std::vector<float> differr(nMatches, 0.0f);
		bool sliderasym = false;

		// Main loop through matches (for sliders).
		for (int mpi = 0; mpi < nMatches; ++mpi) {
			bool asym = false;
			float err = 0.0f;
			int errCount = 0;

			// Loop through weld set of match's first point
			for (int p1 : weldSets[symverts.matches[mpi].first]) {
				// Get p1's diff
				Vector3 p1diff;
				auto dit1 = diffSet->find(p1);
				if (dit1 != diffSet->end())
					p1diff = dit1->second;
				p1diff.x = -p1diff.x;

				// Loop through weld set of match's second point
				for (int p2 : weldSets[symverts.matches[mpi].second]) {
					// Get p2's diff
					Vector3 p2diff;
					auto dit2 = diffSet->find(p2);
					if (dit2 != diffSet->end())
						p2diff = dit2->second;

					// Check for asymmetry
					if (p1diff != p2diff) {
						asym = true;
						err += (p1diff - p2diff).length();
						++errCount;
					}
				}
			}

			// If this match had an asymmetry, record it
			if (asym) {
				aflags[mpi] = true;
				differr[mpi] = err / errCount;
				sliderasym = true;
				r.anyslider[mpi] = true;
			}
		}

		// If this slider had an asymmetry, store the results
		if (sliderasym) {
			r.sliders.emplace_back();
			r.sliders.back().sliderName = sd.name;
			r.sliders.back().aflags = std::move(aflags);
			r.sliders.back().differr = std::move(differr);
		}
	}

	// Get list of symmetric bone pairings and unpaired bones
	std::vector<std::pair<std::string, std::string>> bPairs;
	std::vector<std::string> bSingles;
	MatchSymmetricBoneNames(bPairs, bSingles);
	int nbPairs = static_cast<int>(bPairs.size());
	int nbSingles = static_cast<int>(bSingles.size());
	AnimSkin& skin = workAnim.shapeSkinning[shape->name.get()];

	// Find bone weight asymmetries
	std::unordered_map<uint16_t, float> dummyWeights;
	for (int bpi = 0; bpi < nbPairs + nbSingles; ++bpi) {
		// Get the names of our one or two bones
		std::string b1name, b2name;
		bool isBonePair = bpi < nbPairs;
		if (isBonePair) {
			b1name = bPairs[bpi].first;
			b2name = bPairs[bpi].second;
		}
		else {
			b1name = bSingles[bpi - nbPairs];
			b2name = b1name;
		}

		// Look up the weights for both bones
		auto bnit = skin.boneNames.find(b1name);
		const std::unordered_map<uint16_t, float>& b1w = bnit != skin.boneNames.end() ? skin.boneWeights[bnit->second].weights : dummyWeights;
		bnit = skin.boneNames.find(b2name);
		const std::unordered_map<uint16_t, float>& b2w = bnit != skin.boneNames.end() ? skin.boneWeights[bnit->second].weights : dummyWeights;

		// Initialize all the data we'll be calculating for this bone/bones.
		std::vector<bool> aflags1(nMatches, false);
		std::vector<bool> aflags2(nMatches, false);
		std::vector<float> weighterr1(nMatches, 0.0f);
		std::vector<float> weighterr2(nMatches, 0.0f);
		bool boneasym = false;

		// Main loop through matches for a bone or bone pair.
		for (int mpi = 0; mpi < nMatches; ++mpi) {
			bool asym1 = false, asym2 = false;
			float werr1 = 0.0f, werr2 = 0.0f;
			int werr1count = 0, werr2count = 0;

			// Loop through weld set for match's first point
			for (int p1 : weldSets[symverts.matches[mpi].first]) {
				// Find weights of p1 for bone 1 and bone 2
				float p1b1w = 0.0f, p1b2w = 0.0f;
				auto wit = b1w.find(p1);
				if (wit != b1w.end())
					p1b1w = wit->second;
				wit = b2w.find(p1);
				if (wit != b2w.end())
					p1b2w = wit->second;

				// Loop through weld set for match's second point
				for (int p2 : weldSets[symverts.matches[mpi].second]) {
					// Find weights of p2 for bone 1 and bone 2
					float p2b1w = 0.0f, p2b2w = 0.0f;
					wit = b1w.find(p2);
					if (wit != b1w.end())
						p2b1w = wit->second;
					wit = b2w.find(p2);
					if (wit != b2w.end())
						p2b2w = wit->second;

					// Check for asymmetry between p1's bone 1 weight and
					// p2's bone 2 weight.
					if (p1b1w != p2b2w) {
						asym1 = true;
						werr1 += std::fabs(p1b1w - p2b2w);
						++werr1count;
					}

					// Check for asymmetry between p1's bone 2 weight and
					// p2's bone 1 weight.
					if (p1b2w != p2b1w) {
						asym2 = true;
						werr2 += std::fabs(p1b2w - p2b1w);
						++werr2count;
					}
				}
			}
			// end loops through weld sets of the match's two points

			// Record results for this match for this bone or bone pair
			if (asym1) {
				aflags1[mpi] = true;
				weighterr1[mpi] = werr1 / werr1count;
			}
			if (asym2) {
				aflags2[mpi] = true;
				weighterr2[mpi] = werr2 / werr2count;
			}
			if (asym1 || asym2) {
				r.anybone[mpi] = true;
				boneasym = true;
			}
		}
		// end main loop through matches for a bone or bone pair

		// If we have any weight asymmetries for this bone or bone pair,
		// store the results.
		if (boneasym) {
			r.bones.emplace_back();
			r.bones.back().boneName = b1name;
			r.bones.back().mirroroffset = isBonePair ? 1 : 0;
			r.bones.back().aflags = std::move(aflags1);
			r.bones.back().weighterr = std::move(weighterr1);
			if (isBonePair) {
				r.bones.emplace_back();
				r.bones.back().boneName = b2name;
				r.bones.back().mirroroffset = -1;
				r.bones.back().aflags = std::move(aflags2);
				r.bones.back().weighterr = std::move(weighterr2);
			}
		}
	}
}

std::vector<bool> CalcVertexListForAsymmetryTasks(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, int nVerts) {
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nMatches = static_cast<int>(asyms.positions.size());

	std::vector<bool> doVert(nVerts, false);
	for (int mi = 0; mi < nMatches; ++mi) {
		int p1 = symverts.matches[mi].first;
		int p2 = symverts.matches[mi].second;

		// Positions
		if (tasks.doPos && asyms.positions[mi])
			doVert[p1] = doVert[p2] = true;

		// Sliders
		for (int sai = 0; sai < nSliders; ++sai)
			if (tasks.doSliders[sai] && asyms.sliders[sai].aflags[mi])
				doVert[p1] = doVert[p2] = true;

		// Bones
		for (int bi = 0; bi < nBones; ++bi) {
			if (!asyms.bones[bi].aflags[mi])
				continue;
			if (tasks.doBones[bi])
				doVert[p1] = true;
			if (tasks.doBones[bi + asyms.bones[bi].mirroroffset])
				doVert[p2] = true;
		}
	}

	if (tasks.doUnmatched)
		for (int vi : symverts.unmatched)
			doVert[vi] = true;

	return doVert;
}

void AddWeldedToVertexList(const Mesh::WeldVertsType& weldVerts, std::vector<bool>& verts) {
	for (size_t i = 0; i < verts.size(); ++i) {
		if (!verts[i])
			continue;
		Mesh::DoForEachWeldedVertex(weldVerts, i, [&](int p){verts[p] = true;});
	}
}

static int SelCount(const std::vector<bool>& sel) {
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		c += sel[i];
	return c;
}

static int SelCount(const std::vector<bool>& sel, const std::vector<bool>& osel) {
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		if (sel[i] && osel[i])
			++c;
	return c;
}

static float SelAvg(const std::vector<bool>& sel, const std::vector<float>& vals) {
	float sum = 0.0f;
	int c = 0;
	for (size_t i = 0; i < sel.size(); ++i)
		if (sel[i]) {
			sum += vals[i];
			++c;
		}
	return c ? sum / c : 0.0f;
}

void CalcVertexAsymmetryStats(const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const std::vector<bool>& selVerts, VertexAsymmetryStats& stats) {
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());
	int nMatches = static_cast<int>(asyms.positions.size());

	// Calculate which matches are not masked
	std::vector<bool> selMatches(nMatches);
	for (int mi = 0; mi < nMatches; ++mi)
		selMatches[mi] = selVerts[symverts.matches[mi].first] || selVerts[symverts.matches[mi].second];

	stats.boneCounts.resize(nBones, 0);
	stats.boneAvgs.resize(nBones, 0.0f);
	stats.sliderCounts.resize(nSliders, 0);
	stats.sliderAvgs.resize(nSliders, 0.0f);
	stats.unmaskedCount = SelCount(selVerts);
	stats.posCount = SelCount(selMatches, asyms.positions);
	stats.posAvg = SelAvg(selMatches, asyms.poserr);
	stats.anySliderCount = SelCount(selMatches, asyms.anyslider);
	stats.anyBoneCount = SelCount(selMatches, asyms.anybone);

	for (int si = 0; si < nSliders; ++si) {
		stats.sliderCounts[si] = SelCount(selMatches, asyms.sliders[si].aflags);
		stats.sliderAvgs[si] = SelAvg(selMatches, asyms.sliders[si].differr);
	}

	for (int bi = 0; bi < nBones; ++bi) {
		int bi2 = bi + asyms.bones[bi].mirroroffset;
		float sum = 0.0f;
		int count = 0;

		for (int mi = 0; mi < nMatches; ++mi) {
			int p1 = symverts.matches[mi].first;
			int p2 = symverts.matches[mi].second;
			if (!selVerts[p1] && !selVerts[p2])
				continue;

			// b1 aflags says: p1 b1 mismatches p2 b2
			// b2 aflags says: p1 b2 mismatches p2 b1
			// We need to sum the data for b1; that is, the first and fourth
			// cases.
			if (selVerts[p1] && asyms.bones[bi].aflags[mi]) {
				sum += asyms.bones[bi].weighterr[mi];
				++count;
			}
			if (p1 != p2 && selVerts[p2] && asyms.bones[bi2].aflags[mi]) {
				sum += asyms.bones[bi2].weighterr[mi];
				++count;
			}
		}

		stats.boneCounts[bi] = count;
		if (count)
			stats.boneAvgs[bi] = sum / count;
	}
}

static Vector3 GetUSliderDiff(const UndoStateVertex& usv, const std::string& name) {
	for (const auto& sd : usv.diffs)
		if (sd.sliderName == name)
			return sd.diff;
	return Vector3();
}

static void SetUSliderDiff(UndoStateVertex& usv, const std::string& name, const Vector3& diff) {
	for (auto& sd : usv.diffs)
		if (sd.sliderName == name) {
			sd.diff = diff;
			return;
		}
	usv.diffs.push_back(UndoStateVertexSliderDiff{name, diff});
}

static float GetUWeight(const UndoStateVertex& usv, const std::string& name) {
	for (const auto& bw : usv.weights)
		if (bw.boneName == name)
			return bw.w;
	return 0.0f;
}

// The two versions of SetUWeight are for storing it in uss.boneWeights
// (first version) or usv.weights (second version).  The first is needed
// by the normalizer; the second is the final location.
static void SetUWeight(UndoStateShape& uss, int p, const std::string& bn, float w) {
	for (auto& bw : uss.boneWeights)
		if (bw.boneName == bn) {
			bw.weights[p].endVal = w;
			return;
		}
}

static void SetUWeight(UndoStateVertex& usv, const std::string& bn, float w) {
	for (auto& bw : usv.weights)
		if (bw.boneName == bn) {
			bw.w = w;
			return;
		}
	usv.weights.push_back(UndoStateVertexBoneWeight{bn, w});
}

void OutfitProject::PrepareSymmetrizeVertices(NiShape* shape, UndoStateShape& uss, const SymmetricVertices& symverts, const VertexAsymmetries& asyms, const VertexAsymmetryTasks& tasks, const Mesh::WeldVertsType& weldVerts, const std::vector<bool>& selVerts, const std::vector<std::string>& userNormBones, const std::vector<std::string>& userNotNormBones) {
	int nMatches = static_cast<int>(symverts.matches.size());
	int nVerts = static_cast<int>(shape->GetNumVertices());
	int nSliders = static_cast<int>(asyms.sliders.size());
	int nBones = static_cast<int>(asyms.bones.size());

	// To simplify the handling of vertex data, we collect the data for
	// all vertices.  We will keep track of which vertices we actually
	// modify and put just the modified data in uss at the end.  The new
	// vertex data (tuss.addVerts) is initially a copy of the old vertex
	// data (tuss.delVerts).
	std::vector<uint16_t> allvinds(nVerts);
	for (uint16_t i = 0; i < nVerts; ++i)
		allvinds[i] = i;
	UndoStateShape tuss;
	CollectVertexData(shape, tuss, allvinds);
	tuss.addVerts = tuss.delVerts;
	std::vector<bool> vChanged(nVerts, false);

	// Prepare bone list for bone weight normalizer.  First, we add the
	// bones that are being symmetrized to normBones.
	std::vector<std::string> normBones;	// order is important: sel before unsel
	std::unordered_set<std::string> normBonesSet;
	for (int bai = 0; bai < nBones; ++bai)
		if (tasks.doBones[bai]) {
			normBones.push_back(asyms.bones[bai].boneName);
			normBonesSet.insert(asyms.bones[bai].boneName);
		}
	int nMBones = static_cast<int>(normBones.size());

	// Next, we add the norm-bones from the user to normBones, if they
	// aren't already there.
	bool hasNormBones = false;
	for (const std::string& bone : userNormBones)
		if (!normBonesSet.count(bone)) {
			normBones.push_back(bone);
			normBonesSet.insert(bone);
			hasNormBones = true;
		}

	// Finally, we add the not-norm-bones from the user.  If the user has
	// picked some norm bones that aren't being symmetrized, then we can
	// lock all the not-norm-bones.  Otherwise, they become norm bones.
	std::vector<std::string> lockedBones;
	for (const std::string& bone : userNotNormBones)
		if (!normBonesSet.count(bone)) {
			if (hasNormBones)
				lockedBones.push_back(bone);
			else
				normBones.push_back(bone);
		}

	// Initialize the bone weight normalizer
	BoneWeightAutoNormalizer nzer;
	nzer.SetUp(&tuss, &workAnim, shape->name.get(), normBones, lockedBones, nMBones, hasNormBones);

	// Because we might be adjusting the weights for several different bones
	// for a given point, we have to delay normalization until after the
	// main loop.  Only the first point in each weld set is normalized;
	// the result is copied to the other points in the weld set.
	std::vector<bool> weightAdjustPoint(nVerts, false);

	// Main loop through matches
	std::vector<int> p1s, p2s;
	for (int mi = 0; mi < nMatches; ++mi) {
		int p1 = symverts.matches[mi].first;
		int p2 = symverts.matches[mi].second;
		if (!selVerts[p1] && !selVerts[p2])
			continue;

		Mesh::GetWeldSet(weldVerts, p1, p1s);
		Mesh::GetWeldSet(weldVerts, p2, p2s);

		// Symmetrize position
		if (tasks.doPos && asyms.positions[mi]) {
			// Calculate average
			Vector3 avg;
			int avgcount = 0;
			if (selVerts[p1]) {
				Vector3 sum;
				for (int p : p2s)
					sum += tuss.addVerts[p].pos;
				avg += sum / p2s.size();
				++avgcount;
			}
			if (p1 == p2)
				avg.x = 0;
			else {
				avg.x = -avg.x;
				if (selVerts[p2]) {
					Vector3 sum;
					for (int p : p1s)
						sum += tuss.addVerts[p].pos;
					avg += sum / p1s.size();
					++avgcount;
				}
				avg /= avgcount;
			}

			// Store average
			if (selVerts[p1]) {
				for (int p : p1s) {
					tuss.addVerts[p].pos = avg;
					vChanged[p] = true;
				}
			}
			if (selVerts[p2]) {
				avg.x = -avg.x;
				for (int p : p2s) {
					tuss.addVerts[p].pos = avg;
					vChanged[p] = true;
				}
			}
		}

		// Symmetrize position slider diffs
		for (int si = 0; si < nSliders; ++si) {
			if (!tasks.doSliders[si])
				continue;
			const auto& sd = asyms.sliders[si];
			if (!sd.aflags[mi])
				continue;

			// Calculate average
			Vector3 avg;
			int avgcount = 0;
			if (selVerts[p1]) {
				Vector3 sum;
				for (int p : p2s)
					sum += GetUSliderDiff(tuss.addVerts[p], sd.sliderName);
				avg = sum / p2s.size();
				++avgcount;
			}
			if (p1 == p2)
				avg.x = 0;
			else {
				avg.x = -avg.x;
				if (selVerts[p2]) {
					Vector3 sum;
					for (int p : p1s)
						sum += GetUSliderDiff(tuss.addVerts[p], sd.sliderName);
					avg += sum / p1s.size();
					++avgcount;
				}
				avg /= avgcount;
			}

			// Store average
			if (selVerts[p1]) {
				for (int p : p1s) {
					SetUSliderDiff(tuss.addVerts[p], sd.sliderName, avg);
					vChanged[p] = true;
				}
			}
			if (selVerts[p2]) {
				avg.x = -avg.x;
				for (int p : p2s) {
					SetUSliderDiff(tuss.addVerts[p], sd.sliderName, avg);
					vChanged[p] = true;
				}
			}
		}

		// Symmetrize bone weights
		for (int bi = 0; bi < nBones; ++bi) {
			// Figure out exactly what we need to do for this match, given
			// all our flags.  Note that there are cases (such as with
			// self-matches) where we update a weight twice or sum some
			// values twice, but the result is always the same.  Trying to
			// make this code update things just once or sum things just
			// once would make it much more complex.
			const auto& bd1 = asyms.bones[bi];
			const auto& bd2 = asyms.bones[bi + bd1.mirroroffset];
			if (!bd1.aflags[mi])
				continue;
			bool doThisBone = selVerts[p1] && tasks.doBones[bi];
			bool doMirrorBone = selVerts[p2] && tasks.doBones[bi + bd1.mirroroffset];
			if (!doThisBone && !doMirrorBone)
				continue;

			// Calculate average
			float avg = 0.0f;
			int avgcount = 0;
			if (doThisBone) {
				float sum = 0.0f;
				for (int p : p2s)
					sum += GetUWeight(tuss.addVerts[p], bd2.boneName);
				avg += sum / p2s.size();
				++avgcount;
			}
			if (doMirrorBone) {
				float sum = 0.0f;
				for (int p : p1s)
					sum += GetUWeight(tuss.addVerts[p], bd1.boneName);
				avg += sum / p1s.size();
				++avgcount;
			}
			avg /= avgcount;

			// Store average
			if (doThisBone) {
				if (!weightAdjustPoint[p1s[0]]) {
					weightAdjustPoint[p1s[0]] = true;
					nzer.GrabOneVertexStartingWeights(p1s[0]);
				}
				SetUWeight(tuss, p1s[0], bd1.boneName, avg);
			}
			if (doMirrorBone) {
				if (!weightAdjustPoint[p2s[0]]) {
					weightAdjustPoint[p2s[0]] = true;
					nzer.GrabOneVertexStartingWeights(p2s[0]);
				}
				SetUWeight(tuss, p2s[0], bd2.boneName, avg);
			}
		}
	}

	// Normalize weights and transfer them from tuss.boneWeights to
	// tuss.addVerts.
	for (int p = 0; p < nVerts; ++p) {
		if (!weightAdjustPoint[p])
			continue;

		Mesh::GetWeldSet(weldVerts, p, p1s);
		for (int p1 : p1s)
			vChanged[p1] = true;

		// Normalize weights
		nzer.AdjustWeights(p);

		// Transfer results
		for (const auto& bw : tuss.boneWeights) {
			auto wit = bw.weights.find(p);
			if (wit == bw.weights.end())
				continue;
			float w = wit->second.endVal;
			for (int p1 : p1s)
				SetUWeight(tuss.addVerts[p1], bw.boneName, w);
		}
	}

	// Transfer the data for just the changed vertices from tuss to uss.
	for (int vi = 0; vi < nVerts; ++vi) {
		if (!vChanged[vi])
			continue;
		uss.delVerts.push_back(std::move(tuss.delVerts[vi]));
		uss.addVerts.push_back(std::move(tuss.addVerts[vi]));
	}

	// Collect triangle data.  Every triangle that uses any changed vertex
	// must be deleted and restored.
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);
	int nTris = static_cast<int>(tris.size());
	std::vector<uint32_t> triInds;
	for (int ti = 0; ti < nTris; ++ti)
		if (vChanged[tris[ti].p1] || vChanged[tris[ti].p2] || vChanged[tris[ti].p3])
			triInds.push_back(ti);
	CollectTriangleData(shape, uss, triInds);
	uss.addTris = uss.delTris;
}

std::vector<bool> OutfitProject::CalculateAsymmetricTriangleVertexMask(NiShape* shape, const Mesh::WeldVertsType& weldVerts) {
	// Ideally, this function would return a triangle mask.  But triangle
	// masks aren't implemented in Outfit Studio yet.  So we have to
	// convert the information to a vertex mask.

	// Gather shape's vertices and triangles
	std::vector<Vector3> verts;
	workNif.GetVertsForShape(shape, verts);
	std::vector<Triangle> tris;
	shape->GetTriangles(tris);

	// Find vertex symmetries
	SymmetricVertices symverts;
	MatchSymmetricVertices(shape, weldVerts, symverts);

	// Create welded-vertex set for every vertex
	std::vector<std::vector<int>> weldSets(verts.size());
	for (int i = 0; i < static_cast<int>(verts.size()); ++i)
		Mesh::GetWeldSet(weldVerts, i, weldSets[i]);

	// Create mirror vertex map
	std::vector<int> mVerts(verts.size(), -1);
	for (const auto& mp : symverts.matches) {
		for (int vi : weldSets[mp.first])
			mVerts[vi] = mp.second;
		for (int vi : weldSets[mp.second])
			mVerts[vi] = mp.first;
	}

	// Create set of Triangles so we can quickly check if a triangle exists
	std::unordered_set<Triangle> triSet;
	for (Triangle t : tris) {
		t.rot();
		triSet.insert(t);
	}

	// Main loop through triangles
	std::vector<bool> mask(verts.size(), false);
	for (const Triangle& t : tris) {
		// m1, m2, m3: mirror of each of the triangle's points.  Note that
		// p2 and p3 are swapped, as we flip orientation when mirroring.
		int m1 = mVerts[t.p1];
		int m2 = mVerts[t.p3];
		int m3 = mVerts[t.p2];
		bool bad = false;
		// If any of m1, m2, or m3 is negative, that means the point does
		// not have a mirror match.
		if (m1 < 0 || m2 < 0 || m3 < 0)
			bad = true;
		if (!bad) {
			// Search through the weld sets for our three mirror points,
			// looking for a mirror triangle.
			bool gotmatch = false;
			for (int wm1 : weldSets[m1])
			for (int wm2 : weldSets[m2])
			for (int wm3 : weldSets[m3]) {
				Triangle mt(wm1, wm2, wm3);
				mt.rot();
				if (triSet.count(mt))
					gotmatch = true;
			}
			bad = !gotmatch;
		}
		if (bad) {
			// We've found a triangle that does not have a mirror.
			for (int i : weldSets[t.p1])
				mask[i] = true;
			for (int i : weldSets[t.p2])
				mask[i] = true;
			for (int i : weldSets[t.p3])
				mask[i] = true;
		}
	}
	return mask;
}
