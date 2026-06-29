#include "snake_game/game_context.hpp"

namespace snake_game {

class Snake : public ISnake {
private:
	kernel::Entity head_{0};
	std::vector<kernel::Entity> tail_{};
	kernel::IECS &ecs_;
public:
	Snake(kernel::IECS &ecs, std::int32_t x, std::int32_t y, std::uint32_t tail_size) : ecs_{ecs} {
		this->head_ = this->ecs_.CreateEntity();
		this->ecs_.AddComponent<Meta>(this->head_, Meta{
			.type = ENTITY_TYPE_SNAKE_HEAD,
		});
		this->ecs_.AddComponent<Position>(this->head_, Position{
			.x = x,
			.y = y,
			.px = x,
			.py = y,
			.newFlag = true,
		});
		this->ecs_.AddComponent<Direction>(this->head_, Direction{
			.dir = EDirection::RIGHT,
		});
		this->ecs_.AddComponent<Collider>(this->head_, Collider{});
		this->tail_.push_back(this->head_);

		for (std::uint32_t i = 0; i < tail_size; ++i)
			this->AddTail(-1);
	}

	~Snake() {
		for (auto &e : this->tail_)
			this->ecs_.DestroyEntity(e);
	}

	kernel::Entity Head() const noexcept {
		return this->head_;
	}

	kernel::Entity AddTail(std::int32_t xoff = 0, std::int32_t yoff = 0) {
		kernel::Entity parent = this->tail_.back();
		kernel::Entity e = this->ecs_.CreateEntity();
		auto parent_pos = this->ecs_.GetComponent<Position>(parent);
		auto parent_dir = this->ecs_.GetComponent<Direction>(parent);
		
		this->ecs_.AddComponent<Meta>(e, Meta{
			.type = ENTITY_TYPE_SNAKE_TAIL,
		});
		this->ecs_.AddComponent<Position>(e, Position{
			.x = parent_pos.x + xoff,
			.y = parent_pos.y - yoff,
			.px = parent_pos.x + xoff,
			.py = parent_pos.y - yoff,
			.newFlag = true,
		});
		this->ecs_.AddComponent<Direction>(e, Direction{
			.dir = parent_dir.dir,
		});
		this->ecs_.AddComponent<Parent>(e, Parent{
			.e = this->tail_.back(),
		});
		this->ecs_.AddComponent<Collider>(e, Collider{});
		this->tail_.push_back(e);

		return e;
	}
};

std::unique_ptr<ISnake> CreateSnake(kernel::IECS &ecs, std::int32_t x, std::int32_t y, std::uint32_t tail_size) {
	return std::make_unique<Snake>(ecs, x, y, tail_size);
}

} // namespace snake_game
