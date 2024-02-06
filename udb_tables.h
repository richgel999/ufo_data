// udb_tables.h
// Some portions of this specific file (get_hatch_geo, g_hatch_continents) use strings from
// the "uDb" project by Jérôme Beau, available on github here: https://github.com/RR0/uDb
#pragma once

struct hatch_ref
{
    uint32_t m_ref;
    const char* m_pDesc;
};

static const char* g_hatch_locales[]
{
    "Metropolis",
    "Residential",
    "Town & City",
    "Farmlands",

    "Pasture",      //4
    "Oil & Coal",
    "Tundra",
    "Desert",

    "Mountains",    //8
    "Wetlands",
    "Forest",
    "Rainforest",

    "Coastlands",   //12
    "Offshore",
    "High Seas",
    "Islands",

    "In-flight",    //16
    "Space",
    "Military Base",
    "Unknown",

    "Road + Rails"  //20
};

static const char* g_hatch_continents[]
{
    "North America (Including Central America)",
    "South America",
    "Australia/New Zealand/Great Oceans",
    "Western Europe",

    "Eastern Europe",
    "Asia Mainland (except Vietnam/Cambodia/Laos)",
    "Asia Pacific (except Vietnam/Cambodia/Laos)",
    "Northern and Northwest Africa",

    "Generally on or South of the Equator",
    "Russia and Former Soviet Union (Except Baltics/Ukraine/Belorus)",
    "Middle East (Turkey/Israel/Iran/Arabic speaking lands)",
    "Space"
};

struct hatch_state
{
    const char* m_pCode;
    const char* m_pFull = nullptr;
};

