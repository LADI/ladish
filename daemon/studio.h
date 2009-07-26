/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*****************************************************************************
 *
 * DESCRIPTION:
 *  Studio object helpers
 *
 *****************************************************************************/

#ifndef STUDIO_H__0BEDE85E_4FB3_4D74_BC08_C373A22409C0__INCLUDED
#define STUDIO_H__0BEDE85E_4FB3_4D74_BC08_C373A22409C0__INCLUDED

#include "common.h"

bool
studio_create(
  struct studio ** studio_ptr_ptr);

void
studio_destroy(
  struct studio * studio_ptr);

#endif /* #ifndef STUDIO_H__0BEDE85E_4FB3_4D74_BC08_C373A22409C0__INCLUDED */
