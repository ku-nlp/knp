#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <ios>
#include <iomanip>
#include <stdlib.h>
#include <math.h>
#include <string.h>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

#define INTERSECTION_NUM_THRESHOLD	2

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

// int to string
template<class T>
inline std::string int2string(const T i)
{
    std::ostringstream o;

    o << i;
    return o.str();
}

#endif
