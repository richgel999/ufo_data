// ufojson.cpp
// Copyright (C) 2023 Richard Geldreich, Jr.

#include "utils.h"
#include "markdown_proc.h"
#include "ufojson_core.h"
#include "udb.h"
#include "converters.h"

#include "utf8.h"

//-------------------------------------------------------------------

[[maybe_unused]] // currently unused...
static void detect_bad_urls()
{
    string_vec unique_urls;
    bool utf8_flag = false;
    if (!read_text_file("unique_urls.txt", unique_urls, true, &utf8_flag))
        panic("Can't read file unique_urls.txt");

    uint32_t total_processed = 0;

    string_vec failed_urls;
    for (const auto& url : unique_urls)
    {
        printf("-------------- URL : %u\n", total_processed);

        string_vec reply;
        bool status = invoke_curl(url, reply);
        bool not_found = false;

        if (status)
        {
            for (auto str : reply)
            {
                str = string_lower(str);

                if ((string_find_first(str, "404 not found") != -1) ||
                    (string_find_first(str, "cannot find the requested page") != -1))
                {
                    not_found = true;
                    break;
                }
            }
        }

        if ((!status) || (not_found))
        {
            failed_urls.push_back(url);
            printf("FAILED: %s\n", url.c_str());
        }
        else
        {
            printf("SUCCEEDED: %s\n", url.c_str());
        }

        total_processed++;
        if ((total_processed % 25) == 24)
        {
            if (!write_text_file("failed_urls.txt", failed_urls, false))
                panic("Unable to create file failed_urs.txt");
        }
    }

    printf("Total urls: %zu, failed: %zu\n", unique_urls.size(), failed_urls.size());

    if (!write_text_file("failed_urls.txt", failed_urls, false))
        panic("Unable to create file failed_urs.txt");
}

// Windows defaults to code page 437:
// https://www.ascii-codes.com/
// We want code page 1252
// http://www.alanwood.net/demos/ansi.html

static bool invoke_openai(const char* pPrompt_text, json& result)
{
    uprintf("Prompt: %s\n", pPrompt_text);

    std::string res_str;
    const uint32_t MAX_TRIES = 3;
    for (uint32_t try_index = 0; try_index < MAX_TRIES; try_index++)
    {
        bool status = invoke_openai(pPrompt_text, res_str);
        if (status)
            break;

        if (try_index == MAX_TRIES - 1)
        {
            fprintf(stderr, "invoke_openai() failed!\n");
            return false;
        }
    }

    uprintf("Result: %s\n", res_str.c_str());

    bool success = false;
    try
    {
        result = json::parse(res_str.begin(), res_str.end());
        success = true;
    }
    catch (json::exception& e)
    {
        fprintf(stderr, "json::parse() failed (id %i): %s", e.id, e.what());
        return false;
    }

    if (!result.is_object() || !result.size())
    {
        fprintf(stderr, "Invalid JSON!\n");
        return false;
    }

    return success;
}

static bool invoke_openai(const timeline_event &event, const char *pPrompt_text, json& result)
{
    markdown_text_processor tp;
    tp.init_from_markdown(event.m_desc.c_str());

    std::string desc;
    tp.convert_to_plain(desc, true);

    if ((desc.size() >= 2) && (desc.back() == '('))
        desc.pop_back();

    const uint32_t MAX_SIZE = 4096; // ~1024 tokens
    if (desc.size() > MAX_SIZE)
    {
        int i;
        for (i = MAX_SIZE; i >= 0; i--)
        {
            if (desc[i] == ' ')
            {
                desc.resize(i);
                break;
            }
        }
        if (i < 0)
            panic("Failed shrinking text");
    }

    uprintf("Desc: %s\n\n", desc.c_str());

    std::string prompt_str(pPrompt_text);
    prompt_str += desc;
    prompt_str += "\"";

    return invoke_openai(prompt_str.c_str(), result);
}

[[maybe_unused]] // currently unused...
static void process_timeline_using_openai(const ufo_timeline &timeline)
{
    bool utf8_flag;
    json existing_results;
    load_json_object("openai_results.json", utf8_flag, existing_results);

    json final_result = json::object();

    final_result["results"] = json::array();

    auto& results_array = final_result["results"];

    uint32_t total_failures = 0, total_hits = 0, total_new_results = 0;
    for (uint32_t i = 0; i < timeline.size(); i++)
    //for (uint32_t i = 8515; i < 8516; i++)
    {
        uprintf("****** Record: %u\n", i);

        const timeline_event& event = timeline[i];
        const uint32_t event_crc32 = event.get_crc32();

        json result_obj;
        bool found_flag = false;

        if (existing_results.is_object() && existing_results.size())
        {
            const auto& results_arr = existing_results["results"];
            for (auto it = results_arr.begin(); it != results_arr.end(); ++it)
            {
                const auto& obj = *it;
                if (obj.is_object())
                {
                    uint32_t peek_obj_crc32 = obj["event_crc32"].get<uint32_t>();
                    if (peek_obj_crc32 == event_crc32)
                    {
                        if (obj["event_source_id"] == event.m_source_id)
                        {
                            result_obj = obj;
                            found_flag = true;
                            break;
                        }
                    }
                }
            }
        }

        if (found_flag)
        {
            total_hits++;
            printf("Found hit for record %u\n", i);

            results_array.push_back(result_obj);

            continue;
        }

        if (!invoke_openai(event,
            //"Compose a JSON object from the following quoted text. Give an array (with the key \"locations\") containing all the geographic locations or addresses. Then an array (with the key \"index\") with all phrases that would appear in a book index. Then an array (with the key \"names\") with all the person names. \"",
            "Compose a JSON object from the following quoted text. Give an array (with the key \"locations\") containing all the geographic locations or addresses. Then an array (with the key \"index\") with all phrases that would appear in a book index. Then an array (with the key \"names\") with all the person names. For the locations, aggressively provide as few strings as possible so they can be geocoded. For example, if the input has \"County Waterford in Cappoquin, Ireland\", provide \"County Waterford, Cappoquin, Ireland\". \"",
            result_obj))
        {
            fprintf(stderr, "Invoke AI failed!");

            total_failures++;
            continue;
        }

        result_obj["event_index"] = i;

        result_obj["event_date_str"] = event.m_date_str;

        if (event.m_time_str.size())
            result_obj["event_time"] = event.m_time_str;

        result_obj["event_source"] = event.m_source;
        result_obj["event_source_id"] = event.m_source_id;

        result_obj["event_crc32"] = event_crc32;

        results_array.push_back(result_obj);

        total_new_results++;

        if ((total_new_results % 50) == 0)
        {
            serialize_to_json_file("temp_results.json", final_result, true);
        }
    }

    if (!results_array.size())
        panic("No results");

    if (!serialize_to_json_file("new_openai_results.json", final_result, true))
        panic("failed writing results.json");

    uprintf("Total hits: %u, new results: %u, failures: %u\n", total_hits, total_new_results, total_failures);

    uprintf("Success\n");
}

[[maybe_unused]] // currently unused...
static void process_timeline_using_python(const ufo_timeline& timeline)
{
    json final_result = json::object();

    final_result["results"] = json::array();

    auto& results_array = final_result["results"];

    for (uint32_t i = 0; i < timeline.size(); i++)
    //for (uint32_t i = 0; i < 5; i++)
    {
        uprintf("****** Record: %u\n", i);

        const timeline_event& event = timeline[i];

        string_vec lines;
        lines.push_back(event.m_desc);

        if (!write_text_file("input.txt", lines))
            panic("Failed writing input.txt");

        remove("locations.json");

        Sleep(50);

        int status = system("python.exe pextractlocs.py");
        if (status != EXIT_SUCCESS)
            panic("Failed running python.exe");

        json locs;
        bool utf8_flag;
        if (!load_json_object("locations.json", utf8_flag, locs))
        {
            Sleep(50);

            if (!load_json_object("locations.json", utf8_flag, locs))
                panic("failed reading locations.json");
        }

        for (auto it = locs.begin(); it != locs.end(); ++it)
        {
            if (it->is_string())
                uprintf("%s\n", it->get<std::string>().c_str());
        }

        json new_obj = json::object();
        new_obj.emplace("index", i);
        new_obj.emplace("date", event.m_date_str);
        new_obj.emplace("crc32", event.get_crc32());
        new_obj.emplace("locations", locs);

        results_array.push_back(new_obj);
    }

    if (!results_array.size())
        panic("No results");

    if (!serialize_to_json_file("new_results.json", final_result, true))
        panic("failed writing results.json");
}

enum
{
    gn_geonameid, // integer id of record in geonames database
    gn_name, // name of geographical point(utf8) varchar(200)
    gn_asciiname, // name of geographical point in plain ascii characters, varchar(200)
    gn_alternatenames, // alternatenames, comma separated, ascii names automatically transliterated, convenience attribute from alternatename table, varchar(10000)
    gn_latitude, // latitude in decimal degrees(wgs84)
    gn_longitude, // longitude in decimal degrees(wgs84)
    gn_feature_class, // see http ://www.geonames.org/export/codes.html, char(1)
    gn_feature_code, // see http ://www.geonames.org/export/codes.html, varchar(10)
    gn_country_code, // ISO - 3166 2 - letter country code, 2 characters
    gn_cc2, // alternate country codes, comma separated, ISO - 3166 2 - letter country code, 200 characters
    gn_admin1_code, // fipscode(subject to change to iso code), see exceptions below, see file admin1Codes.txt for display names of this code; varchar(20)
    gn_admin2_code, // code for the second administrative division, a county in the US, see file admin2Codes.txt; varchar(80)
    gn_admin3_code, // code for third level administrative division, varchar(20)
    gn_admin4_code, // code for fourth level administrative division, varchar(20)
    gn_population, // bigint(8 byte int)
    gn_elevation, // in meters, integer
    gn_dem, // digital elevation model, srtm3 or gtopo30, average elevation of 3''x3'' (ca 90mx90m) or 30''x30'' (ca 900mx900m) area in meters, integer.srtm processed by cgiar / ciat.
    gn_timezone, // the iana timezone id(see file timeZone.txt) varchar(40)
    gn_modification_date,
    gn_total
};

struct geoname
{
    std::string m_fields[gn_total];
};

typedef std::vector<geoname> geoname_vec;

static bool is_important_country(const std::string& s)
{
    return (s == "US") || (s == "GB") || (s == "AU") || (s == "CA") || (s == "NZ") || (s == "FR") || (s == "DE") || (s == "BR") || (s == "IT");
}

[[maybe_unused]] // currently unused...
static bool is_favored_country(const std::string& s)
{
    return (s == "US") || (s == "GB") || (s == "AU") || (s == "CA") || (s == "NZ") || (s == "FR") || (s == "DE");
}

const uint32_t TOTAL_FAVORED_COUNTRY_RANKS = 8;

static int get_favored_country_rank(const std::string& s)
{
    if (s == "US")
        return 0;
    else if (s == "CA")
        return 1;
    else if (s == "GB")
        return 2;
    else if (s == "AU")
        return 3;
    else if (s == "NZ")
        return 4;
    else if (s == "FR")
        return 5;
    else if (s == "DE")
        return 6;

    return 7;
}

[[maybe_unused]] // currently unused...
static bool is_country_fcode(const std::string &fcode)
{
    return ((fcode == "PCL") || (fcode == "PCLD") || (fcode == "PCLF") || (fcode == "PCLH") || (fcode == "PCLI") || (fcode == "PCLIX") || (fcode == "PCLS") || (fcode == "TERR"));
}

