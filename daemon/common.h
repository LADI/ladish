/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  stuff that is needed almost everywhere in the ladishd
 *
 *****************************************************************************/

#ifndef COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED
#define COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED

#include "config.h"             /* configure stage result */

#include <stdbool.h>            /* C99 bool */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <dbus/dbus.h>
#include <uuid/uuid.h>

#include "../dbus/service.h"
#include "../common/klist.h"
#include "../common/debug.h"

extern service_t * g_dbus_service;

#endif /* #ifndef COMMON_H__CFDC869A_31AE_4FA3_B2D3_DACA8488CA55__INCLUDED */
