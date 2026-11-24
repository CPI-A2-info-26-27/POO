// Graphic interface for the Game of Life (SFML 2.x), object-oriented version.
//
// The file is organised from the smallest pieces to the biggest:
//   1. Theme + drawing helpers : colors, rounded rectangles, text
//   2. Widgets                 : Button, InputBox -> TextBox / NumberBox
//   3. Settings                : every choice of the user, in one plain struct
//   4. Page                    : abstract class, one subclass per screen
//                                ModePage -> SettingsPage -> SimulationPage
//   5. App                     : owns the window, the settings, the organism and the current page
//   6. main()
//
// The window takes the size of the screen. The button in the top right corner switches to full screen
// (Escape leaves full screen). Since the size of the window can change, nothing has a fixed position:
// every frame, each page places its widgets with layout(width, height).
//
// The game logic (Cell, Organism, Rules...) comes from JDLV_taurique.cpp.
// Its console main() is disabled thanks to JEU_DE_LA_VIE_SANS_MAIN.
//
// Install SFML (once):  sudo apt install libsfml-dev
// Compile:              g++ -std=c++17 -Wall affichage_main.cpp -o jeu_de_la_vie_gui -lsfml-graphics -lsfml-window -lsfml-system
// Run:                  ./jeu_de_la_vie_gui

#define JEU_DE_LA_VIE_SANS_MAIN
#include "JDLV_taurique.cpp"

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// =====================================================================
// 1. Theme + drawing helpers
// =====================================================================

// Dark theme: dark blue-grey background, indigo for the selected elements,
// green for living cells and amber for stones.
namespace Theme {
    const sf::Color Background(15, 23, 42);
    const sf::Color Surface(30, 41, 59);      // buttons, text boxes, dead cells
    const sf::Color SurfaceHover(51, 65, 85);
    const sf::Color Border(71, 85, 105);
    const sf::Color Text(241, 245, 249);
    const sf::Color SubText(148, 163, 184);
    const sf::Color Accent(99, 102, 241);
    const sf::Color Error(248, 113, 113);
    const sf::Color Alive(52, 211, 153);
    const sf::Color Dead(30, 41, 59);
    const sf::Color Stone(245, 158, 11);
}

// Rectangle with rounded corners (SFML does not have one).
// It is a polygon: a quarter of a circle in each corner.
sf::ConvexShape roundedRect(sf::FloatRect box, float radius)
{
    const int pointsPerCorner = 8;
    const float PI = 3.14159265f;
    sf::Vector2f centers[4] = {
        {box.left + box.width - radius, box.top + radius},              // top right
        {box.left + box.width - radius, box.top + box.height - radius}, // bottom right
        {box.left + radius,             box.top + box.height - radius}, // bottom left
        {box.left + radius,             box.top + radius}               // top left
    };
    sf::ConvexShape shape(pointsPerCorner * 4);
    int index = 0;
    for (int c = 0; c < 4; c++)
    {
        for (int p = 0; p < pointsPerCorner; p++)
        {
            float degrees = -90.f + 90.f * c + 90.f * p / (pointsPerCorner - 1);
            float angle = degrees * PI / 180.f;
            shape.setPoint(index, {centers[c].x + radius * std::cos(angle), centers[c].y + radius * std::sin(angle)});
            index++;
        }
    }
    return shape;
}

// Writes a text at (x, y). If centered is true, (x, y) is the center of the text.
// fromUtf8 is needed so that accents (é, è...) are displayed correctly.
void drawText(sf::RenderWindow& window, const sf::Font& font, const std::string& str,
              float x, float y, unsigned size, sf::Color color, bool centered = false)
{
    sf::Text text(sf::String::fromUtf8(str.begin(), str.end()), font, size);
    text.setFillColor(color);
    if (centered)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    }
    text.setPosition(std::round(x), std::round(y)); // whole pixels = sharper text
    window.draw(text);
}

// Where a grid of rows x cols cells is drawn: as big as possible inside an area, and centered.
// Used by the simulation page and by the drawing page.
struct GridView {
    float x0 = 0, y0 = 0, cell = 1; // top left corner and size of one cell (in pixels)
    int rows = 1, cols = 1;

    static GridView fit(sf::FloatRect area, int rows, int cols)
    {
        GridView g;
        g.rows = rows;
        g.cols = cols;
        g.cell = std::min(area.width / cols, area.height / rows);
        g.x0 = area.left + (area.width - g.cell * cols) / 2.f;
        g.y0 = area.top + (area.height - g.cell * rows) / 2.f;
        return g;
    }

    // Which cell is under the pixel (px, py)? false if the pixel is outside the grid.
    bool cellAt(float px, float py, int& i, int& j) const
    {
        if (px < x0 || py < y0)
        {
            return false;
        }
        j = (int)((px - x0) / cell);
        i = (int)((py - y0) / cell);
        return i < rows && j < cols;
    }
};

// Draws every cell of the grid; colorOf(k) gives the color of cell k (k = i * cols + j).
void drawCells(sf::RenderWindow& window, const GridView& g, bool tauric, const std::function<sf::Color(int)>& colorOf)
{
    float gap = g.cell > 6.f ? 1.f : 0.f; // thin line between cells if they are big enough

    sf::RectangleShape background({g.cell * g.cols, g.cell * g.rows});
    background.setPosition(g.x0, g.y0);
    background.setFillColor(Theme::Background);
    background.setOutlineThickness(2.f);
    background.setOutlineColor(tauric ? Theme::Accent : Theme::Border); // indigo frame = torique
    window.draw(background);

    sf::RectangleShape shape({g.cell - gap, g.cell - gap});
    for (int i = 0; i < g.rows; i++)
    {
        for (int j = 0; j < g.cols; j++)
        {
            shape.setFillColor(colorOf(i * g.cols + j));
            shape.setPosition(g.x0 + j * g.cell, g.y0 + i * g.cell);
            window.draw(shape);
        }
    }
}

