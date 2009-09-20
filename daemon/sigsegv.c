/* -*- Mode: C ; c-basic-offset: 2 -*- */
/*
 * LADI Session Handler (ladish)
 *
 * Copyright (C) 2005 - 2008 Jaco Kroon <jaco@kroon.co.za>
 *
 **************************************************************************
 * This file contains code to print out a stack-trace when program segfaults.
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

#include "config.h"

#define NO_CPP_DEMANGLE
#define SIGSEGV_NO_AUTO_INIT

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <errno.h>
#ifndef NO_CPP_DEMANGLE
//#include <cxxabi.h>
char * __cxa_demangle(const char * __mangled_name, char * __output_buffer, size_t * __length, int * __status);
#endif

#include "../log.h"

#if defined(REG_RIP)
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif defined(REG_EIP)
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

static void signal_segv(int signum, siginfo_t* info, void*ptr) {
    static const char *si_codes[3] = {"", "SEGV_MAPERR", "SEGV_ACCERR"};

    size_t i;
    ucontext_t *ucontext = (ucontext_t*)ptr;

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
    int f = 0;
    Dl_info dlinfo;
    void **bp = 0;
    void *ip = 0;
#else
    void *bt[20];
    char **strings;
    size_t sz;
#endif

    if (signum == SIGSEGV)
    {
        log_error("Segmentation Fault!");
    }
    else if (signum == SIGABRT)
    {
        log_error("Abort!");
    }
    else if (signum == SIGILL)
    {
        log_error("Illegal instruction!");
    }
    else if (signum == SIGFPE)
    {
        log_error("Floating point exception!");
    }
    else
    {
        log_error("Unknown bad signal catched!");
    }

    log_error("info.si_signo = %d", signum);
    log_error("info.si_errno = %d", info->si_errno);
    log_error("info.si_code  = %d (%s)", info->si_code, si_codes[info->si_code]);
    log_error("info.si_addr  = %p", info->si_addr);
    for(i = 0; i < NGREG; i++)
        log_error("reg[%02d]       = 0x" REGFORMAT, i, ucontext->uc_mcontext.gregs[i]);

#if defined(SIGSEGV_STACK_X86) || defined(SIGSEGV_STACK_IA64)
# if defined(SIGSEGV_STACK_IA64)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_RIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_RBP];
# elif defined(SIGSEGV_STACK_X86)
    ip = (void*)ucontext->uc_mcontext.gregs[REG_EIP];
    bp = (void**)ucontext->uc_mcontext.gregs[REG_EBP];
# endif

    log_error("Stack trace:");
    while(bp && ip) {
        if(!dladdr(ip, &dlinfo))
            break;

        const char *symname = dlinfo.dli_sname;
#ifndef NO_CPP_DEMANGLE
        int status;
        char *tmp = __cxa_demangle(symname, NULL, 0, &status);

        if(status == 0 && tmp)
            symname = tmp;
#endif

        log_error("% 2d: %p <%s+%u> (%s)",
                ++f,
                ip,
                symname,
                (unsigned)(ip - dlinfo.dli_saddr),
                dlinfo.dli_fname);

#ifndef NO_CPP_DEMANGLE
        if(tmp)
            free(tmp);
#endif

        if(dlinfo.dli_sname && !strcmp(dlinfo.dli_sname, "main"))
            break;

        ip = bp[1];
        bp = (void**)bp[0];
    }
#else
    log_error("Stack trace (non-dedicated):");
    sz = backtrace(bt, 20);
    strings = backtrace_symbols(bt, sz);

    for(i = 0; i < sz; ++i)
        log_error("%s", strings[i]);
#endif
    log_error("End of stack trace");
    exit (-1);
}

int setup_sigsegv() {
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_segv;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGSEGV, &action, NULL) < 0) {
        log_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGILL, &action, NULL) < 0) {
        log_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGABRT, &action, NULL) < 0) {
        log_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    if(sigaction(SIGFPE, &action, NULL) < 0) {
        log_error("sigaction failed. errno is %d (%s)", errno, strerror(errno));
        return 0;
    }

    return 1;
}

#ifndef SIGSEGV_NO_AUTO_INIT
static void __attribute((constructor)) init(void) {
    setup_sigsegv();
}
#endif
