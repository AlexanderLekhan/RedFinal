#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <future>

#include "synchronized.h"

#define MULTI_THREAD_VERSION
//#undef MULTI_THREAD_VERSION

using namespace std;

using DocHits = vector<pair<size_t, size_t>>;

class InvertedIndex
{
public:
    void Add(const string& document);
    template <typename DocHitsMap>
    void LookupAndSum(const string& word,
                      DocHitsMap& docid_count) const;

    const string& GetDocument(size_t id) const
    {
        return docs[id];
    }

    size_t DocsCount() const
    {
        return docs.size();
    }

private:
    map<string, DocHits> m_index;
    vector<string> docs;
};

class SearchResult
{
public:
    SearchResult(size_t maxSize)
    {
        m_data.reserve(maxSize);
    }
    DocHits::const_iterator begin() const
    {
        return m_data.begin();
    }
    DocHits::const_iterator end() const
    {
        return m_data.end();
    }
    void PushBack(pair<size_t, size_t>&& docHits);

private:
    DocHits m_data;
};

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream& document_input);
    void UpdateDocumentBase(istream& document_input);
    void AddQueriesStream(istream& query_input,
                          ostream& search_results_output);
    SearchResult ProcessQuery(const string& query);

private:
    void UpdateDocumentBaseSingleThread(istream& document_input);
    void UpdateDocumentBaseMultiThread(istream& document_input);
    void AddQueriesStreamSingleThread(istream& query_input,
                                      ostream& search_results_output);
    void AddQueriesStreamMultiThread(istream& query_input,
                                     ostream& search_results_output);
    void PrintResult(const string& query,
                     const SearchResult& result,
                     ostream& search_results_output) const;

    static const size_t MAX_OUTPUT = 5;

    Synchronized<InvertedIndex> m_index;
    future<InvertedIndex> m_newIndex;
};
