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

using namespace std;

using DocHits = vector<pair<size_t, size_t>>;

class InvertedIndex
{
public:
    InvertedIndex() = default;
    explicit InvertedIndex(istream& document_input);
    template <typename DocHitsMap>
    void LookupAndSum(const string& word,
                      DocHitsMap& docid_count) const;

    const string& GetDocument(size_t id) const
    {
        return m_docs[id];
    }

    size_t DocsCount() const
    {
        return m_docs.size();
    }

private:
    map<string, DocHits> m_index;
    vector<string> m_docs;
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
    void WaitForAllTasks();

private:
    Synchronized<InvertedIndex> m_index;
    vector<future<void>> m_tasks;
};
