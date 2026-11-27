// Unit tests of the game logic (Organism, Cell, Rules). No window is needed.
//
// A unit test checks the grid computed at iteration t: it is parameterised by a starting grid,
// a number of iterations and the grid expected after these iterations.
//
// This file is included by affichage_main.cpp. It can also be compiled ALONE, without SFML,
// to run only the built-in tests:
//   g++ -std=c++17 -Wall tests_unitaires.cpp -o tests_unitaires
//   ./tests_unitaires

#include "JDLV.cpp" // Organism, Cell, Rules... (game logic)
#include <iostream>
#include <string>
#include <vector>

// Core check: computes "iterations" steps on the organism, then compares the grid obtained
// with the expected grid (true = alive). Returns true if they are identical.
bool checkGrid(Organism& organism, const std::vector<bool>& expected, int iterations)
{
    for (int k = 0; k < iterations; k++)
    {
        organism.nextstep();
    }
    return organism.getGrid(organism.getStep()) == expected;
}

// Prints a grid (true = alive) in the terminal, one line per row: '#' = alive, '.' = dead.
void printGrid(const std::vector<bool>& cells, int width)
{
    for (int k = 0; k < (int)cells.size(); k++)
    {
        std::cout << (cells[k] ? '#' : '.');
        if ((k + 1) % width == 0)
        {
            std::cout << '\n';
        }
    }
}

// One unit test written directly in the code. The grids are written row by row with '0' and '1',
// like the input files, so that they are easy to read.
class GridTest {
    public :
        GridTest(std::string n, std::vector<std::string> start, int it, std::vector<std::string> expected,
                 bool torus = false)
            : name(n), startRows(start), iterations(it), expectedRows(expected), tauric(torus) {}

        const std::string& getName() const
        {
            return name;
        }

        // Builds the organism from the starting grid, computes the iterations and compares.
        // On failure, the expected and obtained grids are printed.
        bool run(const Rules& rules) const
        {
            int length = (int)startRows.size();
            int width = (int)startRows[0].size();
            Organism organism(name, length, width, rules);
            organism.setTauric(tauric);
            organism.init_from_cells(toCells(startRows));

            std::vector<bool> expected;
            for (int v : toCells(expectedRows))
            {
                expected.push_back(v == 1);
            }
            if (checkGrid(organism, expected, iterations))
            {
                return true;
            }
            std::cout << "  Grille attendue :\n";
            printGrid(expected, width);
            std::cout << "  Grille obtenue :\n";
            printGrid(organism.getGrid(organism.getStep()), width);
            return false;
        }

    private :
        static std::vector<int> toCells(const std::vector<std::string>& rows)
        {
            std::vector<int> cells;
            for (const std::string& row : rows)
            {
                for (char c : row)
                {
                    cells.push_back(c == '1' ? 1 : 0);
                }
            }
            return cells;
        }

        std::string name;
        std::vector<std::string> startRows;
        int iterations;
        std::vector<std::string> expectedRows;
        bool tauric;
};

// Built-in tests: well-known patterns whose evolution is known in advance.
// Returns the number of failed tests (0 = everything is correct).
int runUnitTests()
{
    ConwayRules rules;
    std::vector<GridTest> tests = {
        // A lonely cell dies of isolation after 1 iteration.
        GridTest("Cellule isolée : meurt après 1 itération",
                 {"000", "010", "000"}, 1,
                 {"000", "000", "000"}),

        // Block: still life, it never changes.
        GridTest("Bloc : stable après 10 itérations",
                 {"0000", "0110", "0110", "0000"}, 10,
                 {"0000", "0110", "0110", "0000"}),

        // Blinker: oscillator of period 2 (horizontal <-> vertical).
        GridTest("Clignotant : vertical après 1 itération",
                 {"00000", "00000", "01110", "00000", "00000"}, 1,
                 {"00000", "00100", "00100", "00100", "00000"}),
        GridTest("Clignotant : revenu à l'état initial après 2 itérations",
                 {"00000", "00000", "01110", "00000", "00000"}, 2,
                 {"00000", "00000", "01110", "00000", "00000"}),

        // Birth: a dead cell with exactly 3 alive neighbours becomes alive (the L becomes a block).
        GridTest("Naissance : 3 voisines vivantes donnent une cellule vivante",
                 {"0000", "0110", "0100", "0000"}, 1,
                 {"0000", "0110", "0110", "0000"}),

        // Glider: after 4 iterations it has moved one cell down and one cell right.
        GridTest("Planeur : décalé d'une case en diagonale après 4 itérations",
                 {"010000", "001000", "111000", "000000", "000000", "000000"}, 4,
                 {"000000", "001000", "000100", "011100", "000000", "000000"}),

        // Torus: on an 8 x 8 torus, the glider crosses the edges and is back at its start after 32 iterations.
        GridTest("Grille torique : planeur revenu à son départ après 32 itérations",
                 {"01000000", "00100000", "11100000", "00000000",
                  "00000000", "00000000", "00000000", "00000000"}, 32,
                 {"01000000", "00100000", "11100000", "00000000",
                  "00000000", "00000000", "00000000", "00000000"}, true),
    };

    int failed = 0;
    for (const GridTest& t : tests)
    {
        bool ok = t.run(rules);
        std::cout << (ok ? "[OK]     " : "[ECHEC]  ") << t.getName() << std::endl;
        if (!ok)
        {
            failed++;
        }
    }
    std::cout << (tests.size() - failed) << " / " << tests.size() << " tests réussis." << std::endl;
    return failed;
}

// Test parameterised by two files: the starting grid, the expected grid, and the number of iterations.
// Returns true if the grid computed after "iterations" steps is the expected one.
bool testFromFiles(const std::string& startPath, const std::string& expectedPath, int iterations, bool tauric)
{
    ConwayRules rules;
    std::string error;

    Organism organism("test", 0, 0, rules, true);
    organism.setTauric(tauric);
    if (!organism.init_from_file(startPath, error))
    {
        std::cout << "[ECHEC]  " << error << std::endl;
        return false;
    }

    Organism expected("attendu", 0, 0, rules, true); // only used to read the expected grid
    if (!expected.init_from_file(expectedPath, error))
    {
        std::cout << "[ECHEC]  " << error << std::endl;
        return false;
    }
    if (expected.getLength() != organism.getLength() || expected.getWidth() != organism.getWidth())
    {
        std::cout << "[ECHEC]  Les deux grilles n'ont pas la même taille." << std::endl;
        return false;
    }

    bool ok = checkGrid(organism, expected.getGrid(0), iterations);
    std::cout << (ok ? "[OK]     " : "[ECHEC]  ") << startPath << " après " << iterations
              << " itération(s) = " << expectedPath << std::endl;
    if (!ok)
    {
        std::cout << "  Grille attendue :\n";
        printGrid(expected.getGrid(0), expected.getWidth());
        std::cout << "  Grille obtenue :\n";
        printGrid(organism.getGrid(organism.getStep()), organism.getWidth());
    }
    return ok;
}

// When this file is compiled alone, it has its own main() that runs the built-in tests.
int main()
{
    return runUnitTests() == 0 ? 0 : 1;
}
