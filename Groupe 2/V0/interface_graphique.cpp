// Graphic interface for the Game of Life (SFML 2.x).
//
// This file reuses the Cell and Organism classes of Jeu_de_la_vie_avec_sauvegarde.cpp.
// The console main() of that file is disabled thanks to JEU_DE_LA_VIE_SANS_MAIN.
//
// Install SFML (once):  sudo apt install libsfml-dev
// Compile:              g++ -std=c++17 -Wall interface_graphique.cpp -o jeu_de_la_vie_gui -lsfml-graphics -lsfml-window -lsfml-system
// Run:                  ./jeu_de_la_vie_gui

#define JEU_DE_LA_VIE_SANS_MAIN
#include "Jeu_de_la_vie_avec_sauvegarde.cpp"

#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>

namespace {

// ---------- Layout and settings ----------
const unsigned WINDOW_W = 1100;
const unsigned WINDOW_H = 760;

const float GRID_AREA_X = 20.f;
const float GRID_AREA_Y = 20.f;
const float GRID_AREA_W = 760.f;
const float GRID_AREA_H = 720.f;

const float PANEL_X = 800.f;
const float PANEL_W = 280.f;

const int MIN_DIM = 5;
const int MAX_DIM = 200;
const int MIN_DELAY = 10;    // ms between two steps
const int MAX_DELAY = 2000;

// ---------- Colors ----------
const sf::Color BG(30, 30, 36);
const sf::Color PANEL_BG(45, 45, 55);
const sf::Color ALIVE(80, 200, 120);
const sf::Color DEAD(60, 60, 72);
const sf::Color TEXT(230, 230, 230);
const sf::Color TEXT_DIM(150, 150, 160);
const sf::Color ACCENT(240, 190, 80);
const sf::Color BTN(70, 90, 140);
const sf::Color BTN_HOVER(95, 120, 185);
const sf::Color BTN_DISABLED(65, 65, 75);

// SFML expects sf::String: convert UTF-8 so that accents (é, è...) are displayed correctly
sf::String utf8(const std::string& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

struct Button {
    sf::FloatRect box;
    std::string label;
    std::function<void()> action;
    bool enabled;
};

} // namespace

class GameOfLifeGUI {
    private :
        enum class Screen { Settings, Simulation };

        sf::RenderWindow window;
        sf::Font font;
        Screen screen;

        // Choices made on the settings screen
        int rows;      // Organism "length"
        int cols;      // Organism "width"
        int delayMs;   // time between two automatic steps

        // Simulation state
        std::unique_ptr<Organism> organism;
        int viewIndex;     // step currently displayed (can be an old saved step)
        bool playing;
        bool finished;     // a state already seen has been reached
        int cycleStart;    // first step equal to the final state
        sf::Clock clock;
        sf::Time accumulated;

        // ---------- Text / drawing helpers ----------
        void drawText(const std::string& s, float x, float y, unsigned size, sf::Color color = TEXT)
        {
            sf::Text t(utf8(s), font, size);
            t.setFillColor(color);
            t.setPosition(x, y);
            window.draw(t);
        }

        void drawCenteredText(const std::string& s, float cx, float cy, unsigned size, sf::Color color = TEXT)
        {
            sf::Text t(utf8(s), font, size);
            t.setFillColor(color);
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
            t.setPosition(cx, cy);
            window.draw(t);
        }

        void drawButton(const Button& b, sf::Vector2f mouse)
        {
            sf::RectangleShape r(sf::Vector2f(b.box.width, b.box.height));
            r.setPosition(b.box.left, b.box.top);
            if (!b.enabled)
                r.setFillColor(BTN_DISABLED);
            else if (b.box.contains(mouse))
                r.setFillColor(BTN_HOVER);
            else
                r.setFillColor(BTN);
            window.draw(r);
            drawCenteredText(b.label, b.box.left + b.box.width / 2.f, b.box.top + b.box.height / 2.f, 16,
                             b.enabled ? TEXT : TEXT_DIM);
        }

