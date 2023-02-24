// Copyright (C) 2023 Richard Geldreich, Jr.
#include "udb.h"

#include "udb_tables.h"

const uint32_t UDB_RECORD_SIZE = 112;
const uint32_t UDB_REC_TEXT_SIZE = 78;

enum
{
    cFlagMAP, cFlagGND, cFlagCST, cFlagSEA, cFlagAIR, cFlagObsMIL, cFlagObsCIV, cFlagHQO,	// loc/obs flags
    cFlagSCI, cFlagTLP, cFlagNWS, cFlagMID, cFlagHOX, cFlagCNT, cFlagODD, cFlagWAV, // misc flags
    cFlagSCR, cFlagCIG, cFlagDLT, cFlagNLT, cFlagPRB, cFlagFBL, cFlagSUB, cFlagNFO,	// type of ufo craft flags
    cFlagOID, cFlagRBT, cFlagPSH, cFlagMIB, cFlagMON, cFlagGNT, cFlagFIG, cFlagNOC,	// aliens monsters flags
    cFlagOBS, cFlagRAY, cFlagSMP, cFlagMST, cFlagABD, cFlagOPR, cFlagSIG, cFlagCVS,	// apparent ufo occupant activities flags
    cFlagNUC, cFlagDRT, cFlagVEG, cFlagANI, cFlagHUM, cFlagVEH, cFlagBLD, cFlagLND,	// places visited and things affected flags
    cFlagPHT, cFlagRDR, cFlagRDA, cFlagEME, cFlagTRC, cFlagTCH, cFlagHST, cFlagINJ,	// evidence and special effects flags
    cFlagMIL, cFlagBBK, cFlagGSA, cFlagOGA, cFlagSND, cFlagODR, cFlagCOV, cFlagCMF,	// misc details flags

    cTotalFlags = 64
};

#pragma pack(push, 1)
struct udb_rec
{
private:
    int16_t m_year;

    uint8_t m_unknown_and_locale;   // nibbles
    uint8_t m_unknown_and_month;    // nibbles
    uint8_t m_ref_index_high_day;   // 3 bits ref index high, low 5 bits day

    uint8_t m_time;
    uint8_t m_ymdt;                 // 2-bit fields: TDMY accuracy, T lowest, 0=invalid, 1=?, 2=~, 3=accurate
    uint8_t m_duration;
    uint8_t m_unknown1;

    int16_t m_enc_longtitude;
    int16_t m_enc_latitude;

    int16_t m_elevation;
    int16_t m_rel_altitude;

    uint8_t m_unknown2;
    uint8_t m_continent_country;    // nibbles

    uint8_t m_state_or_prov[3];

    uint8_t m_unknown3;

#if 0
    uint8_t m_loc_flags;
    uint8_t m_misc_flags;
    uint8_t m_type_of_ufo_craft_flags;
    uint8_t m_aliens_monsters_flags;
    uint8_t m_apparent_ufo_occupant_activities_flags;
    uint8_t m_places_visited_and_things_affected_flags;
    uint8_t m_evidence_and_special_effects_flags;
    uint8_t m_miscellaneous_details_flags;
#else
    uint8_t m_flags[8];
#endif

    uint8_t m_text[UDB_REC_TEXT_SIZE];

    uint8_t m_reference;
    uint8_t m_ref_index;
    uint8_t m_strangeness_credibility;    // nibbles

public:
    const uint8_t* get_text() const { return m_text; }

    int get_year() const { return m_year; }

    uint32_t get_month() const { return m_unknown_and_month & 0xF; }
    uint32_t get_day() const { return m_ref_index_high_day & 31; }

    // meters
    int get_elevation() const { return m_elevation; }
    int get_rel_altitude() const { return m_rel_altitude; }

    uint32_t get_strangeness() const { return m_strangeness_credibility >> 4; }
    uint32_t get_credibility() const { return m_strangeness_credibility & 0xF; }

    uint32_t get_reference() const { return m_reference; }
    uint32_t get_reference_index() const { return m_ref_index | ((m_ref_index_high_day >> 5) << 8); }

    uint32_t get_continent_code() const { return m_continent_country >> 4; }
    uint32_t get_country_code() const { return m_continent_country & 0xF; }

    uint32_t get_locale() const { return m_unknown_and_locale & 0xF; }

    std::string get_state_or_prov() const
    {
        const uint32_t c0 = m_state_or_prov[0];
        const uint32_t c1 = m_state_or_prov[1];
        const uint32_t c2 = m_state_or_prov[2];

        return dos_to_utf8(string_format("%c%c%c", (c0 >= ' ') ? c0 : ' ', (c1 >= ' ') ? c1 : ' ', (c2 >= ' ') ? c2 : ' '));
    }

    double get_latitude() const { return ((double)m_enc_latitude / 200.0f) * 1.11111111111f; }
    double get_longitude() const { return -((double)m_enc_longtitude / 200.0f) * 1.11111111111f; }

    std::string get_latitude_dms() const { double lat = get_latitude(); return get_deg_to_dms(lat) + ((lat <= 0) ? " S" : " N"); }
    std::string get_longitude_dms() const { double lon = get_longitude(); return get_deg_to_dms(lon) + ((lon <= 0) ? " W" : " E"); }

    // minutes
    uint32_t get_duration() const { return m_duration; }

    enum
    {
        cAccuracyInvalid = 0,
        cAccuracyQuestionable = 1,
        cAccuracyApproximate = 2,
        cAccuracyGood = 3
    };

    bool get_time(std::string& time) const
    {
        uint32_t time_accuracy = m_ymdt & 3;

        if (time_accuracy == cAccuracyInvalid)
            return false;

        uint32_t hour = m_time / 6;
        uint32_t minute = (m_time % 6) * 10;

        if (hour > 23)
        {
            assert(0);
            return false;
        }

        time = string_format("%02u:%02u", hour, minute);
        if (time_accuracy == cAccuracyQuestionable)
            time += "?";
        else if (time_accuracy == cAccuracyApproximate)
            time = "~" + time;

        return true;
    }

    bool get_date(event_date& date) const
    {
        uint32_t year_accuracy = (m_ymdt >> 6) & 3;
        uint32_t month_accuracy = (m_ymdt >> 4) & 3;
        uint32_t day_accuracy = (m_ymdt >> 2) & 3;

        int year = year_accuracy ? get_year() : 0;
        uint32_t month = month_accuracy ? get_month() : 0;
        uint32_t day = day_accuracy ? get_day() : 0;

        if ((day < 1) || (day > 31))
        {
            day = 0;
            day_accuracy = cAccuracyInvalid;
        }

        if ((month < 1) || (month > 12))
        {
            month = 0;
            month_accuracy = cAccuracyInvalid;
        }

        if (!year)
            return false;

        uint32_t min_accuracy = year;
        date.m_year = year;

        if (month)
        {
            date.m_month = month;

            if (!day)
            {
                min_accuracy = std::min(year_accuracy, month_accuracy);
            }
            else
            {
                min_accuracy = std::min(std::min(year_accuracy, month_accuracy), day_accuracy);

                date.m_day = day;
            }
        }

        if (min_accuracy == cAccuracyApproximate)
            date.m_approx = true;
        else if (min_accuracy == cAccuracyQuestionable)
            date.m_fuzzy = true;

        return true;
    }

