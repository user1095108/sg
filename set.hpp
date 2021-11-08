#ifndef SG_SET_HPP
# define SG_SET_HPP
# pragma once

#include <vector>

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
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = mapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = set::value_type;

    static constinit inline auto const cmp{Compare{}};

    node* l_{}, *r_{};
    Key const kv_;

    explicit node(auto&& ...a) noexcept(noexcept(Key())):
      kv_(std::forward<decltype(a)>(a)...)
    {
    }

    ~node() noexcept(noexcept(std::declval<Key>().~Key()))
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept { return kv_; }

    //
    static auto emplace(auto& r, auto&& ...a)
    {
      node* q;
      bool s{}; // success

      key_type k(std::forward<decltype(a)>(a)...);

      auto const f([&](auto&& f, auto& n) -> size_type
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
        auto n(detail::first_node(this));

        do
        {
          l.emplace_back(n);
        }
        while ((n = detail::next_node(this, n)));
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
  using this_class = set;
  node* root_{};

public:
  set() = default;

  set(std::initializer_list<Key> il)
    noexcept(noexcept(*this = il))
    requires(std::is_copy_constructible_v<Key>)
  {
    *this = il;
  }

  set(set const& o) 
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<Key>)
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

    return std::pair(
      iterator(&root_, e ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      const_iterator(&root_, e ? e : g),
      const_iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      iterator(&root_, e ? e : g),
      iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](detail::equal_range(root_, k));

    return std::pair(
      const_iterator(&root_, e ? e : g),
      const_iterator(&root_, g)
    );
  }

  //
  size_type erase(Key const& k)
  {
    return bool(detail::erase(root_, k));
  }

  iterator erase(const_iterator const i)
  {
    return {&root_, detail::erase(root_, *i)};
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
      [&](auto&& v)
      {
        emplace(std::forward<decltype(v)>(v));
      }
    );
  }
};

}

#endif // SG_SET_HPP
