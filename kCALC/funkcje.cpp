#include "funkcje.h"
#include "submenus.h"
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <curses.h>
#include <thread>
#include <chrono>
#include <cstdio>
#include <unordered_map>
#include <string>

const std::vector<Meal>& CalorieTracker::getMealsToday() const {
    return mealsToday;
}

double CalorieTracker::getDailyTarget() const {
    return dailyTarget;
}

std::string normalize_ascii(const std::string& input) {
    std::string out;
    for (size_t i = 0; i < input.size(); ) {
        unsigned char c1 = (unsigned char)input[i];
        if (c1 == 0xC4 && i + 1 < input.size()) {
            unsigned char c2 = (unsigned char)input[i + 1];
            switch (c2) {
            case 0x84: out += 'A'; break; // Ą
            case 0x86: out += 'C'; break; // Ć
            case 0x98: out += 'E'; break; // Ę
            case 0x81: out += 'L'; break; // Ł
            case 0x87: out += 'c'; break; // ć
            case 0x99: out += 'e'; break; // ę
            case 0x82: out += 'l'; break; // ł
            default: out += '?'; break;
            }
            i += 2;
        }
        else if (c1 == 0xC5 && i + 1 < input.size()) {
            unsigned char c2 = (unsigned char)input[i + 1];
            switch (c2) {
            case 0x84: out += 'n'; break; // ń
            case 0x81: out += 'N'; break; // Ń
            case 0x9B: out += 's'; break; // ś
            case 0x9A: out += 'S'; break; // Ś
            case 0xB9: out += 'z'; break; // ź
            case 0xB8: out += 'Z'; break; // Ź
            case 0xBC: out += 'z'; break; // ż
            case 0xBB: out += 'Z'; break; // Ż
            case 0xB3: out += 'o'; break; // ó
            case 0x93: out += 'O'; break; // Ó
            default: out += '?'; break;
            }
            i += 2;
        }
        else {
            out += input[i];
            ++i;
        }
    }
    return out;
}

// --- Improved Back Button ---

void drawBackButton(int y, int x, bool hovered) {
    const char* label = "Wroc";
    int width = (int)strlen(label) + 4; // padding
    if (hovered)
        attron(COLOR_PAIR(2));
    else
        attron(COLOR_PAIR(1));
    mvprintw(y - 2, x, "+------+");
    mvprintw(y - 1, x, "| Wroc |");
    mvprintw(y, x, "+------+");
    if (hovered)
        attroff(COLOR_PAIR(2));
    else
        attroff(COLOR_PAIR(1));
}

// --- Funkcje pomocnicze do zapisu/odczytu celu kalorii ---
double loadCalorieTarget() {
    std::ifstream file("target.txt");
    double val = 0;
    if (file >> val && val > 0)
        return val;
    return 0;
}

void saveCalorieTarget(double val) {
    std::ofstream file("target.txt", std::ios::trunc);
    if (file) file << val << std::endl;
}

// --- CalorieTracker methods ---

CalorieTracker::CalorieTracker(double target) : dailyTarget(target) {}

void CalorieTracker::loadMeals() {
    std::ifstream file("meals.txt");
    if (!file) return;

    mealsToday.clear();
    Meal m;
    while (getline(file, m.datetime)) {
        getline(file, m.name);
        file >> m.kcal >> m.protein >> m.carbs >> m.fat;
        file.ignore();
        mealsToday.push_back(m);
    }
    file.close();
}

void CalorieTracker::saveMeal(const Meal& meal) {
    std::ofstream file("meals.txt", std::ios::app);
    file << meal.datetime << "\n" << meal.name << "\n"
        << meal.kcal << " " << meal.protein << " "
        << meal.carbs << " " << meal.fat << "\n";
    file.close();
}

void CalorieTracker::addMeal(const Meal& meal) {
    mealsToday.push_back(meal);
    saveMeal(meal);
}

