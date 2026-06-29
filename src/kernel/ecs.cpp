#include "src/kernel/include_internal/ecs.hpp"

#include <unordered_map>
#include <format>

namespace kernel {

class EntityManager : public IEntityManager {
private:
	Entity entity_{0};
	std::vector<Entity> reuse_{};
	std::vector<Entity> used_{};
	std::unordered_map<Entity, EntitySignature> signatures_;

	[[noreturn]] void entityNotExist(Entity e) const {
		throw std::runtime_error(std::format("entity '{}' not exist", e));
	}
public:
	Entity Create() override {
		Entity e;

		if (this->reuse_.size() > 0) {
			e = this->reuse_.back();
			this->reuse_.pop_back();
		} else {
			e = this->entity_++;
		}
		this->used_.push_back(e);
		this->signatures_[e] = EntitySignature{};
		return e;
	}

	void Destroy(Entity e) override {
		for (auto it = this->used_.begin(); it != this->used_.end(); it++)
			if (*it == e) {
				this->used_.erase(it);
				this->signatures_.erase(e);
				this->reuse_.push_back(e);
				return;
			}
		this->entityNotExist(e);
	}

	void SetSignature(Entity e, EntitySignature s) override {
		auto it = this->signatures_.find(e);

		if (it == this->signatures_.end())
			entityNotExist(e);
		this->signatures_[e] = s;
	}

	EntitySignature Signature(Entity e) override {
		auto it = this->signatures_.find(e);

		if (it == this->signatures_.end())
			entityNotExist(e);
		return this->signatures_[e];
	}

	void Reset() override {
		this->entity_ = 0;
		this->used_.clear();
		this->reuse_.clear();
		this->signatures_.clear();
	}

	const std::vector<Entity> & GetEntities() override {
		return this->used_;
	}
};

class ComponentManager : public IComponentManager {
private:
	std::unordered_map<ComponentType, std::unique_ptr<IComponentStorage>> storages_;
	std::unordered_map<std::size_t, ComponentType> hash_to_type_;
	ComponentType type_id_{0};

	[[noreturn]] void storageNotRegister(const std::type_info &type) const {
		throw std::runtime_error(std::format("storage for '{}' component not registered", type.name()));
	}

	[[noreturn]] void maxComponentsReached(const std::type_info &type) const {
		throw std::runtime_error(std::format("storage for '{}' component can't register, max component types reached", type.name()));
	}
public:
	void RegisterComponentRaw(const std::type_info &type, std::unique_ptr<IComponentStorage> &storage) override {
		if (this->type_id_ >= COMPONENT_MAX)
			this->maxComponentsReached(type);

		ComponentType type_id = this->type_id_++;

		this->hash_to_type_[type.hash_code()] = type_id;
		this->storages_[type_id] = std::move(storage);
	}

	ComponentType GetComponentTypeRaw(const std::type_info &type) const override {
		auto it = this->hash_to_type_.find(type.hash_code());

		if (it == this->hash_to_type_.end())
			this->storageNotRegister(type);
		return it->second;
	}

	void AddComponentRaw(Entity e, const std::type_info &type, const void *data) override {
		auto it = this->hash_to_type_.find(type.hash_code());

		if (it == this->hash_to_type_.end())
			this->storageNotRegister(type);

		this->storages_[it->second]->Insert(e, data);
	}

	void * GetComponentRaw(Entity e, const std::type_info &type) override {
		auto it = this->hash_to_type_.find(type.hash_code());

		if (it == this->hash_to_type_.end())
			this->storageNotRegister(type);
		
		return this->storages_[it->second]->Get(e);
	}

	void RemoveComponentRaw(Entity e, const std::type_info &type) override {
		auto it = this->hash_to_type_.find(type.hash_code());

		if (it == this->hash_to_type_.end())
			this->storageNotRegister(type);

		return this->storages_[it->second]->Remove(e);
	}

	bool HasComponentRaw(Entity e, const std::type_info &type) const override {
		auto it = this->hash_to_type_.find(type.hash_code());

		if (it == this->hash_to_type_.end())
			this->storageNotRegister(type);

		auto it2 = this->storages_.find(it->second);
		if (it2 == this->storages_.end())
			this->storageNotRegister(type);
		return it2->second->Has(e);
	}

	void EntityDestroyed(Entity e) override {
		for (auto &it : this->storages_)
			it.second->EntityDestroyed(e);
	}

