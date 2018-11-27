#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <vector>
#include <map>
#include <string>

using namespace std;

class DocHits : public vector<pair<size_t, size_t>>
{
public:
    DocHits(const vector<size_t>& singleDocHits);
    DocHits& operator+=(const DocHits& other);
};

class InvertedIndex
{
public:
    void Add(const string& document);
    void LookupAndSum(const string& word, DocHits& docid_count) const;

    const string& GetDocument(size_t id) const
    {
        return docs[id];
    }

private:
    map<string, DocHits> index;
    vector<string> docs;
};

using SearchResult = vector<pair<size_t, size_t>>;

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

    const size_t MAX_OUTPUT = 5;

private:
    void PrintResult(const string& query,
                     const SearchResult& result,
                     ostream& search_results_output) const;
    InvertedIndex index;
};
