/* 
** UBF library, filed table handling routins (i.e. .fd files)
** !!! THERE IS NO SUPPORT for multiple directories with in FLDTBLDIR !!!
** Also the usage of default `fld.tbl' is not supported, as seems to be un-needed
** feature.
** The emulator of UBF library
** Enduro Execution Library
**
** @file fieldtable.c
** 
** -----------------------------------------------------------------------------
** Enduro/X Middleware Platform for Distributed Transaction Processing
** Copyright (C) 2015, ATR Baltic, SIA. All Rights Reserved.
** This software is released under one of the following licenses:
** GPL or ATR Baltic's license for commercial use.
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
** A commercial use license is available from ATR Baltic, SIA
** contact@atrbaltic.com
** -----------------------------------------------------------------------------
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
#include <thlock.h>
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
UBF_field_def_t ** M_bfldidhash2 = NULL; /* FLD ID hash */
UBF_field_def_t ** M_fldnmhash2 = NULL; /* FLD NM hash */
int M_hash2_size = 16000; /* Default size for Hash2 */

/*---------------------------Prototypes---------------------------------*/
private void _bfldidhash_add(UBF_field_def_t *p_fld);
private void _fldnmhash_add(UBF_field_def_t *p_fld);
private int _ubf_load_fld_def(int base,
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
private int init_hash_area(void)
{
    int ret=SUCCEED;
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
        M_bfldidhash2 = malloc(malloc_size);
        if (NULL==M_bfldidhash2)
        {
            _Fset_error_fmt(BMALLOC, "Failed to malloc bfldidhash2, requested: %d bytes err: %s",
                        malloc_size, strerror(errno));
            ret=FAIL;
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
        M_fldnmhash2 = malloc(malloc_size);
        if (NULL==M_fldnmhash2)
        {
            _Fset_error_fmt(BMALLOC, "Failed to malloc fldnmhash2, requested: %d bytes err: %s",
                        malloc_size, strerror(errno));
            ret=FAIL;
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
private void _bfldidhash_add(UBF_field_def_t *p_fld)
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
private int UBF_field_def_id_cmp(UBF_field_def_t *a, UBF_field_def_t *b)
{
    return (a->bfldid==b->bfldid?SUCCEED:FAIL);
}

/**
 * Get field entry from int hash
 */
public UBF_field_def_t * _bfldidhash_get(BFLDID id)
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
private void _fldnmhash_add(UBF_field_def_t *p_fld)
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
private int UBF_field_def_nm_cmp(UBF_field_def_t *a, UBF_field_def_t *b)
{
    return strcmp(a->fldname, b->fldname);
}

/**
 * Get field name entry from hash table.
 * @returns - NULL not found or ptr to UBF_field_def_t
 */
public UBF_field_def_t * _fldnmhash_get(char *key)
{
    /* Get the linear array key */
    int hash_key = str_hash_from_key_fn(key) % M_hash2_size;
    UBF_field_def_t *ret=NULL;
    UBF_field_def_t tmp;
    strcpy(tmp.fldname, key);
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
private int _ubf_load_def_table(void)
{
    char *flddir=NULL;
    char *flds=NULL;
    char *p;
    FILE *fp;
    char tmp_flds[FILENAME_MAX+1];
    char tmp[FILENAME_MAX+1];

    int ret=SUCCEED;

    flddir = getenv(FLDTBLDIR);
    if (NULL==flddir)
    {
        _Fset_error_msg(BFTOPEN, "Field table directory not set - "
                                 "check FLDTBLDIR env var");
        ret=FAIL;
        goto out;
    }
    UBF_LOG(log_debug, "Load field dir [%s]", flddir);

    flds = getenv(FIELDTBLS);
    if (NULL==flds)
    {
        _Fset_error_msg(BFTOPEN, "Field table list not set - "
                 "check FIELDTBLS env var");
        ret=FAIL;
        goto out;
    }

    UBF_LOG(log_debug, "About to load fields list [%s]", flds);

    _ubf_loader_init();

    strcpy(tmp_flds, flds);
    p=strtok(tmp_flds, ",");
    while (NULL!=p && SUCCEED==ret)
    {
        sprintf(tmp, "%s/%s", flddir, p);
        /* Open field table file */
        if (NULL==(fp=fopen(tmp, "r")))
        {
            _Fset_error_fmt(BFTOPEN, "Failed to open %s with error: [%s]", tmp,
                                strerror(errno));
            ret=FAIL;
            goto out;
        }

        ret=_ubf_load_def_file(fp, NULL, NULL, NULL, tmp, FALSE);

        /* Close file */
        fclose(fp);
        p=strtok(NULL, ",");
    }

out:

	return ret;
}

/**
 * Initialized loader.
 * @return
 */
public int _ubf_loader_init(void)
{
    return init_hash_area(); /* << V2 */
}

/**
 * Load file defintion
 * @param fp
 * @return
 */
public int _ubf_load_def_file(FILE *fp, 
                int (*put_text_line) (char *text), /* callback for putting text line */
                int (*put_def_line) (UBF_field_def_t *def),  /* callback for writting defintion */
                int (*put_got_base_line) (char *base), /* callback for base line */
                char *fname,
                int check_dup)
{
    int ret=SUCCEED;
    int base=0;
    char tmp[FILENAME_MAX+1];
    char fldname[UBFFLDMAX+1];
    int line=0;
    
    /* Load into hash */
    while (SUCCEED==ret && NULL!=fgets(tmp, 1024, fp))
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
private int _ubf_load_fld_def(int base, 
                              char *fldinfo,
                              int (*put_def_line) (UBF_field_def_t *def),
                              int check_dup,
                              char *fname,
                              int line)
{
    int ret=SUCCEED;
    char ftype[32]={'\0'};
    UBF_field_def_t *fld, *fld2;
    UBF_field_def_t *reserved;
    dtype_str_t *p = G_dtype_str_map;
    _UBF_INT dtype;
    BFLDID number;

    fld = calloc(1, sizeof(UBF_field_def_t));
    fld2 = calloc(1, sizeof(UBF_field_def_t));

    if (NULL==fld || NULL==fld2)
    {
        _Fset_error_msg(BMALLOC, "Failed to allocate field def space!");
        ret=FAIL;
        goto out;
    }

    sscanf(fldinfo, "%s%d%s", fld->fldname, &(fld->bfldid), ftype);
    fld->bfldid+=base;
    /* Map type */
    while(EOS!=p->fldname[0])
    {
        if (0==strcmp(p->fldname, ftype))
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

    if (EOS==p->fldname[0])
    {
        _Fset_error_fmt(BFTSYNTAX, "Failed to find data type for [%s] in %s:%d!",
                                    ftype, fname, line);
        ret=FAIL;
    }
    else
    {
        /* check dup before adding! */
        if (check_dup)
        {
            if (NULL!=(reserved=_fldnmhash_get(fld->fldname)))
            {
                /* ERROR! ID Already defined! */
                _Fset_error_fmt(BFTSYNTAX, "Duplicate name [%s] already taken by "
                                "[%s]:%d %s:%d!",
                                fld->fldname, reserved->fldname, number,
                                fname, line);
                ret=FAIL;
            }

            if (SUCCEED==ret && NULL!=(reserved=_bfldidhash_get(fld->bfldid)))
            {
                /* ERROR! Name already taken */
                _Fset_error_fmt(BFTSYNTAX, "Duplicate ID [%s]:%d already taken by [%s]:%d "
                                    "%s:%d!",
                                 fld->fldname, number, reserved->fldname, number,
                                 fname, line);
                ret=FAIL;
            }
        }

        if (SUCCEED==ret)
        {
            _bfldidhash_add(fld);
            *fld2 = *fld;
            _fldnmhash_add(fld2);
        }
    }

    /*
     * Call callback for putting data line.
     */
    if (SUCCEED==ret && NULL!=put_def_line)
    {
        ret=put_def_line(fld);
    }
    
out:
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
static int str_keys_equal_fn ( void *key1, void *key2 )
{
    return (0==strcmp(key1, key2)?1:0);
}

/**
 * Checks and reads type table (.fd files)
 * @return SUCCEED/FAIL
 */
public int prepare_type_tables(void)
{
    MUTEX_LOCK;
    {
    int ret=SUCCEED;
#if FIELD_TABLE_NO_RETRY
    static int first=1;
    static int status;
    
    if (NULL==M_bfldidhash && first)
    {
        ret=_ubf_load_def_table();
        first=0;
        status=ret;
    }
    else
    {
        /* return old error code */
        ret=status;
    }
#else
    if (NULL==M_bfldidhash2 || NULL==M_fldnmhash2)
    {
        ret=_ubf_load_def_table();
    }
#endif

    MUTEX_UNLOCK;
    return ret;
    }
}

/**
 * Internal version of Bfname. Does not set error.
 */
public char * _Bfname_int (BFLDID bfldid)
{
    UBF_field_def_t *p_fld;
    static char buf[64];

    if (SUCCEED!=prepare_type_tables())
    {
        if (BFTOPEN==Berror || BFTSYNTAX==Berror)
        {
            _Bunset_error();
        }

        sprintf(buf, "((BFLDID32)%d)", bfldid);

        return buf;
    }

    /* Now try to find the data! */
    p_fld = _bfldidhash_get(bfldid);
    if (NULL==p_fld)
    {
        sprintf(buf, "((BFLDID32)%d)", bfldid);
        return buf;
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
private BFLDID get_from_bfldidstr(char *fldnm)
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
public BFLDID _Bfldid_int (char *fldnm)
{
    UBF_field_def_t *p_fld=NULL;
    BFLDID bfldid;

    if (SUCCEED!=prepare_type_tables())
    {
        /* extending support for BFLDID syntax for read. */
        if (0==strncmp(fldnm, "((BFLDID32)", 10))
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
    p_fld = _fldnmhash_get(fldnm);

    if (NULL!=p_fld)
    {
            return p_fld->bfldid;
    }
    else if (0==strncmp(fldnm, "((BFLDID32)", 10))
    {
        bfldid = get_from_bfldidstr(fldnm);
        return bfldid;
    }
    else
    {
        return BBADFLDID;
    }
}
