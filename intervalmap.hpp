#ifndef DS_SGITREE_HPP
# define DS_SGITREE_HPP
# pragma once

#include <cassert>

#include <algorithm>

#include <compare>

#include <memory>

#include <vector>

#include "sgutils.hpp"

namespace ds
{

template <typename Key, typename Value, class Comp = std::compare_three_way>
class sgitree
{
  static constexpr auto invoke_all(auto f, auto&& ...a) noexcept(noexcept(
    (f(std::forward<decltype(a)>(a)), ...)))
  {
    (f(std::forward<decltype(a)>(a)), ...);
  }

public:
  using size_type = std::size_t;

  struct node
  {
    static constinit inline auto const cmp{Comp{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    typename std::tuple_element_t<0, Key> const k_;
    typename std::tuple_element_t<1, Key> m_;

    std::vector<std::pair<decltype(m_), Value>> v_;

    explicit node(auto&& k, auto&& v):
      k_(std::get<0>(k)),
      m_(std::get<1>(k))
    {
      assert(std::get<0>(k) <= std::get<1>(k));
      v_.emplace_back(std::get<1>(k), std::forward<decltype(v)>(v));
    }

    //
    auto&& key() const noexcept { return k_; }

    //
    void emplace(auto&& k, auto&& v)
    {
      auto const f([&](auto&& f, auto& n) noexcept -> std::size_t
        {
          if (!n)
          {
            n.reset(
              new node(
                std::forward<decltype(k)>(k),
                std::forward<decltype(v)>(v)
              )
            );

            return 1;
          }
          else if (std::get<0>(k) == n->key())
          {
            n->m_ = std::max(n->m_, std::get<1>(k));
            n->v_.emplace_back(std::get<1>(k), std::forward<decltype(v)>(v));

            return ds::size(n.get());
          }

          //
          n->m_ = std::max(n->m_, std::get<1>(k));

          //
          std::size_t sl, sr;

          if (std::get<0>(k) < n->k_) // select left node
          {
            if (sl = f(f, n->l_); !sl)
            {
              return 0;
            }

            sr = ds::size(n->r_);
          }
          else
          {
            if (sr = f(f, n->r_); !sr)
            {
              return 0;
            }

            sl = ds::size(n->l_);
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ?
            (n.reset(n.release()->rebuild()), 0) :
            s;
        }
      );

      if (!f(f, root_))
      {
        node::reset_max(root_.get());
      }
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
          else if (d->k_ == n->k_)
          {
            n->m_ = std::max(n->m_, d->m_);
            std::move(d->v_.begin(), d->v_.end(), std::back_inserter(n->v_));

            return n->size();
          }

          // update n->m_
          n->m_ = std::max(n->m_, d->m_);

          // allocate new node, either to the left or right
          std::size_t sl, sr;

          if (d->k_ < n->k_) // select left node
          {
            if (sl = f(f, n->l_, d); !sl)
            {
              return 0;
            }

            sr = node::size(n->r_);
          }
          else
          {
            if (sr = f(f, n->r_, d); !sr)
            {
              return 0;
            }

            sl = node::size(n->l_);
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ?
            (n.reset(n.release()->rebuild()), 0) :
            s;
        }
      );

      if (auto const r(((d ? f(f, n, d) : ~std::size_t()) & ...)); !r)
      {
        node::reset_max(n.get());
      }
    }

    static void erase(Key const& k)
    {
      auto const [mink, maxk](k);

      node* p{};

      for (auto n(root_.get()); n;)
      {
        if (mink == n->k_)
        {
          std::erase_if(
            n->v_,
            [&](auto&& p) noexcept
            {
              return maxk == std::get<0>(p);
            }
          );

          if (n->v_.empty())
          {
            auto& q(!p ? root_ : n == p->l_.get() ? p->l_ : p->r_);

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

              node::move(root_, n->l_, n->r_);

              delete n;
            }

            if (p) // don't update max if root was changed
            {
              node::reset_max(root_.get());
            }
          }

          break;
        }
        else
        {
          p = n;
          n = mink < n->k_ ? n->l_.get() : n->r_.get();
        }
      }
    }

    static decltype(m_) reset_max(auto const n) noexcept
    {
      assert(n);
      auto m(n->k_);

      std::ranges::for_each(
        std::as_const(n->u_),
        [&](auto&& u) noexcept
        {
          m = std::max(m, u);
        }
      );

      invoke_all(
        [&](auto const n) noexcept
        {
          if (n)
          {
            m = std::max(m, reset_max(n));
          }
        },
        n->l_.get(),
        n->r_.get()
      );

      return n->m_ = m;
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

              break;

            case 1:
              {
                auto const p(l[b]);

                p->l_.release();
                p->r_.release();

                n->l_.release();
                n->r_.release();

                n->r_.reset(p);

                break;
              }

            default:
              assert(i);
              n->l_.release();
              n->l_.reset(f(f, a, i - 1));

              n->r_.release();
              n->r_.reset(f(f, i + 1, b));

              break;
          }

          return n;
        }
      );

      //
      auto const p(f(f, 0, l.size() - 1));

      reset_max(p);

      return p;
    }
  };

private:
  std::unique_ptr<node> root_;

public:
  auto& root() const noexcept { return root_.get(); }

  //
  void clear() { root_.reset(); }

  bool empty() const noexcept { return !size(); }
  
  auto size() const noexcept
  {
    static constinit auto const f(
      [](auto&& f, auto const& n) noexcept -> std::size_t
      {
        return n ? n->v_.size() + f(f, n->l_) + f(f, n->r_) : 0;
      }
    );

    return f(f, root_);
  }

  // iterators

  //
  auto all(Key const& k) const
  {
    std::vector<std::pair<Key, std::reference_wrapper<Value const>>> r;

    auto const [mink, maxk](k);
    auto const eq(mink == maxk);

    auto const f([&](auto&& f, auto const n) -> void
      {
        assert(n);
        auto&& key(n->key());

        if ((maxk > key) || (eq && (maxk == key)))
        {
          std::ranges::for_each(
            n->v_,
            [&](auto&& p)
            {
              if (auto const m(std::get<0>(p)); mink < m)
              {
                r.emplace_back(
                  std::pair(key, m),
                  std::cref(std::get<1>(p))
                );
              }
            }
          );
        }

        invoke_all(
          [&](auto const n)
          {
            if (n && (mink < n->m_))
            {
              f(f, n);
            }
          },
          n->l_.get(),
          n->r_.get()
        );
      }
    );

    if (root_ && (mink < root_->m_))
    {
      f(f, root_.get());
    }

    return r;
  }

  bool any(Key const& k) const noexcept
  {
    auto const [mink, maxk](k);
    auto const eq(mink == maxk);

    for (auto n(root_.get());;)
    {
      if (n)
      {
        if ((maxk > n->key()) || (eq && (maxk == n->key())))
        {
          if (auto const i(std::ranges::find_if(
                std::as_const(n->u_),
                [&](auto&& u) noexcept
                {
                  return mink < u;
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
        auto const l(n->l_.get());

        n = l && (mink < l->m_) ? l : n->r_.get();
      }
      else
      {
        return false;
      }
    }
  }
};

}

#endif // DS_SGITREE_HPP