[[maybe_unused]] // currently unused...
static void process_geodata()
{
    string_vec lines;
    lines.reserve(13000000);

    if (!read_text_file("allCountries.txt", lines, true, nullptr))
        panic("failed reading allCountries.txt");

    int_vec tab_locs;
    tab_locs.reserve(32);

    geoname_vec geonames;
    geonames.resize(13000000);

    uint32_t total_geonames = 0;

    uint32_t max_col_sizes[gn_total];
    clear_obj(max_col_sizes);

    std::unordered_map<char, uint32_t> accepted_class_counts;
    std::unordered_map<char, uint32_t> rejected_class_counts;

    uint32_t total_accepted = 0;

    json output_json = json::array();

    for (const auto& str : lines)
    {
        tab_locs.resize(0);

        for (uint32_t ofs = 0; ofs < str.size(); ofs++)
            if (str[ofs] == '\t')
                tab_locs.push_back(ofs);

        if (tab_locs.size() != 18)
            panic("Unexpected number of tabs");

        tab_locs.push_back((uint32_t)str.size());

        geoname& g = geonames[total_geonames];
        uint32_t cur_ofs = 0;
        for (uint32_t i = 0; i < gn_total; i++)
        {
            g.m_fields[i] = string_slice(str, cur_ofs, tab_locs[i] - cur_ofs);

#if 0
            const uint32_t MAX_ALT_NAMES_SIZE = 1024;
            if ((i == gn_alternatenames) && (g.m_fields[i].size() > MAX_ALT_NAMES_SIZE))
            {
                int j;
                for (j = MAX_ALT_NAMES_SIZE; j >= 0; j--)
                {
                    if (g.m_fields[i][j] == ',')
                    {
                        g.m_fields[i].resize(j);
                        break;
                    }
                }
                if (j < 0)
                    g.m_fields[i].resize(0);
            }
#endif

            max_col_sizes[i] = std::max(max_col_sizes[i], (uint32_t)g.m_fields[i].size());

            cur_ofs = tab_locs[i] + 1;
        }

        bool accept_flag = false;
        bool has_min_pop = false;
        if (g.m_fields[gn_population].size())
        {
            int pop = atoi(g.m_fields[gn_population].c_str());

            const int MIN_POP = 10;
            if (pop >= MIN_POP)
                has_min_pop = true;
        }

        const auto& code = g.m_fields[gn_feature_code];

        char feature_class = ' ';
        if (g.m_fields[gn_feature_class].size() == 1)
            feature_class = g.m_fields[gn_feature_class][0];

        switch (feature_class)
        {
        case 'T': // mountain,hill,rock,...
            if ((code == "MT") || (code == "MTS") || (code == "ATOL") || (code == "CAPE") || (code == "CNYN") || (code == "DSRT") ||
                (code == "ISL") || (code == "ISLS") || (code == "PEN") || (code == "VALS") || (code == "VALX"))
            {
                accept_flag = true;
            }
            break;
        case 'S': // spot, building, farm
            if ((code == "AIRB") || (code == "AIRF") || (code == "AIRP") || (code == "AIRQ") || (code == "BRKS") || (code == "CTRA") ||
                (code == "CTRS") || (code == "INSM") || (code == "ITTR") || (code == "PSN") || (code == "STNE") || (code == "USGE") ||
                (code == "OBS") || (code == "OBSR") || (code == "MFGM") || (code == "FT") || (code == "ASTR") || (code == "FCL") ||
                (code == "PS") || (code == "PSH") || (code == "STNB") || (code == "STNS") || (code == "UNIV"))
            {
                accept_flag = true;
            }
            break;
        case 'L': // parks,area, ...
            if ((code == "MILB") || (code == "MVA") || (code == "NVB") || (code == "BTL") || (code == "CST") || (code == "RNGA") || (code == "PRK") || (code == "RESV"))
            {
                accept_flag = true;
            }
            break;
        case 'A': // country, state, region,...
            accept_flag = true;
            //accept_flag = has_min_pop;
            break;
        case 'H': // stream, lake, ...
            if ((code == "BAY") || (code == "BAYS") || (code == "CHN") || (code == "CHNL") || (code == "CHNM") || (code == "CHNN") ||
                // #REVIEW "CNL" is repeated twice, was something else meant here?
                (code == "CNL") || (code == "CNL") || (code == "LK") || (code == "LKN") || (code == "LKS") || (code == "RSV") || (code == "SD") || (code == "STRT"))
            {
                accept_flag = true;
            }
            break;
        case 'P': // city, village,...
            if ((code == "PPLC") || (code == "PPLG") || (code == "PPLA")) // TODO
                accept_flag = true;
            else if (is_important_country(g.m_fields[gn_country_code]))
                accept_flag = true;
            else
                accept_flag = has_min_pop;
            break;
        case 'R': // road, railroad
            break;
        case 'U': // undersea
            break;
        case 'V': // forest,heath,...
            break;
        default:
            break;
        }

        if (accept_flag)
        {
            accepted_class_counts[feature_class] = accepted_class_counts[feature_class] + 1;

            json obj = json::object();

            obj["id"] = g.m_fields[gn_geonameid].size() ? atoi(g.m_fields[gn_geonameid].c_str()) : -1;
            obj["name"] = g.m_fields[gn_name];
            obj["plainname"] = g.m_fields[gn_asciiname];

            if (g.m_fields[gn_alternatenames].size())
                obj["altnames"] = g.m_fields[gn_alternatenames];

            if (g.m_fields[gn_feature_class].size())
                obj["fclass"] = g.m_fields[gn_feature_class];

            if (g.m_fields[gn_feature_code].size())
                obj["fcode"] = g.m_fields[gn_feature_code];

            if (g.m_fields[gn_country_code].size())
                obj["ccode"] = g.m_fields[gn_country_code];

            if (g.m_fields[gn_cc2].size())
                obj["cc2"] = g.m_fields[gn_cc2];

            if (g.m_fields[gn_admin1_code].size())
                obj["a1"] = g.m_fields[gn_admin1_code];

            if (g.m_fields[gn_admin2_code].size())
                obj["a2"] = g.m_fields[gn_admin2_code];

            if (g.m_fields[gn_admin3_code].size())
                obj["a3"] = g.m_fields[gn_admin3_code];

            if (g.m_fields[gn_admin4_code].size())
                obj["a4"] = g.m_fields[gn_admin4_code];

            obj["lat"] = g.m_fields[gn_latitude].size() ? atof(g.m_fields[gn_latitude].c_str()) : 0.0f;
            obj["long"] = g.m_fields[gn_longitude].size() ? atof(g.m_fields[gn_longitude].c_str()) : 0.0f;

            if (g.m_fields[gn_population].size())
                obj["pop"] = atoi(g.m_fields[gn_population].c_str());

            output_json.push_back(obj);

            total_accepted++;

            if ((total_accepted % 1000) == 0)
                uprintf("Total accepted: %u\n", total_accepted);
        }
        else
        {
            rejected_class_counts[feature_class] = rejected_class_counts[feature_class] + 1;
        }

        total_geonames++;

        if ((total_geonames % 1000000) == 0)
            uprintf("Geonames processed: %u\n", total_geonames);
    }

    if (!serialize_to_json_file("world_features.json", output_json, true))
        panic("failed writing to file world_features.json");

    uprintf("Total accepted: %u\n", total_accepted);

    for (uint32_t i = 0; i < gn_total; i++)
        uprintf("%u\n", max_col_sizes[i]);

    uprintf("Accepted:\n");
    for (const auto& s : accepted_class_counts)
        uprintf("%c %u\n", s.first, s.second);

    uprintf("Rejected:\n");
    for (const auto& s : rejected_class_counts)
        uprintf("%c %u\n", s.first, s.second);
}

#if 0 // unused code...
static const struct
{
    const char* m_pCode;
    int m_level;
} g_geocode_levels[] =
{
    { "ADM1", 1 },
    { "ADM1H", 1 },

    { "ADM2", 2 },
    { "ADM2H", 2 },

    { "ADM3", 3 },
    { "ADM3H", 3 },

    { "ADM4", 4 },
    { "ADM4H", 4 },

    { "ADM5", 5 },
    { "ADM5H", 5 },

    { "ADMD", 1 },
    { "ADMDH", 1 },

    { "PCL", 0 },
    { "PCLD", 0 },
    { "PCLF", 0 },
    { "PCLH", 0 },
    { "PCLI", 0 },
    { "PCLIX", 0 },
    { "PCLS", 0 },
    { "PRSH", 0 },
    { "TERR", 0 },
    { "ZN", 0 },
    { "ZNB", 0 }
};

const int MIN_GEOCODE_LEVEL = 0, MAX_GEOCODE_LEVEL = 5, NUM_GEOCODE_LEVELS = 6;

static int find_geocode_admin_level(const char* pCode)
{
    for (const auto& l : g_geocode_levels)
        if (_stricmp(pCode, l.m_pCode) == 0)
            return l.m_level;

    return -1;
}
#endif // 0 // unused code...

struct country_info
{
    std::string m_iso2_name;
    std::string m_iso3_name;
    std::string m_iso_numeric;
    std::string m_fips;
    std::string m_country_name;
    std::string m_capital_name;
    int m_area;
    int m_pop;
    std::string m_continent;
    int m_geoid;
    int m_rec_index;
    std::string m_neighbors;
    int m_pop_rank;
};

typedef std::vector<country_info> country_info_vec;

static int get_admin_level(const std::string& code)
{
    if (code == "ADM1")
        return 1;
    else if (code == "ADM2")
        return 2;
    else if (code == "ADM3")
        return 3;
    else if (code == "ADM4")
        return 4;
    else if (code == "ADM5")
        return 5;

    if (code == "ADM1H")
        return 1;
    else if (code == "ADM2H")
        return 2;
    else if (code == "ADM3H")
        return 3;
    else if (code == "ADM4H")
        return 4;
    else if (code == "ADM5H")
        return 5;

    return 0;
}

class geodata
{
    enum { HASH_BITS = 23U, HASH_SHIFT = 32U - HASH_BITS, HASH_FMAGIC = 2654435769U, MAX_EXPECTED_RECS = 3000000 };

public:
    geodata()
    {
        assert((1U << HASH_BITS) >= MAX_EXPECTED_RECS * 2U);
    }

    void init()
    {
        uprintf("Reading hierarchy\n");

        load_hierarchy();

        uprintf("Reading world_features.json\n");

        if (!read_text_file("world_features.json", m_filebuf, nullptr))
            panic("Failed reading file");

        uprintf("Deserializing JSON file\n");

        bool status = m_doc.deserialize_in_place((char*)&m_filebuf[0]);
        if (!status)
            panic("Failed parsing JSON document!");

        if (!m_doc.is_array())
            panic("Invalid JSON");

        uprintf("Hashing JSON data\n");

        m_name_hashtab.resize(0);
        m_name_hashtab.resize(1U << HASH_BITS);

        const auto& root_arr = m_doc.get_array();

        //crnlib::timer tm;
        //tm.start();

        uint8_vec name_buf;

        m_geoid_to_rec.clear();
        m_geoid_to_rec.reserve(MAX_EXPECTED_RECS);

        for (uint32_t rec_index = 0; rec_index < root_arr.size(); rec_index++)
        {
            const auto& arr_entry = root_arr[rec_index];
            if (!arr_entry.is_object())
                panic("Invalid JSON");

            int geoid = arr_entry.find_int32("id");
            assert(geoid > 0);
            auto ins_res = m_geoid_to_rec.insert(std::make_pair(geoid, (int)rec_index));
            if (!ins_res.second)
                panic("Invalid id!");

            const auto pName = arr_entry.find_value_variant("name");
            const auto pAltName = arr_entry.find_value_variant("altnames");

            if ((pName == nullptr) || (!pName->is_string()))
                panic("Missing/invalid name field");

            {
                const char* pName_str = pName->get_string_ptr();
                size_t name_size = strlen(pName_str);

                if (name_size > name_buf.size())
                    name_buf.resize(name_size);

                for (uint32_t ofs = 0; ofs < name_size; ofs++)
                    name_buf[ofs] = utolower(pName_str[ofs]);

                uint32_t hash_val = (hash_hsieh(&name_buf[0], name_size) * HASH_FMAGIC) >> HASH_SHIFT;
                m_name_hashtab[hash_val].push_back(rec_index);
            }

            const auto pPlainName = arr_entry.find_value_variant("plainname");
            if ((pPlainName == nullptr) || (!pPlainName->is_string()))
                panic("Missing/invalid plainname field");

            {
                const char* pName_str = pPlainName->get_string_ptr();
                size_t name_size = strlen(pName_str);

                if (name_size > name_buf.size())
                    name_buf.resize(name_size);

                for (uint32_t ofs = 0; ofs < name_size; ofs++)
                    name_buf[ofs] = utolower(pName_str[ofs]);

                uint32_t hash_val = (hash_hsieh(&name_buf[0], name_size) * HASH_FMAGIC) >> HASH_SHIFT;
                m_name_hashtab[hash_val].push_back(rec_index);
            }

            if (pAltName)
            {
                if (!pAltName->is_string())
                    panic("Missing/invalid altname field");

                const char* pAlt_name_str = pAltName->get_string_ptr();
                size_t alt_name_size = strlen(pAlt_name_str);

                if (alt_name_size > name_buf.size())
                    name_buf.resize(alt_name_size);

                for (uint32_t ofs = 0; ofs < alt_name_size; ofs++)
                    name_buf[ofs] = utolower(pAlt_name_str[ofs]);

                uint32_t start_ofs = 0;

                for (uint32_t i = 0; i < alt_name_size; i++)
                {
                    if (pAlt_name_str[i] == ',')
                    {
                        size_t size = i - start_ofs;
                        assert(size);

                        uint32_t hash_val = (hash_hsieh(&name_buf[0] + start_ofs, size) * HASH_FMAGIC) >> HASH_SHIFT;
                        m_name_hashtab[hash_val].push_back(rec_index);

                        start_ofs = i + 1;
                    }
                }

                size_t size = alt_name_size - start_ofs;
                assert(size);

                uint32_t hash_val = (hash_hsieh(&name_buf[0] + start_ofs, size) * HASH_FMAGIC) >> HASH_SHIFT;
                m_name_hashtab[hash_val].push_back(rec_index);
            }

            std::string fclass = arr_entry.find_string_obj("fclass");

            if (fclass == "A")
            {
                std::string fcode(arr_entry.find_string_obj("fcode"));

                if ((fcode == "ADM1") || (fcode == "ADM2") || (fcode == "ADM3") || (fcode == "ADM4"))
                {
                    std::string ccode(arr_entry.find_string_obj("ccode"));

                    std::string a[4] = {
                        arr_entry.find_string_obj("a1"),
                        arr_entry.find_string_obj("a2"),
                        arr_entry.find_string_obj("a3"),
                        arr_entry.find_string_obj("a4") };

                    std::string desc(ccode);

                    for (uint32_t i = 0; i < 4; i++)
                    {
                        if (!a[i].size())
                            break;
                        desc += "." + a[i];
                    }

                    m_admin_map[desc].push_back(std::pair<int, int>(rec_index, get_admin_level(fcode)));
                }
            }
        }

#if 0
        if (m_admin_map.count(desc))
        {
            const int alt_admin_level = get_admin_level(fcode);

            const int cur_rec_index = m_admin_map.find(desc)->second;
            const pjson::value_variant* pCur = &m_doc[cur_rec_index];
            const int cur_admin_level = get_admin_level(pCur->find_string_obj("fcode"));

            if (alt_admin_level < cur_admin_level)
                m_admin_map[desc] = rec_index;
            else if (alt_admin_level == cur_admin_level)
            {
                uprintf("ccode: %s rec: %u geoid: %u name: %s fcode: %s, current rec: %u geoid: %u name: %s fcode: %s\n",
                    ccode.c_str(),
                    rec_index, geoid, pName->get_string_ptr(), fcode.c_str(),
                    cur_rec_index, pCur->find_int32("id"), pCur->find_string_obj("name").c_str(), pCur->find_string_obj("fcode").c_str());
            }
        }
        else
#endif

        for (auto it = m_admin_map.begin(); it != m_admin_map.end(); it++)
        {
            std::vector< std::pair<int, int> >& recs = it->second;

            std::sort(recs.begin(), recs.end(),
                [](const std::pair<int, int>& a, const std::pair<int, int>& b) -> bool
                {
                    return a.second < b.second;
                });

#if 0
            uprintf("%s:\n", it->first.c_str());
            for (uint32_t i = 0; i < recs.size(); i++)
            {
                const int cur_rec_index = recs[i].first;
                const pjson::value_variant* pCur = &m_doc[cur_rec_index];

                uprintf("admlevel: %u, rec: %u geoid: %u name: %s fcode: %s\n",
                    recs[i].second,
                    cur_rec_index, pCur->find_int32("id"), pCur->find_string_obj("name").c_str(), pCur->find_string_obj("fcode").c_str());
            }
#endif
        }

        for (uint32_t i = 0; i < m_name_hashtab.size(); i++)
        {
            if (!m_name_hashtab[i].size())
                continue;

            std::sort(m_name_hashtab[i].begin(), m_name_hashtab[i].end());
            auto new_end = std::unique(m_name_hashtab[i].begin(), m_name_hashtab[i].end());
            m_name_hashtab[i].resize(new_end - m_name_hashtab[i].begin());
        }

        load_country_info();

        //uprintf("geodata::init: Elapsed time: %f\n", tm.get_elapsed_secs());
    }

