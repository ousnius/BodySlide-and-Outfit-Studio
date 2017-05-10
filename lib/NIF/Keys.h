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
	KeyGroup(std::fstream& file) {
		Get(file);
	}

	void Get(std::fstream& file) {
		file.read((char*)&numKeys, 4);
		keys.resize(numKeys);

		if (numKeys > 0) {
			file.read((char*)&interpolation, 4);

			for (int i = 0; i < numKeys; i++) {
				Key<T> key;
				file.read((char*)&key.time, 4);
				file.read((char*)&key.value, sizeof(T));

				switch (interpolation) {
				case 2:
					file.read((char*)&key.forward, sizeof(T));
					file.read((char*)&key.backward, sizeof(T));
					break;
				case 3:
					file.read((char*)&key.tbc, 12);
					break;
				}

				keys[i] = std::move(key);
			}
		}
	}

	void Put(std::fstream& file) {
		file.write((char*)&numKeys, 4);

		if (numKeys > 0) {
			file.write((char*)&interpolation, 4);

			for (int i = 0; i < numKeys; i++) {
				file.write((char*)&keys[i].time, 4);
				file.write((char*)&keys[i].value, sizeof(T));

				switch (interpolation) {
				case 2:
					file.write((char*)&keys[i].forward, sizeof(T));
					file.write((char*)&keys[i].backward, sizeof(T));
					break;
				case 3:
					file.write((char*)&keys[i].tbc, 12);
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