    enum { cMaxFlags = 64 };

    // LOC, MISC, TYPE, ALIENS/MONSTERS, ACTIVITIES, VISITED/THINGS, EVIDENCE/SPECIAL, MISC_DETAILS
    bool get_flag(uint32_t index) const
    {
        assert(index < cMaxFlags);
        return (m_flags[index >> 3] & (1 << (index & 7))) != 0;
    }

#if 0
    uint8_t get_loc_flags() const { return m_loc_flags; }
    uint8_t get_misc_flags() const { return m_misc_flags; }
    uint8_t get_type_of_ufo_craft_flags() const { return m_type_of_ufo_craft_flags; }
    uint8_t get_aliens_monsters_flags() const { return m_aliens_monsters_flags; }
    uint8_t get_apparent_ufo_occupant_activities_flags() const { return m_apparent_ufo_occupant_activities_flags; }
    uint8_t get_places_visited_and_things_affected_flags() const { return m_places_visited_and_things_affected_flags; }
    uint8_t get_evidence_and_special_effects_flags() const { return m_evidence_and_special_effects_flags; }
    uint8_t get_miscellaneous_details_flags() const { return m_miscellaneous_details_flags; }
#endif

    void get_geo(std::string& country_name, std::string& state_or_prov_name) const
    {
        std::string state_or_prov_str(get_state_or_prov());
        string_trim_end(state_or_prov_str);

        if (state_or_prov_str.back() == '.')
            state_or_prov_str.pop_back();

        if (state_or_prov_str.back() == '.')
            state_or_prov_str.pop_back();

        get_hatch_geo(get_continent_code(), get_country_code(), state_or_prov_str, country_name, state_or_prov_name);

        if (state_or_prov_str == "UNK")
            state_or_prov_name = "Unknown";
    }

    std::string get_full_refs() const
    {
        std::string ref(g_hatch_refs_tab[get_reference()]);

        if (g_hatch_refs_tab[get_reference()])
        {
            uint32_t ref_index = get_reference_index();

            if (get_reference() == 93)
            {
                for (const auto& x : g_hatch_refs_93)
                    if (x.m_ref == ref_index)
                    {
                        ref += x.m_pDesc;
                        break;
                    }
            }
            else if (get_reference() == 96)
            {
                for (const auto& x : g_hatch_refs_96)
                    if (x.m_ref == ref_index)
                    {
                        ref += x.m_pDesc;
                        break;
                    }
            }
            else if (get_reference() == 97)
            {
                for (const auto& x : g_hatch_refs_97)
                    if (x.m_ref == ref_index)
                    {
                        ref += x.m_pDesc;
                        break;
                    }
            }
            else if (get_reference() == 98)
            {
                for (const auto& x : g_hatch_refs_98)
                    if (x.m_ref == ref_index)
                    {
                        ref += x.m_pDesc;
                        break;
                    }
            }
            else
            {
                ref += string_format(" (Index %u)", ref_index);
            }
        }

        return ref;
    }
};
#pragma pack(pop)

static std::unordered_map<std::string, std::string> g_dictionary;

struct token
{
    std::string m_token;
    bool m_cap_check;
    bool m_replaced_flag;

    token() :
        m_cap_check(false),
        m_replaced_flag(false)
    {
    }

    token(const std::string& token, bool cap_check, bool replaced_flag) :
        m_token(token),
        m_cap_check(cap_check),
        m_replaced_flag(replaced_flag)
    {
    }
};

std::unordered_set<std::string> g_unique_tokens;
std::vector<string_vec> g_hatch_exception_tokens;

static void init_hatch_cap_exception_tokens()
{
    g_hatch_exception_tokens.resize(std::size(g_cap_exceptions));

    std::string cur_etoken;
    for (uint32_t e = 0; e < std::size(g_cap_exceptions); e++)
    {
        const std::string exception_str(g_cap_exceptions[e]);

        string_vec& etokens = g_hatch_exception_tokens[e];

        for (uint32_t i = 0; i < exception_str.size(); i++)
        {
            uint8_t c = exception_str[i];

            if (c == ' ')
            {
                if (cur_etoken.size())
                {
                    etokens.push_back(cur_etoken);
                    cur_etoken.clear();
                }
            }
            else if (c == '-')
            {
                if (cur_etoken.size())
                {
                    etokens.push_back(cur_etoken);
                    cur_etoken.clear();
                }

                std::string s;
                s.push_back(c);
                etokens.push_back(s);
            }
            else
            {
                cur_etoken.push_back(c);
            }
        }

        if (cur_etoken.size())
        {
            etokens.push_back(cur_etoken);

            cur_etoken.resize(0);
        }
    }
}

static std::string fix_capitilization(std::vector<token>& toks, uint32_t& tok_index)
{
    if (toks[tok_index].m_replaced_flag)
        return toks[tok_index].m_token;

    const uint32_t toks_remaining = (uint32_t)toks.size() - tok_index;

    // Peak ahead on the tokens to see if we need to correct any capitilization using the exception table.
    for (uint32_t e = 0; e < std::size(g_cap_exceptions); e++)
    {
        const string_vec& etokens = g_hatch_exception_tokens[e];

        if (toks_remaining >= etokens.size())
        {
            uint32_t i;
            for (i = 0; i < etokens.size(); i++)
                if ((string_icompare(etokens[i], toks[tok_index + i].m_token.c_str()) != 0) || toks[tok_index + i].m_replaced_flag)
                    break;

            if (i == etokens.size())
            {
                for (i = 0; i < etokens.size(); i++)
                {
                    toks[tok_index + i].m_token = etokens[i];
                    toks[tok_index + i].m_replaced_flag = true;
                }

                std::string res(toks[tok_index].m_token);

                return res;
            }
        }
    }

    std::string str(toks[tok_index].m_token);

    if (!toks[tok_index].m_cap_check)
        return str;

    string_vec wtokens;
    std::string cur_wtoken;

    for (uint32_t i = 0; i < str.size(); i++)
    {
        uint8_t c = str[i];

        if (isalpha(c) || isdigit(c) || ((c == '\'') && (i != 0) && (i != str.size() - 1)))
        {
            cur_wtoken.push_back(c);
        }
        else
        {
            if (cur_wtoken.size())
            {
                wtokens.push_back(cur_wtoken);
                cur_wtoken.clear();
            }

            std::string s;
            s.push_back(c);
            wtokens.push_back(s);
        }
    }

    if (cur_wtoken.size())
    {
        wtokens.push_back(cur_wtoken);
        cur_wtoken.clear();
    }

    for (uint32_t wtoken_index = 0; wtoken_index < wtokens.size(); wtoken_index++)
    {
        std::string& substr = wtokens[wtoken_index];

        if (substr == "A")
            substr = "a";
        else if (substr.size() >= 2)
        {
            bool is_all_uppercase = true;

            for (uint8_t c : substr)
            {
                if (!isupper(c) && (c != '\''))
                {
                    is_all_uppercase = false;
                    break;
                }
            }

            if (is_all_uppercase)
            {
                auto res = g_dictionary.find(string_lower(substr));
                if (res != g_dictionary.end())
                {
                    substr = res->second;
                }
                else
                {
                    substr = string_lower(substr);

                    g_unique_tokens.insert(substr);
                }
            }
        }
    }

    std::string res;
    for (uint32_t wtoken_index = 0; wtoken_index < wtokens.size(); wtoken_index++)
        res += wtokens[wtoken_index];

    return res;
}

