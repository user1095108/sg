#ifndef SG_INTERVALMAP_HPP
# define SG_INTERVALMAP_HPP
# pragma once

#include <list>
#include <vector>

#include "utils.hpp"

#include "multimapiterator.hpp"

namespace sg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class intervalmap
{
public:
  struct node;

  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = multimapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = intervalmap::value_type;

    static constinit inline auto const cmp{Compare{}};

    node* l_{}, *r_{};

    typename std::tuple_element_t<1, Key> m_;
    std::list<value_type> v_;

    explicit node(auto&& k, auto&& v)
    {
      v_.emplace_back(
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      );

      assert(std::get<0>(std::get<0>(v_.back())) <=
        std::get<1>(std::get<0>(v_.back())));

      m_ = std::get<1>(std::get<0>(v_.back()));
    }

    ~node() noexcept(noexcept(std::declval<Key>().~Key(),
      std::declval<Value>().~Value()))
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept
    {
      return std::get<0>(std::get<0>(v_.front()));
    }

    //
    static auto emplace(auto&& r, auto&& a, auto&& v)
    {
      key_type k(std::forward<decltype(a)>(a));
      auto const& [mink, maxk](k);

      node* q;

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            n = q = new node(std::move(k), std::forward<decltype(v)>(v));

            return 1;
          }

          //
          n->m_ = cmp(n->m_, maxk) < 0 ? maxk : n->m_;

          //
          size_type sl, sr;

