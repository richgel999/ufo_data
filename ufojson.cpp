// ufojson.cpp
// Copyright (C) 2023 Richard Geldreich, Jr.

#include "utils.h"
#include "markdown_proc.h"
#include "ufojson_core.h"
#include "udb.h"
#include "converters.h"

//-------------------------------------------------------------------

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

    udb_init();
    
    //udb_dump();

#if 0
    detect_bad_urls();
    return 0;
#endif

    bool status = false, utf8_flag = false;

    unordered_string_set unique_urls;

    uprintf("Convert Nuclear:\n");
    if (!convert_nuk())
        panic("convert_nuk() failed!");
    uprintf("Success\n");
        
#if 1
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

    uprintf("Convert Eberhart:\n");
    if (!convert_eberhart(unique_urls))
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
    status = timeline.load_json("hatch_udb.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading hatch_udb.json");

    status = timeline.load_json("nicap_db.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading nicap_db.json");
        
    status = timeline.load_json("trace.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading trace.json");

    status = timeline.load_json("magnonia.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading magnolia.json");

    status = timeline.load_json("bb_unknowns.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading bb_unknowns.json");

    status = timeline.load_json("ufo_evidence_hall.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading ufo_evidence_hall.json");

    status = timeline.load_json("eberhart.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading eberhart.json");
        
    status = timeline.load_json("johnson.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading johnson.json");

    status = timeline.load_json("nuclear_tests.json", utf8_flag, nullptr, false);
    if (!status)
        panic("Failed loading nuclear_tests.json");
#endif

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
        
    // Write majestic.json, then load it and verify that it saved and loaded correctly.
    {
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

    timeline.write_markdown("timeline.md", "Part 1: Distant Past up to and including 1949", -10000, 1949);
    timeline.write_markdown("timeline_part2.md", "Part 2: 1950 up to and including 1959", 1950, 1959);
    timeline.write_markdown("timeline_part3.md", "Part 3: 1960 up to and including 1979", 1960, 1979);
    timeline.write_markdown("timeline_part4.md", "Part 4: 1980 to Present", 1980, 10000);

    uprintf("Processing successful\n");

    return EXIT_SUCCESS;
}
