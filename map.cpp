#include <iostream>

#include "sgmap.hpp"

using namespace std::literals::string_literals;

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

        std::cout << '(' << n->kv_.first << ',' << n->kv_.second << ')';
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
  ds::sgmap<std::string, int> st{
    {"a"s, -1},
    {"b"s, 0},
    {"c"s, 1}
  };

  st.emplace("d"s, 2);
  st["e"s] = 3;
  st["f"s] = 4;

  /*
  for (std::size_t i{}; 1000 != i; ++i)
  {
    st.emplace(std::string(rand() % 9 + 1, 48 + rand() % (123 - 48)), i);
  }
  */

  dump(st.root());

  std::cout << "height: " << ds::height(st.root()) << std::endl;
  std::cout << "size: " << st.size() << std::endl;

  std::for_each(
    st.crbegin(),
    st.crend(),
    [](auto&& p) noexcept
    {
      std::cout << "(" << p.first << " " << p.second << ")" << std::endl;
    }
  );

  return 0;
}