          if (auto const c(cmp(mink, n->key())); c < 0)
          {
            if (sl = f(f, n->l_); !sl)
            {
              return 0;
            }

            sr = sg::detail::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_); !sr)
            {
              return 0;
            }

            sl = sg::detail::size(n->l_);
          }
          else
          {
            (q = n)->v_.emplace_back(
              std::move(k),
              std::forward<decltype(v)>(v)
            );

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(), 0) : s;
        }
      );

      f(f, r);

      return q;
    }

    static auto equal_range(auto n, auto&& k) noexcept
    {
      decltype(n) g{};

      for (auto const& [mink, maxk](k); n;)
      {
        if (auto const c(cmp(mink, n->key())); c < 0)
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

    static iterator erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.node()); 1 == n->v_.size())
      {
        return {r, std::get<0>(node::erase(r, std::get<0>(*i)))};
      }
      else if (auto const it(i.iterator()); std::next(it) == n->v_.end())
      {
        auto const nn(std::next(i).node());

        n->v_.erase(it);

        return {r, nn};
      }
      else
      {
        return {r, n, n->v_.erase(it)};
      }
    }

    static auto erase(auto& r0, auto&& k)
    {
      using pointer = typename std::remove_cvref_t<decltype(r0)>;
      using node = std::remove_pointer_t<pointer>;

      if (r0)
      {
        auto const& [mink, maxk](k);

        pointer p{};

        for (auto q(&r0);;)
        {
          auto const n(*q);

          if (auto const c(node::cmp(mink, n->key())); c < 0)
          {
            p = n;
            q = &n->l_;
          }
          else if (c > 0)
          {
            p = n;
            q = &n->r_;
          }
          else
          {
            auto const s0(n->v_.size());

            std::erase_if(
              n->v_,
              [&](auto&& v) noexcept
              {
                return cmp(std::get<0>(v), k) == 0;
              }
            );

            if (auto const s1(n->v_.size()); s1)
            {
              return std::tuple(n, s0 - s1);
            }
            else
            {
              auto const nxt(sg::detail::next_node(r0, n));

              if (auto const l(n->l_), r(n->r_); l && r)
              {
                *q = {};

                if (p)
                {
                  node::reset_max(r0, p->key());
                }

                node::move(r0, l, r);
              }
              else
              {
                *q = l ? l : r;

                if (p)
                {
                  node::reset_max(r0, p->key());
                }
              }

              n->l_ = n->r_ = {};
              delete n;

              return std::tuple(nxt, s0);
            }
          }
        }
      }

      return std::tuple(pointer{}, size_type{});
    }

    static void move(auto& n, auto const ...d)
    {
      auto const f([&](auto&& f, auto& n, auto const d) noexcept -> size_type
        {
          if (!n)
          {
            n = d;

            return sg::detail::size(d);
          }

          //
          n->m_ = cmp(n->m_, d->m_) < 0 ? d->m_ : n->m_;

          //
          size_type sl, sr;

          if (auto const c(cmp(d->key(), n->key())); c < 0)
          {
            if (sl = f(f, n->l_, d); !sl)
            {
              return 0;
            }

            sr = sg::detail::size(n->r_);
          }
          else
          {
            if (sr = f(f, n->r_, d); !sr)
            {
              return 0;
            }

            sl = sg::detail::size(n->l_);
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(), 0) : s;
        }
      );

      (f(f, n, d), ...);
    }

    static decltype(node::m_) node_max(auto const n) noexcept
    {
      decltype(node::m_) m(n->key());

      std::for_each(
        n->v_.cbegin(),
        n->v_.cend(),
        [&](auto&& p) noexcept
        {
          auto const tmp(std::get<1>(std::get<0>(p)));
          m = cmp(m, tmp) < 0 ? tmp : m;
        }
      );

      return m;
    }

    static void reset_max(auto const n, auto&& key) noexcept
    {
      auto const f([&](auto&& f, auto const n) noexcept -> decltype(node::m_)
        {
          auto m(node_max(n));

          auto const l(n->l_), r(n->r_);

          if (auto const c(cmp(key, n->key())); c < 0)
          {
            if (r)
            {
              m = cmp(m, r->m_) < 0 ? r->m_ : m;
            }

            auto const tmp(f(f, l)); // visit left
            m = cmp(m, tmp) < 0 ? tmp : m;
          }
          else if (c > 0)
          {
            if (l)
            {
              m = cmp(m, l->m_) < 0 ? l->m_ : m;
            }

            auto const tmp(f(f, r)); // visit right
            m = cmp(m, tmp) < 0 ? tmp : m;
          }
          else // we are there
          {
            if (l)
            {
              m = cmp(m, l->m_) < 0 ? l->m_ : m;
            }

            if (r)
            {
              m = cmp(m, r->m_) < 0 ? r->m_ : m;
            }
          }

          return n->m_ = m;
        }
      );

      f(f, n);
    }

    auto rebuild()
    {
      std::vector<node*> l;
      l.reserve(1024);

      {
        auto n(sg::detail::first_node(this));

        do
        {
          l.emplace_back(n);
        }
        while ((n = sg::detail::next_node(this, n)));
      }

      auto const f([&](auto&& f, auto const a, auto const b) noexcept -> node*
        {
          auto const i((a + b) / 2);
          auto const n(l[i]);

          switch (b - a)
          {
            case 0:
              n->l_ = n->r_ = {};

              n->m_ = node_max(n);

              break;

            case 1:
              {
                auto const p(l[b]);

                n->l_ = p->l_ = p->r_ = {};
                n->r_ = p;

                n->m_ = std::max(node_max(n), p->m_ = node_max(p),
                  [](auto&& a, auto&& b)noexcept{return node::cmp(a, b) < 0;}
                );

                break;
              }

            default:
              {
                auto const l(n->l_ = f(f, a, i - 1));
                auto const r(n->r_ = f(f, i + 1, b));

                n->m_ = std::max({node_max(n), l->m_, r->m_},
                  [](auto&& a, auto&& b)noexcept{return node::cmp(a, b) < 0;}
                );

                break;
              }
          }

          return n;
        }
      );

      return f(f, 0, l.size() - 1);
    }
  };

private:
  using this_class = intervalmap;
  node* root_{};