static std::unordered_map<std::string, hatch_abbrev> g_hatch_abbreviations_map;

static void init_hatch_abbreviations_map()
{
    for (uint32_t abbrev_index = 0; abbrev_index < std::size(g_hatch_abbreviations); abbrev_index++)
    {
        auto res = g_hatch_abbreviations_map.insert(std::make_pair(string_lower(g_hatch_abbreviations[abbrev_index].pAbbrev), g_hatch_abbreviations[abbrev_index]));
        if (!res.second)
            panic("Mutiple Hatch abbreviation: %s", res.first->first.c_str());
    }
}

// Expand abbreviations
static void expand_abbreviations_internal(bool first_line, std::string orig_token, const string_vec& tokens, uint32_t cur_tokens_index, std::vector<token>& toks)
{
    const uint32_t MAX_ABBREVS = 5;

    uint32_t k;
    for (k = 0; k < MAX_ABBREVS; k++)
    {
        std::string new_token(orig_token);

        auto find_res = g_hatch_abbreviations_map.find(string_lower(orig_token));
        if (find_res != g_hatch_abbreviations_map.end())
        {
            if (!first_line || !find_res->second.m_forbid_firstline)
            {
                new_token = find_res->second.pExpansion;

                if (new_token.size())
                    toks.push_back(token(new_token, !first_line && (new_token == orig_token), false));

                break;
            }
        }

        if ((orig_token.size() >= 4) && (uisupper(orig_token[0])))
        {
            std::string month_suffix(orig_token);
            month_suffix.erase(0, 3);

            if ((month_suffix.size() <= 4) && string_is_digits(month_suffix))
            {
                std::string month_prefix(orig_token);
                month_prefix.erase(3, month_prefix.size() - 3);
                std::string search_prefix(string_upper(month_prefix));

                static const char* g_hmonths[12] =
                {
                    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                    "JLY", "AUG", "SEP", "OCT", "NOV", "DEC"
                };

                uint32_t m;
                for (m = 0; m < 12; m++)
                    if (search_prefix == g_hmonths[m])
                        break;

                if (m < 12)
                {
                    toks.push_back(token(g_months[m], !first_line, false));

                    // TODO: This can be improved by checking the # before the token
                    long long val = atoll(month_suffix.c_str());
                    if (val > 31)
                        month_suffix = '\'' + month_suffix;

                    toks.push_back(token(month_suffix, !first_line, false));
                    break;
                }
            }
        }

        size_t p;
        if ((p = orig_token.find_first_of('.')) == std::string::npos)
        {
            // No period(s) - we're done.
            if (new_token.size())
                toks.push_back(token(new_token, !first_line, false));

            break;
        }

        // Specifically detect abbrev. first names like "A." etc. and expand them.
        if (!first_line && (orig_token.size() > 4) && (p == 1) && uisupper(orig_token[0]) && uisupper(orig_token[2]))
        {
            std::string first_name(orig_token);
            first_name.erase(2, first_name.size() - 2);

            toks.push_back(token(first_name, false, false));

            orig_token.erase(0, p + 1);
        }
        else
        {
            // Detect words starting with an abbreviation ending in "."
            std::string prefix(orig_token);

            prefix.erase(p + 1, prefix.size() - (p + 1));

            find_res = g_hatch_abbreviations_map.find(string_lower(prefix));

            if ((find_res != g_hatch_abbreviations_map.end()) && (!first_line || !find_res->second.m_forbid_firstline))
            {
                new_token = find_res->second.pExpansion;

                toks.push_back(token(new_token, false, false));

                orig_token.erase(0, p + 1);
            }
            else
            {
                if (new_token.size())
                    toks.push_back(token(new_token, !first_line, false));

                break;
            }
        }

    } // k

    if (k == MAX_ABBREVS)
    {
        if (orig_token.size())
            toks.push_back(token(orig_token, !first_line, false));
    }
}

static bool is_sentence_ender(uint8_t c)
{
    return (c == '!') || (c == '.') || (c == '?');
}

static void expand_abbreviations(bool first_line, std::string orig_token, const string_vec& tokens, uint32_t cur_tokens_index, std::vector<token>& toks)
{
    std::string new_token(orig_token);

    // Temporarily remove " and ' prefix/suffix chars from the token, before the abbrev checks.
    std::string prefix_char, suffix_char;
    if (orig_token.size() >= 3)
    {
        if ((orig_token[0] == '\'') || (orig_token[0] == '\"'))
        {
            prefix_char.push_back(orig_token[0]);
            orig_token.erase(0, 1);
            new_token = orig_token;
        }

        if ((orig_token.back() == '\'') || (orig_token.back() == '\"'))
        {
            suffix_char.push_back(orig_token.back());
            orig_token.pop_back();
            new_token = orig_token;
        }
    }

    const size_t first_tok = toks.size();

    expand_abbreviations_internal(first_line, orig_token, tokens, cur_tokens_index, toks);

    const size_t num_toks = toks.size() - first_tok;
    assert(num_toks);

    const size_t last_tok = first_tok + num_toks - 1;

    if (prefix_char.size())
        toks[first_tok].m_token = prefix_char + toks[first_tok].m_token;

    if (suffix_char.size())
        toks[last_tok].m_token = toks[last_tok].m_token + suffix_char;

}

