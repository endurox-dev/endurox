/* 
** ATMI API functions
**
** @file atmi.c
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <atmi.h>

#include <ndrstandard.h>
#include <atmi_int.h>
#include <ndebug.h>
#include <Exfields.h>
#include <xa_cmn.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define API_ENTRY {_TPunset_error(); \
    if (!G_atmi_is_init) { \
        /* this means this is dirty client call, do the init */\
        NDRX_DBG_INIT(("ATMI", ""));\
        entry_status=tpinit(NULL);\
    }\
}\
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
__thread tp_command_call_t G_last_call;
/*---------------------------Statics------------------------------------*/
/*---------------------------Prototypes---------------------------------*/

/**
 * API entry for tpacall
 * @param svc
 * @param data
 * @param len
 * @param flags
 * @return
 */
public int	tpacall (char *svc, char *data, long len, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    /*flags|=TPNOREPLY;  force that we do not wait for answer! - not needed here really!
     causes problems with serice async replies!, See doc for tpacall! */
            
    ret=_tpacall(svc, data, len, flags, NULL, FAIL, 0, NULL); /* no reply queue */
    
out:
    return ret;
}

/**
 * Extended version of tpacall, allow extradata + event posting.
 * @param svc
 * @param data
 * @param len
 * @param flags
 * @return
 */
public int	tpacallex (char *svc, char *data, 
        long len, long flags, char *extradata, int dest_node, int ex_flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    /*flags|=TPNOREPLY;  force that we do not wait for answer! - not needed here really!
     causes problems with serice async replies!, See doc for tpacall! */
            
    ret=_tpacall(svc, data, len, flags, extradata, dest_node, ex_flags, NULL); /* no reply queue */
    
out:
    return ret;
}

/**
 * API entry for tpalloc
 * @param type
 * @param subtype
 * @param len
 * @return
 */
public char	* tpalloc (char *type, char *subtype, long len)
{
    char *ret=NULL;
    int entry_status=SUCCEED;
    
/* Allow to skip initalization - this for for clt init (using tpalloc for buffer request)
    API_ENTRY;
    if (SUCCEED!=entry_status)
    {
        ret=NULL;
        goto out;
    }
 */   
    ret=_tpalloc(NULL, type, subtype, len);

out:
    return ret;
}

/**
 * API entry for tprealloc
 * @param type
 * @param subtype
 * @param len
 * @return
 */
public char	* tprealloc (char *buf, long len)
{
    char * ret=NULL;
    int entry_status=SUCCEED;
    
    API_ENTRY;
    if (SUCCEED!=entry_status)
    {
        ret=NULL;
        goto out;
    }
    
    ret=_tprealloc(buf, len);
    
out:
    return ret;
}

/**
 * API version of tpcall
 * Extended with new flag: TPTRANSUSPEND - suspend global TX in progress.
 *          Usable only in sync mode.
 * @param svc
 * @param idata
 * @param ilen
 * @param odata
 * @param olen
 * @param flags
 * @return SUCCEED/FAIL
 */
public int tpcall (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    TPTRANID tranid;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    /* Check some other parameters */
    if (olen==NULL)
    {
        _TPset_error_msg(TPEINVAL, "olen cannot be null");
        ret=FAIL;
        goto out;
    }
    
    /* Check some other parameters */
    if (odata==NULL)
    {
        _TPset_error_msg(TPEINVAL, "odata cannot be null");
        ret=FAIL;
        goto out;
    }
        
    ret=_tpcall (svc, idata, ilen, odata, olen, flags, NULL, NULL, 0);
    
out:
    return ret;
}

/**
 * API version of tpgetrply
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @return SUCCEED/FAIL
 */
