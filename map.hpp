#ifndef SG_MAP_HPP
# define SG_MAP_HPP
# pragma once

#include <memory>

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
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = map::value_type;

    struct empty_t{};

    static constinit inline Compare const cmp;

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
    static auto emplace(auto& r, auto&& a, auto&& v)
    {
      bool s{}; // success
      node* q;

      key_type k(std::forward<decltype(a)>(a));

      auto const f([&](auto&& f, auto& n) -> size_type
        {
          if (!n)
          {
            if constexpr(std::is_same_v<decltype(v), empty_t&&>)
            {
              s = (n = q = new node(std::move(k)));
            }
            else
            {
              s = (n = q = new node(
                  std::move(k),
                  std::forward<decltype(v)>(v)
                )
              );
            }

            return 1;
          }

          //
          size_type sl, sr;

          if (auto const c(cmp(k, n->key())); c < 0)
          {
            if (sl = f(f, n->l_); !sl)
            {
              return {};
            }

            sr = detail::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_); !sr)
            {
              return {};
            }

            sl = detail::size(n->l_);
          }
          else
          {
            q = n;

            return {};
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(s), 0) : s;
        }
      );

      f(f, r);

      return std::pair(q, s);
    }

    auto rebuild(size_type const sz)
    {
//    auto const l(std::make_unique<node*[]>(sz)); // good way
      decltype(this) vla[sz]; // bad way

      {
        auto f([l(&*vla)](auto&& f, auto const n) mutable noexcept -> void
          {
            if (n)
            {
              f(f, detail::left_node(n));

              *l++ = n;

              f(f, detail::right_node(n));
            }
          }
        );

        f(f, this);
      }

      auto const f([l(&*vla)](auto&& f,
        size_type const a, decltype(a) b) noexcept -> node*
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
                auto const nb(n->r_ = l[b]);

                nb->l_ = nb->r_ = n->l_ = {};

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
      return f(f, {}, sz - 1);
    }
  };

private:
  using this_class = map;
  node* root_{};

public:
  map() = default;

  map(std::initializer_list<value_type> il)
    noexcept(noexcept(*this = il))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = il;
  }

  map(map const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  map(map&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  map(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  ~map() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_); }

  //
  auto& operator[](Key const& k)
  {
    return std::get<1>(
      std::get<0>(
        node::emplace(root_, k, typename node::empty_t())
      )->kv_
    );
  }

  auto& operator[](Key&& k)
  {
    return std::get<1>(
      std::get<0>(
        node::emplace(root_, std::move(k), typename node::empty_t())
      )->kv_
    );
  }

  auto& at(Key const& k) noexcept
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  auto const& at(Key const& k) const noexcept
  {
    return std::get<1>(detail::find(root_, k)->kv_);
  }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(detail::find(root_, k));
  }

  //
  auto emplace(auto&& ...a)
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(a)>(a)...));
    return std::tuple(iterator(&root_, n), s);
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(iterator(&root_, e), iterator(&root_, g));
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(const_iterator(&root_, e), const_iterator(&root_, g));
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(iterator(&root_, e), iterator(&root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));
    return std::pair(const_iterator(&root_, e), const_iterator(&root_, g));
  }

  //
  iterator erase(const_iterator const i)
  {
    return {&root_, detail::erase(root_, std::get<0>(*i))};
  }

  size_type erase(Key const& k)
  {
    return bool(detail::erase(root_, k));
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n), s);
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

    return std::tuple(iterator(&root_, n), s);
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

    return std::tuple(iterator(&root_, n), s);
  }
};

}

#endif // SG_MAP_HPP
