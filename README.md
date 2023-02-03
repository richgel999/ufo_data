# UFO Event Timeline Processing Tool

Copyrights:
ufojson tool: Copyright (C) 2023 By Richard Geldreich, Jr. (Apache 2.0 license)
[libsoldout](https://github.com/faelys/libsoldout): Copyright (c) 2009, Natacha Porté  
Data files: George M. Eberhart - Copyright 2022, LeRoy Pea - Copyright 9/8/1988 (updated 3/17/2005), Dr. Donald A. Johnson - Copyright 2012, Fred Keziah - Copyright 1958

## Instructions

This C++ command line tool reads several source files containing UFO related events, which were compiled from a variety of sources, and converts each of them to JSON. It then sorts all of these events into a single large timeline and outputs them as Markdown. [pandoc](https://pandoc.org/) can then be used to convert this Markdown text to HTML.

Currently, the tool is Windows only, but it would be trivial to port to Linux/OSX. (I'll do this soon.) To run it: compile the .SLN using Visual Studio 2022. There are no 3rd party dependencies. Run it in the BIN directory. It'll convert all the source files to JSON then output a file named timeline.md. 

Here's the output [Markdown file converted to PDF](ufo_timeline_v1_04.pdf).

