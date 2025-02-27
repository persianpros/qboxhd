/*
 * JFFS2 -- Journalling Flash File System, Version 2.
 *
 * Copyright (C) 2001 Red Hat, Inc.
 *
 * Created by Arjan van de Ven <arjanv@redhat.com>
 *
 * The original JFFS, from which the design for JFFS2 was derived,
 * was designed and implemented by Axis Communications AB.
 *
 * The contents of this file are subject to the Red Hat eCos Public
 * License Version 1.1 (the "Licence"); you may not use this file
 * except in compliance with the Licence.  You may obtain a copy of
 * the Licence at http://www.redhat.com/
 *
 * Software distributed under the Licence is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing rights and
 * limitations under the Licence.
 *
 * The Original Code is JFFS2 - Journalling Flash File System, version 2
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the RHEPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the RHEPL or the GPL.
 *
 * $Id: compr_rtime.c,v 1.1.1.1 2004/10/27 17:21:32 sturgesa Exp $
 *
 *
 * Very simple lz77-ish encoder.
 *
 * Theory of operation: Both encoder and decoder have a list of "last
 * occurances" for every possible source-value; after sending the
 * first source-byte, the second byte indicated the "run" length of
 * matches
 *
 * The algorithm is intended to only send "whole bytes", no bit-messing.
 *
 */

#include <config.h>
#if (CONFIG_COMMANDS & CFG_CMD_JFFS2)

#include <jffs2/jffs2.h>

void rtime_decompress(unsigned char *data_in, unsigned char *cpage_out,
		      u32 srclen, u32 destlen)
{
	int positions[256];
	int outpos;
	int pos;
	int i;

	outpos = pos = 0;

	for (i = 0; i < 256; positions[i++] = 0);

	while (outpos<destlen) {
		unsigned char value;
		int backoffs;
		int repeat;

		value = data_in[pos++];
		cpage_out[outpos++] = value; /* first the verbatim copied byte */
		repeat = data_in[pos++];
		backoffs = positions[value];

		positions[value]=outpos;
		if (repeat) {
			if (backoffs + repeat >= outpos) {
				while(repeat) {
					cpage_out[outpos++] = cpage_out[backoffs++];
					repeat--;
				}
			} else {
				for (i = 0; i < repeat; i++)
					*(cpage_out + outpos + i) = *(cpage_out + backoffs + i);
				outpos+=repeat;
			}
		}
	}
}

#endif /* CFG_CMD_JFFS2 */
