// Copyright (C) 2023 Richard Geldreich, Jr.
// markdown_proc.h
#pragma once

#include "utils.h"

#include "libsoldout/markdown.h"

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
        // TODO: not fully supporting lists (here for when we're converting to plain text)
        //panic("unsupported markdown feature");

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
    }

    static void table_cell(struct buf* ob, struct buf* text, int flags, void* opaque)
    {
#if 0
        bufprintf(ob, "table_cell: \"%.*s\" %i ", (int)text->size, text->data, flags);
#endif
        //panic("unsupported markdown feature");

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

    markdown_text_processor();

    void clear();

    void fix_redirect_urls();
    void init_from_markdown(const char* pText);
    bool split_in_half(uint32_t ofs, markdown_text_processor& a, markdown_text_processor& b) const;
    uint32_t count_char_in_text(uint8_t c) const;
    bool split_last_parens(markdown_text_processor& a, markdown_text_processor& b) const;
    void convert_to_plain(std::string& out, bool trim_end) const;
    void convert_to_markdown(std::string& out, bool trim_end) const;

private:
    void ensure_detail_ofs(uint32_t ofs);
    void init_from_codes(const std::string& buf);
    void parse_block(const std::string& buf);
    void handle_html(std::string& out, uint32_t text_ofs) const;
    void handle_emphasis(std::string& out, uint32_t text_ofs, int& emphasis, int& emphasis_amount) const;
};
