#ifndef SG_UTILS_HPP
#define SG_UTILS_HPP
# pragma once

#include <cassert>

#include <type_traits>

namespace sg
{

namespace detail
{

inline std::size_t height(auto&& n) noexcept
{
  return n ?
    (n->l_ || n->r_) + std::max(height(n->l_), height(n->r_)) :
    0;
}

inline std::size_t size(auto&& n) noexcept
{
  return n ? 1 + size(n->l_) + size(n->r_) : 0;
}

//
inline auto first_node(auto n) noexcept
{
  for (decltype(n) p; (p = n->l_.get()); n = p);

  return n;
}

inline auto last_node(auto n) noexcept
{
  for (decltype(n) p; (p = n->r_.get()); n = p);

  return n;
}

//
inline auto next(auto r, auto n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (auto const p(n->r_.get()); p)
  {
    decltype(n) const l(first_node(p));

    return l ? l : p;
  }
  else
  {
    auto&& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r->key())); c < 0)
      {
        n = r;
        r = r->l_.get(); // deepest parent greater than us
      }
      else if (c > 0)
      {
        r = r->r_.get();
      }
      else
      {
        return n;
      }
    }
  }
}

inline auto prev(auto r, auto n) noexcept
{
  using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

  if (!n)
  {
    return last_node(r);
  }
  else if (auto const p(n->l_.get()); p)
  {
    decltype(n) const r(last_node(p));

    return r ? r : p;
  }
  else
  {
    auto&& key(n->key());

    for (n = {};;)
    {
      if (auto const c(node::cmp(key, r->key())); c < 0)
      {
        r = r->l_.get();
      }
      else if (c > 0)
      {
        n = r;
        r = r->r_.get(); // deepest parent less than us
      }
      else
      {
        return n;
      }
    }
  }
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
      n = n->l_.get();
    }
    else if (c > 0)
    {
      n = n->r_.get();
    }
    else
    {
      if (auto const r(n->r_.get()); r)
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
      n = n->l_.get();
    }
    else if (c > 0)
    {
      n = n->r_.get();
    }
    else
    {
      break;
    }
  }

  return n;
}

inline void move(auto& n, auto& ...d)
{
  using pointer = typename std::remove_cvref_t<decltype(n)>::pointer;
  using node = std::remove_pointer_t<pointer>;

  auto const f([&](auto&& f, auto& n, auto& d) noexcept -> std::size_t
    {
      if (!n)
      {
        n = std::move(d);

        return 1;
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

      return (3 * sl > S) || (3 * sr > S) ?
        (n.reset(n.release()->rebuild()), 0) :
        s;
    }
  );

  (f(f, n, d), ...);
}

inline auto erase(auto& r, auto&& k)
{
  using pointer = typename std::remove_cvref_t<decltype(r)>::pointer;
  using node = std::remove_pointer_t<pointer>;

  if (auto n(r.get()); n)
  {
    for (pointer p{};;)
    {
      if (auto const c(node::cmp(k, n->key())); c < 0)
      {
        p = n;
        n = n->l_.get();
      }
      else if (c > 0)
      {
        p = n;
        n = n->r_.get();
      }
      else
      {
        auto& q(!p ? r : p->l_.get() == n ? p->l_ : p->r_);

        auto const nxt(next(r.get(), n));

        if (!n->l_ && !n->r_)
        {
          q.reset();
        }
        else if (!n->l_ || !n->r_)
        {
          q = std::move(n->l_ ? n->l_ : n->r_);
        }
        else
        {
          q.release(); // order important

          sg::detail::move(r, n->l_, n->r_);

          delete n;
        }

        return nxt;
      }
    }
  }

  return pointer{};
}

}

constexpr auto erase(auto& c, auto const& k)
{
  c.erase(k);
}

constexpr auto erase_if(auto& c, auto pred)
{
  auto const end(c.end());

  for (auto i(c.begin()); end != c;)
  {
    i = pred(*i) ? c.erase(i) : std::next(i);
  }
}

template <typename T>
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

template <typename T>
inline auto operator<=>(T const& lhs, T const& rhs) noexcept
{
  return std::lexicographical_compare_three_way(
    lhs.begin(), lhs.end(),
    rhs.begin(), rhs.end(),
    T::node::cmp
  );
}

}

#endif // SG_UTILS_HPP