void CalorieTracker::showSummary() {
    // Pobierz dzisiejszą datę w formacie "YYYY-MM-DD"
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);
    char today[11];
    snprintf(today, sizeof(today), "%04d-%02d-%02d", 1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday);

    double sumKcal = 0, sumP = 0, sumC = 0, sumF = 0;
    for (const auto& m : mealsToday) {
        if (m.datetime.substr(0, 10) == today) {
            sumKcal += m.kcal;
            sumP += m.protein;
            sumC += m.carbs;
            sumF += m.fat;
        }
    }

    int row = 2;
    attron(COLOR_PAIR(1));
    box(stdscr, 0, 0);
    mvprintw(row++, 4, "==============================");
    mvprintw(row++, 4, " PODSUMOWANIE DNIA");
    mvprintw(row++, 4, " Zjedzone kcal: %.0f / %.0f", sumKcal, dailyTarget);
    mvprintw(row++, 4, " Bialko: %.1f g", sumP);
    mvprintw(row++, 4, " Wegle: %.1f g", sumC);
    mvprintw(row++, 4, " Tluszcze: %.1f g", sumF);
    mvprintw(row++, 4, "==============================");
    attroff(COLOR_PAIR(1));
    refresh();
}


void CalorieTracker::showMeals() {
    // Przygotuj listę linii do wyświetlenia: rozdziel na dni, wyciągnij godziny
    std::vector<std::string> lines;
    std::string last_date;
    char buf[256];

    // Odwróć kolejność: od najnowszych do najstarszych
    for (auto it = mealsToday.rbegin(); it != mealsToday.rend(); ++it) {
        const auto& m = *it;

        std::string date = m.datetime.substr(8, 2) + "." + m.datetime.substr(5, 2); // DD.MM
        std::string hour = m.datetime.substr(11, 5); // HH:MM
        if (last_date != date) {
            snprintf(buf, sizeof(buf), "----%s----", date.c_str());
            lines.push_back(buf);
            last_date = date;
        }
        snprintf(buf, sizeof(buf), "[%s] %s", hour.c_str(), m.name.c_str());
        lines.push_back(buf);
    }

    // Scrollable lista
    int top = 0;
    int visible = LINES - 8; // miejsce na ramkę, przycisk, nagłówek
    int y_back = LINES - 2, x_back = 4, width_back = 8;
    bool back_hover = false;

    while (true) {
        clear();
        attron(COLOR_PAIR(1));
        box(stdscr, 0, 0);
        mvprintw(1, 2, "Zjedzone posilki:");
        attroff(COLOR_PAIR(1));

        int row = 3;
        for (int i = 0; i < visible && (top + i) < (int)lines.size(); ++i) {
            mvprintw(row++, 4, "%s", lines[top + i].c_str());
        }

        drawBackButton(y_back, x_back, back_hover);
        refresh();

        request_mouse_pos();
        back_hover = (Mouse_status.y >= y_back - 2 && Mouse_status.y <= y_back && Mouse_status.x >= x_back && Mouse_status.x < x_back + width_back);

        int c = getch();
        if (c == KEY_UP || c == 'k') {
            if (top > 0) top--;
        }
        else if (c == KEY_DOWN || c == 'j') {
            if (top + visible < (int)lines.size()) top++;
        }
        else if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) {
            break;
        }
        else if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= y_back - 2 && event.y <= y_back && event.x >= x_back && event.x < x_back + width_back &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
                // Scroll myszką
                if (event.bstate & BUTTON4_PRESSED) { // scroll up
                    if (top > 0) top--;
                }
                if (event.bstate & BUTTON5_PRESSED) { // scroll down
                    if (top + visible < (int)lines.size()) top++;
                }
            }
        }
    }
}

// --- Pomocnicza funkcja do pobierania tylko liczb (z kropką) ---
bool get_numeric_input(char* buf, int maxlen, int y, int x) {
    int pos = 0;
    memset(buf, 0, maxlen + 1);
    move(y, x);
    curs_set(1);

    while (true) {
        int c = getch();
        if (c == '\n' || c == KEY_ENTER) {
            if (pos == 0) continue; // nie pozwól przejść dalej jeśli puste
            buf[pos] = '\0';
            break;
        }
        else if (c == KEY_BACKSPACE || c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                mvaddch(y, x + pos, ' ');
                move(y, x + pos);
            }
            clrtoeol();
            refresh();
        }
        else if (c >= 0 && c <= 255 && isdigit(c) && pos < maxlen) {
            buf[pos++] = (char)c;
            mvaddch(y, x + pos - 1, c);
            move(y, x + pos);
            refresh();
        }
        else if (c == '.' && pos < maxlen && strchr(buf, '.') == nullptr && pos > 0) {
            buf[pos++] = '.';
            mvaddch(y, x + pos - 1, '.');
            move(y, x + pos);
            refresh();
        }
        // ignoruj strzałki i inne specjalne klawisze
        else if (c == KEY_LEFT || c == KEY_RIGHT || c == KEY_UP || c == KEY_DOWN ||
                 c == KEY_HOME || c == KEY_END || c == KEY_PPAGE || c == KEY_NPAGE) {
            continue;
        }
        else {
            // Zignoruj znak, odśwież linijkę i ekran żeby nie pojawił się śmieć
            move(y, x);
            for (int i = 0; i < pos; ++i)
                mvaddch(y, x + i, buf[i]);
            clrtoeol(); 
            move(y, x + pos);
            refresh();
        }
    }
    return true;
}

