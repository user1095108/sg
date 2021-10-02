#ifndef SG_MAP_HPP
# define SG_MAP_HPP
# pragma once

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

  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = map::value_type;

    struct empty_t{};

    static constinit inline auto const cmp{Compare{}};

    node* l_{}, *r_{};
    value_type kv_;

    explicit node(auto&& k) noexcept(noexcept(Key())):
      kv_(std::forward<decltype(k)>(k), Value())
    {
    }

    explicit node(auto&& k, auto&& v) noexcept(noexcept(Key(), Value())):
      kv_(std::forward<decltype(k)>(k), std::forward<decltype(v)>(v))
    {
    }

    ~node() noexcept(noexcept(std::declval<Key>().~Key(),
      std::declval<Value>().~Value()))
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto&& r, auto&& a, auto&& v)
    {
      node* q;
      bool s{}; // success

      key_type k(std::forward<decltype(a)>(a));

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            if constexpr(!std::is_same_v<decltype(v), empty_t&&>)
            {
              s = (n = q = new node(
                  std::move(k),
                  std::forward<decltype(v)>(v)
                )
              );
            }
            else
            {
              s = (n = q = new node(std::move(k)));
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
            q = n;

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(), 0) : s;
        }
      );

      f(f, r);

      return std::tuple(q, s);
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

              break;

            case 1:
              {
                auto const p(l[b]);

                p->l_ = p->r_ = n->l_ = {};
                n->r_ = p;

                break;
              }

            default:
              n->l_ = f(f, a, i - 1);
              n->r_ = f(f, i + 1, b);

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
  using this_class = map;
  node* root_{};

public:
  map() = default;
  map(std::initializer_list<value_type> i) { *this = i; }
  map(map const& o) { *this = o; }
  map(map&&) = default;
  map(std::input_iterator auto const i, decltype(i) j) { insert(i, j); }

  ~map() noexcept(noexcept(root_->~node())) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept { return sg::detail::size(root_); }

  //
  auto& operator[](Key const& k)
  {
    return std::get<1>(std::get<0>(node::emplace(root_, k,
      typename node::empty_t()))->kv_);
  }

  auto& operator[](Key&& k)
  {
    return std::get<1>(std::get<0>(node::emplace(root_, std::move(k),
      typename node::empty_t()))->kv_);
  }

  auto& at(Key const& k)
  {
    return std::get<1>(sg::detail::find(root_, k)->kv_);
  }

  auto const& at(Key const& k) const
  {
    return std::get<1>(sg::detail::find(root_, k)->kv_);
  }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(sg::detail::find(root_, k));
  }

  //
  auto emplace(auto&& ...a)
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(a)>(a)...));

    return std::tuple(iterator(root_, n), s);
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(iterator(root_, e ? e : g), iterator(root_, g));
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(const_iterator(root_, e ? e : g),
      const_iterator(root_, g));
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(iterator(root_, e ? e : g), iterator(root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(const_iterator(root_, e ? e : g),
      const_iterator(root_, g)
    );
  }

  //
  iterator erase(const_iterator const i)
  {
    return iterator(root_, sg::detail::erase(root_, std::get<0>(*i)));
  }

  size_type erase(Key const& k)
  {
    return sg::detail::erase(root_, k) ? 1 : 0;
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, s](node::emplace(root_, std::get<0>(v), std::get<1>(v)));

    return std::tuple(iterator(root_, n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](node::emplace(root_,
      std::get<0>(v), std::move(std::get<1>(v))));

    return std::tuple(iterator(root_, n), s);
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
  auto insert_or_assign(key_type const& k, auto&& v)
  {
    auto const [n, s](
      node::emplace(root_, k, std::forward<decltype(v)>(v))
    );

    if (!s)
    {
      std::get<1>(n->kv_) = std::forward<decltype(v)>(v);
    }

    return std::tuple(iterator(root_, n), s);
  }

  auto insert_or_assign(key_type&& k, auto&& v)
  {
    auto const [n, s](
      node::emplace(root_, std::move(k), std::forward<decltype(v)>(v))
    );

    if (!s)
    {
      std::get<1>(n->kv_) = std::forward<decltype(v)>(v);
    }

    return std::tuple(iterator(root_, n), s);
  }

  //
  friend bool operator!=(map const&, map const&) = default;
  friend bool operator<(map const&, map const&) = default;
  friend bool operator<=(map const&, map const&) = default;
  friend bool operator>(map const&, map const&) = default;
  friend bool operator>=(map const&, map const&) = default;
};

}

#endif // SG_MAP_HPP
