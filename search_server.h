#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <vector>
#include <map>
#include <string>

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
    map<string, DocHits> index;
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
                          ostream& search_results_output) const;

    SearchResult ProcessQuery(const string& query) const;
    void AddQueriesStreamSingleThread(istream& query_input,
                                      ostream& search_results_output) const;
    void AddQueriesStreamMultiThread(istream& query_input,
                                     ostream& search_results_output) const;

    static const size_t MAX_OUTPUT = 5;

private:
    void PrintResult(const string& query,
                     const SearchResult& result,
                     ostream& search_results_output) const;
    InvertedIndex index;
};
