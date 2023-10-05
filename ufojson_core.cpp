// ufojson_core.cpp
// Copyright (C) 2023 Richard Geldreich, Jr.
#include "ufojson_core.h"
#include "markdown_proc.h"

#define TIMELINE_VERSION "1.46"
#define COMPILATION_DATE "10/3/2023"

// Note that May ends in a period.
const char* g_months[12] =
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

const char* g_full_months[12] =
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

const char* g_date_prefix_strings[NUM_DATE_PREFIX_STRINGS] =
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

const char* g_day_of_week[7] =
{
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
    "Thursday",
    "Friday",
    "Saturday"
};

bool is_season(date_prefix_t prefix)
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

int determine_month(const std::string& date, bool begins_with)
{
    uint32_t i;
    for (i = 0; i < 12; i++)
    {
        if (begins_with)
        {
            if (string_begins_with(date, g_full_months[i]))
                return i;
        }
        else
        {
            if (string_icompare(date, g_full_months[i]) == 0)
                return i;
        }
    }
    return -1;
}

int determine_prefix(const std::string& date, bool begins_with)
{
    uint32_t i;
    for (i = 0; i < NUM_DATE_PREFIX_STRINGS; i++)
    {
        if (begins_with)
        {
            if (string_begins_with(date, g_date_prefix_strings[i]))
                return i;
        }
        else
        {
            if (string_icompare(date, g_date_prefix_strings[i]) == 0)
                return i;
        }
    }
    return -1;
}

int determine_day_of_week(const std::string& date, bool begins_with)
{
    uint32_t i;
    for (i = 0; i < 7; i++)
    {
        if (begins_with)
        {
            if (string_begins_with(date, g_day_of_week[i]))
                return i;
        }
        else
        {
            if (string_icompare(date, g_day_of_week[i]) == 0)
                return i;
        }
    }
    return -1;
}

event_date::event_date()
{
    clear();
}

event_date::event_date(const event_date& other)
{
    *this = other;
}

bool event_date::sanity_check() const
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

bool event_date::operator== (const event_date& rhs) const
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

bool event_date::operator!= (const event_date& rhs) const
{
    return !(*this == rhs);
}

event_date& event_date::operator =(const event_date& rhs)
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

void event_date::clear()
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

bool event_date::is_valid() const
{
    return m_year != -1;
}

