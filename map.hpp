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

  using difference_type = detail::difference_type;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

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

    explicit node(auto&& a, auto&& ...b)
      noexcept(noexcept(
          value_type(
            std::piecewise_construct_t{},
            std::forward_as_tuple(std::forward<decltype(a)>(a)),
            std::forward_as_tuple(std::forward<decltype(b)>(b)...)
          )
        )
      ):
      kv_(
        std::piecewise_construct_t{},
        std::forward_as_tuple(std::forward<decltype(a)>(a)),
        std::forward_as_tuple(std::forward<decltype(b)>(b)...)
      )
    {
    }

    ~node() noexcept(std::is_nothrow_destructible_v<decltype(kv_)>)
    {
      delete l_; delete r_;
    }

    //
    auto& key() const noexcept { return std::get<0>(kv_); }

    //
    template <int = 0>
    static auto emplace(auto& r, auto&& k, auto&& ...a)
      noexcept(noexcept(
          new node(
            std::forward<decltype(k)>(k),
            std::forward<decltype(a)>(a)...
          )
        )
      )
      requires(detail::Comparable<Compare, decltype(k), key_type>)
    {
      bool s{}; // success
      node* q;

      auto const f([&](auto&& f, auto& n)
        noexcept(noexcept(
            new node(
              std::forward<decltype(k)>(k),
              std::forward<decltype(a)>(a)...
            )
          )
        ) -> size_type
        {
          if (!n)
          {
            s = (n = q = new node(
                  std::forward<decltype(k)>(k),
                  std::forward<decltype(a)>(a)...
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

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebalance(s), 0) : s;
        }
      );

      f(f, r);

      return std::pair(q, s);
    }

    auto rebalance(size_type const sz) noexcept
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

  map(map const& o)
    noexcept(noexcept(map(o.begin(), o.end())))
    requires(std::is_copy_constructible_v<value_type>):
    map(o.begin(), o.end())
  {
  }

  map(map&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  map(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
  {
    insert(i, j);
  }

  map(std::initializer_list<value_type> l)
    noexcept(noexcept(map(l.begin(), l.end()))):
    map(l.begin(), l.end())
  {
  }

  ~map() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_); }

  //
  template <int = 0>
  auto& operator[](auto&& k)
    noexcept(noexcept(
        node::emplace(root_, std::forward<decltype(k)>(k),
          typename node::empty_t())
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return std::get<1>(
        std::get<0>(
          node::emplace(root_, std::forward<decltype(k)>(k),
            typename node::empty_t())
        )->kv_
      );
  }

  auto& operator[](key_type k)
    noexcept(noexcept(operator[]<0>(std::move(k))))
  {
    return operator[]<0>(std::move(k));
  }

  template <int = 0>
  auto& at(auto const& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return detail::find(root_, k)->kv_;
  }

  auto& at(key_type const k) noexcept { return at<0>(k); }

  template <int = 0>
  auto const& at(auto const& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return detail::find(root_, k)->kv_;
  }

  auto& at(key_type const k) const noexcept { return at<0>(k); }

  //
  template <int = 0>
  size_type count(auto const& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return bool(detail::find(root_, k));
  }

  auto count(key_type const k) const noexcept { return count<0>(k); }

  //
  template <int = 0>
  auto emplace(auto&& k, auto&& ...a)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(a)>(a)...
      )
    );

    return std::tuple(iterator(&root_, n), s);
  }

  auto emplace(key_type k, auto&& ...a)
    noexcept(noexcept(
        emplace<0>(std::move(k), std::forward<decltype(a)>(a)...)
      )
    )
  {
    return emplace<0>(std::move(k), std::forward<decltype(a)>(a)...);
  }

  //
  template <int = 0>
  auto equal_range(auto const& k) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(key_type const k) noexcept { return equal_range<0>(k); }

  template <int = 0>
  auto equal_range(auto const& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  auto equal_range(key_type const k) const noexcept
  {
    return equal_range<0>(k);
  }

  //
  template <int = 0>
  size_type erase(auto const& k)
    noexcept(noexcept(detail::erase(root_, k)))
    requires(detail::Comparable<Compare, decltype(k), key_type> &&
      !std::convertible_to<decltype(k), const_iterator>)
  {
    return bool(detail::erase(root_, k));
  }

  auto erase(key_type const k) noexcept(noexcept(erase<0>(k)))
  {
    return erase<0>(k);
  }

  iterator erase(const_iterator const i)
    noexcept(noexcept(detail::erase(root_, std::get<0>(*i))))
  {
    return {&root_, detail::erase(root_, std::get<0>(*i))};
  }

  //
  template <int = 0>
  auto insert(auto&& v)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::get<0>(std::forward<decltype(v)>(v)),
          std::get<1>(std::forward<decltype(v)>(v))
        )
      )
    )
    requires(
      detail::Comparable<
        Compare,
        decltype(std::get<0>(std::forward<decltype(v)>(v))),
        key_type
      >
    )
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::get<0>(std::forward<decltype(v)>(v)),
        std::get<1>(std::forward<decltype(v)>(v))
      )
    );

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert(value_type v)
    noexcept(noexcept(insert<0>(std::move(v))))
  {
    return insert<0>(std::move(v));
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(emplace(std::get<0>(*i), std::get<1>(*i))))
  {
    std::for_each(
      i,
      j,
      [&](auto&& v)
        noexcept(noexcept(
            emplace(
              std::get<0>(std::forward<decltype(v)>(v)),
              std::get<1>(std::forward<decltype(v)>(v))
            )
          )
        )
      {
        emplace(
          std::get<0>(std::forward<decltype(v)>(v)),
          std::get<1>(std::forward<decltype(v)>(v))
        );
      }
    );
  }

  //
  template <int = 0>
  auto insert_or_assign(auto&& k, auto&& ...b)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::forward<decltype(k)>(k),
          std::forward<decltype(b)>(b)...
        )
      )
    )
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, s](
      node::emplace(
        root_,
        std::forward<decltype(k)>(k),
        std::forward<decltype(b)>(b)...
      )
    );

    if (!s)
    {
      if constexpr(sizeof...(b))
      {
        std::get<1>(n->kv_) = mapped_type(std::forward<decltype(b)>(b)...);
      }
      else
      {
        std::get<1>(n->kv_) = (std::forward<decltype(b)>(b), ...);
      }
    }

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert_or_assign(key_type k, auto&& ...b)
    noexcept(noexcept(
        insert_or_assign<0>(std::move(k), std::forward<decltype(b)>(b)...)
      )
    )
  {
    return insert_or_assign<0>(std::move(k), std::forward<decltype(b)>(b)...);
  }
};

//////////////////////////////////////////////////////////////////////////////
template <int = 0, typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
  requires(detail::Comparable<C, decltype(k), K>)
{
  return c.erase(k);
}

template <int = 0, typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, auto const& k)
  noexcept(noexcept(c.erase(K(k))))
  requires(!detail::Comparable<C, decltype(k), K>)
{
  return c.erase(K(k));
}

template <typename K, typename V, class C>
inline auto erase(map<K, V, C>& c, K const k)
  noexcept(noexcept(erase<0>(c, k)))
{
  return erase<0>(c, k);
}

template <typename K, typename V, class C>
inline auto erase_if(map<K, V, C>& c, auto pred)
  noexcept(noexcept(pred(std::declval<K const&>()), c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n(); pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline void swap(map<K, V, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_MAP_HPP