// =====================================================================
// 2. Widgets
// =====================================================================

// A clickable rectangle with a label.
// What it does when clicked is given in the constructor (onClick),
// so we need only ONE Button class for all the buttons of the program.
// isSelected (optional) says if the button must be drawn highlighted (e.g. the chosen option).
// The position is given later with setBox (by the layout of the page).
class Button {
    public :
        Button(std::string l, std::function<void()> click, std::function<bool()> selected = nullptr)
            : label(l), onClick(click), isSelected(selected) {}

        void setBox(sf::FloatRect b)
        {
            box = b;
        }

        void setLabel(const std::string& l)
        {
            label = l;
        }

        void handleEvent(const sf::Event& event)
        {
            if (event.type == sf::Event::MouseButtonPressed
                && event.mouseButton.button == sf::Mouse::Left
                && box.contains(event.mouseButton.x, event.mouseButton.y))
            {
                onClick();
            }
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) const
        {
            bool selected = isSelected && isSelected();
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            bool hover = box.contains(mouse.x, mouse.y);

            sf::ConvexShape shape = roundedRect(box, 12.f);
            shape.setOutlineThickness(1.f);
            shape.setOutlineColor(selected ? Theme::Accent : Theme::Border);
            if (selected)
            {
                shape.setFillColor(Theme::Accent);
            }
            else
            {
                shape.setFillColor(hover ? Theme::SurfaceHover : Theme::Surface);
            }
            window.draw(shape);

            drawText(window, font, label, box.left + box.width / 2.f, box.top + box.height / 2.f,
                     20, Theme::Text, true);
        }

    private :
        sf::FloatRect box; // position (left, top) and size (width, height)
        std::string label;
        std::function<void()> onClick;
        std::function<bool()> isSelected;
};

// A box where the user can type text.
// Abstract: the subclasses decide which characters are accepted
// and where the typed text is stored.
class InputBox {
    public :
        InputBox(std::string l, std::string initial)
            : text(initial), label(l), focused(false) {}
        virtual ~InputBox() = default;

        void setBox(sf::FloatRect b)
        {
            box = b;
        }

        void handleEvent(const sf::Event& event)
        {
            if (event.type == sf::Event::MouseButtonPressed)
            {
                focused = box.contains(event.mouseButton.x, event.mouseButton.y); // click inside = focus
            }
            else if (focused && event.type == sf::Event::TextEntered)
            {
                sf::Uint32 c = event.text.unicode;
                if (c == 8) // Backspace
                {
                    if (!text.empty())
                    {
                        text.pop_back();
                    }
                }
                else if (c == 13) // Enter
                {
                    focused = false;
                }
                else if (c >= 32 && c < 128 && accepts((char)c)) // printable ASCII only
                {
                    text += (char)c;
                }
                apply();
            }
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) const
        {
            drawText(window, font, label, box.left, box.top - 30.f, 17, Theme::SubText);

            sf::ConvexShape shape = roundedRect(box, 12.f);
            shape.setFillColor(Theme::Surface);
            shape.setOutlineThickness(focused ? 2.f : 1.f);
            shape.setOutlineColor(focused ? Theme::Accent : Theme::Border);
            window.draw(shape);

            std::string shown = focused ? text + "|" : text; // "|" = cursor
            drawText(window, font, shown, box.left + 16.f, box.top + box.height / 2.f - 14.f, 20, Theme::Text);
        }

    protected :
        virtual bool accepts(char c) const = 0; // may this character be typed?
        virtual void apply() = 0;               // copies the text into the setting it edits
        std::string text;

    private :
        sf::FloatRect box;
        std::string label;
        bool focused;
};

// Free text (used for the paths of the files).
// It keeps a REFERENCE to the string it edits: typing in the box changes the setting directly.
class TextBox : public InputBox {
    public :
        TextBox(std::string l, std::string& t)
            : InputBox(l, t), target(t) {}

    protected :
        bool accepts(char) const override
        {
            return text.size() < 50;
        }
        void apply() override
        {
            target = text;
        }

    private :
        std::string& target;
};

// Digits only (used for the number of lines / columns, of steps, and the delay).
class NumberBox : public InputBox {
    public :
        NumberBox(std::string l, int& t, int digits = 4)
            : InputBox(l, std::to_string(t)), target(t), maxDigits(digits) {}

    protected :
        bool accepts(char c) const override
        {
            return std::isdigit((unsigned char)c) && (int)text.size() < maxDigits;
        }
        void apply() override
        {
            target = text.empty() ? 0 : std::stoi(text);
        }

    private :
        int& target;
        int maxDigits;
};

// =====================================================================
// 3. Settings
// =====================================================================

