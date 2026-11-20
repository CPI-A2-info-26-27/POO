// Graphic interface for the Game of Life (SFML 2.x), clean "Apple-like" style.
//
// The program has 3 screens:
//   1. Mode       : choose "Console" or "Graphique"
//   2. Réglages   : random grid or .txt file, stones yes / no, speed
//   3. Simulation : only in graphic mode.
//                   In console mode the window closes and the game runs in the terminal.
//
// This file reuses the classes of JDLV_save.cpp.
// The console main() of that file is disabled thanks to JEU_DE_LA_VIE_SANS_MAIN.
//
// Install SFML (once):  sudo apt install libsfml-dev
// Compile:              g++ -std=c++17 -Wall affichage.cpp -o jeu_de_la_vie_gui -lsfml-graphics -lsfml-window -lsfml-system
// Run:                  ./jeu_de_la_vie_gui
//
// How the buttons work ("immediate mode"):
// every frame, each draw function draws its widget AND checks if it was clicked.
// Example:   if (button("Démarrer", box, true)) start();
// So a button is just one line: no list of buttons to keep up to date.

#define JEU_DE_LA_VIE_SANS_MAIN
#include "JDLV_save.cpp"

#include <SFML/Graphics.hpp>
#include <cmath>
#include <memory>

namespace {

// ---------- Window and limits ----------
const unsigned WINDOW_W = 1100;
const unsigned WINDOW_H = 760;

const int MIN_DIM = 5;
const int MAX_DIM = 200;
const int MIN_DELAY = 10;    // ms between two steps
const int MAX_DELAY = 2000;

// Settings screen: one centered column of white cards
const float COL_W = 640.f;
const float COL_X = (WINDOW_W - COL_W) / 2.f;
const float ROW_H = 56.f;   // height of one row in a card

// ---------- Colors (inspired by the iOS / macOS system colors) ----------
const sf::Color BACKGROUND(242, 242, 247);   // light grey behind everything
const sf::Color CARD(255, 255, 255);         // white blocks
const sf::Color SEPARATOR(225, 225, 230);    // thin lines between rows
const sf::Color LABEL(28, 28, 30);           // main text (almost black)
const sf::Color SECONDARY(142, 142, 147);    // grey text
const sf::Color FILL(229, 229, 234);         // grey of the controls (segmented, stepper, switch OFF)
const sf::Color BLUE(0, 122, 255);           // accent color (buttons, links)
const sf::Color BLUE_HOVER(0, 100, 225);
const sf::Color BLUE_LIGHT(0, 122, 255, 26); // transparent blue for secondary buttons
const sf::Color BLUE_LIGHT_HOVER(0, 122, 255, 46);
const sf::Color GREEN(52, 199, 89);
const sf::Color ORANGE(255, 149, 0);
const sf::Color RED(255, 59, 48);
const sf::Color WHITE(255, 255, 255);

// Colors of the cells
const sf::Color ALIVE_CELL(52, 199, 89);     // green
const sf::Color DEAD_CELL(236, 236, 241);    // very light grey
const sf::Color STONE_CELL(99, 99, 102);     // dark grey

// SFML expects sf::String: convert UTF-8 so that accents (é, è...) are displayed correctly
sf::String utf8(const std::string& s)
{
    return sf::String::fromUtf8(s.begin(), s.end());
}

int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

// Same color with another transparency (0 = invisible, 255 = opaque)
sf::Color withAlpha(sf::Color c, sf::Uint8 alpha)
{
    c.a = alpha;
    return c;
}

} // namespace

class GameOfLifeGUI {
    private :
        enum class Screen { Mode, Settings, Simulation };
        enum class Mode { Console, Graphic };
        enum class Align { Left, Center, Right };
        enum class Field { None, Path, Rows, Cols }; // the text field where the user is typing

        sf::RenderWindow window;
        sf::Font font;
        Screen screen;

        // ---------- Mouse, for the current frame ----------
        sf::Vector2f mouse;     // where the mouse is (for the hover effect)
        sf::Vector2f clickPos;  // where the click happened
        bool clicked;           // true only during the frame of the click
        bool shiftDown;         // Shift held during the click -> steppers move by 10
        sf::Clock blinkClock;   // blinking cursor of the text field

        // ---------- Choices of the Mode and Settings screens ----------
        Mode mode;
        bool fromFile;          // true: grid read from a .txt file / false: random grid
        std::string filePath;   // only used when fromFile is true
        Field focused;          // text field that receives the keyboard (None = no field)
        bool useStones;
        std::string rowsText;   // typed in the "Lignes" field (only used for a random grid)
        std::string colsText;   // typed in the "Colonnes" field
        int rows;               // read from rowsText / colsText when the game starts
        int cols;
        int delayMs;            // time between two steps
        std::string errorMessage; // shown in red on the settings screen

