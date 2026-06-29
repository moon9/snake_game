#include "snake_game/game_context.hpp"

#include <random>

namespace snake_game {

thread_local std::mt19937 RandomGen::engine_;
thread_local bool RandomGen::init_ = false;

class FieldTrack : public IMapTracking {
private:
	std::uint32_t w_{0}, h_{0};
	std::vector<std::int32_t> field_; // index in free_ (-1 - is occupied)
	std::vector<std::int32_t> free_; // index in field_

	void verifyBoundary(std::uint32_t width, std::uint32_t height) const {
		if (width >= this->w_ || height >= this->h_) {
			throw std::runtime_error(std::format("coordinate {}:{} out of bound {}:{}",
				width, height, this->w_, this->h_));
		}
	}
public:
	explicit FieldTrack(std::uint32_t width, std::uint32_t height) :
		w_{width}, h_{height},
		field_(static_cast<std::int32_t>(w_ * h_)), free_(static_cast<std::int32_t>(w_ * h_)) {
		for (std::uint32_t j = 0; j < height; ++j)
			for (std::uint32_t i = 0; i < width; ++i) {
				this->field_[i + j * width] = static_cast<std::int32_t>(i + j * width);
				this->free_[i + j * width] = static_cast<std::int32_t>(i + j * width);
			}
	}

	void SetCell(std::uint32_t x, std::uint32_t y) override {
		std::uint32_t idx = x + y * this->w_;

		this->verifyBoundary(x, y);

		if (this->field_[idx] == -1)
			return;

		auto pos_in_free = this->field_[idx];
		auto last_idx = this->free_.back();
		this->free_[pos_in_free] = last_idx;
		this->field_[last_idx] = pos_in_free;
		this->free_.pop_back();
		this->field_[idx] = -1;
	}

	void ReleaseCell(std::uint32_t x, std::uint32_t y) override {
		std::uint32_t idx = x + y * this->w_;

		this->verifyBoundary(x, y);
		this->field_[idx] = this->free_.size();
		this->free_.push_back(idx);
	}

	std::pair<std::uint32_t, std::uint32_t> GetFreeRndCell() override {
		std::uint32_t idx = this->free_[RandomGen::gen(this->free_.size() - 1)];
		std::uint32_t w = idx % this->w_;
		std::uint32_t h = idx / this->w_;

		return {w, h};
	}
};

std::unique_ptr<IMapTracking> CreateFieldTrack(std::uint32_t width, std::uint32_t height) {
	return std::make_unique<FieldTrack>(width, height);
}

} // namespace snake_game