std::string currentDateTime() {
    time_t now = time(0);
    tm ltm;
    localtime_s(&ltm, &now);

    std::ostringstream oss;
    oss << 1900 + ltm.tm_year << "-"
        << std::setw(2) << std::setfill('0') << 1 + ltm.tm_mon << "-"
        << std::setw(2) << std::setfill('0') << ltm.tm_mday << " "
        << std::setw(2) << std::setfill('0') << ltm.tm_hour << ":"
        << std::setw(2) << std::setfill('0') << ltm.tm_min;
    return oss.str();
}

void saveCustomMeal(const Meal& meal) {
    std::ofstream file("recipes.txt", std::ios::app);
    file << meal.name << "\n" << meal.kcal << " " << meal.protein << " "
        << meal.carbs << " " << meal.fat << "\n";
    file.close();
}

std::vector<Meal> loadRecipes() {
    std::vector<Meal> recipes;
    std::ifstream file("recipes.txt");
    if (!file) return recipes;

    Meal m;
    while (getline(file, m.name)) {
        file >> m.kcal >> m.protein >> m.carbs >> m.fat;
        file.ignore();
        recipes.push_back(m);
    }
    file.close();
    return recipes;
}

void displayRecipes(const std::vector<Meal>& recipes) {
    std::cout << "\n== GOTOWE POSILKI ==\n";
    int idx = 1;
    for (const auto& m : recipes) {
        std::cout << idx++ << ". " << m.name << " - " << m.kcal << " kcal, P: "
            << m.protein << ", C: " << m.carbs << ", F: " << m.fat << "\n";
    }
}

Meal inputMeal() {
    Meal m;
    std::cout << "Nazwa posilku: ";
    std::cin.ignore();
    getline(std::cin, m.name);
    std::cout << "Kcal: ";
    std::cin >> m.kcal;
    std::cout << "Bialko (g): ";
    std::cin >> m.protein;
    std::cout << "Wegle (g): ";
    std::cin >> m.carbs;
    std::cout << "Tluszcze (g): ";
    std::cin >> m.fat;
    m.datetime = currentDateTime();
    return m;
}

// --- Helper: betterListGetch ---

void betterListGetch() {
    nodelay(stdscr, FALSE); // bo psuje się z dynamicznym hoverem - trybem blokującym
    int c;
    while (true) {
        c = getch();
        // Ignoruj kliknięcia myszą i Enter
        if (c != KEY_MOUSE && c != '\n' && c != KEY_ENTER)
            break;
    }
    nodelay(stdscr, TRUE);
}

// --- Main menu ---