        bool consoleRequested;  // true when the window must close to run the game in the terminal

        // ---------- Simulation state ----------
        ConwayRules rules; // must be declared before organism: Organism keeps a reference to it
        std::unique_ptr<Organism> organism;
        std::vector<bool> stones; // position of the stones (they never move)
        int viewIndex;     // step currently displayed (can be an old saved step)
        bool playing;
        bool finished;     // a state already seen has been reached
        int cycleStart;    // first step equal to the final state
        sf::Clock clock;
        sf::Time accumulated;

        // =====================================================================
        //  Basic drawing
        // =====================================================================

        // Rectangle with rounded corners (SFML has no ready-made shape for that).
        // We build a polygon: a quarter of circle in each corner.
        void drawRoundedRect(sf::FloatRect box, float radius, sf::Color color)
        {
            const int POINTS_PER_CORNER = 10;
            const float PI = 3.14159265f;
            radius = std::min(radius, std::min(box.width, box.height) / 2.f);

            // Center of the circle of each corner, clockwise, starting top right
            sf::Vector2f centers[4] = {
                {box.left + box.width - radius, box.top + radius},
                {box.left + box.width - radius, box.top + box.height - radius},
                {box.left + radius,             box.top + box.height - radius},
                {box.left + radius,             box.top + radius}
            };

            sf::ConvexShape shape(POINTS_PER_CORNER * 4);
            int n = 0;
            for (int corner = 0; corner < 4; ++corner)
            {
                for (int p = 0; p < POINTS_PER_CORNER; ++p)
                {
                    float degrees = -90.f + corner * 90.f + p * 90.f / (POINTS_PER_CORNER - 1);
                    float angle = degrees * PI / 180.f;
                    shape.setPoint(n, sf::Vector2f(centers[corner].x + radius * std::cos(angle),
                                                   centers[corner].y + radius * std::sin(angle)));
                    n++;
                }
            }
            shape.setFillColor(color);
            window.draw(shape);
        }

        // Same box, smaller by "margin" pixels on each side
        sf::FloatRect shrink(sf::FloatRect box, float margin)
        {
            return sf::FloatRect(box.left + margin, box.top + margin, box.width - 2 * margin, box.height - 2 * margin);
        }

        // Writes a text. x depends on align (left edge, center or right edge), cy = vertical center.
        void drawText(const std::string& s, float x, float cy, unsigned size, sf::Color color,
                      Align align = Align::Left, bool bold = false)
        {
            sf::Text t(utf8(s), font, size);
            t.setFillColor(color);
            if (bold)
                t.setStyle(sf::Text::Bold);

            sf::FloatRect b = t.getLocalBounds();
            float originX = b.left;
            if (align == Align::Center)
                originX += b.width / 2.f;
            else if (align == Align::Right)
                originX += b.width;

            // Vertical center = middle of a capital letter, so that all texts of a row line up
            sf::Text ref("H", font, size);
            sf::FloatRect r = ref.getLocalBounds();
            t.setOrigin(originX, r.top + r.height / 2.f);

            t.setPosition(std::round(x), std::round(cy)); // round: sharper text
            window.draw(t);
        }

        float textWidth(const std::string& s, unsigned size, bool bold = false)
        {
            sf::Text t(utf8(s), font, size);
            if (bold)
                t.setStyle(sf::Text::Bold);
            return t.getLocalBounds().width;
        }

        // =====================================================================
        //  Mouse helpers
        // =====================================================================

        bool isHovered(sf::FloatRect box)
        {
            return box.contains(mouse);
        }

        // True if the user clicked in the box during this frame.
        // The click is then "used": one click can only trigger one action.
        bool clickedOn(sf::FloatRect box)
        {
            if (clicked && box.contains(clickPos))
            {
                clicked = false;
                return true;
            }
            return false;
        }

        // =====================================================================
        //  Widgets (each one draws itself and handles its click)
        // =====================================================================

        // Rounded button. primary = filled blue, otherwise light blue with blue text.
        // Returns true when clicked.
        bool button(const std::string& label, sf::FloatRect box, bool primary, bool enabled = true)
        {
            sf::Color fill;
            sf::Color text;
            if (!enabled)
            {
                fill = FILL;
                text = SECONDARY;
            }
            else if (primary)
            {
                fill = isHovered(box) ? BLUE_HOVER : BLUE;
                text = WHITE;
            }
            else
            {
                fill = isHovered(box) ? BLUE_LIGHT_HOVER : BLUE_LIGHT;
                text = BLUE;
            }
            drawRoundedRect(box, 12.f, fill);
            drawText(label, box.left + box.width / 2.f, box.top + box.height / 2.f,
                     primary ? 17 : 15, text, Align::Center, primary);
            return enabled && clickedOn(box);
        }

