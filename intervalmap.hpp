#ifndef SG_INTERVALMAP_HPP
# define SG_INTERVALMAP_HPP
# pragma once

#include <algorithm>
#include <execution>

#include <compare>

#include <memory>

#include <list>
#include <vector>

#include "utils.hpp"

#include "intervalmapiterator.hpp"

namespace sg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class intervalmap
{
  static constexpr auto invoke_all(auto f, auto&& ...a) noexcept(noexcept(
    (f(std::forward<decltype(a)>(a)), ...)))
  {
    (f(std::forward<decltype(a)>(a)), ...);
  }

public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

  using size_type = std::size_t;

  struct node
  {
    using value_type = std::pair<Key const, Value>;

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    typename std::tuple_element_t<0, Key> const k_;
    typename std::tuple_element_t<1, Key> m_;

    std::list<std::pair<Key const, Value>> v_;

    explicit node(auto&& k, auto&& v):
      k_(std::get<0>(k)),
      m_(std::get<1>(k))
    {
      assert(std::get<0>(k) <= std::get<1>(k));
      v_.emplace_back(
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      );
    }

    //
    auto&& key() const noexcept { return k_; }

    //
    static auto emplace(auto&& r, auto&& k, auto&& v)
    {
      node* q{};

      auto const& [mink, maxk](k);

      auto const f([&](auto&& f, auto& n) noexcept -> std::size_t
        {
          if (!n)
          {
            n.reset(
              q = new node(
                std::forward<decltype(k)>(k),
                std::forward<decltype(v)>(v)
              )
            );

            return 1;
          }

          //
          n->m_ = std::max(n->m_, maxk);

          //
          size_type sl, sr;

          if (auto const c(cmp(mink, n->key())); c < 0)
          {
            if (sl = f(f, n->l_); !sl)
            {
              return 0;
            }

            sr = sg::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_); !sr)
            {
              return 0;
            }

            sl = sg::size(n->l_);
          }
          else
          {
            q = n.get();
            n->v_.emplace_back(
              std::forward<decltype(k)>(k),
              std::forward<decltype(v)>(v)
            );

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ?
            (n.reset(n.release()->rebuild()), 0) :
            s;
        }
      );

      f(f, r);

      return q;
    }