static std::string decode_hatch(const std::string& str, bool first_line)
{
    std::string res;

    string_vec tokens;
    std::string cur_token;

    bool inside_space = false;
    int prev_c = -1;

    // Phase 1: Tokenize the input string based off examination of (mostly) individual chars, previous chars and upcoming individual chars.
    for (uint32_t i = 0; i < str.size(); i++)
    {
        uint8_t c = str[i];

        const bool is_two_dots = (c == '.') && ((i + 1) < str.size()) && (str[i + 1] == '.');
        const bool is_one_equals = (c == '1') && ((i + 1) < str.size()) && (str[i + 1] == '=');

        const bool prev_is_digit = i && uisdigit(str[i - 1]);
        const bool next_is_plus = ((i + 1) < str.size()) && (str[i + 1] == '+');

        //const bool has_prev = (i != 0);
        //const bool has_next = (i + 1) < str.size();

        if (c == ' ')
        {
            if (cur_token.size())
            {
                tokens.push_back(cur_token);
                cur_token.clear();
            }

            inside_space = true;
        }
        else if (is_one_equals)
        {
            if (cur_token.size())
            {
                tokens.push_back(cur_token);
                cur_token.clear();
            }

            tokens.push_back("1=");
            i++;

            inside_space = false;
        }
        else if (
            (c == ';') || ((c >= 0x18) && (c <= 0x1b)) || (c == '<') || (c == '>') ||
            (c == '=') ||
            (c == '/') ||
            (c == ',') ||
            (c == '?') || (c == '!') ||
            ((!prev_is_digit || next_is_plus) && (c == '+')) ||
            (c == '@') || (c == '-') ||
            is_two_dots
            )
        {
            if (cur_token.size())
            {
                tokens.push_back(cur_token);
                cur_token.clear();
            }

            std::string s;
            s.push_back(c);

            if (is_two_dots)
            {
                s += ".";
                i++;
            }

            tokens.push_back(s);

            inside_space = false;
        }
        else
        {
            cur_token.push_back(c);
            inside_space = false;

            if ((c == 0xf8) || // code page 437 degree sym
                (prev_is_digit && (c == '+') && !next_is_plus))
            {
                tokens.push_back(cur_token);
                cur_token.clear();
            }
        }

        prev_c = c;
    }

    if (cur_token.size())
        tokens.push_back(cur_token);

    // Phase 2: Exceptional fixups that change or split tokens up into multiple tokens.
    string_vec new_tokens;

    for (uint32_t i = 0; i < tokens.size(); i++)
    {
        std::string tok(tokens[i]);

        // Convert "BBK#"
        if (string_begins_with(tok, "BBK#") && (tok.size() > 4))
        {
            new_tokens.push_back("Project Bluebook Case #");

            tok.erase(0, 4);
            new_tokens.push_back(tok);

            continue;
        }

        // Split "k'alt"
        if (string_ends_in(tok, "k'alt"))
        {
            tok.erase(tok.size() - 3, 3);
            new_tokens.push_back(tok);

            new_tokens.push_back("Alt");

            continue;
        }

        // Convert "HI+LO"
        if ((i + 2 < tokens.size()) && (tokens[i] == "HI") && (tokens[i + 1] == "+") && (tokens[i + 2] == "LO"))
        {
            tokens.push_back("high and low");
            i += 2;
            continue;
        }

        // Don't split "4rth" to "4 rth" etc.
        if ((string_icompare(tok, "4RTH") == 0) || (string_icompare(tok, "3rds") == 0) || (string_icompare(tok, "16th") == 0))
        {
            new_tokens.push_back(tok);
            continue;
        }

        if (string_ends_in(tok, "Kmph"))
        {
            new_tokens.push_back(tok);
            continue;
        }

        if (tok == "12Ocm")
        {
            new_tokens.push_back("120cm");
            continue;
        }

        if (string_icompare(tok, "3OOM") == 0)
        {
            new_tokens.push_back("300m");
            continue;
        }

        // If the first char isn't a digit then just continue now, because the rest of this code is concerned with splitting numbers away from words.
        if (!isdigit(tok[0]))
        {
            new_tokens.push_back(tok);
            continue;
        }

        if (tok.size() >= 3)
        {
            // Check for 1-7 digits then ' followed by 1- letters and split
            uint32_t j;
            for (j = 1; j < tok.size(); j++)
                if (tok[j] == '\'')
                    break;

            if ((j < tok.size()) && (j != tok.size() - 1) && (j <= 7))
            {
                uint32_t k;
                for (k = 1; k < j; k++)
                    if (!uisdigit(tok[k]) && (utolower(tok[k]) != 'x') && (utolower(tok[k]) != 'k') && (tok[k] != '.'))
                        break;

                if ((k == j) && (uisalpha(tok[j + 1])))
                {
                    int sp = j + 1;
                    std::string new_tok(tok);
                    new_tok.erase(0, sp);

                    std::string n(tok);

                    n.erase(sp, n.size() - sp);
                    new_tokens.push_back(n);

                    new_tokens.push_back(new_tok);

                    continue;
                }
            }
        }

        // Won't split digits away for tokens < 4 chars
        if ((tok.size() < 4) || (tok == "6F6s"))
        {
            new_tokens.push_back(tok);
            continue;
        }

        // Check for 1-2 digits and alpha and split
        // TODO: support 3-4 digits
        int split_point = -1;
        if (uisalpha(tok[1]))
            split_point = 1;
        else if (uisdigit(tok[1]) && uisalpha(tok[2]) && uisalpha(tok[3]))
            split_point = 2;

        if (split_point > 0)
        {
            std::string new_tok(tok);
            new_tok.erase(0, split_point);

            // Don't split the number digits from some special cases, like hr, cm, mph, etc.
            if ((string_icompare(new_tok, "hr") != 0) &&
                (string_icompare(new_tok, "nd") != 0) &&
                (string_icompare(new_tok, "kw") != 0) &&
                (string_icompare(new_tok, "cm") != 0) &&
                (string_icompare(new_tok, "km") != 0) &&
                (string_icompare(new_tok, "mph") != 0) &&
                (string_icompare(new_tok, "kph") != 0) &&
                (!string_begins_with(new_tok, "K'")))
            {
                std::string n(tok);

                n.erase(split_point, n.size() - split_point);
                new_tokens.push_back(n);

                if (new_tok == "min")
                    new_tok = "minute(s)";

                new_tokens.push_back(new_tok);
            }
            else
            {
                new_tokens.push_back(tok);
            }
        }
        else
        {
            new_tokens.push_back(tok);
        }
    }

    tokens.swap(new_tokens);

    std::vector<token> toks;

    // Phase 3: Compose new string, expanding abbreviations and tokens to one or more words, or combining together special sequences of tokens into specific phrases.
    // Also try to carefully insert spaces into the output, as needed.
    for (uint32_t i = 0; i < tokens.size(); i++)
    {
        const uint32_t num_tokens_left = ((uint32_t)tokens.size() - 1) - i;
        const bool has_prev_token = i > 0, has_next_token = (i + 1) < tokens.size();
        const bool next_token_is_slash = (has_next_token) && (tokens[i + 1][0] == '/');

        bool is_next_dir = false;
        if (has_next_token)
        {
            uint32_t ofs = 1;
            if (tokens[i + 1] == ">")
            {
                ofs = 2;
            }

            if ((i + ofs) < tokens.size())
            {
                std::string next_tok = string_upper(tokens[i + ofs]);

                if ((next_tok.back() == '.') && (next_tok.size() >= 2))
                    next_tok.pop_back();

                if ((next_tok == "N") || (next_tok == "S") || (next_tok == "E") || (next_tok == "W") ||
                    (next_tok == "SW") || (next_tok == "SE") || (next_tok == "NW") || (next_tok == "NE") ||
                    (next_tok == "NNE") || (next_tok == "NNW") || (next_tok == "SSE") || (next_tok == "SSW") ||
                    (next_tok == "ESE"))
                {
                    is_next_dir = true;
                }
            }
        }

        std::string orig_token(tokens[i]);
        std::string new_token(orig_token);

        if (!orig_token.size())
            continue;

        // Handle various exceptions before expending abbreviations
        // TODO: Refactor to table(s)

        // Special handling for RUSS/RUSS.
        if ((tokens[i] == "RUSS") || (tokens[i] == "RUSS.") || (tokens[i] == "RUS") || (tokens[i] == "RUS."))
        {
            if (first_line)
                new_token = "Russia";
            else
                new_token = "Russian";
        }
        // AA FLITE #519 - exception
        // AA LINER
        else if ((tokens[i] == "AA") && (num_tokens_left >= 1) && ((tokens[i + 1] == "FLITE#519") || (tokens[i + 1] == "LINER")))
        {
            new_token = "AA";
        }
        // bright Lt.
        else if ((tokens[i] == "VBRITE") && (num_tokens_left >= 1) && (tokens[i + 1] == "LT"))
        {
            new_token = "vibrant bright light";
            i++;
        }
        // ENERGY SRC
        else if ((tokens[i] == "ENERGY") && (num_tokens_left >= 1) && (tokens[i + 1] == "SRC"))
        {
            new_token = "energy source";
            i++;
        }
        // mid air - exception
        else if ((tokens[i] == "MID") && (num_tokens_left >= 1) && (tokens[i + 1] == "AIR"))
        {
            new_token = "mid";
        }
        // /FORMN or /formation - exception
        else if ((string_icompare(tokens[i], "/") == 0) && (num_tokens_left >= 1) && ((string_icompare(tokens[i + 1], "FORMN") == 0) || (string_icompare(tokens[i + 1], "formation") == 0)))
        {
            new_token = "in formation";
            i++;
        }
        // /FORMNs - exception
        else if ((string_icompare(tokens[i], "/") == 0) && (num_tokens_left >= 1) && ((string_icompare(tokens[i + 1], "FORMNs") == 0) || (string_icompare(tokens[i + 1], "formations") == 0)))
        {
            new_token = "in formations";
            i++;
        }
        // LOST/CLOUDS - exception
        else if ((string_icompare(tokens[i], "LOST") == 0) && (num_tokens_left >= 2) && (tokens[i + 1] == "/") && (string_icompare(tokens[i + 2], "CLOUDS") == 0))
        {
            new_token = "lost in clouds";
            i += 2;
        }
        // LOST/DISTANCE - exception
        else if ((string_icompare(tokens[i], "LOST") == 0) && (num_tokens_left >= 2) && (tokens[i + 1] == "/") && (string_icompare(tokens[i + 2], "DISTANCE") == 0))
        {
            new_token = "lost in the distance";
            i += 2;
        }
        // W-carbide - exception
        else if ((string_icompare(tokens[i], "W") == 0) && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (string_icompare(tokens[i + 2], "carbide") == 0))
        {
            new_token = "W";
        }
        // S-SHAPE - exception
        else if ((tokens[i] == "S") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "SHAPE"))
        {
            new_token = "S";
        }
        // mid-sky - exception
        else if ((tokens[i] == "MID") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "SKY"))
        {
            new_token = "mid";
        }
        // mid-flite - exception
        else if ((tokens[i] == "MID") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "FLITE"))
        {
            new_token = "mid";
        }
        // mid-city - exception
        else if ((tokens[i] == "MID") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "CITY"))
        {
            new_token = "mid";
        }
        // W vee - exception
        else if ((tokens[i] == "W") && (num_tokens_left >= 1) && (tokens[i + 1] == "VEE"))
        {
            new_token = "with vee";
            i++;
        }
        // Lake Mi - exception
        else if ((tokens[i] == "LAKE") && (num_tokens_left >= 1) && (tokens[i + 1] == "Mi"))
        {
            new_token = "Lake Michigan";
            i++;
        }
        // SCI-FI
        else if ((tokens[i] == "SCI") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "FI"))
        {
            new_token = "Sci-Fi";
            i += 2;
        }
        // V-tall
        else if ((tokens[i] == "V") && (num_tokens_left >= 2) && (tokens[i + 1] == "-") && (tokens[i + 2] == "TALL"))
        {
            new_token = "very tall";
            i += 2;
        }
        // 1 OBS/1 OBS. at beginning
        else if ((i == 1) && (tokens[0] == "1") && (tokens[1] == "OBS" || tokens[1] == "OBS."))
        {
            new_token = "observer";
        }
        // CLR WEATHER exception
        else if ((num_tokens_left >= 1) && (tokens[i] == "CLR") && (tokens[i + 1] == "WEATHER"))
        {
            new_token = "clear";
        }
        // WATER DOMES exception (typo fix)
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "WATER") == 0) && (string_icompare(tokens[i + 1], "DOMES") == 0))
        {
            new_token = "water comes";
            i++;
        }
        // W dome exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "W") == 0) && (string_icompare(tokens[i + 1], "DOME") == 0))
        {
            new_token = "with";
        }
        // CLR SKY exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "SKY") == 0))
        {
            new_token = "clear";
        }
        // CLR DOME exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "DOME") == 0))
        {
            new_token = "clear";
        }
        // CLR DOMED exception
        else if ((num_tokens_left >= 2) && (string_icompare(tokens[i], "CLR") == 0) && (tokens[i + 1] == "-") && (string_icompare(tokens[i + 2], "DOMED") == 0))
        {
            new_token = "clear";
        }
        // CLR DOME exception
        else if ((num_tokens_left >= 2) && (string_icompare(tokens[i], "CLR") == 0) && (tokens[i + 1] == "-") && (string_icompare(tokens[i + 2], "DOME") == 0))
        {
            new_token = "clear";
        }
        // CLR RDR exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "RDR") == 0))
        {
            new_token = "clear";
        }
        // CLR CLOCKPIT exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "COCKPIT") == 0))
        {
            new_token = "clear";
        }
        // CLR TORUS exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "TORUS") == 0))
        {
            new_token = "clear";
        }
        // CLR DAY exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "DAY") == 0))
        {
            new_token = "clear";
        }
        // CLR PLASTIC exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "PLASTIC") == 0))
        {
            new_token = "clear";
        }
        // CLR FOTOS exception (a guess, need to verify)
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "FOTOS") == 0))
        {
            new_token = "clear";
        }
        // CLR FOTO exception (a guess, need to verify)
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "FOTO") == 0))
        {
            new_token = "clear";
        }
        // CLR SHOT exception (a guess, need to verify)
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "SHOT") == 0))
        {
            new_token = "clear";
        }
        // CLR BLUE exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "BLUE") == 0))
        {
            new_token = "clear";
        }
        // CLR BUBBLE exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "BUBBLE") == 0))
        {
            new_token = "clear";
        }
        // CLR BUBBLES exception
        else if ((num_tokens_left >= 1) && (string_icompare(tokens[i], "CLR") == 0) && (string_icompare(tokens[i + 1], "BUBBLES") == 0))
        {
            new_token = "clear";
        }
        // S+Cu exception
        else if ((num_tokens_left >= 2) && (tokens[i] == "S") && (tokens[i + 1] == "+") && (tokens[i + 2] == "Cu"))
        {
            new_token = "S";
        }
        // IND OBS exception
        else if ((num_tokens_left >= 1) && (tokens[i] == "IND") && (tokens[i + 1] == "OBS"))
        {
            new_token = "independent";
        }
        // L<>R
        else if ((num_tokens_left >= 3) && (tokens[i] == "L") && (tokens[i + 1] == "<") && (tokens[i + 2] == ">") && (tokens[i + 3] == "R"))
        {
            new_token = "left and right";
            i += 3;
        }
        // <+>
        else if ((num_tokens_left >= 2) && (tokens[i] == "<") && (tokens[i + 1] == "+") && (tokens[i + 2] == ">"))
        {
            new_token = "left and right";
            i += 2;
        }
        else if (orig_token == "NFD")
        {
            if ((!has_next_token) || next_token_is_slash)
                new_token = "No further details";
            else
                new_token = "No further details [in]";
        }
        // Up and down arrows
        else if ((orig_token[0] == 0x18) &&
            ((i + 1) < tokens.size()) && (tokens[i + 1][0] == '+') &&
            ((i + 2) < tokens.size()) && (tokens[i + 2][0] == 0x19))
        {
            const uint32_t at_end = ((i + 3) == tokens.size()) || (tokens[i + 3][0] == '/');
            new_token = !at_end ? "going up and down [to]" : "going up and down";
            i += 2;
        }
        // "V BRITE"
        else if ((orig_token == "V") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "BRITE"))
        {
            new_token = "very bright";
            i++;
        }
        // ++
        else if ((orig_token == "+") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "+"))
        {
            new_token = "and more/others";
            i++;
        }
        // >>
        else if ((orig_token == ">") && ((i + 1) < tokens.size()) && (tokens[i + 1] == ">"))
        {
            const uint32_t at_end = ((i + 2) == tokens.size()) || (tokens[i + 2][0] == '/');
            new_token = (!at_end && !is_next_dir) ? "going quickly [to]" : "going quickly";
            i++;
        }
        // ><
        else if ((orig_token == ">") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "<"))
        {
            new_token = "to/from";
            i++;
        }
        // <>
        else if ((orig_token == "<") && ((i + 1) < tokens.size()) && (tokens[i + 1] == ">"))
        {
            // Larry said "between" but that sounds awkward and would require reordering tokens.
            new_token = "to/from/between";
            i++;
        }
        // >
        else if (orig_token == ">")
        {
            new_token = (has_next_token && !next_token_is_slash && !is_next_dir) ? "going [to]" : "going";
        }
        // Tree up arrows
        else if ((orig_token[0] == 0x18) && (num_tokens_left >= 2) && (tokens[i + 1][0] == 0x18) && (tokens[i + 2][0] == 0x18))
        {
            const uint32_t at_end = ((i + 3) == tokens.size()) || (tokens[i + 3][0] == '/');
            new_token = !at_end ? "extremely quickly going up [to]" : "extremely quickly going up";
            i += 2;
        }
        // Two up arrows
        else if ((orig_token[0] == 0x18) && ((i + 1) < tokens.size()) && (tokens[i + 1][0] == 0x18))
        {
            const uint32_t at_end = ((i + 2) == tokens.size()) || (tokens[i + 2][0] == '/');
            new_token = !at_end ? "quickly going up [to]" : "quickly going up";
            i++;
        }
        // Up arrow
        else if (orig_token[0] == 0x18)
        {
            new_token = (has_next_token && !next_token_is_slash) ? "going up [to]" : "going up";
        }
        // Two down arrows
        else if ((orig_token[0] == 0x19) && ((i + 1) < tokens.size()) && (tokens[i + 1][0] == 0x19))
        {
            const uint32_t at_end = ((i + 2) == tokens.size()) || (tokens[i + 2][0] == '/');
            new_token = !at_end ? "quickly going down [to]" : "quickly going down";
            i++;
        }
        // Down arrow
        else if (orig_token[0] == 0x19)
        {
            new_token = (has_next_token && !next_token_is_slash) ? "going down [to]" : "going down";
        }
        // Two right arrows
        else if ((orig_token[0] == 0x1A) && ((i + 1) < tokens.size()) && (tokens[i + 1][0] == 0x1A))
        {
            const uint32_t at_end = ((i + 2) == tokens.size()) || (tokens[i + 2][0] == '/');
            new_token = !at_end ? "quickly going right [to]" : "quickly going right";
            i++;
        }
        // Right arrow
        else if (orig_token[0] == 0x1A)
        {
            new_token = (has_next_token && !next_token_is_slash) ? "going right [to]" : "going right";
        }
        // Two left arrows
        else if ((orig_token[0] == 0x1B) && ((i + 1) < tokens.size()) && (tokens[i + 1][0] == 0x1B))
        {
            const uint32_t at_end = ((i + 2) == tokens.size()) || (tokens[i + 2][0] == '/');
            new_token = !at_end ? "quickly going left [to]" : "quickly going left";
            i++;
        }
        // Left arrow
        else if (orig_token[0] == 0x1B)
        {
            new_token = (has_next_token && !next_token_is_slash) ? "going left [to]" : "going left";
        }
        // /
        else if (orig_token[0] == '/')
        {
            new_token = "/";
        }
        // +
        else if (orig_token[0] == '+')
        {
            if (!i)
                new_token = "also";
            else if ((i != (tokens.size() - 1)) && (tokens[i + 1][0] != '/'))
                new_token = "and";
            else
                new_token = "and more";
        }
        // @
        else if (orig_token[0] == '@')
        {
            new_token = "at";
        }
        // dbl-word
        else if ((string_icompare(orig_token, "dbl") == 0) && ((i + 1) < tokens.size()) && (tokens[i + 1] == "-"))
        {
            new_token = "double";
        }
        // GLOW-word
        else if ((string_icompare(orig_token, "GLOW") == 0) && ((i + 1) < tokens.size()) && (tokens[i + 1] == "-"))
        {
            new_token = "glowing";
        }
        // A-test
        else if ((orig_token == "A") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "-") &&
            ((i + 2) < tokens.size()) && (string_icompare(tokens[i + 2], "TEST") == 0))
        {
            new_token = "atomic test";
            i += 2;
        }
        // A-plant
        else if ((orig_token == "A") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "-") &&
            ((i + 2) < tokens.size()) && (string_icompare(tokens[i + 2], "PLANT") == 0))
        {
            new_token = "atomic plant";
            i += 2;
        }
        // V-form
        else if ((orig_token == "V") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "-") &&
            ((i + 2) < tokens.size()) && (string_icompare(tokens[i + 2], "FORM") == 0))
        {
            new_token = "V-formation";
            i += 2;
        }
        // 1/2 (to fix spacing issues)
        else if ((orig_token == "1") && ((i + 1) < tokens.size()) && (tokens[i + 1] == "/") &&
            ((i + 2) < tokens.size()) && (tokens[i + 2] == "2"))
        {
            new_token = "1/2";
            i += 2;
        }
        // "W/O"
        else if ((i) &&
            (string_icompare(orig_token, "W") == 0) &&
            ((i + 1) < tokens.size()) && (tokens[i + 1] == "/") &&
            ((i + 2) < tokens.size()) && (string_icompare(tokens[i + 2], "O") == 0))
        {
            new_token = "without";
            i += 2;
        }
        // "S/L"
        else if ((orig_token == "S") &&
            ((i + 1) < tokens.size()) && (tokens[i + 1] == "/") &&
            ((i + 2) < tokens.size()) && (tokens[i + 2] == "L"))
        {
            // No idea what this means yet.
            new_token = "straight and level";
            i += 2;
        }
        // "FOO-FIGHTERS"
        else if ((orig_token == "FOO") &&
            ((i + 1) < tokens.size()) && (tokens[i + 1] == "-") &&
            ((i + 2) < tokens.size()) && (tokens[i + 2] == "FIGHTERS"))
        {
            // Just don't let the abbreviator kick in. Thanks Larry.
        }
        // "W/word"
        else if ((i) &&
            ((orig_token == "W") || (orig_token == "w")) &&
            ((i + 1) < tokens.size()) && (tokens[i + 1] == "/") &&
            (tokens[i - 1] != ">") &&
            (tokens[i - 1] != "<"))
        {
            new_token = "with";
            i++;
        }
        // "1="
        else if (orig_token == "1=")
        {
            new_token = "one is [a]";
        }
        // Exception for "ORG RPT".
        else if ((orig_token == "ORG") && has_next_token && (tokens[i + 1] == "RPT"))
        {
            new_token = "original";
        }
        // TODO: check for line 1 and don't expand these states
        // Exception for ,MT (the state) - don't change to "Mt."
        else if (first_line && orig_token == "MI" && has_prev_token && tokens[i - 1] == ",")
        {
        }
        // Exception for ,MT (the state) - don't change to "Mt."
        else if (first_line && orig_token == "MT" && has_prev_token && tokens[i - 1] == ",")
        {
        }
        // Exception for ,NE (the state) - don't change to "northeast"
        else if (first_line && orig_token == "NE" && has_prev_token && tokens[i - 1] == ",")
        {
        }
        // Exception for ,MS (the state) - don't change to "northeast"
        else if (first_line && orig_token == "MS" && has_prev_token && tokens[i - 1] == ",")
        {
        }
        // Exception for ,AL (the state) - don't change to "northeast"
        else if (first_line && orig_token == "AL" && has_prev_token && tokens[i - 1] == ",")
        {
        }
        else
        {
            expand_abbreviations(first_line, orig_token, tokens, i, toks);
            continue;
        }

        if (new_token.size())
            toks.push_back(token(new_token, !first_line && (new_token == tokens[i]), false));
    }

    // Phase 4: Compose the final string, converting tokens to lower/uppercase and inserting spaces as needed.
    std::string new_str;

    bool in_quote = false;

    for (uint32_t i = 0; i < toks.size(); i++)
    {
        std::string new_token(toks[i].m_token);
        if (!new_token.size())
            continue;

        if (!first_line)
            new_token = fix_capitilization(toks, i);

        // Add a space if the previous string is not empty - excluding special cases where a space isn't necessary.
        if (new_str.size() &&
            (new_token != "..") &&
            (new_token != ",") &&
            (new_token != "!") && (new_token != "?") &&
            (new_token != "+") &&
            (!((new_token == ")") && (new_str.back() == '?'))) &&
            (new_token != ";") && (new_str.back() != ';') &&
            (new_token != "-") && (new_str.back() != '-') &&
            (new_str.back() != '#') &&
            (new_str.back() != '+') &&
            (!(in_quote && (new_token == "\"") && new_str.size() && is_sentence_ender(new_str.back())))
            )
        {
            new_str.push_back(' ');
            //new_str.push_back('*');
        }

        // Append the token string to the output string
        new_str += new_token;

        for (uint8_t c : new_token)
            if (c == '\"')
                in_quote = !in_quote;
    }

    return new_str;
}

