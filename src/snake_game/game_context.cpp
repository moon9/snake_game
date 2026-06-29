#include <SDL3/SDL.h>

#include <queue>

#include "snake_game/game_context.hpp"
#include "snake_game/systems.hpp"
#include "snake_game/level.hpp"

namespace snake_game {

static constexpr std::int32_t CELL_WIDTH = 16;
static constexpr std::int32_t CELL_HEIGHT = 16;
static constexpr std::float_t AMPLITUDE_MS = 200.f;
static constexpr std::float_t ASYMPTOTE_MS = 50.f;
static constexpr std::float_t TARGET_FPS = 60.f;

static constexpr std::int32_t BORDER_OFFSET = 5;
static constexpr std::int32_t BORDER_WIDTH = 5;
static constexpr std::int32_t INFO_PANEL_WIDTH = 200;

static constexpr std::uint32_t SPEED_SCORE = 100;
static constexpr std::uint32_t GROWTH_SCORE = 200;

static constexpr std::uint64_t CalcStepTicksMs(float r) {
	return static_cast<std::uint64_t>(AMPLITUDE_MS * std::expf(-0.1f * r) + ASYMPTOTE_MS);
}

class GameContext : public IGameContext, public kernel::IGameState {
private:
	std::unique_ptr<kernel::IECS> ecs_{nullptr};
	std::unique_ptr<IMapTracking> field_track_{nullptr};
	std::unique_ptr<ISnake> snake_{nullptr};

	std::vector<std::unique_ptr<Items>> items_{};
	std::vector<kernel::Entity> to_delete_{};

	std::queue<EDirection> input_buffer_{};
	static constexpr std::size_t MAX_INPUT_BUFFER = 2;

	std::float_t r_{.0f};
	std::uint64_t accumulator_{0};
	std::uint64_t step_ms_{CalcStepTicksMs(this->r_)};

	LevelDescription level_;

	WindowResizeCb wnd_resize_{nullptr};
	bool reset_ = true;

	std::int32_t wnd_level_width{640};
	std::int32_t wnd_level_height{480};
	SDL_FRect border_rect_;
	SDL_FRect field_rect_;

	std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> score_texture_{nullptr, SDL_DestroyTexture};

	std::uint64_t score_{0};
	bool score_update_{false};

	void clean() {
		this->reset_ = false;

		this->score_ = 0;
		this->score_update_ = true;
		this->score_texture_.reset(nullptr);

		this->r_ = .0f;
		this->accumulator_ = 0;
		this->step_ms_ = CalcStepTicksMs(this->r_);

		std::queue<EDirection> empty;
		this->input_buffer_.swap(empty);

		this->to_delete_.clear();
		this->items_.clear();

		this->snake_.reset();
		this->ecs_->Reset();
		this->field_track_.reset();
	}

	bool isOpposite(EDirection a, EDirection b) const noexcept {
		return (a == EDirection::UP && b == EDirection::DOWN) ||
			(a == EDirection::DOWN && b == EDirection::UP) ||
			(a == EDirection::LEFT && b == EDirection::RIGHT) ||
			(a == EDirection::RIGHT && b == EDirection::LEFT);
	}

public:
	~GameContext() {
		this->clean();
	}

	GameContext(WindowResizeCb wnd_resize) : wnd_resize_(wnd_resize) {}

	void Enter() override {
		this->level_ = LoadLevel1();
		this->wnd_level_width = this->level_.w * CELL_WIDTH;
		this->wnd_level_height = this->level_.h * CELL_HEIGHT;

		std::int32_t w = this->wnd_level_width + (BORDER_OFFSET + BORDER_WIDTH) * 2 + INFO_PANEL_WIDTH, h = this->wnd_level_height + (BORDER_OFFSET + BORDER_WIDTH) * 2;

		if (this->wnd_resize_)
			this->wnd_resize_(w, h);

		this->border_rect_ = {
			.x = static_cast<float>(BORDER_OFFSET),
			.y = static_cast<float>(BORDER_OFFSET),
			.w = static_cast<float>(this->wnd_level_width + BORDER_OFFSET + BORDER_WIDTH),
			.h = static_cast<float>(this->wnd_level_height + BORDER_OFFSET + BORDER_WIDTH),
		};
		this->field_rect_ = {
			.x = static_cast<float>(BORDER_OFFSET + BORDER_WIDTH),
			.y = static_cast<float>(BORDER_OFFSET + BORDER_WIDTH),
			.w = static_cast<float>(this->wnd_level_width),
			.h = static_cast<float>(this->wnd_level_height),
		}; 

		this->ecs_= kernel::CreateECS();

		SDL_Time t;
		SDL_GetCurrentTime(&t);
		RandomGen::Init(SDL_NS_TO_SECONDS(t));
	}

