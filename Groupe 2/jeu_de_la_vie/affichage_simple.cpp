// Compact version of affichage.cpp: same features, simple graphic interface (window = whole screen).
//   Screen 1 (settings)   : mode, grid (random or .txt file), stones, delay, "Demarrer" button
//   Screen 2 (simulation) : grid + buttons (play/pause, step, history, speed, restart, settings)
//   In console mode the window closes and the game runs in the terminal.
//
// Compile: g++ -std=c++17 -Wall affichage_simple.cpp -o jdlv_simple -lsfml-graphics -lsfml-window -lsfml-system
// Run:     ./jdlv_simple

#define JEU_DE_LA_VIE_SANS_MAIN
#include "JDLV_save.cpp"

#include <SFML/Graphics.hpp>
#include <memory>

class SimpleGUI {
    private :
        sf::RenderWindow window;
        sf::Font font;
        sf::Vector2f click;     // position of the click of this frame
        bool clicked = false;
        bool simulation = false; // false = settings screen, true = simulation screen

        // Settings
        bool graphic = true, fromFile = false, useStones = false, typing = false, console = false;
        std::string path = "exemple.txt", error;
        int rows = 30, cols = 30, delayMs = 150;

        // Simulation
        ConwayRules rules;
        std::unique_ptr<Organism> o;
        std::vector<bool> stones;
        int view = 0, cycleStart = -1;
        bool playing = false;
        sf::Clock clock;

        void text(const std::string& s, float x, float y, sf::Color c = sf::Color::Black)
        {
            sf::Text t(sf::String::fromUtf8(s.begin(), s.end()), font, 16);
            t.setFillColor(c);
            t.setPosition(x, y);
            window.draw(t);
        }

        // Plain rectangle button: grey, blue when "selected", light when disabled. Returns true when clicked.
        bool button(const std::string& label, float x, float y, float w, bool selected = false, bool enabled = true)
        {
            sf::RectangleShape r(sf::Vector2f(w, 28));
            r.setPosition(x, y);
            r.setFillColor(!enabled ? sf::Color(235, 235, 235) : selected ? sf::Color(100, 150, 230) : sf::Color(210, 210, 210));
            r.setOutlineColor(sf::Color(150, 150, 150));
            r.setOutlineThickness(1);
            window.draw(r);
            text(label, x + 8, y + 3, enabled ? sf::Color::Black : sf::Color(160, 160, 160));
            if (enabled && clicked && r.getGlobalBounds().contains(click))
            {
                clicked = false;
                return true;
            }
            return false;
        }

        // "label  [-] value [+]"
        void stepper(const std::string& label, int& value, int step, int lo, int hi, float y)
        {
            text(label, 20, y + 3);
            if (button("-", 150, y, 28)) value = std::max(lo, value - step);
            text(std::to_string(value), 190, y + 3);
            if (button("+", 250, y, 28)) value = std::min(hi, value + step);
        }

        bool createOrganism()
        {
            error = "";
            o.reset(new Organism("simple", rows, cols, rules, useStones));
            if (fromFile && !o->init_from_file(path, error))
            {
                o.reset();
                return false;
            }
            if (!fromFile)
                o->init_organism();
            stones = o->getStones();
            view = 0;
            cycleStart = -1;
            playing = false;
            return true;
        }

        // One step forward, detects the end (state already seen) like the console version
        void advance()
        {
            if (cycleStart >= 0)
                return;
            o->nextstep();
            view = o->getStep();
            if (view >= 2 && o->alreadySeen())
            {
                playing = false;
                for (int k = 0; k < view && cycleStart < 0; ++k)
                    if (o->getGrid(k) == o->getGrid(view))
                        cycleStart = k;
            }
        }

        void drawSettings()
        {
            if (clicked)
                typing = false; // a click elsewhere leaves the path field
            text("Jeu de la vie - Reglages", 20, 15);

            text("Mode", 20, 63);
            if (button("Console", 150, 60, 100, !graphic)) graphic = false;
            if (button("Graphique", 260, 60, 100, graphic)) graphic = true;

            text("Grille", 20, 103);
            if (button("Aleatoire", 150, 100, 100, !fromFile)) fromFile = false;
            if (button("Fichier", 260, 100, 100, fromFile)) fromFile = true;

            if (fromFile)
            {
                text("Chemin", 20, 143);
                if (button(path + (typing ? "_" : ""), 150, 140, 300, typing)) typing = true;
            }
            else
            {
                stepper("Lignes", rows, 5, 5, 200, 140);
                stepper("Colonnes", cols, 5, 5, 200, 180);
            }

            text("Pierres", 20, 223);
            if (button("Non", 150, 220, 60, !useStones)) useStones = false;
            if (button("Oui", 220, 220, 60, useStones)) useStones = true;

            stepper("Delai (ms)", delayMs, 50, 10, 2000, 260);

            if (button("Demarrer", 150, 320, 120) && createOrganism())
            {
                if (graphic)
                    simulation = true;
                else
                    console = true; // window closes, the game runs in the terminal
            }
            text(error, 20, 370, sf::Color::Red);
        }

