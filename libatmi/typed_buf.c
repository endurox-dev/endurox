/**
 * @brief General routines for handling buffer conversation
 *
 * @file typed_buf.c
 */
/* -----------------------------------------------------------------------------
 * Enduro/X Middleware Platform for Distributed Transaction Processing
 * Copyright (C) 2009-2016, ATR Baltic, Ltd. All Rights Reserved.
 * Copyright (C) 2017-2019, Mavimax, Ltd. All Rights Reserved.
 * This software is released under one of the following licenses:
 * AGPL (with Java and Go exceptions) or Mavimax's license for commercial use.
 * See LICENSE file for full text.
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys_primitives.h>
#include <utlist.h>

#include <atmi.h>
#include <atmi_int.h>
#include <ubf.h>
#include <ndrstandard.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndebug.h>
#include <thlock.h> /* muli thread support */
#include <typed_string.h>
#include <typed_json.h>
#include <typed_carray.h>
#include <typed_view.h>
#include <tperror.h>
#include <tpadm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
exprivate buffer_obj_t * find_buffer_int(char *ptr);
/*
 * Buffers allocated by process
 */
expublic buffer_obj_t *ndrx_G_buffers=NULL;

exprivate MUTEX_LOCKDECL(M_lock); /* This will allow multiple reads */

/*
 * Buffer descriptors
 */
expublic typed_buffer_descr_t G_buf_descr[] =
{
    {BUF_TYPE_UBF,   "UBF",     "FML",     NULL, UBF_prepare_outgoing, UBF_prepare_incoming,
                                UBF_tpalloc, UBF_tprealloc, UBF_tpfree, UBF_test},
	/* Support for FB32... */
    {BUF_TYPE_UBF,   "UBF32",     "FML32",     NULL, UBF_prepare_outgoing, UBF_prepare_incoming,
                                UBF_tpalloc, UBF_tprealloc, UBF_tpfree, UBF_test},
    {BUF_TYPE_INIT,  "TPINIT",  NULL,      NULL,             NULL,            NULL,
                                TPINIT_tpalloc, NULL, TPINIT_tpfree, NULL},
                                
    {BUF_TYPE_NULL,  "NULL",  NULL,      NULL, TPNULL_prepare_outgoing,  TPNULL_prepare_incoming,
                                TPNULL_tpalloc, NULL, TPNULL_tpfree, NULL},
                                
    {BUF_TYPE_STRING,   "STRING", NULL,     NULL, STRING_prepare_outgoing, STRING_prepare_incoming,
                                STRING_tpalloc, STRING_tprealloc, STRING_tpfree, STRING_test},
                                
    {BUF_TYPE_CARRAY,   "CARRAY", "X_OCTET", NULL, CARRAY_prepare_outgoing, CARRAY_prepare_incoming,
                                CARRAY_tpalloc, CARRAY_tprealloc, CARRAY_tpfree, CARRAY_test},
    {BUF_TYPE_JSON,   "JSON", NULL,     NULL, JSON_prepare_outgoing, JSON_prepare_incoming,
                                JSON_tpalloc, JSON_tprealloc, JSON_tpfree, JSON_test},
    {BUF_TYPE_VIEW,   "VIEW", "VIEW32",     NULL, VIEW_prepare_outgoing, VIEW_prepare_incoming,
                                VIEW_tpalloc, VIEW_tprealloc, VIEW_tpfree, VIEW_test},
    {EXFAIL}
};

/*---------------------------Statics------------------------------------*/

/**
 * This is generic NULL buffer object
 */
exprivate buffer_obj_t M_nullbuf = {.autoalloc = EXFALSE, 
        .buf = NULL, 
        .size=0, 
        .subtype="", 
        .type_id=BUF_TYPE_NULL};
    
/*---------------------------Prototypes---------------------------------*/

/**
 * Compares two buffers in linked list (for search purposes)
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
exprivate int buf_ptr_cmp_fn(buffer_obj_t *a, buffer_obj_t *b)
{
    return (a->buf==b->buf?EXSUCCEED:EXFAIL);
}


/**
 * List currently allocated XATMI buffers
 * @param list buffer list (pointers to actual data storage, not the index entries)
 * @return EXSUCCEED (caller must free list)/EXFAIL (list is not alloc)
 */