// Every choice of the user. The pages read and write it, the widgets edit it.
struct Settings {
    bool graphicMode = true;                 // false = the simulation runs in the terminal, without pause
    bool fromFile = false;                   // false = random grid
    bool drawGrid = false;                   // true = the user draws the grid himself (DrawPage)
    std::string inputPath = "exemple.txt";   // grid to read (only if fromFile)
    std::string outputPath = "resultat.txt"; // console + random grid only: every step is written here
    int length = 50;                         // number of lines
    int width = 50;                          // number of columns
    bool tauric = false;                     // true = the edges are linked
    bool stones = false;
    bool limitSteps = false;                 // true = stop after maxSteps steps
    int maxSteps = 100;
    int delayMs = 50;                       // pause between two steps (graphic mode only)

    // Where the steps are saved: never in graphic mode.
    // Console + random grid: the path chosen by the user. Console + file: "<file>_out.txt".
    std::string savePath() const
    {
        if (graphicMode)
        {
            return "";
        }
        return fromFile ? inputPath + "_out.txt" : outputPath;
    }
};

enum class PageId { Mode, Settings, Draw, Simulation };

// =====================================================================
// 4. Page (abstract)
// =====================================================================

class App; // defined below; the pages only keep a reference to it

// One screen of the program. Each subclass has its own widgets and logic.
class Page {
    public :
        explicit Page(App& a) : app(a) {}
        virtual ~Page() = default;
        virtual void layout(float width, float height) = 0; // places the widgets (called every frame)
        virtual void handleEvent(const sf::Event& event) = 0;
        virtual void update() {} // called every frame (only the simulation needs it)
        virtual void draw(sf::RenderWindow& window, const sf::Font& font) = 0;

    protected :
        App& app;
};

// =====================================================================
// 5. App
// =====================================================================

class App {
    public :
        App()
            : fullscreenButton("Plein écran", [this] { toggleFullscreenRequested = true; })
        {
            createWindow();
            goTo(PageId::Mode);
        }

        bool loadFont();
        void run();

        // The page is changed at the START of the next frame, not right now:
        // goTo is called from a button that belongs to the current page,
        // so destroying that page immediately would destroy the button while it is running.
        void goTo(PageId id)
        {
            nextPage = id;
            pageChangeRequested = true;
        }

        // Console mode: close the window, main() then runs the game in the terminal.
        void quitToConsole()
        {
            consoleRequested = true;
            window.close();
        }

        // Shared state, used by the pages.
        Settings settings;
        ConwayRules rules;                   // must be declared BEFORE organism (organism uses it)
        std::unique_ptr<Organism> organism;  // created by SettingsPage when "Démarrer" is clicked
        bool consoleRequested = false;

    private :
        void createWindow(); // (re)creates the window, in full screen or not
        void switchPage();   // defined after the pages, because it creates them

        sf::RenderWindow window;
        sf::Font font;
        std::unique_ptr<Page> current;
        PageId nextPage = PageId::Mode;
        bool pageChangeRequested = false;

        Button fullscreenButton; // top right corner, on every page
        bool fullscreen = false;
        bool toggleFullscreenRequested = false; // same idea as goTo: done at the start of the next frame
};

// Sizes shared by the pages
namespace Layout {
    const float COLUMN_W = 700.f; // width of the settings column
    const float HALF_W = 340.f;   // width of a button when there are two per line
    const float ROW_H = 54.f;     // height of a button / text box
    const float BOTTOM_H = 64.f;  // height of the big buttons at the bottom
}

// ---------------------------------------------------------------------
// Page 1: choose Console or Graphique
// ---------------------------------------------------------------------
class ModePage : public Page {
    public :
        explicit ModePage(App& a)
            : Page(a),
              console("Console", [this] { choose(false); }),
              graphic("Graphique", [this] { choose(true); }) {}

        void layout(float width, float height) override
        {
            centerX = width / 2;
            centerY = height / 2;
            console.setBox({centerX - 270, centerY + 20, 260, 76});
            graphic.setBox({centerX + 10, centerY + 20, 260, 76});
        }

        void handleEvent(const sf::Event& event) override
        {
            console.handleEvent(event);
            graphic.handleEvent(event);
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) override
        {
            drawText(window, font, "Jeu de la vie", centerX, centerY - 140, 64, Theme::Text, true);
            drawText(window, font, "Choisissez le mode d'affichage", centerX, centerY - 55, 24, Theme::SubText, true);
            console.draw(window, font);
            graphic.draw(window, font);
        }

    private :
        void choose(bool graphicMode)
        {
            app.settings.graphicMode = graphicMode;
            app.goTo(PageId::Settings);
        }

        float centerX = 0, centerY = 0;
        Button console;
        Button graphic;
};

