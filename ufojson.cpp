// ufojson.cpp - Copyright (C) 2023 Richard Geldreich, Jr.

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

static void panic(const char* pMsg, ...);

//------------------------------------------------------------------

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

static bool read_text_file(const char* pFilename, std::vector<std::string>& lines, bool trim_lines = true, bool *pUTF8_flag = nullptr)
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

static bool write_text_file(const char* pFilename, std::vector<std::string>& lines, bool utf8_bom = true)
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

        std::vector<std::string> m_html;

        int m_link_index;

        char m_emphasis;
        uint8_t m_emphasis_amount;
        bool m_linebreak;
        bool m_end_paragraph;
    };

    std::string m_text;
    std::vector<detail> m_details;
    std::vector<std::string> m_links;

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
    int m_month;
    int m_day;

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
        std::vector<std::string> tokens;

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
        std::vector<std::string> new_tokens;
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
                        if (s.size() != strlen(g_full_months[month]))
                            return false;

                        if (d.m_month != -1)
                            return false;

                        d.m_month = month;
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
    std::vector<std::string> m_type;
    std::vector<std::string> m_refs;
    std::vector<std::string> m_locations;
    std::vector<std::string> m_attributes;
    std::vector<std::string> m_see_also;

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

    void print(FILE *pFile)
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

static void create_json_from_timeline(const std::vector<timeline_event>& timeline_events, const char* pTimeline_name, json& j)
{
    j = json::object();

    j[pTimeline_name] = json::array();

    auto &ar = j[pTimeline_name];

    for (size_t i = 0; i < timeline_events.size(); i++)
    {
        json obj;
        timeline_events[i].to_json(obj);

        ar.push_back(obj);
    }
}

static bool write_json(const char* pFilename, const json& j, bool utf8_bom)
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

