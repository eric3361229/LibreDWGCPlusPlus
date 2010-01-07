/*****************************************************************************/
/*  LibreDWG - Free DWG library                                              */
/*  http://code.google.com/p/libredwg/                                       */
/*                                                                           */
/*    based on LibDWG - Free DWG read-only library                           */
/*    http://sourceforge.net/projects/libdwg                                 */
/*    originally written by Felipe Castro <felipo at users.sourceforge.net>  */
/*                                                                           */
/*  Copyright (C) 2008, 2009 Free Software Foundation, Inc.                  */
/*  Copyright (C) 2009 Rodrigo Rodrigues da Silva <pitanga@members.fsf.org>  */
/*  Copyright (C) 2009 Felipe Sanches <jucablues@users.sourceforge.net>      */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either versionn 3 of the License, or (at your option) any later versionn.*/
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

/// Decode

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "bits.h"
#include "dwg.h"
#include "decode.h"
//#include "iostream.h"

//using  namespace std;
int decode_R2004(Bit_Chain* dat, Dwg_Data * dwg);
int decode_R2007(Bit_Chain* dat, Dwg_Data * dwg);

/*--------------------------------------------------------------------------------
 * Welcome to the dark side of the moon...
 * MACROS
 */

#define IS_DECODER

#define FIELD(name,type)\
  _obj->name = bit_read_##type(dat);\
  if (loglevel>=2)\
    {\
        fprintf(stderr, #name ": " FORMAT_##type "\n", _obj->name);\
    }

#define FIELD_VALUE(name) _obj->name

#define ANYCODE -1
#define FIELD_HANDLE(name, handle_code)\
  if (handle_code>=0)\
    {\
      _obj->name = dwg_decode_handleref_with_code(dat, obj, dwg, handle_code);\
    }\
  else\
    {\
      _obj->name = dwg_decode_handleref(dat, obj, dwg);\
    }\
  if (loglevel>=2)\
    {\
      fprintf(stderr, #name ": HANDLE(%d.%d.%lu) absolute:%lu\n",\
        _obj->name->handleref.code,\
        _obj->name->handleref.size,\
        _obj->name->handleref.value,\
        _obj->name->absolute_ref);\
    }

#define FIELD_B(name) FIELD(name, B);
#define FIELD_BB(name) FIELD(name, BB);
#define FIELD_BS(name) FIELD(name, BS);
#define FIELD_BL(name) FIELD(name, BL);
#define FIELD_BD(name) FIELD(name, BD);
#define FIELD_RC(name) FIELD(name, RC);
#define FIELD_RS(name) FIELD(name, RS);
#define FIELD_RD(name) FIELD(name, RD);
#define FIELD_RL(name) FIELD(name, RL);
#define FIELD_MC(name) FIELD(name, MC);
#define FIELD_MS(name) FIELD(name, MS);
#define FIELD_TV(name) FIELD(name, TV);
#define FIELD_T FIELD_TV /*TODO: implement version dependant string fields */
#define FIELD_BT(name) FIELD(name, BT); 
#define FIELD_4BITS(name) _obj->name = bit_read_4BITS(dat);

#define FIELD_BE(name) bit_read_BE(dat, &_obj->name.x, &_obj->name.y, &_obj->name.z);
#define FIELD_DD(name, _default) FIELD_VALUE(name) = bit_read_DD(dat, _default);
#define FIELD_2DD(name, d1, d2) FIELD_DD(name.x, d1); FIELD_DD(name.y, d2);  
#define FIELD_2RD(name) FIELD(name.x, RD); FIELD(name.y, RD);
#define FIELD_2BD(name) FIELD(name.x, BD); FIELD(name.y, BD);
#define FIELD_3RD(name) FIELD(name.x, RD); FIELD(name.y, RD); FIELD(name.z, RD);
#define FIELD_3BD(name) FIELD(name.x, BD); FIELD(name.y, BD); FIELD(name.z, BD);
#define FIELD_3DPOINT(name) FIELD_3BD(name)
#define FIELD_CMC(name)\
  {\
    bit_read_CMC(dat, &_obj->name);\
    if (loglevel>=2)\
      fprintf(stderr, #name ": index %d\n", _obj->name.index);\
  }

//FIELD_VECTOR_N(name, type, size):
// reads data of the type indicated by 'type' 'size' times and stores
// it all in the vector called 'name'.
#define FIELD_VECTOR_N(name, type, size)\
  if (size>0)\
    {\
      _obj->name = (BITCODE_##type*) malloc(size * sizeof(BITCODE_##type));\
      for (vcount=0; vcount< size; vcount++)\
        {\
          _obj->name[vcount] = bit_read_##type(dat);\
          if (loglevel>=2)\
            {\
                fprintf(stderr, #name "[%d]: " FORMAT_##type "\n", vcount, _obj->name[vcount]);\
            }\
        }\
    }

#define FIELD_VECTOR(name, type, size) FIELD_VECTOR_N(name, type, _obj->size)

#define FIELD_2RD_VECTOR(name, size)\
  _obj->name = (BITCODE_2RD *) malloc(_obj->size * sizeof(BITCODE_2RD));\
  for (vcount=0; vcount< _obj->size; vcount++)\
    {\
      FIELD_2RD(name[vcount]);\
    }

#define FIELD_3DPOINT_VECTOR(name, size)\
  _obj->name = (BITCODE_3DPOINT *) malloc(_obj->size * sizeof(BITCODE_3DPOINT));\
  for (vcount=0; vcount< _obj->size; vcount++)\
    {\
      FIELD_3DPOINT(name[vcount]);\
    }

#define HANDLE_VECTOR_N(name, size, code)\
  FIELD_VALUE(name) = (BITCODE_H*) malloc(sizeof(BITCODE_H) * size);\
  for (vcount=0; vcount<size; vcount++)\
    {\
      FIELD_HANDLE(name[vcount], code);\
    }

#define HANDLE_VECTOR(name, sizefield, code) HANDLE_VECTOR_N(name, FIELD_VALUE(sizefield), code)

#define REACTORS(code)\
  FIELD_VALUE(reactors) = (BITCODE_H*) malloc(sizeof(BITCODE_H) * obj->tio.object->num_reactors);\
  for (vcount=0; vcount<obj->tio.object->num_reactors; vcount++)\
    {\
      FIELD_HANDLE(reactors[vcount], code);\
    }

#define XDICOBJHANDLE(code)\
  SINCE(R_2004)\
    {\
      if (!obj->tio.object->xdic_missing_flag)\
        FIELD_HANDLE(xdicobjhandle, code);\
    }\
  PRIOR_VERSIONS\
    {\
      FIELD_HANDLE(xdicobjhandle, code);\
    }

#define REPEAT_N(times, name, type) \
  _obj->name = (type *) malloc(times * sizeof(type));\
  for (rcount=0; rcount<times; rcount++)

#define REPEAT(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount=0; rcount<_obj->times; rcount++)

#define REPEAT2(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount2=0; rcount2<_obj->times; rcount2++)

#define REPEAT3(times, name, type) \
  _obj->name = (type *) malloc(_obj->times * sizeof(type));\
  for (rcount3=0; rcount3<_obj->times; rcount3++)

//TODO unify REPEAT macros!

#define COMMON_ENTITY_HANDLE_DATA \
  dwg_decode_common_entity_handle_data(dat, obj)

#define DWG_ENTITY(token) \
static void \
 dwg_decode_##token (Bit_Chain * dat, Dwg_Object * obj)\
{\
  int vcount, rcount, rcount2, rcount3;\
  if (loglevel)\
  fprintf (stderr, "Entity " #token ":\n");\
	Dwg_Entity_##token *ent, *_obj;\
	Dwg_Data* dwg = obj->parent;\
	dwg->num_entities++;\
	obj->supertype = DWG_SUPERTYPE_ENTITY;\
	obj->tio.entity = (Dwg_Object_Entity*)malloc (sizeof (Dwg_Object_Entity));	\
	obj->tio.entity->tio.token = (Dwg_Entity_##token *)calloc (sizeof (Dwg_Entity_##token), 1); \
	ent = obj->tio.entity->tio.token;\
  _obj=ent;\
  obj->tio.entity->object = obj;\
	if (dwg_decode_entity (dat, obj->tio.entity)) return;\
  if (loglevel) fprintf (stderr, "Entity handle: %d.%d.%lu\n",\
    obj->handle.code,\
    obj->handle.size,\
    obj->handle.value);

#define DWG_ENTITY_END }

#define DWG_OBJECT(token) static void  dwg_decode_ ## token (Bit_Chain * dat, Dwg_Object * obj) {\
  int vcount, rcount, rcount2, rcount3;\
  if (loglevel)\
    fprintf (stderr, "Object " #token ":\n");\
	Dwg_Object_##token *_obj;\
	Dwg_Data* dwg = obj->parent;\
	obj->supertype = DWG_SUPERTYPE_OBJECT;\
	obj->tio.object = (Dwg_Object_Object*)malloc (sizeof (Dwg_Object_Object));	\
	obj->tio.object->tio.token = (Dwg_Object_##token * ) calloc (sizeof (Dwg_Object_##token), 1); \
  obj->tio.object->object = obj;\
	if (dwg_decode_object (dat, obj->tio.object)) return;\
	_obj = obj->tio.object->tio.token;\
  if (loglevel) fprintf (stderr, "Object handle: %d.%d.%lu\n",\
    obj->handle.code,\
    obj->handle.size,\
    obj->handle.value);

#define DWG_OBJECT_END }

/*--------------------------------------------------------------------------------
 * Private functions
 */
static void
dwg_decode_add_object(Dwg_Data * dwg, Bit_Chain * dat,
    long unsigned int address);

static Dwg_Object *
dwg_resolve_handle(Dwg_Data* dwg, unsigned long int handle);

static void
dwg_decode_header_variables(Bit_Chain* dat, Dwg_Data * dwg);

/*--------------------------------------------------------------------------------
 * Public variables
 */
long unsigned int ktl_lastaddress;

static int loglevel = 1;

int decode_R13_R15(Bit_Chain* dat, Dwg_Data * dwg); // froward

/*--------------------------------------------------------------------------------
 * Public function definitions
 */
int
dwg_decode_data(Bit_Chain * dat, Dwg_Data * dwg)
{
  char version[7];
  dwg->num_object_refs = 0;
  dwg->num_layers = 0;
  dwg->num_entities = 0;
  dwg->num_objects = 0;
  dwg->num_classes = 0;

  /* Version */
  dat->byte = 0;
  dat->bit = 0;
  strncpy(version, (const char *)dat->chain, 6);
  version[6] = '\0';

  dwg->header.version = 0;
  if (!strcmp(version, version_codes[R_13]))
    dwg->header.version = R_13;
  if (!strcmp(version, version_codes[R_14]))
    dwg->header.version = R_14;
  if (!strcmp(version, version_codes[R_2000]))
    dwg->header.version = R_2000;
  if (!strcmp(version, version_codes[R_2004]))
    dwg->header.version = R_2004;
  if (!strcmp(version, version_codes[R_2007]))
    dwg->header.version = R_2007;
  if (dwg->header.version == 0)
    {
      fprintf(stderr, "Invalid or unimplemented version code!"
        "This file's version code is: %s\n", version);
      return -1;
    }
  dat->version = (Dwg_Version_Type)dwg->header.version;
  if (loglevel) fprintf(stderr,
      "This file's version code is: %s\n", version);

  if (dat->version < R_2000)
    {
      fprintf(
          stderr,
          "WARNING: This version of LibreDWG is only capable of safely decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s Support for this version is still experimental.\n"
            "It might crash or give you invalid output.\n", version);
      return decode_R13_R15(dat, dwg);
    }

  VERSION(R_2000)
    {
      return decode_R13_R15(dat, dwg);
    }

  VERSION(R_2004)
    {
      fprintf(
          stderr,
          "WARNING: This version of LibreDWG is only capable of properly decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s\n This version is not yet actively developed.\n"
            "It will probably crash and/or give you invalid output.\n", version);
      return decode_R2004(dat, dwg);
    }

  VERSION(R_2007)
    {
      fprintf(
          stderr,
          "WARNING: This version of LibreDWG is only capable of properly decoding version R2000 (code: AC1015) dwg-files.\n"
            "This file's version code is: %s\n This version is not yet actively developed.\n"
            "It will probably crash and/or give you invalid output.\n", version);
      return decode_R2007(dat, dwg);
    }

  //This line should not be reached!
  fprintf(
      stderr,
      "ERROR: LibreDWG could not recognize the version string in this file: %s.\n",
      version);
  return -1;
}

Dwg_Object* dwg_next_object(Dwg_Object* obj){
  if ((obj->index+1) > obj->parent->num_objects-1)
    return 0;
  return &obj->parent->object[obj->index+1];
}

int
decode_R13_R15(Bit_Chain* dat, Dwg_Data * dwg)
{
  unsigned char sig;
  unsigned int section_size = 0;
  unsigned char sgdc[2];
  unsigned int ckr, ckr2, antckr;
  long unsigned int size;
  long unsigned int lasta;
  long unsigned int maplasta;
  long unsigned int duabyte;
  long unsigned int object_begin;
  long unsigned int object_end;
  long unsigned int pvz;
  unsigned int i, j;

  // Still unknown values: 6 'zeroes' and a 'one'
  dat->byte = 0x06;
  if (loglevel)
    fprintf(stderr, "Still unknown values: 6 'zeroes' and a 'one': ");
  for (i = 0; i < 7; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "0x%02X ", sig);
    }
  if (loglevel)
    fprintf(stderr, "\n");

  /* Image Seeker */
  pvz = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Image seeker: 0x%08X\n", (unsigned int) pvz);

  // unknown
  sig = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "Version: %u\n", sig);
  sig = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "Lancxo: %u\n", sig);

  /* Codepage */
  dat->byte = 0x13;
  dwg->header.codepage = bit_read_RS(dat);
  if (loglevel)
    fprintf(stderr, "Codepage: %u\n", dwg->header.codepage);

  /* Section Locator Records */
  dat->byte = 0x15;
  dwg->header.num_sections = bit_read_RL(dat);

  //  why do we have this limit to only 6 sections?
  //  It seems to be a bug, so I'll comment it out and will add dynamic
  //  allocation of the sections vector.
  //  OpenDWG spec speaks of 6 possible values for the record number
  //  Maybe the original libdwg author got confused about that.
  /*
   if (dwg->header.num_sections > 6)
   dwg->header.num_sections = 6;
   */
  dwg->header.section = (Dwg_Section*) malloc(sizeof(Dwg_Section)
      * dwg->header.num_sections);

  for (i = 0; i < dwg->header.num_sections; i++)
    {
      dwg->header.section[i].number = bit_read_RC(dat);
      dwg->header.section[i].address = bit_read_RL(dat);
      dwg->header.section[i].size = bit_read_RL(dat);
    }

  // Check CRC-on
  /*
   ckr = bit_read_CRC (dat);
   dat->byte -= 2;
   bit_create_CRC (dat, 0, 0);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   dat->byte -= 2;
   bit_write_RS (dat, ckr2 ^ 0x8461);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   if (loglevel) fprintf (stderr, "Read: %X\nCalculated: %X\n", ckr, ckr2);
   */

  if (bit_search_sentinel(dat, dwg_sentinel(DWG_SENTINEL_HEADER_END))
      && loglevel)
    fprintf(stderr, "=======> HEADER (end): %8X\n", (unsigned int) dat->byte);

  /*-------------------------------------------------------------------------
   * Unknown section 1
   */

  if (dwg->header.num_sections == 6)
    {
      if (loglevel)
        {
          fprintf(stderr, "========> UNKNOWN 1: %8X\n",
              (unsigned int) dwg->header.section[5].address);
          fprintf(stderr, "   UNKNOWN 1 (end): %8X\n",
              (unsigned int) (dwg->header.section[5].address
                  + dwg->header.section[5].size));
        }
      dat->byte = dwg->header.section[5].address;
      dwg->unknown1.size = DWG_UNKNOWN1_SIZE;
      dwg->unknown1.byte = dwg->unknown1.bit = 0;
      dwg->unknown1.chain = (unsigned char*)malloc(dwg->unknown1.size);
      memcpy(dwg->unknown1.chain, &dat->chain[dat->byte], dwg->unknown1.size);
      //bit_explore_chain ((Bit_Chain *) &dwg->unknown1, dwg->unknown1.size);
      //bit_print ((Bit_Chain *) &dwg->unknown1, dwg->unknown1.size);
    }

  /*-------------------------------------------------------------------------
   * Picture (Pre-R13C3?)
   */

  if (bit_search_sentinel(dat, dwg_sentinel(DWG_SENTINEL_PICTURE_BEGIN)))
    {
      unsigned long int start_address;

      dat->bit = 0;
      start_address = dat->byte;
      if (loglevel)
        fprintf(stderr, "=============> PICTURE: %8X\n",
            (unsigned int) start_address - 16);
      if (bit_search_sentinel(dat, dwg_sentinel(DWG_SENTINEL_PICTURE_END)))
        {
          if (loglevel)
            fprintf(stderr, "        PICTURE (end): %8X\n",
                (unsigned int) dat->byte);
          dwg->picture.size = (dat->byte - 16) - start_address;
          dwg->picture.chain = (unsigned char *) malloc(dwg->picture.size);
          memcpy(dwg->picture.chain, &dat->chain[start_address],
              dwg->picture.size);
        }
      else
        dwg->picture.size = 0;
    }

  /*-------------------------------------------------------------------------
   * Header Variables
   */

  if (loglevel)
    {
      fprintf(stderr, "=====> Header Variables: %8X\n",
          (unsigned int) dwg->header.section[0].address);
      fprintf(stderr, "Header Variables (end): %8X\n",
          (unsigned int) (dwg->header.section[0].address
              + dwg->header.section[0].size));
    }
  dat->byte = dwg->header.section[0].address + 16;
  pvz = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Length: %lu\n", pvz);

  dat->bit = 0;

  dwg_decode_header_variables(dat, dwg);

// Check CRC-on
  ckr = bit_read_CRC(dat);
  /*
   for (i = 0xC001; i != 0xC000; i++)
   {
   dat->byte -= 2;
   bit_write_CRC (dat, dwg->header.section[0].address + 16, i);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   if (ckr == ckr2)
   {
   if (loglevel) fprintf (stderr, "Read: %X\nCreated: %X\t SEMO: %02X\n", ckr, ckr2, i);
   break;
   }
   }
   */

  /*-------------------------------------------------------------------------
   * Classes
   */
  if (loglevel)
    {
      fprintf(stderr, "============> CLASS: %8X\n",
          (unsigned int) dwg->header.section[1].address);
      fprintf(stderr, "       CLASS (end): %8X\n",
          (unsigned int) (dwg->header.section[1].address
              + dwg->header.section[1].size));
    }
  dat->byte = dwg->header.section[1].address + 16;
  dat->bit = 0;

  size = bit_read_RL(dat);
  lasta = dat->byte + size;
  //if (loglevel) fprintf (stderr, "Length: %lu\n", size);

  /* read the classes
   */
  dwg->dwg_ot_layout = 0;
  dwg->num_classes = 0;
  i = 0;
  do
    {
      unsigned int idc;

      idc = dwg->num_classes; if (idc == 0)
        dwg->dwg_class = (Dwg_Class *) malloc(sizeof(Dwg_Class));
      else
        dwg->dwg_class = (Dwg_Class *) realloc(dwg->dwg_class, (idc + 1)
            * sizeof(Dwg_Class));

      dwg->dwg_class[idc].number = bit_read_BS(dat);
      dwg->dwg_class[idc].version = bit_read_BS(dat);
      dwg->dwg_class[idc].appname = bit_read_TV(dat);
      dwg->dwg_class[idc].cppname = bit_read_TV(dat);
      dwg->dwg_class[idc].dxfname = bit_read_TV(dat);
      dwg->dwg_class[idc].wasazombie = bit_read_B(dat);
      dwg->dwg_class[idc].item_class_id = bit_read_BS(dat);

      if (strcmp((const char *)dwg->dwg_class[idc].dxfname, "LAYOUT") == 0)
        dwg->dwg_ot_layout = dwg->dwg_class[idc].number;

      dwg->num_classes++;
      if (dwg->num_classes > 100)
				{
					fprintf(stderr, "number of classes is greater than 100. TODO: Why should we stop here?\n");
		      break;//TODO: Why?!
				}
    }
  while (dat->byte < (lasta - 1));

  // Check CRC-on
  ckr = bit_read_CRC(dat);
  /*
   for (i = 0xC001; i != 0xC000; i++)
   {
   dat->byte -= 2;
   bit_write_CRC (dat, dwg->header.section[1].address + 16, i);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   if (ckr == ckr2)
   {
   if (loglevel) fprintf (stderr, "Read: %X\nCreated: %X\t SEMO: %02X\n", ckr, ckr2, i);
   break;
   }
   }
   */

  dat->byte += 16;
  pvz = bit_read_RL(dat); // Unknown bitlong inter class and object
  //if (loglevel) {
  // fprintf (stderr, "Address: %lu / Content: 0x%08X\n", dat->byte - 4, pvz);
  // fprintf (stderr, "Number of classes read: %u\n", dwg->num_classes);
  //}

  /*-------------------------------------------------------------------------
   * Object-map (object mem)
   */

  dat->byte = dwg->header.section[2].address;
  dat->bit = 0;

  maplasta = dat->byte + dwg->header.section[2].size; // 4
  dwg->num_objects = 0;
  object_begin = dat->size;
  object_end = 0;
  do
    {
      long unsigned int last_address;
      long unsigned int last_handle;
      long unsigned int previous_address = 0;

      duabyte = dat->byte;
      sgdc[0] = bit_read_RC(dat);
      sgdc[1] = bit_read_RC(dat);
      section_size = (sgdc[0] << 8) | sgdc[1];
      if (loglevel) fprintf (stderr, "section_size: %u\n", section_size);
      if (section_size > 2034) // 2032 + 2
        {
          fprintf(stderr, "Error: Object-map section size greater than 2034!");
          //return -1;
        }

      last_handle = 0;
      last_address = 0;
      while (dat->byte - duabyte < section_size)
        {
          long unsigned int kobj;
          long int pvztkt;
          long int pvzadr;

          previous_address = dat->byte;
          pvztkt = bit_read_MC(dat);
          last_handle += pvztkt;
          pvzadr = bit_read_MC(dat);
          last_address += pvzadr;
          //if (loglevel) {
          // fprintf (stderr, "Idc: %li\t", dwg->num_objects);
          // fprintf (stderr, "Handle: %li\tAddress: %li\n", pvztkt, pvzadr);
          //}
          if (dat->byte == previous_address)
            break;
          //if (dat->byte - duabyte >= seksize)
          //break;

          if (object_end < last_address)
            object_end = last_address;
          if (object_begin > last_address)
            object_begin = last_address;

          kobj = dwg->num_objects;
          dwg_decode_add_object(dwg, dat, last_address);
          //if (dwg->num_objects > kobj)
          //dwg->object[dwg->num_objects - 1].handle.value = lastahandle;
          //TODO: blame Juca
        }
      if (dat->byte == previous_address)
        break;
      dat->byte += 2; // CRC

      if (dat->byte >= maplasta)
        break;
    }
  while (section_size > 2);

  if (loglevel)
    {
      fprintf(stderr, "Num objects: %lu\n", dwg->num_objects);
      fprintf(stderr, "=========> Object Data: %8X\n", (unsigned int) object_begin);
    }
  dat->byte = object_end;
  object_begin = bit_read_MS(dat);
  if (loglevel)
    fprintf(stderr, "    Object Data (end): %8X\n", (unsigned int) (object_end + object_begin
        + 2));

  /*
   dat->byte = dwg->header.section[2].address - 2;
   antckr = bit_read_CRC (dat); // Unknown bitdouble inter object data and object map
   if (loglevel) fprintf (stderr, "Address: %08X / Content: 0x%04X\n", dat->byte - 2, antckr);

   // check CRC-on
   antckr = 0xC0C1;
   do
   {
   duabyte = dat->byte;
   sgdc[0] = bit_read_RC (dat);
   sgdc[1] = bit_read_RC (dat);
   section_size = (sgdc[0] << 8) | sgdc[1];
   section_size -= 2;
   dat->byte += section_size;
   ckr = bit_read_CRC (dat);
   dat->byte -= 2;
   bit_write_CRC (dat, duabyte, antckr);
   dat->byte -= 2;
   ckr2 = bit_read_CRC (dat);
   if (loglevel) fprintf (stderr, "Read: %X\nCreated: %X\t SEMO: %X\n", ckr, ckr2, antckr);
   //antckr = ckr;
   } while (section_size > 0);
   */
  if (loglevel)
    {
      fprintf(stderr, "======> Object Map: %8X\n",
          (unsigned int) dwg->header.section[2].address);
      fprintf(stderr, " Object Map (end): %8X\n",
          (unsigned int) (dwg->header.section[2].address
              + dwg->header.section[2].size));
    }

  /*-------------------------------------------------------------------------
   * Dua kap-datenaro
   */

  if (bit_search_sentinel(dat, dwg_sentinel(DWG_SENTINEL_SECOND_HEADER_BEGIN)))
    {
      long unsigned int pvzadr;
      long unsigned int pvz;
      unsigned char sig, sig2;

      if (loglevel)
        fprintf(stderr, "==> Second Header: %8X\n", (unsigned int) dat->byte
            - 16);
      pvzadr = dat->byte;

      pvz = bit_read_RL(dat);
      //if (loglevel) fprintf (stderr, "Size: %lu\n", pvz);

      pvz = bit_read_BL(dat);
      //if (loglevel) fprintf (stderr, "Begin address: %8X\n", pvz);

      //if (loglevel) fprintf (stderr, "AC1015?: ");
      for (i = 0; i < 6; i++)
        {
          sig = bit_read_RC(dat);
          //if (loglevel) fprintf (stderr, "%c", sig >= ' ' && sig < 128 ? sig : '.');
        }

      //if (loglevel) fprintf (stderr, "\nNull?:");
      for (i = 0; i < 5; i++) // 6 if is older...
        {
          sig = bit_read_RC(dat);
          //if (loglevel) fprintf (stderr, " 0x%02X", sig);
        }

      //if (loglevel) fprintf (stderr, "\n4 null bits?: ");
      for (i = 0; i < 4; i++)
        {
          sig = bit_read_B(dat);
          //if (loglevel) fprintf (stderr, " %c", sig ? '1' : '0');
        }

      //if (loglevel) fprintf (stderr, "\nChain?: ");
      for (i = 0; i < 6; i++)
        {
          dwg->second_header.unknown[i] = bit_read_RC(dat);
          //if (loglevel) fprintf (stderr, " 0x%02X", dwg->second_header.unknown[i]);
        }
      if (dwg->second_header.unknown[3] != 0x78
          || dwg->second_header.unknown[5] != 0x06)
        sig = bit_read_RC(dat); // To compensate in the event of a contingent
                                // additional zero not read previously

      //puts("");
      for (i = 0; i < 6; i++)
        {
          sig = bit_read_RC(dat);
          //if (loglevel) fprintf (stderr, "[%u]\n", sig);
          pvz = bit_read_BL(dat);
          //if (loglevel) fprintf (stderr, " Address: %8X\n", pvz);
          pvz = bit_read_BL(dat);
          //if (loglevel) fprintf (stderr, "  Size: %8X\n", pvz);
        }

      bit_read_BS(dat);
      //if (loglevel) fprintf (stderr, "\n14 --------------");
      for (i = 0; i < 14; i++)
        {
          sig2 = bit_read_RC(dat);
          dwg->second_header.handlerik[i].size = sig2;
          //if (loglevel) fprintf (stderr, "\nSize: %u\n", sig2);
          sig = bit_read_RC(dat);
          //if (loglevel) fprintf (stderr, "\t[%u]\n", sig);
          //if (loglevel) fprintf (stderr, "\tChain:");
          for (j = 0; j < sig2; j++)
            {
              sig = bit_read_RC(dat);
              dwg->second_header.handlerik[i].chain[j] = sig;
              //if (loglevel) fprintf (stderr, " %02X", sig);
            }
        }

      // Check CRC-on
      ckr = bit_read_CRC(dat);
      /*
       puts ("");
       for (i = 0; i != 0xFFFF; i++)
       {
       dat->byte -= 2;
       bit_write_CRC (dat, pvzadr, i);
       dat->byte -= 2;
       ckr2 = bit_read_CRC (dat);
       if (ckr == ckr2)
       {
       if (loglevel) fprintf (stderr, "Read: %X\nCreated: %X\t SEMO: %02X\n", ckr, ckr2, i);
       break;
       }
       }
       if (loglevel) {
       fprintf (stderr, " Garbage 1: %08X\n", bit_read_RL (dat));
       fprintf (stderr, " Garbage 2: %08X\n", bit_read_RL (dat));
       }
       */

      if (loglevel && bit_search_sentinel(dat, dwg_sentinel(
          DWG_SENTINEL_SECOND_HEADER_END)))
        fprintf(stderr, " Second Header (end): %8X\n", (unsigned int) dat->byte);
    }

  /*-------------------------------------------------------------------------
   * Section MEASUREMENT
   */

  if (loglevel)
    {
      fprintf(stderr, "========> Unknown 2: %8X\n",
          (unsigned int) dwg->header.section[4].address);
      fprintf(stderr, "   Unknown 2 (end): %8X\n",
          (unsigned int) (dwg->header.section[4].address
              + dwg->header.section[4].size));
    }
  dat->byte = dwg->header.section[4].address;
  dat->bit = 0;
  dwg->measurement = bit_read_RL(dat);

  if (loglevel)
    fprintf(stderr, "Size bytes :\t%lu\n", dat->size);

  //step II of handles parsing: resolve pointers from handle value
  //XXX: move this somewhere else
  for (i = 0; i < dwg->num_object_refs; i++)
    {
      dwg->object_ref[i]->obj = dwg_resolve_handle(dwg,
          dwg->object_ref[i]->absolute_ref);
    }

  return 0;
}

/* R2004 Literal Length
 */
int
read_literal_length(Bit_Chain* dat, unsigned char *opcode)
{
  int total = 0;
  unsigned char byte = bit_read_RC(dat);

  *opcode = 0x00;

  if (byte >= 0x01 && byte <= 0x0F)
    return byte + 3;
  else if (byte == 0)
    {
      total = 0x0F;
      while ((byte = bit_read_RC(dat)) == 0x00)
        {
          total += 0xFF;
        }
      return total + byte + 3;
    }
  else if (byte & 0xF0)
    *opcode = byte;

  return 0;
}

/* R2004 Long Compression Offset
 */
int read_long_compression_offset(Bit_Chain* dat)
{
  int total = 0;
  unsigned char byte = bit_read_RC(dat);
  if (byte >= 0x01 && byte <= 0xFF)
    return byte;
  // else byte = 0x00
  total = 0xFF;
  while ((byte = bit_read_RC(dat)) == 0x00)
      total += 0xFF;
  return total + byte;
}

/* R2004 Two Byte Offset
 */
int read_two_byte_offset(Bit_Chain* dat, int* lit_length)
{
  int offset;
  unsigned char firstByte = bit_read_RC(dat);
  offset = (firstByte >> 2) | (bit_read_RC(dat) << 6);
  *lit_length = (firstByte & 0x03);
  return offset;
}

/* Decompresses a system section of a 2004 DWG flie
 */
int
decompress_R2004_section(Bit_Chain* dat, char *decomp, 
                         unsigned long int comp_data_size)
{
  int lit_length, i;
  int comp_offset, comp_bytes;
  unsigned char opcode1 = 0, opcode2;
  long unsigned int start_byte = dat->byte;
  char *src, *dst = decomp;

  // length of the first sequence of uncompressed or literal data.
  lit_length = read_literal_length(dat, &opcode1);

  for (i = 0; i < lit_length; ++i)
    *dst++ = bit_read_RC(dat);

  opcode1 = 0x00;
  while (dat->byte - start_byte < comp_data_size)
    { 
      if (opcode1 == 0x00)
        opcode1 = bit_read_RC(dat);

      if (opcode1 >= 0x40 && opcode1 <= 0xFF)
        {
          comp_bytes = ((opcode1 & 0xF0) >> 4) - 1;
          opcode2 = bit_read_RC(dat);
          comp_offset = (opcode2 << 2) | ((opcode1 & 0x0C) >> 2);

          if (opcode1 & 0x03)
            {
              lit_length = (opcode1 & 0x03);
              opcode1  = 0x00;
            }
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 >= 0x21 && opcode1 <= 0x3F)
        {
          comp_bytes  = opcode1 - 0x1E;
          comp_offset = read_two_byte_offset(dat, &lit_length);

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 == 0x20)
        {
          comp_bytes  = read_long_compression_offset(dat) + 0x21;
          comp_offset = read_two_byte_offset(dat, &lit_length);

          if (lit_length != 0)
            opcode1 = 0x00;
          else
            lit_length = read_literal_length(dat, &opcode1);
        }
      else if (opcode1 >= 0x12 && opcode1 <= 0x1F)
        {
	  printf( "got opcode1: %d\n" ,opcode1);
	  //          assert(0);  // Not seen yet - could not check

          comp_bytes  = (opcode1 & 0x0F) + 2;
          comp_offset = read_two_byte_offset(dat, &lit_length) - 0x3FFF;

	  printf( "got offset: %d\n" , comp_offset);

          if (lit_length != 0)
            opcode1 = 0x00;
          else
	    {
	      lit_length = read_literal_length(dat, &opcode1);
	      printf("got read opecode: %d length:%d\n",opcode1, lit_length);
	    }

        }
      else if (opcode1 == 0x10)
        {
	  //          assert(0);  // Not seen yet - could not check

          comp_bytes  = read_long_compression_offset(dat) - 9;
          comp_offset = read_two_byte_offset(dat, &lit_length) - 0x3FFF;

          if (lit_length != 0)
            opcode1 = 0x00;
          else
	    {
	      lit_length = read_literal_length(dat, &opcode1);
	      printf("got read opecode: %d length:%d\n",opcode1, lit_length);
	    }
        }
      else if (opcode1 == 0x11)
        {         
          break;     // Terminates the input stream, everything is ok!
        }
      else
        {
          return 1;  // error in input stream 
        }      

      printf("got compressed data %d\n",comp_bytes);
      // copy "compressed data"
      src = dst - comp_offset - 1;
      assert(src > decomp);
      for (i = 0; i < comp_bytes; ++i)
        *dst++ = *src++;
 
      // copy "literal data" 
      printf("got literal data %d\n",lit_length);
      for (i = 0; i < lit_length; ++i)
        *dst++ = bit_read_RC(dat);
    }  

  return 0;  // Success
}

/* Read R2004 Section Map 
 * The Section Map is a vector of number, size, and address triples used
 * to locate the sections in the file. 
 */
void
read_R2004_section_map(Bit_Chain* dat, Dwg_Data * dwg, 
                       unsigned long int comp_data_size, 
                       unsigned long int decomp_data_size)
{
  char *decomp, *ptr;
  int i;
  int section_address;

  dwg->header.num_sections = 0;
  dwg->header.section = 0;

  // allocate memory to hold decompressed data
  decomp = (char *)malloc(decomp_data_size * sizeof(char));
  if (decomp == 0)
    return;   // No memory

  decompress_R2004_section(dat, decomp, comp_data_size);  

  if (loglevel)
    fprintf(stderr, "\n#### 2004 Section Map fields ####\n\n");
  
  section_address = 0x100;  // starting address 
	i = 0;
	int bytes_remaining = decomp_data_size;
	ptr = decomp;
	dwg->header.num_sections = 0;

	while(bytes_remaining)
		{
			if (dwg->header.num_sections==0)
				dwg->header.section = (Dwg_Section*) malloc(sizeof(Dwg_Section));
			else
				dwg->header.section = (Dwg_Section*) realloc(dwg->header.section, sizeof(Dwg_Section) * (dwg->header.num_sections+1));

		  dwg->header.section[i].number  = *((int*)ptr);
		  dwg->header.section[i].size    = *((int*)ptr+1);
		  dwg->header.section[i].address = section_address;
		  section_address += dwg->header.section[i].size;
			bytes_remaining -= 8;
			ptr+=8;

		  if (loglevel)
		    {
		      fprintf(stderr, "SectionNumber: %d\n",   dwg->header.section[i].number);
		      fprintf(stderr, "SectionSize:   %x\n",   dwg->header.section[i].size);
		      fprintf(stderr, "SectionAddr:   %x\n\n", dwg->header.section[i].address);
		    }

			if (dwg->header.section[i].number<0)
				{
			    dwg->header.section[i].parent  = *((int*)ptr);
			    dwg->header.section[i].left  = *((int*)ptr+1);
			    dwg->header.section[i].right  = *((int*)ptr+2);
			    dwg->header.section[i].x00  = *((int*)ptr+3);
					bytes_remaining -= 16;
					ptr+=16;

			    if (loglevel)
			      {
				      fprintf(stderr, "Parent: %d\n",   (int)dwg->header.section[i].parent);
				      fprintf(stderr, "Feft: %d\n",   (int)dwg->header.section[i].left);
				      fprintf(stderr, "Right: %d\n",   (int)dwg->header.section[i].right);
				      fprintf(stderr, "0x00: %d\n\n",   (int)dwg->header.section[i].x00);
						}
				}

			dwg->header.num_sections++;
			i++;
		}
  free(decomp);
}

Dwg_Section* find_section(Dwg_Data *dwg, unsigned long int index)
{
  int i;
  if (dwg->header.section == 0 || index == 0)
    return 0;
  for (i = 0; i < dwg->header.num_sections; ++i)
    {
      if (dwg->header.section[i].number == index)
        return (&dwg->header.section[i]);
    }
  return 0;
}

/* Read R2004 Section Info
 */
void
read_R2004_section_info(Bit_Chain* dat, Dwg_Data *dwg, 
                        unsigned long int comp_data_size, 
                        unsigned long int decomp_data_size)
{  
  char *decomp, *ptr;
  int i, j;
  int section_number;
  int data_size;
  int start_offset;
  int unknown;
  Dwg_Section *section;

  decomp = (char *)malloc(decomp_data_size * sizeof(char));
  if (decomp == 0)
    return;   // No memory

  decompress_R2004_section(dat, decomp, comp_data_size);
  
  memcpy(&dwg->header.num_descriptions, decomp, 4);  
  dwg->header.section_info = (Dwg_Section_Info*) 
    malloc(sizeof(Dwg_Section_Info) * dwg->header.num_descriptions);  
  
  if (loglevel)
    {
      fprintf(stderr, "\n#### 2004 Section Info fields ####\n\n");
      fprintf(stderr, "NumDescriptions: %d\n", *((int*)decomp));
      fprintf(stderr, "0x02:            %x\n", *((int*)decomp + 1));
      fprintf(stderr, "0x00007400:      %x\n", *((int*)decomp + 2));
      fprintf(stderr, "0x00:            %x\n", *((int*)decomp + 3));
      fprintf(stderr, "Unknown:         %x\n", *((int*)decomp + 4));
    }

  ptr = decomp + 20;
  for (i = 0; i < dwg->header.num_descriptions; ++i)
    {      
      //memcpy(&dwg->header.section_info[i], ptr, 96);  
      dwg->header.section_info[i].size            = *((int*)ptr);
      dwg->header.section_info[i].unknown1 	      = *((int*)ptr + 1);
      dwg->header.section_info[i].num_sections    = *((int*)ptr + 2);
      dwg->header.section_info[i].max_decomp_size = *((int*)ptr + 3);  
      dwg->header.section_info[i].unknown2        = *((int*)ptr + 4);	   
      dwg->header.section_info[i].compressed      = *((int*)ptr + 5);
      dwg->header.section_info[i].type            = *((int*)ptr + 6);
      dwg->header.section_info[i].encrypted       = *((int*)ptr + 7);
      ptr += 32;
      memcpy(dwg->header.section_info[i].name, ptr, 64);      
      ptr += 64;

      if (loglevel)
        {
          fprintf(stderr, "\nSection Info description fields\n\n");
          fprintf(stderr, "Size:                  %d\n", 
           (int) dwg->header.section_info[i].size);
          fprintf(stderr, "Unknown:               %d\n", 
           (int) dwg->header.section_info[i].unknown1);	   
          fprintf(stderr, "Number of sections:    %d\n", 
           (int) dwg->header.section_info[i].num_sections);
          fprintf(stderr, "Max decompressed size: %d\n", 
           (int) dwg->header.section_info[i].max_decomp_size);
          fprintf(stderr, "Unknown:               %d\n", 
           (int) dwg->header.section_info[i].unknown2);
          fprintf(stderr, "Compressed (0x02):     %x\n", 
           (unsigned int) dwg->header.section_info[i].compressed);
          fprintf(stderr, "Section Type:          %d\n", 
           (int) dwg->header.section_info[i].type);
          fprintf(stderr, "Encrypted:             %d\n", 
           (int) dwg->header.section_info[i].encrypted);
          fprintf(stderr, "SectionName:           %s\n\n", 
            dwg->header.section_info[i].name);
        }

      dwg->header.section_info[i].sections = (Dwg_Section**)
        malloc(dwg->header.section_info[i].num_sections * sizeof(Dwg_Section*));

      if (dwg->header.section_info[i].num_sections < 10000)
	{
	  fprintf(stderr, "section count %d in area %d\n",dwg->header.section_info[i].num_sections,i);
	  
	  for (j = 0; j < dwg->header.section_info[i].num_sections; j++)
	    {
	      section_number = *((int*)ptr);      // Index into SectionMap
	      data_size      = *((int*)ptr + 1);
	      start_offset   = *((int*)ptr + 2);
	      unknown        = *((int*)ptr + 3);  // high 32 bits of 64-bit start offset?
	      ptr += 16;
	      
	      dwg->header.section_info[i].sections[j] = find_section(dwg, section_number);
	      
	      if (loglevel)
		{
		  fprintf(stderr, "-------------------\n");
		  fprintf(stderr, "Section Number: %d\n", section_number);
		  fprintf(stderr, "Data size:      %d\n", data_size);
		  fprintf(stderr, "Start offset:   %x\n", start_offset);
		  fprintf(stderr, "Unknown:        %d\n", unknown);
		}
	    }
	}// sanity check
      else
	{
	  fprintf(stderr, "section count %d in area %d too high! skipping\n",dwg->header.section_info[i].num_sections,i); 
	}
    }
  free(decomp);
}

/* Encrypted Section Header */
typedef union _encrypted_section_header
{
  unsigned long int long_data[8];
  unsigned char char_data[32];
  struct
  {    
    unsigned long int tag;
    unsigned long int section_type;
    unsigned long int data_size;
    unsigned long int section_size;
    unsigned long int start_offset;
    unsigned long int unknown;
    unsigned long int checksum_1;
    unsigned long int checksum_2;    
  } fields;
} encrypted_section_header;

/* R2004 Class Section
 */
void 
read_2004_class_section(Bit_Chain* dat, Dwg_Data *dwg)
{
  char sentinel[] = {0x8D, 0xA1, 0xC4, 0xB8, 0xC4, 0xA9, 0xF8, 0xC5, 0xC0, 
    0xDC, 0xF4, 0x5F, 0xE7, 0xCF, 0xB6, 0x8A};
  unsigned long int size;
  unsigned long int max_num;
  unsigned long int num_objects, dwg_version, maint_version, unknown;
  unsigned long int class_address, secMask;
  unsigned int class_number;
  Dwg_Section_Info *info = 0;
  encrypted_section_header es;
  char *decomp, *ptr;
  char c;
  int i;

  Bit_Chain class_dat;

  for (i = 0; i < dwg->header.num_descriptions && info == 0; ++i) 
      if (dwg->header.section_info[i].type == 3)
        info = &dwg->header.section_info[i];

  if (info == 0)
    return;

  class_address = info->sections[0]->address;
  dat->byte = class_address;

  fprintf(stderr, "\n\nRaw section header bytes:\n");
  for (i = 0; i < 0x20; i++)
    {
      es.char_data[i] = bit_read_RC(dat);            
      fprintf(stderr, "%x ", es.char_data[i]);
    }

  secMask = 0x4164536b ^ class_address;
  for (i = 0; i < 8; ++i)
    es.long_data[i] ^= secMask;

  if (loglevel)
    {
      fprintf(stderr, "\n\n=== Section (Class) ===\n");
      fprintf(stderr, "Section Tag (should be 0x4163043b): %x\n",
          (unsigned int) es.fields.tag);
      fprintf(stderr, "Section Type:     %x\n",
          (unsigned int) es.fields.section_type);
      fprintf(stderr, "DecompDataSize:   %x\n",
          (unsigned int) es.fields.data_size);
      fprintf(stderr, "CompDataSize:     %x\n",
          (unsigned int) es.fields.section_size);
      fprintf(stderr, "StartOffset:      %x\n",
          (unsigned int) es.fields.start_offset);
      fprintf(stderr, "Checksum1:        %x\n", 
        (unsigned int) es.fields.checksum_1);
      fprintf(stderr, "Checksum2:        %x\n\n", 
        (unsigned int) es.fields.checksum_2);
    }
  
  decomp = (char *)malloc(info->max_decomp_size * sizeof(char));
  if (decomp == 0)
    return;   // No memory

  decompress_R2004_section(dat, decomp, es.fields.section_size);

  class_dat.bit     = 0;
  class_dat.byte    = 0;
  class_dat.chain   = (unsigned char *)decomp;
  class_dat.size    = info->max_decomp_size;
  class_dat.version = dat->version;

  if (bit_search_sentinel(&class_dat, dwg_sentinel(DWG_SENTINEL_CLASS_BEGIN)))
    { 
      int start = class_dat.byte - 16;

      size    = bit_read_RL(&class_dat);  // size of class data area
      max_num = bit_read_BS(&class_dat);  // Maxiumum class number
      c = bit_read_RC(&class_dat);        // 0x00
      c = bit_read_RC(&class_dat);        // 0x00
      c = bit_read_B(&class_dat);         // 1

      dwg->dwg_ot_layout = 0;
      dwg->num_classes = 0;
      i = 0;
      do
        {
          unsigned int idc;

          idc = dwg->num_classes;
          if (idc == 0)
            dwg->dwg_class = (Dwg_Class *) malloc(sizeof(Dwg_Class));
          else
            dwg->dwg_class = (Dwg_Class *) realloc(dwg->dwg_class, (idc + 1)
                * sizeof(Dwg_Class));

          dwg->dwg_class[idc].number        = bit_read_BS(&class_dat);
          dwg->dwg_class[idc].version       = bit_read_BS(&class_dat);
          dwg->dwg_class[idc].appname       = bit_read_TV(&class_dat);
          dwg->dwg_class[idc].cppname       = bit_read_TV(&class_dat);
          dwg->dwg_class[idc].dxfname       = bit_read_TV(&class_dat);
          dwg->dwg_class[idc].wasazombie    = bit_read_B(&class_dat);
          dwg->dwg_class[idc].item_class_id = bit_read_BS(&class_dat);

          num_objects   = bit_read_BL(&class_dat);  // DXF 91
          dwg_version   = bit_read_BS(&class_dat);  // Dwg Version
          maint_version = bit_read_BS(&class_dat);  // Maintenance release version.
          unknown       = bit_read_BL(&class_dat);  // Unknown (normally 0L)
          unknown       = bit_read_BL(&class_dat);  // Unknown (normally 0L)

          if (loglevel)
            {
              fprintf(stderr, "-------------------\n");
              fprintf(stderr, "Number:           %d\n", dwg->dwg_class[idc].number);
              fprintf(stderr, "Version:          %x\n", dwg->dwg_class[idc].version);
              fprintf(stderr, "Application name: %s\n", dwg->dwg_class[idc].appname);
              fprintf(stderr, "C++ class name:   %s\n", dwg->dwg_class[idc].cppname);
              fprintf(stderr, "DXF record name:  %s\n", dwg->dwg_class[idc].dxfname);
            }

          if (strcmp((const char *)dwg->dwg_class[idc].dxfname, "LAYOUT") == 0)
            dwg->dwg_ot_layout = dwg->dwg_class[idc].number;

          dwg->num_classes++;

        } while (class_dat.byte < (size - 1));
    }
}

int
decode_R2004(Bit_Chain* dat, Dwg_Data * dwg)
{
  /* Encripted Data */
  union
  {
    unsigned char encripted_data[0x6c];
    struct
    {
      unsigned char file_ID_string[12];
      unsigned int x00;
      unsigned int x6c;
      unsigned int x04;
      unsigned int root_tree_node_gap;
      unsigned int lowermost_left_tree_node_gap;
      unsigned int lowermost_right_tree_node_gap;
      unsigned int unknown_long;
      unsigned int last_section_id;
      unsigned int last_section_address;
      unsigned int x00_2;
      unsigned int second_header_address;
      unsigned int x00_3;
      unsigned int gap_amount;
      unsigned int section_amount;
      unsigned int x20;
      unsigned int x80;
      unsigned int x40;
      unsigned int section_map_id;
      unsigned int section_map_address;
      unsigned int x00_4;
      unsigned int section_info_id;
      unsigned int section_array_size;
      unsigned int gap_array_size;
      unsigned int CRC;
    } fields;
  } _2004_header_data;

  /* System Section */
  typedef union _system_section
  {
    unsigned char data[0x14];
    struct
    {
      unsigned int section_type;   //0x4163043b
      unsigned int decomp_data_size;
      unsigned int comp_data_size;
      unsigned int compression_type;
      unsigned int checksum;
    } fields;
  } system_section;

  encrypted_section_header es;
  unsigned long int secMask = 0x4164536b;
  system_section ss;
  int rseed = 1;

  Dwg_Section *section;

  int i;
  unsigned long int preview_address, security_type, unknown_long,
      dwg_property_address, vba_proj_address;
  unsigned char sig, dwg_ver, maint_release_ver;

  //6 bytes of 0x00
  dat->byte = 0x06;
  if (loglevel)
    fprintf(stderr, "6 bytes of 0x00: ");
  for (i = 0; i < 6; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "0x%02X ", sig);
    }
  if (loglevel)
    fprintf(stderr, "\n");

  /* Byte 0x00, 0x01, or 0x03 */
  dat->byte = 0x0C;
  sig = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "Byte 0x00, 0x01, or 0x03: 0x%02X\n", sig);

  /* Preview Address */
  dat->byte = 0x0D;
  preview_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Preview Address: 0x%08X\n", (unsigned int) preview_address);

  /* DwgVer */
  dat->byte = 0x11;
  dwg_ver = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "DwgVer: %u\n", dwg_ver);

  /* MaintReleaseVer */
  dat->byte = 0x12;
  maint_release_ver = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "MaintRelease: %u\n", maint_release_ver);

  /* Codepage */
  dat->byte = 0x13;
  dwg->header.codepage = bit_read_RS(dat);
  if (loglevel)
    fprintf(stderr, "Codepage: %u\n", dwg->header.codepage);

  /* 3 0x00 bytes */
  dat->byte = 0x15;
  if (loglevel)
    fprintf(stderr, "3 0x00 bytes: ");
  for (i = 0; i < 3; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "0x%02X ", sig);
    }
  if (loglevel)
    fprintf(stderr, "\n");

  /* SecurityType */
  dat->byte = 0x18;
  security_type = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "SecurityType: 0x%08X\n", (unsigned int) security_type);

  /* Unknown long */
  dat->byte = 0x1C;
  unknown_long = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Unknown long: 0x%08X\n", (unsigned int) unknown_long);

  /* DWG Property Addr */
  dat->byte = 0x20;
  dwg_property_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "DWG Property Addr: 0x%08X\n",
        (unsigned int) dwg_property_address);

  /* VBA Project Addr */
  dat->byte = 0x24;
  vba_proj_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "VBA Project Addr: 0x%08X\n",
        (unsigned int) vba_proj_address);

  /* 0x00000080 */
  dat->byte = 0x28;
  unknown_long = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "0x00000080: 0x%08X\n", (unsigned int) unknown_long);

  /* 0x00 bytes (length = 0x54 bytes) */
  dat->byte = 0x2C;
  for (i = 0; i < 0x54; i++)
    {
      sig = bit_read_RC(dat);
      if (sig != 0 && loglevel)
        fprintf(stderr,
            "Warning: Byte should be zero! But a value=%x was read instead.\n",
            sig);
    }

  dat->byte = 0x80;
  for (i = 0; i < 0x6c; i++)
    {
      rseed *= 0x343fd;
      rseed += 0x269ec3;
      _2004_header_data.encripted_data[i] = bit_read_RC(dat) ^ (rseed >> 0x10);
    }

  if (loglevel)
    {
      fprintf(stderr, "\n#### 2004 File Header Data fields ####\n\n");
      fprintf(stderr, "File ID string (must be AcFssFcAJMB): ");
      for (i = 0; i < 12; i++)
        fprintf(stderr, "%c", _2004_header_data.fields.file_ID_string[i]);
      fprintf(stderr, "\n");
      fprintf(stderr, "0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00);
      fprintf(stderr, "0x6c (long): %x\n",
          (unsigned int) _2004_header_data.fields.x6c);
      fprintf(stderr, "0x04 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x04);
      fprintf(stderr, "Root tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.root_tree_node_gap);
      fprintf(stderr, "Lowermost left tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.lowermost_left_tree_node_gap);
      fprintf(stderr, "Lowermost right tree node gap: %x\n",
          (unsigned int) _2004_header_data.fields.lowermost_right_tree_node_gap);
      fprintf(stderr, "Unknown long: %x\n",
          (unsigned int) _2004_header_data.fields.unknown_long);
      fprintf(stderr, "Last section id: %x\n",
          (unsigned int) _2004_header_data.fields.last_section_id);
      fprintf(stderr, "Last section address: %x\n",
          (unsigned int) _2004_header_data.fields.last_section_address);
      fprintf(stderr, "0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_2);
      fprintf(stderr, "Second header address: %x\n",
          (unsigned int) _2004_header_data.fields.second_header_address);
      fprintf(stderr, "0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_3);
      fprintf(stderr, "Gap amount: %x\n",
          (unsigned int) _2004_header_data.fields.gap_amount);
      fprintf(stderr, "Section amount: %x\n",
          (unsigned int) _2004_header_data.fields.section_amount);
      fprintf(stderr, "0x20 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x20);
      fprintf(stderr, "0x80 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x80);
      fprintf(stderr, "0x40 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x40);
      fprintf(stderr, "Section map id: %x\n",
          (unsigned int) _2004_header_data.fields.section_map_id);
      fprintf(stderr, "Section map address: %x\n",
          (unsigned int) _2004_header_data.fields.section_map_address + 0x100);
      fprintf(stderr, "0x00 (long): %x\n",
          (unsigned int) _2004_header_data.fields.x00_4);
      fprintf(stderr, "Section Info id: %x\n",
          (unsigned int) _2004_header_data.fields.section_info_id);
      fprintf(stderr, "Section array size: %x\n",
          (unsigned int) _2004_header_data.fields.section_array_size);
      fprintf(stderr, "Gap array size: %x\n",
          (unsigned int) _2004_header_data.fields.gap_array_size);
      fprintf(stderr, "CRC: %x\n", (unsigned int) _2004_header_data.fields.CRC);
    }
  
  /*-------------------------------------------------------------------------
   * Section Map
   */
  dat->byte = _2004_header_data.fields.section_map_address + 0x100;

  if (loglevel)
	  fprintf(stderr, "\n\nRaw system section bytes:\n");
  for (i = 0; i < 0x14; i++)
    {
      ss.data[i] = bit_read_RC(dat);
		  if (loglevel)
				fprintf(stderr, "%x ", ss.data[i]);
    }

  if (loglevel)
    {
      fprintf(stderr, "\n\n=== System Section (Section Map) ===\n");
      fprintf(stderr, "Section Type (should be 0x4163043b): %x\n",
          (unsigned int) ss.fields.section_type);
      fprintf(stderr, "DecompDataSize: %x\n",
          (unsigned int) ss.fields.decomp_data_size);
      fprintf(stderr, "CompDataSize: %x\n",
          (unsigned int) ss.fields.comp_data_size);
      fprintf(stderr, "Compression Type: %x\n",
          (unsigned int) ss.fields.compression_type);
      fprintf(stderr, "Checksum: %x\n\n", (unsigned int) ss.fields.checksum);
    }

  read_R2004_section_map(dat, dwg,
      ss.fields.comp_data_size, ss.fields.decomp_data_size);

  if (dwg->header.section == 0)
    {
      fprintf(stderr, "Failed to read R2004 Section Map.\n");
      return -1;
    }

  /*-------------------------------------------------------------------------
   * Section Info
   */
  section = find_section(dwg, _2004_header_data.fields.section_info_id);

  if (section != 0)
    {
      dat->byte = section->address;
      fprintf(stderr, "\n\nRaw system section bytes:\n");
      for (i = 0; i < 0x14; i++)
        {
          ss.data[i] = bit_read_RC(dat);
          fprintf(stderr, "%x ", ss.data[i]);
        }

      if (loglevel)
        {
          fprintf(stderr, "\n\n=== System Section (Section Info) ===\n");
          fprintf(stderr, "Section Type (should be 0x4163043b): %x\n",
              (unsigned int) ss.fields.section_type);
          fprintf(stderr, "DecompDataSize: %x\n",
              (unsigned int) ss.fields.decomp_data_size);
          fprintf(stderr, "CompDataSize: %x\n",
              (unsigned int) ss.fields.comp_data_size);
          fprintf(stderr, "Compression Type: %x\n",
              (unsigned int) ss.fields.compression_type);
          fprintf(stderr, "Checksum: %x\n\n", (unsigned int) ss.fields.checksum);
        }

       read_R2004_section_info(dat, dwg, 
         ss.fields.comp_data_size, ss.fields.decomp_data_size);
    }

  /*-------------------------------------------------------------------------
   * Section: Class
   */
  read_2004_class_section(dat, dwg);

  if (dwg->header.section_info != 0)
    {
      for (i = 0; i < dwg->header.num_descriptions; ++i)
        if (dwg->header.section_info[i].sections != 0)
          free(dwg->header.section_info[i].sections);

      free(dwg->header.section_info);
      dwg->header.num_descriptions = 0;
    }



  fprintf(stderr,
	  "Decoding of DWG version R2004 header is not fully implemented yet. We are going to try\n");
  return 0;
}

int
decode_R2007(Bit_Chain* dat, Dwg_Data * dwg)
{
  int i;
  unsigned long int preview_address, security_type, unknown_long,
      dwg_property_address, vba_proj_address, app_info_address;
  unsigned char sig, DwgVer, MaintReleaseVer;
  unsigned char solomon[0x3d8];

  /* 5 bytes of 0x00 */
  dat->byte = 0x06;
  if (loglevel)
    fprintf(stderr, "5 bytes of 0x00: ");
  for (i = 0; i < 5; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "0x%02X ", sig);
    }
  if (loglevel)
    fprintf(stderr, "\n");

  /* Unknown */
  dat->byte = 0x0B;
  sig = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "Unknown: 0x%02X\n", sig);

  /* Byte 0x00, 0x01, or 0x03 */
  dat->byte = 0x0C;
  sig = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "Byte 0x00, 0x01, or 0x03: 0x%02X\n", sig);

  /* Preview Address */
  dat->byte = 0x0D;
  preview_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Preview Address: 0x%08X\n", (unsigned int) preview_address);

  /* DwgVer */
  dat->byte = 0x11;
  DwgVer = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "DwgVer: %u\n", DwgVer);

  /* MaintReleaseVer */
  dat->byte = 0x12;
  MaintReleaseVer = bit_read_RC(dat);
  if (loglevel)
    fprintf(stderr, "MaintRelease: %u\n", MaintReleaseVer);

  /* Codepage */
  dat->byte = 0x13;
  dwg->header.codepage = bit_read_RS(dat);
  if (loglevel)
    fprintf(stderr, "Codepage: %u\n", dwg->header.codepage);

  /* Unknown */
  dat->byte = 0x15;
  if (loglevel)
    fprintf(stderr, "Unknown: ");
  for (i = 0; i < 3; i++)
    {
      sig = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "0x%02X ", sig);
    }
  if (loglevel)
    fprintf(stderr, "\n");

  /* SecurityType */
  dat->byte = 0x18;
  security_type = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "SecurityType: 0x%08X\n", (unsigned int) security_type);

  /* Unknown long */
  dat->byte = 0x1C;
  unknown_long = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Unknown long: 0x%08X\n", (unsigned int) unknown_long);

  /* DWG Property Addr */
  dat->byte = 0x20;
  dwg_property_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "DWG Property Addr: 0x%08X\n",
        (unsigned int) dwg_property_address);

  /* VBA Project Addr */
  dat->byte = 0x24;
  vba_proj_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "VBA Project Addr: 0x%08X\n",
        (unsigned int) vba_proj_address);

  /* 0x00000080 */
  dat->byte = 0x28;
  unknown_long = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "0x00000080: 0x%08X\n", (unsigned int) unknown_long);

  /* Application Info Address */
  dat->byte = 0x2C;
  app_info_address = bit_read_RL(dat);
  if (loglevel)
    fprintf(stderr, "Application Info Address: 0x%08X\n",
        (unsigned int) app_info_address);

  /* Reed-Solomon(255,239) encoded section */
  if (loglevel)
    fprintf(stderr, "Reed-Solomon(255,239) encoded section:\n\n");
  
  dat->byte = 0x80;
  for (i = 0; i < 0x3d8; i++)
    {
      solomon[i] = bit_read_RC(dat);
      if (loglevel)
        fprintf(stderr, "%2x ", solomon[i]);
    }
  if (loglevel)
    fprintf(stderr, "\n\n");

  /////////////////////////////////////////
  //	incomplete implementation!
  /////////////////////////////////////////

  fprintf(stderr,
      "Decoding of DWG version R2007 header is not fully implemented yet. we are going to try\n");
  return 0;
}

