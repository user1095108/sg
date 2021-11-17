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
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    mapiterator<std::remove_const_t<T>>,
    mapiterator<T const>
  >;

  friend inverse_const_t;

  T* n_;
  T* const* r_;

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
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

  mapiterator(inverse_const_t const& o) noexcept requires(std::is_const_v<T>):
    n_(o.n_),
    r_(o.r_)
  {
  }

  //
  mapiterator& operator=(mapiterator const&) = default;
  mapiterator& operator=(mapiterator&&) = default;

  bool operator==(mapiterator const& o) const noexcept { return o.n_ == n_; }
  bool operator!=(mapiterator const&) const = default;

  // increment, decrement
  auto& operator++() noexcept
  {
    n_ = detail::next_node(*r_, n_);
    return *this;
  }

  auto& operator--() noexcept
  {
    n_ = n_ ? detail::prev_node(*r_, n_) : detail::last_node(*r_);
    return *this;
  }

  auto operator++(int) noexcept { auto const r(*this); ++*this; return r; }
  auto operator--(int) noexcept { auto const r(*this); --*this; return r; }

  // member access
  auto operator->() const noexcept { return &n_->kv_; }
  auto& operator*() const noexcept { return n_->kv_; }

  //
  auto node() const noexcept { return n_; }
};

}

#endif // SG_MAPITERATOR_HPP
