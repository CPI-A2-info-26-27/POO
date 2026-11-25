#ifndef RULES_H
#define RULES_H

#include "CellState.h"

class Rules {
public:
    const CellState* next(const CellState& current, int aliveNeighbors);
};

#endif
