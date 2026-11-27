#ifndef RULES_H
#define RULES_H

#include "CellState.h"

class Rules {
public:
    // The whole rule lives right here, inside the class.
    const CellState* next(const CellState& current, int aliveNeighbors) const
    {
        if (current.isStone())
            return &STONE_STATE;

        bool survives = current.isAlive() && (aliveNeighbors == 2 || aliveNeighbors == 3);
        bool isBorn   = !current.isAlive() && aliveNeighbors == 3;

        if (survives || isBorn)
            return &ALIVE_STATE;
        return &DEAD_STATE;
    }
};

#endif
