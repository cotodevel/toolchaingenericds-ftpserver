/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#ifndef __conf_h__
#define __conf_h__

#include "dsregs.h"
#include "typedefsTGDS.h"
#include "consoleTGDS.h"
#include "ff.h"

#define MAX_CONFIGS     4

// NL
#define MIN(a, b) (((a) < (b))?(a):(b))
#define AL_ID(a,b,c,d)	    0

#define MAX_ARGV  16

typedef struct CONFIG_ENTRY
{
   char *name;                      /* variable name (NULL if comment) */
   char *data;                      /* variable value */
   struct CONFIG_ENTRY *next;       /* linked list */
} CONFIG_ENTRY;

typedef struct CONFIG
{
   CONFIG_ENTRY *head;              /* linked list of config entries */
   char *filename;                  /* where were we loaded from? */
   int dirty;                       /* has our data changed? */
} CONFIG;

typedef struct CONFIG_HOOK
{
   char *section;                   /* hooked config section info */
   int (*intgetter)(char *name, int def);
   char *(*stringgetter)(char *name, char *def);
   void (*stringsetter)(char *name, char *value);
   struct CONFIG_HOOK *next; 
} CONFIG_HOOK;


#ifdef __cplusplus
extern "C" {
#endif

extern CONFIG *config[MAX_CONFIGS] ;
extern CONFIG *config_override ;
extern CONFIG *config_language ;
extern CONFIG *system_config ;
extern CONFIG_HOOK *config_hook ;
extern int config_installed;		//tells wether config was loaded or not (through false or true typedef)


extern void set_config_file(sint8 *filename);
extern void set_config_data(sint8 *data, int length);
extern void override_config_file(sint8 *filename);
extern void override_config_data(sint8 *data, int length);
extern void flush_config_file(void);
extern void reload_config_texts(sint8 *new_language);

extern void push_config_state(void);
extern void pop_config_state(void);

extern void hook_config_section(sint8 *section,int (*intgetter)(sint8 *, int), sint8 *(*stringgetter)(sint8 *, sint8 *),  void (*stringsetter)(sint8 *,sint8 *));

extern int config_is_hooked(sint8 *section);
extern sint8 * get_config_string(sint8 *section, sint8 *name, sint8 *def);
extern int get_config_int(sint8 *section, sint8 *name, int def);
extern int get_config_hex(sint8 *section, sint8 *name, int def);
extern int get_config_oct(sint8 *section, sint8 *name, int def);
extern float get_config_float(sint8 *section, sint8 *name, float def);
extern int get_config_id(sint8 *section, sint8 *name, int def);
extern sint8 ** get_config_argv(sint8 *section, sint8 *name, int *argc);
extern sint8 * get_config_text(sint8 *msg);

extern void set_config_string(sint8 *section, sint8 *name, sint8 *val);
extern void set_config_int(sint8 *section, sint8 *name, int val);
extern void set_config_hex(sint8 *section, sint8 *name, int val);
extern void set_config_oct(sint8 *section, sint8 *name, int size, int val);
extern void set_config_float(sint8 *section, sint8 *name, float val);
extern void set_config_id(sint8 *section, sint8 *name, int val);

extern int list_config_entries(sint8 *section, sint8 ***names);
extern int list_config_sections(sint8 ***names);
extern void free_config_entries(sint8 ***names);

extern sint8 *find_config_section_with_hex(sint8 *name, int hex);
extern sint8 *find_config_section_with_string(sint8 *name, sint8 *str);
extern int	is_section_exists(sint8 *section);

extern void save_config_file();
extern void save_config(CONFIG *cfg);

extern void destroy_config(CONFIG *cfg);
extern void init_config(int loaddata);

extern void set_config(CONFIG **config, sint8 *data, int length, sint8 *filename);
extern void load_config_file(CONFIG **config, sint8 *filename, sint8 *savefile);
extern void prettify_section_name(sint8 *in, sint8 *out);
extern CONFIG_ENTRY *find_config_string(CONFIG *config, char *section, char *name, CONFIG_ENTRY **prev);
extern CONFIG_ENTRY *insert_variable(CONFIG *the_config, CONFIG_ENTRY *p, char *name, char *data);
extern int get_line(sint8 *data, int length, sint8 *name, sint8 *val);
extern char * getMonthNameByNumericalMonth(int num);
extern void trimStr(char *s);


//fixes TGDS-FTPServer socket running out of memory
extern bool _Wifi_InitTGDSFTPServer();
extern bool Wifi_InitTGDSFTPServer(bool useFirmwareSettings);

extern FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
);

extern FRESULT f_unlink_force_removal (
	const TCHAR* path		/* Pointer to the file or directory path */
);

extern char dirNameFull[256];

#ifdef __cplusplus
}
#endif


#endif