// ---------------------------------------------------------------------
// Page 2: settings
// Some lines are shown only in some cases (e.g. the output file: console + random grid only),
// so layout() stacks only the visible lines, one under the other.
// ---------------------------------------------------------------------
class SettingsPage : public Page {
    public :
        explicit SettingsPage(App& a)
            : Page(a),
              randomButton("Aléatoire", [this] { chooseSource(false, false); },
                           [this] { return !app.settings.fromFile && !app.settings.drawGrid; }),
              fileButton("Fichier .txt", [this] { chooseSource(true, false); }, [this] { return app.settings.fromFile; }),
              drawButton("Dessiner", [this] { chooseSource(false, true); }, [this] { return app.settings.drawGrid; }),
              lengthBox("Lignes", a.settings.length),
              widthBox("Colonnes", a.settings.width),
              inputBox("Fichier de la grille", a.settings.inputPath),
              outputBox("Fichier de sauvegarde des étapes (vide = pas de sauvegarde)", a.settings.outputPath),
              closedButton("Bords fermés", [this] { app.settings.tauric = false; }, [this] { return !app.settings.tauric; }),
              tauricButton("Torique", [this] { app.settings.tauric = true; }, [this] { return app.settings.tauric; }),
              stonesYes("Avec pierres", [this] { app.settings.stones = true; }, [this] { return app.settings.stones; }),
              stonesNo("Sans pierres", [this] { app.settings.stones = false; }, [this] { return !app.settings.stones; }),
              untilStable("Jusqu'à stabilité", [this] { app.settings.limitSteps = false; }, [this] { return !app.settings.limitSteps; }),
              fixedSteps("Nombre d'étapes fixé", [this] { app.settings.limitSteps = true; }, [this] { return app.settings.limitSteps; }),
              stepsBox("Nombre d'étapes", a.settings.maxSteps, 6),
              delayBox("Délai entre deux étapes (ms)", a.settings.delayMs),
              back("Retour", [this] { app.goTo(PageId::Mode); }),
              start("Démarrer", [this] { startGame(); }, [] { return true; }) {}

        void layout(float width, float height) override
        {
            const Settings& s = app.settings;
            centerX = width / 2;
            float left = (width - Layout::COLUMN_W) / 2;
            float right = left + Layout::COLUMN_W - Layout::HALF_W;
            labels.clear();

            // Visible lines
            bool showOutput = !s.graphicMode && !s.fromFile;
            bool showSaveNote = !s.graphicMode && s.fromFile;
            int lines = 5 + (showOutput || showSaveNote ? 1 : 0) + (s.limitSteps ? 1 : 0) + (s.graphicMode ? 1 : 0);

            // Space between two lines: as much as possible, but not more than 105 pixels
            float top = 130;
            float bottom = height - 130;
            float lineH = std::min(105.f, (bottom - top) / lines);
            float y = top; // top of the current line (its label); the control is 28 pixels below

            labels.push_back({"Grille de départ", y});
            float thirdW = (Layout::COLUMN_W - 2 * 20.f) / 3; // three buttons on this line
            randomButton.setBox({left, y + 28, thirdW, Layout::ROW_H});
            fileButton.setBox({left + thirdW + 20.f, y + 28, thirdW, Layout::ROW_H});
            drawButton.setBox({left + 2 * (thirdW + 20.f), y + 28, thirdW, Layout::ROW_H});
            y += lineH;

            lengthBox.setBox({left, y + 28, Layout::HALF_W, Layout::ROW_H}); // InputBox draws its own label
            widthBox.setBox({right, y + 28, Layout::HALF_W, Layout::ROW_H});
            inputBox.setBox({left, y + 28, Layout::COLUMN_W, Layout::ROW_H});
            y += lineH;

            if (showOutput)
            {
                outputBox.setBox({left, y + 28, Layout::COLUMN_W, Layout::ROW_H});
                y += lineH;
            }
            if (showSaveNote)
            {
                labels.push_back({"Les étapes seront enregistrées dans : " + s.savePath(), y + 40});
                y += lineH;
            }

            labels.push_back({"Bords de la grille", y});
            closedButton.setBox({left, y + 28, Layout::HALF_W, Layout::ROW_H});
            tauricButton.setBox({right, y + 28, Layout::HALF_W, Layout::ROW_H});
            y += lineH;

            labels.push_back({"Pierres (cellules qui ne changent jamais)", y});
            stonesYes.setBox({left, y + 28, Layout::HALF_W, Layout::ROW_H});
            stonesNo.setBox({right, y + 28, Layout::HALF_W, Layout::ROW_H});
            y += lineH;

            labels.push_back({"Durée de vie de l'organisme", y});
            untilStable.setBox({left, y + 28, Layout::HALF_W, Layout::ROW_H});
            fixedSteps.setBox({right, y + 28, Layout::HALF_W, Layout::ROW_H});
            y += lineH;

            if (s.limitSteps)
            {
                stepsBox.setBox({left, y + 28, Layout::COLUMN_W, Layout::ROW_H});
                y += lineH;
            }
            if (s.graphicMode)
            {
                delayBox.setBox({left, y + 28, Layout::COLUMN_W, Layout::ROW_H});
                y += lineH;
            }

            labelX = left;
            errorY = height - 115;
            back.setBox({left, height - 90, Layout::HALF_W, Layout::BOTTOM_H});
            start.setBox({right, height - 90, Layout::HALF_W, Layout::BOTTOM_H});
        }