        // Blue text that can be clicked (like "‹ Retour" on iPhone)
        bool link(const std::string& label, float x, float cy)
        {
            const unsigned size = 17;
            sf::FloatRect box(x - 6.f, cy - 16.f, textWidth(label, size) + 12.f, 32.f);
            drawText(label, x, cy, size, isHovered(box) ? withAlpha(BLUE, 150) : BLUE);
            return clickedOn(box);
        }

        // iOS switch: green when ON, grey when OFF. "right" = right edge of the switch.
        void toggleSwitch(bool& value, float right, float cy)
        {
            sf::FloatRect track(right - 51.f, cy - 15.5f, 51.f, 31.f);
            drawRoundedRect(track, 15.5f, value ? GREEN : FILL);

            float knobX = value ? track.left + track.width - 15.5f : track.left + 15.5f;
            sf::CircleShape shadow(14.f);
            shadow.setOrigin(14.f, 14.f);
            shadow.setPosition(knobX, cy + 1.f);
            shadow.setFillColor(sf::Color(0, 0, 0, 30));
            window.draw(shadow);

            sf::CircleShape knob(13.5f);
            knob.setOrigin(13.5f, 13.5f);
            knob.setPosition(knobX, cy);
            knob.setFillColor(WHITE);
            window.draw(knob);

            if (clickedOn(track))
                value = !value;
        }

        // iOS "segmented control": two choices side by side, the selected one is white.
        // rightSelected = false -> left choice, true -> right choice.
        void segmentedControl(bool& rightSelected, const std::string& left, const std::string& right, sf::FloatRect box)
        {
            drawRoundedRect(box, 9.f, FILL);
            float half = box.width / 2.f;
            sf::FloatRect leftBox(box.left, box.top, half, box.height);
            sf::FloatRect rightBox(box.left + half, box.top, half, box.height);

            drawRoundedRect(shrink(rightSelected ? rightBox : leftBox, 2.f), 7.f, CARD);
            float cy = box.top + box.height / 2.f;
            drawText(left, leftBox.left + half / 2.f, cy, 14, LABEL, Align::Center, !rightSelected);
            drawText(right, rightBox.left + half / 2.f, cy, 14, LABEL, Align::Center, rightSelected);

            if (clickedOn(leftBox))
                rightSelected = false;
            if (clickedOn(rightBox))
                rightSelected = true;
        }

        // iOS stepper: [ − | + ]. One click = step, Shift + click = 10 x step.
        void stepper(int& value, int step, int lo, int hi, float right, float cy)
        {
            sf::FloatRect box(right - 94.f, cy - 16.f, 94.f, 32.f);
            drawRoundedRect(box, 8.f, FILL);
            sf::FloatRect minusBox(box.left, box.top, 47.f, 32.f);
            sf::FloatRect plusBox(box.left + 47.f, box.top, 47.f, 32.f);

            sf::RectangleShape line(sf::Vector2f(1.f, 18.f)); // small line between - and +
            line.setPosition(box.left + 47.f, cy - 9.f);
            line.setFillColor(sf::Color(200, 200, 205));
            window.draw(line);

            bool canMinus = value > lo;
            bool canPlus = value < hi;
            drawText("−", minusBox.left + 23.5f, cy, 22, canMinus ? LABEL : withAlpha(SECONDARY, 120), Align::Center);
            drawText("+", plusBox.left + 23.5f, cy, 22, canPlus ? LABEL : withAlpha(SECONDARY, 120), Align::Center);

            int amount = shiftDown ? step * 10 : step;
            if (canMinus && clickedOn(minusBox))
                value = clamp(value - amount, lo, hi);
            if (canPlus && clickedOn(plusBox))
                value = clamp(value + amount, lo, hi);
        }

        // Text of the field that has the focus (nullptr if no field has it)
        std::string* focusedText()
        {
            switch (focused)
            {
                case Field::Path: return &filePath;
                case Field::Rows: return &rowsText;
                case Field::Cols: return &colsText;
                default:          return nullptr;
            }
        }

