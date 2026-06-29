#ifndef __SNAKE_GAME_COMPONENTS_HPP__
#define __SNAKE_GAME_COMPONENTS_HPP__

#include <cstdint>

#include "kernel/ecs.hpp"

namespace snake_game {

enum EntityType : std::uint32_t {
	ENTITY_TYPE_NONE = 0,
	ENTITY_TYPE_SNAKE_HEAD,
	ENTITY_TYPE_SNAKE_TAIL,
	ENTITY_TYPE_WALL,
	ENTITY_TYPE_MODIFICATOR_SPEED,
	ENTITY_TYPE_MODIFICATOR_GROWTH,
};

// Components
struct Meta {
	std::uint32_t type{ENTITY_TYPE_NONE};
};

struct Position {
	std::int32_t x = 0, y = 0;
	std::int32_t px = 0, py = 0;
	bool newFlag = false;
};

enum class EDirection : std::int32_t {
	UP, DOWN, LEFT, RIGHT,
};

struct Direction {
	EDirection dir{EDirection::RIGHT};
};

struct Parent {
	kernel::Entity e;
};

struct Collider {};

} // namespace snake_game

#endif /* __SNAKE_GAME_COMPONENTS_HPP__ */