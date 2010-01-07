/*****************************************************************************/
/*  LibreDWG - Free DWG library                                              */
/*  http://code.google.com/p/libredwg/                                       */
/*                                                                           */
/*    based on LibDWG - Free DWG read-only library                           */
/*    http://sourceforge.net/projects/libdwg                                 */
/*    originally written by Felipe Castro <felipo at users.sourceforge.net>  */
/*                                                                           */
/*  Copyright (C) 2008, 2009 Free Software Foundation, Inc.                  */
/*  Copyright (C) 2009 Felipe Sanches <jucablues@users.sourceforge.net>      */
/*  Copyright (C) 2009 Rodrigo Rodrigues da Silva <pitanga@members.fsf.org>  */
/*                                                                           */
/*  This library is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either versionn 3 of the License, or (at your option) any later versionn.*/
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

/* Main source file of the library, whith the API functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bits.h"
#include "common.h"
#include "decode.h"
#include "dwg.h"

/*------------------------------------------------------------------------------
 * Public functions
 */
int
dwg_read_file(char *filename, Dwg_Data * dwg_data)
{
  int sign;
  FILE *fp;
  struct stat attrib;
  size_t size;
  Bit_Chain bit_chain;

  if (stat(filename, &attrib))
    {
      fprintf(stderr, "File not found: %s\n", filename);
      return -1;
    }
  if (!S_ISREG (attrib.st_mode))
    {
      fprintf(stderr, "Error: %s\n", filename);
      return -1;
    }
  fp = fopen(filename, "rb");
  if (!fp)
    {
      fprintf(stderr, "Error while opening the file: %s\n", filename);
      return -1;
    }

  /* Load file to memory
   */
  bit_chain.bit = 0;
  bit_chain.byte = 0;
  bit_chain.size = attrib.st_size;
  bit_chain.chain = (unsigned char *) malloc(bit_chain.size);
  if (!bit_chain.chain)
    {
      fprintf(stderr, "Not enough memory.\n");
      fclose(fp);
      return -1;
    }
  size = 0;
  size = fread(bit_chain.chain, sizeof(char), bit_chain.size, fp);
  if (size != bit_chain.size)
    {
      fprintf(stderr, "Could not read the entire file (%lu out of %lu): %s\n",
          (long unsigned int) size, bit_chain.size, filename);
      fclose(fp);
      free(bit_chain.chain);
      return -1;
    }
  fclose(fp);

  /* Decode the dwg structure
   */
  if (dwg_decode_data(&bit_chain, dwg_data))
    {
      fprintf(stderr, "Failed to decode file: %s\n", filename);
      free(bit_chain.chain);
      return -1;
    }
  free(bit_chain.chain);

  return 0;
}

int
dwg_write_file(char *filename, Dwg_Data * dwg_data)
{
  FILE *dt;
  struct stat atrib;
  Bit_Chain bit_chain;
  bit_chain.version = (Dwg_Version_Type)dwg_data->header.version;

  /* Encode the DWG struct
   bit_chain.size = 0;
   if (dwg_encode_chains (dwg_struct, &bit_chain))
   {
   fprintf (stderr, "Failed to encode datastructure.\n");
   if (bit_chain.size > 0)
   free (bit_chain.chain);
   return -1;
   }
   */

  /* try opening the output file in write mode
   if (!stat (filename, &atrib))
   {
   fprintf (stderr, "The file already exists. We won't overwrite it.");
   return -1;
   }
   dt = fopen (filename, "w");
   if (!dt)
   {
   fprintf (stderr, "Failed to create the file: %s\n", filename);
   return -1;
   }
   */

  /* Write the data into the file
   if (fwrite (bit_chain.chain, sizeof (char), bit_chain.size, dt) != bit_chain.size)
   {
   fprintf (stderr, "Failed to write data into the file: %s\n", filename);
   fclose (dt);
   free (bit_chain.chain);
   return -1;
   }
   fclose (dt);

   if (bit_chain.size > 0)
   free (bit_chain.chain);
   */
  return 0;
}