        void handleEvent(const sf::Event& event) override
        {
            const Settings& s = app.settings;
            randomButton.handleEvent(event);
            fileButton.handleEvent(event);
            drawButton.handleEvent(event);
            if (s.fromFile)
            {
                inputBox.handleEvent(event);
            }
            else
            {
                lengthBox.handleEvent(event);
                widthBox.handleEvent(event);
                if (!s.graphicMode)
                {
                    outputBox.handleEvent(event);
                }
            }
            closedButton.handleEvent(event);
            tauricButton.handleEvent(event);
            stonesYes.handleEvent(event);
            stonesNo.handleEvent(event);
            untilStable.handleEvent(event);
            fixedSteps.handleEvent(event);
            if (s.limitSteps)
            {
                stepsBox.handleEvent(event);
            }
            if (s.graphicMode) // no timer in console mode
            {
                delayBox.handleEvent(event);
            }
            back.handleEvent(event);
            start.handleEvent(event);
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) override
        {
            const Settings& s = app.settings;
            std::string title = s.graphicMode ? "Réglages - mode graphique" : "Réglages - mode console";
            drawText(window, font, title, centerX, 65, 42, Theme::Text, true);

            for (const std::pair<std::string, float>& label : labels)
            {
                drawText(window, font, label.first, labelX, label.second, 17, Theme::SubText);
            }

            randomButton.draw(window, font);
            fileButton.draw(window, font);
            drawButton.draw(window, font);
            if (s.fromFile)
            {
                inputBox.draw(window, font);
            }
            else
            {
                lengthBox.draw(window, font);
                widthBox.draw(window, font);
                if (!s.graphicMode)
                {
                    outputBox.draw(window, font);
                }
            }
            closedButton.draw(window, font);
            tauricButton.draw(window, font);
            stonesYes.draw(window, font);
            stonesNo.draw(window, font);
            untilStable.draw(window, font);
            fixedSteps.draw(window, font);
            if (s.limitSteps)
            {
                stepsBox.draw(window, font);
            }
            if (s.graphicMode)
            {
                delayBox.draw(window, font);
            }

            if (!error.empty())
            {
                drawText(window, font, error, centerX, errorY, 18, Theme::Error, true);
            }
            back.draw(window, font);
            start.draw(window, font);
        }

    private :
        // Checks the settings, creates the organism, then goes to the simulation (or the console).
        void startGame()
        {
            const Settings& s = app.settings;
            if (!s.fromFile && (s.length < 3 || s.width < 3 || s.length > 400 || s.width > 400))
            {
                error = "La grille doit faire entre 3 x 3 et 400 x 400";
                return;
            }
            if (s.limitSteps && s.maxSteps < 1)
            {
                error = "Le nombre d'étapes doit être au moins 1";
                return;
            }
            if (s.drawGrid)
            {
                app.goTo(PageId::Draw); // the organism is created by DrawPage, once the drawing is done
                return;
            }

            app.organism = std::make_unique<Organism>("gui", s.length, s.width, app.rules, s.stones);
            app.organism->setTauric(s.tauric);         // must be set BEFORE the grid is built
            app.organism->setOutputFile(s.savePath()); // same ("" in graphic mode = no save)
            if (s.fromFile)
            {
                std::string message;
                if (!app.organism->init_from_file(s.inputPath, message))
                {
                    error = message;
                    app.organism.reset();
                    return;
                }
            }
            else
            {
                app.organism->init_organism();
            }

            if (s.graphicMode)
            {
                app.goTo(PageId::Simulation);
            }
            else
            {
                app.quitToConsole();
            }
        }

        void chooseSource(bool fromFile, bool drawGrid)
        {
            app.settings.fromFile = fromFile;
            app.settings.drawGrid = drawGrid;
        }

        Button randomButton;
        Button fileButton;
        Button drawButton;
        NumberBox lengthBox;
        NumberBox widthBox;
        TextBox inputBox;
        TextBox outputBox;
        Button closedButton;
        Button tauricButton;
        Button stonesYes;
        Button stonesNo;
        Button untilStable;
        Button fixedSteps;
        NumberBox stepsBox;
        NumberBox delayBox;
        Button back;
        Button start;

        std::vector<std::pair<std::string, float>> labels; // titles of the lines: (text, y), filled by layout
        float centerX = 0, labelX = 0, errorY = 0;
        std::string error; // shown in red when the settings are not valid
};

