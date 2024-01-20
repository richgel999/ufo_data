// converters.cpp
// Copyright (C) 2023 Richard Geldreich, Jr.
#include "ufojson_core.h"
#include "markdown_proc.h"
#include <unordered_map>

#define USE_OPENAI (0)

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

bool convert_magonia(const char* pSrc_filename, const char* pDst_filename, const char* pSource_override, const char* pRef_override, uint32_t TOTAL_COLS, const char* pType_override, bool parens_flag, uint32_t first_rec_index)
{
    string_vec lines;

    if (!read_text_file(pSrc_filename, lines, true, nullptr))
        panic("Can't open file %s", pSrc_filename);

    FILE* pOut_file = ufopen(pDst_filename, "w");
    if (!pOut_file)
        panic("Can't open file %s", pDst_filename);

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"%s Timeline\" : [\n", pSource_override ? pSource_override : "Magonia");

    //const uint32_t TOTAL_RECS = 923;

    uint32_t cur_line = 0;
    uint32_t rec_index = first_rec_index;

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

        string_vec desc_lines;
        first_line.erase(0, TOTAL_COLS);
        string_trim(first_line);
        desc_lines.push_back(first_line);

        std::string time_str;

        for (; ; )
        {
            if (cur_line == lines.size())
                break;

            // Absorb empty line which indicates end of record
            if (desc_lines.size() == 1)
            {
                if (!lines[cur_line].size())
                {
                    cur_line++;
                    break;
                }
            }

            std::string buf(lines[cur_line]);

            if ((desc_lines.size() == 1) && (buf.size() < TOTAL_COLS))
            {
                if (string_find_first(buf, ":") >= 0)
                {
                    // Must be ONLY time
                    time_str = buf;
                    string_trim(time_str);
                    cur_line++;
                    break;
                }
            }

            if (buf.size() < TOTAL_COLS)
                break;

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
                // Don't trim '-' char if the previous char is a digit. This is probably imperfect.
                if (!((desc.size() >= 2) && (isdigit((uint8_t)desc[desc.size() - 2]))))
                    desc.resize(desc.size() - 1);
            }
            else if (desc.size())
            {
                desc += " ";
            }

            desc += desc_lines[j];
        }

        std::string location;

        size_t n = desc.find_first_of('}');
        if (n == std::string::npos)
            n = desc.find_first_of('.');
        if (n == std::string::npos)
            panic("Can't find . char");

        location = desc;
        location.resize(n);
        string_trim(location);

        if (parens_flag)
        {
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
        }

        desc.erase(0, n + 1);
        string_trim(desc);

        std::string ref;
        if (parens_flag)
        {
            size_t f = desc.find_last_of('(');
            size_t e = desc.find_last_of(')');
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
        }

        int year = -1, month = -1, day = -1;
        date_prefix_t date_prefix = cNoPrefix;
        std::string date_suffix;

        std::string temp_date_str(date_str);

        if (string_ends_in(temp_date_str, "'s"))
        {
            temp_date_str.resize(temp_date_str.size() - 2);
            date_suffix = "'s";
        }

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
        else if (string_begins_with(temp_date_str, "Late "))
        {
            date_prefix = cLate;
            temp_date_str.erase(0, strlen("Late "));
            string_trim(temp_date_str);
        }
        else if (string_begins_with(temp_date_str, "Mid "))
        {
            date_prefix = cMiddleOf;
            temp_date_str.erase(0, strlen("Mid "));
            string_trim(temp_date_str);
        }

        size_t f = date_str.find_first_of(',');
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
                    if ((year < 34) || (year > 2050))
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
                    if ((year < 34) || (year > 2050))
                        panic("Failed parsing date");
                }
            }
            else
            {
                if (!isdigit(temp_date_str[0]))
                    panic("Failed parsing date");

                year = atoi(temp_date_str.c_str());
                if ((year < 34) || (year > 2050))
                    panic("Failed parsing date");
            }

        }
        else
        {
            // The Magonia data doesn't use the full range of prefixes we support.
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
            else if (string_begins_with(temp_date_str, "Winter,"))
            {
                date_prefix = cWinter;
                temp_date_str.erase(0, strlen("Winter,"));
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
            else if (string_begins_with(temp_date_str, "Late "))
            {
                date_prefix = cLate;
                temp_date_str.erase(0, strlen("Late "));
                string_trim(temp_date_str);
            }

            if (!isdigit(temp_date_str[0]))
                panic("Failed parsing date");

            year = atoi(temp_date_str.c_str());
            if ((year < 34) || (year > 2050))
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

        if (rec_index > first_rec_index) //1)
            fprintf(pOut_file, ",\n");

        fprintf(pOut_file, "{\n");
        fprintf(pOut_file, "  \"date\" : \"");

        if (date_prefix >= 0)
            fprintf(pOut_file, "%s ", g_date_prefix_strings[date_prefix]);

        if (month == -1)
            fprintf(pOut_file, "%i%s", year, date_suffix.c_str());
        else if (day == -1)
        {
            if (date_suffix.size())
                panic("Invalid date suffix");

            fprintf(pOut_file, "%i/%i", month, year);
        }
        else
        {
            if (date_suffix.size())
                panic("Invalid date suffix");

            fprintf(pOut_file, "%i/%i/%i", month, day, year);
        }
        fprintf(pOut_file, "\",\n");

        fprintf(pOut_file, "  \"desc\": \"%s\",\n", escape_string_for_json(desc).c_str());

        if (location.size())
            fprintf(pOut_file, "  \"location\" : \"%s\",\n", escape_string_for_json(location).c_str());

        if (time_str.size())
            fprintf(pOut_file, "  \"time\" : \"%s\",\n", time_str.c_str());

        if (ref.size())
            fprintf(pOut_file, "  \"ref\": \"%s\",\n", escape_string_for_json(ref).c_str());

        if (pSource_override)
            fprintf(pOut_file, "  \"source_id\" : \"%s_%u\",\n", pSource_override, rec_index);
        else
            fprintf(pOut_file, "  \"source_id\" : \"Magonia_%u\",\n", rec_index);

        fprintf(pOut_file, u8"  \"source\" : \"%s\",\n", pSource_override ? pSource_override : u8"ValléeMagonia");

        if (pType_override)
            fprintf(pOut_file, "  \"type\" : \"%s\"\n", pType_override);
        else
            fprintf(pOut_file, "  \"type\" : \"ufo sighting\"\n");

        fprintf(pOut_file, "}");

        rec_index++;
    }

    fprintf(pOut_file, "\n] }\n");
    fclose(pOut_file);

    return true;
}

