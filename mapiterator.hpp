#ifndef DS_SGTREE_ITERATOR_HPP
# define DS_SGTREE_ITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

template <typename T>
class sgmapiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    sgmapiterator<std::remove_const_t<T>>,
    sgmapiterator<T const>
  >;

  friend inverse_const_t;

  T* r_{};
  T* n_{};

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
  sgmapiterator() = default;

  sgmapiterator(T* const r, T* const n) noexcept:
    r_(r),
    n_(n)
  {
  }

  sgmapiterator(sgmapiterator const&) = default;
  sgmapiterator(sgmapiterator&&) = default;

  sgmapiterator(inverse_const_t const& o) requires(std::is_const_v<T>):
    r_(o.r_),
    n_(o.n_)
  {
  }

  //
  sgmapiterator& operator=(sgmapiterator const&) = default;
  sgmapiterator& operator=(sgmapiterator&&) = default;

  bool operator==(auto const& o) const noexcept
    requires(
      std::is_same_v<std::remove_cvref_t<decltype(o)>, sgmapiterator> ||
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
  auto& operator++() noexcept { return n_ = ds::next(r_, n_), *this; }
  auto& operator--() noexcept { return n_ = ds::prev(r_, n_), *this; }

  auto operator++(int) const noexcept
  {
    return sgmapiterator(r_, ds::next(r_, n_));
  }

  auto operator--(int) const noexcept
  {
    return sgmapiterator(r_, ds::prev(r_, n_));
  }

  // member access
  auto operator->() const noexcept { return &n_->kv_; }
  auto& operator*() const noexcept { return n_->kv_; }
};

#endif // DS_SGTREE_ITERATOR_HPP
