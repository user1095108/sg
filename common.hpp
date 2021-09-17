//
auto& operator=(auto&& o) requires(
  std::same_as<decltype(o), std::remove_cvref_t<this_class>> ||
  std::same_as<
    std::remove_cvref_t<decltype(o)>,
    std::initializer_list<value_type>
  >
)
{
  clear();

  insert(o.begin(), o.end());

  return *this;
}

this_class& operator=(this_class&& o) noexcept = default;

//
auto root() const noexcept { return root_.get(); }

//
void clear() { root_.reset(); }
auto empty() const noexcept { return !size(); }
auto max_size() const noexcept { return ~size_type{} / 3; }

// iterators
iterator begin() noexcept
{
  return root_ ?
    iterator(root_.get(), sg::detail::first_node(root_.get())) :
    iterator();
}

iterator end() noexcept { return {}; }

// const iterators
const_iterator begin() const noexcept
{
  return root_ ?
    const_iterator(root_.get(), sg::detail::first_node(root_.get())) :
    const_iterator();
}

const_iterator end() const noexcept { return {}; }

const_iterator cbegin() const noexcept
{
  return root_ ?
    const_iterator(root_.get(), sg::detail::first_node(root_.get())) :
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
    reverse_iterator(
      iterator{root_.get(), sg::detail::first_node(root_.get())}
    ) :
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
      const_iterator{root_.get(), sg::detail::first_node(root_.get())}
    ) :
    const_reverse_iterator();
}

//
auto equal_range(Key const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return std::pair(
    iterator(root_.get(), e ? e : g),
    iterator(root_.get(), g)
  );
}

//
bool contains(Key const& k) const
{
  return bool(sg::detail::find(root_.get(), k));
}

bool contains(auto&& k) const
{
  return bool(sg::detail::find(root_.get(), std::forward<decltype(k)>(k)));
}

//
auto equal_range(Key const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return std::pair(
    const_iterator(root_.get(), e ? e : g),
    const_iterator(root_.get(), g)
  );
}

auto equal_range(auto const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return std::pair(
    iterator(root_.get(), e ? e : g),
    iterator(root_.get(), g)
  );
}

auto equal_range(auto const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return std::pair(
    const_iterator(root_.get(), e ? e : g),
    const_iterator(root_.get(), g)
  );
}

//
auto erase(const_iterator a, const_iterator const b)
{
  iterator i;

  for (; a != b; i = erase(a), a = i);

  return i;
}

auto erase(std::initializer_list<const_iterator> const il)
{
  iterator r;

  std::for_each(il.begin(), il.end(), [&](auto const i) { r = erase(i); });

  return r;
}

//
auto find(Key const& k) noexcept
{
  return iterator(root_.get(), sg::detail::find(root_.get(), k));
}

auto find(Key const& k) const noexcept
{
  return const_iterator(root_.get(), sg::detail::find(root_.get(), k));
}

//
void insert(std::initializer_list<value_type> const il)
{
  insert(il.begin(), il.end());
}

//
iterator lower_bound(Key const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return {root_.get(), e ? e : g};
}

const_iterator lower_bound(Key const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return {root_.get(), e ? e : g};
}

iterator lower_bound(auto const& k) noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return {root_.get(), e ? e : g};
}

const_iterator lower_bound(auto const& k) const noexcept
{
  auto const [e, g](sg::detail::equal_range(root_.get(), k));
  return {root_.get(), e ? e : g};
}

//
iterator upper_bound(Key const& k) noexcept
{
  return {root_.get(),
    std::get<1>(sg::detail::equal_range(root_.get(), k))};
}

const_iterator upper_bound(Key const& k) const noexcept
{
  return {root_.get(),
    std::get<1>(sg::detail::equal_range(root_.get(), k))};
}

iterator upper_bound(auto const& k) noexcept
{
  return {root_.get(),
    std::get<1>(sg::detail::equal_range(root_.get(), k))};
}

const_iterator upper_bound(auto const& k) const noexcept
{
  return {root_.get(),
    std::get<1>(sg::detail::equal_range(root_.get(), k))};
}
