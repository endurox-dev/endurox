/* 
** General routines for handling buffer conversation
**
** @file typed_buf.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, Mavimax, Ltd. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or Mavimax's license for commercial use.
** -----------------------------------------------------------------------------
** GPL license:
** 
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
** PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA 02111-1307 USA
**
** -----------------------------------------------------------------------------
** A commercial use license is available from Mavimax, Ltd
** contact@mavimax.com
** -----------------------------------------------------------------------------
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <utlist.h>

#include <atmi.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <thlock.h> /* muli thread support */
#include <typed_string.h>
#include <typed_json.h>
#include <typed_carray.h>
#include <tperror.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
private buffer_obj_t * find_buffer_int(char *ptr);
/*
 * Buffers allocated by process
 */
public buffer_obj_t *G_buffers=NULL;

MUTEX_LOCKDECL(M_lock); /* This will allow multiple reads */

/*
 * Buffer descriptors
 */
public typed_buffer_descr_t G_buf_descr[] =
{
    {BUF_TYPE_UBF,   "UBF",     "FML",     NULL, UBF_prepare_outgoing, UBF_prepare_incoming,
                                UBF_tpalloc, UBF_tprealloc, UBF_tpfree, UBF_test},
	/* Support for FB32... */
    {BUF_TYPE_UBF,   "UBF32",     "FML32",     NULL, UBF_prepare_outgoing, UBF_prepare_incoming,
                                UBF_tpalloc, UBF_tprealloc, UBF_tpfree, UBF_test},
    {BUF_TYPE_INIT,  "TPINIT",  NULL,      NULL,             NULL,            NULL,
                                TPINIT_tpalloc, NULL, TPINIT_tpfree, NULL},
    {BUF_TYPE_NULL,  NULL,  NULL,      NULL,             NULL,            NULL,
                                TPNULL_tpalloc, NULL, TPNULL_tpfree, NULL},
    {BUF_TYPE_STRING,   "STRING", NULL,     NULL, STRING_prepare_outgoing, STRING_prepare_incoming,
                                STRING_tpalloc, STRING_tprealloc, STRING_tpfree, STRING_test},
    {BUF_TYPE_CARRAY,   "CARRAY", "X_OCTET", NULL, CARRAY_prepare_outgoing, CARRAY_prepare_incoming,
                                CARRAY_tpalloc, CARRAY_tprealloc, CARRAY_tpfree, CARRAY_test},
    {BUF_TYPE_JSON,   "JSON", NULL,     NULL, JSON_prepare_outgoing, JSON_prepare_incoming,
                                JSON_tpalloc, JSON_tprealloc, JSON_tpfree, JSON_test},
#if 0
/* Those bellow ones are not supported! */
    {BUF_TYPE_STRING,"STRING",  NULL,      NULL, /* todo:  */NULL, /* todo: */NULL, NULL},
    {BUF_TYPE_CARRAY,"CARRAY",  "X_OCTET", NULL, /* todo:  */NULL, /* todo: */NULL, NULL},
#endif
    {FAIL}
};

/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/


/**
 * Compares two buffers in linked list (for search purposes)
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
private int buf_ptr_cmp_fn(buffer_obj_t *a, buffer_obj_t *b)
{
    return (a->buf==b->buf?SUCCEED:FAIL);
}

/**
 * Find the buffer in list of known buffers
 * Publick version, uses locking...
 * TODO: might want to optimize, so that multiple paralel reads are allowed
 * while there is no write...
 * @param ptr
 * @return NULL - buffer not found/ptr - buffer found
 */
public buffer_obj_t * find_buffer(char *ptr)
{
    MUTEX_LOCK_V(M_lock);
    {
    buffer_obj_t *ret;
    
    /*
    ret = find_buffer_int(ptr);
    */
    EXHASH_FIND_PTR( G_buffers, ((void **)&ptr), ret);
        
        
    MUTEX_UNLOCK_V(M_lock);
    return ret;
    }
}

/**
 * Find the buffer in list of known buffers
 * Internal version, no locking used.
 * TODO: Move buffer registry to hash function
 * @param ptr
 * @return NULL - buffer not found/ptr - buffer found
 */
private buffer_obj_t * find_buffer_int(char *ptr)
{
    buffer_obj_t *ret=NULL;

    /*
    eltmp.buf = ptr;
    DL_SEARCH(G_buffers, ret, &eltmp, buf_ptr_cmp_fn);
    */
    
    EXHASH_FIND_PTR( G_buffers, ((void **)&ptr), ret);
    
    return ret;
}