public int tpgetrply (int *cd, char **data, long *len, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    if (cd==NULL)
    {
        _TPset_error_msg(TPEINVAL, "cd cannot be null");
        ret=FAIL;
        goto out;
    }

    if (data==NULL)
    {
        _TPset_error_msg(TPEINVAL, "data cannot be null");
        ret=FAIL;
        goto out;
    }

    if (len==NULL)
    {
        _TPset_error_msg(TPEINVAL, "len cannot be null");
        ret=FAIL;
        goto out;
    }

    if (flags & TPGETANY)
        ret=_tpgetrply (cd, FAIL, data, len, flags);
    else if (*cd <= 0 )
    {
        _TPset_error_msg(TPEINVAL, "*cd <= 0");
        ret=FAIL;
        goto out;
    }
    else
        ret=_tpgetrply (cd, *cd, data, len, flags);
        
      

out:
    return ret;
}

/**
 * API version of tpcall
 * @param svc
 * @param idata
 * @param ilen
 * @param odata
 * @param olen
 * @param flags
 * @param extradata - extra data to be passed over the call
 * @return SUCCEED/FAIL
 */
public int tpcallex (char *svc, char *idata, long ilen,
                char * *odata, long *olen, long flags,
                char *extradata, int dest_node, int ex_flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpcall (svc, idata, ilen, odata, olen, flags, extradata, dest_node, ex_flags);

out:
    return ret;
}

/**
 * Distributed transaction abort.
 * @return SUCCED/FAIL
 */
public int tpabort (long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;
    
    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpabort(flags);
    
out:
    return ret;
}

/**
 * Distributed transaction begin
 * @return SUCCEED/FAIL
 */
public int tpbegin (unsigned long timeout, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;
    
    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpbegin(timeout, flags);
    
out:
    return ret;
}

/**
 * Distro transaction commit
 * @return SUCCED/FAIL
 */
public int tpcommit (long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;
    
    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpcommit(flags);
    
out:
    return ret;
}

/**
 * Open XA interface
 * @return SUCCEED/FAIL
 */
public int	tpopen (void)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;
    
    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpopen();
    
out:
    return ret;
}


/**
 * Close XA interface
 * @return
 */
public int	tpclose (void)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;
    
    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpclose();
    
out:
    return ret;
}

/**
 * Return the current status in global tx or not
 * @return	0 - not int tx, 1 - in transaction
 */
public int	tpgetlev (void)
{
    _TPunset_error();

    if (G_atmi_xa_curtx.txinfo)
    {
        return 1;
    }

    return 0;
}


/**
 * API entry for tpcancel
 * @param cd
 * @return SUCCEED/FAIL
 */
public int	tpcancel (int cd)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpcancel (cd);

out:
    return ret;
}


/**
 * tpfree implementation
 * @param buf
 */
public void	tpfree (char *buf)
{
    _TPunset_error();

    if (NULL!=buf)
    {
        _tpfree(buf, NULL);
    }
    else
    {
        NDRX_LOG(log_warn, "Trying to tpfree NULL buffer!");
    }
}

/**
 * tpterm API entry.
 * We do not do standard initailization.
 * Intentionally we do not do API init here, because we do not wan't that
 * queue gets open for nothing (if this is first API call?)
 * Only not sure how about debug?
 * @return SUCCEED/FAIL
 */
public int	tpterm (void)
{
    _TPunset_error();
    return _tpterm();
}

/**
 * API version of tpconnect
 * @param svc
 * @param data
 * @param len
 * @param flags
 * @return
 */
public int	tpconnect (char *svc, char *data, long len, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpconnect (svc, data, len, flags);

out:
    return ret;
}

/**
 * API version of tprecv
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @param command_id
 * @return
 */
public int	tprecv (int cd, char * *data,
                        long *len, long flags, long *revent)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    short command_id=ATMI_COMMAND_CONVDATA;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tprecv (cd, data, len, flags, revent, &command_id);

out:
    return ret;
}

/**
 * API version of tpsend
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @return
 */
public int	tpsend (int cd, char *data, long len, long flags,
                                    long *revent)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpsend (cd, data, len, flags, revent, ATMI_COMMAND_CONVDATA);

out:
    return ret;
}

/**
 * API version of tpdiscon
 * @param cd
 * @param data
 * @param len
 * @param flags
 * @param revent
 * @return
 */
