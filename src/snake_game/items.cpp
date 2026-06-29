#include "snake_game/game_context.hpp"

namespace snake_game {

class Wall: public Items {
public:
	Wall(IGameContext &ctx, std::int32_t x, std::int32_t y) : Items(ctx, ENTITY_TYPE_WALL, x, y) {
		this->ctx_.ECS().AddComponent<Collider>(this->entity_, Collider{});
	}
};

class SpeedMod: public Items {
public:
	SpeedMod(IGameContext &ctx, std::int32_t x, std::int32_t y) : Items{ctx, ENTITY_TYPE_MODIFICATOR_SPEED, x, y} {
		this->ctx_.ECS().AddComponent<Collider>(this->entity_, Collider{});
	}
};

class GrowthMod: public Items {
public:
	GrowthMod(IGameContext &ctx, std::int32_t x, std::int32_t y) : Items{ctx, ENTITY_TYPE_MODIFICATOR_GROWTH, x, y} {
		this->ctx_.ECS().AddComponent<Collider>(this->entity_, Collider{});
	}
};

std::unique_ptr<Items> CreateWall(IGameContext &ctx, std::int32_t x, std::int32_t y) {
	return std::make_unique<Wall>(ctx, x, y);
}

std::unique_ptr<Items> CreateSpeedMod(IGameContext &ctx, std::int32_t x, std::int32_t y) {
	return std::make_unique<SpeedMod>(ctx, x, y);
}

std::unique_ptr<Items> CreateGrowthMod(IGameContext &ctx, std::int32_t x, std::int32_t y) {
	return std::make_unique<GrowthMod>(ctx, x, y);
}

} // namespace snake_game
