# UFO Event Timeline Processing Tool and UAP Event Dataset

To paraphrase [Charles Fort](https://www.forteana.org/fortsbooks.html), this is [*The Dataset of the Damned*](https://archive.org/details/bookdamnedbycha00fortgoog). The included C/C++ command line tool processes a large amount of scraped, scanned, or imported UAP sighting/event data from the original source formats to a common and simple data format (arrays of JSON objects+Github Flavored Markdown for rich text and URL's), making it usable by other applications and tools, or for AI training purposes.

A simple search engine (using WASM+Emscripten) based off this data is live [here](https://www.ufo-search.com/timeline/search.html).

## Copyrights:

### Code:
* [ufojson](https://github.com/richgel999/ufo_data) tool: Copyright (c) 2023 By [Richard Geldreich, Jr.](https://medium.com/@richgel99) (Apache 2.0 license)

* [libsoldout](https://github.com/faelys/libsoldout): Copyright (c) 2009, Natacha Porté (ISC license)

* [json](https://github.com/nlohmann/json): Copyright (c) 2013-2022 Niels Lohmann (MIT license)

* [utf8.h](https://github.com/sheredom/utf8.h): Not copyrighted (unlicense.org)

* Some portions of the specific file udb_tables.h (such as get_hatch_geo, g_hatch_continents) use strings/tables from the ["uDb" project by Jérôme Beau](https://github.com/RR0/uDb).

### Data files (in 'bin' directory): 
Although the C++ tool itself is open source with a permissive license (Apache 2.0), **any commercial use of the copyrighted data in this repository (in any format, textual, JSON, Markdown, HTML, raw binary records, etc.) may require permission from one or more copyright holders**:

* Richard Geldreich, Jr. - Copyright (c) 2023. Commercial usage permitted (for the specific records in maj2.json) as long as attribution is given.
* George M. Eberhart - Copyright (c) 2022. See [_UFOs and Intelligence: A Timeline_](https://cufos.org/resources/ufo-timeline/). This is the [original PDF's](https://cufos.org/PDFs/pdfs/UFOsandIntelligence.pdf) copyright. Official permission to include his chronology event data was given by email on 5/12/2025.
* LeRoy Pea - Copyright (c) 9/8/1988 (updated 3/17/2005). Permission was explictly given for use by e-mail as long as he is attributed.
* Dr. Donald A. Johnson - Copyright (c) 2012. Efforts to contact him by email in 2023 have been unsuccessful.
* Fred Keziah - Copyright (c) 1958. This poster hasn't been updated in 65 years, so it's probably fine for commercial usage.
* Dr. Jacques F. Vallée - Copyright (c) 1993. This data was published in [_Passport to Magonia: From Folklore to Flying Saucers_](https://www.amazon.com/Passport-Magonia-Folklore-Flying-Saucers/dp/0987422480). This is the book's copyright.
* NICAP - Unknown copyright status. The organization went defunct in 1980, 43 years ago, so it's very likely this data is in the Public Domain.
* \*U\* binary database file U.RND: Copyright 1994-2002 [Larry Hatch](https://www.openminds.tv/larry-hatch-ufo-database-creator-remembered/42142). According to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), "The family has verbally given permission for the database to be used. If you need to speak with them, I have their contact info."
* Jérôme Beau - Copyright © 2000- 2023. French chronology data from [rr0.org](https://rr0.org/). See its license [here](https://rr0.org/Copyright.html) (the GNU Free Documentation License).
* [Godelieve Van Overmeire (1935-2021)](http://cobeps.org/fr/godelieve-van-overmeire) - Her French chronology was explictly not copyrighted, as far as I can tell.
* Richard M. Dolan - Copyright 2009. His compilation of UFO events are from the appendix of the book [_UFOs and the National Security State: The Cover-Up Exposed, 1941-1973_](https://www.amazon.com/UFOs-National-Security-State-Chronology-ebook/dp/B0C94W38QY).
* The [Anonymous PDF](https://github.com/richgel999/ufo_data/blob/main/bin/anon_pdf.md) Markdown file (anon_pdf.md), which originally appeared on the internet [here](https://pdfhost.io/v/gR8lAdgVd_Uap_Timeline_Prepared_By_Another) in late July 2023 in Adobe PDF format, is not copyrighted and is in the Public Domain (i.e. it is not Intellectual Property). All ~600 events were explictly and individually marked "PUBLIC DOMAIN" in the original PDF format document. In any jurisdictions outside the US that don't recognize the Public Domain, this Markdown file (and only the specific file anon_pdf.md) uses the [Unlicense](https://web.archive.org/web/20230426084039/https://unlicense.org/). Here's an [HTML conversion](http://subquantumtech.com/anon_pdf/anon_pdf.html) of this Markdown file, using the [markdown-styles](https://github.com/mixu/markdown-styles) command line tool (you can also use `pandoc -f gfm`):
   *  generate-md --layout github --input ./anon_pdf.md --output anon_pdf
* Thomas R. Adams - Copyright 1991. See the event data in the book [_"The Choppers and the Choppers - Mystery Helicopters and Animal Mutilations"_](https://ia801409.us.archive.org/33/items/adams-thomas-choppers-and-the-choppers-1/Adams%2CThomas%2CChoppers%20and%20the%20Choppers-1_text.pdf). Contact efforts ongoing.
* George D. Fawcett - 1961: [_The Flying Saucers are Hostile!_](https://archive.org/details/the_flying_saucers_are_hostile_george_d_fawcett_1961/mode/2up) - Unknown copyright status, however it states "published with the specific request of the author that no editing or cutting be done, the editors guarantee that this is a word for word presentation of Mr. Fawcett's manuscript." The JSON data in this repo derived from this manuscript is uncut and unedited.

## Instructions

This C++ command line tool reads several source files containing UFO related events, which were compiled from a variety of sources, and converts each of them to JSON. It then sorts all of these events into a single large timeline and outputs them as the Markdown file "timeline.md" and the JSON file "majestic.json". [pandoc](https://pandoc.org/) can then be used to convert this Markdown text file to HTML.

Currently, the tool is Windows only, but it would be trivial to port to Linux/OSX. (I'll do this, eventually.) To run it: compile the .SLN using Visual Studio 2019 or 2022. There are no external 3rd party dependencies. Run it in the BIN directory with the -convert command line option. It'll convert all the source files to JSON then output a file named timeline.md. 

Here's an earlier version of its output [Markdown file converted to PDF](ufo_timeline_v1_04.pdf). The latest HTML timeline is published [here](http://www.subquantumtech.com/timeline/timeline.html).

## Getting started with the JSON data

Check out Larry Hatch's database of ~18k records converted to [utf-8](https://en.wikipedia.org/wiki/UTF-8) [JSON format](https://www.json.org/json-en.html), [here](https://github.com/richgel999/ufo_data/blob/main/bin/hatch_udb.json). The JSON format should be fairly self-explanatory. Also see his [DOS tool](https://archive.org/details/u_full_version), which is where this data came from.

## Special Thanks

* Thanks to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), for assistance in recovering Hatch's DOS-era UFO program.

* Thanks to Jérôme Beau, creator of the ["uDb" project](https://github.com/RR0/uDb), for releasing his database tool. His output and code was very useful.
