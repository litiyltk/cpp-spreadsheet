#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

using namespace std::literals::string_literals;

/*
базовый класс для ячеек разных типов и три наследника:
пустая ячейка, формульная ячейка и текстовая ячейка
*/
class Cell::Impl {
public:
	virtual ~Impl() = default;

	virtual std::string GetText() const = 0;
	virtual Value GetValue(const SheetInterface& sheet) const = 0;
	virtual std::vector<Position> GetReferencedCells() const = 0;
};

class Cell::EmptyImpl final : public Impl {
public:
	Value GetValue(const SheetInterface& sheet) const override {
		return ""s;
	}	

	std::string GetText() const override {
		return ""s;
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	}
};

class Cell::TextImpl final : public Impl {
public:
	TextImpl(std::string text) : text_(std::move(text)) {
	}

	Value GetValue(const SheetInterface&) const override {
		if (text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

	std::vector<Position> GetReferencedCells() const override {
		return {};
	}

private:
	std::string text_;
};

class Cell::FormulaImpl final : public Impl {
public:
	FormulaImpl(std::string formula)
		: formula_(ParseFormula(std::move(formula))) {
	}

	Value GetValue(const SheetInterface& sheet) const override {
		std::variant<double, FormulaError> value = formula_->Evaluate(sheet);
		return std::visit([](const auto& v) -> Value {
			return v;
		}, value);
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_->GetReferencedCells();
	}

	bool HasCache() const {
		return formula_->HasCache();
	}

	void ClearCache() {
		formula_->ClearCache();
	}

private:
	std::unique_ptr<FormulaInterface> formula_;
};

Cell::Cell(const SheetInterface& sheet)
	: sheet_(sheet) {
}

Cell::~Cell() {
}

void Cell::Set(std::string text) {
	ClearCellReferences(); // сбрасываем связи ячейки

	if (text.empty()) { // пустая ячейка
		impl_ = std::make_unique<EmptyImpl>();
	} else if (text[0] == FORMULA_SIGN && text.size() > 1) { // формульная ячейка
		std::unique_ptr<Cell::FormulaImpl> formula = std::make_unique<FormulaImpl>(text.substr(1));
		if (HasCyclicDependence(formula->GetReferencedCells())) { // проверяем циклические зависимости
			throw CircularDependencyException("CircularDependency");
		}
		AddCellReferences(formula->GetReferencedCells()); // добавляем связи ячейки
		impl_ = std::move(formula);
	} else { // текстовая ячейка
		impl_ = std::make_unique<TextImpl>(std::move(text));
	}
	//std::cerr << "Cell::Set: success!" << std::endl;
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue(sheet_);
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

void Cell::AddCellReferences(const std::vector<Position>& referenced_positions) {
	for (const Position& pos : referenced_positions) {
		if (!sheet_.GetCell(pos)) { // создаём пустую ячейку, если нет в таблице
			const_cast<SheetInterface&>(sheet_).SetCell(pos, ""s);
		}

		Cell* cell = const_cast<Cell*>(dynamic_cast<const Cell*>(sheet_.GetCell(pos))); // добавляем связи ячейки 
		referencing_cells_.insert(cell);
		cell->GetDependentCells().insert(this);
	}
}

void Cell::ClearCellReferences() {
	InvalidateCache(dependent_cells_);
	for (Cell* cell : referencing_cells_) {
		if (cell) {
			cell->GetDependentCells().erase(this);
		}
	}
	referencing_cells_.clear();
}

Cell::CellsSet Cell::GetDependentCells() {
	return dependent_cells_;
}

void Cell::InvalidateCache(CellsSet& dependent_cells) {
    CellsSet visited_cells; // контейнер для уже посещённых ячеек

    for (Cell* cell : dependent_cells) {
        InvalidateCacheRecursively(cell, visited_cells);
    }
}

void Cell::InvalidateCacheRecursively(Cell* cell, CellsSet& visited_cells) {
    if (!cell || visited_cells.find(cell) != visited_cells.end()) {
        return; // завершаем, если ячейка не существует или была посещена
    }

    if (FormulaImpl* formula = dynamic_cast<FormulaImpl*>(cell->impl_.get())) {
        if (formula->HasCache()) {
            formula->ClearCache();
			for (Cell* dependent_cell : cell->GetDependentCells()) {
                InvalidateCacheRecursively(dependent_cell, visited_cells); // инвалидируем кэш у зависимых ячеек
            }
        }
    }

    visited_cells.insert(cell); // после проверки добавляем посещённую ячейку в контейнер
}

bool Cell::HasCyclicDependence(const std::vector<Position>& referenced_positions) const {
    CellsSet visited_cells;
    return HasCyclicDependenceRecursively(const_cast<Cell*>(this), referenced_positions, visited_cells);
}

bool Cell::HasCyclicDependenceRecursively(Cell* cell, const std::vector<Position>& referenced_positions, CellsSet& visited_cells) const {
    for (const Position& pos : referenced_positions) {
        Cell* ref_cell = const_cast<Cell*>(dynamic_cast<const Cell*>(sheet_.GetCell(pos)));
        if (!ref_cell || visited_cells.find(cell) != visited_cells.end()) {
            continue;
        }

        if (this == ref_cell) {
            return true; // найдена циклическая зависимость - повторяющаяся ячейка
        }
        
        visited_cells.insert(ref_cell);
        if (HasCyclicDependenceRecursively(cell, ref_cell->GetReferencedCells(), visited_cells)) {
            return true; // найдена циклическая зависимость в ссылающейся ячейке
        }
    }
    return false; // не найдены
}