#ifndef SG_MULTIMAPITERATOR_HPP
# define SG_MULTIMAPITERATOR_HPP
# pragma once

#include <list>

#include <type_traits>

namespace sg
{

template <typename T>
class multimapiterator
{
  using inverse_const_t = std::conditional_t<
    std::is_const_v<T>,
    multimapiterator<std::remove_const_t<T>>,
    multimapiterator<T const>
  >;

  friend inverse_const_t;

  using node_t = std::remove_const_t<T>;

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

private:
  node_t* n_;
  std::conditional_t<
    std::is_const_v<T>,
    typename std::list<std::remove_const_t<value_type>>::const_iterator,
    typename std::list<std::remove_const_t<value_type>>::iterator
  > i_;
  node_t* const* r_;

public:
  multimapiterator() = default;

  multimapiterator(decltype(r_) const r) noexcept:
    n_(),
    i_(),
    r_(r)
  {
  }

  multimapiterator(decltype(r_) const r, decltype(n_) const n) noexcept:
    n_(n),
    r_(r)
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

  multimapiterator(decltype(r_) const r, decltype(n_) const n,
    decltype(i_) const i) noexcept:
    n_(n),
    i_(i),
    r_(r)
  {
  }

  multimapiterator(multimapiterator const&) = default;
  multimapiterator(multimapiterator&&) = default;

  multimapiterator(inverse_const_t const& o) noexcept
    requires(std::is_const_v<T>):
    n_(o.n_),
    i_(o.i_),
    r_(o.r_)
  {
  }

  //
  multimapiterator& operator=(multimapiterator const&) = default;
  multimapiterator& operator=(multimapiterator&&) = default;

  bool operator==(multimapiterator const& o) const noexcept
  {
    return (n_ == o.n_) && (i_ == o.i_);
  }

  // increment, decrement
  auto& operator++() noexcept
  {
    if (i_ = std::next(i_); n_->v_.end() == i_)
    {
      auto const n(n_ = detail::next_node(*r_, n_));
      i_ = n ? n->v_.begin() : decltype(i_){};
    }

    return *this;
  }

  auto& operator--() noexcept
  {
    if (!n_)
    {
      if (auto const n(detail::last_node(*r_)); (n_ = n))
      {
        i_ = std::prev(n->v_.end());
      }
    }
    else if (n_->v_.begin() == i_)
    {
      if (auto const n(detail::prev_node(*r_, n_)); (n_ = n))
      {
        i_ = std::prev(n->v_.end());
      }
    }
    else
    {
      i_ = std::prev(i_);
    }

    return *this;
  }

  auto operator++(int) noexcept { auto const r(*this); ++*this; return r; }
  auto operator--(int) noexcept { auto const r(*this); --*this; return r; }

  // member access
  auto& operator->() const noexcept { return i_; }
  auto& operator*() const noexcept { return *i_; }

  //
  auto& i() const noexcept { return i_; }
  auto n() const noexcept { return n_; }

  //
  explicit operator bool() const noexcept { return n_; }
};

}

#endif // SG_MULTIMAPITERATOR_HPP