    void find(const char* pKey, std::vector<const pjson::value_variant*>& results, std::vector<const pjson::value_variant*>& alt_results) const
    {
        assert(m_name_hashtab.size());

        std::string key(pKey);
        for (char& c : key)
            c = utolower(c);

        const uint32_t hash_val = (hash_hsieh((const uint8_t *)key.c_str(), key.size()) * HASH_FMAGIC) >> HASH_SHIFT;

        results.resize(0);
        alt_results.resize(0);

        const uint_vec& bucket = m_name_hashtab[hash_val];

        for (uint32_t i = 0; i < bucket.size(); i++)
        {
            const uint32_t rec_index = bucket[i];
            const pjson::value_variant* pObj = &m_doc[rec_index];

            const char *pName = pObj->find_string_ptr("name");

            const char* pPlainName = pObj->find_string_ptr("plainname");

            if ((_stricmp(pKey, pName) != 0) && (_stricmp(pKey, pPlainName) != 0))
            {
                const std::string alt_names(pObj->find_string_obj("altnames"));

                string_vec alt_tokens;
                string_tokenize(alt_names, ",", "", alt_tokens);

                uint32_t j;
                for (j = 0; j < alt_tokens.size(); j++)
                    if (string_icompare(alt_tokens[j], pKey) == 0)
                        break;

                if (j < alt_tokens.size())
                    alt_results.push_back(pObj);
            }
            else
            {
                results.push_back(pObj);
            }
        }
    }

    const pjson::document& get_doc() const { return m_doc; }

    struct geo_result
    {
        const pjson::value_variant* m_pVariant;
        bool m_alt;
    };
    typedef std::vector<geo_result> geo_result_vec;

    enum { MAX_ADMINS = 4 };

    uint32_t count_admins(const pjson::value_variant* p) const
    {
        uint32_t num_admins;
        for (num_admins = 0; num_admins < MAX_ADMINS; num_admins++)
        {
            std::string str(p->find_string_obj(string_format("a%u", num_admins + 1).c_str()));

            if (!str.size() || (str == "00"))
                break;
        }

        return num_admins;
    }

    bool is_valid_parent(const pjson::value_variant* pParent, const pjson::value_variant* pChild) const
    {
        if (pParent == pChild)
            return false;

        if (pParent->find_string_obj("ccode") != pChild->find_string_obj("ccode"))
            return false;

        const int parent_id = pParent->find_int32("id");
        const auto find_res = m_geo_hierarchy.find(parent_id);
        if (find_res != m_geo_hierarchy.end())
        {
            const int child_id = pChild->find_int32("id");

            const int_vec& child_indices = find_res->second;

            for (uint32_t i = 0; i < child_indices.size(); i++)
                if (child_indices[i] == child_id)
                    return true;
        }

        const uint32_t num_parent_admins = count_admins(pParent);
        const uint32_t num_child_admins = count_admins(pChild);

        if (num_parent_admins > num_child_admins)
            return false;

        // Example: Anderson, Shasta County, California
        if (num_parent_admins == num_child_admins)
        {
            // TODO: check fcode, child should be higher ADM level vs. parent (i.e. ADM4 vs. AMD1) for 2035607 vs. 8648889
            if (pParent->find_string_obj("fclass") != "A")
                return false;
        }

        for (uint32_t admin_index = 0; admin_index < num_parent_admins; admin_index++)
        {
            std::string id(string_format("a%u", admin_index + 1));

            std::string admin_parent(pParent->find_string_obj(id.c_str()));
            std::string admin_child(pChild->find_string_obj(id.c_str()));

            if (admin_parent != admin_child)
                return false;
        }

        return true;
    }

    bool is_valid_candidate(const std::vector<geo_result_vec> &temp_results, uint32_t tok_index, uint32_t result_index) const
    {
        if (!tok_index)
            return true;

        const uint32_t parent_tok_index = tok_index - 1;

        for (uint32_t i = 0; i < temp_results[parent_tok_index].size(); i++)
        {
            if (!is_valid_parent(temp_results[parent_tok_index][i].m_pVariant, temp_results[tok_index][result_index].m_pVariant))
                continue;

            if (!is_valid_candidate(temp_results, parent_tok_index, i))
                continue;

            return true;
        }

        return false;
    }

    enum
    {
        cRankNamedLocAlt = 0,   // alt

        cRankNamedLoc,          // prim

        cRankNaturalFeature0,   // prim or alt
        cRankNaturalFeature1,   // prim or alt

        cRankVillageNoPopAlt,        // alt

        cRankAdminNoPop,        // not a numbered admin

        cRankPopVillageAlt,     // prim, 1-100
        cRankTownAlt,           // alt, 100+

        cRankCityLevel0Alt,      // alt or alt, 1k+
        cRankCityLevel1Alt,      // alt or alt, 10k+

        cRankAdminCapital4Alt,  // alt cap4
        cRankAdmin4Alt,         // alt admin4

        cRankAdminCapital3Alt,  // alt cap3
        cRankAdmin3Alt,         // alt amind3

        cRankCityLevel2Alt,      // alt or alt, 100k+
        cRankCityLevel3Alt,      // alt or alt, 1mk+

        cRankVillageNoPop,      // prim no pop

        cRankAdmin,             // not numbered, has pop

        cRankPopVillage,        // prim, 1-100
        cRankTown,              // prim, 100+

        cRankAdminCapital2Alt, // alt county seat
        cRankAdmin2Alt,        // alt county

        cRankAdminCapital4,     // prim cap4
        cRankAdmin4,            // prim admin4

        cRankPark,              // prim or alt
        cRankReserve,           // prim or alt

        cRankAdminCapital1Alt, // alt state cap

        cRankCityLevel0,        // prim or alt, 1k+
        cRankCityLevel1,        // prim or alt, 10k+

        cRankAdminCapital3,     // prim cap3
        cRankAdmin3,            // prim admin3

        cRankCityLevel2,        // prim or alt, 100k+
        cRankCityLevel3,        // prim or alt, 1m+

        cRankBaseOrAirport,    // prim or alt

        cRankAdminCapital2, // prim county seat
        cRankAdmin2,        // prim county

        cRankAdminCapital1, // prim state cap

        cRankAdmin1Alt,      // alt state

        cRankPoliticalCapital,  // prim or alt
        cRankGovernmentCapital, // prim or alt

        cRankAdmin1,        // prim state

        // all countries prim or alt
        cRankCountryLevel0,
        cRankCountryLevel1,
        cRankCountryLevel2,
        cRankCountryLevel3,
        cRankCountryLevel4,
        cRankCountryLevel5,
        cRankCountryLevel6,
        cRankCountryLevel7,
        cRankCountryLevel8,
        cRankCountryLevel9,

        cRankTotal,
    };

    int get_rank(const pjson::value_variant* p, bool alt_match) const
    {
        int country_index = get_country_index(p);
        if (country_index >= 0)
            return cRankCountryLevel0 + m_countries[country_index].m_pop_rank;

        //uint32_t admin_count = count_admins(p);

        std::string fclass(p->find_string_obj("fclass"));
        std::string fcode(p->find_string_obj("fcode"));

        if (fclass == "H")
        {
            //if ((code == "BAY") || (code == "BAYS") || (code == "CHN") || (code == "CHNL") || (code == "CHNM") || (code == "CHNN") ||
            //    (code == "CNL") || (code == "CNL") || (code == "LK") || (code == "LKN") || (code == "LKS") || (code == "RSV") || (code == "SD") || (code == "STRT"))

            if ((fcode == "LK") || (fcode == "LKS") || (fcode == "HBR") || (fcode == "BAY") || (fcode == "BAYS"))
                return cRankNaturalFeature1;
            else
                return cRankNaturalFeature0;
        }
        else if (fclass == "S")
        {
            //if ((code == "AIRB") || (code == "AIRF") || (code == "AIRP") || (code == "AIRQ") || (code == "BRKS") || (code == "CTRA") ||
            //    (code == "CTRS") || (code == "INSM") || (code == "ITTR") || (code == "PSN") || (code == "STNE") || (code == "USGE"))

            return cRankBaseOrAirport;
        }
        else if (fclass == "L")
        {
            //if ((code == "MILB") || (code == "MVA") || (code == "NVB") || (code == "BTL") || (code == "CST") || (code == "RNGA") || (code == "PRK") || (code == "RESV"))

            if ((fcode == "MILB") || (fcode == "NVB") || (fcode == "RNGA"))
                return cRankBaseOrAirport;

            if (fcode == "RESV")
                return cRankReserve;

            return cRankPark;
        }
        else if (fclass == "A")
        {
            // countries are all fclass A, a1=00
            // fcode must be PCL, PCLD, PCLF, PCLH, PCLI, PCLIX, PCLS, TERR

            if ((fcode == "PCL") || (fcode == "PCLD") || (fcode == "PCLF") || (fcode == "PCLH") || (fcode == "PCLI") || (fcode == "PCLIX") || (fcode == "PCLS") || (fcode == "TERR"))
                return cRankCountryLevel0;

            if ((fcode == "ADM1") || (fcode == "ADM1H"))
                return alt_match ? cRankAdmin1Alt : cRankAdmin1;

            if ((fcode == "ADM2") || (fcode == "ADM2H"))
                return alt_match ? cRankAdmin2Alt : cRankAdmin2;

            if ((fcode == "ADM3") || (fcode == "ADM3H"))
                return alt_match ? cRankAdmin3Alt : cRankAdmin3;

            if ((fcode == "ADM4") || (fcode == "ADM4H"))
                return alt_match ? cRankAdmin4Alt : cRankAdmin4;

            int pop = p->find_int32("pop");

            if (!pop)
                return cRankAdminNoPop;
            else
                return cRankAdmin;
        }
        else if (fclass == "P")
        {
            if (fcode == "PPLA")
                return alt_match ? cRankAdminCapital1Alt : cRankAdminCapital1;
            else if (fcode == "PPLA2")
                return alt_match ? cRankAdminCapital2Alt : cRankAdminCapital2;
            else if (fcode == "PPLA3")
                return alt_match ? cRankAdminCapital3Alt : cRankAdminCapital3;
            else if ((fcode == "PPLA4") || (fcode == "PPLA5"))
                return alt_match ? cRankAdminCapital4Alt : cRankAdminCapital4;

            if ((fcode == "PPLC") || (fcode == "PPLCH"))
                return cRankPoliticalCapital;

            if (fcode == "PPLG")
                return cRankGovernmentCapital;

            int pop = p->find_int32("pop");

            if (pop > 1000000)
                return alt_match ? cRankCityLevel3Alt : cRankCityLevel3;
            else if (pop >= 100000)
                return alt_match ? cRankCityLevel2Alt : cRankCityLevel2;
            else if (pop >= 10000)
                return alt_match ? cRankCityLevel1Alt : cRankCityLevel1;
            else if (pop >= 1000)
                return alt_match ? cRankCityLevel0Alt : cRankCityLevel0;
            else if (pop >= 100)
                return alt_match ? cRankTownAlt : cRankTown;
            else if (pop)
                return alt_match ? cRankPopVillageAlt : cRankPopVillage;
            else
                return alt_match ? cRankVillageNoPopAlt : cRankVillageNoPop;
        }
        else if (fclass == "T")
        {
            //if ((code == "MT") || (code == "MTS") || (code == "ATOL") || (code == "CAPE") || (code == "CNYN") || (code == "DSRT") ||
            //    (code == "ISL") || (code == "ISLS") || (code == "PEN") || (code == "VALS") || (code == "VALX"))

            if ((fcode == "ISL") || (fcode == "ISLS") || (fcode == "MT") || (fcode == "MTS"))
                return cRankNaturalFeature1;
            else
                return cRankNaturalFeature0;
        }
        else if (fclass == "V")
        {
            return cRankNaturalFeature0;
        }

        return cRankNamedLoc;
    }

    bool is_country(const pjson::value_variant* p) const
    {
        int rank = get_rank(p, false);
        return (rank >= cRankCountryLevel0);
    }

    struct resolve_results
    {
        resolve_results()
        {
            clear();
        }

        void clear()
        {
            m_candidates.resize(0);
            m_sorted_results.resize(0);
            m_best_result.m_pVariant = nullptr;
            m_best_sorted_result_index = 0;
            m_best_score = 0.0f;
            m_num_input_tokens = 0;
            m_strong_match = false;
        }

        geo_result m_best_result;

        uint32_t m_num_input_tokens;
        bool m_strong_match;

        geo_result_vec m_candidates;
        std::vector< std::pair<uint32_t, float> > m_sorted_results;
        uint32_t m_best_sorted_result_index;
        float m_best_score;
    };

