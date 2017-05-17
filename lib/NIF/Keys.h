/*
BodySlide and Outfit Studio
Copyright (C) 2017  Caliente & ousnius
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"

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
	uint interpolation;
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
				case 2:
					stream >> key.forward;
					stream >> key.backward;
					break;
				case 3:
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
				case 2:
					stream << keys[i].forward;
					stream << keys[i].backward;
					break;
				case 3:
					stream << keys[i].tbc;
					break;
				}
			}
		}
	}

	int CalcGroupSize() {
		int groupSize = 4;

		if (numKeys > 0) {
			groupSize += 4;
			groupSize += 4 * numKeys;
			groupSize += sizeof(T) * numKeys;

			switch (interpolation) {
			case 2:
				groupSize += sizeof(T) * numKeys * 2;
				break;
			case 3:
				groupSize += 12 * numKeys;
				break;
			}
		}

		return groupSize;
	}
};
