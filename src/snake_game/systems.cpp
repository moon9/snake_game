#include "snake_game/systems.hpp"

namespace snake_game {

class MoveSystemHandler {
private:
	kernel::IECS &ecs_;
	IMapTracking &map_tracking_;
	kernel::EntitySignature signature_;
	std::int32_t w_, h_;
public:
	MoveSystemHandler(kernel::IECS &ecs, IMapTracking &map_tracking, std::int32_t w, std::int32_t h) :
		ecs_{ecs}, map_tracking_{map_tracking}, w_{w}, h_{h} {
		if (this->w_ <= 0 || this->h_ <= 0)
			throw std::runtime_error(std::format("invalid world size {}x{}", this->w_, this->h_));

		auto type_id = this->ecs_.GetComponentType<Position>();

		this->signature_.set(type_id);
		type_id = this->ecs_.GetComponentType<Direction>();
		this->signature_.set(type_id);
	}

	bool Filter(kernel::Entity e) {
		// filter entity, accept only valid by mask
		return (this->ecs_.SignatureEntity(e) & this->signature_) == this->signature_;
	}

	void Handle(std::span<kernel::Entity> entities) {
		for (auto &e : entities) {
			auto &pos = this->ecs_.GetComponent<Position>(e);
			auto &dir = this->ecs_.GetComponent<Direction>(e);

			pos.px = pos.x;
			pos.py = pos.y;

			if (pos.newFlag) {
				pos.newFlag = false;
				this->map_tracking_.SetCell(pos.x, pos.y);
				continue;
			}

			this->map_tracking_.ReleaseCell(pos.x, pos.y);

			switch (dir.dir) {
			case EDirection::UP:
				pos.y -= 1;
				if (pos.y < 0)
					pos.y = this->h_ - 1;
				break;
			case EDirection::DOWN:
				pos.y = (pos.y + 1) % this->h_;
				break;
			case EDirection::LEFT:
				pos.x -= 1;
				if (pos.x < 0)
					pos.x = this->w_ - 1;
				break;
			case EDirection::RIGHT:
				pos.x = (pos.x + 1) % this->w_;
				break;
			default:
				throw std::runtime_error("unknown move direction");
			}

			this->map_tracking_.SetCell(pos.x, pos.y);
		}
	}

	kernel::EntitySignature Signature() const {
		return this->signature_;
	}
};

class FollowSystemHandler {
private:
	kernel::IECS &ecs_;
	kernel::EntitySignature signature_;
public:
	FollowSystemHandler(kernel::IECS &ecs) : ecs_{ecs} {
		auto type_id = this->ecs_.GetComponentType<Direction>();

		this->signature_.set(type_id);
		type_id = this->ecs_.GetComponentType<Parent>();
		this->signature_.set(type_id);
	}

	bool Filter(kernel::Entity e) {
		// filter entity, accept only valid by mask
		return (this->ecs_.SignatureEntity(e) & this->signature_) == this->signature_;
	}

	void Handle(std::span<kernel::Entity> entities) {
		for (auto &e : entities | std::views::reverse) {
			auto &parent = this->ecs_.GetComponent<Parent>(e);
			auto &dir = this->ecs_.GetComponent<Direction>(e);
			auto &parent_dir = this->ecs_.GetComponent<Direction>(parent.e);
			
			dir.dir = parent_dir.dir;
		}
	}

	kernel::EntitySignature Signature() const {
		return this->signature_;
	}
};

class ColliderSystemHandler {
private:
	IGameContext &ctx_;
	kernel::IECS &ecs_;
	kernel::EntitySignature signature_;

	bool isCollide(const Position &a, const Position &b) const noexcept {
		return a.x >= b.x &&
			   b.x >= a.x &&
			   a.y >= b.y &&
			   b.y >= a.y;
	}
public:
	ColliderSystemHandler(IGameContext &ctx) : ctx_{ctx}, ecs_{ctx.ECS()} {
		auto type_id = this->ecs_.GetComponentType<Position>();

		this->signature_.set(type_id);
		type_id = this->ecs_.GetComponentType<Collider>();
		this->signature_.set(type_id);
	}

	bool Filter(kernel::Entity e) {
		// filter entity, accept only valid by mask
		return (this->ecs_.SignatureEntity(e) & this->signature_) == this->signature_;
	}

	void Handle(std::span<kernel::Entity> entities) {
		kernel::Entity snake_head;
		bool has_head = false;

		for (auto &e : entities) {
			auto &meta = this->ecs_.GetComponent<Meta>(e);

			if (meta.type == ENTITY_TYPE_SNAKE_HEAD) {
				snake_head = e;
				has_head = true;
				break;
			}
		}

		if (!has_head)
			throw std::runtime_error("snake head not found");

		auto &head_pos = this->ecs_.GetComponent<Position>(snake_head);

		for (auto &e : entities) {
			if (e == snake_head)
				continue;

			auto &pos = this->ecs_.GetComponent<Position>(e);

			if (!this->isCollide(head_pos, pos))
				continue;
			
			auto meta = this->ecs_.GetComponent<Meta>(e);

			switch (meta.type) {
			case ENTITY_TYPE_SNAKE_TAIL:
			case ENTITY_TYPE_WALL:
				this->ctx_.ResetLevel();
				return;
			case ENTITY_TYPE_MODIFICATOR_SPEED:
				this->ctx_.IncSpeed();
				this->ctx_.AddToRelease(e);
				this->ctx_.SpawnNewSpeed();
				return;
			case ENTITY_TYPE_MODIFICATOR_GROWTH:
				this->ctx_.IncSnake();
				this->ctx_.AddToRelease(e);
				this->ctx_.SpawnNewGrowth();
				return;
			default:
				throw std::runtime_error(
					std::format("collide {} on {}, but type {} haven't action",
						snake_head, e, meta.type));
			}
		}
	}

	kernel::EntitySignature Signature() const {
		return this->signature_;
	}
};

std::unique_ptr<kernel::ISystem> CreateMoveSystem(std::string name, kernel::IECS &ecs, IMapTracking &map_tracking, std::int32_t w, std::int32_t h) {
	return std::make_unique<kernel::System<MoveSystemHandler>>(name, MoveSystemHandler(ecs, map_tracking, w, h));
}

std::unique_ptr<kernel::ISystem> CreateColliderSystem(std::string name, IGameContext &ctx) {
	return std::make_unique<kernel::System<ColliderSystemHandler>>(name, ColliderSystemHandler(ctx));
}

std::unique_ptr<kernel::ISystem> CreateFollowSystem(std::string name, kernel::IECS &ecs) {
	return std::make_unique<kernel::System<FollowSystemHandler>>(name, FollowSystemHandler(ecs));
}

} // namespace snake_game
