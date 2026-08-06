#ifdef USE_BULLET

#include "hdtCollider.h"
#include <algorithm>

namespace hdt
{

	void ColliderTree::insertCollider(const U32* keys, size_t keyCount, const Collider& c)
	{
		ColliderTree* p = this;
		for (size_t i = 0; i < keyCount && i < 4; ++i) {
			auto f = std::find_if(p->children.begin(), p->children.end(), [=](const ColliderTree& n) { return n.key == keys[i]; });
			if (f == p->children.end()) {
				p->children.push_back(ColliderTree(keys[i]));
				p = &p->children.back();
			} else
				p = &*f;
		}
		p->colliders.push_back(c);
	}

	// finds overlapping pairs between two collider trees
	void ColliderTree::checkCollisionL(ColliderTree* r, std::vector<std::pair<ColliderTree*, ColliderTree*>>& ret)
	{
		enum Mode : uint8_t
		{
			L,  // still splitting a, b is along for the ride
			R   // a is a leaf, now drilling into b's subtree
		};
		struct Entry
		{
			ColliderTree* a;
			ColliderTree* b;
			Mode mode;
		};
		// thread_local: stack is .clear()-ed on each call, so worker thread
		// recreation is harmless (fresh thread sees empty vector). The port
		// runs collision dispatch serially, so no re-entry aliasing is
		// possible; keep this in mind if parallelism is ever reintroduced.
		thread_local std::vector<Entry> stack;
		stack.clear();
		stack.push_back({ this, r, L });
		while (!stack.empty()) {
			if (ret.size() > MaxCollisionPairs) {
				break;
			}

			auto e = stack.back();
			stack.pop_back();

			if (e.a->isKinematic && e.b->isKinematic)
				continue;

			if (e.mode == L) {
				if (!e.a->aabbAll.collideWith(e.b->aabbAll))
					continue;

				// a is a leaf — switch to R-mode and walk down b
				if (e.a->numCollider && e.a->aabbMe.collideWith(e.b->aabbAll)) {
					if (e.a->aabbMe.collideWith(e.b->aabbMe))
						ret.push_back(std::make_pair(e.a, e.b));

					auto begin = e.b->children.data();
					// skip b's kinematic children if a is kinematic (would get culled above anyway)
					auto end = begin + (e.a->isKinematic ? e.b->dynChild : e.b->children.size());
					for (auto i = begin; i < end; ++i)
						stack.push_back({ e.a, i, R });
				}

				// keep splitting a — same kinematic shortcut
				auto begin = e.a->children.data();
				auto end = begin + (e.b->isKinematic ? e.a->dynChild : e.a->children.size());
				for (auto i = begin; i < end; ++i)
					stack.push_back({ i, e.b, L });
			} else {
				// a is always a leaf here (L only pushes R when numCollider is set)
				// numCollider check is technically redundant but whatever, it's cheap
				if (!e.a->numCollider)
					continue;
				if (!e.a->aabbMe.collideWith(e.b->aabbAll))
					continue;
				if (e.a->aabbMe.collideWith(e.b->aabbMe))
					ret.push_back(std::make_pair(e.a, e.b));

				auto begin = e.b->children.data();
				auto end = begin + (e.a->isKinematic ? e.b->dynChild : e.b->children.size());
				for (auto i = begin; i < end; ++i)
					stack.push_back({ e.a, i, R });
			}
		}
	}

	// dead code. checkCollisionL inlines R logic now. kept for API
	void ColliderTree::checkCollisionR(ColliderTree* r, std::vector<std::pair<ColliderTree*, ColliderTree*>>& ret)
	{
		if (isKinematic && r->isKinematic)
			return;

		if (numCollider) {
			if (!aabbMe.collideWith(r->aabbAll))
				return;

			if (aabbMe.collideWith(r->aabbMe))
				ret.push_back(std::make_pair(this, r));

			auto begin = r->children.data();
			auto end = begin + (isKinematic ? r->dynChild : r->children.size());
			for (auto i = begin; i < end; ++i)
				checkCollisionR(i, ret);
		}
	}

	void ColliderTree::clipCollider(const std::function<bool(const Collider&)>& func)
	{
		for (auto& i : children)
			i.clipCollider(func);

		colliders.erase(std::remove_if(colliders.begin(), colliders.end(), func), colliders.end());
		children.erase(std::remove_if(children.begin(), children.end(),
						   [](const ColliderTree& n) -> bool { return n.empty(); }),
			children.end());
	}

	void ColliderTree::updateKinematic(const std::function<float(const Collider*)>& func)
	{
		U32 k = true;
		for (auto& i : colliders) {
			i.flexible = func(&i);
			k &= i.flexible < FLT_EPSILON;
		}

		for (auto& i : children) {
			i.updateKinematic(func);
			k &= i.isKinematic;
		}

		std::sort(colliders.begin(), colliders.end(), [](const Collider& a, const Collider& b) {
			return a.flexible > b.flexible;
		});
		std::sort(children.begin(), children.end(), [](const ColliderTree& a, const ColliderTree& b) {
			return a.isKinematic < b.isKinematic;
		});

		isKinematic = k;

		if (k) {
			dynChild = dynCollider = 0;
		} else {
			for (dynChild = 0; dynChild < children.size(); ++dynChild)
				if (children[dynChild].isKinematic)
					break;

			for (dynCollider = 0; dynCollider < colliders.size(); ++dynCollider)
				if (colliders[dynCollider].flexible < FLT_EPSILON)
					break;
		}
	}