expublic int ndrx_buffer_list(ndrx_growlist_t *list)
{
    int ret = EXSUCCEED;
    int i = 0;
    buffer_obj_t *elt, *tmp;
    
    ndrx_growlist_init(list, 100, sizeof(void *));
    
    MUTEX_LOCK_V(M_lock);
    EXHASH_ITER(hh,ndrx_G_buffers,elt,tmp) 
    {
        ndrx_growlist_add(list, elt->buf, i);
        i++;
    }
    MUTEX_UNLOCK_V(M_lock);
    
out:
        
    if (EXSUCCEED!=ret)
    {
        ndrx_growlist_free(list);
    }

    return ret;
}


/**
 * Find the buffer in list of known buffers
 * Publick version, uses locking...
 * TODO: might want to optimize, so that multiple paralel reads are allowed
 * while there is no write...
 * @param ptr
 * @return NULL - buffer not found/ptr - buffer found
 */
expublic buffer_obj_t * ndrx_find_buffer(char *ptr)
{
    buffer_obj_t *ret;
   
    if (NULL==ptr)
    {
        return &M_nullbuf;
    }
    
    MUTEX_LOCK_V(M_lock);
    EXHASH_FIND_PTR( ndrx_G_buffers, ((void **)&ptr), ret);
    MUTEX_UNLOCK_V(M_lock);
    
    return ret;
    
}

/**
 * Find the buffer in list of known buffers
 * Internal version, no locking used.
 * TODO: Move buffer registry to hash function
 * @param ptr
 * @return NULL - buffer not found/ptr - buffer found
 */
exprivate buffer_obj_t * find_buffer_int(char *ptr)
{
    buffer_obj_t *ret=NULL;

    /*
    eltmp.buf = ptr;
    DL_SEARCH(G_buffers, ret, &eltmp, buf_ptr_cmp_fn);
    */
    
    if (NULL==ptr)
    {
        return &M_nullbuf;
    }
    
    EXHASH_FIND_PTR( ndrx_G_buffers, ((void **)&ptr), ret);
    
    return ret;
}

/**
 * Return the descriptor of typed buffer or NULL in case of error.
 * @param type - type to search for ( must be present )
 * @param subtype - may be NULL
 * @return NULL/or ptr to G_buf_descr[X]
 */