public int	tpdiscon (int cd)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpdiscon (cd);

out:
    return ret;
}

/**
 * TODO
 */
public void	tpsvrdone (void)
{
    NDRX_LOG(log_error, "tpsvrdone - ATMI dummy");
}

#if 0
/**
 * TODO
 * @param
 * @param
 * @return
 */
public int	tpsvrinit (int argc, char **argv)
{
    NDRX_LOG(log_error, "tpsvrinit - ATMI dummy");
    return SUCCEED;
}
#endif

/**
 * API version of tppost
 * @param eventname
 * @param data
 * @param len
 * @param flags
 * @return
 */
int tppost(char *eventname, char *data, long len, long flags)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tppost(eventname, data, len, flags);

out:
    return ret;
}

/**
 * API entry for tpsubscribe
 */
long tpsubscribe(char *eventexpr, char *filter, TPEVCTL *ctl, long flags)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpsubscribe(eventexpr, filter, ctl, flags);

out:
    return ret;
}

/**
 * API version of tpunsubscribe
 */
int tpunsubscribe(long subscription, long flags)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }

    ret=_tpunsubscribe(subscription, flags);

out:
    return ret;
}


/**
 * Not supported, Just for build compliance.
 * @param 
 * @param 
 * @param 
 * @return 
 */
public int tpconvert (char *strrep, char *binrep, long flags)
{
    NDRX_LOG(log_error, "tpconvert - ATMI dummy");
    
    return SUCCEED;
}

/**
 * Suspend global XA transaction
 * @param tranid
 * @param flags
 * @return SUCCEED/FAIL
 */
public int tpsuspend (TPTRANID *tranid, long flags) 
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret = _tpsuspend (tranid, flags);
    
out:
    return ret;
}

/**
 * Resume global XA transaction
 * @param tranid
 * @param flags
 * @return SUCCEED/FAIL
 */
public int tpresume (TPTRANID *tranid, long flags)
{
    int ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret = _tpresume (tranid, flags);
    
out:
    return ret;
}

/**
 * Return type information to caller.
 * The buffer must be allocated by tpalloc.
 * @param ptr
 * @param type
 * @param subtype
 * @return 
 */
public long tptypes (char *ptr, char *type, char *subtype)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    if (ptr==NULL)
    {
        _TPset_error_msg(TPEINVAL, "ptr cannot be null");
        ret=FAIL;
        goto out;
    }

    ret=_tptypes(ptr, type, subtype);

out:
    return ret;
}


/**
 * Return current node id
 * @return FAIL = error, >0 node id.
 */
public long tpgetnodeid(void)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret = G_atmi_env.our_nodeid;
    
out:
    return ret;
}

/**
 * Convert JSON to UBF
 * @param p_ub  output UBF
 * @param buffer    input buffer
 * @return 
 */
public int tpjsontoubf(UBFH *p_ub, char *buffer)
{
    return _tpjsontoubf(p_ub, buffer);
}

/**
 * Convert UBF to JSON text
 * @param p_ub
 * @param buffer
 * @param bufsize
 * @return 
 */
public int tpubftojson(UBFH *p_ub, char *buffer, int bufsize)
{
    return _tpubftojson(p_ub, buffer, bufsize);
}

/**
 * Enqueue transaction - STUB
 */
public int tpenqueue (char *qspace, char *qname, TPQCTL *ctl, char *data, long len, long flags)
{   
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpenqueue (qspace, qname, ctl, data, len, flags);

out:
    return ret;

}

/**
 * Dequeue transaction - STUB
 */
public int tpdequeue (char *qspace, char *qname, TPQCTL *ctl, char **data, long *len, long flags)
{
    long ret=SUCCEED;
    int entry_status=SUCCEED;
    API_ENTRY;

    if (SUCCEED!=entry_status)
    {
        ret=FAIL;
        goto out;
    }
    
    ret=_tpdequeue (qspace, qname, ctl, data, len, flags);

out:
    return ret;
}
