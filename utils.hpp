#ifndef SG_UTILS_HPP
#define SG_UTILS_HPP
# pragma once

#include <cassert>

#include <algorithm>
#include <compare>

#include <utility>

namespace sg
{

namespace detail
{

//
constexpr auto assign(auto& ...a) noexcept
{ // assign idiom
  return [&](auto const ...v) noexcept { ((a = v), ...); };
}

//
inline auto left_node(auto const n) noexcept { return n->l_; }
inline auto right_node(auto const n) noexcept { return n->r_; }

inline auto first_node(auto n) noexcept
{
  for (decltype(n) l; (l = left_node(n)); n = l);

  return n;
}

inline auto last_node(auto n) noexcept
{
  for (decltype(n) r; (r = right_node(n)); n = r);

  return n;
}

inline auto first_node2(auto n, decltype(n) p) noexcept
{
  for (decltype(n) l; (l = left_node(n)); assign(p, n)(n, l));

  return std::pair(n, p);
}

inline auto last_node2(auto n, decltype(n) p) noexcept
{
  for (decltype(n) r; (r = right_node(n)); assign(p, n)(n, r));

  return std::pair(n, p);
}

//
inline auto parent_node(auto r0, decltype(r0) n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  auto&& key(n->key());

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
    else
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
    auto&& key(n->key());

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
      else
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
    auto&& key(n->key());

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
      assign(g, n)(n, left_node(n));
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

  return std::pair(n, g);
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

inline auto erase(auto& r0, auto&& k)
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
    else
    {
      auto const nxt(next_node(r0, n));

      if (auto const l(n->l_), r(n->r_); l && r)
      {
        if (size(l) < size(r))
        {
          auto const [fnn, fnp](first_node2(r, n));

          assign(*q, fnn->l_)(fnn, l);

          if (r != fnn)
          {
            assign(fnp->l_, fnn->r_)(fnn->r_, r);
          }
        }
        else
        {
          auto const [lnn, lnp](last_node2(l, n));

          assign(*q, lnn->r_)(lnn, r);

          if (l != lnn)
          {
            assign(lnp->r_, lnn->l_)(lnn->l_, l);
          }
        }
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

  return pointer{};
}

}

constexpr auto erase(auto& c, auto const& k)
  requires(
    requires{
      c.begin(); c.end();
      &std::remove_cvref_t<decltype(c)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(c)>>
  )
{
  return c.erase(k);
}

constexpr auto erase_if(auto& c, auto pred)
  requires(
    requires{
      c.begin(); c.end();
      &std::remove_cvref_t<decltype(c)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(c)>>
  )
{
  std::size_t r{};

  for (auto i(c.begin()); i.node();)
  {
    i = pred(*i) ? (++r, c.erase(i)) : std::next(i);
  }

  return r;
}

constexpr void swap(auto& lhs, decltype(lhs) rhs) noexcept
  requires(
    requires{
      lhs.begin(); lhs.end();
      &std::remove_cvref_t<decltype(lhs)>::node::cmp;
    } &&
    !std::is_const_v<std::remove_reference_t<decltype(lhs)>>
  )
{
  lhs.swap(rhs);
}

}

#endif // SG_UTILS_HPP
