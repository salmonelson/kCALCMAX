#include "funkcje.h"
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>

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
    double sumKcal = 0, sumP = 0, sumC = 0, sumF = 0;
    for (const auto& m : mealsToday) {
        sumKcal += m.kcal;
        sumP += m.protein;
        sumC += m.carbs;
        sumF += m.fat;
    }

    std::cout << "\n==============================\n";
    std::cout << " PODSUMOWANIE DNIA\n";
    std::cout << " Zjedzone kcal: " << sumKcal << " / " << dailyTarget << "\n";
    std::cout << " Bialko: " << sumP << " g\n";
    std::cout << " Wegle: " << sumC << " g\n";
    std::cout << " Tluszcze: " << sumF << " g\n";
    std::cout << "==============================\n";
}

void CalorieTracker::showMeals() {
    std::cout << "\n== ZJEDZONE POSILKI ==\n";
    for (const auto& m : mealsToday) {
        std::cout << "[" << m.datetime << "] " << m.name << " - "
            << m.kcal << " kcal, P: " << m.protein
            << ", C: " << m.carbs << ", F: " << m.fat << "\n";
    }
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

void menu() {
    double target;
    std::cout << "Podaj dzienny cel kcal: ";
    std::cin >> target;

    CalorieTracker tracker(target);
    tracker.loadMeals();

    while (true) {
        std::cout << "\n==============================\n";
        std::cout << "1. Dodaj posilek recznie\n";
        std::cout << "2. Dodaj posilek z gotowych\n";
        std::cout << "3. Dodaj nowy przepis do gotowych\n";
        std::cout << "4. Pokaz podsumowanie\n";
        std::cout << "5. Pokaz wszystkie posilki\n";
        std::cout << "0. Wyjdz\n";
        std::cout << "==============================\n";
        std::cout << "Wybor: ";

        int choice;
        std::cin >> choice;

        if (choice == 1) {
            Meal m = inputMeal();
            tracker.addMeal(m);
            std::cout << "Dodano!\n";
        }
        else if (choice == 2) {
            auto recipes = loadRecipes();
            if (recipes.empty()) {
                std::cout << "Brak gotowych przepisow.\n";
                continue;
            }
            displayRecipes(recipes);
            std::cout << "Wybierz numer posilku: ";
            int num;
            std::cin >> num;
            if (num >= 1 && num <= (int)recipes.size()) {
                Meal m = recipes[num - 1];
                m.datetime = currentDateTime();
                tracker.addMeal(m);
                std::cout << "Dodano!\n";
            }
            else {
                std::cout << "Zly numer.\n";
            }
        }
        else if (choice == 3) {
            Meal m = inputMeal();
            saveCustomMeal(m);
            std::cout << "Przepis zapisany!\n";
        }
        else if (choice == 4) {
            tracker.showSummary();
        }
        else if (choice == 5) {
            tracker.showMeals();
        }
        else if (choice == 0) {
            break;
        }
        else {
            std::cout << "Nieprawidlowy wybor!\n";
        }
    }
}
