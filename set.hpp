#ifndef SG_SET_HPP
# define SG_SET_HPP
# pragma once

#include <memory>

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

  using size_type = std::size_t;

  using const_iterator = mapiterator<node const>;
  using iterator = mapiterator<node>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  struct node
  {
    using value_type = set::value_type;

    static constinit inline auto const cmp{Compare{}};

    std::unique_ptr<node> l_;
    std::unique_ptr<node> r_;

    Key const kv_;

    explicit node(auto&& k):
      kv_(std::forward<decltype(k)>(k))
    {
    }

    //
    auto&& key() const noexcept { return kv_; }

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

private:
  using this_class = set;
  std::unique_ptr<node> root_;

public:
  set() = default;
  set(std::initializer_list<Key> i) { *this = i; }
  set(set const& o) { *this = o; }
  set(set&&) = default;
  set(std::input_iterator auto const i, decltype(i) j) { insert(i, j); }

# include "common.hpp"

  //
  auto size() const noexcept { return sg::detail::size(root_); }

  //
  size_type count(Key const& k) const noexcept
  {
    return bool(sg::detail::find(root_.get(), k));
  }

  //
  auto emplace(auto&& k)
  {
    auto const [n, s](
      node::emplace(root_, std::forward<decltype(k)>(k))
    );

    return std::tuple(iterator(root_.get(), n), s);
  }

  //
  size_type erase(Key const& k)
  {
    return sg::detail::erase(root_, k) ? 1 : 0;
  }

  iterator erase(const_iterator const i)
  {
    return iterator(root_.get(), sg::detail::erase(root_, *i));
  }

  //
  auto insert(value_type const& v)
  {
    auto const [n, s](node::emplace(root_, v));

    return std::tuple(iterator(root_.get(), n), s);
  }

  auto insert(value_type&& v)
  {
    auto const [n, s](node::emplace(root_, std::move(v)));

    return std::tuple(iterator(root_.get(), n), s);
  }

  void insert(std::input_iterator auto const i, decltype(i) j)
  {
    std::for_each(
      std::execution::unseq,
      i,
      j,
      [&](auto&& v)
      {
        emplace(std::forward<decltype(v)>(v));
      }
    );
  }

  //
  friend bool operator!=(set const&, set const&) noexcept = default;
  friend bool operator<(set const&, set const&) noexcept = default;
  friend bool operator<=(set const&, set const&) noexcept = default;
  friend bool operator>(set const&, set const&) noexcept = default;
  friend bool operator>=(set const&, set const&) noexcept = default;
};

}

#endif // SG_SET_HPP
