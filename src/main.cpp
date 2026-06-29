#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "kernel/game_state.hpp"
#include "snake_game/game_context.hpp"

#include <format>

class Application {
private:
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_{nullptr, SDL_DestroyWindow};
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_{nullptr, SDL_DestroyRenderer};
	std::string game_name_ = "Snake game";
	std::string game_version_ = "0.1";
	std::string game_identifier_ = "com.blablabla.snakegame";
	std::int32_t w = 640, h = 480;

	std::string font_path_ = "./resources/FreeMono.ttf";
	TTF_Font *font_{nullptr};

	std::unique_ptr<kernel::IStateMachine> state_machine_;

	void windowResize(std::int32_t w, std::int32_t h) {
		if (!SDL_SetWindowSize(this->window_.get(), w, h))
			throw std::runtime_error(std::format("sdl window resize failed: {}", SDL_GetError()));
		if (!SDL_SyncWindow(this->window_.get()))
			throw std::runtime_error(std::format("sdl window sync failed: {}", SDL_GetError()));
		this->renderer_.reset(nullptr);
		this->renderer_.reset(SDL_CreateRenderer(this->window_.get(), nullptr));
	}
public:
	void Initialization() {
		SDL_SetAppMetadata(this->game_name_.c_str(), this->game_version_.c_str(), this->game_identifier_.c_str());
		if (!SDL_Init(SDL_INIT_VIDEO))
			throw std::runtime_error(std::format("couldn't initialize sdl: {}", SDL_GetError()));
		
		SDL_Window *wnd;
		SDL_Renderer *renderer;

		if (!SDL_CreateWindowAndRenderer(this->game_name_.c_str(), this->w, this->h, 0, &wnd, &renderer))
			throw std::runtime_error(std::format("couldn't create window/render: {}", SDL_GetError()));

		this->window_.reset(wnd);
		this->renderer_.reset(renderer);

		if (!SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX))
			throw std::runtime_error(std::format("couldn't set render logical render presentation: {}", SDL_GetError()));

		if (!TTF_Init())
			throw std::runtime_error(std::format("sdl3 ttf init failed: {}", SDL_GetError()));

		this->font_ = TTF_OpenFont(this->font_path_.c_str(), 24);
		if (!this->font_)
			throw std::runtime_error(std::format("couldn't open font '{}': {}", this->font_path_, SDL_GetError()));

		this->state_machine_ = kernel::CreatePushDownAutomaton();
		this->state_machine_->Push(snake_game::CreateGameContext(std::bind(&Application::windowResize, this, std::placeholders::_1, std::placeholders::_2)));
	}

	std::unique_ptr<kernel::IStateMachine> & StateMachine() {
		return this->state_machine_;
	}

	SDL_Renderer * Renderer() {
		return this->renderer_.get();
	}

	TTF_Font * Font() {
		return this->font_;
	}

	void CloseFont() {
		if (this->font_)
			TTF_CloseFont(this->font_);
		TTF_Quit();
	}
};

static Application s_app;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
	(void)argc;
	(void)argv;
	*appstate = &s_app;

	try {
		s_app.Initialization();
	} catch (const std::exception &e) {
		SDL_Log("%s", e.what());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
	if (event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;
	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.scancode) {
		case SDL_SCANCODE_ESCAPE:
		case SDL_SCANCODE_Q:
			return SDL_APP_SUCCESS;
		default:
			break;
		}
	}

	Application *app = reinterpret_cast<Application *>(appstate);

	try {
		app->StateMachine()->Event(event);
	} catch (const std::exception &e) {
		SDL_Log("%s", e.what());
		return SDL_APP_FAILURE;
	}
	
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
	Application *app = reinterpret_cast<Application *>(appstate);
	static const std::uint64_t freq_ms = SDL_GetPerformanceFrequency() / 1000;
	static std::uint64_t prev_counter = SDL_GetPerformanceCounter();
	std::uint64_t now_counter = SDL_GetPerformanceCounter();
	std::uint64_t delta_ms = (now_counter - prev_counter) / freq_ms;

	prev_counter = now_counter;

	try {
		app->StateMachine()->Update(delta_ms);
		app->StateMachine()->Render(app->Renderer(), app->Font());
	} catch (const std::exception &e) {
		SDL_Log("%s", e.what());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
	(void)result;
	Application *app = reinterpret_cast<Application *>(appstate);

	app->CloseFont();
}