void menu() {
    double target = loadCalorieTarget();
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_BLACK, COLOR_GREEN);
    init_pair(3, COLOR_WHITE, COLOR_WHITE);   // jasny słupek
    init_pair(4, COLOR_GREEN, COLOR_GREEN);   // ciemny słupek
    init_pair(5, COLOR_BLACK, COLOR_WHITE);

    attron(COLOR_PAIR(1));
    mvprintw(1, 2, "kCALC");
    attroff(COLOR_PAIR(1));

    if (target <= 0) {
        mvprintw(3, 2, "Podaj dzienny cel kcal: ");
        char buf[32];
        get_numeric_input(buf, 31, 3, 25);
        target = atof(buf);
        if (target > 0) saveCalorieTarget(target);
    }

    attron(COLOR_PAIR(1));
    box(stdscr, 0, 0);
    mvprintw(1, 2, "kCALC");
    attroff(COLOR_PAIR(1));
    refresh();

    CalorieTracker tracker(target);
    tracker.loadMeals();

    int highlight = 0;
    const int menuSize = 9;
    const char* options[menuSize] = {
        "Dodaj posilek recznie",
        "Dodaj posilek z listy przepisow",
        "Dodaj nowy przepis do listy",
        "Pokaz podsumowanie dnia",
        "Pokaz historie posilkow",
        "Zmien dzienny cel kcal",
        "Wyczysc historie posilkow",
        "Pokaz wykres kcal (30 dni)",
        "Wyjdz"
    };

    nodelay(stdscr, TRUE);

    while (true) {
        clear();
        attron(COLOR_PAIR(1));
        box(stdscr, 0, 0);
        mvprintw(1, 2, "kCALC");
        attroff(COLOR_PAIR(1));

        for (int i = 0; i < menuSize; ++i) {
            if (i == highlight) {
                attron(COLOR_PAIR(2));
                mvprintw(3 + i, 4, "> %s", options[i]);
                attroff(COLOR_PAIR(2));
            }
            else {
                mvprintw(3 + i, 6, "%s", options[i]);
            }
        }
        refresh();
        int ch = getch();

        // Dynamiczny hover myszką
        request_mouse_pos();
        for (int i = 0; i < menuSize; ++i) {
            int y = 3 + i, x1 = 4, x2 = 4 + (int)strlen(options[i]) + 2;
            if (Mouse_status.y == y && Mouse_status.x >= x1 && Mouse_status.x <= x2) {
                highlight = i;
                break;
            }
        }

        bool wybrano = false;
        if (ch == KEY_UP) {
            highlight = (highlight - 1 + menuSize) % menuSize;
        }
        else if (ch == KEY_DOWN) {
            highlight = (highlight + 1) % menuSize;
        }
        else if (ch == '\n' || ch == KEY_ENTER) {
            wybrano = true;
        }
        else if (ch == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                for (int i = 0; i < menuSize; ++i) {
                    int y = 3 + i, x1 = 4, x2 = 4 + (int)strlen(options[i]) + 2;
                    if (event.y == y && event.x >= x1 && event.x <= x2) {
                        if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED)) {
                            wybrano = true;
                        }
                        break;
                    }
                }
            }
        }

        if (wybrano) {
            clear();
            if (highlight == 0) {
                submenu_add_meal_manual(tracker);
            }
            else if (highlight == 1) {
                submenu_add_meal_from_recipes(tracker);
            }
            else if (highlight == 2) {
                submenu_add_recipe(tracker);
            }
            else if (highlight == 3) {
                submenu_show_summary(tracker);
            }
            else if (highlight == 4) {
                submenu_show_meals(tracker);
            }
            else if (highlight == 5) { // Zmien dzienny cel
                double newTarget = 0;
                char buf[32];
                mvprintw(3, 2, "Nowy dzienny cel kcal: ");
                get_numeric_input(buf, 31, 3, 25);
                newTarget = atof(buf);
                if (newTarget > 0) {
                    saveCalorieTarget(newTarget);
                    target = newTarget;
                    tracker.setDailyTarget(newTarget);
                    mvprintw(5, 2, "Zmieniono cel na %.0f kcal.", newTarget);
                } else {
                    mvprintw(5, 2, "Niepoprawna wartosc.");
                }
                refresh();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            else if (highlight == 6) { // Wyczyść historię posiłków
                int y = 3, x = 2;
                mvprintw(y, x, "Czy na pewno wyczyscic historie posilkow? (t/n)");
                refresh();
                nodelay(stdscr, FALSE);
                int c;
                do {
                    c = getch();
                } while (c != 't' && c != 'n' && c != 'T' && c != 'N' && c != 27);
                nodelay(stdscr, TRUE);

                if (c == 't' || c == 'T') {
                    std::ofstream file("meals.txt", std::ios::trunc);
                    file.close();
                    tracker.clearMeals();
                    mvprintw(y + 2, x, "Historia posilkow zostala wyczyszczona.");
                    refresh();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
                else {
                    mvprintw(y + 2, x, "Anulowano czyszczenie historii.");
                    refresh();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            }
            else if (highlight == 7) {
                submenu_show_chart(tracker);
            }
            else if (highlight == 8) {
                break;
            }

            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
    }
    endwin();
}