#ifndef SG_MAP_HPP
# define SG_MAP_HPP
# pragma once

#include <algorithm>
#include <execution>

#include <compare>

#include <memory>

#include <vector>

#include "utils.hpp"

#include "mapiterator.hpp"

namespace sg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class map
{
public:
  struct node;

  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key const, Value>;

  using size_type = std::size_t;

  using const_iterator = mapiterator<node const>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = map::value_type;

    struct empty_t{};

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    value_type kv_;

    explicit node(auto&& k):
      kv_(std::forward<decltype(k)>(k), Value())
    {
    }

    explicit node(auto&& k, auto&& v):
      kv_(std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v))
    {
    }

    //
    auto&& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto&& r, auto&& k, auto&& v)
    {
      bool s{true};
      node* q{};

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            if constexpr(!std::is_same_v<decltype(v), empty_t&&>)
            {
              n.reset(
                q = new node(
                  std::forward<decltype(k)>(k),
                  std::forward<decltype(v)>(v)
                )
              );
            }
            else
            {
              n.reset(q = new node(std::forward<decltype(k)>(k)));
            }

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
            q = n.get();
            s = false;

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
      return f(f, 0, l.size() - 1);
    }
  };

private:
  std::unique_ptr<node> root_;

public:
  map() = default;
  map(std::initializer_list<value_type> i) { *this = i; }
  map(map const& o) { *this = o; }
  map(map&&) = default;
  map(auto const i, decltype(i) j) { insert(i, j); }

  //
  auto& operator=(auto&& o) requires(
    std::is_same_v<decltype(o), std::remove_cvref_t<map>> ||
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

  auto& operator=(map&& o) noexcept
  {
    return root_ = std::move(o.root_), *this;
  }

  //
  auto root() const noexcept { return root_.get(); }

  //
  void clear() { root_.reset(); }
  auto empty() const noexcept { return !size(); }
  auto max_size() const noexcept { return ~size_type{} / 3; }
  auto size() const noexcept { return sg::detail::size(root_.get()); }

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
      const_iterator(root_.get(), node::first_node(root_.get())) :
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
  auto& operator[](Key const& k)
  {
    return std::get<0>(node::emplace(root_, k,
      typename node::empty_t()))->kv_.second;
  }

  auto& operator[](Key&& k)
  {
    return std::get<0>(node::emplace(root_, std::move(k),
      typename node::empty_t()))->kv_.second;
  }

  auto& at(Key const& k)
  {
    return std::get<1>(sg::detail::find(root_.get(), k)->kv_);
  }

  auto const& at(Key const& k) const
  {
    return std::get<1>(sg::detail::find(root_.get(), k)->kv_);
  }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(sg::detail::find(root_.get(), k));
  }

  //
  auto emplace(auto&& k, auto&& v)
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      )
    );

    return std::tuple(iterator(root_.get(), n), s);
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
  iterator erase(const_iterator const i)
  {
    return iterator(root_.get(), sg::detail::erase(root_, std::get<0>(*i)));
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
    return sg::detail::erase(root_, k) ? 1 : 0;
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
    auto const [n, s](
      node::emplace(root_, v.first, v.second)
    );

    return std::tuple(iterator(root_.get(), n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](
      node::emplace(root_, std::move(v.first), std::move(v.second))
    );

    return std::tuple(iterator(root_.get(), n), s);
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

#endif // SG_MAP_HPP
