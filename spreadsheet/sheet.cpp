#include "sheet.h"

#include <functional>
#include <iostream>
#include <memory>

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPosition");
    }
    if (CellInterface* cell_ptr = GetCell(pos)) {
        dynamic_cast<Cell*>(cell_ptr)->Set(std::move(text));
    } else {
        SetSize(pos);
        sheet_.at(pos.row).at(pos.col) = std::make_unique<Cell>(*this);
        SetCell(pos, std::move(text));
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellImpl<const CellInterface>(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellImpl<CellInterface>(pos);
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPosition");
    }
    if (GetCell(pos)) {
        sheet_.at(pos.row).at(pos.col).reset();
    }
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0;
    int max_col = 0;
    int rows_count = static_cast<int>(sheet_.size());
    for (int row = 0; row < rows_count; ++row) {
        int cols_count = static_cast<int>(sheet_.at(row).size());
        for (int col = 0; col < cols_count; ++col) {
            if (sheet_.at(row).at(col)) {
                max_col = std::max(max_col, col + 1);
                max_row = std::max(max_row, row + 1);
            }
        }
    }
    return { max_row, max_col };
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintSheet(output, PrintType::VALUE);
}
void Sheet::PrintTexts(std::ostream& output) const {
    PrintSheet(output, PrintType::TEXT);
}

void Sheet::PrintSheet(std::ostream& output, PrintType type) const {
    const auto [rows_count, cols_count] = GetPrintableSize();
    for (int row = 0; row < rows_count; ++row) {
        bool is_first = true;
        for (int col = 0; col < cols_count; ++col) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            PrintCell(output, { row, col }, type);
        }
        output << '\n';
    }
}

void Sheet::PrintCell(std::ostream& output,  Position pos, PrintType type = PrintType::TEXT) const {
    if (const CellInterface* cell_ptr = GetCell(pos)) {
        output << (type == PrintType::VALUE ? cell_ptr->GetValue() : cell_ptr->GetText());
    }
}

bool Sheet::IsOutSheet(Position pos) const {
    return pos.row >= static_cast<int>(sheet_.size()) || pos.col >= static_cast<int>(sheet_.at(pos.row).size());
}

template<typename CellType>
CellType* Sheet::GetCellImpl(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("InvalidPosition");
    }
    if (IsOutSheet(pos)) {
        return nullptr;
    }
    return sheet_.at(pos.row).at(pos.col).get();
}

void Sheet::SetSize(Position pos) {
    if (pos.row >= static_cast<int>(sheet_.size())) {
        sheet_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(sheet_.at(pos.row).size())) {
        sheet_.at(pos.row).resize(pos.col + 1);
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}