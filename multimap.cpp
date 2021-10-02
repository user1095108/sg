#include <iostream>

#include "multimap.hpp"

//////////////////////////////////////////////////////////////////////////////
void dump(auto n)
{
  std::vector<decltype(n)> q{n};

  do
  {
    for (auto m(q.size()); m--;)
    {
      auto const n(q.front());
      q.erase(q.begin());

      if (n)
      {
        q.insert(q.end(), {n->l_, n->r_});

        std::cout << '(' << n->key() << ',' << n->v_.size() << ')';
      }
      else
      {
        std::cout << "(null)";
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
  sg::multimap<int, int> st;

  st.emplace(-1, 0);
  st.insert({0, 1});
  st.insert({{1, 2}, {1, 3}});
  st.emplace(2, 3);
  st.emplace(3, 4);

  //st.root().reset(st.root().release()->rebuild());
  dump(st.root());

  std::cout << "height: " << sg::detail::height(st.root()) << std::endl;
  std::cout << "size: " << st.size() << std::endl;

  std::for_each(
    st.begin(),
    st.end(),
    [](auto&& p) noexcept
    {
      std::cout << '(' << p.first << ',' << p.second << ')' << std::endl;
    }
  );

  sg::erase(st, 1);

  std::for_each(
    st.begin(),
    st.end(),
    [](auto&& p) noexcept
    {
      std::cout << '(' << p.first << ',' << p.second << ')' << std::endl;
    }
  );

  return 0;
}