        // ---------- Game actions ----------
        void startSimulation()
        {
            organism.reset(new Organism("interface", rows, cols));
            organism->init_organism();
            viewIndex = 0;
            playing = false;
            finished = false;
            cycleStart = -1;
            accumulated = sf::Time::Zero;
            screen = Screen::Simulation;
        }

        void advanceOneStep()
        {
            if (finished)
                return;
            organism->nextstep();
            int step = organism->getStep();
            viewIndex = step;
            // Same stop condition as the console version
            if (step >= 2 && organism->alreadySeen())
            {
                finished = true;
                playing = false;
                std::vector<bool> last = organism->getGrid(step);
                for (int s = 0; s < step; ++s)
                {
                    if (organism->getGrid(s) == last)
                    {
                        cycleStart = s;
                        break;
                    }
                }
            }
        }

        void togglePlay()
        {
            if (finished)
                return;
            playing = !playing;
            if (playing)
                viewIndex = organism->getStep(); // follow the live evolution
            accumulated = sf::Time::Zero;
        }

        void viewPrevious()
        {
            playing = false;
            if (viewIndex > 0)
                viewIndex--;
        }

        void viewNext()
        {
            playing = false;
            if (viewIndex < organism->getStep())
                viewIndex++;
        }

        // ---------- Buttons of each screen ----------
        std::vector<Button> settingsButtons()
        {
            std::vector<Button> b;
            const float bw = 55.f, bh = 40.f;
            const float x0 = 560.f;
            float y = 200.f;

            auto addRow = [&](int* value, int small, int big, int lo, int hi) {
                b.push_back({sf::FloatRect(x0, y, bw, bh), "-" + std::to_string(big),
                             [=] { *value = clamp(*value - big, lo, hi); }, *value > lo});
                b.push_back({sf::FloatRect(x0 + 60, y, bw, bh), "-" + std::to_string(small),
                             [=] { *value = clamp(*value - small, lo, hi); }, *value > lo});
                b.push_back({sf::FloatRect(x0 + 230, y, bw, bh), "+" + std::to_string(small),
                             [=] { *value = clamp(*value + small, lo, hi); }, *value < hi});
                b.push_back({sf::FloatRect(x0 + 290, y, bw, bh), "+" + std::to_string(big),
                             [=] { *value = clamp(*value + big, lo, hi); }, *value < hi});
                y += 90.f;
            };
            addRow(&rows, 1, 10, MIN_DIM, MAX_DIM);
            addRow(&cols, 1, 10, MIN_DIM, MAX_DIM);
            addRow(&delayMs, 10, 100, MIN_DELAY, MAX_DELAY);

            b.push_back({sf::FloatRect(WINDOW_W / 2.f - 120.f, 520.f, 240.f, 55.f), "Démarrer",
                         [this] { startSimulation(); }, true});
            return b;
        }

        std::vector<Button> simulationButtons()
        {
            std::vector<Button> b;
            const float x = PANEL_X + 20.f;
            const float w = PANEL_W - 40.f;
            const float half = (w - 10.f) / 2.f;
            const float h = 40.f;
            const int step = organism->getStep();

            b.push_back({sf::FloatRect(x, 230.f, w, h), playing ? "Pause" : "Lecture",
                         [this] { togglePlay(); }, !finished});
            b.push_back({sf::FloatRect(x, 280.f, w, h), "Étape suivante",
                         [this] { playing = false; advanceOneStep(); }, !finished});
            b.push_back({sf::FloatRect(x, 345.f, half, h), "< Précédente",
                         [this] { viewPrevious(); }, viewIndex > 0});
            b.push_back({sf::FloatRect(x + half + 10.f, 345.f, half, h), "Suivante >",
                         [this] { viewNext(); }, viewIndex < step});
            b.push_back({sf::FloatRect(x, 410.f, half, h), "Plus lent",
                         [this] { delayMs = clamp(delayMs * 2, MIN_DELAY, MAX_DELAY); }, delayMs < MAX_DELAY});
            b.push_back({sf::FloatRect(x + half + 10.f, 410.f, half, h), "Plus rapide",
                         [this] { delayMs = clamp(delayMs / 2, MIN_DELAY, MAX_DELAY); }, delayMs > MIN_DELAY});
            b.push_back({sf::FloatRect(x, 475.f, w, h), "Nouvelle grille",
                         [this] { startSimulation(); }, true});
            b.push_back({sf::FloatRect(x, 525.f, w, h), "Retour aux réglages",
                         [this] { playing = false; screen = Screen::Settings; }, true});
            return b;
        }

