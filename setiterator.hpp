#ifndef DS_SGSET_ITERATOR_HPP
# define DS_SGSET_ITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

template <typename T>
class sgsetiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    sgsetiterator<std::remove_const_t<T>>,
    sgsetiterator<T const>
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
  sgsetiterator() = default;

  sgsetiterator(T* const r, T* const n) noexcept:
    r_(r),
    n_(n)
  {
  }

  sgsetiterator(sgsetiterator const&) = default;
  sgsetiterator(sgsetiterator&&) = default;

  sgsetiterator(inverse_const_t const& o) requires(std::is_const_v<T>):
    r_(o.r_),
    n_(o.n_)
  {
  }

  //
  sgsetiterator& operator=(sgsetiterator const&) = default;
  sgsetiterator& operator=(sgsetiterator&&) = default;

  bool operator==(auto const& o) const noexcept
    requires(
      std::is_same_v<std::remove_cvref_t<decltype(o)>, sgsetiterator> ||
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
    return sgsetiterator(r_, r_->next(n_));
  }

  auto operator--(int) const noexcept
  {
    return sgsetiterator(r_, r_->prev(n_));
  }

  // member access
  auto operator->() const noexcept { return &n_->k_; }
  auto& operator*() const noexcept { return n_->k_; }
};

#endif // DS_SGSET_ITERATOR_HPP
