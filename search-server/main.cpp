

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in");
        ASSERT_EQUAL_HINT(found_docs.size(), 1, " 1 document found");
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the");
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in").empty(), " empty");
    }
}

void AddingDocuments() {
    const int doc_id1 = 0;
    const int doc_id2 = 1;
    const int doc_id3 = 2;
    const int doc_id4 = 3;

    const std::string content1 = "1word1 1word2 1word3 1word4";
    const std::string content2 = "2word1 2word2 2word3 2word4";
    const std::string content3 = "3word1 3word2 3word3 3word4 3word3 3word4";
    const std::string content4 = "4word1 4word2 4word3 4word4";

    const std::vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        const std::vector<Document>& found_documents = server.FindTopDocuments("word1");
        ASSERT(found_documents.empty());
    }

    {
        SearchServer server;
        server.SetStopWords("stop1 stop2 stop3");
        const std::vector<Document>& fd = server.FindTopDocuments("word2");
        const std::vector<Document>& fds = server.FindTopDocuments("word2", DocumentStatus::BANNED);
        const std::vector<Document>& fdsp = server.FindTopDocuments("word2", [](const int id, const DocumentStatus ds, int rating) {return rating > 1; });
        ASSERT(fd.empty());
        ASSERT(fds.empty());
        ASSERT_HINT(fdsp.empty(), "vector must be empty");
        ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "Documents count == 0");
    }

    {
        SearchServer server;
        server.SetStopWords("1word1 2word2 2word3");
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, { 4, 5, 6, 7, 8 });
        server.AddDocument(doc_id3, content3, DocumentStatus::IRRELEVANT, { 1, 3, 4, 5, 6, 7, 8 });
        server.AddDocument(doc_id4, content4, DocumentStatus::REMOVED, { 4, 5, 6, 7, 8, 20, 9 });

        {
            const std::vector<Document>& fd = server.FindTopDocuments("1word2");
            ASSERT_EQUAL(fd[0].id, doc_id1);
        }

        {
            const std::vector<Document>& fd = server.FindTopDocuments("2word2");
            ASSERT_HINT(fd.empty(), "vector must be empty");
        }

        {
            const std::vector<Document>& fd1 = server.FindTopDocuments("1word2 -1word3");
            ASSERT(fd1.empty());
            const std::vector<Document>& fd2 = server.FindTopDocuments("2word1 -1word3", DocumentStatus::BANNED);
            ASSERT(fd2[0].id == doc_id2);
        }

        {
            const auto& [w1, ds1] = server.MatchDocument("1word1 1word2 2word1 2word2", doc_id1);
            const auto& [w2, ds2] = server.MatchDocument("1word1 1word2 2word1 2word2", doc_id2);
            const auto& [w3, ds3] = server.MatchDocument("1word1 1word2 -2word1 2word2", doc_id1);
            const auto& [w4, ds4] = server.MatchDocument("-1word2 -2word1 2word2", doc_id1);
            ASSERT(w1[0] == "1word2"); ASSERT(ds1 == DocumentStatus::ACTUAL);
            ASSERT(w2[0] == "2word1"); ASSERT(ds2 == DocumentStatus::BANNED);
            ASSERT(w3[0] == "1word2"); ASSERT(ds3 == DocumentStatus::ACTUAL);
            ASSERT(w4.empty());
        }

    }

    {
        SearchServer server;
        server.SetStopWords("1word1 2word2 2word3");
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, { 1, 2, 3 });
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, { 4, 5, 6, 7, 8 });
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, { 1, 3, 4, 5, 6, 7, 8 });
        server.AddDocument(doc_id4, content4, DocumentStatus::ACTUAL, { 4, 5, 6, 7, 8, 20, 9 });

        const std::vector<Document>& fd = server.FindTopDocuments("1word2 -2word1 3word1 3word2 3word3 4word1");
        ASSERT(fd[1].relevance < fd[0].relevance); ASSERT(fd[0].rating < fd[2].rating);
        ASSERT(fd[2].relevance < fd[1].relevance); ASSERT(fd[1].rating < fd[0].rating);
        ASSERT(fd[0].relevance == 0.92419624074659368); ASSERT(fd[0].rating == 4);

    }
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(AddingDocuments);
}








#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6; 

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }
    words.push_back(word);

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id,
                           DocumentData{
                                   ComputeAverageRating(ratings),
                                   status
                           });
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status1 ) const{
        auto res = FindTopDocuments(raw_query, [status1](int document_id, DocumentStatus status, int rating){return status == status1;});
        return res;
    }
    
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {
                text,
                is_minus,
                IsStopWord(text)
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                                                document_id,
                                                relevance,
                                                documents_.at(document_id).rating
                                        });
        }
        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

    cout << "ACTUAL by default:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "BANNED:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }

    return 0;
}
