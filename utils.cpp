// utils.cpp
// Copyright (C) 2023 Richard Geldreich, Jr.
#include "utils.h"

std::string combine_strings(std::string a, const std::string& b)
{
    if (!a.size())
        return b;

    if (!b.size())
        return a;

    if (a.back() == '-')
    {
        if ((a.size() >= 2) && isdigit((uint8_t)a[a.size() - 2]))
        {
        }
        else
        {
            a.pop_back();
            a += b;
        }
    }
    else
    {
        if (a.back() != ' ')
            a += " ";
        a += b;
    }

    return a;
}

std::wstring utf8_to_wchar(const std::string& str, UINT code_page)
{
    if (str.empty())
        return std::wstring();

    int size_needed = MultiByteToWideChar(code_page, 0, &str[0], (int)str.size(), NULL, 0);
    if (!size_needed)
        return std::wstring();

    std::wstring wstrTo(size_needed, 0);
    int res = MultiByteToWideChar(code_page, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    if (!res)
        return std::wstring();

    return wstrTo;
}

std::string wchar_to_utf8(const std::wstring& wstr, UINT code_page)
{
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(code_page, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (!size_needed)
        return std::string();

    std::string strTo(size_needed, 0);
    int res = WideCharToMultiByte(code_page, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    if (!res)
        return std::string();

    return strTo;
}

static uint16_t g_codepage_437_to_unicode_0_31[32] =
{
    ' ', 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC
};

static uint16_t g_codepage_437_to_unicode_128_255[129] =
{
    0x2302,
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
};

// Code page 437 to utf8. WideCharToMultiByte etc. doesn't do the expecting thing for chars<32, and we need them.
std::string dos_to_utf8(const std::string& str)
{
    std::wstring wstr;

    for (uint8_t c : str)
    {
        if (c < 32)
            wstr.push_back(g_codepage_437_to_unicode_0_31[c]);
        else if (c >= 127)
            wstr.push_back(g_codepage_437_to_unicode_128_255[c - 127]);
        else
            wstr.push_back(c);
    }

    return wchar_to_utf8(wstr);
}

bool vformat(std::vector<char>& buf, const char* pFmt, va_list args)
{
    uint32_t buf_size = 8192;

    for (; ; )
    {
        buf.resize(buf_size);

        int res = vsnprintf(&buf[0], buf.size(), pFmt, args);
        if (res == -1)
        {
            assert(false);
            return false;
        }

        if (res <= buf.size() - 1)
            break;

        buf_size *= 2;
        if (buf_size > 16 * 1024 * 1024)
        {
            assert(false);
            return false;
        }
    }
    return true;
}

void ufprintf(FILE* pFile, const char* pFmt, ...)
{
    std::vector<char> buf;

    va_list args;
    va_start(args, pFmt);
    if (!vformat(buf, pFmt, args))
        return;
    va_end(args);

    std::wstring wbuf(utf8_to_wchar(std::string(&buf[0])));

    // Not thread safe, but we don't care
    _setmode(_fileno(pFile), _O_U16TEXT);
    fputws(&wbuf[0], pFile);
    _setmode(_fileno(pFile), _O_TEXT);
}

void uprintf(const char* pFmt, ...)
{
    std::vector<char> buf;

    va_list args;
    va_start(args, pFmt);
    if (!vformat(buf, pFmt, args))
        return;
    va_end(args);

    std::wstring wbuf(utf8_to_wchar(std::string(&buf[0])));

    // Not thread safe, but we don't care
    _setmode(_fileno(stdout), _O_U16TEXT);
    fputws(&wbuf[0], stdout);
    _setmode(_fileno(stdout), _O_TEXT);
}

std::string string_format(const char* pMsg, ...)
{
    std::vector<char> buf;

    va_list args;
    va_start(args, pMsg);
    if (!vformat(buf, pMsg, args))
        return "";
    va_end(args);

    std::string res;
    if (buf.size())
        res.assign(&buf[0]);

    return res;
}

void panic(const char* pMsg, ...)
{
    char buf[4096];

    va_list args;
    va_start(args, pMsg);
    vsnprintf(buf, sizeof(buf), pMsg, args);
    va_end(args);

    ufprintf(stderr, "%s", buf);

    exit(EXIT_FAILURE);
}

FILE* ufopen(const char* pFilename, const char* pMode)
{
    std::wstring wfilename(utf8_to_wchar(pFilename));
    std::wstring wmode(utf8_to_wchar(pMode));

    if (!wfilename.size() || !wmode.size())
        return nullptr;

    FILE* pRes = nullptr;
    _wfopen_s(&pRes, &wfilename[0], &wmode[0]);
    return pRes;
}

std::string& string_trim(std::string& str)
{
    while (str.size() && isspace((uint8_t)str.back()))
        str.pop_back();

    while (str.size() && isspace((uint8_t)str[0]))
        str.erase(0, 1);

    return str;
}

std::string& string_trim_end(std::string& str)
{
    while (str.size() && isspace((uint8_t)str.back()))
        str.pop_back();

    return str;
}

// Case sensitive, returns -1 if can't find
int string_find_first(const std::string& str, const char* pPhrase)
{
    size_t res = str.find(pPhrase, 0);
    if (res == std::string::npos)
        return -1;
    return (int)res;
}

// Case insensitive, returns -1 if can't find
int string_ifind_first(const std::string& str, const char* pPhrase)
{
    const size_t str_size = str.size();
    const size_t phrase_size = strlen(pPhrase);

    assert((int)str_size == str_size);
    assert((int)phrase_size == phrase_size);
    assert(phrase_size);

    if ((!str_size) || (!phrase_size) || (phrase_size > str_size))
        return -1;

    const size_t end_ofs = str_size - phrase_size;
    for (size_t ofs = 0; ofs <= end_ofs; ofs++)
    {
        assert(ofs + phrase_size <= str_size);
        if (_stricmp(str.c_str() + ofs, pPhrase) == 0)
            return (int)ofs;
    }
    
    return -1;
}

int string_icompare(const std::string& a, const char* pB)
{
    const size_t a_len = a.size();
    const size_t b_len = strlen(pB);

    const size_t min_len = std::min(a_len, b_len);

    for (size_t i = 0; i < min_len; i++)
    {
        const int ac = (uint8_t)utolower(a[i]);
        const int bc = (uint8_t)utolower(pB[i]);

        if (ac != bc)
            return (ac < bc) ? -1 : 1;
    }

    if (a_len == b_len)
        return 0;

    return (a_len < b_len) ? -1 : 1;
}

bool string_begins_with(const std::string& str, const char* pPhrase)
{
    const size_t str_len = str.size();

    const size_t phrase_len = strlen(pPhrase);
    assert(phrase_len);

    if (str_len >= phrase_len)
    {
        if (_strnicmp(pPhrase, str.c_str(), phrase_len) == 0)
            return true;
    }

    return false;
}

bool string_ends_in(const std::string& str, const char* pPhrase)
{
    const size_t str_len = str.size();

    const size_t phrase_len = strlen(pPhrase);
    assert(phrase_len);

    if (str_len >= phrase_len)
    {
        if (_stricmp(pPhrase, str.c_str() + str_len - phrase_len) == 0)
            return true;
    }

    return false;
}

std::string encode_url(const std::string& url)
{
    //const char* pValid_chars = ";,/?:@&=+$-_.!~*'()#";
    //const size_t valid_chars_len = strlen(pValid_chars);

    std::string res;
    for (uint32_t i = 0; i < url.size(); i++)
    {
        uint8_t c = (uint8_t)url[i];

        //const bool is_digit = (c >= 0) && (c <= '9');
        //const bool is_upper = (c >= 'A') && (c <= 'Z');
        //const bool is_lower = (c >= 'a') && (c <= 'z');

        // Escape some problematic charactes that confuse some Markdown parsers (even after using Markdown '\' escapes)
        if ((c == ')') || (c == '(') || (c == '_') || (c == '*'))
        {
            res.push_back('%');
            res.push_back(to_hex(c / 16));
            res.push_back(to_hex(c % 16));
            continue;
        }

        res.push_back(c);
    }

    return res;
}

// TODO
uint32_t crc32(const uint8_t* pBuf, size_t size, uint32_t init_crc)
{
    uint32_t crc = ~init_crc;

    for (size_t i = 0; i < size; i++)
    {
        const uint32_t byte = pBuf[i];

        crc = crc ^ byte;

        for (int j = 7; j >= 0; j--)
        {
            uint32_t mask = -((int)(crc & 1));
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}

uint32_t hash_hsieh(const uint8_t* pBuf, size_t len)
{
    if (!pBuf || !len)
        return 0;

    uint32_t h = static_cast<uint32_t>(len);

    const uint32_t bytes_left = len & 3;
    len >>= 2;

    while (len--)
    {
        const uint16_t* pWords = reinterpret_cast<const uint16_t*>(pBuf);

        h += pWords[0];

        const uint32_t t = (pWords[1] << 11) ^ h;
        h = (h << 16) ^ t;

        pBuf += sizeof(uint32_t);

        h += h >> 11;
    }

    switch (bytes_left)
    {
    case 1:
        h += *reinterpret_cast<const signed char*>(pBuf);
        h ^= h << 10;
        h += h >> 1;
        break;
    case 2:
        h += *reinterpret_cast<const uint16_t*>(pBuf);
        h ^= h << 11;
        h += h >> 17;
        break;
    case 3:
        h += *reinterpret_cast<const uint16_t*>(pBuf);
        h ^= h << 16;
        h ^= (static_cast<signed char>(pBuf[sizeof(uint16_t)])) << 18;
        h += h >> 11;
        break;
    default:
        break;
    }

    h ^= h << 3;
    h += h >> 5;
    h ^= h << 4;
    h += h >> 17;
    h ^= h << 25;
    h += h >> 6;

    return h;
}

bool read_binary_file(const char* pFilename, uint8_vec& buf)
{
    const uint64_t MAX_BINARY_FILE_LEN = 168ULL * 1024ULL * (1024ULL * 1024ULL);

    FILE* pFile = ufopen(pFilename, "rb");
    if (!pFile)
        return false;

    _fseeki64(pFile, 0, SEEK_END);
    int64_t len = _ftelli64(pFile);
    if (len < 0)
    {
        fclose(pFile);
        return false;
    }
    _fseeki64(pFile, 0, SEEK_SET);

    if (len > MAX_BINARY_FILE_LEN)
        return false;
    buf.resize(len);

    if (fread(&buf[0], len, 1, pFile) != 1)
    {
        fclose(pFile);
        return false;
    }

    fclose(pFile);
    return true;
}

bool read_text_file(const char* pFilename, string_vec& lines, bool trim_lines, bool* pUTF8_flag)
{
    FILE* pFile = ufopen(pFilename, "r");
    if (!pFile)
        return false;

    bool first_line = true;

    if (pUTF8_flag)
        *pUTF8_flag = false;
        
    while (!feof(pFile))
    {
        char buf[16384];

        char* p = fgets(buf, sizeof(buf), pFile);
        if (!p)
        {
            if (feof(pFile))
                break;

            fclose(pFile);
            return false;
        }

        std::string str(p);

        if (first_line)
        {
            first_line = false;
            if ((str.size() >= 3) &&
                ((uint8_t)str[0] == UTF8_BOM0) &&
                ((uint8_t)str[1] == UTF8_BOM1) &&
                ((uint8_t)str[2] == UTF8_BOM2))
            {
                if (pUTF8_flag)
                    *pUTF8_flag = true;
                str.erase(0, 3);
            }
        }

        while (str.size() && ((str.back() == '\n') || (str.back() == '\r')))
            str.pop_back();

        if (trim_lines)
            string_trim_end(str);

        lines.push_back(str);
    }

    fclose(pFile);
    return true;
}

bool read_text_file(const char* pFilename, std::vector<uint8_t>& buf, bool *pUTF8_flag)
{
    if (pUTF8_flag)
        *pUTF8_flag = false;

    FILE* pFile = ufopen(pFilename, "rb");
    if (!pFile)
    {
        ufprintf(stderr, "Failed reading file %s!\n", pFilename);
        return false;
    }

    fseek(pFile, 0, SEEK_END);
    uint64_t filesize = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);

    buf.resize(filesize + 1);
    fread(&buf[0], 1, filesize, pFile);

    fclose(pFile);

    if ((buf.size() >= 3) &&
        ((uint8_t)buf[0] == UTF8_BOM0) &&
        ((uint8_t)buf[1] == UTF8_BOM1) &&
        ((uint8_t)buf[2] == UTF8_BOM2))
    {
        if (pUTF8_flag)
            *pUTF8_flag = true;

        buf.erase(buf.begin(), buf.begin() + 3);
    }

    return true;
}

bool write_text_file(const char* pFilename, string_vec& lines, bool utf8_bom)
{
    FILE* pFile = ufopen(pFilename, "wb");
    if (!pFile)
        return false;

    if (utf8_bom)
    {
        if ((fputc(UTF8_BOM0, pFile) == EOF) || (fputc(UTF8_BOM1, pFile) == EOF) || (fputc(UTF8_BOM2, pFile) == EOF))
        {
            fclose(pFile);
            return false;
        }
    }

    for (uint32_t i = 0; i < lines.size(); i++)
    {
        if (lines[i].size())
        {
            if (fwrite(lines[i].c_str(), lines[i].size(), 1, pFile) != 1)
            {
                fclose(pFile);
                return false;
            }
        }

        if (fwrite("\r\n", 2, 1, pFile) != 1)
        {
            fclose(pFile);
            return false;
        }
    }

    if (fclose(pFile) == EOF)
        return false;

    return true;
}

bool serialize_to_json_file(const char* pFilename, const json& j, bool utf8_bom)
{
    FILE* pFile = ufopen(pFilename, "wb");
    if (!pFile)
        return false;

    if (utf8_bom)
    {
        if ((fputc(UTF8_BOM0, pFile) == EOF) || (fputc(UTF8_BOM1, pFile) == EOF) || (fputc(UTF8_BOM2, pFile) == EOF))
        {
            fclose(pFile);
            return false;
        }
    }

    std::string d(j.dump(2));

    if (d.size())
    {
        if (fwrite(&d[0], d.size(), 1, pFile) != 1)
        {
            fclose(pFile);
            return false;
        }
    }

    fclose(pFile);

    return true;
}

// Note: This doesn't actually handle utf8. It assumes ANSI (code page 252) text input.
static std::string extract_column_text(const std::string& str, uint32_t ofs, uint32_t len)
{
    if (ofs >= str.size())
        return "";

    const uint32_t max_len = std::min((uint32_t)str.size() - ofs, len);

    std::string res(str);
    if (ofs)
        res.erase(0, ofs);

    if (max_len < res.size())
        res.erase(max_len, res.size());

    string_trim(res);
    return res;
}

// Note: This doesn't actually handle utf8. It assumes ANSI (code page 252) text input.
bool load_column_text(const char* pFilename, std::vector<string_vec>& rows, std::string& title, string_vec& col_titles, bool empty_line_seps, const char* pExtra_col_text)
{
    string_vec lines;
    bool utf8_flag = false;
    if (!read_text_file(pFilename, lines, true, &utf8_flag))
        panic("Failed reading text file %s", pFilename);

    if (utf8_flag)
        panic("load_column_text() doesn't support utf8 yet");

    if (!lines.size() || !lines[0].size())
        panic("Expected title");

    if (lines.size() < 5)
        panic("File too small");

    for (uint32_t i = 0; i < lines.size(); i++)
    {
        if (lines[i].find_first_of(9) != std::string::npos)
            panic("Tab in file");

        string_trim(lines[i]);
    }

    title = lines[0];

    if (lines[1].size())
        panic("Expected empty line");

    std::string col_line = lines[2];

    std::string col_seps = lines[3];
    if ((!col_seps.size()) || (col_seps[0] != '-') || (col_seps.back() != '-'))
        panic("Invalid column seperator line");

    for (uint32_t i = 0; i < col_seps.size(); i++)
    {
        const uint8_t c = col_seps[i];
        if ((c != ' ') && (c != '-'))
            panic("Invalid column separator line");
    }

    int col_sep_start = 0;
    std::vector< std::pair<uint32_t, uint32_t> > column_info; // start offset and len of each column in chars

    for (uint32_t i = 1; i < col_seps.size(); i++)
    {
        const uint8_t c = col_seps[i];
        if (c == ' ')
        {
            if (col_sep_start != -1)
            {
                column_info.push_back(std::make_pair(col_sep_start, i - col_sep_start));
                col_sep_start = -1;
            }
        }
        else
        {
            if (col_sep_start == -1)
                col_sep_start = i;
        }
    }

    if (col_sep_start != -1)
    {
        column_info.push_back(std::make_pair(col_sep_start, (uint32_t)col_seps.size() - col_sep_start));
        col_sep_start = -1;
    }

    if (!column_info.size())
        panic("No columns found");

    col_titles.resize(column_info.size());
    for (uint32_t i = 0; i < column_info.size(); i++)
    {
        col_titles[i] = col_line;
                
        if (column_info[i].first)
            col_titles[i].erase(0, column_info[i].first);

        if (column_info[i].second > col_titles[i].size())
            panic("invalid columns");
                
        col_titles[i].erase(column_info[i].second, col_titles[i].size() - column_info[i].second);
        string_trim(col_titles[i]);
    }

    for (uint32_t i = 0; i < column_info.size() - 1; i++)
        column_info[i].second = column_info[i + 1].first - column_info[i].first;
    column_info.back().second = 32000;

    uint32_t cur_line = 4;

    uint32_t cur_record_index = 0;

    while (cur_line < lines.size())
    {
        string_vec rec_lines;
        rec_lines.push_back(lines[cur_line++]);

        if (empty_line_seps)
        {
            while (cur_line < lines.size())
            {
                if (!lines[cur_line].size())
                    break;

                rec_lines.push_back(lines[cur_line++]);
            }

            // cur_line should be blank, or we're at the end of the file
            if (cur_line < lines.size())
            {
                cur_line++;
                if (cur_line < lines.size())
                {
                    if (!lines[cur_line].size())
                        panic("Expected non-empty line");
                }
            }
        }

        //uprintf("%u:\n", cur_record_index);
        //for (uint32_t i = 0; i < rec_lines.size(); i++)
        //    uprintf("%s\n", rec_lines[i].c_str());

        string_vec col_lines(column_info.size());

        for (uint32_t col_index = 0; col_index < column_info.size(); col_index++)
        {
            for (uint32_t l = 0; l < rec_lines.size(); l++)
            {
                std::string col_text(extract_column_text(rec_lines[l], column_info[col_index].first, column_info[col_index].second));

                if (col_text.size())
                {
                    if (col_lines[col_index].size())
                    {
                        if ((col_lines[col_index].back() != '-') && ((uint8_t)col_lines[col_index].back() != ANSI_SOFT_HYPHEN))
                            col_lines[col_index].push_back(' ');
                        else
                        {
                            if ((col_lines[col_index].size() >= 2) && (!isdigit((uint8_t)col_lines[col_index][col_lines[col_index].size() - 2])))
                                col_lines[col_index].pop_back();
                        }
                    }

                    col_lines[col_index] += col_text;
                }
            }
        }

        if (pExtra_col_text)
            col_lines.push_back(pExtra_col_text);

        // Convert from ANSI (code page 252) to UTF8.
        for (auto& l : col_lines)
            l = ansi_to_utf8(l);

        rows.push_back(col_lines);
        
        cur_record_index++;
    }

    return true;
}

bool invoke_curl(const std::string& args, string_vec& reply)
{
    reply.clear();

    remove("__temp.html");

    // Invoke curl.exe
    std::string cmd(string_format("curl.exe \"%s\" -o __temp.html", args.c_str()));
    uprintf("Command: %s\n", cmd.c_str());

    int status = system(cmd.c_str());
    uprintf("curl returned status %i\n", status);

    if (status != EXIT_SUCCESS)
        return false;

    // Read output file.

    FILE* pFile = ufopen("__temp.html", "rb");
    if (!pFile)
    {
        Sleep(50);
        pFile = ufopen("__temp.html", "rb");
        if (!pFile)
            return false;
    }

    uint8_t buf[6] = { 0,0,0,0,0,0 };
    fread(buf, 5, 1, pFile);
    fclose(pFile);

    // Try to detect some common binary file types

    // PDF
    if (memcmp(buf, "%PDF-", 5) == 0)
    {
        uprintf("PDF file detected\n");

        std::string filename(args);
        for (size_t i = filename.size() - 1; i >= 0; i--)
        {
            if (filename[i] == '/')
            {
                filename.erase(0, i + 1);
                break;
            }
        }

        std::string new_link_deescaped;
        for (uint32_t i = 0; i < filename.size(); i++)
        {
            uint8_t c = filename[i];
            if ((c == '%') && ((i + 2) < filename.size()))
            {
                int da = convert_hex_digit(filename[i + 1]);
                int db = convert_hex_digit(filename[i + 2]);
                if (da >= 0 && db >= 0)
                {
                    int val = da * 16 + db;
                    new_link_deescaped.push_back((uint8_t)val);
                }

                i += 2;
            }
            else
                new_link_deescaped.push_back(c);
        }

        rename("__temp.html", new_link_deescaped.c_str());
        uprintf("Renamed __temp.html to %s\n", new_link_deescaped.c_str());

        return true;
    }

    // JPEG
    if (memcmp(buf, "\xFF\xD8\xFF\xE0", 4) == 0)
    {
        uprintf("JPEG file detected\n");
        return true;
    }

    if (!read_text_file("__temp.html", reply, true, nullptr))
    {
        // Wait a bit and try again, rarely needed under Windows.
        Sleep(50);
        if (!read_text_file("__temp.html", reply, true, nullptr))
            return false;
    }

    return true;
}

void convert_args_to_utf8(string_vec& args, int argc, wchar_t* argv[])
{
    args.resize(argc);

    for (int i = 0; i < argc; i++)
    {
        args[i] = wchar_to_utf8(argv[i]);
        if (args[i].size() >= 2)
        {
            if ((args[i][0] == '\"') && (args[i].back() == '\"'))
            {
                args[i].pop_back();
                args[i].erase(0, 1);
            }
        }
    }
}

std::string string_slice(const std::string& str, size_t ofs, size_t len)
{
    if (!len)
        return "";

    if (ofs > str.size())
    {
        assert(0);
        return "";
    }

    const size_t max_len = str.size() - ofs;

    len = std::min(len, max_len);

    std::string res(str);
    if (ofs)
        res.erase(0, ofs);
    
    if (len)
        res.resize(len);
    
    return res;
}

bool invoke_openai(const std::string& prompt, std::string& reply)
{
    reply.clear();

    // Write prompt to i.txt
    FILE* pFile = ufopen("i.txt", "wb");
    fwrite(prompt.c_str(), prompt.size(), 1, pFile);
    fclose(pFile);

    // Invoke openai.exe
    int status = system("openai.exe i.txt o.txt");
    if (status != EXIT_SUCCESS)
        return false;

    // Read output file.
    string_vec lines;
    if (!read_text_file("o.txt", lines, true, nullptr))
    {
        // Wait a bit and try again, rarely needed under Windows.
        Sleep(50);
        if (!read_text_file("o.txt", lines, true, nullptr))
            return false;
    }

    // Skip any blank lines at the beginning of the reply.
    uint32_t i;
    for (i = 0; i < lines.size(); i++)
    {
        std::string s(lines[i]);
        string_trim(s);
        if (s.size())
            break;
    }

    for (; i < lines.size(); i++)
        reply += lines[i];

    return true;
}

std::string get_deg_to_dms(double deg)
{
    deg = std::round(fabs(deg) * 3600.0f);

    int min_secs = (int)fmod(deg, 3600.0f);

    deg = std::floor((deg - (double)min_secs) / 3600.0f);

    int minutes = min_secs / 60;
    int secs = min_secs % 60;

    return string_format("%02i%:%02i:%02i", (int)deg, minutes, secs);
}

bool load_json_object(const char* pFilename, bool& utf8_flag, json &result_obj)
{
    std::vector<uint8_t> buf;

    if (!read_text_file(pFilename, buf, &utf8_flag))
        return false;

    if (!buf.size())
        return false;

    bool success = false;
    try
    {
        result_obj = json::parse(buf.begin(), buf.end());
        success = true;
    }
    catch (json::exception& e)
    {
        fprintf(stderr, "load_json_object: Parse of file \"%s\" failed (id %i): %s", pFilename, e.id, e.what());
        return false;
    }

    if (!result_obj.is_object() && !result_obj.is_array())
        return false;

    return true;
}

void string_tokenize(
    const std::string &str, 
    const std::string &whitespace,
    const std::string &break_chars,
    string_vec &tokens,
    uint_vec *pOffsets_vec)
{
    tokens.resize(0);
    if (pOffsets_vec)
        pOffsets_vec->resize(0);

    std::string cur_token;
    uint32_t cur_ofs = 0;
        
    for (uint32_t i = 0; i < str.size(); i++)
    {
        uint8_t c = str[i];

        if (whitespace.find_first_of(c) != std::string::npos)
        {
            if (cur_token.size())
            {
                tokens.push_back(cur_token);
                if (pOffsets_vec)
                    pOffsets_vec->push_back(cur_ofs);

                cur_token.clear();
            }
        }
        else if (break_chars.find_first_of(c) != std::string::npos)
        {
            if (cur_token.size())
            {
                tokens.push_back(cur_token);
                if (pOffsets_vec)
                    pOffsets_vec->push_back(cur_ofs);

                cur_token.clear();
            }

            std::string s;
            s.push_back(c);

            tokens.push_back(s);
            if (pOffsets_vec)
                pOffsets_vec->push_back(i);
        }
        else
        {
            if (!cur_token.size())
                cur_ofs = i;

            cur_token.push_back(c);
        }
    }

    if (cur_token.size())
    {
        tokens.push_back(cur_token);
        if (pOffsets_vec)
            pOffsets_vec->push_back(cur_ofs);
    }
}

const double PI = 3.141592653589793238463;

double deg2rad(double deg)
{
    return (deg * PI / 180.0);
}

double rad2deg(double rad)
{
    return (rad * 180.0 / PI);
}

// input in degrees
double geo_distance(double lat1, double lon1, double lat2, double lon2, int unit)
{
    if ((lat1 == lat2) && (lon1 == lon2)) 
        return 0;

    double theta = lon1 - lon2;
    double dist = cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta)) + sin(deg2rad(lat1)) * sin(deg2rad(lat2));
    dist = acos(dist);
    dist = rad2deg(dist);

    dist = dist * 60 * 1.1515;

    switch (unit) 
    {
    case 'M':
        break;
    case 'K':
        dist = dist * 1.609344;
        break;
    case 'N':
        dist = dist * 0.8684;
        break;
    default:
        assert(0);
        break;
    }

    return (dist);
}

std::string remove_bom(std::string str)
{
    if (str.size() >= 3)
    {
        if (((uint8_t)str[0] == UTF8_BOM0) && ((uint8_t)str[1] == UTF8_BOM1) && ((uint8_t)str[2] == UTF8_BOM2))
        {
            str.erase(0, 3);
        }
    }

    return str;
}

int get_next_utf8_code_point_len(const uint8_t* pStr) 
{
    if (pStr == nullptr || *pStr == 0) 
    {
        // Return 0 if the input is null or points to a null terminator
        return 0; 
    }

    const uint8_t firstByte = *pStr;

    if ((firstByte & 0x80) == 0) 
    { 
        // Starts with 0, ASCII character
        return 1;
    }
    else if ((firstByte & 0xE0) == 0xC0) 
    { 
        // Starts with 110
        return 2;
    }
    else if ((firstByte & 0xF0) == 0xE0) 
    { 
        // Starts with 1110
        return 3;
    }
    else if ((firstByte & 0xF8) == 0xF0) 
    { 
        // Starts with 11110
        return 4;
    }
    else 
    {
        // Invalid UTF-8 byte sequence
        return -1;
    }
}

void get_string_words(
    const std::string& str,
    string_vec& words,
    uint_vec* pOffsets_vec)
{
    const uint8_t* pStr = (const uint8_t *)str.c_str();

    words.resize(0);
    if (pOffsets_vec)
        pOffsets_vec->resize(0);

    std::string cur_token;

    const std::string whitespace(" \t\n\r,;:.!?()[]*/\"");
    
    int word_start_ofs = -1;
    
    uint32_t cur_ofs = 0;
    while ((cur_ofs < str.size()) && (pStr[cur_ofs]))
    {
        int l = get_next_utf8_code_point_len(pStr + cur_ofs);
        const uint8_t c = pStr[cur_ofs];

        if (l <= 0)
        {
            assert(0);
            l = 1;
        }

        bool is_whitespace = (whitespace.find_first_of(c) != std::string::npos);

        if ((l == 2) && (c == 0xc2))
        {
            // NO-BREAK SPACE
            if (pStr[cur_ofs + 1] == 0xa0)
                is_whitespace = true;
        }

        if ((l == 2) && (c == 0xCA))
        {
            // single left quote
            if (pStr[cur_ofs + 1] == 0xBB)
                is_whitespace = true;
        }

        if ((l == 3) && (c == 0xE2) && (pStr[cur_ofs + 1] == 0x80))
        {
            // dash
            if (pStr[cur_ofs + 2] == 0x93)
                is_whitespace = true;
            // dash
            if (pStr[cur_ofs + 2] == 0x94)
                is_whitespace = true;
            // left quote
            else if (pStr[cur_ofs + 2] == 0x9C)
                is_whitespace = true;
            // right quote
            else if (pStr[cur_ofs + 2] == 0x9D)
                is_whitespace = true;
            // ellipsis (three dots)
            else if (pStr[cur_ofs + 2] == 0xA)
                is_whitespace = true;
            // ellipsis (three dots)
            else if (pStr[cur_ofs + 2] == 0xA6)
                is_whitespace = true;
            // long dash
            else if (pStr[cur_ofs + 2] == 9)
                is_whitespace = true;
            // left single quote
            else if (pStr[cur_ofs + 2] == 0x98)
                is_whitespace = true;
            // right single quote
            else if (pStr[cur_ofs + 2] == 0x99)
                is_whitespace = true;
            // right double quote
            else if (pStr[cur_ofs + 2] == 0x9D)
                is_whitespace = true;
        }
        
        if (is_whitespace)
        {
            if (cur_token.size())
            {
                words.push_back(cur_token);
                if (pOffsets_vec)
                    pOffsets_vec->push_back(word_start_ofs);

                cur_token.clear();
                word_start_ofs = -1;
            }
        }
        else
        {
            if (word_start_ofs < 0)
                word_start_ofs = cur_ofs;

            if (l == 1)
            {
                cur_token.push_back(utolower(c));
            }
            else
            {
                for (int i = 0; i < l; i++)
                    cur_token.push_back(pStr[cur_ofs + i]);
            }
        }
                
        cur_ofs += l;
    }

    if (cur_token.size())
    {
        words.push_back(cur_token);

        if (pOffsets_vec)
            pOffsets_vec->push_back(word_start_ofs);
    }
}

void get_utf8_code_point_offsets(const char* pStr, int_vec& offsets)
{
    uint32_t cur_ofs = 0;
    
    offsets.resize(0);

    while (pStr[cur_ofs])
    {
        offsets.push_back(cur_ofs);

        cur_ofs += std::max<int>(1, get_next_utf8_code_point_len((const uint8_t*)pStr + cur_ofs));
    }
}