// Load a JSON timeline using pjson
static bool load_json1(const char* pFilename, std::vector<timeline_event>& timeline_events, bool &utf8_flag, const char *pSource_override = nullptr)
{
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
    (void)pName;

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
static bool load_json2(const char* pFilename, std::vector<timeline_event>& timeline_events, bool& utf8_flag, const char* pSource_override = nullptr)
{
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

static bool load_json(const char* pFilename, std::vector<timeline_event>& timeline_events, bool& utf8_flag, const char* pSource_override = nullptr)
{
    const size_t timeline_events_size = timeline_events.size();

    if (!load_json2(pFilename, timeline_events, utf8_flag, pSource_override))
        return false;

    std::vector<timeline_event> timeline_events_alt;
    bool utf8_flag_alt;
    if (!load_json1(pFilename, timeline_events_alt, utf8_flag_alt, pSource_override))
        return false;

    if (utf8_flag != utf8_flag_alt)
        panic("Failed loading timeline events JSON");

    if ((timeline_events.size() - timeline_events_size) != timeline_events_alt.size())
        panic("Failed loading timeline events JSON");
    
    for (size_t i = timeline_events_size; i < timeline_events.size(); i++)
        if (timeline_events[i] != timeline_events_alt[i - timeline_events_size])
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
    std::vector<std::string> lines;

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

        std::vector<std::string> desc_lines;
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

        fprintf(pOut_file, "  \"source_id\" : \"%u\",\n", rec_index);
        
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
    std::vector<std::string> lines;
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
    std::vector<std::string> lines;

    if (!read_text_file("bb_unknowns.txt", lines))
        panic("Can't read file bb_unknowns.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("bb_unknowns.json", "w");
    if (!pOut_file)
        panic("Can't open output file bb_unknowns.json");

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
            fprintf(pOut_file, "  \"source_id\" : \"%u\",\n", total_recs);
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
    std::vector<std::string> lines;

    if (!read_text_file("ufo_evidence_hall.txt", lines))
        panic("Can't read file ufo_evidence_hall.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("ufo_evidence_hall.json", "w");
    if (!pOut_file)
        panic("Can't open output file ufo_evidence_hall.json");

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
            fprintf(pOut_file, "  \"source_id\" : \"%u\",\n", total_recs);
            fprintf(pOut_file, "  \"source\" : \"HallUFOEvidence\",\n");
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

static std::string date_to_string(const event_date& date)
{
    if (date.m_year == -1)
        return "";
        
    std::string res;

    if (date.m_prefix != cNoPrefix)
    {
        res = g_date_prefix_strings[date.m_prefix];
        res += " ";
    }

    if (date.m_month == -1)
    {
        assert(date.m_day == -1);

        char buf[256];
        sprintf_s(buf, "%u", date.m_year);
        res += buf;
    }
    else if (date.m_day == -1)
    {
        char buf[256];
        sprintf_s(buf, "%u/%u", date.m_month + 1, date.m_year);
        res += buf;
    }
    else
    {
        char buf[256];
        sprintf_s(buf, "%u/%u/%u", date.m_month + 1, date.m_day, date.m_year);
        res += buf;
    }

    if (date.m_fuzzy)
        res += " (approximate)";

    return res;
}

static bool convert_eberhart()
{
    std::vector<std::string> lines;

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

    std::vector<std::string> trimmed_lines;
    for (uint32_t i = 0; i < lines.size(); i++)
    {
        std::string s(lines[i]);
        string_trim(s);
        if (s.size())
            trimmed_lines.push_back(s);
    }
    lines.swap(trimmed_lines);
                
    uint32_t cur_line = 0;

    std::vector<std::string> new_lines;
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
            begin_date.m_prefix, 1+begin_date.m_month, begin_date.m_day, begin_date.m_year,
            end_date.m_prefix, 1+end_date.m_month, end_date.m_day, end_date.m_year,
            alt_date.m_prefix, 1+alt_date.m_month, alt_date.m_day, alt_date.m_year);

        uprintf("%s  \n\n", desc.c_str());
#endif
        fprintf(pOut_file, "{\n");

        std::string json_date(date_to_string(begin_date)), json_end_date(date_to_string(end_date)), json_alt_date(date_to_string(alt_date));

        fprintf(pOut_file, "  \"date\" : \"%s\",\n", json_date.c_str());

        if (json_end_date.size())
            fprintf(pOut_file, "  \"end_date\" : \"%s\",\n", json_end_date.c_str());

        if (json_alt_date.size())
            fprintf(pOut_file, "  \"alt_date\" : \"%s\",\n", json_alt_date.c_str());
                
        fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(desc).c_str());
        fprintf(pOut_file, "  \"source_id\" : \"%u\",\n", event_num);
        
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

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"Johnson Timeline\" : [\n");

    std::vector<std::string> combined_lines;

    uint32_t total_recs = 0;

    for (uint32_t month = 0; month < 12; month++)
    {
        for (uint32_t day = 0; day < days_in_month[month]; day++)
        {
            char buf[256];
            sprintf_s(buf, "Johnson/%s%02u.txt", g_full_months[month], day + 1);
                        
            std::vector<std::string> lines;

            bool utf8_flag = false;
            bool status = read_text_file(buf, lines, true, &utf8_flag);
            if (!status)
                panic("Can't open file %s\n", buf);

            uprintf("Read file %s %u\n", buf, utf8_flag);

            std::vector<std::string> filtered_lines;
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
                        
            if (lines[2].size() || lines[4].size())
                printf("!");

            lines.erase(lines.begin(), lines.begin() + 5);
                        
            //for (uint32_t i = 0; i < lines.size(); i++)
            //    uprintf("%04u: \"%s\"\n", i, lines[i].c_str());
            //uprintf("\n");

            std::vector<std::string> new_lines;

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

                fprintf(pOut_file, "  \"source_id\" : \"%u\",\n", total_recs);

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
    assert(b.m_prefix == cMiddleOf && b.m_year == 1970 && b.m_month == 11 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("December 16?", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 11 && b.m_day == 16 && b.m_fuzzy);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Fall 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cFall && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 27, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 27);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("March-June", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 2 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 5 && e.m_day == -1);
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
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 2 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("Late January 1970-Summer 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cLate && b.m_year == 1970 && b.m_month == 0 && b.m_day == -1);
    assert(e.m_prefix == cSummer && e.m_year == 1971 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 5 or 6", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 0 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("January 20-25", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 0 && e.m_day == 25);
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
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 1 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("February 25, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 25);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 1 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20 1970-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 0 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 1 && e.m_day == 25);
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

static void convert_args_to_utf8(std::vector<std::string> &args, int argc, wchar_t* argv[])
{
    args.resize(argc);

    for (int i = 0; i < argc; i++)
        args[i] = wchar_to_utf8(argv[i]);
}

// Main code
int wmain(int argc, wchar_t* argv[])
{
    assert(cTotalPrefixes == sizeof(g_date_prefix_strings) / sizeof(g_date_prefix_strings[0]));

    std::vector<std::string> args;
    convert_args_to_utf8(args, argc, argv);

    tests();

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

    std::vector<timeline_event> timeline_events;

    bool utf8_flag = false;
    bool status;
        
    status = load_json("maj2.json", timeline_events, utf8_flag, "Maj2");
    if (!status)
        panic("Failed loading maj2.json");
            
    status = load_json("trace.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading trace.json");

    status = load_json("magnonia.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading magnolia.json");

    status = load_json("bb_unknowns.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading bb_unknowns.json");

    status = load_json("ufo_evidence_hall.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading ufo_evidence_hall.json");

    status = load_json("eberhart.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading eberhart.json");
        
    status = load_json("johnson.json", timeline_events, utf8_flag);
    if (!status)
        panic("Failed loading johnson.json");

    FILE* pOut_file = ufopen("timeline.md", "w");
    if (!pOut_file)
        return EXIT_FAILURE;
        
    putc(UTF8_BOM0, pOut_file);
    putc(UTF8_BOM1, pOut_file);
    putc(UTF8_BOM2, pOut_file);

    uprintf("Load success, %zu total events\n", timeline_events.size());

    std::sort(timeline_events.begin(), timeline_events.end());

    // Write majestic.json, then load it and verify that it saved and loaded correctly.
    {
        json j;
        create_json_from_timeline(timeline_events, "Majestic Timeline", j);
        
        if (!write_json("majestic.json", j, true))
            panic("Failed writing majestic.json!");
        
        std::vector<timeline_event> timeline_events_comp;
        bool utf8_flag_comp;
        if (!load_json("majestic.json", timeline_events_comp, utf8_flag_comp))
            panic("Failed loading majestic.json");

        if (timeline_events.size() != timeline_events_comp.size())
            panic("Failed loading timeline events JSON");

        for (size_t i = 0; i < timeline_events.size(); i++)
            if (timeline_events[i] != timeline_events_comp[i])
                panic("Failed comparing majestic.json");
    }

    std::map<int, int> year_histogram;

    fprintf(pOut_file, "<meta charset=\"utf-8\">\n");

    fprintf(pOut_file, "# UFO Event Timeline v1.04 - Compiled 2/3/2023\n\n");

    fputs(u8"An automated compilation by Richard Geldreich, Jr. using public data from [Dr. Jacques Vallée](https://en.wikipedia.org/wiki/Jacques_Vall%C3%A9e), [Pea Research](https://www.academia.edu/9813787/GOVERNMENT_INVOLVEMENT_IN_THE_UFO_COVER_UP_CHRONOLOGY_based), [George M. Eberhart](http://www.cufos.org/UFO_Timeline.html), [Richard H. Hall](https://en.wikipedia.org/wiki/Richard_H._Hall), \
[Dr. Donald A. Johnson](https://web.archive.org/web/20160821221627/http://www.ufoinfo.com/onthisday/sametimenextyear.html), [Fred Keziah](https://medium.com/@richgel99/1958-keziah-poster-recreation-completed-82fdb55750d8), and [Don Berliner](https://github.com/richgel999/uap_resources/blob/main/bluebook_uncensored_unknowns_don_berliner.pdf).\n\n", pOut_file);

    fputs(u8"Copyrights: George M. Eberhart - Copyright 2022, LeRoy Pea - Copyright 9/8/1988 (updated 3/17/2005), Dr. Donald A. Johnson - Copyright 2012, Fred Keziah - Copyright 1958\n\n", pOut_file);

    fprintf(pOut_file, "## Table of Contents\n\n");

    fprintf(pOut_file, "[Year Histogram](#yearhisto)  \n\n");

    std::set<uint32_t> year_set;
    int min_year = 9999, max_year = -10000;
    for (uint32_t i = 0; i < timeline_events.size(); i++)
    {
        int y;
        year_set.insert(y = timeline_events[i].m_begin_date.m_year);
        min_year = std::min(min_year, y);
        max_year = std::max(max_year, y);
    }

    fprintf(pOut_file, "### Year Shortcuts\n");

    uint32_t n = 0;
    for (int y = min_year; y <= max_year; y++)
    {
        if (year_set.find(y) != year_set.end())
        {
            fprintf(pOut_file, "[%u](#year%u) ", y, y);
            n++;
            if (n == 10)
            {
                fprintf(pOut_file, "  \n");
                n = 0;
            }
        }
    }
    fprintf(pOut_file, "  \n\n");

    fprintf(pOut_file, "## Event Timeline\n");

    int cur_year = -9999;

    for (uint32_t i = 0; i < timeline_events.size(); i++)
    {
        uint32_t hash = crc32((const uint8_t*)timeline_events[i].m_desc.c_str(), timeline_events[i].m_desc.size());
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_year, sizeof(timeline_events[i].m_begin_date.m_year), hash);
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_month, sizeof(timeline_events[i].m_begin_date.m_month), hash);
        hash = crc32((const uint8_t*)&timeline_events[i].m_begin_date.m_day, sizeof(timeline_events[i].m_begin_date.m_day), hash);

        int year = timeline_events[i].m_begin_date.m_year;
        if (year != cur_year)
        {
            fprintf(pOut_file, "## Year: %u\n\n", year);

            fprintf(pOut_file, "### <a name=\"%08X\"></a> <a name=\"year%u\">Event %i (%08X)</a>\n", hash, year, i, hash);
            cur_year = year;
        }
        else
        {
            fprintf(pOut_file, "### <a name=\"%08X\"></a> Event %i (%08X)\n", hash, i, hash);
        }

        timeline_events[i].print(pOut_file);

        year_histogram[year] = year_histogram[year] + 1;

        fprintf(pOut_file, "\n");
    }

    fprintf(pOut_file, "#  <a name=\"yearhisto\">Year Histogram</a>\n");

    for (auto it = year_histogram.begin(); it != year_histogram.end(); ++it)
    {
        fprintf(pOut_file, "* %i, %i\n", it->first, it->second);
    }

    fclose(pOut_file);

    uprintf("Processing successful\n");

    return EXIT_SUCCESS;
}
