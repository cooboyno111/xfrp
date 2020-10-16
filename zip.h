/* vim: set et ts=4 sts=4 sw=4 : */
/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/** @file zip.h
    @brief zlib related function
    @author Copyright (C) 2016 Dengfeng Liu <liudengfeng@kunteng.org>
*/

#ifndef _ZIP_H_
#define _ZIP_H_

#define CHUNK   16384  
#define windowBits 		15
#define GZIP_ENCODING 	16

typedef unsigned char uint8;

int deflate_write(uint8 *source, int len, uint8 **dest, int *wlen, int gzip);

int inflate_read(uint8 *source, int len, uint8 **dest, int *rlen, int gzip);

#endif
