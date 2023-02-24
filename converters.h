// converters.h
// Copyright (C) 2023 Richard Geldreich, Jr.
#pragma once
#include "utils.h"

void converters_init();

bool convert_magnonia(const char* pSrc_filename, const char* pDst_filename, const char* pSource_override = nullptr, const char* pRef_override = nullptr);
bool convert_bluebook_unknowns();
bool convert_hall();
bool convert_eberhart(unordered_string_set& unique_urls);
bool convert_johnson();
bool convert_nicap(unordered_string_set& unique_urls);
bool convert_nuk();