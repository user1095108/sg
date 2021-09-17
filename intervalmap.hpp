#ifndef SG_INTERVALMAP_HPP
# define SG_INTERVALMAP_HPP
# pragma once

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
public:
  struct node;

  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

  using size_type = std::size_t;

  using const_iterator = intervalmapiterator<node const>;
  using iterator = intervalmapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = intervalmap::value_type;

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    typename std::tuple_element_t<1, Key> m_;

    std::list<value_type> v_;

    explicit node(auto&& k, auto&& v):
      m_(std::get<1>(k))
    {
      assert(std::get<0>(k) <= std::get<1>(k));
      v_.emplace_back(
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      );
    }

    //
    auto&& key() const noexcept
    {
      return std::get<0>(std::get<0>(v_.front()));
    }

    //
    static auto emplace(auto&& r, auto&& k, auto&& v)
    {
      node* q{};

      auto const& [mink, maxk](k);

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
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
            (q = n.get())->v_.emplace_back(
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

    static auto equal_range(auto n, auto&& k) noexcept
    {
      using node = std::remove_const_t<std::remove_pointer_t<decltype(n)>>;

      decltype(n) g{};

      if (auto& mink(std::get<0>(k)); n)
      {
        do
        {
          if (auto const c(node::cmp(mink, n->key())); c < 0)
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
            if (auto const r(n->r_.get()); !g && r)
            {
              g = sg::detail::first_node(r);
            }

            break;
          }
        }
        while (n);
      }

      return std::tuple(n, g);
    }

    static auto erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.node()); 1 == n->v_.size())
      {
        return iterator{r.get(), std::get<0>(node::erase(r, n->key()))};
      }
      else if (auto const k(i.iterator()); std::prev(n->v_.end()) == k)
      {
        auto const j(std::next(i));

        n->v_.erase(k);

        return j;
      }
      else
      {
        return iterator{r.get(), n, n->v_.erase(k)};
      }
    }

    static auto erase(auto& r, auto&& k)
    {
      using pointer = typename std::remove_cvref_t<decltype(r)>::pointer;
      using node = std::remove_pointer_t<pointer>;

      if (auto n(r.get()); n)
      {
        auto const& [mink, maxk](k);

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
            auto const s(n->v_.size());

            auto& q(!p ? r : p->l_.get() == n ? p->l_ : p->r_);

            auto const nxt(next(r.get(), n));

            if (!n->l_ && !n->r_)
            {
              q.reset();

              if (p)
              {
                node::reset_max(r.get(), p);
              }
            }
            else if (!n->l_ || !n->r_)
            {
              q = std::move(n->l_ ? n->l_ : n->r_);

              if (p)
              {
                node::reset_max(r.get(), q.get());
              }
            }
            else
            {
              q.release(); // order important

              if (p)
              {
                node::reset_max(r.get(), p);
              }

              node::move(r, n->l_, n->r_);

              delete n;
            }

            return std::tuple(nxt, s);
          }
        }
      }

      return std::tuple(pointer{}, size_type{});
    }

    static void move(auto& n, auto& ...d)
    {
      auto const f([&](auto&& f, auto& n, auto& d) noexcept -> std::size_t
        {
          assert(d);
          if (!n)
          {
            n = std::move(d);

            return 1;
          }

          //
          n->m_ = std::max(n->m_, d->m_);

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

          return (3 * sl > S) || (3 * sr > S) ?
            (n.reset(n.release()->rebuild()), 0) :
            s;
        }
      );

      (f(f, n, d), ...);
    }

    static decltype(node::m_) reset_max(auto const n, auto const p) noexcept
    {
      auto&& key(p->key());

      auto const f([&](auto&& f, auto const n) noexcept -> decltype(node::m_)
        {
          decltype(node::m_) m(n->key());

          std::for_each(
            n->v_.cbegin(),
            n->v_.cend(),
            [&](auto&& p) noexcept
            {
              m = std::max(m, std::get<1>(std::get<0>(p)));
            }
          );

          auto const l(n->l_.get()), r(n->r_.get());

          if (auto const c(cmp(key, n->key())); c < 0)
          {
            if (r)
            {
              m = std::max(m, r->m_);
            }

            m = std::max(m, f(f, l)); // visit left
          }
          else if (c > 0)
          {
            if (l)
            {
              m = std::max(m, l->m_);
            }

            m = std::max(m, f(f, r)); // visit right
          }
          else // we are there
          {
            if (l)
            {
              m = std::max(m, l->m_);
            }

            if (r)
            {
              m = std::max(m, r->m_);
            }
          }

          return n->m_ = m;
        }
      );

      return f(f, n);
    }

    static decltype(node::m_) reset_nodes_max(auto const n,
      auto const ...c) noexcept
    {
      decltype(node::m_) m(n->key());

      std::for_each(
        n->v_.cbegin(),
        n->v_.cend(),
        [&](auto&& p) noexcept
        {
          m = std::max(m, std::get<1>(std::get<0>(p)));
        }
      );

      m = std::max({m, reset_nodes_max(c)...});

      return n->m_ = m;
    }

    auto rebuild()
    {
      std::vector<node*> l;

      {
        auto n(sg::detail::first_node(this));

        do
        {
          l.emplace_back(n);
        }
        while ((n = sg::detail::next(this, n)));
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

              reset_nodes_max(n);

              break;

            case 1:
              {
                auto const p(l[b]);

                p->l_.release();
                p->r_.release();

                n->l_.release();
                n->r_.release();

                n->r_.reset(p);

                reset_nodes_max(n, n->r_.get());

                break;
              }

            default:
              {
                assert(i);
                n->l_.release();
                n->r_.release();

                auto m(reset_nodes_max(n));

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

private:
  using this_class = intervalmap;
  std::unique_ptr<node> root_;

public:
  intervalmap() = default;
  intervalmap(std::initializer_list<value_type> i) { *this = i; }
  intervalmap(intervalmap const& o) { *this = o; }
  intervalmap(intervalmap&&) = default;
  intervalmap(std::input_iterator auto const i, decltype(i) j){insert(i, j);}

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
    if (auto n(root_.get()); n)
    {
      for (auto const& [mink, maxk](k);;)
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
            return n->v_.size();
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
    return node::erase(root_, i);
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(node::erase(root_, k));
  }

  //
  auto insert(value_type const& v)
  {
    return iterator(root_.get(), node::emplace(root_,
      std::get<0>(v), std::get<1>(v)));
  }

  auto insert(value_type&& v)
  {
    return iterator(root_.get(), node::emplace(root_,
      std::move(std::get<0>(v)), std::move(std::get<1>(v))));
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
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

  //
  friend bool operator!=(intervalmap const&,
    intervalmap const&) noexcept = default;
  friend bool operator<(intervalmap const&,
    intervalmap const&) noexcept = default;
  friend bool operator<=(intervalmap const&,
    intervalmap const&) noexcept = default;
  friend bool operator>(intervalmap const&,
    intervalmap const&) noexcept = default;
  friend bool operator>=(intervalmap const&,
    intervalmap const&) noexcept = default;

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

            if (cg0)
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
    auto const& [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    if (auto n(root_.get()); n && (node::cmp(mink, n->m_) < 0))
    {
      for (;;)
      {
        auto const c(node::cmp(maxk, n->key()));
        auto const cg0(c > 0);

        if (cg0 || (eq && (c == 0)))
        {
          if (auto const i(std::find_if(
                std::execution::unseq,
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
        if (auto const l(n->l_.get()); l && (node::cmp(mink, l->m_) < 0))
        {
          n = l;
        }
        else if (auto const r(n->r_.get());
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