        std::vector<Button> currentButtons()
        {
            return screen == Screen::Settings ? settingsButtons() : simulationButtons();
        }

        // ---------- Events ----------
        void handleClick(sf::Vector2f mouse)
        {
            std::vector<Button> buttons = currentButtons();
            for (const Button& b : buttons)
            {
                if (b.enabled && b.box.contains(mouse))
                {
                    b.action();
                    return;
                }
            }
        }

        void handleKey(sf::Keyboard::Key key)
        {
            if (screen == Screen::Settings)
            {
                if (key == sf::Keyboard::Enter)
                    startSimulation();
                else if (key == sf::Keyboard::Escape)
                    window.close();
                return;
            }
            switch (key)
            {
                case sf::Keyboard::Space:  togglePlay(); break;
                case sf::Keyboard::N:      playing = false; advanceOneStep(); break;
                case sf::Keyboard::Left:   viewPrevious(); break;
                case sf::Keyboard::Right:  viewNext(); break;
                case sf::Keyboard::R:      startSimulation(); break;
                case sf::Keyboard::Escape: playing = false; screen = Screen::Settings; break;
                default: break;
            }
        }

        // ---------- Rendering ----------
        void drawSettings(sf::Vector2f mouse)
        {
            drawCenteredText("Jeu de la vie", WINDOW_W / 2.f, 70.f, 44, ACCENT);
            drawCenteredText("Choisissez les dimensions de la grille et la vitesse d'évolution",
                             WINDOW_W / 2.f, 125.f, 18, TEXT_DIM);

            const char* labels[] = {"Nombre de lignes", "Nombre de colonnes", "Délai entre étapes (ms)"};
            int values[] = {rows, cols, delayMs};
            for (int k = 0; k < 3; ++k)
            {
                float y = 200.f + k * 90.f;
                drawText(labels[k], 200.f, y + 8.f, 22);
                sf::RectangleShape box(sf::Vector2f(100.f, 40.f));
                box.setPosition(560.f + 120.f, y);
                box.setFillColor(PANEL_BG);
                window.draw(box);
                drawCenteredText(std::to_string(values[k]), 560.f + 170.f, y + 20.f, 22, ACCENT);
            }
            drawCenteredText("Grille de " + std::to_string(rows) + " x " + std::to_string(cols) + " = "
                             + std::to_string(rows * cols) + " cellules",
                             WINDOW_W / 2.f, 480.f, 18, TEXT_DIM);

            for (const Button& b : settingsButtons())
                drawButton(b, mouse);

            drawCenteredText("Entrée : démarrer    Échap : quitter", WINDOW_W / 2.f, 700.f, 15, TEXT_DIM);
        }

        void drawGrid(const std::vector<bool>& cells)
        {
            float cell = std::min(GRID_AREA_W / cols, GRID_AREA_H / rows);
            float gap = cell >= 4.f ? 1.f : 0.f;
            float offX = GRID_AREA_X + (GRID_AREA_W - cell * cols) / 2.f;
            float offY = GRID_AREA_Y + (GRID_AREA_H - cell * rows) / 2.f;

            // One vertex array (2 triangles per cell) is much faster than one shape per cell
            sf::VertexArray va(sf::Triangles, static_cast<std::size_t>(rows) * cols * 6);
            std::size_t v = 0;
            for (int i = 0; i < rows; ++i)
            {
                for (int j = 0; j < cols; ++j)
                {
                    sf::Color c = cells[i * cols + j] ? ALIVE : DEAD;
                    float x1 = offX + j * cell, y1 = offY + i * cell;
                    float x2 = x1 + cell - gap, y2 = y1 + cell - gap;
                    sf::Vector2f p[6] = {{x1, y1}, {x2, y1}, {x2, y2}, {x1, y1}, {x2, y2}, {x1, y2}};
                    for (int k = 0; k < 6; ++k)
                    {
                        va[v].position = p[k];
                        va[v].color = c;
                        ++v;
                    }
                }
            }
            window.draw(va);
        }

