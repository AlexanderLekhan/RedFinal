#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <future>

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

vector<pair<size_t, size_t> > process_query(const string& query,
                                            const InvertedIndex& index,
                                            const size_t MAX_OUTPUT)
{
    const auto words = SplitIntoWords(query);
    DocHits docid_count;

    for (const auto& word : words)
    {
        docid_count += index.Lookup(word);
    }

    vector<pair<size_t, size_t> > search_results;

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
    return search_results;
}

struct ResultBatch
{
    ResultBatch(vector<string> queriesBatch) :
        queries(queriesBatch)
    {
        results.reserve(queries.size());
    }
    vector<string> queries;
    vector<SearchResult> results;
};

ResultBatch process_queries_batch(vector<string> queries,
                                  const InvertedIndex& index,
                                  const size_t MAX_OUTPUT)
{
    ResultBatch resultBatch(move(queries));
    for (const string& q : resultBatch.queries)
    {
        resultBatch.results.push_back(process_query(q, index, MAX_OUTPUT));
    }
    return resultBatch;
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output) const
{
    const size_t max_batch_size = 5000;
    vector<string> queries;
    queries.reserve(max_batch_size);
    vector < future < ResultBatch > > futures;
    bool ok = true;

    while (ok)
    {
        string current_query;
        ok = getline(query_input, current_query).good();
        queries.push_back(move(current_query));

        if (!(ok && queries.size() < max_batch_size))
        {
            futures.push_back(async(process_queries_batch, move(queries), ref(index), MAX_OUTPUT));
        }
    }

    for (future<ResultBatch>& f : futures)
    {
        const ResultBatch& resultBatch(f.get());
        size_t i = 0;

        for (const SearchResult& r : resultBatch.results)
        {
            PrintResult(resultBatch.queries[i++], r, search_results_output);
        }
    }
}

void SearchServer::PrintResult(const string& query,
                               const SearchResult& result,
                               ostream &search_results_output) const
{
    search_results_output << query << ':';

    for (auto [docid, hitcount] : result)
    {
        search_results_output
            << " {" << "docid: " << docid << ", "
            << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
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
