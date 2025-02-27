#ifndef __SYSTEMACE_H
#define __SYSTEMACE_H
/*
 * Copyright (c) 2004 Picture Elements, Inc.
 *    Stephen Williams (steve@picturel.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ident "$Id: systemace.h,v 1.1.1.1 2004/10/27 17:21:33 sturgesa Exp $"

#ifdef CONFIG_SYSTEMACE

# include  <part.h>

block_dev_desc_t *  systemace_get_dev(int dev);

#endif	/* CONFIG_SYSTEMACE */
#endif	/* __SYSTEMACE_H */