    bool resolve(const std::string& str, resolve_results &resolve_res) const
    {
        uprintf("--- Candidates for query: %s\n", str.c_str());

        resolve_res.clear();

        string_vec toks;
        string_tokenize(str, ",", "", toks);

        for (auto& tok_str : toks)
        {
            string_vec temp_toks;
            string_tokenize(tok_str, " \t", "", temp_toks);

            tok_str.resize(0);
            for (const auto& s : temp_toks)
            {
                if (tok_str.size())
                    tok_str += " ";
                tok_str += s;
            }
        }

        resolve_res.m_num_input_tokens = (uint32_t)toks.size();

        // First token will be the most significant (country etc.), last token will be the least significant.
        std::reverse(toks.begin(), toks.end());

        std::vector<geo_result_vec> temp_results(toks.size());

        size_t total_results = 0;

        for (uint32_t toks_index = 0; toks_index < toks.size(); toks_index++)
        {
            const std::string& tok_str = toks[toks_index];

            std::vector<const pjson::value_variant*> results;
            std::vector<const pjson::value_variant*> alt_results;

            find(tok_str.c_str(), results, alt_results);

            if ((!toks_index) && (tok_str.size() == 2))
            {
                auto find_it = m_country_to_record_index.find(tok_str);
                if (find_it != m_country_to_record_index.end())
                {
                    const uint32_t country_rec_index = find_it->second;

                    alt_results.push_back(&m_doc[country_rec_index]);
                }
            }

            total_results += results.size() + alt_results.size();

            for (auto p : results)
            {
#if 0
                uprintf("-- full match: tok: %u, id: %u, name: \"%s\", altnames: \"%s\", lat: %f, long: %f, ccode=%s, a1=%s, a2=%s, a3=%s, a4=%s, fclass: %s, fcode: %s, pop: %i\n",
                    toks_index,
                    p->find_int32("id"),
                    p->find_string_ptr("name"), p->find_string_ptr("altnames"), p->find_float("lat"), p->find_float("long"),
                    p->find_string_ptr("ccode"),
                    p->find_string_ptr("a1"), p->find_string_ptr("a2"), p->find_string_ptr("a3"), p->find_string_ptr("a4"),
                    p->find_string_ptr("fclass"),
                    p->find_string_ptr("fcode"),
                    p->find_int32("pop"));
#endif

                temp_results[toks_index].push_back({ p, false });
            }

            for (auto p : alt_results)
            {
#if 0
                uprintf("-- alt match: tok: %u, id: %u, name: \"%s\", altnames: \"%s\", lat: %f, long: %f, ccode=%s, a1=%s, a2=%s, a3=%s, a4=%s, fclass: %s, fcode: %s, pop: %i\n",
                    toks_index,
                    p->find_int32("id"),
                    p->find_string_ptr("name"), p->find_string_ptr("altnames"), p->find_float("lat"), p->find_float("long"),
                    p->find_string_ptr("ccode"),
                    p->find_string_ptr("a1"), p->find_string_ptr("a2"), p->find_string_ptr("a3"), p->find_string_ptr("a4"),
                    p->find_string_ptr("fclass"),
                    p->find_string_ptr("fcode"),
                    p->find_int32("pop"));
#endif

                temp_results[toks_index].push_back({ p, true });
            }
        }

        // 0=last token (Country)
        // size-1=first token (location/city/town)

        if (!total_results)
        {
            uprintf("No results\n");
            return false;
        }

        //uprintf("Candidates for query: %s\n", str.c_str());

        std::vector<uint32_t> valid_candidates;

        const uint32_t last_tok_index = (uint32_t)toks.size() - 1;
        //const bool single_token = (toks.size() == 1);

        for (uint32_t i = 0; i < temp_results[last_tok_index].size(); i++)
        {
            if (is_valid_candidate(temp_results, last_tok_index, i))
                valid_candidates.push_back(i);
        }

        std::vector< std::pair<uint32_t, float> > candidate_results[TOTAL_FAVORED_COUNTRY_RANKS];
        uint32_t total_country_rankings = 0;
        [[maybe_unused]] uint32_t total_candidates = 0;

        for (uint32_t candidate_index_iter = 0; candidate_index_iter < valid_candidates.size(); candidate_index_iter++)
        {
            const uint32_t candidate_index = valid_candidates[candidate_index_iter];

            const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

            std::string name(p->find_string_obj("name"));
            std::string plainname(p->find_string_obj("plainname"));
            std::string ccode(p->find_string_obj("ccode"));

            float candidate_score = (float)get_rank(p, temp_results[last_tok_index][candidate_index].m_alt);

            if ((name == toks[last_tok_index]) || (plainname == toks[last_tok_index]))
                candidate_score += (float).5f;
            else if ((string_icompare(name, toks[last_tok_index].c_str()) == 0) || (string_icompare(plainname, toks[last_tok_index].c_str()) == 0))
                candidate_score += (float).2f;
            else
            {
                if (toks[last_tok_index].size() >= 5)
                {
                    if ((string_find_first(name, toks[last_tok_index].c_str()) != -1) ||
                        (string_find_first(plainname, toks[last_tok_index].c_str()) != -1))
                    {
                        candidate_score += (float).25f;
                    }
                }

                const std::string alt_names(p->find_string_obj("altnames"));

                string_vec alt_tokens;
                string_tokenize(alt_names, ",", "", alt_tokens);

                uint32_t j;
                for (j = 0; j < alt_tokens.size(); j++)
                {
                    if (alt_tokens[j] == toks[last_tok_index])
                    {
                        candidate_score += .1f;
                        break;
                    }
                }
            }

            candidate_score += p->find_float("pop") / 40000000.0f;

            const int country_rank = get_favored_country_rank(ccode);
            assert(static_cast<uint32_t>(country_rank) < TOTAL_FAVORED_COUNTRY_RANKS);

            if (!candidate_results[country_rank].size())
                total_country_rankings++;

            candidate_results[country_rank].push_back(std::make_pair(candidate_index, candidate_score));

            total_candidates++;
        }

        // 1. If there's just one country rank group, choose the best score in that country rank group.
        // 2. If they matched against a country, choose the highest ranking country, prioritizing the favored countries first.
        // 3. Check for states, state capitals or other significant admin districts in the favored countries, in order
        // If they matched against a country's capital, choose the highest ranking capital.
        // 5. Check for significant enough cities in the favored countries, in order
        // 6. Choose the one with the highest score

        const geo_result* pBest_result = nullptr;
        float best_score = -1.0f;
        const std::vector< std::pair<uint32_t, float> >* pBest_ranking_vec = nullptr;
        uint32_t best_ranking_index = 0;

        // Sort each ranked country group by score, highest to lowest in each.
        for (auto& r : candidate_results)
        {
            if (r.size())
            {
                std::sort(r.begin(), r.end(),
                    [](const std::pair<uint32_t, float>& a, const std::pair<uint32_t, float>& b) -> bool
                    {
                        return a.second > b.second;
                    });
            }
        }

#if 0
        for (uint32_t rank_iter = 0; rank_iter < TOTAL_FAVORED_COUNTRY_RANKS; rank_iter++)
        {
            for (auto & c : candidate_results[rank_iter])
            {
                const uint32_t candidate_index = c.first;
                const float score = c.second;

                const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

                uprintf("-- score: %f, country rank: %u, alt: %u, id: %u, name: \"%s\", altnames: \"%s\", lat: %f, long: %f, ccode=%s, a1=%s, a2=%s, a3=%s, a4=%s, fclass: %s, fcode: %s, pop: %i\n",
                    score,
                    rank_iter,
                    temp_results[last_tok_index][candidate_index].m_alt,
                    p->find_int32("id"),
                    p->find_string_ptr("name"), p->find_string_ptr("altnames"), p->find_float("lat"), p->find_float("long"),
                    p->find_string_ptr("ccode"),
                    p->find_string_ptr("a1"), p->find_string_ptr("a2"), p->find_string_ptr("a3"), p->find_string_ptr("a4"),
                    p->find_string_ptr("fclass"),
                    p->find_string_ptr("fcode"),
                    p->find_int32("pop"));
            }
        }
#endif

        if (total_country_rankings == 1)
        {
            // Only one ranked country group in the candidate results, so just choose the one with the highest score.

            for (const auto& r : candidate_results)
            {
                if (r.size())
                {
                    pBest_ranking_vec = &r;
                    break;
                }
            }

            assert(pBest_ranking_vec);

            uint32_t candidate_index = (*pBest_ranking_vec)[0].first;

            best_score = (*pBest_ranking_vec)[0].second;
            pBest_result = &temp_results[last_tok_index][candidate_index];
            best_ranking_index = 0;

            resolve_res.m_strong_match = (temp_results[last_tok_index].size() == 1);
        }
        else
        {
            // Multiple ranked country groups.

            // Check for US states (primary or alt)
            {
                uint32_t r_index = 0;
                const std::vector< std::pair<uint32_t, float> >& r = candidate_results[r_index];

                for (uint32_t i = 0; i < r.size(); i++)
                {
                    const uint32_t candidate_index = r[i].first;
                    const float score = r[i].second;

                    const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

                    const int rank = get_rank(p, temp_results[last_tok_index][candidate_index].m_alt);

                    if ((rank == cRankAdmin1) || (rank == cRankAdmin1Alt))
                    {
                        pBest_result = &temp_results[last_tok_index][candidate_index];
                        best_score = score;
                        pBest_ranking_vec = &r;
                        best_ranking_index = i;
                        break;
                    }
                }
            }

            if (!pBest_result)
            {
                // First check for any country hits from any ranked country group.
                for (const auto& r : candidate_results)
                {
                    for (uint32_t i = 0; i < r.size(); i++)
                    {
                        const uint32_t candidate_index = r[i].first;
                        const float score = r[i].second;

                        const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

                        const int rank = get_rank(p, temp_results[last_tok_index][candidate_index].m_alt);

                        if (rank >= cRankCountryLevel0)
                        {
                            pBest_result = &temp_results[last_tok_index][candidate_index];
                            best_score = score;
                            pBest_ranking_vec = &r;
                            best_ranking_index = i;
                            break;
                        }
                    }

                    if (pBest_result)
                        break;
                }
            }

            if (!pBest_result)
            {
                // Check for country capitals or states in the favored countries
                for (uint32_t r_index = 0; r_index < (TOTAL_FAVORED_COUNTRY_RANKS - 1); r_index++)
                {
                    const std::vector< std::pair<uint32_t, float> >& r = candidate_results[r_index];

                    for (uint32_t i = 0; i < r.size(); i++)
                    {
                        const uint32_t candidate_index = r[i].first;
                        const float score = r[i].second;

                        const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;
                        //const bool was_alt = temp_results[last_tok_index][candidate_index].m_alt;

                        const int rank = get_rank(p, temp_results[last_tok_index][candidate_index].m_alt);

                        if ((rank == cRankAdmin1Alt) || (rank == cRankAdmin1) || (rank == cRankPoliticalCapital) || (rank == cRankGovernmentCapital))
                        {
                            pBest_result = &temp_results[last_tok_index][candidate_index];
                            best_score = score;
                            pBest_ranking_vec = &r;
                            best_ranking_index = i;
                            break;
                        }
                    }

                    if (pBest_result)
                        break;
                }
            }

            if (!pBest_result)
            {
                // Check for large cities with pops > 100K, from any country, this will catch "Moscow"
                int best_pop = 99999;

                for (uint32_t r_index = 0; r_index < TOTAL_FAVORED_COUNTRY_RANKS; r_index++)
                {
                    const std::vector< std::pair<uint32_t, float> >& r = candidate_results[r_index];

                    for (uint32_t i = 0; i < r.size(); i++)
                    {
                        const uint32_t candidate_index = r[i].first;
                        const float score = r[i].second;

                        const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

                        int pop = p->find_int32("pop");

                        if ((!temp_results[last_tok_index][candidate_index].m_alt) && (pop > best_pop))
                        {
                            best_pop = pop;

                            pBest_result = &temp_results[last_tok_index][candidate_index];
                            best_score = score;
                            pBest_ranking_vec = &r;
                            best_ranking_index = i;
                        }
                    }
                }
            }

            if (!pBest_result)
            {
                // Check for large enough cities in our favored countries
                for (uint32_t r_index = 0; r_index < (TOTAL_FAVORED_COUNTRY_RANKS - 1); r_index++)
                {
                    const std::vector< std::pair<uint32_t, float> >& r = candidate_results[r_index];

                    for (uint32_t i = 0; i < r.size(); i++)
                    {
                        const uint32_t candidate_index = r[i].first;
                        const float score = r[i].second;

                        const pjson::value_variant* p = temp_results[last_tok_index][candidate_index].m_pVariant;

                        const int rank = get_rank(p, temp_results[last_tok_index][candidate_index].m_alt);

                        if (rank >= cRankCityLevel2)
                        {
                            if (score > best_score)
                            {
                                pBest_result = &temp_results[last_tok_index][candidate_index];
                                best_score = score;
                                pBest_ranking_vec = &r;
                                best_ranking_index = i;
                            }
                        }
                    }
                }
            }

            if (!pBest_result)
            {
                // Fall back to choosing the highest score
                for (uint32_t r_index = 0; r_index < TOTAL_FAVORED_COUNTRY_RANKS; r_index++)
                {
                    const std::vector< std::pair<uint32_t, float> >& r = candidate_results[r_index];

                    for (uint32_t i = 0; i < r.size(); i++)
                    {
                        const uint32_t candidate_index = r[i].first;
                        const float score = r[i].second;

                        if (score > best_score)
                        {
                            best_score = score;

                            pBest_result = &temp_results[last_tok_index][candidate_index];

                            pBest_ranking_vec = &r;
                            best_ranking_index = i;
                        }
                    }
                }
            }
        }

#if 0
        uprintf("\n");
        uprintf("---- Query: %s\n", str.c_str());
#endif

        if (!pBest_result)
        {
            uprintf("No results\n");
            return false;
        }

        resolve_res.m_best_result = *pBest_result;
        resolve_res.m_candidates = temp_results[last_tok_index];
        resolve_res.m_sorted_results = *pBest_ranking_vec;
        resolve_res.m_best_sorted_result_index = best_ranking_index;
        resolve_res.m_best_score = best_score;

        [[maybe_unused]] const pjson::value_variant* pVariant = pBest_result->m_pVariant;

#if 0
        uprintf("Result: score:%f, alt: %u, id: %u, name: \"%s\", lat: %f, long: %f, ccode=%s, a1=%s, a2=%s, a3=%s, a4=%s, fclass: %s, fcode: %s, pop: %i\n",
            best_score,
            pBest_result->m_alt,
            pVariant->find_int32("id"),
            pVariant->find_string_ptr("name"), pVariant->find_float("lat"), pVariant->find_float("long"),
            pVariant->find_string_ptr("ccode"),
            pVariant->find_string_ptr("a1"), pVariant->find_string_ptr("a2"), pVariant->find_string_ptr("a3"), pVariant->find_string_ptr("a4"),
            pVariant->find_string_ptr("fclass"),
            pVariant->find_string_ptr("fcode"),
            pVariant->find_int32("pop"));

        std::string rdesc(get_desc(pBest_result->m_pVariant));
        uprintf("Desc: %s\n", rdesc.c_str());
#endif

        return true;
    }

