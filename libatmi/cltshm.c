/**
 * @brief Client admin shared memory handler
 *  Added to libatm so that tpadmsv can use it too.
 *
 * @file cltshm.c
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
#include <ndrx_config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <sys/sem.h>

#include <atmi.h>
#include <atmi_shm.h>
#include <ndrstandard.h>
#include <ndebug.h>
#include <ndrxd.h>
#include <ndrxdcmn.h>
#include <userlog.h>

/* shm_* stuff, and mmap() */
#include <sys/mman.h>
#include <sys/types.h>
/* exit() etc */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <nstd_shm.h>
#include <cpm.h>
/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate ndrx_shm_t M_clt_shm = {.fd=EXFAIL, .path="", .mem=NULL}; /**< Shared mem block, clt */
exprivate ndrx_sem_t M_clt_sem = {.semid=0};        /**< Protect the shared memory */
exprivate int M_attached = EXFALSE;     /**< Are we attached ? */

/*---------------------------Prototypes---------------------------------*/

/**
 * Attach to shared memory of the Client Processes
 * @param attach_only attach only, if does not exists - fail
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_cltshm_init(int attach_only)
{
    int ret = EXSUCCEED;
    
    if (M_attached)
    {
        NDRX_LOG(log_warn, "Already attached to CPM/CLT SHM");
        goto out;
    }
    
    M_clt_shm.fd = EXFAIL;
    M_clt_shm.key = G_atmi_env.ipckey + NDRX_SHM_CPM_KEYOFSZ;

    snprintf(M_clt_shm.path, sizeof(M_clt_shm.path), NDRX_SHM_CPM, G_atmi_env.qprefix);
    M_clt_shm.size = sizeof(ndrx_clt_shm_t)*G_atmi_env.max_clts;
    
    if (attach_only)
    {
        if (EXSUCCEED!=ndrx_shm_attach(&M_clt_shm))
        {
            NDRX_LOG(log_error, "Failed to attach ",
                        M_clt_shm.path);
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_shm_open(&M_clt_shm, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open shm [%s] - System V Queues cannot work",
                    M_clt_shm.path);
        EXFAIL_OUT(ret);
    }
    
    memset(&M_clt_sem, 0, sizeof(M_clt_sem));
    
    /* Init the RW semaphores */
    M_clt_sem.key = G_atmi_env.ipckey + NDRX_SEM_CPMLOCKS;
    M_clt_sem.nrsems = 1;
    M_clt_sem.maxreaders = NDRX_CPMSHM_MAX_READERS;
    
    NDRX_LOG(log_debug, "CPMSHM: Using service semaphore key: %d max readers: %d", 
            M_clt_sem.key, M_clt_sem.maxreaders);
    
    /* OK, either create or attach... */
    if (attach_only)
    {
        if (EXSUCCEED!=ndrx_sem_attach(&M_clt_sem))
        {
            NDRX_LOG(log_error, "Failed to attach semaphore for CPM "
                    "map shared mem");
            EXFAIL_OUT(ret);
        }
    }
    else if (EXSUCCEED!=ndrx_sem_open(&M_clt_sem, EXTRUE))
    {
        NDRX_LOG(log_error, "Failed to open semaphore for CPM "
                "map shared mem");
        userlog("Failed to open semaphore for CPM "
                "map shared mem");
        EXFAIL_OUT(ret);
    }
    
    M_attached = EXTRUE;
 out:
    NDRX_LOG(log_debug, "returns %d", ret);
    return ret;
}

/**
 * Generate the hash of the cpm key
 * @param conf hash config
 * @param key_get key data
 * @param key_len key len (not used)
 * @return slot number within the array
 */
exprivate int cltkey_key_hash(ndrx_lh_config_t *conf, void *key_get, size_t key_len)
{
    return ndrx_hash_fn(key_get) % conf->elmmax;
}

/**
 * Generate debug for CPM key
 * @param conf hash config
 * @param key_get key data
 * @param key_len not used
 * @param dbg_out output debug string
 * @param dbg_len debug len
 */
exprivate void cltkey_key_debug(ndrx_lh_config_t *conf, void *key_get, 
        size_t key_len, char *dbg_out, size_t dbg_len)
{
    NDRX_STRNCPY_SAFE(dbg_out, key_get, dbg_len);
}

/**
 * CPM shared mem key debug value
 * @param conf hash config
 * @param idx index number
 * @param dbg_out debug buffer
 * @param dbg_len debug len
 */
exprivate void cltkey_val_debug(ndrx_lh_config_t *conf, int idx, char *dbg_out, 
        size_t dbg_len)
{
    NDRX_STRNCPY_SAFE(dbg_out, NDRX_CPM_INDEX((*conf->memptr), idx)->key, dbg_len);
}

/**
 * Compare the keys
 * @param conf hash config
 * @param key_get key
 * @param key_len key len (not used)
 * @param idx index at which to compare
 * @return 0 = equals, others not.
 */
