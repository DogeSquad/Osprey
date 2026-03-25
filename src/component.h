#pragma once

namespace osp {

class Entity;

class Component {
public:
	enum class State {
		Uninitialized,
		Initializing,
		Active,
		Destroying,
		Destroyed
	};

protected:
	State state = State::Uninitialized;
	Entity* owner = nullptr;

public:
	virtual ~Component();

	void Initialize();
	void Destroy();

	bool IsActive() const { return state == State::Active; }

	void SetOwner(Entity* entity) { owner = entity; }
	Entity* GetOwner() const { return owner; }

protected:
	virtual void OnInitialize() {}
	virtual void OnDestroy() {}
	virtual void Update(float deltaTime) {}
	virtual void Render() {}

	friend class Entity; // Allow Entity to call protected methods
};

}