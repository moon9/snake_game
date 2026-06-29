#ifndef __SNAKE_GAME_LEVEL_HPP__
#define __SNAKE_GAME_LEVEL_HPP__

#include <cstdint>
#include <vector>

namespace snake_game {

struct EntityBlueprint {
	std::uint32_t type{0};
	std::int32_t x{0}, y{0};
};

struct LevelDescription {
	std::int32_t w{0}, h{0};
	std::int32_t snake_x{0}, snake_y{0};
	std::uint32_t tail_size{5};
	std::uint32_t growth_size{4};
	std::uint32_t speed_size{4};
	std::vector<EntityBlueprint> entities{};
};

const LevelDescription & LoadLevel1();

} // namespace snake_game

#endif /* __SNAKE_GAME_LEVEL_HPP__ */
