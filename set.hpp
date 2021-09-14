#ifndef DS_SGSET_HPP
# define DS_SGSET_HPP
# pragma once

#include <algorithm>
#include <execution>

#include <compare>

#include <memory>

#include <vector>

#include "utils.hpp"

#include "setiterator.hpp"

namespace sg
{

template <typename Key, class Comp = std::compare_three_way>
class set
{
public:
  using key_type = Key;

  using size_type = std::size_t;

  struct node
  {
    using key_type = Key;

    static constinit inline auto const cmp{Comp{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    Key const k_;

    explicit node(auto&& k):
      k_(std::forward<decltype(k)>(k))
    {
    }

    //
    auto&& key() const noexcept { return k_; }

    //
    static auto emplace(auto&& r, auto&& k)
    {
      bool s{true};
      node* q{};

      auto const f([&](auto&& f, auto& n) noexcept -> size_type
        {
          if (!n)
          {
            n.reset(q = new node(std::forward<decltype(k)>(k)));

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

            sr = sg::size(n->r_);
          }
          else if (c > 0)
          {
            if (sr = f(f, n->r_); !sr)
            {
              return 0;
            }

            sl = sg::size(n->l_);
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
        auto n(first_node(this));

        do
        {
          l.emplace_back(n);

          n = next(this, n);
        }
        while (n);
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

  using const_iterator = setiterator<node const>;
  using iterator = setiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  std::unique_ptr<node> root_;

public:
  set() = default;

  set(std::initializer_list<Key> i) { *this = i; }

  set(set const& o) { *this = o; }

  set(set&&) = default;

  //
  auto& operator=(auto&& o) requires(
    std::is_same_v<decltype(o), set&> ||
    std::is_same_v<decltype(o), set const&> ||
    std::is_same_v<decltype(o), set const&&> ||
    std::is_same_v<
      std::remove_cvref_t<decltype(o)>,
      std::initializer_list<Key>
    >
  )
  {
    clear();

    std::for_each(
      std::execution::unseq,
      o.begin(),
      o.end(),
      [&](auto&& p)
      {
        emplace(p);
      }
    );

    return *this;
  }

  auto& operator=(set&& o) noexcept
  {
    return root_ = std::move(o.root_), *this;
  }

  //
  auto root() const noexcept { return root_.get(); }

  //
  void clear() { root_.reset(); }
  bool empty() const noexcept { return !size(); }
  size_type max_size() const noexcept { return ~size_type{} / 3; }
  size_type size() const noexcept { return sg::size(root_); }

  // iterators
  iterator begin() noexcept
  {
    return root_ ?
      iterator(root_.get(), sg::first_node(root_.get())) :
      iterator();
  }

  iterator end() noexcept { return {}; }

  // const iterators
  const_iterator begin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), sg::first_node(root_.get())) :
      const_iterator();
  }

  const_iterator end() const noexcept { return {}; }

  const_iterator cbegin() const noexcept
  {
    return root_ ?
      const_iterator(root_.get(), sg::first_node(root_.get())) :
      const_iterator();
  }

  const_iterator cend() const noexcept { return {}; }

  // reverse iterators
  reverse_iterator rbegin() noexcept
  {
    return root_ ?
      reverse_iterator(iterator(root_.get(), nullptr)) :
      reverse_iterator();
  }

  reverse_iterator rend() noexcept
  {
    return root_ ?
      reverse_iterator(iterator{root_.get(), sg::first_node(root_.get())}) :
      reverse_iterator();
  }

  // const reverse iterators
  const_reverse_iterator crbegin() const noexcept
  {
    return root_ ?
      const_reverse_iterator(const_iterator(root_.get(), nullptr)) :
      const_reverse_iterator();
  }

  const_reverse_iterator crend() const noexcept
  {
    return root_ ?
      const_reverse_iterator(
        const_iterator{root_.get(), sg::first_node(root_.get())}
      ) :
      const_reverse_iterator();
  }

  //
  bool contains(Key const& k) const
  {
    return bool(sg::find(root_.get(), k));
  }

  bool contains(auto&& k) const
  {
    return bool(sg::find(root_.get(), std::forward<decltype(k)>(k)));
  }

  size_type count(Key const& k) const noexcept
  {
    return bool(sg::find(root_.get(), k));
  }

  //
  auto emplace(auto&& k)
  {
    auto const [n, s](
      node::emplace(root_, std::forward<decltype(k)>(k))
    );

    return std::tuple(setiterator<node>(root_.get(), n), s);
  }

  //
  size_type erase(Key const& k)
  {
    return std::get<1>(sg::erase(root_, k));
  }

  iterator erase(const_iterator const i)
  {
    return iterator(root_.get(), std::get<0>(sg::erase(root_, *i)));
  }

  auto erase(const_iterator a, const_iterator const b)
  {
    for (; a != b; a = erase(a));
    return a;
  }

  auto erase(std::initializer_list<const_iterator> il)
  {
    iterator r;

    std::for_each(il.begin(), il.end(),
      [&](auto&& i) { r = erase(i); }
    );

    return r;
  }

  //
  auto find(Key const& k) noexcept
  {
    return iterator(root_.get(), sg::find(root_.get(), k));
  }

  auto find(Key const& k) const noexcept
  {
    return const_iterator(root_.get(), sg::find(root_.get(), k));
  }
};

}

#endif // DS_SGSET_HPP
