// ufojson.cpp - Copyright (C) 2023 Richard Geldreich, Jr.
#define TIMELINE_VERSION "1.10"

#ifdef _MSC_VER
#pragma warning (disable:4100) // unreferenced formal parameter
#pragma warning (disable:4505) // unreferenced function with internal linkage has been removed)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#endif

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

#include <fcntl.h>
#include <io.h>

#include "libsoldout/markdown.h"
#include "pjson.h"

#include "json/json.hpp"
using json = nlohmann::json;

#define USE_OPENAI (0)

const uint32_t UTF8_BOM0 = 0xEF, UTF8_BOM1 = 0xBB, UTF8_BOM2 = 0xBF;

// Code page 1242 (ANSI) soft hyphen character.
// See http://www.alanwood.net/demos/ansi.html
const uint32_t ANSI_SOFT_HYPHEN = 0xAD; 

static void panic(const char* pMsg, ...);

typedef std::vector<std::string> string_vec;

//------------------------------------------------------------------

static bool string_is_digits(const std::string& s)
{
    for (char c : s)
        if (!isdigit((uint8_t)c))
            return false;

    return true;
}

static bool string_is_alpha(const std::string& s)
{
    for (char c : s)
        if (!isalpha((uint8_t)c))
            return false;

    return true;
}

static std::string combine_strings(std::string a, const std::string& b)
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

// Convert an UTF8 string to a wide Unicode String
static std::wstring utf8_to_wchar(const std::string& str, UINT code_page = CP_UTF8)
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

// Convert a wide Unicode string to an UTF8 string
static std::string wchar_to_utf8(const std::wstring& wstr, UINT code_page = CP_UTF8)
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

static std::string ansi_to_utf8(const std::string &str)
{
    return wchar_to_utf8(utf8_to_wchar(str, 1252));
}

// utf8 string format 
static bool vformat(std::vector<char> &buf, const char* pFmt, va_list args)
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

// utf8 printf to FILE*
static void ufprintf(FILE *pFile, const char* pFmt, ...)
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

// utf8 print to stdout
static void uprintf(const char* pFmt, ...)
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

static std::string string_format(const char* pMsg, ...)
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

static void panic(const char* pMsg, ...)
{
    char buf[4096];

    va_list args;
    va_start(args, pMsg);
    vsnprintf(buf, sizeof(buf), pMsg, args);
    va_end(args);

    ufprintf(stderr, "%s", buf);
    
    exit(EXIT_FAILURE);
}

// Open a file given a utf8 filename
static FILE* ufopen(const char* pFilename, const char* pMode)
{
    std::wstring wfilename(utf8_to_wchar(pFilename));
    std::wstring wmode(utf8_to_wchar(pMode));

    if (!wfilename.size() || !wmode.size())
        return nullptr;

    FILE* pRes = nullptr;
    _wfopen_s(&pRes, &wfilename[0], &wmode[0]);
    return pRes;
}

static std::string string_lower(std::string str)
{
    for (char& c : str)
        c = (char)tolower((uint8_t)c);
    return str;
}

static std::string& string_trim(std::string& str)
{
    while (str.size() && isspace((uint8_t)str.back()))
        str.pop_back();

    while (str.size() && isspace((uint8_t)str[0]))
        str.erase(0, 1);

    return str;
}

static std::string& string_trim_end(std::string& str)
{
    while (str.size() && isspace((uint8_t)str.back()))
        str.pop_back();

    return str;
}

// Case sensitive, returns -1 if can't find
static int string_find_first(const std::string& str, const char* pPhrase)
{
    size_t res = str.find(pPhrase, 0);
    if (res == std::string::npos)
        return -1;
    return (int)res;
}

// Case insensitive
static bool string_begins_with(const std::string& str, const char* pPhrase)
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

// Case insensitive
static bool string_ends_in(const std::string& str, const char* pPhrase)
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

static char to_hex(uint32_t val)
{
    assert(val <= 15);
    return (char)((val <= 9) ? ('0' + val) : ('A' + val - 10));
}

static std::string encode_url(const std::string& url)
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

static uint32_t crc32(const uint8_t* pBuf, size_t size, uint32_t init_crc = 0)
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

static bool read_text_file(const char* pFilename, string_vec& lines, bool trim_lines = true, bool *pUTF8_flag = nullptr)
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

static bool read_text_file(const char* pFilename, std::vector<uint8_t>& buf, bool& utf8_flag)
{
    utf8_flag = false;

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
        utf8_flag = true;

        buf.erase(buf.begin(), buf.begin() + 3);
    }

    return true;
}

static bool write_text_file(const char* pFilename, string_vec& lines, bool utf8_bom = true)
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
        if ((fwrite(lines[i].c_str(), lines[i].size(), 1, pFile) != 1) || (fwrite("\r\n", 2, 1, pFile) != 1))
        {
            fclose(pFile);
            return false;
        }
    }

    if (fclose(pFile) == EOF)
        return false;

    return true;
}

static bool serialize_to_json_file(const char* pFilename, const json& j, bool utf8_bom)
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

//-------------------------------------------------------------------

struct markdown
{
    enum
    {
        cCodeSig = 0xFE,

        cCodeLink = 1,
        cCodeEmphasis,
        cCodeText,
        cCodeParagraph,
        cCodeLinebreak,
        cCodeHTML
    };

    static void bufappend(struct buf* out, struct buf* in)
    {
        assert(in != out);

        if (in && in->size)
            bufput(out, in->data, in->size);
    }

    static void writelen(struct buf* ob, uint32_t size)
    {
        bufputc(ob, (uint8_t)(size & 0xFF));
        bufputc(ob, (uint8_t)((size >> 8) & 0xFF));
        bufputc(ob, (uint8_t)((size >> 16) & 0xFF));
        bufputc(ob, (uint8_t)((size >> 24) & 0xFF));
    }

    static std::string get_string(const std::string& buf, uint32_t& cur_ofs, uint32_t text_size)
    {
        std::string text;
        if (cur_ofs + text_size > buf.size())
            panic("Buffer too small");

        text.append(buf.c_str() + cur_ofs, text_size);
        cur_ofs += text_size;

        return text;
    }

    static uint32_t get_len32(const std::string& buf, uint32_t& ofs)
    {
        if ((ofs + 4) > buf.size())
            panic("Buffer too small");

        uint32_t l = (uint8_t)buf[ofs] |
            (((uint8_t)buf[ofs + 1]) << 8) |
            (((uint8_t)buf[ofs + 2]) << 16) |
            (((uint8_t)buf[ofs + 3]) << 24);

        ofs += 4;

        return l;
    }

    static void prolog(struct buf* ob, void* opaque)
    {
    }

    static void epilog(struct buf* ob, void* opaque)
    {
    }

    /* block level callbacks - NULL skips the block */
    static void blockcode(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "blockcode: \"%.*s\" ", (int)text->size, text->data);
#endif
        panic("unsupported markdown feature");
    }

    static void blockquote(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "blockquote: \"%.*s\" ", (int)text->size, text->data);
#endif
        panic("unsupported markdown feature");
    }

    static void blockhtml(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "blockhtml: \"%.*s\" ", (int)text->size, text->data);
#endif
        panic("unsupported markdown feature");
    }

    static void header(struct buf* ob, struct buf* text, int level, void* opaque)
    {
#if 0
        bufprintf(ob, "header: %i \"%.*s\" ", level, (int)text->size, text->data);
#endif
        panic("unsupported markdown feature");
    }

    static void hrule(struct buf* ob, void* opaque)
    {
        panic("unsupported markdown feature");
    }

    static void list(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
        panic("unsupported markdown feature");
    }

    static void listitem(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
        panic("unsupported markdown feature");
    }

    static void paragraph(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "paragraph: \"%.*s\" ", (int)text->size, text->data);
#endif
        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
    }

    static void table(struct buf* ob, struct buf* head_row, struct buf* rows, void* opaque)
    {
#if 0
        bufprintf(ob, "table: \"%.*s\" \"%.*s\" ", (int)head_row->size, head_row->data, (int)rows->size, rows->data);
#endif
        panic("unsupported markdown feature");
    }

    static void table_cell(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
#if 0
        bufprintf(ob, "table_cell: \"%.*s\" %i ", (int)text->size, text->data, flags);
#endif
        panic("unsupported markdown feature");
    }

    static void table_row(struct buf* ob, struct buf* cells, int flags, void* opaque)
    {
#if 0
        bufprintf(ob, "table_row: \"%.*s\" %i ", (int)cells->size, cells->data, flags);
#endif
        panic("unsupported markdown feature");
    }

    static int autolink(struct buf* ob, struct buf* link, enum mkd_autolink type, void* opaque)
    {
#if 0
        bufprintf(ob, "autolink: %u \"%.*s\" ", type, (int)link->size, link->data);
#endif
        panic("unsupported markdown feature");
        return 1;
    }

    static int codespan(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "codespan: \"%.*s\" ", (int)text->size, text->data);
#endif
        panic("unsupported markdown feature");
        return 1;
    }

    static int double_emphasis(struct buf* ob, struct buf* text, char c, void* opaque)
    {
#if 0
        bufprintf(ob, "double_emphasis: %u ('%c') [%.*s] ", c, c, (int)text->size, text->data);
#endif
        if (!text || !text->size)
            return 1;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeEmphasis);
        bufputc(ob, c);
        bufputc(ob, 2);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);

        return 1;
    }

    static int emphasis(struct buf* ob, struct buf* text, char c, void* opaque)
    {
#if 0
        bufprintf(ob, "emphasis: %u ('%c') [%.*s] ", c, c, (int)text->size, text->data);
#endif

        if (!text || !text->size)
            return 1;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeEmphasis);
        bufputc(ob, c);
        bufputc(ob, 1);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);

        return 1;
    }

    static int image(struct buf* ob, struct buf* link, struct buf* title, struct buf* alt, void* opaque)
    {
#if 0
        bufprintf(ob, "image: \"%.*s\" \"%.*s\" \"%.*s\" ",
            (int)link->size, link->data,
            (int)title->size, title->data,
            (int)alt->size, alt->data);
#endif
        panic("unsupported markdown feature");
        return 1;
    }

    static int linebreak(struct buf* ob, void* opaque)
    {
#if 0
        bufprintf(ob, "linebreak ");
#endif

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeLinebreak);

        return 1;
    }

    static int link(struct buf* ob, struct buf* link, struct buf* title, struct buf* content, void* opaque)
    {
#if 0
        printf("link: {%.*s} {%.*s} {%.*s}\n",
            link ? (int)link->size : 0,
            link ? link->data : nullptr,
            title ? (int)title->size : 0,
            title ? title->data : nullptr,
            content ? (int)content->size : 0,
            content ? content->data : nullptr);
#endif
        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeLink);
        writelen(ob, (uint32_t)link->size);
        writelen(ob, (uint32_t)content->size);

        bufappend(ob, link);
        bufappend(ob, content);

        return 1;
    }

    static int raw_html_tag(struct buf* ob, struct buf* tag, void* opaque)
    {
        //bufprintf(ob, "raw_html_tag: \"%.*s\" ", (int)tag->size, tag->data);

        if (!tag || !tag->size)
            return 1;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeHTML);
        writelen(ob, (uint32_t)tag->size);
        bufappend(ob, tag);

        return 1;
    }

    static int triple_emphasis(struct buf* ob, struct buf* text, char c, void* opaque)
    {
        //bufprintf(ob, "triple_emphasis: %u ('%c') [%.*s] ", c, c, (int)text->size, text->data);

        if (!text || !text->size)
            return 1;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeEmphasis);
        bufputc(ob, c);
        bufputc(ob, 3);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);

        return 1;
    }

    static void normal_text(struct buf* ob, struct buf* text, void* opaque)
    {
        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeText);
        writelen(ob, (uint32_t)text->size);
        for (uint32_t i = 0; i < text->size; i++)
        {
            uint8_t c = text->data[i];
            if (c == '\n')
                bufputc(ob, ' ');
            else if (c != 1)
            {
                assert(c >= 32 || c == '\t');
                bufputc(ob, c);
            }
        }
    }
};

const struct mkd_renderer mkd_parse =
{
    markdown::prolog,
    markdown::epilog,

    markdown::blockcode,
    markdown::blockquote,
    markdown::blockhtml,
    markdown::header,
    markdown::hrule,
    markdown::list,
    markdown::listitem,
    markdown::paragraph,
    markdown::table,
    markdown::table_cell,
    markdown::table_row,

    markdown::autolink,
    markdown::codespan,
    markdown::double_emphasis,
    markdown::emphasis,
    markdown::image,
    markdown::linebreak,
    markdown::link,
    markdown::raw_html_tag,
    markdown::triple_emphasis,

    //markdown::entity,
    nullptr,
    markdown::normal_text,

    64,
    "*_",
    nullptr
};

static bool markdown_should_escape(int c)
{
    switch (c)
    {
    case '\\':
    case '`':
    case '*':
    case '_':
    case '{':
    case '}':
    case '[':
    case ']':
    case '<':
    case '>':
    case '(':
    case ')':
    case '#':
        //case '-':
        //case '.':
        //case '!':
    case '|':
        return true;
    default:
        break;
    }

    return false;
}

static std::string escape_markdown(const std::string& str)
{
    std::string out;

    for (uint32_t i = 0; i < str.size(); i++)
    {
        uint8_t c = str[i];

        if (markdown_should_escape(c))
            out.push_back('\\');

        out.push_back(c);
    }

    return out;
}

static int convert_hex_digit(int d)
{
    if ((d >= 'a') && (d <= 'f'))
        return (d - 'a') + 10;
    else if ((d >= 'A') && (d <= 'F'))
        return (d - 'A') + 10;
    else if ((d >= '0') && (d <= '9'))
        return d - '0';

    return -1;
}

class markdown_text_processor
{
public:
    struct detail
    {
        detail() : m_link_index(-1), m_emphasis(0), m_emphasis_amount(0), m_linebreak(false), m_end_paragraph(false) { }

        string_vec m_html;

        int m_link_index;

        char m_emphasis;
        uint8_t m_emphasis_amount;
        bool m_linebreak;
        bool m_end_paragraph;
    };

    std::string m_text;
    std::vector<detail> m_details;
    string_vec m_links;

    markdown_text_processor()
    {
    }

    void clear()
    {
        m_text.clear();
        m_details.clear();
        m_links.clear();
    }
        
