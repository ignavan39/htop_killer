#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace htop_killer::core {

template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0);

public:
    void push(T value) noexcept {
        data_[head_] = std::move(value);
        head_ = (head_ + 1) % Capacity;
        if (size_ < Capacity) ++size_;
    }

    [[nodiscard]] bool        empty()    const noexcept { return size_ == 0; }
    [[nodiscard]] bool        full()     const noexcept { return size_ == Capacity; }
    [[nodiscard]] std::size_t size()     const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return Capacity; }

    [[nodiscard]] const T& operator[](std::size_t idx) const noexcept {
        return data_[(head_ + Capacity - size_ + idx) % Capacity];
    }

    [[nodiscard]] const T& front() const noexcept { return (*this)[0]; }
    [[nodiscard]] const T& back()  const noexcept { return (*this)[size_ - 1]; }

    [[nodiscard]] std::vector<T> to_vector() const {
        std::vector<T> v;
        v.reserve(size_);
        for (std::size_t i = 0; i < size_; ++i) v.push_back((*this)[i]);
        return v;
    }

    void clear() noexcept { head_ = 0; size_ = 0; }

private:
    std::array<T, Capacity> data_{};
    std::size_t             head_ = 0;
    std::size_t             size_ = 0;
};

} // namespace htop_killer::core