exprivate int cltkey_compare(ndrx_lh_config_t *conf, void *key_get, 
        size_t key_len, int idx)
{
    return strcmp(NDRX_CPM_INDEX((*conf->memptr), idx)->key, key_get);
}

/**
 * Get shm pos by key
 * @param key client key, format "tag/subsection" ?
 * @param oflag O_CREATE if create new rec
 * @param[out] pos position found suitable to succeed request
 * @param[out] have_value valid value is found? EXTRUE/EXFALSE.
 * @return EXTRUE -> found position/ EXFALSE - no position found
 */
expublic int ndrx_cltshm_get_key(char *key, int oflag, int *pos, int *have_value)
{
    static ndrx_lh_config_t conf;
    static int first = EXTRUE;
    
    if (first)
    {
        conf.elmmax = G_atmi_env.max_clts;
        conf.elmsz = sizeof(ndrx_clt_shm_t);
        conf.flags_offset = EXOFFSET(ndrx_clt_shm_t, flags);
        conf.memptr = (void **)&(M_clt_shm.mem);
        conf.p_key_hash=&cltkey_key_hash;
        conf.p_key_debug=&cltkey_key_debug;
        conf.p_val_debug=&cltkey_val_debug;
        conf.p_compare=&cltkey_compare;
        first = EXFALSE;
    }
    
    return ndrx_lh_position_get(&conf, key, 0, oflag, pos, have_value, "cltkey");

}

/**
 * Disconnect from shared memory block
 */
expublic void ndrx_cltshm_detach(void)
{
    NDRX_LOG(log_debug, "cltshm detach");
    ndrx_shm_close(&M_clt_shm);
    ndrx_sem_close(&M_clt_sem);
    
    M_attached = EXFALSE;
}

/**
 * Remove the shared resources
 * @return 
 */
expublic int ndrx_cltshm_remove(int force)
{
    int ret = EXSUCCEED;
    
    NDRX_LOG(log_debug, "cltshm remove force: %d", force);
    
    if (M_clt_shm.fd !=EXFAIL)
    {
        if (EXSUCCEED!=ndrx_shm_remove(&M_clt_shm))
        {
            ret = EXFAIL;
        }
        M_clt_shm.fd = EXFAIL;
    }
    
    if (EXSUCCEED!=ndrx_sem_remove(&M_clt_sem, force))
    {
        ret = EXFAIL;
    }
    
out:
    return ret;
}

/**
 * Get shared mem obj
 * INIT control is up to user.
 * @return  shm obj
 */
expublic ndrx_shm_t* ndrx_cltshm_mem_get(void)
{
    return &M_clt_shm;
}

/**
 * Get semaphore obj.
 * INIT control is up to user.
 * @return sem obj
 */
expublic ndrx_sem_t* ndrx_cltshm_sem_get(void)
{
    return &M_clt_sem;
}

/**
 * Find the current pid of the process
 * @param [in] key key with <FS> seperator
 * @param [out] procname process name hint, opt
 * @param [in] procnamesz procname length 
 * @param [out] p_stattime time stamp when started, opt
 * @return pid or EXFAIL if not found
 */
expublic pid_t ndrx_cltshm_getpid(char *key, char *procname, 
        size_t procnamesz, time_t *p_stattime)
{
    pid_t ret = EXFAIL;
    int pos;
    int have_value;
    /* readlock */
    
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_clt_sem, 0, NDRX_SEM_TYP_READ))
    {
        goto out;
    }

    if (ndrx_cltshm_get_key(key, 0, &pos, &have_value))
    {
        if (have_value)
        {
            ndrx_clt_shm_t *el = NDRX_CPM_INDEX(M_clt_shm.mem, pos);
            
            /* if value is set, then we got pid */
            ret = el->pid;
            
            if (NULL!=procname)
            {
                NDRX_STRNCPY_SAFE(procname, el->procname, procnamesz);
            }
            
            if (NULL!=p_stattime)
            {
                memcpy(p_stattime, &el->stattime, sizeof(el->stattime));
            }
            
        }
    }
    
    /* unlock */
    ndrx_sem_rwunlock(&M_clt_sem, 0, NDRX_SEM_TYP_READ);
out:
    return ret;
}

/**
 * Set position.
 * This will check the key, if key requests, ISUSED, then new position will
 * be tried to allocate.
 * @param key CPM process key
 * @param pid PID of the process (used only if setting up)
 * @param flags see NDRX_CPM_MAP_* flags
 * @param procname process name only if adding the cored with ISUSED
 * @return EXSUCCEED/EXFAIL (memory full)
 */
