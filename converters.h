// converters.h
// Copyright (C) 2023 Richard Geldreich, Jr.
#pragma once
#include "utils.h"

void converters_init();

bool convert_magonia(const char* pSrc_filename, const char* pDst_filename, const char* pSource_override = nullptr, const char* pRef_override = nullptr, uint32_t TOTAL_COLS = 15, const char *pType_override = nullptr, bool parens_flag = true, uint32_t first_rec_index = 1);
bool convert_dolan(const char* pSrc_filename, const char* pDst_filename, const char* pSource, const char* pType, const char* pRef);
bool convert_bluebook_unknowns();
bool convert_hall();
bool convert_eberhart(unordered_string_set& unique_urls);
bool convert_johnson();
bool convert_nicap(unordered_string_set& unique_urls);
bool convert_nuk();
bool convert_anon();
bool convert_rr0();
bool convert_overmeire();