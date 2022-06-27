#ifndef SG_MAP_HPP
# define SG_MAP_HPP
# pragma once

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
  using size_type = detail::size_type;
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

    explicit node(auto&& k, empty_t&&)
      noexcept(noexcept(value_type(std::forward<decltype(k)>(k), Value()))):
      kv_(std::forward<decltype(k)>(k), Value())
    {
    }

    explicit node(auto&& k, auto&& ...a)
      noexcept(noexcept(
          value_type(
            std::piecewise_construct_t{},
            std::forward_as_tuple(std::forward<decltype(k)>(k)),
            std::forward_as_tuple(std::forward<decltype(a)>(a)...)
          )
        )
      ):
      kv_(
        std::piecewise_construct_t{},
        std::forward_as_tuple(std::forward<decltype(k)>(k)),
        std::forward_as_tuple(std::forward<decltype(a)>(a)...)
      )
    {
    }

    ~node() noexcept(std::is_nothrow_destructible_v<decltype(kv_)>)
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept { return std::get<0>(kv_); }

    //
    static auto emplace(auto& r, auto&& a, auto&& ...v)
      noexcept(noexcept(
          new node(
            key_type(std::forward<decltype(a)>(a)),
            std::forward<decltype(v)>(v)...
          )
        )
      )
    {
      bool s{}; // success
      node* q;

      key_type k(std::forward<decltype(a)>(a));

      auto const f([&](auto&& f, auto& n)
        noexcept(noexcept(
            new node(
              std::move(k),
              std::forward<decltype(v)>(v)...
            )
          )
        ) -> size_type
        {
          if (!n)
          {
            s = (n = q = new node(
                std::move(k),
                std::forward<decltype(v)>(v)...
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

    auto rebuild(size_type const sz) noexcept
    {
      auto const l(static_cast<node**>(SG_ALLOCA(sizeof(this) * sz)));

      {
        auto f([l(l)](auto&& f, auto const n) mutable noexcept -> void
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

      auto const f([l](auto&& f,
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
              detail::assign(n->l_, n->r_)(f(f, a, i - 1), f(f, i + 1, b));
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

  map(std::initializer_list<value_type> l)
    noexcept(noexcept(*this = l))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = l;
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
    noexcept(noexcept(node::emplace(root_, k, typename node::empty_t())))
  {
    return std::get<1>(
      std::get<0>(
        node::emplace(root_, k, typename node::empty_t())
      )->kv_
    );
  }

  auto& operator[](Key&& k)
    noexcept(noexcept(
        node::emplace(root_, std::move(k), typename node::empty_t())
      )
    )
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
  size_type count(auto&& k, char = {}) const noexcept
    requires(
      std::three_way_comparable_with<
        key_type,
        std::remove_cvref_t<decltype(k)>
      >
    )
  {
    return bool(detail::find(root_, k));
  }

  size_type count(key_type const& k) const noexcept { return count(k, {}); }

  //
  auto emplace(Key&& k, auto&& ...a)
    noexcept(
      noexcept(
        node::emplace(
          root_,
          std::move(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::move(k),
        std::forward<decltype(a)>(a)...
      )
    );

    return std::tuple(iterator(&root_, n), s);
  }

  auto emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(a)>(a)...));

    return std::tuple(iterator(&root_, n), s);
  }

  //
  auto equal_range(auto&& k, char = {}) noexcept
    requires(
      std::three_way_comparable_with<
        key_type,
        std::remove_cvref_t<decltype(k)>
      >
    )
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(auto&& k, char = {}) const noexcept
    requires(
      std::three_way_comparable_with<
        key_type,
        std::remove_cvref_t<decltype(k)>
      >
    )
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  auto equal_range(key_type const& k) noexcept
  {
    return equal_range(k, {});
  }

  auto equal_range(key_type const& k) const noexcept
  {
    return equal_range(k, {});
  }

  //
  iterator erase(const_iterator const i)
    noexcept(noexcept(detail::erase(root_, std::get<0>(*i))))
  {
    return {&root_, detail::erase(root_, std::get<0>(*i))};
  }

  size_type erase(auto&& k, char = {})
    noexcept(noexcept(detail::erase(root_, k)))
    requires(
      std::three_way_comparable_with<
        key_type,
        std::remove_cvref_t<decltype(k)>
      >
    )
  {
    return bool(detail::erase(root_, k));
  }

  size_type erase(key_type const& k) noexcept(noexcept(erase(k, {})))
  {
    return erase(k, {});
  }

  //
  auto insert(value_type const& v)
    noexcept(noexcept(node::emplace(root_, std::get<0>(v), std::get<1>(v))))
  {
    auto const [n, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert(value_type&& v)
    noexcept(noexcept(node::emplace(root_, std::get<0>(v), std::get<1>(v))))
  {
    auto const [n, s](
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    );

    return std::tuple(iterator(&root_, n), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(emplace(std::get<0>(*i), std::get<1>(*i))))
  {
    std::for_each(
      i,
      j,
      [&](auto&& v) noexcept(noexcept(
          emplace(std::get<0>(v), std::get<1>(v))
        )
      )
      {
        emplace(std::get<0>(v), std::get<1>(v));
      }
    );
  }

  //
  auto insert_or_assign(auto&& k, auto&& v)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(v)>(v)
        )
      )
    )
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      )
    );

    if (!s)
    {
      std::get<1>(n->kv_) = std::forward<decltype(v)>(v);
    }

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert_or_assign(key_type&& k, auto&& v)
    noexcept(noexcept(
        node::emplace(root_, std::move(k), std::forward<decltype(v)>(v))
      )
    )
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

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
{
  return c.erase(k);
}

template <typename K, typename V, class C>
inline auto erase_if(map<K, V, C>& c, auto pred)
  noexcept(noexcept(c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n();)
  {
    i = pred(*i) ? ++r, c.erase(i) : std::next(i);
  }

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline void swap(map<K, V, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_MAP_HPP