	void Reset() override {
		this->storages_.clear();
		this->type_id_ = 0;
	}
};

class SystemManager : public ISystemManager {
private:
	std::vector<std::unique_ptr<ISystem>> systems_{};
public:
	void Add(std::unique_ptr<ISystem> system) override {
		this->systems_.push_back(std::move(system));
	}

	void Invoke() override {
		for (auto &system : this->systems_)
			system->Processing();
	}

	void EntityDestroyed(Entity e) override {
		for (auto &system : this->systems_)
			system->RemoveEntity(e);
	}

	void Reset() override {
		this->systems_.clear();
	}

	void EntitySignatureChanged(Entity e, EntitySignature s) override {
		for (auto &system : this->systems_) {
			auto system_sig = system->Signature();
			if (system->HasEntity(e)) {
				if ((s & system_sig) != system_sig)
					system->RemoveEntity(e);
			} else {
				if ((s & system_sig) == system_sig)
					system->AddEntity(e);
			}
		}
	}
};

class ECSFacade : public IECS {
private:
	std::unique_ptr<IEntityManager> entity_manager_{nullptr};
	std::unique_ptr<IComponentManager> component_manager_{nullptr};
	std::unique_ptr<ISystemManager> system_manager_{nullptr};
public:
	ECSFacade(std::unique_ptr<IEntityManager> entity_manager,
		std::unique_ptr<IComponentManager> component_manager,
		std::unique_ptr<ISystemManager> system_manager) :
		entity_manager_{std::move(entity_manager)},
		component_manager_{std::move(component_manager)},
		system_manager_{std::move(system_manager)} {}

	void Reset() override {
		this->system_manager_->Reset();
		this->component_manager_->Reset();
		this->entity_manager_->Reset();
	}

	Entity CreateEntity() override {
		return this->entity_manager_->Create();
	}

	void DestroyEntity(Entity e) override {
		this->system_manager_->EntityDestroyed(e);
		this->component_manager_->EntityDestroyed(e);
		this->entity_manager_->Destroy(e);
	}

	void SetSignatureEntity(Entity e, EntitySignature s) override {
		this->entity_manager_->SetSignature(e, s);
	}

	EntitySignature SignatureEntity(Entity e) {
		return this->entity_manager_->Signature(e);
	}

	const std::vector<Entity> & GetEntities() override {
		return this->entity_manager_->GetEntities();
	}

	void RegisterComponentRaw(const std::type_info &type, std::unique_ptr<IComponentStorage> &storage) override {
		this->component_manager_->RegisterComponentRaw(type, storage);
	}

	ComponentType GetComponentTypeRaw(const std::type_info &type) const override {
		return this->component_manager_->GetComponentTypeRaw(type);
	}

	void AddComponentRaw(Entity e, const std::type_info &type, const void *data) override {
		this->component_manager_->AddComponentRaw(e, type, data);

		auto s = this->entity_manager_->Signature(e);
		auto type_id = this->component_manager_->GetComponentTypeRaw(type);

		s.set(type_id);
		this->entity_manager_->SetSignature(e, s);
		this->system_manager_->EntitySignatureChanged(e, s);
	}

	void * GetComponentRaw(Entity e, const std::type_info &type) override {
		return this->component_manager_->GetComponentRaw(e, type);
	}

	void RemoveComponentRaw(Entity e, const std::type_info &type) override {
		this->component_manager_->RemoveComponentRaw(e, type);

		auto s = this->entity_manager_->Signature(e);
		auto type_id = this->component_manager_->GetComponentTypeRaw(type);

		s.set(type_id, false);
		this->entity_manager_->SetSignature(e, s);
		this->system_manager_->EntitySignatureChanged(e, s);
	}

	bool HasComponentRaw(Entity e, const std::type_info &type) const override {
		return this->component_manager_->HasComponentRaw(e, type);
	}

	void AddSystem(std::unique_ptr<ISystem> system) override {
		this->system_manager_->Add(std::move(system));
	}

	void InvokeSystems() override {
		this->system_manager_->Invoke();
	}
};

std::unique_ptr<IECS> CreateECS() {
	return std::make_unique<ECSFacade>(std::make_unique<EntityManager>(), std::make_unique<ComponentManager>(), std::make_unique<SystemManager>());
}

}