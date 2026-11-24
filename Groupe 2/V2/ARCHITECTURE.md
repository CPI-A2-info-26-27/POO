# Jeu de la vie : architecture et explication du code

Ce document explique comment le projet est organisé et comment le code fonctionne.
Il s'adresse à des personnes qui **ne connaissent pas bien le C++** : chaque notion
du langage est expliquée la première fois qu'elle apparaît (encadrés « 💡 C++ »).

> La version à jour est **V2**. Les dossiers `V0` et `V1` sont d'anciennes versions
> gardées pour l'historique (voir la [dernière partie](#8-historique-des-versions)).

---

## Sommaire

1. [Le jeu de la vie en 2 minutes](#1-le-jeu-de-la-vie-en-2-minutes)
2. [Organisation des fichiers](#2-organisation-des-fichiers)
3. [Compiler et lancer le programme](#3-compiler-et-lancer-le-programme)
4. [Vue d'ensemble de l'architecture](#4-vue-densemble-de-larchitecture)
5. [La logique du jeu : `JDLV_taurique.cpp`](#5-la-logique-du-jeu--jdlv_tauriquecpp)
6. [L'interface graphique : `affichage_main.cpp`](#6-linterface-graphique--affichage_maincpp)
7. [Le parcours complet d'une partie](#7-le-parcours-complet-dune-partie)
8. [Historique des versions](#8-historique-des-versions)
9. [Petit lexique C++](#9-petit-lexique-c)

---

## 1. Le jeu de la vie en 2 minutes

Le « jeu de la vie » (inventé par John Conway) n'est pas un jeu où l'on joue :
c'est une **simulation**. On a une grille de cases, appelées **cellules**. Chaque
cellule est soit **vivante**, soit **morte**.

À chaque **étape**, toutes les cellules changent **en même temps** selon le nombre
de voisines vivantes (les 8 cases autour d'elle) :

| Cellule actuellement | Voisines vivantes | Étape suivante |
|---|---|---|
| Vivante | 2 ou 3 | reste vivante (survie) |
| Vivante | 0, 1, 4 ou plus | meurt (isolement ou surpopulation) |
| Morte | exactement 3 | devient vivante (naissance) |
| Morte | autre nombre | reste morte |

Ce projet ajoute deux options :

- **Les pierres** : des cellules spéciales qui ne changent **jamais** et ne comptent
  pas comme vivantes. Ce sont des obstacles.
- **La grille torique** : les bords sont reliés. Une cellule tout à droite a pour
  voisines les cellules tout à gauche, et pareil entre le haut et le bas (comme
  dans Pac-Man, où l'on sort d'un côté pour revenir de l'autre).

La simulation s'arrête quand la grille revient dans un état **déjà vu** (elle
tourne alors en boucle pour toujours) ou quand un nombre d'étapes maximum est atteint.

---

## 2. Organisation des fichiers

```
Groupe 2/
├── ARCHITECTURE.md          ← ce document
├── V0/                      ← première version (historique)
├── V1/                      ← deuxième version (historique)
└── V2/                      ← VERSION ACTUELLE
    ├── JDLV_taurique.cpp    ← la logique du jeu (cellules, règles, grille)
    ├── affichage_main.cpp   ← l'interface graphique + la fonction main()
    ├── exemple.txt          ← exemple de grille de départ à charger
    ├── resultat.txt         ← exemple de fichier de sauvegarde produit par le programme
    ├── lahister.txt         ← autre fichier de sauvegarde produit par le programme
    ├── jeu_de_la_vie_gui    ← programme compilé (exécutable)
    └── jdlv_simple          ← ancien exécutable compilé
```

Les fichiers sans extension (`jeu_de_la_vie_gui`, `jdlv_simple`…) sont des
**exécutables** : le résultat de la compilation. Ils ne se lisent pas, ils se lancent.

Le code est séparé en **deux couches** bien distinctes :

| Fichier | Rôle | Sait-il dessiner une fenêtre ? |
|---|---|---|
| `JDLV_taurique.cpp` | **Le « moteur »** : ce qu'est une cellule, les règles, le calcul des étapes, la lecture/écriture de fichiers | Non |
| `affichage_main.cpp` | **L'« interface »** : la fenêtre, les boutons, les pages, le dessin de la grille | Oui (avec la bibliothèque SFML) |

Cette séparation est volontaire : on pourrait remplacer toute l'interface graphique
sans toucher une seule ligne du moteur.

---

## 3. Compiler et lancer le programme

> 💡 **C++ : la compilation.** Le C++ n'est pas exécuté directement comme Python.
> Il faut d'abord le **compiler** : un programme (le compilateur `g++`) traduit le
> code source (`.cpp`) en un fichier exécutable que l'ordinateur comprend.

Installer la bibliothèque graphique SFML (une seule fois) :

```bash
sudo apt install libsfml-dev
```

Compiler (depuis le dossier `V2`) :

```bash
g++ -std=c++17 -Wall affichage_main.cpp -o jeu_de_la_vie_gui -lsfml-graphics -lsfml-window -lsfml-system
```

Détail de la commande :

| Morceau | Signification |
|---|---|
| `g++` | le compilateur C++ |
| `-std=c++17` | utiliser la version 2017 du langage C++ |
| `-Wall` | afficher tous les avertissements (aide à repérer les erreurs) |
| `affichage_main.cpp` | le fichier à compiler (il inclut lui-même `JDLV_taurique.cpp`) |
| `-o jeu_de_la_vie_gui` | nom de l'exécutable produit |
| `-lsfml-graphics -lsfml-window -lsfml-system` | « lier » les 3 morceaux de la bibliothèque SFML |

Lancer :

```bash
./jeu_de_la_vie_gui
```

On ne compile qu'**un seul** fichier car `affichage_main.cpp` commence par
`#include "JDLV_taurique.cpp"`, ce qui revient à copier-coller tout le moteur au
début de l'interface.

> 💡 **C++ : `#include`.** Cette ligne demande d'insérer le contenu d'un autre
> fichier à cet endroit. `#include <iostream>` (avec `< >`) insère une bibliothèque
> standard ; `#include "fichier.cpp"` (avec `" "`) insère un fichier du projet.

---

## 4. Vue d'ensemble de l'architecture

Le projet est écrit en **programmation orientée objet** (POO).

> 💡 **C++ : classe et objet.** Une **classe** est un plan de construction (ex. :
> « une cellule a un état et des voisines, et sait calculer son état suivant »).
> Un **objet** est une chose construite à partir de ce plan (ex. : la cellule de la
> ligne 3, colonne 7). Une classe contient des **attributs** (des données) et des
> **méthodes** (des fonctions qui agissent sur ces données).

### Schéma général

```
                         ┌──────────────────── affichage_main.cpp ────────────────────┐
                         │                                                            │
   main() ──crée──►  App │ (fenêtre, police, réglages, page courante, organisme)      │
                         │   │                                                        │
                         │   ├── Settings   : tous les choix de l'utilisateur         │
                         │   ├── Page (abstraite)                                     │
                         │   │     ├── ModePage        : Console ou Graphique ?       │
                         │   │     ├── SettingsPage    : réglages                     │
                         │   │     ├── DrawPage        : dessiner la grille           │
                         │   │     └── SimulationPage  : voir la simulation           │
                         │   └── Widgets : Button, InputBox → TextBox / NumberBox     │
                         └───────────────────────────┬────────────────────────────────┘
                                                     │ utilise
                         ┌──────────────────── JDLV_taurique.cpp ─────────────────────┐
                         │  Organism  (la grille entière, l'historique, les fichiers) │
                         │     └── Cell × (lignes × colonnes)                         │
                         │            ├── un CellState : Alive / Dead / Stone         │
                         │            └── une liste de pointeurs vers ses voisines    │
                         │  Rules (abstraite) ──► ConwayRules                         │
                         │  motifs (formes prêtes à dessiner)                         │
                         │  runConsoleSimulation() (mode terminal)                    │
                         └────────────────────────────────────────────────────────────┘
```

### Les grandes idées

1. **Le polymorphisme pour les états** : une cellule ne stocke pas un simple
   « vrai/faux », mais un *objet état* (`AliveState`, `DeadState` ou `StoneState`).
   Chaque état sait répondre à « es-tu vivant ? », « es-tu une pierre ? ».
2. **Le polymorphisme pour les règles** : les règles du jeu sont dans une classe à
   part (`ConwayRules`). Pour inventer d'autres règles, il suffit d'écrire une
   nouvelle classe, sans modifier `Cell` ni `Organism`.
3. **Chaque cellule connaît ses voisines** : au démarrage, on donne à chaque
   cellule la liste de ses voisines. Ensuite, compter les voisines vivantes est
   très simple, et le mode torique ne change que cette étape d'initialisation.
4. **Une page = une classe** dans l'interface : chaque écran gère ses propres
   boutons et son propre dessin.

---

## 5. La logique du jeu : `JDLV_taurique.cpp`

### 5.1 Les `#include` du début

```cpp
#include <iostream>   // afficher dans le terminal (std::cout)
#include <fstream>    // lire et écrire des fichiers
#include <string>     // le type texte std::string
#include <list>       // le conteneur std::list
#include <vector>     // le conteneur std::vector (tableau qui grandit)
#include <algorithm>  // std::find (chercher dans une liste)
#include <thread>     // faire une pause
#include <chrono>     // mesurer des durées (millisecondes)
...
```

> 💡 **C++ : `std::`.** Tout ce qui vient de la bibliothèque standard du C++ est
> rangé dans l'espace de noms `std` (« standard »). `std::string` se lit donc
> « le type `string` de la bibliothèque standard ».

> 💡 **C++ : `std::vector` et `std::list`.** Ce sont des listes d'éléments du même
> type. `std::vector<int>` est une liste d'entiers, `std::vector<bool>` une liste de
> vrai/faux. On ajoute un élément à la fin avec `.push_back(x)` et on lit le
> k-ième avec `v[k]`.

### 5.2 `struct motifs` : des formes prêtes à l'emploi

```cpp
struct motifs {
    std::list<int> ship_tr = {0,1,1,
                              1,0,1,
                              0,0,1};
    ...
};
```

C'est un catalogue de formes (rond, carré, étoile, « vaisseaux », etc.). Chaque
forme est une liste de 0 et de 1 écrite ligne par ligne : 1 = cellule vivante.
La forme ci-dessus est un « vaisseau » de 3 × 3 cases.

Ces formes sont utilisées par la page de dessin (`DrawPage`) : l'utilisateur
choisit une forme et la « tamponne » sur la grille.

> 💡 **C++ : `struct` vs `class`.** C'est presque la même chose. Par convention,
> on utilise `struct` pour un simple regroupement de données, et `class` pour un
> objet avec un comportement et des données cachées.

### 5.3 Les états d'une cellule : `CellState`, `AliveState`, `DeadState`, `StoneState`

```cpp
class CellState {
    public :
        virtual ~CellState() = default;
        virtual bool isAlive() const = 0;
        virtual bool isStone() const { return false; }
        virtual std::string symbol() const = 0;
};
```

`CellState` est une **classe abstraite** : elle décrit ce qu'un état *doit savoir
faire*, sans dire comment. Les trois **classes filles** donnent les réponses :

| Classe | `isAlive()` | `isStone()` | `symbol()` (affichage console) |
|---|---|---|---|
| `AliveState` | `true` | `false` |
| `DeadState` | `false` | `false` |
| `StoneState` | `false` | `true` |

> 💡 **C++ : héritage.** `class AliveState : public CellState` veut dire
> « `AliveState` *est un* `CellState` ». Elle hérite de tout ce que `CellState`
> déclare et doit compléter ce qui manque.

> 💡 **C++ : `virtual`, `= 0` et `override`.**
> - `virtual` : cette méthode peut être **redéfinie** par les classes filles, et
>   c'est la version de la classe fille qui sera appelée.
> - `= 0` : la méthode n'a **pas de code** ici (« méthode virtuelle pure ») ; les
>   classes filles sont obligées de la définir. Une classe qui en contient une
>   est *abstraite* : on ne peut pas créer d'objet `CellState` directement.
> - `override` : dans la classe fille, signale « je redéfinis la méthode du
>   parent ». Le compilateur vérifie qu'on ne s'est pas trompé de nom.
> - `const` après les parenthèses : la méthode **ne modifie pas** l'objet.

> 💡 **C++ : `public` / `private`.** Ce qui est `public` est utilisable par tout le
> monde. Ce qui est `private` n'est utilisable qu'à l'intérieur de la classe. Cela
> protège les données contre les modifications accidentelles.

Ensuite, on crée **un seul exemplaire** de chaque état, partagé par toutes les cellules :

```cpp
const AliveState ALIVE_STATE;
const DeadState DEAD_STATE;
const StoneState STONE_STATE;
```

Une grille de 400 × 400 n'a donc pas 160 000 objets « état » : toutes les cellules
vivantes pointent vers le même `ALIVE_STATE`. C'est économe en mémoire, et cela
permet de comparer facilement : « est-ce que l'état de cette cellule est
`&STONE_STATE` ? ».

### 5.4 Les règles : `Rules` et `ConwayRules`

```cpp
class Rules {
    public :
        virtual const CellState* next(const CellState& current, int aliveCount) const = 0;
};
```

Une règle est une fonction : **(état actuel, nombre de voisines vivantes) → état suivant**.

`ConwayRules` implémente les règles classiques du tableau de la partie 1, plus
une ligne pour les pierres :

```cpp
if (current.isAlive()) {
    if (aliveCount == 2 || aliveCount == 3) return &ALIVE_STATE; // survie
    return &DEAD_STATE;                                          // mort
}
if (&current == &STONE_STATE) return &STONE_STATE;   // une pierre reste une pierre
if (aliveCount == 3) return &ALIVE_STATE;            // naissance
return &DEAD_STATE;                                  // reste morte
```

> 💡 **C++ : pointeurs (`*`) et références (`&`).** C'est la notion la plus
> déroutante du C++.
> - Un **pointeur** (`const CellState*`) est une *adresse* : il indique *où* se
>   trouve un objet en mémoire, comme une adresse postale. `&ALIVE_STATE` signifie
>   « l'adresse de `ALIVE_STATE` ». Avec un pointeur `p`, on appelle une méthode par
>   `p->isAlive()`.
> - Une **référence** (`const CellState& current`) est un *autre nom* pour un objet
>   existant. Elle évite de faire une copie. On l'utilise comme l'objet lui-même :
>   `current.isAlive()`.
> - `&current == &STONE_STATE` compare deux adresses : « est-ce exactement le même
>   objet ? ».

Pourquoi une classe séparée ? Pour **ajouter d'autres règles** (par exemple
« HighLife », où une cellule naît aussi avec 6 voisines), il suffirait d'écrire
`class HighLifeRules : public Rules { ... }` et de la donner à l'`Organism`. Aucune
autre classe n'aurait besoin de changer.

### 5.5 Une case de la grille : `Cell`

```cpp
class Cell {
    private :
        const CellState* state;     // état actuel
        const CellState* next;      // état à l'étape suivante
        int x, y;                   // position dans la grille
        std::list<Cell*> neighbors; // les voisines (des pointeurs vers d'autres Cell)
        ...
```

Méthodes importantes :

| Méthode | Ce qu'elle fait |
|---|---|
| `isAlive()` | demande à son état s'il est vivant |
| `getState()` | renvoie l'état actuel |
| `addNeighbor(c)` | ajoute la cellule `c` à la liste des voisines |
| `statesAround(rules)` | compte les voisines vivantes et demande aux règles l'état suivant, rangé dans `next` |
| `applyNext()` | remplace l'état actuel par `next` |

**Pourquoi deux attributs `state` et `next` ?** Parce que toutes les cellules
doivent changer *en même temps*. Si on modifiait chaque cellule directement, ses
voisines calculées après elle verraient déjà le nouvel état, ce qui fausserait le
résultat. On fait donc deux passes :

1. toutes les cellules calculent leur `next` (sans rien modifier de visible) ;
2. toutes les cellules appliquent leur `next`.

> 💡 **C++ : constructeur.** La ligne
> `Cell(const CellState* s, int posX, int posY) : state(s), next(s), x(posX), y(posY) {}`
> est le **constructeur** : la fonction appelée à la création d'une cellule. La
> partie après `:` (la « liste d'initialisation ») remplit les attributs.

> 💡 **C++ : boucle `for (Cell* neighbor : neighbors)`.** Se lit « pour chaque
> `neighbor` dans la liste `neighbors` ». C'est l'équivalent du `for x in liste`
> de Python.

### 5.6 La grille entière : `Organism`

`Organism` est la classe centrale du moteur. Elle possède toutes les cellules, sait
les faire évoluer, garde l'historique et gère les fichiers.

#### Ses attributs

| Attribut | Rôle |
|---|---|
| `length`, `width` | nombre de lignes et de colonnes |
| `withstones` | les pierres sont-elles activées ? |
| `tauric` | la grille est-elle torique ? |
| `saving`, `filepathout` | faut-il écrire chaque étape dans un fichier, et lequel ? |
| `grid` | toutes les cellules |
| `rules` | les règles à utiliser (référence vers un objet `Rules`) |
| `save` | **l'historique** : une copie (vivant/mort) de la grille à chaque étape |
| `step` | numéro de l'étape actuelle |

#### La grille : un tableau à 1 dimension

```cpp
Cell** grid;
grid = new Cell*[length * width];
grid[i * width + j] = new Cell(...);
```

On pourrait s'attendre à un tableau à 2 dimensions `grid[i][j]`. Ici, toutes les
cases sont mises **bout à bout** dans une seule liste. La case de la ligne `i`,
colonne `j` se trouve à la position `i * width + j`.

Exemple avec 3 colonnes :

```
Grille :            Tableau 1D :
 (0,0) (0,1) (0,2)   index 0 1 2
 (1,0) (1,1) (1,2)   index 3 4 5      → (1,2) est à 1*3 + 2 = 5
```

Cette même convention d'indice `k = i * width + j` est utilisée **partout** dans le
projet, y compris dans l'interface graphique.

> 💡 **C++ : `new` et `delete`.** `new Cell(...)` crée un objet en mémoire et
> renvoie son adresse. En C++, la mémoire créée avec `new` n'est **pas libérée
> automatiquement** : il faut appeler `delete` soi-même. C'est le rôle du
> **destructeur** `~Organism()`, appelé automatiquement quand l'organisme est
> détruit : il fait `delete` sur chaque cellule puis `delete[]` sur le tableau.

#### Créer la grille : trois façons

| Méthode | Source de la grille |
|---|---|
| `init_organism()` | **aléatoire** : chaque case a ~1 chance sur 9 d'être une pierre (si activées), sinon 1 chance sur 2 d'être vivante |
| `init_from_file(path, error)` | **un fichier `.txt`** (voir le format ci-dessous). Renvoie `false` et remplit `error` si le fichier est invalide |
| `init_from_cells(cells)` | **un dessin** fait par l'utilisateur dans l'interface (liste de 0/1/2) |

Les trois préparent une liste d'états puis appellent la même méthode privée
`buildGrid(states)`, qui :

1. crée toutes les cellules ;
2. relie chaque cellule à ses voisines (voir ci-dessous) ;
3. enregistre l'étape 0 dans l'historique (`saveGrid()`) ;
4. si la sauvegarde est activée, crée le fichier de sortie et y écrit l'étape 0.

> 💡 **C++ : passage par référence pour « renvoyer » plusieurs choses.** Dans
> `bool init_from_file(const std::string& path, std::string& error)`, la fonction
> renvoie `true`/`false`, et *en plus* écrit le message d'erreur dans `error`.
> Comme `error` est une référence (`&`), c'est la variable de l'appelant qui est
> modifiée.

**Format du fichier d'entrée** (exemple : `exemple.txt`) :

```
15 15            ← nombre de lignes, nombre de colonnes
0 1 0 0 0 ...    ← puis une valeur par cellule, ligne par ligne
0 0 1 0 0 ...       0 = morte, 1 = vivante, 2 = pierre
...                 (espaces et retours à la ligne ignorés)
```

Si les pierres sont désactivées, les `2` sont lus comme des cellules mortes.

#### Relier les voisines : bords fermés ou torique

C'est ici, et **seulement ici**, que le mode torique fait une différence.

**Bords fermés** — `cellsAroundExist(i, j)` : on regarde les 8 cases autour
(`dx` et `dy` vont de -1 à +1) et on n'ajoute que celles qui sont **dans** la
grille. Une cellule de coin n'a donc que 3 voisines.

**Torique** — `defineCellsAround(i, j)` : on utilise le **modulo** (`%`, le reste
de la division) pour « faire le tour » :

```cpp
int x = (i + dx + length) % length;
int y = (j + dy + width) % width;
```

Exemple avec 10 lignes : la voisine du dessus de la ligne 0 est la ligne
`(0 - 1 + 10) % 10 = 9`, c'est-à-dire la dernière ligne. On ajoute `length` avant
le modulo pour éviter un nombre négatif.

#### Avancer d'une étape : `nextstep()`

```cpp
void nextstep() {
    step++;
    // 1re passe : chaque cellule calcule son état suivant
    for (...) grid[i * width + j]->statesAround(rules);
    // 2e passe : chaque cellule applique son état suivant
    for (...) grid[i * width + j]->applyNext();
    saveGrid();                 // ajoute la nouvelle grille à l'historique
    if (saving) resultSave();   // et l'écrit dans le fichier si demandé
}
```

#### L'historique et la détection de stabilité

- `saveGrid()` ajoute à `save` une copie de la grille sous forme de liste de
  `bool` (vivant ou non).
- `getGrid(s)` renvoie la grille de l'étape `s`. C'est ce qui permet à
  l'interface de **revenir en arrière** (bouton « Précédent »).
- `alreadySeen()` cherche si la grille actuelle est **identique** à une grille
  précédente (parmi les 1000 dernières au plus, pour rester rapide). Si oui, la
  simulation va se répéter à l'infini : on considère l'organisme **stable**.

> 💡 **C++ : `std::find(debut, fin, valeur)`.** Cherche `valeur` entre `debut` et
> `fin`. S'il ne trouve rien, il renvoie `fin`. D'où le test `!= save.end() - 1` :
> « on a trouvé quelque chose avant la dernière case (qui est la grille actuelle
> elle-même) ».

#### Écrire dans un fichier : `createSaveFile()` et `resultSave()`

- `createSaveFile()` crée (ou vide) le fichier de sortie et écrit sa taille,
  ex. `50 x 50`.
- `resultSave()` ajoute à la fin du fichier `Etape N` suivi de la grille
  (0 / 1 / 2). Voir `resultat.txt` pour un exemple.

> 💡 **C++ : fichiers.** `std::ifstream` lit un fichier (*input*), `std::ofstream`
> écrit dans un fichier (*output*). `std::ios::app` signifie « ajouter à la fin »
> (*append*) au lieu d'écraser. `fichier << "texte"` écrit, `fichier >> variable` lit.

#### Les réglages à faire **avant** de créer la grille

```cpp
Organism o("nom", 50, 50, rules, true);   // 50x50, avec pierres
o.setTauric(true);                        // AVANT init_...
o.setOutputFile("resultat.txt");          // AVANT init_...
o.init_organism();
```

`setTauric` et `setOutputFile` doivent être appelés avant `init_...`, car c'est
`buildGrid` qui relie les voisines et crée le fichier.

> 💡 **C++ : paramètre par défaut.** Dans le constructeur
> `Organism(std::string n, int l, int w, const Rules& r, bool useStones = false)`,
> `= false` veut dire que l'on peut omettre ce paramètre : il vaudra alors `false`.

### 5.7 Le mode console : `runConsoleSimulation()`

```cpp
void runConsoleSimulation(Organism& o, int delayMs = 0, int maxSteps = 0)
```

Fonction (hors de toute classe) qui fait tourner la simulation dans le terminal :
elle appelle `o.nextstep()` en boucle et affiche le numéro d'étape, jusqu'à ce que :

- l'organisme soit stable (`alreadySeen()`), **ou**
- le nombre maximum d'étapes soit atteint (`maxSteps`, 0 = pas de limite).

La condition `o.getStep() < 2 || ...` force au moins deux étapes avant de tester
la stabilité.

En mode console, la grille n'est pas dessinée dans le terminal : les étapes sont
écrites dans le **fichier de sauvegarde**, qu'on peut consulter ensuite.

`"\033[2J\033[H"` est un code spécial qui efface le terminal et remet le curseur
en haut.

---

## 6. L'interface graphique : `affichage_main.cpp`

L'interface utilise **SFML**, une bibliothèque C++ pour ouvrir une fenêtre, dessiner
des formes et du texte, et réagir à la souris et au clavier. Tout ce qui commence
par `sf::` vient de SFML (`sf::Color`, `sf::RenderWindow`, `sf::Event`…).

Le fichier est organisé du plus petit au plus gros :

```
1. Thème + fonctions de dessin   (couleurs, rectangle arrondi, texte, grille)
2. Widgets                        (Button, InputBox → TextBox / NumberBox)
3. Settings                       (tous les choix de l'utilisateur)
4. Page (abstraite)               (un écran)
5. App + les 4 pages              (la fenêtre et la navigation)
6. main()
```

### 6.0 Comment fonctionne un programme graphique ?

Un programme graphique tourne dans une **boucle infinie**, environ 60 fois par
seconde (60 « images » ou *frames*). À chaque tour :

1. on **place** les éléments selon la taille actuelle de la fenêtre ;
2. on **traite les événements** (clic, touche, redimensionnement…) ;
3. on **met à jour** ce qui doit l'être (ex. : avancer la simulation) ;
4. on **efface** l'écran et on **redessine** tout.

C'est exactement ce que fait `App::run()` (voir 6.5).

### 6.1 Le thème et les fonctions de dessin

```cpp
namespace Theme {
    const sf::Color Background(15, 23, 42);   // bleu-gris foncé
    const sf::Color Alive(52, 211, 153);      // vert : cellules vivantes
    const sf::Color Stone(245, 158, 11);      // ambre : pierres
    ...
}
```

Toutes les couleurs sont regroupées à un seul endroit (valeurs Rouge, Vert, Bleu
de 0 à 255). Pour changer l'apparence, il suffit de modifier ce bloc.

> 💡 **C++ : `namespace`.** Un espace de noms regroupe des éléments sous un même
> préfixe. On écrit `Theme::Alive` pour la couleur des cellules vivantes, ce qui
> évite les confusions avec d'autres variables nommées `Alive`.

Fonctions utilitaires :

| Fonction | Rôle |
|---|---|
| `roundedRect(box, radius)` | fabrique un rectangle aux coins arrondis (SFML n'en a pas) : un polygone avec un quart de cercle à chaque coin |
| `drawText(...)` | écrit un texte, éventuellement centré ; gère les accents (UTF-8) |
| `GridView::fit(area, rows, cols)` | calcule la taille d'une case pour que la grille soit **la plus grande possible** dans une zone, et centrée |
| `GridView::cellAt(px, py, i, j)` | inverse : quel case `(i, j)` se trouve sous le pixel `(px, py)` ? (utile pour les clics) |
| `drawCells(window, view, tauric, colorOf)` | dessine toutes les cases ; la couleur de chaque case est donnée par la fonction `colorOf(k)`. Le cadre est indigo si la grille est torique |

> 💡 **C++ : `std::function` et les lambdas.** `std::function<sf::Color(int)>` est
> une variable qui contient **une fonction** prenant un `int` et renvoyant une
> couleur. On peut y mettre une **lambda**, c'est-à-dire une petite fonction écrite
> sur place :
> ```cpp
> [&](int k) { return alive[k] ? Theme::Alive : Theme::Dead; }
> ```
> - `[ ]` : ce que la lambda « capture » de l'extérieur (`&` = tout, par référence ;
>   `this` = l'objet courant).
> - `(int k)` : ses paramètres.
> - `{ ... }` : son code.
> - `condition ? a : b` vaut `a` si la condition est vraie, sinon `b`.

Grâce à cela, la même fonction `drawCells` sert à la fois pour la simulation et
pour la page de dessin : seule la façon de choisir la couleur change.

### 6.2 Les widgets (éléments d'interface)

#### `Button`

Un rectangle cliquable avec un texte. **Une seule classe** suffit pour tous les
boutons, car l'action à effectuer est donnée à la création sous forme de lambda :

```cpp
Button start("Démarrer", [this] { startGame(); });
Button tauricButton("Torique",
                    [this] { app.settings.tauric = true; },    // au clic
                    [this] { return app.settings.tauric; });   // est-il sélectionné ?
```

- 1er paramètre : le texte affiché ;
- 2e paramètre `onClick` : ce qu'il faut faire au clic ;
- 3e paramètre facultatif `isSelected` : renvoie `true` si le bouton doit être
  affiché en surbrillance (option choisie).

Méthodes :
- `setBox(rect)` : position et taille (décidées par la page) ;
- `handleEvent(event)` : si c'est un clic gauche dans le rectangle → appelle `onClick()` ;
- `draw(window, font)` : dessine le bouton (indigo si sélectionné, plus clair si la
  souris le survole).

#### `InputBox`, `TextBox`, `NumberBox`

Des zones où l'utilisateur tape du texte. `InputBox` est **abstraite** et contient
tout le code commun : clic pour activer la zone, touche Retour arrière, Entrée,
dessin, curseur `|`. Elle laisse deux choses aux classes filles :

| Méthode virtuelle | `TextBox` (chemins de fichiers) | `NumberBox` (nombres) |
|---|---|---|
| `accepts(c)` : ce caractère est-il autorisé ? | oui, tant que le texte fait moins de 50 caractères | seulement les chiffres, avec un nombre max de chiffres |
| `apply()` : que faire du texte ? | le copier dans la chaîne visée | le convertir en entier (`std::stoi`) et le copier dans l'entier visé |

Point clé : chaque zone garde une **référence** vers le réglage qu'elle modifie
(`std::string& target` ou `int& target`). Taper dans la zone modifie donc
**directement** le réglage correspondant dans `Settings`.

> 💡 **C++ : `protected`.** Entre `public` et `private` : accessible dans la classe
> et dans ses classes filles, mais pas ailleurs.

### 6.3 `Settings` : tous les choix de l'utilisateur

```cpp
struct Settings {
    bool graphicMode = true;      // false = simulation dans le terminal
    bool fromFile = false;        // grille lue depuis un fichier ?
    bool drawGrid = false;        // grille dessinée à la main ?
    std::string inputPath = "exemple.txt";
    std::string outputPath = "resultat.txt";
    int length = 50, width = 50;
    bool tauric = false;
    bool stones = false;
    bool limitSteps = false;
    int maxSteps = 100;
    int delayMs = 50;             // pause entre deux étapes (mode graphique)
    std::string savePath() const; // où sauvegarder les étapes
};
```

Une simple boîte de données, avec des valeurs par défaut. Toutes les pages la lisent
et la modifient.

`savePath()` décide où les étapes sont enregistrées :

| Mode | Grille | Fichier de sauvegarde |
|---|---|---|
| Graphique | n'importe laquelle | aucun (l'historique est en mémoire) |
| Console | aléatoire ou dessinée | le chemin choisi par l'utilisateur (`resultat.txt` par défaut) |
| Console | depuis un fichier `X` | `X_out.txt` |

`enum class PageId { Mode, Settings, Draw, Simulation };` donne un nom à chaque
écran.

> 💡 **C++ : `enum class`.** Une liste de valeurs nommées. Une variable de type
> `PageId` ne peut valoir que `PageId::Mode`, `PageId::Settings`, etc. C'est plus
> clair et plus sûr que des numéros 0, 1, 2, 3.

### 6.4 `Page` : la classe abstraite des écrans

```cpp
class Page {
    public :
        virtual void layout(float width, float height) = 0; // placer les widgets
        virtual void handleEvent(const sf::Event& event) = 0; // réagir aux clics/touches
        virtual void update() {}                              // avancer dans le temps
        virtual void draw(sf::RenderWindow& window, const sf::Font& font) = 0; // dessiner
    protected :
        App& app;   // accès à l'application (réglages, organisme, navigation)
};
```

Chaque écran est une classe fille qui remplit ces 4 méthodes. `App` n'a pas besoin
de savoir sur quelle page il se trouve : il appelle simplement `current->draw(...)`
et c'est la bonne version qui s'exécute. C'est le **polymorphisme**, comme pour
`CellState` et `Rules`.

`layout` est appelé **à chaque image** : comme la fenêtre peut changer de taille
(plein écran, redimensionnement), aucune position n'est fixée une fois pour toutes.

### 6.5 `App` : l'application

`App` possède tout ce qui est partagé :

| Attribut | Rôle |
|---|---|
| `window` | la fenêtre SFML |
| `font` | la police de caractères |
| `settings` | les réglages de l'utilisateur |
| `rules` | les règles de Conway |
| `organism` | l'organisme en cours (créé au clic sur « Démarrer ») |
| `current` | la page affichée |
| `fullscreenButton` | le bouton « Plein écran » en haut à droite, présent sur toutes les pages |

> 💡 **C++ : `std::unique_ptr`.** Un pointeur « intelligent » : il possède l'objet
> pointé et le **détruit automatiquement** quand on n'en a plus besoin (quand on le
> remplace ou que l'`App` est détruite). Pas besoin d'écrire `delete`.
> `std::make_unique<ModePage>(*this)` crée une `ModePage` et renvoie un tel pointeur.

Méthodes importantes :

- **`goTo(id)`** : demande un changement de page. Le changement n'a pas lieu tout
  de suite mais **au début de l'image suivante**. Pourquoi ? `goTo` est appelé
  depuis un bouton de la page actuelle. Si on détruisait la page immédiatement, on
  détruirait le bouton *pendant* qu'il est en train de s'exécuter → plantage.
- **`switchPage()`** : effectue réellement le changement en créant la nouvelle page.
- **`quitToConsole()`** : ferme la fenêtre ; `main()` prend alors le relais dans
  le terminal.
- **`createWindow()`** : (re)crée la fenêtre à la taille de l'écran, en plein écran ou non.
- **`loadFont()`** : essaie plusieurs polices connues (Linux, Windows, macOS) et
  garde la première trouvée.
- **`run()`** : la boucle principale :

```cpp
while (window.isOpen()) {
    // 1. changements demandés (plein écran, page)
    // 2. placer les widgets selon la taille de la fenêtre
    current->layout(width, height);
    // 3. traiter tous les événements en attente
    while (window.pollEvent(event)) {
        fermeture → window.close()
        redimensionnement → adapter la vue
        Échap en plein écran → quitter le plein écran
        sinon → fullscreenButton.handleEvent(event); current->handleEvent(event);
    }
    // 4. mise à jour
    current->update();
    // 5. dessin
    window.clear(Theme::Background);
    current->draw(window, font);
    fullscreenButton.draw(window, font);
    window.display();
}
```

> 💡 **C++ : déclaration avant définition.** `class App;` (seul, plus haut) annonce
> « une classe `App` existe, elle sera définie plus loin ». Cela permet à `Page` de
> l'utiliser avant sa définition complète. De même, `App::switchPage()` est écrite
> **après** les pages, car elle a besoin de les connaître pour les créer.
> `App::run()` signifie « la méthode `run` de la classe `App` », écrite en dehors
> de la classe.

### 6.6 Les quatre pages

#### `ModePage` — choisir le mode

Un titre et deux boutons : **Console** et **Graphique**. Le clic enregistre le choix
dans `settings.graphicMode` et va vers `SettingsPage`.

#### `SettingsPage` — les réglages

Contient tous les boutons et zones de saisie :

| Ligne | Contrôles | Visible quand |
|---|---|---|
| Grille de départ | Aléatoire / Fichier .txt / Dessiner | toujours |
| Taille | Lignes, Colonnes (`NumberBox`) | grille aléatoire ou dessinée |
| Fichier de la grille | `TextBox` | grille depuis un fichier |
| Fichier de sauvegarde | `TextBox` | console + grille non issue d'un fichier |
| Bords | Bords fermés / Torique | toujours |
| Pierres | Avec / Sans | toujours |
| Durée de vie | Jusqu'à stabilité / Nombre d'étapes fixé | toujours |
| Nombre d'étapes | `NumberBox` | si « Nombre d'étapes fixé » |
| Délai (ms) | `NumberBox` | mode graphique |
| | Retour / Démarrer | toujours |

Comme certaines lignes n'apparaissent que dans certains cas, `layout()` empile
uniquement les lignes visibles et calcule l'espacement pour que tout tienne dans
la fenêtre.

Au clic sur **Démarrer**, `startGame()` :

1. **vérifie** les réglages (taille entre 3×3 et 400×400, au moins 1 étape) et
   affiche une erreur en rouge sinon ;
2. si « Dessiner » est choisi → va vers `DrawPage` (l'organisme sera créé là-bas) ;
3. sinon **crée l'`Organism`**, règle le mode torique et le fichier de sauvegarde,
   puis le remplit (aléatoirement ou depuis le fichier — en cas d'erreur de
   lecture, le message s'affiche et on reste sur la page) ;
4. va vers `SimulationPage` (mode graphique) ou appelle `quitToConsole()` (mode console).

#### `DrawPage` — dessiner sa grille

- À gauche : la grille vide. À droite : un bouton par **outil**.
- Outils : *Cellule*, *Gomme*, *Pierre* (si les pierres sont activées), puis toutes
  les formes de `motifs` (rond, carré, étoiles, vaisseaux…).
- Chaque outil est une petite structure `Tool` : un nom, une taille `w × h`, un
  motif de 0/1, et la valeur à écrire (0 = morte, 1 = vivante, 2 = pierre).
- **Clic gauche** : « tamponne » l'outil centré sur la case cliquée (`stamp`).
  Avec un outil d'une seule case, on peut maintenir et glisser pour peindre.
- **Clic droit** : efface (et glisser pour effacer plusieurs cases).
- Sous la souris, un **aperçu** semi-transparent montre où la forme sera posée
  (`drawPreview`).
- En mode torique, une forme posée près d'un bord **déborde de l'autre côté**
  (fonction `target`, même principe de modulo que dans le moteur).
- Le dessin est stocké dans `std::vector<int> cells` (même indice
  `k = i * width + j` que l'`Organism`).
- **Lancer** crée l'`Organism` avec `init_from_cells(cells)`, puis va vers la
  simulation ou la console.

`toolButtons.reserve(...)` réserve la place à l'avance pour que les boutons ne
soient jamais déplacés en mémoire (les lambdas qu'ils contiennent font référence
à leur position).

#### `SimulationPage` — regarder la simulation

- En haut : `Étape 12 / 40`, puis éventuellement `(limite 100)`, `torique`, et la
  raison de la fin.
- Au centre : la grille, la plus grande possible (`GridView::fit`).
- En bas : **Retour**, **Précédent**, **Pause/Lecture**, **Suivant**.

Fonctionnement :

- `view` est l'étape **affichée**, qui peut être plus ancienne que la dernière
  étape calculée (`org.getStep()`), car l'utilisateur peut revenir en arrière.
- `update()` est appelée à chaque image : si la lecture est en cours et que
  `delayMs` millisecondes se sont écoulées (mesurées par `sf::Clock`), on appelle
  `showNext()`.
- `showNext()` :
  - si on regarde une ancienne étape → on avance simplement dans l'historique ;
  - sinon → `org.nextstep()` calcule une nouvelle étape, puis on vérifie si
    l'organisme est stable (`alreadySeen()`) ou si la limite d'étapes est atteinte.
    Dans ce cas, `endReason` est rempli et la lecture s'arrête.
- `drawGrid()` récupère la grille de l'étape `view` (`org.getGrid(view)`) et la
  dessine : ambre pour les pierres, vert pour les vivantes, gris foncé pour les mortes.
  Les pierres ne changeant jamais, leur position n'est lue qu'une fois, à la
  création de la page (`org.getStones()`).

### 6.7 `main()` : le point de départ

> 💡 **C++ : `main()`.** Tout programme C++ commence par la fonction `main`. Quand
> elle se termine, le programme s'arrête. `return 0` signifie « tout s'est bien
> passé », `return 1` « il y a eu une erreur ».

```cpp
int main() {
    std::srand(std::time(nullptr));   // initialise le hasard avec l'heure actuelle
    App app;                          // crée la fenêtre et affiche ModePage
    if (!app.loadFont()) { ... return 1; }
    app.run();                        // boucle principale, jusqu'à la fermeture

    // Si "Console" a été choisi : la fenêtre est fermée, on continue dans le terminal
    if (app.consoleRequested && app.organism) {
        runConsoleSimulation(*app.organism, 0, maxSteps);
        ...affiche le nom du fichier de sauvegarde...
    }
    return 0;
}
```

`std::srand(std::time(nullptr))` est nécessaire pour que la grille aléatoire soit
différente à chaque lancement (sinon `rand()` donnerait toujours la même suite).

---

## 7. Le parcours complet d'une partie

### En mode graphique

```
main()
 └─ App()                → crée la fenêtre, page = ModePage
 └─ app.run()            → boucle 60 fois/s
      ModePage           → clic "Graphique"   → settings.graphicMode = true
      SettingsPage       → réglages, clic "Démarrer"
                             └─ startGame()
                                  ├─ new Organism(...)
                                  ├─ setTauric(), setOutputFile("")
                                  └─ init_organism() / init_from_file()
                                        └─ buildGrid() → cellules + voisines + étape 0
      (ou DrawPage       → dessin, clic "Lancer" → init_from_cells())
      SimulationPage     → toutes les delayMs ms :
                             showNext()
                               └─ org.nextstep()
                                    ├─ chaque Cell::statesAround(rules)
                                    │     └─ ConwayRules::next(état, nbVivantes)
                                    ├─ chaque Cell::applyNext()
                                    └─ saveGrid()  → historique
                               └─ org.alreadySeen() ? → fin
```

### En mode console

```
ModePage (clic "Console") → SettingsPage → startGame()
   └─ Organism créé avec un fichier de sauvegarde
   └─ app.quitToConsole() → la fenêtre se ferme, run() se termine
main() reprend :
   └─ runConsoleSimulation(organism)
        └─ boucle nextstep() + écriture de chaque étape dans le fichier
           jusqu'à stabilité ou limite d'étapes
```

---

## 8. Historique des versions

| Version | Fichiers | Ce qui a changé |
|---|---|---|
| **V0** | `Jeu_de_la_vie_sans_sauvegarde.cpp`, `Jeu_de_la_vie_avec_sauvegarde.cpp`, `interface_graphique.cpp` | Première version : une cellule stocke un simple `bool` (vivante/morte). Version sans puis avec historique pour détecter les boucles. Première interface SFML à taille fixe (1100 × 760). |
| **V1** | `JDLV_save.cpp`, `affichage.cpp`, `affichage_simple.cpp` | Introduction des **états polymorphes** (`CellState`), des **règles** (`Rules`), des **pierres** et de la lecture de grille depuis un `.txt`. Interface à 3 écrans (Mode / Réglages / Simulation) en style « immediate mode » (chaque bouton est une fonction appelée à chaque image). |
| **V2** | `JDLV_taurique.cpp`, `affichage_main.cpp` | Ajout du **mode torique**, des **motifs** et de la **page de dessin**, de la limite d'étapes, de la sauvegarde configurable. Interface réécrite **en objets** (classes `Button`, `InputBox`, `Page`, `App`), adaptée à toutes les tailles d'écran, avec plein écran. |

---

## 9. Petit lexique C++

| Notion | En une phrase | Exemple dans le projet |
|---|---|---|
| Classe / objet | Un plan / une chose construite selon ce plan | `class Cell` / chaque case de la grille |
| Attribut | Une donnée d'un objet | `step`, `length` dans `Organism` |
| Méthode | Une fonction d'un objet | `nextstep()` |
| Constructeur | Fonction appelée à la création d'un objet | `Organism(...)` |
| Destructeur `~` | Fonction appelée à la destruction d'un objet | `~Organism()` libère les cellules |
| `public` / `private` / `protected` | Qui a le droit d'utiliser un membre | `Cell::state` est `private` |
| Héritage `: public` | Une classe « est une sorte de » une autre | `AliveState : public CellState` |
| `virtual` / `override` | Méthode redéfinissable / redéfinie | `isAlive()` |
| `= 0` | Méthode sans code, à définir obligatoirement par les filles | `Rules::next` |
| Classe abstraite | Classe avec au moins une méthode `= 0`, non instanciable | `CellState`, `Rules`, `Page`, `InputBox` |
| Polymorphisme | Appeler la même méthode et obtenir le comportement de la vraie classe | `current->draw(...)` |
| Pointeur `T*` | Adresse d'un objet | `Cell*` dans `neighbors` |
| `&x` | L'adresse de `x` | `&ALIVE_STATE` |
| `p->f()` | Appeler `f` sur l'objet pointé par `p` | `neighbor->isAlive()` |
| Référence `T&` | Autre nom d'un objet existant, sans copie | `const Rules& rules` |
| `const` | « Ne sera pas modifié » | `isAlive() const` |
| `new` / `delete` | Créer / détruire un objet à la main | `new Cell(...)` |
| `std::unique_ptr` | Pointeur qui détruit tout seul son objet | `App::organism` |
| `std::vector`, `std::list` | Listes d'éléments | `std::vector<bool> save` |
| Lambda `[...](...) {...}` | Petite fonction écrite sur place | actions des boutons |
| `std::function` | Variable contenant une fonction | `Button::onClick` |
| `namespace` | Préfixe qui regroupe des noms | `Theme::Alive`, `std::cout` |
| `enum class` | Liste de valeurs nommées | `PageId::Simulation` |
| `#include` | Insérer un autre fichier | `#include "JDLV_taurique.cpp"` |
| `a ? b : c` | `b` si `a` est vrai, sinon `c` | `playing ? "Pause" : "Lecture"` |
| `%` (modulo) | Reste de la division entière | bords toriques |
