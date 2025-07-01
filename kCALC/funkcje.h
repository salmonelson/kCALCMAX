#ifndef FUNKCJE_H
#define FUNKCJE_H

#include <iostream>
#include <vector>
#include <string>

struct Meal {
    std::string name;
    double kcal=0;
    double protein=0;
    double carbs=0;
    double fat=0;
    std::string datetime;
};

class CalorieTracker {
private:
    std::vector<Meal> mealsToday;
    double dailyTarget;

public:
    CalorieTracker(double target);
    void loadMeals();
    void saveMeal(const Meal& meal);
    void addMeal(const Meal& meal);
    void showSummary();
    void showMeals();

    void setDailyTarget(double target) { dailyTarget = target; }
    void clearMeals() { mealsToday.clear(); }

    const std::vector<Meal>& getMealsToday() const;
    double getDailyTarget() const;
};

std::string currentDateTime();
void saveCustomMeal(const Meal& meal);
std::vector<Meal> loadRecipes();
void displayRecipes(const std::vector<Meal>& recipes);
Meal inputMeal();
void menu();
bool get_numeric_input(char* buf, int size, int y, int x);
std::string normalize_ascii(const std::string& input);

#endif