expublic int ndrx_cltshm_setpos(char *key, pid_t pid, short flags, char *procname)
{
    int ret = EXFAIL;
    int pos;
    int have_value;
    int oflag = 0;
    ndrx_clt_shm_t* el;
    
    if (flags & NDRX_CPM_MAP_ISUSED)
    {
        oflag |=O_CREAT;
    }
    /* readlock */
    
    if (EXSUCCEED!=ndrx_sem_rwlock(&M_clt_sem, 0, NDRX_SEM_TYP_WRITE))
    {
        goto out;
    }

    if (ndrx_cltshm_get_key(key, oflag, &pos, &have_value))
    {
        
        /* we got a position */
        el = NDRX_CPM_INDEX(M_clt_shm.mem, pos);
        if (oflag)
        {
            NDRX_STRCPY_SAFE(el->key, key);
            el->pid = pid;
            NDRX_STRCPY_SAFE(el->procname, procname);
            el->flags = flags;
            time (&(el->stattime));
        }
        else
        {
            el->flags = flags;
        }
        
        ret = EXSUCCEED;
    }
    
    NDRX_LOG(log_error, "YOPT LOOKUP!");
    ndrx_cltshm_get_key(key, oflag, &pos, &have_value);
    
    /* unlock */
    ndrx_sem_rwunlock(&M_clt_sem, 0, NDRX_SEM_TYP_WRITE);
out:
    
    if (EXSUCCEED==ret)
    {
        if (oflag)
        {
            NDRX_LOG(log_info, "Process installed in CPM SHM: [%s]/%s/%d/%hd",
                    key, procname, (int)pid, flags);
        }
        else
        {
            NDRX_LOG(log_info, "Process removed from CPM SHM: [%s]/%s/%d/%hd",
                    key, NDRX_CPM_INDEX(M_clt_shm.mem, pos)->procname, 
                    (int)NDRX_CPM_INDEX(M_clt_shm.mem, pos)->pid, flags);
        }
    }
    
    return ret;
}
    
/**
 * Scan the shared memory with client data, try to kill
 * probably the largest part is already dead due to child killings.
 * After wards, remove the shared segments
 * @param [in] signals list of signals for kill, terminated with EXFAIL
 * @param [out] p_was_any if there was any killings set to EXTRUE
 */
expublic void ndrx_cltshm_down(int *signals, int *p_was_any)
{
    int i, s;
    ndrx_clt_shm_t *el;
    string_list_t* cltchildren = NULL;
    int was_kill = EXFALSE;
    char *mem_cpy;
    size_t cpsz;
    int err;
    
    /* but if future api will allow to deregister client, then
     * shared memory shall be writable to them..
     */
    
    if (ndrx_cltshm_init(EXTRUE))
    {
        NDRX_LOG(log_warn, "CLTSHM processing down");
        
        cpsz = G_atmi_env.max_clts * sizeof(ndrx_clt_shm_t);
        mem_cpy = NDRX_MALLOC(cpsz);
        
        if (NULL==mem_cpy)
        {
            err = errno;
            NDRX_LOG(log_error, "Failed to malloc %d bytes: %s", 
                    cpsz, strerror(err));
            userlog("Failed to malloc %d bytes: %s", 
                    cpsz, strerror(err));
            
            /* nothing todo; */
            goto out;
        }
        
        if (EXSUCCEED!=ndrx_sem_rwlock(&M_clt_sem, 0, NDRX_SEM_TYP_WRITE))
        {
            goto out;
        }
        
        /* thus we need to copy of memory...! */
        memcpy(mem_cpy, M_clt_shm.mem, cpsz);
        
         /* unlock */
        ndrx_sem_rwunlock(&M_clt_sem, 0, NDRX_SEM_TYP_WRITE);        
        
        for (s=0; EXFAIL!=signals[s]; s++)
        {
            for (i=0; i<G_atmi_env.max_clts; i++)
            {
                el = NDRX_CPM_INDEX(mem_cpy, i);

                if (el->flags & NDRX_CPM_MAP_ISUSED, 
                        ndrx_sys_is_process_running_by_pid(el->pid))
                {
                    /* grab the childs at first loop*/
                    if (0==s)
                    {
                        ndrx_proc_children_get_recursive(&cltchildren, el->pid);
                    }
                    
                    /* kill the process by it self */
                    kill(el->pid, signals[s]);
                    was_kill = EXTRUE;
                } /* if used */
            } /* for map entry */
            
            /* not need to wait after final signal */
            if (was_kill && EXFAIL!=signals[s+1])
            {
                sleep(EX_KILL_SLEEP_SECS);
            }
            else
            {
                break;
            }
        } /* for signal */
        
        ndrx_proc_kill_list(cltchildren);
        ndrx_string_list_free(cltchildren);

        cltchildren = NULL;
        
        /* deatch and remove shared mem... */
        ndrx_cltshm_detach();
        ndrx_cltshm_remove(EXTRUE);
    }
    
out:
    *p_was_any = was_kill;
    return;
}
/* vim: set ts=4 sw=4 et smartindent: */
