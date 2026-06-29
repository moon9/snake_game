#ifndef __KERNEL_ECS_HPP__
#define __KERNEL_ECS_HPP__

#include <bitset>
#include <cstdint>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace kernel {

static constexpr std::uint8_t COMPONENT_MAX = 32;

using Entity = std::uint32_t;
using EntitySignature = std::bitset<COMPONENT_MAX>;
using ComponentType = std::uint8_t;

class IComponentStorage {
public:
	virtual ~IComponentStorage() = default;
	virtual void Insert(Entity e, const void *data) = 0;
	virtual void Remove(Entity e) = 0;
	virtual void * Get(Entity e) = 0;
	virtual bool Has(Entity e) = 0;
	virtual void EntityDestroyed(Entity e) = 0;
};

template<typename T>
class ComponentStorage : public IComponentStorage {
private:
	std::vector<T> data_;
	std::unordered_map<Entity, std::size_t> entity_to_index_;
	std::vector<Entity> index_to_entity_;

	[[noreturn]] void entityNotExist(Entity e) {
		throw std::runtime_error(std::format("entity '{}' not exist in '{}' component storage", e, typeid(T).name()));
	}

	[[noreturn]] void entityAlreadyContain(Entity e) {
		throw std::runtime_error(std::format("entity '{}' already contain '{}' component", e, typeid(T).name()));
	}
public:
	void Insert(Entity e, const void *data) override {
		if (this->entity_to_index_.contains(e))
			this->entityAlreadyContain(e);

		this->entity_to_index_[e] = this->data_.size();
		this->index_to_entity_.push_back(e);
		this->data_.push_back(*static_cast<const T *>(data));
	}

	void Remove(Entity e) override {
		auto it = this->entity_to_index_.find(e);

		if (it == this->entity_to_index_.end())
			this->entityNotExist(e);

		std::size_t idx = it->second;
		std::size_t last_idx = this->data_.size() - 1;
		Entity last_entity = this->index_to_entity_.back();

		if (idx != last_idx) {
			this->data_[idx] = std::move(this->data_[last_idx]);
			this->entity_to_index_[last_entity] = idx;
			this->index_to_entity_[idx] = last_entity;
		}

		this->data_.pop_back();
		this->index_to_entity_.pop_back();
		this->entity_to_index_.erase(it);
	}

	void * Get(Entity e) override {
		auto it = this->entity_to_index_.find(e);

		if (it == this->entity_to_index_.end())
			this->entityNotExist(e);

		return static_cast<void *>(&this->data_[it->second]);
	}

	bool Has(Entity e) override {
		return this->entity_to_index_.contains(e);
	}

	void EntityDestroyed(Entity e) override {
		if (this->Has(e))
			this->Remove(e);
	}
};

class ISystem {
public:
	virtual ~ISystem() = default;
	virtual void AddEntity(Entity e) = 0;
	virtual void RemoveEntity(Entity e) = 0;
	virtual void Processing() = 0;
	virtual EntitySignature Signature() const = 0;
	virtual bool HasEntity(Entity e) const = 0;
};

template<typename T>
concept SystemHandler = requires(T t, Entity e, std::span<Entity> span) {
	{ t.Handle(span) } -> std::same_as<void>;
	{ t.Filter(e) } -> std::same_as<bool>;
	{ t.Signature() } -> std::same_as<EntitySignature>;
};

template<SystemHandler T>
class System : public ISystem {
private:
	std::string name_ = "default system";
	std::vector<Entity> entities_{};
	T handler_;
public:
	System(std::string name, T handler) : 
	name_{name}, handler_{std::move(handler)} {}

	void AddEntity(Entity e) override {
		if (this->handler_.Filter(e))
			this->entities_.push_back(e);
	}

	void RemoveEntity(Entity e) override {
		if (!this->handler_.Filter(e))
			return;

		for (auto it = this->entities_.begin(); it != this->entities_.end(); it++)
			if ((*it) == e) {
				entities_.erase(it);
				return;
			}

		throw std::runtime_error(std::format("'{}': no entity '{}' was found", this->name_, e));
	}

	void Processing() override {
		this->handler_.Handle(this->entities_);
	}

	EntitySignature Signature() const override {
		return this->handler_.Signature();
	}

	bool HasEntity(Entity e) const override {
		for (auto it = this->entities_.begin(); it != this->entities_.end(); it++)
			if ((*it) == e) {
				return true;
			}
		return false;
	}
};

class IECS {
public:
	virtual ~IECS() = default;
	virtual void Reset() = 0;

	// Entity.
	virtual Entity CreateEntity() = 0;
	virtual void DestroyEntity(Entity e) = 0;
	virtual void SetSignatureEntity(Entity e, EntitySignature s) = 0;
	virtual EntitySignature SignatureEntity(Entity e) = 0;
	virtual const std::vector<Entity> & GetEntities() = 0;

	// Component raw.
	virtual void RegisterComponentRaw(const std::type_info &type, std::unique_ptr<IComponentStorage> &storage) = 0;
	virtual ComponentType GetComponentTypeRaw(const std::type_info &type) const = 0;
	virtual void AddComponentRaw(Entity e, const std::type_info &type, const void *data) = 0;
	virtual void * GetComponentRaw(Entity e, const std::type_info &type) = 0;
	virtual void RemoveComponentRaw(Entity e, const std::type_info &type) = 0;
	virtual bool HasComponentRaw(Entity e, const std::type_info &type) const = 0;

	// Component template interface.
	template<typename T>
	void RegisterComponent(std::unique_ptr<IComponentStorage> &&storage) {
		this->RegisterComponentRaw(typeid(T), storage);
	}

	template<typename T>
	ComponentType GetComponentType() {
		return this->GetComponentTypeRaw(typeid(T));
	}

	template<typename T>
	void AddComponent(Entity e, const T &component) {
		this->AddComponentRaw(e, typeid(T), &component);
	}

	template<typename T>
	T & GetComponent(Entity e) {
		void *ptr = this->GetComponentRaw(e, typeid(T));

		if (!ptr)
			throw std::runtime_error(std::format("entity '{}' not contain component '{}'", e, typeid(T).name()));

		return *static_cast<T *>(ptr);
	}

	template<typename T>
	void RemoveComponent(Entity e) {
		this->RemoveComponentRaw(e, typeid(T));
	}

	template<typename T>
	bool HasComponent(Entity e) {
		return this->HasComponentRaw(e, typeid(T));
	}

	// System.
	virtual void AddSystem(std::unique_ptr<ISystem> system) = 0;
	virtual void InvokeSystems() = 0;
};

std::unique_ptr<IECS> CreateECS();

} // namespace kernel

#endif /* __KERNEL_ECS_HPP__ */
