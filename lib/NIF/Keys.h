/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"

enum KeyType : uint {
	NO_INTERP,
	LINEAR_KEY,
	QUADRATIC_KEY,
	TBC_KEY,
	XYZ_ROTATION_KEY,
	CONST_KEY
};

struct TBC {
	float tension = 0.0f;
	float bias = 0.0f;
	float continuity = 0.0f;
};

template<typename T>
struct Key {
	float time = 0.0f;
	T value;
	T forward;
	T backward;
	TBC tbc;
};

template<typename T>
class KeyGroup {
private:
	uint numKeys = 0;
	KeyType interpolation = NO_INTERP;
	std::vector<Key<T>> keys;

public:
	void Get(NiStream& stream) {
		stream >> numKeys;
		keys.resize(numKeys);

		if (numKeys > 0) {
			stream >> interpolation;

			for (int i = 0; i < numKeys; i++) {
				Key<T> key;
				stream >> key.time;
				stream >> key.value;

				switch (interpolation) {
				case QUADRATIC_KEY:
					stream >> key.forward;
					stream >> key.backward;
					break;
				case TBC_KEY:
					stream >> key.tbc;
					break;
				default:
					break;
				}

				keys[i] = std::move(key);
			}
		}
	}

	void Put(NiStream& stream) {
		stream << numKeys;

		if (numKeys > 0) {
			stream << interpolation;

			for (int i = 0; i < numKeys; i++) {
				stream << keys[i].time;
				stream << keys[i].value;

				switch (interpolation) {
				case QUADRATIC_KEY:
					stream << keys[i].forward;
					stream << keys[i].backward;
					break;
				case TBC_KEY:
					stream << keys[i].tbc;
					break;
				default:
					break;
				}
			}
		}
	}

	KeyType GetInterpolationType() {
		return interpolation;
	}

	void SetInterpolationType(const KeyType interp) {
		interpolation = interp;
	}

	uint GetNumKeys() {
		return numKeys;
	}

	Key<T> GetKey(const int id) {
		return keys[id];
	}

	void SetKey(const int id, const Key<T>& key) {
		keys[id] = key;
	}

	void AddKey(const Key<T>& key) {
		keys.push_back(key);
		numKeys++;
	}

	void RemoveKey(const int id) {
		keys.erase(keys.begin() + id);
		numKeys--;
	}

	void ClearKeys() {
		keys.clear();
		numKeys = 0;
	}
};
