#ifndef SG_SET_HPP
# define SG_SET_HPP
# pragma once

#include "utils.hpp"

#include "mapiterator.hpp"

namespace sg
{

template <typename Key, class Compare = std::compare_three_way>
class set
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

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
    using value_type = set::value_type;

    static constinit inline Compare const cmp;

    node* l_{}, *r_{};
    Key const kv_;

    explicit node(auto&& ...a)
      noexcept(noexcept(Key(std::forward<decltype(a)>(a)...))):
      kv_(std::forward<decltype(a)>(a)...)
    {
    }

    ~node() noexcept(std::is_nothrow_destructible_v<decltype(kv_)>)
    {
      delete l_; delete r_;
    }

    //
    auto& key() const noexcept { return kv_; }

    //
    static auto emplace(auto& r, auto&& k)
      noexcept(noexcept(new node(std::forward<decltype(k)>(k))))
      requires(detail::Comparable<Compare, decltype(k), key_type>)
    {
      node* q;
      bool s{}; // success

      auto const f([&](auto&& f, auto& n)
        noexcept(noexcept(new node(std::forward<decltype(k)>(k)))) ->
        size_type
        {
          if (!n)
          {
            s = (n = q = new node(std::forward<decltype(k)>(k)));

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

          return (3 * sl > S) || (3 * sr > S) ? n = n->rebalance(s), 0 : s;
        }
      );

      f(f, r);

      return std::pair(q, s);
    }

    static auto emplace(auto& r, auto&& ...a)
      noexcept(noexcept(
          node::emplace(
            r,
            key_type(std::forward<decltype(a)>(a)...)
          )
        )
      )
      requires(std::is_constructible_v<key_type, decltype(a)...>)
    {
      return node::emplace(r, key_type(std::forward<decltype(a)>(a)...));
    }

    auto rebalance(size_type const sz) noexcept
    {
      auto const a(static_cast<node**>(SG_ALLOCA(sizeof(this) * sz)));
      auto b(a);

      {
        auto f([&](auto&& f, auto const n) noexcept -> void
          {
            if (n)
            {
              f(f, detail::left_node(n));

              *b++ = n;

              f(f, detail::right_node(n));
            }
          }
        );

        f(f, this);
      }

      //
      auto const f([](auto&& f, auto const a, decltype(a) b) noexcept -> node*
        {
          node* n;

          if (b == a)
          {
            n = *a;
            n->l_ = n->r_ = {};
          }
          else if (b == a + 1)
          {
            n = *a;
            auto const nb(n->r_ = *b);

            nb->l_ = nb->r_ = n->l_ = {};
          }
          else
          {
            auto const m(std::midpoint(a, b));
            n = *m;

            detail::assign(n->l_, n->r_)(f(f, a, m - 1), f(f, m + 1, b));
          }

          return n;
        }
      );

      //
      return f(f, a, b - 1);
    }
  };

private:
  using this_class = set;
  node* root_{};

public:
  set() = default;

  set(set const& o) 
    noexcept(noexcept(set(o.begin(), o.end())))
    requires(std::is_copy_constructible_v<value_type>):
    set(o.begin(), o.end())
  {
  }

  set(set&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  set(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
  {
    insert(i, j);
  }

  set(std::initializer_list<value_type> l)
    noexcept(noexcept(set(l.begin(), l.end()))):
    set(l.begin(), l.end())
  {
  }

  ~set() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_); }

  //
  template <int = 0>
  bool count(auto const& k) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    return detail::find(root_, k);
  }

  auto count(key_type const k) const noexcept { return count<0>(k); }

  //
  auto emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(a)>(a)...));

    return std::tuple(iterator(&root_, n), s);
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
    noexcept(noexcept(detail::erase(root_, *i)))
  {
    return {&root_, detail::erase(root_, *i)};
  }

  //
  template <int = 0>
  auto insert(auto&& k)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(k)>(k))))
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(k)>(k)));

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert(value_type k)
    noexcept(noexcept(insert<0>(std::move(k))))
  {
    return insert<0>(std::move(k));
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(emplace(*i)))
  {
    std::for_each(
      i,
      j,
      [&](auto&& v) noexcept(noexcept(emplace(std::forward<decltype(v)>(v))))
      {
        emplace(std::forward<decltype(v)>(v));
      }
    );
  }
};

//////////////////////////////////////////////////////////////////////////////
template <int = 0, typename K, class C>
inline auto erase(set<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(K(k))))
  requires(!detail::Comparable<C, decltype(k), K>)
{
  return c.erase(K(k));
}

template <int = 0, typename K, class C>
inline auto erase(set<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
  requires(detail::Comparable<C, decltype(k), K>)
{
  return c.erase(k);
}

template <typename K, class C>
inline auto erase(set<K, C>& c, K const k)
  noexcept(noexcept(erase<0>(c, k)))
{
  return erase<0>(c, k);
}

template <typename K, class C>
inline auto erase_if(set<K, C>& c, auto pred)
  noexcept(noexcept(pred(std::declval<K const&>()), c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n(); pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, class C>
inline void swap(set<K, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_SET_HPP
