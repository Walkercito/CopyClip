#include "ui/Fuzzy.hpp"

#include <cctype>
#include <cstddef>

namespace copyclip::ui {

namespace {

[[nodiscard]] char lower(char c) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

} // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): query/text read clearly.
bool fuzzy_matches(std::string_view query, std::string_view text) {
    if (query.empty()) {
        return true;
    }
    std::size_t matched = 0;
    for (const char glyph : text) {
        if (lower(glyph) == lower(query[matched])) {
            ++matched;
            if (matched == query.size()) {
                return true;
            }
        }
    }
    return false;
}

} // namespace copyclip::ui