// ---------------------------------------------------------------------
// Page 2b: the user draws the starting grid ("Dessiner" in the settings)
// On the right: one button per tool (single cell, eraser, stone, and the shapes of "motifs").
// Click a tool, then click on the grid: the shape is placed centered on the clicked cell.
// Single-cell tools can also be dragged to paint. Right click = erase a cell.
// ---------------------------------------------------------------------
class DrawPage : public Page {
    public :
        explicit DrawPage(App& a)
            : Page(a),
              cells(a.settings.length * a.settings.width, 0),
              back("Retour", [this] { app.goTo(PageId::Settings); }),
              clear("Tout effacer", [this] { std::fill(cells.begin(), cells.end(), 0); }),
              start("Lancer", [this] { startGame(); }, [] { return true; })
        {
            motifs m;
            tools.push_back({"Cellule", 1, 1, {1}, 1});
            tools.push_back({"Gomme", 1, 1, {1}, 0});
            if (a.settings.stones)
            {
                tools.push_back({"Pierre", 1, 1, {1}, 2});
            }
            addShape("Rond 7x7", 7, 7, m.round_7x7);
            addShape("Carré 9x9", 9, 9, m.square_9x9);
            addShape("Rect. 5x9", 5, 9, m.rectangle_v5x9);
            addShape("Rect. 9x5", 9, 5, m.rectangle_h9x5);
            addShape("Étoile 9x9", 9, 9, m.star_9x9);
            addShape("Étoile 7x5", 7, 5, m.star_7x5);
            addShape("Vaisseau HD", 3, 3, m.ship_tr);
            addShape("Vaisseau HG", 3, 3, m.ship_tl);
            addShape("Vaisseau BD", 3, 3, m.ship_br);
            addShape("Vaisseau BG", 3, 3, m.ship_bl);
            addShape("U 3x3", 3, 3, m.u_3x3);
            addShape("Infini 13x13", 13, 13, m.infinite_1);
            addShape("Infini 5x5", 5, 5, m.infinite_5x5);
            addShape("Bloc", 2, 2, m.block_2x2);
            addShape("Ruche", 4, 3, m.beehive_4x3);
            addShape("Pain", 4, 4, m.loaf_4x4);
            addShape("Bateau", 3, 3, m.boat_3x3);
            addShape("Baignoire", 3, 3, m.tub_3x3);
            addShape("Clignotant", 3, 1, m.blinker_3x1);
            addShape("Crapaud", 4, 2, m.toad_4x2);
            addShape("Phare", 4, 4, m.beacon_4x4);
            addShape("Pentadécathlon", 10, 3, m.pentadecathlon_10x3);
            addShape("Planeur", 3, 3, m.glider_3x3);
            addShape("LWSS", 5, 4, m.lwss_5x4);
            addShape("MWSS", 6, 5, m.mwss_6x5);
            addShape("HWSS", 7, 5, m.hwss_7x5);
            addShape("R-pentomino", 3, 3, m.r_pentomino_3x3);
            addShape("Diehard", 8, 3, m.diehard_8x3);
            addShape("Gland", 7, 3, m.acorn_7x3);
            addShape("Canon Gosper", 36, 9, m.gosper_gun_36x9);
            addShape("Mare", 4, 4, m.pond_4x4);
            addShape("Barge", 4, 4, m.barge_4x4);
            addShape("Serpent", 4, 2, m.snake_4x2);
            addShape("Horloge", 4, 4, m.clock_4x4);
            addShape("Galaxie de Kok", 9, 9, m.kok_galaxy_9x9);
            addShape("Oiseau-tonnerre", 3, 5, m.thunderbird_3x5);

            toolButtons.reserve(tools.size()); // no reallocation: the buttons keep their address
            for (int t = 0; t < (int)tools.size(); t++)
            {
                toolButtons.emplace_back(tools[t].label, [this, t] { selected = t; }, [this, t] { return selected == t; });
            }
        }

        void layout(float width, float height) override
        {
            // Tool buttons: as many rows as fit above the bottom buttons, then as many columns as needed.
            // If that needs more than MAX_COLS columns, the buttons get shorter so that everything fits.
            const float BTN_W = 200.f, GAP = 12.f, TOP = 115.f;
            const int MAX_COLS = 4;
            float btnH = 60.f;
            int n = (int)toolButtons.size();
            float available = height - 110.f - TOP; // space between the panel title and the bottom buttons
            int rows = std::max(1, (int)((available + GAP) / (btnH + GAP)));
            int cols = (n + rows - 1) / rows;
            if (cols > MAX_COLS)
            {
                cols = MAX_COLS;
                rows = (n + cols - 1) / cols;
                btnH = std::max(30.f, (available + GAP) / rows - GAP);
            }
            float panelW = cols * BTN_W + (cols - 1) * GAP;
            float panelX = width - 40.f - panelW;
            gridArea = {40.f, 80.f, panelX - 40.f - 40.f, height - 190.f};
            centerX = gridArea.left + gridArea.width / 2;
            panelTitle = {panelX, 80.f};
            for (int t = 0; t < n; t++) // filled column by column
            {
                float x = panelX + (t / rows) * (BTN_W + GAP);
                float y = TOP + (t % rows) * (btnH + GAP);
                toolButtons[t].setBox({x, y, BTN_W, btnH});
            }
            float y = height - 86;
            back.setBox({40, y, 160, 56});
            clear.setBox({centerX - 100, y, 200, 56});
            start.setBox({width - 40 - 200, y, 200, 56});
            view = GridView::fit(gridArea, app.settings.length, app.settings.width);
        }

