#ifndef CELLSTATE_H
#define CELLSTATE_H

#include <string>

class CellState {
public:
    virtual ~CellState() = default;
    virtual bool isAlive() const = 0;
    virtual bool isStone() const { return false; }
    virtual std::string symbol() const = 0;
};

class AliveState : public CellState {
public:
    bool isAlive() const override { return true; }
    std::string symbol() const override { return "🟩 "; }
};

class DeadState : public CellState {
public:
    bool isAlive() const override { return false; }
    std::string symbol() const override { return "🟥 "; }
};

class StoneState : public CellState {
public:
    bool isAlive() const override { return false; }
    bool isStone() const override { return true; }
    std::string symbol() const override { return "⬛ "; }
};

extern const AliveState ALIVE_STATE;
extern const DeadState  DEAD_STATE;
extern const StoneState STONE_STATE;

#endif
