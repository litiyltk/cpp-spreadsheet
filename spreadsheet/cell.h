#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <unordered_set>

class Cell : public CellInterface {
public:
    struct Hasher { // хэшер для контейнер ячеек
        size_t operator() (Cell* cell_ptr) const {
            return ptr_hasher_(cell_ptr) * simple_number_;
        }

    private:
        std::hash<void*> ptr_hasher_;
        static constexpr size_t simple_number_ = 67;
    };

    using CellsSet = std::unordered_set<Cell*, Hasher>;

    Cell(const SheetInterface& sheet);
    ~Cell();

    void Set(std::string text) override;

    Value GetValue() const override;
    std::string GetText() const override;
    
    std::vector<Position> GetReferencedCells() const override;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    const SheetInterface& sheet_;
    
    CellsSet dependent_cells_; // зависимые ячейки
    CellsSet referencing_cells_; // ссылающиеся ячейки

    // добавляет ссылки ячейки на другие ячейки
    void AddCellReferences(const std::vector<Position>& referenced_positions);

    // удаляет все ссылки на другие ячейки и сбрасывает зависимости ячейки
    void ClearCellReferences();

    // возвращает зависимые ячейки
    CellsSet GetDependentCells();

    // очищает кэш всех зависимых ячеек
    void InvalidateCache(CellsSet& dependent_cells);

    // вспомогательная функция для рекурсивной инвалидации кеша
    void InvalidateCacheRecursively(Cell* cell, CellsSet& visited_cells);

    // определяет, есть ли у текущей ячейки циклические зависимости
    bool HasCyclicDependence(const std::vector<Position>& referenced_positions) const;
    
    // вспомогательный метод рекурсивно проверяет циклические зависимости
    bool HasCyclicDependenceRecursively(Cell* cell, const std::vector<Position>& referenced_positions, CellsSet& visited_cells) const;
};

// Выводит значение или строку ошибки из ячейки 
std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value);