    std::string get_desc(const pjson::value_variant* p) const
    {
        std::string res;

        std::string ccode(p->find_string_obj("ccode"));
        std::string fclass(p->find_string_obj("fclass"));
        std::string fcode(p->find_string_obj("fcode"));

        std::string a[4] = { p->find_string_obj("a1"), p->find_string_obj("a2"), p->find_string_obj("a3"), p->find_string_obj("a4") };

        uint32_t num_admins = count_admins(p);

        res += p->find_string_obj("name");

        const int start_admin = num_admins - 1;

        for (int i = start_admin; i >= 0; i--)
        {
            std::string x(ccode);
            for (int j = 0; j <= i; j++)
                x += "." + a[j];

            auto find_res(m_admin_map.find(x));
            if (find_res != m_admin_map.end())
            {
                const std::vector< std::pair<int, int> >& recs = find_res->second;

                assert(recs.size());

                int cur_level = recs[0].second;
                for (uint32_t j = 0; j < recs.size(); j++)
                {
                    if (recs[j].second != cur_level)
                        break;

                    int rec_index = recs[j].first;

                    const pjson::value_variant* q = &m_doc[rec_index];

                    if (i == (int)(num_admins - 1))
                    {
                        if (!is_valid_parent(q, p))
                        {
                            goto skip;
                        }
                    }

                    if (!string_ends_in(res, q->find_string_obj("name").c_str()))
                    {
                        if (j)
                            res += "/";
                        else
                            res += ", ";

                        res += q->find_string_obj("name");
                    }
                }
            }

        skip:;
        }

        auto find_res(m_country_to_record_index.find(ccode));
        if (find_res != m_country_to_record_index.end())
        {
            int country_rec_index = find_res->second;

            if (res != m_doc[country_rec_index].find_string_obj("name"))
                res += ", " + m_doc[country_rec_index].find_string_obj("name");
        }

        return res;
    }

private:
    pjson::document m_doc;
    uint8_vec m_filebuf;
    std::vector<uint_vec> m_name_hashtab;

    std::unordered_map<int, int> m_geoid_to_rec;

    country_info_vec m_countries;
    std::unordered_map<int, int> m_rec_index_to_country_index;
    std::unordered_map<int, int> m_geoid_to_country_index;

    // maps iso2 name to m_countries[] record index
    std::unordered_map<std::string, uint32_t> m_country_to_record_index;

    // maps parent to child or children geoid's, used to handle places that are in multiple counties (for example)
    std::unordered_map<int, int_vec> m_geo_hierarchy;

    // maps CC+ADM1+ADM2+etc. to (rec index, admin_level)
    std::unordered_map<std::string, std::vector< std::pair<int, int> > > m_admin_map;

    int get_country_index(const pjson::value_variant* p) const
    {
        int geoid = p->find_int32("id");
        auto find_res = m_geoid_to_country_index.find(geoid);

        if (find_res == m_geoid_to_country_index.end())
            return -1;

        return find_res->second;
    }

    static void extract_tab_fields(const std::string& str, string_vec& fields)
    {
        std::vector<int> tab_locs;
        tab_locs.resize(0);

        for (uint32_t ofs = 0; ofs < str.size(); ofs++)
            if (str[ofs] == '\t')
                tab_locs.push_back(ofs);

        tab_locs.push_back((uint32_t)str.size());

        fields.resize(tab_locs.size());

        uint32_t cur_ofs = 0;
        for (uint32_t i = 0; i < fields.size(); i++)
        {
            fields[i] = string_slice(str, cur_ofs, tab_locs[i] - cur_ofs);
            string_trim(fields[i]);
            cur_ofs = tab_locs[i] + 1;
        }
    }

    void load_hierarchy()
    {
        string_vec lines;
        if (!read_text_file("geoHierarchy.txt", lines, false, nullptr))
            panic("Failed reading geoHierarchy.txt");

        m_geo_hierarchy.clear();
        m_geo_hierarchy.reserve(500000);

        for (std::string &str : lines)
        {
            string_vec fields;
            extract_tab_fields(str, fields);

            if (fields.size() < 2)
                panic("Invalid geoHierarchy.txt");

            int parent_index = atoi(fields[0].c_str());
            int child_index = atoi(fields[1].c_str());

            m_geo_hierarchy[parent_index].push_back(child_index);
        }
    }

    void load_country_info()
    {
        string_vec country_lines;
        country_lines.reserve(256);

        if (!read_text_file("countryInfo.txt", country_lines, false, nullptr))
            panic("failed reading countryInfo.txt");

        std::vector< std::pair<int, int> > sorted_by_pop;

        for (uint32_t country_rec_index = 0; country_rec_index < country_lines.size(); country_rec_index++)
        {
            const std::string& str = country_lines[country_rec_index];
            if (!str.size())
                continue;
            if (str[0] == '#')
                continue;

            std::vector<int> tab_locs;
            tab_locs.resize(0);

            for (uint32_t ofs = 0; ofs < str.size(); ofs++)
                if (str[ofs] == '\t')
                    tab_locs.push_back(ofs);

            const uint32_t N = 18;
            if (tab_locs.size() != N)
                panic("Unexpected number of tabs");

            tab_locs.push_back((uint32_t)str.size());

            string_vec fields(N + 1);

            enum
            {
                cCountryISO, cCountryISO3, cCountryISONumeric, cCountryfips, cCountryName, cCountryCapital,
                cCountryAreaSqKm, cCountryPopulation, cCountryContinent, cCountryTld, cCountryCurrencyCode, cCountryCurrencyName,
                cCountryPhone, cCountryPostalCodeFormat, cCountryPostalCodeRegex, cCountryLanguages, cCountrygeonameid, cCountryNeighbours, cCountryEquivalentFipsCode
            };

            uint32_t cur_ofs = 0;
            for (uint32_t i = 0; i < fields.size(); i++)
            {
                fields[i] = string_slice(str, cur_ofs, tab_locs[i] - cur_ofs);
                string_trim(fields[i]);
                cur_ofs = tab_locs[i] + 1;
            }

            const int country_index = (int)m_countries.size();

            m_countries.resize(m_countries.size() + 1);
            country_info& new_c = m_countries.back();

            new_c.m_iso2_name = fields[cCountryISO];
            new_c.m_iso3_name = fields[cCountryISO3];
            new_c.m_iso_numeric = fields[cCountryISONumeric];
            new_c.m_fips = fields[cCountryfips];
            new_c.m_country_name = fields[cCountryName];
            new_c.m_capital_name = fields[cCountryCapital];
            new_c.m_area = atoi(fields[cCountryAreaSqKm].c_str());
            new_c.m_pop = atoi(fields[cCountryPopulation].c_str());
            new_c.m_continent = fields[cCountryContinent];
            new_c.m_geoid = atoi(fields[cCountrygeonameid].c_str());
            new_c.m_pop_rank = 0;

            if (!m_geoid_to_rec.count(new_c.m_geoid))
                panic("Failed finding geoid");

            new_c.m_rec_index = m_geoid_to_rec[new_c.m_geoid];
            new_c.m_neighbors = fields[cCountryNeighbours];

            if (m_rec_index_to_country_index.count(new_c.m_rec_index) != 0)
                panic("Invalid rec index");

            m_rec_index_to_country_index[new_c.m_rec_index] = country_index;
            m_geoid_to_country_index[new_c.m_geoid] = country_index;

            auto ins_res = m_country_to_record_index.insert(std::make_pair(new_c.m_iso2_name, new_c.m_rec_index));
            if (!ins_res.second)
                panic("Failed inserting country");

            sorted_by_pop.push_back(std::make_pair(new_c.m_pop, country_index));

#if 0
            {
                const pjson::value_variant* p = &m_doc[new_c.m_rec_index];

                uprintf("a1: %s\n", p->find_string_ptr("a1"));
                uprintf("fclass: %s\n", p->find_string_ptr("fclass"));
                uprintf("fcode: %s\n", p->find_string_ptr("fcode"));
#if 0
                uprintf("-- id: %u, name: \"%s\", lat: %f, long: %f, ccode=%s, a1=%s, a2=%s, a3=%s, a4=%s, fclass: %s, fcode: %s, pop: %i\n",
                    p->find_int32("id"),
                    p->find_string_ptr("name"), p->find_float("lat"), p->find_float("long"),
                    p->find_string_ptr("ccode"),
                    p->find_string_ptr("a1"), p->find_string_ptr("a2"), p->find_string_ptr("a3"), p->find_string_ptr("a4"),
                    p->find_string_ptr("fclass"),
                    p->find_string_ptr("fcode"),
                    p->find_int32("pop"));
#endif
            }

            // countries are all fclass A, a1=00
            // fcode must be PCL, PCLD, PCLF, PCLH, PCLI, PCLIX, PCLS, TERR
#endif

        }

        std::sort(sorted_by_pop.begin(), sorted_by_pop.end(),
            [](const std::pair<int, int>& a, const std::pair<int, int>& b) -> bool
            {
                return a.first < b.first;
            });

        for (uint32_t i = 0; i < sorted_by_pop.size(); i++)
        {
            uint32_t country_index = sorted_by_pop[i].second;
            m_countries[country_index].m_pop_rank = (i * 3 + ((int)sorted_by_pop.size() / 2)) / ((int)sorted_by_pop.size() - 1);
        }
    }
};

static bool has_match(const std::string& str, const std::string& pat, bool ignore_case)
{
    if (ignore_case)
        return (string_ifind_first(str, pat.c_str()) != -1);
    else
        return (string_find_first(str, pat.c_str()) != -1);
}

static bool has_match(const string_vec& str_vec, const std::string& pat, bool ignore_case)
{
    for (const auto& str : str_vec)
        if (has_match(str, pat, ignore_case))
            return true;
    return false;
}

static const char* s_kwic_stop_words[] =
{
    "a", "about", "above", "after", "again", "against", "all", "am", "an", "and", "any", "are", "as",
    "at", "be", "because", "been", "before", "being", "below", "between", "both", "but", "by", "can",
    "could", "did", "do", "does", "doing", "don", "down", "during", "each", "few", "for", "from",
    "further", "had", "has", "have", "having", "he", "her", "here", "hers", "herself", "him", "himself",
    "his", "how", "i", "if", "in", "into", "is", "it", "its", "itself", "just", "me", "more", "most",
    "my", "myself", "no", "nor", "not", "now", "of", "off", "on", "once", "only", "or", "other", "our",
    "ours", "ourselves", "out", "over", "own", "re", "s", "same", "she", "should", "so", "some", "such",
    "t", "than", "that", "the", "their", "theirs", "them", "themselves", "then", "there", "these", "they",
    "this", "those", "through", "to", "too", "under", "until", "up", "very", "was", "we", "were", "what",
    "when", "where", "which", "while", "who", "whom", "why", "will", "with", "you", "your", "yours",
    "yourself", "yourselves", "although", "also", "already", "another", "seemed", "seem", "seems"
};
[[maybe_unused]]
const uint32_t NUM_STOP_WORDS = (uint32_t)std::size(s_kwic_stop_words);

