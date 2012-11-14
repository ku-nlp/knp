/*
  Darts -- Double-ARray Trie System

  $Id$;

  Copyright(C) 2001-2007 Taku Kudo <taku@chasen.org>
  All rights reserved.
*/
#include <stdlib.h>
#include <darts.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

// split function with split_num
template<class T>
inline int split_string(const std::string &src, const std::string &key, T &result, int split_num)
{
    result.clear();
    int len =  src.size();
    int i = 0, si = 0, count = 0;

    while(i < len) {
	while (i < len && key.find(src[i]) != std::string::npos) { si++; i++; } // skip beginning spaces
	while (i < len && key.find(src[i]) == std::string::npos) i++; // skip contents
	if (split_num && ++count >= split_num) { // reached the specified num
	    result.push_back(src.substr(si, len - si)); // push the remainder string
	    break;
	}
	result.push_back(src.substr(si, i - si));
	si = i;
    }

    return result.size();
}

// split function
template<class T>
inline int split_string(const std::string &src, const std::string &key, T &result)
{
    return split_string(src, key, result, 0);
}

int progress_bar(size_t current, size_t total) {
  static char bar[] = "*******************************************";
  static int scale = sizeof(bar) - 1;
  static int prev = 0;

  int cur_percentage  = static_cast<int>(100.0 * current/total);
  int bar_len         = static_cast<int>(1.0   * current*scale/total);

  if (prev != cur_percentage) {
    printf("Making Double Array: %3d%% |%.*s%*s| ",
           cur_percentage, bar_len, bar, scale - bar_len, "");
    if (cur_percentage == 100)  printf("\n");
    else                        printf("\r");
    fflush(stdout);
  }

  prev = cur_percentage;

  return 1;
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " File Index" << std::endl;
    return -1;
  }

  std::string file  = argv[argc-2];
  std::string index = argv[argc-1];

  Darts::DoubleArray da;

  std::vector<const char *> key;
  std::vector<Darts::DoubleArray::result_type> val;
  std::istream *is;

  if (file == "-") {
    is = &std::cin;
  } else {
    is = new std::ifstream(file.c_str());
  }

  if (!*is) {
    std::cerr << "Cannot Open: " << file << std::endl;
    return -1;
  }

  std::string line;
  while (std::getline(*is, line)) {
    std::vector<std::string> splitted_line;
    split_string(line, " ", splitted_line);

    if (splitted_line.size() == 2) {
        char *key_char = new char[splitted_line[0].size()+1];
        std::strcpy(key_char, splitted_line[0].c_str());
        size_t value_int = atoi(splitted_line[1].c_str());

        key.push_back(key_char);
        val.push_back(value_int);
    }
  }
  if (file != "-") delete is;

  if (da.build(key.size(), &key[0], 0, &val[0], &progress_bar) != 0
      || da.save(index.c_str()) != 0) {
    std::cerr << "Error: cannot build double array  " << file << std::endl;
    return -1;
  };

  for (unsigned int i = 0; i < key.size(); i++)
    delete [] key[i];

  std::cout << "Done!, Compression Ratio: " <<
    100.0 * da.nonzero_size() / da.size() << " %" << std::endl;

  return 0;
}
