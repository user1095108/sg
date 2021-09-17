#ifndef SG_MULTIMAP_HPP
# define SG_MULTIMAP_HPP
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
class multimap
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
    using value_type = multimap::value_type;

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    std::list<value_type> v_;

    explicit node(auto&& k, auto&& v)
    {
      v_.emplace_back(
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      );
    }

    //
    auto&& key() const noexcept { return std::get<0>(v_.front()); }

    //
    static auto emplace(auto&& r, auto&& k, auto&& v)
    {
      node* q{};

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
  multimap() = default;
  multimap(std::initializer_list<value_type> i) { *this = i; }
  multimap(multimap const& o) { *this = o; }
  multimap(multimap&&) = default;
  multimap(auto const i, decltype(i) j) { insert(i, j); }

  //
  auto& operator=(auto&& o) requires(
    std::is_same_v<decltype(o), std::remove_cvref_t<multimap>> ||
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
        emplace(std::get<0>(p), std::get<1>(p));
      }
    );

    return *this;
  }

  auto& operator=(multimap&& o) noexcept
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
      iterator(root_.get(), sg::detail::first_node(root_.get())) :
      iterator();
  }

  iterator end() noexcept { return {}; }

  // const iterators
  const_iterator begin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), sg::detail::first_node(root_.get())) :
      const_iterator();
  }

  const_iterator end() const noexcept { return {}; }

  const_iterator cbegin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), sg::detail::first_node(root_.get())) :
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
      reverse_iterator(
        iterator{root_.get(), sg::detail::first_node(root_.get())}
      ) :
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
        const_iterator{root_.get(), sg::detail::first_node(root_.get())}
      ) :
      const_reverse_iterator();
  }

  //
  bool contains(Key const& k) const
  {
    return bool(sg::detail::find(root_.get(), k));
  }

  bool contains(auto&& k) const
  {
    return bool(sg::detail::find(root_.get(), std::forward<decltype(k)>(k)));
  }

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

  auto erase(const_iterator a, const_iterator const b)
  {
    for (; a != b; a = erase(a));

    return a;
  }

  auto erase(std::initializer_list<const_iterator> const il)
  {
    iterator r;

    std::for_each(il.begin(), il.end(), [&](auto const i) { r = erase(i); });

    return r;
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(node::erase(root_, k));
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_.get(), k));
    return std::pair(
      iterator(root_.get(), e ? e : g),
      iterator(root_.get(), g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_.get(), k));
    return std::pair(
      const_iterator(root_.get(), e ? e : g),
      const_iterator(root_.get(), g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_.get(), k));
    return std::pair(
      iterator(root_.get(), e ? e : g),
      iterator(root_.get(), g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_.get(), k));
    return std::pair(
      const_iterator(root_.get(), e ? e : g),
      const_iterator(root_.get(), g)
    );
  }

  //
  auto find(Key const& k) noexcept
  {
    return iterator(root_.get(), sg::detail::find(root_.get(), k));
  }

  auto find(Key const& k) const noexcept
  {
    return const_iterator(root_.get(), sg::detail::find(root_.get(), k));
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
    insert(il.begin(), il.end());
  }

  //
  iterator lower_bound(Key const& k) noexcept
  {
    auto const& [e, g](sg::detail::equal_range(root_.get(), k));
    return {root_.get(), e ? e : g};
  }

  const_iterator lower_bound(Key const& k) const noexcept
  {
    auto const& [e, g](sg::detail::equal_range(root_.get(), k));
    return {root_.get(), e ? e : g};
  }

  iterator lower_bound(auto const& k) noexcept
  {
    auto const& [e, g](sg::detail::equal_range(root_.get(), k));
    return {root_.get(), e ? e : g};
  }

  const_iterator lower_bound(auto const& k) const noexcept
  {
    auto const& [e, g](sg::detail::equal_range(root_.get(), k));
    return {root_.get(), e ? e : g};
  }

  //
  iterator upper_bound(Key const& k) noexcept
  {
    return {root_.get(),
      std::get<1>(sg::detail::equal_range(root_.get(), k))};
  }

  const_iterator upper_bound(Key const& k) const noexcept
  {
    return {root_.get(),
      std::get<1>(sg::detail::equal_range(root_.get(), k))};
  }

  iterator upper_bound(auto const& k) noexcept
  {
    return {root_.get(),
      std::get<1>(sg::detail::equal_range(root_.get(), k))};
  }

  const_iterator upper_bound(auto const& k) const noexcept
  {
    return {root_.get(),
      std::get<1>(sg::detail::equal_range(root_.get(), k))};
  }
};

}

#endif // SG_MULTIMAP_HPP
