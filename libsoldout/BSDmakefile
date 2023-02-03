# Makefile

# Copyright (c) 2009, Natacha Porté
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

DEPDIR	 = depends
ALLDEPS	 = $(DEPDIR)/all

AR	?= ar
CC	?= cc
CFLAGS	?= -g -O3 -Wall -Werror
LDFLAGS	?=

all:		libsoldout.a libsoldout.so mkd2html mkd2latex mkd2man

.PHONY:		all amal clean


# amalgamation
amal:
	@./make-amal


# libraries

libsoldout.a:	markdown.o array.o buffer.o renderers.o
	$(AR) rs $(.TARGET) $(.ALLSRC)

libsoldout.so:	libsoldout.so.1
	ln -s $(.ALLSRC) $(.TARGET)

libsoldout.so.1:	markdown.o array.o buffer.o renderers.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname=$(.TARGET) \
		$(.ALLSRC) -o $(.TARGET)


# executables

mkd2html:	mkd2html.o libsoldout.so
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

mkd2latex:	mkd2latex.o libsoldout.so
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

mkd2man:	mkd2man.o libsoldout.so
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)


# Housekeeping

GNUmakefile:	BSDmakefile
	@sed	-e 's/^\(all:.*\)GNUmakefile /\1/'			\
		-e 's/\(rm .*\)GNUmakefile /\1/'			\
		-e '/^GNUmakefile:/,/^$$/d'				\
		-e 's/\$$(\.ALLSRC)/$$^/g'				\
		-e 's/\$$(\.IMPSRC)/$$</g'				\
		-e 's/\$$(\.OODATE)/$$?/g'				\
		-e 's/\$$(\.MEMBER)/$$%/g'				\
		-e 's/\$$(\.PREFIX)/$$*/g'				\
		-e 's/\$$(\.TARGET)/$$@/g'				\
		-e 's/^\.sinclude/-include/'				\
		-e 's/\.include/include/' $(.ALLSRC) > $(.TARGET)

benchmark:	benchmark.o libsoldout.so
	$(CC) $(LDFLAGS) $(.ALLSRC) -o $(.TARGET)

clean:
	rm -f *.o
	rm -f libsoldout.a libsoldout.so libsoldout.so.*
	rm -f mkd2html mkd2latex mkd2man benchmark
	rm -rf $(DEPDIR)


# dependencies

.sinclude "$(ALLDEPS)"


# generic object compilations

.c.o:
	@mkdir -p $(DEPDIR)
	@touch $(ALLDEPS)
	@$(CC) -MM $(.IMPSRC) > $(DEPDIR)/$(.PREFIX).d
	@grep -q "$(.PREFIX).d" $(ALLDEPS) \
			|| echo ".include \"$(.PREFIX).d\"" >> $(ALLDEPS)
	$(CC) $(CFLAGS) -std=c99 -fPIC -c -o $(.TARGET) $(.IMPSRC)
