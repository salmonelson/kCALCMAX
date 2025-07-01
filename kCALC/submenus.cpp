#include "submenus.h"
#include "funkcje.h"
#include <curses.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <map>
#include <algorithm>

// Pomocniczy przycisk "Wroc"
extern void drawBackButton(int y, int x, bool hovered);
extern std::string currentDateTime();
extern std::vector<Meal> loadRecipes();
extern void saveCustomMeal(const Meal& meal);


// --- Dodaj posi³ek rêcznie ---
void submenu_add_meal_manual(CalorieTracker& tracker) {
    char name[64] = "";
    double kcal = 0, protein = 0, carbs = 0, fat = 0;
    int y = 2, x = 2;
    char buf[32];
    echo();
    curs_set(1);

    // Nazwa posilku
    mvprintw(y, x, "Nazwa posilku: ");
    while (true) {
        move(y, x + 15);
        clrtoeol();
        getnstr(name, 63);
        if (strlen(name) > 0) break;
    }
    y++;

    // Kcal
    mvprintw(y, x, "Kcal: ");
    while (true) {
        move(y, x + 6);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 6)) {
            kcal = atof(buf);
            break;
        }
    }
    y++;

    // Bialko
    mvprintw(y, x, "Bialko (g): ");
    while (true) {
        move(y, x + 12);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 12)) {
            protein = atof(buf);
            break;
        }
    }
    y++;

    // Wegle
    mvprintw(y, x, "Wegle (g): ");
    while (true) {
        move(y, x + 11);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 11)) {
            carbs = atof(buf);
            break;
        }
    }
    y++;

    // Tluszcze
    mvprintw(y, x, "Tluszcze (g): ");
    while (true) {
        move(y, x + 13);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 13)) {
            fat = atof(buf);
            break;
        }
    }
    noecho();
    curs_set(0);

    Meal m;
    m.name = normalize_ascii(name);
    m.kcal = kcal;
    m.protein = protein;
    m.carbs = carbs;
    m.fat = fat;
    m.datetime = currentDateTime();
    tracker.addMeal(m);

    // Komunikat "Dodano!" i przycisk Wroc/ESC
    int by = LINES - 2, bx = 4, width = 8;
    bool back_hover = false;
    while (true) {
        drawBackButton(by, bx, back_hover);
        mvprintw(by - 4, bx, "Dodano!");
        refresh();
        request_mouse_pos();
        back_hover = (Mouse_status.y >= by - 2 && Mouse_status.y <= by && Mouse_status.x >= bx && Mouse_status.x < bx + width);
        int c = getch();
        if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
        if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= by - 2 && event.y <= by && event.x >= bx && event.x < bx + width &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
            }
        }
    }
}

