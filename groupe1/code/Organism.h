#ifndef ORGANISM_H
#define ORGANISM_H

#include <string>
#include <vector>
#include "Cell.h"
#include "Rules.h"

class Organism {
private:
    std::string name;
    int length;
    int width;
    bool withStones;
    std::vector<Cell*> grid;
    const Rules& rules;
    std::vector<std::vector<bool>> history;

    Cell* at(int row, int col) const { return grid[row * width + col]; }
    void linkNeighbors(int row, int col);
    void buildGrid(const std::vector<const CellState*>& states);

public:
    int step;

    Organism(const std::string& n, int l, int w, const Rules& r, bool useStones = false);
    ~Organism();

    void init_organism();
    bool init_from_file(const std::string& path, std::string& error);

    int getLength() const { return length; }
    int getWidth() const { return width; }
    int getStep() const { return step; }

    void nextstep();
    void saveGrid();
    const std::vector<bool>& getGrid(int s) const { return history[s]; }
    std::vector<bool> getStones() const;
    bool alreadySeen() const;
    void tab() const;
};

#endif
