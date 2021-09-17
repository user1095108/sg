#ifndef SG_MAP_HPP
# define SG_MAP_HPP
# pragma once

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
  using this_class = map;
  std::unique_ptr<node> root_;

public:
  map() = default;
  map(std::initializer_list<value_type> i) { *this = i; }
  map(map const& o) { *this = o; }
  map(map&&) = default;
  map(std::input_iterator auto const i, decltype(i) j) { insert(i, j); }

# include "common.hpp"

  //
  auto size() const noexcept { return sg::detail::size(root_.get()); }

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
  iterator erase(const_iterator const i)
  {
    return iterator(root_.get(), sg::detail::erase(root_, std::get<0>(*i)));
  }

  size_type erase(Key const& k)
  {
    return sg::detail::erase(root_, k) ? 1 : 0;
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, s](node::emplace(root_, std::get<0>(v), std::get<1>(v)));

    return std::tuple(iterator(root_.get(), n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](node::emplace(root_,
      std::move(std::get<0>(v)), std::move(std::get<1>(v)))
    );

    return std::tuple(iterator(root_.get(), n), s);
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
  friend bool operator!=(map const&, map const&) noexcept = default;
  friend bool operator<(map const&, map const&) noexcept = default;
  friend bool operator<=(map const&, map const&) noexcept = default;
  friend bool operator>(map const&, map const&) noexcept = default;
  friend bool operator>=(map const&, map const&) noexcept = default;
};

}

#endif // SG_MAP_HPP