// --- Dodaj posi³ek z gotowych ---
void submenu_add_meal_from_recipes(CalorieTracker& tracker) {
    auto recipes = loadRecipes();
    if (recipes.empty()) {
        mvprintw(2, 2, "Brak gotowych przepisow.");
        int y = LINES - 2, x = 4;
        bool back_hover = false;
        int width = 8;
        while (true) {
            drawBackButton(y, x, back_hover);
            refresh();
            request_mouse_pos();
            back_hover = (Mouse_status.y >= y - 2 && Mouse_status.y <= y && Mouse_status.x >= x && Mouse_status.x < x + width);
            int c = getch();
            if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
            if (c == KEY_MOUSE) {
                MEVENT event;
                if (nc_getmouse(&event) == OK) {
                    if (event.y >= y - 2 && event.y <= y && event.x >= x && event.x < x + width &&
                        (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                        break;
                    }
                }
            }
        }
        return;
    }

    int sel = 0;
    int y_back = LINES - 2, x_back = 4, width_back = 8;
    int visible = LINES - 10;
    int top = 0;
    while (true) {
        clear();
        mvprintw(1, 2, "Wybierz posilek:");
        int mouse_hover = -1;
        bool back_hover = false;

        // Scrollowanie: pilnuj, ¿eby wybrany by³ widoczny
        if (sel < top) top = sel;
        if (sel >= top + visible) top = sel - visible + 1;

        // Wyœwietlanie widocznych posi³ków
        for (int i = 0; i < visible && (top + i) < (int)recipes.size(); ++i) {
            int idx = top + i;
            int row = 3 + i;
            if (idx == sel) attron(COLOR_PAIR(2));
            mvprintw(row, 4, "%d. %s - %.0f kcal", idx + 1, recipes[idx].name.c_str(), recipes[idx].kcal);
            if (idx == sel) attroff(COLOR_PAIR(2));
        }

        // Obs³uga hovera myszk¹ na liœcie
        request_mouse_pos();
        mouse_hover = -1;
        for (int i = 0; i < visible && (top + i) < (int)recipes.size(); ++i) {
            int idx = top + i;
            int row = 3 + i;
            int x1 = 4, x2 = 4 + (int)recipes[idx].name.length() + 15; // szacunkowo
            if (Mouse_status.y == row && Mouse_status.x >= x1 && Mouse_status.x <= x2) {
                mouse_hover = idx;
                break;
            }
        }
        if (mouse_hover != -1) sel = mouse_hover;

        // Obs³uga hovera na przycisku Wroc
        back_hover = (Mouse_status.y >= y_back - 2 && Mouse_status.y <= y_back && Mouse_status.x >= x_back && Mouse_status.x < x_back + width_back);

        drawBackButton(y_back, x_back, back_hover);

        refresh();

        int c = getch();
        if (c == KEY_UP) {
            if (sel > 0) sel--;
        }
        else if (c == KEY_DOWN) {
            if (sel + 1 < (int)recipes.size()) sel++;
        }
        else if ((c == '\n' || c == KEY_ENTER) && !back_hover) {
            Meal m = recipes[sel];
            m.datetime = currentDateTime();
            tracker.addMeal(m);

            // Wyœwietl komunikat na sekundê
            drawBackButton(y_back, x_back, false);
            mvprintw(y_back - 4, x_back, "Dodano %s!", recipes[sel].name.c_str());
            refresh();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        else if ((c == '\n' || c == KEY_ENTER) && back_hover) {
            break;
        }
        else if (c == 27 || c == KEY_LEFT) {
            break;
        }
        else if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                // Klik na posi³ek
                for (int i = 0; i < visible && (top + i) < (int)recipes.size(); ++i) {
                    int idx = top + i;
                    int row = 3 + i;
                    int x1 = 4, x2 = 4 + (int)recipes[idx].name.length() + 15;
                    if (event.y == row && event.x >= x1 && event.x <= x2 &&
                        (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                        sel = idx;
                        Meal m = recipes[sel];
                        m.datetime = currentDateTime();
                        tracker.addMeal(m);

                        // Wyœwietl komunikat na sekundê
                        drawBackButton(y_back, x_back, false);
                        mvprintw(y_back - 4, x_back, "Dodano %s!", recipes[sel].name.c_str());
                        refresh();
                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        goto redraw;
                    }
                }
                // Klik na przycisk Wroc
                if (event.y >= y_back - 2 && event.y <= y_back && event.x >= x_back && event.x < x_back + width_back &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
                // Scroll myszk¹
                if (event.bstate & BUTTON4_PRESSED) { // scroll up
                    if (sel > 0) sel--;
                }
                if (event.bstate & BUTTON5_PRESSED) { // scroll down
                    if (sel + 1 < (int)recipes.size()) sel++;
                }
            }
        }
    redraw:
        ;
    }
}

// --- Dodaj nowy przepis do gotowych ---
void submenu_add_recipe(CalorieTracker&) {
    bool cancelled = false;
    char name[64] = "";
    double kcal = 0, protein = 0, carbs = 0, fat = 0;
    int y = 2, x = 2;
    char buf[32];
    echo();
    
    // Nazwa przepisu
    mvprintw(y, x, "Nazwa przepisu: ");
    while (true) {
        move(y, x + 16);
        clrtoeol();
        getnstr(name, 63);
        if (strlen(name) > 0) break;
    }
    y++;

    // Kcal
    mvprintw(y, x, "Kcal: ");
    while (true) {
        move(y, x + 6);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 6)) {
            kcal = atof(buf);
            break;
        }
    }
    y++;

    // Bialko
    mvprintw(y, x, "Bialko (g): ");
    while (true) {
        move(y, x + 12);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 12)) {
            protein = atof(buf);
            break;
        }
    }
    y++;

    // Wegle
    mvprintw(y, x, "Wegle (g): ");
    while (true) {
        move(y, x + 11);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 11)) {
            carbs = atof(buf);
            break;
        }
    }
    y++;

    // Tluszcze
    mvprintw(y, x, "Tluszcze (g): ");
    while (true) {
        move(y, x + 13);
        clrtoeol();
        if (get_numeric_input(buf, 31, y, x + 13)) {
            fat = atof(buf);
            break;
        }
    }
    noecho();
    curs_set(0);

    Meal m;
    m.name = normalize_ascii(name);
    m.kcal = kcal;
    m.protein = protein;
    m.carbs = carbs;
    m.fat = fat;
    saveCustomMeal(m);

    // Komunikat "Przepis zapisany!" i przycisk Wroc/ESC
    int by = LINES - 2, bx = 4, width = 8;
    bool back_hover = false;
    while (true) {
        drawBackButton(by, bx, back_hover);
        mvprintw(by - 4, bx, "Przepis zapisany!");
        refresh();
        request_mouse_pos();
        back_hover = (Mouse_status.y >= by - 2 && Mouse_status.y <= by && Mouse_status.x >= bx && Mouse_status.x < bx + width);
        int c = getch();
        if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
        if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= by - 2 && event.y <= by && event.x >= bx && event.x < bx + width &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
            }
        }
    }
}