	void Exit() override {
		this->ecs_.reset(nullptr);
	}

	void Pause() override {
		SDL_Log("Game loop paused");
	}

	void Resume() override {
		SDL_Log("Game loop resumed");
	}

	void Event(const SDL_Event *e) override {
		if (e->type == SDL_EVENT_KEY_DOWN) {
			switch (e->key.scancode) {
			case SDL_SCANCODE_UP:
			case SDL_SCANCODE_W:
				this->QueueSnakeHeadDirection(EDirection::UP);
				break;
			case SDL_SCANCODE_DOWN:
			case SDL_SCANCODE_S:
				this->QueueSnakeHeadDirection(EDirection::DOWN);
				break;
			case SDL_SCANCODE_LEFT:
			case SDL_SCANCODE_A:
				this->QueueSnakeHeadDirection(EDirection::LEFT);
				break;
			case SDL_SCANCODE_RIGHT:
			case SDL_SCANCODE_D:
				this->QueueSnakeHeadDirection(EDirection::RIGHT);
				break;
			case SDL_SCANCODE_R:
				this->SetReset();
				break;
			default:
				break;
			}
		}
	}

	void Update(std::uint64_t delta_ms) override {
		const std::uint64_t step_ms = this->step_ms_;

		this->accumulator_ += delta_ms;

		if (this->accumulator_ >= step_ms) {
			this->accumulator_ -= step_ms;
			this->InitLevel();
			this->EnqueueShakeHeadDirection();
			this->Systems();
			this->DeferProcessing();
		}
	}

	void Render(SDL_Renderer *renderer, TTF_Font *font) override {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 68, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(renderer, &this->border_rect_);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(renderer, &this->field_rect_);

		std::vector<SDL_FRect> head, tails, walls, speeds, growths;

		tails.reserve(100);
		walls.reserve(100);

		for (const auto &e : this->Entities()) {
			auto s = this->ECS().SignatureEntity(e);

			if (!s[this->ECS().GetComponentType<Position>()])
				continue;

			auto &pos = this->ECS().GetComponent<Position>(e);

			float render_x = pos.x;
			float render_y = pos.y;

			SDL_FRect rect{
				.x = render_x * CELL_WIDTH + 10,
				.y = render_y * CELL_HEIGHT + 10,
				.w = static_cast<float>(CELL_WIDTH),
				.h = static_cast<float>(CELL_HEIGHT),
			};

			auto &meta = this->ECS().GetComponent<Meta>(e);

			switch (meta.type) {
			case ENTITY_TYPE_SNAKE_HEAD:
				head.push_back(rect);
				break;
			case ENTITY_TYPE_SNAKE_TAIL:
				tails.push_back(rect);
				break;
			case ENTITY_TYPE_WALL:
				walls.push_back(rect);
				break;
			case ENTITY_TYPE_MODIFICATOR_SPEED:
				speeds.push_back(rect);
				break;
			case ENTITY_TYPE_MODIFICATOR_GROWTH:
				growths.push_back(rect);
				break;
			default:
				throw std::runtime_error("unsupported meta type for render");
			}
		}

		auto draw_batch = [&](const std::vector<SDL_FRect> &batch, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
			if (batch.empty())
				return;
			SDL_SetRenderDrawColor(renderer, r, g, b, a);
			SDL_RenderFillRects(renderer, batch.data(), batch.size());
		};

		draw_batch(head, 200, 0, 0, SDL_ALPHA_OPAQUE);
		draw_batch(tails, 0, 127, 0, SDL_ALPHA_OPAQUE);
		draw_batch(walls, 255, 0, 0, SDL_ALPHA_OPAQUE);
		draw_batch(speeds, 64, 64, 0, SDL_ALPHA_OPAQUE);
		draw_batch(growths, 255, 0, 255, SDL_ALPHA_OPAQUE);

		// info panel.
		// TODO: нужно переработать работу с текстом, сделать атлас шрифта и рендерить как нужно
		if (this->score_update_) {
			SDL_Color score_color = {210, 175, 38, SDL_ALPHA_OPAQUE};
			std::string score_str = std::format("score: {:06}", this->score_);
			SDL_Surface *score_surface = TTF_RenderText_Blended(font, score_str.c_str(), 0, score_color);
			SDL_Texture *score_texture = SDL_CreateTextureFromSurface(renderer, score_surface);

			SDL_DestroySurface(score_surface);
			this->score_texture_.reset(score_texture);
			this->score_update_ = false;
		}
		SDL_FRect dst = {
			.x = static_cast<float>(this->wnd_level_width + (BORDER_OFFSET + BORDER_WIDTH) * 2),
			.y = 20.f,
			.w = .0f,
			.h = .0f,
		};
		SDL_GetTextureSize(this->score_texture_.get(), &dst.w, &dst.h);
		SDL_RenderTexture(renderer, this->score_texture_.get(), NULL, &dst);

		SDL_RenderPresent(renderer);
		SDL_Delay(1);
	}

