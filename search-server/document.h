   
#pragma once

#include <iostream>

struct Document {
    Document() = default;
    Document(int, double, int);

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

std::ostream& operator<<(std::ostream&, const Document&);

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};
