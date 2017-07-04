/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
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
	float tension;
	float bias;
	float continuity;
};

template<typename T>
struct Key {
	float time;
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
	KeyGroup() {}
	KeyGroup(NiStream& stream) {
		Get(stream);
	}

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
				}
			}
		}
	}
};
