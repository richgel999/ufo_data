/* buffer.h - automatic buffer structure */

/*
 * Copyright (c) 2008, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#ifndef LITHIUM_BUFFER_H
#define LITHIUM_BUFFER_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/********************
 * TYPE DEFINITIONS *
 ********************/

/* struct buf • character array buffer */
struct buf {
	char *	data;	/* actual character data */
	size_t	size;	/* size of the string */
	size_t	asize;	/* allocated size (0 = volatile buffer) */
	size_t	unit;	/* reallocation unit size (0 = read-only buffer) */
	int	ref; };	/* reference count */



/**********
 * MACROS *
 **********/

/* CONST_BUF • global buffer from a string literal */
#define CONST_BUF(name, string) \
	static struct buf name = { string, sizeof string -1, sizeof string }


/* VOLATILE_BUF • macro for creating a volatile buffer on the stack */
#define VOLATILE_BUF(name, strname) \
	struct buf name = { strname, strlen(strname) }


/* BUFPUTSL • optimized bufputs of a string literal */
#define BUFPUTSL(output, literal) \
	bufput(output, literal, sizeof literal - 1)


/***********************
 * FUNCTION ATTRIBUTES *
 ***********************/

/* BUF_ALLOCATOR • the function returns a completely new pointer */
#ifdef __GNUC__
#define BUF_ALLOCATOR \
	__attribute__ ((malloc))
#else
#define BUF_ALLOCATOR
#endif


/* BUF_PRINTF_LIKE • marks the function as behaving like printf */
#ifdef __GNUC__
#define BUF_PRINTF_LIKE(format_index, first_variadic_index) \
	__attribute__ ((format (printf, format_index, first_variadic_index)))
#else
#define BUF_PRINTF_LIKE(format_index, first_variadic_index)
#endif


/********************
 * BUFFER FUNCTIONS *
 ********************/

/* bufcasecmp • case-insensitive buffer comparison */
int
bufcasecmp(const struct buf *, const struct buf *);

/* bufcmp • case-sensitive buffer comparison */
int
bufcmp(const struct buf *, const struct buf *);

/* bufcmps • case-sensitive comparison of a string to a buffer */
int
bufcmps(const struct buf *, const char *);

/* bufdup • buffer duplication */
struct buf *
bufdup(const struct buf *, size_t)
	BUF_ALLOCATOR;

/* bufgrow • increasing the allocated size to the given value */
int
bufgrow(struct buf *, size_t);

/* bufnew • allocation of a new buffer */
struct buf *
bufnew(size_t)
	BUF_ALLOCATOR;

/* bufnullterm • NUL-termination of the string array (making a C-string) */
void
bufnullterm(struct buf *);

/* bufprintf • formatted printing to a buffer */
void
bufprintf(struct buf *, const char *, ...)
	BUF_PRINTF_LIKE(2, 3);

/* bufput • appends raw data to a buffer */
void
bufput(struct buf *, const void*, size_t);

/* bufputs • appends a NUL-terminated string to a buffer */
void
bufputs(struct buf *, const char*);

/* bufputc • appends a single char to a buffer */
void
bufputc(struct buf *, char);

/* bufrelease • decrease the reference count and free the buffer if needed */
void
bufrelease(struct buf *);

/* bufreset • frees internal data of the buffer */
void
bufreset(struct buf *);

/* bufset • safely assigns a buffer to another */
void
bufset(struct buf **, struct buf *);

/* bufslurp • removes a given number of bytes from the head of the array */
void
bufslurp(struct buf *, size_t);

/* buftoi • converts the numbers at the beginning of the buf into an int */
int
buftoi(struct buf *, size_t, size_t *);

/* vbufprintf • stdarg variant of formatted printing into a buffer */
void
vbufprintf(struct buf *, const char*, va_list);


/********************
 * GLOBAL VARIABLES *
 ********************/

#ifdef BUFFER_STATS

extern long buffer_stat_nb;
extern size_t buffer_stat_alloc_bytes;

#endif /* def BUFFER_STATS */

#ifdef __cplusplus
}
#endif

#endif /* ndef LITHIUM_BUFFER_H */

/* vim: set filetype=c: */
