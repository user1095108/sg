#ifndef SG_MULTIMAP_HPP
# define SG_MULTIMAP_HPP
# pragma once

#include <list>
#include <vector>

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

  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using reference = value_type&;
  using const_reference = value_type const&;

  using const_iterator = multimapiterator<node const>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using iterator = multimapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;

  struct node
  {
    using value_type = multimap::value_type;

    static constinit inline auto const cmp{Compare{}};

    node* l_{}, *r_{};
    std::list<value_type> v_;

    explicit node(auto&& ...a)
    {
      v_.emplace_back(std::forward<decltype(a)>(a)...);
    }

    ~node() noexcept(noexcept(std::declval<Key>().~Key(),
      std::declval<Value>().~Value()))
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept { return std::get<0>(v_.front()); }

    //
    static auto emplace(auto& r, auto&& a, auto&& v)
    {
      node* q;

      key_type k(std::forward<decltype(a)>(a));

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            n = q = new node(std::move(k), std::forward<decltype(v)>(v));

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
            (q = n)->v_.emplace_back(
              std::move(k),
              std::forward<decltype(v)>(v)
            );

            return 0;
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(), 0) : s;
        }
      );

      f(f, r);

      return q;
    }

    static iterator erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.node()); 1 == n->v_.size())
      {
        return {r, std::get<0>(node::erase(r, n->key()))};
      }
      else if (auto const it(i.iterator()); std::next(it) == n->v_.end())
      {
        auto const nn(std::next(i).node());

        n->v_.erase(it);

        return {r, nn};
      }
      else
      {
        return {r, n, n->v_.erase(it)};
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
            auto const nxt(sg::detail::next_node(r0, n));
            auto const s(n->v_.size());

            if (auto const l(n->l_), r(n->r_); l && r)
            {
              *q = {};
              sg::detail::move(r0, l, r);
            }
            else
            {
              *q = l ? l : r;
            }

            n->l_ = n->r_ = {};
            delete n;

            return std::tuple(nxt, s);
          }
        }
      }

      return std::tuple(pointer{}, size_type{});
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
  using this_class = multimap;
  node* root_{};

public:
  multimap() = default;

  multimap(std::initializer_list<value_type> const il)
    noexcept(noexcept(*this = il))
  {
    *this = il;
  }

  multimap(multimap const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_assignable_v<mapped_type>)
  {
    *this = o;
  }

  multimap(multimap&& o) noexcept
  {
    *this = std::move(o);
  }

  multimap(std::input_iterator auto const i, decltype(i) j) { insert(i, j); }

  ~multimap() noexcept(noexcept(root_->~node())) { delete root_; }

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
  size_type count(Key const& k) const noexcept
  {
    if (auto n(root_); n)
    {
      for (;;)
      {
        if (auto const c(node::cmp(k, n->key())); c < 0)
        {
          n = sg::detail::left_node(n);
        }
        else if (c > 0)
        {
          n = sg::detail::right_node(n);
        }
        else
        {
          return n->v_.size();
        }
      }
    }

    return {};
  }

  //
  auto emplace(auto&& ...a)
  {
    return iterator(root_,
      node::emplace(root_, std::forward<decltype(a)>(a)...));
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(
      iterator(root_, e ? e : g),
      iterator(root_, g)
    );
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(
      const_iterator(root_, e ? e : g),
      const_iterator(root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(
      iterator(root_, e ? e : g),
      iterator(root_, g)
    );
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](sg::detail::equal_range(root_, k));

    return std::pair(
      const_iterator(root_, e ? e : g),
      const_iterator(root_, g)
    );
  }

  //
  iterator erase(const_iterator const i)
  {
    return node::erase(root_, i);
  }

  size_type erase(Key const& k)
  {
    return std::get<1>(node::erase(root_, k));
  }

  //
  iterator insert(value_type const& v)
  {
    return {
      root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
  }

  iterator insert(value_type&& v)
  {
    return {
      root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
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
};

}

#endif // SG_MULTIMAP_HPP
