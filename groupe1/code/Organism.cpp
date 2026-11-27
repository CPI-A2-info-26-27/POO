#include "Organism.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>

Organism::Organism(const std::string& n, int l, int w, const Rules& r, bool useStones)
    : name(n), length(l), width(w), withStones(useStones), rules(r), step(0) {}

Organism::~Organism()
{
    for (Cell* c : grid)
        delete c;
}

void Organism::linkNeighbors(int row, int col)
{
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
        {
            if (dr == 0 && dc == 0)
                continue;
            int r = row + dr;
            int c = col + dc;
            if (r >= 0 && r < length && c >= 0 && c < width)
                at(row, col)->addNeighbor(at(r, c));
        }
}

void Organism::buildGrid(const std::vector<const CellState*>& states)
{
    step = 0;
    grid.clear();
    for (const CellState* s : states)
        grid.push_back(new Cell(s));

    for (int row = 0; row < length; ++row)
        for (int col = 0; col < width; ++col)
            linkNeighbors(row, col);

    saveGrid();
}

void Organism::init_organism()
{
    std::vector<const CellState*> states;
    for (int k = 0; k < length * width; ++k)
    {
        int roll = std::rand() % 9;
        if (withStones && roll == 0)
            states.push_back(&STONE_STATE);
        else if (std::rand() % 2 == 0)
            states.push_back(&ALIVE_STATE);
        else
            states.push_back(&DEAD_STATE);
    }
    buildGrid(states);
}

bool Organism::init_from_file(const std::string& path, std::string& error)
{
    std::ifstream file(path);
    if (!file)
    {
        error = "Impossible d'ouvrir le fichier : " + path;
        return false;
    }

    int rows = 0, cols = 0;
    if (!(file >> rows >> cols) || rows <= 0 || cols <= 0)
    {
        error = "La première ligne doit contenir : lignes colonnes (ex : 10 20)";
        return false;
    }

    const int expected = rows * cols;
    std::vector<const CellState*> states;
    char c;
    while ((int)states.size() < expected && file >> c)
    {
        switch (c)
        {
            case '0': states.push_back(&DEAD_STATE); break;
            case '1': states.push_back(&ALIVE_STATE); break;
            case '2':
                if (withStones) states.push_back(&STONE_STATE);
                else            states.push_back(&DEAD_STATE);
                break;
            default:
                error = std::string("Caractère inattendu dans le fichier : '") + c
                      + "' (seuls 0, 1 et 2 sont permis)";
                return false;
        }
    }

    if ((int)states.size() < expected)
    {
        error = "Il manque des cellules : " + std::to_string(expected) + " attendues, "
              + std::to_string(states.size()) + " trouvées";
        return false;
    }

    length = rows;
    width = cols;
    buildGrid(states);
    return true;
}

void Organism::nextstep()
{
    step++;
    for (Cell* c : grid)
        c->computeNext(rules);
    for (Cell* c : grid)
        c->applyNext();
    saveGrid();
}

void Organism::saveGrid()
{
    std::vector<bool> snapshot;
    for (const Cell* c : grid)
        snapshot.push_back(c->isAlive());
    history.push_back(snapshot);
}

std::vector<bool> Organism::getStones() const
{
    std::vector<bool> stones;
    for (const Cell* c : grid)
        stones.push_back(c->getState()->isStone());
    return stones;
}

bool Organism::alreadySeen() const
{
    auto last = history.end() - 1;
    return std::find(history.begin(), last, history[step]) != last;
}

void Organism::tab() const
{
    for (int row = 0; row < length; ++row)
    {
        for (int col = 0; col < width; ++col)
            std::cout << at(row, col)->getState()->symbol();
        std::cout << '\n';
    }
    std::cout << std::flush;
}
