#include <iostream>

#include "intervalmap.hpp"

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
        q.insert(q.end(), {n->l_.get(), n->r_.get()});

        std::cout << '(' << n->k_ << ',' << n->m_ << ')';
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
  sg::intervalmap<std::pair<int, int>, int> st;

  st.emplace(std::pair(-1, 0), -1);
  st.emplace(std::pair(0, 1), 0);
  st.emplace(std::pair(1, 2), 1);
  st.emplace(std::pair(2, 3), 2);
  st.emplace(std::pair(3, 5), 3);

  //st.root().reset(st.root().release()->rebuild());
  dump(st.root());

  std::cout << "height: " << sg::height(st.root()) << std::endl;
  std::cout << "size: " << st.size() << std::endl;
  std::cout << "any: " << st.any(std::pair(0, 1)) << std::endl;

  for (auto&& p: st.all(std::pair(1, 3)))
  {
    std::cout << '(' << p.first.first << ',' << p.first.second << ") " <<
      p.second << std::endl;
  }

  return 0;
}