        void drawSimulation(sf::Vector2f mouse)
        {
            std::vector<bool> cells = organism->getGrid(viewIndex);
            drawGrid(cells);

            sf::RectangleShape panel(sf::Vector2f(PANEL_W, static_cast<float>(WINDOW_H)));
            panel.setPosition(PANEL_X, 0.f);
            panel.setFillColor(PANEL_BG);
            window.draw(panel);

            int alive = 0;
            for (bool c : cells)
                if (c) alive++;
            const int step = organism->getStep();
            const float x = PANEL_X + 20.f;

            drawText("Jeu de la vie", x, 20.f, 28, ACCENT);
            drawText("Grille : " + std::to_string(rows) + " x " + std::to_string(cols), x, 70.f, 17);
            drawText("Étape affichée : " + std::to_string(viewIndex) + " / " + std::to_string(step), x, 98.f, 17);
            drawText("Cellules vivantes : " + std::to_string(alive), x, 126.f, 17);
            drawText("Délai : " + std::to_string(delayMs) + " ms", x, 154.f, 17);

            std::string status;
            sf::Color statusColor = TEXT;
            if (finished)
            {
                statusColor = ACCENT;
                int period = step - cycleStart;
                if (period == 1)
                    status = "Stable depuis l'étape " + std::to_string(cycleStart);
                else
                    status = "Cycle de période " + std::to_string(period) + "\n(depuis l'étape " + std::to_string(cycleStart) + ")";
            }
            else if (playing)
                status = "En cours...";
            else
                status = "En pause";
            drawText(status, x, 182.f, 17, statusColor);

            for (const Button& b : simulationButtons())
                drawButton(b, mouse);

            drawText("Raccourcis :\n"
                     "Espace : lecture / pause\n"
                     "N : étape suivante\n"
                     "Gauche / Droite : historique\n"
                     "R : nouvelle grille\n"
                     "Échap : réglages",
                     x, 600.f, 14, TEXT_DIM);
        }

    public :
        GameOfLifeGUI()
            : window(sf::VideoMode(WINDOW_W, WINDOW_H), "Jeu de la vie", sf::Style::Titlebar | sf::Style::Close),
              screen(Screen::Settings), rows(30), cols(30), delayMs(150),
              viewIndex(0), playing(false), finished(false), cycleStart(-1)
        {
            window.setFramerateLimit(60);
        }

        bool loadFont()
        {
            const char* candidates[] = {
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
                "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                "C:/Windows/Fonts/arial.ttf",
                "/System/Library/Fonts/Supplemental/Arial.ttf",
            };
            for (const char* path : candidates)
                if (font.loadFromFile(path))
                    return true;
            return false;
        }

        void run()
        {
            while (window.isOpen())
            {
                sf::Event event;
                while (window.pollEvent(event))
                {
                    if (event.type == sf::Event::Closed)
                        window.close();
                    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                        handleClick(sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)));
                    else if (event.type == sf::Event::KeyPressed)
                        handleKey(event.key.code);
                }

                // Automatic evolution
                sf::Time elapsed = clock.restart();
                if (screen == Screen::Simulation && playing)
                {
                    accumulated += elapsed;
                    if (accumulated >= sf::milliseconds(delayMs))
                    {
                        accumulated = sf::Time::Zero;
                        advanceOneStep();
                    }
                }

                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                window.clear(BG);
                if (screen == Screen::Settings)
                    drawSettings(mouse);
                else
                    drawSimulation(mouse);
                window.display();
            }
        }
};

int main()
{
    std::srand(std::time(nullptr));
    GameOfLifeGUI gui;
    if (!gui.loadFont())
    {
        std::cerr << "Impossible de charger une police (installez fonts-dejavu)." << std::endl;
        return 1;
    }
    gui.run();
    return 0;
}