static void decode_hatch_desc(const udb_rec* pRec, std::string& db_str, std::string& loc_str, std::string& desc_str)
{
    for (uint32_t i = 0; i < UDB_REC_TEXT_SIZE; i++)
    {
        if (pRec->get_text()[i] == 0)
            break;
        db_str.push_back(pRec->get_text()[i]);
    }

    std::string orig_desc(db_str);
    string_vec desc;
    for (; ; )
    {
        size_t pos = orig_desc.find_first_of(':');
        if (pos == std::string::npos)
        {
            desc.push_back(string_trim(orig_desc));
            break;
        }
        else
        {
            std::string s(orig_desc);
            s.erase(pos, s.size() - pos);
            desc.push_back(string_trim(s));

            orig_desc.erase(0, pos + 1);
        }
    }

    for (uint32_t i = 0; i < desc.size(); i++)
    {
        std::string str(decode_hatch(desc[i], !i));
        if (!str.size())
            continue;

        if (desc_str.size())
        {
            if (desc_str.back() != '.' && desc_str.back() != '!' && desc_str.back() != '?')
                desc_str += ".";

            desc_str += " ";
        }

        if (!i)
        {
            loc_str = string_upper(str);
        }
        else
        {
            if (uislower(str[0]))
                str[0] = utoupper(str[0]);
            else if ((str[0] == '\"') && (str.size() >= 2) && (uislower(str[1])))
                str[1] = utoupper(str[1]);
            else if ((str[0] == '\'') && (str.size() >= 2) && (uislower(str[1])))
                str[1] = utoupper(str[1]);
            else if ((str[0] == '(') && (str.size() >= 2) && (uislower(str[1])))
                str[1] = utoupper(str[1]);

            desc_str += str;
        }
    }

    if (desc_str.size() && desc_str.back() != '.' && desc_str.back() != '!' && desc_str.back() != '?')
    {
        if ((desc_str.back() == ')') && (!string_ends_in(desc_str, "(s)")))
        {
            desc_str.pop_back();
            if (desc_str.back() == ' ')
                desc_str.pop_back();

            if (desc_str.size() && desc_str.back() != '.' && desc_str.back() != '!' && desc_str.back() != '?')
                desc_str += ".";

            desc_str += ")";
        }
        else
        {
            desc_str += ".";
        }
    }

    db_str = dos_to_utf8(db_str);
    loc_str = dos_to_utf8(loc_str);
    desc_str = dos_to_utf8(desc_str);
}

