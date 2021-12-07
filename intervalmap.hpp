#ifndef SG_INTERVALMAP_HPP
# define SG_INTERVALMAP_HPP
# pragma once

#include "utils.hpp"

#include "multimapiterator.hpp"

namespace sg
{

template <typename Key, typename Value,
  class Compare = std::compare_three_way>
class intervalmap
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
    using value_type = intervalmap::value_type;

    static constinit inline Compare const cmp;

    node* l_{}, *r_{};

    typename std::tuple_element_t<1, Key> m_;
    std::list<value_type> v_;

    explicit node(auto&& k, auto&& v)
    {
      v_.emplace_back(
        std::forward<decltype(k)>(k),
        std::forward<decltype(v)>(v)
      );

      assert(std::get<0>(std::get<0>(v_.back())) <=
        std::get<1>(std::get<0>(v_.back())));

      m_ = std::get<1>(std::get<0>(v_.back()));
    }

    ~node() noexcept(noexcept(std::declval<Key>().~Key(),
      std::declval<Value>().~Value()))
    {
      delete l_; delete r_;
    }

    //
    auto&& key() const noexcept
    {
      return std::get<0>(std::get<0>(v_.front()));
    }

    //
    static auto emplace(auto& r, auto&& a, auto&& v)
    {
      key_type k(std::forward<decltype(a)>(a));
      auto const& [mink, maxk](k);

      node* q;

      auto const f([&](auto&& f, auto& n) -> size_type
        {
          if (!n)
          {
            n = q = new node(std::move(k), std::forward<decltype(v)>(v));

            return 1;
          }

          //
          n->m_ = cmp(n->m_, maxk) < 0 ? maxk : n->m_;

          //
          size_type sl, sr;

          if (auto const c(cmp(mink, n->key())); c < 0)
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
            (q = n)->v_.emplace_back(
              std::move(k),
              std::forward<decltype(v)>(v)
            );

            return {};
          }

          //
          auto const s(1 + sl + sr), S(2 * s);

          return (3 * sl > S) || (3 * sr > S) ? (n = n->rebuild(s), 0) : s;
        }
      );

      f(f, r);

      return q;
    }

    static auto equal_range(auto n, auto&& k) noexcept
    {
      decltype(n) g{};

      for (auto const& [mink, maxk](k); n;)
      {
        if (auto const c(cmp(mink, n->key())); c < 0)
        {
          detail::assign(g, n)(n, left_node(n));
        }
        else if (c > 0)
        {
          n = right_node(n);
        }
        else
        {
          if (auto const r(right_node(n)); r)
          {
            g = first_node(r);
          }

          break;
        }
      }

      return std::pair(n, g);
    }

    static iterator erase(auto& r, const_iterator const i)
    {
      if (auto const n(i.n()); 1 == n->v_.size())
      {
        return {&r, std::get<0>(node::erase(r, std::get<0>(*i)))};
      }
      else if (auto const it(i.i()); std::next(it) == n->v_.end())
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
      using pointer = typename std::remove_cvref_t<decltype(r0)>;
      using node = std::remove_pointer_t<pointer>;

      if (r0)
      {
        auto const& [mink, maxk](k);

        pointer p{};

        for (auto q(&r0);;)
        {
          auto const n(*q);

          if (auto const c(node::cmp(mink, n->key())); c < 0)
          {
            detail::assign(p, q)(n, &n->l_);
          }
          else if (c > 0)
          {
            detail::assign(p, q)(n, &n->r_);
          }
          else
          {
            auto const s(n->v_.size());
            auto const nxt(detail::next_node(r0, n));

            if (auto const l(n->l_), r(n->r_); l && r)
            {
              if (detail::size(l) < detail::size(r))
              {
                auto const [fnn, fnp](detail::first_node2(r, n));

                detail::assign(*q, fnn->l_)(fnn, l);

                if (r == fnn)
                {
                  reset_max(r0, r->key());
                }
                else
                {
                  detail::assign(fnp->l_, fnn->r_)(fnn->r_, r);

                  reset_max(r0, fnp->key());
                }
              }
              else
              {
                auto const [lnn, lnp](detail::last_node2(l, n));

                detail::assign(*q, lnn->r_)(lnn, r);

                if (l == lnn)
                {
                  reset_max(r0, l->key());
                }
                else
                {
                  detail::assign(lnp->r_, lnn->l_)(lnn->l_, l);

                  reset_max(r0, lnp->key());
                }
              }
            }
            else
            {
              *q = l ? l : r;

              if (p)
              {
                reset_max(r0, p->key());
              }
            }

            n->l_ = n->r_ = {};
            delete n;

            return std::pair(nxt, s);
          }
        }
      }

      return std::pair(pointer{}, std::size_t{});
    }

    static auto node_max(auto const n) noexcept
    {
      decltype(node::m_) m(n->key());

      std::for_each(
        n->v_.cbegin(),
        n->v_.cend(),
        [&](auto&& p) noexcept
        {
          auto const tmp(std::get<1>(std::get<0>(p)));
          m = cmp(m, tmp) < 0 ? tmp : m;
        }
      );

      return m;
    }

    static void reset_max(auto const n, auto&& key) noexcept
    {
      auto const f([&](auto&& f, auto const n) noexcept -> decltype(node::m_)
        {
          auto m(node_max(n));

          auto const l(n->l_), r(n->r_);

          if (auto const c(cmp(key, n->key())); c < 0)
          {
            if (r)
            {
              m = cmp(m, r->m_) < 0 ? r->m_ : m;
            }

            auto const tmp(f(f, l)); // visit left
            m = cmp(m, tmp) < 0 ? tmp : m;
          }
          else if (c > 0)
          {
            if (l)
            {
              m = cmp(m, l->m_) < 0 ? l->m_ : m;
            }

            auto const tmp(f(f, r)); // visit right
            m = cmp(m, tmp) < 0 ? tmp : m;
          }
          else // we are there
          {
            if (l)
            {
              m = cmp(m, l->m_) < 0 ? l->m_ : m;
            }

            if (r)
            {
              m = cmp(m, r->m_) < 0 ? r->m_ : m;
            }
          }

          return n->m_ = m;
        }
      );

      f(f, n);
    }

    auto rebuild(size_type const sz)
    {
//    auto const l(std::make_unique<node*[]>(sz)); // good
      auto const l(static_cast<node**>(ALLOCA(sizeof(this) * sz))); // bad

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

              n->m_ = node_max(n);

              break;

            case 1:
              {
                auto const nb(n->r_ = l[b]);

                n->l_ = nb->l_ = nb->r_ = {};

                n->m_ = std::max(node_max(n), nb->m_ = node_max(nb),
                  [](auto&& a, auto&& b)noexcept{return node::cmp(a, b) < 0;}
                );

                break;
              }

            default:
              auto const l(f(f, a, i - 1)), r(f(f, i + 1, b));
              detail::assign(n->l_, n->r_)(l, r);

              n->m_ = std::max({node_max(n), l->m_, r->m_},
                [](auto&& a, auto&& b)noexcept{return node::cmp(a, b) < 0;}
              );
          }

          return n;
        }
      );

      return f(f, {}, sz - 1);
    }
  };

