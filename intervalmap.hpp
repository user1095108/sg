#ifndef SG_INTERVALMAP_HPP
# define SG_INTERVALMAP_HPP
# pragma once

#include <memory>

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

  using size_type = std::size_t;

  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = multimapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

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
      auto const& [mink, maxk](k);
      node* q;

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
        return {r.get(), std::get<0>(node::erase(r, std::get<0>(*i)))};
      }
      else if (auto const it(i.iterator()); std::prev(n->v_.end()) == it)
      {
        auto const nn(std::next(i).node());

        n->v_.erase(it);

        return {r.get(), nn};
      }
      else
      {
        return {r.get(), n, n->v_.erase(it)};
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
            auto const s0(n->v_.size());

            std::erase_if(
              n->v_,
              [&](auto&& p) noexcept
              {
                return cmp(std::get<0>(p), k) == 0;
              }
            );

            if (auto const s1(n->v_.size()); s1)
            {
              return std::tuple(n, s0 - s1);
            }
            else
            {
              auto& q(!p ? r : p->l_.get() == n ? p->l_ : p->r_);

              auto const nxt(sg::detail::next_node(r.get(), n));

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

              return std::tuple(nxt, s0);
            }
          }
        }
      }

      return std::tuple(pointer{}, size_type{});
    }

    static void move(auto& n, auto& ...d)
    {
      auto const f([&](auto&& f, auto& n, auto& d) noexcept -> std::size_t
        {
          if (!n)
          {
            n = std::move(d);

            return 1;
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
            std::execution::unseq,
            n->v_.cbegin(),
            n->v_.cend(),
            [&](auto&& p) noexcept
            {
              auto const tmp(std::get<1>(std::get<0>(p)));
              m = cmp(m, tmp) < 0 ? tmp : m;
            }
          );

          auto const l(n->l_.get()), r(n->r_.get());

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

      return f(f, n);
    }

    static decltype(node::m_) reset_nodes_max(auto const n,
      auto const ...c) noexcept
    {
      decltype(node::m_) m(n->key());

      std::for_each(
        std::execution::unseq,
        n->v_.cbegin(),
        n->v_.cend(),
        [&](auto&& p) noexcept
        {
          auto const tmp(std::get<1>(std::get<0>(p)));
          m = cmp(m, tmp) < 0 ? tmp : m;
        }
      );

      decltype(node::m_) tmp;
      ((tmp = reset_nodes_max(c), m = cmp(m, tmp) < 0 ? tmp : m), ...);

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
        while ((n = sg::detail::next_node(this, n)));
      }

      auto const f([&](auto&& f, auto const a, auto const b) noexcept -> node*
        {
          auto const i((a + b) / 2);
          auto const n(l[i]);

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
                auto m(reset_nodes_max(n));

                if (auto const p(f(f, a, i - 1)); p)
                {
                  m = cmp(m, p->m_) < 0 ? p->m_ : m;
                  n->l_.release();
                  n->l_.reset(p);
                }

                if (auto const p(f(f, i + 1, b)); p)
                {
                  m = cmp(m, p->m_) < 0 ? p->m_ : m;
                  n->r_.release();
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
            n = sg::detail::left_node(n);
          }
          else if (c > 0)
          {
            n = sg::detail::right_node(n);
          }
          else
          {
            return std::count(
              std::execution::unseq,
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
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_.get(), k));
    return std::pair(
      iterator(root_.get(), e ? e : g),
      iterator(root_.get(), g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_.get(), k));
    return std::pair(
      const_iterator(root_.get(), e ? e : g),
      const_iterator(root_.get(), g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_.get(), k));
    return std::pair(
      iterator(root_.get(), e ? e : g),
      iterator(root_.get(), g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_.get(), k));
    return std::pair(
      const_iterator(root_.get(), e ? e : g),
      const_iterator(root_.get(), g)
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