/**
 * Return the descriptor of typed buffer or NULL in case of error.
 * @param type - type to search for ( must be present )
 * @param subtype - may be NULL
 * @return NULL/or ptr to G_buf_descr[X]
 */
private typed_buffer_descr_t * get_buffer_descr(char *type, char *subtype)
{
    typed_buffer_descr_t *p=G_buf_descr;
    typed_buffer_descr_t *ret = NULL;

    while (FAIL!=p->type_id)
    {
        if ((NULL!=p->type && 0==strcmp(p->type, type)) || 
                        (NULL!=p->alias && 0==strcmp(p->alias, type)) ||
                        p->type == type /*NULL buffer*/)
        {
            /* check subtype (if used) */
            if ((NULL!=p->subtype && (NULL==subtype || EOS==subtype[0])) ||
                        (NULL==p->subtype && NULL!=subtype && EOS!=subtype[0]))
            {
                /* search for next */
            } /* Assume empty string subtype as empty/null */
            else if (NULL!=p->subtype && NULL!=subtype && EOS!=subtype[0])
            {
                /* compare subtypes */
                if (0==strcmp(p->subtype, subtype))
                {
                    ret=p;
                    break;
                }
            }
            else
            {
                ret=p;
                break;
            }
        }
        p++;
    }
    return ret;
}

/**
 * Internal version of tpalloc
 * @param type
 * @param subtype
 * @param len
 * @return
 */
public char * _tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len)
{
    MUTEX_LOCK_V(M_lock);
    {
    char *ret=NULL;
    int i=0;
    typed_buffer_descr_t *usr_type = NULL;
    buffer_obj_t *node;
    char fn[] = "_tpalloc";
    
    NDRX_LOG(log_debug, "%s: type=%s len=%d", fn, type, len);
    
    if (NULL==known_type)
    {
        if (NULL==(usr_type = get_buffer_descr(type, subtype)))
        {
            _TPset_error_fmt(TPEOTYPE, "Unknown type (%s)/subtype(%s)", type, subtype);
            ret=NULL;
            goto out;
        }
    }
    else
    {
        /* will re-use known type */
        usr_type = known_type;
    }

    /* now allocate the memory  */
    if (NULL==(ret=usr_type->pf_alloc(usr_type, len)))
    {
        /* error detail should be already set */
        goto out;
    }

    /* now append the memory list with allocated block */
    if (NULL==(node=(buffer_obj_t *)NDRX_MALLOC(sizeof(buffer_obj_t))))
    {
        _TPset_error_fmt(TPEOS, "%s: Failed to allocate buffer list node: %s", fn,
                                        strerror(errno));
        ret=NULL;

        goto out;
    }

    /* Now append the list */
    memset(node, 0, sizeof(buffer_obj_t));

    node->buf = ret;
    NDRX_LOG(log_debug, "%s: type=%s len=%d allocated=%p", fn, type, len, ret);
    node->size = len;
    
    node->type_id = usr_type->type_id;
    node->sub_type_id = 0; /* So currently sub-type not supported. ok */

    /* Save the allocated buffer in the list */
    /* DL_APPEND(G_buffers, node); */
    EXHASH_ADD_PTR(G_buffers, buf, node);

out:
    MUTEX_UNLOCK_V(M_lock);
    return ret;
    }
}

/**
 * Internal version of tprealloc
 * @param buf
 * @param
 * @return
 */
public char * _tprealloc (char *buf, long len)
{
    MUTEX_LOCK_V(M_lock);
    {
    char *ret=NULL;
    buffer_obj_t * node;
    char fn[] = "_tprealloc";
    typed_buffer_descr_t *buf_type = NULL;

    NDRX_LOG(log_debug, "_tprealloc buf=%p, len=%ld", buf, len);
    if (NULL==(node=find_buffer_int(buf)))
    {
         _TPset_error_fmt(TPEINVAL, "%s: Buffer %p is not know to system", fn, buf);
        ret=NULL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "%s buf=%p autoalloc=%hd", 
                        fn, buf, node->autoalloc);

    buf_type = &G_buf_descr[node->type_id];

    /*
     * Do the actual buffer re-allocation!
     */
    if (NULL==(ret=buf_type->pf_realloc(buf_type, buf, len)))
    {
        ret=NULL;
        goto out;
    }

    node->buf = ret;
    /* update the hash list */
    EXHASH_DEL(G_buffers, node);
    EXHASH_ADD_PTR(G_buffers, buf, node);
    
    node->size = len;

out:
    MUTEX_UNLOCK_V(M_lock);
    return ret;
    }
}

