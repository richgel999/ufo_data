# UFO Event Timeline Processing Tool

## Copyrights:

### Code:
* [ufojson](https://github.com/richgel999/ufo_data) tool: Copyright (c) 2023 By [Richard Geldreich, Jr.](https://medium.com/@richgel99) (Apache 2.0 license)

* [libsoldout](https://github.com/faelys/libsoldout): Copyright (c) 2009, Natacha Porté (ISC license)

* [json](https://github.com/nlohmann/json): Copyright (c) 2013-2022 Niels Lohmann (MIT license)

* Some portions of the specific file udb_tables.h (such as get_hatch_geo, g_hatch_continents) use strings/tables from the ["uDb" project by Jérôme Beau](https://github.com/RR0/uDb).

### Data files (in 'bin' directory): 
Although the C++ tool itself is open source with a permissive license (Apache 2.0), **any commercial use of the copyrighted data in this repository (in any format, textual, JSON, Markdown, HTML, raw binary records, etc.) may require permission from one or more copyright holders**:

* George M. Eberhart - Copyright (c) 2022
* LeRoy Pea - Copyright (c) 9/8/1988 (updated 3/17/2005). Permission was given for use as long as he is attributed.
* Dr. Donald A. Johnson - Copyright (c) 2012. Efforts to contact him by email in 2023 have been unsuccessful.
* Fred Keziah - Copyright (c) 1958. This poster hasn't been updated in 65 years, so it's probably fine for commercial usage.
* Dr. Jacques F. Vallée - Copyright (c) 1993. (This data was published in one of his books - this is the book's copyright.)
* Richard Geldreich, Jr. - Copyright (c) 2023. Commercial usage permitted as long as attribution is given.
* NICAP - Unknown copyright status. The organization went defunct in 1980, 43 years ago, so it's very likely this data is in the Public Domain.
* \*U\* binary database file U.RND: Copyright 1994-2002 [Larry Hatch](https://www.openminds.tv/larry-hatch-ufo-database-creator-remembered/42142). According to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), "The family has verbally given permission for the database to be used. If you need to speak with them, I have their contact info."
* The [Anonymous PDF](https://github.com/richgel999/ufo_data/blob/main/bin/anon_pdf.md) Markdown file (anon_pdf.md), which originally appeared on the internet [here](https://pdfhost.io/v/gR8lAdgVd_Uap_Timeline_Prepared_By_Another) in late July 2023 in Adobe PDF format, is not copyrighted and is in the Public Domain (i.e. it is not Intellectual Property). All ~600 events were explictly and individually marked "PUBLIC DOMAIN" in the original PDF format document. In any jurisdictions outside the US that don't recognize the Public Domain, this Markdown file (and ONLY the specific file anon_pdf.md) uses the [Unlicense](https://web.archive.org/web/20230426084039/https://unlicense.org/).

## Instructions

This C++ command line tool reads several source files containing UFO related events, which were compiled from a variety of sources, and converts each of them to JSON. It then sorts all of these events into a single large timeline and outputs them as the Markdown file "timeline.md" and the JSON file "majestic.json". [pandoc](https://pandoc.org/) can then be used to convert this Markdown text file to HTML.

Currently, the tool is Windows only, but it would be trivial to port to Linux/OSX. (I'll do this, eventually.) To run it: compile the .SLN using Visual Studio 2022. There are no external 3rd party dependencies. Run it in the BIN directory. It'll convert all the source files to JSON then output a file named timeline.md. 

Here's an earlier version of its output [Markdown file converted to PDF](ufo_timeline_v1_04.pdf). The latest HTML timeline is published [here](http://www.subquantumtech.com/timeline/timeline.html).

## Getting started with the JSON data

Check out Larry Hatch's database of ~18k records converted to [utf-8](https://en.wikipedia.org/wiki/UTF-8) [JSON format](https://www.json.org/json-en.html), [here](https://github.com/richgel999/ufo_data/blob/main/bin/hatch_udb.json). The JSON format should be self-explanatory. 

## Special Thanks

* Thanks to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), for assistance in recovering Hatch's DOS-era UFO program.

* Thanks to Jérôme Beau, creator of the ["uDb" project](https://github.com/RR0/uDb), for releasing his database tool. His output and code was very useful.
