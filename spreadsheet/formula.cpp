#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <optional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {

class Formula final : public FormulaInterface {
public:
    explicit Formula(std::string expression)
        try
            : ast_(ParseFormulaAST(expression)) {
                SetCells();
        } catch (const std::exception& exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }

    Value Evaluate(const SheetInterface& sheet) const override {
        if (!cache_.has_value()) {
            try {
                cache_ = ast_.Execute(sheet);
            } catch (const FormulaError& fe) {
                cache_ = fe;
            }
        }
        return *cache_;
    }

    std::string GetExpression() const override {
        std::ostringstream strm;
        ast_.PrintFormula(strm);
        return strm.str();
    }

    std::vector<Position> GetReferencedCells() const {
        return cells_;
    }

    bool HasCache() const override {
        return cache_.has_value();
    }
    void ClearCache() override {
        cache_.reset();
    }

private:
    FormulaAST ast_;
    std::vector<Position> cells_;
    mutable std::optional<Value> cache_;

    void SetCells() {
        std::forward_list<Position> cells = ast_.GetCells();
        cells.unique();
        std::vector<Position> sorted_cells{ std::make_move_iterator(cells.begin()), std::make_move_iterator(cells.end()) };
        std::sort(sorted_cells.begin(), sorted_cells.end());
        cells_ = std::move(sorted_cells);
    }

};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
