#ifndef SG_SET_ITERATOR_HPP
# define SG_SET_ITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

template <typename T>
class setiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    setiterator<std::remove_const_t<T>>,
    setiterator<T const>
  >;

  friend inverse_const_t;

  T* r_{};
  T* n_{};

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = std::conditional_t<
      std::is_const_v<T>,
      typename T::key_type const,
      typename T::key_type
    >;

  using pointer = value_type*;
  using reference = value_type&;

public:
  setiterator() = default;

  setiterator(T* const r, T* const n) noexcept:
    r_(r),
    n_(n)
  {
  }

  setiterator(setiterator const&) = default;
  setiterator(setiterator&&) = default;

  setiterator(inverse_const_t const& o) requires(std::is_const_v<T>):
    r_(o.r_),
    n_(o.n_)
  {
  }

  //
  setiterator& operator=(setiterator const&) = default;
  setiterator& operator=(setiterator&&) = default;

  bool operator==(auto const& o) const noexcept
    requires(
      std::is_same_v<std::remove_cvref_t<decltype(o)>, setiterator> ||
      std::is_same_v<std::remove_cvref_t<decltype(o)>, inverse_const_t>
    )
  {
    return n_ == o.n_;
  }

  bool operator!=(auto&& o) const noexcept
  {
    return !(*this == std::forward<decltype(o)>(o));
  }

  // increment, decrement
  auto& operator++() noexcept { return n_ = next(r_, n_), *this; }
  auto& operator--() noexcept { return n_ = prev(r_, n_), *this; }

  auto operator++(int) const noexcept
  {
    return setiterator(r_, r_->next(n_));
  }

  auto operator--(int) const noexcept
  {
    return setiterator(r_, r_->prev(n_));
  }

  // member access
  auto operator->() const noexcept { return &n_->k_; }
  auto& operator*() const noexcept { return n_->k_; }
};

#endif // SG_SET_ITERATOR_HPP
