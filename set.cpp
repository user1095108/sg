#include <iostream>

#include "sgset.hpp"

//////////////////////////////////////////////////////////////////////////////
int main()
{
  ds::sgset<int> s{4, 3, 2, 1};

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

  return 0;
}