/*--------------------------------------------------------------------------------
 * Private functions
 */

static int
dwg_decode_entity(Bit_Chain * dat, Dwg_Object_Entity * ent)
{
  unsigned int i;
  unsigned int size;
  int error = 2;

  SINCE(R_2000)
    {
      ent->bitsize = bit_read_RL(dat);
    }

  error = bit_read_H(dat, &(ent->object->handle));
  if (error)
    {
      fprintf(
          stderr,
          "dwg_decode_entity:\tError in object handle! Current Bit_Chain address: 0x%0x\n",
          (unsigned int) dat->byte);
      ent->bitsize = 0;
      ent->extended_size = 0;
      ent->picture_exists = 0;
      ent->num_handles = 0;
      return 0;
    }

  ent->extended_size = 0;
  while (size = bit_read_BS(dat))
    {
      if (loglevel)
        fprintf(stderr, "EED size: %lu\n", (long unsigned int)size);
      if (size > 10210)
        {
          fprintf(
              stderr,
              "dwg_decode_entity: Absurd! Extended object data size: %lu. Object: %lu (handle).\n",
              (long unsigned int) size, ent->object->handle.value);
          ent->bitsize = 0;
          ent->extended_size = 0;
          ent->picture_exists = 0;
          ent->num_handles = 0;
          //XXX
          return -1;
          //break;
        }
      if (ent->extended_size == 0)
        {
          ent->extended = (char *)malloc(size);
          ent->extended_size = size;
        }
      else
        {
          ent->extended_size += size;
          ent->extended = (char *)realloc(ent->extended, ent->extended_size);
        }
      error = bit_read_H(dat, &ent->extended_handle);
      if (error)
        fprintf(stderr, "Ops...\n");
      for (i = ent->extended_size - size; i < ent->extended_size; i++)
        ent->extended[i] = bit_read_RC(dat);
    }
  ent->picture_exists = bit_read_B(dat);
  if (ent->picture_exists)
    {
      ent->picture_size = bit_read_RL(dat);
      if (ent->picture_size < 210210)
        {
          ent->picture = (char *)malloc(ent->picture_size);
          for (i = 0; i < ent->picture_size; i++)
            ent->picture[i] = bit_read_RC(dat);
        }
      else
        {
          fprintf(
              stderr,
              "dwg_decode_entity:  Absurd! Picture-size: %lu kB. Object: %lu (handle).\n",
              ent->picture_size / 1000, ent->object->handle.value);
          bit_advance_position(dat, -(4 * 8 + 1));
        }
    }

  VERSIONS(R_13,R_14)
    {
      ent->bitsize = bit_read_RL(dat);
    }

  ent->entity_mode = bit_read_BB(dat);
  ent->num_reactors = bit_read_BL(dat);

  SINCE(R_2004)
    {
      ent->xdict_missing_flag = bit_read_B(dat);
    }

  VERSIONS(R_13,R_14)
    {
      ent->isbylayerlt = bit_read_B(dat);
    }

  ent->nolinks = bit_read_B(dat);
  bit_read_CMC(dat, &ent->color);
  ent->linetype_scale = bit_read_BD(dat);

  SINCE(R_2000)
    {
      ent->linetype_flags = bit_read_BB(dat);
      ent->plotstyle_flags = bit_read_BB(dat);
    }

  SINCE(R_2007)
    {
      ent->material_flags = bit_read_BB(dat);
      ent->shadow_flags = bit_read_RC(dat);
    }

  ent->invisible = bit_read_BS(dat);

  SINCE(R_2000)
    {
      ent->lineweight = bit_read_RC(dat);
    }

  return 0;
}

