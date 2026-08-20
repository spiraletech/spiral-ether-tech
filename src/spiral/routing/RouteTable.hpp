#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

// Ordered, predicate-based routing recovered from the Spiral plugin dispatcher.
// Predicates receive const Signal&, so route inspection itself has no need to
// mutate engine state. First matching rule wins; later rules are not consulted.
class RouteTable {
public:
    using Predicate = std::function<bool(const Signal&)>;

    struct Rule {
        std::string name;
        std::string destination;
        Predicate canHandle;
    };

    struct Match {
        std::size_t index = 0;
        std::string name;
        std::string destination;
    };

    bool addRule(std::string name, std::string destination, Predicate predicate)
    {
        if (name.empty() || destination.empty() || !predicate) {
            return false;
        }

        rules_.push_back(Rule{
            std::move(name),
            std::move(destination),
            std::move(predicate)
        });
        return true;
    }

    std::optional<Match> resolve(const Signal& signal) const
    {
        for (std::size_t i = 0; i < rules_.size(); ++i) {
            const Rule& rule = rules_[i];
            if (rule.canHandle && rule.canHandle(signal)) {
                return Match{i, rule.name, rule.destination};
            }
        }
        return std::nullopt;
    }

    void clear()
    {
        rules_.clear();
    }

    std::size_t size() const noexcept
    {
        return rules_.size();
    }

    const std::vector<Rule>& rules() const noexcept
    {
        return rules_;
    }

private:
    std::vector<Rule> rules_;
};

} // namespace spiral
