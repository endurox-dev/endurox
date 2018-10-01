/**
 * @brief Common config server
 *
 * @file cconfsv.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <utlist.h>

#include <ndebug.h>
#include <atmi.h>
#include <atmi_int.h>
#include <typed_buf.h>
#include <ndrstandard.h>
#include <ubf.h>
#include <Exfields.h>
#include <atmi_shm.h>

#include "cconfsv.h"
#include "userlog.h"
#include <cconfig.h>
#include <inicfg.h>
#include <nerror.h>
#include <ubfutil.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/

/**
 * Lookup config
 */
struct cconf_req
{
    char **resources; /* files to lookup, if NULL, then default Enduro/X */
    char *lookup_sections; /* section to get */
    
    char **sectionmask; /* list of section startings to return the values for */
    
    char cmd;
};
typedef struct cconf_req cconf_req_t;
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/
exprivate ndrx_inicfg_t *M_config = NULL;
/*---------------------------Prototypes---------------------------------*/

/**
 * Set the error response
 * @param p_ub
 * @param code
 * @param msg
 * @return 
 */
exprivate void set_error(UBFH *p_ub, short code, char *msg)
{
    Bchg(p_ub, EX_NERROR_CODE, 0, (char *)&code, 0);
    Bchg(p_ub, EX_NERROR_MSG, 0, (char *)msg, 0);
}

/**
 * Free data allocated by load_data();
 * @param req
 */
exprivate void free_data(cconf_req_t *req)
{
    int i;
    
    if (NULL!=req->resources)
    {
        i = 0;
        while (NULL!=req->resources[i])
        {
            NDRX_FREE(req->resources[i]);
            i++;
        }
        
        NDRX_FREE(req->resources);
    }
    
    i = 0;
    if (NULL!=req->sectionmask)
    {
        while (NULL!=req->sectionmask[i])
        {
            NDRX_FREE(req->sectionmask[i]);
            i++;
        }
        NDRX_FREE(req->sectionmask);
    }
    
    if (NULL!=req->lookup_sections)
    {
        NDRX_FREE(req->lookup_sections);
    }
}

/**
 * Load the request data
 * @param p_svc
 */