    static auto erase(auto& r, auto&& k)
    {
      using pointer = typename std::remove_cvref_t<decltype(r)>::pointer;
      using node = std::remove_pointer_t<pointer>;

      if (auto n(r.get()); n)
      {
        auto const [mink, maxk](k);

        for (pointer p{};;)
        {
          if (auto const c(node::cmp(mink, n->key())); c < 0)
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
            auto const s0(n->v_.size());

            std::erase_if(
              n->v_,
              [&](auto&& p) noexcept
              {
                return cmp(maxk, std::get<1>(std::get<0>(p))) == 0;
              }
            );

            if (auto const s1(n->v_.size()); s1)
            {
              return std::tuple(n, s0 - s1);
            }
            else
            {
              auto& q(!p ? r : p->l_.get() == n ? p->l_ : p->r_);

              auto const nxt(next(r.get(), n));
              bool rebuilt{};

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

                rebuilt = node::move(r, n->l_, n->r_);

                delete n;
              }

              if (p && !rebuilt)
              {
                node::reset_max(r.get());
              }

              return std::tuple(nxt, s0);
            }
          }
        }
      }

      return std::tuple(pointer{}, 0);
    }

    static bool move(auto& n, auto& ...d)
    {
      auto const f([&](auto&& f, auto& n, auto& d) noexcept -> std::size_t
        {
          if (!n)
          {
            n = std::move(d);

            return 1;
          }

          //
          n->m_ = std::max(n->m_, d->m_);

          //
          std::size_t sl, sr;

          if (auto const c(cmp(d->k_, n->key())); c < 0)
          {
            if (sl = f(f, n->l_, d); !sl)
            {
              return 0;
            }

            sr = node::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_, d); !sr)
            {
              return 0;
            }

            sl = node::size(n->l_);
          }
          else
          {
            std::move(d->v_.begin(), d->v_.end(), std::back_inserter(n->v_));

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ?
            (n.reset(n.release()->rebuild()), 0) :
            s;
        }
      );

      return (!f(f, n, d) || ...);
    }

    static decltype(node::m_) reset_max(auto const n) noexcept
    {
      if (n)
      {
        decltype(node::m_) m(n->k_);

        std::for_each(
          n->v_.cbegin(),
          n->v_.cend(),
          [&](auto&& p) noexcept
          {
            m = std::max(m, std::get<1>(std::get<0>(p)));
          }
        );

        return n->m_ = std::max(
          {m, reset_max(n->l_.get()), reset_max(n->r_.get())}
        );
      }
      else
      {
        return {};
      }
    }

    auto rebuild()
    {
      std::vector<node*> l;

      {
        auto n(first_node(this));

        do
        {
          l.emplace_back(n);

          n = next(this, n);
        }
        while (n);
      }

      auto const f([&](auto&& f, auto const a, auto const b) noexcept -> node*
        {
          auto const i((a + b) / 2);
          auto const n(l[i]);
          assert(n);

          switch (b - a)
          {
            case 0:
              n->l_.release();
              n->r_.release();

              reset_max(n);

              break;

            case 1:
              {
                auto const p(l[b]);

                p->l_.release();
                p->r_.release();

                n->l_.release();
                n->r_.release();

                n->r_.reset(p);

                reset_max(n);

                break;
              }

            default:
              {
                assert(i);
                n->l_.release();
                n->r_.release();

                auto m(reset_max(n));

                if (auto const p(f(f, a, i - 1)); p)
                {
                  m = std::max(m, p->m_);
                  n->l_.reset(p);
                }

                if (auto const p(f(f, i + 1, b)); p)
                {
                  m = std::max(m, p->m_);
                  n->r_.reset(p);
                }

                n->m_ = m;

                break;
              }
          }

          return n;
        }
      );

      return f(f, 0, l.size() - 1);
    }
  };

  using const_iterator = intervalmapiterator<node const>;
  using iterator = intervalmapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  std::unique_ptr<node> root_;

