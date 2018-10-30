/**
 * @brief Enduro/X Standard library
 *
 * @file ndrstandard.h
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2018, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL or Mavimax's license for commercial use.
 * -----------------------------------------------------------------------------
 * AGPL license:
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License, version 3 as published
 * by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Affero General Public License, version 3
 * for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * -----------------------------------------------------------------------------
 * A commercial use license is available from Mavimax, Ltd
 * contact@mavimax.com
 * -----------------------------------------------------------------------------
 */
#ifndef NDRSTANDARD_H
#define	NDRSTANDARD_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <ndrx_config.h>
#include <stdint.h>
#include <string.h>
    
#if UINTPTR_MAX == 0xffffffff
/* 32-bit */
#define SYS32BIT
#elif UINTPTR_MAX == 0xffffffffffffffff
#define SYS64BIT
#else
/* wtf */
#endif

#ifndef EXFAIL
#define EXFAIL		-1
#endif
    
#ifndef EXSUCCEED
#define EXSUCCEED		0
#endif

#ifndef expublic
#define expublic
#endif
    
#ifndef exprivate
#define exprivate     	static
#endif

#ifndef EXEOS
#define EXEOS             '\0'
#endif
    
#ifndef EXBYTE
#define EXBYTE(x) ((x) & 0xff)
#endif

#ifndef EXFALSE
#define EXFALSE        0
#endif

#ifndef EXTRUE
#define EXTRUE         1
#endif

/** Directory seperator symbol */
#define EXDIRSEP      '/'

#define N_DIM(a)        (sizeof(a)/sizeof(*(a)))

#ifndef EXFAIL_OUT
#define EXFAIL_OUT(X)    {X=EXFAIL; goto out;}
#endif


#ifndef EXOFFSET
#ifdef SYS64BIT
#define EXOFFSET(STRUCT,ELM)   ((long) &(((STRUCT *)0)->ELM) )
#else
#define EXOFFSET(STRUCT,ELM)   ((const int) &(((STRUCT *)0)->ELM) )
#endif
#endif
    
#define NDRX_WORD_SIZE  (int)sizeof(void *)*8

#ifndef EXELEM_SIZE
#define EXELEM_SIZE(STRUCT,ELM)        (sizeof(((STRUCT *)0)->ELM))
#endif

#define NDRX_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
    
#define NDRX_ATMI_MSG_MAX_SIZE   65536 /* internal */
    
#define NDRX_STACK_MSG_FACTOR        30 /* max number of messages in stack */
    
/* Feature #127 
 * Allow dynamic buffer sizing with Variable Length Arrays (VLS) in C99
 */
extern NDRX_API long ndrx_msgsizemax (void);
#define NDRX_MSGSIZEMAX          ndrx_msgsizemax()

#define NDRX_PADDING_MAX         16 /* Max compiled padding in struct (assumed) */

#if 0
/*These are slow! */
#define NDRX_STRCPY_SAFE(X, Y) {strncpy(X, Y, sizeof(X)-1);\
                          X[sizeof(X)-1]=EXEOS;}
	
#define NDRX_STRNCPY_SAFE(X, Y, N) {strncpy(X, Y, N-1);\
                          X[N]=EXEOS;}
	
#endif

#ifdef HAVE_STRNLEN

#define NDRX_STRNLEN strnlen

#else

extern NDRX_API size_t ndrx_strnlen(char *str, size_t max);

#define NDRX_STRNLEN ndrx_strnlen

#endif
    
#ifdef HAVE_STRLCPY
#define NDRX_STRCPY_SAFE(X, Y) strlcpy(X, Y, sizeof(X));
	
#define NDRX_STRNCPY_SAFE(X, Y, N) strlcpy(X, Y, N);
    
#else
    
/* TODO: switch to STRLCPY if  */
#define NDRX_STRCPY_SAFE(X, Y) {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = sizeof(X)-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
		ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy(X, Y, ndrx_I5SmWDM_len);\
	X[ndrx_I5SmWDM_len]=0;\
	}
    