exprivate int load_data(UBFH *p_ub, cconf_req_t *req)
{
    int ret = EXSUCCEED;
    int i;
    int nr_resources = Boccur(p_ub, EX_CC_RESOURCE);
    int nr_section_masks = Boccur(p_ub, EX_CC_SECTIONMASK);
    char fn[] = "load_data";
    
    NDRX_LOG(log_debug, "load_data called, resources: %d", nr_resources);
    
    if (EXSUCCEED!=Bget(p_ub, EX_CC_CMD, 0, &(req->cmd), 0L))
    {
        req->cmd = NDRX_CCONFIG_CMD_GET;
    }
    
    if (nr_resources>0)
    {
        req->resources = NDRX_CALLOC(nr_resources+1, sizeof(char *));
        if (NULL==req->resources)
        {
            NDRX_LOG(log_error, "Failed to allocate resources: %s", strerror(errno));
            set_error(p_ub, NEMALLOC, "malloc failed for resources");
            EXFAIL_OUT(ret);
        }
        
        for (i=0; i<nr_resources; i++)
        {
            if (NULL==(req->resources[i] = Bgetalloc(p_ub, EX_CC_RESOURCE, i, NULL)))
            {
                NDRX_LOG(log_error, "Failed to get EX_CC_RESOURCE[%d]: %s", i, 
                        Bstrerror(Berror));
                set_error(p_ub, NESYSTEM, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        NDRX_LOG(log_debug, "Resources loaded ok...");    
    }
    
    if (nr_section_masks>0)
    {
        req->sectionmask = NDRX_CALLOC(nr_section_masks+1, sizeof(char *));
        if (NULL==req->sectionmask)
        {
            NDRX_LOG(log_error, "Failed to allocate section_masks: %s", strerror(errno));
            set_error(p_ub, NEMALLOC, "malloc failed for section_masks");
            EXFAIL_OUT(ret);
        }
        
        for (i=0; i<nr_section_masks; i++)
        {
            if (NULL==(req->sectionmask[i] = Bgetalloc(p_ub, EX_CC_SECTIONMASK, i, NULL)))
            {
                NDRX_LOG(log_error, "Failed to get EX_CC_SECTIONMASK[%d]: %s", i, 
                        Bstrerror(Berror));
                set_error(p_ub, NESYSTEM, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        NDRX_LOG(log_debug, "Section masks loaded ok...");    
    }
    
    /* Load EX_CC_SECTIONMASK if have */
    
    if (Bpres(p_ub, EX_CC_LOOKUPSECTION, 0))
    {
        if (NULL==(req->lookup_sections = Bgetalloc(p_ub, EX_CC_LOOKUPSECTION, 0, NULL)))
        {
            NDRX_LOG(log_error, "Failed to get EX_CC_LOOKUPSECTION: %s", 
                        Bstrerror(Berror));
            set_error(p_ub, NESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    NDRX_LOG(log_info, "Request data parsed ok");
    
    
out:

    NDRX_LOG(log_info, "%s returns  %d", fn, ret);

    return ret;
}

/**
 * Get the config.
 * @param p_ub
 * @param req
 * @return 
 */
exprivate int config_get(UBFH *p_ub, cconf_req_t *req)
{
    char msg[256];
    int ret = EXSUCCEED;
    int nr_mand = Boccur(p_ub, EX_CC_MANDATORY);
    int nr_formats1 = Boccur(p_ub, EX_CC_FORMAT_KEY);
    int nr_formats2 = Boccur(p_ub, EX_CC_FORMAT_FORMAT);
    int i;
    char *p = NULL;
    char *key = NULL;
    char *format = NULL;
    char *format2 = NULL;
    char *p2;
    char fmt_code;
    int min_len;
    int max_len;
    int len;
    float f;
    ndrx_inicfg_section_keyval_t *out = NULL, 
            *iter = NULL, 
            *iter_tmp = NULL, 
            *hash_key = NULL;
    
    if (nr_formats1!=nr_formats2)
    {
        NDRX_LOG(log_error, "Invalid format field count: "
                "EX_CC_FORMAT_FIELD = %d but EX_CC_FORMAT_FORMAT = %d",
                nr_formats1, nr_formats2);
        set_error(p_ub, NEINVAL, "Invalid EX_CC_FORMAT_FIELD, EX_CC_FORMAT_FORMAT "
                "count, must match.");
        EXFAIL_OUT(ret);
    }
    
    /* add any resources */
    if (EXSUCCEED!=ndrx_inicfg_get_subsect(M_config, 
        req->resources, req->lookup_sections, &out))
    {
        NDRX_LOG(log_error, "Failed to lookup for [%s]: %s", 
                req->lookup_sections, Nstrerror(Nerror));
        set_error(p_ub, NEINVAL, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    
    /* check mandatory fields */
    for (i=0; i<nr_mand; i++)
    {
        if (NULL!=p)
        {
            NDRX_FREE(p);
        }
        
        if (NULL!=(p = Bgetalloc(p_ub, EX_CC_MANDATORY, i, NULL)))
        {
            if (NULL==ndrx_keyval_hash_get(out, p))
            {
                snprintf(msg, sizeof(msg), "%s", p);
                set_error(p_ub, NEMANDATORY, msg);
                NDRX_LOG(log_error, "Mandatory key is missing: [%s] in config result", p);
                EXFAIL_OUT(ret);
            }
        }
        else
        {
            NDRX_LOG(log_error, "Bgetalloc error: %s", Nstrerror(Nerror));
            set_error(p_ub, NESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    
    /* check for formats */
    for (i=0; i<nr_formats1; i++)
    {
        if (NULL!=key)
        {
            NDRX_FREE(key);
        }
        
        if (NULL!=format)
        {
            NDRX_FREE(format);
        }
        
        if (NULL!=format2)
        {
            NDRX_FREE(format2);
        }
        
        min_len = 0;
        max_len = 0;
        
        if (NULL!=(key = Bgetalloc(p_ub, EX_CC_FORMAT_KEY, i, NULL)) &&
                NULL!=(format = Bgetalloc(p_ub, EX_CC_FORMAT_FORMAT, i, NULL)))
        {
            fmt_code = format[0];
            
            if (NULL==(format2 = strdup(format)))
            {
                set_error(p_ub, NEMALLOC, strerror(errno));
                NDRX_LOG(log_error, "Failed to strdup format");
                EXFAIL_OUT(ret);
            }
            
            if ('t'!=fmt_code)
            {
                /* search for dots */
                p2 = strchr(format, '.');
                
                /* ok there is not MIN */
                if (NULL==p2)
                {
                    snprintf(msg, sizeof(msg), "Invalid format code: [%s]", format2);
                    set_error(p_ub, NEINVAL, msg);
                    NDRX_LOG(log_error, "%s", msg);
                    EXFAIL_OUT(ret);
                } 
                else if (p2==(format+1))
                {
                    /* In this case next symbol is dot thus no length */
                    min_len = 0;
                }
                else
                {
                    /* have some len */
                    *p2 = EXEOS;
                    min_len = atoi(format+1);
                }
                p2++;
                
                /* get out the max */
                if (EXEOS==*p2)
                {
                    /* So the first dot was finished with EOS, it is invalid format */
                    snprintf(msg, sizeof(msg), "Invalid format code: [%s]", format2);
                    set_error(p_ub, NEINVAL, msg);
                    NDRX_LOG(log_error, "%s", msg);
                    EXFAIL_OUT(ret);
                } 
                else if (NULL==(p2=strrchr(p2, '.')))
                {
                    snprintf(msg, sizeof(msg), "Invalid format code: [%s], "
                            "missing second dot", format2);
                    set_error(p_ub, NEINVAL, msg);
                    NDRX_LOG(log_error, "%s", msg);
                    EXFAIL_OUT(ret);
                }
                
                p2++;
                
                if (EXEOS==*p2)
                {
                    snprintf(msg, sizeof(msg), "Invalid format code: [%s], "
                            "missing max len", format2);
                    set_error(p_ub, NEINVAL, msg);
                    NDRX_LOG(log_error, "%s", msg);
                    EXFAIL_OUT(ret);
                }
                
                max_len = atoi(p2);
                
                NDRX_LOG(log_debug, "constructed check: type [%c] min_len [%d] max_len [%d]",
                        fmt_code, min_len, max_len);
            }
            
            /* get the field it self */
            if (NULL==(hash_key=ndrx_keyval_hash_get(out, key)))
            {
                NDRX_LOG(log_debug, "cannot check format for key [%s] - field missing",
                            key);
                continue; /* mandatory checks will verify it for existance if needed */
            }
            
            switch (fmt_code)
            {
                case 't':
                    /* check for boolean */
                    if (!('Y' == hash_key->val[0] ||
                            'y' == hash_key->val[0] ||
                            'T' == hash_key->val[0] ||
                            't' == hash_key->val[0] ||
                            'N' == hash_key->val[0] ||
                            'n' == hash_key->val[0] ||
                            'F' == hash_key->val[0] ||
                            'f' == hash_key->val[0] ||
                            '1' == hash_key->val[0] ||
                            '0' == hash_key->val[0]))
                    {
                        /* invalid value... */
                        set_error(p_ub, NEFORMAT, hash_key->key);
                        
                        NDRX_LOG(log_error, "Format error for key: [%s] (section %s) [%s]", 
                                hash_key->key, hash_key->section, hash_key->val);
                        EXFAIL_OUT(ret);
                    }
                    break;
                    
                case 'i':
                    if (!ndrx_isint(hash_key->val))
                    {
                        /* invalid value... */
                        set_error(p_ub, NEFORMAT, hash_key->key);
                        
                        NDRX_LOG(log_error, "Invalid integer: [%s] (section %s) [%s]", 
                                hash_key->key, hash_key->section, hash_key->val);
                        EXFAIL_OUT(ret);
                    }
                case 'n':
                    /* do no break, we want length tests for all of the types... */
                    if(sscanf(hash_key->val, "%f", &f) == 0)  //It's a float.
                    {
                        set_error(p_ub, NEFORMAT, hash_key->key);
                        
                        NDRX_LOG(log_error, "Invalid floating point value: [%s]"
                                " (section %s) [%s]", 
                                hash_key->key, hash_key->section, hash_key->val);
                        EXFAIL_OUT(ret);
                    }
                case 's':
                    /* if is string, just check the len */
                    len = strlen(hash_key->val);
                    if (len < min_len || len>max_len)
                    {
                        set_error(p_ub, NEFORMAT, hash_key->key);
                        
                        NDRX_LOG(log_error, "Format error for key: [%s] (section %s) "
                                "len: %d min_len: %d max_len: %d", 
                                hash_key->key, hash_key->section, len,
                                min_len, max_len);
                        EXFAIL_OUT(ret);
                    }
                    break;
                    
                default:
                    /* unsupported check.. */
                    snprintf(msg, sizeof(msg), "Invalid check: [%c]", fmt_code);
                    set_error(p_ub, NEINVAL, msg);
                    NDRX_LOG(log_error, "%s", msg);
                    EXFAIL_OUT(ret);
                    break;
            }
        }
        else
        {
            NDRX_LOG(log_error, "Bgetalloc error: %s", Nstrerror(Nerror));
            set_error(p_ub, NESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
                
    }
    
    /* load stuff to buffer */ 
    EXHASH_ITER(hh, out, iter, iter_tmp)
    {
        if (EXSUCCEED!=Badd(p_ub, EX_CC_SECTION, iter->section, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CC_SECTION (section: %s): %s", 
                        iter->section, Nstrerror(Nerror));
            set_error(p_ub, NEINVAL, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Badd(p_ub, EX_CC_KEY, iter->key, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CC_KEY (key: %s): %s", 
                        iter->key, Nstrerror(Nerror));
            set_error(p_ub, NEINVAL, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
        
        if (EXSUCCEED!=Badd(p_ub, EX_CC_VALUE, iter->val, 0L))
        {
            NDRX_LOG(log_error, "Failed to set EX_CC_VALUE (val: %s): %s", 
                        iter->val, Nstrerror(Nerror));
            set_error(p_ub, NEINVAL, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
out:
    if (NULL!=p)
    {
        NDRX_FREE(p);
    }

    if (NULL!=out)
    {
        ndrx_keyval_hash_free(out);
    }

    if (NULL!=key)
    {
        NDRX_FREE(key);
    }

    if (NULL!=format)
    {
        NDRX_FREE(format);
    }

    if (NULL!=format2)
    {
        NDRX_FREE(format2);
    }

    return ret;
}

/**
 * list th config (this runs in conversational mode)
 * @param p_ub
 * @param req
 * @return 
 */
exprivate int config_list(UBFH *p_ub, cconf_req_t *req, int cd)
{
    int ret = EXSUCCEED;
    ndrx_inicfg_section_t *sections=NULL, *iter=NULL, *iter_tmp=NULL;
    ndrx_inicfg_section_keyval_t *key_iter = NULL, *key_iter_tmp = NULL;
    long revent;
    BFLDID rm_list[] =  {   
        EX_CC_SECTION, 
        EX_CC_KEY, 
        EX_CC_VALUE, 
        BBADFLDID
    };
    
    if (EXSUCCEED!=ndrx_inicfg_iterate(M_config, req->resources, 
            req->sectionmask, &sections))
    {
        NDRX_LOG(log_error, "Failed to iterate sections: %s", Nstrerror(Nerror));
        set_error(p_ub, NESYSTEM, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    EXHASH_ITER(hh, sections, iter, iter_tmp)
    {
        NDRX_LOG(log_info, "iter: section [%s]", iter->section);
        
        /* Remove all  EX_CC_SECTION/ EX_CC_KEY/ EX_CC_VALUE from buffer
         * so that we list fresh data for each section.
         */
        if (EXSUCCEED!=Bdelete(p_ub, rm_list))
        {
            if (Berror!=BNOTPRES)
            {
                NDRX_LOG(log_error, "Buffer cleanup failed: %s", Bstrerror(Berror));
                set_error(p_ub, NESYSTEM, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        
        EXHASH_ITER(hh, sections->values, key_iter, key_iter_tmp)
        {
            if (EXSUCCEED!=Badd(p_ub, EX_CC_SECTION, key_iter->section, 0L))
            {
                NDRX_LOG(log_error, "Failed to set EX_CC_SECTION (section: %s): %s", 
                            key_iter->section, Nstrerror(Nerror));
                set_error(p_ub, NEINVAL, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED!=Badd(p_ub, EX_CC_KEY, key_iter->key, 0L))
            {
                NDRX_LOG(log_error, "Failed to set EX_CC_KEY (key: %s): %s", 
                            key_iter->key, Nstrerror(Nerror));
                set_error(p_ub, NEINVAL, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }

            if (EXSUCCEED!=Badd(p_ub, EX_CC_VALUE, key_iter->val, 0L))
            {
                NDRX_LOG(log_error, "Failed to set EX_CC_VALUE (val: %s): %s", 
                            key_iter->val, Nstrerror(Nerror));
                set_error(p_ub, NEINVAL, Bstrerror(Berror));
                EXFAIL_OUT(ret);
            }
        }
        
        ndrx_debug_dump_UBF(log_debug, "CCONF listing section", p_ub);
        
        /* send the stuff way... */
        if (EXFAIL == tpsend(cd,
                    (char *)p_ub,
                    0L,
                    0,
                    &revent))
        {
            NDRX_LOG(log_error, "Send data failed [%s] %ld",
                                tpstrerror(tperrno), revent);
            set_error(p_ub, NESYSTEM, tpstrerror(tperrno));
                EXFAIL_OUT(ret);
                
        }
        else
        {
            NDRX_LOG(log_debug,"sent ok");
        }

    }
    
    /* Remove any response (so that we finish cleanly) */
    if (EXSUCCEED!=Bdelete(p_ub, rm_list))
    {
        if (Berror!=BNOTPRES)
        {
            NDRX_LOG(log_error, "Buffer cleanup failed: %s", Bstrerror(Berror));
            set_error(p_ub, NESYSTEM, Bstrerror(Berror));
            EXFAIL_OUT(ret);
        }
    }
    
    
    
out:
    return ret;
}

/**
 * Common config server
 * 
 * @param p_svc
 */
void CCONF (TPSVCINFO *p_svc)
{
    int ret=EXSUCCEED;
    UBFH *p_ub = (UBFH *)p_svc->data;
    cconf_req_t req;
    char tmp[256];
    int i;
    /* Common config command */
    
    /*
     * Allocate extra 32KB for config storage.
     */
    if (NULL==(p_ub = (UBFH *)tprealloc ((char *)p_ub, Bsizeof (p_ub) + 32000)))
    {
        userlog("buffer realloc failed!");
        NDRX_LOG(log_error, "tprealloc failed");
        EXFAIL_OUT(ret);
    }
    
    ndrx_debug_dump_UBF(log_debug, "CCONF request buffer", p_ub);
    
    
    memset(&req, 0, sizeof(req));
    
    if (EXSUCCEED!=load_data(p_ub, &req))
    {
        NDRX_LOG(log_error, "Request data failed to parse");
        EXFAIL_OUT(ret);
    }
    
    /* Add any resources */
    NDRX_LOG(log_debug, "Loading any resources (if new)");
    if (NULL!=req.resources && NULL!=req.resources[0])
    {
        i=0;
        while (NULL!=req.resources[i])
        {
            NDRX_LOG(log_debug, "Loading [%s]", req.resources[i]);
            /* any section please */
            if (EXSUCCEED!=ndrx_inicfg_add(M_config, req.resources[i], NULL))
            {
                NDRX_LOG(log_error, "Failed to add resource [%s]: %s", 
                        req.resources[i], Nstrerror(Nerror));
                set_error(p_ub, NEINVALINI, Nstrerror(Nerror));
                EXFAIL_OUT(ret);
            }
            i++;
        }
    }

    /* refresh config - all sections please */
    NDRX_LOG(log_debug, "Refreshing configs...");
    if (EXSUCCEED!=ndrx_inicfg_reload(M_config, NULL))
    {
        NDRX_LOG(log_error, "Refresh failed: %s", Nstrerror(Nerror));
        set_error(p_ub, NESYSTEM, Nstrerror(Nerror));
        
    }
    
    NDRX_LOG(log_info, "Ready for lookup");
    
    switch (req.cmd)
    {
        case NDRX_CCONFIG_CMD_GET:
            if (EXSUCCEED!=config_get(p_ub, &req))
            {
                NDRX_LOG(log_error, "config_get failed!");
                EXFAIL_OUT(ret);
            }
            break;
        case NDRX_CCONFIG_CMD_LIST:
            
            if (EXSUCCEED!=config_list(p_ub, &req, p_svc->cd))
            {
                NDRX_LOG(log_error, "config_list failed!");
                EXFAIL_OUT(ret);
            }
            break;
        default:
            NDRX_LOG(log_error, "Unsupported config command: %c", req.cmd);
            sprintf(tmp, "Unsupported config command: %c", req.cmd);
            set_error(p_ub, NEINVAL, tmp);
            break;
    }
    
out:
    free_data(&req);

    ndrx_debug_dump_UBF(log_debug, "CCONF response buffer", p_ub);

    tpreturn(  ret==EXSUCCEED?TPSUCCESS:TPFAIL,
                0,
                (char *)p_ub,
                0L,
                0L);
}

/*
 * Do initialization
 */
int NDRX_INTEGRA(tpsvrinit)(int argc, char **argv)
{
    int ret=EXSUCCEED;

    NDRX_LOG(log_debug, "tpsvrinit called");
    
    if (EXSUCCEED!=ndrx_cconfig_load_general(&M_config))
    {
        NDRX_LOG(log_error, "Failed to general config (Enduro/X config)!");
        EXFAIL_OUT(ret);
    }
    
    if (!M_config)
    {
        NDRX_LOG(log_error, "CCONFIG must be set in order to run cconfsrv");
        EXFAIL_OUT(ret);   
    }
    
    if (EXSUCCEED!=tpadvertise(NDRX_SVC_CCONF, CCONF))
    {
        NDRX_LOG(log_error, "Failed to initialize CCONF!");
        EXFAIL_OUT(ret);
    }
    
    
out:
    return ret;
}

/**
 * Finish off the processing
 */
void NDRX_INTEGRA(tpsvrdone) (void)
{
    if (NULL!=M_config)
    {
        ndrx_inicfg_free(M_config);
    }
}

/* vim: set ts=4 sw=4 et smartindent: */
