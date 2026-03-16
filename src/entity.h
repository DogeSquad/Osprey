#pragma once

#include "component.h"

#include <vector>
#include <memory>
#include <string>

namespace osp {

class Entity {
private:
	std::string name;
	bool active = true;
	std::vector<std::unique_ptr<Component>> components;
	
public:
	explicit Entity(const std::string& entityName) : name(entityName) {}

	const std::string& GetName() { return name; }
	bool IsActive() { return active; }
	void SetActive(bool isActive) { active = isActive; }

	void Initialize();
	void Update(float deltaTime);
	void Render();

	template<typename T, typename... Args> 
	T* AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* componentPtr = component.get();
		components.push_back(std::move(component));

		return componentPtr;
	}

	template<typename T> 
	T* GetComponent()
	{
		for (auto& component : components) {
			if (T* result = dynamic_cast<T*>(component.get())) {
				return result;
			}
		}

		return nullptr;
	}

	template<typename T>
	bool RemoveComponent()
	{
		for (auto it = components.begin(); it != components.end(); ++it) {
			if (dynamic_cast<T*>(it->get())) {
				components.erase(it);
				return true;
			}
		}
		return false;
	}

};

} // namespace osp