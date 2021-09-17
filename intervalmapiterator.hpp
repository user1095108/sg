#ifndef SG_INTERVALMAPITERATOR_HPP
# define SG_INTERVALMAPITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

template <typename T>
class intervalmapiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    intervalmapiterator<std::remove_const_t<T>>,
    intervalmapiterator<T const>
  >;

  friend inverse_const_t;

  std::remove_const_t<T>* r_{};
  std::remove_const_t<T>* n_{};

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

private:
  std::conditional_t<
    std::is_const_v<T>,
    typename std::list<std::remove_const_t<value_type>>::const_iterator,
    typename std::list<std::remove_const_t<value_type>>::iterator
  > i_{};

public:
  intervalmapiterator() = default;

  intervalmapiterator(std::remove_const_t<T>* const r,
    std::remove_const_t<T>* const n) noexcept:
    r_(r),
    n_(n)
  {
    if (n)
    {
      if constexpr(std::is_const_v<T>)
      {
        i_ = n->v_.cbegin();
      }
      else
      {
        i_ = n->v_.begin();
      }
    }
  }

  intervalmapiterator(std::remove_const_t<T>* const r,
    std::remove_const_t<T>* const n,
    decltype(i_) const i) noexcept:
    r_(r),
    n_(n),
    i_(i)
  {
  }

  intervalmapiterator(intervalmapiterator const&) = default;
  intervalmapiterator(intervalmapiterator&&) = default;

  intervalmapiterator(inverse_const_t const& o) requires(std::is_const_v<T>):
    r_(o.r_),
    n_(o.n_),
    i_(o.i_)
  {
  }

  //
  intervalmapiterator& operator=(intervalmapiterator const&) = default;
  intervalmapiterator& operator=(intervalmapiterator&&) = default;

  bool operator==(auto const& o) const noexcept
    requires(
      std::is_same_v<std::remove_cvref_t<decltype(o)>, intervalmapiterator> ||
      std::is_same_v<std::remove_cvref_t<decltype(o)>, inverse_const_t>
    )
  {
    return (n_ == o.n_) && (i_ == o.i_);
  }

  bool operator!=(auto&& o) const noexcept
  {
    return !(*this == std::forward<decltype(o)>(o));
  }

  // increment, decrement
  auto& operator++() noexcept
  {
    if (i_ = std::next(i_); n_->v_.end() == i_)
    {
      n_ = sg::next(r_, n_);
      i_ = n_ ? n_->v_.begin() : decltype(i_){};
    }

    return *this;
  }

  auto& operator--() noexcept
  {
    if (!n_ || (n_->v_.begin() == i_))
    {
      n_ = sg::prev(r_, n_);
      i_ = n_ ? std::prev(n_->v_.end()) : decltype(i_){};
    }
    else
    {
      i_ = std::prev(i_);
    }

    return *this;
  }

  auto operator++(int) noexcept { auto r(*this); return ++*this, r; }
  auto operator--(int) noexcept { auto r(*this); return --*this, r; }

  // member access
  auto& operator->() const noexcept { return i_; }
  auto& operator*() const noexcept { return *i_; }

  //
  auto& iterator() const noexcept { return i_; }
  auto node() const noexcept { return n_; }
};

#endif // SG_INTERVALMAPITERATOR_HPP
