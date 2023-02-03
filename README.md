# UFO Event Timeline Processing Tool
Copyright (C) 2023 By Richard Geldreich, Jr.

## Instructions

This C++ command line tool reads several source files containing UFO related events, which were compiled from a variety of sources, and converts each of them to JSON. It then sorts all of these events into a single large timeline and outputs them as Markdown. [pandoc](https://pandoc.org/) can then be used to convert this Markdown text to HTML.

Currently, the tool is Windows only, but it would be trivial to port to Linux/OSX. (I'll do this soon.) To run it: compile the .SLN using Visual Studio 2022. There are no 3rd party dependencies. Run it in the BIN directory. It'll convert all the source files to JSON then output a file named timeline.md. 

Here's the output [Markdown file converted to PDF](ufo_timeline_v1_04.pdf).

