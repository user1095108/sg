#ifndef SG_MULTIMAP_HPP
# define SG_MULTIMAP_HPP
# pragma once

#include "utils.hpp"

#include "multimapiterator.hpp"

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

  using difference_type = detail::difference_type;
  using size_type = detail::size_type;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = multimapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = multimap::value_type;

    static constinit inline Compare const cmp;

    node* l_{}, *r_{};
    std::list<value_type> v_;

    explicit node(auto&& k, auto&& ...a)
      noexcept(noexcept(
          v_.emplace_back(
            std::piecewise_construct_t{},
            std::forward_as_tuple(std::forward<decltype(k)>(k)),
            std::forward_as_tuple(std::forward<decltype(a)>(a)...)
          )
        )
      )
    {
      v_.emplace_back(
        std::piecewise_construct_t{},
        std::forward_as_tuple(std::forward<decltype(k)>(k)),
        std::forward_as_tuple(std::forward<decltype(a)>(a)...)
      );
    }

    ~node() noexcept(std::is_nothrow_destructible_v<decltype(v_)>)
    {
      delete l_; delete r_;
    }

    //
    auto& key() const noexcept { return std::get<0>(v_.front()); }

    //
    static auto emplace(auto& r, auto&& a, auto&& ...b)
      noexcept(
        noexcept(
          new node(
            key_type(std::forward<decltype(a)>(a)),
            std::forward<decltype(b)>(b)...
          )
        )
      )
    {
      node* q;

      key_type k(std::forward<decltype(a)>(a));

      auto const f([&](auto&& f, auto& n)
        noexcept(
          noexcept(new node(std::move(k), std::forward<decltype(b)>(b)...))
        ) -> size_type
        {
          if (!n)
          {
            n = q = new node(std::move(k), std::forward<decltype(b)>(b)...);

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

            sr = detail::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_); !sr)
            {
              return 0;
            }

            sl = detail::size(n->l_);
          }
          else
          {
            (q = n)->v_.emplace_back(
              std::piecewise_construct_t{},
              std::forward_as_tuple(std::move(k)),
              std::forward_as_tuple(std::forward<decltype(b)>(b)...)
            );

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebalance(s), 0) : s;
        }
      );

      f(f, r);

      return q;
    }

    static iterator erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.n()); 1 == n->v_.size())
      {
        return {&r, std::get<0>(node::erase(r, n->key()))};
      }
      else if (auto const it(i.iterator()); std::next(it) == n->v_.end())
      {
        auto const nn(std::next(i).n());

        n->v_.erase(it);

        return {&r, nn};
      }
      else
      {
        return {&r, n, n->v_.erase(it)};
      }
    }

    static auto erase(auto& r0, auto&& k)
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
  using this_class = multimap;
  node* root_{};

public:
  multimap() = default;

  multimap(std::initializer_list<value_type> l)
    noexcept(noexcept(*this = l))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = l;
  }

  multimap(multimap const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  multimap(multimap&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  multimap(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  ~multimap() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  size_type size() const noexcept
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
  size_type count(auto&& k, char = {}) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
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

  size_type count(key_type const& k) const noexcept { return count(k, {}); }

  //
  iterator emplace(Key&& k, auto&& ...a)
    noexcept(noexcept(
        node::emplace(
          root_,
          std::move(k),
          std::forward<decltype(a)>(a)...
        )
      )
    )
  {
    return {
      &root_,
      node::emplace(
        root_,
        std::move(k),
        std::forward<decltype(a)>(a)...
      )
    };
  }

  iterator emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    return {&root_, node::emplace(root_, std::forward<decltype(a)>(a)...)};
  }

  //
  auto equal_range(auto&& k, char = {}) noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(auto&& k, char = {}) const noexcept
    requires(detail::Comparable<Compare, decltype(k), key_type>)
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
    noexcept(noexcept(node::emplace(root_, std::get<0>(v), std::get<1>(v))))
  {
    return {
      &root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
  }

  iterator insert(value_type&& v)
    noexcept(noexcept(node::emplace(root_, std::get<0>(v), std::get<1>(v))))
  {
    return {
      &root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
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
};

//////////////////////////////////////////////////////////////////////////////
template <int = 0, typename K, typename V, class C>
inline auto erase(multimap<K, V, C>& c, auto&& k)
  noexcept(noexcept(c.erase(std::forward<decltype(k)>(k))))
{
  return c.erase(std::forward<decltype(k)>(k));
}

template <typename K, typename V, class C>
inline auto erase(multimap<K, V, C>& c, K k)
  noexcept(noexcept(erase<0>(c, std::move(k))))
{
  return erase<0>(c, std::move(k));
}

template <typename K, typename V, class C>
inline auto erase_if(multimap<K, V, C>& c, auto pred)
  noexcept(
    noexcept(pred(std::declval<K>())) &&
    noexcept(c.erase(c.begin()))
  )
{
  typename std::remove_reference_t<decltype(c)>::size_type r{};

  for (auto i(c.begin()); i.n(); pred(*i) ? ++r, i = c.erase(i) : ++i);

  return r;
}

//////////////////////////////////////////////////////////////////////////////
template <typename K, typename V, class C>
inline void swap(multimap<K, V, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_MULTIMAP_HPP