static int
dwg_decode_object(Bit_Chain * dat, Dwg_Object_Object * ord)
{
  unsigned int i;
  unsigned int size;
  int error = 2;

  SINCE(R_2000)
    {
      ord->bitsize = bit_read_RL(dat);
    }

  error = bit_read_H(dat, &ord->object->handle);
  if (error)
    {
      fprintf(stderr,
          "\tError in object handle! Bit_Chain current address: 0x%0x\n",
          (unsigned int) dat->byte);
      ord->bitsize = 0;
      ord->extended_size = 0;
      ord->num_handles = 0;
      return -1;
    }
  ord->extended_size = 0;
  while (size = bit_read_BS(dat))
    {
      if (size > 10210)
        {
          fprintf(
              stderr,
              "dwg_decode_object: Absurd! Extended object data size: %lu. Object: %lu (handle).\n",
              (long unsigned int) size, ord->object->handle.value);
          ord->bitsize = 0;
          ord->extended_size = 0;
          ord->num_handles = 0;
          return 0;
        }
      if (ord->extended_size == 0)
        {
          ord->extended = (unsigned char *)malloc(size);
          ord->extended_size = size;
        }
      else
        {
          ord->extended_size += size;
          ord->extended = (unsigned char *)realloc(ord->extended, ord->extended_size);
        }
      error = bit_read_H(dat, &ord->extended_handle);
      if (error)
        fprintf(stderr, "Ops...\n");
      for (i = ord->extended_size - size; i < ord->extended_size; i++)
        ord->extended[i] = bit_read_RC(dat);
    }

  VERSIONS(R_13,R_14)
    {
      ord->bitsize = bit_read_RL(dat);
    }

  ord->num_reactors = bit_read_BL(dat);

  SINCE(R_2004)
    {
      ord->xdic_missing_flag = bit_read_B(dat);
    }

  return 0;
}