    void fix_redirect_urls()
    {
        for (uint32_t link_index = 0; link_index < m_links.size(); link_index++)
        {
            const char* pPrefix = "https://www.google.com/url?q=";

            if (!string_begins_with(m_links[link_index], pPrefix))
                continue;

            size_t p;
            if ((p = m_links[link_index].find("&sa=D&source=editors&ust=")) == std::string::npos)
                continue;

            size_t r = m_links[link_index].find("&usg=");
            if ((r == std::string::npos) || (r < p))
                continue;

            if ((r - p) != 41)
                continue;

            if ((m_links[link_index].size() - r) != 33)
                continue;
            
            if ((m_links[link_index].size() - p) != 74)
                continue;

            std::string new_link(m_links[link_index]);
            new_link.erase(p, new_link.size() - p);

            new_link.erase(0, strlen(pPrefix));

            // De-escape the string
            std::string new_link_deescaped;
            for (uint32_t i = 0; i < new_link.size(); i++)
            {
                uint8_t c = new_link[i];
                if ( (c == '%') && ((i + 2) < new_link.size()) )
                {
                    int da = convert_hex_digit(new_link[i + 1]);
                    int db = convert_hex_digit(new_link[i + 2]);
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

            //printf("%s\n", new_link.c_str());

            m_links[link_index] = new_link_deescaped;
        }

        for (uint32_t i = 0; i < m_links.size(); i++)
            m_links[i] = encode_url(m_links[i]);
    }

    void init_from_markdown(const char* pText)
    {
        struct buf* pIn = bufnew(4096);
        bufputs(pIn, pText);

        struct buf* pOut = bufnew(4096);
        markdown(pOut, pIn, &mkd_parse);

        std::string buf;
        buf.append((char*)pOut->data, pOut->size);

        init_from_codes(buf);

        bufrelease(pIn);
        bufrelease(pOut);
    }

    bool split_in_half(uint32_t ofs, markdown_text_processor& a, markdown_text_processor& b) const
    {
        assert((this != &a) && (this != &b));

        if (m_details[ofs].m_emphasis != 0)
            return false;

        a.m_text = m_text;
        a.m_details = m_details;
        a.m_links = m_links;

        b.m_text = m_text;
        b.m_details = m_details;
        b.m_links = m_links;

        a.m_text.erase(ofs, a.m_text.size() - ofs);
        a.m_details.erase(a.m_details.begin() + ofs, a.m_details.end());

        b.m_text.erase(0, ofs);
        b.m_details.erase(b.m_details.begin(), b.m_details.begin() + ofs);

        return true;
    }

    uint32_t count_char_in_text(uint8_t c) const
    {
        uint32_t num = 0;
        for (uint32_t i = 0; i < m_text.size(); i++)
        {
            if ((uint8_t)m_text[i] == c)
                num++;
        }
        return num;
    }

    bool split_last_parens(markdown_text_processor& a, markdown_text_processor& b) const
    {
        a.clear();
        b.clear();

        if (!m_text.size())
            return false;

        int ofs = (int)m_text.size() - 1;
        while ((m_text[ofs] == '\n') || (m_text[ofs] == ' '))
        {
            if (!ofs)
                return false;
            ofs--;
        }

        if (m_text[ofs] == '.')
        {
            if (!ofs)
                return false;

            ofs--;
        }

        if (m_text[ofs] != ')')
            return false;

        int level = 0;
        while (ofs >= 0)
        {
            uint8_t c = (uint8_t)m_text[ofs];

            if (c == ')')
                level++;
            else if (c == '(')
            {
                level--;
                if (!level)
                    break;
            }

            ofs--;
        }
        if (ofs < 0)
            return false;

        return split_in_half(ofs, a, b);
    }

    void convert_to_plain(std::string& out, bool trim_end) const
    {
        for (uint32_t i = 0; i < m_text.size(); i++)
        {
            uint8_t c = m_text[i];

            assert((c == '\n') || (c == '\t') || (c >= 32));

            out.push_back(c);
        }

        if (trim_end)
        {
            while (out.size() && out.back() == '\n')
                out.pop_back();

            string_trim_end(out);
        }
    }

    void convert_to_markdown(std::string& out, bool trim_end) const
    {
        int emphasis = 0, emphasis_amount = 0;
        int cur_link_index = -1;

        for (uint32_t text_ofs = 0; text_ofs < m_text.size(); text_ofs++)
        {
            if (m_details[text_ofs].m_link_index != -1)
            {
                // Inside link at current position

                if (cur_link_index == -1)
                {
                    // Not currently inside a link, so start a new link

                    handle_html(out, text_ofs);
                                        
                    out.push_back('[');

                    // Beginning new link
                    handle_emphasis(out, text_ofs, emphasis, emphasis_amount);
                }
                else if (cur_link_index != m_details[text_ofs].m_link_index)
                {
                    // Switching to different link, so flush current link and start a new one
                    handle_emphasis(out, text_ofs, emphasis, emphasis_amount);

                    out += "](";

                    for (uint32_t j = 0; j < m_links[cur_link_index].size(); j++)
                    {
                        uint8_t c = m_links[cur_link_index][j];
                        if (markdown_should_escape(c))
                            out.push_back('\\');
                        out.push_back(c);
                    }

                    out.push_back(')');

                    handle_html(out, text_ofs);
                                        
                    out.push_back('[');
                }
                else
                {
                    // Currently inside a link which hasn't changed

                    handle_html(out, text_ofs);

                    handle_emphasis(out, text_ofs, emphasis, emphasis_amount);
                }

                cur_link_index = m_details[text_ofs].m_link_index;
            }
            else
            {
                // Not inside link at current position

                if (cur_link_index != -1)
                {
                    // Flush current link
                    handle_emphasis(out, text_ofs, emphasis, emphasis_amount);

                    out += "](";

                    for (uint32_t j = 0; j < m_links[cur_link_index].size(); j++)
                    {
                        uint8_t c = m_links[cur_link_index][j];
                        if (markdown_should_escape(c))
                            out.push_back('\\');
                        out.push_back(c);
                    }

                    out.push_back(')');

                    handle_html(out, text_ofs);
                                        
                    cur_link_index = -1;
                }
                else
                {
                    handle_html(out, text_ofs);

                    handle_emphasis(out, text_ofs, emphasis, emphasis_amount);
                }
            }

            if (m_details[text_ofs].m_linebreak)
            {
                out.push_back(' ');

                // One space will already be in the text.
                //out.push_back(' ');
            }

            uint8_t c = m_text[text_ofs];
            if (markdown_should_escape(c))
            {
                // Markdown escape
                out.push_back('\\');
            }

            out.push_back(c);
        }

        if (emphasis != 0)
        {
            // Flush last emphasis
            for (int j = 0; j < emphasis_amount; j++)
                out.push_back((uint8_t)emphasis);
        }
        emphasis = 0;
        emphasis_amount = 0;

        if (cur_link_index != -1)
        {
            // Flush last link
            out += "](";

            for (uint32_t j = 0; j < m_links[cur_link_index].size(); j++)
            {
                uint8_t c = m_links[cur_link_index][j];
                if (markdown_should_escape(c))
                    out.push_back('\\');
                out.push_back(c);
            }

            out.push_back(')');
            cur_link_index = -1;
        }
                
        if (m_details.size() > m_text.size())
        {
            if (m_details.size() != m_text.size() + 1)
                panic("details array too large");

            if (m_details.back().m_html.size())
            {
                for (uint32_t i = 0; i < m_details.back().m_html.size(); i++)
                    out += m_details.back().m_html[i];
            }
        }

        if (trim_end)
        {
            while (out.size() && out.back() == '\n')
                out.pop_back();

            string_trim_end(out);
        }
    }

private:
    void ensure_detail_ofs(uint32_t ofs)
    {
        if (m_details.size() <= ofs)
            m_details.resize(ofs + 1);
    }

    void init_from_codes(const std::string& buf)
    {
        m_text.resize(0);
        m_details.resize(0);
        m_links.resize(0);

        parse_block(buf);
    }

    void parse_block(const std::string& buf)
    {
        uint32_t cur_ofs = 0;
        while (cur_ofs < buf.size())
        {
            uint8_t sig = (uint8_t)buf[cur_ofs];

            if (sig != markdown::cCodeSig)
                panic("Expected code block signature");

            cur_ofs++;
            if (cur_ofs == buf.size())
                panic("Premature end of buffer");

            uint8_t code_type = (uint8_t)buf[cur_ofs];
            cur_ofs++;

            switch (code_type)
            {
            case markdown::cCodeLink:
            {
                const uint32_t link_size = markdown::get_len32(buf, cur_ofs);
                const uint32_t content_size = markdown::get_len32(buf, cur_ofs);

                std::string link(markdown::get_string(buf, cur_ofs, link_size));
                std::string content(markdown::get_string(buf, cur_ofs, content_size));

                const uint32_t link_index = (uint32_t)m_links.size();
                m_links.push_back(link);

                const uint32_t start_text_ofs = (uint32_t)m_text.size();

                parse_block(content);

                const uint32_t end_text_ofs = (uint32_t)m_text.size();
                if (end_text_ofs)
                {
                    ensure_detail_ofs(end_text_ofs - 1);

                    for (uint32_t i = start_text_ofs; i < end_text_ofs; i++)
                        m_details[i].m_link_index = link_index;
                }

                break;
            }
            case markdown::cCodeEmphasis:
            {
                if (cur_ofs >= buf.size())
                    panic("Buffer too small");

                const uint8_t c = (uint8_t)buf[cur_ofs++];

                if (cur_ofs >= buf.size())
                    panic("Buffer too small");

                const uint32_t amount = (uint8_t)buf[cur_ofs++];

                const uint32_t text_size = markdown::get_len32(buf, cur_ofs);

                std::string text(markdown::get_string(buf, cur_ofs, text_size));

                const uint32_t start_text_ofs = (uint32_t)m_text.size();

                parse_block(text);

                const uint32_t end_text_ofs = (uint32_t)m_text.size();

                if (end_text_ofs)
                {
                    ensure_detail_ofs(end_text_ofs - 1);

                    for (uint32_t i = start_text_ofs; i < end_text_ofs; i++)
                    {
                        m_details[i].m_emphasis = c;
                        m_details[i].m_emphasis_amount = (uint8_t)amount;
                    }
                }

                break;
            }
            case markdown::cCodeText:
            {
                const uint32_t text_size = markdown::get_len32(buf, cur_ofs);
                std::string text(markdown::get_string(buf, cur_ofs, text_size));

                for (size_t i = 0; i < text.size(); i++)
                {
                    // value 1 is written by the markdown parser when it wants to delete a \n
                    if (text[i] != 1)
                        m_text.push_back(text[i]);
                }

                break;
            }
            case markdown::cCodeParagraph:
            {
                const uint32_t text_size = markdown::get_len32(buf, cur_ofs);
                std::string text(markdown::get_string(buf, cur_ofs, text_size));

                parse_block(text);

                m_text += "\n";
                m_text += "\n";

                ensure_detail_ofs((uint32_t)m_text.size() - 1);
                m_details[m_text.size() - 1].m_end_paragraph = true;

                break;
            }
            case markdown::cCodeLinebreak:
            {
                m_text += "\n";

                ensure_detail_ofs((uint32_t)m_text.size() - 1);
                m_details[m_text.size() - 1].m_linebreak = true;

                break;
            }
            case markdown::cCodeHTML:
            {
                const uint32_t text_size = markdown::get_len32(buf, cur_ofs);
                std::string text(markdown::get_string(buf, cur_ofs, text_size));

                uint32_t ofs = (uint32_t)m_text.size();
                ensure_detail_ofs(ofs);
                m_details[ofs].m_html.push_back(text);

                break;
            }
            default:
                panic("Invalid code");
                break;
            }
        }

        if (m_text.size())
            ensure_detail_ofs((uint32_t)m_text.size() - 1);
    }

    void handle_html(std::string& out, uint32_t text_ofs) const
    {
        // Any HTML appears before this character
        for (uint32_t i = 0; i < m_details[text_ofs].m_html.size(); i++)
            out += m_details[text_ofs].m_html[i];
    }

    void handle_emphasis(std::string& out, uint32_t text_ofs, int& emphasis, int& emphasis_amount) const
    {
        if (m_details[text_ofs].m_emphasis != 0)
        {
            // Desired emphasis
            if ((m_details[text_ofs].m_emphasis == emphasis) && (m_details[text_ofs].m_emphasis_amount == emphasis_amount))
            {
                // No change to emphasis

                // Any HTML appears before this character
                //for (uint32_t i = 0; i < m_details[text_ofs].m_html.size(); i++)
                //    out += m_details[text_ofs].m_html[i];
            }
            else
            {
                // Change to emphasis
                if (emphasis != 0)
                {
                    // Flush out current emphasis
                    for (int j = 0; j < emphasis_amount; j++)
                        out.push_back((uint8_t)emphasis);
                }

                // Any HTML appears before this character
                //for (uint32_t i = 0; i < m_details[text_ofs].m_html.size(); i++)
                //    out += m_details[text_ofs].m_html[i];

                emphasis = m_details[text_ofs].m_emphasis;
                emphasis_amount = m_details[text_ofs].m_emphasis_amount;

                // Start new emphasis
                for (int j = 0; j < emphasis_amount; j++)
                    out.push_back((uint8_t)emphasis);
            }
        }
        else if (m_details[text_ofs].m_emphasis == 0)
        {
            // Desires no emphasis
            if (emphasis != 0)
            {
                // Flush out current emphasis
                for (int j = 0; j < emphasis_amount; j++)
                    out.push_back((uint8_t)emphasis);
            }
            emphasis = 0;
            emphasis_amount = 0;

            // Any HTML appears before this character
            //for (uint32_t i = 0; i < m_details[text_ofs].m_html.size(); i++)
            //    out += m_details[text_ofs].m_html[i];
        }
    }
};

//-------------------------------------------------------------------

// Note that May ends in a period.
static const char* g_months[12] =
{
    "Jan.",
    "Feb.",
    "Mar.",
    "Apr.",
    "May.",
    "Jun.",
    "Jul.",
    "Aug.",
    "Sep.",
    "Oct.",
    "Nov.",
    "Dec."
};

static const char* g_full_months[12] =
{
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December"
};

enum date_prefix_t
{
    cNoPrefix = -1,
        
    cEarlySpring,
    cEarlySummer,
    cEarlyAutumn,
    cEarlyFall,
    cEarlyWinter,

    cMidSpring,
    cMidSummer,
    cMidAutumn,
    cMidFall,
    cMidWinter,

    cLateSpring,
    cLateSummer,
    cLateAutumn,
    cLateFall,
    cLateWinter,

    cSpring,
    cSummer,
    cAutumn,
    cFall,
    cWinter,
    
    cEarly,
    cMiddleOf,
    cLate,
    cEndOf,

    cTotalPrefixes
};

static const char* g_date_prefix_strings[] =
{
    "Early Spring",
    "Early Summer",
    "Early Autumn",
    "Early Fall",
    "Early Winter",

    "Mid Spring",
    "Mid Summer",
    "Mid Autumn",
    "Mid Fall",
    "Mid Winter",
    
    "Late Spring",
    "Late Summer",
    "Late Autumn",
    "Late Fall",
    "Late Winter",

    "Spring",
    "Summer",
    "Autumn",
    "Fall",
    "Winter",

    "Early",
    "Mid",
    "Late",
    "End of"
};

static bool is_season(date_prefix_t prefix)
{
    switch (prefix)
    {
    case cEarlySpring:
    case cEarlySummer:
    case cEarlyAutumn:
    case cEarlyFall:
    case cEarlyWinter:

    case cMidSpring:
    case cMidSummer:
    case cMidAutumn:
    case cMidFall:
    case cMidWinter:

    case cLateSpring:
    case cLateSummer:
    case cLateAutumn:
    case cLateFall:
    case cLateWinter:

    case cSpring:
    case cSummer:
    case cAutumn:
    case cFall:
    case cWinter:
        return true;
    default:
        break;
    }

    return false;
}

static int determine_month(const std::string& date)
{
    uint32_t i;
    for (i = 0; i < 12; i++)
        if (string_begins_with(date, g_full_months[i]))
            return i;
    return -1;
}

struct event_date
{
    date_prefix_t m_prefix;

    int m_year;
    int m_month;    // 1 based: [1,12] (NOT ZERO BASED), -1=invalid
    int m_day;      // 1 based: [1,31], -1=invalid

    bool m_fuzzy; // ?
    bool m_plural; // 's

    bool m_approx; // (approximate)
    bool m_estimated; // (estimated)
                
    event_date()
    {
        clear();
    }

    event_date(const event_date& other)
    {
        *this = other;
    }

    bool sanity_check() const
    {
        if (m_year == -1)
            return false;

        if ((m_month == 0) || (m_day == 0))
            return false;

        if ((m_month < -1) || (m_month > 12))
            return false;
        
        if ((m_day < -1) || (m_day > 31))
            return false;

        if (m_plural)
        {
            if (m_month != -1)
                return false;
        }

        if (m_month == -1)
        {
            if (m_day != -1)
                return false;
        }

        if (is_season(m_prefix))
        {
            if (m_month != -1)
                return false;
            
            if (m_day != -1)
                return false;
        }
        else if ((m_prefix >= cEarly) && (m_prefix <= cEndOf))
        {
            if (m_day != -1)
                return false;
        }

        return true;
    }

    bool operator== (const event_date& rhs) const
    {
        return (m_prefix == rhs.m_prefix) &&
            (m_year == rhs.m_year) &&
            (m_month == rhs.m_month) &&
            (m_day == rhs.m_day) &&
            (m_fuzzy == rhs.m_fuzzy) &&
            (m_plural == rhs.m_plural) &&
            (m_approx == rhs.m_approx) &&
            (m_estimated == rhs.m_estimated);
    }

    bool operator!= (const event_date& rhs) const
    {
        return !(*this == rhs);
    }

    event_date& operator =(const event_date& rhs)
    {
        m_prefix = rhs.m_prefix;
        m_year = rhs.m_year;
        m_month = rhs.m_month;
        m_day = rhs.m_day;
        m_fuzzy = rhs.m_fuzzy;
        m_plural = rhs.m_plural;
        m_approx = rhs.m_approx;
        m_estimated = rhs.m_estimated;
        return *this;
    }

    void clear()
    {
        m_prefix = cNoPrefix;

        m_year = -1;
        m_month = -1;
        m_day = -1;
                
        m_fuzzy = false; //?
        m_plural = false; // 's
        m_approx = false; // (approximate)
        m_estimated = false; // (estimated)
    }

    bool is_valid() const
    {
        return m_year != -1;
    }

    std::string get_string() const
    {
        if (m_year == -1)
            return "";

        std::string res;

        if (m_prefix != cNoPrefix)
        {
            res = g_date_prefix_strings[m_prefix];
            res += " ";
        }

        if (m_month == -1)
        {
            assert(m_day == -1);

            char buf[256];
            sprintf_s(buf, "%u", m_year);
            res += buf;
        }
        else if (m_day == -1)
        {
            assert(m_month >= 1 && m_month <= 12);

            char buf[256];
            sprintf_s(buf, "%u/%u", m_month, m_year);
            res += buf;
        }
        else
        {
            assert(m_month >= 1 && m_month <= 12);

            char buf[256];
            sprintf_s(buf, "%u/%u/%u", m_month, m_day, m_year);
            res += buf;
        }

        if (m_plural)
            res += "'s";

        if (m_fuzzy)
            res += "?";
        
        if (m_approx)
            res += " (approximate)";
        
        if (m_estimated)
            res += " (estimated)";

        return res;
    }

    // Parses basic dates (not ranges). 
    // Date can end in "(approximate)", "(estimated)", "?", or "'s".
    // 2 digit dates converted to 1900+.
    // Supports year, month/year, or month/day/year.
    bool parse(const char* pStr)
    {
        clear();

        std::string temp(pStr);

        string_trim(temp);

        if (!temp.size())
            return false;

        if (isalpha(temp[0]))
        {
            uint32_t i;
            for (i = 0; i < cTotalPrefixes; i++)
                if (string_begins_with(temp, g_date_prefix_strings[i]))
                    break;
            if (i == cTotalPrefixes)
                return false;

            temp.erase(0, strlen(g_date_prefix_strings[i]));

            m_prefix = static_cast<date_prefix_t>(i);
        }

        string_trim(temp);
                
        if (!temp.size())
            return false;

        if (string_ends_in(temp, "(approximate)"))
        {
            m_approx = true;
            temp.resize(temp.size() - strlen("(approximate)"));
        }
        else if (string_ends_in(temp, "(estimated)"))
        {
            m_estimated = true;
            temp.resize(temp.size() - strlen("(estimated)"));
        }

        string_trim(temp);

        if (!temp.size())
            return false;

        if (string_ends_in(temp, "'s"))
        {
            m_plural = true;
            temp.resize(temp.size() - 2);

            string_trim(temp);
        }

        if (string_ends_in(temp, "?"))
        {
            m_fuzzy = true;
            temp.resize(temp.size() - 1);

            string_trim(temp);
        }

        if (!temp.size())
            return false;

        int num_slashes = 0;
        std::string date_strs[3];

        for (size_t i = 0; i < temp.size(); i++)
        {
            if (temp[i] == '/')
            {
                num_slashes++;
                if (num_slashes == 3)
                    return false;
            }
            else if (isdigit(temp[i]))
            {
                date_strs[num_slashes] += temp[i];
            }
            else
            {
                return false;
            }
        }
                
        if (num_slashes == 0)
        {
            m_year = atoi(date_strs[0].c_str());
        }
        else if (num_slashes == 1)
        {
            m_month = atoi(date_strs[0].c_str());
            if ((m_month < 1) || (m_month > 12))
                return false;

            m_year = atoi(date_strs[1].c_str());
        }
        else
        {
            m_month = atoi(date_strs[0].c_str());
            if ((m_month < 1) || (m_month > 12))
                return false;

            m_day = atoi(date_strs[1].c_str());
            if ((m_day < 1) || (m_day > 31))
                return false;

            m_year = atoi(date_strs[2].c_str());
        }

        if ((m_year >= 1) && (m_year <= 99))
            m_year += 1900;

        if ((m_year < 0) || (m_year > 2100))
            return false;

        return true;
    }

    // More advanced date range parsing, used for converting the Eberhart timeline.
    // Note this doesn't support "'s", "(approximate)", "(estimated)", or converting 2 digit years to 1900'.
    static bool parse_eberhart_date_range(std::string date,
        event_date& begin_date,
        event_date& end_date, event_date& alt_date,
        int required_year = -1)
    {
        begin_date.clear();
        end_date.clear();
        alt_date.clear();

        if (!date.size())
            return false;

        string_trim(date);

        // First change Unicode EN DASH (e2 80 93) to '-'
        std::string fixed_date;
        for (uint32_t i = 0; i < date.size(); i++)
        {
            if (((i + 2) < date.size()) && ((uint8_t)date[i] == 0xE2) && ((uint8_t)date[i + 1] == 0x80) && ((uint8_t)date[i + 2] == 0x93))
            {
                fixed_date.push_back('-');
                i += 2;
            }
            else
                fixed_date.push_back(date[i]);
        }
        date.swap(fixed_date);

        // Now tokenize the input
        string_vec tokens;

        const int MIN_YEAR = 100, MAX_YEAR = 2050;

        {
            bool in_token = false;
            uint32_t ofs = 0;
            std::string cur_token;
            while (ofs < date.size())
            {
                const uint8_t c = date[ofs];

                if ((c == ' ') || (c == '(') || (c == ')') || (c == ','))
                {
                    if (in_token)
                    {
                        tokens.push_back(cur_token);
                        cur_token.clear();
                        in_token = false;
                    }
                }
                else if (c == '-')
                {
                    if (in_token)
                    {
                        tokens.push_back(cur_token);
                        cur_token.clear();
                        in_token = false;
                    }

                    tokens.push_back("-");
                }
                else
                {
                    cur_token.push_back((char)c);
                    in_token = true;

                    if ((cur_token == "mid") || (cur_token == "Mid"))
                    {
                        if ((ofs + 1) < date.size())
                        {
                            // Fix "Mid-December" by slamming the upcoming dash to space.
                            if (date[ofs + 1] == '-')
                                date[ofs + 1] = ' ';
                        }
                    }
                }

                ofs++;
            }

            if (in_token)
            {
                tokens.push_back(cur_token);
                cur_token.clear();
                in_token = false;
            }
        }

        // Combine together separate tokens for "Early summer" into a single token to simplify the next loop.
        string_vec new_tokens;
        for (uint32_t i = 0; i < tokens.size(); i++)
        {
            bool processed_flag = false;

            if ( ((i + 1) < tokens.size()) && isalpha((uint8_t)tokens[i][0]) && isalpha((uint8_t)tokens[i + 1][0]) )
            {
                uint32_t a;
                for (a = cEarly; a <= cLate; a++)
                    if (string_begins_with(tokens[i], g_date_prefix_strings[a]))
                        break;
                
                uint32_t b;
                for (b = cSpring; b <= cWinter; b++)
                    if (string_begins_with(tokens[i + 1], g_date_prefix_strings[b]))
                        break;

                if ( (a <= cLate) && (b <= cWinter) )
                {
                    new_tokens.push_back(tokens[i] + " " + tokens[i + 1]);
                    
                    i++;

                    processed_flag = true;
                }
            }
            
            if (!processed_flag)
            {
                new_tokens.push_back(tokens[i]);
            }
        }

        tokens.swap(new_tokens);

        // Process each token individually, using a simple state machine.
        uint32_t cur_token = 0;
        bool inside_end = false;
        bool inside_alt = false;

        while (cur_token < tokens.size())
        {
            std::string& s = tokens[cur_token];
            if (!s.size())
                return false;

            if (s[0] == '-')
            {
                if (s.size() != 1)
                    return false;

                if (inside_alt)
                    return false;

                if (inside_end)
                    return false;

                inside_end = true;
            }
            else if (isdigit((uint8_t)s[0]))
            {
                event_date& d = inside_alt ? alt_date : (inside_end ? end_date : begin_date);

                if (string_ends_in(s, "?"))
                {
                    if (d.m_fuzzy)
                        return false;

                    d.m_fuzzy = true;

                    s.pop_back();
                }

                int val = atoi(s.c_str());
                if ((!val) || (val == INT_MIN) || (val == INT_MAX))
                    return false;

                char buf[32];
                sprintf_s(buf, "%i", val);
                if (strlen(buf) != s.size())
                    return false;

                if (d.m_month != -1)
                {
                    if ((d.m_day != -1) || (d.m_prefix != cNoPrefix) || (val >= MIN_YEAR))
                    {
                        if (d.m_year != -1)
                            return false;
                        if ((val < MIN_YEAR) || (val > MAX_YEAR))
                            return false;

                        if ((!inside_end) && (!inside_alt) && (required_year != -1))
                        {
                            if (val != required_year)
                                return false;
                        }

                        d.m_year = val;
                    }
                    else
                    {
                        if ((val < 1) || (val > 31))
                            return false;

                        if (d.m_day != -1)
                            return false;

                        d.m_day = val;
                    }
                }
                else
                {
                    if ((inside_end || inside_alt) && (begin_date.m_month != -1) && (val <= 31))
                    {
                        // Handle cases like "January 20-25" or "January 20 or 25".

                        if (d.m_month != -1)
                            return false;

                        d.m_month = begin_date.m_month;
                        
                        if ((val < 1) || (val > 31))
                            return false;

                        if (d.m_day != -1)
                            return false;
                        d.m_day = val;
                    }
                    else
                    {
                        if (d.m_year != -1)
                            return false;
                        if ((val < MIN_YEAR) || (val > MAX_YEAR))
                            return false;

                        if ((!inside_end) && (!inside_alt) && (required_year != -1))
                        {
                            if (val != required_year)
                                return false;
                        }

                        d.m_year = val;
                    }
                }
            }
            else if (isalpha((uint8_t)s[0]))
            {
                if (string_begins_with(s, "or"))
                {
                    if (s.size() != 2)
                        return false;

                    if (inside_alt)
                        return false;

                    inside_alt = true;
                    inside_end = false;
                }
                else
                {
                    event_date& d = inside_alt ? alt_date : (inside_end ? end_date : begin_date);

                    if (string_ends_in(s, "?"))
                    {
                        if (d.m_fuzzy)
                            return false;

                        d.m_fuzzy = true;

                        s.pop_back();
                    }

                    int month = determine_month(s);
                    if (month >= 0)
                    {
                        assert(month <= 11);

                        if (s.size() != strlen(g_full_months[month]))
                            return false;

                        if (d.m_month != -1)
                            return false;

                        d.m_month = month + 1;
                    }
                    else
                    {
                        uint32_t i;
                        for (i = 0; i < cTotalPrefixes; i++)
                            if (string_begins_with(s, g_date_prefix_strings[i]))
                                break;
                        if (i == cTotalPrefixes)
                            return false;

                        if (s.size() != strlen(g_date_prefix_strings[i]))
                            return false;

                        if (d.m_prefix != -1)
                            return false;

                        d.m_prefix = static_cast<date_prefix_t>(i);
                    }
                }
            }
            else
            {
                return false;
            }

            cur_token++;
        }

        // If there's no begin date, then error
        if ((begin_date.m_year == -1) && (begin_date.m_month == -1) && (begin_date.m_prefix == cNoPrefix))
            return false;

        // Check for no begin year specified
        if (begin_date.m_year == -1)
            begin_date.m_year = required_year;

        // Check for no end year specified
        if ((end_date.m_year == -1) && ((end_date.m_prefix != cNoPrefix) || (end_date.m_month != -1)))
            end_date.m_year = required_year;

        // Check for no alt year specified
        if ((inside_alt) && (alt_date.m_year == -1) && ((alt_date.m_prefix != cNoPrefix) || (alt_date.m_month != -1)))
            alt_date.m_year = required_year;

        if (!check_date_prefix(begin_date))
            return false;
        if (!check_date_prefix(end_date))
            return false;
        if (!check_date_prefix(alt_date))
            return false;

        return true;
    }

    // Note the returned date may be invalid. It's only intended for sorting/comparison purposes against other sort dates.
    void get_sort_date(int& year, int& month, int& day) const
    {
        year = m_year;
        month = 0;
        day = 0;
                
        if (m_month == -1)
        {
            // All we have is a year, no month or date supplied.
            if (m_plural)
            {
                const bool is_century = (m_year % 100) == 0;
                const bool is_decade = (m_year % 10) == 0;

                // 1900's, 1910's, 1800's etc.
                if (m_prefix == cEarly)
                {
                    if (is_century)
                        year += 10;
                    else if (is_decade)
                        year += 1;
                }
                else if (m_prefix == cMiddleOf)
                {
                    if (is_century)
                        year += 50;
                    else if (is_decade)
                        year += 5;
                }
                else if (m_prefix == cLate)
                {
                    if (is_century)
                        year += 80;
                    else if (is_decade)
                        year += 8;
                }
                else if (m_prefix == cEndOf)
                {
                    if (is_century)
                        year += 90;
                    else if (is_decade)
                        year += 9;
                }
                else
                {
                    // 1980's goes before 1980
                    day = -1;
                }
            }
            else
            {
                // 1900, 1910, 1800 etc.
                switch (m_prefix)
                {
                    case cEarlySpring:
                    {
                        month = 3;
                        day = 1;
                        break;
                    }
                    case cEarlySummer:
                    {
                        month = 6;
                        day = 1;
                        break;
                    }
                    case cEarlyAutumn:
                    {
                        month = 9;
                        day = 1;
                        break;
                    }
                    case cEarlyFall:
                    {
                        month = 9;
                        day = 1;
                        break;
                    }
                    case cEarlyWinter:
                    {
                        month = 12;
                        day = 1;
                        break;
                    }
                    case cMidSpring:
                    {
                        month = 4;
                        day = 1;
                        break;
                    }
                    case cMidSummer:
                    {
                        month = 7;
                        day = 1;
                        break;
                    }
                    case cMidAutumn:
                    {
                        month = 10;
                        day = 1;
                        break;
                    }
                    case cMidFall:
                    {
                        month = 10;
                        day = 1;
                        break;
                    }
                    case cMidWinter:
                    {
                        month = 1;
                        day = 1;
                        break;
                    }
                    case cLateSpring:
                    {
                        month = 5;
                        day = 15;
                        break;
                    }
                    case cLateSummer:
                    {
                        month = 8;
                        day = 15;
                        break;
                    }
                    case cLateAutumn:
                    {
                        month = 11;
                        day = 15;
                        break;
                    }
                    case cLateFall:
                    {
                        month = 11;
                        day = 15;
                        break;
                    }
                    case cLateWinter:
                    {
                        month = 2;
                        day = 15;
                        break;
                    }
                    case cEarly:
                    {
                        day = 1;
                        break;
                    }
                    case cSpring:
                    {
                        month = 3;
                        day = 20;
                        break;
                    }
                    case cSummer:
                    {
                        month = 6;
                        day = 21;
                        break;
                    }
                    case cMiddleOf:
                    {
                        month = 6;
                        day = 15;
                        break;
                    }
                    case cAutumn:
                    {
                        month = 9;
                        day = 23;
                        break;
                    }
                    case cFall:
                    {
                        month = 9;
                        day = 23;
                        break;
                    }
                    case cLate:
                    {
                        month = 10;
                        day = 15;
                        break;
                    }
                    case cWinter:
                    {
                        month = 12;
                        day = 21;
                        break;
                    }
                    case cEndOf:
                    {
                        month = 12;
                        day = 1;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        else if (m_day == -1)
        {
            // We have a year and a month, but no date
            month = m_month;

            // 1/1900, 4/1910, 12/1805 etc.
            switch (m_prefix)
            {
                case cEarly:
                {
                    day = 2;
                    break;
                }
                case cMiddleOf:
                {
                    day = 15;
                    break;
                }
                case cLate:
                {
                    day = 25;
                    break;
                }
                case cEndOf:
                {
                    day = 28;
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            month = m_month;
            day = m_day;
        }
    }

    // Compares two timeline dates. true if lhs < rhs
    static bool compare(const event_date& lhs, const event_date& rhs)
    {
        int lhs_year, lhs_month, lhs_day;
        lhs.get_sort_date(lhs_year, lhs_month, lhs_day);

        int rhs_year, rhs_month, rhs_day;
        rhs.get_sort_date(rhs_year, rhs_month, rhs_day);

        if (lhs_year < rhs_year)
            return true;
        else if (lhs_year == rhs_year)
        {
            if (lhs_month < rhs_month)
                return true;
            else if (lhs_month == rhs_month)
            {
                if (lhs_day < rhs_day)
                    return true;
            }
        }

        return false;
    }

 private:
         
    static bool check_date_prefix(const event_date& date)
    {
        // Prefix sanity checks
        if (date.m_prefix != cNoPrefix)
        {
            // Can't specify a specific day if you've given us a prefix.
            if (date.m_day != -1)
                return false;

            if (is_season(date.m_prefix))
            {
                // Can't specify an explicit month if you've given us a season.
                if (date.m_month != -1)
                    return false;
            }
        }
        return true;
    }
};

struct timeline_event
{
    std::string m_date_str;
    std::string m_time_str;

    std::string m_alt_date_str;

    std::string m_end_date_str;
        
    event_date m_begin_date;
    event_date m_end_date;
    event_date m_alt_date;

    std::string m_desc;
    string_vec m_type;
    string_vec m_refs;
    string_vec m_locations;
    string_vec m_attributes;
    string_vec m_see_also;

    std::string m_rocket_type;
    std::string m_rocket_altitude;
    std::string m_rocket_range;

    std::string m_atomic_type;
    std::string m_atomic_kt;
    std::string m_atomic_mt;

    std::string m_source_id;

    std::string m_source;

    bool operator==(const timeline_event& rhs) const
    {
#define COMP(X) if (X != rhs.X) return false;
        COMP(m_date_str);
        COMP(m_time_str);
        COMP(m_alt_date_str);
        COMP(m_end_date_str);
        COMP(m_begin_date);
        COMP(m_end_date);
        COMP(m_alt_date);
        COMP(m_desc);
        COMP(m_type);
        COMP(m_refs);
        COMP(m_locations);
        COMP(m_attributes);
        COMP(m_see_also);
        COMP(m_rocket_type);
        COMP(m_rocket_altitude);
        COMP(m_rocket_range);
        COMP(m_atomic_type);
        COMP(m_atomic_kt);
        COMP(m_atomic_mt);
        COMP(m_source_id);
        COMP(m_source);
#undef COMP
        return true;
    }

    bool operator!=(const timeline_event& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator< (const timeline_event& rhs) const
    {
        return event_date::compare(m_begin_date, rhs.m_begin_date);
    }

    void print(FILE *pFile) const
    {
        fprintf(pFile, "Date: %s  \n", m_date_str.c_str());
        if (m_alt_date_str.size())
            fprintf(pFile, "Alternate date: %s  \n", m_alt_date_str.c_str());

        if (m_time_str.size())
            fprintf(pFile, "Time: %s  \n", m_time_str.c_str());

        if (m_end_date_str.size())
            fprintf(pFile, "End date: %s  \n", m_end_date_str.c_str());

        fprintf(pFile, "Description: %s  \n", m_desc.c_str());

        for (uint32_t i = 0; i < m_type.size(); i++)
            fprintf(pFile, "Type: %s  \n", m_type[i].c_str());

        for (uint32_t i = 0; i < m_refs.size(); i++)
            fprintf(pFile, "Reference: %s  \n", m_refs[i].c_str());

        for (uint32_t i = 0; i < m_locations.size(); i++)
            fprintf(pFile, "Location: %s  \n", m_locations[i].c_str());

        for (uint32_t i = 0; i < m_attributes.size(); i++)
            fprintf(pFile, "Attributes: %s  \n", m_attributes[i].c_str());

        for (uint32_t i = 0; i < m_see_also.size(); i++)
            fprintf(pFile, "See also: %s  \n", m_see_also[i].c_str());

        if (m_rocket_type.size())
            fprintf(pFile, "Rocket type: %s  \n", m_rocket_type.c_str());
        if (m_rocket_altitude.size())
            fprintf(pFile, "Rocket altitude: %s  \n", m_rocket_altitude.c_str());
        if (m_rocket_range.size())
            fprintf(pFile, "Rocket range: %s  \n", m_rocket_range.c_str());

        if (m_atomic_type.size())
            fprintf(pFile, "Atomic type: %s  \n", m_atomic_type.c_str());
        if (m_atomic_kt.size())
            fprintf(pFile, "Atomic KT: %s  \n", m_atomic_kt.c_str());
        if (m_atomic_mt.size())
            fprintf(pFile, "Atomic MT: %s  \n", m_atomic_mt.c_str());

        if (m_source_id.size())
            fprintf(pFile, "Source ID: %s  \n", m_source_id.c_str());

        if (m_source.size())
            fprintf(pFile, "Source: %s  \n", m_source.c_str());
    }

    void from_json(const json& obj, const char *pSource_override)
    {
        auto date = obj.find("date");
        auto alt_date = obj.find("alt_date");
        auto time = obj.find("time");
        auto end_date = obj.find("end_date");
        auto desc = obj.find("desc");
        auto location = obj.find("location");
        auto ref = obj.find("ref");
        auto type = obj.find("type");
        auto attributes = obj.find("attributes");
        auto atomic_kt = obj.find("atomic_kt");
        auto atomic_mt = obj.find("atomic_mt");
        auto atomic_type = obj.find("atomic_type");
        auto rocket_type = obj.find("rocket_type");
        auto see_also = obj.find("see_also");
        auto rocket_altitude = obj.find("rocket_altitude");
        auto rocket_range = obj.find("rocket_range");
        auto source_id = obj.find("source_id");
        auto source = obj.find("source");

        if (desc == obj.end())
            panic("Missing desc");

        if (!desc->is_string())
            panic("Invalid desc");

        if ((date == obj.end()) || (!date->is_string()))
            panic("Missing date");

        if (type != obj.end() && !type->is_string() && !type->is_array())
            panic("Invalid type");

        if (end_date != obj.end() && !end_date->is_string())
            panic("Invalid end_date");

        if (alt_date != obj.end() && !alt_date->is_string())
            panic("Invalid alt_date");

        if (time != obj.end() && !time->is_string())
            panic("Invalid time");
                
        m_date_str = (*date);
        if (!m_begin_date.parse(m_date_str.c_str()))
            panic("Failed parsing date %s\n", m_date_str.c_str());

        if (end_date != obj.end())
        {
            m_end_date_str = (*end_date);

            if (!m_end_date.parse(m_end_date_str.c_str()))
                panic("Failed parsing end date %s\n", m_end_date_str.c_str());
        }

        if (alt_date != obj.end())
        {
            m_alt_date_str = (*alt_date);

            if (!m_alt_date.parse(m_alt_date_str.c_str()))
                panic("Failed parsing alt date %s\n", m_alt_date_str.c_str());
        }

        if (time != obj.end())
            m_time_str = (*time);

        if (type != obj.end())
        {
            if (type->is_string())
            {
                m_type.resize(1);
                m_type[0] = (*type);
            }
            else
            {
                m_type.resize(type->size());
                for (uint32_t j = 0; j < type->size(); j++)
                {
                    if (!(*type)[j].is_string())
                        panic("Invalid type");

                    m_type[j] = (*type)[j];
                }
            }
        }

        m_desc = (*desc);

        if (source_id != obj.end())
        {
            if (!source_id->is_string())
                panic("Invalid source ID");

            m_source_id = (*source_id);
        }

        if (location != obj.end())
        {
            if (location->is_array())
            {
                for (uint32_t j = 0; j < location->size(); j++)
                {
                    const auto& loc_arr_entry = (*location)[j];

                    if (!loc_arr_entry.is_string())
                        panic("Invalid location");

                    m_locations.push_back(loc_arr_entry);
                }
            }
            else
            {
                if (!location->is_string())
                    panic("Invalid location");

                m_locations.push_back((*location));
            }
        }

        if (ref != obj.end())
        {
            if (ref->is_array())
            {
                for (uint32_t j = 0; j < ref->size(); j++)
                {
                    const auto& ref_arr_entry = (*ref)[j];

                    if (!ref_arr_entry.is_string())
                        panic("Invalid ref");

                    m_refs.push_back(ref_arr_entry);
                }
            }
            else
            {
                if (!ref->is_string())
                    panic("Invalid ref");

                m_refs.push_back((*ref));
            }
        }

        if (atomic_kt != obj.end())
        {
            if (!atomic_kt->is_string())
                panic("Invalid atomic_kt");
            m_atomic_kt = (*atomic_kt);
        }

        if (atomic_mt != obj.end())
        {
            if (!atomic_mt->is_string())
                panic("Invalid atomic_mt");
            m_atomic_mt = (*atomic_mt);
        }

        if (atomic_type != obj.end())
        {
            if (!atomic_type->is_string())
                panic("Invalid atomic_type");
            m_atomic_type = (*atomic_type);
        }

        if (rocket_type != obj.end())
        {
            if (!rocket_type->is_string())
                panic("Invalid rocket_type");
            m_rocket_type = (*rocket_type);
        }

        if (rocket_altitude != obj.end())
        {
            if (!rocket_altitude->is_string())
                panic("Invalid rocket_altitude");
            m_rocket_altitude = (*rocket_altitude);
        }

        if (rocket_range != obj.end())
        {
            if (!rocket_range->is_string())
                panic("Invalid rocket_range");
            m_rocket_range = (*rocket_range);
        }

        if (see_also != obj.end())
        {
            if (see_also->is_array())
            {
                for (uint32_t j = 0; j < see_also->size(); j++)
                {
                    const auto& see_also_array_entry = (*see_also)[j];

                    if (!see_also_array_entry.is_string())
                        panic("Invalid see_also");

                    m_see_also.push_back(see_also_array_entry);
                }
            }
            else
            {
                if (!see_also->is_string())
                    panic("Invalid see_also");

                m_see_also.push_back((*see_also));
            }
        }

        if (attributes != obj.end())
        {
            if (attributes->is_array())
            {
                for (uint32_t j = 0; j < attributes->size(); j++)
                {
                    const auto& attr_entry = (*attributes)[j];

                    if (!attr_entry.is_string())
                        panic("Invalid attributes");

                    m_attributes.push_back(attr_entry);
                }
            }
            else
            {
                if (!attributes->is_string())
                    panic("Invalid attributes");

                m_attributes.push_back(*attributes);
            }
        }

        if (pSource_override)
            m_source = pSource_override;
        else if (source != obj.end())
        {
            if (!source->is_string())
                panic("Invalid source");

            m_source = (*source);
        }
    }

    void to_json(json& j) const
    {
        j = json::object();

        j["date"] = m_date_str;
        j["desc"] = m_desc;
                
        if (m_alt_date_str.size())
            j["alt_date"] = m_alt_date_str;

        if (m_end_date_str.size())
            j["end_date"] = m_end_date_str;

        if (m_time_str.size())
            j["time"] = m_time_str;
                
        if (m_type.size() > 1)
            j["type"] = m_type;
        else if (m_type.size())
            j["type"] = m_type[0];
            
        if (m_refs.size() > 1)
            j["ref"] = m_refs;
        else if (m_refs.size())
            j["ref"] = m_refs[0];

        if (m_locations.size() > 1)
            j["location"] = m_locations;
        else if (m_locations.size())
            j["location"] = m_locations[0];

        if (m_attributes.size() > 1)
            j["attributes"] = m_attributes;
        else if (m_attributes.size())
            j["attributes"] = m_attributes[0];

        if (m_see_also.size() > 1)
            j["see_also"] = m_see_also;
        else if (m_see_also.size())
            j["see_also"] = m_see_also[0];

        if (m_rocket_type.size())
            j["rocket_type"] = m_rocket_type;
        if (m_rocket_altitude.size())
            j["rocket_altitude"] = m_rocket_altitude;
        if (m_rocket_range.size())
            j["rocket_range"] = m_rocket_range;

        if (m_atomic_type.size())
            j["atomic_type"] = m_atomic_type;
        if (m_atomic_kt.size())
            j["atomic_kt"] = m_atomic_kt;
        if (m_atomic_mt.size())
            j["atomic_mt"] = m_atomic_mt;

        if (m_source_id.size())
            j["source_id"] = m_source_id;
        
        if (m_source.size())
            j["source"] = m_source;
    }
};

typedef std::vector<timeline_event> timeline_event_vec;

class ufo_timeline
{
public:
    ufo_timeline() :
        m_name("Unnamed Timeline")
    {
    }

    size_t size() const { return m_events.size(); }

    timeline_event& operator[] (size_t i) { return m_events[i]; }
    const timeline_event& operator[] (size_t i) const { return m_events[i]; }

    const std::string& get_name() const { return m_name; }
    void set_name(const std::string& str) { m_name = str; }

    const timeline_event_vec& get_events() const { return m_events; }
    timeline_event_vec& get_events() { return m_events; }

    void sort()
    {
        std::sort(m_events.begin(), m_events.end());
    }

    void create_json(json& j) const
    {
        j = json::object();

        const char* pTimeline_name = m_name.c_str();

        j[pTimeline_name] = json::array();

        auto& ar = j[pTimeline_name];

        for (size_t i = 0; i < m_events.size(); i++)
        {
            json obj;
            m_events[i].to_json(obj);

            ar.push_back(obj);
        }
    }

    bool write_file(const char* pFilename, bool utf8_bom = true)
    {
        json j;
        create_json(j);

        return serialize_to_json_file(pFilename, j, utf8_bom);
    }
        
    bool load_json(const char* pFilename, bool& utf8_flag, const char* pSource_override = nullptr);
    
    bool write_markdown(const char* pTimeline_filename);

private:
    timeline_event_vec m_events;
    std::string m_name;

    bool load_json1(const char* pFilename, bool& utf8_flag, const char* pSource_override = nullptr);
    bool load_json2(const char* pFilename, bool& utf8_flag, const char* pSource_override = nullptr);
};

bool ufo_timeline::write_markdown(const char *pTimeline_filename)
{
    const std::vector<timeline_event>& timeline_events = m_events;

    FILE* pTimeline_file = ufopen(pTimeline_filename, "w");
    if (!pTimeline_file)
        panic("Failed creating file %s", pTimeline_file);

    fputc(UTF8_BOM0, pTimeline_file);
    fputc(UTF8_BOM1, pTimeline_file);
    fputc(UTF8_BOM2, pTimeline_file);
    fprintf(pTimeline_file, "<meta charset=\"utf-8\">\n");

    std::map<int, int> year_histogram;
        
    fprintf(pTimeline_file, "# <a name=\"Top\">UFO Event Timeline v" TIMELINE_VERSION " - Compiled 2/9/2023</a>\n\n");

    fputs(
u8R"(An automated compilation by <a href="https://twitter.com/richgel999">Richard Geldreich, Jr.</a> using public data from <a href="https://en.wikipedia.org/wiki/Jacques_Vall%C3%A9e">Dr. Jacques Vallée</a>,
<a href="https://www.academia.edu/9813787/GOVERNMENT_INVOLVEMENT_IN_THE_UFO_COVER_UP_CHRONOLOGY_based">Pea Research</a>, <a href="http://www.cufos.org/UFO_Timeline.html">George M. Eberhart</a>,
<a href="https://en.wikipedia.org/wiki/Richard_H._Hall">Richard H. Hall</a>, <a href="https://web.archive.org/web/20160821221627/http://www.ufoinfo.com/onthisday/sametimenextyear.html">Dr. Donald A. Johnson</a>,
<a href="https://medium.com/@richgel99/1958-keziah-poster-recreation-completed-82fdb55750d8">Fred Keziah</a>, <a href="https://github.com/richgel999/uap_resources/blob/main/bluebook_uncensored_unknowns_don_berliner.pdf">Don Berliner</a>, and [NICAP](https://www.nicap.org/).

## Copyrights: 
- Richard Geldreich, Jr. - Copyright (c) 2023 (all parsed dates and events marked \"maj2\" unless otherwise attributed)
- Dr. Jacques F. Vallée - Copyright (c) 1993
- LeRoy Pea - Copyright (c) 9/8/1988 (updated 3/17/2005)
- George M. Eberhart - Copyright (c) 2022
- Dr. Donald A. Johnson - Copyright (c) 2012
- Fred Keziah - Copyright (c) 1958

## Source Code:
This website is created automatically using a [C++](https://en.wikipedia.org/wiki/C%2B%2B) command line tool called “ufojson”. It parses the raw text and [Markdown](https://en.wikipedia.org/wiki/Markdown) source data to [JSON format](https://www.json.org/json-en.html), which is then converted to a single large web page using [pandoc](https://pandoc.org/). This tool's source code and all of the raw source and JSON data is located [here on github](https://github.com/richgel999/ufo_data).)", pTimeline_file);
                
    fprintf(pTimeline_file, "\n\n## Table of Contents\n\n");

    fprintf(pTimeline_file, "<a href = \"timeline.html#yearhisto\">Year Histogram</a>\n\n");

    std::set<uint32_t> year_set;
    int min_year = 9999, max_year = -10000;
    for (uint32_t i = 0; i < timeline_events.size(); i++)
    {
        int y;
        year_set.insert(y = timeline_events[i].m_begin_date.m_year);
        min_year = std::min(min_year, y);
        max_year = std::max(max_year, y);
    }

    fprintf(pTimeline_file, "### Years\n");

    uint32_t n = 0;
    for (int y = min_year; y <= max_year; y++)
    {
        if (year_set.find(y) != year_set.end())
        {
            fprintf(pTimeline_file, "<a href=\"timeline.html#year%i\">%i</a> ", y, y);

            n++;
            if (n == 10)
            {
                fprintf(pTimeline_file, "  \n");
                n = 0;
            }
        }
    }
    fprintf(pTimeline_file, "  \n\n");

    fprintf(pTimeline_file, "## Event Timeline\n");

    int cur_year = -9999;
        
    for (uint32_t i = 0; i < timeline_events.size(); i++)
    {
        uint32_t hash = crc32((const uint8_t*)timeline_events[i].m_desc.c_str(), timeline_events[i].m_desc.size());
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_year, sizeof(timeline_events[i].m_begin_date.m_year), hash);
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_month, sizeof(timeline_events[i].m_begin_date.m_month), hash);
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_day, sizeof(timeline_events[i].m_begin_date.m_day), hash);

        int year = timeline_events[i].m_begin_date.m_year;
        if ((year != cur_year) && (year > cur_year))
        {
            fprintf(pTimeline_file, "\n---\n\n");
            fprintf(pTimeline_file, "## <a name=\"year%u\">Year: %u</a> <a href=\"timeline.html#Top\">(Back to Top)</a>\n\n", year, year);

            fprintf(pTimeline_file, "### <a name=\"%08X\">Event %i (%08X)</a>\n", hash, i, hash);

            cur_year = year;
        }
        else
        {
            fprintf(pTimeline_file, "### <a name=\"%08X\"></a> Event %i (%08X)\n", hash, i, hash);
        }

        timeline_events[i].print(pTimeline_file);

        year_histogram[year] = year_histogram[year] + 1;

        fprintf(pTimeline_file, "\n");
    }

    fprintf(pTimeline_file, "\n---\n\n");

    fprintf(pTimeline_file, "#  <a name=\"yearhisto\">Year Histogram</a>\n");

    for (auto it = year_histogram.begin(); it != year_histogram.end(); ++it)
    {
        fprintf(pTimeline_file, "* %i, %i\n", it->first, it->second);
    }

    fclose(pTimeline_file);
    
    return true;
}

// Load a JSON timeline using pjson
bool ufo_timeline::load_json1(const char* pFilename, bool &utf8_flag, const char *pSource_override)
{
    std::vector<timeline_event>& timeline_events = m_events;

    utf8_flag = false;

    std::vector<uint8_t> buf;
    if (!read_text_file(pFilename, buf, utf8_flag))
        return false;

    pjson::document doc;
    bool status = doc.deserialize_in_place((char*)&buf[0]);
    if (!status)
    {
        ufprintf(stderr, "Failed parsing JSON document!\n");
        return false;
    }
        
    const auto& root_obj = doc.get_object()[0];

    const char* pName = root_obj.get_key().get_ptr();
    
    m_name = pName;

    const auto& root_arr = root_obj.get_value();
    if (!root_arr.is_array())
    {
        ufprintf(stderr, "Invalid JSON\n");
        return false;
    }

    for (uint32_t i = 0; i < root_arr.size(); i++)
    {
        const auto& arr_entry = root_arr[i];
        if (!arr_entry.is_object())
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        const auto pDate = arr_entry.find_value_variant("date");
        const auto pAlt_date = arr_entry.find_value_variant("alt_date");
        const auto pTime = arr_entry.find_value_variant("time");
        const auto pEnd_date = arr_entry.find_value_variant("end_date");
        const auto pDesc = arr_entry.find_value_variant("desc");
        const auto pLocation = arr_entry.find_value_variant("location");
        const auto pRef = arr_entry.find_value_variant("ref");
        const auto pType = arr_entry.find_value_variant("type");
        const auto pAttributes = arr_entry.find_value_variant("attributes");
        const auto pAtomic_kt = arr_entry.find_value_variant("atomic_kt");
        const auto pAtomic_mt = arr_entry.find_value_variant("atomic_mt");
        const auto pAtomic_type = arr_entry.find_value_variant("atomic_type");
        const auto pRocket_type = arr_entry.find_value_variant("rocket_type");
        const auto pSee_also = arr_entry.find_value_variant("see_also");
        const auto pRocket_altitude = arr_entry.find_value_variant("rocket_altitude");
        const auto pRocket_range = arr_entry.find_value_variant("rocket_range");
        const auto pSource_ID = arr_entry.find_value_variant("source_id");
        const auto pSource = arr_entry.find_value_variant("source");

        if ((!pDate) || (!pDesc))
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        if ((!pDate->is_string()) || (!pDesc->is_string()))
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        if (pType && !pType->is_string() && !pType->is_array())
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        if ((pEnd_date) && (!pEnd_date->is_string()))
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        if ((pAlt_date) && (!pAlt_date->is_string()))
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        if ((pTime) && (!pTime->is_string()))
        {
            ufprintf(stderr, "Invalid JSON\n");
            return false;
        }

        timeline_events.resize(timeline_events.size() + 1);

        timeline_event& new_event = timeline_events.back();

        new_event.m_date_str = pDate->as_string();
        if (!new_event.m_begin_date.parse(pDate->as_string_ptr()))
        {
            ufprintf(stderr, "Failed parsing date %s\n", pDate->as_string_ptr());
            return false;
        }

        if (pEnd_date)
        {
            new_event.m_end_date_str = pEnd_date->as_string();
            if (!new_event.m_end_date.parse(pEnd_date->as_string_ptr()))
            {
                ufprintf(stderr, "Failed parsing end date %s\n", pEnd_date->as_string_ptr());
                return false;
            }
        }

        if (pAlt_date)
        {
            new_event.m_alt_date_str = pAlt_date->as_string();
            if (!new_event.m_alt_date.parse(pAlt_date->as_string_ptr()))
            {
                ufprintf(stderr, "Failed parsing alt date %s\n", pAlt_date->as_string_ptr());
                return false;
            }
        }

        if (pTime)
        {
            new_event.m_time_str = pTime->as_string();
        }

        if (pType)
        {
            if (pType->is_string())
            {
                new_event.m_type.resize(1);
                new_event.m_type[0] = pType->as_string();
            }
            else
            {
                new_event.m_type.resize(pType->size());
                for (uint32_t j = 0; j < pType->size(); j++)
                {
                    new_event.m_type[j] = (*pType)[j].as_string();
                }
            }
        }

        new_event.m_desc = pDesc->as_string();

        if (pSource_ID)
            new_event.m_source_id = pSource_ID->as_string();

        if (pLocation)
        {
            if (pLocation->is_array())
            {
                for (uint32_t j = 0; j < pLocation->size(); j++)
                {
                    const auto& loc_arr_entry = (*pLocation)[j];

                    if (!loc_arr_entry.is_string())
                    {
                        ufprintf(stderr, "Invalid JSON\n");
                        return false;
                    }
                                        
                    new_event.m_locations.push_back(loc_arr_entry.as_string_ptr());
                }
            }
            else
            {
                if (!pLocation->is_string())
                {
                    ufprintf(stderr, "Invalid JSON\n");
                    return false;
                }
                                
                new_event.m_locations.push_back(pLocation->as_string());
            }
        }

        if (pRef)
        {
            if (pRef->is_array())
            {
                for (uint32_t j = 0; j < pRef->size(); j++)
                {
                    const auto& ref_arr_entry = (*pRef)[j];

                    if (!ref_arr_entry.is_string())
                    {
                        ufprintf(stderr, "Invalid JSON\n");
                        return false;
                    }
                                        
                    new_event.m_refs.push_back(ref_arr_entry.as_string());
                }
            }
            else
            {
                if (!pRef->is_string())
                {
                    ufprintf(stderr, "Invalid JSON\n");
                    return false;
                }
                                
                new_event.m_refs.push_back(pRef->as_string());
            }
        }

        if (pAtomic_kt)
        {
            if (!pAtomic_kt->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_atomic_kt = pAtomic_kt->as_string();
        }

        if (pAtomic_mt)
        {
            if (!pAtomic_mt->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_atomic_mt = pAtomic_mt->as_string();
        }

        if (pAtomic_type)
        {
            if (!pAtomic_type->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_atomic_type = pAtomic_type->as_string();
        }

        if (pRocket_type)
        {
            if (!pRocket_type->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_rocket_type = pRocket_type->as_string();
        }

        if (pRocket_altitude)
        {
            if (!pRocket_altitude->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_rocket_altitude = pRocket_altitude->as_string();
        }

        if (pRocket_range)
        {
            if (!pRocket_range->is_string())
            {
                ufprintf(stderr, "Invalid JSON\n");
                return false;
            }
            
            new_event.m_rocket_range = pRocket_range->as_string();
        }

        if (pSee_also)
        {
            if (pSee_also->is_array())
            {
                for (uint32_t j = 0; j < pSee_also->size(); j++)
                {
                    const auto& see_also_array = (*pSee_also)[j];

                    if (!see_also_array.is_string())
                    {
                        ufprintf(stderr, "Invalid JSON\n");
                        return false;
                    }
                                        
                    new_event.m_see_also.push_back(see_also_array.as_string());
                }
            }
            else
            {
                if (!pSee_also->is_string())
                {
                    ufprintf(stderr, "Invalid JSON\n");
                    return false;
                }
                                
                new_event.m_see_also.push_back(pSee_also->as_string());
            }
        }

        if (pAttributes)
        {
            if (pAttributes->is_array())
            {
                for (uint32_t j = 0; j < pAttributes->size(); j++)
                {
                    const auto& attr_entry = (*pAttributes)[j];

                    if (!attr_entry.is_string())
                    {
                        ufprintf(stderr, "Invalid JSON\n");
                        return false;
                    }
                                        
                    new_event.m_attributes.push_back(attr_entry.as_string());
                }
            }
            else
            {
                if (!pAttributes->is_string())
                {
                    ufprintf(stderr, "Invalid JSON\n");
                    return false;
                }
                                
                new_event.m_attributes.push_back(pAttributes->as_string());
            }
        }

        if (pSource_override)
            new_event.m_source = pSource_override;
        else if (pSource)
            new_event.m_source = pSource->as_string();
    }

    return true;
}

// Load a JSON timeline using nlohmann::json
bool ufo_timeline::load_json2(const char* pFilename, bool& utf8_flag, const char* pSource_override)
{
    std::vector<timeline_event>& timeline_events = m_events;

    std::vector<uint8_t> buf;

    if (!read_text_file(pFilename, buf, utf8_flag))
        return false;

    if (!buf.size())
        panic("Input file is empty");

    json js;
    
    bool success = false;
    try
    {
        js = json::parse(buf.begin(), buf.end());
        success = true;
    }
    catch (json::exception& e)
    {
        panic("json::parse() failed (id %i): %s", e.id, e.what());
    }

    if (!js.is_object() || !js.size())
        panic("Invalid JSON");

    auto first_entry = js.begin();

    if (!first_entry->is_array())
        panic("Invalid JSON");

    m_name = first_entry.key();
    
    const size_t first_event_index = timeline_events.size();
    timeline_events.resize(first_event_index + first_entry->size());

    for (uint32_t i = 0; i < first_entry->size(); i++)
    {
        auto obj = (*first_entry)[i];

        if (!obj.is_object())
            panic("Invalid JSON");
        
        timeline_events[first_event_index + i].from_json(obj, pSource_override);
    }
    
    return true;
}

bool ufo_timeline::load_json(const char* pFilename, bool& utf8_flag, const char* pSource_override)
{
    std::vector<timeline_event>& timeline_events = m_events;

    const size_t timeline_events_size = timeline_events.size();

    if (!load_json2(pFilename, utf8_flag, pSource_override))
        return false;

    ufo_timeline alt;
    bool utf8_flag_alt;
    if (!alt.load_json1(pFilename, utf8_flag_alt, pSource_override))
        return false;

    if (utf8_flag != utf8_flag_alt)
        panic("Failed loading timeline events JSON");

    if ((timeline_events.size() - timeline_events_size) != alt.size())
        panic("Failed loading timeline events JSON");
    
    for (size_t i = timeline_events_size; i < timeline_events.size(); i++)
        if (timeline_events[i] != alt[i - timeline_events_size])
            panic("Failed loading timeline events JSON");
    
    return true;
}

// Escapes quoted strings.
static std::string escape_string_for_json(const std::string& str)
{
    std::string new_str;
    for (uint32_t i = 0; i < str.size(); i++)
    {
        char c = str[i];
        if (c == '"')
            new_str.push_back('\\');
        else if (c == '\\')
            new_str.push_back('\\');
        else if (c == '\n')
        {
            new_str.push_back('\\');
            new_str.push_back('n');
            continue;
        }

        new_str.push_back(c);
    }

    return new_str;
}

static bool convert_magnonia(const char *pSrc_filename, const char *pDst_filename, const char *pSource_override = nullptr, const char *pRef_override = nullptr)
{
    string_vec lines;

    if (!read_text_file(pSrc_filename, lines))
        panic("Can't open file %s", pSrc_filename);

    FILE* pOut_file = ufopen(pDst_filename, "w");
    if (!pOut_file)
        panic("Can't open file %s", pDst_filename);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"%s Timeline\" : [\n", pSource_override ? pSource_override : "Magnonia");
        
    //const uint32_t TOTAL_RECS = 923;
    const uint32_t TOTAL_COLS = 15;

    uint32_t cur_line = 0;
    uint32_t rec_index = 1;
    while (cur_line < lines.size())
    {
        if (!lines[cur_line].size())
            panic("Line %u is empty", cur_line);

        int index = atoi(lines[cur_line++].c_str());
        if (index != (int)rec_index)
            panic("Unexpected index");

        if (cur_line == lines.size())
            panic("Out of lines");

        std::string first_line(lines[cur_line++]);

        std::string date_str(first_line);
        if (date_str.size() > TOTAL_COLS)
            date_str.resize(TOTAL_COLS);
        string_trim(date_str);

        if (first_line.size() < (TOTAL_COLS + 1))
        {
            if (cur_line == lines.size())
                panic("Out of lines");

            first_line = lines[cur_line++];
            if (first_line.size() < (TOTAL_COLS + 1))
                panic("Line too small");
        }

        string_vec desc_lines;
        first_line.erase(0, TOTAL_COLS);
        string_trim(first_line);
        desc_lines.push_back(first_line);

        std::string time_str;
                
        for (; ; )
        {
            if (cur_line == lines.size())
                break;

            if (lines[cur_line].size() < TOTAL_COLS)
                break;

            std::string buf(lines[cur_line]);

            if (desc_lines.size() == 1)
            {
                if (buf.size() >= TOTAL_COLS)
                {
                    time_str = buf;
                    time_str.resize(TOTAL_COLS);
                    string_trim(time_str);

                    buf.erase(0, TOTAL_COLS);
                }
            }

            string_trim(buf);
            desc_lines.push_back(buf);

            cur_line++;
        }

        std::string desc;
        for (uint32_t j = 0; j < desc_lines.size(); j++)
        {
            if (desc.size() && desc.back() == '-')
            {
                // Don't trim '-' char if the previous char is a digit. This is probably imperfect.
                if (!((desc.size() >= 2) && (isdigit((uint8_t)desc[desc.size() - 2]))))
                    desc.resize(desc.size() - 1);
            }
            else if (desc.size())
            {
                desc += " ";
            }

            desc += desc_lines[j];
        }

        std::string location;

        size_t n = desc.find_first_of('.');
        if (n == std::string::npos)
            panic("Can't find . char");

        location = desc;
        location.resize(n);
        string_trim(location);

        size_t f = location.find_first_of('(');
        size_t e = location.find_last_of(')');
        if ((f != std::string::npos) && (e == location.size() - 1))
        {
            std::string state(location);
            state.erase(0, f + 1);
            if (state.size())
                state.resize(state.size() - 1);
            string_trim(state);

            location.erase(f, location.size() - f);
            string_trim(location);

            location += ", ";
            location += state;
        }

        desc.erase(0, n + 1);
        string_trim(desc);

        std::string ref;
        f = desc.find_last_of('(');
        e = desc.find_last_of(')');
        if ((f != std::string::npos) && (e != std::string::npos))
        {
            if ((f < e) && (e == desc.size() - 1))
            {
                ref = desc.c_str() + f + 1;
                if (ref.size())
                    ref.pop_back();
                string_trim(ref);

                desc.resize(f);
                string_trim(desc);
            }
        }

        int year = -1, month = -1, day = -1;
        date_prefix_t date_prefix = cNoPrefix;

        std::string temp_date_str(date_str);

        if (string_begins_with(temp_date_str, "End "))
        {
            date_prefix = cEndOf;
            temp_date_str.erase(0, strlen("End "));
            string_trim(temp_date_str);
        }
        else if (string_begins_with(temp_date_str, "Early "))
        {
            date_prefix = cEarly;
            temp_date_str.erase(0, strlen("Early "));
            string_trim(temp_date_str);
        }

        f = date_str.find_first_of(',');
        uint32_t m;
        for (m = 0; m < 12; m++)
            if (string_begins_with(temp_date_str, g_months[m]))
                break;

        if (m != 12)
        {
            month = m + 1;

            temp_date_str.erase(0, strlen(g_months[m]));
            string_trim(temp_date_str);

            if (!temp_date_str.size())
                panic("Failed parsing date");

            f = temp_date_str.find_first_of(',');
            if (f != std::string::npos)
            {
                if (f == 0)
                {
                    temp_date_str.erase(0, 1);
                    string_trim(temp_date_str);

                    if (!isdigit(temp_date_str[0]))
                        panic("Failed parsing date");

                    year = atoi(temp_date_str.c_str());
                    if ((year <= 100) || (year > 2050))
                        panic("Failed parsing date");
                }
                else
                {
                    day = atoi(temp_date_str.c_str());

                    temp_date_str.erase(0, f + 1);
                    string_trim(temp_date_str);

                    if (!isdigit(temp_date_str[0]))
                        panic("Failed parsing date");

                    year = atoi(temp_date_str.c_str());
                    if ((year <= 100) || (year > 2050))
                        panic("Failed parsing date");
                }
            }
            else
            {
                if (!isdigit(temp_date_str[0]))
                    panic("Failed parsing date");

                year = atoi(temp_date_str.c_str());
                if ((year <= 100) || (year > 2050))
                    panic("Failed parsing date");
            }

        }
        else
        {
            // The Magnonia data doesn't use the full range of prefixes we support.
            if (string_begins_with(temp_date_str, "Summer,"))
            {
                date_prefix = cSummer;
                temp_date_str.erase(0, strlen("Summer,"));
                string_trim(temp_date_str);
            }
            else if (string_begins_with(temp_date_str, "Spring,"))
            {
                date_prefix = cSpring;
                temp_date_str.erase(0, strlen("Spring,"));
                string_trim(temp_date_str);
            }
            else if (string_begins_with(temp_date_str, "Fall,"))
            {
                date_prefix = cFall;
                temp_date_str.erase(0, strlen("Fall,"));
                string_trim(temp_date_str);
            }
            else if (string_begins_with(temp_date_str, "End "))
            {
                date_prefix = cEndOf;
                temp_date_str.erase(0, strlen("End "));
                string_trim(temp_date_str);
            }
            else if (string_begins_with(temp_date_str, "Early "))
            {
                date_prefix = cEarly;
                temp_date_str.erase(0, strlen("Early "));
                string_trim(temp_date_str);
            }

            if (!isdigit(temp_date_str[0]))
                panic("Failed parsing date");

            year = atoi(temp_date_str.c_str());
            if ((year <= 100) || (year > 2050))
                panic("Failed parsing date");
        }

        if (pRef_override)
        {
            if (ref.size())
                ref += " ";
            ref += pRef_override;
        }
        else
            ref += " ([Vallee](https://web.archive.org/web/20120415100852/http://www.ufoinfo.com/magonia/magonia.shtml))";

        //printf("## %u. Date: \"%s\" (%s %i/%i/%i), Time: \"%s\" Location: \"%s\", Ref: \"%s\"\n",
        //    i, date_str.c_str(), (date_prefix >= 0) ? g_date_prefix_strings[date_prefix] : "", month, day, year, time_str.c_str(), location.c_str(), ref.c_str());
        //printf("%s\n", desc.c_str());

        if (rec_index > 1)
            fprintf(pOut_file, ",\n");

        fprintf(pOut_file, "{\n");
        fprintf(pOut_file, "  \"date\" : \"");

        if (date_prefix >= 0)
            fprintf(pOut_file, "%s ", g_date_prefix_strings[date_prefix]);

        if (month == -1)
            fprintf(pOut_file, "%i", year);
        else if (day == -1)
            fprintf(pOut_file, "%i/%i", month, year);
        else
            fprintf(pOut_file, "%i/%i/%i", month, day, year);
        fprintf(pOut_file, "\",\n");

        fprintf(pOut_file, "  \"desc\": \"%s\",\n", escape_string_for_json(desc).c_str());

        if (location.size())
            fprintf(pOut_file, "  \"location\" : \"%s\",\n", escape_string_for_json(location).c_str());

        if (time_str.size())
            fprintf(pOut_file, "  \"time\" : \"%s\",\n", time_str.c_str());

        if (ref.size())
            fprintf(pOut_file, "  \"ref\": \"%s\",\n", escape_string_for_json(ref).c_str());

        fprintf(pOut_file, "  \"source_id\" : \"Magnonia_%u\",\n", rec_index);
        
        fprintf(pOut_file, u8"  \"source\" : \"%s\",\n", pSource_override ? pSource_override : u8"ValléeMagnonia");

        fprintf(pOut_file, "  \"type\" : \"ufo sighting\"\n");
                                
        fprintf(pOut_file, "}");

        rec_index++;
    }

    fprintf(pOut_file, "\n] }\n");
    fclose(pOut_file);

    return true;
}

static bool invoke_openai(const std::string& prompt, std::string& reply)
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
    if (!read_text_file("o.txt", lines))
    {
        // Wait a bit and try again, rarely needed under Windows.
        Sleep(50);
        if (!read_text_file("o.txt", lines))
            return false;
    }

    // Skip any blank lines at the beginning of the reply.
    uint32_t i;
    for ( i = 0; i < lines.size(); i++)
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

static bool convert_bluebook_unknowns()
{
    string_vec lines;

    if (!read_text_file("bb_unknowns.txt", lines))
        panic("Can't read file bb_unknowns.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("bb_unknowns.json", "w");
    if (!pOut_file)
        panic("Can't open output file bb_unknowns.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"BlueBookUnknowns Timeline\" : [\n");

    while (cur_line < lines.size())
    {
        std::string rec;

        while (cur_line < lines.size())
        {
            std::string l(lines[cur_line]);
            cur_line++;

            string_trim(l);

            if (!l.size())
                break;

            if (rec.size() && rec.back() == '-')
                rec += l;
            else
            {
                rec += ' ';
                rec += l;
            }
        }

        if (rec.size())
        {
            //printf("%u. %s\n", total_recs + 1, rec.c_str());

            size_t semi_ofs = rec.find_first_of(';');
            if (semi_ofs == std::string::npos)
                panic("Unable to find initial ; char");

            std::string date_str(rec);
            date_str.resize(semi_ofs);
            string_trim(date_str);

            rec.erase(0, semi_ofs + 1);
            string_trim(rec);

            size_t period_ofs = rec.find_first_of(';');
            if (period_ofs == std::string::npos)
            {
                period_ofs = rec.find_first_of('.');
                if (period_ofs == std::string::npos)
                    panic("Unable to find . char");

                if (((period_ofs >= 2) && (rec[period_ofs - 1] == 't') && (rec[period_ofs - 2] == 'F')) ||
                    ((period_ofs >= 2) && (rec[period_ofs - 1] == 't') && (rec[period_ofs - 2] == 'M')))
                {
                    period_ofs = rec.find('.', period_ofs + 1);
                    if (period_ofs == std::string::npos)
                        panic("Unable to find . char");
                }
            }

            std::string location_str(rec);
            location_str.resize(period_ofs);
            string_trim(location_str);

            rec.erase(0, period_ofs + 1);
            string_trim(rec);

            size_t time_period_ofs = rec.find_first_of('.');
            if (time_period_ofs == std::string::npos)
                panic("Unable to find , char");

            std::string time_str(rec);
            time_str.resize(time_period_ofs);
            string_trim(time_str);

            rec.erase(0, time_period_ofs + 1);
            string_trim(rec);
            
            //printf("Rec: %u\n", total_recs + 1);
            //printf("Location: %s\n", location_str.c_str());
            //printf("Time: %s\n", time_str.c_str());
            //printf("Desc: %s\n", rec.c_str());

            std::string json_date;
            if ((string_begins_with(date_str, "Spring")) || (string_begins_with(date_str, "Summer")))
            {
                json_date = date_str;
            }
            else
            {
                size_t date_space = date_str.find_first_of(' ');
                if (date_space == std::string::npos)
                    panic("Unable to find space char");

                size_t date_comma = date_str.find_first_of(',');
                
                int month;

                if (string_begins_with(date_str, "Sept."))
                    month = 8;
                else
                {
                    for (month = 0; month < 12; month++)
                    {
                        if (string_begins_with(date_str, g_months[month]))
                            break;
                        if (string_begins_with(date_str, g_full_months[month]))
                            break;
                    }

                    if (month == 12)
                        panic("Failed finding month");
                }

                char buf[256];

                int day, year;
                if (date_comma == std::string::npos)
                {
                    year = atoi(date_str.c_str() + date_space + 1);
                    if ((year < 1900) || (year > 1969))
                        panic("Invalid year");

                    sprintf_s(buf, "%u/%u", month + 1, year);
                }
                else
                {
                    day = atoi(date_str.c_str() + date_space + 1);
                    if ((day < 1) || (day > 31))
                        panic("Invalid day");

                    year = atoi(date_str.c_str() + date_comma + 1);
                    if ((year < 1900) || (year > 1969))
                        panic("Invalid year");

                    sprintf_s(buf, "%u/%u/%u", month + 1, day, year);
                }
                
                json_date = buf;
            }
            //printf("JSON Date: %s\n", date_str.c_str());
            //printf("Date: %s\n", json_date.c_str());

            fprintf(pOut_file, "{\n");
            fprintf(pOut_file, "  \"date\" : \"%s\",\n", json_date.c_str());
            fprintf(pOut_file, "  \"time\" : \"%s\",\n", time_str.c_str());
            fprintf(pOut_file, "  \"location\" : \"%s\",\n", escape_string_for_json(location_str).c_str());
            fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(rec).c_str());
            fprintf(pOut_file, "  \"source_id\" : \"BerlinerBBU_%u\",\n", total_recs);
            fprintf(pOut_file, "  \"source\" : \"BerlinerBBUnknowns\",\n");
            fprintf(pOut_file, "  \"ref\" : \"[BlueBook Unknowns PDF](https://github.com/richgel999/uap_resources/blob/main/bluebook_uncensored_unknowns_don_berliner.pdf)\",\n");
            fprintf(pOut_file, "  \"type\" : \"ufo sighting\"\n");
            fprintf(pOut_file, "}");
            if (cur_line < lines.size())
                fprintf(pOut_file, ",");
            fprintf(pOut_file, "\n");

            total_recs++;
        }
    }

    fprintf(pOut_file, "] }\n");
    fclose(pOut_file);

    return true;
}

static std::string convert_hall_to_json_date(const std::string& date_str)
{
    std::string json_date;

    if ((string_begins_with(date_str, "Spring")) ||
        (string_begins_with(date_str, "Summer")) ||
        (string_begins_with(date_str, "Late")) ||
        (string_begins_with(date_str, "Early")))
    {
        json_date = date_str;
    }
    else
    {
        size_t date_space = date_str.find_first_of(' ');
        if (date_space == std::string::npos)
            panic("Unable to find space char");

        size_t date_comma = date_str.find_first_of(',');

        int month;

        if (string_begins_with(date_str, "Sept."))
            month = 8;
        else
        {
            for (month = 0; month < 12; month++)
            {
                if (string_begins_with(date_str, g_months[month]))
                    break;
                if (string_begins_with(date_str, g_full_months[month]))
                    break;
            }

            if (month == 12)
                panic("Failed finding month");
        }

        char buf[256];

        int day, year;
        if (date_comma == std::string::npos)
        {
            year = atoi(date_str.c_str() + date_space + 1);
            if ((year < 1900) || (year > 2000))
                panic("Invalid year");

            sprintf_s(buf, "%u/%u", month + 1, year);
        }
        else
        {
            day = atoi(date_str.c_str() + date_space + 1);
            if ((day < 1) || (day > 31))
                panic("Invalid day");

            year = atoi(date_str.c_str() + date_comma + 1);
            if ((year < 1900) || (year > 2000))
                panic("Invalid year");

            sprintf_s(buf, "%u/%u/%u", month + 1, day, year);
        }

        json_date = buf;
    }
    return json_date;
}

static bool convert_hall()
{
    string_vec lines;

    if (!read_text_file("ufo_evidence_hall.txt", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("ufo_evidence_hall.json", "w");
    if (!pOut_file)
        panic("Can't open output file ufo_evidence_hall.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"UFOEvidenceHall Timeline\" : [\n");

    const uint32_t MAX_RECS = 2000;

    while (cur_line < lines.size())
    {
        std::string rec(lines[cur_line]);
        cur_line++;

        string_trim(rec);
        if (rec.empty())
            panic("Encountered empty line");

        size_t first_semi_ofs = rec.find_first_of(';');
        if (first_semi_ofs == std::string::npos)
            panic("Can't first first semi");

        size_t second_semi_ofs = rec.find_first_of(';');
        if (second_semi_ofs == std::string::npos)
            panic("Can't find second semi");

        while (cur_line < lines.size())
        {
            std::string l(lines[cur_line]);
            string_trim(l);

            if (!l.size())
                panic("Encountered empty line");

            size_t semi_ofs = l.find_first_of(';');
            if (semi_ofs != std::string::npos)
                break;

            cur_line++;

            if (rec.size() && rec.back() == '-')
                rec += l;
            else
            {
                rec += ' ';
                rec += l;
            }
        }

        if (rec.size())
        {
            //printf("%u. %s\n", total_recs + 1, rec.c_str());

            std::string date_str(rec);
            date_str.resize(first_semi_ofs);
            string_trim(date_str);

            rec.erase(0, first_semi_ofs + 1);
            string_trim(rec);

            second_semi_ofs = rec.find_first_of(';');
            if (second_semi_ofs == std::string::npos)
                panic("Can't find second semi");

            std::string location_str(rec);
            location_str.resize(second_semi_ofs);
            string_trim(location_str);
            if (location_str.size() && location_str.back() == '.')
                location_str.pop_back();

            rec.erase(0, second_semi_ofs + 1);
            string_trim(rec);

            //uprintf("Rec: %u\n", total_recs + 1);
            //uprintf("Date: %s\n", date_str.c_str());
            //uprintf("Location: %s\n", location_str.c_str());
            //uprintf("Desc: %s\n", rec.c_str());

            std::string json_date(convert_hall_to_json_date(date_str));
            std::string json_end_date;

            //uprintf("JSON Date: %s\n", date_str.c_str());
            //uprintf("Date: %s\n", json_date.c_str());

            if (string_begins_with(rec, "to "))
            {
                size_t dot_ofs = rec.find_first_of('.');

                if (dot_ofs == std::string::npos)
                    panic("Invalid to date");

                std::string to_date(rec);
                to_date.resize(dot_ofs);
                to_date.erase(0, 3);
                string_trim(to_date);

                rec.erase(0, dot_ofs + 1);
                string_trim(rec);

                json_end_date = convert_hall_to_json_date(to_date);
            }

            size_t k = rec.find_last_of('(');
            if (k != std::string::npos)
            {
                if ((string_begins_with(rec.c_str() + k, "(Section ")) ||
                    (string_begins_with(rec.c_str() + k, "(section ")) ||
                    (string_begins_with(rec.c_str() + k, "(Sections ")) ||
                    (string_begins_with(rec.c_str() + k, "(sections ")))
                {
                    rec.erase(k);
                    string_trim(rec);
                }
            }

#if USE_OPENAI
            std::string prompt_str("Best categorize the following quoted text into one or more of these categories as a json array: sighting, landing, natural phenomenom, newspaper article, report or memo, official, abduction, medical, occupant or alien or creature encounter, or historical event: \"");
                        
            prompt_str += rec;
            prompt_str += "\"";

            std::string type_str;
            bool status = invoke_openai(prompt_str, type_str);
            if (!status)
                panic("invoke_openai failed!\n");

            for (size_t i = 0; i < type_str.size(); i++)
            {
                uint8_t c = type_str[i];

                if ((c >= 32) && (c <= 127))
                    type_str[i] = (char)tolower(c);
            }
#else
            std::string type_str("[\"sighting\"]");
#endif

            fprintf(pOut_file, "{\n");
            fprintf(pOut_file, "  \"date\" : \"%s\",\n", json_date.c_str());
            if (json_end_date.size())
                fprintf(pOut_file, "  \"end_date\" : \"%s\",\n", json_end_date.c_str());
            fprintf(pOut_file, "  \"location\" : \"%s\",\n", escape_string_for_json(location_str).c_str());
            fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(rec).c_str());
            fprintf(pOut_file, "  \"source_id\" : \"HallUFOE2_%u\",\n", total_recs);
            fprintf(pOut_file, "  \"source\" : \"HallUFOEvidence2\",\n");
            fprintf(pOut_file, "  \"ref\" : \"[The UFO Evidence by Richard H. Hall](https://www.amazon.com/UFO-Evidence-Richard-Hall/dp/0760706271)\",\n");
            if (type_str.size())
                fprintf(pOut_file, "  \"type\" : %s\n", type_str.c_str());

            fprintf(pOut_file, "}");
            if (cur_line < lines.size())
                fprintf(pOut_file, ",");
            fprintf(pOut_file, "\n");

#if USE_OPENAI
            fflush(pOut_file);
#endif

            total_recs++;
            if (total_recs == MAX_RECS)
                break;
        }
    }

    fprintf(pOut_file, "] }\n");
    fclose(pOut_file);

    return true;
}

static bool convert_eberhart()
{
    string_vec lines;

    if (!read_text_file("ufo1_199.md", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo200_399.md", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo400_599.md", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo600_906_1.md", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo600_906_2.md", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    string_vec trimmed_lines;
    for (uint32_t i = 0; i < lines.size(); i++)
    {
        std::string s(lines[i]);
        string_trim(s);
        if (s.size())
            trimmed_lines.push_back(s);
    }
    lines.swap(trimmed_lines);
                
    uint32_t cur_line = 0;

    string_vec new_lines;
    while (cur_line < lines.size())
    {
        std::string line(lines[cur_line]);

        bool all_digits = true;
        for (uint32_t i = 0; i < line.size(); i++)
        {
            if (!isdigit((uint8_t)line[i]))
            {
                all_digits = false;
                break;
            }
        }

        if (line == "* * *")
        {
        }
        else if (all_digits && ((line.size() == 3) || (line.size() == 4)))
        {
            char buf[256];
            sprintf_s(buf, "# %s", line.c_str());
            new_lines.push_back(buf);
        }
        else if (line == "#")
        {
            if ((cur_line + 1) >= lines.size())
                panic("Out of lines");

            if (!isdigit((char)lines[cur_line + 1][0]))
                panic("Can't fine year");

            char buf[256];
            sprintf_s(buf, "# %s", lines[cur_line + 1].c_str());
            new_lines.push_back(buf);
            cur_line++;
        }
        else if ((line.size() >= 5) && (line[0] == '*') && (line[1] == '*'))
        {
            uint32_t year = atoi(line.c_str() + 2);

            if ((year < 800) || (year > 2050))
                panic("Invalid year");

            char buf[256];
            sprintf_s(buf, "# %u", year);
            new_lines.push_back(buf);
        }
        else
            new_lines.push_back(line);

        cur_line++;
    }
    lines.swap(new_lines);

    //write_text_file("temp.txt", lines);

    FILE* pOut_file = ufopen("eberhart.json", "w");
    if (!pOut_file)
        panic("Can't open output file eberhart.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"Eberhart Timeline\" : [\n");

    int cur_year = -1;
    cur_line = 0;

    uint32_t event_num = 0, total_unattributed = 0;

    while (cur_line < lines.size())
    {
        std::string line(lines[cur_line]);

        cur_line++;

        if (!line.size())
            continue;

        if (line[0] == '#')
        {
            int year = atoi(line.c_str() + 1);
            if ((year < 100) || (year > 2050))
                panic("Invalid year");

            cur_year = year;

            continue;
        }

        size_t dash_pos = line.find(u8"—");
        if (dash_pos == std::string::npos)
            panic("Failed finding dash\n");

        std::string date(line);
        date.erase(dash_pos, date.size());
        string_trim(date);

        if (!date.size())
            panic("Date too small");

        line.erase(0, dash_pos + 3);
        string_trim(line);

        if (!line.size())
            panic("Line too small");

        event_date begin_date;
        event_date end_date;
        event_date alt_date;
        bool year_status = event_date::parse_eberhart_date_range(date, begin_date, end_date, alt_date, cur_year);
        if (!year_status)
            panic("Date parse failed");

        std::string desc(line);
        while (cur_line < lines.size())
        {
            std::string temp(lines[cur_line]);

            if (temp[0] == '#')
                break;

            size_t d = temp.find(u8"—");

            const uint32_t DASH_THRESH_POS = 42;
            if ((d != std::string::npos) && (d < DASH_THRESH_POS))
            {
                std::string temp_date(temp);
                temp_date.erase(d, temp_date.size());
                string_trim(temp_date);

                event_date b, e, a;
                bool spec_year_status = event_date::parse_eberhart_date_range(temp_date, b, e, a, cur_year);
                if (spec_year_status)
                    break;
            }

            if ((desc.size()) && (desc.back() != '-'))
                desc += ' ';

            if (!lines[cur_line].size())
                panic("Unexpected empty line");

            desc += lines[cur_line];
            cur_line++;
        }
                
        std::string ref;

        {
            markdown_text_processor mt;
            mt.init_from_markdown(desc.c_str());

            mt.fix_redirect_urls();

            markdown_text_processor a, b;
            bool status = mt.split_last_parens(a, b);
            
            if (!status)
            {
                uprintf("Failing text: %s\n", desc.c_str());
                
                panic("Unable to find attribution in record %u", event_num);
            }

            std::string a_md, b_md;
            
            desc.resize(0);
            a.convert_to_markdown(desc, true);
                        
            b.convert_to_markdown(ref, true);
            
            if (ref == "\\(\\)")
            {
                ref.clear();
                total_unattributed++;
            }
            else if (ref.size() >= 4)
            {
                if ( (ref[0] == '\\') && 
                    (ref[1] == '(') && 
                    (ref.back() == ')') && 
                    (ref[ref.size() - 2] == '\\')
                    )
                {
                    ref.erase(0, 2);

                    ref.pop_back();
                    ref.pop_back();
                }
            }
        }

#if 0
        uprintf("## Event %u\n", event_num);

        uprintf("Date: %s  \n", date.c_str());

        uprintf("(%i %i/%i/%i)-(%i %i,%i,%i) alt: (%i %i,%i,%i)  \n",
            begin_date.m_prefix, begin_date.m_month, begin_date.m_day, begin_date.m_year,
            end_date.m_prefix, end_date.m_month, end_date.m_day, end_date.m_year,
            alt_date.m_prefix, alt_date.m_month, alt_date.m_day, alt_date.m_year);

        uprintf("%s  \n\n", desc.c_str());
#endif
        fprintf(pOut_file, "{\n");

        std::string json_date(begin_date.get_string()), json_end_date(end_date.get_string()), json_alt_date(alt_date.get_string());

        fprintf(pOut_file, "  \"date\" : \"%s\",\n", json_date.c_str());

        if (json_end_date.size())
            fprintf(pOut_file, "  \"end_date\" : \"%s\",\n", json_end_date.c_str());

        if (json_alt_date.size())
            fprintf(pOut_file, "  \"alt_date\" : \"%s\",\n", json_alt_date.c_str());
                
        fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(desc).c_str());
        fprintf(pOut_file, "  \"source_id\" : \"Eberhart_%u\",\n", event_num);
        
        fprintf(pOut_file, "  \"source\" : \"EberhartUFOI\",\n");

        if (!ref.size())
        {
            fprintf(pOut_file, "  \"ref\" : \"[Eberhart](http://www.cufos.org/pdfs/UFOsandIntelligence.pdf)\"\n");
        }
        else
        {
            fprintf(pOut_file, "  \"ref\" : [ \"[Eberhart](http://www.cufos.org/pdfs/UFOsandIntelligence.pdf)\", \"%s\" ]\n", escape_string_for_json(ref).c_str());
        }
        
        fprintf(pOut_file, "}");

        if (cur_line < lines.size())
            fprintf(pOut_file, ",");

        fprintf(pOut_file, "\n");

        event_num++;
    }

    fprintf(pOut_file, "] }\n");
    fclose(pOut_file);

    uprintf("Total records: %u\n", event_num);
    uprintf("Total unattributed: %u\n", total_unattributed);

    return true;
}

static bool convert_johnson()
{
    static const uint32_t days_in_month[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    FILE* pCombined_file = nullptr;

#if 0
    pCombined_file = ufopen("combined.txt", "wb");
    if (!pCombined_file)
        panic("Can't open combined.txt");

    fputc(UTF8_BOM0, pCombined_file);
    fputc(UTF8_BOM1, pCombined_file);
    fputc(UTF8_BOM2, pCombined_file);
#endif

    FILE* pOut_file = ufopen("johnson.json", "w");
    if (!pOut_file)
        panic("Can't open output file johnson.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"Johnson Timeline\" : [\n");

    string_vec combined_lines;

    uint32_t total_recs = 0;

    for (uint32_t month = 0; month < 12; month++)
    {
        for (uint32_t day = 0; day < days_in_month[month]; day++)
        {
            char buf[256];
            sprintf_s(buf, "Johnson/%s%02u.txt", g_full_months[month], day + 1);
                        
            string_vec lines;

            bool utf8_flag = false;
            bool status = read_text_file(buf, lines, true, &utf8_flag);
            if (!status)
                panic("Can't open file %s\n", buf);

            uprintf("Read file %s %u\n", buf, utf8_flag);

            string_vec filtered_lines;
            bool found_end = false;
            for (uint32_t line_index = 0; line_index < lines.size(); line_index++)
            {
                if ((lines[line_index].size() >= 2) && (lines[line_index][0] == '|') && (lines[line_index].back() == '|'))
                {
                    std::string l(lines[line_index]);

                    l.erase(0, 1);
                    l.pop_back();

                    // Convert non-break spaces to spaces
                    std::string new_l;
                    for (uint32_t i = 0; i < l.size(); i++)
                    {
                        uint8_t c = (uint8_t)l[i];
                        if ((c == 0xC2) && ((i + 1) < l.size()) && ((uint8_t)l[i + 1] == 0xA0))
                        {
                            new_l.push_back(' ');
                            i++;
                        }
                        // EN DASH
                        else if ((c == 0xE2) && ((i + 2) < l.size()) && ((uint8_t)l[i + 1] == 0x80) && ((uint8_t)l[i + 2] == 0x93))
                        {
                            new_l.push_back('-');
                            i += 2;
                        }
                        else
                            new_l.push_back(l[i]);
                    }
                                        
                    l.swap(new_l);

                    string_trim(l);
                                       
                    // See if we're at the end of the page
                    if ( (string_find_first(l, "Written by Donald A. Johnson") != -1) ||
                        (string_find_first(l, "Written by Donald Johnson") != -1) ||
                        (string_find_first(l, "Written by Donald A Johnson") != -1) ||
                        (string_find_first(l, "Compiled from the UFOCAT computer database") != -1) ||
                        (string_find_first(l, u8"© Donald A. Johnson") != -1) ||
                        (string_begins_with(l, "Themes: ")) )
                    {
                        found_end = true;
                        break;
                    }
                                                            
                    filtered_lines.push_back(l);
                }
            }
            lines.swap(filtered_lines);

            if (!found_end)
                panic("Couldn't find end");
                        
            if (lines.size() < 6)
                panic("File too small");

            if ((lines[0] != "[On This Day]") || (string_find_first(lines[1], "Encounters with Aliens on this Day") == -1))
                panic("Couldn't find beginning");
                       
            lines.erase(lines.begin(), lines.begin() + 5);
                        
            //for (uint32_t i = 0; i < lines.size(); i++)
            //    uprintf("%04u: \"%s\"\n", i, lines[i].c_str());
            //uprintf("\n");

            string_vec new_lines;

            for (uint32_t i = 0; i < lines.size(); i++)
            {
                if (lines[i].size() && 
                    (lines[i].back() == ']') &&
                    ((string_find_first(lines[i], "[") == -1) || (lines[i].front() == '[')) )
                {
                    int back_delta_pos = -1;

                    for (int back_delta = 0; back_delta <= 3; back_delta++)
                    {
                        if ((int)i < back_delta)
                            break;
                        if (lines[i - back_delta].size() && lines[i - back_delta].front() == '[') 
                        {
                            back_delta_pos = i - back_delta;
                            break;
                        }
                    }

                    bool find_extra_text = true;

                    if (back_delta_pos == -1)
                    {
                        for (int back_delta = 0; back_delta <= 3; back_delta++)
                        {
                            if ((int)i < back_delta)
                                break;
                            if (string_find_first(lines[i - back_delta], "[") != -1)
                            {
                                back_delta_pos = i - back_delta;
                                break;
                            }
                        }

                        if (back_delta_pos == -1)
                            panic("Can't find back delta");
                        find_extra_text = false;
                    }

                    if ((back_delta_pos < 2) || lines[back_delta_pos - 1].size())
                    {
                        new_lines.push_back(lines[i]);
                        continue;
                    }

                    if (find_extra_text)
                    {
                        back_delta_pos -= 2;

                        while (back_delta_pos && lines[back_delta_pos].size())
                        {
                            back_delta_pos--;
                        }
                    }

                    uint32_t total_lines_to_erase = (i + 1) - (back_delta_pos + 1);

                    //for (uint32_t j = 0; j < total_lines_to_erase; j++)
                    //    uprintf("\"%s\"\n", new_lines[(new_lines.size() - total_lines_to_erase) + j].c_str());

                    new_lines.erase(new_lines.begin() + (new_lines.size() - total_lines_to_erase), new_lines.end());
                }
                else
                {
                    new_lines.push_back(lines[i]);
                }
            }

            lines.swap(new_lines);

#if 0
            for (uint32_t i = 0; i < lines.size(); i++)
            {
                fwrite(lines[i].c_str(), lines[i].size(), 1, pCombined_file);
                fwrite("\r\n", 2, 1, pCombined_file);
            }
#endif

            // Sanity checks
            for (uint32_t i = 0; i < lines.size(); i++)
            {
                const std::string &line = lines[i];
                
                if (line.size() < 7)
                    continue;

                if (line.find_first_of('|') != std::string::npos)
                {
                    panic("Bad line");
                }

                if (isdigit((uint8_t)line[0]) &&
                    isdigit((uint8_t)line[1]) &&
                    isdigit((uint8_t)line[2]) &&
                    isdigit((uint8_t)line[3]) &&
                    (line[4] == ',') && (i >= 1) && (lines[i - 1].size() == 0))
                {
                    panic("Bad line");
                }

                if (string_begins_with(line, "In ") && 
                    (i >= 1) && (lines[i - 1].size() == 0))
                {
                    panic("Bad line");
                }

                if (  (string_begins_with(line, "On the ") || 
                       string_begins_with(line, "On this ") ||
                       string_begins_with(line, "That same ") ||
                       string_begins_with(line, "At dusk ") ||
                       string_begins_with(line, "At dawn ") ||
                       string_begins_with(line, "There were ") ||
                       string_begins_with(line, "A UFO was seen ") || string_begins_with(line, "An abduction occurred ") || 
                       string_begins_with(line, "Also in ") || 
                       string_begins_with(line, "There were a ") ||
                       (string_begins_with(line, "At ") && line.size() >= 4 && isdigit((uint8_t)line[3])) ) &&
                    (i >= 1) && (lines[i - 1].size() == 0))
                {
                    panic("Bad line");
                }
                                
                if (isdigit((uint8_t)line[0]) &&
                    isdigit((uint8_t)line[1]) &&
                    isdigit((uint8_t)line[2]) &&
                    isdigit((uint8_t)line[3]) &&
                    (line[4] == ' ') &&
                    (line[5] == '-') &&
                    (line[6] != ' '))
                {
                    panic("Bad line");
                }

                if (isdigit((uint8_t)line[0]) &&
                    isdigit((uint8_t)line[1]) &&
                    isdigit((uint8_t)line[2]) &&
                    isdigit((uint8_t)line[3]) &&
                    (line[4] == '-') &&
                    (line[5] == ' '))
                {
                    panic("Bad line");
                }
            }
                                                
            uint32_t cur_line = 0;
            int prev_year = -1;

            while (cur_line < lines.size())
            {
                std::string first_line = lines[cur_line];
                if (first_line.size() < 8)
                    panic("First line in block too small");

                if (!isdigit((uint8_t)first_line[0]) ||
                    !isdigit((uint8_t)first_line[1]) ||
                    !isdigit((uint8_t)first_line[2]) ||
                    !isdigit((uint8_t)first_line[3]) ||
                    (first_line[4] != ' ') ||
                    (first_line[5] != '-') ||
                    (first_line[6] != ' '))
                {
                    panic("Bad begin block");
                }

                int year = atoi(first_line.c_str());
                if ((year < 1000) || (year > 2020))
                    panic("Invalid year");
                
                if (year < prev_year)
                    panic("Year went backwards");

                prev_year = year;

                first_line.erase(0, 7);

                std::string record_text(first_line);

                cur_line++;

                while (cur_line < lines.size())
                {
                    const std::string &next_line = lines[cur_line];

                    if (isdigit((uint8_t)next_line[0]) &&
                        isdigit((uint8_t)next_line[1]) &&
                        isdigit((uint8_t)next_line[2]) &&
                        isdigit((uint8_t)next_line[3]) &&
                        (next_line[4] == ' ') &&
                        (next_line[5] == '-') &&
                        (next_line[6] == ' '))
                    {
                        break;
                    }

                    if (record_text.size())
                    {
                        if (next_line.size() == 0)
                            record_text += "<br/><br/>";
                        else if (record_text.back() != '-')
                            record_text.push_back(' ');
                    }
                                        
                    record_text += next_line;

                    cur_line++;
                }

                if (string_ends_in(record_text, "<br/><br/>"))
                {
                    record_text.erase(record_text.begin() + record_text.size() - 10, record_text.end());
                }

                string_trim(record_text);

                if (pCombined_file)
                {
                    char buf2[256];
                    sprintf_s(buf2, "Record %u, year %i:\r\n", total_recs, year);
                    fwrite(buf2, strlen(buf2), 1, pCombined_file);

                    fwrite(&record_text[0], record_text.size(), 1, pCombined_file);
                    fwrite("\r\n", 2, 1, pCombined_file);
                }

                std::string ref;

                // Extract reference(s) at end of record.
                size_t src_pos = record_text.find("(Source:");
                if (src_pos == std::string::npos)
                    src_pos = record_text.find("(Sources:");
                
                if ((src_pos != std::string::npos) && 
                    ( (record_text.back() == ')') || ((record_text.back() == '.') && (record_text[record_text.size() - 2] == ')')) )
                   )
                {
                    ref = record_text;
                                        
                    ref.erase(0, src_pos);
                    
                    if (ref.back() == '.')
                        ref.pop_back();
                    assert(ref.back() == ')');
                    ref.pop_back();
                    
                    assert(ref[0] == '(');
                    ref.erase(0, 1);

                    if (string_begins_with(ref, "Source:"))
                        ref.erase(0, 7);
                    else
                        ref.erase(0, 8);
                    
                    string_trim(ref);

                    record_text.erase(src_pos, record_text.size() - src_pos);
                }
                else
                {
                    //uprintf("%s\n\n", record_text.c_str());
                }

                if (total_recs)
                    fprintf(pOut_file, ",\n");

                fprintf(pOut_file, "{\n");
                
                char buf2[256];
                sprintf_s(buf2, "%u/%u/%u", month + 1, day + 1, year);
                fprintf(pOut_file, "  \"date\" : \"%s\",\n", buf2);

                fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(record_text).c_str());

                fprintf(pOut_file, "  \"source_id\" : \"Johnson_%u\",\n", total_recs);

                fprintf(pOut_file, "  \"source\" : \"Johnson\",\n");

                if (ref.size())
                {
                    fprintf(pOut_file, "  \"ref\" : [ \"[Johnson](https://web.archive.org/web/http://www.ufoinfo.com/onthisday/calendar.html)\", \"%s\" ]\n",
                        escape_string_for_json(ref).c_str());
                }
                else
                    fprintf(pOut_file, "  \"ref\" : \"[Johnson](https://web.archive.org/web/http://www.ufoinfo.com/onthisday/calendar.html)\"\n");

                fprintf(pOut_file, "}");
    
                total_recs++;
            }

            if (pCombined_file)
                fflush(pCombined_file);
        }
    }

    fprintf(pOut_file, "\n] }\n");
    fclose(pOut_file);
        
    printf("Total records: %u\n", total_recs);

    if (pCombined_file)
        fclose(pCombined_file);

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
static bool load_column_text(const char* pFilename, std::vector<string_vec>& rows, std::string &title, string_vec &col_titles)
{
    string_vec lines;
    bool utf8_flag = false;
    if (!read_text_file(pFilename, lines, utf8_flag))
        panic("Failed reading text file %s", pFilename);

    if (utf8_flag)
        panic("load_column_text() doesn't support utf8 yet");

    if (!lines.size() || !lines[0].size())
        panic("Expected title");

    if (lines.size() < 6)
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

        uprintf("%u:\n", cur_record_index);
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
                        if ((col_lines[col_index].back() != '-') && ( (uint8_t)col_lines[col_index].back() != ANSI_SOFT_HYPHEN))
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

        // Convert from ANSI (code page 252) to UTF8.
        for (auto& l : col_lines)
            l = ansi_to_utf8(l);
                        
        for (uint32_t col_index = 0; col_index < column_info.size(); col_index++)
        {
            uprintf("%s\n", col_lines[col_index].c_str());
        }

        uprintf("\n");
        
        cur_record_index++;
    }
                
    return true;
}

static bool test_eberhart_date()
{
    event_date b, e, a;

    if (!event_date::parse_eberhart_date_range("Late summer", b, e, a, 1970)) return false;
    assert(b.m_prefix == cLateSummer && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Early winter", b, e, a, 1970)) return false;
    assert(b.m_prefix == cEarlyWinter && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Mid autumn", b, e, a, 1970)) return false;
    assert(b.m_prefix == cMidAutumn && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Mid autumn 1970 (or Late summer)", b, e, a, 1970)) return false;
    assert(b.m_prefix == cMidAutumn && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cLateSummer && a.m_year == 1970 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Mid-December", b, e, a, 1970)) return false;
    assert(b.m_prefix == cMiddleOf && b.m_year == 1970 && b.m_month == 12 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("December 16?", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 12 && b.m_day == 16 && b.m_fuzzy);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Fall 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cFall && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 27, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 27);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("March-June", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 3 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 6 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Summer or Fall", b, e, a, 1970)) return false;
    assert(b.m_prefix == cSummer && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cFall && a.m_year == 1970 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Summer-Fall or Winter", b, e, a, 1970)) return false;
    assert(b.m_prefix == cSummer && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cFall && e.m_year == 1970 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cWinter && a.m_year == 1970 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 5 or March 6", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 3 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("Late January 1970-Summer 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cLate && b.m_year == 1970 && b.m_month == 1 && b.m_day == -1);
    assert(e.m_prefix == cSummer && e.m_year == 1971 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 5 or 6", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 1 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("January 20-25", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 1 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Summer", b, e, a, 1970)) return false;
    assert(b.m_prefix == cSummer && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Summer-Winter", b, e, a, 1970)) return false;
    assert(b.m_prefix == cSummer && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cWinter && e.m_year == 1970 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Early", b, e, a, 1970)) return false;
    assert(b.m_prefix == cEarly && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20-February 25", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("February 25, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 2 && b.m_day == 25);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20 1970-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    // These should all fail
    if (event_date::parse_eberhart_date_range("January 20q 1970-February 25, 1971", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("Januaryq 20 1970-February 25, 1971", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("      ", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("   ,  ", b, e, a, 1970)) return false;

    return true;
}

static void print_nocr(const std::string& s)
{
    std::string new_string;

    for (uint32_t i = 0; i < s.size(); i++)
    {
        if ((s[i] != '\n') && (s[i] != 1))
            new_string.push_back(s[i]);
        else
            new_string.push_back(' ');
    }

    uprintf("%s", new_string.c_str());
}

//-------------------------------------------------------------------

static void tests()
{
    std::string blah;
    blah.push_back(ANSI_SOFT_HYPHEN);

#if 0
    // should print a dash (code page 1252 - ANSI Latin 1)
    putc((char)ANSI_SOFT_HYPHEN, stdout);
    // should print a dash
    uprintf("%s\n", wchar_to_utf8(utf8_to_wchar(blah, CP_ACP)).c_str());
#endif

    //fprintf(u8"“frightening vision”");
    //ufprintf(stderr, u8"“frightening vision”");
    assert(crc32((const uint8_t*)"TEST", 4) == 0xeeea93b8);
    assert(crc32((const uint8_t*)"408tdsfjdsfjsdh893!;", 20) == 0xa044e016);
    if (!test_eberhart_date()) return panic("test_eberhart_date failed!");

    // rg hack hack
#if 0
    //const char *p = "_Hello, [world](http://www.google.com)_ <br/><br/>This is a _test._ **COOL**\nBlah Blah\nZA ZB ZC  \nZD ZE EF\nHDR\nThis is a test\n\nPara 1\n\nPara 2";
    //const char* p = "Hello, [**world**](http://www.google.com).  \nThis is a test.\n\nNew \\*paragraph\\*.";
    //const char* p = "<br/><br/>[_B_](WWW.A.COM) **[C](WWW.B.COM)**<br/><br/>This is a test<br/><br/>Blah  \nBlah  \n\nNew (This is a test!).";
    
    //bufprintf(pIn, "A\nB  \nC\n_This is a blah_[XXXX](YYYY(S))");
    
    //const char* p = u8R"(Chemist [Gustaf Ljunggren](https://www.google.com/url?q=https://en.wikipedia.org/wiki/Gustaf_Ljunggren_(chemist)&sa=D&source=editors&ust=1674889728009134&usg=AOvVaw2v_Cymx15I5Ic1eNEYeeBr) of the Swedish National Defense Research Institute summarizes for the Swedish Defense staff his analysis of 27 finds of mysterious substances, allegedly from ghost rockets. None are radioactive and all have mundane explanations. (Anders Liljegren and Clas Svahn, “The Ghost Rockets,” UFOs 1947–1987, Fortean Tomes, 1987, pp. 33–34))";
//    const char* p = u8R"(Blah  
//English clergyman and philosopher [_John Wilkins_](https://www.google.com/url?q=https://en.wikipedia.org/wiki/John_Wilkins&sa=D&source=editors&ust=1674889727243386&usg=AOvVaw1hw56rPPqRvDJzjdV0g8Zb) writes The Discovery of a World in the Moone, in which he highlights the similarities of the Earth and the Moon (seas, mountains, atmosphere) and concludes that the Moon is likely to be inhabited by living beings, whom the calls “Selenites.” (Maria Avxentevskaya, “[How 17th Century](https://www.google.com/url?q=https://www.realclearscience.com/articles/2017/12/02/how_17th_century_dreamers_planned_to_reach_the_moon_110476.html&sa=D&source=editors&ust=1674889727243765&usg=AOvVaw13_nH4qqo0LYqJqnhq4_eI) [Dreamers Planned to Reach the Moon,](https://www.google.com/url?q=https://www.realclearscience.com/articles/2017/12/02/how_17th_century_dreamers_planned_to_reach_the_moon_110476.html&sa=D&source=editors&ust=1674889727244030&usg=AOvVaw2K5FMN315Pjxq_xO7wp7Ga)” <br/><br/>Real Clear Science, December 2, 2017)  )";

    //const char* p = u8R"(Pierre Lagrange, “[_Agobard, la Magonie et les ovnis_,](https://www.google.com/url?q=https://pierrelagrangesociologie.files.wordpress.com/2020/08/lagrange-agobard-magonie-ufologie-lhistoire-440-2017-10-p28-29.pdf&sa=D&source=editors&ust=1674889727239396&usg=AOvVaw1U01Ykx3tRTQS4QKENJuGi)” Actualité, no. 440 (October 2017): 28–29; Wikipedia, “[Magonia (mythology)](https://www.google.com/url?q=https://en.wikipedia.org/wiki/Magonia_(mythology)&sa=D&source=editors&ust=1674889727239728&usg=AOvVaw0JOQanVKKoRClyKQPK5SJi)”))";
    const char* p = "<br/>blah<br/>_[Agobard,](www.blah.com)_<br/> blah<br/>blah <br/>[_Agobard_,](www.blah.com)<br/>";

    //const char* p = "***[sssss](www.dddd.com)*** _Blah_ *Cool*_Zeek_";
    //const char* p = "P1\nP2  \nP3\n\nP4\nP5\nP6\n\nP7\nP8 **Blah** _[ZEEK](WWW.Z.COM)_";

    uprintf("Original markdown: %s\n", p);
    uprintf("----------\n");

    markdown_text_processor mt;
    mt.init_from_markdown(p);
    uprintf("Internal text: %s\n", mt.m_text.c_str());
    uprintf("----------\n");

    std::string plain;
    mt.convert_to_plain(plain, false);
    uprintf("Plain text: %s\n", mt.m_text.c_str());
    uprintf("----------\n");

#if 0
    markdown_text_processor a, b;
    bool status = mt.split_last_parens(a, b);
    uprintf("status: %u\n", status);

    std::string a_md, b_md;
    a.convert_to_markdown(a_md);
    b.convert_to_markdown(b_md);

    uprintf("A:\n%s\n", a_md.c_str());
    
    uprintf("----------\n");
    uprintf("B:\n%s\n", b_md.c_str());
#endif


#if 1
    std::string md;
    mt.convert_to_markdown(md, false);
       
#if 0
    uprintf("Plain text:\n");

    for (uint32_t i = 0; i < mt.m_text.size(); i++)
    {
        char c = mt.m_text[i];
        if (c == ' ')
            uprintf(".");
        else
            uprintf("%c", (uint8_t)c);
    }
    
    uprintf("%s\n", mt.m_text.c_str());
    
    uprintf("\n------\n");
#endif
        
    uprintf("Converted back to markdown:\n");

#if 0
    for (uint32_t i = 0; i < md.size(); i++)
    {
        char c = md[i];
        if (c == ' ')
            uprintf(".");
        else
            uprintf("%c", (uint8_t)c);
    }
#endif
    
    uprintf("\n%s\n", md.c_str());
    
    uprintf("\n------\n");

#endif

    exit(0);
#endif
}

enum
{
    cMonthFlag = 1,
    cBeginToLateFlag = 2,
    cSeasonFlag = 4,
    cApproxFlag = 8,
    cOrFlag = 16,
    cDashFlag = 32,
    cXXFlag = 64,
    cFuzzyFlag = 128,
    cSlashFlag = 256
};

static const struct
{
    const char* m_pStr;
    uint32_t m_flag;
    uint32_t m_month;
    date_prefix_t m_date_prefix;
} g_special_phrases[] =
{
    { "january", cMonthFlag, 1 },
    { "february", cMonthFlag, 2 },
    { "march", cMonthFlag, 3 },
    { "april", cMonthFlag, 4 },
    { "may", cMonthFlag, 5 },
    { "june", cMonthFlag, 6 },
    { "july", cMonthFlag, 7 },
    { "august", cMonthFlag, 8 },
    { "september", cMonthFlag, 9 },
    { "october", cMonthFlag, 10 },
    { "november", cMonthFlag, 11 },
    { "december", cMonthFlag, 12 },

    { "jan.", cMonthFlag, 1 },
    { "feb.", cMonthFlag, 2 },
    { "mar.", cMonthFlag, 3 },
    { "apr.", cMonthFlag, 4 },
    { "may.", cMonthFlag, 5 },
    { "jun.", cMonthFlag, 6 },
    { "jul.", cMonthFlag, 7 },
    { "aug.", cMonthFlag, 8 },
    { "sep.", cMonthFlag, 9 },
    { "oct.", cMonthFlag, 10 },
    { "nov.", cMonthFlag, 11 },
    { "dec.", cMonthFlag, 12 },

    { "late", cBeginToLateFlag, 0, cLate },
    { "early", cBeginToLateFlag, 0, cEarly },
    { "middle", cBeginToLateFlag, 0, cMiddleOf },
    { "end", cBeginToLateFlag, 0, cEndOf },

    { "spring", cSeasonFlag, 0, cSpring },
    { "summer", cSeasonFlag, 0, cSummer },
    { "autumn", cSeasonFlag, 0, cAutumn },
    { "fall", cSeasonFlag, 0, cFall },
    { "winter", cSeasonFlag, 0, cWinter },
    { "wint", cSeasonFlag, 0, cWinter },
    
    { "approx", cApproxFlag },
    
    { "jan", cMonthFlag, 1 },
    { "feb", cMonthFlag, 2 },
    { "mar", cMonthFlag, 3 },
    { "apr", cMonthFlag, 4 },
    { "may", cMonthFlag, 5 },
    { "jun", cMonthFlag, 6 },
    { "jul", cMonthFlag, 7 },
    { "aug", cMonthFlag, 8 },
    { "sep", cMonthFlag, 9 },
    { "oct", cMonthFlag, 10 },
    { "nov", cMonthFlag, 11 },
    { "dec", cMonthFlag, 12 },

    { "mid", cBeginToLateFlag, 0, cMiddleOf },

    { "or", cOrFlag },
    { "xx", cXXFlag },
    { "-", cDashFlag },
    { "?", cFuzzyFlag },
    { "/", cSlashFlag }
};

const uint32_t NUM_SPECIAL_PHRASES = sizeof(g_special_phrases) / sizeof(g_special_phrases[0]);

enum 
{
    cSpecialJan,
    cSpecialFeb,
    cSpecialMar,
    cSpecialApr,
    cSpecialMay,
    cSpecialJun,
    cSpecialJul,
    cSpecialAug,
    cSpecialSep,
    cSpecialOct,
    cSpecialNov,
    cSpecialDec,

    cSpecialJan2,
    cSpecialFeb2,
    cSpecialMar2,
    cSpecialApr2,
    cSpecialMay2,
    cSpecialJun2,
    cSpecialJul2,
    cSpecialAug2,
    cSpecialSep2,
    cSpecialOct2,
    cSpecialNov2,
    cSpecialDec2,

    cSpecialLate,
    cSpecialEarly,
    cSpecialMiddle,
    cSpecialEnd,

    cSpecialSpring,
    cSpecialSummer,
    cSpecialAutumn,
    cSpecialFall,
    cSpecialWinter,
    cSpecialWinter2,

    cSpecialApprox,
    
    cSpecialJan3,
    cSpecialFeb3,
    cSpecialMar3,
    cSpecialApr3,
    cSpecialMay3,
    cSpecialJun3,
    cSpecialJul3,
    cSpecialAug3,
    cSpecialSep3,
    cSpecialOct3,
    cSpecialNov3,
    cSpecialDec3,

    cSpecialMid,

    cSpecialOr,
    cSpecialXX,
    cSpecialDash,
    cSpecialFuzzy,
    cSpecialSlash,
    
    cSpecialTotal
};

static int get_special_from_token(int64_t tok)
{
    if (tok >= 0)
        panic("Invalid token");
    
    int64_t spec = -tok - 1;
    if (spec >= cSpecialTotal)
        panic("Invalid token");

    return (int)spec;
}

static bool convert_nipcap_date(std::string date, event_date& begin_date, event_date& end_date, event_date& alt_date)
{
    assert(cSpecialTotal == NUM_SPECIAL_PHRASES);

    const uint32_t MIN_YEAR = 1860;
    const uint32_t MAX_YEAR = 2012;

    string_trim(date);

    bool is_all_digits = true;

    for (char& c : date)
    {
        if (c < 0)
            return false;

        c = (char)tolower((uint8_t)c);
        
        if (!isdigit(c))
            is_all_digits = false;
    }

    if (is_all_digits)
    {
        // Handle most common, simplest cases.
        if (date.size() == 6)
        {
            // YYMMDD
            int year = convert_hex_digit(date[0]) * 10 + convert_hex_digit(date[1]);
            int month = convert_hex_digit(date[2]) * 10 + convert_hex_digit(date[3]);
            int day = convert_hex_digit(date[4]) * 10 + convert_hex_digit(date[5]);

            if (year <= 8)
                year += 2000;
            else
                year += 1900;

            if (month > 12)
                return false;

            if (day > 31)
                return false;

            begin_date.m_year = year;
            if (month != 0)
            {
                begin_date.m_month = month ? month : -1;

                if (day != 0)
                    begin_date.m_day = day;
            }
            else
            {
                if (day != 0)
                    return false;
            }
            return true;
        }
        else if (date.size() == 8)
        {
            // YYYYMMDD
            int year = convert_hex_digit(date[0]) * 1000 + convert_hex_digit(date[1]) * 100 + convert_hex_digit(date[2]) * 10 + convert_hex_digit(date[3]);
            int month = convert_hex_digit(date[4]) * 10 + convert_hex_digit(date[5]);
            int day = convert_hex_digit(date[6]) * 10 + convert_hex_digit(date[7]);

            if ((year < MIN_YEAR) || (year > MAX_YEAR))
                return false;

            if (month > 12)
                return false;

            if (day > 31)
                return false;

            if (month == 0)
            {
                if (day != 0)
                    return false;

                begin_date.m_year = year;
            }
            else if (day == 0)
            {
                begin_date.m_year = year;
                begin_date.m_month = month ? month : -1;
            }
            else
            {
                begin_date.m_year = year;
                begin_date.m_month = month ? month : -1;
                begin_date.m_day = day;
            }
            return true;
        }
        else
            return false;
    }

    // Tokenize the input then only parse those cases we explictly support. Everything else is an error.

    std::vector<int64_t> tokens;
    std::vector<int> digits;

    uint32_t special_flags = 0, cur_ofs = 0;
        
    while (cur_ofs < date.size())
    {
        if (isdigit(date[cur_ofs]))
        {
            int64_t val = 0;
            int num_digits = 0;

            do 
            {
                if (!isdigit(date[cur_ofs]))
                    break;

                val = val * 10 + convert_hex_digit(date[cur_ofs++]);
                if (val < 0)
                    return false;

                num_digits++;

            } while (cur_ofs < date.size());

            tokens.push_back(val);
            digits.push_back(num_digits);
        }
        else if (date[cur_ofs] == ' ')
        {
            cur_ofs++;
        }
        else
        {
            std::string cur_str(date.c_str() + cur_ofs);

            int phrase_index;
            for (phrase_index = 0; phrase_index < NUM_SPECIAL_PHRASES; phrase_index++)
                if (string_begins_with(cur_str, g_special_phrases[phrase_index].m_pStr))
                    break;

            if (phrase_index == NUM_SPECIAL_PHRASES)
                return false;

            tokens.push_back(-(phrase_index + 1));
            digits.push_back(0);
            
            cur_ofs += (uint32_t)strlen(g_special_phrases[phrase_index].m_pStr);

            special_flags |= g_special_phrases[phrase_index].m_flag;
        }
    }

    assert(tokens.size() == digits.size());

    // Just not supporting slashes in here.
    if (special_flags & cSlashFlag)
        return false;

    if (!tokens.size())
        return false;

    // First token must be a number
    if ((digits[0] != 2) && (digits[0] != 4) && (digits[0] != 6) && (digits[0] != 8))
        return false;
                    
    if (special_flags & cSeasonFlag)
    {
        // Either YYSeason or YYYYSeason

        if (tokens.size() != 2)
            return false;

        int year = 0;
        if (digits[0] == 2)
            year = (int)tokens[0] + 1900;
        else if (digits[0] == 4)
        {
            year = (int)tokens[0];
            if ((year < MIN_YEAR) || (year > MAX_YEAR))
                return false;
        }
        else
            return false;

        begin_date.m_year = year;
        
        if (tokens[1] >= 0)
            return false;
        
        int64_t special_index = -tokens[1] - 1;
        if (special_index >= NUM_SPECIAL_PHRASES)
            return false;

        begin_date.m_prefix = g_special_phrases[special_index].m_date_prefix;
        
        return true;
    }
    else if (special_flags & cMonthFlag)
    {
        // Not supporting explicit month
        return false;
    }
    
    // No explicit season and no month - handle XX's
    if ((tokens.size() == 2) && (tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialXX))
    {
        if (digits[0] == 4)
        {
            // YYMMXX 
            int year = 1900 + (int)(tokens[0] / 100);
            int month = (int)(tokens[0] % 100);

            if (month > 12)
                return false;

            begin_date.m_year = year;
            begin_date.m_month = month ? month : -1;
        }
        else if (digits[0] == 6)
        {
            // YYYYMMXX 

            int year = (int)(tokens[0] / 100);
            if ((year < MIN_YEAR) || (year > MAX_YEAR))
                return false;

            int month = (int)(tokens[0] % 100);
            if (month > 12)
                return false;

            begin_date.m_year = year;
            begin_date.m_month = month ? month : -1;
        }
        else
        {
            return false;
        }

        return true;
    }
    else if ((tokens.size() == 3) && (tokens[1] < 0) && (tokens[2] < 0) && (get_special_from_token(tokens[1]) == cSpecialXX) && (get_special_from_token(tokens[2]) == cSpecialXX))
    {
        if (digits[0] == 2)
        {
            // YYXXXX
            begin_date.m_year = (int)tokens[0] + 1900;
        }
        else if (digits[0] == 4)
        {
            // YYYYXXXX
            begin_date.m_year = (int)tokens[0];
            if ((begin_date.m_year < MIN_YEAR) || (begin_date.m_year > MAX_YEAR))
                return false;
        }
        else
            return false;

        return true;
    }

    if (special_flags & cXXFlag)
        return false;

    if (digits[0] == 2)
    {
        // YY
        begin_date.m_year = (int)tokens[0] + 1900;
    }
    else if (digits[0] == 4)
    {
        // YYMM
        begin_date.m_year = (int)(tokens[0] / 100) + 1900;
        
        begin_date.m_month = (int)(tokens[0] % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;
    }
    else if (digits[0] == 6)
    {
        // YYMMDD
        begin_date.m_year = (int)(tokens[0] / 10000) + 1900;
        
        begin_date.m_month = (int)((tokens[0] / 100) % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;

        begin_date.m_day = (int)(tokens[0] % 100);
        if (begin_date.m_day >= 31)
            return false;

        if ((begin_date.m_month == -1) && (begin_date.m_day))
            return false;
    }
    else if (digits[0] == 8)
    {
        // YYYYMMDD
        begin_date.m_year = (int)(tokens[0] / 10000);
        if ((begin_date.m_year < MIN_YEAR) || (begin_date.m_year > MAX_YEAR))
            return false;

        begin_date.m_month = (int)((tokens[0] / 100) % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;

        begin_date.m_day = (int)(tokens[0] % 100);
        if (begin_date.m_day >= 31)
            return false;

        if ((begin_date.m_month == -1) && (begin_date.m_day))
            return false;
    }
    else
    {
        return false;
    }

    if ((tokens.size() == 2) && (tokens[1] < 0) && 
        ( (get_special_from_token(tokens[1]) >= cSpecialLate) && (get_special_from_token(tokens[1]) <= cSpecialEnd) ||
          (get_special_from_token(tokens[1]) == cSpecialMid) )
        )
    {
        // 2 tokens, ends in "late", "middle", "early" etc.
        if (begin_date.m_day != -1)
            return false;

        if (tokens[1] >= 0)
            return false;

        int64_t special_index = -tokens[1] - 1;
        if (special_index >= NUM_SPECIAL_PHRASES)
            return false;
        
        begin_date.m_prefix = g_special_phrases[special_index].m_date_prefix;
        
        return true;
    }

    if (special_flags & cBeginToLateFlag)
        return false;

    if ((tokens.size() == 3) && (tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialDash) && (tokens[2] >= 0))
    {
        if ((digits[0] == 6) && (digits[2] == 2))
        {
            // YYMMDD-DD
            end_date = begin_date;

            if (tokens[2] > 31)
                return false;

            end_date.m_day = (int)tokens[2];
            
            return true;
        }
        else if ((digits[0] == 4) && (digits[2] == 4))
        {
            // YYMM-YYMM

            end_date.m_year = (int)(tokens[2] / 100) + 1900;

            end_date.m_month = (int)(tokens[2] % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 6))
        {
            // YYMMDD-YYMMDD
            end_date.m_year = (int)(tokens[2] / 10000) + 1900;

            end_date.m_month = (int)((tokens[2] / 100) % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            end_date.m_day = (int)(tokens[2] % 100);
            if (end_date.m_day >= 31)
                return false;

            return true;
        }
        else if ((digits[0] == 8) && (digits[2] == 8))
        {
            // YYYYMMDD-YYYYMMDD
            end_date.m_year = (int)(tokens[2] / 10000);
            if ((end_date.m_year < MIN_YEAR) || (end_date.m_year > MAX_YEAR))
                return false;

            end_date.m_month = (int)((tokens[2] / 100) % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            end_date.m_day = (int)(tokens[2] % 100);
            if (end_date.m_day >= 31)
                return false;

            if ((end_date.m_month == -1) && (end_date.m_day))
                return false;

            return true;
        }
        else
        {
            return false;
        }
    }

    if (special_flags & cDashFlag)
        return false;

    if ((tokens.size() == 2) && (get_special_from_token(tokens[1]) == cSpecialFuzzy))
    {
        begin_date.m_fuzzy = true;
        return true;
    }

    if (special_flags & cFuzzyFlag)
        return false;

    if ((tokens.size() == 3) && (get_special_from_token(tokens[1]) == cSpecialOr))
    {
        if ((digits[0] == 2) && (digits[2] == 2))
        {
            // YY or YY
            alt_date.m_year = (int)tokens[2] + 1900;
            
            return true;
        }
        else if ((digits[0] == 4) && (digits[2] == 4))
        {
            // YYMM or YYMM
            alt_date.m_year = (int)(tokens[2] / 100) + 1900;

            alt_date.m_month = (int)(tokens[2] % 100);
            if (alt_date.m_month > 12)
                return false;
            else if (!alt_date.m_month)
                alt_date.m_month = -1;

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 2))
        {
            // YYMMDD or DD
            alt_date = begin_date;

            if (tokens[2] > 31)
                return false;

            alt_date.m_day = (int)tokens[2];

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 6))
        {
            // YYMMDD or YYMMDD
            alt_date.m_year = (int)(tokens[2] / 10000) + 1900;

            alt_date.m_month = (int)((tokens[2] / 100) % 100);
            if (alt_date.m_month > 12)
                return false;
            else if (!alt_date.m_month)
                return false;

            alt_date.m_day = (int)(tokens[2] % 100);
            if (alt_date.m_day >= 31)
                return false;

            return true;
        }
        else
        {
            return false;
        }
    }

    if (special_flags & cOrFlag)
        return false;

    if ( (tokens.size() == 2) && ((tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialApprox)) )
    {
        begin_date.m_approx = true;
        return true;
    }

    if (special_flags & cApproxFlag)
        return false;

    if (tokens.size() > 1)
        return false;
            
    return true;
}

const int NICAP_FIRST_CAT = 1;
const int NICAP_LAST_CAT = 11;

static const char* g_nicap_categories[11] =
{
    "01 - Distant Encounters",
    "02 - Close Encounters",
    "03 - EME Cases",
    "04 - Animal Reactions",
    "05 - Medical Incidents",
    "06 - Trace Cases",
    "07 - Entity Cases",
    "08 - Photographic Cases",
    "09 - RADAR Cases",
    "10 - Nuclear Connection",
    "11 - Aviation Cases"
};

static const char* g_nicap_archive_urls[] =
{
    "040228USS%5FSupplydir.htm",
    "421019guadalcanaldir.htm",
    "4409xxoakridge%5Fdir.htm",
    "470116northseadir.htm",
    "470117northseadir.htm",
    "470828japandir.htm",
    "490723delphidir.htm",
    "500110lasvegasdir.htm",
    "500227coultervilledir.htm",
    "500309selfridgedir.htm",
    "500322%5F4925dir.htm",
    "500529mtvernondir.htm",
    "510709dir.htm",
    "520425dir.htm",
    "520501georgedir.htm",
    "520512roswelldir.htm",
    "520526koreadir.htm",
    "520619goosedir.htm",
    "520719wnsdir.htm",
    "520723jamestown%5F01%5Fdir.htm",
    "520723jamestown%5F02%5Fdir.htm",
    "520729dir.htm",
    "520729langley%5Fdir.htm",
    "5207xxdir.htm",
    "520807sanantondir.htm",
    "530213carswelldir.htm",
    "530416eprairiedir.htm",
    "530618iwodir.htm",
    "531016presqudir.htm",
    "531228marysvilledir.htm",
    "571102level%5Fdir.htm",
    "571106b.htm",
    "571110dir.htm",
    "580505dir.htm",
    "640629dir.htm",
    "640904dir.htm",
    "650916dir.htm",
    "660314dir.htm",
    "730202dir.htm",
    "731120mtvernon%5Fdir.htm",
    "770701avianodir.htm",
    "7710xxfostoriadir.htm",
    "801227rendledir.htm",
    "821004ukraine%5Fdir.htm",
    "861117alaskadir.htm",
    "870823dir.htm",
    "aca%5F540621.htm",
    "adickesdir.htm",
    "alamo670302dir.htm",
    "alamos500225Bdir.htm",
    "alamos520722dir.htm",
    "albany520226dir.htm",
    "albuq490106dir.htm",
    "albuq510825dir.htm",
    "albuq520510dir.htm",
    "albuq520528dir.htm",
    "ar-570730dir.htm",
    "ar-641030dir.htm",
    "ar-770126.htm",
    "ar-hubbarddir.htm",
    "arlington470707dir.htm",
    "ashley480408dir.htm",
    "balt520721dir.htm",
    "barradir.htm",
    "benson520403dir.htm",
    "bermuda490124dir.htm",
    "blackstone480724docs1.htm",
    "bonlee501023dir.htm",
    "candir.htm",
    "canogapark571111dir.htm",
    "cashlandir.htm",
    "cavecreek471014dir.htm",
    "chamble480726docs1.htm",
    "colosprings520709dir.htm",
    "columbus520618dir.htm",
    "compass.htm",
    "contrexeville761214dir.htm",
    "cortez490127dir.htm",
    "costarica1dir.htm",
    "coynedir.htm",
    "daggett.htm",
    "dayton471020dir.htm",
    "dayton520712dir.htm",
    "dillon490403dir.htm",
    "dmonthan490516dir.htm",
    "dodgeville4710xxdir.htm",
    "elko490502dir.htm",
    "ellsworth.htm",
    "en-590626dir.htm",
    "f-86dir.htm",
    "fairchild520120dir.htm",
    "fayette491228dir.htm",
    "fortdixdir.htm",
    "france57.htm",
    "ftbliss490519dir.htm",
    "ftmyers501206dir.htm",
    "ftrich470917dir.htm",
    "ftsumner470710dir.htm",
    "fulda520602dir.htm",
    "george520501dir.htm",
    "glenburnie520329dir.htm",
    "gormandir.htm",
    "greenville480419dir.htm",
    "harmondir.htm",
    "hawaii.htm",
    "hawesville870722dir.htm",
    "hickam490104dir.htm",
    "holloman491213dir.htm",
    "hood490427dir.htm",
    "houston520520dir.htm",
    "hynekrv6.htm",
    "indy480731dir.htm",
    "keesler520507dir.htm",
    "keywest501114dir.htm",
    "kirtland500321dir.htm",
    "kirtland800809dir.htm",
    "knoxville500301dir.htm",
    "lmeade520402dir.htm",
    "longmead520417dir.htm",
    "louisville520615dir.htm",
    "lubbock520802dir.htm",
    "lukedir.htm",
    "lynn520701dir.htm",
    "mainbrace.htm",
    "malmstrom75dir.htm",
    "maple480829dir.htm",
    "marshall520429dir.htm",
    "mcchord520617dir.htm",
    "meromdir.htm",
    "minn.htm",
    "minot681028dir.htm",
    "mobile520722dir.htm",
    "mongodir.htm",
    "monroe480528dir.htm",
    "moses520501dir.htm",
    "mtvernon.htm",
    "mufon%5Farg0809%5F83.htm",
    "mufon%5Fau0513%5F83.htm",
    "mufon%5Fau0520%5F83.htm",
    "mufon%5Fau0722%5F83.htm",
    "mufon%5Fbo0302%5F83.htm",
    "mufon%5Fbr0612%5F83.htm",
    "mufon%5Fgb0119%5F83.htm",
    "mufon%5Fgb0315%5F83.htm",
    "mufon%5Fgb0812%5F83.htm",
    "mufon%5Fmx0000%5F83.htm",
    "mufon%5Fru0714%5F83.htm",
    "mufon%5Fru0826%5F83.htm",
    "musk5307XXdir.htm",
    "naha520422dir.htm",
    "nahant520723dir.htm",
    "natcity520513dir.htm",
    "ncp-oakridge501013.htm",
    "nederland660206dir.htm",
    "nellisdir.htm",
    "nkorea520526dir.htm",
    "norwooddir.htm",
    "oakridge501105dir.htm",
    "oakridge511207dir.htm",
    "ontario520412dir.htm",
    "osceola520725dir.htm",
    "paintsville020114dir.htm",
    "palomar491014dir.htm",
    "palomar491125dir.htm",
    "phoenix520405dir.htm",
    "phoenix520808dir.htm",
    "placerville470814dir.htm",
    "plymouth500427dir.htm",
    "pontiac520427dir.htm",
    "portagedir.htm",
    "portland470704dir.htm",
    "pottsdown520723dir.htm",
    "ramore530630dir.htm",
    "rd-750503dir.htm",
    "reno490905dir.htm",
    "reseda570328dir.htm",
    "rockville520722dir.htm",
    "rogue490524dir.htm",
    "roseville520427dir.htm",
    "roswell520217dir.htm",
    "russia551014dir.htm",
    "sanacacia480717dir.htm",
    "sandiego521217dir.htm",
    "sands480405.htm",
    "sands490424dir.htm",
    "sanmarcos520721dir.htm",
    "savannah520510dir.htm",
    "selfridge511124dir.htm",
    "sharondir.htm",
    "shreve520416dir.htm",
    "springer490425dir.htm",
    "stmaries470703dir.htm",
    "sts80.htm",
    "tehrandir.htm",
    "temple520406dir.htm",
    "terredir.htm",
    "tucson490428dir.htm",
    "ubatubadir.htm",
    "vaughn47latedir.htm",
    "vaughn481103dir.htm",
    "vincennes.htm",
    "walnutlake520525dir.htm",
    "washington520713dir.htm",
    "wsands2dir.htm",
    "yuma520417dir.htm",
    "yuma520427dir.htm",
    "zamoradir.htm",
    "charleston1980.htm"
};

const uint32_t NUM_NICAP_ARCHIVE_URLS = sizeof(g_nicap_archive_urls) / sizeof(g_nicap_archive_urls[0]);

static bool convert_nicap()
{
    string_vec lines;
    bool utf8_flag = false;
    
    if (!read_text_file("nicap1_150.md", lines, true, &utf8_flag))
        panic("Failed reading input file");
    if (!utf8_flag)
        panic("Expecting utf8 data");

    if (!read_text_file("nicap151_334.md", lines, true, &utf8_flag))
        panic("Failed reading input file");
    if (!utf8_flag)
        panic("Expecting utf8 data");

    string_vec new_lines;
    for (std::string& str : lines)
        if (str.size())
            new_lines.push_back(str);

    lines.swap(new_lines);
    if (!lines.size())
        panic("File empty");

    json js_doc = json::object();
    js_doc["NICAP Data"] = json::array();

    auto &js_doc_array = js_doc["NICAP Data"];

    std::string prev_date, prev_orig_desc;
    
    uint32_t num_repeated_recs = 0;
                
    uint32_t record_index = 0;
    uint32_t cur_line_index = 0;
    while (cur_line_index < lines.size())
    {
        std::string date(lines[cur_line_index]);
        if (!date.size())
            panic("Invalid date");

        cur_line_index++;

        std::string ref;
        
        if (date.front() == '[')
        {
            ref = date;

            markdown_text_processor mtp;
            mtp.init_from_markdown(ref.c_str());
            
            mtp.fix_redirect_urls();

            if (mtp.m_links.size())
            {
                for (uint32_t i = 0; i < mtp.m_links.size(); i++)
                {
                    std::string& s = mtp.m_links[i];
                    uint32_t j;
                    for (j = 0; j < NUM_NICAP_ARCHIVE_URLS; j++)
                        if (string_find_first(s, g_nicap_archive_urls[j]) != -1)
                            break;
                    
                    if (j < NUM_NICAP_ARCHIVE_URLS)
                        mtp.m_links[i] = "https://web.archive.org/web/100/" + mtp.m_links[i];
                }
            }

            ref.clear();
            mtp.convert_to_markdown(ref, true);

            // 6+2+2+1 chars
            if (date.size() < 11)
                panic("Invalid date");

            date.erase(0, 1);

            uint32_t i;
            bool found_end_bracket = false;
            for (i = 0; i < date.size(); i++)
            {
                if (date[i] == ']')
                {
                    date.erase(i, date.size() - i);
                    found_end_bracket = true;
                    break;
                }
            }

            if (!found_end_bracket)
                panic("Invalid date");
        }

        event_date begin_date, end_date, alt_date;

        bool status = convert_nipcap_date(date, begin_date, end_date, alt_date);
        if (!status)
            panic("Failed converting NICAP date");

        // Get first line of city
        if (cur_line_index == lines.size())
            panic("Out of lines");

        std::string city(lines[cur_line_index++]);
        if (!city.size() || (city == "* * *"))
            panic("Expected city");
        
        if (cur_line_index == lines.size())
            panic("Failed parsing NICAP data");

        // Look ahead for category, 1-11
        int cat_index = -1;
        int cat_line_index = -1;

        for (uint32_t i = 0; i < 5; i++)
        {
            if ((cur_line_index + i) >= lines.size())
                break;

            const std::string& s = lines[cur_line_index + i];
            if ( (s.size() <= 2) && (string_is_digits(s)) )
            {
                int val = atoi(s.c_str());
                if ((val >= NICAP_FIRST_CAT) && (val <= NICAP_LAST_CAT))
                {
                    cat_index = val;
                    cat_line_index = cur_line_index + i;
                    break;
                }
            }
        }
        if (cat_index == -1)
            panic("Can't find category");

        for (int i = cur_line_index; i < (cat_line_index - 1); i++)
            city = combine_strings(city, lines[i]);
        
        // Get state or country, which is right before the category
        std::string state_or_country(lines[cat_line_index - 1]);
        if (!state_or_country.size() || (state_or_country == "* * * "))
            panic("Expected state or country");
                        
        if (cur_line_index == lines.size())
            panic("Failed parsing NICAP data");

        cur_line_index = cat_line_index + 1;
                        
        string_vec rec;

        while (cur_line_index < lines.size())
        {
            const std::string& str = lines[cur_line_index];
            
            if (str == "* * *")
            {
                cur_line_index++;

                break;
            }
            else if (str.size() && str[0] == '[')
                break;

            if ((str.size() >= 2) && isdigit((uint8_t)str[0]) && isdigit((uint8_t)str[1]))
            {
                event_date next_begin_date, next_end_date, next_alt_date;
                bool spec_status = convert_nipcap_date(str, next_begin_date, next_end_date, next_alt_date);
                if (spec_status)
                    break;
            }

            rec.push_back(str);
            cur_line_index++;
        }

        if (!rec.size())
            panic("Failed parsing record");

        std::string code, bb;
        int rating = -1;
        bool nc_flag = false, lc_flag = false;
               
        for (int i = 0; i < std::min(5, (int)rec.size()); i++)
        {
            if (rec[i] == "NC")
            {
                nc_flag = true;
                rec.erase(rec.begin() + i);
                break;
            }
        }

        for (int i = 0; i < std::min(5, (int)rec.size()); i++)
        {
            if (rec[i] == "LC")
            {
                lc_flag = true;
                rec.erase(rec.begin() + i);
                break;
            }
        }

        if (!rec.size())
            panic("Invalid record");
                
        if (string_is_alpha(rec[0]) && (rec[0].size() <= 2))
        {
            if ((rec[0] == "A") || (rec[0] == "R") || (rec[0] == "En") || (rec[0] == "An") || (rec[0] == "C") ||
                (rec[0] == "E") || (rec[0] == "D") || (rec[0] == "M") || (rec[0] == "I") || (rec[0] == "U") || (rec[0] == "N") || (rec[0] == "T") || (rec[0] == "aM"))
            {
                code = rec[0];
                rec.erase(rec.begin());
                
                if (!rec.size())
                    panic("Invalid record");
            }
            else
            {
                panic("Unknown code");
            }
        }

        int i, j = std::min(5, (int)rec.size());
        for (i = 0; i < j; i++)
            if ((rec[i] == "BBU") || (rec[i] == "BB"))
                break;

        bool found_bbu = false;
        if (i < j)
        {
            bb = rec[i];
            rec.erase(rec.begin() + i);

            if (!rec.size())
                panic("Invalid record");

            found_bbu = true;
        }

        if (string_is_digits(rec[0]) && (rec[0].size() == 1))
        {
            int val = atoi(rec[0].c_str());
            if ((val < 0) || (val > 5))
                panic("Invalid rating");
            rating = val;

            rec.erase(rec.begin());

            if (!rec.size())
                panic("Invalid record");
        }

        if (!found_bbu && string_is_digits(rec[0]) && (rec[0].size() >= 2))
        {
            int val = atoi(rec[0].c_str());
            if ((val >= 12) && (val <= 12607))
            {
                bb = rec[0];

                rec.erase(rec.begin());

                if (!rec.size())
                    panic("Invalid record");
            }
            else
            {
                panic("Bad BB #");
            }
        }

        json js;
        js["date"] = begin_date.get_string();

        if (end_date.is_valid())
            js["end_date"] = end_date.get_string();

        if (alt_date.is_valid())
            js["alt_date"] = alt_date.get_string();

        js["ref"] = json::array();
        if (ref.size())
            js["ref"].push_back("NICAP: " + ref);
        js["ref"].push_back("[NICAP Listing by Date PDF](http://www.nicap.org/NSID/NSID_DBListingbyDate.pdf)");

        if (state_or_country.size())
            js["location"] = city + ", " + state_or_country;
        else
            js["location"] = city;

        std::string desc;
        for (const std::string& s : rec)
            desc = combine_strings(desc, s);

        assert(cat_index >= 1 && cat_index <= 11);
        
        const std::string orig_desc(desc);

        desc = desc + string_format(" (NICAP: %s", g_nicap_categories[cat_index - 1]);
        if (code.size())
            desc = desc + string_format(", Code: %s", code.c_str());
        if (rating != -1)
            desc = desc + string_format(", Rating: %i", rating);
        if (bb.size())
            desc = desc + string_format(", BB: %s", bb.c_str());
        if (nc_flag)
            desc = desc + string_format(", NC");
        if (lc_flag)
            desc = desc + string_format(", LC");
        desc += ")";

        if ((desc[0] >= 0) && islower((uint8_t)desc[0]))
            desc[0] = (char)toupper((uint8_t)desc[0]);

        js["desc"] = desc;
        js["source"] = "NICAP_DB";
        js["source_id"] = "NICAP_DB" + string_format("_%u", record_index);

        if ((prev_orig_desc.size()) && (orig_desc == prev_orig_desc) && (js["date"] == prev_date))
        {
            // It's a repeated record, with just a different category. 
            std::string new_desc(js_doc_array.back()["desc"]);

            new_desc += string_format(" (NICAP: %s)", g_nicap_categories[cat_index - 1]);

            js_doc_array.back()["desc"] = new_desc;

            num_repeated_recs++;
        }
        else
        {
            prev_date = js["date"];
            prev_orig_desc = orig_desc;
            
            js_doc_array.push_back(js);
        }
                
#if 0
        uprintf("Record: %u\n", record_index);

        uprintf("Date string: %s\n", date.c_str());

        uprintf("Date: %s\n", begin_date.get_string().c_str());

        if (end_date.is_valid())
            uprintf("End date: %s\n", end_date.get_string().c_str());

        if (alt_date.is_valid())
            uprintf("Alt date: %s\n", alt_date.get_string().c_str());

        if (ref.size())
            uprintf("Ref: %s\n", ref.c_str());

        uprintf("City: %s\n", city.c_str());
        uprintf("State or County: %s\n", state_or_country.c_str());

        uprintf("Category: %i\n", cat_index);

        if (code.size())
            uprintf("NICAP code: %s\n", code.c_str());
        if (rating != -1)
            uprintf("NICAP Rating: %u\n", rating);
        if (bb.size())
            uprintf("NICAP BB: %s\n", bb.c_str());
        if (nc_flag || lc_flag)
            uprintf("NICAP nc_flag: %u lc_flag: %u\n", nc_flag, lc_flag);

        uprintf("Record:\n");
        for (const std::string& s : rec)
            uprintf("%s\n", s.c_str());

        uprintf("\n");
#endif
                
        record_index++;
    }

    if (!serialize_to_json_file("nicap_db.json", js_doc, true))
        panic("Failed serializing JSON file");

    uprintf("Processed %u records, skipping %u repeated records\n", record_index, num_repeated_recs);

    return true;
}

static void convert_args_to_utf8(string_vec&args, int argc, wchar_t* argv[])
{
    args.resize(argc);

    for (int i = 0; i < argc; i++)
        args[i] = wchar_to_utf8(argv[i]);
}

// Windows defaults to code page 437:
// https://www.ascii-codes.com/
// We want code page 1252
// http://www.alanwood.net/demos/ansi.html

// Main code
int wmain(int argc, wchar_t* argv[])
{
    assert(cTotalPrefixes == sizeof(g_date_prefix_strings) / sizeof(g_date_prefix_strings[0]));

    string_vec args;
    convert_args_to_utf8(args, argc, argv);

    // Set ANSI Latin 1; Western European (Windows) code page for output.
    SetConsoleOutputCP(1252);
    
    tests();

#if 0
    std::vector<string_vec> rows;
    std::string title;
    string_vec col_titles;
    load_column_text("ufoevid13.txt", rows, title, col_titles);
    return 0;
#endif

    bool utf8_flag = false;
    bool status;
    ufo_timeline timeline;
    status = timeline.load_json("maj2.json", utf8_flag, "Maj2");
    if (!status)
        panic("Failed loading maj2.json");
    for (uint32_t i = 0; i < timeline.size(); i++)
        timeline[i].m_source_id = string_format("%s_%u", timeline[i].m_source.c_str(), i);

#if 1
    uprintf("Convert NICAP:\n");
    if (!convert_nicap())
        panic("convert_nicap() failed!");
    uprintf("Success\n");

    uprintf("Convert Johnson:\n");
    if (!convert_johnson())
        panic("convert_johnson() failed!");
    uprintf("Success\n");

    uprintf("Convert Eberhart:\n");
    if (!convert_eberhart())
        panic("convert_eberthart() failed!");
    uprintf("Success\n");

    uprintf("Convert Trace:\n");
    if (!convert_magnonia("trace.txt", "trace.json", "Trace", " [Trace Cases](https://www.thenightskyii.org/trace.html)"))
        panic("convert_magnonia() failed!");
    uprintf("Success\n");

    uprintf("Convert Magnonia:\n");
    if (!convert_magnonia("magnonia.txt", "magnonia.json"))
        panic("convert_magnonia() failed!");
    uprintf("Success\n");
    
    uprintf("Convert Hall:\n");
    if (!convert_hall())
        panic("convert_hall() failed!");
    uprintf("Success\n");

    uprintf("Convert Bluebook Unknowns:\n");
    if (!convert_bluebook_unknowns())
        panic("convert_bluebook_unknowns failed!");
    uprintf("Success\n");
#endif

    status = timeline.load_json("nicap_db.json", utf8_flag);
    if (!status)
        panic("Failed loading nicap_db.json");
        
    status = timeline.load_json("trace.json", utf8_flag);
    if (!status)
        panic("Failed loading trace.json");

    status = timeline.load_json("magnonia.json", utf8_flag);
    if (!status)
        panic("Failed loading magnolia.json");

    status = timeline.load_json("bb_unknowns.json", utf8_flag);
    if (!status)
        panic("Failed loading bb_unknowns.json");

    status = timeline.load_json("ufo_evidence_hall.json", utf8_flag);
    if (!status)
        panic("Failed loading ufo_evidence_hall.json");

    status = timeline.load_json("eberhart.json", utf8_flag);
    if (!status)
        panic("Failed loading eberhart.json");
        
    status = timeline.load_json("johnson.json", utf8_flag);
    if (!status)
        panic("Failed loading johnson.json");

    for (uint32_t i = 0; i < timeline.size(); i++)
    {
        if (!timeline[i].m_begin_date.sanity_check())
            panic("Date failed sanity check");

        if (timeline[i].m_end_date.is_valid())
        {
            if (!timeline[i].m_end_date.sanity_check())
                panic("Date failed sanity check");
        }

        if (timeline[i].m_alt_date.is_valid())
        {
            if (!timeline[i].m_alt_date.sanity_check())
                panic("Date failed sanity check");
        }

        // roundtrip test
        event_date test_date;

        if (!test_date.parse(timeline[i].m_date_str.c_str()))
            panic("Date failed sanity check");

        if (test_date != timeline[i].m_begin_date)
            panic("Date failed sanity check");

        std::string test_str(timeline[i].m_begin_date.get_string());
        if (test_str != timeline[i].m_date_str)
        {
            fprintf(stderr, "Date failed roundtrip: %s %s %s\n", timeline[i].m_source_id.c_str(), timeline[i].m_date_str.c_str(), test_str.c_str());
            panic("Date failed sanity check");
        }

        if (timeline[i].m_end_date.is_valid())
        {
            if (!test_date.parse(timeline[i].m_end_date_str.c_str()))
                panic("Date failed sanity check");

            if (test_date != timeline[i].m_end_date)
                panic("Date failed sanity check");

            std::string test_str2(timeline[i].m_end_date.get_string());
            if (test_str2 != timeline[i].m_end_date_str)
            {
                fprintf(stderr, "Date failed roundtrip: %s %s %s\n", timeline[i].m_source_id.c_str(), timeline[i].m_end_date_str.c_str(), test_str2.c_str());
                panic("End date failed sanity check");
            }
        }
        else if (timeline[i].m_end_date_str.size())
            panic("Date failed sanity check");

        if (timeline[i].m_alt_date.is_valid())
        {
            if (!test_date.parse(timeline[i].m_alt_date_str.c_str()))
                panic("Date failed sanity check");

            if (test_date != timeline[i].m_alt_date)
                panic("Date failed sanity check");

            std::string test_str3(timeline[i].m_alt_date.get_string());
            if (test_str3 != timeline[i].m_alt_date_str)
            {
                fprintf(stderr, "Date failed roundtrip: %s %s %s\n", timeline[i].m_source_id.c_str(), timeline[i].m_alt_date_str.c_str(), test_str3.c_str());
                panic("Alt date failed sanity check");
            }
        }
        else if (timeline[i].m_alt_date_str.size())
            panic("Date failed sanity check");

    }
        
    uprintf("Load success, %zu total events\n", timeline.get_events().size());

    timeline.sort();
        
    // Write majestic.json, then load it and verify that it saved and loaded correctly.
    {
        timeline.set_name("Majestic Timeline");
        timeline.write_file("majestic.json", true);

        ufo_timeline timeline_comp;
        bool utf8_flag_comp;
        if (!timeline_comp.load_json("majestic.json", utf8_flag_comp))
            panic("Failed loading majestic.json");

        if (timeline.get_events().size() != timeline_comp.get_events().size())
            panic("Failed loading timeline events JSON");

        for (size_t i = 0; i < timeline.get_events().size(); i++)
            if (timeline[i] != timeline_comp[i])
                panic("Failed comparing majestic.json");
    }

    timeline.write_markdown("timeline.md");

    uprintf("Processing successful\n");

    return EXIT_SUCCESS;
}
