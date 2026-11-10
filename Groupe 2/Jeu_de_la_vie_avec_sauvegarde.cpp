#include <iostream>
#include <string>
#include <list>
#include <cstdlib>
#include <ctime>

class Cell {
    private : 
        bool state; // true for alive, false for dead
        bool next;  // state on the next step
        int x, y; // Position of the cell in the grid
        std::list<Cell*> neighbors; // List of pins of the neighboring cells

    public :
        Cell(bool s, int posX, int posY) : state(s), x(posX), y(posY) {}; // Constructor initializes the cell with a given state
        bool isAlive() 
        {
            return state; 
        }
        void setAlive() 
        {
            state = true; 
        } 
        void setDEAD () 
        {
            state = false; 
        }
        void setNextAlive() 
        {
            next = true; 
        } 
        void setNextDEAD () 
        {
            next = false; 
        }
        bool compareState() 
        {
            return state == next;
        }
        void addNeighbor(Cell* c)
        {
            neighbors.push_back(c);
        }
        void applyNext()
        {
            state = next;
        }
        void statesAround()
        {
            int aliveCount = 0;
            for (Cell* neighbor : neighbors) 
            {
                if (neighbor->isAlive()) 
                {
                    aliveCount++;
                }
            }
            next = state;
            if (state)
            {
                if (not(aliveCount == 2 || aliveCount == 3))
                {
                    setNextDEAD(); // Next cell dies
                }
            }
            else 
            {
                if (aliveCount == 3) 
                {
                    setNextAlive(); // Next cell becomes alive
                } 
            }
        };
};

class Organism {
    private :
        std::string name;
        int length;
        int width;
        int aliveCount;
        int deadCount;
        bool stable;
        Cell** grid; // 2D array of cells represented as a 1D array
        std::list<std::pair<int, Cell**>> save; // List of grid per step for saving the state of the organism
        
        void cellsAroundExist(int i, int j)
        {
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int neighborX = i + dx;
                    int neighborY = j + dy;
                    if (not (dx == 0 && dy == 0))
                    {
                        if (neighborX >= 0 && neighborX < length && neighborY >= 0 && neighborY < width) 
                        {
                            grid[i * width + j]->addNeighbor(grid[neighborX * width + neighborY]);
                        }
                    }
                }
            }
        }

        public :
        int step;
        Organism(std::string n, int l, int w) : name(n), length(l), width(w), stable(false), grid(NULL), step(0) {};
        ~Organism()
        {
            if (grid != NULL)
            {
                for (int k = 0; k < length * width; ++k)
                {
                    delete grid[k];
                }
                delete[] grid;
            }
        }
        void init_organism() 
        { 
            step = 0; 
            grid = new Cell*[length * width];
            for (int i = 0; i < length; ++i) 
            {
                for (int j = 0; j < width; ++j) 
                {
                    grid[i * width + j] = new Cell(rand() % 2 == 0, i, j);
                }
            }
            for (int i = 0; i < length; ++i) // liaison des voisines
            {
                for (int j = 0; j < width; ++j) 
                {
                    cellsAroundExist(i, j);
                }
            }
        }
        void nextstep() 
        { 
            step++; // Increment the step count
            stable = true; // Assume the organism is stable until proven otherwise
            for (int i = 0; i < length; ++i) 
            {
                for (int j = 0; j < width; ++j) 
                {
                    grid[i * width + j]->statesAround(); // calcul of the new states
                }
            }
            save.push_back(std::make_pair(step, grid));
            for (int i = 0; i < length; ++i) // 2nd loop to apply the new states and check for stability
            {
                for (int j = 0; j < width; ++j) 
                {
                    if (not grid[i * width + j]->compareState())
                    {
                        stable = false;
                    }
                    grid[i * width + j]->applyNext();
                }
            }
        } 
        int getStep() const 
        { 
            return step; 
        }
        Organism** getGrid(int step)  
        { 
            return save[step].second; 
        }
        bool isStable() const 
        {
            return stable;
        }
        void tab()
        {
            for (int i = 0; i < length; ++i) 
            {
                for (int j = 0; j < width; ++j) 
                {
                    if (grid[i * width + j]->isAlive())
                    {
                        std::cout << "🟩 "; // Print 1 for alive cells
                    } 
                    else 
                    {
                        std::cout << "🟥 "; // Print 0 for dead cells
                    }
                }
                std::cout << std::endl;
            }
        }
};

int main() {
    std::srand(std::time(nullptr));
    Organism o("test", 16, 16);
    o.init_organism();
    o.tab();
    while (!o.isStable() || o.getStep() < 100)
    {
        o.nextstep();
        std::cout << "\nEtape " << o.getStep() << "\n";
        o.tab();
    }
    std::cout << "L'organisme est stable, fin de la simulation." << std::endl;
    return 0;
};