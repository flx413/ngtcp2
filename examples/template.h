/*
 * ngtcp2
 *
 * Copyright (c) 2017 ngtcp2 contributors
 * Copyright (c) 2015 ngttp2 contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <functional>
#include <utility>
#include <type_traits>
#include "cxxcompat.h"

template <typename T,
          typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
[[nodiscard]] constexpr auto as_unsigned(T n) noexcept {
  return static_cast<std::make_unsigned_t<T>>(n);
}

template <typename T,
          typename std::enable_if<std::is_integral<T>::value &&
                                    std::is_unsigned<T>::value,
                                  int>::type = 0>
[[nodiscard]] constexpr auto as_signed(T n) noexcept {
  return static_cast<std::make_signed_t<T>>(n);
}

template <typename T, std::size_t N>
[[nodiscard]] auto as_uint8_span(std::span<T, N> s) noexcept {
  return std::span<const uint8_t, N == std::dynamic_extent ? std::dynamic_extent
                                                           : N * sizeof(T)>{
    reinterpret_cast<const uint8_t *>(s.data()), s.size_bytes()};
}

template <typename F> struct Defer {
  explicit Defer(F &&f) noexcept(std::is_nothrow_constructible_v<F, F &&>)
    : f(std::forward<F>(f)) {}
  ~Defer() { f(); }

  Defer(Defer &&o) = delete;
  Defer(const Defer &) = delete;
  Defer &operator=(const Defer &) = delete;
  Defer &operator=(Defer &&) = delete;

  F f;
};

template <typename F> [[nodiscard]] Defer<std::decay_t<F>> defer(F &&f) {
  return Defer<std::decay_t<F>>(std::forward<F>(f));
}

template <typename T, size_t N> constexpr size_t array_size(T (&)[N]) {
  return N;
}

template <typename T, size_t N> constexpr size_t str_size(T (&)[N]) {
  return N - 1;
}

// User-defined literals for K, M, and G (powers of 1024)

constexpr unsigned long long operator""_k(unsigned long long k) {
  return k * 1024;
}

constexpr unsigned long long operator""_m(unsigned long long m) {
  return m * 1024 * 1024;
}

constexpr unsigned long long operator""_g(unsigned long long g) {
  return g * 1024 * 1024 * 1024;
}

template <typename T, std::size_t N>
[[nodiscard]] std::span<uint8_t, N == std::dynamic_extent ? std::dynamic_extent
                                                          : N * sizeof(T)>
as_writable_uint8_span(std::span<T, N> s) noexcept {
  return std::span<uint8_t, N == std::dynamic_extent ? std::dynamic_extent
                                                     : N * sizeof(T)>{
    reinterpret_cast<uint8_t *>(s.data()), s.size_bytes()};
}

template <typename R,
          typename std::enable_if<
            !std::is_array<typename std::remove_reference<R>::type>::value,
            int>::type = 0,
          typename = decltype(std::data(std::declval<R &>())),
          typename = decltype(std::size(std::declval<R &>()))>
[[nodiscard]] std::string_view as_string_view(R &&r) {
  return std::string_view{
    reinterpret_cast<std::string_view::const_pointer>(std::data(r)),
    std::size(r)};
}

#endif // !defined(TEMPLATE_H)
