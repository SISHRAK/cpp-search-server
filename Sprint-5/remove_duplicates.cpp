
#include <string>
#include <set>
#include <algorithm>

#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> all_words;
    std::set<int> document_ids;
    for (const int document_id : search_server) {
        std::set<std::string> words;
        for (const auto& word_to_frequency : search_server.GetWordFrequencies(document_id)) {
            words.insert(word_to_frequency.first);
        }

        if (all_words.count(words)) {
            std::cout << "Found duplicate document id " << document_id << std::endl;
            document_ids.emplace(document_id);
        }

        all_words.emplace(words);
    }

    for (const auto document_id : document_ids) {
        search_server.RemoveDocument(document_id);
    }
}
