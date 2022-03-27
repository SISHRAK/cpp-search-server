#pragma once
#include <cmath>
#include <math.h>
#include <set>
#include <map>
#include <algorithm>

#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    std::set<std::string> stop_words_;

    std::map<std::string, std::map<int, double>> word_to_document_freqs_;

    std::map<int, DocumentData> documents_;

    std::vector<int> document_ids_;

public:
    // Defines an invalid document id
    // You can refer this constant as SearchServer::INVALID_DOCUMENT_ID
    inline static constexpr int INVALID_DOCUMENT_ID = -1;

    template <typename StringContainer>
    explicit SearchServer(const StringContainer&);

    explicit SearchServer(const std::string&);

    void AddDocument(int, const std::string&, DocumentStatus, const std::vector<int>&);

    inline int GetDocumentCount() const noexcept{
        return documents_.size();
    }

    int GetDocumentId(int index)const {

        if (GetDocumentCount() < index || index < 0) {
            throw std::out_of_range("non-existend ID");
        }
        return document_ids_.at(index);
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const;

    std::vector<Document> FindTopDocuments(const std::string&, DocumentStatus) const;

    std::vector<Document> FindTopDocuments(const std::string&) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string&, int) const;

private:

    static bool IsValidWord(const std::string&);

    static int ComputeAverageRating(const std::vector<int>&);

    inline bool IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

    [[nodiscard]] bool SplitIntoWordsNoStop(const std::string&, std::vector<std::string>&) const;

    [[nodiscard]] bool ParseQueryWord(std::string, QueryWord&) const;

    [[nodiscard]] bool ParseQuery(const std::string&, Query&) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string&) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query&, DocumentPredicate) const;

    template <typename StringContainer>
    void CheckValidity(const StringContainer&);

    template <typename StringContainer>
    std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer&);
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) {
    CheckValidity(stop_words);
    stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentPredicate document_predicate) const {
    std::vector<Document> result;
    Query query;
    if (!ParseQuery(raw_query, query)) {
        throw std::invalid_argument("invalid request");
    }
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    // Exchange matched_documents and result instead of deep copying
    result.swap(matched_documents);
    return result;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}

template <typename StringContainer>
void SearchServer::CheckValidity(const StringContainer& strings) {
    for (const std::string& str : strings) {
        if (!IsValidWord(str)) {
            throw std::invalid_argument("invalid stop-words in constructor");
        }
    }
}

template <typename StringContainer>
std::set<std::string> SearchServer::MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