private:
  using this_class = intervalmap;
  node* root_{};

public:
  intervalmap() = default;

  intervalmap(std::initializer_list<value_type> const il)
    noexcept(noexcept(*this = il))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = il;
  }

  intervalmap(intervalmap const& o)
    noexcept(noexcept(*this = o))
    requires(std::is_copy_constructible_v<value_type>)
  {
    *this = o;
  }

  intervalmap(intervalmap&& o)
    noexcept(noexcept(*this = std::move(o)))
  {
    *this = std::move(o);
  }

  intervalmap(std::input_iterator auto const i, decltype(i) j)
    noexcept(noexcept(insert(i, j)))
    requires(std::is_constructible_v<value_type, decltype(*i)>)
  {
    insert(i, j);
  }

  ~intervalmap() noexcept(noexcept(delete root_)) { delete root_; }

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
      for (auto& [mink, maxk](k);;)
      {
        if (node::cmp(mink, n->m_) < 0)
        {
          if (auto const c(node::cmp(mink, n->key())); c < 0)
          {
            n = detail::left_node(n);
          }
          else if (c > 0)
          {
            n = detail::right_node(n);
          }
          else
          {
            return std::count(
              n->v_.cbegin(),
              n->v_.cend(),
              [&](auto&& p) noexcept
              {
                return node::cmp(k, std::get<0>(p)) == 0;
              }
            );
          }
        }
      }
    }

    return {};
  }

  //
  iterator emplace(auto&& ...a)
  {
    return {&root_, node::emplace(root_, std::forward<decltype(a)>(a)...)};
  }

  //
  auto equal_range(Key const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(iterator(&root_, e ? e : g), iterator(root_, g));
  }

  auto equal_range(Key const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(
      const_iterator(&root_, e ? e : g),
      const_iterator(&root_, g)
    );
  }

  auto equal_range(auto const& k) noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(iterator(&root_, e ? e : g), iterator(&root_, g));
  }

  auto equal_range(auto const& k) const noexcept
  {
    auto const [e, g](node::equal_range(root_, k));

    return std::pair(const_iterator(&root_, e), const_iterator(&root_, g));
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
      &root_,
      node::emplace(root_, std::get<0>(v), std::get<1>(v))
    };
  }

  iterator insert(value_type&& v)
  {
    return {
      &root_,
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

  //
  void all(Key const& k, auto g) const
  {
    auto& [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    auto const f([&](auto&& f, auto const n) -> void
      {
        if (n && (node::cmp(mink, n->m_) < 0))
        {
          auto const c(node::cmp(maxk, n->key()));

          if (auto const cg0(c > 0); cg0 || (eq && (c == 0)))
          {
            std::for_each(
              n->v_.cbegin(),
              n->v_.cend(),
              [&](auto&& p)
              {
                if (node::cmp(mink, std::get<1>(std::get<0>(p))) < 0)
                {
                  g(std::forward<decltype(p)>(p));
                }
              }
            );

            if (cg0)
            {
              f(f, n->r_);
            }
          }

          f(f, n->l_);
        }
      }
    );

    f(f, root_);
  }

  bool any(Key const& k) const noexcept
  {
    auto& [mink, maxk](k);
    auto const eq(node::cmp(mink, maxk) == 0);

    if (auto n(root_); n && (node::cmp(mink, n->m_) < 0))
    {
      for (;;)
      {
        auto const c(node::cmp(maxk, n->key()));
        auto const cg0(c > 0);

        if (cg0 || (eq && (c == 0)))
        {
          if (auto const i(std::find_if(
                n->v_.cbegin(),
                n->v_.cend(),
                [&](auto&& p) noexcept
                {
                  return node::cmp(mink, std::get<1>(std::get<0>(p))) < 0;
                }
              )
            );
            n->v_.cend() != i
          )
          {
            return true;
          }
        }

        //
        if (auto const l(n->l_); l && (node::cmp(mink, l->m_) < 0))
        {
          n = l;
        }
        else if (auto const r(n->r_);
          cg0 && r && (node::cmp(mink, r->m_) < 0))
        {
          n = r;
        }
        else
        {
          break;
        }
      }
    }

    return false;
  }
};

}

#endif // SG_INTERVALMAP_HPP
