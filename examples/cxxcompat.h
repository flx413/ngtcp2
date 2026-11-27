/*
 * ngtcp2
 *
 * Copyright (c) 2025 ngtcp2 contributors
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
#ifndef CXXCOMPAT_H
#define CXXCOMPAT_H

// cxxcompat.h provides the subset of the C++20/23 standard library
// facilities that the ngtcp2 examples use, so that they can be built
// as C++17.  It must only be used by the examples, never by the
// library itself.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iterator>
#include <limits>
#include <new>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace std {

// ------------------------------------------------------------------
// std::remove_cvref (C++20)
// ------------------------------------------------------------------

template <typename T> struct remove_cvref {
  using type = remove_cv_t<remove_reference_t<T>>;
};

template <typename T> using remove_cvref_t = typename remove_cvref<T>::type;

// ------------------------------------------------------------------
// std::dynamic_extent and std::span (C++20)
// ------------------------------------------------------------------

inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

namespace cxxcompat_detail {

template <typename T> struct is_span : false_type {};

} // namespace cxxcompat_detail

template <typename T, size_t Extent = dynamic_extent> class span {
public:
  using element_type = T;
  using value_type = remove_cv_t<T>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_t extent = Extent;

  constexpr span() noexcept
    : data_(nullptr), size_(Extent == dynamic_extent ? 0 : Extent) {}

  constexpr span(pointer data, size_type count) noexcept
    : data_(data), size_(count) {}

  constexpr span(pointer first, pointer last) noexcept
    : data_(first), size_(static_cast<size_type>(last - first)) {}

  template <size_t N,
            typename enable_if<(Extent == dynamic_extent || N == Extent),
                               int>::type = 0>
  constexpr span(element_type (&arr)[N]) noexcept : data_(arr), size_(N) {}

  template <typename U, size_t N,
            typename enable_if<is_convertible<U (*)[], T (*)[]>::value &&
                                 (Extent == dynamic_extent || N == Extent),
                               int>::type = 0>
  constexpr span(array<U, N> &arr) noexcept
    : data_(arr.data()), size_(N) {}

  template <typename U, size_t N,
            typename enable_if<is_convertible<const U (*)[], T (*)[]>::value &&
                                 (Extent == dynamic_extent || N == Extent),
                               int>::type = 0>
  constexpr span(const array<U, N> &arr) noexcept
    : data_(arr.data()), size_(N) {}

  template <typename C,
            typename enable_if<
              !cxxcompat_detail::is_span<remove_cv_t<C>>::value &&
                !is_array<remove_reference_t<C>>::value &&
                is_convertible<decltype(std::declval<C &>().data()),
                               pointer>::value,
              int>::type = 0>
  constexpr span(C &c) noexcept : data_(c.data()), size_(c.size()) {}

  template <typename U, size_t N,
            typename enable_if<is_convertible<U (*)[], T (*)[]>::value &&
                                 (Extent == dynamic_extent || N == Extent),
                               int>::type = 0>
  constexpr span(const span<U, N> &s) noexcept
    : data_(s.data()), size_(s.size()) {}

  constexpr pointer data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }
  constexpr bool empty() const noexcept { return size_ == 0; }

  constexpr reference operator[](size_type i) const { return data_[i]; }
  constexpr reference front() const { return data_[0]; }
  constexpr reference back() const { return data_[size_ - 1]; }

  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size_; }
  constexpr reverse_iterator rbegin() const noexcept {
    return reverse_iterator{end()};
  }
  constexpr reverse_iterator rend() const noexcept {
    return reverse_iterator{begin()};
  }

  constexpr span<T, dynamic_extent> first(size_type count) const {
    return {data_, count};
  }

  constexpr span<T, dynamic_extent> last(size_type count) const {
    return {data_ + (size_ - count), count};
  }

  constexpr span<T, dynamic_extent> subspan(size_type offset) const {
    return {data_ + offset, size_ - offset};
  }

  constexpr span<T, dynamic_extent> subspan(size_type offset,
                                            size_type count) const {
    return {data_ + offset, count};
  }

  template <size_t Count> constexpr span<T, Count> first() const {
    static_assert(Extent == dynamic_extent || Count <= Extent,
                  "Count must not exceed the span extent");
    return {data_, Count};
  }

  template <size_t Count> constexpr span<T, Count> last() const {
    static_assert(Extent == dynamic_extent || Count <= Extent,
                  "Count must not exceed the span extent");
    return {data_ + (size_ - Count), Count};
  }

  template <size_t Offset, size_t Count = dynamic_extent>
  constexpr span<T, Count == dynamic_extent ? dynamic_extent : Count>
  subspan() const {
    return {data_ + Offset, Count == dynamic_extent ? size_ - Offset : Count};
  }

private:
  pointer data_;
  size_type size_;
};

namespace cxxcompat_detail {

template <typename T, size_t N> struct is_span<span<T, N>> : true_type {};

} // namespace cxxcompat_detail

template <typename T, size_t N> span(T (&)[N])->span<T, N>;

template <typename U, size_t N> span(array<U, N> &)->span<U, N>;

template <typename U, size_t N>
span(const array<U, N> &)->span<const U, N>;

template <typename C,
          typename enable_if<
            !cxxcompat_detail::is_span<remove_cv_t<C>>::value &&
              !is_array<remove_reference_t<C>>::value,
            int>::type = 0>
span(C &)->span<remove_pointer_t<decltype(std::declval<C &>().data())>,
                dynamic_extent>;

template <typename C,
          typename enable_if<
            !cxxcompat_detail::is_span<remove_cv_t<C>>::value &&
              !is_array<remove_reference_t<C>>::value,
            int>::type = 0>
span(const C &)->span<
  remove_pointer_t<decltype(std::declval<const C &>().data())>,
  dynamic_extent>;

template <typename It>
span(It, size_t)->span<remove_pointer_t<It>, dynamic_extent>;

// ------------------------------------------------------------------
// std::cxx17::unexpected, std::expected (C++23)
// ------------------------------------------------------------------

namespace cxx17 {

template <typename E> class unexpected {
public:
  constexpr explicit unexpected(E e) : err_(std::move(e)) {}

  constexpr E &error() & noexcept { return err_; }
  constexpr const E &error() const &noexcept { return err_; }
  constexpr E &&error() && noexcept { return std::move(err_); }

private:
  E err_;
};

template <typename E> unexpected(E)->unexpected<E>;

template <typename E> class bad_expected_access : public exception {
public:
  explicit bad_expected_access(E e) : err_(std::move(e)) {}

  const char *what() const noexcept override {
    return "bad expected access";
  }

  E &error() & noexcept { return err_; }
  const E &error() const &noexcept { return err_; }

private:
  E err_;
};

template <typename T, typename E> class expected {
public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  expected() : val_{}, has_(true) {}

  expected(const T &v) : val_(v), has_(true) {}

  expected(T &&v) : val_(std::move(v)), has_(true) {}

  expected(const unexpected<E> &u) : err_(u.error()), has_(false) {}

  expected(unexpected<E> &&u) : err_(std::move(u.error())), has_(false) {}

  expected(const expected &o) : has_(o.has_) {
    if (has_) {
      new (&val_) T(o.val_);
    } else {
      new (&err_) E(o.err_);
    }
  }

  expected(expected &&o) noexcept(is_nothrow_move_constructible<T>::value &&
                                  is_nothrow_move_constructible<E>::value)
    : has_(o.has_) {
    if (has_) {
      new (&val_) T(std::move(o.val_));
    } else {
      new (&err_) E(std::move(o.err_));
    }
  }

  ~expected() {
    if (has_) {
      val_.~T();
    } else {
      err_.~E();
    }
  }

  expected &operator=(const expected &o) {
    if (this == &o) {
      return *this;
    }
    destroy();
    has_ = o.has_;
    if (has_) {
      new (&val_) T(o.val_);
    } else {
      new (&err_) E(o.err_);
    }
    return *this;
  }

  expected &operator=(expected &&o) noexcept(
    is_nothrow_move_constructible<T>::value &&
    is_nothrow_move_constructible<E>::value) {
    if (this == &o) {
      return *this;
    }
    destroy();
    has_ = o.has_;
    if (has_) {
      new (&val_) T(std::move(o.val_));
    } else {
      new (&err_) E(std::move(o.err_));
    }
    return *this;
  }

  constexpr explicit operator bool() const noexcept { return has_; }
  constexpr bool has_value() const noexcept { return has_; }

  constexpr T &operator*() & { return val_; }
  constexpr const T &operator*() const & { return val_; }
  constexpr T &&operator*() && { return std::move(val_); }

  constexpr T *operator->() { return &val_; }
  constexpr const T *operator->() const { return &val_; }

  T &value() & {
    if (!has_) {
      throw bad_expected_access<E>(err_);
    }
    return val_;
  }

  const T &value() const & {
    if (!has_) {
      throw bad_expected_access<E>(err_);
    }
    return val_;
  }

  E &error() & {
    if (has_) {
      throw bad_expected_access<E>(E{});
    }
    return err_;
  }

  const E &error() const & {
    if (has_) {
      throw bad_expected_access<E>(E{});
    }
    return err_;
  }

  template <typename U> T value_or(U &&def) const & {
    return has_ ? val_ : static_cast<T>(std::forward<U>(def));
  }

  template <typename U> T value_or(U &&def) && {
    return has_ ? std::move(val_) : static_cast<T>(std::forward<U>(def));
  }

  template <typename... Args> T &emplace(Args &&...args) {
    destroy();
    new (&val_) T(std::forward<Args>(args)...);
    has_ = true;
    return val_;
  }

private:
  void destroy() noexcept {
    if (has_) {
      val_.~T();
    } else {
      err_.~E();
    }
  }

  union {
    T val_;
    E err_;
  };

  bool has_;
};

template <typename E> class expected<void, E> {
public:
  using value_type = void;
  using error_type = E;
  using unexpected_type = unexpected<E>;

  expected() noexcept : has_(true) {}

  expected(const unexpected<E> &u) : err_(u.error()), has_(false) {}

  expected(unexpected<E> &&u) : err_(std::move(u.error())), has_(false) {}

  expected(const expected &o) : has_(o.has_) {
    if (!has_) {
      new (&err_) E(o.err_);
    }
  }

  expected(expected &&o) noexcept(is_nothrow_move_constructible<E>::value)
    : has_(o.has_) {
    if (!has_) {
      new (&err_) E(std::move(o.err_));
    }
  }

  ~expected() {
    if (!has_) {
      err_.~E();
    }
  }

  expected &operator=(const expected &o) {
    if (this == &o) {
      return *this;
    }
    destroy();
    has_ = o.has_;
    if (!has_) {
      new (&err_) E(o.err_);
    }
    return *this;
  }

  expected &operator=(expected &&o) noexcept(
    is_nothrow_move_constructible<E>::value) {
    if (this == &o) {
      return *this;
    }
    destroy();
    has_ = o.has_;
    if (!has_) {
      new (&err_) E(std::move(o.err_));
    }
    return *this;
  }

  constexpr explicit operator bool() const noexcept { return has_; }
  constexpr bool has_value() const noexcept { return has_; }

  void value() const {
    if (!has_) {
      throw bad_expected_access<E>(err_);
    }
  }

  E &error() & {
    if (has_) {
      throw bad_expected_access<E>(E{});
    }
    return err_;
  }

  const E &error() const & {
    if (has_) {
      throw bad_expected_access<E>(E{});
    }
    return err_;
  }

  void emplace() noexcept {
    destroy();
    has_ = true;
  }

private:
  void destroy() noexcept {
    if (!has_) {
      err_.~E();
    }
  }

  union {
    E err_;
  };

  bool has_;
};

} // namespace cxx17

using cxx17::expected;

// ------------------------------------------------------------------
// std::endian, std::bit_cast, std::rotl, std::countl_zero (C++20)
// ------------------------------------------------------------------

enum class endian {
  little = 0x01020304,
  big = 0x04030201,
  native =
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    big
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    little
#else
    little
#endif
};

template <typename To, typename From,
          typename enable_if<sizeof(To) == sizeof(From) &&
                               is_trivially_copyable<To>::value &&
                               is_trivially_copyable<From>::value,
                             int>::type = 0>
To bit_cast(const From &src) noexcept {
  To dst;
  memcpy(&dst, &src, sizeof(To));
  return dst;
}

template <typename T,
          typename enable_if<is_unsigned<T>::value, int>::type = 0>
constexpr T rotl(T x, int s) noexcept {
  constexpr int digits = std::numeric_limits<T>::digits;
  const auto r = s % digits;
  return static_cast<T>((x << r) | (x >> ((digits - r) % digits)));
}

template <typename T,
          typename enable_if<is_unsigned<T>::value, int>::type = 0>
constexpr T rotr(T x, int s) noexcept {
  return rotl(x, -s);
}

template <typename T,
          typename enable_if<is_unsigned<T>::value, int>::type = 0>
constexpr int countl_zero(T x) noexcept {
  constexpr int digits = std::numeric_limits<T>::digits;

  if (!x) {
    return digits;
  }

  auto count = 0;

  for (auto i = digits - 1; i >= 0; --i) {
    if ((x >> i) & static_cast<T>(1)) {
      break;
    }
    ++count;
  }

  return count;
}

// ------------------------------------------------------------------
// std::cmp_* (C++20)
// ------------------------------------------------------------------

template <typename T, typename U>
constexpr bool cmp_less(T t, U u) noexcept {
  if constexpr (is_signed<T>::value == is_signed<U>::value) {
    return t < u;
  } else if constexpr (is_signed<T>::value) {
    return t < 0 || static_cast<make_unsigned_t<T>>(t) < u;
  } else {
    return u >= 0 && t < static_cast<make_unsigned_t<U>>(u);
  }
}

template <typename T, typename U>
constexpr bool cmp_greater(T t, U u) noexcept {
  return cmp_less(u, t);
}

template <typename T, typename U>
constexpr bool cmp_less_equal(T t, U u) noexcept {
  return !cmp_less(u, t);
}

template <typename T, typename U>
constexpr bool cmp_greater_equal(T t, U u) noexcept {
  return !cmp_less(t, u);
}

template <typename R, typename T>
constexpr bool in_range(T t) noexcept {
  return cmp_greater_equal(t, numeric_limits<R>::min()) &&
         cmp_less_equal(t, numeric_limits<R>::max());
}

// ------------------------------------------------------------------
// std::to_array (C++20)
// ------------------------------------------------------------------

template <typename T, size_t N>
constexpr array<remove_cv_t<T>, N> to_array(T (&a)[N]) {
  array<remove_cv_t<T>, N> res{};

  for (size_t i = 0; i < N; ++i) {
    res[i] = a[i];
  }

  return res;
}

template <typename T, size_t N>
constexpr array<remove_cv_t<T>, N> to_array(T (&&a)[N]) {
  array<remove_cv_t<T>, N> res{};

  for (size_t i = 0; i < N; ++i) {
    res[i] = std::move(a[i]);
  }

  return res;
}

// ------------------------------------------------------------------
// A small subset of std::ranges algorithms (C++20)
// ------------------------------------------------------------------

namespace ranges {

template <typename R> constexpr auto begin(R &&r) -> decltype(std::begin(r)) {
  return std::begin(r);
}

template <typename R> constexpr auto end(R &&r) -> decltype(std::end(r)) {
  return std::end(r);
}

template <typename R> constexpr auto size(R &&r) -> decltype(std::size(r)) {
  return std::size(r);
}

template <typename R> constexpr auto data(R &&r) -> decltype(std::data(r)) {
  return std::data(r);
}

template <typename R>
constexpr auto distance(R &&r)
  -> decltype(std::distance(std::begin(r), std::end(r))) {
  return std::distance(std::begin(r), std::end(r));
}

template <typename I>
constexpr auto next(I it, typename std::iterator_traits<I>::difference_type n)
  -> I {
  return std::next(it, n);
}

template <typename I>
constexpr void advance(I &it,
                       typename std::iterator_traits<I>::difference_type n) {
  std::advance(it, n);
}

template <typename I, typename O> struct in_out_result {
  I in;
  O out;
};

template <typename R, typename O>
constexpr auto copy(R &&r, O result)
  -> in_out_result<decltype(std::begin(r)), O> {
  auto last = std::end(r);
  return {std::move(last), std::copy(std::begin(r), std::end(r), result)};
}

template <typename I, typename O>
constexpr in_out_result<I, O> copy(I first, I last, O result) {
  return {last, std::copy(first, last, result)};
}

template <typename I, typename N, typename O,
          typename enable_if<is_integral<N>::value, int>::type = 0>
constexpr in_out_result<I, O> copy_n(I first, N n, O result) {
  auto last = std::next(first, n);
  return {last, std::copy_n(first, n, result)};
}

template <typename R, typename T>
constexpr auto find(R &&r, const T &value) -> decltype(std::begin(r)) {
  return std::find(std::begin(r), std::end(r), value);
}

template <typename I, typename T>
constexpr I find(I first, I last, const T &value) {
  return std::find(first, last, value);
}

template <typename R, typename Pred>
constexpr auto find_if(R &&r, Pred pred) -> decltype(std::begin(r)) {
  return std::find_if(std::begin(r), std::end(r), pred);
}

template <typename I, typename Pred>
constexpr I find_if(I first, I last, Pred pred) {
  return std::find_if(first, last, pred);
}

template <typename R, typename Pred>
constexpr auto find_if_not(R &&r, Pred pred) -> decltype(std::begin(r)) {
  return std::find_if_not(std::begin(r), std::end(r), pred);
}

template <typename I, typename Pred>
constexpr I find_if_not(I first, I last, Pred pred) {
  return std::find_if_not(first, last, pred);
}

template <typename R1, typename R2>
constexpr bool equal(R1 &&r1, R2 &&r2) {
  return std::equal(std::begin(r1), std::end(r1), std::begin(r2),
                    std::end(r2));
}

template <typename R1, typename R2, typename Pred>
constexpr bool equal(R1 &&r1, R2 &&r2, Pred pred) {
  return std::equal(std::begin(r1), std::end(r1), std::begin(r2),
                    std::end(r2), pred);
}

template <typename R> constexpr auto max_element(R &&r) {
  return std::max_element(std::begin(r), std::end(r));
}

template <typename I> constexpr I max_element(I first, I last) {
  return std::max_element(first, last);
}

template <typename R, typename T> constexpr void fill(R &&r, const T &value) {
  std::fill(std::begin(r), std::end(r), value);
}

template <typename R> constexpr void reverse(R &&r) {
  std::reverse(std::begin(r), std::end(r));
}

template <typename R, typename Pred>
constexpr bool all_of(R &&r, Pred pred) {
  return std::all_of(std::begin(r), std::end(r), pred);
}

template <typename I, typename T>
constexpr void iota(I first, I last, T value) {
  std::iota(first, last, value);
}

} // namespace ranges

// ------------------------------------------------------------------
// A small subset of std::format, std::print and std::println (C++20/23)
// ------------------------------------------------------------------

template <typename T, typename Enable = void> struct formatter;

template <typename T>
struct formatter<T, typename enable_if<is_integral<T>::value>::type> {
  std::string format(T v, std::string_view spec = {}) const {
    char buf[64];

    if (spec == "#x" || spec == "x" || spec == "#X" || spec == "X") {
      auto u = static_cast<unsigned long long>(v);
      auto fmt = (spec[0] == '#') ? ((spec.back() == 'X') ? "0X%llX" : "0x%llx")
                                  : ((spec.back() == 'X') ? "%llX" : "%llx");
      snprintf(buf, sizeof(buf), fmt, u);
      return buf;
    }

    if (is_signed<T>::value) {
      snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
    } else {
      snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(v));
    }

    return buf;
  }
};

template <> struct formatter<bool> {
  std::string format(bool v, std::string_view = {}) const {
    return v ? "true" : "false";
  }
};

template <> struct formatter<char> {
  std::string format(char v, std::string_view = {}) const { return {v}; }
};

template <typename T>
struct formatter<T, typename enable_if<is_floating_point<T>::value>::type> {
  std::string format(T v, std::string_view spec = {}) const {
    char buf[128];

    if (spec.size() >= 3 && spec.front() == '.' && spec.back() == 'f') {
      auto prec = std::string{spec.substr(1, spec.size() - 2)};
      auto fmt = "%." + prec + "f";
      snprintf(buf, sizeof(buf), fmt.c_str(), static_cast<double>(v));
      return buf;
    }

    snprintf(buf, sizeof(buf), "%g", static_cast<double>(v));
    return buf;
  }
};

template <> struct formatter<std::string> {
  std::string format(const std::string &v, std::string_view = {}) const {
    return v;
  }
};

template <> struct formatter<std::string_view> {
  std::string format(std::string_view v, std::string_view = {}) const {
    return std::string{v};
  }
};

template <typename T>
struct formatter<
  T *, typename enable_if<!is_same<remove_cv_t<T>, char>::value>::type> {
  std::string format(const T *p, std::string_view = {}) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", static_cast<const void *>(p));
    return buf;
  }
};

template <> struct formatter<const char *> {
  std::string format(const char *v, std::string_view = {}) const {
    return v ? std::string{v} : std::string{"(null)"};
  }
};

template <> struct formatter<char *> {
  std::string format(const char *v, std::string_view = {}) const {
    return v ? std::string{v} : std::string{"(null)"};
  }
};

namespace cxxcompat_detail {

template <typename T>
std::string format_value(const T &v, std::string_view spec) {
  return formatter<decay_t<T>>{}.format(v, spec);
}

inline void format_into(std::string &out, const char *fmt) {
  for (auto p = fmt; *p;) {
    if (p[0] == '{' && p[1] == '{') {
      out += '{';
      p += 2;
      continue;
    }

    if (p[0] == '}' && p[1] == '}') {
      out += '}';
      p += 2;
      continue;
    }

    out += *p++;
  }
}

template <typename T, typename... Rest>
void format_into(std::string &out, const char *fmt, const T &v,
                 const Rest &...rest) {
  for (auto p = fmt; *p; ++p) {
    if (p[0] == '{' && p[1] == '{') {
      out += '{';
      ++p;
      continue;
    }

    if (p[0] == '}' && p[1] == '}') {
      out += '}';
      ++p;
      continue;
    }

    if (*p == '{') {
      auto q = p + 1;
      while (*q && *q != '}') {
        ++q;
      }

      auto spec = p + 1;
      if (*spec == ':') {
        ++spec;
      }

      out += format_value(
        v, std::string_view{spec, static_cast<size_t>(q - spec)});
      format_into(out, q + 1, rest...);
      return;
    }

    out += *p;
  }

  format_into(out, "");
}

} // namespace cxxcompat_detail

template <typename... Args>
std::string format(const char *fmt, const Args &...args) {
  std::string out;
  cxxcompat_detail::format_into(out, fmt, args...);
  return out;
}

template <typename... Args>
void print(FILE *out, const char *fmt, const Args &...args) {
  auto s = format(fmt, args...);
  fwrite(s.data(), 1, s.size(), out);
}

template <typename... Args>
void print(const char *fmt, const Args &...args) {
  print(stdout, fmt, args...);
}

template <typename... Args>
void println(FILE *out, const char *fmt, const Args &...args) {
  auto s = format(fmt, args...);
  fwrite(s.data(), 1, s.size(), out);
  fputc('\n', out);
}

template <typename... Args>
void println(const char *fmt, const Args &...args) {
  println(stdout, fmt, args...);
}

inline void println() { fputc('\n', stdout); }
inline void println(FILE *out) { fputc('\n', out); }

} // namespace std

#endif // !defined(CXXCOMPAT_H)
