#ifndef __SNAKE_GAME_SYSTEMS_HPP__
#define __SNAKE_GAME_SYSTEMS_HPP__

#include <cstdint>
#include <span>
#include <ranges>

#include "kernel/ecs.hpp"

#include "snake_game/components.hpp"
#include "snake_game/game_context.hpp"

namespace snake_game {

std::unique_ptr<kernel::ISystem> CreateMoveSystem(std::string name, kernel::IECS &ecs, IMapTracking &map_tracking, std::int32_t w, std::int32_t h);
std::unique_ptr<kernel::ISystem> CreateColliderSystem(std::string name, IGameContext &ctx);
std::unique_ptr<kernel::ISystem> CreateFollowSystem(std::string name, kernel::IECS &ecs);

} // namespace snake_game

#endif /* __SNAKE_GAME_SYSTEMS_HPP__ */