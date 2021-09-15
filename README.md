# sg
This project is dedicated to the [Scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree), the simplest and least resource-demanding [self-balancing binary search tree](https://en.wikipedia.org/wiki/Self-balancing_binary_search_tree).

Because of their low overhead, use of the `<=>` operator (2 comparisons for the price of 1) and because they share common properties with all other [BST](https://en.wikipedia.org/wiki/Binary_search_tree)-based containers, the [Scapegoat tree](https://en.wikipedia.org/wiki/Scapegoat_tree)-based `sg::` containers will sometimes outperform `std::` containers, while requiring less resources.

# build instructions
    g++ -std=c++20 -Ofast set.cpp -o s
    g++ -std=c++20 -Ofast map.cpp -o m
