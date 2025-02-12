// HEX SDK

#ifndef HEX_STRING_UTIL_H
#define HEX_STRING_UTIL_H

#ifdef __cplusplus

#include <sstream>
#include <vector>
#include <algorithm>

namespace hex_string_util {

/** to convert a value to a string */
template<typename T>
inline std::string toString(const T& val)
{
    std::stringstream ss;
    ss << val;
    std::string s;
    ss >> s;
    return s;
};

inline bool isInteger(const std::string &s)
{
    if (s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+')))
        return false ;

    char * p ;
    strtol(s.c_str(), &p, 10) ;

    return (*p == 0) ;
}

/** return true if s1 starts with s2, otherwise false */
inline bool startsWith(const std::string& s1, const std::string& s2)
{
    return (s1.compare(0, s2.length(), s2) == 0);
}

/** return true if s1 ends with s2, otherwise false */
inline bool endsWith(const std::string& s1, const std::string& s2)
{
    size_t i = s1.rfind(s2);
    return ((i != std::string::npos) && (i == (s1.length() - s2.length())));
}

/* split a string into a provided vector */
inline std::vector<std::string>&
split(const std::string& s, char delim, std::vector<std::string>& elems)
{
    std::stringstream ss(s);
    std::string item;

    while(std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }

    return elems;
}

/* split a string and return resulting vector */
inline std::vector<std::string>
split(const std::string & s, char delim)
{
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

/* strip leading characters */
inline void lstrip(std::string& s, const char* cset = "\t ")
{
    size_t c = s.find_first_not_of(cset);
    if (c != std::string::npos)
        s.erase(0, c);
}

/* strip trailing characters */
inline void rstrip(std::string& s, const char* cset = "\t ")
{
    size_t c = s.find_last_not_of(cset);
    if (c != std::string::npos)
        s.erase(c + 1);
}

/* strip leading and trailing characters */
inline void strip(std::string& s, const char* cset = "\t ")
{
    lstrip(s, cset);
    rstrip(s, cset);
}


inline std::string
toLower(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

inline std::string
toUpper(std::string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

inline void
remove(std::string& s, char r)
{
    s.erase(std::remove(s.begin(), s.end(), r), s.end());
}

inline void
replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);

    if(start_pos != std::string::npos)
        str.replace(start_pos, from.length(), to);
}

inline std::string
escapeDoubleQuote(const std::string &str)
{
    // 1. Escape each quote(") to 2 quote '""'
    std::stringstream out;
    for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
        if (*it == '"')
            out << "\"\"";
        else
            out << *it;
    }
    return out.str();
}

} /* namespace hex_string_util */

#endif /* __cplusplus */

#endif /* endif HEX_STRING_UTIL_H */