	void ColliderTree::updateAabb()
	{
		struct Frame
		{
			ColliderTree* node;
			size_t childIdx;
		};
		thread_local std::vector<Frame> stack;
		stack.clear();
		stack.push_back({ this, 0 });

		while (!stack.empty()) {
			auto& f = stack.back();
			if (f.childIdx < f.node->children.size()) {
				auto* child = &f.node->children[f.childIdx++];
				stack.push_back({ child, 0 });
				continue;
			}

			auto* node = f.node;
			stack.pop_back();

			// old code merged into aabb[0] directly which nuked collider 0's actual bbox,
			// making it always pass narrow-phase checks. accumulate in registers instead
			if (node->numCollider) {
				__m128 mn = node->aabb[0].m_min;
				__m128 mx = node->aabb[0].m_max;
				auto* aabbPtr = node->aabb + 1;
				auto* aabbEnd = node->aabb + node->numCollider;
				for (; aabbPtr < aabbEnd; ++aabbPtr) {
					mn = _mm_min_ps(mn, aabbPtr->m_min);
					mx = _mm_max_ps(mx, aabbPtr->m_max);
				}
				node->aabbMe.m_min = mn;
				node->aabbMe.m_max = mx;
			}

			__m128 allMin = node->aabbMe.m_min;
			__m128 allMax = node->aabbMe.m_max;
			for (auto& child : node->children) {
				allMin = _mm_min_ps(allMin, child.aabbAll.m_min);
				allMax = _mm_max_ps(allMax, child.aabbAll.m_max);
			}
			node->aabbAll.m_min = allMin;
			node->aabbAll.m_max = allMax;
		}
	}

	void ColliderTree::visitColliders(const std::function<void(Collider*)>& func)
	{
		for (auto& i : colliders)
			func(&i);

		for (auto& i : children)
			i.visitColliders(func);
	}

	void ColliderTree::optimize()
	{
		for (auto& i : children)
			i.optimize();

		children.erase(
			std::remove_if(children.begin(), children.end(), [](const ColliderTree& n) { return n.empty(); }),
			children.end());

		while (children.size() == 1 && children[0].colliders.empty()) {
			vectorA16<ColliderTree> temp;
			temp.swap(children.front().children);
			children.swap(temp);
		}

		if (children.size() == 1 && colliders.empty()) {
			colliders = children[0].colliders;

			vectorA16<ColliderTree> temp;
			temp.swap(children[0].children);
			children.swap(temp);
		}
	}

	// same deal as checkCollisionL - iterative, early return still works since we just bail out of the loop
	bool ColliderTree::collapseCollideL(ColliderTree* r)
	{
		enum Mode : uint8_t
		{
			L,
			R
		};
		struct Entry
		{
			ColliderTree* a;
			ColliderTree* b;
			Mode mode;
		};
		thread_local std::vector<Entry> stack;
		stack.clear();
		stack.push_back({ this, r, L });

		while (!stack.empty()) {
			auto e = stack.back();
			stack.pop_back();

			if (e.a->isKinematic && e.b->isKinematic)
				continue;

			if (e.mode == L) {
				if (!e.a->aabbAll.collideWith(e.b->aabbAll))
					continue;

				if (e.a->numCollider && e.a->aabbMe.collideWith(e.b->aabbAll)) {
					if (e.a->aabbMe.collideWith(e.b->aabbMe))
						return true;

					auto begin = e.b->children.data();
					auto end = begin + (e.a->isKinematic ? e.b->dynChild : e.b->children.size());
					for (auto i = begin; i < end; ++i)
						stack.push_back({ e.a, i, R });
				}

				auto begin = e.a->children.data();
				auto end = begin + (e.b->isKinematic ? e.a->dynChild : e.a->children.size());
				for (auto i = begin; i < end; ++i)
					stack.push_back({ i, e.b, L });
			} else {
				if (!e.a->numCollider)
					continue;
				if (!e.a->aabbMe.collideWith(e.b->aabbAll))
					continue;

				if (e.a->aabbMe.collideWith(e.b->aabbMe))
					return true;

				auto begin = e.b->children.data();
				auto end = begin + (e.a->isKinematic ? e.b->dynChild : e.b->children.size());
				for (auto i = begin; i < end; ++i)
					stack.push_back({ e.a, i, R });
			}
		}
		return false;
	}

	// dead code.. collapseCollideL inlines R logic now. kept for API
	bool ColliderTree::collapseCollideR(ColliderTree* r)
	{
		if (isKinematic && r->isKinematic)
			return false;

		if (numCollider) {
			if (!aabbMe.collideWith(r->aabbAll))
				return false;

			if (aabbMe.collideWith(r->aabbMe))
				return true;

			auto begin = r->children.data();
			auto end = begin + (isKinematic ? r->dynChild : r->children.size());
			for (auto i = begin; i < end; ++i)
				if (collapseCollideR(i))
					return true;
		}

		return false;
	}

	void ColliderTree::exportColliders(vectorA16<Collider>& exportTo)
	{
		numCollider = static_cast<hdt::U32>(colliders.size());
		cbuf = (Collider*)exportTo.size();
		for (auto& i : colliders) {
			exportTo.push_back(i);
		}

		for (auto& i : children)
			i.exportColliders(exportTo);
	}

	void ColliderTree::remapColliders(Collider* start, Aabb* startAabb)
	{
		vectorA16<Collider> tmp;
		colliders.swap(tmp);
		auto offset = (size_t)cbuf;
		cbuf = start + offset;
		aabb = startAabb + offset;

		for (auto& i : children)
			i.remapColliders(start, startAabb);
	}
}

#endif  // USE_BULLET
