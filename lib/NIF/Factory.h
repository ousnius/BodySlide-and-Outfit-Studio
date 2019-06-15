/*
BodySlide and Outfit Studio
See the included LICENSE file
*/

#pragma once

#include "BasicTypes.h"
#include "Animation.h"
#include "bhk.h"
#include "ExtraData.h"
#include "Geometry.h"
#include "Keys.h"
#include "Objects.h"
#include "Particles.h"
#include "Shaders.h"
#include "Skin.h"

#include <unordered_map>

class NiFactory {
public:
	virtual NiObject* Create() = 0;
	virtual NiObject* Load(NiStream& stream) = 0;
};

template<typename T>
class NiFactoryType : public NiFactory {
public:
	// Create new NiObject
	virtual NiObject* Create() override {
		return new T();
	}

	// Load new NiObject from file
	virtual NiObject* Load(NiStream& stream) override {
		T* nio = new T();
		nio->Get(stream);
		return nio;
	}
};

class NiFactoryRegister {
public:
	// Constructor registers the block types
	NiFactoryRegister();

	template<typename T>
	void RegisterFactory() {
		// Any NiObject can be registered together with its block name
		m_registrations.emplace(T::BlockName, std::make_shared<NiFactoryType<T>>());
	}

	// Get block factory via header std::string
	std::shared_ptr<NiFactory> GetFactoryByName(const std::string& name) {
		auto it = m_registrations.find(name);
		if (it != m_registrations.end())
			return it->second;

		return nullptr;
	}

	// Get static instance of factory register
	static NiFactoryRegister& Get();

protected:
	std::unordered_map<std::string, std::shared_ptr<NiFactory>> m_registrations;
};
