#ifndef SG_MULTISET_HPP
# define SG_MULTISET_HPP
# pragma once

#include <memory>

#include <list>
#include <vector>

#include "utils.hpp"

#include "intervalmapiterator.hpp"

namespace sg
{

template <typename Key, class Compare = std::compare_three_way>
class multiset
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

  using size_type = std::size_t;

  using const_iterator = intervalmapiterator<node const>;
  using iterator = intervalmapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = multiset::value_type;

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    std::list<value_type> v_;

    explicit node(auto&& k)
    {
      v_.emplace_back(std::forward<decltype(k)>(k));
    }

    //
    auto&& key() const noexcept { return v_.front(); }

    //
    static auto emplace(auto&& r, auto&& k)
    {
      node* q{};

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            n.reset(q = new node(std::forward<decltype(k)>(k)));

            return 1;
          }

          //
          size_type sl, sr;

          if (auto const c(cmp(k, n->key())); c < 0)
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
            (q = n.get())->v_.emplace_back(std::forward<decltype(k)>(k));

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

    static auto erase(auto& r, const_iterator const i)
    {
      auto const n(i.node());

      return 1 == n->v_.size() ?
        iterator{r.get(), std::get<0>(node::erase(r, n->key()))} :
        iterator{r.get(), n, n->v_.erase(i.iterator())};
    }

    static auto erase(auto& r, auto&& k)
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
            auto const s(n->v_.size());

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

            return std::tuple(nxt, s);
          }
        }
      }

      return std::tuple(pointer{}, size_type{});
    }

    auto rebuild()
    {
      std::vector<node*> l;

      {
        auto n(sg::detail::first_node(this));

        do
        {
          l.emplace_back(n);

          n = sg::detail::next(this, n);
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
      return f(f, 0, l.size() - 1);
    }
  };

private:
  std::unique_ptr<node> root_;

public:
  multiset() = default;
  multiset(std::initializer_list<value_type> i) { *this = i; }
  multiset(multiset const& o) { *this = o; }
  multiset(multiset&&) = default;
  multiset(auto const i, decltype(i) j) { insert(i, j); }

  //
  auto& operator=(auto&& o) requires(
    std::is_same_v<decltype(o), std::remove_cvref_t<multiset>> ||
    std::is_same_v<
      std::remove_cvref_t<decltype(o)>,
      std::initializer_list<value_type>
    >
  )
  {
    clear();

    std::for_each(
      std::execution::unseq,
      o.begin(),
      o.end(),
      [&](auto&& p)
      {
        emplace(p);
      }
    );

    return *this;
  }

  multiset& operator=(multiset&& o) noexcept = default;

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
      for (;;)
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
          return n->v_.size();
        }
      }
    }

    return {};
  }

  //
  auto emplace(auto&& k)
  {
    return iterator(root_.get(), node::emplace(root_,
      std::forward<decltype(k)>(k)));
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
    return iterator(root_.get(), node::emplace(root_, v.first));
  }

  auto insert(value_type&& v)
  {
    return iterator(root_.get(), node::emplace(root_, std::move(v)));
  }

  void insert(auto const i, decltype(i) j)
  {
    std::for_each(
      std::execution::unseq,
      i,
      j,
      [&](auto&& v)
      {
        emplace(v);
      }
    );
  }

  //
  friend bool operator!=(multiset const&, multiset const&) noexcept = default;
  friend bool operator<(multiset const&, multiset const&) noexcept = default;
  friend bool operator<=(multiset const&, multiset const&) noexcept = default;
  friend bool operator>(multiset const&, multiset const&) noexcept = default;
  friend bool operator>=(multiset const&, multiset const&) noexcept = default;
};

}

#endif // SG_MULTISET_HPP
