#include "Cell.h"
#include "Rules.h"

Cell::Cell(const CellState* initial) : state(initial), nextState(initial) {}

void Cell::computeNext(const Rules& rules)
{
    int alive = 0;
    for (const Cell* n : neighbors)
        if (n->isAlive())
            alive++;
    nextState = rules.next(*state, alive);
}