        void handleEvent(const sf::Event& event) override
        {
            back.handleEvent(event);
            clear.handleEvent(event);
            start.handleEvent(event);
            for (Button& b : toolButtons)
            {
                b.handleEvent(event);
            }

            int i, j;
            if (event.type == sf::Event::MouseButtonPressed
                && view.cellAt(event.mouseButton.x, event.mouseButton.y, i, j))
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    stamp(tools[selected], i, j);
                    painting = tools[selected].w == 1 && tools[selected].h == 1; // single cell: keep painting while dragging
                }
                else if (event.mouseButton.button == sf::Mouse::Right)
                {
                    cells[i * app.settings.width + j] = 0;
                    erasing = true;
                }
            }
            else if (event.type == sf::Event::MouseButtonReleased)
            {
                painting = false;
                erasing = false;
            }
            else if (event.type == sf::Event::MouseMoved
                     && view.cellAt(event.mouseMove.x, event.mouseMove.y, i, j))
            {
                if (painting)
                {
                    stamp(tools[selected], i, j);
                }
                else if (erasing)
                {
                    cells[i * app.settings.width + j] = 0;
                }
            }
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) override
        {
            drawText(window, font, "Dessin : choisissez un motif à droite, puis cliquez sur la grille  ·  clic droit = effacer",
                     centerX, 42, 22, Theme::Text, true);
            drawText(window, font, "Motifs", panelTitle.x, panelTitle.y, 17, Theme::SubText);

            drawCells(window, view, app.settings.tauric, [&](int k)
            {
                if (cells[k] == 2)
                {
                    return Theme::Stone;
                }
                return cells[k] == 1 ? Theme::Alive : Theme::Dead;
            });
            drawPreview(window);

            for (const Button& b : toolButtons)
            {
                b.draw(window, font);
            }
            back.draw(window, font);
            clear.draw(window, font);
            start.draw(window, font);
        }

    private :
        // A tool: a pattern of w x h values (1 = part of the shape), and the value it writes (0, 1 or 2).
        struct Tool {
            std::string label;
            int w, h;
            std::vector<int> pattern;
            int value;
        };

        void addShape(const std::string& label, int w, int h, const std::list<int>& shape)
        {
            std::vector<int> pattern(shape.begin(), shape.end());
            if ((int)pattern.size() != w * h) // wrong size in "motifs": the shape is ignored
            {
                std::cerr << "Motif " << label << " ignoré : " << pattern.size() << " cases au lieu de " << w * h << std::endl;
                return;
            }
            tools.push_back({label, w, h, pattern, 1});
        }

        // Cell (i, j) of the grid covered by the cell (r, c) of a shape centered on (ci, cj).
        // Torique grid: wraps around the edges. Closed edges: false if it falls outside.
        bool target(const Tool& t, int ci, int cj, int r, int c, int& i, int& j) const
        {
            int rows = app.settings.length, cols = app.settings.width;
            i = ci - t.h / 2 + r;
            j = cj - t.w / 2 + c;
            if (app.settings.tauric)
            {
                i = ((i % rows) + rows) % rows;
                j = ((j % cols) + cols) % cols;
                return true;
            }
            return i >= 0 && i < rows && j >= 0 && j < cols;
        }

        void stamp(const Tool& t, int ci, int cj)
        {
            for (int r = 0; r < t.h; r++)
            {
                for (int c = 0; c < t.w; c++)
                {
                    int i, j;
                    if (t.pattern[r * t.w + c] == 1 && target(t, ci, cj, r, c, i, j))
                    {
                        cells[i * app.settings.width + j] = t.value;
                    }
                }
            }
        }

        // Shows where the selected tool will be placed, under the mouse.
        void drawPreview(sf::RenderWindow& window)
        {
            sf::Vector2i mouse = sf::Mouse::getPosition(window);
            int ci, cj;
            if (!view.cellAt(mouse.x, mouse.y, ci, cj))
            {
                return;
            }
            const Tool& t = tools[selected];
            sf::RectangleShape shape({view.cell, view.cell});
            sf::Color color = Theme::Accent;
            color.a = 150; // half transparent: the grid stays visible
            shape.setFillColor(color);
            for (int r = 0; r < t.h; r++)
            {
                for (int c = 0; c < t.w; c++)
                {
                    int i, j;
                    if (t.pattern[r * t.w + c] == 1 && target(t, ci, cj, r, c, i, j))
                    {
                        shape.setPosition(view.x0 + j * view.cell, view.y0 + i * view.cell);
                        window.draw(shape);
                    }
                }
            }
        }

        // Creates the organism from the drawing, then goes to the simulation (or the console).
        void startGame()
        {
            const Settings& s = app.settings;
            app.organism = std::make_unique<Organism>("gui", s.length, s.width, app.rules, s.stones);
            app.organism->setTauric(s.tauric);
            app.organism->setOutputFile(s.savePath());
            app.organism->init_from_cells(cells);
            if (s.graphicMode)
            {
                app.goTo(PageId::Simulation);
            }
            else
            {
                app.quitToConsole();
            }
        }

        std::vector<int> cells; // the drawing: 0 = dead, 1 = alive, 2 = stone (same indexing as Organism)
        std::vector<Tool> tools;
        std::vector<Button> toolButtons;
        int selected = 0;       // index of the selected tool
        bool painting = false;  // left button held with a single-cell tool
        bool erasing = false;   // right button held
        GridView view;
        sf::FloatRect gridArea;
        sf::Vector2f panelTitle;
        float centerX = 0;
        Button back;
        Button clear;
        Button start;
};

// ---------------------------------------------------------------------
// Page 3: the simulation (graphic mode only)
// ---------------------------------------------------------------------
class SimulationPage : public Page {
    public :
        explicit SimulationPage(App& a)
            : Page(a),
              org(*a.organism),
              stones(org.getStones()), // stones never change: read them once
              back("Retour", [this] { app.goTo(PageId::Settings); }),
              previous("Précédent", [this] { showPrevious(); }),
              playPause("Pause", [this] { togglePlay(); }),
              next("Suivant", [this] { playing = false; showNext(); }) {}

        void layout(float width, float height) override
        {
            gridArea = {40.f, 80.f, width - 80.f, height - 190.f};
            centerX = width / 2;
            float y = height - 86;
            back.setBox({40, y, 160, 56});
            previous.setBox({centerX - 275, y, 180, 56});
            playPause.setBox({centerX - 80, y, 160, 56});
            next.setBox({centerX + 95, y, 180, 56});
        }

        void handleEvent(const sf::Event& event) override
        {
            back.handleEvent(event);
            previous.handleEvent(event);
            playPause.handleEvent(event);
            next.handleEvent(event);
        }

        void update() override
        {
            playPause.setLabel(playing ? "Pause" : "Lecture");
            if (playing && clock.getElapsedTime().asMilliseconds() >= app.settings.delayMs)
            {
                clock.restart();
                showNext();
            }
        }

