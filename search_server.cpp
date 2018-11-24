#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

vector<string> SplitIntoWords(const string& line)
{
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input)
{
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document); )
    {
        new_index.Add(move(current_document));
    }

    index = move(new_index);
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output)
{
    for (string current_query; getline(query_input, current_query); )
    {
        const auto words = SplitIntoWords(current_query);
        DocHits docid_count;

        for (const auto& word : words)
        {
            docid_count += index.Lookup(word);
        }

        vector<pair<size_t, size_t>> search_results;

        for (size_t i = 0; docid_count.size() > 0 && i < MAX_OUTPUT; ++i)
        {
            auto curMax =
            max_element(docid_count.begin(), docid_count.end(),
                        [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs)
                        {
                            int64_t lhs_docid = lhs.first;
                            auto lhs_hit_count = lhs.second;
                            int64_t rhs_docid = rhs.first;
                            auto rhs_hit_count = rhs.second;
                            return make_pair(lhs_hit_count, -lhs_docid)
                                 < make_pair(rhs_hit_count, -rhs_docid);
                        });
            search_results.push_back(*curMax);
            docid_count.erase(curMax);
        }

        search_results_output << current_query << ':';

        for (auto [docid, hitcount] : search_results)
        {
            search_results_output << " {"
            << "docid: " << docid << ", "
            << "hitcount: " << hitcount << '}';
        }
        search_results_output << endl;
    }
}

void InvertedIndex::Add(const string& document)
{
    docs.push_back(document);
    const size_t docid = docs.size() - 1;

    for (const auto& word : SplitIntoWords(document))
    {
        ++index[word][docid];
    }
}

DocHits InvertedIndex::Lookup(const string& word) const
{
    if (auto it = index.find(word); it != index.end())
    {
        return it->second;
    }
    else
    {
        return {};
    }
}