/**
 * Find a pointer to an object given it's id (handle)
 */
static Dwg_Object *
dwg_resolve_handle(Dwg_Data* dwg, unsigned long int absref)
{
  //FIXME find a faster algorithm
  int i;
  for (i = 0; i < dwg->num_objects; i++)
    if (dwg->object[i].handle.value == absref)
      return &dwg->object[i];
  return 0;
}

#define REFS_PER_REALLOC 100

static Dwg_Object_Ref *
dwg_decode_handleref(Bit_Chain * dat, Dwg_Object * obj, Dwg_Data* dwg)
{
  // Welcome to the house of evil code!
  Dwg_Object_Ref* ref = (Dwg_Object_Ref *) malloc(sizeof(Dwg_Object_Ref));

  if (bit_read_H(dat, &ref->handleref))
    {
      if (obj)
        {
          fprintf(stderr,
            "Error reading handle in object whose handle is: %d.%d.%lu\n",
            obj->handle.code, obj->handle.size, obj->handle.value);
        }
      else
        {
          fprintf(stderr, "Error reading handle in the header variables section\n");
        }
      free(ref);
      return 0;
    }

  //Reserve memory space for object references
  if (dwg->num_object_refs == 0)
    dwg->object_ref = (Dwg_Object_Ref **) malloc(REFS_PER_REALLOC * sizeof(Dwg_Object_Ref*));
  else
    if (dwg->num_object_refs % REFS_PER_REALLOC == 0)
      dwg->object_ref = (Dwg_Object_Ref **) realloc(dwg->object_ref,
          (dwg->num_object_refs + REFS_PER_REALLOC) * sizeof(Dwg_Object_Ref*));

  dwg->object_ref[dwg->num_object_refs++] = ref;

  ref->absolute_ref = ref->handleref.value;
  ref->obj = 0;

  //we receive a null obj when we are reading
  // handles in the header variables section
  if (!obj)
    return ref;

  /*
   * sometimes the code indicates the type of ownership
   * in other cases the handle is stored as an offset from some other handle
   * how is it determined?
   */
  ref->absolute_ref = 0;
  switch(ref->handleref.code) //that's right: don't bother the code on the spec.
    {
    case 0x06: //what if 6 means HARD_OWNER?
      ref->absolute_ref = (obj->handle.value + 1);
      break;
    case 0x08:
      ref->absolute_ref = (obj->handle.value - 1);
      break;
    case 0x0A:
      ref->absolute_ref = (obj->handle.value + ref->handleref.value);
      break;
    case 0x0C:
      ref->absolute_ref = (obj->handle.value - ref->handleref.value);
      break;
    default: //0x02, 0x03, 0x04, 0x05 or none
      ref->absolute_ref = ref->handleref.value;
      break;
    }
  return ref;
}

