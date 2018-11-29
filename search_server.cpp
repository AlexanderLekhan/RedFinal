#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <future>
#include <cassert>

#define MULTI_THREAD_VERSION
#undef MULTI_THREAD_VERSION

using namespace std;

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
        if (!current_document.empty())
        {
            new_index.Add(move(current_document));
        }
    }

    index = move(new_index);
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output) const
{
#ifdef MULTI_THREAD_VERSION
    AddQueriesStreamMultiThread(query_input, search_results_output);
#else
    AddQueriesStreamSingleThread(query_input, search_results_output);
#endif
}

SearchResult SearchServer::ProcessQuery1(const string& query) const
{
    const auto words = SplitIntoWords(query);
    using DocHitsArray = vector<size_t>;
    DocHitsArray docid_count(MAX_DOCS, 0);

    {
        DUR_ACCUM("LookupAndSum");
        for (const auto& word : words)
        {
            index.LookupAndSum(word, docid_count);
        }
    }

    SearchResult search_result;

    {
        DUR_ACCUM("Top5");

        for (size_t i = 0; i < MAX_OUTPUT; ++i)
        {
            DocHitsArray::iterator curMax = max_element(docid_count.begin(), docid_count.end());

            if (*curMax == 0)
                break;

            search_result.push_back(make_pair(curMax - docid_count.begin(), *curMax));
            *curMax = 0;
        }
    }
    return search_result;
}

SearchResult SearchServer::ProcessQuery(const string& query) const
{
    const auto words = SplitIntoWords(query);
    DocHits docid_count(MAX_DOCS, make_pair(0, 0));

    {
        DUR_ACCUM("LookupAndSum");
        for (const auto& word : words)
        {
            const DocHits& dh = index.Lookup(word);

            for (auto& [doc, hits] : dh)
            {
                docid_count[doc].first = doc;
                docid_count[doc].second += hits;
            }
        }
    }

    SearchResult search_results;
    {
        DUR_ACCUM("Fill_search_results");
        search_results.reserve(MAX_DOCS);

        for (auto& [doc, hits] : docid_count)
        {
            if (hits > 0)
            {
                search_results.push_back(make_pair(doc, hits));
            }
        }
    }

    {
        DUR_ACCUM("Sort_search_results");
        sort(search_results.begin(), search_results.end(),
            [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs)
            {
                int64_t lhs_docid = lhs.first;
                auto lhs_hit_count = lhs.second;
                int64_t rhs_docid = rhs.first;
                auto rhs_hit_count = rhs.second;
                return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
            });
    }

    if (search_results.size() > MAX_OUTPUT)
    {
        search_results.resize(MAX_OUTPUT);
    }
    return search_results;
}

void SearchServer::AddQueriesStreamSingleThread(istream& query_input,
                                                ostream& search_results_output) const
{
    DUR_ACCUM();
    for (string current_query; getline(query_input, current_query); )
    {
        PrintResult(current_query, ProcessQuery(current_query), search_results_output);
    }
}

void SearchServer::PrintResult(const string& query,
                               const SearchResult& result,
                               ostream &search_results_output) const
{
    DUR_ACCUM();
    search_results_output << query << ':';

    for (auto [docid, hitcount] : result)
    {
        search_results_output
            << " {" << "docid: " << docid << ", "
            << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
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
                                  const SearchServer& srv)
{
    ResultBatch resultBatch(move(queries));
    for (const string& q : resultBatch.queries)
    {
        resultBatch.results.push_back(srv.ProcessQuery(q));
    }
    return resultBatch;
}

void SearchServer::AddQueriesStreamMultiThread(istream& query_input,
                                               ostream& search_results_output) const
{
    const size_t max_batch_size = 1000;
    vector<string> queries;
    queries.reserve(max_batch_size);
    vector<future<ResultBatch>> futures;
    bool ok = true;

    while (ok)
    {
        string current_query;
        ok = getline(query_input, current_query).good();

        if (!current_query.empty())
        {
            queries.push_back(move(current_query));
        }

        if (!(ok && queries.size() < max_batch_size))
        {
            futures.push_back(async(process_queries_batch, move(queries), ref(*this)));
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

void InvertedIndex::Add(const string& document)
{
    docs.push_back(document);
    const size_t docid = docs.size() - 1;
    map<string, size_t> wordHits;

    for (const auto& word : SplitIntoWords(document))
    {
        ++wordHits[word];
    }
    for (auto& [word, hits] : wordHits)
    {
        index[word].push_back(make_pair(docid, hits));
    }
}

const DocHits& InvertedIndex::Lookup(const string& word) const
{
    DUR_ACCUM();
    auto it = index.find(word);
    return it != index.end()? it->second : EMPTY_DOC_HITS;
}

template <typename DocHitsMap>
void InvertedIndex::LookupAndSum(const string& word,
                                 DocHitsMap& docid_count) const
{
    DUR_ACCUM();
    auto it = index.find(word);

    if (it != index.end())
    {
        for (auto& [docid, hits] : it->second)
        {
            docid_count[docid] += hits;
        }
    }
}
