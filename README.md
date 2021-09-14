# sg
This project is dedicated to the Scapegoat tree, the simplest and least resource-hungry [self-balancing binary search tree](https://en.wikipedia.org/wiki/Self-balancing_binary_search_tree). It provides alternatives to several [STL](https://en.wikipedia.org/wiki/Standard_Template_Library) containers (`std::map`, `std::set`).

Because of its low overhead, use of the `<=>` operator (2 comparisons for the price of 1) and because it shares common properties with all other BSTs, the humble `sg::map` will sometimes outperform `std::map`, despite using less resources.