std::string event_date::get_string() const
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
        sprintf_s(buf, "%i", m_year);
        res += buf;
    }
    else if (m_day == -1)
    {
        assert(m_month >= 1 && m_month <= 12);

        char buf[256];
        sprintf_s(buf, "%u/%i", m_month, m_year);
        res += buf;
    }
    else
    {
        assert(m_month >= 1 && m_month <= 12);

        char buf[256];
        sprintf_s(buf, "%u/%u/%i", m_month, m_day, m_year);
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

// Parses basic dates (not ranges). Works with dates returned by get_string().
// Date can end in "(approximate)", "(estimated)", "?", or "'s".
// 2 digit dates converted to 1900+.
// Supports year, month/year, or month/day/year.
bool event_date::parse(const char* pStr, bool fix_20century_dates)
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

    if (string_ends_in(temp, "?"))
    {
        m_fuzzy = true;
        temp.resize(temp.size() - 1);

        string_trim(temp);
    }

    if (string_ends_in(temp, "'s"))
    {
        m_plural = true;
        temp.resize(temp.size() - 2);

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
    
    if (fix_20century_dates)
    {
        if ((m_year >= 1) && (m_year <= 99))
            m_year += 1900;
    }

    if ((m_year < 0) || (m_year > 2100))
        return false;

    return true;
}

// More advanced date range parsing, used for converting the Eberhart timeline.
// Note this doesn't support "(approximate)", "(estimated)", or converting 2 digit years to 1900's.
bool event_date::parse_eberhart_date_range(std::string date,
    event_date& begin_date,
    event_date& end_date, event_date& alt_date,
    int required_year)
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

        if (((i + 1) < tokens.size()) && isalpha((uint8_t)tokens[i][0]) && isalpha((uint8_t)tokens[i + 1][0]))
        {
            uint32_t a;
            for (a = cEarly; a <= cLate; a++)
                if (string_begins_with(tokens[i], g_date_prefix_strings[a]))
                    break;

            uint32_t b;
            for (b = cSpring; b <= cWinter; b++)
                if (string_begins_with(tokens[i + 1], g_date_prefix_strings[b]))
                    break;

            if ((a <= cLate) && (b <= cWinter))
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

            bool expecting_year = false;

            if (string_ends_in(s, "?"))
            {
                if (d.m_fuzzy)
                    return false;

                d.m_fuzzy = true;

                s.pop_back();
            }
            else if (string_ends_in(s, "'s"))
            {
                if (d.m_plural)
                    return false;

                d.m_plural = true;
                
                s.pop_back();
                s.pop_back();

                expecting_year = true;
            }
            else if (string_ends_in(s, "s"))
            {
                if (d.m_plural)
                    return false;

                d.m_plural = true;

                s.pop_back();

                expecting_year = true;
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
                    if (expecting_year)
                        return false;

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
                    if (expecting_year)
                        return false;

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
void event_date::get_sort_date(int& year, int& month, int& day) const
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
#if 0
                if (is_century)
                    year += 10;
                else if (is_decade)
                    year += 1;
#endif
                day = -1;
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
bool event_date::compare(const event_date& lhs, const event_date& rhs)
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

bool event_date::check_date_prefix(const event_date& date)
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

static void get_date_range(const event_date& evt, event_date& begin, event_date& end)
{
    assert(evt.is_valid());

    begin.clear();
    end.clear();

    begin.m_year = evt.m_year;
    end.m_year = evt.m_year;

    const bool has_prefix = (evt.m_prefix != cNoPrefix);

    if ((evt.m_month == -1) && (evt.m_day == -1))
    {
        // Year only

        if (has_prefix)
        {
            switch (evt.m_prefix)
            {
            case cEarlySpring:
                begin.m_month = 3;
                begin.m_day = 20;
                end.m_month = 4;
                end.m_day = 10;
                break;
            case cEarlySummer:
                begin.m_month = 6;
                begin.m_day = 21;
                end.m_month = 7;
                end.m_day = 10;
                break;
            case cEarlyAutumn:
            case cEarlyFall:
                begin.m_month = 9;
                begin.m_day = 23;
                end.m_month = 10;
                end.m_day = 10;
                break;
            case cEarlyWinter:
                begin.m_month = 12;
                begin.m_day = 21;
                end.m_month = 1;
                end.m_day = 10;
                break;
            case cMidSpring:
                begin.m_month = 4;
                begin.m_day = 11;
                end.m_month = 5;
                end.m_day = 10;
                break;
            case cMidSummer:
                begin.m_month = 7;
                begin.m_day = 11;
                end.m_month = 8;
                end.m_day = 10;
                break;
            case cMidAutumn:
            case cMidFall:
                begin.m_month = 10;
                begin.m_day = 11;
                end.m_month = 11;
                end.m_day = 10;
                break;
            case cMidWinter:
                begin.m_month = 1;
                begin.m_day = 11;
                end.m_month = 2;
                end.m_day = 10;
                break;
            case cLateSpring:
                begin.m_month = 5;
                begin.m_day = 11;
                end.m_month = 6;
                end.m_day = 20;
                break;
            case cLateSummer:
                begin.m_month = 8;
                begin.m_day = 11;
                end.m_month = 9;
                end.m_day = 20;
                break;
            case cLateAutumn:
            case cLateFall:
                begin.m_month = 11;
                begin.m_day = 11;
                end.m_month = 12;
                end.m_day = 20;
                break;
            case cLateWinter:
                begin.m_month = 2;
                begin.m_day = 11;
                end.m_month = 3;
                end.m_day = 19;
                break;
            case cSpring:
                begin.m_month = 3;
                begin.m_day = 20;
                end.m_month = 6;
                end.m_day = 20;
                break;
            case cSummer:
                begin.m_month = 6;
                begin.m_day = 21;
                end.m_month = 9;
                end.m_day = 22;
                break;
            case cAutumn:
            case cFall:
                begin.m_month = 9;
                begin.m_day = 23;
                end.m_month = 12;
                end.m_day = 20;
                break;
            case cWinter:
                begin.m_month = 12;
                begin.m_day = 21;
                end.m_month = 3;
                end.m_day = 19;
                break;
            case cEarly:
                if ((evt.m_plural) && ((evt.m_year % 100) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;
                    end.m_year = evt.m_year + 33;
                }
                else if ((evt.m_plural) && ((evt.m_year % 10) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;
                    end.m_year = evt.m_year + 3;
                }
                else
                {
                    begin.m_month = 1;
                    begin.m_day = 1;
                    end.m_month = 4;
                    end.m_day = 30;
                }

                break;
            case cMiddleOf:
                if ((evt.m_plural) && ((evt.m_year % 100) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;

                    begin.m_year = evt.m_year + 40;
                    end.m_year = evt.m_year + 60;
                }
                else if ((evt.m_plural) && ((evt.m_year % 10) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;

                    begin.m_year = evt.m_year + 4;
                    end.m_year = evt.m_year + 6;
                }
                else
                {
                    begin.m_month = 5;
                    begin.m_day = 1;
                    end.m_month = 8;
                    end.m_day = 31;
                }
                break;
            case cLate:
                if ((evt.m_plural) && ((evt.m_year % 100) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;

                    begin.m_year = evt.m_year + 70;
                    end.m_year = evt.m_year + 99;
                }
                else if ((evt.m_plural) && ((evt.m_year % 10) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;
                    begin.m_year = evt.m_year + 7;
                    end.m_year = evt.m_year + 9;
                }
                else
                {
                    begin.m_month = 9;
                    begin.m_day = 1;
                    end.m_month = 12;
                    end.m_day = 31;
                }
                break;
            case cEndOf:
                if ((evt.m_plural) && ((evt.m_year % 100) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;
                    begin.m_year = evt.m_year + 80;
                    end.m_year = evt.m_year + 99;
                }
                else if ((evt.m_plural) && ((evt.m_year % 10) == 0))
                {
                    begin.m_month = 1;
                    begin.m_day = 1;

                    end.m_month = 12;
                    end.m_day = 31;
                    begin.m_year = evt.m_year + 8;
                    end.m_year = evt.m_year + 9;
                }
                else
                {
                    begin.m_month = 11;
                    begin.m_day = 1;
                    end.m_month = 12;
                    end.m_day = 31;
                }
                break;
            }
        }
        else
        {
            begin.m_month = 1;
            begin.m_day = 1;

            end.m_month = 12;
            end.m_day = 31;

            if ((evt.m_plural) && ((evt.m_year % 100) == 0))
                end.m_year = evt.m_year + 99;
            else if ((evt.m_plural) && ((evt.m_year % 10) == 0))
                end.m_year = evt.m_year + 9;
        }
    }
    else if (evt.m_day == -1)
    {
        // month must be valid here
        assert(evt.m_month >= 1);

        // Month/year
        begin.m_month = evt.m_month;
        begin.m_day = 1;

        end.m_month = evt.m_month;
        end.m_day = 31; // doesn't need to be always valid, just has to always encompass the entire month

        if (has_prefix)
        {
            switch (evt.m_prefix)
            {
            case cEarly:
                begin.m_day = 1;
                end.m_day = 9;
                break;
            case cMiddleOf:
                begin.m_day = 10;
                end.m_day = 19;
                break;
            case cLate:
                begin.m_day = 20;
                end.m_day = 31;
                break;
            case cEndOf:
                begin.m_day = 25;
                end.m_day = 31;
                break;
            default:
                printf("Invalid prefix\n");
                break;
            }
        }
    }
    else
    {
        // Month/day/year
        begin.m_month = evt.m_month;
        begin.m_day = evt.m_day;

        end.m_month = evt.m_month;
        end.m_day = evt.m_day;
    }
}

static bool check_event_date_interval(const event_date& evt_begin, const event_date& evt_end)
{
    if ((evt_begin.m_year < 0) || (evt_begin.m_month <= 0) || (evt_begin.m_day <= 0))
        return false;

    if ((evt_end.m_year < 0) || (evt_end.m_month <= 0) || (evt_end.m_day <= 0))
        return false;

    if (evt_end.m_year < evt_begin.m_year)
    {
        return false;
    }
    else if (evt_end.m_year == evt_begin.m_year)
    {
        if (evt_end.m_month < evt_begin.m_month)
        {
            return false;
        }
        else if (evt_end.m_month == evt_begin.m_month)
        {
            if (evt_end.m_day < evt_begin.m_day)
            {
                return false;
            }
        }
    }
    return true;
}

static int compute_yearday(int month, int day, int year)
{
    assert((month >= 1) && (month <= 12));
    assert((day >= 1) && (day <= 31));
    assert(year >= 0);

    return (year * 12 * 32) + (month - 1) * 32 + (day - 1);
}

// false=reject event
// true=accept event
static bool date_filter(
    int start_month, int start_day, int start_year,
    const event_date& evt_begin, const event_date& evt_end)
{
    // Ensure we've got a valid interval with a single span.
    if (!check_event_date_interval(evt_begin, evt_end))
    {
        panic("Invalid date range");
        return false;
    }

    if ((start_month >= 1) && (start_day >= 1) && (start_year >= 0))
    {
        // user has provided a valid month/day/year, and the event intervals are guaranteed to be fully valid, so compute the yeardays and compare

        const int evt_begin_yearday = compute_yearday(evt_begin.m_month, evt_begin.m_day, evt_begin.m_year);
        const int evt_end_yearday = compute_yearday(evt_end.m_month, evt_end.m_day, evt_end.m_year);
        assert(evt_begin_yearday <= evt_end_yearday);

        const int32_t yearday = compute_yearday(start_month, start_day, start_year);
        if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
            return true;

        return false;
    }

    if (start_month >= 1)
    {
        // Just a month, but they could have specified a year.
        if (start_year >= 0)
        {
            // month/year
            // user provided a valid month/year, do an interval check
            const int evt_begin_yearday = compute_yearday(evt_begin.m_month, 1, evt_begin.m_year);
            const int evt_end_yearday = compute_yearday(evt_end.m_month, 31, evt_end.m_year);
            assert(evt_begin_yearday <= evt_end_yearday);

            const int32_t yearday = compute_yearday(start_month, 15, start_year);
            if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
                return true;
        }
        else if (start_day != -1)
        {
            // month, day, no year
            // user has provided only a valid month, no year, try an interval check
            const int evt_begin_yearday = compute_yearday(evt_begin.m_month, evt_begin.m_day, 0);
            const int evt_end_yearday = compute_yearday(evt_end.m_month, evt_end.m_day, evt_end.m_year - evt_begin.m_year);
            assert(evt_begin_yearday <= evt_end_yearday);

            // we have a complete month interval for the event, starting at year 0

            // first try the month in the first year
            int32_t yearday = compute_yearday(start_month, start_day, 0);
            if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
                return true;

            // try 1 year later
            yearday = compute_yearday(start_month, start_day, 1);
            if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
                return true;
        }
        else
        {
            // month, no year
            // user has provided only a valid month, no year, try an interval check
            const int evt_begin_yearday = compute_yearday(evt_begin.m_month, 1, 0);
            const int evt_end_yearday = compute_yearday(evt_end.m_month, 31, evt_end.m_year - evt_begin.m_year);
            assert(evt_begin_yearday <= evt_end_yearday);

            // we have a complete month interval for the event, starting at year 0

            // first try the month in the first year
            int32_t yearday = compute_yearday(start_month, 15, 0);
            if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
                return true;

            // try 1 year later
            yearday = compute_yearday(start_month, 15, 1);
            if ((evt_begin_yearday <= yearday) && (yearday <= evt_end_yearday))
                return true;
        }
    }

    return false;
}

static bool date_filter(
    int start_month, int start_day, int start_year,
    int end_month, int end_day, int end_year,
    const event_date& evt_begin, const event_date& evt_end)
{
    assert(start_month >= 1 && start_month <= 12);
    assert(start_day >= 1 && start_day <= 31);
    assert(start_year >= 0);
    assert(end_month >= 1 && end_month <= 12);
    assert(end_day >= 1 && end_day <= 31);
    assert(end_year >= 0);
    assert(start_year <= end_year);

    // Ensure we've got a valid interval with a single span.
    if (!check_event_date_interval(evt_begin, evt_end))
    {
        panic("Invalid date range");
        return false;
    }

    const int evt_begin_yearday = compute_yearday(evt_begin.m_month, evt_begin.m_day, evt_begin.m_year);
    const int evt_end_yearday = compute_yearday(evt_end.m_month, evt_end.m_day, evt_end.m_year);
    assert(evt_begin_yearday <= evt_end_yearday);

    const int32_t start_yearday = compute_yearday(start_month, start_day, start_year);
    const int32_t end_yearday = compute_yearday(end_month, end_day, end_year);
    assert(start_yearday <= end_yearday);

    // now see if there's any overlap at all between the 2 spans
    // separating axis tests
    if (evt_end_yearday < start_yearday)
        return false;
    if (evt_begin_yearday > end_yearday)
        return false;

    return true;
}

// start_year must be valid
// false=reject event
// true=accept event
bool date_filter_single(
    int start_month, int start_day, int start_year,
    const event_date& evt_b, const event_date& evt_e)
{
    event_date evt_begin, evt_end;
    get_date_range(evt_b, evt_begin, evt_end);
    bool is_winter = (evt_begin.m_year == evt_end.m_year) && (evt_begin.m_month > evt_end.m_month);

    // evt_begin/evt_end both have valid day/months/years, but may be 1 or 2 spans (for winter)

    if (evt_e.is_valid())
    {
        event_date evt_begin2, evt_end2;
        get_date_range(evt_e, evt_begin2, evt_end2);

        // should be a valid date interval here with 1 span
        if (!check_event_date_interval(evt_begin, evt_end2))
        {
            printf("Invalid event range");
        }
        else
        {
            evt_end = evt_end2;
            is_winter = false;
        }
    }

    assert(evt_begin.m_year <= evt_end.m_year);

    // Filter by any user provided year
    if (start_year != -1)
    {
        if ((start_year < evt_begin.m_year) || (start_year > evt_end.m_year))
            return false;

        if ((start_month == -1) && (start_day == -1))
        {
            // User only provided a year to check, and it's in range so we're done.
            return true;
        }
    }

    // Special case if they are looking for specific days with no month, possibly a year (this is the Dr. Johnson case - so let's not get too fancy for now).
    if ((start_month == -1) && (start_day >= 1))
    {
        // If there's a prefix, it's not a specific day so don't accept it.
        if ((evt_b.m_prefix != cNoPrefix) || (evt_e.m_prefix != cNoPrefix))
            return false;
        assert(!is_winter);

        // Only check timeline events that have full month/day/year, otherwise we'll pull in too many vague events
        if ((evt_b.m_month >= 1) && (evt_b.m_day >= 1))
        {
            if (evt_b.m_day == start_day)
                return true;

            if (evt_e.is_valid() && (evt_e.m_month >= 1) && (evt_e.m_day >= 1))
            {
                // it's an event span
                if (evt_e.m_day == start_day)
                    return true;

                int next_month = evt_b.m_month + 1;
                if (next_month == 13)
                    next_month = 1;

                // Span is inside 1 month
                if (evt_b.m_month == evt_e.m_month)
                {
                    if ((evt_b.m_day <= start_day) && (start_day <= evt_e.m_day))
                        return true;
                }
                // Span is 2 months (anything larger would accept all days)
                else if (evt_e.m_month == next_month)
                {
                    if ((evt_b.m_day <= start_day) || (start_day <= evt_e.m_day))
                        return true;
                }
                // Span is 3>= months, so any day would fall within a full center month
                else
                {
                    return true;
                }
            }
        }

        return false;
    }

    // We've already handled the year, day/year, or day cases.
    // Now must be month/day/year, month/year.

    if (is_winter)
    {
        // Handle 2 spans in one year
        assert(evt_begin.m_year == evt_end.m_year);

        event_date e1, e2;
        e1 = evt_begin;

        e2.m_year = evt_begin.m_year;
        e2.m_month = 12;
        e2.m_day = 31;

        bool f1 = date_filter(start_month, start_day, start_year, e1, e2);
        if (f1)
            return true;

        event_date e3, e4;
        e3.m_year = evt_end.m_year;
        e3.m_day = 1;
        e3.m_month = 1;

        e4.m_year = evt_end.m_year;
        e4.m_month = evt_end.m_month;
        e4.m_day = evt_end.m_day;
        bool f2 = date_filter(start_month, start_day, start_year, e3, e4);
        if (f2)
            return true;
    }
    else
    {
        // Only has a single span, may span years
        return date_filter(start_month, start_day, start_year, evt_begin, evt_end);
    }

    return false;
}

// start_year/end_years must be valid
bool date_filter_range(
    int start_month, int start_day, int start_year,
    int end_month, int end_day, int end_year,
    const event_date& evt_b, const event_date& evt_e)
{
    if ((start_year < 0) || (end_year < 0) || (start_year > end_year))
        panic("Invalid year range");

    if (start_month < 1)
        start_month = 1;
    if (start_day < 1)
        start_day = 1;

    if (end_day < 1)
        end_day = 31;
    if (end_month < 1)
        end_month = 12;

    event_date evt_begin, evt_end;
    get_date_range(evt_b, evt_begin, evt_end);
    bool is_winter = (evt_begin.m_year == evt_end.m_year) && (evt_begin.m_month > evt_end.m_month);

    // evt_begin/evt_end both have valid day/months/years, but may be 1 or 2 spans (for winter)

    if (evt_e.is_valid())
    {
        event_date evt_begin2, evt_end2;
        get_date_range(evt_e, evt_begin2, evt_end2);

        // should be a valid date interval here with 1 span
        if (!check_event_date_interval(evt_begin, evt_end2))
        {
            printf("Invalid event range");
        }
        else
        {
            evt_end = evt_end2;
            is_winter = false;
        }
    }

    assert(evt_begin.m_year <= evt_end.m_year);

    if (is_winter)
    {
        // Handle 2 spans in one year
        assert(evt_begin.m_year == evt_end.m_year);

        event_date e1, e2;
        e1 = evt_begin;

        e2.m_year = evt_begin.m_year;
        e2.m_month = 12;
        e2.m_day = 31;

        bool f1 = date_filter(start_month, start_day, start_year, end_month, end_day, end_year, e1, e2);
        if (f1)
            return true;

        event_date e3, e4;
        e3.m_year = evt_end.m_year;
        e3.m_day = 1;
        e3.m_month = 1;

        e4.m_year = evt_end.m_year;
        e4.m_month = evt_end.m_month;
        e4.m_day = evt_end.m_day;
        bool f2 = date_filter(start_month, start_day, start_year, end_month, end_day, end_year, e3, e4);
        if (f2)
            return true;
    }
    else
    {
        // Only has a single span, may span years
        return date_filter(start_month, start_day, start_year, end_month, end_day, end_year, evt_begin, evt_end);
    }

    return false;
}

#if 0
void test()
{
    event_date b, e;

    b.m_plural = true;
    b.m_prefix = cEarly;

    b.m_month = -1;
    b.m_day = -1;
    b.m_year = 1900;

    //e.m_month = 6;
    //e.m_day = 3;
    //e.m_year = 1950;

    for (uint32_t y = 1899; y < 2001; y++)
    {
        uint32_t m = 6;
        //for (uint32_t m = 1; m <= 12; m++)
        {
            bool f = date_filter_single(m, -1, y, b, e);
            //bool f = date_filter_range(m, -1, 1950,   -1, -1, -1,   b, e);
            printf("%u, %u: %u\n", m, y, f);
        }
    }


}
#endif

bool timeline_event::operator==(const timeline_event& rhs) const
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
    COMP(m_key_value_data);
#undef COMP
    return true;
}

bool timeline_event::operator!=(const timeline_event& rhs) const
{
    return !(*this == rhs);
}

bool timeline_event::operator< (const timeline_event& rhs) const
{
    return event_date::compare(m_begin_date, rhs.m_begin_date);
}

void timeline_event::print(FILE* pFile) const
{
    fprintf(pFile, "**Date:** %s  \n", m_date_str.c_str());
    if (m_alt_date_str.size())
        fprintf(pFile, "**Alternate date:** %s  \n", m_alt_date_str.c_str());

    if (m_time_str.size())
        fprintf(pFile, "**Time:** %s  \n", m_time_str.c_str());

    if (m_end_date_str.size())
        fprintf(pFile, "**End date:** %s  \n", m_end_date_str.c_str());

    if (m_locations.size() > 1)
    {
        fprintf(pFile, "**Locations:** ");
        for (uint32_t i = 0; i < m_locations.size(); i++)
        {
            if (i)
                fprintf(pFile, "; ");
            fprintf(pFile, "%s", m_locations[i].c_str());
        }
        fprintf(pFile, "  \n");
    }
    else if (m_locations.size())
    {
        fprintf(pFile, "**Location:** %s  \n", m_locations[0].c_str());
    }

    fprintf(pFile, "**Description:** %s  \n", m_desc.c_str());

    for (uint32_t i = 0; i < m_type.size(); i++)
        fprintf(pFile, "**Type:** %s  \n", m_type[i].c_str());

    for (uint32_t i = 0; i < m_refs.size(); i++)
        fprintf(pFile, "**Reference:** %s  \n", m_refs[i].c_str());

    if (m_source.size() && m_source_id.size())
    {
        fprintf(pFile, "**Source:** %s, **ID:** %s  \n", m_source.c_str(), m_source_id.c_str());
    }
    else
    {
        if (m_source.size())
            fprintf(pFile, "**Source:** %s  \n", m_source.c_str());
        if (m_source_id.size())
            fprintf(pFile, "**Source ID:** %s  \n", m_source_id.c_str());
    }

    if (m_attributes.size())
    {
        fprintf(pFile, "  \n**Attributes:** ");

        for (uint32_t i = 0; i < m_attributes.size(); i++)
        {
            const std::string& x = m_attributes[i];
            if ((x.size() >= 5) && (x[3] == ':') && (uisupper(x[0]) && uisupper(x[1]) && uisupper(x[2])))
                fprintf(pFile, "**%s**%s", string_slice(x, 0, 4).c_str(), string_slice(x, 4).c_str());
            else
                fprintf(pFile, "%s", x.c_str());

            if (i != (m_attributes.size() - 1))
                fprintf(pFile, ", ");
        }

        fprintf(pFile, "  \n");
    }

    for (uint32_t i = 0; i < m_see_also.size(); i++)
        fprintf(pFile, "**See also:** %s  \n", m_see_also[i].c_str());

    if (m_rocket_type.size())
        fprintf(pFile, "**Rocket type:** %s  \n", m_rocket_type.c_str());
    if (m_rocket_altitude.size())
        fprintf(pFile, "**Rocket altitude:** %s  \n", m_rocket_altitude.c_str());
    if (m_rocket_range.size())
        fprintf(pFile, "**Rocket range:** %s  \n", m_rocket_range.c_str());

    if (m_atomic_type.size())
        fprintf(pFile, "**Atomic type:** %s  \n", m_atomic_type.c_str());
    if (m_atomic_kt.size())
        fprintf(pFile, "**Atomic KT:** %s  \n", m_atomic_kt.c_str());
    if (m_atomic_mt.size())
        fprintf(pFile, "**Atomic MT:** %s  \n", m_atomic_mt.c_str());

    if (m_key_value_data.size())
    {
        fprintf(pFile, "  \n**Extra Data:** ");

        uint32_t total_printed = 0;
        for (const auto& x : m_key_value_data)
        {
            fprintf(pFile, "**%s:** \"%s\"", x.first.c_str(), x.second.c_str());
            total_printed++;

            if (total_printed != m_key_value_data.size())
            {
                fprintf(pFile, ", ");
            }
        }
        if (total_printed)
            fprintf(pFile, "  \n");
    }
}

void timeline_event::from_json(const json& obj, const char* pSource_override, bool fix_20century_dates)
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
    if (!m_begin_date.parse(m_date_str.c_str(), fix_20century_dates))
        panic("Failed parsing date %s\n", m_date_str.c_str());
        
    if (end_date != obj.end())
    {
        m_end_date_str = (*end_date);

        if (!m_end_date.parse(m_end_date_str.c_str(), fix_20century_dates))
            panic("Failed parsing end date %s\n", m_end_date_str.c_str());
    }

    if (alt_date != obj.end())
    {
        m_alt_date_str = (*alt_date);

        if (!m_alt_date.parse(m_alt_date_str.c_str(), fix_20century_dates))
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

    m_key_value_data.clear();

    const json::const_iterator udb_data = obj.find("key_vals");
    if (udb_data != obj.end())
    {
        if (!udb_data->is_object())
            panic("Expected udb object");

        for (auto x = udb_data->begin(); x != udb_data->end(); ++x)
        {
            if (!x->is_string())
                panic("Expected string");

            if (!x.key().size() || !x.value().size())
                panic("Empty strings");

            m_key_value_data.push_back(std::make_pair(x.key(), x.value()));
        }
    }
}

void timeline_event::to_json(json& j) const
{
    j = json::object();

    j["date"] = m_date_str;

    if (m_desc.size())
        j["desc"] = m_desc;

#if 0
    if (m_plain_desc.size())
        j["plain_desc"] = m_plain_desc;
#endif

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

#if 0
    if (m_plain_refs.size() > 1)
        j["plain_ref"] = m_plain_refs;
    else if (m_plain_refs.size())
        j["plain_ref"] = m_plain_refs[0];
#endif

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

    if (m_key_value_data.size())
    {
        auto &udb_obj = (j["key_vals"] = json::object());
        for (const auto& x : m_key_value_data)
        {
            assert(x.first.size() && x.second.size());

            udb_obj[x.first] = x.second;
        }
    }

    if (m_search_words.size())
    {
        j["search"] = m_search_words;
    }
}

uint32_t timeline_event::get_crc32() const
{
    uint32_t hash = crc32((const uint8_t*)m_desc.c_str(), m_desc.size());

    hash = crc32((const uint8_t*)&m_begin_date.m_year, sizeof(m_begin_date.m_year), hash);
    hash = crc32((const uint8_t*)&m_begin_date.m_month, sizeof(m_begin_date.m_month), hash);
    hash = crc32((const uint8_t*)&m_begin_date.m_day, sizeof(m_begin_date.m_day), hash);

    return hash;
}

void ufo_timeline::create_plaintext()
{
    for (uint32_t i = 0; i < m_events.size(); i++)
    {
        timeline_event& te = m_events[i];

        if (te.m_desc.size())
        {
            markdown_text_processor tp;
            tp.init_from_markdown(te.m_desc.c_str());
            tp.convert_to_plain(te.m_plain_desc, true);
        }

        if (te.m_refs.size())
        {
            te.m_plain_refs.resize(te.m_refs.size());

            for (uint32_t j = 0; j < te.m_refs.size(); j++)
            {
                markdown_text_processor tp;
                tp.init_from_markdown(te.m_refs[j].c_str());
                tp.convert_to_plain(te.m_plain_refs[j], true);
            }
        }

        string_vec plain_locations(te.m_locations.size());
        for (uint32_t j = 0; j < te.m_locations.size(); j++)
        {
            markdown_text_processor tp;
            tp.init_from_markdown(te.m_locations[j].c_str());
            tp.convert_to_plain(plain_locations[j], true);
        }

        string_vec words;
        get_string_words(te.m_plain_desc, words, nullptr, "-");
        
        for (uint32_t j = 0; j < te.m_plain_refs.size(); j++)
        {
            string_vec temp_words;
            get_string_words(te.m_plain_refs[j], temp_words, nullptr, "-");
            
            words.insert(words.end(), temp_words.begin(), temp_words.end());
        }

        for (uint32_t j = 0; j < plain_locations.size(); j++)
        {
            string_vec temp_words;
            get_string_words(plain_locations[j], temp_words, nullptr, "-");

            words.insert(words.end(), temp_words.begin(), temp_words.end());
        }

        string_vec new_words;
        for (uint32_t j = 0; j < words.size(); j++)
        {
            std::string tmp(ustrlwr(words[j]));
            if (!tmp.size() || is_stop_word(tmp))
                continue;
            
            std::string nrm_tmp(normalize_word(tmp));

            if (!nrm_tmp.size() || is_stop_word(nrm_tmp))
                continue;
                           
            new_words.push_back(nrm_tmp);
        }

        for (uint32_t j = 0; j < new_words.size(); j++)
        {
            if (te.m_search_words.size())
                te.m_search_words.push_back(' ');

            te.m_search_words += new_words[j];
        }
    }
}

bool ufo_timeline::write_markdown(const char* pTimeline_filename, const char *pDate_range_desc, int begin_year, int end_year, bool single_file_output, event_urls_map_t &event_urls, bool output_kwic_directory)
{
    const std::vector<timeline_event>& timeline_events = m_events;

    uint32_t first_event_index = UINT32_MAX, last_event_index = 0;
    for (uint32_t i = 0; i < timeline_events.size(); i++)
    {
        const int y = timeline_events[i].m_begin_date.m_year;

        if ((y >= begin_year) && (y <= end_year))
        {
            first_event_index = std::min(first_event_index, i);
            last_event_index = std::max(last_event_index, i);
        }
    }
    
    if (first_event_index > last_event_index)
        panic("Can't find events");

    std::string html_filename(pTimeline_filename);
    size_t dot_ofs = html_filename.find_last_of('.');
    if (dot_ofs != std::string::npos)
        html_filename = string_slice(html_filename, 0, dot_ofs) + ".html";

    FILE* pTimeline_file = ufopen(pTimeline_filename, "w");
    if (!pTimeline_file)
        panic("Failed creating file %s", pTimeline_file);

    fputc(UTF8_BOM0, pTimeline_file);
    fputc(UTF8_BOM1, pTimeline_file);
    fputc(UTF8_BOM2, pTimeline_file);
    fprintf(pTimeline_file, "<meta charset=\"utf-8\">\n");
        
    if ((pDate_range_desc) && (strlen(pDate_range_desc)))
        fprintf(pTimeline_file, "\n# <a name=\"Top\">UFO/UAP Event Chronology, %s, v" TIMELINE_VERSION " - Compiled " COMPILATION_DATE "</a>\n\n", pDate_range_desc);
    else
        fprintf(pTimeline_file, "\n# <a name=\"Top\">UFO/UAP Event Chronology, v" TIMELINE_VERSION " - Compiled " COMPILATION_DATE "</a>\n\n");

    fputs(
        u8R"(An automated compilation by <a href="https://twitter.com/richgel999">Richard Geldreich, Jr.</a> using public data from <a href="https://en.wikipedia.org/wiki/Jacques_Vall%C3%A9e">Dr. Jacques Vallée</a>,
<a href="https://www.academia.edu/9813787/GOVERNMENT_INVOLVEMENT_IN_THE_UFO_COVER_UP_CHRONOLOGY_based">Pea Research</a>, <a href="http://www.cufos.org/UFO_Timeline.html">George M. Eberhart</a>,
<a href="https://en.wikipedia.org/wiki/Richard_H._Hall">Richard H. Hall</a>, <a href="https://web.archive.org/web/20160821221627/http://www.ufoinfo.com/onthisday/sametimenextyear.html">Dr. Donald A. Johnson</a>,
<a href="https://medium.com/@richgel99/1958-keziah-poster-recreation-completed-82fdb55750d8">Fred Keziah</a>, <a href="https://github.com/richgel999/uap_resources/blob/main/bluebook_uncensored_unknowns_don_berliner.pdf">Don Berliner</a>,
<a href="https://www.openminds.tv/larry-hatch-ufo-database-creator-remembered/42142">Larry Hatch</a>, [NICAP](https://www.nicap.org/), [Thomas R. Adams](https://www.lulu.com/shop/ray-boeche/bloodless-cuts/hardcover/product-22167360.html?page=1&pageSize=4), [George D. Fawcett](https://archive.ph/eQwIL), [Chris Aubeck](https://books.google.com/books/about/Return_to_Magonia.html?id=JBGNjgEACAAJ&source=kp_author_description), [Philip L. Rife](https://www.amazon.com/Didnt-Start-Roswell-Encounters-Coverups/dp/059517339X), [Richard Dolan](https://richarddolanmembers.com/), [Jérôme Beau](https://rr0.org/), [Godelieve Van Overmeire](http://cobeps.org/fr/godelieve-van-overmeire), and an anonymous individual or group.

## Some non-summarized events fall under one of these copyrights:
- Richard Geldreich, Jr. - Copyright (c) 2023 (events marked \"maj2\" unless otherwise attributed)
- Dr. Jacques F. Vallée - Copyright (c) 1993
- LeRoy Pea - Copyright (c) 9/8/1988 (updated 3/17/2005)
- George M. Eberhart - Copyright (c) 2022
- Dr. Donald A. Johnson - Copyright (c) 2012
- Fred Keziah - Copyright (c) 1958
- Larry Hatch - Copyright (c) 1992-2002
- Thomas R. Adams - Copyright (c) 1991
- Richard Dolan - Copyright (c) 2002
- Jérôme Beau - Copyright (c) 2000-2023

## Update History:
- v1.46: Adding ~3700 events, translated from the French chronology [_Mini catalogue chronologique des observations OVNI_](https://web.archive.org/web/20060107070423/http://users.skynet.be/sky84985/chrono.html) by Belgian ufologist [Godelieve Van Overmeire, 1935-2021](http://cobeps.org/fr/godelieve-van-overmeire). Note these events are from the old HTML version on archive.org, not the larger [(10k event) PDF version](http://www.cobeps.org/pdf/Chronologie-OVNI-VOG.pdf). It is unclear if these events are copyrighted. I didn't see a copyright in either the HTML or PDF versions.
- v1.43: Added ~3160 events, translated from a French chronology to English using OpenAI, from [rr0.org](https://rr0.org/). I believe this chronology was composed by Jérôme Beau. Its license is [here](https://rr0.org/Copyright.html).
- v1.40: Added digitized events/newspaper clippings from [Frank Scully's papers at the American Heritage Center in Laramie, WY](https://archiveswest.orbiscascade.org/ark:80444/xv506256), summarized the events from the timeline on the [Disclosure Diaries](https://www.disclosurediaries.com/) website, and added more misc. events. Fixed auto-translation issue in the search page.
- v1.38: Added a [client-side search engine](search.html). There are a bunch of features I'm going to add to this engine, for now it can only search for keywords in the desc, location and and reference fields.
- v1.37: Updated intro text, added total number of events to each event year, added a few 1800's events.
- v1.36: Extracted and summarized the events in the book [_It Didn't Start with Roswell_ by Philip L. Rife](https://www.amazon.com/Didnt-Start-Roswell-Encounters-Coverups/dp/059517339X). Also extracted the military UFO events from Richard Dolan's book [_UFOs and the National Security State: Chronology of a Cover-up, 1941–1973_](https://www.amazon.com/UFOs-National-Security-State-Chronology-ebook/dp/B0C94W38QY).
- v1.34: Added more modern events, 1917 Mystery Airplane newspaper articles.
- v1.33: More events: Events from George D. Fawcett, short AI summaries of Stringfield's 1978 MUFON symposium presentation, and short AI summaries of the pre-industrial era sighting events from the book [_Wonders in the Sky: Unexplained Aerial Objects from Antiquity to Modern Times_](https://www.amazon.com/Wonders-Sky-Unexplained-Objects-Antiquity/dp/1585428205).
- v1.30: Added 203 Mystery Helicopter/mutilation related events (1970's-1980's) compiled by author/researcher [Thomas R. Adams](https://www.lulu.com/shop/ray-boeche/bloodless-cuts/hardcover/product-22167360.html?page=1&pageSize=4) (1945-2015) (or see [here](http://copycateffect.blogspot.com/2018/06/Adams-Massey-Obits.html)), from his book [_The Choppers - and the Choppers, Mystery Helicopters and Animal Mutilations_](http://www.ignaciodarnaude.com/avistamientos_ovnis/Adams,Thomas,Choppers%20and%20the%20Choppers-1.pdf), minor fixes 
- v1.28: Added KWIC (Key Word in Context) index.
- v1.27: Imported Anonymous PDF's contents, originally from [here](https://pdfhost.io/v/gR8lAdgVd_Uap_Timeline_Prepared_By_Another), with fixed URL's
- v1.23-1.24: Added a handful of key historical events, such as Edward Tauss the head of CIA UFO disinformation in the 50's
- v1.22: Fixing the date of Dr. Eric W. Davis's March, 2020 classified briefing to the Senate (I had it listed as March 2019) - info from NY Times. Basic locations added to Eberhart records using OpenAI.
- v1.20: Split up into 5 parts, to work around iPhone web browser limits. Minor spelling and grammer fixes throughout timeline.
- v1.15: Eberhart records now have basic locations, thanks to OpenAI's Davinci-3 AI model. They aren't perfect, but it's a good start to geocoding them.
- v1.14: Added nuclear test data, over 2000 records, from the [_Worldwide Nuclear Explosions_](https://web.archive.org/web/20220407121213/https://www.ldeo.columbia.edu/~richards/my_papers/WW_nuclear_tests_IASPEI_HB.pdf) paper by Yang, North, Romney, and Richards. Note the locations in the paper are approximate, and the yields are not super accurate, which are two problems I'll fix over time. I improved the coordinates of the earliest USA/USSR tests by looking them up from Wikipedia.
- v1.13: Split up the timeline into 4 parts. Still not the best solution, but it avoids croaking browsers.
- v1.12: Added \*U\* database record data, using a custom event description decoder to handle his 1k+ abbreviations and custom syntax
- v1.11: Crawled all ~10.5k unique URL's in this timeline using [curl](https://curl.se/) and fixed dead URL's to use archive.org.
- v1.10: Added NICAP DB data.

## Important Notes:
Best viewed on a desktop/laptop, not a mobile device. On Windows, Firefox works best, followed by Edge, then Chrome.

I've split up the timeline into 4 parts, to reduce their sizes: distant past up to 1949, 1950-1959, 1960-1979, and 1980-present.

The majority of the events in this chronology are sighting related, however it's important to be aware that this is a timeline of 
UFO/UAP related _events_, not necessarily or exclusively UFO _sightings_. **This is not exclusively a UFO sightings timeline or database.**

Some sighting reports or events appear multiple times in this timeline because they appear in more than one data source. I view this as a useful feature.

Currently, the events are not sorted by time of day, only by date. Some sources have separate "time" fields, but most don't. This will be fixed once the event times are automatically extracted from the description fields.

A few events don't have firm dates, for example "Summer of 1947", or "Late July 1952". In these instances the compilation code uses fixed dates I selected for date sorting purposes. (See the code for the specific dates.)

## Source Code:
This website is created automatically using a [C++](https://en.wikipedia.org/wiki/C%2B%2B) command line tool called “ufojson”. It parses the raw text and [Markdown](https://en.wikipedia.org/wiki/Markdown) source data to [JSON format](https://www.json.org/json-en.html), which is then converted to a single large web page using [pandoc](https://pandoc.org/). This tool's source code and all of the raw source and JSON data is located [here on github](https://github.com/richgel999/ufo_data).)", pTimeline_file);

    fputs("\n", pTimeline_file);

    if (!single_file_output)
    {
        fputs("\n", pTimeline_file);

        fputs(
u8R"(## Year Ranges
1. [Part 1: Distant past up to and including 1949](timeline.html)
2. [Part 2: 1950 up to and including 1959](timeline_part2.html)
3. [Part 3: 1960 up to and including 1969](timeline_part3.html)
4. [Part 4: 1970 up to and including 1979](timeline_part4.html)
5. [Part 5: 1980 to present](timeline_part5.html))", pTimeline_file);

        fputs("\n", pTimeline_file);

        fprintf(pTimeline_file, "\n## [Timeline Search Engine](search.html)\n\n");
    }

    if (output_kwic_directory)
    {
        fputs("\n", pTimeline_file);
        fprintf(pTimeline_file, "\n## KWIC (Key Word in Context) Index\n\n");

        for (uint32_t j = 0; j < NUM_KWIC_FILE_STRINGS; j++)
        {
            std::string r(get_kwic_index_name(j));

            std::string url = string_format("[%s](kwic_%s.html)", r.c_str(), r.c_str());
            fprintf(pTimeline_file, "%s\n", url.c_str());
        }
    }

    fprintf(pTimeline_file, "\n## Table of Contents\n\n");

    fprintf(pTimeline_file, "<a href = \"#yearhisto\">Year Histogram</a>\n\n");

    std::set<uint32_t> year_set;
    int min_year = 9999, max_year = -10000;
    for (uint32_t i = first_event_index; i <= last_event_index; i++)
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
            fprintf(pTimeline_file, "<a href=\"#year%i\">%i</a> ", y, y);

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

    std::map<int, int> year_histogram;

    for (uint32_t i = first_event_index; i <= last_event_index; i++)
    {
        int year = timeline_events[i].m_begin_date.m_year;
        
        year_histogram[year] = year_histogram[year] + 1;
    }

    for (uint32_t i = first_event_index; i <= last_event_index; i++)
    {
        uint32_t hash = timeline_events[i].get_crc32();

        int year = timeline_events[i].m_begin_date.m_year;
        if ((year != cur_year) && (year > cur_year))
        {
            fprintf(pTimeline_file, "\n---\n\n");
            fprintf(pTimeline_file, "## <a name=\"year%u\">Year: %u</a>, %i Event(s), <a href=\"#Top\">(Back to Top)</a>\n\n", year, year, year_histogram[year]);

            fprintf(pTimeline_file, "### <a name=\"%08X\">Event %i (%08X)</a>\n", hash, i, hash);

            cur_year = year;
        }
        else
        {
            fprintf(pTimeline_file, "### <a name=\"%08X\"></a> Event %i (%08X)\n", hash, i, hash);
        }

        timeline_events[i].print(pTimeline_file);

        //year_histogram[year] = year_histogram[year] + 1;

        fprintf(pTimeline_file, "\n");

        //std::string url( string_format("[%s #%u](%s#%08X)", timeline_events[i].m_date_str.c_str(), i, html_filename.c_str(), hash) );
        //<a href = "https://www.example.com">link to Example.com< / a> inside the pre section.
        std::string url( string_format("<a href=\"%s#%08X\">%s #%u</a>", 
            html_filename.c_str(), hash,
            timeline_events[i].m_date_str.c_str(), i) );

        event_urls.insert(std::make_pair((int)i, url));
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

// Load a JSON timeline using nlohmann::json
bool ufo_timeline::load_json(const char* pFilename, bool& utf8_flag, const char* pSource_override, bool fix_20century_dates)
{
    std::vector<timeline_event>& timeline_events = m_events;

    std::vector<uint8_t> buf;

    if (!read_text_file(pFilename, buf, &utf8_flag))
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

        timeline_events[first_event_index + i].from_json(obj, pSource_override, fix_20century_dates);
    }

    return true;
}