/**
 * Safe copy to target buffer (not overflowing it...)
 * N - buffer size of X 
 * This always copies EOS
 * @param X dest buffer
 * @param Y source buffer
 * @param N dest buffer size
 */	
#define NDRX_STRNCPY_SAFE(X, Y, N) {\
	int ndrx_I5SmWDM_len = strlen(Y);\
	int ndrx_XgCmDEk_bufzs = (N)-1;\
	if (ndrx_I5SmWDM_len > ndrx_XgCmDEk_bufzs)\
	{\
		ndrx_I5SmWDM_len = ndrx_XgCmDEk_bufzs;\
	}\
	memcpy((X), (Y), ndrx_I5SmWDM_len);\
	(X)[ndrx_I5SmWDM_len]=0;\
	}
#endif

/**
 * in case if dest buffer allows, we copy EOS too
 * This is compatible with strncpy
 */
#define NDRX_STRNCPY(X, Y, N) {\
	int ndrx_I5SmWDM_len = strlen(Y)+1;\
	if (ndrx_I5SmWDM_len > (N))\
	{\
		ndrx_I5SmWDM_len = (N);\
	}\
	memcpy((X), (Y), ndrx_I5SmWDM_len);\
	}

/**
 * Copy the maxing at source buffer, not checking the dest
 * N - number of symbols to test in source buffer.
 * The dest buffer is assumed to be large enough.
 */
#define NDRX_STRNCPY_SRC(X, Y, N) {\
        int ndrx_I5SmWDM_len = NDRX_STRNLEN((Y), (N));\
        memcpy((X), (Y), ndrx_I5SmWDM_len);\
	}

/**
 * Copy last NRLAST_ chars from SRC_ to DEST_
 * safe mode with target buffer size checking.
 * @param DEST_ destination buffer where to copy the string (should be static def)
 * @param SRC_ source string to copy last bytes off
 * @param NRLAST_ number of bytes to copy from string to dest
 */
#define NDRX_STRCPY_LAST_SAFE(DEST_, SRC_, NRLAST_) {\
        int ndrx_KFWnP6Q_len = strlen(SRC_);\
        if (ndrx_KFWnP6Q_len > (NRLAST_)) {\
            NDRX_STRCPY_SAFE((DEST_), ((SRC_)+ (ndrx_KFWnP6Q_len - (NRLAST_))) );\
        } else {\
            NDRX_STRCPY_SAFE((DEST_), (SRC_));\
        }\
	}

#ifdef EX_HAVE_STRCAT_S

#define NDRX_STRCAT_S(DEST_, DEST_SIZE_, SRC_) strcat_s(DEST_, DEST_SIZE_, SRC_)

#else
/**
 * Cat string at the end. Dest size of the string is given
 * @param DEST_ dest buffer
 * @param DEST_SIZE_ dest buffer size
 * @param SRC_ source buffer to copy
 */
#define NDRX_STRCAT_S(DEST_, DEST_SIZE_, SRC_) {\
        int ndrx_VeIlgbK9tx_len = strlen(DEST_);\
        NDRX_STRNCPY_SAFE( (DEST_+ndrx_VeIlgbK9tx_len), SRC_, (DEST_SIZE_ - ndrx_VeIlgbK9tx_len));\
}
#endif

/*
 * So we use these two macros where we need know that more times they will be
 * true, than false. This makes some boost for CPU code branching.
 * 
 * So for example if expect that some variable (c) must not be NULL
 * then we could run NDRX_UNLIKELY(NDRX_UNLIKELY).
 * This is useful for error handling, because mostly we do not have errors like
 * malloc fail etc..
 */
#if HAVE_EXPECT

#define NDRX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define NDRX_UNLIKELY(x)    __builtin_expect(!!(x), 0)

#else

/*
 * And this is fallback if we do not have expect compiler option.
 */
#define NDRX_LIKELY(x)      (x)
#define NDRX_UNLIKELY(x)    (x)

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* NDRSTANDARD_H */

/* vim: set ts=4 sw=4 et smartindent: */
