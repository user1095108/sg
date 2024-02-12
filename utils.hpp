#ifndef SG_UTILS_HPP
#define SG_UTILS_HPP
# pragma once

#if defined(_WIN32)
# include <malloc.h>
# define SG_ALLOCA(x) _alloca(x)
#else
# include <stdlib.h>
# define SG_ALLOCA(x) alloca(x)
#endif // SG_ALLOCA

#include <cassert>
#include <cstdint>

#include <algorithm>
#include <compare>

#include <numeric> // std::midpoint()
#include <tuple>
#include <utility>

namespace sg::detail
{

using difference_type = std::ptrdiff_t;
using size_type = std::size_t;

template <class C, class U, class V>
concept Comparable =
  !std::is_void_v<
    std::common_comparison_category_t<
      std::remove_cvref_t<
        decltype(std::declval<C>()(std::declval<U>(), std::declval<V>()))
      >
    >
  >;

inline auto assign(auto& ...a) noexcept
{ // assign idiom
  return [&](auto const ...b) noexcept { assign((a = b)...); };
}

//
inline auto left_node(auto const n) noexcept { return n->l_; }
inline auto right_node(auto const n) noexcept { return n->r_; }

inline auto first_node(auto n) noexcept
{ // first node of node subtree, can be node itself
  for (decltype(n) l; (l = left_node(n)); n = l);

  return n;
}

inline auto last_node(auto n) noexcept
{ // last node of node subtree, can be node itself
  for (decltype(n) r; (r = right_node(n)); n = r);

  return n;
}

inline auto first_node2(auto n, decltype(n) p) noexcept
{
  for (decltype(n) l; (l = left_node(n)); assign(n, p)(l, n));

  return std::pair(n, p);
}

inline auto last_node2(auto n, decltype(n) p) noexcept
{
  for (decltype(n) r; (r = right_node(n)); assign(n, p)(r, n));

  return std::pair(n, p);
}

//
inline auto parent_node(auto r0, decltype(r0) n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  auto const& key(n->key());

  for (n = {};;)
  {
    if (auto const c(node::cmp(key, r0->key())); c < 0)
    {
      assign(n, r0)(r0, left_node(r0));
    }
    else if (c > 0)
    {
      assign(n, r0)(r0, right_node(r0));
    }
    else [[unlikely]]
    {
      return n;
    }
  }
}

inline auto next_node(auto r0, decltype(r0) n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (decltype(n) const rn(right_node(n)); rn)
  {
    return first_node(rn);
  }
  else
  {
    auto const& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r0->key())); c < 0)
      {
        assign(n, r0)(r0, left_node(r0)); // deepest parent greater than us
      }
      else if (c > 0)
      {
        r0 = right_node(r0);
      }
      else [[unlikely]]
      {
        return n;
      }
    }
  }
}

inline auto prev_node(auto r0, decltype(r0) n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (decltype(n) const ln(left_node(n)); ln)
  {
    return last_node(ln);
  }
  else
  {
    auto const& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r0->key())); c < 0)
      {
        r0 = left_node(r0);
      }
      else if (c > 0)
      {
        assign(n, r0)(r0, right_node(r0)); // deepest parent less than us
      }
      else [[unlikely]]
      {
        return n;
      }
    }
  }
}

//
inline size_type height(auto const n) noexcept
{
  return n ?
    (left_node(n) || right_node(n)) +
    std::max(height(left_node(n)), height(right_node(n))) :
    size_type{};
}

inline size_type size(auto const n) noexcept
{
  return n ? 1 + size(n->l_) + size(n->r_) : decltype(size(n)){};
}

//
inline auto equal_range(auto n, auto const& k) noexcept
  requires(Comparable<decltype(n->cmp), decltype(k), decltype(n->key())>)
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  decltype(n) g{};

  while (n)
  {
    if (auto const c(node::cmp(k, n->key())); c < 0)
    {
      assign(g, n)(n, left_node(n));
    }
    else if (c > 0)
    {
      n = right_node(n);
    }
    else [[unlikely]]
    {
      if (auto const r(right_node(n)); r)
      {
        g = first_node(r);
      }

      break;
    }
  }

  return std::pair(n ? n : g, g);
}

inline auto find(auto n, auto const& k) noexcept
  requires(Comparable<decltype(n->cmp), decltype(k), decltype(n->key())>)
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
    else [[unlikely]]
    {
      break;
    }
  }

  return n;
}

inline auto erase(auto& r0, auto const& k)
  noexcept(noexcept(delete r0))
  requires(Comparable<decltype(r0->cmp), decltype(k), decltype(r0->key())>)
{
  using pointer = std::remove_cvref_t<decltype(r0)>;
  using node = std::remove_pointer_t<pointer>;

  for (auto q(&r0); *q;)
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
    else [[unlikely]]
    {
      auto const nxt(next_node(r0, n));

      if (auto const l(n->l_), r(n->r_); l && r)
      {
        if (size(l) < size(r))
        {
          auto const [fnn, fnp](first_node2(r, n));

          assign(*q, fnn->l_)(fnn, l);

          if (r != fnn)
          { // avoid loop
            assign(fnp->l_, fnn->r_)(fnn->r_, r);
          }
        }
        else
        {
          auto const [lnn, lnp](last_node2(l, n));

          assign(*q, lnn->r_)(lnn, r);

          if (l != lnn)
          { // avoid loop
            assign(lnp->r_, lnn->l_)(lnn->l_, l);
          }
        }
      }
      else
      {
        *q = l ? l : r;
      }

      assign(n->l_, n->r_)(nullptr, nullptr);
      delete n;

      return nxt;
    }
  }

  return pointer{};
}

}

#endif // SG_UTILS_HPP
