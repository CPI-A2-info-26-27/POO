#ifndef RULES_H
#define RULES_H

#include "CellState.h"

class Rules {
public:
    virtual ~Rules() = default;
    virtual const CellState* next(const CellState& current, int aliveNeighbors) const = 0;
};

class ConwayRules : public Rules {
public:
    const CellState* next(const CellState& current, int aliveNeighbors) const override;
};

#endif