        // Field where the user types a text (path of the .txt file, number of rows / columns)
        void textField(std::string& text, const std::string& placeholder, sf::FloatRect box, Field id)
        {
            bool isFocused = (focused == id);
            if (isFocused)
                drawRoundedRect(shrink(box, -2.f), 10.f, withAlpha(BLUE, 90)); // blue ring = "I am typing here"
            drawRoundedRect(box, 8.f, FILL);

            float cy = box.top + box.height / 2.f;
            float maxWidth = box.width - 24.f;
            if (text.empty() && !isFocused)
            {
                drawText(placeholder, box.left + 12.f, cy, 15, SECONDARY);
            }
            else
            {
                // If the path is too long, only show its end (the file name is the useful part)
                std::string shown = text;
                while (!shown.empty() && textWidth(shown, 15) > maxWidth)
                {
                    shown.erase(0, 1);
                    while (!shown.empty() && (shown[0] & 0xC0) == 0x80) // do not cut an accented letter in two
                        shown.erase(0, 1);
                }
                drawText(shown, box.left + 12.f, cy, 15, LABEL);

                // Blinking cursor at the end of the text
                if (isFocused && blinkClock.getElapsedTime().asMilliseconds() % 1000 < 500)
                {
                    sf::RectangleShape cursor(sf::Vector2f(2.f, 20.f));
                    cursor.setPosition(box.left + 13.f + textWidth(shown, 15), cy - 10.f);
                    cursor.setFillColor(BLUE);
                    window.draw(cursor);
                }
            }
            if (clickedOn(box))
            {
                focused = id;
                blinkClock.restart();
            }
        }

        // Small grey title above a card
        void sectionHeader(const std::string& title, float cy)
        {
            drawText(title, COL_X + 16.f, cy, 13, SECONDARY);
        }

        // White card with "nbRows" rows and thin lines between them
        void drawCard(float top, int nbRows)
        {
            drawRoundedRect(sf::FloatRect(COL_X, top, COL_W, nbRows * ROW_H), 12.f, CARD);
            for (int k = 1; k < nbRows; ++k)
            {
                sf::RectangleShape line(sf::Vector2f(COL_W - 16.f, 1.f));
                line.setPosition(COL_X + 16.f, top + k * ROW_H);
                line.setFillColor(SEPARATOR);
                window.draw(line);
            }
        }

        // Vertical center of the row number "index" of a card starting at "top"
        float rowCenter(float top, int index)
        {
            return top + index * ROW_H + ROW_H / 2.f;
        }

        // =====================================================================
        //  Game actions
        // =====================================================================

        // Creates the organism with the chosen settings. Returns false (and sets errorMessage) on error.
        bool createOrganism()
        {
            errorMessage = "";
            if (fromFile && filePath.empty())
            {
                errorMessage = "Indiquez le chemin du fichier .txt";
                return false;
            }
            if (!fromFile)
            {
                // Read the size typed in the fields (only digits can be typed, at most 3)
                const std::string range = " (entre " + std::to_string(MIN_DIM) + " et " + std::to_string(MAX_DIM) + ")";
                rows = rowsText.empty() ? 0 : std::stoi(rowsText);
                cols = colsText.empty() ? 0 : std::stoi(colsText);
                if (rows < MIN_DIM || rows > MAX_DIM)
                {
                    errorMessage = "Nombre de lignes invalide" + range;
                    return false;
                }
                if (cols < MIN_DIM || cols > MAX_DIM)
                {
                    errorMessage = "Nombre de colonnes invalide" + range;
                    return false;
                }
            }
            organism.reset(new Organism("interface", rows, cols, rules, useStones));
            if (fromFile)
            {
                std::string error;
                if (!organism->init_from_file(filePath, error))
                {
                    errorMessage = error;
                    organism.reset();
                    return false;
                }
            }
            else
            {
                organism->init_organism();
            }
            stones = organism->getStones();
            return true;
        }

        // "Démarrer" button of the settings screen
        void start()
        {
            focused = Field::None;
            if (mode == Mode::Console)
            {
                if (createOrganism())
                    consoleRequested = true; // the window closes at the end of this frame
            }
            else
            {
                startSimulation();
            }
        }