        void draw(sf::RenderWindow& window, const sf::Font& font) override
        {
            std::string status = "Étape " + std::to_string(view) + " / " + std::to_string(org.getStep());
            if (app.settings.limitSteps)
            {
                status += " (limite " + std::to_string(app.settings.maxSteps) + ")";
            }
            if (app.settings.tauric)
            {
                status += "   ·   torique";
            }
            if (!endReason.empty())
            {
                status += "   ·   " + endReason;
            }
            drawText(window, font, status, centerX, 42, 24, endReason.empty() ? Theme::Text : Theme::Alive, true);

            drawGrid(window);

            back.draw(window, font);
            previous.draw(window, font);
            playPause.draw(window, font);
            next.draw(window, font);
        }

    private :
        // Goes one step forward. If we are looking at an old step, we just replay the history;
        // otherwise the organism computes a new step.
        void showNext()
        {
            if (view < org.getStep())
            {
                view++;
                return;
            }
            if (!endReason.empty()) // the simulation is over
            {
                playing = false;
                return;
            }
            org.nextstep();
            view = org.getStep();
            if (org.alreadySeen())
            {
                endReason = "état déjà vu : l'organisme est stable";
            }
            else if (app.settings.limitSteps && org.getStep() >= app.settings.maxSteps)
            {
                endReason = "nombre d'étapes atteint";
            }
            if (!endReason.empty())
            {
                playing = false;
            }
        }

        void showPrevious()
        {
            playing = false;
            if (view > 0)
            {
                view--;
            }
        }

        void togglePlay()
        {
            playing = !playing;
            clock.restart();
        }

        // Draws the step "view", as big as possible in gridArea.
        void drawGrid(sf::RenderWindow& window)
        {
            const std::vector<bool>& alive = org.getGrid(view);
            GridView g = GridView::fit(gridArea, org.getLength(), org.getWidth());
            drawCells(window, g, app.settings.tauric, [&](int k) // k: same indexing as Organism
            {
                if (stones[k])
                {
                    return Theme::Stone;
                }
                return alive[k] ? Theme::Alive : Theme::Dead;
            });
        }

        Organism& org;
        std::vector<bool> stones;
        int view = 0;          // step currently displayed (can be an old one)
        bool playing = true;
        std::string endReason; // empty while the simulation goes on
        sf::Clock clock;       // time since the last step
        sf::FloatRect gridArea;
        float centerX = 0;
        Button back;
        Button previous;
        Button playPause;
        Button next;
};

// ---------------------------------------------------------------------
// App methods that need the pages
// ---------------------------------------------------------------------

void App::createWindow()
{
    sf::VideoMode screen = sf::VideoMode::getDesktopMode(); // size of the screen of the PC
    if (fullscreen)
    {
        window.create(screen, "Jeu de la vie", sf::Style::Fullscreen);
    }
    else
    {
        window.create(screen, "Jeu de la vie", sf::Style::Default);
        window.setPosition({0, 0});
    }
    window.setFramerateLimit(60);
    fullscreenButton.setLabel(fullscreen ? "Quitter plein écran" : "Plein écran");
}

bool App::loadFont()
{
    const char* candidates[] = {
        "/usr/share/fonts/truetype/ubuntu/UbuntuSans[wdth,wght].ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
    };
    for (const char* path : candidates)
    {
        if (font.loadFromFile(path))
        {
            return true;
        }
    }
    return false;
}

void App::switchPage()
{
    pageChangeRequested = false;
    switch (nextPage)
    {
        case PageId::Mode:       current = std::make_unique<ModePage>(*this); break;
        case PageId::Settings:   current = std::make_unique<SettingsPage>(*this); break;
        case PageId::Draw:       current = std::make_unique<DrawPage>(*this); break;
        case PageId::Simulation: current = std::make_unique<SimulationPage>(*this); break;
    }
}

// Main loop: every frame -> requested changes, layout, events, update, draw.
void App::run()
{
    while (window.isOpen())
    {
        if (toggleFullscreenRequested)
        {
            toggleFullscreenRequested = false;
            fullscreen = !fullscreen;
            createWindow();
        }
        if (pageChangeRequested)
        {
            switchPage();
        }

        // Place the widgets for the current size of the window
        float width = (float)window.getSize().x;
        float height = (float)window.getSize().y;
        fullscreenButton.setBox({width - 236, 16, 220, 44});
        current->layout(width, height);

        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            else if (event.type == sf::Event::Resized)
            {
                // Without this, SFML would stretch the drawing instead of using the new space
                window.setView(sf::View(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height)));
            }
            else if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape && fullscreen)
            {
                toggleFullscreenRequested = true;
            }
            else
            {
                fullscreenButton.handleEvent(event);
                current->handleEvent(event);
            }
        }

        current->update();

        window.clear(Theme::Background);
        current->draw(window, font);
        fullscreenButton.draw(window, font); // drawn last: always on top
        window.display();
    }
}

// =====================================================================
// 6. main
// =====================================================================

int main()
{
    std::srand(std::time(nullptr));

    App app;
    if (!app.loadFont())
    {
        std::cerr << "Aucune police trouvée." << std::endl;
        return 1;
    }
    app.run();

    // "Console" was chosen: the window is closed, the game continues in the terminal, without pause.
    if (app.consoleRequested && app.organism)
    {
        int maxSteps = app.settings.limitSteps ? app.settings.maxSteps : 0; // 0 = until stable
        runConsoleSimulation(*app.organism, 0, maxSteps);
        if (!app.settings.savePath().empty())
        {
            std::cout << "Toutes les étapes sont dans : " << app.settings.savePath() << std::endl;
        }
    }
    return 0;
}
