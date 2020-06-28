/**
 * @brief UBF library, filed table handling routins (i.e. .fd files)
 *   !!! THERE IS NO SUPPORT for multiple directories with in FLDTBLDIR !!!
 *   Also the usage of default `fld.tbl' is not supported, as seems to be un-needed
 *   feature.
 *   The emulator of UBF library
 *   Enduro Execution Library
 *
 * @file fieldtable.c
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

/*---------------------------Includes-----------------------------------*/
#include <string.h>
#include <ubf_int.h>
#include <stdlib.h>
#include <errno.h>

#include <fieldtable.h>
#include <fdatatype.h>
#include <ferror.h>
#include <utlist.h>

#include "ndebug.h"
#include "ubf_tls.h"
#include <thlock.h>
#include <ubfdb.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

/* Hash tables V2 
 * We have dynamically allocated linear space for UBF_field defs.
 * Field defs by it self are linked lists, if hashkey gets the same (by modulus).
 * To get the entry out the search in linked list is done from the data key,
 * this is because, we never know is the node correct or not, even if there is only
 * one element in the list.
 * 
 * The example of data structures:
 * 
 *  + [UBF DEF 1] -> [UBF DEF 2] ->..-> [UBF DEF X]
 *  |   + [UBF DEF 4] -> [UBF DEF 5] ->..-> [UBF DEF Y]
 *  |   |
 * [0]-[1]-[2]..-..[n] # Linear array for hash key modulus
 * 
 * This ensures that our hash tables can correctly get data out (no duplicates!!)
 */
exprivate  UBF_field_def_t ** M_bfldidhash2 = NULL; /* FLD ID hash */
exprivate  UBF_field_def_t ** M_fldnmhash2 = NULL; /* FLD NM hash */
exprivate  volatile int M_field_def_loaded = EXFALSE;       /* IS UBF loaded? */
exprivate  int M_hash2_size = 16000; /* Default size for Hash2 */

/*---------------------------Prototypes---------------------------------*/
exprivate void _bfldidhash_add(UBF_field_def_t *p_fld);
exprivate void _fldnmexhash_add(UBF_field_def_t *p_fld);
exprivate int _ubf_load_fld_def(int base,
                              char *fldinfo,
                              int (*put_def_line) (UBF_field_def_t *def),
                              int check_dup,
                              char *fname,
                              int line);

static unsigned int str_hash_from_key_fn( void *k );
static int str_keys_equal_fn ( void *key1, void *key2 );


/**
 * Hash 2 structure init
 * @return SUCCEED/FAIL
 */