template<typename T>
static void check_for_hatch_tab_dups(const T& tab)
{
    std::unordered_set<int> ids;
    for (const auto& x : tab)
        if (!ids.insert(x.m_ref).second)
            panic("Duplicate hatch ref table id");
}

static void init_dict()
{
    string_vec dict;

    uprintf("Reading dictionary\n");
    bool utf8_flag = false;
    if (!read_text_file("uppercase_dict.txt", dict, true, &utf8_flag))
        panic("Failed reading uppercase_dict.txt");

    for (auto str : dict)
    {
        string_trim(str);
        if (str.size() && uisupper(str[0]))
        {
            g_dictionary.insert(std::make_pair(string_lower(str), str));
        }
    }

    uprintf("Done reading dictionary, %u uppercase words\n", g_dictionary.size());
}

void udb_init()
{    
    assert(sizeof(udb_rec) == UDB_RECORD_SIZE);

    check_for_hatch_tab_dups(g_hatch_refs);
    check_for_hatch_tab_dups(g_hatch_refs_93);
    check_for_hatch_tab_dups(g_hatch_refs_96);
    check_for_hatch_tab_dups(g_hatch_refs_97);
    check_for_hatch_tab_dups(g_hatch_refs_98);

    for (uint32_t i = 0; i < std::size(g_hatch_refs); i++)
        g_hatch_refs_tab[g_hatch_refs[i].m_ref] = g_hatch_refs[i].m_pDesc;

    init_hatch_abbreviations_map();
    init_hatch_cap_exception_tokens();
    init_dict();
}

