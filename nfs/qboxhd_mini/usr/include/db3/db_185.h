/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: db_185.h,v 11.4 2000/02/14 02:59:54 bostic Exp $
 */

#ifndef _DB_185_H_
#define	_DB_185_H_

#include <sys/types.h>

#include <limits.h>

/*
 * XXX
 * Handle function prototypes and the keyword "const".  This steps on name
 * space that DB doesn't control, but all of the other solutions are worse.
 */
#undef	__P
#if defined(__STDC__) || defined(__cplusplus)
#define	__P(protos)	protos		/* ANSI C prototypes */
#else
#define	const
#define	__P(protos)	()		/* K&R C preprocessor */
#endif

#define	RET_ERROR	-1		/* Return values. */
#define	RET_SUCCESS	 0
#define	RET_SPECIAL	 1

#ifndef	__BIT_TYPES_DEFINED__
#define	__BIT_TYPES_DEFINED__





#endif

/*
 * XXX
 * SGI/IRIX already has a pgno_t.
 */
#ifdef	sgi
#define	pgno_t	db_pgno_t
#endif

#define	MAX_PAGE_NUMBER	0xffffffff	/* >= # of pages in a file */
typedef u_int32_t	pgno_t;
#define	MAX_PAGE_OFFSET	65535		/* >= # of bytes in a page */
typedef u_int16_t	indx_t;
#define	MAX_REC_NUMBER	0xffffffff	/* >= # of records in a tree */
typedef u_int32_t	recno_t;

/* Key/data structure -- a Data-Base Thang. */
typedef struct {
	void	*data;			/* data */
	size_t	 size;			/* data length */
} DBT;

/* Routine flags. */
#define	R_CURSOR	1		/* del, put, seq */
#define	__R_UNUSED	2		/* UNUSED */
#define	R_FIRST		3		/* seq */
#define	R_IAFTER	4		/* put (RECNO) */
#define	R_IBEFORE	5		/* put (RECNO) */
#define	R_LAST		6		/* seq (BTREE, RECNO) */
#define	R_NEXT		7		/* seq */
#define	R_NOOVERWRITE	8		/* put */
#define	R_PREV		9		/* seq (BTREE, RECNO) */
#define	R_SETCURSOR	10		/* put (RECNO) */
#define	R_RECNOSYNC	11		/* sync (RECNO) */

typedef enum { DB_BTREE, DB_HASH, DB_RECNO } DBTYPE;

/* Access method description structure. */
typedef struct __db {
	DBTYPE type;			/* Underlying db type. */
	int (*close)	__P((struct __db *));
	int (*del)	__P((const struct __db *, const DBT *, u_int));
	int (*get)	__P((const struct __db *, const DBT *, DBT *, u_int));
	int (*put)	__P((const struct __db *, DBT *, const DBT *, u_int));
	int (*seq)	__P((const struct __db *, DBT *, DBT *, u_int));
	int (*sync)	__P((const struct __db *, u_int));
	void *internal;			/* Access method private. */
	int (*fd)	__P((const struct __db *));
} DB;

#define	BTREEMAGIC	0x053162
#define	BTREEVERSION	3

/* Structure used to pass parameters to the btree routines. */
typedef struct {
#define	R_DUP		0x01	/* duplicate keys */
	u_int32_t flags;
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t maxkeypage;	/* maximum keys per page */
	u_int32_t minkeypage;	/* minimum keys per page */
	u_int32_t psize;	/* page size */
	int	(*compare)	/* comparison function */
	    __P((const DBT *, const DBT *));
	size_t	(*prefix)	/* prefix function */
	    __P((const DBT *, const DBT *));
	int	lorder;		/* byte order */
} BTREEINFO;

#define	HASHMAGIC	0x061561
#define	HASHVERSION	2

/* Structure used to pass parameters to the hashing routines. */
typedef struct {
	u_int32_t bsize;	/* bucket size */
	u_int32_t ffactor;	/* fill factor */
	u_int32_t nelem;	/* number of elements */
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t		/* hash function */
		(*hash) __P((const void *, size_t));
	int	lorder;		/* byte order */
} HASHINFO;

/* Structure used to pass parameters to the record routines. */
typedef struct {
#define	R_FIXEDLEN	0x01	/* fixed-length records */
#define	R_NOKEY		0x02	/* key not required */
#define	R_SNAPSHOT	0x04	/* snapshot the input */
	u_int32_t flags;
	u_int32_t cachesize;	/* bytes to cache */
	u_int32_t psize;	/* page size */
	int	lorder;		/* byte order */
	size_t	reclen;		/* record length (fixed-length records) */
	u_char	bval;		/* delimiting byte (variable-length records */
	char	*bfname;	/* btree file name */
} RECNOINFO;

#if defined(__cplusplus)
extern "C" {
#endif
#define	dbopen	__db185_open
DB *__db185_open __P((const char *, int, int, DBTYPE, const void *));

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_185_H_ */
