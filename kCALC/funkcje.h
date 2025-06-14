#ifndef FUNKCJE_H
#define FUNKCJE_H

#include <iostream>
#include <vector>
#include <string>

struct Meal {
    std::string name;
    double kcal;
    double protein=0;
    double carbs;
    double fat;
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
};

std::string currentDateTime();
void saveCustomMeal(const Meal& meal);
std::vector<Meal> loadRecipes();
void displayRecipes(const std::vector<Meal>& recipes);
Meal inputMeal();
void menu();

#endif