static bool create_kwic_index(const ufo_timeline &timeline, const ufo_timeline::event_urls_map_t &event_urls, bool book_flag = false, const char *pOutput_filename_base = nullptr, const char *pTitle = nullptr, const char *pHeader = nullptr)
{
    struct word_usage
    {
        uint32_t m_event;
        uint32_t m_ofs;
    };
    typedef std::vector<word_usage> word_usage_vec;

    string_vec event_plain_descs;
    event_plain_descs.reserve(timeline.size());

    typedef std::unordered_map<std::string, word_usage_vec> word_map_t;
    word_map_t word_map;
    word_map.reserve(timeline.size() * 20);

    std::unordered_set<std::string> stop_word_set;
    for (const auto& str : s_kwic_stop_words)
        stop_word_set.insert(str);

    for (uint32_t i = 0; i < timeline.size(); i++)
    {
        const timeline_event& event = timeline[i];

        std::string desc;
        if (book_flag)
        {
            desc = event.m_desc;
        }
        else
        {
            markdown_text_processor tp;
            tp.init_from_markdown(event.m_desc.c_str());

            tp.convert_to_plain(desc, true);
        }

        std::string locs;
        for (uint32_t u = 0; u < event.m_locations.size(); u++)
            locs += event.m_locations[u] + " ";

        desc = locs + desc;

        event_plain_descs.push_back(desc);

        //uprintf("%u. \"%s\"\n", i, desc.c_str());

        string_vec words;
        uint_vec offsets;
        get_string_words(desc, words, &offsets);
        for (uint32_t j = 0; j < words.size(); j++)
        {
            if (words[j].length() == 1)
                continue;
            if (words[j].length() > 30)
                continue;
            if (stop_word_set.count(words[j]))
                continue;

            word_usage wu;
            wu.m_event = i;
            wu.m_ofs = offsets[j];

            auto it = word_map.find(words[j]);
            if (it != word_map.end())
                it->second.push_back(wu);
            else
            {
                word_usage_vec v;
                v.push_back(wu);

                word_map.insert(std::make_pair(words[j], v));
            }

            //uprintf("[%s] ", words[j].c_str());
        }
        //uprintf("\n");

    }

    std::vector< word_map_t::const_iterator > sorted_words;
    for (word_map_t::const_iterator it = word_map.begin(); it != word_map.end(); ++it)
        sorted_words.push_back(it);

    std::sort(sorted_words.begin(), sorted_words.end(), [](word_map_t::const_iterator a, word_map_t::const_iterator b)
        {
            //return a->first < b->first;
            return utf8casecmp(a->first.c_str(), b->first.c_str()) < 0;
        }
    );

    string_vec kwic_file_strings_header[NUM_KWIC_FILE_STRINGS];
    string_vec kwic_file_strings_contents[NUM_KWIC_FILE_STRINGS];

    for (uint32_t i = 0; i < NUM_KWIC_FILE_STRINGS; i++)
    {
        std::string name(get_kwic_index_name(i));

        kwic_file_strings_header[i].push_back("<meta charset=\"utf-8\">");
        kwic_file_strings_header[i].push_back("");
        if (pTitle)
            kwic_file_strings_header[i].push_back(string_format("# <a name=\"Top\">%s, KWIC Index Page: %s</a>", pTitle, name.c_str()));
        else
            kwic_file_strings_header[i].push_back(string_format("# <a name=\"Top\">UFO Event Timeline, KWIC Index Page: %s</a>", name.c_str()));

        if (!book_flag)
        {
            kwic_file_strings_header[i].push_back("");
            kwic_file_strings_header[i].push_back("[Back to Main timeline](timeline.html)");
        }

        if (pHeader)
        {
            kwic_file_strings_header[i].push_back(pHeader);
        }

        kwic_file_strings_header[i].push_back("");
        kwic_file_strings_header[i].push_back("## Letters Directory:");

        for (uint32_t j = 0; j < NUM_KWIC_FILE_STRINGS; j++)
        {
            std::string r(get_kwic_index_name(j));

            std::string url;
            if (pOutput_filename_base)
                url = string_format("[%s](%s%s.html)", r.c_str(), pOutput_filename_base, r.c_str());
            else
                url = string_format("[%s](kwic_%s.html)", r.c_str(), r.c_str());

            kwic_file_strings_header[i].push_back(url);
        }

        kwic_file_strings_header[i].push_back("");
        kwic_file_strings_header[i].push_back("## Words Directory:");
    }

    uint32_t sorted_word_index = 0;
    uint32_t num_words_printed = 0;
    for (auto sit : sorted_words)
    {
        const auto it = *sit;

        const std::string& word = it.first;
        const word_usage_vec& usages = it.second;

        uint8_t first_char = word[0];

        uint32_t file_index = 0;
        if (uislower(first_char))
            file_index = first_char - 'a';
        else if (uisdigit(first_char))
            file_index = 26;
        else
            file_index = 27;

        string_vec& output_strs_header = kwic_file_strings_header[file_index];

        output_strs_header.push_back(
            string_format("<a href=\"#word_%u\">\"%s\"</a>", sorted_word_index, word.c_str())
        );

        num_words_printed++;
        if ((num_words_printed % 8) == 0)
        {
            output_strs_header.push_back("  ");
        }

        string_vec& output_strs = kwic_file_strings_contents[file_index];

        output_strs.push_back(string_format("## <a name=\"word_%u\">Word: \"%s\"</a>, %u instance(s) <a href=\"#Top\">(Back to Top)</a>", sorted_word_index, word.c_str(), (uint32_t)usages.size()));

        int_vec word_char_offsets;
        get_utf8_code_point_offsets(word.c_str(), word_char_offsets);

        output_strs.push_back("<pre>");

        for (uint32_t j = 0; j < usages.size(); j++)
        {
            const std::string& str = event_plain_descs[usages[j].m_event];
            const int str_ofs = usages[j].m_ofs;

            int_vec event_char_offsets;
            get_utf8_code_point_offsets(str.c_str(), event_char_offsets);

            int l;
            for (l = 0; l < (int)event_char_offsets.size(); l++)
                if (str_ofs == event_char_offsets[l])
                    break;
            if (l == static_cast<int>(event_char_offsets.size()))
                l = 0;

            const int PRE_CONTEXT_CHARS = 35;
            const int POST_CONTEXT_CHARS = 40;

            // in chars
            int s = std::max<int>(0, l - PRE_CONTEXT_CHARS);
            int e = std::min<int>(l + std::max<int>(POST_CONTEXT_CHARS, (int)word_char_offsets.size()), (int)event_char_offsets.size());
            int char_len = e - s;

            // in bytes
            int start_ofs = event_char_offsets[s];
            int prefix_bytes = event_char_offsets[l] - start_ofs;
            int end_ofs = (e >= static_cast<int>(event_char_offsets.size())) ? (int)str.size() : event_char_offsets[e];
            int len = end_ofs - start_ofs;

            std::string context_str(string_slice(str, start_ofs, len));

            context_str = string_slice(context_str, 0, prefix_bytes) + "<b>" +
                string_slice(context_str, prefix_bytes, word.size()) + "</b>" +
                string_slice(context_str, prefix_bytes + word.size());

            if (l < PRE_CONTEXT_CHARS)
            {
                for (int q = 0; q < (PRE_CONTEXT_CHARS - l); q++)
                {
                    context_str = " " + context_str;
                    //context_str = string_format("%c%c", 0xC2, 0xA0) + context_str; // non-break space
                    char_len++;
                }
            }

            for (uint32_t i = 0; i < context_str.size(); i++)
                if ((uint8_t)context_str[i] < 32U)
                    context_str[i] = ' ';

            int total_chars = PRE_CONTEXT_CHARS + POST_CONTEXT_CHARS;
            if (char_len < total_chars)
            {
                for (int i = char_len; i < total_chars; i++)
                    context_str += ' ';
                //context_str += string_format("%c%c", 0xC2, 0xA0); // non-break space
            }

            std::string url_str(event_urls.find(usages[j].m_event)->second);

            //output_strs.push_back( string_format("`%s` %s  ", context_str.c_str(), url_str.c_str()) );

            output_strs.push_back(string_format("%s %s  ", context_str.c_str(), url_str.c_str()));
        }
        output_strs.push_back("</pre>");
        output_strs.push_back("\n");

        sorted_word_index++;
    }

    for (uint32_t i = 0; i < NUM_KWIC_FILE_STRINGS; i++)
    {
        std::string filename;

        filename = pOutput_filename_base ? pOutput_filename_base : "kwic_";
        if (i < 26)
            filename += string_format("%c", 'a' + i);
        else if (i == 26)
            filename += "numbers";
        else
            filename += "misc";

        filename += ".md";

        string_vec file_strings(kwic_file_strings_header[i]);
        file_strings.push_back("");

        for (auto& str : kwic_file_strings_contents[i])
            file_strings.push_back(str);

        if (!write_text_file(filename.c_str(), file_strings, true))
            panic("Failed writing output file\n");
    }

    return true;
}

static bool load_book_json(
    const char *pSource_filename,
    const char *pURL,
    const char *pName,
    ufo_timeline &timeline,
    ufo_timeline::event_urls_map_t &event_urls)
{
    bool utf8_flag;
    json js;
    if (!load_json_object(pSource_filename, utf8_flag, js))
        return false;

    const uint32_t first_event_index = (uint32_t)timeline.size();
    timeline.get_events().resize(first_event_index + js.size());

    for (uint32_t i = 0; i < js.size(); i++)
    {
        auto obj = js[i];

        if (!obj.is_object())
            panic("Invalid JSON");

        timeline_event& event = timeline.get_events()[first_event_index + i];

        const auto& p = obj.find("page");
        if ((p == obj.end()) || (!p->is_number_unsigned()))
            panic("Invalid JSON");

        const uint32_t page_index = (uint32_t)(*p);

        const auto& t = obj.find("text");
        if ((t == obj.end()) || (!t->is_string()))
            panic("Invalid JSON");
        const std::string text = *t;

        event.m_date_str = "1/1/1900";
        event.m_begin_date.parse(event.m_date_str.c_str(), false);

        event.m_source_id = string_format("%u", page_index);
        //event.m_desc = text;

        bool prev_soft_hypen = false;
        for (uint32_t j = 0; j < text.size(); j++)
        {
            uint8_t c = (uint8_t)text[j];

            uint8_t nc = 0;
            if (j != (text.size() - 1))
                nc = (uint8_t)text[j + 1];

            // 0xC2 AD
            if ((c == 0xC2) && (nc == 0xAD))
            {
                if (event.m_desc.back() == ' ')
                    event.m_desc.pop_back();

                j++;
                prev_soft_hypen = true;
            }
            else if ((c == '<') || (c == '>'))
            {
            }
            else
            {
                if ((prev_soft_hypen) && (c == ' '))
                {

                }
                else
                    event.m_desc.push_back((char)c);

                prev_soft_hypen = false;
            }
        }

        std::string raw_url(string_format(pURL, page_index - 1));

        std::string url(string_format("<a href=\"%s\">%s Page #%u</a>",
            raw_url.c_str(),
            pName,
            page_index));

        event_urls.insert(std::make_pair((int)(i + first_event_index), url));
    }

    return true;
}

const uint32_t NUM_CRASHCONF_URLS = 7;
const uint32_t CRASHCONF_FIRST_YEAR = 2003;
static const char* g_crashconf_urls[NUM_CRASHCONF_URLS] =
{
    "https://archive.org/details/crash-retrieval-conference-proceedings-1st-annual-nov-2003/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-2nd-annual-nov-2004-lq.pdf/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-3rd-annual-2005/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-4th-annual-2006/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-5th-annual-2007/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-6th-annual-2008/page/n%u/mode/1up",
    "https://archive.org/details/crash-retrieval-conference-proceedings-7th-annual-2009/page/n%u/mode/1up"
};

struct crashconf_chapter
{
    const char* m_pName;
    int m_page;
};

const uint32_t MAX_CRASHCONF_CHAPTERS = 20;
static const crashconf_chapter g_crashconf_chapters[NUM_CRASHCONF_URLS][MAX_CRASHCONF_CHAPTERS] =
{
    {
        { u8"_Famous UFO-Related Quotes_, Ryan S. Wood", 5 },
        { u8"_The First Roswell? Evidence for A Crash Retrieval In Cape Girardeau Missouri_, Ryan S. Wood", 21 },
        { u8"_Maury Island UFO: The Crisman Conspiracy_, Kenn Thomas", 41 },
        { u8"_Crash Retrieval on the Plains of St. Augustine, New Mexico_, Art Campbell", 43 },
        { u8"_The 1963 Crash Retrieval North of Albuquerque, New Mexico_, Budd Hopkins", 51 },
        { u8"_The Kecksburg Incident: An Updated Review_, Stan Gordon", 53 },
        { u8"_Cosmic Crashes: UFO Crashes In Britain_, Nick Redfern", 73 },
        { u8"_Puerto Rico UFO Crashes_, Jonathan Downes", 81 },
        { u8"_The \"British Area 51\" & Hacker Matthew Bevan_, Matthew Williams", 83 },
        { u8"_The Strange Death of James Forrestal_, Peter Robbins", 85 },
        { u8"_Research Update: Anomalous Energy Transformations in Layered Bi/Mg Metal_, Linda Howe", 97 },
        { u8"_New Attacks on Roswell and MJ-12_, Stanton Friedman", 117 },
        { u8"_Project Beta: How U.S. Intelligence Created an Alien Invasion_, Greg Bishop", 141 },
        { u8"_Evidence From The National Archives: Governmental Proof of Keen UFO Interest_, Ryan Wood", 153 },
        { u8"_Authenticating the Special Operations Manual_, Dr. Robert M. Wood", 169 },
        { u8"_The President and UFOs - Top Secret_, Grant Cameron", 189 },
        { u8"_Resolving The “Emulation” In Directives Between Twining & Wedemeyer_, Ryan S. Wood", 203 }
    },
    {
        { u8"_Crashed UFOs: A Worldwide History_, Nick Redfern", 4 },
        { u8"_The Very First UFO Crash Retrieval?_, Jim Marrs", 26 },
        { u8"_The Shag Harbor Incident_, Don Ledger", 34 },
        { u8"_The Bentwaters/Woodbridge UFO Incident: New & Previously Unpublished Pieces of the Puzzle_, Peter Robbins", 46 },
        { u8"_UFO Crash Retrievals: U.S. Government Policy of Denial in the Interest of National Security_, Linda Moulton Howe", 60 },
        { u8"_The Presidents and the Hard Evidence (Part 1)_, Grant Cameron", 84 },
        { u8"_Correlating Leonard Stringfield's Crash-Retrieval Reports_, Robert M. Wood", 106 },
        { u8"_The Road to Roswell: A Primer of Undisputed Facts and Factual Disputes_, Paul Davids", 132 },
        { u8"_Inside the USSR Majestic 12 Program_, David Pace", 152 },
        { u8"_The Presidents and the Hard Evidence (Part 2)_, Grant Cameron", 176 },
        { u8"_Murder and Majestic 12: An Investigation of UFO-Related Deaths_, Ryan S. Wood", 198 },
        { u8"_UFO Crash at Aztec_, Paul Kimball", 214 },
        { u8"_Report on the Ramey Office Photos_, Ron Regehr", 226 },
        { u8"_Debris from Two Crashed UFOs in New Mexico_, 1947, Chuck Wade", 240 },
        { u8"_Evidence for the Existence of TOP SECRET/MJ-12: An Examination of Leaked Documents_, Ryan S. Wood", 242 }
    },
    {
        { u8"_Examination of Reverse Engineering Claims_, Robert M. Wood, Ph.D.", 3 },
        { u8"_Researching the Energy Technology of UFOs_, Tom Valone, Ph.D., PE", 13 },
        { u8"_Varginha, Brazil Crash Retrieval Case, January 1966_, Roger Leir", 31 },
        { u8"_The Kingman UFO Crash of 1953_, Nick Redfern", 39 },
        { u8"_An Alleged 1953 UFO Crash and Burial Near Garrison, Utah_, Linda Moulton Howe", 51 },
        { u8"_Sociological Implications of Disclosure_, Richard M. Dolan", 73 },
        { u8"_Orgone Energy, Wilhelm Reich and UFOs_, Peter Robbins", 87 },
        { u8"_New Archeological Evidence from the Mac Brazel, July, 1947 Debris Site_, Chuck Zukowski and Debbie Ziegelmeyer", 107 },
        { u8"_Will the Rocket Engine Go the Way of the Dodos?_, Jim Marrs", 115 },
        { u8"_Majic Eyes Only: Earth's Encounters with Extraterrestrial Technology_, Ryan S. Wood", 117 },
        { u8"_Alien Autopsy Film: A Decade Later_, Philip Mantle", 145 },
        { u8"_San Antonio, NM August 1945 Crash Retrieval_, Reme Baca", 153 },
        { u8"_Analysis of Metal Fragments from a UFO_, Roger Leir", 159 },
        { u8"_Reverse Engineering and Alien Astronautics_, William F. Hamilton III", 161 },
        { u8"_Arizona, 1998: Landed UFO, Military, & Murder?_, Ken Storch", 177 }
    },
    {
        { u8"_The UFO \"Why?\" Questions_, Stanton T. Friedman", 4 },
        { u8"_The Covert World of UOF Crash Retrievals?_, Dr. Michael Saila", 14 },
        { u8"_UFO Crash Retrievals: The History of Leaks and Structure of Secrecy_, Richard M. Dolan", 25 },
        { u8"_Defying Gravity: The Parallel Universe of T. Townsend Brown_, Paul Schatzkin", 34 },
        { u8"_The Devils Hole UFO Crash_, Ryan S. Wood", 44 },
        { u8"_Scenarios of Contact_, Michael Lindemann", 48 },
        { u8"_After the Fire: How the Government Responds to Top Secret Crashes_, Peter W. Merlin", 71 },
        { u8"_The Flatwoods Monster Incident — The Night They Were Here_, Frank Feschino", 89 },
        { u8"_Alien Viruses & Leaked Documents_, Dr. Robert M. Wood", 101 },
        { u8"_1974 The Year of Crashed UFOs Alien Viruses and Biological Warfare_, Nick Redfern", 119 },
        { u8"_Military Voices: Firsthand Testimonies About Non-Human Aerial Craft and Entities_, Linda Moulton Howe", 139 },
        { u8"_The Mystery of December 6, 1950_, Dr. Bruce Maccabee", 164 },
        { u8"_Lummi Island Incident: A Watery Roswell_, Mathew E. Thuney", 180 },
        { u8"_The Bigelow Foundation: Historical Review October 1992 - April 1994_, Dr. Angela Thompson", 196 }
    },
    {
        { u8"_News Management In the Wake of A UFO Crash_, Terry Hansen", 7 },
        { u8"_An Encyclopaedia of Flying Saucers_, Dr. Robert M. Wood", 42 },
        { u8"_After the Retrievals: The Covert Program to Replicate Alien Technology_, Richard M. Dolan", 50 },
        { u8"_UFO and Alien Imagery in Advertising_, Peter Robbins", 59 },
        { u8"_A Close Encounter With Whistleblower Gary Mckinnon_, Matthew Williams", 78 },
        { u8"_Flying Saucers: For Real!_, Michael H. Schrott", 93 },
        { u8"_Elk Mountain Wyoming UFO Crash_, Cameron Debow", 121 },
        { u8"_Project Moon Dust: How the Government Recovers Crashed Flying Saucers_, Nick Redfern", 132 },
        { u8"_May 28, 1974: Glowing Disc Encounter with Military in Albuquerque, New Mexico_, Linda Moulton Howe", 145 },
        { u8"_UFOs and Media Desensitization of Children_, Karyn Dolan", 177 },
        { u8"_1974 Coyame, Mexico UFO Crash, New Revelations_, Ruben Uriarte & Noe Torres", 184 },
        { u8"_A Survey of Retrieved Physical Evidence from South American UFO Cases_, J. Antonio Huneeus", 193 }
    },
    {
        { u8"_April 18, 1962: The Las Vegas UFO Crash_, Dr. Kevin Randle", 7 },
        { u8"_Why UFOs Are A National Security Threat_, Nick Redfern", 23 },
        { u8"_The Westport UFO Crash Retrieval Event_, James Clarkson", 40 },
        { u8"_Ministry of Defense X-Files_, Nick Pope", 61 },
        { u8"_Bentwaters 1980: Telpathic lights?_, Linda Moulton Howe", 71 },
        { u8"_Tunguska: 100 Years Later_, Nick Redfern", 97 },
        { u8"_Forensic Linquistics and the Majestic Documents_, Dr. Robert M. Wood", 104 },
        { u8"_Nazification of America_, Jim Marrs", 123 },
        { u8"_UFOs: The Good News & Bad News, By Dr. John B. Alexander_, Dr. John B. Alexander", 146 },
        { u8"_UFOs and the National Security State, Volume II-1973-1991_, Richard Dolan", 157 },
        { u8"_1974 Coyame, Mexico UFO Crash, New Revelations_, Ruben Uriarte & Noe Torres (no slides)", 1 },
        { u8"_A Survey of Retrieved Physical Evidence from South American UFO Cases_, J. Antonio Huneeus (no slides)", 1 },
    },
    {
        { u8"_Edward Teller and UFOs: Forensic Analysis of Questioned Documents_, Dr. Robert M. Wood", 7 },
        { u8"_Maury Island Revisited_, Kenn Thomas", 29 },
        { u8"_Timeline & Political Analysis of President Kennedy's Efforts to Declassify UFO Files and the Alleged MJ-12 Assassination Directive_, Dr. Michael Solla", 35 },
        { u8"_The Kingman UFO Crash: Updates and New Revelations_, Nick Redfern", 48 },
        { u8"_December 1980 RAF Bentwaters Update with John Burroughs_, Linda Moulton Howe", 57 },
        { u8"_June Crain: Witness to the UFO Crash Cover-up_, James Clarkson", 87 },
        { u8"_Bombshell UFO Case Files: Excerpts from the CUFOs Archives_, Michael Schrott", 97 },
        { u8"_VORONEZH: A Personal Reappraisal of Russia's Best Known CE3 Incident_, Peter Robbins", 134 },
        { u8"_UFOs and the National Security State_, Richard Dolan", 141 },
        { u8"_Russian Esponiage and UFOs_, James Carrion", 154 },
        { u8"_The California Drone and CARET Documents_, TK Davis and Frank Dixon", 186 }
    }
};