bool udb_dump()
{
    uint8_vec udb;
    if (!read_binary_file("u.rnd", udb))
        return false;

    const uint32_t TOTAL_RECS = 18123;
    if ((udb.size() / UDB_RECORD_SIZE) < TOTAL_RECS)
        panic("Invalid file size");

    string_vec output;

    const udb_rec* pRecs = reinterpret_cast<const udb_rec*>(&udb.front());
    for (uint32_t rec_index = 1; rec_index < TOTAL_RECS; rec_index++)
        //for (uint32_t rec_index = 18038; rec_index <= 18038; rec_index++)
    {
        const udb_rec* pRec = pRecs + rec_index;

        std::string db_str, loc_str, desc_str;
        decode_hatch_desc(pRec, db_str, loc_str, desc_str);

        event_date ed;
        pRec->get_date(ed);
        std::string date_str(ed.get_string());

        {
            uprintf("\n----------%u: Date: %s, Strangeness: %u, Credibility: %u\n", rec_index, date_str.c_str(), pRec->get_strangeness(), pRec->get_credibility());
            std::string time;
            if (pRec->get_time(time))
                uprintf("Time: %s\n", time.c_str());

            if (pRec->get_duration())
                uprintf("Duration: %u mins\n", pRec->get_duration());

            if (pRec->get_elevation() != -99)
                uprintf("Elevation: %im\n", pRec->get_elevation());

            if ((pRec->get_rel_altitude() != 0) && (pRec->get_rel_altitude() != 999))
                uprintf("Altitude: %im\n", pRec->get_rel_altitude());

            uprintf("Location: %s\n", loc_str.c_str());

            std::string country_name, state_or_prov_name;
            pRec->get_geo(country_name, state_or_prov_name);

            const uint32_t continent_code = pRec->get_continent_code();

            uprintf("Country: %s, State/Province: %s (%s), Continent: %s\n", country_name.c_str(), state_or_prov_name.c_str(), pRec->get_state_or_prov().c_str(),
                (continent_code < std::size(g_hatch_continents)) ? g_hatch_continents[continent_code] : "?");

            uprintf("Latitude/Longitude: %f %f, %s %s\n", pRec->get_latitude(), pRec->get_longitude(), pRec->get_latitude_dms().c_str(), pRec->get_longitude_dms().c_str());

            const uint32_t locale = pRec->get_locale();
            if (locale < std::size(g_hatch_locales))
                uprintf("Locale: %s\n", g_hatch_locales[locale]);

            uprintf("UDB Desc: %s\n", db_str.c_str());

            uprintf("Decoded Desc: %s\n", desc_str.c_str());

            uint32_t total_flags = 0;
            for (uint32_t f = 0; f < udb_rec::cMaxFlags; f++)
            {
                if (!f) // map
                    continue;

                if (pRec->get_flag(f))
                    total_flags++;
            }

            if (total_flags)
            {
                uprintf("Flags: ");

                uint32_t num_flags_printed = 0;
                for (uint32_t f = 0; f < udb_rec::cMaxFlags; f++)
                {
                    if (!f) // map
                        continue;

                    if (pRec->get_flag(f))
                    {
                        uprintf("%s", g_pHatch_flag_descs[f]);

                        num_flags_printed++;
                        if (num_flags_printed < total_flags)
                        {
                            uprintf(", ");

                            if ((num_flags_printed % 2) == 0)
                                uprintf("\n");
                        }
                    }
                }

                uprintf("\n");
            }

            uprintf("Ref: %s\n", pRec->get_full_refs().c_str());
        }

        output.push_back(string_format("Date: %s\nLocation: \"%s\"\nDescription: \"%s\"\n", date_str.c_str(), loc_str.c_str(), desc_str.c_str()));
    }

    string_vec toks;
    for (const auto& str : g_unique_tokens)
        toks.push_back(str);
    write_text_file("unique_tokens.txt", toks, false);

    write_text_file("output.txt", output, true);

    return true;
}

