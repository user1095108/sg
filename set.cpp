#include <iostream>

#include "set.hpp"

//////////////////////////////////////////////////////////////////////////////
int main()
{
  sg::set<int> s{4, 3, 2, 1};

  std::cout << s.contains(3) << std::endl;
  std::cout << s.contains(100) << std::endl;

  std::for_each(
    s.cbegin(),
    s.cend(),
    [](auto&& p) noexcept
    {
      std::cout << p << std::endl;
    }
  );

  std::cout << "eq: " << (s == sg::set<int>{1, 2, 3, 4}) << std::endl;

  s.erase({std::next(s.cbegin()), std::prev(s.cend())});

  std::for_each(
    s.cbegin(),
    s.cend(),
    [](auto&& p) noexcept
    {
      std::cout << p << std::endl;
    }
  );

  return 0;
}
