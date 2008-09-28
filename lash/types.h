/*
 *   LASH
 *
 *   Copyright (C) 2008 Juuso Alasuutari <juuso.alasuutari@gmail.com>
 *   Copyright (C) 2002 Robert Ham <rah@bash.sh>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __LASH_TYPES_H__
#define __LASH_TYPES_H__

#include <stdbool.h>
#include <uuid/uuid.h>

#ifdef __cplusplus
extern "C" {
#endif


enum
{
	LASH_TYPE_DOUBLE  = 'd',
	LASH_TYPE_INTEGER = 'u',
	LASH_TYPE_STRING  = 's',
	LASH_TYPE_RAW     = '-',
};


enum LASH_Client_Flag
{
  LASH_Config_Data_Set    =  0x00000001,  /* wants to save data on the server */
  LASH_Config_File        =  0x00000002,  /* wants to save data in files */

  LASH_Server_Interface   =  0x00000004,  /* is a server interface */
  LASH_No_Autoresume      =  0x00000008,  /* server shouldn't try to resume a lost client with this one */
  LASH_Terminal           =  0x00000010,   /* runs in a terminal */

  LASH_No_Start_Server    =  0x00000020   /* do not attempt to automatically start server */
};


enum LASH_Event_Type
{
  LASH_Event_Unknown = 0,

  /* for normal clients */
  LASH_Client_Name = 1,  /* set the client's user-visible name */
  LASH_Jack_Client_Name, /* tell the server what name the client is connected to jack with */
  LASH_Alsa_Client_ID,   /* tell the server what id the client is connected to the alsa sequencer with */
  LASH_Save_File,        /* tell clients to save to files */
  LASH_Restore_File,     /* tell clients to restore from files */
  LASH_Save_Data_Set,    /* tell clients to send the server a data set */
  LASH_Restore_Data_Set, /* tell clients a data set will be arriving */
  LASH_Save,             /* save the project */
  LASH_Quit,             /* tell the server to close the connection */
  LASH_Server_Lost,      /* the server disconnected */

  /* for the server interface */
  LASH_Project_Add,             /* new project has been created */
  LASH_Project_Remove,          /* existing project has been lost */
  LASH_Project_Dir,             /* change project dir */
  LASH_Project_Name,            /* change project name */
  LASH_Client_Add,              /* a new client has been added to a project */
  LASH_Client_Remove,           /* a client has been lost from a project */
  LASH_Percentage               /* display a percentage of an action to the user */
};


/** LASH client handle */
typedef struct _lash_client lash_client_t;

/** Opaque handle used for reading and writing configs. The functions
    \ref lash_config_read, \ref lash_config_write, and
    \ref lash_config_write_raw must always be supplied with the
    lash_config_handle_t pointer provided by liblash as the first
    parameter of a \ref LashConfigCallback function. */
typedef struct _lash_config_handle lash_config_handle_t;

/** Client event callback function */
typedef bool (*LashEventCallback) (void *user_data);

/** Client config message callback function */
typedef bool (*LashConfigCallback) (lash_config_handle_t *config_handle,
                                    void                 *user_data);

/** Controller event callback function type. */
typedef void (*LashControlCallback) (enum LASH_Event_Type  type,
                                     const char           *string1,
                                     const char           *string2,
                                     uuid_t                client_id,
                                     void                 *user_data);


/* Types which belong entirely to the old API */

#include <stdint.h>

/* the set of lash-specific arguments that are extracted from argc/argv */
typedef struct _lash_args lash_args_t;

/* an event */
typedef struct _lash_event lash_event_t;

/* a bit of key-indexed data */
typedef struct _lash_config lash_config_t;

typedef uint32_t lash_protocol_t;

#ifdef __cplusplus
}
#endif


#endif /* __LASH_TYPES_H__ */
