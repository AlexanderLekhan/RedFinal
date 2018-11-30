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
//#undef MULTI_THREAD_VERSION

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

    lock_guard g(m_indexMutex);
    index = move(new_index);
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output)
{
#ifdef MULTI_THREAD_VERSION
    AddQueriesStreamMultiThread(query_input, search_results_output);
#else
    AddQueriesStreamSingleThread(query_input, search_results_output);
#endif
}

SearchResult SearchServer::ProcessQuery(const string& query)
{
    const auto words = SplitIntoWords(query);
    vector<size_t> docHits(0);

    {
        DUR_ACCUM("LookupAndSum");
        lock_guard g(m_indexMutex);
        docHits.resize(index.DocsCount(), 0);

        for (const auto& word : words)
        {
            index.LookupAndSum(word, docHits);
        }
    }

    SearchResult search_result(MAX_OUTPUT);

    {
        DUR_ACCUM("Top5");
        for (size_t doc = 0; doc < docHits.size(); ++doc)
        {
            if (docHits[doc] > 0)
                search_result.PushBack(make_pair(doc, docHits[doc]));
        }
    }
    return search_result;
}

void SearchServer::AddQueriesStreamSingleThread(istream& query_input,
                                                ostream& search_results_output)
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
                                  SearchServer& srv)
{
    ResultBatch resultBatch(move(queries));
    for (const string& q : resultBatch.queries)
    {
        resultBatch.results.push_back(srv.ProcessQuery(q));
    }
    return resultBatch;
}

void SearchServer::AddQueriesStreamMultiThread(istream& query_input,
                                               ostream& search_results_output)
{
    const size_t max_batch_size = 3000;
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

void SearchResult::PushBack(pair<size_t, size_t>&& docHits)
{
    DocHits::iterator curr = m_data.end();

    for ( ; curr != m_data.begin(); --curr)
    {
        if (prev(curr)->second >= docHits.second)
            break;
    }
    if (m_data.size() < m_data.capacity())
    {
        if (curr == m_data.end())
            m_data.push_back(move(docHits));
        else
            m_data.insert(curr, move(docHits));
    }
    else
    {
        if (curr != m_data.end())
        {
            for (DocHits::iterator it = prev(m_data.end()); it != curr; --it)
            {
                *it = move(*prev(it));
            }
            *curr = move(docHits);
        }
    }
}