static bool convert_rec(uint32_t rec_index, const udb_rec* pRec, timeline_event& event)
{
    std::string db_str, loc_str, desc_str;
    decode_hatch_desc(pRec, db_str, loc_str, desc_str);

    pRec->get_date(event.m_begin_date);
    
    if (event.m_begin_date.m_year <= 0)
        return false;
    
    std::string time;
    if (pRec->get_time(time))
    {
        if (time != "00:00?")
            event.m_time_str = time;
    }

    event.m_date_str = event.m_begin_date.get_string();

    event.m_locations.push_back(loc_str);

    event.m_desc = desc_str;
    
    // TODO
    event.m_type.push_back("sighting");

    event.m_source_id = string_format("Hatch_UDB_%u", rec_index);
    event.m_source = "Hatch";
                
    for (uint32_t f = 0; f < udb_rec::cMaxFlags; f++)
        if ((f != cFlagMAP) && (pRec->get_flag(f)))
            event.m_attributes.push_back(g_pHatch_flag_descs[f]);

    event.m_refs.push_back(pRec->get_full_refs());
    
    event.m_key_value_data.push_back(std::make_pair("LocationLink", string_format("[Google Maps](https://www.google.com/maps/place/%f,%f)", pRec->get_latitude(), pRec->get_longitude())));
    
    event.m_key_value_data.push_back(std::make_pair("LatLong", string_format("%f %f", pRec->get_latitude(), pRec->get_longitude())));
    event.m_key_value_data.push_back(std::make_pair("LatLongDMS", string_format("%s %s", pRec->get_latitude_dms().c_str(), pRec->get_longitude_dms().c_str())));

    event.m_key_value_data.push_back(std::make_pair("HatchDesc", db_str));

    event.m_key_value_data.push_back(std::make_pair("Duration", string_format("%u", pRec->get_duration())));

    std::string country_name, state_or_prov_name;
    pRec->get_geo(country_name, state_or_prov_name);

    event.m_key_value_data.push_back(std::make_pair("Country", country_name));
    event.m_key_value_data.push_back(std::make_pair("State/Prov", state_or_prov_name));

    event.m_key_value_data.push_back(std::make_pair("Strangeness", string_format("%u", pRec->get_strangeness())));
    event.m_key_value_data.push_back(std::make_pair("Credibility", string_format("%u", pRec->get_credibility())));

    const uint32_t locale = pRec->get_locale();
    if (locale < std::size(g_hatch_locales))
        event.m_key_value_data.push_back(std::make_pair("Locale", g_hatch_locales[locale]));

    if (pRec->get_elevation() != -99)
        event.m_key_value_data.push_back(std::make_pair("Elev", string_format("%i", pRec->get_elevation())));
    
    if ((pRec->get_rel_altitude() != 0) && (pRec->get_rel_altitude() != 999))
        event.m_key_value_data.push_back(std::make_pair("RelAlt", string_format("%i", pRec->get_rel_altitude())));
    
    return true;
}

bool udb_convert()
{
    uint8_vec udb;
    if (!read_binary_file("u.rnd", udb))
        return false;

    const uint32_t TOTAL_RECS = 18123;
    if ((udb.size() / UDB_RECORD_SIZE) < TOTAL_RECS)
        panic("Invalid file size");

    const udb_rec* pRecs = reinterpret_cast<const udb_rec*>(&udb.front());

    ufo_timeline timeline;

    for (uint32_t rec_index = 1; rec_index < TOTAL_RECS; rec_index++)
    {
        const udb_rec* pRec = pRecs + rec_index;

        timeline_event event;
        if (!convert_rec(rec_index, pRec, event))
            continue;

        timeline.get_events().push_back(event);
    }

    if (!timeline.get_events().size())
        panic("Empty timeline)");

    timeline.set_name("Hatch_UDB_Timeline");

    return timeline.write_file("hatch_udb.json", true);
}