public:
  intervalmap() = default;
  intervalmap(intervalmap&&) = default;

  auto& operator=(intervalmap&& o) noexcept
  {
    return root_ = std::move(o.root_), *this;
  }

  //
  auto root() const noexcept { return root_.get(); }

  //
  void clear() { root_.reset(); }
  bool empty() const noexcept { return !size(); }
  auto max_size() const noexcept { return ~size_type{} / 3; }
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

  // iterators
  iterator begin() noexcept
  {
    return root_ ?
      iterator(root_.get(), sg::first_node(root_.get())) :
      iterator();
  }

  iterator end() noexcept { return {}; }

  // const iterators
  const_iterator begin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), node::first_node(root_.get())) :
      const_iterator();
  }

  const_iterator end() const noexcept { return {}; }

  const_iterator cbegin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), sg::first_node(root_.get())) :
      const_iterator();
  }

  const_iterator cend() const noexcept { return {}; }

  // reverse iterators
  reverse_iterator rbegin() noexcept
  {
    return root_ ?
      reverse_iterator(iterator(root_.get(), nullptr)) :
      reverse_iterator();
  }

  reverse_iterator rend() noexcept
  {
    return root_ ?
      reverse_iterator(iterator{root_.get(), sg::first_node(root_.get())}) :
      reverse_iterator();
  }

  // const reverse iterators
  const_reverse_iterator crbegin() const noexcept
  {
    return root_ ?
      const_reverse_iterator(const_iterator(root_.get(), nullptr)) :
      const_reverse_iterator();
  }

  const_reverse_iterator crend() const noexcept
  {
    return root_ ?
      const_reverse_iterator(
        const_iterator{root_.get(), sg::first_node(root_.get())}
      ) :
      const_reverse_iterator();
  }

  //
  bool contains(Key const& k) const
  {
    return bool(sg::find(root_.get(), k));
  }

  bool contains(auto&& k) const
  {
    return bool(sg::find(root_.get(), std::forward<decltype(k)>(k)));
  }

  size_type count(Key const& k) const noexcept
  {
    if (auto n(root_.get()); n)
    {
      for (auto const [mink, maxk](k);;)
      {
        if (node::cmp(mink, n->m_) < 0)
        {
          if (auto const c(node::cmp(mink, n->key())); c < 0)
          {
            n = n->l_.get();
          }
          else if (c > 0)
          {
            n = n->r_.get();
          }
          else
          {
            size_type cnt{};

            std::for_each(
              std::execution::unseq,
              n->v_.cbegin(),
              n->v_.cend(),
              [&](auto&& p) noexcept
              {
                if (node::cmp(maxk, std::get<1>(std::get<0>(p))) == 0)
                {
                  ++cnt;
                }
              }
            );

            return cnt;
          }
        }
      }
    }

    return {};
  }

  //
  auto emplace(auto&& k, auto&& v)
  {
    auto const n(
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      )
    );

    return iterator(root_.get(), n);
  }

  //
  iterator erase(const_iterator const i)
  {
    return iterator(root_.get(),
      std::get<0>(sg::erase(root_, std::get<0>(*i))));
  }

  auto erase(const_iterator a, const_iterator const b)
  {
    for (; a != b; a = erase(a));

    return a;
  }

  auto erase(std::initializer_list<const_iterator> il)
  {
    iterator r;

    std::for_each(
      il.begin(),
      il.end(),
      [&](auto&& i) { r = erase(i); }
    );

    return r;
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(sg::erase(root_, k));
  }

  //
  auto find(Key const& k) noexcept
  {
    return iterator(root_.get(), sg::find(root_.get(), k));
  }

  auto find(Key const& k) const noexcept
  {
    return const_iterator(root_.get(), sg::find(root_.get(), k));
  }

  //
  auto insert(value_type const& v)
  {
    return iterator(root_.get(),
      node::emplace(root_, v.first, v.second));
  }

  auto insert(value_type&& v)
  {
    return iterator(root_.get(),
      node::emplace(root_, std::move(v.first), std::move(v.second)));
  }

  void insert(auto const i, decltype(i) j)
  {
    std::for_each(
      std::execution::unseq,
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::get<0>(v), std::get<1>(v));
      }
    );
  }

  void insert(std::initializer_list<value_type> const il)
  {
    std::for_each(
      std::execution::unseq,
      il.begin(),
      il.end(),
      [&](auto&& v)
      {
        emplace(std::get<0>(v), std::get<1>(v));
      }
    );
  }

  //
  void all(Key const& k, auto g) const
  {
    auto const [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    auto const f([&](auto&& f, auto const n) -> void
      {
        if (n && (node::cmp(mink, n->m_) < 0))
        {
          if (auto const c(node::cmp(maxk, n->key()));
            (c > 0) || (eq && (c == 0)))
          {
            std::for_each(
              std::execution::unseq,
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

            if (c > 0)
            {
              f(f, n->r_.get());
            }
          }

          f(f, n->l_.get());
        }
      }
    );

    f(f, root_.get());
  }

  bool any(Key const& k) const noexcept
  {
    auto const [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    if (auto n(root_.get()); n && (node::cmp(mink, n->m_) < 0))
    {
      for (;;)
      {
        auto const c(node::cmp(maxk, n->key()));

        if ((c > 0) || (eq && (c == 0)))
        {
          if (auto const i(std::find_if(
                std::execution::unseq,
                n->v_.cbegin(),
                n->v_.cend(),
                [&](auto&& p) noexcept
                {
                  // mink < key_maximum
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

        if (auto const l(n->l_.get()); l && (node::cmp(mink, l->m_) < 0))
        {
          n = l;
        }
        else if (auto const r(n->r_.get());
          (c > 0) && r && (node::cmp(mink, r->m_) < 0))
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
