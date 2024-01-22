#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <thread>
#include <mutex>
#include <iomanip>

using namespace std;

class Dictionary
{
private:
  vector<string> words_in_file; // store every single word from file
  vector<vector<vector<vector<int>>>> dictionary; // [len][pos][char][words]
  vector<string> possible_words; // all possible words that may be matched
  vector<int> words_found; // all words found through searching

public:
  Dictionary(string filename); // constructor to store words into containers from file
  void generate_possible(const string &pattern, string current, size_t index); // combine characters if there're {} in pattern
  void fix_with_length(const string &pattern); // fix length if there're integers in pattern
  void search_pattern(const string &pattern); // organize searching process
  void intersect_sets(const string &word); // find intersection of each possible word
  void search_only_length(const string &word); // if there're no alphabet
  static bool is_digit(char s) { return isdigit(s); }
  static bool is_alpha(char s) { return isalpha(s); }
  int get_num_match() { return words_found.size(); } // get number of matched words
  void print_result()
  {
    sort(words_found.begin(), words_found.end());
    for (auto index : words_found)
      cout << words_in_file[index] << endl;
  }
};

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    cout << "USAGE: program_name filename" << endl;
    ;
    return 1;
  }
  auto load_start = chrono::high_resolution_clock::now();
  Dictionary dic(argv[1]);
  auto load_end = chrono::high_resolution_clock::now();
  auto load_duration = chrono::duration_cast<chrono::nanoseconds>(load_end - load_start).count();
  float load_time = load_duration / 1000000000.0;
  cout << "Load time = " << fixed << setprecision(9) << load_time << " secs" << endl;
  string pattern;
  cout << "Enter a pattern: ";
  cin >> pattern;
  while (pattern != "quit") // while user doesn't input quit
  {
    auto search_start = chrono::high_resolution_clock::now();
    dic.search_pattern(pattern);
    auto search_end = chrono::high_resolution_clock::now();
    auto search_duration = chrono::duration_cast<chrono::nanoseconds>(search_end - search_start).count();
    dic.print_result();
    float search_time = search_duration / 1000000000.0;
    cout << "Pattern = " << pattern << ", Words matched = " << dic.get_num_match() << ", Search time = " << fixed << setprecision(9) << search_time << " secs" << endl;
    cout << "Enter a pattern: ";
    cin >> pattern;
  }
}

Dictionary::Dictionary(string filename)
{
  ifstream file(filename);
  string word;
  int max_length = 0; // determine max length of word to resize 4D vector
  while (file >> word)
  {
    words_in_file.emplace_back(word);
    if (max_length < word.length())
      max_length = word.length();
  }

  file.close();
  dictionary.resize(max_length + 1, vector<vector<vector<int>>>(max_length + 1, vector<vector<int>>(26)));
  int pos = 0;
  for (const auto &word : words_in_file)
  {
    for (int i = 0; i < word.length(); i++)
    {
      dictionary[word.length()][i][word[i] - 97].emplace_back(pos);
    }
    pos++;
  }
}

void Dictionary::search_pattern(const string &pattern)
{
  possible_words.clear();
  words_found.clear();
  if (count(pattern.begin(), pattern.end(), '{') > 0) // if there're {}
    generate_possible(pattern, "", 0);
  else
    possible_words.emplace_back(pattern);

  if (count_if(pattern.begin(), pattern.end(), is_digit) > 0)
  {
    vector<string> tmp(possible_words);
    possible_words.clear();
    for (const auto &word : tmp)
      fix_with_length(word);
  }
  if (count_if(pattern.begin(), pattern.end(), is_alpha) == 0)
  {
    for (const auto &word : possible_words)
      search_only_length(word);
    return;
  }

  for (const auto &word : possible_words) // go trough all possible
    intersect_sets(word);
}

void Dictionary::generate_possible(const string &pattern, string current, size_t index)
{
  if (index >= pattern.length()) // if reaching the last index
  {
    possible_words.push_back(current);
    return;
  }

  if (pattern[index] == '{') 
  {
    auto closingIndex = pattern.find('}', index);
    if (closingIndex != string::npos)
    {
      string choices = pattern.substr(index + 1, closingIndex - index - 1);
      for (auto val : choices)
      {
        generate_possible(pattern, current + val, closingIndex + 1); // recursive
      }
    }
  }
  else
  {
    generate_possible(pattern, current + pattern[index], index + 1);
  }
}

void Dictionary::fix_with_length(const string &pattern)
{
  vector<string> result;
  result.emplace_back("");
  for (int i = 0; i < pattern.length(); i++)
  {
    if (isalpha(pattern[i]))
    {
      for (auto &word : result)
        word = word + pattern[i];
    }
    else // if it is integer
    {
      // if next index is not ":"
      if (i == pattern.length() - 1 || pattern[i + 1] != ':')
      {
        int len = 0;
        if (isdigit(pattern[i + 1]))
          len = (pattern[i] - '0') * 10;
        else
          len = pattern[i] - '0';
        for (auto &word : result)
          word = word + string(len, '*');
      }
      // if next index is ":"
      else
      {
        vector<string> new_words;
        for (auto &word : result)
        {
          int min = pattern[i] - '0' + 1;
          int max = 0;
          if (isdigit(pattern[i + 3]))
          {
            max = (pattern[i + 2] - '0') * 10 + (pattern[i + 3] - '0');
            i++;
          }
          else
            max = pattern[i + 2] - '0';
          for (int j = min; j <= max; j++)
          {
            new_words.emplace_back(word + string(j, '*'));
          }
          word = word + string(min - 1, '*');
        }
        result.insert(result.end(), new_words.begin(), new_words.end());
        i += 2;
      }
    }
  }
  possible_words.insert(possible_words.end(), result.begin(), result.end());
}

void Dictionary::intersect_sets(const string &word)
{
  int index;
  for (int i = 0; i < word.length(); i++)
  {
    if (word[i] != '*')
    {
      index = i; // find the first index to start with
      break;
    }
  }
  auto matched_words = dictionary[word.length()][index][word[index] - 97];
  if (matched_words.empty())
    return;
  for (int i = index + 1; i < word.length(); i++)
  {
    if (word[i] != '*')
    {
      const auto &tmp = dictionary[word.length()][i][word[i] - 97];
      auto itr = set_intersection(matched_words.begin(), matched_words.end(), tmp.begin(), tmp.end(), matched_words.begin()); // get intersection of two vectors
      matched_words.erase(itr, matched_words.end());
      if (matched_words.empty())
        return;
    }
  }
  words_found.insert(words_found.end(), matched_words.begin(), matched_words.end());
}

void Dictionary::search_only_length(const string &word)
{
  for (const auto &v1 : dictionary[word.length()])
    for (const auto &v2 : v1)
      for (const auto &val : v2)
        words_found.emplace_back(val); // store every word with same length
}
