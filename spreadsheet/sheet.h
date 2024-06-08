#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

enum class PrintType {
    VALUE,
    TEXT
};

class Sheet : public SheetInterface {
public:
    Sheet() = default;
    ~Sheet() override = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<CellInterface>>> sheet_; // контейнер для хранения ячеек таблицы

    // выводит содержимое всех ячейки таблицы в поток в зависимости от типа: Text, Value
    void PrintSheet(std::ostream& output, PrintType type) const;

    // выводит содержимое ячейки в поток в зависимости от типа: Text, Value
    void PrintCell(std::ostream& output, Position pos, PrintType type) const;

    // определяет, находится позиция за границами таблицы
    bool IsOutSheet(Position pos) const;

    // вспомогательная функция для GetCell
    template<typename CellType>
    CellType* GetCellImpl(Position pos) const;

    // устанавливает размер таблицы
    void SetSize(Position pos);
};

