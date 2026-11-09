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
        void setPosition(int posX, int posY) 
        { 
            x = posX; y = posY; 
        }
        void getPosition(int &posX, int &posY) 
        { 
            posX = x; posY = y;
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
        Cell** grid; // 2D array of cells represented as a 1D array
        
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
        Organism(std::string n, int l, int w) : name(n), length(l), width(w), grid(NULL), step(0) {};
        ~Organism() // libère la mémoire
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
            for (int i = 0; i < length; ++i) 
            {
                for (int j = 0; j < width; ++j) 
                {
                    grid[i * width + j]->statesAround(); // calcul des nouveaux états
                }
            }
            for (int i = 0; i < length; ++i) // 2e passe, application simultanée
            {
                for (int j = 0; j < width; ++j) 
                {
                    grid[i * width + j]->applyNext();
                }
            }
        } 
        std::string getName() const 
        { 
            return name; 
        }
        int getLength() const 
        { 
            return length; 
        }
        int getWidth() const 
        { 
            return width; 
        }
        int getStep() const 
        { 
            return step; 
        }
        void tab()
        {
            for (int i = 0; i < length; ++i) 
            {
                for (int j = 0; j < width; ++j) 
                {
                    if (grid[i * width + j]->isAlive())
                    {
                        std::cout << "1 "; // Print 1 for alive cells
                    } 
                    else 
                    {
                        std::cout << "0 "; // Print 0 for dead cells
                    }
                }
                std::cout << std::endl;
            }
        }
};

int main() {
    std::srand(std::time(nullptr));
    Organism o("test", 10, 10);
    o.init_organism();
    o.tab();
    for (int i = 0; i < 5; ++i) {
        o.nextstep();
        std::cout << "\nEtape " << o.getStep() << "\n";
        o.tab();
    }
    return 0;
};