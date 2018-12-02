#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <cassert>

using namespace std;

vector<string> SplitIntoWords(const string& line)
{
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

template <typename DocHitsMap>
void InvertedIndex::LookupAndSum(const string& word,
                                 DocHitsMap& docid_count) const
{
    auto it = m_index.find(word);

    if (it != m_index.end())
    {
        for (auto& [docid, hits] : it->second)
        {
            docid_count[docid] += hits;
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
        m_index[word].push_back(make_pair(docid, hits));
    }
}

SearchServer::SearchServer(istream& document_input)
{
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input)
{
    UpdateDocumentBaseMultiThread(document_input);
}

void SearchServer::UpdateDocumentBaseSingleThread(istream& document_input)
{
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document); )
    {
        if (!current_document.empty())
        {
            new_index.Add(move(current_document));
        }
    }

    auto access = m_index.GetAccess();
    access.ref_to_value = move(new_index);
}

void SearchServer::UpdateDocumentBaseMultiThread(istream& document_input)
{
    lock_guard g(m_newIndexMutex);
    m_newIndex = async([&document_input]()
    {
        InvertedIndex new_index;

        for (string current_document; getline(document_input, current_document); )
        {
            if (!current_document.empty())
            {
                new_index.Add(move(current_document));
            }
        }

        return new_index;
    });
}

SearchResult SearchServer::ProcessQuery(const string& query)
{
    {
        lock_guard g(m_newIndexMutex);

        if (m_newIndex.valid())
        {
            auto access = m_index.GetAccess();

            if (access.ref_to_value.DocsCount() == 0
                || m_newIndex.wait_for(chrono::seconds(0)) == future_status::ready)
            {
                access.ref_to_value = move(m_newIndex.get());
            }
        }
    }

    const auto words = SplitIntoWords(query);
    vector<size_t> docHits(0);

    {
        auto access = m_index.GetAccess();
        docHits.resize(access.ref_to_value.DocsCount(), 0);

        for (const auto& word : words)
        {
            access.ref_to_value.LookupAndSum(word, docHits);
        }
    }

    SearchResult search_result(MAX_OUTPUT);

    {
        for (size_t doc = 0; doc < docHits.size(); ++doc)
        {
            if (docHits[doc] > 0)
                search_result.PushBack(make_pair(doc, docHits[doc]));
        }
    }
    return search_result;
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output)
{
    AddQueriesStreamMultiThread(query_input, search_results_output);
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
    const size_t max_batch_size = 2000;
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
