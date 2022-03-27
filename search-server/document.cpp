#include "document.h"

Document::Document(int id, double relevance, int rating)
    :id(id)
    , relevance(relevance)
    , rating(rating) {
}

std::ostream& operator<<(std::ostream& os, const Document& d) {
    os << "{ document_id = " << d.id << ", relevance = " << d.relevance << ", rating = " << d.rating << " }";
    return os;
}
