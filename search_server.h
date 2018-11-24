#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
using namespace std;

class DocHits : public map<size_t, size_t>
{
public:
    DocHits& operator+=(const DocHits& other)
    {
        for (auto& [doc, hits] : other)
        {
            (*this)[doc] += hits;
        }
        return *this;
    }
};

class InvertedIndex
{
public:
    void Add(const string& document);
    DocHits Lookup(const string& word) const;

    const string& GetDocument(size_t id) const
    {
        return docs[id];
    }

private:
    map<string, DocHits> index;
    vector<string> docs;
};

using SearchResult = vector < pair < size_t, size_t > >;

class SearchServer
{
public:
    SearchServer() = default;
    explicit SearchServer(istream& document_input);
    void UpdateDocumentBase(istream& document_input);
    void AddQueriesStream(istream& query_input, ostream& search_results_output) const;

    const size_t MAX_OUTPUT = 5;

private:
    void PrintResult(const string& query,
                     const SearchResult& result,
                     ostream& search_results_output) const;
    InvertedIndex index;
};
