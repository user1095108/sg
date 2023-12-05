#ifndef SG_MAPITERATOR_HPP
# define SG_MAPITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

namespace sg
{

template <typename T>
class mapiterator
{
  using iterator_t = mapiterator<std::remove_const_t<T>>;
  friend mapiterator<T const>;

  T* n_;
  T* const* r_;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = detail::difference_type;
  using value_type = std::conditional_t<
      std::is_const_v<T>,
      typename T::value_type const,
      typename T::value_type
    >;

  using pointer = value_type*;
  using reference = value_type&;

public:
  mapiterator() = default;

  mapiterator(T* const* const r, T* const n = {}) noexcept:
    n_(n),
    r_(r)
  {
  }

  mapiterator(mapiterator const&) = default;
  mapiterator(mapiterator&&) = default;

  mapiterator(iterator_t const& o) noexcept requires(std::is_const_v<T>):
    n_(o.n_),
    r_(o.r_)
  {
  }

  //
  mapiterator& operator=(mapiterator const&) = default;
  mapiterator& operator=(mapiterator&&) = default;

  mapiterator& operator=(iterator_t const& o) noexcept
    requires(std::is_const_v<T>)
  {
    n_ = o.n_; r_ = o.r_; return *this;
  }

  bool operator==(mapiterator const& o) const noexcept { return n_ == o.n_; }

  // increment, decrement
  auto& operator++() noexcept
  {
    n_ = detail::next_node(*r_, n_); return *this;
  }

  auto& operator--() noexcept
  {
    n_ = n_ ? detail::prev_node(*r_, n_) : detail::last_node(*r_);

    return *this;
  }

  mapiterator operator++(int) noexcept
  {
    auto const n(n_); n_ = detail::next_node(*r_, n_); return {n, r_};
  }

  mapiterator operator--(int) noexcept
  {
    auto const n(n_);

    n_ = n_ ? detail::prev_node(*r_, n_) : detail::last_node(*r_);

    return {n, r_};
  }

  // member access
  auto operator->() const noexcept { return &n_->kv_; }
  auto& operator*() const noexcept { return n_->kv_; }

  //
  explicit operator bool() const noexcept { return n_; }
};

}

#endif // SG_MAPITERATOR_HPP
