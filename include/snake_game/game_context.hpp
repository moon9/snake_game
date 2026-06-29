#ifndef __SNAKE_GAME_GAME_CONTEXT_HPP__
#define __SNAKE_GAME_GAME_CONTEXT_HPP__

#include <cmath>
#include <cstdint>
#include <functional>
#include <utility>
#include <random>

#include "kernel/game_state.hpp"
#include "kernel/ecs.hpp"
#include "snake_game/components.hpp"

namespace snake_game {

class IMapTracking {
public:
	virtual ~IMapTracking() = default;

	virtual void SetCell(std::uint32_t w, std::uint32_t h) = 0;
	virtual void ReleaseCell(std::uint32_t w, std::uint32_t h) = 0;
	virtual std::pair<std::uint32_t, std::uint32_t> GetFreeRndCell() = 0;
};

class IGameContext {
public:
	virtual ~IGameContext() = default;

	virtual kernel::IECS & ECS() = 0;
	virtual IMapTracking & MapTracking() = 0;

	virtual void AddToRelease(kernel::Entity e) = 0;

	virtual void ResetLevel() = 0;

	virtual void IncSpeed(std::float_t inc = 0.5f) = 0;
	virtual void IncSnake() = 0;

	virtual void SpawnNewGrowth() = 0;
	virtual void SpawnNewSpeed() = 0;
};

class ISnake {
public:
	virtual ~ISnake() = default;
	virtual kernel::Entity Head() const noexcept = 0;
	virtual kernel::Entity AddTail(std::int32_t xoff = 0, std::int32_t yoff = 0) = 0;
};

class Items {
protected:
	kernel::Entity entity_{0};
	IGameContext &ctx_;

public:
	Items(IGameContext &ctx, std::uint32_t type, std::int32_t x, std::int32_t y) : ctx_{ctx} {
		this->entity_ = this->ctx_.ECS().CreateEntity();
		this->ctx_.ECS().AddComponent<Meta>(this->entity_, Meta{
			.type = type,
		});
		this->ctx_.ECS().AddComponent<Position>(this->entity_, Position{
			.x = x,
			.y = y,
		});
		this->ctx_.MapTracking().SetCell(x, y);
	}

	virtual ~Items() {
		this->ctx_.ECS().DestroyEntity(this->entity_);
	}

	kernel::Entity GetEntity() const noexcept{
		return this->entity_;
	}
};

class RandomGen {
private:
	static thread_local std::mt19937 engine_;
	static thread_local bool init_;

public:
	static void Init(std::uint64_t seed = 0) {
		if (init_)
			return;

		std::seed_seq seq{seed};
		engine_.seed(seq);
		init_ = true;
	}

	static std::uint32_t gen(std::uint64_t max) {
		return std::uniform_int_distribution<std::uint64_t>(0, max)(engine_);
	}
};

using WindowResizeCb = std::function<void(std::int32_t, std::int32_t)>;

std::unique_ptr<IMapTracking> CreateFieldTrack(std::uint32_t width, std::uint32_t height);
std::unique_ptr<ISnake> CreateSnake(kernel::IECS &ecs, std::int32_t x, std::int32_t y, std::uint32_t tail_size);
std::unique_ptr<Items> CreateWall(IGameContext &ctx, std::int32_t x, std::int32_t y);
std::unique_ptr<Items> CreateSpeedMod(IGameContext &ctx, std::int32_t x, std::int32_t y);
std::unique_ptr<Items> CreateGrowthMod(IGameContext &ctx, std::int32_t x, std::int32_t y);
std::unique_ptr<kernel::IGameState> CreateGameContext(WindowResizeCb wnd_resize);

} // namespace snake_game

#endif /* __SNAKE_GAME_GAME_CONTEXT_HPP__ */