#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace enginez::utils {

    template <typename T, size_t Capacity> class inplace_vector {
        static_assert(Capacity > 0, "Capacity must be greater than 0");

      private:
        alignas(T) unsigned char m_data[Capacity * sizeof(T)];
        size_t m_size = 0;

      public:
        inplace_vector() = default;

        ~inplace_vector() {
            clear();
        }

        inplace_vector(const inplace_vector& other) {
            for (size_t i = 0; i < other.m_size; ++i) {
                push_back(other[i]);
            }
        }

        inplace_vector(std::initializer_list<T> init) {
            assert(init.size() <= Capacity && "initializer_list exceeds inplace_vector capacity");
            for (const auto& item : init) {
                push_back(item);
            }
        }

        inplace_vector& operator=(const inplace_vector& other) {
            if (this != &other) {
                clear();
                for (size_t i = 0; i < other.m_size; ++i) {
                    push_back(other[i]);
                }
            }
            return *this;
        }

        inplace_vector(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
            for (size_t i = 0; i < other.m_size; ++i) {
                push_back(std::move(other[i]));
            }
            other.clear();
        }

        inplace_vector& operator=(inplace_vector&& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
            if (this != &other) {
                clear();
                for (size_t i = 0; i < other.m_size; ++i) {
                    push_back(std::move(other[i]));
                }
                other.clear();
            }
            return *this;
        }

        template <typename... Args> T& emplace_back(Args&&... args) {
            assert(m_size < Capacity && "inplace_vector capacity exceeded");
            T* ptr = data() + m_size;
            ::new (static_cast<void*>(ptr)) T(std::forward<Args>(args)...);
            m_size++;
            return *ptr;
        }

        void push_back(const T& value) {
            emplace_back(value);
        }

        void push_back(T&& value) {
            emplace_back(std::move(value));
        }

        void pop_back() {
            assert(m_size > 0 && "pop_back called on empty vector");
            m_size--;
            data()[m_size].~T();
        }

        void clear() {
            for (size_t i = 0; i < m_size; ++i) {
                data()[i].~T();
            }
            m_size = 0;
        }

        T* data() noexcept {
            return reinterpret_cast<T*>(m_data);
        }
        const T* data() const noexcept {
            return reinterpret_cast<const T*>(m_data);
        }

        T& operator[](size_t idx) {
            assert(idx < m_size);
            return data()[idx];
        }
        const T& operator[](size_t idx) const {
            assert(idx < m_size);
            return data()[idx];
        }

        size_t size() const noexcept {
            return m_size;
        }
        static constexpr size_t capacity() noexcept {
            return Capacity;
        }
        bool empty() const noexcept {
            return m_size == 0;
        }

        T* begin() noexcept {
            return data();
        }
        T* end() noexcept {
            return data() + m_size;
        }
        const T* begin() const noexcept {
            return data();
        }
        const T* end() const noexcept {
            return data() + m_size;
        }
    };

} // namespace enginez::utils