public:
  intervalmap() = default;
  intervalmap(std::initializer_list<value_type> const il) { *this = il; }
  intervalmap(intervalmap const& o) { *this = o; }
  intervalmap(intervalmap&&) = default;
  intervalmap(std::input_iterator auto const i, decltype(i) j){insert(i, j);}

  ~intervalmap() noexcept(noexcept(root_->~node())) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept
  {
    static constinit auto const f(
      [](auto&& f, auto const& n) noexcept -> size_type
      {
        return n ? n->v_.size() + f(f, n->l_) + f(f, n->r_) : 0;
      }
    );

    return f(f, root_);
  }

  //
  size_type count(Key const& k) const noexcept
  {
    if (auto n(root_); n)
    {
      for (auto const& [mink, maxk](k);;)
      {
        if (node::cmp(mink, n->m_) < 0)
        {
          if (auto const c(node::cmp(mink, n->key())); c < 0)
          {
            n = sg::detail::left_node(n);
          }
          else if (c > 0)
          {
            n = sg::detail::right_node(n);
          }
          else
          {
            return std::count(
              n->v_.cbegin(),
              n->v_.cend(),
              [&](auto&& p) noexcept
              {
                return node::cmp(k, std::get<0>(p)) == 0;
              }
            );
          }
        }
      }
    }

    return {};
  }

  //
  iterator emplace(auto&& ...a)
  {
    return {
      root_,
      node::emplace(root_, std::forward<decltype(a)>(a)...)
    };
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(iterator(root_, e ? e : g), iterator(root_, g));
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(
      const_iterator(root_, e ? e : g),
      const_iterator(root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(iterator(root_, e ? e : g), iterator(root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(
      const_iterator(root_, e ? e : g),
      const_iterator(root_, g)
    );
  }

  //
  iterator erase(const_iterator const i)
  {
    return node::erase(root_, i);
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(node::erase(root_, k));
  }

  //
  iterator insert(value_type const& v)
  {
    return {
      root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
  }

  iterator insert(value_type&& v)
  {
    return {
      root_,
      node::emplace(root_, std::get<0>(v), std::move(std::get<1>(v)))
    };
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::get<0>(v), std::get<1>(v));
      }
    );
  }

  //
  void all(Key const& k, auto g) const
  {
    auto const& [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    auto const f([&](auto&& f, auto const n) -> void
      {
        if (n && (node::cmp(mink, n->m_) < 0))
        {
          auto const c(node::cmp(maxk, n->key()));

          if (auto const cg0(c > 0); cg0 || (eq && (c == 0)))
          {
            std::for_each(
              n->v_.cbegin(),
              n->v_.cend(),
              [&](auto&& p)
              {
                if (node::cmp(mink, std::get<1>(std::get<0>(p))) < 0)
                {
                  g(std::forward<decltype(p)>(p));
                }
              }
            );

            if (cg0)
            {
              f(f, n->r_);
            }
          }

          f(f, n->l_);
        }
      }
    );

    f(f, root_);
  }

  bool any(Key const& k) const noexcept
  {
    auto const& [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    if (auto n(root_); n && (node::cmp(mink, n->m_) < 0))
    {
      for (;;)
      {
        auto const c(node::cmp(maxk, n->key()));
        auto const cg0(c > 0);

        if (cg0 || (eq && (c == 0)))
        {
          if (auto const i(std::find_if(
                n->v_.cbegin(),
                n->v_.cend(),
                [&](auto&& p) noexcept
                {
                  return node::cmp(mink, std::get<1>(std::get<0>(p))) < 0;
                }
              )
            );
            n->v_.cend() != i
          )
          {
            return true;
          }
        }

        //
        if (auto const l(n->l_); l && (node::cmp(mink, l->m_) < 0))
        {
          n = l;
        }
        else if (auto const r(n->r_);
          cg0 && r && (node::cmp(mink, r->m_) < 0))
        {
          n = r;
        }
        else
        {
          break;
        }
      }
    }

    return false;
  }
};

}

#endif // SG_INTERVALMAP_HPP