// --- Pokaz podsumowanie ---
void submenu_show_summary(CalorieTracker& tracker) {
    flushinp();
    tracker.showSummary();
    int y = LINES - 2, x = 4;
    bool back_hover = false;
    int width = 8;
    while (true) {
        drawBackButton(y, x, back_hover);
        refresh();
        request_mouse_pos();
        back_hover = (Mouse_status.y >= y - 2 && Mouse_status.y <= y && Mouse_status.x >= x && Mouse_status.x < x + width);
        int c = getch();
        if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
        if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= y - 2 && event.y <= y && event.x >= x && event.x < x + width &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
            }
        }
    }
}

// --- Pokaz wszystkie posilki ---
void submenu_show_meals(CalorieTracker& tracker) {
    tracker.showMeals();
    int y = LINES - 2, x = 4;
    bool back_hover = false;
    int width = 8;
    while (true) {
        drawBackButton(y, x, back_hover);
        refresh();
        request_mouse_pos();
        back_hover = (Mouse_status.y >= y - 2 && Mouse_status.y <= y && Mouse_status.x >= x && Mouse_status.x < x + width);
        int c = getch();
        if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
        if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= y - 2 && event.y <= y && event.x >= x && event.x < x + width &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
            }
        }
    }
}

void submenu_show_chart(CalorieTracker& tracker) {
    std::map<std::string, double> dayKcal;
    std::vector<std::string> last30days;
    time_t now = time(0);
    tm ltm;
    char buf[16];

    for (int i = 29; i >= 0; --i) {
        time_t t = now - i * 24 * 60 * 60;
        localtime_s(&ltm, &t);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", 1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday);
        last30days.push_back(buf);
        dayKcal[buf] = 0;
    }

    for (const auto& m : tracker.getMealsToday()) {
        std::string date = m.datetime.substr(0, 10);
        if (dayKcal.count(date))
            dayKcal[date] += m.kcal;
    }

    double maxKcal = tracker.getDailyTarget();
    for (const auto& d : last30days)
        if (dayKcal[d] > maxKcal) maxKcal = dayKcal[d];

    int chartHeight = std::min(15, LINES - 10);
    int chartWidth = std::min(60, COLS - 14);
    int barWidth = std::max(1, chartWidth / 30);

    // Wyœrodkowanie wykresu
    int x0 = (COLS - (chartWidth + 10)) / 2 + 8; // +8 dla miejsca na oœ Y i opis
    int y0 = 4;
    int yLabelX = x0 - 7;

    while (true) {
        clear();
        attron(COLOR_PAIR(1));
        box(stdscr, 0, 0);
        mvprintw(1, (COLS - 44) / 2, "Wykres dziennego spozycia kalorii (30 dni)");
        attroff(COLOR_PAIR(1));

        // Oœ Y: wartoœci co drug¹ linijkê
        for (int i = 0; i <= chartHeight; ++i) {
            if (i % 2 == 0) {
                int kcalVal = (int)(maxKcal * (chartHeight - i) / chartHeight);
                mvprintw(y0 + i, yLabelX, "%4d |", kcalVal);
            }
            else {
                mvprintw(y0 + i, yLabelX, "     |");
            }
        }

        // Napis "kcal" poœrodku osi Y, po lewej stronie wykresu
        int yKcal = y0 + chartHeight / 2;
        mvprintw(yKcal, yLabelX - 6, "kcal");

        // S³upki: co drugi w kolorze (bia³y/zielony), wype³nione spacjami
        for (int d = 0; d < 30; ++d) {
            int barH = (int)(dayKcal[last30days[d]] / maxKcal * chartHeight + 0.5);
            int color = (d % 2 == 0) ? 3 : 4; // naprzemiennie bia³y/zielony
            attron(COLOR_PAIR(color));
            for (int h = 0; h < barH; ++h) {
                for (int w = 0; w < barWidth; ++w) {
                    mvaddch(y0 + chartHeight - h, x0 + d * barWidth + w, ' ');
                }
            }
            attroff(COLOR_PAIR(color));
        }

        // Oœ X (daty co kilka dni)
        for (int d = 0; d < 30; d += 5) {
            mvprintw(y0 + chartHeight + 1, x0 + d * barWidth, "%s", last30days[d].substr(5).c_str());
        }

        // Opis osi X "dzien"
        mvprintw(y0 + chartHeight + 3, x0 + chartWidth / 2 - 2, "data");

        // Linia celu (czarny tekst, podœwietlenie jak s³upki: bia³y lub zielony)
        int targetY = y0 + chartHeight - (int)(tracker.getDailyTarget() / maxKcal * chartHeight + 0.5);
        if (targetY >= y0 && targetY <= y0 + chartHeight) {
            for (int d = 0; d < 30; ++d) {
                int color = (d % 2 == 0) ? 5 : 2; // 5: czarny na bia³ym, 6: czarny na zielonym
                attron(COLOR_PAIR(color) | A_BOLD);
                for (int w = 0; w < barWidth; ++w) {
                    mvaddch(targetY, x0 + d * barWidth + w, '-');
                }
                attroff(COLOR_PAIR(color) | A_BOLD);
            }
            attron(COLOR_PAIR(1) | A_BOLD);
            mvprintw(targetY, x0 + barWidth * 30 + 2, "dzienny cel");
            attroff(COLOR_PAIR(1) | A_BOLD);
        }

        // Przycisk wyjœcia
        int by = LINES - 2, bx = 4, width = 8;
        bool back_hover = false;
        drawBackButton(by, bx, back_hover);
        refresh();

        request_mouse_pos();
        back_hover = (Mouse_status.y >= by - 2 && Mouse_status.y <= by && Mouse_status.x >= bx && Mouse_status.x < bx + width);

        int c = getch();
        if ((c == 27 || c == KEY_LEFT) || ((c == '\n' || c == KEY_ENTER) && back_hover)) break;
        if (c == KEY_MOUSE) {
            MEVENT event;
            if (nc_getmouse(&event) == OK) {
                if (event.y >= by - 2 && event.y <= by && event.x >= bx && event.x < bx + width &&
                    (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED))) {
                    break;
                }
            }
        }
    }
}