        void drawSimulation()
        {
            // Grid in the biggest square that fits on the left (200 px are kept for the panel)
            const float area = std::min(window.getSize().x - 200.f, window.getSize().y - 20.f);
            const int L = o->getLength(), W = o->getWidth();
            const float cell = area / std::max(L, W);
            const std::vector<bool>& cells = o->getGrid(view);
            sf::RectangleShape r(sf::Vector2f(std::max(1.f, cell - 1), std::max(1.f, cell - 1)));
            for (int k = 0; k < L * W; ++k)
            {
                r.setFillColor(stones[k] ? sf::Color(90, 90, 90) : cells[k] ? sf::Color(50, 180, 80) : sf::Color(225, 225, 225));
                r.setPosition(10 + (k % W) * cell, 10 + (k / W) * cell);
                window.draw(r);
            }

            // Side panel
            const float x = 25 + cell * W;
            const int step = o->getStep();
            text("Etape " + std::to_string(view) + " / " + std::to_string(step), x, 10);
            text("Delai : " + std::to_string(delayMs) + " ms", x, 32);
            std::string status = playing ? "En cours" : "En pause";
            if (cycleStart >= 0)
                status = step - cycleStart == 1 ? "Stable (etape " + std::to_string(cycleStart) + ")"
                                                : "Cycle de periode " + std::to_string(step - cycleStart);
            text(status, x, 54, cycleStart >= 0 ? sf::Color(220, 120, 0) : sf::Color::Black);

            if (button(playing ? "Pause" : "Lecture", x, 90, 150, false, cycleStart < 0))
            {
                playing = !playing;
                view = step;
            }
            if (button("Etape suivante", x, 125, 150, false, cycleStart < 0)) { playing = false; advance(); }
            if (button("<", x, 160, 70, false, view > 0)) { playing = false; view--; }
            if (button(">", x + 80, 160, 70, false, view < step)) { playing = false; view++; }
            if (button("Plus lent", x, 195, 150)) delayMs = std::min(2000, delayMs * 2);
            if (button("Plus rapide", x, 230, 150)) delayMs = std::max(10, delayMs / 2);
            if (button("Recommencer", x, 280, 150)) createOrganism();
            if (button("Reglages", x, 315, 150)) { playing = false; simulation = false; }
        }

    public :
        // Window as big as the screen (and resizable)
        SimpleGUI() : window(sf::VideoMode::getDesktopMode(), "Jeu de la vie", sf::Style::Default)
        {
            window.setFramerateLimit(60);
        }

        bool loadFont()
        {
            for (const char* p : {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                  "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                  "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                                  "C:/Windows/Fonts/arial.ttf"})
                if (font.loadFromFile(p))
                    return true;
            return false;
        }

        void run()
        {
            while (window.isOpen() && !console)
            {
                clicked = false;
                sf::Event e;
                while (window.pollEvent(e))
                {
                    if (e.type == sf::Event::Closed)
                        window.close();
                    else if (e.type == sf::Event::Resized) // keep 1 unit = 1 pixel, the grid adapts to the new size
                        window.setView(sf::View(sf::FloatRect(0, 0, e.size.width, e.size.height)));
                    else if (e.type == sf::Event::MouseButtonPressed)
                    {
                        clicked = true;
                        click = sf::Vector2f(e.mouseButton.x, e.mouseButton.y);
                    }
                    else if (e.type == sf::Event::TextEntered && typing && !simulation)
                    {
                        if (e.text.unicode == 8 && !path.empty()) path.pop_back(); // Backspace
                        else if (e.text.unicode >= 32 && e.text.unicode < 127) path += (char)e.text.unicode;
                    }
                }

                if (simulation && playing && clock.getElapsedTime().asMilliseconds() >= delayMs)
                {
                    clock.restart();
                    advance();
                }

                window.clear(sf::Color::White);
                if (simulation)
                    drawSimulation();
                else
                    drawSettings();
                window.display();
            }
            if (console)
            {
                window.close();
                runConsoleSimulation(*o, delayMs);
            }
        }
};

int main()
{
    std::srand(std::time(nullptr));
    SimpleGUI gui;
    if (!gui.loadFont())
    {
        std::cerr << "Impossible de charger une police (installez fonts-dejavu)." << std::endl;
        return 1;
    }
    gui.run();
    return 0;
}
