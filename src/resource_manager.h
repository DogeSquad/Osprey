#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <typeindex>

namespace osp {

class Resource final {
private: 
	std::string resourceId;
	bool loaded = false;

public:
	explicit Resource(const std::string& id) : resourceId(id) {}
	virtual ~Resource() = default;

	const std::string& GetId() const { return resourceId; }
	bool IsLoaded() const { return loaded; }

	bool Load() {
		loaded = doLoad();
		return loaded;
	}

	void Unload() {
		doUnload();
		loaded = false;
	}

protected:
	virtual bool doLoad() = 0;
	virtual bool doUnload() = 0;
};

template<typename T>
class ResourceHandle {
private:
	std::string resourceId;
	ResourceManager* resourceManager;

public:
	ResourceHandle() : resourceManager(nullptr) {}

	ResourceHandle(const std::string& id, ResourceManager* manager) 
		: resourceId(id), resourceManager(manager) {}

	T* Get() const {
		if (!resourceManager) return nullptr;
		return resourceManager->GetResource<T>(resourceId);
	}

	bool IsValid() const {
		return resourceManager && resourceManager->HasResource<T>(resourceId);
	}

	const std::string& GetId() const { return resourceId; }

	T* operator->() const { return Get(); }
	T& operator*() const { return *Get(); }
	operator bool() const { return IsValid(); }
};

class ResourceManager {
private:
	std::unordered_map<std::type_index,
		std::unordered_map<std::string, std::shared_ptr<Resource>>> resources;

	struct ResourceData {
		std::shared_ptr<Resource> resource;
		int refCount;
	};
	std::unordered_map<std::type_index,
		std::unordered_map<std::string, ResourceData>> refCounts;
};

}