static Dwg_Object_Ref *
dwg_decode_handleref_with_code(Bit_Chain * dat, Dwg_Object * obj, Dwg_Data* dwg, unsigned int code)
{
  Dwg_Object_Ref * ref;
  ref = dwg_decode_handleref(dat, obj, dwg);
  if (ref->absolute_ref == 0 && ref->handleref.code != code)
    {
      if (loglevel) fprintf(stderr, "ERROR: expected a CODE %d handle\nERROR: ", code);
      //TODO: At the moment we are tolerating wrong codes in handles.
      // in the future we might want to get strict and return 0 here so that code will crash
      // whenever it reaches the first handle parsing error. This might make debugging easier.
      //return 0;
    }
  return ref;
}

static void
dwg_decode_header_variables(Bit_Chain* dat, Dwg_Data * dwg)
{
  Dwg_Header_Variables* _obj = &dwg->header_vars;
  Dwg_Object* obj=0;

/*
  VERSION(R_2007)
    {
      FIELD_RL(bitsize);
    }

  FIELD_RL (bitsize);
*/

  FIELD_BD (unknown_0);
  FIELD_BD (unknown_1);
  FIELD_BD (unknown_2);
  FIELD_BD (unknown_3);
  FIELD_TV (unknown_4);
  FIELD_TV (unknown_5);
  FIELD_TV (unknown_6);
  FIELD_TV (unknown_7);
  FIELD_BL (unknown_8);
  FIELD_BL (unknown_9);

  VERSIONS(R_13, R_14)
    {
      FIELD_BS(unknown_10);
    }

  PRE(R_2004)
    {
      FIELD_HANDLE (current_viewport_entity_header, ANYCODE);
    }

  FIELD_B (DIMASO);
  FIELD_B (DIMSHO);

  VERSIONS(R_13, R_14)
    {
      FIELD_B (DIMSAV); //undocumented
    }

  FIELD_B (PLINEGEN);
  FIELD_B (ORTHOMODE);
  FIELD_B (REGENMODE);
  FIELD_B (FILLMODE);
  FIELD_B (QTEXTMODE);
  FIELD_B (PSLTSCALE);
  FIELD_B (LIMCHECK);

  VERSIONS(R_13, R_14)
    {
      FIELD_B (BLIPMODE);
    }

  SINCE(R_2004)
    {
      FIELD_B (unknown_11); //undocumented
    }

  FIELD_B (user_timer_onoff);
  FIELD_B (SKPOLY);
  FIELD_B (ANGDIR);
  FIELD_B (SPLFRAME);

  VERSIONS(R_13, R_14)
    {
      FIELD_B (ATTREQ);
      FIELD_B (ATTDIA);
    }

  FIELD_B (MIRRTEXT);
  FIELD_B (WORLDVIEW);

  VERSIONS(R_13, R_14)
    {
      FIELD_B (WIREFRAME); //Undocumented
    }

  FIELD_B (TILEMODE);
  FIELD_B (PLIMCHECK);
  FIELD_B (VISRETAIN);

  VERSIONS(R_13, R_14)
    {
      FIELD_B (DELOBJ);
    }

  FIELD_B (DISPSILH);
  FIELD_B (PELLIPSE);

  VERSION(R_13)
    {
      FIELD_BS (SAVEIMAGES);
    }

  VERSIONS(R_14, R_2000)
    {
      FIELD_BS (PROXYGRAPHICS);
    }

  VERSIONS(R_13, R_14)
    {
      FIELD_BS (DRAGMODE);
    }

  FIELD_BS (TREEDEPTH);
  FIELD_BS (LUNITS);
  FIELD_BS (LUPREC);
  FIELD_BS (AUNITS);
  FIELD_BS (AUPREC);

  VERSIONS(R_13, R_14)
    {
      FIELD_BS (OSMODE);
    }

  FIELD_BS (ATTMODE);

  VERSIONS(R_13, R_14)
    {
      FIELD_BS (COORDS);
    }

  FIELD_BS (PDMODE);

  VERSIONS(R_13, R_14)
    {
      FIELD_BS (PICKSTYLE);
    }

  SINCE(R_2004)
    {
      FIELD_BL (unknown_12);
      FIELD_BL (unknown_13);
      FIELD_BL (unknown_14);
    }

  FIELD_BS (USERI1);
  FIELD_BS (USERI2);
  FIELD_BS (USERI3);
  FIELD_BS (USERI4);
  FIELD_BS (USERI5);
  FIELD_BS (SPLINESEGS);
  FIELD_BS (SURFU);
  FIELD_BS (SURFV);
  FIELD_BS (SURFTYPE);
  FIELD_BS (SURFTAB1);
  FIELD_BS (SURFTAB2);
  FIELD_BS (SPLINETYPE);
  FIELD_BS (SHADEDGE);
  FIELD_BS (SHADEDIF);
  FIELD_BS (UNITMODE);
  FIELD_BS (MAXACTVP);
  FIELD_BS (ISOLINES);
  FIELD_BS (CMLJUST);
  FIELD_BS (TEXTQLTY);
  FIELD_BD (LTSCALE);
  FIELD_BD (VTEXTSIZE);
  FIELD_BD (TRACEWID);
  FIELD_BD (SKETCHINC);
  FIELD_BD (FILLETRAD);
  FIELD_BD (THICKNESS);
  FIELD_BD (ANGBASE);
  FIELD_BD (PDSIZE);
  FIELD_BD (PLINEWID);
  FIELD_BD (USERR1);
  FIELD_BD (USERR2);
  FIELD_BD (USERR3);
  FIELD_BD (USERR4);
  FIELD_BD (USERR5);
  FIELD_BD (CHAMFERA);
  FIELD_BD (CHAMFERB);
  FIELD_BD (CHAMFERC);
  FIELD_BD (CHAMFERD);
  FIELD_BD (FACETRES);
  FIELD_BD (CMLSCALE);
  FIELD_BD (CELTSCALE);
  FIELD_TV (MENUNAME);
  FIELD_BL (TDCREATE_JULIAN_DAY);
  FIELD_BL (TDCREATE_MILLISECONDS);
  FIELD_BL (TDUPDATE_JULIAN_DAY);
  FIELD_BL (TDUPDATE_MILLISECONDS);

  SINCE(R_2004)
    {
      FIELD_BL (unknown_15);
      FIELD_BL (unknown_16);
      FIELD_BL (unknown_17);
    }

  FIELD_BL (TDINDWG_DAYS);
  FIELD_BL (TDINDWG_MILLISECONDS);
  FIELD_BL (TDUSRTIMER_DAYS);
  FIELD_BL (TDUSRTIMER_MILLISECONDS);
  FIELD_CMC (CECOLOR);
  FIELD_HANDLE (HANDSEED, ANYCODE);
  FIELD_HANDLE (CLAYER, ANYCODE);
  FIELD_HANDLE (TEXTSTYLE, ANYCODE);
  FIELD_HANDLE (CELTYPE, ANYCODE);

  SINCE(R_2007)
    {
      FIELD_HANDLE (CMATERIAL, ANYCODE);
    }

  FIELD_HANDLE (DIMSTYLE, ANYCODE);
  FIELD_HANDLE (CMLSTYLE, ANYCODE);

  SINCE(R_2000)
    {
      FIELD_BD (PSVPSCALE);
    }

  FIELD_3BD (INSBASE_PSPACE);
  FIELD_3BD (EXTMIN_PSPACE);
  FIELD_3BD (EXTMAX_PSPACE);
  FIELD_2RD (LIMMIN_PSPACE);
  FIELD_2RD (LIMMAX_PSPACE);
  FIELD_BD (ELEVATION_PSPACE);
  FIELD_3BD (UCSORG_PSPACE);
  FIELD_3BD (UCSXDIR_PSPACE);
  FIELD_3BD (UCSYDIR_PSPACE);
  FIELD_HANDLE (UCSNAME_PSPACE, ANYCODE);

  SINCE(R_2000)
    {
      FIELD_HANDLE (PUCSBASE, ANYCODE);
      FIELD_BS (PUCSORTHOVIEW);
      FIELD_HANDLE (PUCSORTHOREF, ANYCODE);
      FIELD_3BD (PUCSORGTOP);
      FIELD_3BD (PUCSORGBOTTOM);
      FIELD_3BD (PUCSORGLEFT);
      FIELD_3BD (PUCSORGRIGHT);
      FIELD_3BD (PUCSORGFRONT);
      FIELD_3BD (PUCSORGBACK);
    }

  FIELD_3BD (INSBASE_MSPACE);
  FIELD_3BD (EXTMIN_MSPACE);
  FIELD_3BD (EXTMAX_MSPACE);
  FIELD_2RD (LIMMIN_MSPACE);
  FIELD_2RD (LIMMAX_MSPACE);
  FIELD_BD (ELEVATION_MSPACE);
  FIELD_3BD (UCSORG_MSPACE);
  FIELD_3BD (UCSXDIR_MSPACE);
  FIELD_3BD (UCSYDIR_MSPACE);
  FIELD_HANDLE (UCSNAME_MSPACE, ANYCODE);

  SINCE(R_2000)
    {
      FIELD_HANDLE (UCSBASE, ANYCODE);
      FIELD_BS (UCSORTHOVIEW);
      FIELD_HANDLE (UCSORTHOREF, ANYCODE);
      FIELD_3BD (UCSORGTOP);
      FIELD_3BD (UCSORGBOTTOM);
      FIELD_3BD (UCSORGLEFT);
      FIELD_3BD (UCSORGRIGHT);
      FIELD_3BD (UCSORGFRONT);
      FIELD_3BD (UCSORGBACK);
      FIELD_TV (DIMPOST);
      FIELD_TV (DIMAPOST);
    }

  VERSIONS(R_13, R_14)
    {
      FIELD_B (DIMTOL);
      FIELD_B (DIMLIM);
      FIELD_B (DIMTIH);
      FIELD_B (DIMTOH);
      FIELD_B (DIMSE1);
      FIELD_B (DIMSE2);
      FIELD_B (DIMALT);
      FIELD_B (DIMTOFL);
      FIELD_B (DIMSAH);
      FIELD_B (DIMTIX);
      FIELD_B (DIMSOXD);
      FIELD_RC (DIMALTD);
      FIELD_RC (DIMZIN);
      FIELD_B (DIMSD1);
      FIELD_B (DIMSD2);
      FIELD_RC (DIMTOLJ);
      FIELD_RC (DIMJUST);
      FIELD_RC (DIMFIT);
      FIELD_B (DIMUPT);
      FIELD_RC (DIMTZIN);
      FIELD_RC (DIMMALTZ);
      FIELD_RC (DIMMALTTZ);
      FIELD_RC (DIMTAD);
      FIELD_BS (DIMUNIT);
      FIELD_BS (DIMAUNIT);
      FIELD_BS (DIMDEC);
      FIELD_BS (DIMTDEC);
      FIELD_BS (DIMALTU);
      FIELD_BS (DIMALTTD);
      FIELD_HANDLE (DIMTXSTY, ANYCODE);
    }

  FIELD_BD (DIMSCALE);
  FIELD_BD (DIMASZ);
  FIELD_BD (DIMEXO);
  FIELD_BD (DIMDLI);
  FIELD_BD (DIMEXE);
  FIELD_BD (DIMRND);
  FIELD_BD (DIMDLE);
  FIELD_BD (DIMMTP);
  FIELD_BD (DIMMTM);

  SINCE(R_2007)
    {
      FIELD_BD (DIMFXL);
      FIELD_BD (DIMJOGANG);
      FIELD_BS (DIMTFILL);
      FIELD_CMC (DIMTFILLCLR);
    }

  SINCE(R_2000)
    {
      FIELD_B (DIMTOL);
      FIELD_B (DIMLIM);
      FIELD_B (DIMTIH);
      FIELD_B (DIMTOH);
      FIELD_B (DIMSE1);
      FIELD_B (DIMSE2);
      FIELD_BS (DIMTAD);
      FIELD_BS (DIMZIN);
      FIELD_BS (DIMAZIN);
    }

  SINCE(R_2007)
    {
      FIELD_BS (DIMARCSYM);
    }

  FIELD_BD (DIMTXT);
  FIELD_BD (DIMCEN);
  FIELD_BD (DIMTSZ);
  FIELD_BD (DIMALTF);
  FIELD_BD (DIMLFAC);
  FIELD_BD (DIMTVP);
  FIELD_BD (DIMTFAC);
  FIELD_BD (DIMGAP);

  VERSIONS(R_13, R_14)
    {
      FIELD_T (DIMPOST_T);
      FIELD_T (DIMAPOST_T);
      FIELD_T (DIMBLK_T);
      FIELD_T (DIMBLK1_T);
      FIELD_T (DIMBLK2_T);
    }

  SINCE(R_2000)
    {
      FIELD_BD (DIMALTRND);
      FIELD_B (DIMALT);
      FIELD_BS (DIMALTD);
      FIELD_B (DIMTOFL);
      FIELD_B (DIMSAH);
      FIELD_B (DIMTIX);
      FIELD_B (DIMSOXD);
    }

  FIELD_CMC (DIMCLRD);
  FIELD_CMC (DIMCLRE);
  FIELD_CMC (DIMCLRT);

  SINCE(R_2000)
    {
      FIELD_BS (DIMADEC);
      FIELD_BS (DIMDEC);
      FIELD_BS (DIMALTU);
      FIELD_BS (DIMALTTD);
      FIELD_BS (DIMAUNIT);
      FIELD_BS (DIMFRAC);
      FIELD_BS (DIMLUNIT);
      FIELD_BS (DIMDSEP);
      FIELD_BS (DIMTMOVE);
      FIELD_BS (DIMJUST);
      FIELD_BS (DIMJUST);//TESTE
      FIELD_B (DIMSD1);
      FIELD_B (DIMSD2);
      FIELD_BS (DIMTOLJ);
      FIELD_BS (DIMTZIN);
      FIELD_BS (DIMALTZ);
      FIELD_BS (DIMALTTZ);
      FIELD_B (DIMUPT);
      FIELD_BS (DIMATFIT);
    }

  SINCE(R_2007)
    {
      FIELD_B (DIMFXLON);
    }

  SINCE(R_2000)
    {
      FIELD_HANDLE (DIMTXTSTY, ANYCODE);
      FIELD_HANDLE (DIMLDRBLK, ANYCODE);
      FIELD_HANDLE (DIMBLK, ANYCODE);
      FIELD_HANDLE (DIMBLK1, ANYCODE);
      FIELD_HANDLE (DIMBLK2, ANYCODE);
    }

  SINCE(R_2007)
    {
      FIELD_HANDLE (DIMLTYPE, ANYCODE);
      FIELD_HANDLE (DIMLTEX1, ANYCODE);
      FIELD_HANDLE (DIMLTEX2, ANYCODE);
    }

  SINCE(R_2000)
    {
      FIELD_BS (DIMLWD);
      FIELD_BS (DIMLWE);
    }

  FIELD_HANDLE (BLOCK_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (LAYER_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (STYLE_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (LINETYPE_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (VIEW_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (UCS_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (VPORT_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (APPID_CONTROL_OBJECT, ANYCODE);
  FIELD_HANDLE (DIMSTYLE_CONTROL_OBJECT, ANYCODE);

  VERSIONS(R_13, R_2000)
    {
      FIELD_HANDLE (VIEWPORT_ENTITY_HEADER_CONTROL_OBJECT, ANYCODE);
    }

  FIELD_HANDLE (DICTIONARY_ACAD_GROUP, ANYCODE);
  FIELD_HANDLE (DICTIONARY_ACAD_MLINESTYLE, ANYCODE);
  FIELD_HANDLE (DICTIONARY_NAMED_OBJECTS, ANYCODE);

  SINCE(R_2000)
    {
      FIELD_BS (unknown_18);
      FIELD_BS (unknown_19);
      FIELD_TV (HYPERLINKBASE);
      FIELD_TV (STYLESHEET);
      FIELD_HANDLE (DICTIONARY_LAYOUTS, ANYCODE);
      FIELD_HANDLE (DICTIONARY_PLOTSETTINGS, ANYCODE);
      FIELD_HANDLE (DICTIONARY_PLOTSTYLES, ANYCODE);
    }

  SINCE(R_2004)
    {
      FIELD_HANDLE (DICTIONARY_MATERIALS, ANYCODE);
      FIELD_HANDLE (DICTIONARY_COLORS, ANYCODE);
    }

  SINCE(R_2007)
    {
      FIELD_HANDLE (DICTIONARY_VISUALSTYLE, ANYCODE);
    }

  SINCE(R_2000)
    {
      FIELD_BL (FLAGS);
      FIELD_BS (INSUNITS);
      FIELD_BS (CEPSNTYPE);
      if (FIELD_VALUE(CEPSNTYPE) == 3)
        {
          FIELD_HANDLE (CPSNID, ANYCODE);
        }
      FIELD_TV (FINGERPRINTGUID);
      FIELD_TV (VERSIONGUID);
    }

  SINCE(R_2004)
    {
      FIELD_RC (SORTENTS);
      FIELD_RC (IDEXCTL);
      FIELD_RC (HIDETEXT);
      FIELD_RC (XCLIPFRAME);
      FIELD_RC (DIMASSOC);
      FIELD_RC (HALOGAP);
      FIELD_BS (OBSCUREDCOLOR);
      FIELD_BS (INTERSECTIONCOLOR);
      FIELD_RC (OBSCUREDLTYPE);
      FIELD_RC (INTERSECTIONDISPLAY);
      FIELD_TV (PROJECTNAME);
    }

  FIELD_HANDLE (BLOCK_RECORD_PAPER_SPACE, ANYCODE);
  FIELD_HANDLE (BLOCK_RECORD_MODEL_SPACE, ANYCODE);
  FIELD_HANDLE (LTYPE_BYLAYER, ANYCODE);
  FIELD_HANDLE (LTYPE_BYBLOCK, ANYCODE);
  FIELD_HANDLE (LTYPE_CONTINUOUS, ANYCODE);

  SINCE(R_2007)
    {
      FIELD_B (unknown_20);
      FIELD_BL (unknown_21);
      FIELD_BL (unknown_22);
      FIELD_BD (unknown_23);
      FIELD_BD (unknown_24);
      FIELD_BD (unknown_25);
      FIELD_BD (unknown_26);
      FIELD_BD (unknown_27);
      FIELD_BD (unknown_28);
      FIELD_RC (unknown_29);
      FIELD_RC (unknown_30);
      FIELD_BD (unknown_31);
      FIELD_BD (unknown_32);
      FIELD_BD (unknown_33);
      FIELD_BD (unknown_34);
      FIELD_BD (unknown_35);
      FIELD_BD (unknown_36);
      FIELD_BS (unknown_37);
      FIELD_RC (unknown_38);
      FIELD_BD (unknown_39);
      FIELD_BD (unknown_40);
      FIELD_BD (unknown_41);
      FIELD_BL (unknown_42);
      FIELD_RC (unknown_43);
      FIELD_RC (unknown_44);
      FIELD_RC (unknown_45);
      FIELD_RC (unknown_46);
      FIELD_B (unknown_47);
      FIELD_CMC (unknown_48);
      FIELD_HANDLE (unknown_49, ANYCODE);
      FIELD_HANDLE (unknown_50, ANYCODE);
      FIELD_HANDLE (unknown_51, ANYCODE);
      FIELD_RC (unknown_52);
      FIELD_BD (unknown_53);
    }

  SINCE(R_14)
    {
      FIELD_BS (unknown_54);
      FIELD_BS (unknown_55);
      FIELD_BS (unknown_56);
      FIELD_BS (unknown_57);
    }

  FIELD_RS (CRC);
}

static void
dwg_decode_common_entity_handle_data(Bit_Chain * dat, Dwg_Object * obj)
{

  Dwg_Object_Entity *ent;
  int i;
  ent = obj->tio.entity;

  //TODO: check what is the condition for the presence of this handle:
  //	ent->subentity_ref_handle = dwg_decode_handleref_with_code (dat, obj, obj->parent, 3);

  if (ent->num_reactors)
    ent->reactors = (Dwg_Object_Ref**) malloc(ent->num_reactors * sizeof(Dwg_Object_Ref*));
  for (i = 0; i < ent->num_reactors; i++)
    {
      ent->reactors[i] = dwg_decode_handleref_with_code(dat, obj, obj->parent, 4);
    }

  ent->xdicobjhandle = dwg_decode_handleref_with_code(dat, obj, obj->parent, 3);

  VERSIONS(R_13,R_14)
    {
      ent->layer = dwg_decode_handleref_with_code(dat, obj, obj->parent, 5);
      if (!ent->isbylayerlt)
        {
          ent->ltype = dwg_decode_handleref_with_code(dat, obj, obj->parent, 5);
        }
    }

  if (0)
    { //TODO: these are optional. Figure out what is the condition.
      // These seem to depend on FEDCBA flags: look at page 53 in the spec.
      ent->prev_entity = dwg_decode_handleref_with_code(dat, obj, obj->parent, 4);
      ent->next_entity = dwg_decode_handleref_with_code(dat, obj, obj->parent, 4);
    }

  SINCE(R_2000)
    {
      ent->layer = dwg_decode_handleref_with_code(dat, obj, obj->parent, 5);
      if (ent->linetype_flags == 3)
        {
          ent->ltype = dwg_decode_handleref_with_code(dat, obj, obj->parent, 5);
        }
      if (ent->plotstyle_flags == 3)
        {
          ent->plotstyle = dwg_decode_handleref_with_code(dat, obj, obj->parent, 5);
        }
    }

  if (dat->version >= R_2007 && ent->material_flags == 3)
    {
      ent->material = dwg_decode_handleref(dat, obj, obj->parent);
    }

}

/* OBJECTS *******************************************************************/

#include<dwg.spec>

static int
dwg_decode_variable_type(Dwg_Data * dwg, Bit_Chain * dat, Dwg_Object* obj)
{
  if ((obj->type - 500) > dwg->num_classes)
    return 1;
  int i = obj->type - 500;

  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "DICTIONARYVAR"))
    {
      dwg_decode_DICTIONARYVAR(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "DICTIONARYWDFLT"))
    {
      //dwg_decode_DICTIONARYWDLFT(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "HATCH"))
    {
      dwg_decode_HATCH(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "IDBUFFER"))
    {
      dwg_decode_IDBUFFER(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "IMAGE"))
    {
      dwg_decode_IMAGE(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "IMAGEDEF"))
    {
      dwg_decode_IMAGEDEF(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "IMAGEDEFREACTOR"))
    {
      dwg_decode_IMAGEDEFREACTOR(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "LAYER_INDEX"))
    {
      dwg_decode_LAYER_INDEX(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "LAYOUT"))
    {
      dwg_decode_LAYOUT(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "LWPLINE"))
    {
      dwg_decode_LWPLINE(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "OLE2FRAME"))
    {
      dwg_decode_OLE2FRAME(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "PLACEHOLDER"))
    {
//TODO:      dwg_decode_PLACEHOLDER(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "RASTERVARIABLES"))
    {
      dwg_decode_RASTERVARIABLES(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "SORTENTSTABLE"))
    {
      dwg_decode_SORTENTSTABLE(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "SPATIAL_FILTER"))
    {
      dwg_decode_SPATIAL_FILTER(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "SPATIAL_INDEX"))
    {
      dwg_decode_SPATIAL_INDEX(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "VBA_PROJECT"))
    {
//TODO:      dwg_decode_VBA_PROJECT(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "WIPEOUTVARIABLE"))
    {
//TODO:      dwg_decode_WIPEOUTVARIABLE(dat, obj);
      return 0;
    }
  if (!strcmp((const char *)dwg->dwg_class[i].dxfname, "XRECORD"))
    {
      dwg_decode_XRECORD(dat, obj);
      return 0;
    }

  return 1;
}

/*--------------------------------------------------------------------------------
 * Private functions, which depend from the preceding
 */
static void
dwg_decode_add_object(Dwg_Data * dwg, Bit_Chain * dat,
    long unsigned int address)
{
  long unsigned int previous_address;
  long unsigned int object_address;
  unsigned char previous_bit;
  Dwg_Object *obj;

  /* Keep the previous address
   */
  previous_address = dat->byte;
  previous_bit = dat->bit;

  /* Use the indicated address for the object
   */
  dat->byte = address;
  dat->bit = 0;

  /*
   * Reserve memory space for objects
   */
  if (dwg->num_objects == 0)
    dwg->object = (Dwg_Object *) malloc(sizeof(Dwg_Object));
  else
    dwg->object = (Dwg_Object *) realloc(dwg->object, (dwg->num_objects + 1)
        * sizeof(Dwg_Object));

  if (loglevel)
      fprintf(stderr, "\n\n======================\nObject number: %lu",
          dwg->num_objects);

  obj = &dwg->object[dwg->num_objects];
  obj->index = dwg->num_objects;
  dwg->num_objects++;

  obj->handle.code = 0;
  obj->handle.size = 0;
  obj->handle.value = 0;

  obj->parent = dwg;
  obj->size = bit_read_MS(dat);
  object_address = dat->byte;
  ktl_lastaddress = dat->byte + obj->size; /* (calculate the bitsize) */
  obj->type = bit_read_BS(dat);

  if (loglevel)
    fprintf(stderr, " Type: %d\n", obj->type);

  /* Check the type of the object
   */
  switch (obj->type)
    {
  case DWG_TYPE_TEXT:
    dwg_decode_TEXT(dat, obj);
    break;
  case DWG_TYPE_ATTRIB:
    dwg_decode_ATTRIB(dat, obj);
    break;
  case DWG_TYPE_ATTDEF:
    dwg_decode_ATTDEF(dat, obj);
    break;
  case DWG_TYPE_BLOCK:
    dwg_decode_BLOCK(dat, obj);
    break;
  case DWG_TYPE_ENDBLK:
    dwg_decode_ENDBLK(dat, obj);
    break;
  case DWG_TYPE_SEQEND:
    dwg_decode_SEQEND(dat, obj);
    break;
  case DWG_TYPE_INSERT:
    dwg_decode_INSERT(dat, obj);
    break;
  case DWG_TYPE_MINSERT:
    dwg_decode_MINSERT(dat, obj);
    break;
  case DWG_TYPE_VERTEX_2D:
    dwg_decode_VERTEX_2D(dat, obj);
    break;
  case DWG_TYPE_VERTEX_3D:
    dwg_decode_VERTEX_3D(dat, obj);
    break;
  case DWG_TYPE_VERTEX_MESH:
    dwg_decode_VERTEX_MESH(dat, obj);
    break;
  case DWG_TYPE_VERTEX_PFACE:
    dwg_decode_VERTEX_PFACE(dat, obj);
    break;
  case DWG_TYPE_VERTEX_PFACE_FACE:
    dwg_decode_VERTEX_PFACE_FACE(dat, obj);
    break;
  case DWG_TYPE_POLYLINE_2D:
    dwg_decode_POLYLINE_2D(dat, obj);
    break;
  case DWG_TYPE_POLYLINE_3D:
    dwg_decode_POLYLINE_3D(dat, obj);
    break;
  case DWG_TYPE_ARC:
    dwg_decode_ARC(dat, obj);
    break;
  case DWG_TYPE_CIRCLE:
    dwg_decode_CIRCLE(dat, obj);
    break;
  case DWG_TYPE_LINE:
    dwg_decode_LINE(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_ORDINATE:
    dwg_decode_DIMENSION_ORDINATE(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_LINEAR:
    dwg_decode_DIMENSION_LINEAR(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_ALIGNED:
    dwg_decode_DIMENSION_ALIGNED(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_ANG3PT:
    dwg_decode_DIMENSION_ANG3PT(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_ANG2LN:
    dwg_decode_DIMENSION_ANG2LN(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_RADIUS:
    dwg_decode_DIMENSION_RADIUS(dat, obj);
    break;
  case DWG_TYPE_DIMENSION_DIAMETER:
    dwg_decode_DIMENSION_DIAMETER(dat, obj);
    break;
  case DWG_TYPE_POINT:
    dwg_decode_POINT(dat, obj);
    break;
  case DWG_TYPE__3DFACE:
    dwg_decode__3DFACE(dat, obj);
    break;
  case DWG_TYPE_POLYLINE_PFACE:
    dwg_decode_POLYLINE_PFACE(dat, obj);
    break;
  case DWG_TYPE_POLYLINE_MESH:
    dwg_decode_POLYLINE_MESH(dat, obj);
    break;
  case DWG_TYPE_SOLID:
    dwg_decode_SOLID(dat, obj);
    break;
  case DWG_TYPE_TRACE:
    dwg_decode_TRACE(dat, obj);
    break;
  case DWG_TYPE_SHAPE:
    dwg_decode_SHAPE(dat, obj);
    break;
  case DWG_TYPE_VIEWPORT:
    dwg_decode_VIEWPORT(dat, obj);
    break;
  case DWG_TYPE_ELLIPSE:
    dwg_decode_ELLIPSE(dat, obj);
    break;
  case DWG_TYPE_SPLINE:
    dwg_decode_SPLINE(dat, obj);
    break;
  case DWG_TYPE_REGION:
    dwg_decode_REGION(dat, obj);
    break;
  case DWG_TYPE_3DSOLID:
    dwg_decode__3DSOLID(dat, obj);
    break;
  case DWG_TYPE_BODY:
    dwg_decode_BODY(dat, obj);
    break;
  case DWG_TYPE_RAY:
    dwg_decode_RAY(dat, obj);
    break;
  case DWG_TYPE_XLINE:
    dwg_decode_XLINE(dat, obj);
    break;
  case DWG_TYPE_DICTIONARY:
    dwg_decode_DICTIONARY(dat, obj);
    break;
  case DWG_TYPE_MTEXT:
    dwg_decode_MTEXT(dat, obj);
    break;
  case DWG_TYPE_LEADER:
    dwg_decode_LEADER(dat, obj);
    break;
  case DWG_TYPE_TOLERANCE:
    dwg_decode_TOLERANCE(dat, obj);
    break;
  case DWG_TYPE_MLINE:
    dwg_decode_MLINE(dat, obj);
    break;
  case DWG_TYPE_BLOCK_CONTROL:
    dwg_decode_BLOCK_CONTROL(dat, obj);
    break;
  case DWG_TYPE_BLOCK_HEADER:
    dwg_decode_BLOCK_HEADER(dat, obj);
    break;
  case DWG_TYPE_LAYER_CONTROL:
    //set LAYER_CONTROL object - helps keep track of layers
    obj->parent->layer_control = obj;
    dwg_decode_LAYER_CONTROL(dat, obj);
    break;
  case DWG_TYPE_LAYER:
    dwg_decode_LAYER(dat, obj);
    break;
  case DWG_TYPE_SHAPEFILE_CONTROL:
    dwg_decode_SHAPEFILE_CONTROL(dat, obj);
    break;
  case DWG_TYPE_SHAPEFILE:
    dwg_decode_SHAPEFILE(dat, obj);
    break;
  case DWG_TYPE_LTYPE_CONTROL:
    dwg_decode_LTYPE_CONTROL(dat, obj);
    break;
  case DWG_TYPE_LTYPE:
    dwg_decode_LTYPE(dat, obj);
    break;
  case DWG_TYPE_VIEW_CONTROL:
    dwg_decode_VIEW_CONTROL(dat, obj);
    break;
  case DWG_TYPE_VIEW:
    dwg_decode_VIEW(dat, obj);
    break;
  case DWG_TYPE_UCS_CONTROL:
    dwg_decode_UCS_CONTROL(dat, obj);
    break;
  case DWG_TYPE_UCS:
    dwg_decode_UCS(dat, obj);
    break;
  case DWG_TYPE_VPORT_CONTROL:
    dwg_decode_VPORT_CONTROL(dat, obj);
    break;
  case DWG_TYPE_VPORT:
    dwg_decode_VPORT(dat, obj);
    break;
  case DWG_TYPE_APPID_CONTROL:
    dwg_decode_APPID_CONTROL(dat, obj);
    break;
  case DWG_TYPE_APPID:
    dwg_decode_APPID(dat, obj);
    break;
  case DWG_TYPE_DIMSTYLE_CONTROL:
    dwg_decode_DIMSTYLE_CONTROL(dat, obj);
    break;
  case DWG_TYPE_DIMSTYLE:
    dwg_decode_DIMSTYLE(dat, obj);
    break;
  case DWG_TYPE_VP_ENT_HDR_CONTROL:
    dwg_decode_VP_ENT_HDR_CONTROL(dat, obj);
    break;
  case DWG_TYPE_VP_ENT_HDR:
    dwg_decode_VP_ENT_HDR(dat, obj);
    break;
  case DWG_TYPE_GROUP:
    dwg_decode_GROUP(dat, obj);
    break;
  case DWG_TYPE_MLINESTYLE:
    dwg_decode_MLINESTYLE(dat, obj);
    break;
  default:
    if (obj->type == dwg->dwg_ot_layout)
      dwg_decode_LAYOUT(dat, obj);
    else
      {
        if (!dwg_decode_variable_type(dwg, dat, obj))
          {
            obj->supertype = DWG_SUPERTYPE_UNKNOWN;
            obj->tio.unknown = (unsigned char*)malloc(obj->size);
            memcpy(obj->tio.unknown, &dat->chain[object_address], obj->size);
          }
      }
    }

  /*
   if (obj->supertype != DWG_SUPERTYPE_UNKNOWN)
   {
   fprintf (stderr, " Begin address:\t%10lu\n", address);
   fprintf (stderr, " Last address:\t%10lu\tSize: %10lu\n", dat->byte, obj->size);
   fprintf (stderr, "End address:\t%10lu (calculated)\n", address + 2 + obj->size);
   }
   */

  /* Register the previous addresses for return
   */
  dat->byte = previous_address;
  dat->bit = previous_bit;
}

#undef IS_DECODER