static bool create_crashconf_kwic_index()
{
    ufo_timeline timeline;
    ufo_timeline::event_urls_map_t event_urls;

    std::string header("This is an automatically generated [KWIC Index](https://en.wikipedia.org/wiki/Key_Word_in_Context) of the 2003-2009 Crash Retrieval Conference proceedings, created by [Richard Geldreich Jr.](https://twitter.com/richgel999).\n\nHere are links to each year's proceedings and each presentation:\n");

    for (uint32_t i = 0; i < NUM_CRASHCONF_URLS; i++)
    {
        std::string year_str(string_format("%u", CRASHCONF_FIRST_YEAR + i));

        printf("Processing year: %s\n", year_str.c_str());

        if (!load_book_json(
            string_format("crash_conf_%s.json", year_str.c_str()).c_str(),
            g_crashconf_urls[i],
            year_str.c_str(),
            timeline,
            event_urls))
        {
            return false;
        }

        header += string_format("* [%s Crash Retrieval Conference Proceedings (archive.org)](%s)\n", year_str.c_str(), string_format(g_crashconf_urls[i], 0).c_str());

        for (uint32_t j = 0; j < MAX_CRASHCONF_CHAPTERS; j++)
        {
            if (!g_crashconf_chapters[i][j].m_pName)
                break;
            header += string_format("  * [%s](%s)\n", g_crashconf_chapters[i][j].m_pName, string_format(g_crashconf_urls[i], g_crashconf_chapters[i][j].m_page - 1).c_str());
        }
    }

    return create_kwic_index(timeline, event_urls, true, "crashconf_kwic_", "Crash Retrieval Conference Proceedings", header.c_str());
}

[[maybe_unused]]
static int md_trim(const string_vec& args)
{
    if (args.size() != 3)
        panic("Expecting 2 filenames\n");

    string_vec src_file_lines;

    if (!read_text_file(args[1].c_str(), src_file_lines, true, nullptr))
        panic("Failed reading source file %s\n", args[1].c_str());

    uprintf("Read file %s\n", args[1].c_str());

    bool found_header = false, found_footer = false;

    for (uint32_t i = 0; i < std::min<size_t>(15, src_file_lines.size()); i++)
    {
        const std::string& str = src_file_lines[i];
        if (!str.size())
            continue;

        if (string_find_first(str, "---------------") >= 0)
        {
            found_header = true;
            src_file_lines.erase(src_file_lines.begin(), src_file_lines.begin() + i + 1);
            break;
        }
        else if (string_begins_with(str, "- "))
        {
            found_header = true;
            src_file_lines.erase(src_file_lines.begin(), src_file_lines.begin() + i);
            break;
        }
    }

    while (src_file_lines.size())
    {
        src_file_lines[0] = string_trim_end(src_file_lines[0]);
        if (src_file_lines[0].size())
            break;
        src_file_lines.erase(src_file_lines.begin());
    }

    for (int i = std::max<int>(0, (int)src_file_lines.size() - 1); i >= std::max<int>(0, (int)src_file_lines.size() - 5); --i)
    {
        const std::string& str = src_file_lines[i];
        if (!str.size())
            continue;

        if ( (string_find_first(str, "[Chronologie](annees.html)") >= 0) ||
             (string_find_first(str, "[Contact](Contact.html)") >= 0) ||
             (string_find_first(str, "[Home](/)") >= 0))
        {
            found_footer = true;
            src_file_lines.erase(src_file_lines.begin() + i, src_file_lines.end());
            break;
        }
    }

    while (src_file_lines.size())
    {
        std::string& str = src_file_lines[src_file_lines.size() - 1];
        str = string_trim_end(str);
        if (str.size())
            break;
        src_file_lines.erase(src_file_lines.begin() + src_file_lines.size() - 1);
    }

    uprintf("Found header: %u footer: %u\n", found_header, found_footer);

    if (!write_text_file(args[2].c_str(), src_file_lines, true))
        panic("Failed writing output file %s\n", args[2].c_str());

    uprintf("Wrote file %s\n", args[2].c_str());

    return EXIT_SUCCESS;
}

#define TRANSLATE_RECORDS (1)

static bool translate_record(const string_vec& in, string_vec& out)
{
#if !TRANSLATE_RECORDS
    out = in;
    return true;
#else
    assert(in.size());

    string_vec prompt;
    prompt.push_back("Precisely translate this UFO/saucer event record from French to English. Preserve all formatting and new lines, especially the first 2 lines, which contain the date and location. If the record is all-caps, correct it so it's not.");

    prompt.push_back("\"");
    for (const auto& str : in)
        prompt.push_back(str);
    prompt.push_back("\"");

    if (!invoke_openai(prompt, out))
        return false;

    return true;
#endif
}

[[maybe_unused]]
static int md_translate(const string_vec& args)
{
    if (args.size() != 3)
        panic("Expecting 2 filenames\n");

    string_vec src_file_lines;

    if (!read_text_file(args[1].c_str(), src_file_lines, true, nullptr))
        panic("Failed reading source file %s\n", args[1].c_str());

    uprintf("Read file %s\n", args[1].c_str());

    if (src_file_lines.size() < 3)
        panic("File too small\n");

    if (!src_file_lines[0].size() || src_file_lines[0][0] != '!')
        panic("Invalid file\n");
    if (!src_file_lines[1].size() || src_file_lines[1][0] != '!')
        panic("Invalid file\n");

    const int first_year = atoi(src_file_lines[0].c_str() + 1);
    const int last_year = atoi(src_file_lines[1].c_str() + 1);

    if (!first_year || !last_year || (first_year > last_year))
        panic("Invalid header");

    string_vec out_lines;

    uint32_t cur_line = 2;
    string_vec cur_rec;
    std::vector<string_vec> tran_recs;
    while (cur_line < src_file_lines.size())
    {
        std::string l(src_file_lines[cur_line]);
        string_trim(l);

        const bool has_dash = (l.size() > 2) && (l[0] == '#');

        if (has_dash)
        {
            int y = atoi(l.c_str() + 1);
            if (y)
            {
                if ((y < first_year) || (y > last_year))
                    panic("Invalid year\n");
            }
        }
        else if (l.size() && uisdigit(l[0]))
        {
            int y = atoi(l.c_str());
            if ((y) && (y >= first_year) && (y <= last_year))
                uprintf("Suspicious year at line %u\n", cur_line + 1);
        }

        if (cur_rec.size() == 0)
        {
            if (has_dash)
            {
                cur_rec.push_back(src_file_lines[cur_line]);
            }
            else
            {
                panic("Unrecognized text: %s at line %u\n", src_file_lines[cur_line].c_str(), cur_line + 1);
            }
        }
        else
        {
            if (has_dash)
            {
                uprintf("----------------- Record:\n");
                for (uint32_t i = 0; i < cur_rec.size(); i++)
                    uprintf("%s\n", cur_rec[i].c_str());

                tran_recs.push_back(cur_rec);

                cur_rec.resize(0);
            }

            cur_rec.push_back(src_file_lines[cur_line]);
        }

        cur_line++;
    }

    if (cur_rec.size())
    {
        uprintf("Record:\n");
        for (uint32_t i = 0; i < cur_rec.size(); i++)
            uprintf("%s\n", cur_rec[i].c_str());

        tran_recs.push_back(cur_rec);
    }

    uprintf("Found %zu records\n", tran_recs.size());

    for (uint32_t rec_index = 0; rec_index < tran_recs.size(); rec_index++)
    {
        uprintf("------------------\n");

        string_vec tran_rec;
        if (!translate_record(tran_recs[rec_index], tran_rec))
        {
            uprintf("Failed translating record %u!\n", rec_index);

            if (tran_recs[rec_index].size())
                out_lines.push_back(tran_recs[rec_index][0]);
            out_lines.push_back("FAILED!\n");
        }
        else
        {
            if (tran_rec.size())
            {
                if (!tran_rec[0].size())
                    tran_rec.erase(tran_rec.begin());
            }

            if (tran_rec.size())
            {
                std::string& first_line = tran_rec[0];
                if (first_line.size())
                {
                    if (first_line[0] != '#')
                        first_line = "#" + first_line;
                }
            }

            uprintf("Translated record:\n");
            for (uint32_t i = 0; i < tran_rec.size(); i++)
            {
                uprintf("%s\n", tran_rec[i].c_str());

                out_lines.push_back(tran_rec[i]);
            }
        }
    }

    if (!write_text_file(args[2].c_str(), out_lines, true))
        panic("Failed writing output file %s\n", args[2].c_str());

    uprintf("Wrote file %s\n", args[2].c_str());

    return EXIT_SUCCESS;
}

