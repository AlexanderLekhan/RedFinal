#include "parse.h"

string_view Strip(string_view s) {
  while (!s.empty() && isspace(s.front())) {
    s.remove_prefix(1);
  }
  while (!s.empty() && isspace(s.back())) {
    s.remove_suffix(1);
  }
  return s;
}

vector<string_view> SplitBy(string_view s, char sep) {
  vector<string_view> result;
  while (!s.empty()) {
    size_t pos = s.find(sep);
    result.push_back(s.substr(0, pos));
    s.remove_prefix(pos != s.npos ? pos + 1 : s.size());
  }
  return result;
}

string_view ReadToken(string_view& sv)
{
    size_t pos1 = 0;

    while (!sv.empty() && isspace(sv[pos1]))
    {
        ++pos1;
    }
    auto pos2 = sv.find(' ', pos1);
    string_view result;

    if (pos2 != string::npos)
    {
        result = sv.substr(pos1, pos2 - pos1);
        sv.remove_prefix(pos2);
    }
    else
    {
        result = sv.substr(pos1, pos2);
        sv.remove_prefix(sv.size());
    }
    return result;
}

vector<string> SplitIntoWords(const string& line)
{
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

vector<string_view> SplitIntoWordsView(string_view str)
{
    vector<string_view> result;

    for (string_view word = ReadToken(str); !word.empty(); word = ReadToken(str))
    {
        result.push_back(word);
    }

    return result;
}