bool convert_bluebook_unknowns()
{
    string_vec lines;

    if (!read_text_file("bb_unknowns.txt", lines, true, nullptr))
        panic("Can't read file bb_unknowns.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("bb_unknowns.json", "w");
    if (!pOut_file)
        panic("Can't open output file bb_unknowns.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

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
            fprintf(pOut_file, "  \"source_id\" : \"BerlinerBBU_%u\",\n", total_recs);
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

bool convert_hall()
{
    string_vec lines;

    if (!read_text_file("ufo_evidence_hall.txt", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    uint32_t cur_line = 0;

    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen("ufo_evidence_hall.json", "w");
    if (!pOut_file)
        panic("Can't open output file ufo_evidence_hall.json");

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

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
            fprintf(pOut_file, "  \"source_id\" : \"HallUFOE2_%u\",\n", total_recs);
            fprintf(pOut_file, "  \"source\" : \"HallUFOEvidence2\",\n");
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

bool convert_dolan(const char *pSrc_filename, const char *pDst_filename, const char *pSource, const char *pType, const char *pRef)
{
    string_vec lines;

    if (!read_text_file(pSrc_filename, lines, true, nullptr))
        panic("Can't read file %s", pSrc_filename);

    uint32_t cur_line = 0;
    uint32_t total_recs = 0;

    FILE* pOut_file = ufopen(pDst_filename, "w");
    if (!pOut_file)
        panic("Can't open output file %s", pDst_filename);

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\" %s Timeline\" : [\n", pSource);

    while (cur_line < lines.size())
    {
        std::string rec(lines[cur_line]);
        cur_line++;

        string_trim(rec);
        if (rec.empty())
            panic("Encountered empty line");
        if (rec.size() < 54)
            panic("Line too small");

        std::string date_str(rec);
        date_str = string_slice(date_str, 0, 16);
        string_trim(date_str);

        std::string location_str(string_slice(rec, 16, 36));
        string_trim(location_str);

        rec = string_slice(rec, 52);
        string_trim(rec);

        fprintf(pOut_file, "{\n");
        fprintf(pOut_file, "  \"date\" : \"%s\",\n", date_str.c_str());

        fprintf(pOut_file, "  \"location\" : \"%s\",\n", escape_string_for_json(location_str).c_str());
        fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(rec).c_str());

        if (pType)
            fprintf(pOut_file, "  \"type\" : \"%s\",\n", pType);

        if (pRef)
            fprintf(pOut_file, "  \"ref\" : \"%s\",\n", escape_string_for_json(pRef).c_str());

        fprintf(pOut_file, "  \"source_id\" : \"%s_%u\",\n", pSource, total_recs);
        fprintf(pOut_file, "  \"source\" : \"%s\"\n", pSource);

        fprintf(pOut_file, "}");
        if (cur_line < lines.size())
            fprintf(pOut_file, ",");
        fprintf(pOut_file, "\n");

        total_recs++;
    }

    fprintf(pOut_file, "] }\n");
    fclose(pOut_file);

    return true;
}

static const char* g_bad_urls[] =
{
    "https://www.thelivingmoon.com/41pegasus/12insiders/McDonnell%5FDouglas%5FUFO%5FStudies.html",
    "https://ufo.com.br/artigos/relatos---piloto-se-arrisca-em-prova-de-fogo-no-parana.html",
    "http://boblazardebunked.com/investigating-s4-e115/",
    "http://boblazardebunked.com/real-area-51-tech/",
    "https://history.nebraska.gov/sites/history.nebraska.gov/files/doc/publications/NH1979UFOs.pdf",
    "http://alienhunter.org/about/becoming-the-alien-hunter/",
    "http://brumac.8k.com/ChristmasTree/",
    "http://dl.lilibook.ir/2016/03/Other-Tongues-Other-Flesh.pdf",
    "http://documents.irevues.inist.fr/bitstream/handle/2042/51980/meteo%5F1995%5F11%5F8.pdf",
    "http://files.afu.se/Downloads/Magazines/United%20Kingdom/%20Merseyside%20UFO%20Bulletin/Merseyside%20UFO%20Bulletin%20-%20Vol%201%20No%201%20-%201968.pdf",
    "http://ufologie.patrickgross.org/1954/19oct1954criteuil.htm",
    "http://www.bluebookarchive.org/page.aspx?PageCode=NARA-PBB85-812",
    "http://www.bluebookarchive.org/page.aspx?PageCode=NARA-PBB85-813",
    "http://www.bluebookarchive.org/page.aspx?PageCode=NARA-PBB85-816",
    "http://www.bluebookarchive.org/page.aspx?PageCode=NARA-PBB92-607",
    "http://www.bluebookarchive.org/page.aspx?pagecode=NARA-PBB90-354",
    "http://www.cheniere.org/",
    "http://www.historycommons.org/entity.jsp?entity=james%5Fo%5F%5Fconnell",
    "http://www.islandone.org/LEOBiblio/SETI1.HTM",
    "http://www.nicap.org/france74.gif",
    "http://www.phils.com.au/hollanda.htm",
    "http://www.spellconsulting.com/reality/Norway%5FSpiral.html",
    "http://www.wsmr-history.org/HallOfFame52.htm",
    "http://www.zanoverallsongs.com/bio/",
    "https://area51specialprojects.com/area51%5Ftimeline.html",
    "https://area51specialprojects.com/u2%5Fpilots.html",
    "https://cieloinsolito.com/?p=724",
    "https://cieloinsolito.com/wp-content/uploads/2019/09/heretheyare.pdf",
    "https://clubdeleonescuernavacaac.club/1965-1966-joaquin-diaz-gonzalez/",
    "https://dpo.tothestarsacademy.com/blog/crada-faq",
    "https://files.afu.se/Downloads/Magazines/Switzerland/Weltraumbote/Weltraumbote%20-%20No%2001%20-%201955-1956.pdf",
    "https://files.afu.se/Downloads/UFO%20reports/Scandinavia/GR%20translations/46-02-21%20Finland%20meteor/1946-02-21%20Report%20Meteor%20over%20Finland%20and%20east%20Sweden.docx",
    "https://mauriziobaiata.net/2011/11/10/intervista-ad-eugenio-siragusa-1919-2006-la-verita-non-si-vende-e-non-si-compra/",
    "https://psi-encyclopedia.spr.ac.uk/articles/helene-smith",
    "https://trinitysecret.com/the-other-lessons-of-trinity/",
    "https://ufologie.patrickgross.org/1954/17oct1954cier.htm",
    "https://ufologie.patrickgross.org/1954/1oct1954bry.htm",
    "https://ufologie.patrickgross.org/1954/4oct1954poncey.htm",
    "https://ufoscoop.com/richard-c-doty/",
    "https://worldhistoryproject.org/1948/12/20/project-twinkle-established-to-monitor-green-fireball-sightings",
    "https://www.abqjournal.com/obits/profiles/0722057profiles11-07-09.htm",
    "https://www.alternatewars.com/BBOW/ABC%5FWeapons/US%5FNuclear%5FStockpile.htm",
    "https://www.amberley-books.com/community-james-p-templeton",
    "https://www.barnesandnoble.com/w/creative-realism-rolf-alexander/1132618455",
    "https://www.charlotteobserver.com/latest-news/article125658529.html",
    "https://www.hotspotsz.com/former-reporter-recounts-ufo-tale/",
    "https://www.modrall.com/attorney/r-e-thompson/",
    "https://www.mondenouveau.fr/presence-extraterrestre-ummo-une-imposture-ou-pas/",
    "https://www.ourstrangeplanet.com/the-san-luis-valley/guest-editorials/mad-cow-disease-and-cattle-mutilations/",
    "https://www.realclearscience.com/articles/2017/12/02/how%5F17th%5Fcentury%5Fdreamers%5Fplanned%5Fto%5Freach%5Fthe%5Fmoon%5F110476.html",
    "https://www.rhun.co.nz/files/cia/cia1/44%5Fcia%5Fciaall2.pdf",
    "https://www.spacelegalissues.com/the-french-anti-ufo-municipal-law-of-1954/",
    "https://www.thevoicebeforethevoid.net/incidentcomplaint-report-by-commander-44-missile-security-squadron-ellsworth-air-force-base-south-dakota/",
    "https://www.uapsg.com/2020/07/argentina-ufo-declassification.html",
    "https://www.webcitation.org/6mx4huFGk",
    "https://www.webcitation.org/6mx4rfU20",
    "https://www.webcitation.org/6mx5Youbh",
    "https://zazenlife.com/2011/12/18/project-mk-ultra-the-c-i-as-experiments-with-mind-control/"
};

const uint32_t NUM_BAD_URLS = sizeof(g_bad_urls) / sizeof(g_bad_urls[0]);

static std::string fix_bar_urls(const std::string& url)
{
    for (uint32_t i = 0; i < NUM_BAD_URLS; i++)
        if (url == g_bad_urls[i])
            return "https://web.archive.org/web/100/" + url;

    return url;
}

bool convert_eberhart(unordered_string_set& unique_urls)
{
    string_vec lines;

    if (!read_text_file("ufo1_199.md", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo200_399.md", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo400_599.md", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo600_906_1.md", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    if (!read_text_file("ufo600_906_2.md", lines, true, nullptr))
        panic("Can't read file ufo_evidence_hall.txt");

    json eberhart_openai;
    bool utf8_flag;
    if (!load_json_object("eberhart_openai.json", utf8_flag, eberhart_openai))
        panic("Failed loading eberhart_openai.json");

    if (eberhart_openai.find("results") == eberhart_openai.end())
        panic("Couldn't find results");

    const auto& openai_res = eberhart_openai["results"];
    if (!openai_res.is_array())
        panic("Couldn't find results");

    std::unordered_map<uint32_t, std::vector<uint32_t> > openai_res_hash;

    for (uint32_t l = 0; l < openai_res.size(); l++)
    {
        const auto& rec = openai_res[l];
        if (!rec.contains("event_crc32") || !rec.contains("event_date_str") || !rec.contains("locations"))
            panic("Invalid OpenAI JSON data");

        std::vector<uint32_t> list;
        list.push_back(l);

        auto res = openai_res_hash.insert(std::make_pair(rec["event_crc32"].get<uint32_t>(), list));
        if (!res.second)
            (res.first)->second.push_back(l);
    }

    string_vec useful_locs;
    if (!read_text_file("eberhart_useful_locations.txt", useful_locs, true, nullptr))
        panic("failed reading eberhart_useful_locations");

    unordered_string_set useful_locs_set;
    for (const auto& str : useful_locs)
        useful_locs_set.insert(str);

    string_vec trimmed_lines;
    for (uint32_t i = 0; i < lines.size(); i++)
    {
        std::string s(lines[i]);
        string_trim(s);
        if (s.size())
            trimmed_lines.push_back(s);
    }
    lines.swap(trimmed_lines);

    uint32_t cur_line = 0;

    string_vec new_lines;
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

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"Eberhart Timeline\" : [\n");

    int cur_year = -1;
    cur_line = 0;

    uint32_t event_num = 0, total_unattributed = 0;
    uint32_t total_openai_recs_found = 0;

    string_vec location_strs;

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

            for (uint32_t i = 0; i < mt.m_links.size(); i++)
                unique_urls.insert(mt.m_links[i]);

            for (auto& str : mt.m_links)
                str = fix_bar_urls(str);

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
                if ((ref[0] == '\\') &&
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
            begin_date.m_prefix, begin_date.m_month, begin_date.m_day, begin_date.m_year,
            end_date.m_prefix, end_date.m_month, end_date.m_day, end_date.m_year,
            alt_date.m_prefix, alt_date.m_month, alt_date.m_day, alt_date.m_year);

        uprintf("%s  \n\n", desc.c_str());
#endif
        fprintf(pOut_file, "{\n");

        std::string json_date(begin_date.get_string()), json_end_date(end_date.get_string()), json_alt_date(alt_date.get_string());

        fprintf(pOut_file, "  \"date\" : \"%s\",\n", json_date.c_str());

        if (json_end_date.size())
            fprintf(pOut_file, "  \"end_date\" : \"%s\",\n", json_end_date.c_str());

        if (json_alt_date.size())
            fprintf(pOut_file, "  \"alt_date\" : \"%s\",\n", json_alt_date.c_str());

        fprintf(pOut_file, "  \"desc\" : \"%s\",\n", escape_string_for_json(desc).c_str());
        fprintf(pOut_file, "  \"source_id\" : \"Eberhart_%u\",\n", event_num);

        fprintf(pOut_file, "  \"source\" : \"EberhartUFOI\",\n");

        uint32_t hash = crc32((const uint8_t*)desc.c_str(), desc.size());
        hash = crc32((const uint8_t*)&begin_date.m_year, sizeof(begin_date.m_year), hash);
        hash = crc32((const uint8_t*)&begin_date.m_month, sizeof(begin_date.m_month), hash);
        hash = crc32((const uint8_t*)&begin_date.m_day, sizeof(begin_date.m_day), hash);

        auto find_res = openai_res_hash.find(hash);
        if (find_res != openai_res_hash.end())
        {
            const std::vector<uint32_t>& list = find_res->second;

            for (uint32_t l = 0; l < list.size(); l++)
            {
                const uint32_t rec_index = list[l];

                const auto& rec = openai_res[rec_index];

                if (!rec.contains("event_crc32") || !rec.contains("event_date_str") || !rec.contains("locations"))
                    panic("Invalid OpenAI JSON data");

                if (rec["event_crc32"] != hash)
                    panic("hash failed");

                if (rec["event_date_str"] != json_date)
                    continue;

                const auto& loc = rec["locations"];
                if (loc.size())
                {
                    uint32_t total_useful_locs = 0;
                    for (uint32_t k = 0; k < loc.size(); k++)
                    {
                        if (useful_locs_set.find(loc[k]) != useful_locs_set.end())
                            total_useful_locs++;
                    }

                    if (total_useful_locs)
                    {
                        fprintf(pOut_file, "  \"location\" : [ ");

                        uint32_t total_useful_locs_printed = 0;

                        for (uint32_t k = 0; k < loc.size(); k++)
                        {
                            if (useful_locs_set.find(loc[k]) != useful_locs_set.end())
                            {
                                if (total_useful_locs_printed)
                                    fprintf(pOut_file, ", ");

                                fprintf(pOut_file, "\"%s\"", escape_string_for_json(loc[k]).c_str());

                                total_useful_locs_printed++;
                            }
                            else
                            {
                                location_strs.push_back(loc[k]);
                            }
                        }

                        fprintf(pOut_file, " ],\n");
                    }
                }

                total_openai_recs_found++;
                break;
            }
        }

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

    write_text_file("rejected_location_strs.txt", location_strs, true);

    fprintf(pOut_file, "] }\n");
    fclose(pOut_file);

    uprintf("Total records: %u\n", event_num);
    uprintf("Total unattributed: %u\n", total_unattributed);
    uprintf("Total OpenAI recs found: %u\n", total_openai_recs_found);

    return true;
}

bool convert_johnson()
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

    fputc(UTF8_BOM0, pOut_file);
    fputc(UTF8_BOM1, pOut_file);
    fputc(UTF8_BOM2, pOut_file);

    fprintf(pOut_file, "{\n");
    fprintf(pOut_file, "\"Johnson Timeline\" : [\n");

    string_vec combined_lines;

    uint32_t total_recs = 0;

    for (uint32_t month = 0; month < 12; month++)
    {
        for (uint32_t day = 0; day < days_in_month[month]; day++)
        {
            char buf[256];
            sprintf_s(buf, "Johnson/%s%02u.txt", g_full_months[month], day + 1);

            string_vec lines;

            bool utf8_flag = false;
            bool status = read_text_file(buf, lines, true, &utf8_flag);
            if (!status)
                panic("Can't open file %s\n", buf);

            uprintf("Read file %s %u\n", buf, utf8_flag);

            string_vec filtered_lines;
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
                    if ((string_find_first(l, "Written by Donald A. Johnson") != -1) ||
                        (string_find_first(l, "Written by Donald Johnson") != -1) ||
                        (string_find_first(l, "Written by Donald A Johnson") != -1) ||
                        (string_find_first(l, "Compiled from the UFOCAT computer database") != -1) ||
                        (string_find_first(l, u8"© Donald A. Johnson") != -1) ||
                        (string_begins_with(l, "Themes: ")))
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

            lines.erase(lines.begin(), lines.begin() + 5);

            //for (uint32_t i = 0; i < lines.size(); i++)
            //    uprintf("%04u: \"%s\"\n", i, lines[i].c_str());
            //uprintf("\n");

            string_vec new_lines;

            for (uint32_t i = 0; i < lines.size(); i++)
            {
                if (lines[i].size() &&
                    (lines[i].back() == ']') &&
                    ((string_find_first(lines[i], "[") == -1) || (lines[i].front() == '[')))
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
                const std::string& line = lines[i];

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

                if ((string_begins_with(line, "On the ") ||
                    string_begins_with(line, "On this ") ||
                    string_begins_with(line, "That same ") ||
                    string_begins_with(line, "At dusk ") ||
                    string_begins_with(line, "At dawn ") ||
                    string_begins_with(line, "There were ") ||
                    string_begins_with(line, "A UFO was seen ") || string_begins_with(line, "An abduction occurred ") ||
                    string_begins_with(line, "Also in ") ||
                    string_begins_with(line, "There were a ") ||
                    (string_begins_with(line, "At ") && line.size() >= 4 && isdigit((uint8_t)line[3]))) &&
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
                    const std::string& next_line = lines[cur_line];

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
                    ((record_text.back() == ')') || ((record_text.back() == '.') && (record_text[record_text.size() - 2] == ')')))
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

                fprintf(pOut_file, "  \"source_id\" : \"Johnson_%u\",\n", total_recs);

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
    assert(b.m_prefix == cMiddleOf && b.m_year == 1970 && b.m_month == 12 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("December 16?", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 12 && b.m_day == 16 && b.m_fuzzy);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("Fall 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cFall && b.m_year == 1970 && b.m_month == -1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 27, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 27);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("March-June", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 3 && b.m_day == -1);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 6 && e.m_day == -1);
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
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 3 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("Late January 1970-Summer 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cLate && b.m_year == 1970 && b.m_month == 1 && b.m_day == -1);
    assert(e.m_prefix == cSummer && e.m_year == 1971 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 5 or 6", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 5);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == 1970 && a.m_month == 1 && a.m_day == 6);

    if (!event_date::parse_eberhart_date_range("January 20-25", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 1 && e.m_day == 25);
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
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1970 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("February 25, 1970", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 2 && b.m_day == 25);
    assert(e.m_prefix == cNoPrefix && e.m_year == -1 && e.m_month == -1 && e.m_day == -1);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    if (!event_date::parse_eberhart_date_range("January 20 1970-February 25, 1971", b, e, a, 1970)) return false;
    assert(b.m_prefix == cNoPrefix && b.m_year == 1970 && b.m_month == 1 && b.m_day == 20);
    assert(e.m_prefix == cNoPrefix && e.m_year == 1971 && e.m_month == 2 && e.m_day == 25);
    assert(a.m_prefix == cNoPrefix && a.m_year == -1 && a.m_month == -1 && a.m_day == -1);

    // These should all fail
    if (event_date::parse_eberhart_date_range("January 20q 1970-February 25, 1971", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("Januaryq 20 1970-February 25, 1971", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("      ", b, e, a, 1970)) return false;
    if (event_date::parse_eberhart_date_range("   ,  ", b, e, a, 1970)) return false;

    return true;
}

[[maybe_unused]] // currently unused...
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

static void converters_test()
{
    std::string blah;
    blah.push_back(ANSI_SOFT_HYPHEN);

#if 0
    // should print a dash (code page 1252 - ANSI Latin 1)
    putc((char)ANSI_SOFT_HYPHEN, stdout);
    // should print a dash
    uprintf("%s\n", wchar_to_utf8(utf8_to_wchar(blah, CP_ACP)).c_str());
#endif

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

enum
{
    cMonthFlag = 1,
    cBeginToLateFlag = 2,
    cSeasonFlag = 4,
    cApproxFlag = 8,
    cOrFlag = 16,
    cDashFlag = 32,
    cXXFlag = 64,
    cFuzzyFlag = 128,
    cSlashFlag = 256
};

static constexpr struct
{
    const char* m_pStr;
    uint32_t m_flag;
    uint32_t m_month = 0;
    date_prefix_t m_date_prefix = cNoPrefix;
} g_special_phrases[] =
{
    { "january", cMonthFlag, 1 },
    { "february", cMonthFlag, 2 },
    { "march", cMonthFlag, 3 },
    { "april", cMonthFlag, 4 },
    { "may", cMonthFlag, 5 },
    { "june", cMonthFlag, 6 },
    { "july", cMonthFlag, 7 },
    { "august", cMonthFlag, 8 },
    { "september", cMonthFlag, 9 },
    { "october", cMonthFlag, 10 },
    { "november", cMonthFlag, 11 },
    { "december", cMonthFlag, 12 },

    { "jan.", cMonthFlag, 1 },
    { "feb.", cMonthFlag, 2 },
    { "mar.", cMonthFlag, 3 },
    { "apr.", cMonthFlag, 4 },
    { "may.", cMonthFlag, 5 },
    { "jun.", cMonthFlag, 6 },
    { "jul.", cMonthFlag, 7 },
    { "aug.", cMonthFlag, 8 },
    { "sep.", cMonthFlag, 9 },
    { "oct.", cMonthFlag, 10 },
    { "nov.", cMonthFlag, 11 },
    { "dec.", cMonthFlag, 12 },

    { "late", cBeginToLateFlag, 0, cLate },
    { "early", cBeginToLateFlag, 0, cEarly },
    { "middle", cBeginToLateFlag, 0, cMiddleOf },
    { "end", cBeginToLateFlag, 0, cEndOf },

    { "spring", cSeasonFlag, 0, cSpring },
    { "summer", cSeasonFlag, 0, cSummer },
    { "autumn", cSeasonFlag, 0, cAutumn },
    { "fall", cSeasonFlag, 0, cFall },
    { "winter", cSeasonFlag, 0, cWinter },
    { "wint", cSeasonFlag, 0, cWinter },

    { "approx", cApproxFlag },

    { "jan", cMonthFlag, 1 },
    { "feb", cMonthFlag, 2 },
    { "mar", cMonthFlag, 3 },
    { "apr", cMonthFlag, 4 },
    { "may", cMonthFlag, 5 },
    { "jun", cMonthFlag, 6 },
    { "jul", cMonthFlag, 7 },
    { "aug", cMonthFlag, 8 },
    { "sep", cMonthFlag, 9 },
    { "oct", cMonthFlag, 10 },
    { "nov", cMonthFlag, 11 },
    { "dec", cMonthFlag, 12 },

    { "mid", cBeginToLateFlag, 0, cMiddleOf },

    { "or", cOrFlag },
    { "xx", cXXFlag },
    { "-", cDashFlag },
    { "?", cFuzzyFlag },
    { "/", cSlashFlag }
};

constexpr int NUM_SPECIAL_PHRASES = static_cast<int>(std::size(g_special_phrases));

enum
{
    cSpecialJan,
    cSpecialFeb,
    cSpecialMar,
    cSpecialApr,
    cSpecialMay,
    cSpecialJun,
    cSpecialJul,
    cSpecialAug,
    cSpecialSep,
    cSpecialOct,
    cSpecialNov,
    cSpecialDec,

    cSpecialJan2,
    cSpecialFeb2,
    cSpecialMar2,
    cSpecialApr2,
    cSpecialMay2,
    cSpecialJun2,
    cSpecialJul2,
    cSpecialAug2,
    cSpecialSep2,
    cSpecialOct2,
    cSpecialNov2,
    cSpecialDec2,

    cSpecialLate,
    cSpecialEarly,
    cSpecialMiddle,
    cSpecialEnd,

    cSpecialSpring,
    cSpecialSummer,
    cSpecialAutumn,
    cSpecialFall,
    cSpecialWinter,
    cSpecialWinter2,

    cSpecialApprox,

    cSpecialJan3,
    cSpecialFeb3,
    cSpecialMar3,
    cSpecialApr3,
    cSpecialMay3,
    cSpecialJun3,
    cSpecialJul3,
    cSpecialAug3,
    cSpecialSep3,
    cSpecialOct3,
    cSpecialNov3,
    cSpecialDec3,

    cSpecialMid,

    cSpecialOr,
    cSpecialXX,
    cSpecialDash,
    cSpecialFuzzy,
    cSpecialSlash,

    cSpecialTotal
};

static int get_special_from_token(int64_t tok)
{
    if (tok >= 0)
        panic("Invalid token");

    int64_t spec = -tok - 1;
    if (spec >= cSpecialTotal)
        panic("Invalid token");

    return (int)spec;
}

static constexpr bool nipcap_date_is_year_valid(
    int year)
{
    const uint32_t MIN_YEAR = 1860;
    const uint32_t MAX_YEAR = 2012;
    return static_cast<uint32_t>(year) >= MIN_YEAR
        && static_cast<uint32_t>(year) <= MAX_YEAR;
}

static bool convert_nipcap_date(std::string date, event_date& begin_date, event_date& end_date, event_date& alt_date)
{
    static_assert(cSpecialTotal == NUM_SPECIAL_PHRASES);

    string_trim(date);

    bool is_all_digits = true;

    for (char& c : date)
    {
        if (c < 0)
            return false;

        c = (char)tolower((uint8_t)c);

        if (!isdigit(c))
            is_all_digits = false;
    }

    if (is_all_digits)
    {
        // Handle most common, simplest cases.
        if (date.size() == 6)
        {
            // YYMMDD
            int year = convert_hex_digit(date[0]) * 10 + convert_hex_digit(date[1]);
            int month = convert_hex_digit(date[2]) * 10 + convert_hex_digit(date[3]);
            int day = convert_hex_digit(date[4]) * 10 + convert_hex_digit(date[5]);

            if (year <= 8)
                year += 2000;
            else
                year += 1900;

            if (month > 12)
                return false;

            if (day > 31)
                return false;

            begin_date.m_year = year;
            if (month != 0)
            {
                begin_date.m_month = month ? month : -1;

                if (day != 0)
                    begin_date.m_day = day;
            }
            else
            {
                if (day != 0)
                    return false;
            }
            return true;
        }
        else if (date.size() == 8)
        {
            // YYYYMMDD
            int year = convert_hex_digit(date[0]) * 1000 + convert_hex_digit(date[1]) * 100 + convert_hex_digit(date[2]) * 10 + convert_hex_digit(date[3]);
            int month = convert_hex_digit(date[4]) * 10 + convert_hex_digit(date[5]);
            int day = convert_hex_digit(date[6]) * 10 + convert_hex_digit(date[7]);

            if (!nipcap_date_is_year_valid(year))
                return false;

            if (month > 12)
                return false;

            if (day > 31)
                return false;

            if (month == 0)
            {
                if (day != 0)
                    return false;

                begin_date.m_year = year;
            }
            else if (day == 0)
            {
                begin_date.m_year = year;
                begin_date.m_month = month ? month : -1;
            }
            else
            {
                begin_date.m_year = year;
                begin_date.m_month = month ? month : -1;
                begin_date.m_day = day;
            }
            return true;
        }
        else
            return false;
    }

    // Tokenize the input then only parse those cases we explicitly support. Everything else is an error.

    std::vector<int64_t> tokens;
    std::vector<int> digits;

    uint32_t special_flags = 0, cur_ofs = 0;

    while (cur_ofs < date.size())
    {
        if (isdigit(date[cur_ofs]))
        {
            int64_t val = 0;
            int num_digits = 0;

            do
            {
                if (!isdigit(date[cur_ofs]))
                    break;

                val = val * 10 + convert_hex_digit(date[cur_ofs++]);
                if (val < 0)
                    return false;

                num_digits++;

            } while (cur_ofs < date.size());

            tokens.push_back(val);
            digits.push_back(num_digits);
        }
        else if (date[cur_ofs] == ' ')
        {
            cur_ofs++;
        }
        else
        {
            std::string cur_str(date.c_str() + cur_ofs);

            int phrase_index;
            for (phrase_index = 0; phrase_index < NUM_SPECIAL_PHRASES; phrase_index++)
                if (string_begins_with(cur_str, g_special_phrases[phrase_index].m_pStr))
                    break;

            if (phrase_index == NUM_SPECIAL_PHRASES)
                return false;

            tokens.push_back(-(phrase_index + 1));
            digits.push_back(0);

            cur_ofs += (uint32_t)strlen(g_special_phrases[phrase_index].m_pStr);

            special_flags |= g_special_phrases[phrase_index].m_flag;
        }
    }

    assert(tokens.size() == digits.size());

    // Just not supporting slashes in here.
    if (special_flags & cSlashFlag)
        return false;

    if (!tokens.size())
        return false;

    // First token must be a number
    if ((digits[0] != 2) && (digits[0] != 4) && (digits[0] != 6) && (digits[0] != 8))
        return false;

    if (special_flags & cSeasonFlag)
    {
        // Either YYSeason or YYYYSeason

        if (tokens.size() != 2)
            return false;

        int year = 0;
        if (digits[0] == 2)
            year = (int)tokens[0] + 1900;
        else if (digits[0] == 4)
        {
            year = (int)tokens[0];
            if (!nipcap_date_is_year_valid(year))
                return false;
        }
        else
            return false;

        begin_date.m_year = year;

        if (tokens[1] >= 0)
            return false;

        int64_t special_index = -tokens[1] - 1;
        if (special_index >= NUM_SPECIAL_PHRASES)
            return false;

        begin_date.m_prefix = g_special_phrases[special_index].m_date_prefix;

        return true;
    }
    else if (special_flags & cMonthFlag)
    {
        // Not supporting explicit month
        return false;
    }

    // No explicit season and no month - handle XX's
    if ((tokens.size() == 2) && (tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialXX))
    {
        if (digits[0] == 4)
        {
            // YYMMXX
            int year = 1900 + (int)(tokens[0] / 100);
            int month = (int)(tokens[0] % 100);

            if (month > 12)
                return false;

            begin_date.m_year = year;
            begin_date.m_month = month ? month : -1;
        }
        else if (digits[0] == 6)
        {
            // YYYYMMXX

            int year = (int)(tokens[0] / 100);
            if (!nipcap_date_is_year_valid(year))
                return false;

            int month = (int)(tokens[0] % 100);
            if (month > 12)
                return false;

            begin_date.m_year = year;
            begin_date.m_month = month ? month : -1;
        }
        else
        {
            return false;
        }

        return true;
    }
    else if ((tokens.size() == 3) && (tokens[1] < 0) && (tokens[2] < 0) && (get_special_from_token(tokens[1]) == cSpecialXX) && (get_special_from_token(tokens[2]) == cSpecialXX))
    {
        if (digits[0] == 2)
        {
            // YYXXXX
            begin_date.m_year = (int)tokens[0] + 1900;
        }
        else if (digits[0] == 4)
        {
            // YYYYXXXX
            begin_date.m_year = (int)tokens[0];
            if (!nipcap_date_is_year_valid(begin_date.m_year))
                return false;
        }
        else
            return false;

        return true;
    }

    if (special_flags & cXXFlag)
        return false;

    if (digits[0] == 2)
    {
        // YY
        begin_date.m_year = (int)tokens[0] + 1900;
    }
    else if (digits[0] == 4)
    {
        // YYMM
        begin_date.m_year = (int)(tokens[0] / 100) + 1900;

        begin_date.m_month = (int)(tokens[0] % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;
    }
    else if (digits[0] == 6)
    {
        // YYMMDD
        begin_date.m_year = (int)(tokens[0] / 10000) + 1900;

        begin_date.m_month = (int)((tokens[0] / 100) % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;

        begin_date.m_day = (int)(tokens[0] % 100);
        if (begin_date.m_day >= 31)
            return false;

        if ((begin_date.m_month == -1) && (begin_date.m_day))
            return false;
    }
    else if (digits[0] == 8)
    {
        // YYYYMMDD
        begin_date.m_year = (int)(tokens[0] / 10000);
        if (!nipcap_date_is_year_valid(begin_date.m_year))
            return false;

        begin_date.m_month = (int)((tokens[0] / 100) % 100);
        if (begin_date.m_month > 12)
            return false;
        else if (!begin_date.m_month)
            begin_date.m_month = -1;

        begin_date.m_day = (int)(tokens[0] % 100);
        if (begin_date.m_day >= 31)
            return false;

        if ((begin_date.m_month == -1) && (begin_date.m_day))
            return false;
    }
    else
    {
        return false;
    }

    if ((tokens.size() == 2) && (tokens[1] < 0) &&
        (((get_special_from_token(tokens[1]) >= cSpecialLate) && (get_special_from_token(tokens[1]) <= cSpecialEnd)) ||
            (get_special_from_token(tokens[1]) == cSpecialMid))
        )
    {
        // 2 tokens, ends in "late", "middle", "early" etc.
        if (begin_date.m_day != -1)
            return false;

        if (tokens[1] >= 0)
            return false;

        int64_t special_index = -tokens[1] - 1;
        if (special_index >= NUM_SPECIAL_PHRASES)
            return false;

        begin_date.m_prefix = g_special_phrases[special_index].m_date_prefix;

        return true;
    }

    if (special_flags & cBeginToLateFlag)
        return false;

    if ((tokens.size() == 3) && (tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialDash) && (tokens[2] >= 0))
    {
        if ((digits[0] == 6) && (digits[2] == 2))
        {
            // YYMMDD-DD
            end_date = begin_date;

            if (tokens[2] > 31)
                return false;

            end_date.m_day = (int)tokens[2];

            return true;
        }
        else if ((digits[0] == 4) && (digits[2] == 4))
        {
            // YYMM-YYMM

            end_date.m_year = (int)(tokens[2] / 100) + 1900;

            end_date.m_month = (int)(tokens[2] % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 6))
        {
            // YYMMDD-YYMMDD
            end_date.m_year = (int)(tokens[2] / 10000) + 1900;

            end_date.m_month = (int)((tokens[2] / 100) % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            end_date.m_day = (int)(tokens[2] % 100);
            if (end_date.m_day >= 31)
                return false;

            return true;
        }
        else if ((digits[0] == 8) && (digits[2] == 8))
        {
            // YYYYMMDD-YYYYMMDD
            end_date.m_year = (int)(tokens[2] / 10000);
            if (!nipcap_date_is_year_valid(end_date.m_year))
                return false;

            end_date.m_month = (int)((tokens[2] / 100) % 100);
            if (end_date.m_month > 12)
                return false;
            else if (!end_date.m_month)
                end_date.m_month = -1;

            end_date.m_day = (int)(tokens[2] % 100);
            if (end_date.m_day >= 31)
                return false;

            if ((end_date.m_month == -1) && (end_date.m_day))
                return false;

            return true;
        }
        else
        {
            return false;
        }
    }

    if (special_flags & cDashFlag)
        return false;

    if ((tokens.size() == 2) && (get_special_from_token(tokens[1]) == cSpecialFuzzy))
    {
        begin_date.m_fuzzy = true;
        return true;
    }

    if (special_flags & cFuzzyFlag)
        return false;

    if ((tokens.size() == 3) && (get_special_from_token(tokens[1]) == cSpecialOr))
    {
        if ((digits[0] == 2) && (digits[2] == 2))
        {
            // YY or YY
            alt_date.m_year = (int)tokens[2] + 1900;

            return true;
        }
        else if ((digits[0] == 4) && (digits[2] == 4))
        {
            // YYMM or YYMM
            alt_date.m_year = (int)(tokens[2] / 100) + 1900;

            alt_date.m_month = (int)(tokens[2] % 100);
            if (alt_date.m_month > 12)
                return false;
            else if (!alt_date.m_month)
                alt_date.m_month = -1;

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 2))
        {
            // YYMMDD or DD
            alt_date = begin_date;

            if (tokens[2] > 31)
                return false;

            alt_date.m_day = (int)tokens[2];

            return true;
        }
        else if ((digits[0] == 6) && (digits[2] == 6))
        {
            // YYMMDD or YYMMDD
            alt_date.m_year = (int)(tokens[2] / 10000) + 1900;

            alt_date.m_month = (int)((tokens[2] / 100) % 100);
            if (alt_date.m_month > 12)
                return false;
            else if (!alt_date.m_month)
                return false;

            alt_date.m_day = (int)(tokens[2] % 100);
            if (alt_date.m_day >= 31)
                return false;

            return true;
        }
        else
        {
            return false;
        }
    }

    if (special_flags & cOrFlag)
        return false;

    if ((tokens.size() == 2) && ((tokens[1] < 0) && (get_special_from_token(tokens[1]) == cSpecialApprox)))
    {
        begin_date.m_approx = true;
        return true;
    }

    if (special_flags & cApproxFlag)
        return false;

    if (tokens.size() > 1)
        return false;

    return true;
}

const int NICAP_FIRST_CAT = 1;
const int NICAP_LAST_CAT = 11;

static const char* g_nicap_categories[11] =
{
    "01 - Distant Encounters",
    "02 - Close Encounters",
    "03 - EME Cases",
    "04 - Animal Reactions",
    "05 - Medical Incidents",
    "06 - Trace Cases",
    "07 - Entity Cases",
    "08 - Photographic Cases",
    "09 - RADAR Cases",
    "10 - Nuclear Connection",
    "11 - Aviation Cases"
};

static const char* g_nicap_archive_urls[] =
{
    "040228USS%5FSupplydir.htm",
    "421019guadalcanaldir.htm",
    "4409xxoakridge%5Fdir.htm",
    "470116northseadir.htm",
    "470117northseadir.htm",
    "470828japandir.htm",
    "490723delphidir.htm",
    "500110lasvegasdir.htm",
    "500227coultervilledir.htm",
    "500309selfridgedir.htm",
    "500322%5F4925dir.htm",
    "500529mtvernondir.htm",
    "510709dir.htm",
    "520425dir.htm",
    "520501georgedir.htm",
    "520512roswelldir.htm",
    "520526koreadir.htm",
    "520619goosedir.htm",
    "520719wnsdir.htm",
    "520723jamestown%5F01%5Fdir.htm",
    "520723jamestown%5F02%5Fdir.htm",
    "520729dir.htm",
    "520729langley%5Fdir.htm",
    "5207xxdir.htm",
    "520807sanantondir.htm",
    "530213carswelldir.htm",
    "530416eprairiedir.htm",
    "530618iwodir.htm",
    "531016presqudir.htm",
    "531228marysvilledir.htm",
    "571102level%5Fdir.htm",
    "571106b.htm",
    "571110dir.htm",
    "580505dir.htm",
    "640629dir.htm",
    "640904dir.htm",
    "650916dir.htm",
    "660314dir.htm",
    "730202dir.htm",
    "731120mtvernon%5Fdir.htm",
    "770701avianodir.htm",
    "7710xxfostoriadir.htm",
    "801227rendledir.htm",
    "821004ukraine%5Fdir.htm",
    "861117alaskadir.htm",
    "870823dir.htm",
    "aca%5F540621.htm",
    "adickesdir.htm",
    "alamo670302dir.htm",
    "alamos500225Bdir.htm",
    "alamos520722dir.htm",
    "albany520226dir.htm",
    "albuq490106dir.htm",
    "albuq510825dir.htm",
    "albuq520510dir.htm",
    "albuq520528dir.htm",
    "ar-570730dir.htm",
    "ar-641030dir.htm",
    "ar-770126.htm",
    "ar-hubbarddir.htm",
    "arlington470707dir.htm",
    "ashley480408dir.htm",
    "balt520721dir.htm",
    "barradir.htm",
    "benson520403dir.htm",
    "bermuda490124dir.htm",
    "blackstone480724docs1.htm",
    "bonlee501023dir.htm",
    "candir.htm",
    "canogapark571111dir.htm",
    "cashlandir.htm",
    "cavecreek471014dir.htm",
    "chamble480726docs1.htm",
    "colosprings520709dir.htm",
    "columbus520618dir.htm",
    "compass.htm",
    "contrexeville761214dir.htm",
    "cortez490127dir.htm",
    "costarica1dir.htm",
    "coynedir.htm",
    "daggett.htm",
    "dayton471020dir.htm",
    "dayton520712dir.htm",
    "dillon490403dir.htm",
    "dmonthan490516dir.htm",
    "dodgeville4710xxdir.htm",
    "elko490502dir.htm",
    "ellsworth.htm",
    "en-590626dir.htm",
    "f-86dir.htm",
    "fairchild520120dir.htm",
    "fayette491228dir.htm",
    "fortdixdir.htm",
    "france57.htm",
    "ftbliss490519dir.htm",
    "ftmyers501206dir.htm",
    "ftrich470917dir.htm",
    "ftsumner470710dir.htm",
    "fulda520602dir.htm",
    "george520501dir.htm",
    "glenburnie520329dir.htm",
    "gormandir.htm",
    "greenville480419dir.htm",
    "harmondir.htm",
    "hawaii.htm",
    "hawesville870722dir.htm",
    "hickam490104dir.htm",
    "holloman491213dir.htm",
    "hood490427dir.htm",
    "houston520520dir.htm",
    "hynekrv6.htm",
    "indy480731dir.htm",
    "keesler520507dir.htm",
    "keywest501114dir.htm",
    "kirtland500321dir.htm",
    "kirtland800809dir.htm",
    "knoxville500301dir.htm",
    "lmeade520402dir.htm",
    "longmead520417dir.htm",
    "louisville520615dir.htm",
    "lubbock520802dir.htm",
    "lukedir.htm",
    "lynn520701dir.htm",
    "mainbrace.htm",
    "malmstrom75dir.htm",
    "maple480829dir.htm",
    "marshall520429dir.htm",
    "mcchord520617dir.htm",
    "meromdir.htm",
    "minn.htm",
    "minot681028dir.htm",
    "mobile520722dir.htm",
    "mongodir.htm",
    "monroe480528dir.htm",
    "moses520501dir.htm",
    "mtvernon.htm",
    "mufon%5Farg0809%5F83.htm",
    "mufon%5Fau0513%5F83.htm",
    "mufon%5Fau0520%5F83.htm",
    "mufon%5Fau0722%5F83.htm",
    "mufon%5Fbo0302%5F83.htm",
    "mufon%5Fbr0612%5F83.htm",
    "mufon%5Fgb0119%5F83.htm",
    "mufon%5Fgb0315%5F83.htm",
    "mufon%5Fgb0812%5F83.htm",
    "mufon%5Fmx0000%5F83.htm",
    "mufon%5Fru0714%5F83.htm",
    "mufon%5Fru0826%5F83.htm",
    "musk5307XXdir.htm",
    "naha520422dir.htm",
    "nahant520723dir.htm",
    "natcity520513dir.htm",
    "ncp-oakridge501013.htm",
    "nederland660206dir.htm",
    "nellisdir.htm",
    "nkorea520526dir.htm",
    "norwooddir.htm",
    "oakridge501105dir.htm",
    "oakridge511207dir.htm",
    "ontario520412dir.htm",
    "osceola520725dir.htm",
    "paintsville020114dir.htm",
    "palomar491014dir.htm",
    "palomar491125dir.htm",
    "phoenix520405dir.htm",
    "phoenix520808dir.htm",
    "placerville470814dir.htm",
    "plymouth500427dir.htm",
    "pontiac520427dir.htm",
    "portagedir.htm",
    "portland470704dir.htm",
    "pottsdown520723dir.htm",
    "ramore530630dir.htm",
    "rd-750503dir.htm",
    "reno490905dir.htm",
    "reseda570328dir.htm",
    "rockville520722dir.htm",
    "rogue490524dir.htm",
    "roseville520427dir.htm",
    "roswell520217dir.htm",
    "russia551014dir.htm",
    "sanacacia480717dir.htm",
    "sandiego521217dir.htm",
    "sands480405.htm",
    "sands490424dir.htm",
    "sanmarcos520721dir.htm",
    "savannah520510dir.htm",
    "selfridge511124dir.htm",
    "sharondir.htm",
    "shreve520416dir.htm",
    "springer490425dir.htm",
    "stmaries470703dir.htm",
    "sts80.htm",
    "tehrandir.htm",
    "temple520406dir.htm",
    "terredir.htm",
    "tucson490428dir.htm",
    "ubatubadir.htm",
    "vaughn47latedir.htm",
    "vaughn481103dir.htm",
    "vincennes.htm",
    "walnutlake520525dir.htm",
    "washington520713dir.htm",
    "wsands2dir.htm",
    "yuma520417dir.htm",
    "yuma520427dir.htm",
    "zamoradir.htm",
    "charleston1980.htm"
};

const uint32_t NUM_NICAP_ARCHIVE_URLS = sizeof(g_nicap_archive_urls) / sizeof(g_nicap_archive_urls[0]);

bool convert_nicap(unordered_string_set& unique_urls)
{
    string_vec lines;
    bool utf8_flag = false;

    if (!read_text_file("nicap1_150.md", lines, true, &utf8_flag))
        panic("Failed reading input file");
    if (!utf8_flag)
        panic("Expecting utf8 data");

    if (!read_text_file("nicap151_334.md", lines, true, &utf8_flag))
        panic("Failed reading input file");
    if (!utf8_flag)
        panic("Expecting utf8 data");

    string_vec new_lines;
    for (std::string& str : lines)
        if (str.size())
            new_lines.push_back(str);

    lines.swap(new_lines);
    if (!lines.size())
        panic("File empty");

    json js_doc = json::object();
    js_doc["NICAP Data"] = json::array();

    auto& js_doc_array = js_doc["NICAP Data"];

    std::string prev_date, prev_orig_desc;

    uint32_t num_repeated_recs = 0;

    uint32_t record_index = 0;
    uint32_t cur_line_index = 0;
    while (cur_line_index < lines.size())
    {
        std::string date(lines[cur_line_index]);
        if (!date.size())
            panic("Invalid date");

        cur_line_index++;

        std::string ref;

        if (date.front() == '[')
        {
            ref = date;

            markdown_text_processor mtp;
            mtp.init_from_markdown(ref.c_str());

            mtp.fix_redirect_urls();

            if (mtp.m_links.size())
            {
                for (uint32_t i = 0; i < mtp.m_links.size(); i++)
                {
                    std::string& s = mtp.m_links[i];
                    uint32_t j;
                    for (j = 0; j < NUM_NICAP_ARCHIVE_URLS; j++)
                        if (string_find_first(s, g_nicap_archive_urls[j]) != -1)
                            break;

                    if (j < NUM_NICAP_ARCHIVE_URLS)
                        mtp.m_links[i] = "https://web.archive.org/web/100/" + mtp.m_links[i];
                }
            }

            for (uint32_t i = 0; i < mtp.m_links.size(); i++)
                unique_urls.insert(mtp.m_links[i]);

            for (auto& str : mtp.m_links)
                str = fix_bar_urls(str);

            ref.clear();
            mtp.convert_to_markdown(ref, true);

            // 6+2+2+1 chars
            if (date.size() < 11)
                panic("Invalid date");

            date.erase(0, 1);

            uint32_t i;
            bool found_end_bracket = false;
            for (i = 0; i < date.size(); i++)
            {
                if (date[i] == ']')
                {
                    date.erase(i, date.size() - i);
                    found_end_bracket = true;
                    break;
                }
            }

            if (!found_end_bracket)
                panic("Invalid date");
        }

        event_date begin_date, end_date, alt_date;

        bool status = convert_nipcap_date(date, begin_date, end_date, alt_date);
        if (!status)
            panic("Failed converting NICAP date");

        // Get first line of city
        if (cur_line_index == lines.size())
            panic("Out of lines");

        std::string city(lines[cur_line_index++]);
        if (!city.size() || (city == "* * *"))
            panic("Expected city");

        if (cur_line_index == lines.size())
            panic("Failed parsing NICAP data");

        // Look ahead for category, 1-11
        int cat_index = -1;
        int cat_line_index = -1;

        for (uint32_t i = 0; i < 5; i++)
        {
            if ((cur_line_index + i) >= lines.size())
                break;

            const std::string& s = lines[cur_line_index + i];
            if ((s.size() <= 2) && (string_is_digits(s)))
            {
                int val = atoi(s.c_str());
                if ((val >= NICAP_FIRST_CAT) && (val <= NICAP_LAST_CAT))
                {
                    cat_index = val;
                    cat_line_index = cur_line_index + i;
                    break;
                }
            }
        }
        if (cat_index == -1)
            panic("Can't find category");

        for (int i = cur_line_index; i < (cat_line_index - 1); i++)
            city = combine_strings(city, lines[i]);

        // Get state or country, which is right before the category
        std::string state_or_country(lines[cat_line_index - 1]);
        if (!state_or_country.size() || (state_or_country == "* * * "))
            panic("Expected state or country");

        if (cur_line_index == lines.size())
            panic("Failed parsing NICAP data");

        cur_line_index = cat_line_index + 1;

        string_vec rec;

        while (cur_line_index < lines.size())
        {
            const std::string& str = lines[cur_line_index];

            if (str == "* * *")
            {
                cur_line_index++;

                break;
            }
            else if (str.size() && str[0] == '[')
                break;

            if ((str.size() >= 2) && isdigit((uint8_t)str[0]) && isdigit((uint8_t)str[1]))
            {
                event_date next_begin_date, next_end_date, next_alt_date;
                bool spec_status = convert_nipcap_date(str, next_begin_date, next_end_date, next_alt_date);
                if (spec_status)
                    break;
            }

            rec.push_back(str);
            cur_line_index++;
        }

        if (!rec.size())
            panic("Failed parsing record");

        std::string code, bb;
        int rating = -1;
        bool nc_flag = false, lc_flag = false;

        for (int i = 0; i < std::min(5, (int)rec.size()); i++)
        {
            if (rec[i] == "NC")
            {
                nc_flag = true;
                rec.erase(rec.begin() + i);
                break;
            }
        }

        for (int i = 0; i < std::min(5, (int)rec.size()); i++)
        {
            if (rec[i] == "LC")
            {
                lc_flag = true;
                rec.erase(rec.begin() + i);
                break;
            }
        }

        if (!rec.size())
            panic("Invalid record");

        if (string_is_alpha(rec[0]) && (rec[0].size() <= 2))
        {
            if ((rec[0] == "A") || (rec[0] == "R") || (rec[0] == "En") || (rec[0] == "An") || (rec[0] == "C") ||
                (rec[0] == "E") || (rec[0] == "D") || (rec[0] == "M") || (rec[0] == "I") || (rec[0] == "U") || (rec[0] == "N") || (rec[0] == "T") || (rec[0] == "aM"))
            {
                code = rec[0];
                rec.erase(rec.begin());

                if (!rec.size())
                    panic("Invalid record");
            }
            else
            {
                panic("Unknown code");
            }
        }

        int i, j = std::min(5, (int)rec.size());
        for (i = 0; i < j; i++)
            if ((rec[i] == "BBU") || (rec[i] == "BB"))
                break;

        bool found_bbu = false;
        if (i < j)
        {
            bb = rec[i];
            rec.erase(rec.begin() + i);

            if (!rec.size())
                panic("Invalid record");

            found_bbu = true;
        }

        if (string_is_digits(rec[0]) && (rec[0].size() == 1))
        {
            int val = atoi(rec[0].c_str());
            if ((val < 0) || (val > 5))
                panic("Invalid rating");
            rating = val;

            rec.erase(rec.begin());

            if (!rec.size())
                panic("Invalid record");
        }

        if (!found_bbu && string_is_digits(rec[0]) && (rec[0].size() >= 2))
        {
            int val = atoi(rec[0].c_str());
            if ((val >= 12) && (val <= 12607))
            {
                bb = rec[0];

                rec.erase(rec.begin());

                if (!rec.size())
                    panic("Invalid record");
            }
            else
            {
                panic("Bad BB #");
            }
        }

        json js;
        js["date"] = begin_date.get_string();

        if (end_date.is_valid())
            js["end_date"] = end_date.get_string();

        if (alt_date.is_valid())
            js["alt_date"] = alt_date.get_string();

        js["ref"] = json::array();
        if (ref.size())
            js["ref"].push_back("NICAP: " + ref);
        js["ref"].push_back("[NICAP Listing by Date PDF](http://www.nicap.org/NSID/NSID_DBListingbyDate.pdf)");

        if (state_or_country.size())
            js["location"] = city + ", " + state_or_country;
        else
            js["location"] = city;

        std::string desc;
        for (const std::string& s : rec)
            desc = combine_strings(desc, s);

        assert(cat_index >= 1 && cat_index <= 11);

        const std::string orig_desc(desc);

        desc = desc + string_format(" (NICAP: %s", g_nicap_categories[cat_index - 1]);
        if (code.size())
            desc = desc + string_format(", Code: %s", code.c_str());
        if (rating != -1)
            desc = desc + string_format(", Rating: %i", rating);
        if (bb.size())
            desc = desc + string_format(", BB: %s", bb.c_str());
        if (nc_flag)
            desc = desc + string_format(", NC");
        if (lc_flag)
            desc = desc + string_format(", LC");
        desc += ")";

        if ((desc[0] >= 0) && islower((uint8_t)desc[0]))
            desc[0] = (char)toupper((uint8_t)desc[0]);

        js["desc"] = desc;
        js["source"] = "NICAP_DB";
        js["source_id"] = "NICAP_DB" + string_format("_%u", record_index);

        if ((prev_orig_desc.size()) && (orig_desc == prev_orig_desc) && (js["date"] == prev_date))
        {
            // It's a repeated record, with just a different category.
            std::string new_desc(js_doc_array.back()["desc"]);

            new_desc += string_format(" (NICAP: %s)", g_nicap_categories[cat_index - 1]);

            js_doc_array.back()["desc"] = new_desc;

            num_repeated_recs++;
        }
        else
        {
            prev_date = js["date"];
            prev_orig_desc = orig_desc;

            js_doc_array.push_back(js);
        }

#if 0
        uprintf("Record: %u\n", record_index);

        uprintf("Date string: %s\n", date.c_str());

        uprintf("Date: %s\n", begin_date.get_string().c_str());

        if (end_date.is_valid())
            uprintf("End date: %s\n", end_date.get_string().c_str());

        if (alt_date.is_valid())
            uprintf("Alt date: %s\n", alt_date.get_string().c_str());

        if (ref.size())
            uprintf("Ref: %s\n", ref.c_str());

        uprintf("City: %s\n", city.c_str());
        uprintf("State or County: %s\n", state_or_country.c_str());

        uprintf("Category: %i\n", cat_index);

        if (code.size())
            uprintf("NICAP code: %s\n", code.c_str());
        if (rating != -1)
            uprintf("NICAP Rating: %u\n", rating);
        if (bb.size())
            uprintf("NICAP BB: %s\n", bb.c_str());
        if (nc_flag || lc_flag)
            uprintf("NICAP nc_flag: %u lc_flag: %u\n", nc_flag, lc_flag);

        uprintf("Record:\n");
        for (const std::string& s : rec)
            uprintf("%s\n", s.c_str());

        uprintf("\n");
#endif

        record_index++;
    }

    if (!serialize_to_json_file("nicap_db.json", js_doc, true))
        panic("Failed serializing JSON file");

    uprintf("Processed %u records, skipping %u repeated records\n", record_index, num_repeated_recs);

    return true;
}

void converters_init()
{
    converters_test();
}

bool convert_nuk()
{
    std::string title;
    string_vec col_titles;

    std::vector<string_vec> rows;

    bool success = load_column_text("nuktest_usa.txt", rows, title, col_titles, false, "USA");
    success = success && load_column_text("nuktest_ussr.txt", rows, title, col_titles, false, "USSR");
    success = success && load_column_text("nuktest_britain.txt", rows, title, col_titles, false, "Britain");
    success = success && load_column_text("nuktest_china.txt", rows, title, col_titles, false, "China");
    success = success && load_column_text("nuktest_france.txt", rows, title, col_titles, false, "France");
    success = success && load_column_text("nuktest_india.txt", rows, title, col_titles, false, "India");
    success = success && load_column_text("nuktest_pakistan.txt", rows, title, col_titles, false, "Pakistan");
    success = success && load_column_text("nuktest_unknown.txt", rows, title, col_titles, false, "Unknown");
    if (!success)
        panic("Failed loading nuk column text file");

    enum { cColDate, cColTime, cColLat, cColLong, cColDepth, cColMb, cColY, cColYMax, cColType, cColName, cColSource, cColCountry, cColTotal };

    ufo_timeline timeline;

    uint32_t event_id = 0;
    for (const string_vec& x : rows)
    {
        if (x.size() != cColTotal)
            panic("Invalid # of cols");

        timeline_event event;
        if (!event.m_begin_date.parse(x[cColDate].c_str(), true))
            panic("Failed parsing date");

        event.m_date_str = event.m_begin_date.get_string();

        if (x[cColTime] != "00:00:00.0")
            event.m_time_str = x[cColTime];

        event.m_type.push_back("atomic");
        event.m_locations.push_back(x[cColLat] + " " + x[cColLong]);

        std::string attr;

        std::string t(string_upper(x[cColType]));

        bool salvo = false;
        if (string_ends_in(t, "_SALVO"))
        {
            salvo = true;
            t = string_slice(t, 0, t.size() - 6);
        }

        if (t.size() == 1)
        {
            if (t[0] == 'A')
                attr += "Air";
            else if (t[0] == 'W')
                attr += "Water";
            else if (t[0] == 'U')
                attr += "Underground";
            else
                panic("Unknown type");
        }
        else if ((t.size() == 4) || (t.size() == 5))
        {
            if (t.size() == 5)
            {
                if (t.back() != 'G')
                    panic("Invalid type");
            }
            else
            {
                //US(U), USSR(S), France(F), China(C), India(I), United Kingdom(G), or Pakistan(P)

                const char c = t.back();
                if ((c != 'U') && (c != 'S') && (c != 'F') && (c != 'C') && (c != 'I') && (c != 'G') && (c != 'P'))
                    panic("Invalid type");
            }

            if (t[0] != 'N')
                panic("Not nuclear");

            if (t[1] == 'A')
                attr += "Air";
            else if (t[1] == 'W')
                attr += "Water";
            else if (t[1] == 'U')
                attr += "Underground";
            else
                panic("Unknown type");

            if (t[2] == 'C')
                attr += ", confirmed";
            else if (t[2] == 'P')
                attr += ", presumed";
            else
                panic("Unknown type");

            if (salvo)
                attr += ", salvo";
        }
        else
            panic("Invalid type");

        event.m_desc = string_format("Nuclear test: %s. Country: %s", attr.c_str(), x[cColCountry].c_str());

        if ((x[cColName].size()) && (x[cColName] != "-"))
            event.m_desc += string_format(u8" Name: “%s”", x[cColName].c_str());

        if (x[cColY].size())
            event.m_desc += string_format(" Yield: %sKT", x[cColY].c_str());

        if (x[cColYMax].size())
            event.m_desc += string_format(" YieldMax: %sKT", x[cColYMax].c_str());

        if (x[cColLat].size() && x[cColLong].size())
        {
            event.m_key_value_data.push_back(string_pair("LatLong", x[cColLat] + " " + x[cColLong]));

            const double lat = atof(x[cColLat].c_str());
            const double lon = atof(x[cColLong].c_str());

            std::string latitude_dms = get_deg_to_dms(lat) + ((lat <= 0) ? " S" : " N");
            std::string longitude_dms = get_deg_to_dms(lon) + ((lon <= 0) ? " W" : " E");

            event.m_key_value_data.push_back(string_pair("LatLongDMS", latitude_dms + " " + longitude_dms));
        }

        if (x[cColDepth].size())
            event.m_key_value_data.push_back(string_pair("NukeDepth", x[cColDepth]));

        if (x[cColMb].size())
            event.m_key_value_data.push_back(string_pair("NukeMb", x[cColMb]));

        if (x[cColY].size())
            event.m_key_value_data.push_back(string_pair("NukeY", x[cColY]));

        if (x[cColYMax].size())
            event.m_key_value_data.push_back(string_pair("NukeYMax", x[cColYMax]));

        if (x[cColType].size())
            event.m_key_value_data.push_back(string_pair("NukeType", x[cColType]));

        if (x[cColName] != "-")
            event.m_key_value_data.push_back(string_pair("NukeName", x[cColName]));

        event.m_key_value_data.push_back(string_pair("NukeSource", x[cColSource]));
        event.m_key_value_data.push_back(string_pair("NukeCountry", x[cColCountry]));

        if (x[cColLat].size() && x[cColLong].size())
        {
            event.m_key_value_data.push_back(std::make_pair("LocationLink", string_format("[Google Maps](https://www.google.com/maps/place/%s,%s)", x[cColLat].c_str(), x[cColLong].c_str())));
        }

        event.m_refs.push_back("[\"columbia.edu: Worldwide Nuclear Explosions\", by Yang, North, Romney, and Richards](https://www.ldeo.columbia.edu/~richards/my_papers/WW_nuclear_tests_IASPEI_HB.pdf)");
        event.m_refs.push_back("[\"archive.org: Worldwide Nuclear Explosions\", by Yang, North, Romney, and Richards](https://web.archive.org/web/20220407121213/https://www.ldeo.columbia.edu/~richards/my_papers/WW_nuclear_tests_IASPEI_HB.pdf)");

        event.m_source = "NukeExplosions";
        event.m_source_id = event.m_source + string_format("_%u", event_id);

        timeline.get_events().push_back(event);

        event_id++;
    }

    if (!timeline.get_events().size())
        panic("Empty timeline)");

    timeline.set_name("Nuclear Test Timeline");

    return timeline.write_file("nuclear_tests.json", true);
}

bool convert_anon()
{
    string_vec lines;
    bool utf8_flag = false;

    const char* pFilename = "anon_pdf.md";
    if (!read_text_file(pFilename, lines, true, &utf8_flag))
        panic("Failed reading text file %s", pFilename);

    ufo_timeline timeline;

    uint32_t event_id = 1;

    uint32_t line_index = 0;
    while (line_index < lines.size())
    {
        std::string& s = lines[line_index++];

        if (!string_begins_with(s, "**(PUBLIC DOMAIN)** - **"))
            continue;

        if (s.size() < 27)
            panic("Invalid string");

        //[0x00000026] 0xe2 'â'	char
        //[0x00000027]	0x80 '€'	char
        //[0x00000028]	0x94 '”'	char

        const int8_t c = -30;// (int8_t)0xE2;
        size_t dash_pos = s.find_first_of(c);
        if (dash_pos == std::string::npos)
            panic("Invalid string");

        if ((dash_pos + 2) >= s.size())
            panic("Invalid string");

        timeline_event event;

        std::string date_str(s);
        date_str.erase(0, 24);

        size_t ofs = date_str.find_first_of('*');
        if (ofs == std::string::npos)
            panic("Invalid string");

        date_str.erase(date_str.begin() + ofs, date_str.end());

        const std::string orig_date_str(date_str);

        if (string_is_digits(date_str))
        {
            // just a year
            int year = atoi(date_str.c_str());
            if ((year < 1947) || (year > 2030))
                panic("Invalid year");

            event.m_begin_date.m_year = year;
        }
        // Handle exceptions manually - there are just not enough dates in this file to justify writing custom parsers, and they didn't use a consistent standard anyway.
        else if (date_str == "1980s Undefined")
        {
            event.m_begin_date.m_year = 1980;
            event.m_begin_date.m_fuzzy = true;
            event.m_begin_date.m_plural = true;
        }
        else if (date_str == "1984/1985")
        {
            event.m_begin_date.m_year = 1984;
            event.m_end_date.m_year = 1985;
        }
        else if (date_str == "June/July 1971")
        {
            event.m_begin_date.m_month = 6;
            event.m_begin_date.m_year = 1971;

            event.m_end_date.m_month = 7;
            event.m_end_date.m_year = 1971;
        }
        else if (date_str == "June-July 2011")
        {
            event.m_begin_date.m_month = 6;
            event.m_begin_date.m_year = 2011;
            event.m_end_date.m_month = 7;
            event.m_end_date.m_year = 2011;
        }
        else if (date_str == "1966-1967")
        {
            event.m_begin_date.m_year = 1966;
            event.m_end_date.m_year = 1967;
        }
        else if (date_str == "31 July 1965-2 August 1965")
        {
            event.m_begin_date.m_day = 31;
            event.m_begin_date.m_month = 7;
            event.m_begin_date.m_year = 1965;

            event.m_end_date.m_day = 2;
            event.m_end_date.m_month = 8;
            event.m_end_date.m_year = 1965;
        }
        else if (date_str == "Mid-1990s")
        {
            event.m_begin_date.m_prefix = cMiddleOf;
            event.m_begin_date.m_plural = true;
            event.m_begin_date.m_year = 1990;
        }
        else if (date_str == "Late 1990s")
        {
            event.m_begin_date.m_prefix = cLate;
            event.m_begin_date.m_plural = true;
            event.m_begin_date.m_year = 1990;
        }
        else if (date_str == "Mid 1995")
        {
            event.m_begin_date.m_prefix = cMiddleOf;
            event.m_begin_date.m_year = 1995;
        }
        else if (date_str == "01 Jan 2007")
        {
            event.m_begin_date.m_day = 1;
            event.m_begin_date.m_month = 1;
            event.m_begin_date.m_year = 2007;
        }
        else if (date_str == "5 Jan 2007")
        {
            event.m_begin_date.m_day = 5;
            event.m_begin_date.m_month = 1;
            event.m_begin_date.m_year = 2007;
        }
        else if (date_str == "Early 1994")
        {
            event.m_begin_date.m_prefix = cEarly;
            event.m_begin_date.m_year = 1994;
        }
        else if (date_str == "Early 1995")
        {
            event.m_begin_date.m_prefix = cEarly;
            event.m_begin_date.m_year = 1995;
        }
        else if (date_str == "May/June 2022")
        {
            event.m_begin_date.m_month = 5;
            event.m_begin_date.m_year = 2022;

            event.m_end_date.m_month = 6;
            event.m_end_date.m_year = 2022;
        }
        else if (date_str == "Late 1990s")
        {
            event.m_begin_date.m_prefix = cLate;
            event.m_begin_date.m_year = 1990;
        }
        else if (date_str == "Late 1956")
        {
            event.m_begin_date.m_prefix = cLate;
            event.m_begin_date.m_year = 1956;
        }
        else if (date_str == "Late 1963")
        {
            event.m_begin_date.m_prefix = cLate;
            event.m_begin_date.m_year = 1963;
        }
        else if (date_str == "Late 1964")
        {
            event.m_begin_date.m_prefix = cLate;
            event.m_begin_date.m_year = 1964;
        }
        else if (date_str == "23-24 October 2010")
        {
            event.m_begin_date.m_day = 23;
            event.m_begin_date.m_month = 10;
            event.m_begin_date.m_year = 2010;

            event.m_end_date.m_day = 24;
            event.m_end_date.m_month = 10;
            event.m_end_date.m_year = 2010;
        }
        else if (date_str == "1980s")
        {
            event.m_begin_date.m_plural = true;
            event.m_begin_date.m_year = 1980;
        }
        else if (uisdigit(date_str[0]))
        {
            // Day Month Year
            int day = atoi(date_str.c_str());
            if ((day < 1) || (day > 31))
                panic("Failed parsing date: %s\n", date_str.c_str());

            size_t n = date_str.find_first_of(' ');
            if (n == std::string::npos)
                panic("Failed parsing date: %s\n", date_str.c_str());

            date_str.erase(date_str.begin(), date_str.begin() + n + 1);

            if (!event_date::parse_eberhart_date_range(date_str, event.m_begin_date, event.m_end_date, event.m_alt_date, -1))
                panic("Failed parsing date: %s\n", date_str.c_str());

            event.m_begin_date.m_day = day;
        }
        else
        {
            // Month Year
            if (!event_date::parse_eberhart_date_range(date_str, event.m_begin_date, event.m_end_date, event.m_alt_date, -1))
                panic("Failed parsing date: %s\n", date_str.c_str());
        }

        event.m_date_str = event.m_begin_date.get_string();

        if (event.m_end_date.is_valid())
            event.m_end_date_str = event.m_end_date.get_string();

        if (event.m_alt_date.is_valid())
            event.m_alt_date_str = event.m_alt_date.get_string();

#if 0
        uprintf("%s, %s-%s %s\n", orig_date_str.c_str(),
            event.m_date_str.c_str(),
            event.m_end_date_str.c_str(),
            event.m_alt_date_str.c_str());
#endif

        string_vec event_strs;
        event_strs.push_back(std::string(s));
        event_strs[0].erase(event_strs[0].begin(), event_strs[0].begin() + dash_pos + 3);

        string_trim(event_strs[0]);

        while (line_index < lines.size())
        {
            std::string& ns = lines[line_index];
            if (string_begins_with(ns, "**(PUBLIC DOMAIN)** - **"))
                break;

            string_trim(ns);

            line_index++;

            event_strs.push_back(ns);
        }

        for (uint32_t i = 0; i < event_strs.size(); i++)
        {
            if (i != event_strs.size() - 1)
                event_strs[i] += "\n";
        }

#if 0
        uprintf("-----\n");
        for (uint32_t i = 0; i < event_strs.size(); i++)
            uprintf("\"%s\"\n", event_strs[i].c_str());
#endif

        for (uint32_t i = 0; i < event_strs.size(); i++)
            event.m_desc += event_strs[i];

        event.m_refs.push_back("[Anonymous 2023 PDF](https://archive.org/details/anon_pdf_from_markdown)");
        //event.m_type.push_back("event");
        event.m_source = "Anon2023PDF";
        event.m_source_id = "Anon2023PDF" + string_format("_%u", event_id);

        timeline.get_events().push_back(event);

        event_id++;
    }

    if (!timeline.get_events().size())
        panic("Empty timeline)");

    timeline.set_name("Anonymous 2023 PDF");

    return timeline.write_file("anon_pdf.json", true);
}

static void fix_rr0_urls(std::string& str, int year)
{
    if (!str.size())
        return;

    markdown_text_processor tp;
    tp.init_from_markdown(str.c_str());
    if (tp.m_used_unsupported_feature)
        panic("Used a unsupported Markdown feature\n");

    for (uint32_t i = 0; i < tp.m_links.size(); i++)
    {
        std::string& url = tp.m_links[i];

        if (string_begins_with(url, "http"))
            url = "https://web.archive.org/web/2005/" + url;
        else if (url.size() && url[0] == '#')
            url = string_format("https://web.archive.org/web/2005/http://rr0.org/%u.html", year) + url;
        else
            url = "https://web.archive.org/web/2005/http://rr0.org/" + url;
    }

    str.resize(0);
    tp.convert_to_markdown(str, true);
}

static int md_convert(const char* pSrc_filename, int year, ufo_timeline& tm)
{
    string_vec src_file_lines;

    if (!read_text_file(pSrc_filename, src_file_lines, true, nullptr))
        return -1;

    if ((year < 0) || (year > 3000))
        panic("Invalid year\n");

    uprintf("Read file %s\n", pSrc_filename);

    uint32_t cur_line = 0;
    string_vec cur_rec;
    std::vector<string_vec> tran_recs;
    while (cur_line < src_file_lines.size())
    {
        std::string l(src_file_lines[cur_line]);
        string_trim(l);
        if (l.size() >= 2)
        {
            if ((l[0] == '\"') && (l.back() == '\"'))
            {
                l.pop_back();
                l.erase(0, 1);
            }
        }

        l = string_replace(l, "\\[", "[");
        l = string_replace(l, "\\]", "]");

        bool has_dash = ((l.size() > 2) && (l[0] == '-') && (l[1] == ' '));

        if (cur_rec.size() == 0)
        {
            if (has_dash)
            {
                cur_rec.push_back(l);
            }
            else if (l.size())
            {
                panic("Unrecognized text: %s at line %u\n", l.c_str(), cur_line + 1);
            }
        }
        else
        {
            if (has_dash)
            {
                //uprintf("Record:\n");
                //for (uint32_t i = 0; i < cur_rec.size(); i++)
                //    uprintf("%s\n", cur_rec[i].c_str());

                tran_recs.push_back(cur_rec);

                cur_rec.resize(0);
            }

            cur_rec.push_back(l);
        }

        cur_line++;
    }

    if (cur_rec.size())
    {
        //uprintf("Record:\n");
        //for (uint32_t i = 0; i < cur_rec.size(); i++)
        //    uprintf("%s\n", cur_rec[i].c_str());

        tran_recs.push_back(cur_rec);
    }

    uprintf("Found %zu records\n", tran_recs.size());

    for (uint32_t rec_index = 0; rec_index < tran_recs.size(); rec_index++)
    {
        string_vec& rec = tran_recs[rec_index];

        for (auto& str : rec)
            string_trim(str);

        while (rec.size())
        {
            if (rec[0].size())
                break;
            rec.erase(rec.begin());
        }

        while (rec.size())
        {
            if (rec.back().size())
                break;
            rec.erase(rec.begin() + rec.size() - 1);
        }

        if (rec.size())
        {
            if (rec[0][0] != '-')
                panic("Invalid record\n");

            rec[0].erase(0, 1);
            string_trim(rec[0]);
        }
    }

    event_date cur_date;
    cur_date.m_year = year;

    for (uint32_t rec_index = 0; rec_index < tran_recs.size(); rec_index++)
    {
        string_vec& rec = tran_recs[rec_index];

        if (!rec.size())
            continue;

        //uprintf("----------------\n");

        timeline_event timeline_evt;

        std::string& first_line = rec[0];

        if (string_find_first(first_line, "***") >= 0)
            panic("Invalid Markdown\n");

        if (first_line[0] == '-')
            panic("Line has multiple dashes\n");

        std::string event_time;

        if (string_begins_with(first_line, "**"))
        {
            std::string event_date(first_line);
            event_date.erase(0, 2);

            int end_ofs = string_find_first(event_date, "**");
            if (end_ofs < 0)
                panic("Invalid Markdown\n");

            first_line = event_date;
            first_line.erase(0, end_ofs + 2);
            if (first_line.size() && (first_line[0] == ':'))
                first_line.erase(0, 1);
            string_trim(first_line);

            event_date.erase(event_date.begin() + end_ofs, event_date.end());

            if (event_date.size() && (event_date.back() == ':'))
                event_date.pop_back();
            string_trim(event_date);

            event_time = event_date;

            string_vec tokens;
            string_tokenize(event_date, " ", ",:()?:-", tokens);

            if (tokens.size() && (tokens.back() == ":"))
                tokens.pop_back();

            for (auto& str : tokens)
                str = string_lower(str);

            int month_index = -1, month_tok_index = -1, prefix_index = -1;

            for (uint32_t j = 0; j < tokens.size(); j++)
            {
                std::string& str = tokens[j];
                assert(str.size());

                month_index = determine_month(str, false);
                if (month_index >= 0)
                {
                    month_tok_index = j;
                    break;
                }
                else if (str == "mars")
                {
                    month_index = 2;
                    month_tok_index = j;
                    break;
                }
            }

            if (month_index >= 0)
            {
                int day_index = -1;

                if (month_tok_index >= 1)
                {
                    std::string& prefix_str = tokens[month_tok_index - 1];
                    if (isdigit(prefix_str[0]))
                    {
                        day_index = atoi(prefix_str.c_str());
                        if ((day_index < 1) || (day_index > 31))
                            panic("Invalid day\n");
                    }
                }

                if ((day_index < 0) && ((month_tok_index + 1) < static_cast<int>(tokens.size())))
                {
                    std::string& suffix_str = tokens[month_tok_index + 1];
                    if (isdigit(suffix_str[0]))
                    {
                        bool is_time = false;
                        if ((month_tok_index + 2) < static_cast<int>(tokens.size()))
                        {
                            is_time = (tokens[month_tok_index + 2] == ":");
                        }

                        if (!is_time)
                        {
                            day_index = atoi(suffix_str.c_str());
                            if ((day_index < 1) || (day_index > 31))
                                panic("Invalid day\n");
                        }
                    }
                }

                if ((day_index < 0) && (month_index >= 1))
                {
                    if (month_tok_index > 0)
                    {
                        prefix_index = determine_prefix(event_date, true);

                        if (prefix_index < 0)
                            panic("Invalid date");
                    }
                }

                cur_date.m_month = month_index + 1;
                cur_date.m_prefix = (date_prefix_t)prefix_index;

                if (day_index != -1)
                    cur_date.m_day = day_index;
                else
                    cur_date.m_day = -1;
            }
            else
            {
                for (uint32_t j = 0; j < tokens.size(); j++)
                {
                    std::string& str = tokens[j];
                    assert(str.size());

                    prefix_index = determine_prefix(str, false);
                    if (prefix_index >= 0)
                        break;
                }

                if (prefix_index == -1)
                {
                    if (tokens.size() == 1)
                    {
                        if ((string_ends_in(tokens[0], "am")) || (string_ends_in(tokens[0], "pm")) ||
                            (tokens[0] == "evening") || (tokens[0] == "morning") || (tokens[0] == "night") || (tokens[0] == "afternoon"))
                        {
                        }
                        else
                        {
                            panic("Unrecognized date");
                        }
                    }
                }
                else
                {
                    cur_date.m_day = -1;
                    cur_date.m_month = -1;
                    cur_date.m_prefix = (date_prefix_t)prefix_index;
                }
            }

            //uprintf("%s: %i/%i/%i prefix: %i\n", event_date.c_str(), cur_date.m_month, cur_date.m_day, cur_date.m_year, cur_date.m_prefix);
        }
        else
        {
            //uprintf("%i/%i/%i prefix: %i\n", cur_date.m_month, cur_date.m_day, cur_date.m_year, cur_date.m_prefix);
        }

        if (!rec[0].size())
            rec.erase(rec.begin());

        if (rec.size() && rec[0].size())
        {
            if (rec[0][0] == ':')
            {
                rec[0].erase(0, 1);
                string_trim(rec[0]);
            }
        }

        if (rec.size())
        {
            std::string rec_text("(Translated from French)");

            for (uint32_t i = 0; i < rec.size(); i++)
            {
                if (rec_text.size())
                {
                    if (rec_text.back() != '-')
                        rec_text.push_back(' ');
                }

                rec_text += rec[i];
            }

            const char* pSource_str = "{.source}";
            const uint32_t source_len = (uint32_t)strlen(pSource_str);

            for (; ; )
            {
                int source_ofs = string_find_first(rec_text.c_str(), pSource_str);
                if (source_ofs < 0)
                    break;

                int start_ofs = source_ofs - 1;
                if (start_ofs < 0)
                    panic("Invalid .source\n");

                if (rec_text[start_ofs] != ']')
                    panic("Invalid .source\n");

                start_ofs--;
                if (start_ofs < 0)
                    panic("Invalid .source\n");

                int in_bracket_counter = 1;
                while ((start_ofs >= 0) && (in_bracket_counter > 0))
                {
                    if (rec_text[start_ofs] == ']')
                        in_bracket_counter++;
                    else if (rec_text[start_ofs] == '[')
                        in_bracket_counter--;

                    start_ofs--;
                }

                if (in_bracket_counter != 0)
                    panic("Invalid .source\n");

                int s = start_ofs + 1;
                int e = source_ofs + (int)source_len;
                int l = e - s;

                std::string ref(string_slice(rec_text, s, l));

                if ((e < static_cast<int>(rec_text.size())) && ((rec_text[e] == '.') || (rec_text[e] == ']')))
                {
                    while (s > 0)
                    {
                        if ((rec_text[s - 1] != ' ') && (rec_text[s - 1] != '['))
                            break;
                        s--;
                        l++;
                    }
                }

                if ((e < static_cast<int>(rec_text.size())) && (rec_text[e] == ']'))
                {
                    e++;
                    l++;
                }

                rec_text.erase(s, l);

                ref.erase(ref.size() - source_len, source_len);

                bool removed_back_bracket = false;
                while (ref.size() && (ref.back() == ']'))
                {
                    ref.pop_back();
                    removed_back_bracket = true;
                }

                bool removed_front_bracket = false;
                while (ref.size() && (ref[0] == '['))
                {
                    ref.erase(0, 1);
                    removed_front_bracket = true;
                }

                if (removed_front_bracket)
                {
                    for (uint32_t i = 0; i < ref.size(); i++)
                    {
                        if (ref[i] == '[')
                            break;

                        if (ref[i] == ']')
                        {
                            ref = "[" + ref;
                            break;
                        }
                    }
                }

                if (removed_back_bracket)
                {
                    for (int i = (int)ref.size() - 1; i >= 0; --i)
                    {
                        if (ref[i] == '[')
                        {
                            ref += "]";
                            break;
                        }

                        if (ref[i] == ']')
                            break;
                    }
                }

                fix_rr0_urls(ref, year);

                //uprintf("Ref: \"%s\"\n", ref.c_str());

                timeline_evt.m_refs.push_back(ref);
            }

            //rec_text = string_replace(rec_text, "[]", "");
            if (string_ends_in(rec_text, "["))
                rec_text.pop_back();

            string_trim(rec_text);

            fix_rr0_urls(rec_text, year);

            if (event_time.size())
                rec_text += " (" + event_time + ")";

            //uprintf("%s\n", rec_text.c_str());
            if (!rec_text.size())
                panic("No description\n");

            timeline_evt.m_desc = rec_text;
            timeline_evt.m_begin_date = cur_date;
            timeline_evt.m_date_str = cur_date.get_string();
            timeline_evt.m_source = "rr0";
            timeline_evt.m_source_id = string_format("rr0_%u", (uint32_t)tm.get_events().size());
            timeline_evt.m_refs.push_back(string_format(u8"[rr0 - Beau](https://web.archive.org/web/20040704025950/http://www.rr0.org/%u.html)", year));

            tm.get_events().push_back(timeline_evt);
        }

    } // rec_index

    tm.set_name("rr0");

    uprintf("Success\n");

    return 1;
}

const uint32_t NUM_EXPECTED_RR0_YEARS = 248;

bool convert_rr0()
{
    uint32_t total_years = 0;
    ufo_timeline tm;
    for (int year = 0; year <= 2002; year++)
    {
        std::string filename = string_format("rr0/tran%u.md", year);

        int status = md_convert(filename.c_str(), year, tm);
        if (status == -1)
        {
            //uprintf("Skipping year - not found\n");
            continue;
        }

        if (status == 0)
            panic("md_convert() failed!\n");

        total_years++;
    }

    tm.write_file("rr0.json");

    uprintf("Processed %u years\n", total_years);

    return total_years >= NUM_EXPECTED_RR0_YEARS;
}

static bool overmeire_convert(const std::string& in_filename, ufo_timeline& tm)
{
    string_vec src_file_lines;

    if (!read_text_file(in_filename.c_str(), src_file_lines, true, nullptr))
        panic("Failed reading source file %s\n", in_filename.c_str());

    uprintf("----------------------- Read file %s\n", in_filename.c_str());

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
                tran_recs.push_back(cur_rec);

                cur_rec.resize(0);
            }

            cur_rec.push_back(src_file_lines[cur_line]);
        }

        cur_line++;
    }

    if (cur_rec.size())
        tran_recs.push_back(cur_rec);

    uprintf("Found %zu records\n", tran_recs.size());

    int prev_year = -1;
    for (uint32_t rec_index = 0; rec_index < tran_recs.size(); rec_index++)
    {
        const string_vec& strs = tran_recs[rec_index];
        if (strs.size() < 3)
            panic("Invalid record\n");

        string_vec tokens;
        string_tokenize(strs[0], " ", "#,:()?:-", tokens);

        if (tokens.size() && tokens.front() == "#")
            tokens.erase(tokens.begin());

        if (tokens.size() && (tokens.back() == ":"))
            tokens.pop_back();

        for (auto& str : tokens)
            str = string_lower(str);

        int year = -1, year_tok_index = -1;
        for (year_tok_index = 0; year_tok_index < static_cast<int>(tokens.size()); year_tok_index++)
        {
            int y = atoi(tokens[year_tok_index].c_str());
            if ((y > 0) && (y >= first_year) && (y <= last_year))
            {
                year = y;
                break;
            }
        }
        if (year < 0)
            panic("Can't find year\n");

        if (year < prev_year)
            panic("Year out of order\n");

        const bool plural = string_ends_in(tokens[year_tok_index].c_str(), "s");

        tokens.erase(tokens.begin() + year_tok_index);

        int month_index = -1, month_tok_index = -1, prefix_index = -1;

        for (uint32_t j = 0; j < tokens.size(); j++)
        {
            std::string& str = tokens[j];
            assert(str.size());

            month_index = determine_month(str, false);
            if (month_index >= 0)
            {
                month_tok_index = j;
                break;
            }
            else if (str == "mars")
            {
                month_index = 2;
                month_tok_index = j;
                break;
            }
        }

        event_date cur_date;
        cur_date.m_year = year;
        cur_date.m_plural = plural;

        int day_index = -1;

        if (month_index >= 0)
        {
            cur_date.m_month = month_index + 1;

            if (month_tok_index >= 1)
            {
                std::string& prefix_str = tokens[month_tok_index - 1];
                if (isdigit(prefix_str[0]))
                {
                    day_index = atoi(prefix_str.c_str());
                    if ((day_index < 1) || (day_index > 31))
                        panic("Invalid day\n");
                }
            }

            if ((day_index < 0) &&
                ((month_tok_index + 1) < static_cast<int>(tokens.size())))
            {
                std::string& suffix_str = tokens[month_tok_index + 1];
                if (isdigit(suffix_str[0]))
                {
                    bool is_time = false;
                    if ((month_tok_index + 2) < static_cast<int>(tokens.size()))
                    {
                        is_time = (tokens[month_tok_index + 2] == ":");
                    }

                    if (!is_time)
                    {
                        day_index = atoi(suffix_str.c_str());
                        if ((day_index < 1) || (day_index > 31))
                            panic("Invalid day\n");
                    }
                }
            }

            if (day_index != -1)
                cur_date.m_day = day_index;
        }

        if (day_index < 0)
        {
            uint32_t i;
            for (i = 0; i < NUM_DATE_PREFIX_STRINGS; i++)
            {
                if (string_ifind_first(strs[0], g_date_prefix_strings[i]) >= 0)
                {
                    prefix_index = i;
                    break;
                }
            }

            if (prefix_index < 0)
            {
                if (string_ifind_first(strs[0], "beginning of") >= 0)
                    prefix_index = cEarly;
            }
        }

        if ((string_ifind_first(strs[0], "approx") >= 0) ||
            (string_ifind_first(strs[0], "around") >= 0) ||
            (string_ifind_first(strs[0], "inaccurate") >= 0) ||
            (string_ifind_first(strs[0], "unknown") >= 0) ||
            (string_ifind_first(strs[0], "no precise") >= 0) ||
            (string_ifind_first(strs[0], "unspecified") >= 0) ||
            (string_ifind_first(strs[0], "supposed") >= 0) ||
            (strs[0].find_first_of('?') != std::string::npos))
            cur_date.m_approx = true;

        cur_date.m_prefix = (date_prefix_t)prefix_index;

        //printf("%s = %s\n", strs[0].c_str(), cur_date.get_string().c_str());

        if (strs.size() < 3)
            panic("Invalid record\n");

        std::string rec_text;

        std::string loc_text(strs[1]);
        if (loc_text.size() && (loc_text.back() == ';'))
            loc_text.pop_back();

        for (uint32_t i = 2; i < strs.size(); i++)
        {
            if (rec_text.size())
            {
                if (rec_text.back() != '-')
                    rec_text.push_back(' ');
            }

            rec_text += strs[i];
        }

        string_trim(rec_text);

        if (string_ends_in(rec_text, ")."))
        {
            //uprintf("--- %s\n", rec_text.c_str());
            rec_text.pop_back();
        }

        if (!rec_text.size())
            panic("No description");

        string_vec refs;
        string_vec notes;
        while (rec_text.size() && rec_text.back() == ')')
        {
            int in_bracket_counter = 1;
            int start_ofs = (int)rec_text.size() - 2;
            while ((start_ofs >= 0) && (in_bracket_counter > 0))
            {
                if (rec_text[start_ofs] == ')')
                    in_bracket_counter++;
                else if (rec_text[start_ofs] == '(')
                    in_bracket_counter--;

                start_ofs--;
            }

            if (in_bracket_counter != 0)
                panic("Invalid ref\n");

            std::string ref_str(string_slice(rec_text, start_ofs + 1));
            if (ref_str == "(...)")
                break;
            ref_str.erase(0, 1);
            ref_str.pop_back();
            if ( (string_begins_with(ref_str, "note from vog")) ||
                 (string_begins_with(ref_str, "note of vog")) ||
                 (string_begins_with(ref_str, "vog:")) )
                notes.push_back(ref_str);
            else
                refs.push_back(ref_str);

            rec_text = string_slice(rec_text, 0, start_ofs + 1);
            string_trim(rec_text);
        }

        for (auto& str : notes)
            rec_text += " [" + str + "]";

        if (!rec_text.size())
            panic("No description");

        if (!cur_date.sanity_check())
            panic("Invalid date");

        timeline_event evt;
        evt.m_begin_date = cur_date;
        evt.m_date_str = cur_date.get_string();
        evt.m_locations.push_back(loc_text);
        evt.m_desc = "(Translated from French) " + rec_text;
        evt.m_refs = refs;
        evt.m_source = "Overmeire";
        evt.m_source_id = string_format("Overmeire_%zu", tm.get_events().size());
        evt.m_refs.push_back("[_Mini catalogue chronologique des observations OVNI_, by Godelieve Van Overmeire](https://web.archive.org/web/20060107070423/http://users.skynet.be/sky84985/chrono.html)");

        std::string trial_date(string_format("#%u", year));
        if (cur_date.m_month >= 1)
        {
            trial_date += string_format(", %s", g_full_months[cur_date.m_month - 1]);
            if (cur_date.m_day >= 1)
                trial_date += string_format(" %u", cur_date.m_day);
        }
        if (trial_date != strs[0])
            evt.m_desc += " (" + string_slice(strs[0], 1) + ")";

        tm.get_events().push_back(evt);

        prev_year = year;
    }

    tm.set_name("Overmeire");

    return true;
}

bool convert_overmeire()
{
    ufo_timeline tm;
    for (int index = 1; index <= 28; index++)
    {
        std::string in_filename = string_format("overmeiere/tran_chron%i.txt", index);

        if (!does_file_exist(in_filename.c_str()))
        {
            uprintf("Skipping file %s - doesn't exists\n", in_filename.c_str());
            continue;
        }

        if (!overmeire_convert(in_filename, tm))
            panic("overmeire_convert() failed!\n");

    }
    tm.write_file("overmeire.json");
    return true;
}