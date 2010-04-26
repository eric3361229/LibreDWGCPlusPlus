/*****************************************************************************/
/*  LibreDWG - Free DWG library                                              */
/*                                                                           */
/*    based on LibDWG - Free DWG read-only library                           */
/*    http://sourceforge.net/projects/libdwg                                 */
/*    originally written by Felipe Castro <felipo at users.sourceforge.net>  */
/*                                                                           */
/*  Copyright (C) 2008, 2009 Free Software Foundation, Inc.                  */
/*  Copyright (C) 2009 Felipe Corrêa da Silva Sanches <juca@members.fsf.org> */
/*  Copyright (C) 2009 Rodrigo Rodrigues da Silva <pitanga@members.fsf.org>  */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 3 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

///  Functions for decoding dwg data-structures.
#ifndef DECODE_H
#define DECODE_H

#include "bits.h"
#include "dwg.h"

int dwg_decode_data(Bit_Chain * bit_chain, Dwg_Data * dwg_data);
int decode_R2004(Bit_Chain * bit_chain, Dwg_Data * dwg_data);
int decode_R2007(Bit_Chain * bit_chain, Dwg_Data * dwg_data);


#endif
