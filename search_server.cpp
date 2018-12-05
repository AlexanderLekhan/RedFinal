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

InvertedIndex::InvertedIndex(istream& document_input)
{
    for (string current_document; getline(document_input, current_document); )
    {
        if (current_document.empty())
            continue;

        m_docs.push_back(move(current_document));
        const size_t docid = m_docs.size() - 1;

        for (const string& word : SplitIntoWords(m_docs.back()))
        {
            DocHits& docHits = m_index[word];

            if (!docHits.empty() && docHits.back().first == docid)
            {
                ++docHits.back().second;
            }
            else
            {
                docHits.push_back(make_pair(docid, 1));
            }
        }
    }
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

SearchServer::SearchServer(istream& document_input)
{
    UpdateDocumentBase(document_input);
}

void update_document_base(istream& document_input, Synchronized<InvertedIndex>& index)
{
    bool flag = true;
    {
        auto access = index.GetAccess();
        if (access.ref_to_value.DocsCount() == 0)
        {
            access.ref_to_value = move(InvertedIndex(document_input));
            flag = false;
        }
    }
    if (flag)
    {
        InvertedIndex new_index(document_input);
        index.GetAccess().ref_to_value = move(new_index);
    }
}

void SearchServer::UpdateDocumentBase(istream& document_input)
{
    m_tasks.push_back(async(update_document_base, ref(document_input), ref(m_index)));
}

void process_query_stream(istream& query_input,
                          ostream& search_results_output,
                          Synchronized<InvertedIndex>& index)
{
    static const size_t MAX_OUTPUT = 5;

    for (string current_query; getline(query_input, current_query); )
    {
        if (current_query.empty())
            continue;

        const auto words = SplitIntoWords(current_query);
        vector<size_t> docHits(0);

        {
            auto access = index.GetAccess();
            docHits.resize(access.ref_to_value.DocsCount(), 0);

            for (const auto& word : words)
            {
                access.ref_to_value.LookupAndSum(word, docHits);
            }
        }

        SearchResult search_result(MAX_OUTPUT);

        for (size_t doc = 0; doc < docHits.size(); ++doc)
        {
            if (docHits[doc] > 0)
                search_result.PushBack(make_pair(doc, docHits[doc]));
        }

        search_results_output << current_query << ':';

        for (auto [docid, hitcount] : search_result)
        {
            search_results_output
                << " {" << "docid: " << docid << ", "
                << "hitcount: " << hitcount << '}';
        }
        search_results_output << endl;
    }
}

void SearchServer::AddQueriesStream(istream& query_input,
                                    ostream& search_results_output)
{
    m_tasks.push_back(async(process_query_stream,
                            ref(query_input),
                            ref(search_results_output),
                            ref(m_index)));
}

void SearchServer::WaitForAllTasks()
{
    for (auto& t : m_tasks)
    {
        if (t.valid())
            t.get();
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