// Main code
int wmain(int argc, wchar_t* argv[])
{
    assert(cTotalPrefixes == sizeof(g_date_prefix_strings) / sizeof(g_date_prefix_strings[0]));

    string_vec args;
    convert_args_to_utf8(args, argc, argv);

    // Set ANSI Latin 1; Western European (Windows) code page for output.
    SetConsoleOutputCP(1252);
    //SetConsoleOutputCP(CP_UTF8);

    converters_init();
    init_norm();
    udb_init();

#if 0
    // TODO: Move to converters.cpp
    //return md_trim(args);

    ufo_timeline tm;
    for (int index = 1; index <= 28; index++)
    {
        std::string in_filename = string_format("fc2/chron%i.txt", index);
        std::string out_filename = string_format("tran_chron%i.txt", index);

        if (!does_file_exist(in_filename.c_str()))
        {
            uprintf("Skipping file %s - doesn't exists\n", in_filename.c_str());
            continue;
        }

        if (does_file_exist(out_filename.c_str()))
        {
            uprintf("Skipping file %s - already exists\n", out_filename.c_str());
            continue;
        }

        string_vec a = { "", in_filename, out_filename };
        int status = md_translate(a);
        if (status != EXIT_SUCCESS)
            panic("md_translate() failed!\n");
    }
    exit(0);
#endif

    bool status = false, utf8_flag = false;

    unordered_string_set unique_urls;

    bool single_file_output = false;

    string_vec filter_strings;
    bool filter_ignore_case = false;
    bool filter_all_flag = false;
    std::string title_str("All events");
    bool conversion_flag = false;
    bool crashconf_flag = false;

    int arg_index = 1;
    while (arg_index < argc)
    {
        std::string arg(args[arg_index]);
        uint8_t t = arg[0];
        arg_index++;

        const uint32_t num_args_remaining = argc - arg_index;

        if (t == '-')
        {
            if (arg == "-convert")
            {
                conversion_flag = true;
            }
            else if (arg == "-crashconf")
            {
                crashconf_flag = true;
            }
            else if (arg == "-filter")
            {
                if (num_args_remaining < 1)
                    panic(string_format("Expected parameter after option: %s", arg.c_str()).c_str());
                else
                {
                    if (!args[arg_index].size())
                        panic("Invalid filter string");

                    filter_strings.push_back(args[arg_index++]);
                }
            }
            else if (arg == "-title")
            {
                if (num_args_remaining < 1)
                    panic(string_format("Expected parameter after option: %s", arg.c_str()).c_str());
                else
                {
                    title_str = args[arg_index++];
                }
            }
            else if (arg == "-filter_all")
            {
                filter_all_flag = true;
            }
            else if (arg == "-filter_ignore_case")
            {
                filter_ignore_case = false;
            }
            else if (arg == "-single_file")
            {
                single_file_output = true;
            }
            else
            {
                panic(string_format("Unrecognized option: %s", arg.c_str()).c_str());
            }
        }
        else
        {
            panic(string_format("Unrecognized option: %s", arg.c_str()).c_str());
        }
    }

    if (crashconf_flag)
    {
        uprintf("Crashing crashconf KWIC index\n");
        if (!create_crashconf_kwic_index())
            panic("create_crashconf_book_kwic_index() failed");
        uprintf("Processing successful\n");
        return EXIT_SUCCESS;
    }

    if (conversion_flag)
    {
        uprintf("Convert Overmeire:\n");
        if (!convert_overmeire())
            panic("convert_overmeire() failed!");
        uprintf("Success\n");

        uprintf("Convert rr0:\n");
        if (!convert_rr0())
            panic("convert_rr0() failed!");
        uprintf("Success\n");

        uprintf("Convert scully:\n");
        if (!convert_magonia("scully.txt", "scully.json", "Scully", u8"[Frank Scully Papers, American Heritage Center, Box 3 FF5](https://archiveswest.orbiscascade.org/ark:80444/xv506256)", 40, "ufo sighting", false))
            panic("convert scully failed!");
        uprintf("Success\n");

        uprintf("Convert dolan.txt:\n");
        if (!convert_dolan("dolan.txt", "dolan.json", "Dolan", "ufo sighting", u8"[_UFOs and the National Security State: Chronology of a Cover-up, 1941–1973_, by Richard Dolan](https://archive.org/details/ufosnationalsecu00dola/mode/2up)"))
            panic("convert_dolan() failed!\n");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap1:\n");
        if (!convert_magonia("pre_roswell_chap1.txt", "pre_roswell_chap1.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "mystery airship", false))
            panic("convert pre_roswell_chap1 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap2:\n");
        if (!convert_magonia("pre_roswell_chap2.txt", "pre_roswell_chap2.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 86))
            panic("convert pre_roswell_chap2 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap3:\n");
        if (!convert_magonia("pre_roswell_chap3.txt", "pre_roswell_chap3.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 109))
            panic("convert pre_roswell_chap3 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap4:\n");
        if (!convert_magonia("pre_roswell_chap4.txt", "pre_roswell_chap4.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 145))
            panic("convert pre_roswell_chap4 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap5:\n");
        if (!convert_magonia("pre_roswell_chap5.txt", "pre_roswell_chap5.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 175))
            panic("convert pre_roswell_chap5 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap6:\n");
        if (!convert_magonia("pre_roswell_chap6.txt", "pre_roswell_chap6.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 201))
            panic("convert pre_roswell_chap6 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap7:\n");
        if (!convert_magonia("pre_roswell_chap7.txt", "pre_roswell_chap7.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 225))
            panic("convert pre_roswell_chap7 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap8:\n");
        if (!convert_magonia("pre_roswell_chap8.txt", "pre_roswell_chap8.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 301))
            panic("convert pre_roswell_chap8 failed!");
        uprintf("Success\n");

        uprintf("Convert pre_roswell_chap9:\n");
        if (!convert_magonia("pre_roswell_chap9.txt", "pre_roswell_chap9.json", "Rife", u8"[_It Didn't Start with Roswell_, by Philip L. Rife, 2001](https://archive.org/details/it-didnt-start-with-roswell-50-years-of-amazing-ufo-crashes-close-encounters-and)", 28, "UFO sighting", false, 348))
            panic("convert pre_roswell_chap9 failed!");
        uprintf("Success\n");

        uprintf("Convert ancient:\n");
        if (!convert_magonia("ancient.txt", "ancient.json", "WondersInTheSky", u8"[_Wonders in the Sky: Unexplained Aerial Objects From Antiquity To Modern Times_, by Jacques Vallée and Chris Aubeck, 2009](https://archive.org/details/JacquesValleeChrisAubeckWondersInTheSkyUnexplainedAerialObjectsFromAntiquityToModernTimes/mode/2up)", 48, "ufo sighting", false, 23))
            panic("convert ancient failed!");
        uprintf("Success\n");

        uprintf("Convert hostile:\n");
        if (!convert_magonia("hostile.txt", "hostile.json", "HostileFawcett", "[_The Flying Saucers are Hostile!_, by George D. Fawcett 1961](https://archive.org/details/the_flying_saucers_are_hostile_george_d_fawcett_1961/mode/2up)", 68, "ufo sighting"))
            panic("convert hostile failed!");
        uprintf("Success\n");

        uprintf("Convert mut:\n");
        if (!convert_magonia("mut.txt", "mut.json", "MysteryHelicoptersAdams", "[The Choppers - and the Choppers, Mystery Helicopters and Animal Mutilations, Thomas R. Adams, 1991](http://www.ignaciodarnaude.com/avistamientos_ovnis/Adams,Thomas,Choppers%20and%20the%20Choppers-1.pdf)", 20, "mystery helicopter/mutilation related"))
            panic("convert_magonia() failed!");
        uprintf("Success\n");

        uprintf("Convert Eberhart:\n");
        if (!convert_eberhart(unique_urls))
            panic("convert_eberthart() failed!");
        uprintf("Success\n");

        uprintf("Convert Nuclear:\n");
        if (!convert_nuk())
            panic("convert_nuk() failed!");
        uprintf("Success\n");

        uprintf("Convert Hatch UDB:\n");
        if (!udb_convert())
            panic("udb_convert() failed!");
        uprintf("Success\n");

        uprintf("Convert NICAP:\n");
        if (!convert_nicap(unique_urls))
            panic("convert_nicap() failed!");
        uprintf("Success\n");

        uprintf("Convert Johnson:\n");
        if (!convert_johnson())
            panic("convert_johnson() failed!");
        uprintf("Success\n");

        uprintf("Convert Trace:\n");
        if (!convert_magonia("trace.txt", "trace.json", "Trace", " [Trace Cases](https://www.thenightskyii.org/trace.html)"))
            panic("convert_magonia() failed!");
        uprintf("Success\n");

        uprintf("Convert Magonia:\n");
        if (!convert_magonia("magonia.txt", "magonia.json"))
            panic("convert_magonia() failed!");
        uprintf("Success\n");

        uprintf("Convert Hall:\n");
        if (!convert_hall())
            panic("convert_hall() failed!");
        uprintf("Success\n");

        uprintf("Convert Bluebook Unknowns:\n");
        if (!convert_bluebook_unknowns())
            panic("convert_bluebook_unknowns failed!");
        uprintf("Success\n");

        uprintf("Convert anon_pdf.md:\n");
        if (!convert_anon())
            panic("convert_anon failed!");
        uprintf("Success\n");
    } // if (conversion_flag)

    uprintf("Total unique URL's: %u\n", (uint32_t)unique_urls.size());

    string_vec urls;
    for (const auto& s : unique_urls)
        urls.push_back(s);
    std::sort(urls.begin(), urls.end());
    write_text_file("unique_urls.txt", urls, false);
    uprintf("Wrote unique_urls.txt\n");

    ufo_timeline timeline;

    status = timeline.load_json("maj2.json", utf8_flag, "Maj2", true);
    if (!status)
        panic("Failed loading maj2.json");
    for (uint32_t i = 0; i < timeline.size(); i++)
        timeline[i].m_source_id = string_format("%s_%u", timeline[i].m_source.c_str(), i);

#if 1
    status = timeline.load_json("overmeire.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading overmeire.json");

    status = timeline.load_json("rr0.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading rr0.json");

    status = timeline.load_json("scully.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading scully.json");

    status = timeline.load_json("dolan.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading dolan.json");

    status = timeline.load_json("hatch_udb.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading hatch_udb.json");

    status = timeline.load_json("nicap_db.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading nicap_db.json");

    status = timeline.load_json("trace.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading trace.json");

    status = timeline.load_json("magonia.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading magnolia.json");

    status = timeline.load_json("bb_unknowns.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading bb_unknowns.json");

    status = timeline.load_json("ufo_evidence_hall.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading ufo_evidence_hall.json");

    status = timeline.load_json("nuclear_tests.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading nuclear_tests.json");

    status = timeline.load_json("eberhart.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading eberhart.json");

    status = timeline.load_json("johnson.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading johnson.json");

    status = timeline.load_json("mut.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading mut.json");

    status = timeline.load_json("hostile.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading hostile.json");

    status = timeline.load_json("ancient.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading hostile.json");

    status = timeline.load_json("pre_roswell_chap1.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap1.json");

    status = timeline.load_json("pre_roswell_chap2.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap2.json");

    status = timeline.load_json("pre_roswell_chap3.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap3.json");

    status = timeline.load_json("pre_roswell_chap4.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap4.json");

    status = timeline.load_json("pre_roswell_chap5.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap5.json");

    status = timeline.load_json("pre_roswell_chap6.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap6.json");

    status = timeline.load_json("pre_roswell_chap7.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap7.json");

    status = timeline.load_json("pre_roswell_chap8.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap8.json");

    status = timeline.load_json("pre_roswell_chap9.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading pre_roswell_chap9.json");
#endif

    status = timeline.load_json("anon_pdf.json", utf8_flag, nullptr, false);
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

        if (!test_date.parse(timeline[i].m_date_str.c_str(), false))
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
            if (!test_date.parse(timeline[i].m_end_date_str.c_str(), false))
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
            if (!test_date.parse(timeline[i].m_alt_date_str.c_str(), false))
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

    if (filter_strings.size())
    {
        ufo_timeline new_timeline;

        for (uint32_t i = 0; i < timeline.size(); i++)
        {
            const timeline_event& event = timeline[i];

            uint32_t total_matched = 0;
            for (uint32_t j = 0; j < filter_strings.size(); j++)
            {
                bool match_flag = false;

                const std::string& filter_string = filter_strings[j];

                if (has_match(event.m_date_str, filter_string, filter_ignore_case) ||
                    has_match(event.m_time_str, filter_string, filter_ignore_case) ||
                    has_match(event.m_alt_date_str, filter_string, filter_ignore_case) ||
                    has_match(event.m_end_date_str, filter_string, filter_ignore_case) ||
                    has_match(event.m_desc, filter_string, filter_ignore_case) ||
                    has_match(event.m_type, filter_string, filter_ignore_case) ||
                    has_match(event.m_refs, filter_string, filter_ignore_case) ||
                    has_match(event.m_locations, filter_string, filter_ignore_case) ||
                    has_match(event.m_attributes, filter_string, filter_ignore_case) ||
                    has_match(event.m_see_also, filter_string, filter_ignore_case) ||
                    has_match(event.m_rocket_type, filter_string, filter_ignore_case) ||
                    has_match(event.m_rocket_altitude, filter_string, filter_ignore_case) ||
                    has_match(event.m_rocket_range, filter_string, filter_ignore_case) ||
                    has_match(event.m_atomic_type, filter_string, filter_ignore_case) ||
                    has_match(event.m_atomic_kt, filter_string, filter_ignore_case) ||
                    has_match(event.m_atomic_mt, filter_string, filter_ignore_case) ||
                    has_match(event.m_source_id, filter_string, filter_ignore_case) ||
                    has_match(event.m_source, filter_string, filter_ignore_case))
                {
                    match_flag = true;
                }

                if (match_flag)
                    total_matched++;
            }

            if ( ((filter_all_flag) && (total_matched == filter_strings.size())) ||
                 ((!filter_all_flag) && (total_matched > 0)) )
            {
                new_timeline.get_events().push_back(event);
            }
        }

        if (!new_timeline.size())
            panic("No events founds.\n");
        else
            uprintf("Found %zu event(s)\n", new_timeline.size());

        timeline.get_events().swap(new_timeline.get_events());
    }

    uprintf("Writing timeline markdown\n");

    ufo_timeline::event_urls_map_t event_urls;

    if (single_file_output)
    {
        timeline.write_markdown("timeline.md", title_str.c_str(), -10000, 10000, true, event_urls, false);
    }
    else
    {
        timeline.write_markdown("timeline.md", "Part 1: Distant Past up to and including 1949", -10000, 1949, false, event_urls, true);
        timeline.write_markdown("timeline_part2.md", "Part 2: 1950 up to and including 1959", 1950, 1959, false, event_urls, true);
        timeline.write_markdown("timeline_part3.md", "Part 3: 1960 up to and including 1969", 1960, 1969, false, event_urls, true);
        timeline.write_markdown("timeline_part4.md", "Part 4: 1970 up to and including 1979", 1970, 1979, false, event_urls, true);
        timeline.write_markdown("timeline_part5.md", "Part 5: 1980 to Present", 1980, 10000, false, event_urls, true);
    }

    uprintf("Writing majestic.json\n");

    timeline.create_plaintext();

    // Write majestic.json, then load it and verify that it saved and loaded correctly.
    {
        for (uint32_t i = 0; i < timeline.size(); i++)
        {
            timeline[i].m_key_value_data.push_back(string_pair("url", event_urls.find(i)->second));
        }
        timeline.set_name("Majestic Timeline");
        timeline.write_file("majestic.json", true);

        ufo_timeline timeline_comp;
        bool utf8_flag_comp;
        if (!timeline_comp.load_json("majestic.json", utf8_flag_comp, nullptr, false))
            panic("Failed loading majestic.json");

        if (timeline.get_events().size() != timeline_comp.get_events().size())
            panic("Failed loading timeline events JSON");

        for (size_t i = 0; i < timeline.get_events().size(); i++)
            if (timeline[i] != timeline_comp[i])
                panic("Failed comparing majestic.json");
    }

    uprintf("Creating KWIC index\n");
    if (!create_kwic_index(timeline, event_urls))
        panic("Failed creating KWIC index");

    uprintf("Processing successful\n");

    return EXIT_SUCCESS;
}