	void InitLevel() {
		if (!this->reset_)
			return;

		this->clean();

		this->field_track_ = CreateFieldTrack(this->level_.w, this->level_.h);

		// register components.
		this->ecs_->RegisterComponent<Meta>(std::make_unique<kernel::ComponentStorage<Meta>>());
		this->ecs_->RegisterComponent<Position>(std::make_unique<kernel::ComponentStorage<Position>>());
		this->ecs_->RegisterComponent<Direction>(std::make_unique<kernel::ComponentStorage<Direction>>());
		this->ecs_->RegisterComponent<Parent>(std::make_unique<kernel::ComponentStorage<Parent>>());
		this->ecs_->RegisterComponent<Collider>(std::make_unique<kernel::ComponentStorage<Collider>>());

		// register systems.
		this->ecs_->AddSystem(CreateMoveSystem("MoveSystem",this->ECS(), *this->field_track_.get(), 40, 40));
		this->ecs_->AddSystem(CreateColliderSystem("Collider system", *this));
		this->ecs_->AddSystem(CreateFollowSystem("Follow system", this->ECS()));

		// snake.
		this->snake_ = CreateSnake(this->ECS(), this->level_.snake_x, this->level_.snake_y, this->level_.tail_size);

		for (auto &entity : this->level_.entities) {
			switch (entity.type) {
			case ENTITY_TYPE_WALL: {
				this->items_.push_back(CreateWall(*this, entity.x, entity.y));
				break;
			}
			default:
				continue;
			}
		}

		for (std::uint32_t i = 0; i < this->level_.speed_size; ++i) {
			auto rng_pos = this->field_track_->GetFreeRndCell();
			this->items_.push_back(CreateSpeedMod(*this, rng_pos.first, rng_pos.second));	
		}

		for (std::uint32_t i = 0; i < this->level_.growth_size; ++i) {
			auto rng_pos = this->field_track_->GetFreeRndCell();
			this->items_.push_back(CreateGrowthMod(*this, rng_pos.first, rng_pos.second));	
		}
	}

	void Systems() {
		this->ecs_->InvokeSystems();
	}

	const std::vector<kernel::Entity> & Entities() const noexcept {
		return this->ecs_->GetEntities();
	}

	void SetReset() {
		this->reset_ = true;
	}

	void QueueSnakeHeadDirection(EDirection dir) {
		EDirection last_dir = this->ecs_->GetComponent<Direction>(this->snake_->Head()).dir;

		if (!this->input_buffer_.empty())
			last_dir = this->input_buffer_.back();

		if (this->isOpposite(dir, last_dir))
			return;

		if (this->input_buffer_.size() < MAX_INPUT_BUFFER)
			this->input_buffer_.push(dir);
	}

	void EnqueueShakeHeadDirection() {
		if (this->input_buffer_.empty())
			return;

		auto &head_dir = this->ECS().GetComponent<Direction>(this->snake_->Head());

		head_dir.dir = this->input_buffer_.front();
		this->input_buffer_.pop();
	}

	kernel::IECS & ECS() noexcept {
		return *this->ecs_;
	}

	IMapTracking & MapTracking() {
		return *this->field_track_;
	}

	void ResetLevel() {
		this->reset_ = true;
	}

	void IncSpeed(std::float_t inc = 0.5f) {
		this->r_ += inc;
		this->step_ms_ = CalcStepTicksMs(this->r_);
		this->score_ += SPEED_SCORE;
		this->score_update_ = true;
	}

	void AddToRelease(kernel::Entity e) {
		this->to_delete_.push_back(e);
	}

	void DeferProcessing() {
		for (auto &e : this->to_delete_)
			for (auto it = this->items_.begin(); it != this->items_.end(); it++)
				if ((*it)->GetEntity() == e) {
					this->items_.erase(it);
					break;
				}

		this->to_delete_.clear();
	}

	void IncSnake() {
		this->snake_->AddTail();
		this->score_ += GROWTH_SCORE;
		this->score_update_ = true;
	}

	void SpawnNewGrowth(){
		auto rng_pos = this->field_track_->GetFreeRndCell();
		this->items_.push_back(CreateGrowthMod(*this, rng_pos.first, rng_pos.second));
	}

	void SpawnNewSpeed(){
		auto rng_pos = this->field_track_->GetFreeRndCell();
		this->items_.push_back(CreateSpeedMod(*this, rng_pos.first, rng_pos.second));
	}
};

std::unique_ptr<kernel::IGameState> CreateGameContext(WindowResizeCb wnd_resize) {
	return std::make_unique<GameContext>(wnd_resize);
}

} // namespace snake_game
