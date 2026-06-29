#ifndef __INTERNAL_KERNEL_ECS_HPP__
#define __INTERNAL_KERNEL_ECS_HPP__

#include "kernel/ecs.hpp"

namespace kernel {

class IResetManager {
public:
	virtual ~IResetManager() = default;
	virtual void Reset() = 0;
};

class IEntityManager : public IResetManager {
public:
	virtual ~IEntityManager() = default;
	virtual Entity Create() = 0;
	virtual void Destroy(Entity e) = 0;
	virtual void SetSignature(Entity e, EntitySignature s) = 0;
	virtual EntitySignature Signature(Entity e) = 0;
	virtual const std::vector<Entity> & GetEntities() = 0;
};

class IComponentManager : public IResetManager {
public:
	virtual ~IComponentManager() = default;

	virtual void RegisterComponentRaw(const std::type_info &type, std::unique_ptr<IComponentStorage> &storage) = 0;
	virtual ComponentType GetComponentTypeRaw(const std::type_info &type) const = 0;
	virtual void AddComponentRaw(Entity e, const std::type_info &type, const void *data) = 0;
	virtual void * GetComponentRaw(Entity e, const std::type_info &type) = 0;
	virtual void RemoveComponentRaw(Entity e, const std::type_info &type) = 0;
	virtual bool HasComponentRaw(Entity e, const std::type_info &type) const = 0;
	virtual void EntityDestroyed(Entity e) = 0;
};

class ISystemManager : public IResetManager {
public:
	virtual ~ISystemManager() = default;
	virtual void Add(std::unique_ptr<ISystem> system) = 0;
	virtual void Invoke() = 0;
	virtual void EntityDestroyed(Entity e) = 0;
	virtual void EntitySignatureChanged(Entity e, EntitySignature s) = 0;
};

} // namespace kernel

#endif /* __INTERNAL_KERNEL_ECS_HPP__ */