unsigned char *
dwg_bmp(Dwg_Data *stk, long int *size)
{
  char num_pictures;
  char code;
  unsigned i;
  int plene;
  long int header_size;
  Bit_Chain *dat;

  dat = (Bit_Chain*) &stk->picture;
  dat->bit = 0;
  dat->byte = 0;

  bit_read_RL(dat);
  num_pictures = bit_read_RC(dat);
  //printf ("num_pictures: %i\n", num_pictures);

  *size = 0;
  plene = 0;
  header_size = 0;
  for (i = 0; i < num_pictures; i++)
    {
      code = bit_read_RC(dat);
      //printf ("\t%i - Code: %i\n", i, code);
      //printf ("\t\tAdress: 0x%x\n", bit_read_RL (dat));
      bit_read_RL(dat);
      if (code == 1)
        {
          header_size += bit_read_RL(dat);
          //printf ("\t\tHeader size: %i\n", header_size);
        }
      else if (code == 2 && plene == 0)
        {
          *size = bit_read_RL(dat);
          plene = 1;
          //printf ("\t\tBMP size: %i\n", *size);
        }
      else if (code == 3)
        {
          bit_read_RL(dat);
          //printf ("\t\tWMF size: 0x%x\n", bit_legi_RL (dat));
        }
      else
        {
          bit_read_RL(dat);
          //printf ("\t\tSize: 0x%x\n", bit_read_RL (dat));
        }
    }
  dat->byte += header_size;
  //printf ("Current adress: 0x%x\n", dat->byte);

  if (*size > 0)
    return (dat->chain + dat->byte);
  else
    return NULL;
}

double
dwg_model_x_min(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMIN_MSPACE.x;
}

double
dwg_model_x_max(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMAX_MSPACE.x;
}

double
dwg_model_y_min(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMIN_MSPACE.y;
}

double
dwg_model_y_max(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMAX_MSPACE.y;
}

double
dwg_model_z_min(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMIN_MSPACE.z;
}

double
dwg_model_z_max(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMAX_MSPACE.z;
}

double
dwg_page_x_min(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMIN_PSPACE.x;
}

double
dwg_page_x_max(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMAX_PSPACE.x;
}

double
dwg_page_y_min(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMIN_PSPACE.y;
}

double
dwg_page_y_max(Dwg_Data *dwg)
{
  return dwg->header_vars.EXTMAX_PSPACE.y;
}

unsigned int
dwg_get_layer_count(Dwg_Data *dwg)
{
  return dwg->layer_control->tio.object->tio.LAYER_CONTROL->num_entries;
}

Dwg_Object_LAYER **
dwg_get_layers(Dwg_Data *dwg)
{
  int i;
  Dwg_Object_LAYER ** layers = (Dwg_Object_LAYER **) malloc(
		dwg_get_layer_count(dwg) * sizeof (Dwg_Object_LAYER*));
  for (i=0; i<dwg_get_layer_count(dwg); i++)
    {
      layers[i] = dwg->layer_control->tio.object->tio.LAYER_CONTROL->
            layers[i]->obj->tio.object->tio.LAYER;
    }
  return layers;
}

long unsigned int
dwg_get_object_count(Dwg_Data *dwg)
{
  return dwg->num_objects;
}

long unsigned int
dwg_get_object_object_count(Dwg_Data *dwg)
{
  return dwg->num_objects - dwg->num_entities;
}

long unsigned int
dwg_get_entity_count(Dwg_Data *dwg)
{
  return dwg->num_entities;
}

Dwg_Object_Entity **
dwg_get_entities(Dwg_Data *dwg)
{
  long unsigned int i, ent_count = 0;
  Dwg_Object_Entity ** entities = (Dwg_Object_Entity **) malloc(
                dwg_get_entity_count(dwg) * sizeof (Dwg_Object_Entity*));
  for (i=0; i<dwg->num_objects; i++)
    {
      if (dwg->object[i].supertype == DWG_SUPERTYPE_ENTITY)
        {
          entities[ent_count] = dwg->object[i].tio.entity;
          ent_count++;
        }
    }
  return entities;
}

Dwg_Object_LAYER *
dwg_get_entity_layer(Dwg_Object_Entity * ent)
{
  return ent->layer->obj->tio.object->tio.LAYER;
}

void
dwg_free(Dwg_Data * dwg)
{
  if (dwg->header.section)
    free(dwg->header.section);
}
