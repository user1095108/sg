# sg
This project is dedicated to the [Scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree), the simplest and least resource-intensive [self-balancing binary search tree](https://en.wikipedia.org/wiki/Self-balancing_binary_search_tree). It provides alternatives to several [STL](https://en.wikipedia.org/wiki/Standard_Template_Library) containers (`std::map`, `std::set`).

Because of their low overhead, use of the `<=>` operator (2 comparisons for the price of 1) and because they share common properties with all other [BSTs](https://en.wikipedia.org/wiki/Binary_search_tree), the Scapegoat tree-based `sg::` containers will sometimes outperform `std::` containers, while requiring less resources.
