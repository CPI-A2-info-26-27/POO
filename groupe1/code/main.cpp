#include <iostream>
#include <vector>
#include "Cell.h"
#include "CellState.h"
#include "Rules.h"

int main()
{
    const int W = 5, H = 5;
    Rules rules;

    // Build a 5x5 grid, all dead to start.
    std::vector<std::vector<Cell>> grid;
    grid.reserve(H);
    for (int y = 0; y < H; ++y) {
        std::vector<Cell> row;
        row.reserve(W);
        for (int x = 0; x < W; ++x)
            row.emplace_back(&DEAD_STATE);
        grid.push_back(std::move(row));
    }

    // Link the 8 neighbors of every cell.
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int ny = y + dy, nx = x + dx;
                    if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                    grid[y][x].addNeighbor(&grid[ny][nx]);
                }

    // A horizontal "blinker": three alive cells in the middle row.
    grid[2][1] = Cell(&ALIVE_STATE);
    grid[2][2] = Cell(&ALIVE_STATE);
    grid[2][3] = Cell(&ALIVE_STATE);
    // Re-link the replaced cells (assignment above dropped their neighbors).
    for (int x = 1; x <= 3; ++x)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int ny = 2 + dy, nx = x + dx;
                if (ny < 0 || ny >= H || nx < 0 || nx >= W) continue;
                grid[2][x].addNeighbor(&grid[ny][nx]);
            }

    auto print = [&]() {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x)
                std::cout << grid[y][x].getState()->symbol();
            std::cout << '\n';
        }
        std::cout << '\n';
    };

    for (int gen = 0; gen < 3; ++gen) {
        std::cout << "Generation " << gen << ":\n";
        print();
        for (auto& row : grid)
            for (auto& c : row)
                c.computeNext(rules);
        for (auto& row : grid)
            for (auto& c : row)
                c.applyNext();
    }

    return 0;
}
