// Copyright (C) 2023 Richard Geldreich, Jr.
// markdown_proc.h
#pragma once

#include "utils.h"

#include "libsoldout/markdown.h"

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
    bool m_used_unsupported_feature;

    markdown_text_processor();

    void clear();

    void fix_redirect_urls();
    
    // Note \n escapes will escape "n", not result in a CR.
    void init_from_markdown(const char* pText);
    bool split_in_half(uint32_t ofs, markdown_text_processor& a, markdown_text_processor& b) const;
    uint32_t count_char_in_text(uint8_t c) const;
    bool split_last_parens(markdown_text_processor& a, markdown_text_processor& b) const;
    
    void convert_to_plain(std::string& out, bool trim_end) const;
    
    // Warning: Only a few core features are supported. If after parsing m_used_unsupported_feature is true, then this will not be lossless.
    void convert_to_markdown(std::string& out, bool trim_end) const;

private:
    void ensure_detail_ofs(uint32_t ofs);
    void init_from_codes(const std::string& buf);
    void parse_block(const std::string& buf);
    void handle_html(std::string& out, uint32_t text_ofs) const;
    void handle_emphasis(std::string& out, uint32_t text_ofs, int& emphasis, int& emphasis_amount) const;
};
