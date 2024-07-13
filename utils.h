// utils.h
// Copyright (C) 2023 Richard Geldreich, Jr.
#pragma once

#ifdef _MSC_VER
#pragma warning (disable:4100) // unreferenced formal parameter
#pragma warning (disable:4505) // unreferenced function with internal linkage has been removed)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#endif

#include <fcntl.h>
#include <io.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>

#include <algorithm>
#include <map>
#include <set>
#include <varargs.h>
#include <string>

#include <unordered_set>

#include "pjson.h"

#include "libsoldout/markdown.h"

#include "json/json.hpp"
using json = nlohmann::json;

typedef std::vector<std::string> string_vec;
typedef std::unordered_set<std::string> unordered_string_set;
typedef std::vector<uint8_t> uint8_vec;
typedef std::pair<std::string, std::string> string_pair;
typedef std::vector<int> int_vec;
typedef std::vector<uint32_t> uint_vec;

const uint32_t UTF8_BOM0 = 0xEF, UTF8_BOM1 = 0xBB, UTF8_BOM2 = 0xBF;

// Code page 1242 (ANSI) soft hyphen character.
// See http://www.alanwood.net/demos/ansi.html
const uint32_t ANSI_SOFT_HYPHEN = 0xAD;

template<typename T> inline void clear_obj(T& obj) { memset(&obj, 0, sizeof(T)); }

void panic(const char* pMsg, ...);

//------------------------------------------------------------------

inline bool string_is_digits(const std::string& s)
{
    for (char c : s)
        if (!isdigit((uint8_t)c))
            return false;

    return true;
}

inline bool string_is_alpha(const std::string& s)
{
    for (char c : s)
        if (!isalpha((uint8_t)c))
            return false;

    return true;
}

std::string combine_strings(std::string a, const std::string& b);

// Convert an UTF8 string to a wide Unicode String
std::wstring utf8_to_wchar(const std::string& str, UINT code_page = CP_UTF8);

// Convert a wide Unicode string to an UTF8 string
std::string wchar_to_utf8(const std::wstring& wstr, UINT code_page = CP_UTF8);

inline std::string ansi_to_utf8(const std::string& str) { return wchar_to_utf8(utf8_to_wchar(str, 1252)); }

// Code page 437 to utf8. WideCharToMultiByte etc. doesn't do the expecting thing for chars<32, and we need them.
std::string dos_to_utf8(const std::string& str);

// utf8 string format 
bool vformat(std::vector<char>& buf, const char* pFmt, va_list args);

// utf8 printf to FILE*
void ufprintf(FILE* pFile, const char* pFmt, ...);

// utf8 print to stdout
void uprintf(const char* pFmt, ...);

std::string string_format(const char* pMsg, ...);

void panic(const char* pMsg, ...);

// Open a file given a utf8 filename
FILE* ufopen(const char* pFilename, const char* pMode);

// like tolower() but doesn't assert on negative values and doesn't factor in locale
inline char utolower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return (c - 'A') + 'a';
    return c;
}

inline uint8_t utolower(uint8_t c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return (c - 'A') + 'a';
    return c;
}

// like toupper() but doesn't assert on negative values and doesn't factor in locale
inline char utoupper(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (c - 'a') + 'A';
    return c;
}

inline uint8_t utoupper(uint8_t c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (c - 'a') + 'A';
    return c;
}

// like isdigit() but doesn't assert on negative values and doesn't factor in locale
inline bool uisdigit(char c)
{
    return (c >= '0') && (c <= '9');
}

inline bool uisdigit(uint8_t c)
{
    return (c >= '0') && (c <= '9');
}

// like isupper() but doesn't assert on negative values and doesn't factor in locale
inline bool uisupper(char c)
{
    return (c >= 'A') && (c <= 'Z');
}

inline bool uisupper(uint8_t c)
{
    return (c >= 'A') && (c <= 'Z');
}

// like islower() but doesn't assert on negative values and doesn't factor in locale
inline bool uislower(char c)
{
    return (c >= 'a') && (c <= 'z');
}

inline bool uislower(uint8_t c)
{
    return (c >= 'a') && (c <= 'z');
}