#if 0
/**
 * Free up allocated buffers
 */
public void free_up_buffers(void)
{
    MUTEX_LOCK_V(M_lock);
    {
        
    buffer_obj_t *elt, *tmp;
    typed_buffer_descr_t *buf_type = NULL;
    
    DL_FOREACH_SAFE(G_buffers,elt,tmp) 
    {
        /* Free up the buffer */
        buf_type = &G_buf_descr[elt->sub_type_id];
        buf_type->pf_free(buf_type, elt->buf);
        DL_DELETE(G_buffers,elt);
        NDRX_FREE(elt);
    }
    
    MUTEX_UNLOCK_V(M_lock);
    }
}
#endif

/**
 * Remove the buffer
 * @param buf
 */
public void _tpfree (char *buf, buffer_obj_t *known_buffer)
{
    MUTEX_LOCK_V(M_lock);
    {
    buffer_obj_t *elt;
    typed_buffer_descr_t *buf_type = NULL;

    NDRX_LOG(log_debug, "_tpfree buf=%p", buf);
    /* Work out the buffer */
    if (NULL!=known_buffer)
        elt=known_buffer;
    else
        elt=find_buffer_int(buf);

    if (NULL!=elt)
    {
        buf_type = &G_buf_descr[elt->type_id];
        /* Remove it! */
        buf_type->pf_free(buf_type, elt->buf);
        
        /* Remove that stuff from our linked list!! */
        /* DL_DELETE(G_buffers,elt); */
        EXHASH_DEL(G_buffers, elt);
        
        /* delete elt by it self */
        NDRX_FREE(elt);
    }
    MUTEX_UNLOCK_V(M_lock);
    }
}

/**
 * Check is buffer auto or not
 * @param buf ATMI allocated buffer
 * @return TRUE (1) if auto buffer, 0 manual buffer, -1 if error (unknown buffer)
 */
public int _tpisautobuf(char *buf)
{
    int ret;
    
    MUTEX_LOCK_V(M_lock);
    {
        buffer_obj_t *elt;

        elt=find_buffer_int(buf);

        if (NULL!=elt)
        {
            ret = elt->autoalloc;
            NDRX_LOG(log_debug, "_tpisautobuf buf=%p, autoalloc=%d", buf, ret);
        }
        else
        {
            _TPset_error_msg(TPEINVAL, "ptr points to unknown buffer, "
                "not allocated by tpalloc()!");
            ret=FAIL;
        }
        
    MUTEX_UNLOCK_V(M_lock);
    }
    
    return ret;
}

#if 0
/**
 * This does free up auto-allocated buffers.
 */
public void free_auto_buffers(void)
{
    MUTEX_LOCK_V(M_lock);
    {
        
    buffer_obj_t *elt, *tmp;
    typed_buffer_descr_t *buf_type = NULL;
    
    NDRX_LOG(log_debug, "free_auto_buffers enter");
    
    DL_FOREACH_SAFE(G_buffers,elt,tmp) 
    {
        if (elt->autoalloc)
        {
            NDRX_LOG(log_debug, "Free-up auto buffer : %p typeid: %d", 
                    elt->buf, elt->type_id);
            
            /* Free up the buffer */
            buf_type = &G_buf_descr[elt->type_id];
            buf_type->pf_free(buf_type, elt->buf);
            DL_DELETE(G_buffers,elt);
            /* fix up memory leak issues. */
            NDRX_FREE(elt);
        }
    }
    
    MUTEX_UNLOCK_V(M_lock);
    }
}
#endif

/**
 * Internal version of tptypes. Returns type info
 * @param ptr
 * @param type
 * @param subtype
 * @return 
 */
public long _tptypes (char *ptr, char *type, char *subtype)
{
    MUTEX_LOCK_V(M_lock);
    {
    long ret=SUCCEED;
    
    typed_buffer_descr_t *buf_type = NULL;

    buffer_obj_t *buf =  find_buffer_int(ptr);
    
    if (NULL==buf)
    {
        _TPset_error_msg(TPEINVAL, "ptr points to unknown buffer, "
                "not allocated by tpalloc()!");
        ret=FAIL;
        goto out;
    }
    else
    {
        ret = buf->size;
    }
    
    buf_type = &G_buf_descr[buf->type_id];
    
    if (NULL!=type)
    {
        
        strcpy(type, buf_type->type);
    }
    
    if (NULL!=subtype && NULL!=buf_type->subtype)
    {
        strcpy(subtype, buf_type->type);
    }
    
out:
    MUTEX_UNLOCK_V(M_lock);
    return ret;
    }
}
