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
      bool s{true};
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

            if (auto const i(std::find_if(
                std::execution::unseq,
                n->v_.cbegin(),
                n->v_.cend(),
                [&](auto&& p) noexcept
                {
                  return cmp(std::get<1>(std::get<0>(p)), maxk) == 0;
                }
              )
            ); n->v_.cend() == i)
            {
              n->v_.emplace_back(
                std::forward<decltype(k)>(k),
                std::forward<decltype(v)>(v)
              );
            }
            else
            {
              s = false;
            }

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

      return std::tuple(q, s);
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
            std::erase_if(
              n->v_,
              [&](auto&& p) noexcept
              {
                return maxk == std::get<1>(std::get<0>(p));
              }
            );

            if (n->v_.size())
            {
              return std::tuple(n, 1);
            }
            else
            {
              auto& q(!p ? r : p->l_.get() == n ? p->l_ : p->r_);

              auto const s(n->v_.size());
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

              return std::tuple(nxt, s);
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

    static decltype(m_) reset_max(auto const n) noexcept
    {
      auto m(n->k_);

      std::for_each(
        n->v_.cbegin(),
        n->v_.cend(),
        [&](auto&& p) noexcept
        {
          m = std::max(m, std::get<1>(std::get<0>(p)));
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

  //
  auto emplace(auto&& k, auto&& v)
  {
    auto const [n, s](
      node::emplace(root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      )
    );

    //return std::tuple(intervalmapiterator<node>(root_.get(), n), s);
  }

  //
  auto all(Key const& k) const
  {
    std::vector<std::pair<Key, std::reference_wrapper<Value const>>> r;

    auto const [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    auto const f([&](auto&& f, auto const n) -> void
      {
        assert(n);
        auto&& key(n->key());

        if (auto const c(node::cmp(maxk, key)); (c > 0) || (eq && (c == 0)))
        {
          std::for_each(
            std::execution::unseq,
            n->v_.cbegin(),
            n->v_.cend(),
            [&](auto&& p)
            {
              if (auto const pkey(std::get<0>(p));
                node::cmp(mink, std::get<1>(pkey)) < 0)
              {
                r.emplace_back(
                  pkey,
                  std::cref(std::get<1>(p))
                );
              }
            }
          );
        }

        invoke_all(
          [&](auto const n)
          {
            if (n && (node::cmp(mink, n->m_) < 0))
            {
              f(f, n);
            }
          },
          n->l_.get(),
          n->r_.get()
        );
      }
    );

    if (root_ && (node::cmp(mink, root_->m_) < 0))
    {
      f(f, root_.get());
    }

    return r;
  }

  bool any(Key const& k) const noexcept
  {
    auto const [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    for (auto n(root_.get());;)
    {
      if (n)
      {
        auto&& key(n->key());

        if (auto const c(node::cmp(maxk, key)); (c > 0) || (eq && (c == 0)))
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

        //
        auto const l(n->l_.get());
        n = l && (node::cmp(mink, l->m_) < 0) ? l : n->r_.get();
      }
      else
      {
        return false;
      }
    }
  }
};

}

#endif // SG_INTERVALMAP_HPP
