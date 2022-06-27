#include <iostream>

#include "intervalmap.hpp"

//////////////////////////////////////////////////////////////////////////////
void dump(auto n)
{
  std::list<decltype(n)> q{n};

  do
  {
    for (auto m(q.size()); m--;)
    {
      auto const n(q.front());
      q.erase(q.begin());

      if (n)
      {
        q.insert(q.end(), {n->l_, n->r_});

        std::cout << '{' << n->key() << ',' << n->m_ << '}';
      }
      else
      {
        std::cout << "{null}";
      }

      std::cout << ' ';
    }

    std::cout << std::endl;
  }
  while (q.size() && std::any_of(q.cbegin(), q.cend(),
    [](auto const p) noexcept { return p; }));
}

//////////////////////////////////////////////////////////////////////////////
int main()
{
  sg::intervalmap<std::pair<int, int>, int> st;

  st.emplace({-1, 0}, -1);
  st.insert({{0, 1}, 0});
  st.insert({{{1, 2}, 1}, {std::pair(1, 4), 1}});
  st.emplace({2, 3}, 2);
  st.emplace({3, 5}, 3);

  //st.root().reset(st.root().release()->rebuild());
  dump(st.root());

  std::cout << "height: " << sg::detail::height(st.root()) << std::endl;
  std::cout << "size: " << st.size() << std::endl;
  std::cout << "any: " << st.any(std::pair(0, 1)) << std::endl;

  std::for_each(
    st.crbegin(),
    st.crend(),
    [](auto&& p) noexcept
    {
      std::cout << '(' << p.first.first << ',' << p.first.second << ')' << std::endl;
    }
  );

  st.all(
    {-1, 0},
    [](auto&& p)
    {
      std::cout << '(' << p.first.first << ',' << p.first.second << ") " <<
        p.second << std::endl;
    }
  );

  srandom(time(nullptr));

  while (st.size())
  {
    st.erase(std::next(st.begin(), random() % st.size()));
  }

  return 0;
}
