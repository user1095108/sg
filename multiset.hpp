#ifndef SG_MULTISET_HPP
# define SG_MULTISET_HPP
# pragma once

#include "utils.hpp"

#include "multimapiterator.hpp"

namespace sg
{

template <typename Key, class Compare = std::compare_three_way>
class multiset
{
public:
  struct node;

  using key_type = Key;
  using value_type = Key;

  using difference_type = detail::difference_type;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using iterator = multimapiterator<node const>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = multiset::value_type;

    static constinit inline Compare const cmp;

    node* l_{}, *r_{};
    std::list<value_type> v_;

    explicit node(auto&& k)
      noexcept(noexcept(v_.emplace_back(std::forward<decltype(k)>(k))))
    {
      v_.emplace_back(std::forward<decltype(k)>(k));
    }

    ~node() noexcept(std::is_nothrow_destructible_v<decltype(v_)>)
    {
      delete l_; delete r_;
    }

    //
    auto& key() const noexcept { return v_.front(); }

    //
    static auto emplace(auto& r, auto&& k)
      noexcept(noexcept(new node(std::forward<decltype(k)>(k))))
      requires(detail::Comparable<Compare, decltype(k), key_type>)
    {
      auto const [q, s](detail::emplace(r, k, [&]()
          noexcept(noexcept(new node(std::forward<decltype(k)>(k))))
          {
            return new node(std::forward<decltype(k)>(k));
          }
        )
      );

      if (!s) q->v_.emplace_back(std::forward<decltype(k)>(k));

      return q;
    }

    static auto emplace(auto& r, auto&& ...a)
      noexcept(noexcept(node::emplace(r, std::forward<decltype(a)>(a)...)))
      requires(std::is_constructible_v<key_type, decltype(a)...>)
    {
      return node::emplace(r, std::forward<decltype(a)>(a)...);
    }

    static iterator erase(auto& r0, const_iterator const i)
      noexcept(noexcept(std::declval<node>().v_.erase(i.i()),
        node::erase(r0, i.n(), i.p())))
    {
      if (auto const n(i.n()); 1 == n->v_.size())
      {
        return {&r0, std::get<0>(node::erase(r0, n->key()))};
      }
      else if (auto const it(i.iterator()); std::next(it) == n->v_.end())
      {
        auto const nn(std::next(i).n());

        n->v_.erase(it);

        return {&r0, nn};
      }
      else
      {
        return {&r0, n, n->v_.erase(it)};
      }
    }

    static auto erase(auto& r0, auto const& k)
    {
      using pointer = std::remove_cvref_t<decltype(r0)>;
      using node = std::remove_pointer_t<pointer>;

      if (r0)
      {
        for (auto q(&r0);;)
        {
          auto const n(*q);

          if (auto const c(node::cmp(k, n->key())); c < 0)
          {
            q = &n->l_;
          }
          else if (c > 0)
          {
            q = &n->r_;
          }
          else
          {
            auto const nxt(detail::next_node(r0, n));
            size_type const s(n->v_.size());

            if (auto const l(n->l_), r(n->r_); l && r)
            {
              if (detail::size(l) < detail::size(r))
              {
                auto const [fnn, fnp](detail::first_node2(r, n));

                *q = fnn;
                fnn->l_ = l;

                if (r != fnn)
                {
                  fnp->l_ = fnn->r_;
                  fnn->r_ = r;
                }
              }
              else
              {
                auto const [lnn, lnp](detail::last_node2(l, n));

                *q = lnn;
                lnn->r_ = r;

                if (l != lnn)
                {
                  lnp->r_ = lnn->l_;
                  lnn->l_ = l;
                }
              }
            }
            else
            {
              *q = l ? l : r;
            }

            n->l_ = n->r_ = {};
            delete n;

            return std::pair(nxt, s);
          }
        }
      }

      return std::pair(pointer{}, size_type{});
    }
  };

private:
  using this_class = multiset;
  node* root_{};

public:
  multiset() = default;

  multiset(multiset const& o)
    noexcept(noexcept(insert(o.begin(), o.end())))
    requires(std::is_copy_constructible_v<value_type>)
  {
    insert(o.begin(), o.end());
  }

  multiset(multiset&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  multiset(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
  {
    insert(i, j);
  }

  multiset(std::initializer_list<value_type> l)
    noexcept(noexcept(insert(l.begin(), l.end())))
  {
    insert(l.begin(), l.end());
  }

  ~multiset() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept
  {
    static constinit auto const f(
      [](auto&& f, auto const n) noexcept -> size_type
      {
        return n ? n->v_.size() + f(f, n->l_) + f(f, n->r_) : 0;
      }
    );

    return f(f, root_);
  }

  //
  template <int = 0>
  size_type count(auto const& k) const noexcept
    requires(detail::Comparable<Compare, key_type, decltype(k)>)
  {
    if (auto n(root_); n)
    {
      for (;;)
      {
        if (auto const c(node::cmp(k, n->key())); c < 0)
        {
          n = detail::left_node(n);
        }
        else if (c > 0)
        {
          n = detail::right_node(n);
        }
        else
        {
          return n->v_.size();
        }
      }
    }

    return {};
  }

  auto count(key_type const k) const noexcept { return count<0>(k); }

  //
  iterator emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    return {&root_, node::emplace(root_, std::forward<decltype(a)>(a)...)};
  }

  //
  template <int = 0>
  auto equal_range(auto&& k) noexcept
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(key_type k) noexcept
  {
    return equal_range<0>(std::move(k));
  }

  template <int = 0>
  auto equal_range(auto&& k) const noexcept
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  auto equal_range(key_type k) const noexcept
  {
    return equal_range<0>(std::move(k));
  }

  //
  template <int = 0>
  size_type erase(auto&& k)
    noexcept(noexcept(node::erase(root_, k)))
    requires(!std::convertible_to<decltype(k), const_iterator>)
  {
    return std::get<1>(node::erase(root_, k));
  }

  auto erase(key_type k) noexcept(noexcept(erase<0>(std::move(k))))
  {
    return erase<0>(std::move(k));
  }

  iterator erase(const_iterator const i)
    noexcept(noexcept(node::erase(root_, i)))
  {
    return node::erase(root_, i);
  }

  //
  iterator insert(value_type const& v)
    noexcept(noexcept(node::emplace(root_, v)))
  {
    return {&root_, node::emplace(root_, v)};
  }

  iterator insert(value_type&& v)
    noexcept(noexcept(node::emplace(root_, std::move(v))))
  {
    return {&root_, node::emplace(root_, std::move(v))};
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
inline auto erase(multiset<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(K(k))))
  requires(!detail::Comparable<C, decltype(k), K>)
{
  return c.erase(K(k));
}

template <int = 0, typename K, class C>
inline auto erase(multiset<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
  requires(detail::Comparable<C, decltype(k), K>)
{
  return c.erase(k);
}

template <typename K, class C>
inline auto erase(multiset<K, C>& c, K const k)
  noexcept(noexcept(erase<0>(c, k)))
{
  return erase<0>(c, k);
}

template <typename K, class C>
inline auto erase_if(multiset<K, C>& c, auto pred)
  noexcept(noexcept(c.erase(c.begin())))
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n(); pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, class C>
inline void swap(multiset<K, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_MULTISET_HPP