expublic typed_buffer_descr_t * ndrx_get_buffer_descr(char *type, char *subtype)
{
    typed_buffer_descr_t *p=G_buf_descr;
    typed_buffer_descr_t *ret = NULL;

    while (EXFAIL!=p->type_id)
    {
        if ((NULL!=p->type && 0==strcmp(p->type, type)) || 
                        (NULL!=p->alias && 0==strcmp(p->alias, type)) ||
                        p->type == type /*NULL buffer*/)
        {
            /* subtype is passed to the type engine.. */
            ret=p;
            break;
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
expublic char * ndrx_tpalloc (typed_buffer_descr_t *known_type,
                    char *type, char *subtype, long len)
{
    char *ret=NULL;
    typed_buffer_descr_t *usr_type = NULL;
    buffer_obj_t *node;
    
    NDRX_LOG(log_debug, "%s: type=%s, subtype=%s len=%d",  
            __func__, (NULL==type?"NULL":type),
            (NULL==subtype?"NULL":subtype), len);
    
    if (NULL==known_type)
    {
        if (NULL==(usr_type = ndrx_get_buffer_descr(type, subtype)))
        {
            ndrx_TPset_error_fmt(TPEOTYPE, "Unknown type (%s)/subtype(%s)", 
                    (NULL==type?"NULL":type), (NULL==subtype?"NULL":subtype));
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
    if (NULL==(ret=usr_type->pf_alloc(usr_type, subtype, &len)))
    {
        /* error detail should be already set */
        goto out;
    }

    /* now append the memory list with allocated block */
    if (NULL==(node=(buffer_obj_t *)NDRX_MALLOC(sizeof(buffer_obj_t))))
    {
        ndrx_TPset_error_fmt(TPEOS, "%s: Failed to allocate buffer list node: %s",  __func__,
                                        strerror(errno));
        ret=NULL;

        goto out;
    }

    /* Now append the list */
    memset(node, 0, sizeof(buffer_obj_t));

    node->buf = ret;
    NDRX_LOG(log_debug, "%s: type=%s subtype=%s len=%d allocated=%p", 
             __func__, usr_type->type,
            (NULL==subtype?"NULL":subtype),
            len, ret);
    node->size = len;
    
    node->type_id = usr_type->type_id;
    
    if (NULL==subtype)
    {
        node->subtype[0] = EXEOS;
    }
    else
    {
        NDRX_STRCPY_SAFE(node->subtype, subtype);
    }

    MUTEX_LOCK_V(M_lock);
    /* Save the allocated buffer in the list */
    /* DL_APPEND(G_buffers, node); */
    EXHASH_ADD_PTR(ndrx_G_buffers, buf, node);
    MUTEX_UNLOCK_V(M_lock);

out:
    
    return ret;
}

/**
 * Internal version of tprealloc
 * @param buf
 * @param
 * @return
 */
expublic char * ndrx_tprealloc (char *buf, long len)
{
    char *ret=NULL;
    buffer_obj_t * node;
    typed_buffer_descr_t *buf_type = NULL;

    NDRX_LOG(log_debug, "%s buf=%p, len=%ld",  __func__, buf, len);

    if (NULL==buf)
    {
        /* No realloc for NULL buffer */
        goto out_nolock;
    }
    
    if (NULL==(node=ndrx_find_buffer(buf)))
    {
        MUTEX_UNLOCK_V(M_lock);
         ndrx_TPset_error_fmt(TPEINVAL, "%s: Buffer %p is not know to system",  
                 __func__, buf);
        ret=NULL;
        goto out;
    }
    
    NDRX_LOG(log_debug, "%s buf=%p autoalloc=%hd", 
                         __func__, buf, node->autoalloc);

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
    MUTEX_LOCK_V(M_lock);
    EXHASH_DEL(ndrx_G_buffers, node);
    EXHASH_ADD_PTR(ndrx_G_buffers, buf, node);
    MUTEX_UNLOCK_V(M_lock);
    
    node->size = len;

out:
    
out_nolock:
    return ret;
    
}

/**
 * Remove the buffer
 * @param buf
 */
expublic void ndrx_tpfree (char *buf, buffer_obj_t *known_buffer)
{
    buffer_obj_t *elt;
    typed_buffer_descr_t *buf_type = NULL;
    tp_command_call_t * last_call;

    NDRX_LOG(log_debug, "_tpfree buf=%p", buf);

    if (NULL==buf)
    {
        /* nothing to do for NULLs */
        return;
    }
    
    /* Work out the buffer */
    if (NULL!=known_buffer)
        elt=known_buffer;
    else
    {
        elt=ndrx_find_buffer(buf);
    }

    if (NULL!=elt)
    {
        /* Check isn't it auto buffer, then reset that one too */
        last_call = ndrx_get_G_last_call();
        
        if (elt==last_call->autobuf)
        {
            /* kill the auto buf.. 
             * Thus user is left to manage the buffer by it self.
             */
            last_call->autobuf = NULL;
        }
             
        buf_type = &G_buf_descr[elt->type_id];
        /* Remove it! */
        buf_type->pf_free(buf_type, elt->buf);
        
        /* Remove that stuff from our linked list!! */
        /* DL_DELETE(G_buffers,elt); */
        MUTEX_LOCK_V(M_lock);
        EXHASH_DEL(ndrx_G_buffers, elt);
        MUTEX_UNLOCK_V(M_lock);
        
        /* delete elt by it self */
        NDRX_FREE(elt);
    }
    
}

/**
 * Check is buffer auto or not
 * @param buf ATMI allocated buffer
 * @return TRUE (1) if auto buffer, 0 manual buffer, -1 if error (unknown buffer)
 */
expublic int ndrx_tpisautobuf(char *buf)
{
    int ret;
    buffer_obj_t *elt;

    /* NULLs are Auto! */
    if (NULL==buf)
    {
        return EXTRUE;
    }

    elt=ndrx_find_buffer(buf);

    if (NULL!=elt)
    {
        ret = elt->autoalloc;
        NDRX_LOG(log_debug, "_tpisautobuf buf=%p, autoalloc=%d", buf, ret);
    }
    else
    {
        ndrx_TPset_error_msg(TPEINVAL, "ptr points to unknown buffer, "
            "not allocated by tpalloc()!");
        ret=EXFAIL;
    }
        
    return ret;
}

/**
 * Internal version of tptypes. Returns type info
 * @param ptr
 * @param type
 * @param subtype
 * @return 
 */
expublic long ndrx_tptypes (char *ptr, char *type, char *subtype)
{
    long ret=EXSUCCEED;
    
    typed_buffer_descr_t *buf_type = NULL;
    buffer_obj_t *buf;

    buf =  ndrx_find_buffer(ptr);
    
    if (NULL==buf)
    {
        ndrx_TPset_error_msg(TPEINVAL, "ptr points to unknown buffer, "
                "not allocated by tpalloc()!");
        ret=EXFAIL;
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
    
    if (NULL!=subtype && EXEOS!=buf->subtype[0])
    {
        strcpy(subtype, buf->subtype);
    }
    
out:
    
    return ret;
    
}
/* vim: set ts=4 sw=4 et smartindent: */