// like isalpha() but doesn't assert on negative values and doesn't factor in locale
inline bool uisalpha(char c)
{
    return uisupper(c) || uislower(c);
}

inline bool uisalpha(uint8_t c)
{
    return uisupper(c) || uislower(c);
}

inline int convert_hex_digit(int d)
{
    if ((d >= 'a') && (d <= 'f'))
        return (d - 'a') + 10;
    else if ((d >= 'A') && (d <= 'F'))
        return (d - 'A') + 10;
    else if ((d >= '0') && (d <= '9'))
        return d - '0';

    return -1;
}

inline std::string string_lower(std::string str)
{
    for (char& c : str)
        c = (char)utolower((uint8_t)c);
    return str;
}

inline std::string string_upper(std::string str)
{
    for (char& c : str)
        c = (char)utoupper((uint8_t)c);
    return str;
}

std::string& string_trim(std::string& str);

std::string& string_trim_end(std::string& str);

// Case sensitive, returns -1 if can't find
int string_find_first(const std::string& str, const char* pPhrase);

int string_ifind_first(const std::string& str, const char* pPhrase);

int string_icompare(const std::string& a, const char* pB);

// Case insensitive
bool string_begins_with(const std::string& str, const char* pPhrase);

// Case insensitive
bool string_ends_in(const std::string& str, const char* pPhrase);

std::string string_slice(const std::string& str, size_t ofs, size_t len = UINT32_MAX);

inline char to_hex(uint32_t val)
{
    assert(val <= 15);
    return (char)((val <= 9) ? ('0' + val) : ('A' + val - 10));
}

std::string encode_url(const std::string& url);

uint32_t crc32(const uint8_t* pBuf, size_t size, uint32_t init_crc = 0);

uint32_t hash_hsieh(const uint8_t* pBuf, size_t len);

bool read_binary_file(const char* pFilename, uint8_vec& buf);

bool read_text_file(const char* pFilename, string_vec& lines, bool trim_lines, bool* pUTF8_flag);

bool read_text_file(const char* pFilename, std::vector<uint8_t>& buf, bool *pUTF8_flag);

bool write_text_file(const char* pFilename, const string_vec& lines, bool utf8_bom = true);

bool serialize_to_json_file(const char* pFilename, const json& j, bool utf8_bom);

bool load_column_text(const char* pFilename, std::vector<string_vec>& rows, std::string& title, string_vec& col_titles, bool empty_line_seps, const char *pExtra_col_text);

bool invoke_curl(const std::string& args, string_vec& reply);

void convert_args_to_utf8(string_vec& args, int argc, wchar_t* argv[]);

bool invoke_openai(const std::string& prompt, std::string& reply);
bool invoke_openai(const string_vec& prompt, string_vec& reply);

std::string get_deg_to_dms(double deg);

bool load_json_object(const char* pFilename, bool& utf8_flag, json& result_obj);

inline bool load_json_object(const char* pFilename, json& result_obj) { bool utf8_flag = false; return load_json_object(pFilename, utf8_flag, result_obj); }

void string_tokenize(const std::string& str, const std::string& whitespace, const std::string& break_chars, string_vec& tokens, uint_vec* pOffsets_vec = nullptr);

double deg2rad(double deg);
double rad2deg(double rad);

// input in degrees
double geo_distance(double lat1, double lon1, double lat2, double lon2, int unit = 'M');

std::string remove_bom(std::string str);

int get_next_utf8_code_point_len(const uint8_t* pStr);
void get_string_words(const std::string& str, string_vec& words, uint_vec* pOffsets_vec, const char *pAdditional_whitespace = nullptr);
void get_utf8_code_point_offsets(const char* pStr, int_vec& offsets);

void init_norm();
void normalize_diacritics(const char* pStr, std::string& res);
std::string normalize_word(const std::string& str);
bool is_stop_word(const std::string& word);

std::string ustrlwr(const std::string& s);

std::string string_replace(const std::string& str, const std::string& find, const std::string& repl);

bool does_file_exist(const char* pFilename);

int get_julian_date(int M, int D, int Y);

// Returns 0-6:
// Sun 0
// Mon 1
// Tue 2
// Wed 3
// Thu 4
// Fri 5
// Sat 6
int get_day_of_week(int julian);