static void get_hatch_geo(uint32_t cont_code, uint32_t country_code, const std::string& state_or_prov,
    std::string& country_name,
    std::string& state_or_prov_name)
{
    country_name = "?";
    state_or_prov_name = state_or_prov;

    switch (cont_code)
    {
    case 0: // North America
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Canada";

            static const hatch_state s_ca_states[] =
            {
                { "ALB", u8"Alberta" }, { "ALT", u8"Alta" }, { "BCO", u8"British Columbia"}, { "MBA", u8"Manitoba" },
                { "NBR", u8"New Brunswick" }, { "NFL", u8"Newfoundland"}, { "NSC", u8"Nova Scotia"}, { "NWT", u8"Northwest Territories" },
                { "ONT", u8"Ontario" }, { "QBC", u8"Quebec"}, { "SSK", u8"Sasketchewan"}, { "YUK", u8"Yukon"}, { "PEI", u8"Prince Edward Island" }
            };

            for (const auto& x : s_ca_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 2:
        {
            country_name = u8"USA";

            static const hatch_state s_us_states[] =
            {
                { "ALA", u8"Alabama" }, { "ALB", u8"Alabama" }, { "ALS", u8"Alaska" }, { "ARK", u8"Arkansas" },
                { "ARZ", u8"Arizona" }, { "CLR", u8"Colorado" }, { "CNN", u8"Connecticut" }, { "DLW", u8"Delaware" },
                { "FLR", u8"Florida" }, { "GRG", u8"Georgia" }, { "HWI", u8"Hawaii" }, { "IOW", u8"Iowa" },
                { "KNT", u8"Kentucky" }, { "MNE", u8"Maine" }, { "MNT", u8"Montana" }, { "MSC", u8"Massachusetts" },
                { "MSO", u8"Missouri" }, { "MSP", u8"Mississippi" }, { "NBR", u8"Nebraska" }, { "NCR", "North Carolina" },
                { "NDK", u8"North Dakota" }, { "OHI", u8"Ohio" }, { "ORE", u8"Oregon" }, { "SCR", u8"South Carolina" },
                { "SDK", u8"South Dakota" }, { "UTA", u8"Utah" }, { "WVA", u8"Western Virginia" }, { "WSH", u8"Washington" },
                { "NMX", u8"New Mexico" }, { "MNS", u8"Minnesota" }, { "WSC", u8"Wisconsin" }, { "PNS", u8"Pennsylvania" },
                { "NJR", u8"New Jersey" }, { "ME," u8"Maine" }, { "PRC", u8"Puerto Rico" }, { "TNS", u8"Tennessee" },
                { "WYO", u8"Wyoming" }, { "CNC", u8"Connecticut" }, { "LSN", u8"Louisiana" }, { "RHD", u8"Rhode Island" },
                { "CLF", u8"California" }, { "NVD", u8"Nevada" }, { "KNS", u8"Kansas" }, { "ILN", u8"Illinois" },
                { "IND", u8"Indiana" }, { "TXS", u8"Texas" }, { "MCH", u8"Michigan" }, { "MLD", u8"Maryland" },
                { "KNY", u8"Kentucky" }, { "IDH", u8"Idaho" }, { "NYK", u8"New York" }, { "NHM", u8"New Hampshire" },
                { "OKL", u8"Oklahoma" }, { "VRG", u8"Virginia" }, { "VRM", u8"Vermont" }
            };

            for (const auto& x : s_us_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 3:
        {
            country_name = u8"Mexico";

            static const hatch_state s_us_states[] =
            {
                { "SNL", u8"Sinaloa"} , { "CHH", u8"Chihuahua" }, { "BCN", u8"Baja California" }, { "SNR", u8"Sonora" }
            };

            for (const auto& x : s_us_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 4:
        {
            country_name = u8"Guatemala";
            break;
        }
        case 5:
        {
            country_name = u8"Belize";
            break;
        }
        case 6:
        {
            country_name = u8"Honduras";
            break;
        }
        case 7:
        {
            country_name = u8"El Salvador";
            break;
        }
        case 8:
        {
            country_name = u8"Nicaragua";
            break;
        }
        case 9:
        {
            country_name = u8"Costa Rica";
            break;
        }
        case 10:
        {
            country_name = u8"Panama";

            if (state_or_prov == "CNZ")
                state_or_prov_name = u8"Canal Zone";

            break;
        }
        }

        break;
    }
    case 1: // South America
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Brazil";

            static const hatch_state s_brazil_states[] =
            {
                { "AMZ", u8"Amazonas" }, { "BAH", u8"Bahia" }, { "MG", u8"Minas Gerais" }, { "RIO", u8"Rio" }, { "SPL", u8"São Paulo" }
            };

            for (const auto& x : s_brazil_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 2:
        {
            country_name = u8"Paraguay";
            break;
        }
        case 3:
        {
            country_name = u8"Uruguay";
            break;
        }
        case 4:
        {
            country_name = u8"Argentina";

            if (state_or_prov == "BNA")
                state_or_prov_name = u8"Buenos Aires";
            else if (state_or_prov == "JJY")
                state_or_prov_name = u8"Juhuy";

            break;
        }
        case 5:
        {
            country_name = u8"Chile";

            if (state_or_prov == "ANT")
                state_or_prov_name = u8"Antofagasta";
            else if (state_or_prov == "ATC")
                state_or_prov_name = u8"Atacama";

            break;
        }
        case 6:
        {
            country_name = u8"Bolivia";
            break;
        }
        case 7:
        {
            country_name = u8"Peru";

            if (state_or_prov == "ARQ")
                state_or_prov_name = u8"Arequipa";

            break;
        }
        case 8:
        {
            country_name = u8"Ecuador";
            break;
        }
        case 9:
        {
            country_name = u8"Colombia";
            if (state_or_prov == "BGT")
                state_or_prov_name = u8"Bogota";

            break;
        }
        case 10:
        {
            country_name = u8"Venezuela";
            break;
        }
        case 11:
        {
            country_name = u8"Guyanas (all 3 of them)";
            if (state_or_prov == "SRN")
                state_or_prov_name = u8"Surinam";
            break;
        }
        }

        break;
    }
    case 2: // Australia/New Zealand/Great Oceans
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Australia";

            static const hatch_state s_aus_states[] =
            {
                { "ACT", u8"Australian Capital Territory" }, { "VCT", u8"Victoria" }, { "NTR", u8"Northern Territory" }, { "QLD", u8"Queensland" },
                { "SAU", u8"South Australia" }, { "WAU", u8"Western Australia" }
            };

            for (const auto& x : s_aus_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 2:
        {
            country_name = u8"New Zealand";
            break;
        }
        case 3:
        {
            country_name = u8"Atlantic Ocean + islands";

            static const hatch_state s_at_states[] = { { "AZR", u8"Azores" }, { "BAH", u8"Bahamas" }, { "BRM", u8"Bermuda" } };

            for (const auto& x : s_at_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 4:
        {
            country_name = u8"Pacific Ocean and non - Asian islands";
            break;
        }
        case 5:
        {
            country_name = u8"Caribbean area";
            break;
        }
        case 6:
        {
            country_name = u8"Indian Ocean + islands";
            break;
        }
        case 7:
        {
            country_name = u8"Arctic above 70 degrees North";
            break;
        }
        case 8:
        {
            country_name = u8"Antarctic below 70 degrees South";
            if (state_or_prov == "VST")
                state_or_prov_name = u8"Vostok";
            break;
        }
        case 9:
        {
            country_name = u8"Iceland";
            break;
        }
        case 10:
        {
            country_name = u8"Greenland";
            break;
        }
        }

        break;
    }
    case 3: // Western Europe
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Great Britain and Ireland";

            static const hatch_state s_bri_states[] =
            {
                { "IRL", u8"Ireland" }, { "ENG", u8"England" }, { "SCT", u8"Scotland" }, { "NI", u8"Northern Ireland" }
            };

            for (const auto& x : s_bri_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 2:
        {
            country_name = u8"Scandanavian and Finland";

            static const hatch_state s_scan_states[] =
            {
                { "NRW", u8"Norway" }, { "FNL", u8"Finland" }, { "SWD", u8"Sweden" }, { "DMK", u8"Danemark" }
            };

            for (const auto& x : s_scan_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 3:
        {
            country_name = u8"Germany";

            static const hatch_state s_ger_states[] =
            {
                { "BDW", u8"Bade-Wurtemberg" }, { "BVR", u8"Bavaria" }, { "SXN", u8"Saxony" }, { "DMK", u8"Vienna" }
            };

            for (const auto& x : s_ger_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 4:
        {
            country_name = u8"Belgium, Netherlandsand Luxembourg";

            if (state_or_prov == "BLG")
                state_or_prov_name = u8"Belgium";
            else if (state_or_prov == "NTH")
                state_or_prov_name = u8"Netherlands";

            break;
        }
        case 5:
        {
            country_name = u8"France";

            static const hatch_state s_france_states[] =
            {
                { "ARG", u8"Ariège" }, { "AUB", u8"Aube" }, { "AUD", u8"Aude" }, { "AIN", u8"Ain" },
                { "ALR", u8"Allier" }, { "AND", u8"Andorre" }, { "AVR", u8"Aveyron" }, { "BLF", u8"Territoire de Belfort" },
                { "BRH", u8"Bas-Rhin" }, { "SMR", u8"Seine Maritime" }, { "ADC", u8"Ardèche" }, { "ASN", u8"Aisne" },
                { "ADN", u8"Ardennes" }, { "AMR", u8"Alpes Maritimes" }, { "AHP", u8"Alpes de Haute Provence" }, { "BDR", u8"Bouches-du-Rhône" },
                { "CDO", u8"Côte-d\'Or" }, { "CHM", u8"Charente-Maritime" }, { "CHN", u8"Charente" }, { "CHR", u8"Cher" },
                { "CLV", u8"Calvados" }, { "CNT", u8"Cantal" }, { "CRS", u8"Creuse" }, { "DBS", u8"Doubs" },
                { "DRD", u8"Dordogne" }, { "DRM", u8"Drôme" }, { "DSV", u8"Deux-Sèvres" }, { "ESN", u8"Essonne" },
                { "E&L", u8"Eure-et-Loir" }, { "FNS", u8"Finistère" }, { "FRB", u8"Bretagne" }, { "GRD", u8"Gard" },
                { "GRN", u8"Gironde" }, { "GRS", u8"Gers" }, { "M&M", u8"Meurthe-et-Moselle" }, { "HAL", u8"Hautes Alpes" },
                { "HCS", u8"Haute Corse" }, { "HGR", u8"Haute Garonne" }, { "HLR", u8"Haute-Loire" }, { "HPY", u8"Hautes-Pyrénées" },
                { "HRL", u8"Hérault" }, { "HRH", u8"Haut-Rhin" }, { "HSA", u8"Haute-Saône" }, { "HVN", u8"Haute-Vienne" },
                { "I&L", u8"Indre-et-Loire" }, { "I&V", u8"Ille-et-Vilaine" }, { "INR", u8"Indre" }, { "ISR", u8"Isère" },
                { "JRA", u8"Jura" }, { "L&C", u8"Loir-et-Cher" }, { "L&G", u8"Lot-et-Garonne" }, { "LOI", u8"Loire" },
                { "LRE", u8"Loire" }, { "LRT", u8"Loiret" }, { "LOT", u8"Lot" }, { "LRA", u8"Loire Atlantique" },
                { "LND", u8"Landes" }, { "LZR", u8"Lozère" }, { "PRS", u8"Paris" }, { "M&L", u8"Maine-et-Loire" },
                { "MNC", u8"Manche" }, { "MRB", u8"Morbihan" }, { "MRN", u8"Marne" }, { "MSE", u8"Meuse" },
                { "MSL", u8"Moselle" }, { "NRD", u8"Nord" }, { "NVR", u8"Nièvre" }, { "OIS", u8"Oise" },
                { "PDC", u8"Pas-de-Calais" }, { "PDD", u8"Puy-de-Dôme" }, { "PYO", u8"Pyrénées-Orientales" }, { "RHN", u8"Rhône" },
                { "S&L", u8"Saône-et-Loire" }, { "S&M", u8"Seine-et-Marne" }, { "SMM", u8"Somme" }, { "T&G", u8"Tarn-et-Garonne" },
                { "TRN", u8"Tarn" }, { "VAR", u8"Var" }, { "VCL", u8"Vaucluse" }, { "VDM", u8"Val-de-Marne" },
                { "VNN", u8"Vienne" }, { "VND", u8"Vendée" }, { "VSG", u8"Vosges" }, { "YVL", u8"Yvelines" }
            };

            for (const auto& x : s_france_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 6:
        {
            country_name = u8"Spain";

            static const hatch_state s_spain_states[] =
            {
                { "ALB", u8"Albacete" }, { "BDJ", u8"Badajoz" }, { "BRC", u8"Barcelone" }, { "BRG", u8"Burgos" },
                { "CNC", u8"Cuenca" }, { "GRN", u8"Grenada" }, { "VLN", u8"Valencian" },
            };

            for (const auto& x : s_spain_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 7:
        {
            country_name = u8"Portugal";

            static const hatch_state s_po_states[] =
            {
                { "ALG", u8"Algarve" }, { "BRL", u8"Beira littoral" }, { "DRO", u8"Douro" },
            };

            for (const auto& x : s_po_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 8:
        {
            country_name = u8"Austria";
            if (state_or_prov == "UAU")
                state_or_prov_name = u8"Upper Austria";

            break;
        }
        case 9:
        {
            country_name = u8"Italy";

            static const hatch_state s_it_states[] =
            {
                { "AL", u8"Alessandria" }, { "AQ", u8"Aquila" }, { "ASC", u8"Ascoli Piceno" }, { "AN", u8"Ancona" },
                { "BS", u8"Brescia" }, { "BA", u8"Bari" }, { "BG", u8"Bergamo" }, { "BO", u8"Bologna" },
                { "CA", u8"Cagliari" }, { "CMP", u8"Campania" }, { "CUN", u8"Cuneo" }, { "GR", u8"Grosseto" },
                { "FI", u8"Firenze" }, { "MI", u8"Milano" }, { "LMB", u8"Lombardy" }, { "LU", u8"Lucques" },
                { "PDA", u8"Padua" }, { "PDM", u8"Piedmont" }, { "SGV", u8"Segovia" }, { "TO", u8"Torino" },
                { "VA", u8"Varese" }, { "RG", u8"Ragusa" }, { "RM", u8"Rome" },
            };

            for (const auto& x : s_it_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 10:
        {
            country_name = u8"Switzerland";

            static const hatch_state s_sw_states[] = { { "VAU", u8"Vaud" }, { "BRN", u8"Berne" }, { "BSL", u8"Basel" } };

            for (const auto& x : s_sw_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 11:
        {
            country_name = u8"Greece and Island nations";
            break;
        }
        }

        break;
    }
    case 4: // Eastern Europe
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = "Poland";

            if (state_or_prov == "WRS")
                state_or_prov_name = u8"Warsaw";
            else if (state_or_prov == "KRK")
                state_or_prov_name = u8"Krakow";

            break;
        }
        case 2:
        {
            country_name = u8"Czech and Slovak Republics";
            break;
        }
        case 3:
        {
            country_name = u8"Hungary";
            break;
        }
        case 4:
        {
            country_name = u8"Former Yugoslavia";
            break;
        }
        case 5:
        {
            country_name = u8"Romania";

            static const hatch_state s_ro_states[] =
            {
                { "BCU", u8"Bacău" }, { "BHR", u8"Bihor" }, { "BSV", u8"Brasov" }, { "BUC", u8"Bucharest" }, { "BCH", u8"Bucharest" }, { "CNS", u8"Constanța" },
            };

            for (const auto& x : s_ro_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 6:
        {
            country_name = u8"Bulgaria";

            if (state_or_prov == "SOF")
                state_or_prov_name = u8"Sofia";

            break;
        }
        case 7:
        {
            country_name = u8"Albania";
            break;
        }
        case 8:
        {
            country_name = u8"Estonia, Latvia& Lithuania";

            if (state_or_prov == "LTH")
                state_or_prov_name = u8"Lithuania";
            else if (state_or_prov == "LTV")
                state_or_prov_name = u8"Latvia";

            break;
        }
        case 9:
        {
            country_name = u8"Belorus";
            break;
        }
        case 10:
        {
            country_name = u8"Ukraine";
            break;
        }
        }

        break;
    }
    case 5: // Asia Mainland (except Vietnam/Cambodia/Laos)
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"China";

            static const hatch_state s_china_states[] =
            {
                { "ANH", u8"Anhui" }, { "BEI", u8"Beijing" }, { "JNS", u8"Jiangsu" }, { "JNX", u8"Jianxi" }, { "SHD", u8"Shandong" }
            };

            for (const auto& x : s_china_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 2:
        {
            country_name = u8"Mongolia";
            break;
        }
        case 3:
        {
            country_name = u8"India";
            if (state_or_prov == "MHR")
                state_or_prov_name = u8"Maharashtra";
            break;
        }
        case 4:
        {
            country_name = u8"Pakistan";
            break;
        }
        case 5:
        {
            country_name = u8"Afghanistan";
            if (state_or_prov == "GHZ")
                state_or_prov_name = u8"Ghazni";
            break;
        }
        case 6:
        {
            country_name = u8"Himalayan states";
            break;
        }
        case 7:
        {
            country_name = u8"Bangladesh";
            break;
        }
        case 8:
        {
            country_name = u8"Burma";
            break;
        }
        case 9:
        {
            country_name = u8"Korea(both sides)";
            break;
        }
        }

        break;
    }
    case 6: // Asia Pacific
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Japan";
            if (state_or_prov == "KYU")
                state_or_prov_name = u8"Kyushu";
            else if (state_or_prov == "HNS")
                state_or_prov_name = u8"Honshu Island";

            break;
        }
        case 2:
        {
            country_name = u8"Philippines";
            break;
        }
        case 3:
        {
            country_name = u8"Taiwan China";
            break;
        }
        case 4:
        {
            country_name = u8"Vietnam";
            break;
        }
        case 5:
        {
            country_name = u8"Laos";
            break;
        }
        case 6:
        {
            country_name = u8"Cambodia";
            break;
        }
        case 7:
        {
            country_name = u8"Thailand";
            break;
        }
        case 8:
        {
            country_name = u8"Malaysia";
            break;
        }
        case 9:
        {
            country_name = u8"Indonesia";
            break;
        }
        }

        break;
    }
    case 7: // Northern and Northwest Africa
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Egypt";
            break;
        }
        case 2:
        {
            country_name = u8"Sudan";
            break;
        }
        case 3:
        {
            country_name = u8"Ethiopia";
            break;
        }
        case 4:
        {
            country_name = u8"Libya";
            break;
        }
        case 5:
        {
            country_name = u8"Tunisia";
            break;
        }
        case 6:
        {
            country_name = u8"Algeria";

            static const hatch_state s_alger_states[] =
            {
                { "ALG", u8"Alger" }, { "ORN", u8"Oran" }, { "LAM", u8"Lamoriciere" }, { "BOU", u8"Boukanefis" },
                { "MOS", u8"Mostaganem" }, { "CNS", u8"Constantine" }, { "BCH", u8"Bechar" }, { "ANB", u8"Annaba", }
            };

            for (const auto& x : s_alger_states)
                if (state_or_prov == x.m_pCode)
                    state_or_prov_name = x.m_pFull;

            break;
        }
        case 7:
        {
            country_name = u8"Morocco";
            if (state_or_prov == "AGD")
                state_or_prov_name = u8"Agadir";
            break;
        }
        case 8:
        {
            country_name = u8"Sahara";
            break;
        }
        case 9:
        {
            country_name = u8"Ivory Coast, Ghana, Togo, Benin, Liberia";
            break;
        }
        case 10:
        {
            country_name = u8"Nigeria";
            break;
        }
        }

        break;
    }
    case 8: // Generally on or South of the Equator
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Republic of South Africa";
            if (state_or_prov == "NTL")
                state_or_prov_name = u8"KwaZulu-Natal";
            else if (state_or_prov == "OFS")
                state_or_prov_name = u8"Orange Free State";
            break;
        }
        case 2:
        {
            country_name = u8"Zimbabwe & Zambia";
            break;
        }
        case 3:
        {
            country_name = u8"Angola";
            break;
        }
        case 4:
        {
            country_name = u8"Kalahari Desert";
            break;
        }
        case 5:
        {
            country_name = u8"Mozambique";
            break;
        }
        case 6:
        {
            country_name = u8"Tanzania";
            break;
        }
        case 7:
        {
            country_name = u8"Uganda";
            break;
        }
        case 8:
        {
            country_name = u8"Kenya";
            break;
        }
        case 9:
        {
            country_name = u8"Somalia";
            break;
        }
        case 10:
        {
            country_name = u8"Congo states";
            break;
        }
        case 11:
        {
            country_name = u8"Ivory Coast,Ghana,Togo,Benin,Liberia etc.";
            break;
        }
        case 12:
        {
            country_name = u8"Nigeria";
            break;
        }
        }

        break;
    }
    case 9: // Russia and Former Soviet Union (Except Baltics/Ukraine/Belorus)
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Russia";
            if (state_or_prov == "LEN")
                state_or_prov_name = u8"Leningrad";
            break;
        }
        case 2:
        {
            country_name = u8"Georgia";
            if (state_or_prov == "ABK")
                state_or_prov_name = u8"Abkhazia";
            break;
        }
        case 3:
        {
            country_name = u8"Armenia";
            break;
        }
        case 4:
        {
            country_name = u8"Azerbaijan";
            break;
        }
        case 5:
        {
            country_name = u8"Kazakh Republic";
            if (state_or_prov == "ALM")
                state_or_prov_name = u8"Alma";
            break;
        }
        case 6:
        {
            country_name = u8"Turkmen Republic";
            break;
        }
        case 7:
        {
            country_name = u8"Uzbek Republic";
            break;
        }
        case 8:
        {
            country_name = u8"Tadzhik Republic";
            break;
        }
        }

        break;
    }
    case 10: // Middle East (Turkey/Israel/Iran/Arabic speaking lands)
    {
        switch (country_code)
        {
        case 1:
        {
            country_name = u8"Turkey";
            if (state_or_prov == "ANK")
                state_or_prov_name = "Ankara";
            break;
        }
        case 2:
        {
            country_name = u8"Syria";
            break;
        }
        case 3:
        {
            country_name = u8"Iraq";
            if (state_or_prov == "CHL")
                state_or_prov_name = "Chaldea";
            break;
        }
        case 4:
        {
            country_name = u8"Iran";
            break;
        }
        case 5:
        {
            country_name = u8"Jordan";
            if (state_or_prov == "AMN")
                state_or_prov_name = "Amman";
            break;
        }
        case 6:
        {
            country_name = u8"Israel";
            break;
        }
        case 7:
        {
            country_name = u8"Arabian Peninsula";
            break;
        }
        case 8:
        {
            country_name = u8"Kuwait";
            break;
        }
        case 9:
        {
            country_name = u8"Cyprus";
            break;
        }
        case 10:
        {
            country_name = u8"Lebanon";
            if (state_or_prov == "TYR")
                state_or_prov_name = "Tyre";
            else if (state_or_prov == "BEI")
                state_or_prov_name = "Beirut";
            else if (state_or_prov == "BRT")
                state_or_prov_name = "Beirut";
            break;
        }
        }

        break;
    }
    case 11: // Space
    {
        switch (country_code)
        {
        case 1: country_name = u8"Earth Orbit or seen from space stations/capsules"; break;
        case 2: country_name = u8"The Moon"; break;
        case 3: country_name = u8"Venus"; break;
        case 4: country_name = u8"Mars"; break;
        case 5: country_name = u8"Asteroids"; break;
        case 6: country_name = u8"Jupiter"; break;
        case 7: country_name = u8"Saturn"; break;
        case 8: country_name = u8"Uranus"; break;
        case 9: country_name = u8"Neptune"; break;
        case 10: country_name = u8"Deep Space"; break;
        case 11: country_name = u8"Pluto"; break;
        }

        break;
    }
    default:
        break;
    }
}

// References
static const hatch_ref g_hatch_refs[] =
{
    {0, "HYNEK, J. Allen: The UFO EXPERIENCE, A Scientific Inquiry; Ballentine Books, NY 1974,1977."},
    {1, "HYNEK, J. Allen & IMBROGNIO, Phil: NIGHT SIEGE, the Hudson Valley UFO Sightings; Ballentine Books, NY 1987"},
    {2, "VALLEE, Jacques: UFO'S IN SPACE - Anatomy of a Phenomenon; Henry Regnery 1966 & Ballentine PB 1974 294pp."},
    {3, "VALLEE, Jacques: CHALLENGE to SCIENCE, The UFO ENIGMA; Henry Regnery 1966 & Ballentine Books, NY 1974. PB"},
    {4, "VALLEE, Jacques: DIMENSIONS, A Casebook of Alien Contact Ballentine, NY 1989. PB"},
    {5, "VALLEE, Jacques: CONFRONTATIONS, A Scientists Search..; Ballentine Books, NY 1990+91.PB"},
    {6, "VALLEE, Jacques: REVELATIONS, Alien Contact and Human Deception; Ballentine Books 1991+92."},
    {7, "VALLEE, Jacques: MESSENGERS of DECEPTION; Bantam PB 1979 272pp."},
    {8, "VALLEE, Jacques: PASSPORT TO MAGONIA; H. Regnery, Chicago HC 1969 & Contemporary Books,Chicago 1993. 372pp."},
    {9, "VALLEE, Jacques: UFO CHRONICLES of the SOVIET UNION; Ballentine Books HB 1992. 212pp."},
    {10, "SPENCER, John & EVANS, Hillary (ed): PHENOMENON - 40 YEARS of F.S.; BUFORA/Avon Books, NY 1988"},
    {11, "SPENCER, John: The UFO ENCYCLOPEDIA; BUFORA/Acon Books, NY. 1991. 340pp."},
    {12, "STURROCK, Dr. Peter A.: The UFO ENIGMA; Warner Books, NY 1999; ISBN 0-446-52565-0 HB 404pp."},
    {13, "FOWLER, Raymond E.: The WATCHERS; Bantam PB, NY 1990"},
    {14, "HYNEK & VALLEE: The EDGE of REALITY, A Progress Report, Chicago, Regnery 1975. 301pp 24cm"},
    {15, "RANDLE, Kevin & SCHMITT,Donald: UFO CRASH at ROSWELL; Avon Books, NY 1991. 327pp."},
    {16, "HOPKINS, Budd: MISSING TIME; 255pp. G. P. Putnam's 1980 & Ballentine PB 1988. 255p."},
    {17, "BALLESTER OLMOS, V-J: FOTOCAT Catalog: Publishing data to be announced."},
    {18, "STURROCK, Dr. Peter A: REPORT / SURVEY of the AMERICAN ASTRONOMICAL SOCIETY of the UFO PROBLEM; Stanford, CA 1977."},
    {19, "STEIGER, Brad & WHRITENOUR, Joan: FLYING SAUCERS ARE HOSTILE; Award Books PB 1967."},
    {20, "STEIGER, Brad: The UFO ABDUCTORS. Berkley Books, NY 1988"},
    {21, "STEIGER, Brad (ed.): PROJECT BLUEBOOK; Ballentine Books 1976. 423pp."},
    {22, "BERLITZ, Charles & MOORE, William: The ROSWELL INCIDENT; Putnam 1980 & Berkley Books 1988."},
    {23, "FALLA, GEOFFREY: VEHICLE INTERFERENCE PROJECT. BUFORA Ltd. 1979. C.F. Lockwood & A.R. Rice editors."},
    {24, "UFO INVESTIGATOR: NICAP: Kensington, MD & Washington, DC [defunct]. University Microfilms, Ann Arbor, MI 48106"},
    {25, "HALL, Richard: UNINVITED GUESTS; Aurora Press, Santa Fe, NM 1988. 381pp."},
    {26, "FAWCETT, Lawrence & GREENWOOD, Barry: The UFO COVERUP (Formerly Clear Intent); Prentice Hall, NJ 1984. 264pp."},
    {27, "FLAMMONDE, Paris: The AGE of FLYING SAUCERS; Hawthorne Books, NY 1971"},
    {28, "FLAMMONDE, Paris: UFO EXIST!; Ballentine Books, NY 1976. PB 480pp."},
    {29, "RUPPELT, Edward J.: The REPORT ON UNIDENTIFIED FLYING OBJECTS; Doubleday, NY 1956 HC.243pp."},
    {30, "FIGEUT, Michel & RUCHON, Jean-Louis: OVNI - Le Premier Dossier..; Alain LeFeuvre, Paris 1979."},
    {31, "RUTLEDGE, Harley D. Ph.D.: PROJECT IDENTIFICATION; Prentice-Hall, NJ 1981."},
    {32, "DRUFFEL, Ann & ROGO, D. Scott:The TUJUNGA CANYON CONTACTS Signet Books PB 1989"},
    {33, "EMENEGGER, Robert: UFO'S PAST PRESENT & FUTURE; Ballentine Books 1974."},
    {34, "BLUNDELL, Nigel & BOAR, Roger: The WORLDS GREATEST UFO MYSTERIES; Octopus, London 1989 & PB/Berkley Books, NY 1990"},
    {35, "WILSON, Clifford: The ALIEN AGENDA; 1974 & Signet PB 1988. (religious interpretation.)"},
    {36, "LINDEMANN, Michael (ed.): UFO's and the ALIEN PRESENCE - Six Viewpoints; 2020 Group, Santa Barbara, CA 1991. 233pp."},
    {37, "CONDON, Dr. Edward: SCIENTIFIC STUDY of UFO..etc; Bantam 1969. 965pp."},
    {38, "HUNT, Gerry: BIZARRE AMERICA; Berkley Books, NY. PB 1988. [ inexpensive ]"},
    {39, "ITACAT, Catalogo Italiano degli Incontri Ravvicinati. Maurizio Verga, 1989 CISU/Torino, Italy."},
    {40, "CUFON UFO Network Site (former BBS) various downloads. http://www.cufon.org"},
    {41, "HALL, Richard: The UFO Evidence II: HB: Scarecrow Press Lanham, MD. http://www.scarecrowpress.com ISBN 0-8108-3881-8"},
    {42, "SALISBURY, Frank: The UTAH UFO DISPLAY -- a Biologist Reports; 1974 Devin Adair, New Greenwich,CT. HB 286pp. ISBN 0-815-97000-5"},
    {43, "HITT, Michael D: GEORGIA'S AERIAL PHENOMENON 1947-1987 Self published 1999. ref. Jan Aldrich."},
    {44, "SANDERSON, Ivan: INVISIBLE RESIDENTS; World Publishing HB 1970 & Avon Books PB 1973"},
    {45, "SANDERSON, Ivan: UNINVITED VISITORS; Cowles Educational Corp. NY 1967 HB"},
    {46, "KEHOE, Maj Donald: FLYING SAUCERS TOP SECRET; NICAP and G.P. Putnam's Sons, NY 1960 HB."},
    {47, "KEHOE, Maj Donald: ALIENS FROM SPACE; Doubleday, Garden City, NY 1973 HB"},
    {48, "MICHEL, Aime: The TRUTH ABOUT FLYING SAUCERS; S. G. Phillips 1956 & Pyramid Books, NY 1967."},
    {49, "MICHEL, Aime: FLYING SAUCERS & the STRAIGHT LINE THEORY HB (hard to find)"},
    {50, "BALLESTER OLMOS, Vicente-Juan: CATALOG of 200 TYPE-1 EVENTS in SPAIN & PORTUGAL; CUFOS 1976."},
    {51, "ZEIDMAN, Jennie: The LUMBERTON REPORT; CUFOS 1976. (N. Carolina, 1975)"},
    {52, "FULLER, John G: ALIENS IN THE SKIES; Putnam's Sons and Berkley Books, NY 1969."},
    {53, "HOLZER, Hans: THE UFO-NAUTS; Fawcett Gold Medal Books, Greenwich, CT 1976."},
    {54, "MUSGRAVE, John Brent: UFO OCCUPANTS & CRITTERS; Global Communications, NY 1979. 8x11 66pp."},
    {55, "HYNEK, Dr. J. Allen: THE HYNEK UFO REPORT; Dell Publ., NY 1977. 298pp."},
    {56, "UFOCAT CATALOG: Assorted Printouts. Donald A. Johnson Ph.D (CUFOS).  P.O. Box 446, Concord, NH 03302-0446"},
    {57, "GUASP, Miguel: TEORIA de PROCESOS de los OVNI; Valencia, Spain, 1973. Self published. Scholarly. Short catalog."},
    {58, "GINDILIS, L.M & MENLOV & PETROVSKAYA: OBSERVATIONS of ANOMALOUS ATMOSPHERIC PHENO./USSR; CUFOS/USSR ACADEMY of SCIENCE 1979."},
    {59, "SCIENCE & MECHANICS (eds.) OFFICIAL GUIDE to UFO'S; Ace Books, NY. 1968."},
    {60, "FARISH, Jucius: UFO NEWSCLIPPING SERVICE: $5/Monthly. Plumerville, AR."},
    {61, "STRIEBER, Whitley: COMMUNION- A true story; Wilson & Neff, NY 1987 & Avon Books PB 1988."},
    {62, "STRIEBER, Whitley: TRANSFORMATION--; William Morrow, NY 1988. Avon Books PB 1989"},
    {63, "NATIONAL ENQUIRER UFO REPORT; Pocket Books, NY 1985."},
    {64, "DAVID, Jay (ed.): FLYING SAUCERS HAVE ARRIVED: HB World Publ. NY 1970."},
    {65, "HALL, Richard H: From AIRSHIPS to ARNOLD (1900-1946) UFO Research Coalition; Fairfax, VA 2000."},
    {66, "SAGAN/PAGE:UFO'S a SCIENTIFIC DEBATE: Cornell Univ. Press, Ithaca, NY 1972"},
    {67, "BOWEN, Charles (ed.) The HUMANOIDS: HB Henry Regnery, Chicago 1969. 256pp. PB 1977 Futura, London"},
    {68, "STEIGER, Brad: STRANGERS from the SKIES; Award Books, NY 1966."},
    {69, "BLUM, Ralph & Judy: BEYOND EARTH: Mans Contact with UFO's Bantam, NY 1974"},
    {70, "OLSEN, Thomas: REFERENCE for OUTSTANDING UFO SIGHTINGS REPORTS. 1966 UFO Information Retrieval Center, Riderwood, MD 21139."},
    {71, "SACHS, Margaret: CELESTIAL PASSENGERS - UFO'S and SPACE TRAVEL; Penguin Books, 1977."},
    {72, "FLICKENGER, Don: NORTH DAKOTA UFO files. Well Documented NICAP files for the 1960s: Xeroxed by Tom Tulien"},
    {73, "EDWARDS, Frank: FLYING SAUCERS-HERE AND NOW; 1967 Lyle Stewart & Bantam Books 1968."},
    {74, "EDWARDS, Frank: FLYING SAUCERS-SERIOUS BUSINESS; Lyle Stewart & Bantam Books 1966."},
    {75, "EDWARDS, Frank: STRANGE WORLD; 1964 & Carol Paperbacks 1992."},
    {76, "ANDREWS, George C: EXTRA-TERRESTRIALS AMONG US; FATE/Llewellyn Publs, St. Paul, MN 1992"},
    {77, "CRYSTAL,Ellen: SILENT INVASION - Shocking Discoveries; Paragon House, NY 1991. 190pp."},
    {78, "LOFTIN, Robert: IDENTIFIED FLYING SAUCERS: D. McKay, NY 1968"},
    {79, "RODEGHIER, Mark/CUFOS: UFO REPORTS INVOLVING VEHICLE INTERFERENCE; CUFOS 1979. Available as CUFOS UFO Archive #1 CD-ROM. http://www.cufos.org/Pubform.pdf"},
    {80, "RANDLES, Jenny: ALIEN CONTACTS & ABDUCTIONS; Sterling Publ., NY 1993."},
    {81, "RANDLES, Jenny: FROM OUT OF THE BLUE; Berkley Books, NY 1993 PB 233pp."},
    {82, "RANDLES, Jenny: FS & HOW TO SEE THEM: Sterling Publ. Co. NY 1992"},
    {83, "FOWLER, Raymond E.: UFO'S, INTERPLANETARY VISITORS; Prentice Hall & Bantam Books, NY 1979. 346pp."},
    {84, "HOBANA Ion & WEVERBERGH,Julien:  UFO'S from BEHIND the  IRON CURTAIN; Bantam Books,1975. 308pp."},
    {85, "LESLIE, Desmond & ADAMSKI,G: Flying Saucers Have Landed; British Book Center, NY 1956"},
    {86, "ROGO, D. Scott (ed): ALIEN ABDUCTIONS, True Cases of UFO Kidnappings; Signet/Penguin, NY 1980."},
    {87, "RANDLES, Jenny: ALIEN ABDUCTIONS - the Mystery Solved; Inner Light, NJ 1988."},
    {88, "RANDLE, Kevin D.: The OCTOBER SCENARIO; Berkley Books, NY 1988."},
    {89, "RANDLE, Kevin D.: The UFO CASEBOOK; Warner Books 1989. 256pp."},
    {90, "GIBBONS, Gavin: The COMING of the SPACE SHIPS; Neville-Spearman, London HB 1956.  13s.6d"},
    {91, "GENERAL KNOWLEDGE ( various, unreferenced. )"},
    {92, "TV PROGRAM/NEWS ( various, unreferenced. )"},
    {93, "Newspaper account: "},
    {94, "PERSONAL INTERVIEW or Experience."},
    {95, "UFO MAGAZINE (USA) Monthly, Los Angeles, CA"},
    {96, "Database/website: "},
    {97, "Periodical: "},
    {98, "Book: "},
    {99, "Reference unknown or lost. NFD."},
    {100, "STORY, Ronald (ed.) : ENCYLOPEDIA of UFO'S: Doubleday, Garden City, NY 1980"},
    {101, "WALTERS, Ed & Francis: The GULF BREEZE SIGHTINGS; Wm. Morrow 1990 & Avon PB 1991. 370pp."},
    {102, "WALTERS, Ed & Francis: UFO ABDUCTIONS in GULF BREEZE; Avon Books, NY 1994"},
    {103, "CLARK, Jerome: UFO ENCOUNTERS & BEYOND; Signet Books 1993. 192pp."},
    {104, "TRENCH, Brinsley Le Poer: MYSTERIOUS VISITORS - the UFO Story; Stein & Day, NY 1971-73"},
    {105, "PRATT, Bob: UFO DANGER ZONE - Terror and Death in Brazil. Horus House Press, Madison, WI 1996"},
    {106, "WILKINS, Harold T.: FLYING SAUCERS ON THE ATTACK; Ace Books, NY PB 1967."},
    {107, "WILKINS, Harold T.: FLYING SAUCERS UNCENSORED; Citadel 1955 & Pyramid Books, NY PB 1967."},
    {108, "CLARK, Jerome & COLEMAN, Joren: THE UNIDENTIFIED; Warner PBs, NY 1975"},
    {109, "LORENZEN, Coral & Jim: UFO's OVER THE AMERICAS; Signet Books, NY 1968. 243p."},
    {110, "LORENZEN, Coral & Jim: ABDUCTED!; APRO/Berkley Books, NY 1977. 227pp."},
    {111, "LORENZEN, Coral: FLYING SAUCERS, the Starting Evidence; Signet Books, NY 1962 & 1966. 278pp. Originally \"The Great F. S. Hoax\"."},
    {112, "LORENZEN, Coral & Jim: FLYING SAUCER OCCUPANTS; Signet Books, NY 1967."},
    {113, "LORENZEN, Coral & Jim: ENCOUNTERS with UFO OCCUPANTS; APRO/Berkley Medallion Books 1976. 424pp"},
    {114, "HAINES, Richard Ph.D: PROJECT DELTA; 1994, LDA Press, PO Box 880, Los Altos, CA 94023 USA"},
    {115, "HAINES, Richard Ph.D: The MELBOURNE EPISODE; LDA Press 1990."},
    {116, "HAINES, Richard Ph.D: ADVANCED AERIAL DEVICES REPORTED during the KOREAN WAR; 1990 LDA Press, PO Box 880, Los Altos, CA 94023"},
    {117, "HAINES, Richard Ph.D: JUSA-CISAAF TRANSLATIONs (from Russian)."},
    {118, "RIDGE, F: UFO FILTER CENTER Newsletter. Monthly 618 Davis Dr. Mount Vernon, IN  47620"},
    {119, "ADLER, Bill (ed.): LETTERS to the AIR FORCE on UFO's; Dell Paperbacks 1967;"},
    {120, "GOOD, Timothy: ABOVE TOP SECRET; Sidgewick & Jackson, London 1987 & Quill Books, 1988. 592pp."},
    {121, "GOOD, Timothy: ALIEN UPDATE. [Haines personal library]"},
    {122, "GOOD, Timothy: The UFO REPORT [1990]; Avon Books 1991. 237pp."},
    {123, "GOOD, Timothy: The UFO REPORT 1991; Sidgewick & Jackson, London 1992"},
    {124, "GOOD, Timothy: The UFO REPORT 1992; Sidgewick & Jackson, London 1993"},
    {125, "GOOD, Timothy: ALIEN CONTACT, Top-Secret UFO Files Revealed; William Morrow, NY 1993. 288pp."},
    {126, "[to be re-assigned]"},
    {127, "[to be re-assigned]"},
    {128, "GROSS, Loren: UFO'S a HISTORY 1896. Self published 1974, 87"},
    {129, "GROSS, Loren: CHARLES FORT & UFO'S. Self published, 1976"},
    {130, "GROSS, Loren: UFO's a HISTORY 1946 The Ghost Rockets. 1972,82,88."},
    {131, "GROSS, Loren: UFO'S a HISTORY 1947: 1988 book and recent Supplements."},
    {132, "GROSS, Loren: UFO'S a HISTORY 1948: 2 books: 1988 & 2000 Supplement."},
    {133, "GROSS, Loren: UFO'S a HISTORY 1949: 3 books: 1988 & 2000 2001 Supplements."},
    {134, "GROSS, Loren: UFO's a HISTORY-1950 (4+books) 1989+2000  Self published."},
    {135, "GROSS, Loren: UFO'S a HISTORY 1951 (2 booklets) Self published."},
    {136, "GROSS, Loren: UFO's a HISTORY-1952 (12 booklets) Self published."},
    {137, "GROSS, Loren: UFO's a HISTORY-1953 (6 booklets) Self published."},
    {138, "GROSS, Loren: UFO's a HISTORY-1954 (5 booklets) 1994 (2nd edition)"},
    {139, "GROSS, Loren: UFO's a HISTORY-1955 (3 booklets) Self published."},
    {140, "GROSS, Loren: UFO's a HISTORY-1956 (3 booklets) 1994 Self published."},
    {141, "GROSS, Loren: UFO's a HISTORY-1957 (19 booklets) Self published."},
    {142, "GROSS, Loren: UFO's a HISTORY-1958 (7 booklets) 1998/03 Self published."},
    {143, "GROSS, Loren: UFO's a HISTORY-1959 (4 booklets) 1999/00 Self published."},
    {144, "to be assigned. ]"},
    {145, "SPRINKLE, R. Leo Ph.D: Private Files"},
    {146, "SUFOI UFO NEWS. Annual. Articles translated from UFO - Nyt. Skandavinsk UFO Information; Postbox 6, DK-2820 Gentoffte, DENMARK."},
    {147, "FERRUGHELLI, PAUL: SIGHTINGS YEARBOOKs (by year.)"},
    {148, "HAINES, Richard Ph.D: CE-5, CLOSE ENCOUNTERS of the 5th KIND. 1999 Sourcebooks Inc. Naperville, IL ISBN 1-57071-427-4. 242 case files."},
    {149, "BERLINER, Don & HUNEEUS, Antonio: UFO BRIEFING DOCUMENT The Best Available Evidence, Nov. 1995 CUFOS/FUFOR/MUFO. 128pp 8x11."},
    {150, "WEINSTEIN, Dominique; UAP: 80 Years of Pilot Sightings Privately published. 7 avenue Bartholome. 75015 Paris, FRANCE."},
    {151, "KLOIAN, Richard: UFO's in the New York TIMES, 1947-1987 1994 Document Research Services, Berkeley, CA"},
    {152, "BOURRET, Jean-Claude: Le NOUVEAU DEFI des OVNI. Editions France Loisirs, Paris. 1976 ISBN 2-7242-0104-3 395pp."},
    {153, "BOURRET, Jean-Claude: OVNI .. l'ARMEE PARLE. Editions France Empire, Paris. 1979 (softcover)"},
    {154, "ZINSTAG, Lou & ALLEMANN, Dr. T; UFO-SICHTUNGEN uber der SCHWEIZ; UFO-Verlag, Basel/Zurich 1958 (via Bruno Mancusi)"},
    {155, "FOWLER, Raymond: The ALLAGASH ABDUCTIONS. Wild Flower Press; Tigard, OR 1993. 347pp."},
    {156, "FSR CASE HISTORIES. Bimonthly/London. Supplements 2 - 18 (final issue)"},
    {157, "The WEST VIRGINIA UFO NEWSLETTER. Bob Teets editor. Headline Books Inc. PO Box 52; Terra Alta, WV 26764 Bimonthly. Terra Alta at AOL.com 304/789-5951"},
    {158, "The AUSTRALIAN UFO BULLETIN; Qtrly. Judith MAGEE Ed. VUFORS PO Box 43, Moorabin, VIC 3189 Australia."},
    {159, "UFOMANIA Bulletin (qtrly). Didier GOMEZ: BP 300, 260 rue des Pyrenees, 75960 Paris cedex 20., FRANCE. 20pp."},
    {160, "MUFON UFO JOURNAL, Seguin, TX USA. Monthly."},
    {161, "HENDRY, Allan: The UFO HANDBOOK. Doubleday, Garden City, NJ 1979."},
    {162, "LANDSBURG, Alan: In SEARCH of EXTRATERRESTRIALS; Bantam Books PB 1976"},
    {163, "TIME-LIFE: The UFO PHENOMENON; Time-Life Books, Alexandria, VA. ( undated coffee-table hardcover.)"},
    {164, "BERLINER,Don & FRIEDMAN, Stanton: CRASH at CORONA 1992 1997 Marlowe & Co. NY ISBN 1-56924-863-X."},
    {165, "REHN, K. Gosta: UFO's HERE and NOW; Translated/Swedish Patricia Crompton: London; Abelard 1974"},
    {166, "McWAYNE, Glenn & GRAHAM, David: The NEW UFO SIGHTINGS; Warner PB 1974"},
    {167, "HIND, Cynthia: UFO'S, AFRICAN ENCOUNTERS; Gemini Books, Salisbury, Zimbabwe 1982. 236pp"},
    {168, "VESCO, Renato: INTERCEPT but DON'T SHOOT; transl. from Italian. Goofy."},
    {169, "RANDLES, Jenny & Warrington, P.: SCIENCE and the UFO'S; Basil Blackwell, NY 1985"},
    {170, "FILER's FILES: George A. Filer: MUFON Eastern Dir. <Majorstar at aol.com>"},
    {171, "ALDRICH, Jan J.: Private research papers. A huge mass of hard-won information."},
    {172, "KEHOE, Donald: FLYING SAUCERS FROM OUTER SPACE; Henry Holt, NY 1953 HC."},
    {173, "KEHOE, Donald: The FLYING SAUCER CONSPIRACY; Henry Holt, NY 1955; HC"},
    {174, "CHALKER,Bill: The OZ FILES. Australian Sightings. 1996 Duffy & Snellgrove.  NSW,Australia. ISBN 1-875989-04-8"},
    {175, "HAINES, Richard & WEINSTEIN,Dominique: Preliminary Study of 64 Pilot Sightings involving EME on Aircraft Systems NARCAP TR-3 2001"},
    {176, "STRINGFIELD, Leonard: SITUATION RED - The UFO Siege. Fawcett Crest, NY 1977"},
    {177, "Von LUDVIGER, Illobrand ed. UFO's-SEUGEN UND ZEICHEN. Berlin 1995 edition q-Verlags GmbH ISBN 3-86124-300-8."},
    {178, "WEBB, David: 1973, YEAR of the HUMANOIDS. 2nd Edition May 1976 CUFOS Evanston,IL (edited / Mimi Hynek)"},
    {179, "LEDGER, Don: MARITIME UFO FILES: 1998 Nimbus Publ. Ltd. Halifax, NS. ISBN 1-55109-269-7 162pp. trade paper."},
    {180, "PHILLIPS, Ted: Study of PHYSICAL TRACES ASSOCIATED with UFO SIGHTINGS; CUFOS 1975."},
    {181, "SPENCER, John Wallace: NO EARTHLY EXPLANATION; Bantam Books 1974."},
    {182, "MICHELL, John: The FLYING SAUCER VISION; Ace-Star Books, NY PB 1967."},
    {183, "YOUNG, Mort: UFO-TOP SECRET; Essandess Special Edition, 1967. PB"},
    {184, "MAGOR, John: OUR UFO VISITORS. Hancock House, Canada & Seattle, WA. 1977."},
    {185, "BERLINER, DON: The BLUEBOOK UNKNOWNS; (FUFOR)."},
    {186, "INFORESPACE Journal. SOBEPS Group, Belgium; bimonthly"},
    {187, "BLOECHER, Ted: REPORT / the UFO WAVE of 1947; Institute of Atmospheric Physics, University of Arizona, Tucson 1967."},
    {188, "TEETS, Bob: WEST VIRGINIA UFO's; Headline Books, PO Box 52, Terra Alta, WV 1995 213pp."},
    {189, "RANDLE, Kevin D.: A HISTORY of UFO CRASHES; Avon Books, Dresden, TN 1995"},
    {190, "LOURENCO, Victor: The PORTUCAT CATALOG; (BBS Download)."},
    {191, "AFU NEWSLETTER (English). Anders Liljegren (ed). Archives for UFO Research; PO Box 11027; S-600 11 Norrkoping, SWEDEN"},
    {192, "ANDREWS, George: EXTRATERRESTRIAL FRIENDS and FOES; Illuminet Press,Lilburn,GA 1993."},
    {193, "PHENOMENES SPATIAUX: Group GEPA, Gen. M. Lionel Chassin Paris. Quarterly 1964?-1977. Merged into Grp. GEPAN"},
    {194, "LUMIERES DANS la NUIT (LDLN), Paris. Bimonthly since 1957. LDLN B.P.3; 77123 Le Vaudoue, FRANCE."},
    {195, "FLYING SAUCER REVIEW (FSR), London. Bimonthly. PO Box 162; High Wycombe, Bucks. HP135 DZ, England."},
    {196, "BRAZILIAN UFO REPORT: Michael Wysmierski, Brazil."},
    {197, "WEINSTEIN, Dominique: French Newsclippings: Sept-Dec 1954. 100s/articles by date. Privately Publ., Paris, 1999."},
    {198, "MANEY, Charles & HALL, Richard: The CHALLENGE OF UFO'S; NICAP, Washington, DC. 1961"},
    {199, "RANDLES, Jenny & WARRINGTON, Peter (eds): UFO's a BRITISH PERSPECTIVE; Robert Hale, London 1979."},
    {200, "INTERNATIONAL UFO REPORTER (IUR) Bimonthly. CUFOS, Chicago. Highly regarded."},
    {201, "SIFAKIS, Carl: The OFFICIAL QUIDE to UFO SIGHTINGS; Sterling Publ, NY 1979. (Gee whiz!)"},
    {202, "HAINES, Richard (ed): UFO PHENOMENA and the BEHAVIORAL SCIENTIST. Scarecrow Press, Metuchen, NJ 1979. 13 essays."},
    {203, "KEEL, John: OPERATION TROJAN HORSE; Putnam's Sons, NY 1970."},
    {204, "SOULE, Gardner: UFO's & IFOs - A Factual Report..; Putnam's Sons, NY. 1968?"},
    {205, "ACTAEON ADVISOR (qtrly) Actaeon Group; PO.Box 515123, St. Louis, MO 63151-1290 314/994-1290."},
    {206, "BREIER, Rogerio Port: CAUSISTICA UFOLOGICA de 1992. Catalog. UBPVD Group, Gravatai, RS, Brazil 1993"},
    {207, "STONE, Reuben: UFO INVESTIGATION; 1993 Blitz/Bookmark, Enderby, England."},
    {208, "DONG, Paul: UFO's Over MODERN CHINA. 1983 UFO Photo Archives; P. O. Box 17206; Tucson, AZ 85710."},
    {209, "UFO - Rivista di informazione Ufologica; 2 issues/year CISU. Giovanni Settimo ed.; C.P.82, 10100 Torino, ITALY."},
    {210, "The APRO BULLETIN: Monthly. Jim & Coral Lorenzen: Tucson,AZ (USA. defunct) By volume and number."},
    {211, "GREEN, Gabriel: LETS FACE THE FACTS ABOUT UFO's. Popular Library, NY 1967. [JMC]"},
    {212, "CATHIE, Bruce: HARMONIC 33. A. & A. Reed, Wellington, NZ"},
    {213, "BOWEN, Charles: ENCOUNTER CASES from FLYING SAUCER REVIEW. Signet Books 1977/78."},
    {214, "STEIGER, Brad: The RAINBOW CONSPIRACY; Pinnacle Books, NY 1994 PB"},
    {215, "BANCHS, Rodolfo: LAS EVIDENCIAS de los OVNI; Rudolfo Alonso Editor, Argentina. 1976"},
    {216, "STRAND, Erling: PROJECT HESSDALEN Final Technical Rpt; Norway, 1984."},
    {217, "SIDER, Jean: DOSSIER 1954 et l'Imposture Rationaliste; 1997 Editions RAMUEL, Paris. ISBN 2-910401-23-5 (2 volumes)"},
    {218, "FOWLER, Ray: CASEBOOK of a UFO INVESTIGATOR; Prentice-Hall, Englewood Cliff,NJ 1981."},
    {219, "MAUGE, Claude: French sightings catalog by author.; Recontres de Lyon symposium, 1988."},
    {220, "NATIONAL UFO REPORTING CENTER (NURC) BBS and Website. P.O. Box 45623, University Station, Seattle, WA 98145."},
    {221, "BOUGARD, Michel: SOUCOUPES VOLANTES; SOBEPS Group, Belgium 1976. HB"},
    {222, "GRANCHI, Irene: UFO's and ABDUCTIONS in BRAZIL: Horus House Press, Madison, WI: 1992 (English translation 1994) ISBN 1-881852-09-1"},
    {223, "PINDEVIC, Thierry: Le NOEUD GORDIEN ou la FANTASTIQUE HISTOIRE des OVNI. Editions France-Empire, Paris 1979."},
    {224, "STEIGER, Brad & WHRITENOUR, Joan: FLYING SAUCER INVASION. Award Books, NY 1969. 156pp PB. 15 Essays + catalog."},
    {225, "SPENCER, Jown Wallace: The UFO YEARBOOK. Phillips Publ Springfield, MA 1976."},
    {226, "PHENOMENA Revue / SOS-OVNI, France; Perry Petrakis ed. B.P 324; 13611 Aix-Cedex 1, Fr. sosovni at pacwan.mm-soft.fr"},
    {227, "SHUTTLEWOOD, Arthur: The WARMINSTER MYSTERY. Neville-Spearman/Tandem Books PB 1967, London."},
    {228, "ERSKINE, Allen Louis: THEY ARE THEY WATCHING US. Belmont Tower Books, NY 1967."},
    {229, "UFO RESEARCH AUSTRALIA NEWSLETTER. (Details missing)"},
    {230, "LDLN CONTACT LECTEURS: by Series and #. May'68 -JAN'72 (Entire set of 18 issues) LDLN local chapters & individual members' UFO reports."},
    {231, "The UFO REGISTER. CONTACT INTERNATIONAL/UK. (annual sightings catalog.)"},
    {232, "AWARENESS (Quarterly) CONTACT INTERNATIONAL/UK: listed by year."},
    {233, "STEIGER, Brad & WHRITENOUR, Joan: The ALLENDE LETTERS. 1968 Eugene Olsen Publ + Award Books, NY PB"},
    {234, "DEVEREUX, Paul: EARTH LIGHTS REVELATION. 1989 Blanford/UK & 1991 Biddles Ltd, Guilford, Engl. PB"},
    {235, "MACDONALD, James E.: Report to the American Society of Newspaper Editors."},
    {236, "BOLETIM CEPEX. (Qtrly) Centro de Estudios e Pesquisos Exologicos; Sumaro, SP, Brazil."},
    {237, "RIDGE, Francis: REGIONAL ENCOUNTERS in the FC Files. 170pp. UFO FILTER CENTER, Mt. Vernon, IN 1994"},
    {238, "MARIE, Franck: OVNI CONTACT-Un Enquete Choc. 400 French sightings on 05NOV90! 525pp. 1993 BANQUE OVNI. BP 41 - 92224 Bagneux cedex, FRANCE."},
    {239, "BANQUE OVNI: (Franck Marie ed.) A series of reports on UFO waves. Listed by Report number."},
    {240, "UFO ROUNDUP weekly email of UFO news: Joseph Trainor. http://www.digiserve.com/ufoinfo/roundup/ .."},
    {241, "ACUFOS AUSTRALIA: Sightings Catalogs 1978-1982. Keith Basterfield. PO Box 546, Gosford, NSW Australia 2250."},
    {242, "HALL, Richard: The UFO EVIDENCE; NICAP,Washington, DC 1964. Reprinted 1997 Barnes & Noble Books ( only $7.98! )"},
    {243, "SCHUESSLER, John F.: UFO-RELATED HUMAN PHYSIOLOGICAL EFFECTS. 1996 J. Schuessler, POB 58485, Houston, TX 77258"},
    {244, "UFO AFRINEWS; Twice annually. Cynthia Hind, editor; PO Box MP 49, Mount Pleasant, Harare, Zimbabwe."},
    {245, "ROUSSEL, Robert: OVNI, les Verites Cachees. Ed. Albin  Michel,Paris 1994 ISBN 2-226-06646-2. 346 pp softback"},
    {246, "GARREAU, Charles: SOUCOUPES VOLANTES, Vingt-cinq Ans d'Enquetes. 3rd Editn = 1973? Maison Mame ISBN 2-250-00464-1"},
    {247, "[ to be assigned ]"},
    {248, "[ to be assigned ]"},
    {249, "CLARK, Jerome: High Strangeness: UFO's from 1960 through 1979. The UFO ENCYCLOPEDIA Volume 3. 1996 Omnigraphics Inc Detroit, MI ISBN 1-55888-742-3"},
    {250, "[ to be assigned ]"},
    {251, "BOEDEC, Jean-Francois: Les OVNI EN BRETAGNE, Anatomie d'un Phenomene. 1978 Editions Fernand Lanore, Paris."},
    {252, "CAMERON, Vicky: UFO EXPERIENCES in CANADA, Don't tell anyone but.. ; 1995 General Store Publishing ;  Burnstown, Ontario ISBN 1-896182-20-8"},
    {253, "SIEFRIED, Richard & CARTER, Michael: NATIVE ENCOUNTERS M. S. Graphics, 1994. ISBN 0-9642461-0-4 ( Oklahoma MUFON etc.)"},
};

// 93: NEWPAPERS and FOOTNOTES (By sighting location)
static const hatch_ref g_hatch_refs_93[] =
{
    {0, "Unknown or lost."},
    { 1, "Commugny, Vaud, SWZ. Le Bouquet: 07 FEB 1973. clipping Bruno Mancusi, Switzerland." },
    { 2, "Payerne, Vaud, SWZ.  L'Illustre: 07 OCT 1954. clipping Bruno Mancusi, Switzerland." },
    { 3, "La Tour-de-Peilz, SWZ. Gazette de Lausanne: 28 OCT 1952 Bruno Mancusi, Switzerland." },
    { 4, "Fribourg, SWZ 1974. Irish Catholic, Dublin 20 NOV 1975 and GUB Bulletin #7 (in french) Clippings /Bruno Mancusi,  Switzerland." },
    { 5, "La Chaux-de-Fonds, Neuchatel, SWZ. 24 Heures, 28 APR 1976 and Tribune de Geneve, 28APR & 03MAY, 1976. /Bruno Mancusi." },
    { 6, "Capello de Viglio, Ticino, SWZ. Corriere del Ticino, 23 DEC 1988. / Bruno Mancusi." },
    { 7, "San Francisco Chronicle. California. Various dates." },
    { 8, "Scandia, MN 20 SEP 80: Forest Lake Times (Weekly) v77#19 Thurs. 25SEP80 pg 1. also APRO Bulletin v29#3 MAR '81" },
    { 9, "Canary Islands: NATIONAL INQUIRER: undated clipping = 2/3 of pg 4." },
    { 10, "Knickerbocker News, Albany, NY 1966 (date unknown) (via Joe Ritrovato)" },
    { 11, "Le Journal de l'Ile [Reunion] #14872, 23AUG97. 357 rue Marechal Leclerc BP166, 97463 St. Denis CEDEX, Ile de la Reunion." },
    { 12, "le Tour-de-Treme, SWZL: GUB Bulletin #2, 1979 pg. 11 c/o Bruno Mancusi." },
    { 13, "Evansville, IN Courier: 12AUG98 via Stig Ajermose on Toronto email list." },
    { 14, "Hebei Daily>Baokan Wenzhai>Hong Kong Standard>Agence France Presse>UFO Updates+UFO Roundup 1998 #48" },
    { 15, "CNI News for 21AUG95. -and- American correspondent Roy S. Carson in Venezuela 14OCT95" },
    { 16, "Kal Korff sighting: via UFO Updates / Ufomind archives http://www.ufomind/com/ufo/updates/1997/may/m06-025/shtml" },
    { 17, "UFOHIST1.TXT from PEA Research / Ultra Majic BBS: http://www.ipacific.net.au/~pavig/govtinvolvementincove_488.shtml" },
    { 18, "UFO Window Website / Joe Ritrovato: Australia page http://www.flash.net/~joerit/docs2/au/au_chron.htm" },
    { 19, "File UFO907 / UFOBBS via Dr. Don Johnson http://www.ufobbs.com/txt1/907.ufo" },
    { 20, "NUFORC Report: Syracuse, NY: 14JLY47: [archive.org](https://web.archive.org/web/20000607005846/http://www.ufocenter.com/S3933.html)" },
    { 21, "Report to Skywatchers: <cuforg at interweb-design.co.uk>   http://www.interweb-design.co.uk/cuforg Ian Darlington, ed." },
    { 22, "Diario do Grande ABC for 8JLY99. Cr: Thiago Tichetti <thiagolt at opengate.com.br>" },
    { 23, "Journal de Montreal; 21AUG99.  via Michel M. DesChamps <ufoman at ican.net>" },
    { 24, "Little Rock, AR 1973: Jennifer Varner. AR-OCT73.TXT <jjenvasa at aol.com>  Private correspondence." },
    { 25, "Lancaster, OH 1897: Cincinnati Enquirer, 25APR1897. http://home.fuse.net/ufo/Aeribarque.html (via Kenny Young)" },
    { 26, "Richmond, VA Times-Dispatch, 29 Oct., 1999 (via UFO Updates) www.gatewayva.com/rtd/dailynews/virginia/ufo29.shtml " },
    { 27, "Ideal de Granada for 27APR2001. ALHAMA2001.html here. http://sib.nodos.com (click on Alhama de Granada.) Original rpt via Scott Corrales." },
    { 30, "Fyffe, AL 1989: 3 newspaper accounts on file her: Fyffe1,2,3.UFO in download folder: from WWW.skeptictank.org" },
    { 31, "Edmond, OK Sun for 3Aug65+31Oct01 on file here as OK3AUG65.txt cr: E.B.Knapp" },
    { 32, "Princeton Herald [NJ] for 11JLY47, on file here as 18JUNE47.txt cr: Joel Carpenter." },
    { 33, "PHILPOT, KY: 4-5AUG67: PHILPOT.TXT file in C:/DOWNLOADS Credit: Mike Christol & Richard Hall" },
    { 40, "George Hebert in Norfolk (VA) Ledger-Dispatch for 9 July1947. On file here: NORFOLK47.TXT" },
    { 50, "On file here: TURKEY801.TXT from ABC News online via UFO Updates list." },
    { 60, "Milliyet (Turkish newspaper) 23AUG2001: via Dominique Weinstein private correspondence; filed here." },
    { 70, "Correspondence on file: folder CHENNAI (Madras) India." },
    { 101, "CISU Newsflash #324/6Sept01: Monselice.txt on file. Il Gazzettino, Padua & Il Mattino di Padova, 17-30 AUG 2001" },
    { 102, "CISU Newsflash #347/26MAR02: CISU-NF347.txt on file." },
    { 103, "CISU Newsflash #369/28NOV02: 28SEP02.txt on file here. Numerous Italian newspapers listed." },
    { 120, "Trapua, RGS, BRZ: Near Cachoeira do Sul. LDLN #84 from Jornal do Dia;  Porto Alegre, RGS for 05 AUG 1965." },
    { 130, "Moyobamba, PERU:Data from Ray Stanford correspondence. Folder PERU57 on file here." },
    { 140, "On file here as TARAWA43.TXT from Jan Aldrich." },
    { 145, "CUFOS documents on file here in folder 1JAN54." },
};

// 96: "DATABASEs/WEBSITE"
static const hatch_ref g_hatch_refs_96[] =
{
    { 0, "UNKNOWN or LOST."},
    { 1, "LARRY HATCH: *U* DATABASE.  <webmaster at larryhatch.net> http://www.larryhatch.net" },
    { 40, "UFOROM/Manitoba, Canada: Chris Rutkowski: www.iufog.org/zines/sgj/uforom.html" },
    { 42, "RUTKOWSKI, C: UFOROM excerpts filed here as RUTKOW.MSC" },
    { 43, "RUTKOWSKI, C: UFOROM excerpts filed here as REP95A.MSC" },
    { 60, "ALLEN,DON: (moderator) MUFONET.BBS: excerpts file here as SCRAPS.CAT" },
    { 61, "http://www.netlink.co.uk/users/ddpweb/ufonews.html LGM-BBS: on file here as 960602.AUS" },
    { 65, "O'BRIAN, CHRIS: San Luis Valley, CO/NM catalog. Excerptsfiled here as SANLUISV.TXT" },
    { 66, "DIXON,Gloria, Dir. of Investigations/BUFORA Catalog. Excerpts filed here as BUFORA96.UK" },
    { 67, "TAYLOR, HERB: Cloud Cigar Catalog filed CCGLISTHT.TXT Case taken from HALL,Richard: The UFO Evidence II" },
    { 71, "RICHARD HAINES Article from IUR Vol.25 #1 http://www.nicap.dabsol.uk/IURhaines1956.htm" },
    { 80, "CNI NEWS: ISCNI News Center, Michael Lindemann ed. http://www.cninews.com/ .." },
    { 101, "http://www.visi.com/~jhenry/95-1r.htm#apr ISCNI from  Wanda Sudrala <WSudrala at AOL.com> Filed here: Newell95.TXT" },
    { 102, "ftp://ftp.eskimo.com/u/r/rmrice/ufo/reports/rep95a Filed here as SBarbara95.TXT" },
    { 103, "Huntsv1196.TXT on file here. Broken links on Internet." },
    { 104, "Julesburg75.TXT on file here. Web page taken down." },
    { 105, "MauriceLA95.TXT on file. Hardcopy from Joe Ritrovato. Original Web page was taken down (Mike Broussard)" },
    { 106, "http://www.ufoworld.co.uk/kathy.htm On file here as Stingray1974.txt" },
    { 107, "http://www.geocities.com/Area51/Rampart/2653/rep96c.txtOn file here: NMX030396.TXT" },
    { 108, "Michael Curta: Colorado Hotspots. Saved = GJCOLO95.TXT http://www.comufon.org/hotspots.htm" },
    { 109, "http://powerup.com.au/~smack/crashmex.htm Saved here as Chihuahua74.txt  (questionable)" },
    { 110, "JOE RITROVATO: San Francisco, CA: <joerit at flash.net> http://www.flash.net/~joerit/evidence.html#index9" },
    { 111, "PARANET and similar BBS Downloads. Files no longer accessible. Seeking better sources. [help!]" },
    { 113, "Lurid account on file here as L9DAKOTA.TXT taken from  http://cbr.nc.us.mensa.org/homepages/andypage/alsworth.htm" },
    { 114, "Lurid account on file here as TomsRvr.TXT taken from: http://www.shoah.free-online.co.uk/801/Abduct/hudsonvalley.html" },
    { 115, "Don Johnson: Edwards88Box.TXT on file here: from: http://www.ufobbs.com/txt1/080.html" },
    { 116, "Rustic account on file here as FtAdams1988.TXT from: http://area51.upsu.plym.ac.uk/~moosie/ufo/txt/sight/132.htm" },
    { 117, "http://www.geocities.com/Area51/Labyrinth/6897/main.htm Click on State / Country: search by date." },
    { 118, "http://www.execpc.com/~ahoj/wfiles/files/uf890318.htm also see -/uf890113.htm -/uf890810.htm -/uf8901xx.htm" },
    { 119, "http://www.xdream.freeserve.co.uk/UFOBase/V-Formations.htm" },
    { 121, "UFO's over New England Website ( Select by state ) http://www.geocities.com/Area51/Nova/8874/ufomenu.html" },
    { 122, "Highly doubtful: http://www.cseti.org/crashes/070.htm" },
    { 123, "http://tacna.infored.com.pe/ufo/videos/video2.html Video. Contact UFO Scotland for more info." },
    { 124, "www.shoah.free-online.co.uk/801/Abduct/mass2.html Long HUFON report on paper copy here." },
    { 125, "http://www.ufoworld.co.uk/j.htm : TwoHarbors95.TXT: http://www.visi.com/~jhenry/95-1r.htm#mar" },
    { 126, "http://enterprise.powerup.com.au/~auforn/160595.htm" },
    { 127, "http://home.flash.net/~joerit/docs2/world/winnipeg.htm" },
    { 129, "http://www.project1947.com/fig/1945a.htm" },
    { 128, "http://www.flash.net/~joerit/docs2/world/index9.htm" },
    { 131, "PROJECT 1947: www.project1947.com/fig/usaf14.htm Special page, June,2000" },
    { 132, "http://www.science-frontiers.com/sf034/sf034p22.htm" },
    { 140, "MALTA UFO RESEARCH: J.J.Mercieca. www.mufor.org email: mufor at maltanet.net" },
    { 145, "http://powerup.com.au/~smack/page9.htm  Australian catalog online." },
    { 160, "BASTERFIELD, KEITH: AUFORN group, with Diane Harrison. <tbknetw at fan.net.au>  http://www.fan.net.au/~tkbnetw/new" },
    { 161, "BASTERFIELD, KEITH: ENTCAT97.DOC: Catalog of Australian Entity and related reports: October 1997" },
    { 180, "Victorian UFO Research Society (VUFORS), Australia www.ozemail.com.au/~vufors" },
    { 190, "MUFON CATALOG download. Filed here as MUFONUFO.TXT" },
    { 200, "Chez HEIDI site: Alain Stauffer. GREPI / Switzerland  http://home.worldcom.ch/~dbenaroy/heidi.html" },
    { 210, "Jorge Martin: Puerto Rico: email <jmartin at coqui.net> and/or < evidencia at rforest.net >" },
    { 230, "New York MUFON. Larry Clark dir. <lclark at ibm.net> http://www.nymufon.org. Report NY sightings here!" },
    { 240, "Mark Cashman  email: <mcashman at IX.netcom.com> Website: http://www.temporaldoorway.com" },
    { 270, "Michel Deschamps: (Ontario) <ufoman at ican.net>" },
    { 280, "MUFON CT: Mark Cashman: Full online report is at: http://www.temporaldoorway.com/mufonct/report/991213/" },
    { 301, "Wallingford, NJ 1995: File here = WallingNJ95.TXT" },
    { 302, "1989RPTS.TXT filed here (Don Allen and many others) http://www.ecst.csuchico.edu/~num44/ufo/MUFON.89" },
    { 311, "Gilles MILOT: C:/DOWNLOAD/PERCE.TXT <aqu at videotron.ca>" },
    { 320, "Personal investigation by Dominique Weinstein. Filed here as Martigues48.txt" },
    { 404, "UFOZONE/Argentina email: ufozone at geocities.com www.geocities.com/CapeCanaveral/3823" },
    { 410, "UFOBC, British Columbia, Canada. Bill Oliver. www.ufobc.org" },
    { 411, "UFOBC-CANUFO/YUKON: Martin Jasek: <Martin at ufobc.org> www.ufobc.org/yukon/littlefox.html <canufo at egroups.com> <mjjasek at yknet.yk.ca>" },
    { 412, "UFOBC: Thorough study: www.ufobc.ca/yukon/22indx.htm" },
    { 413, "UFOBC/YUKON: Des Clark/Martin Jasek <Martin at ufobc.org> www.ufobc.org/yukon/closeupx.htm  <mjjasek at yknet.yk.ca>" },
    { 420, "Brian Vike: HBCCUFO <hbccufo at telus.net> BC/Canadian reports filed here by date." },
    { 450, "Paul Stonehill. Filed here: C:/DOWNLOAD/SOVWATER.TXT http://www.incredible.spb.ru/incr3.htm" },
    { 528, "MIAMI UFO CENTER: Dr. Virgilio Sanchez-Ocejo) ufomiami at aol.com" },
    { 516, "Maurizio VERGA: ITACAT Catalog online: http://www.ufo.it/english/main.htm" },
    { 531, "Blaine Wasylkiw, Yellowknife, NWT, Canada http://www.ssimicro.com/~ufoinfo" },
};

// 97: "OTHER PERIODICALS."
static const hatch_ref g_hatch_refs_97[] =
{
    {0, "Unknown or lost."},
    { 1, "UFO-NORWAY NEWS (Norway, in English) No. 1 1989. Mentz Kaarbo ed. PO Box 14; N-3133 DUKEN,Norway. 1-2 issues/year" },
    { 2, "SUFOI NEWS (in English) No.11 1990. Kim Moller Hansen ed. SUFOI, Postbox 6, DK-2820 GENTOFTE, Denmark. [now *U* ref. 146]" },
    { 3, "Journal fur UFO Forschung (JUFOF). Hans Werner Peiniger ed. ISBN 0723-7766.  GEP group, Postfach 2361, D-58473 Ludenscheid, Germany." },
    { 4, "CdU Internacional; Annual supplement to Cuadernos de Ufologia, Apartado 5.041, 39080 Santander, Spain. Jose Ruesga Montiel ed. 1995+96 only." },
    { 5, "URUGUAYAN UFO REPORTER. Mimeograph (Spanish) CIOVI; (JUN/JLY '90 only) PO Box 10833; Montevideo, Uruguay." },
    { 6, "UFO CONTACT (IGAP Newsletter); Ib Lauland ed. DENMARK. Quarterly. \"Dedicated to George Adamski.\"" },
    { 7, "CONTACTO OVNI. German Flores Trujillo ed. Editorial Mina SA de CV; Tokio 424, Cpl. Portales, Mexico D.F., MEXICO. (#23 only. Glossy covers)" },
    { 8, "UFO - Rivista di informazione Ufologica; 2 issues/year CISU. Giovanni Settimo ed.; C.P.82, 10100 Torino, ITALY. now: Primary ref /r209." },
    { 9, "CISU UFO Newsflash: (email distribution) Edoardo Russo Editor, Torino Italia  <e.russo at cisu.org>" },
    { 10, "Notiziario UFO. Centro Ufologico Nazionale (CUN). Roberto Pinotti ed. Via Oderica da Pordenone 36, 50127 Firenze, Italy. #110-11 only." },
    { 11, "UFO Especial. GEP / CBPDV ; Quarterly. A.J.Gevaerd ed. C.P. 2182, R.Bezerra de Menezes 68, 79021 Campo Grande, MS, Brazil." },
    { 12, "UFO MAGAZINE (UK). Quest International. P.O. Box 2, Grassington,Skipton,N.Yorks BD23 5UY. Graham Birdsall, ed. bimonthly." },
    { 13, "AS UFONOTAS, Boletim Informativo do CEU; Jean Alencar dir. C.P. 689; 60000 Fortaleza,Ceara, BRAZIL. 2/year. 16pg  mimeographed." },
    { 14, "ICAAR (Newsletter). Cheryl Powell & Linda Gaudet eds. PO Box 154. Goffstown, NH 03045. (v1 #3 1994 only.)" },
    { 15, "ALTERNATIVE PERCEPTIONS quarterly. Brent Raynes ed. 326 Haggard St. Waynesboro, TN 38485 (615) 722-5976. Publ./White Buffalo Books, Memphis." },
    { 16, "AUSTRALIAN INTERNATIONAL UFO FLYING SAUCER RESEARCH Inc Colin O.Norris, ed; GPO 2004, Adelaide, S.Australia 5001. (Subterranean reptiles etc.)" },
    { 17, "UFO NACHRICHTEN. 62 Weisbaden, Germany. (Ebenalp and Amriswil,SWZ see issue #9 pg.3) clippings /Bruno Mancusi)" },
    { 18, "GUB BULLETIN. (Friborg, SWZ = #7,1980. Le Paquier, SWZ = issue #5 1980.) French language." },
    { 19, "WELTRAUMBOTE: Monthly, Zurich, SWZ. Clipping/B.Mancusi. (Val Poschiavo: #56-57 Nov.-Dec 1960, p23.)" },
    { 20, "LA MUFON NEWSLETTER. Bimonthly.  W.L. \"Barney\" Garner editor. Email garner at eatel.net Final issue Nov-Dec, 1996 = Vol.8 #6. 752 Daventry Dr., Baton Rouge,LA. [Mansura rf]" },
    { 21, "SAMIZDAT QUARTERLY: Scott Corrales ed.: SPECIAL REPORT The GALICIAN UFO WAVE of 1995-96. (via Arcturus Books.)" },
    { 22, "SAMIZDAT QUARTERLY: Scott Corrales ed.: [Ecuador JUN96 = Fall'96 issue via Internet #70.]" },
    { 35, "FSR Special Issue #4: August 1971." },
    { 36, "FSR Special Issue #5: November 1973." },
    { 41, "J. Antonio Huneeus: Incident at Usovo. The ANOMALIST 7, Winter 1998-99. Dennis Stacy ed." },
    { 50, "QUEST (newsletter): UFO INTERNATIONAL. Allan Parsons ed.: Kingswood, Bristol, UK: 8 issues only, Jan '79 to Jan '81." },
    { 51, "PHENOMENA RESEARCH REPORT: UFO Reporting Center, Seattle, WA. Vol. 3 #1 (issue 25) 1977-8?" },
    { 52, "VSD Magazine, France: Special Issue. OVNIs Les Preuves Scientifiques; July 1998 (via D. Weinstein)" },
    { 54, "VSD Magazine, France: Special Issue. OVNIs: Nouvelles Evidences; Fall 2000 (via D. Weinstein)" },
    { 56, "VSD Magazine, France: Special Issue. OVNIs: 50 ans de Rapports Officiels; June 2002 (Bernard Thouanel)" },
    { 57, "VSD Mag., France: Special Issue. OVNIs: Les Premieres Signes; October 2002 (Bernard Thouanel)" },
    { 60, "UFO, Revista di Informazione Ufologica: CISU/Torino, IT No. 20 Dec.1997 /Edoardo Russo." },
    { 61, "UFO, Revista di Informazione Ufologica: CISU/Torino, IT No. 21 Nov.1998 /Edoardo Russo." },
    { 62, "UFO, Revista di Informazione Ufologica: CISU/Torino, IT No. 16 JLY 1995 pp. 25-30 Marco Orlandi & Renzo Cabassi." },
    { 70, "UFO HISTORICAL REVUE (UHR Qtrly) Barry Greenwood. Issue #6 Mar., 2000. POB 176, STONEHAM,MA  02180" },
    { 71, "UFO HISTORICAL REVUE (UHR Qtrly) Barry Greenwood. Issue #1 June, 1998. POB 176, STONEHAM,MA  02180" },
    { 72, "UFO HISTORICAL REVUE (UHR Qtrly) Barry Greenwood. Issue #8 Febr, 2001. POB 176, STONEHAM,MA  02180" },
    { 81, "English Mechanic #100 (16 Oct. 1914) pg. 256: Albert Alfred Buss: \"Cosmic and Terrestrial Flosam and Jetsam\" http://www.resologist.net/lands224.htm#N_3_" },
    { 82, "S.P.A.C.E. Bulletin #19: PJ47 post/Herb Taylor saved as 18JUN58.TXT here." },
};

// 98: "OTHER BOOKS", Misc. Books, reports, files & correspondance.
static const hatch_ref g_hatch_refs_98[] =
{
    { 8, "GUIEU, Jimmy: Les Soucoupes Volantes vient de un Autre Monde: pg 48. [from 2 scanned pages]"},
    { 9, "McCAMPBELL, James M.: Personal meetings/interviews." },
    { 10, "VALLEE, Jacques: Personal meetings and correspondance." },
    { 11, "STURROCK, Dr. Peter (Stanford, CA):  Personal meetings and correspondance." },
    { 12, "FOWLER, Raymond: The ANDREASSON AFFAIR; Prentice-Hall 1979 & Bantam 1980." },
    { 14, "WEINSTEIN, Dominique (Paris); Personal meetings and Correspondence." },
    { 15, "BLOECHER, Ted: THE STONEHENGE INCIDENTS. January 1975; report/investigations by Bloecher, Budd Hopkins and Jerry Stoehrer" },
    { 17, "HOPKINS, Budd: INTRUDERS; Random House 1987; Ballentine PB 1988. (Cathy Davis abds. \"Alien daughter\" pg. 221)" },
    { 18, "HOPKINS, Budd: WITNESSED: The True Story of the Brooklyn Bridge Abductions. Pocket Books, NY 1996 ISBN 0-671-56915-5 (Linda Cortile)" },
    { 22, "BERLITZ, Charles & MOORE, Wm: The ROSWELL INCIDENT; Putnam 1980 & Berkley Books 1988." },
    { 23, "BERLITZ, Charles: WORLD of STRANGE PHENOMENA; Fawcett Crest 1988 (Minot, ND shooting on pg. 243. Hog mutilation / Norway, SC on pg. 50)" },
    { 24, "FULLER, John G.: INTERRUPTED JOURNEY; 1966. Dell Books PB 1987. (Betty & Barney Hill. Entire book.)" },
    { 26, "O'BRIEN, Christopher: The MYSTERIOUS VALLEY (San Luis Vly, CO) 1996 St. Martins Paperbacks, NY." },
    { 28, "PAGET, Peter: The Welsh Triangle. Panther PB, Granada Publishing London 1979." },
    { 30, "EVANS, Hillary & STACY,Dennis eds.: UFO 1947-1997, 50 Years of Flying Saucers: John Brown Publ./Fortean Times London 1997: ISBN 1-870870-999" },
    { 34, "RULLAN, Tony: Retrieved lost BBK file: Dorion, ONT 1964 Paper copy on file." },
    { 39, "KLASS, Philip J.: UFO's EXPLAINED; Vintage Books/Random House, NY 1974  (Crowder/South Hill, SC 1967 p.135. Shoals, IN 1968 pg. 13 )" },
    { 41, "BLUM, Howard: OUT THERE; Simon & Schuster, NY HB 1990: (Elmwood, WI ref.)" },
    { 43, "BENDER, Albert K.: FLYING SAUCERS and the THREE MEN. 1962 & 1968 Paperback Library, NY. Foreward/Gray Barker. 160 pp." },
    { 44, "STRICKLAND: EXTRA-TERRESTRIALS ON EARTH. Grossett & Dunlap, NY 1977. [La Llagosta, Spain pg. 58]" },
    { 50, "ENGBERG, Robt. E.: REPORT on INVESTIGATION of SCANDIA, MN SIGHTINGS of 22MAR78; 22pp. Self published: 15FEB79." },
    { 51, "Personal acct: Michael Christol: http://www.icq.com <mchristo at mindspring.com>" },
    { 57, "DEFENSE NUCLEAR AGENCY docm# DNA 6035F: Castle Series Nuclear Weapons Tests. pp 64+274+341 c/o Daniel Wilson 15 Parkview Dr., Painesville, OH 44077" },
    { 60, "THE UFO ENIGMA. Report No. 83-205 SPR, Congressional Research Service, Library of Congress, June 20, 1983. UG 633 Marcia S. Smith /Aerospace, Science Policy Research Div." },
    { 65, "DAVID, Jay (ed.): The FLYING SAUCER READER;  Signet & New American Library 1967. 252 pp." },
    { 70, "HASSAL, Peter; The NZ FILES, UFO's in New Zealand; 1998 [1965 Stewart Isl. USO on Page 93.]" },
    { 72, "WHITE, Dale: IS SOMETHING UP THERE?; Doubleday 1968 & Scholastic PB 1969." },
    { 80, "RANDLES, Jenny: UFO Crash Landing? 1998 Blandford, UK ISBN 0-7137-2655-5. Full account/1980 Rendlesham events." },
    { 90, "DeMARY, Tom: 5OCT2002 by telescope: on file TDDisk.txt" },
    { 101, "BERLITZ, Charles: The DEVIL'S TRIANGLE; Wynwood Press 1989 NY, NY; ISBN 0-922066. Taki Kyoto Maru, pp 130-131." },
    { 105, "TRENCH, Brinsley Le Poer: OPERATION EARTH; Neville Spearman 1969 & Tandem, London 1974. (Belfast, N. Irl pg. 18)" },
    { 115, "HAINES, Richard Ph.D: The MELBOURNE EPISODE; LDA Press 1990. (Valentich/Bass Strait. Cape Otway pg. 159)" },
    { 148, "HAINES, Dr. Richard F.: Unpublished aircraft sightings files. (El Paso, TX 1956 pg 96.)" },
    { 152, "BOURRET, Jean-Claude: La NOUVELLE VAGUE des SOUCOUPES VOLANTES; Editions France-Empire, Paris. 1974 (Geneva, SWZ 1969 pg. 60)" },
    { 153, "BOURQUIN, Gilbert A.; L'INVISIBLE NOUS FAIT SIGNALE; Editions Robert SA, 2740 Moutier Switzerland 1968. (Lebanon, CT 1982 pg. 18)" },
    { 164, "VANCE, Adrian: UFO's, the EYE & CAMERA; (Kenai Peninsula photo pg 40. Sedona, AZ pg 49.)" },
    { 167, "De TURRIS, G. & FUSCO, S.: OBIETTIVO sugli UFO (1975) pg. 37. per Edoardo Russo. Gives 03MAY45 date for Eastern Pfalzerwald FBL incident."},
    {168, "VESCO, Renato: INTERCEPT but DON'T SHOOT; transl. from Italian. Goofy."},
    {169, "RANDLES, Jenny & Warrington, P.: SCIENCE and the UFO's; Basil Blackwell, NY 1985"},
    {170, "STANFORD, Ray: SOCORRO SAUCER in a PENTAGON PANTRY; HB Austin, TX Blueapple Books 1976. 211pp."},
    {174, "STEVENS, Wendell: UFO ABDUCTION AT MARINGA: UFO Photo Archives,  Tucson, AZ. (Sorocaba, Brazil Jan, 1979  pg99?)"},
    {176, "RANDLE, Kevin D: PROJECT BLUEBOOK EXPOSED. Marlowe & Co. NY 1997 ISBN 1-56924-746-3. Libr./Congress 97-72378."},
    {177, "RANDLES, Jenny & HOUGH, Peter: SPONTANEOUS HUMAN COMBUSTION; Berkley Books, NY 1992/94. (York, UK pg 147. Owestry, UK pg 103.)"},
    {178, "BERGIER, Jacques: E.T. VISITATIONS, PREHISTORIC TIMES to the PRESENT; J'ai Ju, France 1970 & H. Regnery, Chicago 1973, Signet, NY 1974. (Milan, 1491 pg 85. Esterhazy, pg 163)"},
    {179, "GREENFIELD, Irving A.: The UFO REPORT; Lancer Books, NY 1967. (USS Supply 1904 pg 59. Sag Harbor, NY 1966 pg 19)"},
    {193, "VALLEE, Jacques:The INVISIBLE COLLEGE; E. P. Dutton, NY 1975. Hardcover. (Washington, DC 1959 pg 74. Ashland, NE 1967 pg 57.)"},
    {197, "CATHIE, Bruce: HARMONIC 695; A & A Reed, Wellington, N. Z. 1971."},
    {202, "HAINES, Richard (ed): UFO PHENOMENA and the BEHAVIORAL SCIENTIST. Scarecrow Press, Metuchen, NJ 1979. (Medicine Bow, WY pg. 225)"},
    {203, "HAINES, Richard (via Dom. Weinstein) BLUEBOOK Reports taken from 16mm Microfilm at Maxwell AFB (partial)"},
    {210, "LEONE, Matteo: CISU Italy case summary. Saved here as C:\\DOWNLOAD\\SANTENA.TXT"},
    {219, "MAUGE, Claude: (RECONTRES de LYONS 1988); Sightings Catalog. (Tradate, ITL 1954 pg 74. Eschallens, SWZ pg 57)"},
    {222, "SMITH, Warren:  The BOOK of ENCOUNTERS. Zebra Books, Kensington Publ., NY 1976. PB 253pp. (Schirmer/Ashland, WI p87. Madison, WI Apr. 1970 pg. 114)"},
    {223, "SMITH, Willy: ON PILOTS and UFO's. UNICAT Project, 8011 SW 189th St. Miami, FL 33175 USA. March 1997."},
    {226, "KENT, Malcom: The TERROR ABOVE US. Tower Books, NY 1967 (Entire PB book: IBM programmers abducted.)"},
    {228, "ERSKINE, Allen Louis: THEY ARE THEY WATCHING US. Belmont Tower Books, NY 1967."},
    {235, "MACDONALD, James E.: Report to the American Society of Newspaper Editors. (Owasca, IN 1958 pg 23. Davis, CA pg 19)"},
    {243, "RANDLES, Jenny: UFO CONSPIRACY - the first 40 years. Javelin, London 1988. (Dee Estuary, UK July 1981 pg. 127)"},
    {251, "FEINDT, Carl: C:/DOWNLOADS:WOLFVILLE.TXT from original newspaper account CR: Don Ledger."},
    {5, "JACOBS, David: The UFO CONTROVERSY in AMERICA. Signet 1976 / Indiana University Press 1975. LoC #74-11886"},
    {301, "Email Interview: Bricks 1917 at aol.com. Filed here under C:\\Ongoing\\BRICKS1,2,3.TXT"},
    {302, "Email Interview: Jim Mortellaro. Address on file. Filed here under HARTSD98.TXT"},
    {311, "Email Interview: Joe Foster/USCG filed here under Mellon70a.txt"},
    {320, "ALDRICH, Jan: Interview with witness etc. Filed here as Xmasisland1967.txt"},
};

static const char* g_hatch_refs_tab[256];

static const char* g_pHatch_flag_descs[64] =
{
    "MAP: Coords known",
    "GND: Observer(s) on ground",
    "CST: Observer(s) in coastal area/just offshore",
    "SEA: Observer(s) at sea",
    "AIR: Observer(s) on aircraft",
    "MIL: Observer(s) military",
    "CIV: Observer(s) civilian",
    "HQO: High-quality observer(s)",

    "SCI: Scientist involvement",
    "TLP: Telepathy",
    "NWS: News media report",
    "MID: Likely mis-identification",
    "HOX: Suspicion of hoax",
    "CNT: Contactee related",
    "ODD: Atypical/Forteana/paranormal",
    "WAV: Wave/cluster/flap",

    "SCR: Classic saucer/disk/sphere shaped UFO",
    "CIG: Torpedo/cigar/cylinder shaped UFO",
    "DLT: Delta/vee/boomerang/rectangular shaped UFO",
    "NLT: Nightlights/points of light",
    "PRB: Probe: Possibly remote controlled",
    "FBL: Fireball: Blazing undistinguished form/orb.",
    "SUB: Submersible: Rises from or submerges into water",
    "NFO: No craft seen",

    "OID: Humanoid: Small alien or \"Grey\"",
    "RBT: Possible robot or \"Grey\", mechanical motions",
    "PSH: Pseudo-human: Clone/organic robot/human-like",
    "MIB: Men in Black",
    "MON: Monster: Life form fits no category",
    "GNT: Giant: Large/tall alien",
    "FIG: Undefined or poorly seen figure/entity",
    "NOC: No entity/occupant seen by observer(s)",

    "OBS: Observation/chasing vehicles",
    "RAY: Odd light/searchlight/beam/laser-like",
    "SMP: Sampling of plant/animal/soil/tissue/specimens",
    "MST: Missing time or time anomaly",
    "ABD: Human or animal abduction",
    "OPR: Operations on humans/animal mutilation/surgery",
    "SIG: Signals/responses to/from/between UFO's/occupants/observers",
    "CVS: Conversation/communication",

    "NUC: Nuclear facility related",
    "DRT: Dirt/soil traces/marks/footprints etc.",
    "VEG: Plants affected or sampled/crop circles",
    "ANI: Animals affected or injuries/marks",
    "HUM: Humans affected: Injury/burns/marks/abduction/death",
    "VEH: Vehicle affected: Marks/damage/EME effects",
    "BLD: Building/man-made structure/roads/power lines",
    "LND: UFO landing or any part touches ground",

    "PHT: Photos/movies/videos taken",
    "RDR: Radar traces/blips",
    "RDA: Radiation/high energy fields detected",
    "EME: Electro-Magnetic Effects",
    "TRC: Physical traces",
    "TCH: New technical details/clues",
    "HST: Historical account",
    "INJ: Injuries, illness/death, mutilations",

    "MIL: Military investigation",
    "BBK: Project BLUEBOOK",
    "GSA: Covert Security Agency",
    "OGA: Non-Covert Government Agencies",
    "SND: UFO sounds heard or recorded",
    "ODR: Odors associated with UFO's",
    "COV: Indication of coverup",
    "CMF: Camouflage/disguise"
};

struct hatch_abbrev
{
    const char* pAbbrev;
    const char* pExpansion;
    bool m_forbid_firstline = false;
};

static const hatch_abbrev g_hatch_abbreviations[] =
{
    { "1000S",                                      "thousands" },
    { "100S",                                       "hundreds" },
    { "10MIN",                                      "10 minutes" },
    { "10S",                                        "tens" },
    { "15HRS",                                      "15 hours" },
    { "15MIN",                                      "15 minutes" },
    { "180DGR",                                     "180 degree" },
    { "1HI",                                        "1 high" },
    { "1HR",                                        "one hour" },
    { "1MI",                                        "1 mile" },
    { "2HR",                                        "2 hour" },
    { "2HRS",                                       "2 hours" },
    { "2MI",                                        "2 miles" },
    { "2ND",                                        "2nd" },
    { "3HR",                                        "3 hour" },
    { "3HRS",                                       "3 hours" },
    { "3MIN",                                       "3 minutes" },
    { "4MI"   ,                                     "4 miles" },
    { "4RTH",                                       "fourth" },
    { "5MIN",                                       "5 minutes" },
    { "8MIN",                                       "8 minutes" },
    { "AA",                                         "Anti-Aircraft guns/teams", true },
    { "AAB",                                        "Army Air Base" },
    { "AAF",                                        "Army Air Force (AAF)" },
    { "AB",                                         "airbase", true },
    { "ABD",                                        "abduction" },
    { "ABD'D",                                      "abducted" },
    { "ABD.",                                       "abduction/abducted" },
    { "ABDS",                                       "abductions" },
    { "ABO.",                                       "aborigine" },
    { "ABQ",                                        "Albuquerque" },
    { "ABQJ",                                       "Albuquerque Journal", },
    { "ABS",                                        "absolute(ly)" },
    { "ABS.",                                       "absolute(ly)" },
    { "ACCEL",                                      "acceleration" },
    { "ACCEL.",                                     "acceleration" },
    { "ACCELERTN",                                  "acceleration" },
    { "ACCELS",                                     "accelerations" },
    { "ACCT",                                       "account" },
    { "ACCT.",                                      "account" },
    { "ACFT",                                       "aircraft" },
    { "ACRFT",                                      "aircraft" },
    { "ACRS",                                       "across" },
    { "ACS",                                        "across" },
    { "ADC",                                        "Air Defense Command" },
    { "ADF",                                        "Automatic Direction Finder (ADF)" },
    { "ADM",                                        "Admiral" },
    { "AEC",                                        "Atomic Energy Commission (AEC)" },
    { "AERO.",                                      "aerospace" },
    { "AEROFLT",                                    "Aeroflot" },
    { "AEROSPC",                                    "Aerospace" },
    { "AF",                                         "Air Force", true },
    { "AFB",                                        "Air Force Base", },
    { "AFOSI",                                      "USAF Office of Special Investigation (AFOSI)" },
    { "AFSR",                                       "Australian Flying Saucer Review (AFSR)" },
    { "AGCY",                                       "agency" },
    { "AGNST",                                      "against" },
    { "AIRCRFT",                                    "aircraft" },
    { "AIRPT",                                      "airport" },
    { "AL",                                         "airline(s)/airliner", true },
    { "ALBQQ",                                      "Albuquerque" },
    { "ALBUQQ",                                     "Albuquerque" },
    { "ALLO",                                       "all over/all about" },
    { "ALT",                                        "altitude" },
    { "ALT.",                                       "altitude" },
    { "ALTERNATLY",                                 "alternately" },
    { "ALTITUD",                                    "altitude" },
    { "ALUM",                                       "aluminum" },
    { "ALUM.",                                      "aluminum" },
    { "ALUMN",                                      "aluminum" },
    { "ALUMN.",                                     "aluminum" },
    { "AM.",                                        "amateur" },
    { "AMMO",                                       "ammunition" },
    { "ANIMLS",                                     "animals" },
    { "ANON",                                       "anonymous" },
    { "ANON.",                                      "anonymous" },
    { "ANTNNAS",                                    "antennas" },
    { "AP",                                         "Associated Press (AP)", true },
    { "APELIKE",                                    "ape-like" },
    { "APPR",                                       "appear" },
    { "APPRCHD",                                    "approached" },
    { "APR",                                        "April" },
    { "APR.",                                       "April" },
    { "APRL",                                       "April" },
    { "APROX",                                      "approximate" },
    { "APRX",                                       "approximate" },
    { "APT",                                        "airport/apartment" },
    { "APT.",                                       "airport/apartment" },
    { "APTS",                                       "airports/apartments" },
    { "ARCS",                                       "arcs" },
    { "ARK",                                        "ark" },
    { "ARRECIBO",                                   "Arecibo" },
    { "ASSY",                                       "Assembly" },
    { "ASTR",                                       "astronomer" },
    { "ASTRON",                                     "astronomer" },
    { "ASTRON.",                                    "Astronomer" },
    { "ASTRONMRS",                                  "astronomers" },
    { "ASTRONOMR",                                  "astronomer" },
    { "ASTRONOMRS",                                 "astronomers" },
    { "ASTRONS",                                    "astronomers" },
    { "ASTRS",                                      "astronomers" },
    { "ATC",                                        "Air Traffic Controller" },
    { "ATCS",                                       "Air Traffic Controllers" },
    { "ATEST",                                      "A-test" },
    { "ATIC",                                       "USAF Tech. Intel. Center. (ATIC)" },
    { "ATL",                                        "Atlantic Ocean." },
    { "AUG",                                        "August" },
    { "AUG.",                                       "August" },
    { "AUSTRL",                                     "Australia" },
    { "AVAIL",                                      "available" },
    { "BACKWD",                                     "backward" },
    { "BBK",                                        "Blue Book" },
    { "BBK#",                                       "Blue Book Case #" },
    { "BDB",                                        "apparent size" },
    { "BELG",                                       "Belgium" },
    { "BERLINER",                                   "Don Berliner" },
    { "BHND",                                       "behind" },
    { "BINOC",                                      "(seen thru) binoculars" },
    { "BINOCS",                                     "(seen thru) binoculars" },
    { "BLD",                                        "buildings(s)" },
    { "BLDG",                                       "building" },
    { "BLDGS",                                      "buildings" },
    { "BLK",                                        "black" },
    { "BLK.",                                       "black" },
    { "BLU",                                        "blue" },
    { "BLU)",                                       "blue)" },
    { "BLW",                                        "below" },
    { "BOTTM",                                      "bottom" },
    { "BR.",                                        "bridge" },
    { "BRILL.",                                     "brilliant" },
    { "BRIT.",                                      "British" },
    { "BRITE",                                      "bright" },
    { "BRITENS",                                    "brightens" },
    { "BRITS",                                      "British" },
    { "BRN",                                        "brown colored" },
    { "BRT",                                        "bright/brilliant" },
    { "BRZL",                                       "Brazil" },
    { "BTM",                                        "bottom/underside" },
    { "BTWN",                                       "between" },
    { "C.FORT",                                     "Charles Fort" },
    { "CAL",                                        "California" },
    { "CALC",                                       "Calculated" },
    { "CALIF",                                      "California" },
    { "CANT",                                       "can't" },
    { "CAPT",                                       "Captain" },
    { "CCL",                                        "circle" },
    { "CCLS",                                       "circles" },
    { "CCW",                                        "counterclockwise" },
    { "CF",                                         "can't find/locate" },
    { "CGR",                                        "cylinder/cigar-shape" },
    { "CGRS",                                       "cigars" },
    { "CH.FORT",                                    "Charles Fort" },
    { "CHEM",                                       "chemical" },
    { "CHEM.",                                      "chemical" },
    { "CHP",                                        "California Highway Patrol (CHP)" },
    { "CIG",                                        "cigar shape" },
    { "CIR",                                        "circle" },
    { "CIR.",                                       "circular" },
    { "CIRC",                                       "circular" },
    { "CIRC.",                                      "circular" },
    { "CIRCL",                                      "circular" },
    { "CIRCL.",                                     "circular" },
    { "CIRCLR",                                     "circular" },
    { "CIRCLS",                                     "circles" },
    { "CIRCS",                                      "circles" },
    { "CIV",                                        "civil" },
    { "CIV.",                                       "civil" },
    { "CIVS",                                       "civilians" },
    { "CLDR",                                       "colored" },
    { "CLR",                                        "color" },
    { "CLR.",                                       "clr." }, // unclear
    { "CLRD",                                       "colored" },
    { "CLRS",                                       "color(s)" },
    { "CMF",                                        "camouflage" },
    { "CMPLX",                                      "complex" },
    { "CMPTR",                                      "computer" },
    { "CNT",                                        "contactee" },
    { "CNTR",                                       "controller" },
    { "CNTRL",                                      "central" },
    { "CO.",                                        "Co." },
    { "COASLINE",                                   "coastline" },
    { "COL",                                        "Col." },
    { "COLLISN",                                    "collision" },
    { "COMM",                                       "commercial aircraft" },
    { "COMM'R",                                     "commissioner" },
    { "COMM.",                                      "Comm." },
    { "COMPLX",                                     "complex" },
    { "CONPLETELY",                                 "completely" },
    { "CONSTR",                                     "construction" },
    { "CONSTRUCTN",                                 "construction" },
    { "CONTRL",                                     "control" },
    { "CONTRUCTION",                                "construction" },
    { "CONVRSN",                                    "conversion" },
    { "COORD",                                      "coordinate" },
    { "COORDS",                                     "coordinates" },
    { "COORDS.",                                    "coordinates" },
    { "COPTER",                                     "helicopter" },
    { "COPTERS",                                    "helicopters" },
    { "CORNFLD",                                    "cornfield" },
    { "COV",                                        "evidence of official coverup" },
    { "COVRS",                                      "covers" },
    { "CPT",                                        "captain" },
    { "CRCL",                                       "circle" },
    { "CRCLS",                                      "circles" },
    { "CRIFO",                                      "Civilian Research, Interplanetary Flying Objects (CRIFO)" },
    { "CROC",                                       "crocodile" },
    { "CTR",                                        "center" },
    { "CTRL",                                       "control" },
    { "CVL",                                        "civil" },
    { "CVR",                                        "cover" },
    { "CVRG",                                       "coverage" },
    { "CVS",                                        "conversation (any communication between us and them)" },
    { "CW",                                         "clockwise" },
    { "CYL",                                        "cylinder/cylindrical object" },
    { "CYL.",                                       "cylinder/cylindrical object" },
    { "CYLS",                                       "cylinders" },
    { "D.BERLINER",                                 "Don Berliner" },
    { "DAYLITE",                                    "daylight" },
    { "DBL",                                        "double" },
    { "DBL.",                                       "double" },
    { "DEC",                                        "December" },
    { "DEC.",                                       "December" },
    { "DECENDING",                                  "descending" },
    { "DEF.",                                       "definitely" },
    { "DEGR",                                       "degrees" },
    { "DENVER.CO",                                  "Denver, CO" },
    { "DEPT",                                       "Department" },
    { "DEPT.",                                      "Department" },
    { "DEPTS",                                      "Departments" },
    { "DESCR",                                      "description" },
    { "DESCRIPT",                                   "description" },
    { "DESCRIPTS",                                  "descriptions" },
    { "DEVAUGHN",                                   "Devaughn" },
    { "DIAM",                                       "diameter" },
    { "DIAMND",                                     "diamond" },
    { "DIFF",                                       "different" },
    { "DIFF.",                                      "different" },
    { "DIPL",                                       "diplomatic" },
    { "DIR",                                        "direction" },
    { "DIR.",                                       "direction" },
    { "DIRN",                                       "direction" },
    { "DIRS",                                       "directions" },
    { "DIRS.",                                      "directions" },
    { "DIST",                                       "distant" },
    { "DISTANC",                                    "distance" },
    { "DISTANGE",                                   "distance" },
    { "DISTR",                                      "DISTR" },
    { "DLT",                                        "delta/triangle/box-like craft" },
    { "DLT(?)",                                     "delta/triangle/box-like craft (?)" },
    { "DLTS",                                       "delta/triangle/box-like crafts" },
    { "DMND",                                       "diamond" },
    { "DNE",                                        "does not exist" },
    { "DOC",                                        "document" },
    { "DOCS",                                       "documents" },
    { "DOESNT",                                     "doesn't" },
    { "DONT",                                       "don't" },
    { "DR",                                         "Dr." },
    { "DR.",                                        "Dr." },
    { "DRK",                                        "dark" },
    { "DRK.",                                       "dark" },
    { "DRT",                                        "dirt/roadway" },
    { "DRVR",                                       "driver" },
    { "DSK",                                        "disk" },
    { "DSKS",                                       "disks" },
    { "DSRT",                                       "desert" },
    { "DTL",                                        "no detail" },
    { "DUR",                                        "duration" },
    { "E",                                          "east" },
    { "E.",                                         "east" },
    { "EAL",                                        "Eastern Airlines" },
    { "EARTHLITES",                                 "earthlights" },
    { "EFCT",                                       "effect" },
    { "ELE",                                        "electric" },
    { "ELEC",                                       "electric" },
    { "ELEC.",                                      "electric" },
    { "ELECT",                                      "electric" },
    { "ELECTR",                                     "electric" },
    { "ELECTRCL",                                   "electrical" },
    { "ELEM",                                       "electro-magnetic" },
    { "elem.",                                      "elementary" },
    { "ELLIPTCL",                                   "elliptical" },
    { "ELSEWHR",                                    "elsewhere" },
    { "EME",                                        "electro-magnetic effect (EME)" },
    { "EME.",                                       "electro-magnetic effect (EME)" },
    { "EME'S",                                      "electro-magnetic effects (EME)" },
    { "EMES",                                       "malfunctions due to EME (electro-magnetic effects)" },
    { "EMP",                                        "empire" },
    { "EMPL",                                       "employ" },
    { "ENCY.",                                      "UFO Encyclopedia (book)" },
    { "ENE",                                        "east-northeast" },
    { "ENG",                                        "engineer", true },
    { "EQL",                                        "equilateral/equal" },
    { "EQL.",                                       "equilateral/equal" },
    { "ESTM.",                                      "estimated" },
    { "ETC",                                        "etc." },
    { "EVES",                                       "evenings" },
    { "EXECS",                                      "executives" },
    { "EXPL",                                       "explosive" },
    { "EXPLAN",                                     "explanation" },
    { "F86S",                                       "F86's" },
    { "FBL",                                        "fireball" },
    { "FBLS",                                       "fireballs" },
    { "FBLS.",                                      "fireballs" },
    { "FCL",                                        "facility/installation" },
    { "FCLS",                                       "facilities/installations" },
    { "FCLTY",                                      "facility" },
    { "FEB",                                        "February" },
    { "FEB.",                                       "February" },
    { "FEM",                                        "female" },
    { "FIG",                                        "figure" },
    { "FIG.",                                       "figure" },
    { "FIGS",                                       "figure(s)" },
    { "FLASHLITE",                                  "flashlight" },
    { "FLASHLITES",                                 "flashlights" },
    { "FLD",                                        "field" },
    { "FLITE",                                      "flight" },
    { "FLITEPATH",                                  "flight path" },
    { "FLITES",                                     "flights" },
    { "FLOODLITE",                                  "floodlight" },
    { "FLOODLITES",                                 "floodlights" },
    { "FLOURSCNT",                                  "fluorescent" },
    { "FLT",                                        "flight" },
    { "FLTS",                                       "flights" },
    { "FM",                                         "from", true },
    { "FOLO",                                       "follow" },
    { "FOLO'D",                                     "followed" },
    { "FOLOS",                                      "follows" },
    { "FOO",                                        "Foo Fighters" },
    { "FORMN",                                      "formation" },
    { "FORMNS",                                     "formations" },
    { "FOTO",                                       "photograph" },
    { "FOTOG",                                      "photograph" },
    { "FOTOGRAPHERS",                               "photographers" },
    { "(FOTOS",                                     "(photographs" },
    { "FOTOS",                                      "photographs" },
    { "FOTOS(LOST)",                                "photographs (lost)" },
    { "FPRINT",                                     "footprint" },
    { "FPRINTS",                                    "footprints" },
    { "FR.",                                        "Fr." },
    { "FRAGM",                                      "fragment" },
    { "FRAGS",                                      "fragments" },
    { "FREQ",                                       "frequency" },
    { "FREQ.",                                      "frequency" },
    { "FREQS",                                      "frequencies" },
    { "FREQS.",                                     "frequencies" },
    { "FRM",                                        "from" },
    { "FRMN",                                       "formation" },
    { "FRWY",                                       "freeway" },
    { "FSLAGE",                                     "fuselage" },
    { "FSLG",                                       "fuselage" },
    { "FSR",                                        "Flying Saucer Review (FSR)" },
    { "FT",                                         "Ft.", true },
    { "FT.",                                        "Ft." },
    { "FTBALL",                                     "football" },
    { "FTPRINTS",                                   "footprints" },
    { "FUSILAGE",                                   "fuselage" },
    { "FUSLG",                                      "fuselage" },
    { "FWD",                                        "forward" },
    { "FWY",                                        "freeway" },
    { "GEOM",                                       "geometric" },
    { "GEOM.",                                      "geometric" },
    { "GIS",                                        "soldiers" },
    { "GLIDS",                                      "glides" },
    { "GLO",                                        "glowing" },
    { "GLO.",                                       "glowing" },
    { "GLOB",                                       "globular" },
    { "GLOW.",                                      "glowing" },
    { "GND",                                        "ground" },
    { "GND.",                                       "ground" },
    { "GNDS",                                       "grounds" },
    { "GNDS)",                                      "grounds)" },
    { "GOC",                                        "Ground Observer Corps (GOC)" },
    { "GOV",                                        "government" },
    { "GOV'T",                                      "government" },
    { "GOVT",                                       "government" },
    { "GOVT.",                                      "government" },
    { "GRD",                                        "ground" },
    { "GRN",                                        "green" },
    { "GRN.",                                       "green" },
    { "GRND",                                       "grounded" },
    { "GRNDS",                                      "grounds" },
    { "GRP",                                        "group" },
    { "GRPS",                                       "groups" },
    { "GRY",                                        "grey" },
    { "GSA",                                        "Government Security Agency" },
    { "GT",                                         "great", true },
    { "GT.",                                        "great" },
    { "HDLITES",                                    "headlights" },
    { "HEADLITE",                                   "headlight" },
    { "HEADLITES",                                  "headlights" },
    { "HEMISPHR",                                   "hemisphere" },
    { "HI",                                         "high", true },
    { "HIQ.",                                       "HIQ." },
    { "HIV",                                        "high tension power lines" },
    { "HORIZ",                                      "horizontal" },
    { "HORIZNTAL",                                  "horizontal" },
    { "HORIZNTL",                                   "horizontal" },
    { "HORZNTL",                                    "horizontal" },
    { "HOVR",                                       "hover" },
    { "HOVRS",                                      "hovers" },
    { "HOX",                                        "possible hoax" },
    { "HQ",                                         "headquarters", true },
    { "HRS",                                        "hours" },
    { "HRXN",                                       "horizontal" },
    { "HRZN",                                       "horizon" },
    { "HRZNTAL",                                    "horizontal" },
    { "HRZNTL",                                     "horizontal" },
    { "HS",                                         "High School", true },
    { "HST",                                        "history" },
    { "HUM",                                        "humming" },
    { "HV",                                         "HV", true },
    { "HVR",                                        "hover" },
    { "HVRS",                                       "hovers" },
    { "HVRS.",                                      "hovers" },
    { "HWY",                                        "highway" },
    { "HWY.",                                       "highway" },
    { "HZNTL",                                      "horizontal" },
    { "HZTL",                                       "horizontal" },
    { "IDENT",                                      "identical" },
    { "IDENT.",                                     "identical" },
    { "IFS",                                        "Inforespace Review (SOBEPS Group, Belgium)" },
    { "INC",                                        "including/include" },
    { "INCL",                                       "include/including" },
    { "INCOHERANT",                                 "incoherent" },
    { "INCOHERANTLY",                               "incoherently" },
    { "INCRED",                                     "incredible" },
    { "INCRED.",                                    "incredible" },
    { "INDEPENDANT",                                "independent" },
    { "INDEPENDANTLY",                              "independently" },
    { "IND.",                                       "ind." },  //unclear
    { "INJ",                                        "injury/sickness/death or mutilations" },
    { "INOP",                                       "inoperative" },
    { "INQ",                                        "inquire" },
    { "INQR",                                       "inquiry" },
    { "INS",                                        "inspect" },
    { "INSP",                                       "inspect" },
    { "INSTR",                                      "instrument" },
    { "INSTR'S",                                    "instruments" },
    { "INSTRM",                                     "instrument" },
    { "INSTRMNTS",                                  "instruments" },
    { "INSTRMS",                                    "instruments" },
    { "INSTRUCTR",                                  "instruction" },
    { "INSTRUMNT",                                  "instrument" },
    { "INSTRUMNTS",                                 "instruments" },
    { "INSTRUMS",                                   "instruments" },
    { "INSTUMENTS",                                 "instruments" },
    { "INT",                                        "intelligence" },
    { "INT.",                                       "intelligence" },
    { "INTERMITTANT",                               "intermittent" },
    { "INTERMTT",                                   "intermittent" },
    { "INTERNL",                                    "internal" },
    { "INTERVLS",                                   "intervals" },
    { "INTROFFC",                                   "interoffice" },
    { "INTVLS",                                     "intervals" },
    { "INV",                                        "investigation/investigators" },
    { "INV'D",                                      "investigated" },
    { "INVEST.",                                    "investigation" },
    { "INVESTG",                                    "investigate" },
    { "INVESTGN",                                   "investigation" },
    { "INVESTICATION",                              "investigation" },
    { "INVESTIGATN",                                "investigation" },
    { "INVESTIGATR",                                "investigator" },
    { "INVIS",                                      "invisible" },
    { "INVISIBL",                                   "invisible" },
    { "INVOLV",                                     "involve" },
    { "INVOLVEMT",                                  "involvement" },
    { "INVSBL",                                     "invisible" },
    { "INVSBLE",                                    "invisible" },
    { "INVSGN",                                     "investigation" },
    { "INVST",                                      "investigate" },
    { "INVSTG",                                     "investigate" },
    { "INVSTGN",                                    "investigating" },
    { "INVSTN",                                     "investigation" },
    { "INVs",                                       "INVs" }, // dunno
    { "IRREG.",                                     "irregular" },
    { "ITAN.",                                      "Italian" },
    { "IUR",                                        "International UFO Reporter (CUFOS Group, USA)" },
    { "JAN",                                        "Jan" },
    { "JAN.",                                       "January" },
    { "JLY",                                        "July" },
    { "JORNAL",                                     "Journal" },
    { "JRNL",                                       "Journal" },
    { "JUL",                                        "July" },
    { "JUL.",                                       "July" },
    { "JUN",                                        "June" },
    { "JUN.",                                       "June" },
    { "K.ARNOLD",                                   "Kenneth Arnold" },
    { "KITELIKE",                                   "kite-like" },
    { "L.A.TIMES",                                  "LA Times" },
    { "LBS",                                        "pounds" },
    { "LCL",                                        "local (as a local wave)" },
    { "LDLN",                                       "Lumieres dans la Nuit (monthly, Paris)" },
    { "LITE",                                       "light" },
    { "LITEBEAM",                                   "light-beam" },
    { "LITED",                                      "lighted" },
    { "LITEHOUSE",                                  "lighthouse" },
    { "LITENING",                                   "lightening" },
    { "LITES",                                      "lights" },
    { "LITES.",                                     "lights" },
    { "LITING",                                     "lighting" },
    { "LITS",                                       "lights" },
    { "LK",                                         "Lake", true },
    { "LL",                                         "longitude & latitude coords." },
    { "LND",                                        "landing" },
    { "LO",                                         "low", true },
    { "LOC",                                        "location" },
    { "LOCs",                                       "locations" },
    { "LOCs.",                                      "locations" },
    { "LOC.",                                       "location" },
    { "LOK",                                        "location" },
    { "LRG",                                        "large" },
    { "LRG.",                                       "large" },
    { "LRGE",                                       "large" },
    { "LRGR",                                       "larger" },
    { "LT",                                         "Lt.", true },
    { "LT.",                                        "Lt." },
    { "LUM",                                        "luminous" },
    { "LUM.",                                       "luminous" },
    { "LUMN",                                       "luminous/glowing" },
    { "LUMN.",                                      "luminous" },
    { "LUMN.SLVR",                                  "luminous silver" },
    { "LVL",                                        "level" },
    { "LVLS",                                       "levels" },
    { "LVS",                                        "leaves (something behind)" },
    { "LVTD",                                       "levitated" },
    { "LVTN",                                       "Levitation" },
    { "MAGN",                                       "magnetic/magnitude" },
    { "MAGN.",                                      "magnetic" },
    { "MAGNETIZD",                                  "magnetized" },
    { "MAGNETZD",                                   "magnetized" },
    { "MAGNTZD",                                    "magnetized" },
    { "MAJ.",                                       "Maj." },
    { "MANUSCRIPT.",                                "manuscript" },
    { "MAR",                                        "March" },
    { "MAR.",                                       "March" },
    { "MATS",                                       "Military Air Transport Service (MATS, US)" },
    { "MAY",                                        "May" },
    { "MBIKES",                                     "motorbikes" },
    { "MC",                                         "motorcycle", true },
    { "MCYCL",                                      "motorcycle" },
    { "MCYCLE",                                     "motorcycle" },
    { "MCYCLES",                                    "motorcycles" },
    { "MDS",                                        "doctors" },
    { "MECH",                                       "mechanical" },
    { "MECH.",                                      "mechanical" },
    { "MED",                                        "medical" },
    { "MED.",                                       "medical" },
    { "MEM",                                        "memory" },
    { "MEX",                                        "Mexico" },
    { "MGR",                                        "manager" },
    { "MI",                                         "mile(s)", true },
    { "MI.",                                        "mile(s)" },
    { "MIBS",                                       "Men in Black" },
    { "MID",                                        "misidentified (conventional phenomena)" },
    { "MIDNITE",                                    "midnight" },
    { "MIL",                                        "military" },
    { "MIL.",                                       "military" },
    { "MIN.",                                       "min." },
    { "MINS",                                       "minutes" },
    { "MJ",                                         "MJ" },
    { "MJR",                                        "Major" },
    { "MLT.",                                       "Military" },
    { "MNMT",                                       "monument" },
    { "MNRS",                                       "maneuvers" },
    { "MNV",                                        "maneuver" },
    { "MNVR",                                       "maneuver" },
    { "MNVRBLE",                                    "maneuverable" },
    { "MNVRING",                                    "maneuvering" },
    { "MNVRS",                                      "maneuvers" },
    { "MNVRS.",                                     "maneuvers." },
    { "MOD",                                        "Ministry of Defense (MoD, Britain)" },
    { "MON",                                        "monster" },
    { "MONS",                                       "monster(s)" },
    { "MOONLITE",                                   "moonlight" },
    { "MOONSIZE",                                   "moon-size" },
    { "MOTHERSHIP'HVRS",                            "mothership hovers" },
    { "MOTO",                                       "mopod/motorscooter/motorbike" },
    { "MR",                                         "Mr.", true },
    { "MR.",                                        "Mr." },
    { "MRS",                                        "Mrs." },
    { "MRS.",                                       "Mrs." },
    { "MS",                                         "Ms.", true },
    { "MS.",                                        "Ms." },
    { "MSG",                                        "message" },
    { "MSITES",                                     "missile sites" },
    { "MST",                                        "missing time" },
    { "MST?",                                       "missing time?" },
    { "MT",                                         "Mt." },
    { "MT.",                                        "Mt." },
    { "MTL",                                        "metal" },
    { "MTL.",                                       "metallic" },
    { "MTLC",                                       "metallic" },
    { "MTLS",                                       "MTLS" },
    { "MTN",                                        "mountain" },
    { "MTN.",                                       "mountain" },
    { "MTNS",                                       "mountains" },
    { "MTNS.",                                      "mountains" },
    { "MTNSIDE",                                    "mountainside" },
    { "MTR",                                        "meter" },
    { "MTRCYCLE",                                   "motorcycle" },
    { "MTRS",                                       "meters" },
    { "MULT",                                       "multiple" },
    { "MULT.",                                      "multiple" },
    { "MULTICLR",                                   "multicolor" },
    { "MULTICLRD",                                  "multicolored" },
    { "MULTICOLRD",                                 "multicolored" },
    { "MUTL'd",                                     "mutilated" },
    { "MUT'D",                                      "mutilated" },
    { "MUTD",                                       "mutilated" },
    { "MUTL",                                       "mutilation" },
    { "MUTL.",                                      "mutilation" },
    { "MUTLNS",                                     "mutilations" },
    { "MUTS",                                       "mutilations" },
    { "MVR",                                        "maneuver" },
    { "MVRS",                                       "maneuvers" },
    { "MNVRable",                                   "maneuverable" },
    { "N",                                          "north" },
    { "N.",                                         "north" },
    { "N/",                                         "north of" },
    { "N>",                                         "from the north to" },
    { "N>S",                                        "from north to south" },
    { "NAT'L",                                      "National" },
    { "NATL",                                       "National" },
    { "NAV",                                        "navigator" },
    { "NAVGR",                                      "navigator" },
    { "NAVIGN.",                                    "navigation" },
    { "NE",                                         "northeast" },
    { "NFD",                                        "No further details [in]" },
    { "NFD.",                                       "No further details in available sources." },
    { "NFG",                                        "no good/useless/broken/photos show nothing" },
    { "NFO",                                        "no flying object seen (might be nearby)" },
    { "NITE",                                       "night" },
    { "NITE.",                                      "night." },
    { "NITE.)",                                     "night.)" },
    { "NITE)",                                      "night)" },
    { "NITELITES",                                  "nightlights" },
    { "NITES",                                      "nights" },
    { "NLS",                                        "night lights" },
    { "NLT",                                        "night light" },
    { "NLT'S",                                      "night lights" },
    { "NLTS",                                       "night lights" },
    { "NLTS)",                                      "night lights)" },
    { "NLTS.",                                      "night lights" },
    { "NMRS",                                       "numerous" },
    { "NOC",                                        "no occupant seen" },
    { "NOTHNG",                                     "nothing" },
    { "NOV",                                        "November" },
    { "NOV.",                                       "November" },
    { "NP",                                         "not plotted on maps" },
    { "NR",                                         "near" },
    { "NR.",                                        "near" },
    { "NRLY",                                       "nearly" },
    { "NRS",                                        "nears" },
    { "NTL",                                        "National" },
    { "NTL.",                                       "National" },
    { "NUC",                                        "nuclear facility (military, institutional etc.)" },
    { "NUC.",                                       "nuclear facility (military, institutional etc.)" },
    { "NUCL",                                       "nuclear" },
    { "NUCL.",                                      "nuclear" },
    { "NVL",                                        "naval" },
    { "NW",                                         "northwest" },
    { "NYT",                                        "The New York Times (NYT)" },
    { "OBJ",                                        "object" },
    { "OBJ.",                                       "object" },
    { "OBJS",                                       "objects" },
    { "OBS",                                        "observer(s)" },
    { "(OBS",                                       "(observer", true },
    { "OBS.",                                       "observer(s)" },
    { "OBS'BONES",                                  "observer's bones" },
    { "OBS'CURED",                                  "observer cured" },
    { "OBS'EYES",                                   "observer's eyes" },
    { "OBS'FACE",                                   "observer's face" },
    { "OBS'HAIR",                                   "observer's hair" },
    { "OBS'HIDE",                                   "observers hide" },
    { "OBS'HOMES",                                  "observer's homes" },
    { "OBS'MIND",                                   "observer's mind" },
    { "OBS'NAME",                                   "observer's name" },
    { "OBS'ROOM",                                   "observer's room" },
    { "OBS'TOOTH",                                  "observer's tooth" },
    { "OBS'VISION",                                 "observer's vision" },
    { "OBSD",                                       "observed" },
    { "OBSS",                                       "observers" },
    { "OCT",                                        "October" },
    { "OCT.",                                       "October" },
    { "ODR",                                        "odor or smell (associated with UFO's)" },
    { "OFC",                                        "Officer" },
    { "OFCR",                                       "Officer" },
    { "OFCRS",                                      "Officers" },
    { "OFFCR",                                      "Officer" },
    { "OGA",                                        "other (overt) Government Agency" },
    { "OHVD",                                       "overhead" },
    { "OID",                                        "small humanoid (or Grey)" },
    { "OIDS",                                       "small humanoids (or Greys)" },
    { "OPP.",                                       "opposite" },
    { "OPR",                                        "operation (incl. mutilations)" },
    { "OPS",                                        "operators" },
    { "ORB",                                        "sphere/orb/globe" },
    { "ORE",                                        "Oregon" },
    { "ORG",                                        "orange" },
    { "ORG.",                                       "orange" },
    { "ORIG",                                       "original" },
    { "ORIG.",                                      "original" },
    { "ORNG",                                       "orange" },
    { "OSC",                                        "oscillate/oscillation" },
    { "OSC.",                                       "oscillate/oscillation" },
    { "OSCL",                                       "oscillate" },
    { "OSCS",                                       "oscillates" },
    { "OUTMNVR",                                    "outmaneuvers" },
    { "OUTMNVRS",                                   "outmaneuvers" },
    { "OUTW",                                       "OUTW" },
    { "OVERHD",                                     "overhead" },
    { "OVHD",                                       "overhead" },
    { "OVHD.",                                      "overhead" },
    { "OVR",                                        "over" },
    { "OVRGND",                                     "over ground" },
    { "OVRHD",                                      "overhead" },
    { "P&O",                                        "Pacific and Orient Steamship Line" },
    { "PANL",                                       "panel" },
    { "PER",                                        "according to" },
    { "PERS",                                       "persons" },
    { "PHONO.",                                     "phonograph" },
    { "PHOTO'D",                                    "photographed" },
    { "PHOTOD",                                     "photographed" },
    { "PHOTOG",                                     "photograph" },
    { "PHOTOGR",                                    "photograph" },
    { "PHOTOGRAP",                                  "photograph" },
    { "PHOTOGS",                                    "photographs" },
    { "PHTS",                                       "photographs" },
    { "PIX",                                        "photograph" },
    { "PK",                                         "Parkway", true },
    { "PKWY",                                       "Parkway" },
    { "PLNT",                                       "plant" },
    { "PORTHLS",                                    "portholes" },
    { "PORTHS",                                     "portholes" },
    { "PORTS",                                      "portholes" },
    { "PORTSHOLES",                                 "portholes" },
    { "POSITIO",                                    "position" },
    { "POSS",                                       "possible" },
    { "POSS.",                                      "possible" },
    { "POSSBL",                                     "possible" },
    { "POWDR",                                      "powder" },
    { "POWS",                                       "prisoners of war" },
    { "PRB",                                        "probe (unmanned scout vehicle)" },
    { "PREG",                                       "pregnant" },
    { "PRES",                                       "president" },
    { "PRESD",                                      "president" },
    { "PRIEST.",                                    "priest" },
    { "PRJCT",                                      "project" },
    { "PRLYZD",                                     "paralyzed" },
    { "PRLZD",                                      "paralyzed" },
    { "PRLZED",                                     "paralyzed" },
    { "PROBS",                                      "probes" },
    { "PROF",                                       "Professor" },
    { "PROF.",                                      "Prof." },
    { "PROJ",                                       "Project" },
    { "PROJ.",                                      "Project" },
    { "PROP.",                                      "propeller" },
    { "PROSPEKT",                                   "prospector" },
    { "PROV.",                                      "province" },
    { "PRV GNDS",                                   "Proving Grounds" },
    { "PRV WNDS",                                   "prevailing winds" },
    { "PRV",                                        "proving" },
    { "PSH",                                        "pseudo-human/entity" },
    { "PSHS",                                       "pseudo-humans/entities" },
    { "PT",                                         "point/port/platinum metal", true },
    { "PT.",                                        "Pt." },
    { "PURRS",                                      "purrs" },
    { "PVT",                                        "private" },
    { "PVT.",                                       "private" },
    { "PWR",                                        "power" },
    { "RBTS",                                       "robot(s)/android(s)" },
    { "RDA",                                        "radiation/radioactivity" },
    { "RDF",                                        "Radio Direction Finder (RDF)" },
    { "RDO",                                        "radio" },
    { "RDO(",                                       "radio(" },
    { "RDOs",                                       "radios" },
    { "RDR",                                        "RADAR" },
    { "RDR.",                                       "RADAR" },
    { "RDRS",                                       "RADAR's" },
    { "RECON",                                      "reconnaissance" },
    { "RECT",                                       "rectangular-box shaped", },
    { "RECT.",                                      "rectangular" },
    { "RECTANGLR",                                  "rectangular" },
    { "RECTNGL",                                    "rectangle" },
    { "RECTWINDOW",                                 "rectangular window" },
    { "REF",                                        "reference" },
    { "REF.",                                       "reference" },
    { "REFNRY",                                     "refinery" },
    { "REFS",                                       "references" },
    { "REFS.",                                      "references" },
    { "REG",                                        "regular" },
    { "REG.",                                       "regular" },
    { "REGLR.",                                     "regular" },
    { "REPT",                                       "report" },
    { "REPTS",                                      "reports" },
    { "RET.",                                       "retired" },
    { "RETD",                                       "retired" },
    { "RETD.",                                      "retired" },
    { "RETRACTIBLE",                                "retractable" },
    { "REV.",                                       "Rev." },
    { "RF",                                         "reference", true },
    { "RFI",                                        "Radio Frequency Interference (RFI)" },
    { "RGLR",                                       "regular" },
    { "RICH.",                                      "Richard" },
    { "RND",                                        "round" },
    { "RND.",                                       "round" },
    { "RPD",                                        "rapid" },
    { "RPS",                                        "reports" },
    { "RPT",                                        "report" },
    { "RPT.",                                       "report" },
    { "RPTD",                                       "reported" },
    { "RPTS",                                       "report(s)" },
    { "RR",                                         "railroad/railway", true },
    { "RSRCH",                                      "research" },
    { "RSRVR",                                      "reservoir" },
    { "RSVN",                                       "reservation" },
    { "RSVR",                                       "reservoir" },
    { "RT",                                         "Rt.", true },
    { "RTE",                                        "RTE" },
    { "RVR",                                        "river" },
    { "RVR.",                                       "river" },
    { "RVS",                                        "(in) reverse" },
    { "S",                                          "south" },
    { "S&L",                                        "straight & level flight" },
    { "S.",                                         "south" },
    { "SAC",                                        "Strategic Air Command (SAC)" },
    { "SCI",                                        "scientist/science" },
    { "SCI.",                                       "scientist/science" },
    { "SCI.AM",                                     "Scientific American" },
    { "SCIS",                                       "scientists" },
    { "SCOTL",                                      "Scotland" },
    { "SCR",                                        "saucer", true },
    { "SCR.",                                       "saucer", true },
    { "SCR'S",                                      "saucers" },
    { "SCRL",                                       "saucer" },
    { "SCRS",                                       "saucers" },
    { "SCRS(",                                      "saucers(" },
    { "SE",                                         "southeast" },
    { "SEARCHLITE",                                 "searchlight" },
    { "SEARCHLITES",                                "searchlights" },
    { "SEC",                                        "second(s)" },
    { "SECR",                                       "SECR" },
    { "SECR.",                                      "secretary" },
    { "SECS",                                       "seconds" },
    { "SEMICRC",                                    "semicircle" },
    { "SENSATN",                                    "sensation" },
    { "SEP",                                        "separate" },
    { "SEP.",                                       "separate" },
    { "SEPT",                                       "September" },
    { "SEPT.",                                      "September" },
    { "SEQNCE",                                     "sequence" },
    { "SEQUENC",                                    "sequence" },
    { "SGT",                                        "Sgt." },
    { "SGTS",                                       "Sergeants" },
    { "SHADOED",                                    "shadowed" },
    { "SHNY",                                       "shiny" },
    { "SHT",                                        "short" },
    { "SIGNLS",                                     "signals" },
    { "SIGS",                                       "signal(s) of any type involving UFO's\aliens" },
    { "SILENC",                                     "silence" },
    { "SLC",                                        "Salt Lake" },
    { "SLITE",                                      "searchlight" },
    { "SLNT",                                       "silent" },
    { "SLNT.",                                      "silent" },
    { "SLNTLY",                                     "silently" },
    { "SLO",                                        "slow" },
    { "SLO.",                                       "slow" },
    { "SLT",                                        "searchlight" },
    { "SLV",                                        "silver" },
    { "SLVER",                                      "silver" },
    { "SLVR",                                       "silver" },
    { "SLVRY",                                      "silvery" },
    { "SM",                                         "small", true },
    { "SMALLR",                                     "smaller" },
    { "SMEL",                                       "smell" },
    { "SML",                                        "small" },
    { "SML.",                                       "small" },
    { "SMP",                                        "sample(s)" },
    { "SMPL",                                       "sample" },
    { "SMPLS",                                      "samples" },
    { "SND",                                        "sounds (made by UFO's)" },
    { "SNGL",                                       "single" },
    { "SNY",                                        "SNY" },
    { "SOBEPS",                                     "Belgian UFO Society (SOBEPS)" },
    { "SOUNDS",                                     "sounds" },
    { "SOV",                                        "Soviet" },
    { "SOV.",                                       "Soviet" },
    { "SPACEFLITE",                                 "spaceflight" },
    { "SPC",                                        "in space" },
    { "SPD",                                        "speed/velocity" },
    { "SPDS",                                       "speeds" },
    { "SPHERICA",                                   "spherical" },
    { "SPHR",                                       "sphere (usually orb)" },
    { "SPOTLITE",                                   "spotlight" },
    { "SPOTLITES",                                  "spotlights" },
    { "SPOTLT",                                     "spotlight" },
    { "SPOTLTS",                                    "spotlights" },
    { "SQDR",                                       "Squadron" },
    { "SQDRN",                                      "Squadron" },
    { "SQR",                                        "square" },
    { "SQR.",                                       "square" },
    { "SRCHLITES",                                  "searchlights" },
    { "SRT",                                        "SRT" },
    { "SRVL",                                       "several" },
    { "SS",                                         "steamship", true },
    { "SSUIT",                                      "spacesuit" },
    { "SSUITS",                                     "spacesuits" },
    { "ST",                                         "St." },
    { "ST.",                                        "St." },
    { "STABLZR",                                    "stabilizer" },
    { "STARLIKE",                                   "star-like" },
    { "STARTREK",                                   "Star Trek" },
    { "STATN",                                      "station" },
    { "STN",                                        "station/depot/facility" },
    { "STN.",                                       "station" },
    { "STNS",                                       "stations" },
    { "STOPLITE",                                   "stop-light" },
    { "STREETLITE",                                 "street light" },
    { "STREETLITES",                                "street lights" },
    { "STRT",                                       "straight/strait(s)" },
    { "STRUCT.",                                    "structure" },
    { "STUDYS",                                     "studies" },
    { "SUB",                                        "submersible/USO" },
    { "SUB.",                                       "submersible" },
    { "SUBASS'Y",                                   "subassembly" },
    { "SUBM",                                       "submarine/submerged" },
    { "SUBM.",                                      "submarine/submerged" },
    { "SUNLIKE",                                    "sun-like" },
    { "SUNLITE",                                    "sunlight" },
    { "SURF",                                       "surface(s)" },
    { "SVL",                                        "several" },
    { "SVLR",                                       "several" },
    { "SVR",                                        "several" },
    { "SVRL",                                       "several" },
    { "SVRL.",                                      "several" },
    { "SVRL/BASE",                                  "several on (mil) base" },
    { "SW",                                         "southwest" },
    { "SW)",                                        "southwest)" },
    { "SWITZERLND",                                 "Switzerland" },
    { "TALL'BALLOON",                               "tall balloon" },
    { "TARS",                                       "sailors" },
    { "TCH",                                        "Trans-Canada Highway" },
    { "TECHN",                                      "technician" },
    { "TELEFOTO",                                   "telephoto" },
    { "TELEFOTOS",                                  "telephotos" },
    { "TEMP.",                                      "temporarily" },
    { "TGHT",                                       "tight" },
    { "THEODLT",                                    "theodolite" },
    { "THRU",                                       "through" },
//    { "TIL",                                        "til" },
    { "TLP",                                        "telepathy" },
    { "TLPS",                                       "telepaths" },
    { "TRAJ",                                       "trajectory" },
    { "TRAJ.",                                      "trajectory" },
    { "TRAJCT",                                     "trajectory" },
    { "TRAJECT",                                    "trajectory" },
    { "TRAJECTRY",                                  "trajectory" },
    { "TRAJS",                                      "trajectories" },
    { "TRANPARENT",                                 "transparent" },
    { "TRC",                                        "physical trace(s)" },
    { "TRCS",                                       "physical traces" },
    { "TRIANGL",                                    "triangular" },
    { "TRIANGLAR",                                  "triangular" },
    { "TRIANGLR",                                   "triangular" },
    { "TRIANGLR.",                                  "triangular" },
    { "TRIANGLS",                                   "triangles" },
    { "TRIANGLULAR",                                "triangular" },
    { "TRIANLR",                                    "triangular" },
    { "TRNGL",                                      "triangle" },
    { "TRNGLR",                                     "triangular" },
    { "TRVL",                                       "travel" },
    { "TSCOPE",                                     "(seen thru) telescope" },
    { "TSCOPES",                                    "telescopes" },
    { "TUB",                                        "tub" },
    { "TWR",                                        "tower" },
    { "TWRD",                                       "toward(s)" },
    { "UAL",                                        "United Airlines (UAL)" },
    { "UC#",                                        "UFOCAT Record #" },
    { "UEM",                                        "(unsolicited email/not verified)" },
    { "UFOS",                                       "UFO's" },
    { "UID",                                        "unidentified" },
    { "UNINTELLIBIBLE",                             "unintelligible" },
    { "UNIV",                                       "university/universe(al)" },
    { "UNIV.",                                      "university" },
    { "UNK",                                        "unknown" },
    { "UNKN",                                       "unknown" },
    { "UNKN.",                                      "unknown." },
    { "UNS",                                        "UFO Newsclipping Service" },
    { "UPI",                                        "United Press International" },
    { "USMC",                                       "U.S. Marine Corps (USMC)" },
    { "USN",                                        "United States Navy (USN)" },
    { "UTURN",                                      "u-turn" },
    { "V-FORM",                                     "V-formation" },
    { "VAC.",                                       "vacuum" },
    { "VARI",                                       "variable" },
    { "VARI-CLRD",                                  "variable-colored" },
    { "VBIG",                                       "very big" },
    { "VBRIEF",                                     "very brief" },
    { "VBRITE",                                     "vibrant bright" },
    { "VCLOSE",                                     "very close" },
    { "VCVR",                                       "Vancouver" },
    { "VDARK",                                      "very dark" },
    { "VEG",                                        "vegetation" },
    { "VEG.",                                       "vegetation" },
    { "VEGETATI",                                   "vegetation" },
    { "VEGETATION",                                 "vegetation" },
    { "VELO",                                       "velocity" },
    { "VELOC",                                      "velocity" },
    { "VERT",                                       "vertical" },
    { "VERT.",                                      "vertical" },
    { "VERTCL",                                     "vertical" },
    { "VERTICL",                                    "vertical" },
    { "VET.",                                       "veteran" },
    { "VFAST",                                      "very fast" },
    { "VFLAT",                                      "very flat" },
    { "VFORM",                                      "V formation" },
    { "VFORMN",                                     "V-formation" },
    { "VFORMNS",                                    "V formations" },
    { "VHI",                                        "very high" },
    { "VHIGH",                                      "very high" },
    { "VHOT",                                       "very hot" },
    { "VIS",                                        "visual (observation)" },
    { "VISBL",                                      "visible" },
    { "VIZ",                                        "visual" },
    { "VLARGE",                                     "very large" },
    { "VLNT",                                       "violent" },
    { "VLO",                                        "very low" },
    { "VLONG",                                      "very long" },
    { "VLOW",                                       "very low" },
    { "VLRG",                                       "very large" },
    { "VPR",                                        "vapor" },
    { "VRTCL",                                      "vertical" },
    { "VRTCL.",                                     "vertical" },
    { "VSHARP",                                     "very sharp" },
    { "VSHINY",                                     "very shiny" },
    { "VSHORT",                                     "very short" },
    { "VSICK",                                      "very sick" },
    { "VSLOW",                                      "very slow" },
    { "VSTRANGE",                                   "very strange" },
    { "VSTRONG",                                    "very strong" },
    { "VTCL",                                       "vertical" },
    { "VV",                                         "very very" },
    { "VVFAST",                                     "very very fast" },
    { "W",                                          "west" },
    { "W.",                                         "west" },
    { "WAV",                                        "wave" },
    { "WEATHR",                                     "weather" },
    { "WHT",                                        "white" },
    { "WHT.",                                       "white" },
    { "WISC",                                       "Wisconsin" },
    { "WMN",                                        "women" },
    { "WO/",                                        "without" },
    { "WOMANS",                                     "woman's" },
    { "WONT",                                       "won't" },
    { "WRLD",                                       "world" },
    { "WTHR",                                       "weather" },
    { "WTHRMAN",                                    "weatherman" },
    { "WTNS",                                       "witness" },
    { "XBLACK",                                     "extremely black" },
    { "XBRITE",                                     "extremely bright" },
    { "XDARK",                                      "extremely dark" },
    { "XFAST",                                      "extremely fast" },
    { "XFORM",                                      "transform" },
    { "XFORMER",                                    "transformer" },
    { "XLARGE",                                     "extremely large" },
    { "XLO",                                        "extremely low" },
    { "XLUCENT",                                    "translucent" },
    { "XMAS",                                       "Christmas" },
    { "XMISSION",                                   "transmission" },
    { "XMITTER",                                    "transmitter" },
    { "XMITTR",                                     "transmitter" },
    { "XPARENT",                                    "transparent" },
    { "XPARNT",                                     "transparent" },
    { "XPERT",                                      "expert" },
    { "XPORT",                                      "transport" },
    { "XPORTED",                                    "exported" },
    { "XPRNT",                                      "transparent" },
    { "XRAY",                                       "array" },
    { "XRAYS",                                      "arrays" },
    { "XSISTOR",                                    "transistor" },
    { "XTRA",                                       "extra" },
    { "XTREME",                                     "extreme" },
    { "XTREMELY",                                   "extremely" },
    { "XTRM",                                       "extreme" },
    { "XXFAST",                                     "incredibly fast" },
    { "YDS",                                        "yards" },
    { "YEL",                                        "yellow" },
    { "YEL.",                                       "yellow" },
    { "YLW",                                        "yellow" },
    { "YLW.",                                       "yellow" },
    { "YLWD",                                       "yellowed" },
    { "YRS",                                        "years" },
    { "ZAP",                                        "strike with a disabling beam" },
    { "WEDn.",                                      "Wednesday" },
    { "HVRing",                                     "hovering" },
    { "HVY",                                        "heavy", true },
    { "Rte.",                                       "Route" },
    { "Can.",                                       "Can." },
    { "grav.",                                      "gravitational" },
    { "ledger.",                                    "ledger." },
    { "nav.",                                       "navigation" },
    { "invisbl",                                    "invisible" },
    { "Penlites",                                   "penlights" },
    { "Astonomer",                                  "Astronomer" },
    { "unk.",                                       "unknown" },
    { "(scr",                                       "(saucer" },
    { "CAPTn",                                      "Captain" },
    { "Wierd",                                      "weird" },
    { "Lngth",                                      "length" },
    { "sig",                                        "signal", true },
    { "visibl",                                     "visible" },
    { "NDS",                                        "needs" },
    { "electr.",                                    "electrical" },
    { "(nr",                                        "(near" },
    { "chans",                                      "channels", true },
    { "russ.",                                      "Russian", true },
    { "dan.",                                       "Danish", true },
    { "behnd",                                      "behind", true },
    { "dgr",                                        "degree", true },
    { "lite.",                                      "light", true },
    { "COLLISTION",                                 "collision" },
    { "Vsbl",                                       "visible", true },
    { "Vsbl.",                                      "visible", true },
    { "SCR(",                                       "saucer(" },
    { "telecopes",                                  "telescopes" },
    { "elecrics",                                   "electrics" },
    { "rbt",                                        "robot", true },
    { "gowing",                                     "glowing", true },
    { "imposs.",                                    "impossible", true },
    { "Trucated",                                   "truncated" },
    { "Stonewood.",                                 "Stonewood" },
    { "MUTLd",                                      "mutilated" },
    { "GNTS",                                       "giants" },
    { "VLY",                                        "valley" },
    { "invis.",                                     "invisible", true },
    { "lev's",                                      "levitates", true },
    { "Lk.",                                        "lake", false },
    { "RSV",                                        "Reservation", true },
    { "Sat.",                                       "satellite", true },
    { "(Sat.",                                      "(satellite", true },
    { "af.",                                        "Air Force", true },
    { "jap.",                                       "Japanese", true },
    { "explanatn",                                  "explanation", true },
    { "EXPLAN.",                                    "explanation", true },
    { "estm",                                       "estimated", true },
    { "CENTR",                                      "center" },
    { "CENTR.",                                     "center" },
    { "scrs.",                                      "saucers", true },
    { "elev",                                       "elevation", true },
    { "elev.",                                      "elevation", true },
    { "INTERVLS.",                                  "intervals" },
    { "WIDEx1mTALL",                                "wide and 1m tall", true },
    { "Flt.",                                       "flight" },
    { "EVAPs",                                      "evaporates" },
    { "50x10x35'SLNT",                              "50x10x35' silent" },
    { "obsvs",                                      "observations", true },
    { "obsvs.",                                     "observations", true },
    { "360dgr",                                     "360 degrees", true },
    { "Al.",                                        "Al.", true },
    { "PUBl",                                       "public", true },
    { "VEGTn",                                      "vegetation", true },
    { "VEGTn.",                                     "vegetation", true },
    { "dia.",                                       "diameter", true },
    { "diam.",                                      "diameter", true },
    { "Rsvr.",                                      "reservoir", true },
    { "rectngl.",                                   "rectangule", true },
    { "Wheatfld",                                   "wheatfield", true },
    { "inv.",                                       "inv.", true }, // several meanings
    { "RNDed",                                      "rounded", true },
    { "Relands",                                    "re-lands", true },
    { "vinyard",                                    "vineyard", true },
    { "vinyards",                                   "vineyards", true },
    { "Lg.",                                        "large", true },
    { "crypto.",                                    "crypto.", true }, //unsure
    { "Adamskii",                                   "Adamski", true },
    { "Perf.",                                      "perfect", true },
    { "Elctric",                                    "electric", true },
    { "Wilmington.NC",                              "Wilmington NC", true },
    { "Isls",                                       "islands" },
    { "Isl",                                        "island" },
    { "Isl)",                                       "island)" },
    { "Obs(16)",                                    "observers(16)", true },
    { "Towm",                                       "town", true },
    { "VFORMs",                                     "vertical formations", true },
    { "ABDd",                                       "abducted", true },
    { "ABDd.",                                      "abducted", true },
    { "Eqltrl",                                     "equilateral", true },
    { "Seo.obs",                                    "several observers", true }, // a guess, seems fine
    { "fotog.",                                     "photographer", true },
    { "rectanglar",                                 "rectangular", true },
    { "VCT",                                        "VCT", true }, // don't know, used once
    { "Wendover.NV",                                "Wendover NV", true },
    { "SDI",                                        "Strategic Defense Initiative (SDI)", true },
    { "refl",                                       "reflected", true },
    { "mns",                                        "minutes", true },
    { "Simlr",                                      "similar", true },
    { "INSTRs",                                     "instruments", true },
    { "ENGn",                                       "engineering", true },
    { "ENGn.",                                      "engineering", true },
    { "lv",                                         "leave", true },
    { "OSCLTs",                                     "oscillates", true },
    { "COM.",                                       "communication", true },
    { "Missp.",                                     "Mississippi" },
    { "Phys",                                       "physical", true },
    { "Phys.",                                      "physical", true },
    { "Silnt",                                      "silent", true },
    { "Sec.",                                       "Sec.", true }, // security or secretary, ugh
    { "INVESTIGn",                                  "investigation", true },
    { "INVESTIGn.",                                 "investigation", true },
    { "tomatos",                                    "tomatoes", true },
    { "bli",                                        "blue", true }, //spelling fix
    { "rgbw",                                       "red-green-blue-white", true },
    { "ENGs",                                       "engineers", true },
    { "ENGs.",                                      "engineers", true },
    { "Satl.",                                      "satellite", true },
    { "dn",                                         "down", true },
    { "SUBSTATn",                                   "substation" },
    { "struct",                                     "structure" },
    { "FORMn.",                                     "formation", true },
    { "DIRns",                                      "directions", true },
    { "DIRns.",                                     "directions", true },
    { "dwarflike",                                  "dwarf-like", true },
    { "car-size",                                   "car-size", true },
    { "Rainstrm",                                   "rainstorm", true },
    { "SWR",                                        "somewhat rectangular (SWR)", true }, // unsure, thanks David White
    { "SWR.",                                       "somewhat rectangular (SWR)", true }, // unsure, thanks David White
    { "Boomrng",                                    "boomerang", true },
    { "Squeeky",                                    "squeaky", true },
    { "Johnsons",                                   "Johnson's", true },
    { "holes.gnd",                                  "holes in ground", true },
    { "Whirlwhind",                                 "whirlwind", true },
    { "Trglr",                                      "triangular", true },
    { "ISL.",                                       "island", true },
    { "Visl",                                       "visible", true },
    { "instrm.",                                    "instruments", true },
    { "ovs",                                        "observations", true },
    { "CAPTns",                                     "Captains", true },
    { "ARROWHd",                                    "arrow headed", true },
    { "Turb",                                       "turn back", true }, // a guess but seems fine
    { "investgd",                                   "investigated", true },
    { "yufos",                                      "UFO's", true }, //misspelling
    { "FOTOd",                                      "photographed", true },
    { "pg",                                         "page", true },
    { "pg.",                                        "page", true },
    { "Col.",                                       "Col.", true },
    { "Amer.",                                      "American", true },
    { "Amer",                                       "American", true },
    { "miss.river",                                 "Mississippi River", true },
    { "mich",                                       "Michigan", true },
    { "Ff.",                                        "French Foreign", true },
    { "intervw",                                    "interview", true },
    { "FOTOGs",                                     "photographs", true },
    { "Hube",                                       "huge", true }, // apparent typo
    { "VLUMn",                                      "very luminous", true },
    { "VLUMn.",                                     "very luminous", true },
    { "Hlites",                                     "headlights", true },
    { "Hvrng",                                      "hovering", true },
    { "STRUCTs",                                    "structures", true },
    { "Twx",                                        "telex", true },
    { "Missl",                                      "missile", true },
    { "Rtes",                                       "routes" },
    { "rept.",                                      "report", true },
    { "nfast",                                      "NFAST", true }, // dunno
    { "mellon",                                     "Mellon", true },
    { "Lndg",                                       "landing", true },
    { "Dly",                                        "Daily", true },
    { "abandonded",                                 "abandoned", true },
    { "ABDn",                                       "abduction", true },
    { "sny.",                                       "shiny", true },
    { "rd",                                         "road", true },
    { "Bocomes",                                    "becomes", true },
    { "ROTn",                                       "rotation", true },
    { "ROTn.",                                      "rotation", true },
    { "dwgs",                                       "drawings", true },
    { "coml",                                       "commercial", true }, // best guess
    { "DEPRSs",                                     "depressions", true },
    { "techn.",                                     "technical", true },
    { "Descripts.",                                 "descriptions", true },
    { "frq",                                        "frequency", true },
    { "SEMICRC.",                                   "semicircular", true },
    { "Mwide",                                      "meter wide", true },
    { "mhi",                                        "meter high", true },
    { "Fantastc",                                   "fantastic", true },
    { "Warehse",                                    "warehouse", true },
    { "Cresc",                                      "crescent", true },
    { "ojs",                                        "objects", true },
    { "Recogn.",                                    "recognition", true }, // unsure nut seems reasonable
    { "Vfrmn",                                      "V formation", true },
    { "Airborn",                                    "airborne", true },
    { "srvl.",                                      "several", true },
    { "Eqlt",                                       "equilateral", true },
    { "Eqlt.",                                      "equilateral", true },
    { "Arg.",                                       "Arg.", true }, //unsure
    { "asst",                                       "Assistant", true },
    { "asst.",                                      "Assistant", true },
    { "dist.",                                      "District", true },
    { "atty",                                       "Attorney", true },
    { "LUMns",                                      "luminous", true },
    { "WARNED2",                                    "warned 2", true },
    { "cat&mouse",                                  "cat & mouse", true },
    { "Twrs",                                       "towers", true },
    { "fant.",                                      "fantastic", true },
    { "foos",                                       "Foo-Fighters", true },
    { "OBS'rib",                                    "observer's rib", true }, // strange
    { "HVTS",                                       "hovers", true }, //misspelling
    { "Airl",                                       "Airline", true },
    { "eq.",                                        "earthquake", true },
    { "eq",                                         "earthquake", true },
    { "(eq",                                        "(earthquake", true },
    { "Dipl.",                                      "diplomatic", true },
    { "inqry",                                      "inquery", true },
    { "Eng.",                                       "eng.", true },
    { "Demagn",                                     "demagnetized", true },
    { "comcentric",                                 "concentric", true },
    { "ele.",                                       "electric", true },
    { "DURn",                                       "duration", true },
    { "SUBSTn",                                     "substation", true },
    { "landing.",                                   "landing", true },
    { "OFCs",                                       "officers", true },
    { "CIRF.",                                      "CIRF.", true }, // dunno
    { "auto.",                                      "auto.", true }, // dunno
    { "flite#519",                                  "flight #519", true },
    { "Midsentence",                                "mid-sentence", true },
    { "FOLOd",                                      "followed", true },
    { "ELECTRIFd",                                  "electrified", true },
    { "Hlite",                                      "headlight", true },
    { "engr",                                       "engineer", true },
    { "engr.",                                      "engineering", true },
    { "Violnt",                                     "violent", true },
    { "conf.",                                      "conf.", true },
    { "Newpapers",                                  "newspapers", true },
    { "Rivr",                                       "river", true },
    { "rgb",                                        "red/green/blue", true },
    { "self-Illum.",                                "self-illuminated", true },
    { "src",                                        "saucer", true },
    { "VERTCL.",                                    "vertical", true },
    { "excell.",                                    "excellent", true },
    { "Enorm.",                                     "enormous", true },
    { "DERBYs",                                     "Derbyshire" },
    { "HZN",                                        "horizon", true },
    { "GN",                                         "green", true },
    { "Alamosa.",                                   "Alamosa.", true },
    { "globalls",                                   "blowballs", true },
    { "Japn.",                                      "Japanese", true },
    { "Possbl.",                                    "Possible", true },
    { "slvr.",                                      "silver", true },
    { "Leb.",                                       "Leb.", true },
    { "Extention",                                  "extension", true },
    { "INSTRts",                                    "instruments", true },
    { "Var.",                                       "various", true },
    { "Img",                                        "image", true },
    { "FLUCTs",                                     "fluctuates", true },
    { "rtps",                                       "reports", true }, // "separate rtps"
    { "Math.",                                      "Mathematics", true },
    { "indp.",                                      "independent", true },
    { "frag",                                       "fragment", true },
    { "PUBLd",                                      "publicized", true },
    { "Illumn.",                                    "illuminated", true },
    { "spotlited",                                  "spotlighted", true },
    { "Mme.",                                       "Mme.", true },
    { "Stationry",                                  "stationary", true },
    { "Stromlo.",                                   "Stromlo.", true },
    { "Engnr",                                      "Engineer", true },
    { "elect.",                                     "electric", true },
    { "Nmrs.",                                      "numerous", true },
    { "SS.",                                        "SS.", true },
    { "abductns",                                   "abductions", true },
    { "ACCEL'n",                                    "acceleration", true },
    { "Planetlike",                                 "planet-like", true },
    { "aprox.",                                     "appproximate", true },
    { "NEIGHBORHd",                                 "neighborhood", true },
    { "Rpt'd",                                      "reported", true },
    { "Astromomers",                                "astronomers", true },
    { "Collsn",                                     "collision", true },
    { "DOC'd",                                      "documented", true },
    { "Mdia",                                       "meters diameter", true },
    { "Excellnt",                                   "excellent", true },
    { "MUTLs",                                      "mutilations", true },
    { "Engl&Spanish!",                              "English and Spanish!", true },
    { "Newspaper",                                  "newspaper", true },
    { "Familys",                                    "families", true },
    { "Undr",                                       "under", true },
    { "Aux.",                                       "aux.", true },
    { "Srp.",                                       "Srp.", true },
    { "Phila.",                                     "Philadelphia", true },
    { "App.",                                       "apparent", true },
    { "Unintellibigle",                             "unintelligible",  true },
    { "Ital.",                                      "Italian", true },
    { "Lmtd",                                       "limited", true },
    { "Parlyzd",                                    "paralyzed", true },
    { "inquery",                                    "inquiry", true },
    { "MOTOs",                                      "motorists", true },
    { "Irreg",                                      "irregular", true },
    { "MTLd",                                       "mutilated", true },
    { "confidntl",                                  "confidential", true },
    { "Polarzd",                                    "polarized", true },
    { "DUMBELLs",                                   "dumbell-shaped craft", true },
};

// This table cannot change the length of strings, just case.
static const char* g_cap_exceptions[] =
{
    //"Hill Air Force Base", // TODO: not working because "Air Force Base" is one token, not 3
    "Satus Peak",
    "Itaparica Island",
    "Moira Lake",
    "Terrigal Road",
    "Chiricahua Mountains",
    "Boardman Lake",
    "RCA Building",
    "Mersea Island",
    "Vallekilde College",
    "Midway Island",
    "1st",
    "Biscayne Bay",
    "Chowpatty Beach",
    "El Yunque Mountain",
    "Red River Arsenal",
    "Indian Point Nuclear Facility",
    "Damhus Lake",
    "Wenatchee Valley",
    "Imurissu River Bridge",
    "Brunel Ward",
    "Manzano Mountains",
    "Lake George",
    "Lake Ponchartrain",
    "Lake Superior",
    "Lake Langsjoen",
    "Lake Champlain",
    "Lake Erie",
    "Lake Laberge",
    "Lake Butte des Morts",
    "Kirkstall Museum",
    "DoD Report",
    "D. Dam",
    "Red River",
    "Mechanical Engineer",
    "Lake Ryzl",
    "Saufley Field",
    "Sheppard Air Force Base",
    "Ohio River",
    "National Airport",
    "Puget Sound",
    "Villa Nevares",
    "Lake Victoria",
    "Alexander the Great",
    "Verdun Museum",
    "Nijo Castle",
    "Westminster Hall",
    "Charles Fort",
    "Book of the Damned",
    "Journal de Lozere",
    "Boniface Manuscript",
    "Sutro House",
    "Seal Rock",
    //"Dives",
    "Mt. Skirt",
    "Gros-Mont",
    "Cmdnt Georget",
    "Clearwater River Canyon",
    "Warmley Village",
    "English",
    "Calze Steso!",
    "Fager Mountains",
    "Coral Lorenzen",
    "Nobby's Head",
    "US planes",
    "US fighter",
    "US fighters",
    "US aircraft",
    "Bomber Squadron",
    "415th Bombers",
    "Foo-Fighters",
    "Goteborg News",
    "Prime Minister",
    "New Orleans",
    "US WACs",
    "(AP)",
    "E. Sprinkle",
    "B. Savage",
    "Albuquerque Journal",
    "Vancouver Sun",
    "LaPlante",
    "Oregon and Wash",
    "C. Cross",
    "McLeod",
    "Black Mountain",
    "Bert Hall",
    "Canal Street",
    "McKee Bridge",
    "McKee",
    "Ft. Worth",
    "William Good",
    "Maltese Cross",
    "L.A.",
    "City Hall",
    "McCann",
    "Holman Airport",
    "Lake Cobbosseecontee",
    "McChord",
    "Hetch Hetchy Aqueduct",
    "LaPaz",
    // #REVIEW Does this need to be double question mark? clang trips on "trigraph ignored" -Wtrigraphs
    "Sea Island'(??)",
    "Loren Gross",
    "Test Pilot",
    "Brawley News",
    "Minnechaug Mountain",
    "Air National Guard",
    "National Guard",
    "Project Twinkle",
    "Pikes Peak",
    "Denver News",
    "Lambert Field",
    "Post Dispatch",
    "no UFO",
    "Blackcomb Mountain",
    "Harding Mall",
    "Hawkes Bay",
    "Hells Canyon",
    "Highway Patrol",
    "Hogg Mountain",
    "Hondo Beach",
    "Leek Field",
    "Lekness Airfield",
    "Riverview Lane",
    "Royal Gorge",
    "Semiahoo Bay",
    "Tappan Zee Bridge",
    "Tully Pond",
    "Tyrol Village",
    "Wagle Mountain",
    "Bob Oechsler",
    "Top Secret",
    "Aire Valley",
    "Alameda Naval Yard",
    "Alcova Reservoir",
    "Siskiyou Co",
    "Elysian Park",
    "Mathieson Chemical Plant",
    "Eastern Hills",
    "Lake Clarendon",
    "Seneca Lake",
    "China Daily",
    "News Dispatch",
    "Cardigan Bay",
    "department store",
    "Griffith Park",
    "Ozarks College",
    "Lower Richland",
    "Baer Field",
    "Clarion Ledger.",
    "Clarion Ledger",
    "L. I. Sound",
    "State Highway Patrolman",
    "Anti-Mine",
    "L. Gross",
    "Loire River",
    "Bergen-Aan-See",
    "Michelin-Man",
    "Diario Insular",
    "Secretary Talbot",
    "Bill Lear",
    "Bill Taylor",
    "Elliot Bay",
    "Gulf Breeze",
    "Witton Wood",
    "Lake Mayer",
    "Crown St.",
    "Lake Peblinge",
    "Maple Creek",
    "Nabbs Lane",
    "Jacarepagua Lagoon",
    "Henry Ford II",
    "Air Force",
    "NJ Turnpike",
    "Fattorini's",
    "Po River",
    "Delta Flight",
    "Niagara Falls",
    "Swan Lake",
    "MacKenzie River",
    "MacKenzie",
    "Lake Krupat",
    "Natrona Co Airport",
    "Okanagan Valley",
    "CB Radio",
    "Johnson's Wax",
    "Bass Harbor",
    "Swans Island",
    "Rainbow Lake",
    "Rhone Valley",
    "Portsdown Hill",
    "Mella Road",
    "Kodak Co",
    "Lake Siljan",
    "A10 Thunderbolts",
    "Tujunga Canyon",
    "Lake Michigan",
    "Waha Grade",
    "Seco Nuclear Plant",
    "Davis Monthan",
    "Phillipine Bear",
    "Columbia River",
    "Itaipu Rock",
    "Ogilbay Park",
    "MacDill",
    "Thau Reservoir",
    "River Doux",
    "River Seine",
    "Seine River",
    "River Kwai",
    "Brazos River",
    "Snake River Canyon",
    "Pardo River",
    "Orinoco River",
    "Connecticut River",
    "Norman River",
    "Ottawa River",
    "Rappahannock River",
    "Great Miami River",
    "Arkansas River",
    "Ben Hai River",
    "Addo River",
    "Pomba River",
    "Murray River",
    "Tagus River",
    "Metauro River",
    "Qingge River",
    "Mississippi River",
    "Yellowstone River",
    "Hudson River",
    "Birch River",
    "Ouse River",
    "Rio Grande River",
    "Mersey River",
    "Potomac River",
    "Wenatchee River",
    "Han-Gang River",
    "St. Clair River",
    "River Canyon",
    "River Road",
    "Tevere River",
    "Lake Michigan",
    "Lake Gertrude",
    "Harvey Lake",
    "Broadwater Lake",
    "Mystic Lake",
    "Wagle Mountain Lake",
    "Lake Okangan",
    "Coffeen Lake",
    "Lake Leman",
    "Lake Green",
    "Ozark Lake",
    "Lake Stanley Draper",
    "Lake Langhalsen",
    "Lake Cowichan",
    "Lake Norman",
    "Lake Murray",
    "Estes Lake",
    "Fork Lake",
    "Lake Fryken",
    "Kamloops Lake",
    "Lake Kariba",
    "James River",
    "Platte River",
    "Tamegafor River",
    "River Valley",
    "Guaiba River",
    "Isle of Wight",
    "Coyote Point",
    "Gurupi River",
    "Mono Counties",
    "Taconic Parkway",
    "Cifex Group",
    "NY Daily News",
    "Chatsworth Reservoir",
    "Austin Lake",
    "Dundry Hills",
    "Park Wardens",
    "Mountbatten Estate",
    "Landeforet Reservoir",
    "American Canyon Road",
    "Phinney Ridge",
    "Mayfield Road",
    "Waterman Road",
    "Kinzy Road",
    "Penge Road",
    "Woodside Road",
    "Hamilton Road",
    "Kenai Mountains",
    "Maure Reservoir",
    "Osmasten Road",
    "Mezayon Valley",
    "Ortley Beach",
    "Carl Sagan's",
    "Elementary School",
    "Ellington Field",
    "Parry Crater",
    "Portola Road",
    "Avila Mountain",
    "Sea Island",
    "Marajo Bay",
    "Falklands War",
    "Peterson Airfield",
    "Nature Reserve",
    "Tillamook Head",
    "Hya Kuri A",
    "LBJ Ranch",
    "Van Nuys",
    "Hawkes Bay",
    "Ubac Mountains",
    "Chemical Engineer",
    "Aftonbladet Daily",
    "Deputy McGuire",
    "Port Ormond",
    "Hydel Canal",
    "Bogue Inlet",
    "Guanabara Bay",
    "Naugatuck Valley",
    "Poconos Mountains",
    "Kolb Road",
    "Welland Canal",
    "Lewisville Dam",
    "Jeff Field",
    "Menastash Ridge",
    "Khami Prison",
    "Kittinney Mountains",
    "Duchesne River",
    "Kittinney Mountains",
    "Skulte Airport",
    "Petit-Honach Mountain",
    "Trimble Road",
    "Barents Sea",
    "Eiffel Tower",
    "Monay Plains",
    "Ramapo Mountains",
    "Apure River",
    "Kazak Mountains",
    "Tillamook Head",
    "Kathy Davis",
    "Kyodo News Agency",
    "Newport Bridge",
    "McGuire",
    "Pan-AM",
    "M. Barker",
    "McGilvery",
    "DeKalb",
    "rises",
    "falls",
    "spins",
    "moons",
    "videod",
    "UFOCAT",
    "McDonalds",
    "reappears",
    "Bassett",
    "responds",
    "glides",
    "dives",
    "AT&T",
    "stars",
    "McMinnville",
    "McCullen",
    "spirals",
    "McDonnell",
    "McGuire",
    "McDivitt",
};
