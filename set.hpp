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
    auto&& key() const noexcept { return kv_; }

    //
    static auto emplace(auto& r, auto&& ...a)
      noexcept(noexcept(new node(key_type(std::forward<decltype(a)>(a)...))))
    {
      bool s{}; // success
      node* q;

      key_type k(std::forward<decltype(a)>(a)...);

      auto const f([&](auto&& f, auto& n)
        noexcept(noexcept(new node(std::move(k)))) -> size_type
        {
          if (!n)
          {
            s = (n = q = new node(std::move(k)));

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
  using this_class = set;
  node* root_{};

public:
  set() = default;

  set(std::initializer_list<value_type> const l)
    noexcept(noexcept(*this = l))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = l;
  }

  set(set const& o) 
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  set(set&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  set(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<Key, decltype(*i)>)
  {
    insert(i, j);
  }

  ~set() noexcept(noexcept(delete root_)) { delete root_; }

# include "common.hpp"

  //
  auto size() const noexcept { return detail::size(root_); }

  //
  size_type count(auto const& k) const noexcept
  {
    return bool(detail::find(root_, k));
  }

  //
  auto emplace(auto&& ...a)
    noexcept(noexcept(node::emplace(root_, std::forward<decltype(a)>(a)...)))
  {
    auto const [n, s](node::emplace(root_, std::forward<decltype(a)>(a)...));

    return std::tuple(iterator(&root_, n), s);
  }

  //
  auto equal_range(auto const& k) noexcept
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(iterator(&root_, nl), iterator(&root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [nl, g](detail::equal_range(root_, k));

    return std::pair(const_iterator(&root_, nl), const_iterator(&root_, g));
  }

  //
  iterator erase(const_iterator const i)
  {
    return {&root_, detail::erase(root_, *i)};
  }

  size_type erase(auto const& k)
    requires(!std::is_convertible_v<decltype(k), const_iterator>)
  {
    return bool(detail::erase(root_, k));
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, s](node::emplace(root_, v));

    return std::tuple(iterator(&root_, n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](node::emplace(root_, std::move(v)));

    return std::tuple(iterator(&root_, n), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      i,
      j,
      [&](auto&& v){ emplace(std::forward<decltype(v)>(v)); }
    );
  }
};

//////////////////////////////////////////////////////////////////////////////
template <typename K, class C>
inline auto erase(set<K, C>& c, auto const& k)
  noexcept(noexcept(c.erase(k)))
{
  return c.erase(k);
}

template <typename K, class C>
inline auto erase_if(set<K, C>& c, auto pred)
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
template <typename K, class C>
inline void swap(set<K, C>& l, decltype(l) r) noexcept { l.swap(r); }

}

#endif // SG_SET_HPP
