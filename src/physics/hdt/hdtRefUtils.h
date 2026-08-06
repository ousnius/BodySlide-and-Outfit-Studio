#pragma once

// Standard C++ replacements for the game-engine facilities the original
// hdtSMP64 code relied on (CommonLibSSE smart pointers and interned strings,
// Intel TBB parallel loops). Simulation code is otherwise ported unchanged.
//
// The only file in this directory without an upstream counterpart; see
// README.md for where the rest comes from and under which license.

#include <atomic>
#include <cctype>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#if defined(_MSC_VER)
#define HDT_FORCEINLINE __forceinline
#else
#define HDT_FORCEINLINE inline __attribute__((always_inline))
#endif

// used between the struct/class keyword and the type name
#define HDT_ALIGN16 alignas(16)

namespace hdt {
// Intrusive ref-count base replacing RE::BSIntrusiveRefCounted. Ref<T> deletes
// the object when the count returns to zero.
class RefCounted {
public:
	RefCounted() = default;
	RefCounted(const RefCounted&) {}
	RefCounted& operator=(const RefCounted&) { return *this; }
	virtual ~RefCounted() = default;

	uint32_t IncRef() const { return ++m_refCount; }
	uint32_t DecRef() const { return --m_refCount; }
	long getRefCount() const { return m_refCount; }

private:
	mutable std::atomic<uint32_t> m_refCount{0};
};

// Intrusive smart pointer replacing RE::BSTSmartPointer.
template<class T>
class Ref {
public:
	Ref() = default;
	Ref(std::nullptr_t) {}
	Ref(T* p)
		: m_ptr(p) {
		if (m_ptr)
			m_ptr->IncRef();
	}
	Ref(const Ref& o)
		: Ref(o.m_ptr) {}
	Ref(Ref&& o) noexcept
		: m_ptr(o.m_ptr) {
		o.m_ptr = nullptr;
	}
	template<class U>
	Ref(const Ref<U>& o)
		: Ref(static_cast<T*>(o.get())) {}
	~Ref() { reset(); }

	Ref& operator=(const Ref& o) {
		Ref(o).swap(*this);
		return *this;
	}
	Ref& operator=(Ref&& o) noexcept {
		Ref(std::move(o)).swap(*this);
		return *this;
	}
	Ref& operator=(T* p) {
		Ref(p).swap(*this);
		return *this;
	}

	void reset() {
		if (m_ptr) {
			if (m_ptr->DecRef() == 0)
				delete m_ptr;
			m_ptr = nullptr;
		}
	}
	void swap(Ref& o) noexcept { std::swap(m_ptr, o.m_ptr); }

	T* get() const { return m_ptr; }
	T* operator->() const { return m_ptr; }
	T& operator*() const { return *m_ptr; }
	explicit operator bool() const { return m_ptr != nullptr; }
	bool operator==(const Ref& o) const { return m_ptr == o.m_ptr; }
	bool operator!=(const Ref& o) const { return m_ptr != o.m_ptr; }
	bool operator==(const T* p) const { return m_ptr == p; }
	bool operator!=(const T* p) const { return m_ptr != p; }

private:
	T* m_ptr = nullptr;
};

template<class T>
Ref<T> make_ref(T* p) {
	return Ref<T>(p);
}

// Case-insensitive string replacing RE::BSFixedString. Skyrim bone and tag
// names match case-insensitively; SMP XMLs in the wild depend on that.
class IDStr {
public:
	IDStr() = default;
	IDStr(const char* s)
		: m_str(s ? s : "") {}
	IDStr(std::string s)
		: m_str(std::move(s)) {}

	const char* c_str() const { return m_str.c_str(); }
	const std::string& str() const { return m_str; }
	bool empty() const { return m_str.empty(); }

	bool operator==(const IDStr& o) const {
		if (m_str.size() != o.m_str.size())
			return false;
		for (size_t i = 0; i < m_str.size(); ++i)
			if (std::tolower(static_cast<unsigned char>(m_str[i])) != std::tolower(static_cast<unsigned char>(o.m_str[i])))
				return false;
		return true;
	}
	bool operator!=(const IDStr& o) const { return !(*this == o); }

private:
	std::string m_str;
};

// Serial replacements for the TBB parallel loops of the original. The preview
// simulates a single actor, so parallelism is a contained future optimization.
namespace par {
	template<class It, class Fn>
	void for_each(It begin, It end, Fn fn) {
		for (; begin != end; ++begin)
			fn(*begin);
	}
	template<class Index, class Fn>
	void for_range(Index begin, Index end, Fn fn) {
		for (Index i = begin; i != end; ++i)
			fn(i);
	}
}
}

namespace std {
template<>
struct hash<hdt::IDStr> {
	size_t operator()(const hdt::IDStr& s) const {
		// FNV-1a over lowercased bytes so hashing matches the
		// case-insensitive equality.
		size_t h = 14695981039346656037ull;
		for (const char c : s.str()) {
			h ^= static_cast<size_t>(std::tolower(static_cast<unsigned char>(c)));
			h *= 1099511628211ull;
		}
		return h;
	}
};
}
