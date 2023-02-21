# UFO Event Timeline Processing Tool

## Copyrights:
* [ufojson](https://github.com/richgel999/ufo_data) tool: Copyright (c) 2023 By [Richard Geldreich, Jr.](https://medium.com/@richgel99) (Apache 2.0 license)

* [libsoldout](https://github.com/faelys/libsoldout): Copyright (c) 2009, Natacha Porté (ISC license)

* [json](https://github.com/nlohmann/json): Copyright (c) 2013-2022 Niels Lohmann (MIT license)

* Some portions of the specific file udb_tables.h (such as get_hatch_geo, g_hatch_continents) use strings/tables from the ["uDb" project by Jérôme Beau](https://github.com/RR0/uDb).

### Data files (in 'bin' directory): 
* George M. Eberhart - Copyright (c) 2022
* LeRoy Pea - Copyright (c) 9/8/1988 (updated 3/17/2005)
* Dr. Donald A. Johnson - Copyright (c) 2012
* Fred Keziah - Copyright (c) 1958
* Dr. Jacques F. Vallée - Copyright (c) 1993
* Richard Geldreich, Jr. - Copyright (c) 2023
* NICAP - Unknown copyright status
* \*U\* binary database file U.RND: Copyright 1994-2002 [Larry Hatch](https://www.openminds.tv/larry-hatch-ufo-database-creator-remembered/42142). According to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), "The family has verbally given permission for the database to be used. If you need to speak with them, I have their contact info."

Note: Although the C++ tool itself is open source with a permissive license (Apache 2.0), any commercial use of the copyrighted data in this repository may require permission from one or more copyright holders.

## Instructions

This C++ command line tool reads several source files containing UFO related events, which were compiled from a variety of sources, and converts each of them to JSON. It then sorts all of these events into a single large timeline and outputs them as the Markdown file "timeline.md" and the JSON file "majestic.json". [pandoc](https://pandoc.org/) can then be used to convert this Markdown text file to HTML.

Currently, the tool is Windows only, but it would be trivial to port to Linux/OSX. (I'll do this soon.) To run it: compile the .SLN using Visual Studio 2022. There are no external 3rd party dependencies. Run it in the BIN directory. It'll convert all the source files to JSON then output a file named timeline.md. 

Here's the output [Markdown file converted to PDF](ufo_timeline_v1_04.pdf). The latest HTML timeline is published [here](http://www.subquantumtech.com/timeline/timeline.html).

## Special Thanks

* Thanks to David Marler, Executive Director of the [National UFO Historical Records Center](http://www.nufohrc.org/), for assistance in recovering Hatch's DOS-era UFO program.

* Thanks to Jérôme Beau, creator of the ["uDb" project](https://github.com/RR0/uDb), for releasing his database tool. His output and code was very useful.
