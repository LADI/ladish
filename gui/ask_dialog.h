/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2009 Nedko Arnaudov <nedko@arnaudov.name>
 *
 **************************************************************************
 * This file contains the interface of ask dialog
 **************************************************************************
 *
 * LADI Session Handler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LADI Session Handler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LADI Session Handler. If not, see <http://www.gnu.org/licenses/>
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ASK_DIALOG_H__67D26AF3_8FEA_44E4_B7CA_744898EF85D2__INCLUDED
#define ASK_DIALOG_H__67D26AF3_8FEA_44E4_B7CA_744898EF85D2__INCLUDED

#include "common.h"

bool
ask_dialog(
  bool * result_ptr,            /* true - yes, false - no */
  const char * text,
  const char * secondary_text_format,
  ...);

#endif /* #ifndef ASK_DIALOG_H__67D26AF3_8FEA_44E4_B7CA_744898EF85D2__INCLUDED */
