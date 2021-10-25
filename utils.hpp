#ifndef SG_UTILS_HPP
#define SG_UTILS_HPP
# pragma once

#include <cassert>

#include <algorithm>

#include <compare>

namespace sg
{

namespace detail
{

//
inline auto left_node(auto const n) noexcept { return n->l_; }
inline auto right_node(auto const n) noexcept { return n->r_; }

inline auto first_node(auto n) noexcept
{
  for (decltype(n) p; (p = left_node(n)); n = p);

  return n;
}

inline auto last_node(auto n) noexcept
{
  for (decltype(n) p; (p = right_node(n)); n = p);

  return n;
}

//
inline auto parent_node(auto r0, auto n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  auto&& key(n->key());

  for (n = {};;)
  {
    if (auto const c(node::cmp(key, r0->key())); c < 0)
    {
      n = r0;
      r0 = left_node(r0);
    }
    else if (c > 0)
    {
      n = r0;
      r0 = right_node(r0);
    }
    else
    {
      return n;
    }
  }
}

inline auto next_node(auto r0, auto n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (auto const p(right_node(n)); p)
  {
    decltype(n) const f(first_node(p));

    return f ? f : p;
  }
  else
  {
    auto&& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r0->key())); c < 0)
      {
        n = r0;
        r0 = left_node(r0); // deepest parent greater than us
      }
      else if (c > 0)
      {
        r0 = right_node(r0);
      }
      else
      {
        return n;
      }
    }
  }
}

inline auto prev_node(auto r0, auto n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (!n)
  {
    return last_node(r0);
  }
  else if (auto const p(left_node(n)); p)
  {
    decltype(n) const l(last_node(p));

    return l ? l : p;
  }
  else
  {
    auto&& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r0->key())); c < 0)
      {
        r0 = left_node(r0);
      }
      else if (c > 0)
      {
        n = r0;
        r0 = right_node(r0); // deepest parent less than us
      }
      else
      {
        return n;
      }
    }
  }
}

//
inline std::size_t height(auto const n) noexcept
{
  return n ?
    (left_node(n) || right_node(n)) +
    std::max(height(left_node(n)), height(right_node(n))) :
    std::size_t{};
}

inline std::size_t size(auto const n) noexcept
{
  return n ? 1 + size(n->l_) + size(n->r_) : 0;
}

//
inline auto equal_range(auto n, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  decltype(n) g{};

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      g = n;
      n = left_node(n);
    }
    else if (c > 0)
    {
      n = right_node(n);
    }
    else
    {
      if (auto const r(right_node(n)); r)
      {
        g = first_node(r);
      }

      break;
    }
  }

  return std::tuple(n, g);
}

inline auto find(auto n, auto&& k) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      n = left_node(n);
    }
    else if (c > 0)
    {
      n = right_node(n);
    }
    else
    {
      break;
    }
  }

  return n;
}

inline void move(auto& n, auto const ...d)
{
  using pointer = std::remove_cvref_t<decltype(n)>;
  using node = std::remove_pointer_t<pointer>;

  auto const f([&](auto&& f, auto& n, auto const d) noexcept -> std::size_t
    {
      if (!n)
      {
        n = d;

        return size(d);
      }

      //
      std::size_t sl, sr;

      if (auto const c(node::cmp(d->key(), n->key())); c < 0)
      {
        if (sl = f(f, n->l_, d); !sl)
        {
          return 0;
        }

        sr = size(n->r_);
      }
      else
      {
        if (sr = f(f, n->r_, d); !sr)
        {
          return 0;
        }

        sl = size(n->l_);
      }

      //
      auto const s(1 + sl + sr), S(2 * s);

      return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(), 0) : s;
    }
  );

  (f(f, n, d), ...);
}

inline auto erase(auto& r0, auto&& k)
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  if (r0)
  {
    for (auto q(&r0);;)
    {
      auto const n(*q);

      if (auto const c(node::cmp(k, n->key())); c < 0)
      {
        q = &n->l_;
      }
      else if (c > 0)
      {
        q = &n->r_;
      }
      else
      {
        auto const nxt(next_node(r0, n));

        if (auto const l(n->l_), r(n->r_); l && r)
        {
          *q = {};
          sg::detail::move(r0, l, r);
        }
        else
        {
          *q = l ? l : r;
        }

        n->l_ = n->r_ = {};
        delete n;

        return nxt;
      }
    }
  }

  return pointer{};
}

}

template <typename T> requires(
  requires(T c){c.begin(); c.end(); &T::node::cmp;})
inline bool operator==(T const& lhs, T const& rhs) noexcept 
{
  return std::equal(
    lhs.begin(), lhs.end(),
    rhs.begin(), rhs.end(),
    [](auto&& a, auto && b) noexcept
    {
      return T::node::cmp(a, b) == 0;
    }
  );
}

template <typename T> requires(
  requires(T c){c.begin(); c.end(); &T::node::cmp;})
inline auto operator<=>(T const& lhs, T const& rhs) noexcept
{
  return std::lexicographical_compare_three_way(
    lhs.begin(), lhs.end(),
    rhs.begin(), rhs.end(),
    T::node::cmp
  );
}

constexpr auto erase(auto& c, auto const& k) requires(
  requires{c.begin(); c.end();
  &std::remove_cvref_t<decltype(c)>::node::cmp;} &&
  !std::is_const_v<std::remove_cvref_t<decltype(c)>>)
{
  return c.erase(k);
}

constexpr auto erase_if(auto& c, auto pred) requires(
  requires{c.begin(); c.end();
  &std::remove_cvref_t<decltype(c)>::node::cmp;} &&
  !std::is_const_v<std::remove_cvref_t<decltype(c)>>)
{
  std::size_t r{};

  auto const end(c.end());

  for (auto i(c.begin()); end != i;)
  {
    i = pred(*i) ? (++r, c.erase(i)) : std::next(i);
  }

  return r;
}

}

#endif // SG_UTILS_HPP
