#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>



// индексы компонентов
// перечисление, пользовательский тип Ingredient с конечным набором именованных констант
// имена констант не попадают во внешнюю область видимости (нужно писать Ingredient::kTobacco)
// нет неявного преобразования к целочисленным типам (в int придётся приводить через static_cast)
// три константы этого типа и их целочисленные значения
enum class Ingredient { kTobacco = 0, kPaper = 1, kMatches = 2 }; // префикс k в именах обозначает константу

// функция возвращает целочисленный индекс для значения строгого перечисления Ingredient (enum class)
// явное преобразование к целочисленному типу
// std::size_t - тип результата, беззнаковы целочисленный тип
constexpr std::size_t IngredientIndex(Ingredient ingredient) {
  return static_cast<std::size_t>(ingredient);
}

// ф-ия нужна для того, чтобы быстро получить имя по индексу
// названия ингридиентов в им. падеже
constexpr std::array<std::string_view, 3> kIngredientNames{
    "табак", "бумага", "спички"};

// то же самое, но в тв. падеже
constexpr std::array<std::string_view, 3> kSmokerResources{
    "табаком", "бумагой", "спичками"};

// списко всех видов курильщиков
constexpr std::array<Ingredient, 3> kAllSmokers{
    Ingredient::kTobacco, Ingredient::kPaper, Ingredient::kMatches};

// кол-во типов курильщиков
constexpr std::size_t kSmokerCount = kAllSmokers.size();

// пара компонентов, который кладет на стол посредник
// другими словами, два компонента, которых не хватает для конкретного курильщика
constexpr std::array<std::array<Ingredient, 2>, 3> kComponentsForSmoker{{
    {Ingredient::kPaper, Ingredient::kMatches},
    {Ingredient::kTobacco, Ingredient::kMatches},
    {Ingredient::kTobacco, Ingredient::kPaper},
}};

// ф-ия возвращает строковое имя ингридинета по индексу (по значению перечисление Ingredient)
std::string_view IngredientToString(Ingredient ingredient) {
  return kIngredientNames[IngredientIndex(ingredient)];
}

// ф-ия возвращает тип курильщака в формате:
// курильщик с ...
std::string SmokerLabel(Ingredient ingredient) {
  const auto index = IngredientIndex(ingredient);
  return std::string{"курильщик с "} + std::string{kSmokerResources[index]};
}

// ф-ия возвращает недостающие компненты для конкретного курильщика
// курильщику недостает *пары компонентов*
std::array<Ingredient, 2> ComponentsFor(Ingredient smoker) {
  return kComponentsForSmoker[IngredientIndex(smoker)];
}

