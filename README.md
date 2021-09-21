# sg
This project provides [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree)-based alternatives to all [STL](https://en.wikipedia.org/wiki/Standard_Template_Library) [ordered associative containers](https://en.wikipedia.org/wiki/Associative_containers): `set`, `map`, `multiset`, `multimap` and 1 more, `intervalmap`.

The [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree) is the simplest and least resource-demanding [self-balancing binary search tree](https://en.wikipedia.org/wiki/Self-balancing_binary_search_tree). Because of their low overhead, use of the `<=>` operator (2 comparisons for the price of 1) and because they share common properties with all other [BST](https://en.wikipedia.org/wiki/Binary_search_tree)-based containers, the [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree)-based `sg::` containers  sometimes outperform `std::` containers, while requiring less resources.

Any [BST](https://en.wikipedia.org/wiki/Binary_search_tree) implementation can serve as a basis for an `ìntervalmap` implementation, but choosing the simplest, [scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree), conserves some resources and, hopefully, makes for a more robust implementation. `set`, `map`, `multiset` and `multimap` are stepping stones, in a way, towards an `ìntervalmap` implementation.

# build instructions
    g++ -std=c++20 -Ofast set.cpp -o s
    g++ -std=c++20 -Ofast map.cpp -o m