exprivate int init_hash_area(void)
{
    int ret=EXSUCCEED;
    char *p;
    int malloc_size = sizeof(ft_node_t *)*M_hash2_size;
    int i;
    static int first = 1;
    UBF_field_def_t *elt, *tmp, *head;

    /* Init the table memory */
    if (first)
    {
        if (NULL!=(p=getenv("NDRX_UBFMAXFLDS")))
        {
            M_hash2_size = atoi(p);
            malloc_size = sizeof(ft_node_t *)*M_hash2_size;
        }
        UBF_LOG(log_debug, "Using NDRX_UBFMAXFLDS: %d", M_hash2_size);
        first = 0;
    }

    if (NULL==M_bfldidhash2)
    {
        M_bfldidhash2 = NDRX_MALLOC(malloc_size);
        if (NULL==M_bfldidhash2)
        {
            ndrx_Bset_error_fmt(BMALLOC, "Failed to malloc bfldidhash2, requested: %d bytes err: %s",
                        malloc_size, strerror(errno));
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        /* Free up data if any, this could make init slower
         * but normally after first fail, it should not retry the op anyway
         */
        for (i=0; i<M_hash2_size; i++)
        {
            if (NULL!=M_bfldidhash2[i])
            {
                head = M_bfldidhash2[i];
                LL_FOREACH_SAFE(head,elt,tmp)
                {
                    LL_DELETE(head, elt);
                }
            } /* if */
        } /* for */
    }

    if (NULL==M_fldnmhash2)
    {
        M_fldnmhash2 = NDRX_MALLOC(malloc_size);
        if (NULL==M_fldnmhash2)
        {
            ndrx_Bset_error_fmt(BMALLOC, "Failed to malloc fldnmhash2, requested: %d bytes err: %s",
                        malloc_size, strerror(errno));
            ret=EXFAIL;
            goto out;
        }
    }
    else
    {
        /* Free up data if any, this could make init slower
         * but normally after first fail, it should not retry the op anyway
         */
        for (i=0; i<M_hash2_size; i++)
        {
            if (NULL!=M_fldnmhash2[i])
            {
                head = M_fldnmhash2[i];
                LL_FOREACH_SAFE(head,elt,tmp)
                {
                    LL_DELETE(head, elt);
                }
            } /* if */
        } /* for */
    }
    
    /* clean up space */
    memset(M_bfldidhash2, 0, malloc_size);
    /* clean up space */
    memset(M_fldnmhash2, 0, malloc_size);
    
out:
    return ret;
}

/**
 * Add Field ID Hash record
 */
exprivate void _bfldidhash_add(UBF_field_def_t *p_fld)
{
    int hash_key = p_fld->bfldid % M_hash2_size; /* Simple mod based hash */
    
    LL_APPEND(M_bfldidhash2[hash_key], p_fld);
}

/**
 * Compares two field defs and returns equality result
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
exprivate int UBF_field_def_id_cmp(UBF_field_def_t *a, UBF_field_def_t *b)
{
    return (a->bfldid==b->bfldid?EXSUCCEED:EXFAIL);
}

/**
 * Get field entry from int hash
 */
expublic UBF_field_def_t * _bfldidhash_get(BFLDID id)
{
    /* Get the linear array key */
    int hash_key = id % M_hash2_size; /* Simple mod based hash */
    UBF_field_def_t *ret=NULL;
    UBF_field_def_t tmp;
    
    tmp.bfldid=id;
    LL_SEARCH(M_bfldidhash2[hash_key],ret,&tmp,UBF_field_def_id_cmp);
    
    return ret;
}

/**
 * Add Field Name Hash record
 */
exprivate void _fldnmexhash_add(UBF_field_def_t *p_fld)
{
    /* Get the linear array key */
    int hash_key = str_hash_from_key_fn(p_fld->fldname) % M_hash2_size;
    LL_APPEND(M_fldnmhash2[hash_key], p_fld);

#ifdef EXTRA_FT_DEBUG
    printf("field [%s] key [%d] result [0%x] - [0%x] added\n", p_fld->fldname, hash_key, p_fld, M_fldnmhash2[hash_key]);
#endif
}


/**
 * Compare two fields by using name
 * @param a
 * @param b
 * @return 0 - equal/ -1 - not equal
 */
exprivate int UBF_field_def_nm_cmp(UBF_field_def_t *a, UBF_field_def_t *b)
{
    return strcmp(a->fldname, b->fldname);
}

/**
 * Get field name entry from hash table.
 * @returns - NULL not found or ptr to UBF_field_def_t
 */
expublic UBF_field_def_t * ndrx_fldnmhash_get(char *key)
{
    /* Get the linear array key */
    int hash_key = str_hash_from_key_fn(key) % M_hash2_size;
    UBF_field_def_t *ret=NULL;
    UBF_field_def_t tmp;
    NDRX_STRCPY_SAFE(tmp.fldname, key);
    LL_SEARCH(M_fldnmhash2[hash_key],ret,&tmp,UBF_field_def_nm_cmp);

#ifdef EXTRA_FT_DEBUG
    printf("field [%s] key [%d] result [0%x] - [0%x]\n", key, hash_key, ret, M_bfldidhash2[hash_key]);
#endif
    return ret;
}

/**
 * Initialize bisubf buffer
 *
 * Probably we need to build two hashes, one for BFLDID as key
 * And second hash we use field name as key
 *
 */
exprivate int _ubf_load_def_table(void)
{
    char *flddir=NULL;
    char *flds=NULL;
    char *p, *pd;
    char *p_flds, *p_flddir;
    FILE *fp;
    char tmp_flds[FILENAME_MAX+1];
    char tmp_flddir[FILENAME_MAX+1];
    char tmp[FILENAME_MAX+1];
    int exist_fld;

    int ret=EXSUCCEED;

    flddir = getenv(CONF_FLDTBLDIR);
    if (NULL==flddir)
    {
        ndrx_Bset_error_msg(BFTOPEN, "Field table directory not set - "
                                 "check FLDTBLDIR env var");
        ret=EXFAIL;
        goto out;
    }
    UBF_LOG(log_debug, 
            "Load field dir [%s] (multiple directories supported)", flddir);

    flds = getenv(CONF_FIELDTBLS);
    if (NULL==flds)
    {
        ndrx_Bset_error_msg(BFTOPEN, "Field table list not set - "
                 "check FIELDTBLS env var");
        ret=EXFAIL;
        goto out;
    }

    UBF_LOG(log_debug, "About to load fields list [%s]", flds);

    _ubf_loader_init();

    NDRX_STRCPY_SAFE(tmp_flds, flds);
    p=strtok_r(tmp_flds, ",", &p_flds);
    while (NULL!=p)
    {
        exist_fld=EXFALSE;
        NDRX_STRCPY_SAFE(tmp_flddir, flddir);
        pd=strtok_r(tmp_flddir, ":", &p_flddir);
        while ( NULL!=pd && EXFALSE==exist_fld)
        {
            snprintf(tmp, sizeof(tmp), "%s/%s", pd, p);
            UBF_LOG(log_debug, "Open field table file [%s]", tmp);
            
            /* Open field table file */
            if (NULL==(fp=NDRX_FOPEN(tmp, "r")))
            {
                UBF_LOG(log_debug, "Failed to open %s with error: [%s]", 
                        tmp, strerror(errno));
            }
            else
            {
                ret=ndrx_ubf_load_def_file(fp, NULL, NULL, NULL, tmp, EXFALSE);
                exist_fld=EXTRUE;
                NDRX_FCLOSE(fp);
            }
            pd=strtok_r(NULL, ":", &p_flddir);
        }

        if ( EXFALSE==exist_fld )
        {
            ndrx_Bset_error_fmt(BFTOPEN, "Failed to open %s in [%s]", p, flddir);
            ret=EXFAIL;
            goto out;
        }
        /* Close file */
        p=strtok_r(NULL, ",", &p_flds);
    }
    
    /* Load FIeld database... if have one! */

    M_field_def_loaded = EXTRUE;

out:

    return ret;
}

/**
 * Initialized loader.
 * @return
 */
expublic int _ubf_loader_init(void)
{
    return init_hash_area(); /* << V2 */
}

/**
 * Load file defintion
 * @param fp
 * @return
 */
expublic int ndrx_ubf_load_def_file(FILE *fp, 
                int (*put_text_line) (char *text), /* callback for putting text line */
                int (*put_def_line) (UBF_field_def_t *def),  /* callback for writting defintion */
                int (*put_got_base_line) (char *base), /* callback for base line */
                char *fname,
                int check_dup)
{
    int ret=EXSUCCEED;
    int base=0;
    char tmp[FILENAME_MAX+1];
    char fldname[UBFFLDMAX+1];
    int line=0;
    
    /* Load into hash */
    while (EXSUCCEED==ret && NULL!=fgets(tmp, 1024, fp))
    {
        line++;
        
        UBF_LOG(log_dump, "Loading debug line [%s]", tmp);
        /* Parse out the line & load into hash */
        switch (tmp[0])
        {
            case '$':
                /* Callback for putting text line line */
                if (NULL!=put_text_line)
                    ret=put_text_line(tmp+1);

            case '#':
            case '\n':
                /* Do nothing */
                break;
            case '*':
                /* Load base */
                sscanf(tmp, "%s%d", fldname, &base);
                if (0!=strcmp("*base", fldname))
                {
                    base=0;
                }
                
                /* callback for baseline! */
                if (NULL!=put_got_base_line)
                    ret=put_got_base_line(tmp);

                break;
            default:
                ret=_ubf_load_fld_def(base, tmp, put_def_line, check_dup, 
                                        fname, line);
            break;
        }
    }
    return ret;
}

/**
 * Parse field info table & load into hashes.
 */
exprivate int _ubf_load_fld_def(int base, 
                              char *fldinfo,
                              int (*put_def_line) (UBF_field_def_t *def),
                              int check_dup,
                              char *fname,
                              int line)
{
    int ret=EXSUCCEED;
    char ftype[NDRX_UBF_TYPE_LEN+1]={'\0'};
    UBF_field_def_t *fld, *fld2;
    UBF_field_def_t *reserved;
    dtype_str_t *p = G_dtype_str_map;
    _UBF_INT dtype;
    BFLDID number;

    fld = NDRX_CALLOC(1, sizeof(UBF_field_def_t));
    fld2 = NDRX_CALLOC(1, sizeof(UBF_field_def_t));

    if (NULL==fld || NULL==fld2)
    {
        ndrx_Bset_error_msg(BMALLOC, "Failed to allocate field def space!");
        ret=EXFAIL;
        goto out;
    }

    sscanf(fldinfo, "%s%d%s", fld->fldname, &(fld->bfldid), ftype);
    fld->bfldid+=base;
    /* Map type */
    while(EXEOS!=p->fldname[0])
    {
        if (0==strcmp(p->fldname, ftype) || p->altname && 0==strcmp(p->altname, ftype))
        {
            fld->fldtype = p->fld_type;
            dtype = p->fld_type;
            break;
        }
        p++;
    }
    dtype<<=EFFECTIVE_BITS;

    /* Add leading type bits... */
    number = fld->bfldid; /* keep the number value */
    fld->bfldid |= dtype;

    UBF_LOG(log_dump, "Adding [%s] - id [%d] - [%s]",
                            fld->fldname, fld->bfldid, fldinfo);

    if (EXEOS==p->fldname[0])
    {
        ndrx_Bset_error_fmt(BFTSYNTAX, "Failed to find data type for [%s] in %s:%d!",
                                    ftype, fname, line);
        ret=EXFAIL;
    }
    else
    {
        /* check dup before adding! */
        if (check_dup)
        {
            if (NULL!=(reserved=ndrx_fldnmhash_get(fld->fldname)))
            {
                /* ERROR! ID Already defined! */
                ndrx_Bset_error_fmt(BFTSYNTAX, "Duplicate name [%s] already taken by "
                                "[%s]:%d %s:%d!",
                                fld->fldname, reserved->fldname, number,
                                fname, line);
                ret=EXFAIL;
            }

            if (EXSUCCEED==ret && NULL!=(reserved=_bfldidhash_get(fld->bfldid)))
            {
                /* ERROR! Name already taken */
                ndrx_Bset_error_fmt(BFTSYNTAX, "Duplicate ID [%s]:%d already taken by [%s]:%d "
                                    "%s:%d!",
                                 fld->fldname, number, reserved->fldname, number,
                                 fname, line);
                ret=EXFAIL;
            }
        }

        if (EXSUCCEED==ret)
        {
            _bfldidhash_add(fld);
            *fld2 = *fld;
            _fldnmexhash_add(fld2);
        }
    }

    /*
     * Call callback for putting data line.
     */
    if (EXSUCCEED==ret && NULL!=put_def_line)
    {
        ret=put_def_line(fld);
    }
    
out:
    if (EXSUCCEED!=ret)
    {
        if (NULL!=fld)
        {
            NDRX_FREE(fld);
        }

        if (NULL!=fld2)
        {
            NDRX_FREE(fld2);
        }

    }
    return ret;
}

/**
 * Make key out of string. Gen
 */
static unsigned int str_hash_from_key_fn( void *k )
{
    unsigned int hash = 5381;
    int c;
    char *str = (char *)k;

    while ((c = (int)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Check that string keys are qual
 */
static int str_keys_equal_fn( void *key1, void *key2 )
{
    return (0==strcmp(key1, key2)?1:0);
}

/**
 * Checks and reads type table (.fd files)
 * @return SUCCEED/FAIL
 */
expublic int ndrx_prepare_type_tables(void)
{
    /* If not loaded. */
    if (!M_field_def_loaded)
    {
        /* Lock */
        MUTEX_LOCK;
        {
            int ret=EXSUCCEED;

            /* if still not loaded by concurrent thread, then load */
            if (!M_field_def_loaded)
            {
                ret=_ubf_load_def_table();
            }
            /* Feature #295 */
            if (EXSUCCEED==ret)
            {
                /* try to load UBF DB */
                if (EXFAIL==ndrx_ubfdb_Bflddbload())
                {
                    ret=EXFAIL;
                }
            }

            MUTEX_UNLOCK;
            return ret;
        }
    }
    else
    {
        return EXSUCCEED;
    }
}

/**
 * Internal version of Bfname. Does not set error.
 */
expublic char * ndrx_Bfname_int (BFLDID bfldid)
{
    UBF_field_def_t *p_fld;
    UBF_TLS_ENTRY;

    if (EXSUCCEED!=ndrx_prepare_type_tables())
    {
        if (BFTOPEN==Berror || BFTSYNTAX==Berror)
        {
            ndrx_Bunset_error();
        }

        snprintf(G_ubf_tls->bfname_buf, sizeof(G_ubf_tls->bfname_buf), 
                "((BFLDID32)%d)", bfldid);

        return G_ubf_tls->bfname_buf;
    }

    /* Now try to find the data! */
    p_fld = _bfldidhash_get(bfldid);
    
    if (NULL==p_fld && NULL!=ndrx_G_ubf_db)
    {
        char *p;
        p = ndrx_ubfdb_Bflddbname(bfldid);
        if (NULL==p)
        {
            if (BNOTFLD==Berror)
            {
                ndrx_Bunset_error();
            }
        }
        else
        {
            return p; /* return ptr to field */
        }
    }
    
    if (NULL==p_fld)
    {
        snprintf(G_ubf_tls->bfname_buf, sizeof(G_ubf_tls->bfname_buf),
                "((BFLDID32)%d)", bfldid);
        return G_ubf_tls->bfname_buf;
    }
    else
    {
        return p_fld->fldname;
    }
}

/**
 * Parse bfldid out from ((BFLDID32)%d) synatx
 * @param fldnm
 * @return BADFLD in failure/BFLDID
 */
exprivate BFLDID get_from_bfldidstr(char *fldnm)
{
    BFLDID ret;
    sscanf(fldnm, "((BFLDID32)%d)", &ret);
    UBF_LOG(log_warn, "Parsed ((BFLDID32)%%d) from [%s] got id %d",
                                    fldnm, ret);
    return ret;
}

/*
 * Return BFLDID from name! (internal version, parses ((BFLDID)%d)
 * Does not reports errors.
 */
expublic BFLDID ndrx_Bfldid_int (char *fldnm)
{
    UBF_field_def_t *p_fld=NULL;
    BFLDID bfldid;

    if (EXSUCCEED!=ndrx_prepare_type_tables())
    {
        /* extending support for BFLDID syntax for read. */
        if (0==strncmp(fldnm, "((BFLDID32)", 11))
        {
            bfldid = get_from_bfldidstr(fldnm);
            return bfldid;
        }
        else
        {
            return BBADFLDID;
        }
    }
    
    /* Now we can try to do lookup */
    p_fld = ndrx_fldnmhash_get(fldnm);
    
    if (NULL==p_fld && NULL!=ndrx_G_ubf_db)
    {
        if (BBADFLDID==(bfldid = ndrx_ubfdb_Bflddbid(fldnm)))
        {
            if (BNOTFLD==Berror)
            {
                ndrx_Bunset_error();
            }
        }
        else
        {
            return bfldid;
        }
    }

    if (NULL!=p_fld)
    {
        return p_fld->bfldid;
    }
    else if (0==strncmp(fldnm, "((BFLDID32)", 11))
    {
        bfldid = get_from_bfldidstr(fldnm);
        return bfldid;
    }
    else
    {
        return BBADFLDID;
    }
}
/* vim: set ts=4 sw=4 et smartindent: */
