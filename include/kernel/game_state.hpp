#ifndef __KERNEL_GAME_STATE_HPP__
#define __KERNEL_GAME_STATE_HPP__

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstdint>
#include <memory>

namespace kernel
{

class IGameState {
public:
	virtual ~IGameState() = default;

	virtual void Enter() = 0;
	virtual void Exit() = 0;
	virtual void Pause() {};
	virtual void Resume() {};

	virtual void Event(const SDL_Event *e) = 0;
	virtual void Update(std::uint64_t delta_ms) = 0;
	virtual void Render(SDL_Renderer *renderer, TTF_Font *font) = 0;
};

class IStateMachine {
public:
	virtual ~IStateMachine() = default;

	virtual void Push(std::unique_ptr<IGameState> state) = 0;
	virtual void Pop() = 0;

	virtual void Event(const SDL_Event *e) = 0;
	virtual void Update(std::uint64_t delta_ms) = 0;
	virtual void Render(SDL_Renderer *renderer, TTF_Font *font) = 0;
};

std::unique_ptr<IStateMachine> CreatePushDownAutomaton();

} // namespace kernel


#endif /* __KERNEL_GAME_STATE_HPP__ */