        // Graphic mode: (re)creates the grid and shows the simulation screen
        void startSimulation()
        {
            if (!createOrganism())
            {
                screen = Screen::Settings; // show the error
                return;
            }
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

        void goToSettings()
        {
            playing = false;
            screen = Screen::Settings;
        }

        // =====================================================================
        //  Screen 1 : choice of the mode
        // =====================================================================

        // Big clickable card of the mode screen. Returns true when clicked.
        bool modeCard(sf::FloatRect box, const std::string& title, const std::string& line1,
                      const std::string& line2, bool graphicIcon)
        {
            bool hover = isHovered(box);
            if (hover)
                drawRoundedRect(shrink(box, -3.f), 21.f, BLUE); // blue border when the mouse is over
            drawRoundedRect(box, 18.f, CARD);

            float cx = box.left + box.width / 2.f;

            // Icon: rounded square of 84 x 84
            sf::FloatRect icon(cx - 42.f, box.top + 44.f, 84.f, 84.f);
            if (graphicIcon)
            {
                // A small 3 x 3 grid with a "glider"
                drawRoundedRect(icon, 20.f, sf::Color(232, 245, 236));
                const bool glider[9] = {false, true, false,
                                        false, false, true,
                                        true, true, true};
                for (int k = 0; k < 9; ++k)
                {
                    sf::FloatRect cell(icon.left + 15.f + (k % 3) * 19.f, icon.top + 15.f + (k / 3) * 19.f, 16.f, 16.f);
                    drawRoundedRect(cell, 4.f, glider[k] ? GREEN : sf::Color(210, 230, 216));
                }
            }
            else
            {
                // A small black terminal
                drawRoundedRect(icon, 20.f, LABEL);
                drawText(">_", cx, icon.top + 42.f, 30, GREEN, Align::Center, true);
            }

            drawText(title, cx, box.top + 172.f, 24, LABEL, Align::Center, true);
            drawText(line1, cx, box.top + 210.f, 15, SECONDARY, Align::Center);
            drawText(line2, cx, box.top + 232.f, 15, SECONDARY, Align::Center);
            drawText("Choisir ›", cx, box.top + box.height - 34.f, 16, hover ? BLUE : withAlpha(BLUE, 170), Align::Center);
            return clickedOn(box);
        }

        void drawModeScreen()
        {
            const float cx = WINDOW_W / 2.f;
            drawText("Jeu de la vie", cx, 140.f, 40, LABEL, Align::Center, true);
            drawText("Comment voulez-vous afficher la simulation ?", cx, 192.f, 18, SECONDARY, Align::Center);

            const float cardW = 320.f;
            const float cardH = 320.f;
            const float gap = 32.f;
            const float top = 250.f;
            sf::FloatRect consoleBox(cx - cardW - gap / 2.f, top, cardW, cardH);
            sf::FloatRect graphicBox(cx + gap / 2.f, top, cardW, cardH);

            if (modeCard(consoleBox, "Console", "Le jeu s'affiche", "dans le terminal", false))
            {
                mode = Mode::Console;
                screen = Screen::Settings;
            }
            if (modeCard(graphicBox, "Graphique", "Fenêtre interactive avec", "lecture, pause et historique", true))
            {
                mode = Mode::Graphic;
                screen = Screen::Settings;
            }

            drawText("C : console    ·    G : graphique    ·    Échap : quitter", cx, 700.f, 14, SECONDARY, Align::Center);
        }

        // =====================================================================
        //  Screen 2 : settings
        // =====================================================================

        void drawSettingsScreen()
        {
            const float right = COL_X + COL_W - 16.f; // right edge of the controls inside a card

            // A click anywhere leaves the text field (the field itself gives the focus back if clicked)
            if (clicked)
                focused = Field::None;

            // ----- Top bar -----
            if (link("‹ Mode", COL_X, 40.f))
                screen = Screen::Mode;
            drawText("Réglages", COL_X, 88.f, 34, LABEL, Align::Left, true);
            drawText(mode == Mode::Console ? "Mode console" : "Mode graphique",
                     COL_X + COL_W, 92.f, 15, SECONDARY, Align::Right);

            float y = 145.f; // "cursor": goes down as we draw the sections

            // ----- GRILLE -----
            sectionHeader("GRILLE", y);
            y += 18.f;
            bool wasFromFile = fromFile;
            drawCard(y, fromFile ? 2 : 3);

            drawText("Initialisation", COL_X + 16.f, rowCenter(y, 0), 17, LABEL);
            segmentedControl(fromFile, "Générée", "Fichier .txt", sf::FloatRect(right - 260.f, rowCenter(y, 0) - 16.f, 260.f, 32.f));
            if (fromFile != wasFromFile)
                errorMessage = "";

            if (fromFile)
            {
                // Only the path: the size of the grid is written in the file
                drawText("Chemin", COL_X + 16.f, rowCenter(y, 1), 17, LABEL);
                textField(filePath, "ex : grilles/planeur.txt",
                          sf::FloatRect(COL_X + 130.f, rowCenter(y, 1) - 17.f, right - COL_X - 130.f, 34.f), Field::Path);
                y += 2 * ROW_H;
                drawText("1re ligne : lignes colonnes, puis 0 = morte, 1 = vivante, 2 = pierre",
                         COL_X + 16.f, y + 18.f, 13, SECONDARY);
                y += 30.f;
            }
            else
            {
                drawText("Lignes", COL_X + 16.f, rowCenter(y, 1), 17, LABEL);
                textField(rowsText, "ex : 30",
                          sf::FloatRect(right - 140.f, rowCenter(y, 1) - 17.f, 140.f, 34.f), Field::Rows);

                drawText("Colonnes", COL_X + 16.f, rowCenter(y, 2), 17, LABEL);
                textField(colsText, "ex : 30",
                          sf::FloatRect(right - 140.f, rowCenter(y, 2) - 17.f, 140.f, 34.f), Field::Cols);
                y += 3 * ROW_H;
                drawText("Entre " + std::to_string(MIN_DIM) + " et " + std::to_string(MAX_DIM) + " lignes / colonnes",
                         COL_X + 16.f, y + 18.f, 13, SECONDARY);
                y += 30.f;
            }
            y += 34.f;

            // ----- OPTIONS -----
            sectionHeader("OPTIONS", y);
            y += 18.f;
            drawCard(y, 1);
            drawText("Pierres", COL_X + 16.f, rowCenter(y, 0) - 9.f, 17, LABEL);
            drawText(fromFile ? "Les cases « 2 » du fichier deviennent des pierres"
                              : "Environ 1 case sur 9 devient une pierre fixe",
                     COL_X + 16.f, rowCenter(y, 0) + 12.f, 13, SECONDARY);
            toggleSwitch(useStones, right, rowCenter(y, 0));
            y += ROW_H + 34.f;

            // ----- ANIMATION -----
            sectionHeader("ANIMATION", y);
            y += 18.f;
            drawCard(y, 1);
            drawText("Délai entre deux étapes", COL_X + 16.f, rowCenter(y, 0), 17, LABEL);
            drawText(std::to_string(delayMs) + " ms", right - 110.f, rowCenter(y, 0), 17, SECONDARY, Align::Right);
            stepper(delayMs, 10, MIN_DELAY, MAX_DELAY, right, rowCenter(y, 0));

            // ----- Error + start button -----
            if (!errorMessage.empty())
                drawText(errorMessage, WINDOW_W / 2.f, 610.f, 15, RED, Align::Center);

            sf::FloatRect startBox(WINDOW_W / 2.f - 150.f, 635.f, 300.f, 52.f);
            if (button(mode == Mode::Console ? "Lancer dans le terminal" : "Démarrer", startBox, true))
                start();

            drawText("Entrée : démarrer    ·    Échap : retour    ·    Maj + clic sur − / + : par 10",
                     WINDOW_W / 2.f, 722.f, 13, SECONDARY, Align::Center);
        }

        // =====================================================================
        //  Screen 3 : simulation (graphic mode)
        // =====================================================================

        void drawGrid(const std::vector<bool>& cells, sf::FloatRect area)
        {
            const int nRows = organism->getLength();
            const int nCols = organism->getWidth();
            float cell = std::min(area.width / nCols, area.height / nRows);
            float gap = cell >= 4.f ? 1.f : 0.f;
            float offX = area.left + (area.width - cell * nCols) / 2.f;
            float offY = area.top + (area.height - cell * nRows) / 2.f;

            // One vertex array (2 triangles per cell) is much faster than one shape per cell
            sf::VertexArray va(sf::Triangles, static_cast<std::size_t>(nRows) * nCols * 6);
            std::size_t v = 0;
            for (int i = 0; i < nRows; ++i)
            {
                for (int j = 0; j < nCols; ++j)
                {
                    int k = i * nCols + j;
                    sf::Color c = stones[k] ? STONE_CELL : (cells[k] ? ALIVE_CELL : DEAD_CELL);
                    float x1 = offX + j * cell;
                    float y1 = offY + i * cell;
                    float x2 = x1 + cell - gap;
                    float y2 = y1 + cell - gap;
                    sf::Vector2f p[6] = {{x1, y1}, {x2, y1}, {x2, y2}, {x1, y1}, {x2, y2}, {x1, y2}};
                    for (int n = 0; n < 6; ++n)
                    {
                        va[v].position = p[n];
                        va[v].color = c;
                        ++v;
                    }
                }
            }
            window.draw(va);
        }

        // One line "label ....... value" of the information list
        void infoRow(const std::string& label, const std::string& value, float left, float right, float cy, bool lineBelow)
        {
            drawText(label, left, cy, 15, SECONDARY);
            drawText(value, right, cy, 15, LABEL, Align::Right);
            if (lineBelow)
            {
                sf::RectangleShape line(sf::Vector2f(right - left, 1.f));
                line.setPosition(left, cy + 16.f);
                line.setFillColor(SEPARATOR);
                window.draw(line);
            }
        }

        void drawSimulationScreen()
        {
            std::vector<bool> cells = organism->getGrid(viewIndex); // copy: the organism may be replaced by a button below

            // ----- Grid (left) -----
            sf::FloatRect gridCard(24.f, 24.f, 736.f, 712.f);
            drawRoundedRect(gridCard, 18.f, CARD);
            drawGrid(cells, shrink(gridCard, 20.f));

            // ----- Side panel (right) -----
            sf::FloatRect panel(784.f, 24.f, 292.f, 712.f);
            drawRoundedRect(panel, 18.f, CARD);
            const float left = panel.left + 20.f;
            const float right = panel.left + panel.width - 20.f;
            const float w = right - left;
            const float half = (w - 10.f) / 2.f;
            const int step = organism->getStep();

            drawText("Simulation", left, 62.f, 26, LABEL, Align::Left, true);

            // Status "pill" (colored label)
            std::string status;
            sf::Color statusColor;
            if (finished)
            {
                int period = step - cycleStart;
                status = period == 1 ? "Stable depuis l'étape " + std::to_string(cycleStart)
                                     : "Cycle de période " + std::to_string(period);
                statusColor = ORANGE;
            }
            else if (playing)
            {
                status = "En cours";
                statusColor = GREEN;
            }
            else
            {
                status = "En pause";
                statusColor = SECONDARY;
            }
            sf::FloatRect pill(left, 90.f, textWidth(status, 13, true) + 24.f, 26.f);
            drawRoundedRect(pill, 13.f, withAlpha(statusColor, 40));
            drawText(status, pill.left + 12.f, pill.top + 13.f, 13, statusColor, Align::Left, true);

            // Information list
            int alive = 0;
            int nbStones = 0;
            for (std::size_t k = 0; k < cells.size(); ++k)
            {
                if (cells[k])
                    alive++;
                if (stones[k])
                    nbStones++;
            }
            infoRow("Étape", std::to_string(viewIndex) + " / " + std::to_string(step), left, right, 148.f, true);
            infoRow("Cellules vivantes", std::to_string(alive), left, right, 182.f, true);
            infoRow("Pierres", useStones ? std::to_string(nbStones) : "Non", left, right, 216.f, true);
            infoRow("Grille", std::to_string(organism->getLength()) + " × " + std::to_string(organism->getWidth()),
                    left, right, 250.f, true);
            infoRow("Délai", std::to_string(delayMs) + " ms", left, right, 284.f, false);

            // Buttons
            std::string playLabel = finished ? "Terminé" : (playing ? "Pause" : "Lecture");
            if (button(playLabel, sf::FloatRect(left, 318.f, w, 50.f), true, !finished))
                togglePlay();
            if (button("Étape suivante", sf::FloatRect(left, 380.f, w, 40.f), false, !finished))
            {
                playing = false;
                advanceOneStep();
            }
            if (button("‹ Précédente", sf::FloatRect(left, 428.f, half, 40.f), false, viewIndex > 0))
                viewPrevious();
            if (button("Suivante ›", sf::FloatRect(left + half + 10.f, 428.f, half, 40.f), false, viewIndex < step))
                viewNext();
            if (button("Plus lent", sf::FloatRect(left, 476.f, half, 40.f), false, delayMs < MAX_DELAY))
                delayMs = clamp(delayMs * 2, MIN_DELAY, MAX_DELAY);
            if (button("Plus rapide", sf::FloatRect(left + half + 10.f, 476.f, half, 40.f), false, delayMs > MIN_DELAY))
                delayMs = clamp(delayMs / 2, MIN_DELAY, MAX_DELAY);
            if (button(fromFile ? "Recommencer" : "Nouvelle grille", sf::FloatRect(left, 536.f, w, 40.f), false))
                startSimulation();
            if (button("Réglages", sf::FloatRect(left, 584.f, w, 40.f), false))
                goToSettings();

            // Keyboard shortcuts
            const char* shortcuts[] = {"Espace : lecture / pause", "N : étape suivante",
                                       "Gauche / Droite : historique", "R : nouvelle grille", "Échap : réglages"};
            for (int k = 0; k < 5; ++k)
                drawText(shortcuts[k], left, 648.f + k * 17.f, 12, SECONDARY);
        }

        // =====================================================================
        //  Keyboard
        // =====================================================================

        void handleKey(const sf::Event::KeyEvent& key)
        {
            if (screen == Screen::Mode)
            {
                if (key.code == sf::Keyboard::C)
                {
                    mode = Mode::Console;
                    screen = Screen::Settings;
                }
                else if (key.code == sf::Keyboard::G)
                {
                    mode = Mode::Graphic;
                    screen = Screen::Settings;
                }
                else if (key.code == sf::Keyboard::Escape)
                    window.close();
                return;
            }

            if (screen == Screen::Settings)
            {
                if (key.code == sf::Keyboard::Enter)
                    start();
                else if (key.code == sf::Keyboard::Escape)
                {
                    if (focused != Field::None)
                        focused = Field::None;
                    else
                        screen = Screen::Mode;
                }
                else if (key.code == sf::Keyboard::Tab && !fromFile)
                {
                    // Tab: go from "Lignes" to "Colonnes" and back
                    focused = (focused == Field::Rows) ? Field::Cols : Field::Rows;
                    blinkClock.restart();
                }
                else if (focused != Field::None && key.code == sf::Keyboard::V && key.control)
                {
                    // Ctrl + V: paste the text (without line breaks)
                    for (char c : sf::Clipboard::getString().toAnsiString())
                        if (c != '\n' && c != '\r')
                            addCharacter(static_cast<unsigned char>(c));
                }
                return;
            }

            // Simulation screen
            switch (key.code)
            {
                case sf::Keyboard::Space:  togglePlay(); break;
                case sf::Keyboard::N:      playing = false; advanceOneStep(); break;
                case sf::Keyboard::Left:   viewPrevious(); break;
                case sf::Keyboard::Right:  viewNext(); break;
                case sf::Keyboard::R:      startSimulation(); break;
                case sf::Keyboard::Escape: goToSettings(); break;
                default: break;
            }
        }

        // Adds one character to the focused field.
        // The "Lignes" / "Colonnes" fields only accept digits, at most 3 of them.
        void addCharacter(sf::Uint32 unicode)
        {
            std::string* text = focusedText();
            if (text == nullptr || unicode < 32 || unicode == 127) // not Enter, Tab, Escape...
                return;
            if (focused == Field::Rows || focused == Field::Cols)
            {
                if (unicode < '0' || unicode > '9' || text->size() >= 3)
                    return;
                *text += static_cast<char>(unicode);
                return;
            }
            std::basic_string<sf::Uint8> bytes = sf::String(unicode).toUtf8();
            text->append(bytes.begin(), bytes.end());
        }

        // A character typed on the keyboard (used only by the text fields)
        void handleTextInput(sf::Uint32 unicode)
        {
            std::string* text = focusedText();
            if (screen != Screen::Settings || text == nullptr)
                return;
            blinkClock.restart(); // the cursor stays visible while typing
            if (unicode == 8) // Backspace: erase the last character
            {
                while (!text->empty() && (text->back() & 0xC0) == 0x80) // end of an accented letter
                    text->pop_back();
                if (!text->empty())
                    text->pop_back();
            }
            else
            {
                addCharacter(unicode);
            }
        }

    public :
        GameOfLifeGUI()
            : window(sf::VideoMode(WINDOW_W, WINDOW_H), "Jeu de la vie", sf::Style::Titlebar | sf::Style::Close,
                     sf::ContextSettings(0, 0, 8)), // 8 = antialiasing: smooth rounded corners
              screen(Screen::Mode), clicked(false), shiftDown(false),
              mode(Mode::Graphic), fromFile(false), filePath("exemple.txt"), focused(Field::None), useStones(false),
              rowsText("30"), colsText("30"), rows(30), cols(30), delayMs(150), consoleRequested(false),
              viewIndex(0), playing(false), finished(false), cycleStart(-1)
        {
            window.setFramerateLimit(60);
        }

        bool loadFont()
        {
            const char* candidates[] = {
                "/usr/share/fonts/truetype/ubuntu/UbuntuSans[wdth,wght].ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/TTF/DejaVuSans.ttf",
                "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
                "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
                "C:/Windows/Fonts/segoeui.ttf",
                "/System/Library/Fonts/SFNS.ttf",
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
                // ----- 1. Events -----
                clicked = false;
                sf::Event event;
                while (window.pollEvent(event))
                {
                    if (event.type == sf::Event::Closed)
                        window.close();
                    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
                    {
                        clicked = true;
                        clickPos = sf::Vector2f(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                        shiftDown = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
                    }
                    else if (event.type == sf::Event::KeyPressed)
                        handleKey(event.key);
                    else if (event.type == sf::Event::TextEntered)
                        handleTextInput(event.text.unicode);
                }
                if (!window.isOpen())
                    break;

                // ----- 2. Automatic evolution -----
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

                // ----- 3. Drawing (and clicks on the widgets) -----
                mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                window.clear(BACKGROUND);
                if (screen == Screen::Mode)
                    drawModeScreen();
                else if (screen == Screen::Settings)
                    drawSettingsScreen();
                else
                    drawSimulationScreen();
                window.display();

                if (consoleRequested)
                    window.close();
            }

            // Console mode: the window is closed, the game continues in the terminal
            if (consoleRequested && organism)
                runConsoleSimulation(*organism, delayMs);
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
