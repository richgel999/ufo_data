// Copyright (C) 2023 Richard Geldreich, Jr.
// markdown_proc.cpp
#include "markdown_proc.h"

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
        // TODO: unsupported block quotes (here for when we're converting to plain text)
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
    }

    static void blockhtml(struct buf* ob, struct buf* text, void* opaque)
    {
#if 0
        bufprintf(ob, "blockhtml: \"%.*s\" ", (int)text->size, text->data);
#endif
        // TODO: Not fully supported - just dropping it
        //panic("unsupported markdown feature");

        if (opaque)
            *(bool*)opaque = true;
    }

    static void header(struct buf* ob, struct buf* text, int level, void* opaque)
    {
#if 0
        bufprintf(ob, "header: %i \"%.*s\" ", level, (int)text->size, text->data);
#endif
        // TODO: Not fully supported
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
    }

    static void hrule(struct buf* ob, void* opaque)
    {
        // TODO
        //panic("unsupported markdown feature");
        
        if (opaque)
            *(bool*)opaque = true;
    }

    static void list(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
        // TODO: not fully supporting lists (here for when we're converting to plain text)
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
    }

    static void listitem(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
        // TODO: not fully supporting lists (here for when we're converting to plain text)
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
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
        //panic("unsupported markdown feature");

        // TODO: not fully supported, just for plaintext conversion
        if (opaque)
            *(bool*)opaque = true;
    }

    static void table_cell(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
#if 0
        bufprintf(ob, "table_cell: \"%.*s\" %i ", (int)text->size, text->data, flags);
#endif
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        // TODO: not fully supported, just for plaintext conversion
        if (!text || !text->size)
            return;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeParagraph);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);
    }

    static void table_row(struct buf* ob, struct buf* cells, int flags, void* opaque)
    {
#if 0
        bufprintf(ob, "table_row: \"%.*s\" %i ", (int)cells->size, cells->data, flags);
#endif
        //panic("unsupported markdown feature");
        // TODO: not fully supported, just for plaintext conversion

        if (opaque)
            *(bool*)opaque = true;
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
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        bufputc(ob, (uint8_t)cCodeSig);
        bufputc(ob, (uint8_t)cCodeText);
        writelen(ob, (uint32_t)text->size);
        bufappend(ob, text);

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
        //panic("unsupported markdown feature");
        if (opaque)
            *(bool*)opaque = true;

        if (alt)
        {
            bufputc(ob, (uint8_t)cCodeSig);
            bufputc(ob, (uint8_t)cCodeText);
            writelen(ob, (uint32_t)alt->size);
            bufappend(ob, alt);
        }

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

static struct mkd_renderer g_mkd_parse =
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

markdown_text_processor::markdown_text_processor() : 
    m_used_unsupported_feature(false)
{
}

void markdown_text_processor::clear()
{
    m_used_unsupported_feature = false;
    m_text.clear();
    m_details.clear();
    m_links.clear();
}

void markdown_text_processor::fix_redirect_urls()
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
            if ((c == '%') && ((i + 2) < new_link.size()))
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

void markdown_text_processor::init_from_markdown(const char* pText)
{
    struct buf* pIn = bufnew(4096);
    bufputs(pIn, pText);

    struct buf* pOut = bufnew(4096);
    
    m_used_unsupported_feature = false;
    g_mkd_parse.opaque = &m_used_unsupported_feature;
    markdown(pOut, pIn, &g_mkd_parse);

    std::string buf;
    buf.append((char*)pOut->data, pOut->size);

    init_from_codes(buf);

    bufrelease(pIn);
    bufrelease(pOut);
}

bool markdown_text_processor::split_in_half(uint32_t ofs, markdown_text_processor& a, markdown_text_processor& b) const
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

uint32_t markdown_text_processor::count_char_in_text(uint8_t c) const
{
    uint32_t num = 0;
    for (uint32_t i = 0; i < m_text.size(); i++)
    {
        if ((uint8_t)m_text[i] == c)
            num++;
    }
    return num;
}

bool markdown_text_processor::split_last_parens(markdown_text_processor& a, markdown_text_processor& b) const
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

void markdown_text_processor::convert_to_plain(std::string& out, bool trim_end) const
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

void markdown_text_processor::convert_to_markdown(std::string& out, bool trim_end) const
{
    if (m_used_unsupported_feature)
        printf("markdown_text_processor::convert_to_markdown: Warning, one or more Markdown features were used in this text and won't be losslessly converted.\n");

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

void markdown_text_processor::ensure_detail_ofs(uint32_t ofs)
{
    if (m_details.size() <= ofs)
        m_details.resize(ofs + 1);
}

void markdown_text_processor::init_from_codes(const std::string& buf)
{
    m_text.resize(0);
    m_details.resize(0);
    m_links.resize(0);

    parse_block(buf);
}

void markdown_text_processor::parse_block(const std::string& buf)
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

void markdown_text_processor::handle_html(std::string& out, uint32_t text_ofs) const
{
    // Any HTML appears before this character
    for (uint32_t i = 0; i < m_details[text_ofs].m_html.size(); i++)
        out += m_details[text_ofs].m_html[i];
}

void markdown_text_processor::handle_emphasis(std::string& out, uint32_t text_ofs, int& emphasis, int& emphasis_amount) const
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

#if 0
const char* pText =
u8R"(

<ul>test</ul>

_text1_  
**text2**  
**_text3_**  

![alt text](https://github.com/n48.png "Logo Title")

# Heading 1
## Heading 2
### Heading 3

1. XXXXX  
  1. Item 1
  2. Item 2
2. YYYYY
3. ZZZZZ

| Tables        | Are           | Cool  |
| ------------- |:-------------:| -----:|
| col 3 is      | right-aligned | $1600 |
| col 2 is      | centered      |   $12 |
| zebra stripes | are neat      |    $1 |

* [blahblah](www.blah1.com)
* [blahblah2](www.blah2.com)

`
this is code 1
this is code 2
`

```
this is code 3
this is code 4
```

> blockquote 1  
> blockquote 2

---

* AAA
* BBB
  * ZZZZ1
  * ZZZZ2
* CCC)";

markdown_text_processor tp;
tp.init_from_markdown(pText);

std::string desc;
tp.convert_to_plain(desc, true);

uprintf("%s\n", desc.c_str());

return 0;
#endif
