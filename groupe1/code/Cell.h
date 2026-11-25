#ifndef CELL_H
#define CELL_H

#include <list>
#include "CellState.h"

class Rules;

class Cell {
private:
    const CellState* state;
    const CellState* nextState;
    std::list<Cell*> neighbors;

public:
    explicit Cell(const CellState* initial);

    bool isAlive() const { return state->isAlive(); }
    const CellState* getState() const { return state; }

    void addNeighbor(Cell* c) { neighbors.push_back(c); }

    void computeNext(const Rules& rules);
    void applyNext() { state = nextState; }
};

#endif
