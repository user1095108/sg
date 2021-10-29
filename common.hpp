//
auto& operator=(this_class const& o)
{
  if (this != &o)
  {
    clear();
    insert(o.begin(), o.end());
  }

  return *this;
}

auto& operator=(this_class&& o) noexcept(noexcept(delete root_))
{
  if (this != &o)
  {
    clear();

    root_ = o.root_;
    o.root_ = {};
  }

  return *this;
}

auto& operator=(std::initializer_list<value_type> const il)
{
  clear();
  insert(il.begin(), il.end());

  return *this;
}

void swap(this_class& o) noexcept { std::swap(root_, o.root_); }

//
friend bool operator!=(this_class const&, this_class const&) = default;
friend bool operator<(this_class const&, this_class const&) = default;
friend bool operator<=(this_class const&, this_class const&) = default;
friend bool operator>(this_class const&, this_class const&) = default;
friend bool operator>=(this_class const&, this_class const&) = default;

// iterators
iterator begin() noexcept
{
  return root_ ?
    iterator(root_, sg::detail::first_node(root_)) :
    iterator();
}

iterator end() noexcept { return {root_}; }

// const iterators
const_iterator begin() const noexcept
{
  return root_ ?
    const_iterator(root_, sg::detail::first_node(root_)) :
    const_iterator();
}

const_iterator end() const noexcept { return {root_}; }

const_iterator cbegin() const noexcept
{
  return root_ ?
    const_iterator(root_, sg::detail::first_node(root_)) :
    const_iterator();
}

const_iterator cend() const noexcept { return {root_}; }

// reverse iterators
reverse_iterator rbegin() noexcept
{
  return root_ ?
    reverse_iterator(iterator(root_)) :
    reverse_iterator();
}

reverse_iterator rend() noexcept
{
  return root_ ?
    reverse_iterator(
      iterator{root_, sg::detail::first_node(root_)}
    ) :
    reverse_iterator();
}

// const reverse iterators
const_reverse_iterator crbegin() const noexcept
{
  return root_ ?
    const_reverse_iterator(const_iterator(root_)) :
    const_reverse_iterator();
}

const_reverse_iterator crend() const noexcept
{
  return root_ ?
    const_reverse_iterator(
      const_iterator{root_, sg::detail::first_node(root_)}
    ) :
    const_reverse_iterator();
}

//
auto root() const noexcept { return root_; }

//
void clear() noexcept(noexcept(delete root_)) { delete root_; root_ = {}; }
auto empty() const noexcept { return !size(); }
auto max_size() const noexcept { return ~size_type{} / 3; }

//
bool contains(Key const& k) const
{
  return bool(sg::detail::find(root_, k));
}

bool contains(auto&& k) const
{
  return bool(sg::detail::find(root_, std::forward<decltype(k)>(k)));
}

//
iterator erase(const_iterator a, const_iterator const b)
{
  iterator i(b);

  for (; a != b; i = erase(a), a = i);

  return i;
}

iterator erase(std::initializer_list<const_iterator> const il)
{
  iterator r;

  // must be sequential
  std::for_each(il.begin(), il.end(), [&](auto const i) { r = erase(i); });

  return r;
}

//
iterator find(Key const& k) noexcept
{
  return iterator(root_, sg::detail::find(root_, k));
}

const_iterator find(Key const& k) const noexcept
{
  return const_iterator(root_, sg::detail::find(root_, k));
}

//
void insert(std::initializer_list<value_type> const il) noexcept(
  noexcept(insert(il.begin(), il.end())))
{
  insert(il.begin(), il.end());
}

//
iterator lower_bound(Key const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_, k));
  return {root_, e ? e : g};
}

const_iterator lower_bound(Key const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_, k));
  return {root_, e ? e : g};
}

iterator lower_bound(auto const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_, k));
  return {root_, e ? e : g};
}

const_iterator lower_bound(auto const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_, k));
  return {root_, e ? e : g};
}

//
iterator upper_bound(Key const& k) noexcept
{
  return {
    root_,
    std::get<1>(sg::detail::equal_range(root_, k))
  };
}

const_iterator upper_bound(Key const& k) const noexcept
{
  return {
    root_,
    std::get<1>(sg::detail::equal_range(root_, k))
  };
}

iterator upper_bound(auto const& k) noexcept
{
  return {
    root_,
    std::get<1>(sg::detail::equal_range(root_, k))
  };
}

const_iterator upper_bound(auto const& k) const noexcept
{
  return {
    root_,
    std::get<1>(sg::detail::equal_range(root_, k))
  };
}
