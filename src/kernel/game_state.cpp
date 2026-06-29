#include "kernel/game_state.hpp"

#include <vector>
#include <stdexcept>

namespace kernel {

class PushDownAutomaton : public IStateMachine {
private:
	std::vector<std::unique_ptr<IGameState>> stack_;
public:
	void Push(std::unique_ptr<IGameState> state) override {
		if (!this->stack_.empty())
			this->stack_.back()->Pause();
		this->stack_.push_back(std::move(state));
		this->stack_.back()->Enter();
	}

	void Pop() override {
		if (this->stack_.empty())
			throw std::runtime_error("invalid stack pop in state machine, stack is currently empty");
		this->stack_.back()->Exit();
		this->stack_.pop_back();
		if (!this->stack_.empty())
			this->stack_.back()->Resume();
	}

	void Event(const SDL_Event *e) override {
		if (this->stack_.empty())
			return;
		this->stack_.back()->Event(e);
	}

	void Update(std::uint64_t delta_ms) override {
		if (this->stack_.empty())
			return;
		this->stack_.back()->Update(delta_ms);
	}

	void Render(SDL_Renderer *renderer, TTF_Font *font) override {
		if (this->stack_.empty())
			return;
		this->stack_.back()->Render(renderer, font);
	}

};

std::unique_ptr<IStateMachine> CreatePushDownAutomaton() {
	return std::make_unique<PushDownAutomaton>();
}

} // namespace kernel

