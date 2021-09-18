#ifndef SG_MULTIMAPITERATOR_HPP
# define SG_MULTIMAPITERATOR_HPP
# pragma once

#include <iterator>

#include <type_traits>

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

  node_t* r_{};
  node_t* n_{};

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
  multimapiterator() noexcept = default;

  multimapiterator(node_t* const r) noexcept:
    r_(r)
  {
  }

  multimapiterator(node_t* const r, node_t* const n) noexcept:
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

  multimapiterator(node_t* const r, node_t* const n,
    decltype(i_) const i) noexcept:
    r_(r),
    n_(n),
    i_(i)
  {
  }

  multimapiterator(multimapiterator const&) noexcept = default;
  multimapiterator(multimapiterator&&) noexcept = default;

  multimapiterator(inverse_const_t const& o) noexcept
    requires(std::is_const_v<T>):
    r_(o.r_),
    n_(o.n_),
    i_(o.i_)
  {
  }

  //
  multimapiterator& operator=(multimapiterator const&) noexcept = default;
  multimapiterator& operator=(multimapiterator&&) noexcept = default;

  bool operator==(auto const& o) const noexcept
    requires(
      std::is_same_v<std::remove_cvref_t<decltype(o)>, multimapiterator> ||
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
      n_ = sg::detail::next_node(r_, n_);
      i_ = n_ ? n_->v_.begin() : decltype(i_){};
    }

    return *this;
  }

  auto& operator--() noexcept
  {
    if (!n_ || (n_->v_.begin() == i_))
    {
      n_ = sg::detail::prev_node(r_, n_);
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

#endif // SG_MULTIMAPITERATOR_HPP
