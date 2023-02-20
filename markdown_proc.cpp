// Copyright (C) 2023 Richard Geldreich, Jr.
// markdown_proc.cpp
#include "markdown_proc.h"

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

markdown_text_processor::markdown_text_processor()
{
}

void markdown_text_processor::clear()
{
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
    markdown(pOut, pIn, &mkd_parse);

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
