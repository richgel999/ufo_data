// ufojson_core.h
// Copyright (C) 2023 Richard Geldreich, Jr.
#pragma once
#include "utils.h"

// Note that May ends in a period.
extern const char* g_months[12];
extern const char* g_full_months[12];

const uint32_t NUM_DATE_PREFIX_STRINGS = 24;
extern const char* g_date_prefix_strings[NUM_DATE_PREFIX_STRINGS];

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

bool is_season(date_prefix_t prefix);
int determine_month(const std::string& date);

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

    event_date();
    
    event_date(const event_date& other);
    
    bool sanity_check() const;
    
    bool operator== (const event_date& rhs) const;
    
    bool operator!= (const event_date& rhs) const;
    
    event_date& operator =(const event_date& rhs);
    
    void clear();
    
    bool is_valid() const;
    
    std::string get_string() const;
    
    // Parses basic dates (not ranges). 
    // Date can end in "(approximate)", "(estimated)", "?", or "'s".
    // 2 digit dates converted to 1900+.
    // Supports year, month/year, or month/day/year.
    bool parse(const char* pStr, bool fix_20century_dates);
    
    // More advanced date range parsing, used for converting the Eberhart timeline.
    // Note this doesn't support "'s", "(approximate)", "(estimated)", or converting 2 digit years to 1900'.
    static bool parse_eberhart_date_range(std::string date,
        event_date& begin_date,
        event_date& end_date, event_date& alt_date,
        int required_year = -1);
    
    // Note the returned date may be invalid. It's only intended for sorting/comparison purposes against other sort dates.
    void get_sort_date(int& year, int& month, int& day) const;
    
    // Compares two timeline dates. true if lhs < rhs
    static bool compare(const event_date& lhs, const event_date& rhs);
    
private:

    static bool check_date_prefix(const event_date& date);
};

struct timeline_event
{
    std::string m_date_str;
    std::string m_time_str; // military 
        
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

    std::vector<string_pair> m_key_value_data;
            
    bool operator==(const timeline_event& rhs) const;

    bool operator!=(const timeline_event& rhs) const;

    bool operator< (const timeline_event& rhs) const;

    void print(FILE* pFile) const;
    
    void from_json(const json& obj, const char* pSource_override, bool fix_20century_dates);
    
    void to_json(json& j) const;

    uint32_t get_crc32() const;
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
        std::stable_sort(m_events.begin(), m_events.end());
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

    bool load_json(const char* pFilename, bool& utf8_flag, const char* pSource_override, bool fix_20century_dates);

    bool write_markdown(const char* pTimeline_filename, const char* pDate_range_desc, int begin_year, int end_year, bool single_file_output);

private:
    timeline_event_vec m_events;
    std::string m_name;
};
