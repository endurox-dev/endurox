/**
 * @brief LCF posting
 *
 * @file cmd_lcf.c
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
#include <memory.h>
#include <sys/param.h>

#include <ndrstandard.h>
#include <ndebug.h>

#include <ndrx.h>
#include <ndrxdcmn.h>
#include <atmi_int.h>
#include <gencall.h>
#include <errno.h>
#include <lcf.h>
#include <linenoise.h>

#include "nclopt.h"

#include <exhash.h>
#include <utlist.h>
#include <userlog.h>
#include <lcfint.h>

/*---------------------------Externs------------------------------------*/
/*---------------------------Macros-------------------------------------*/
#define PAGE_1          1       /**< page 1 infos */
#define PAGE_2          2       /**< page 2 infos */
#define PAGE_3          3       /**< page 3 infos */
/*---------------------------Enums--------------------------------------*/
/*---------------------------Typedefs-----------------------------------*/
/*---------------------------Globals------------------------------------*/
/*---------------------------Statics------------------------------------*/

exprivate linenoiseCompletions *M_lc;   /**< params to callback */
exprivate char *M_lc_buf; /**< params to callback */
exprivate int  M_lc_buf_len; /**< params to callback */

/**
 * Perform stock Enduro/X stock LCF command initialization
 * @return EXSUCCEED/EXFAIL
 */
expublic int ndrx_xadmin_lcf_init(void)
{
    int ret = EXSUCCEED;
    
    ndrx_lcf_reg_xadmin_t xcmd;
    
    /* Disable LCF command */
    memset(&xcmd, 0, sizeof(xcmd));
    
    xcmd.version = NDRX_LCF_XCMD_VERSION;
    xcmd.command = NDRX_LCF_CMD_DISABLED;
    NDRX_STRCPY_SAFE(xcmd.cmdstr, NDRX_LCF_CMDSTR_DISABLE);
    xcmd.dfltslot = 0;
    
    /* first argument is mandatory */
    NDRX_STRCPY_SAFE(xcmd.helpstr, "Disable published command");

    if (EXSUCCEED!=ndrx_lcf_xadmin_add_int(&xcmd))
    {
        NDRX_LOG(log_error, "Failed to register %d [%s] LCF command: %s",
                xcmd.command, xcmd.cmdstr, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
    /* logrotate: */
    memset(&xcmd, 0, sizeof(xcmd));
    
    xcmd.version = NDRX_LCF_XCMD_VERSION;
    xcmd.command = NDRX_LCF_CMD_LOGROTATE;
    NDRX_STRCPY_SAFE(xcmd.cmdstr, NDRX_LCF_CMDSTR_LOGROTATE);
    xcmd.dfltslot = NDRX_LCF_SLOT_LOGROTATE;
    xcmd.dfltflags =NDRX_LCF_FLAG_ALL|NDRX_LCF_FLAG_DOSTARTUPEXP;
    NDRX_STRCPY_SAFE(xcmd.helpstr, "Perform logrotate");

    if (EXSUCCEED!=ndrx_lcf_xadmin_add_int(&xcmd))
    {
        NDRX_LOG(log_error, "Failed to register %d [%s] LCF command: %s",
                xcmd.command, xcmd.cmdstr, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
        
    /* logger change: */
    memset(&xcmd, 0, sizeof(xcmd));
    
    xcmd.version = NDRX_LCF_XCMD_VERSION;
    xcmd.command = NDRX_LCF_CMD_LOGCHG;
    NDRX_STRCPY_SAFE(xcmd.cmdstr, NDRX_LCF_CMDSTR_LOGCHG);
    xcmd.dfltslot = NDRX_LCF_SLOT_LOGCHG;
    
    /* first argument is mandatory */
    xcmd.dfltflags = NDRX_LCF_FLAG_ARGA;
    NDRX_STRCPY_SAFE(xcmd.helpstr, "Perform logrotate");

    if (EXSUCCEED!=ndrx_lcf_xadmin_add_int(&xcmd))
    {
        NDRX_LOG(log_error, "Failed to register %d [%s] LCF command: %s",
                xcmd.command, xcmd.cmdstr, Nstrerror(Nerror));
        EXFAIL_OUT(ret);
    }
    
out:
    return ret;
}

/**
 * Print LCF commands callback
 * @param xcmd LCF xadmin command
 */
exprivate void lcf_print_cmds(ndrx_lcf_reg_xadminh_t *xcmd)
{
    
    fprintf(stdout, "\t\t%s\t%s (dslot: %d)\n", xcmd->cmdstr, 
            xcmd->xcmd.helpstr, xcmd->xcmd.dfltslot);

    if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ARGA)
    {
        fprintf(stdout, "\t\t\t-A parameters is mandatory\n");
    }
    
    if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ARGB)
    {
        fprintf(stdout, "\t\t\t-B parameters is mandatory\n");
    }
    
    if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ALL)
    {
        fprintf(stdout, "\t\t\tApplies to all binaries (-A by default)\n");
    }
    
    if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_DOSTARTUP)
    {
        fprintf(stdout, "\t\t\tCommand also executes at startup of binaries\n");
    }
    
    if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_DOSTARTUPEXP)
    {
        fprintf(stdout, "\t\t\tCommand also executes at startup of binaries with expiry\n");
    }
    
}

/**
 * Complete the command here
 * @param xcmd
 */
exprivate void cmd_lcf_completion_add(ndrx_lcf_reg_xadminh_t *xcmd)
{
    char tmpbuf[128];
    snprintf(tmpbuf, sizeof(tmpbuf), "lcf %s", xcmd->cmdstr);
    
    if (0==strncmp(tmpbuf, M_lc_buf, M_lc_buf_len))
    {
        linenoiseAddCompletion(M_lc, tmpbuf);
    }
}

/**
 * Add completions for lcf
 * @param lc completion handler
 * @return 
 */
expublic int cmd_lcf_completion(linenoiseCompletions *lc, char *buf)
{
    M_lc=lc;
    M_lc_buf=buf;
    M_lc_buf_len=strlen(buf);
    ndrx_lcf_xadmin_list(cmd_lcf_completion_add);
    
    return EXSUCCEED;
}

/**
 * Return the list of LCF commands registered with xadmin
 * @return 
 */
expublic int cmd_lcf_help(void)
{
    fprintf(stdout, "\tPublished COMMANDs:\n");
    
    ndrx_lcf_xadmin_list(lcf_print_cmds);
    
    fprintf(stdout, "\n");
    
    return EXSUCCEED;
}

/**
 * Print lcf data
 */
expublic void print_lcf_data(int page)
{
    int i, j;
    char flagsstr[64];
    ndrx_lcf_command_t *cur;
    char cmdstr[8+1];
    
static const struct {
    long flag;
    char *flagstr;

} flag_map[] = {
    {  NDRX_LCF_FLAG_PID, "p" },
    {  NDRX_LCF_FLAG_BIN, "b" }, 
    {  NDRX_LCF_FLAG_ALL, "a" }, 
    {  NDRX_LCF_FLAG_ARGA, "A" }, 
    {  NDRX_LCF_FLAG_ARGB, "B" }, 
    {  NDRX_LCF_FLAG_DOREX, "r" }, 
    {  NDRX_LCF_FLAG_DOSTARTUP, "n" }, 
    {  NDRX_LCF_FLAG_DOSTARTUPEXP, "e" }
};

    if (PAGE_1==page)
    {
        fprintf(stderr, "SLOT  CID COMMAND  FLAGS    PROCID           #SEEN #APPL #FAIL      AGE FEEDBACK\n");
        fprintf(stderr, "---- ---- -------- -------- ---------------- ----- ----- ----- -------- --------\n");
    }
    else if (PAGE_2==page)
    {
        fprintf(stderr, "SLOT  CID COMMAND  ARG_A                                      ARG_B\n");
        fprintf(stderr, "---- ---- -------  ------------------------------------------ ------------------\n");
    }
    else if (PAGE_3==page)
    {
        fprintf(stderr, "SLOT  CID COMMAND  FEEDBACKMSG\n");
        fprintf(stderr, "---- ---- -------  -------------------------------------------------------------\n");
    }
    
    if (EXSUCCEED!=ndrx_lcf_read_lock())
    {
        NDRX_LOG(log_error, "Failed to lock shm for read.");
        goto out;
    }
    
    /* loop over the shm... */
    
    for (i=0; i<ndrx_G_libnstd_cfg.lcfmax; i++)
    {
        cur=&ndrx_G_shmcfg->commands[i];
        
        
        if (NDRX_LCF_CMD_DISABLED==cur->command)
        {
            /* no more details for disabled command */
            fprintf(stdout, "%4d %4d %-8.8s\n",
                    i, 
                    cur->command, 
                    NDRX_LCF_CMDSTR_DISABLE);
        }
        else if (PAGE_1==page)
        {
            char procid[16+1];
            
            /* build flags string */
            flagsstr[0] = EXEOS;
            
            for (j=0; j<N_DIM(flag_map); j++)
            {
                if (cur->flags & flag_map[j].flag)
                {
                    NDRX_STRCAT_S(flagsstr, sizeof(flagsstr), flag_map[j].flagstr);
                }
            }
            
            fprintf(stdout, "%4d %4d %-8.8s %-8.8s %-16.16s %5.5s %5.5s %5.5s %8.8s %8ld\n",
                    i, 
                    cur->command, 
                    ndrx_decode_str(cur->cmdstr, cmdstr, sizeof(cmdstr)),
                    flagsstr, 
                    ndrx_decode_str(cur->procid, procid, sizeof(procid)),
                    ndrx_decode_num(cur->seen,    0, 0, 1),
                    ndrx_decode_num(cur->applied, 1, 0, 1),
                    ndrx_decode_num(cur->failed, 2, 0, 1),
                    ndrx_decode_msec(ndrx_stopwatch_get_delta(&cur->publtim), 0, 0, 2),
                    cur->fbackcode);
            
        }
        else if (PAGE_2==page)
        {
            char arg_a[42+1];
            char arg_b[18+1];
            
            fprintf(stdout, "%4d %4d %-8.8s %-42.42s %-18.18s\n",
                    i, 
                    cur->command, 
                    ndrx_decode_str(cur->cmdstr, cmdstr, sizeof(cmdstr)),
                    ndrx_decode_str(cur->arg_a, arg_a, sizeof(arg_a)),
                    ndrx_decode_str(cur->arg_b, arg_b, sizeof(arg_b))
                    );            
        }
        else if (PAGE_3==page)
        {
            char fbackmsg[61+1];
            
            fprintf(stdout, "%4d %4d %-8.8s %-61.61s\n",
                    i, 
                    cur->command, 
                    ndrx_decode_str(cur->cmdstr, cmdstr, sizeof(cmdstr)),
                    ndrx_decode_str(cur->fbackmsg, fbackmsg, sizeof(fbackmsg))
                    );
        }
    }
    
    ndrx_lcf_read_unlock();
out:
    return;
}

/**
 * Print or set the LCF command
 * @param p_cmd_map
 * @param argc
 * @param argv
 * @return SUCCEED
 */
expublic int cmd_lcf(cmd_mapping_t *p_cmd_map, int argc, char **argv, int *p_have_next)
{
    int ret = EXSUCCEED;
    char pid[NAME_MAX]="";
    char binary[NAME_MAX]="";
    int all = EXFALSE;
    
    int regex = EXFALSE;
    int exec_start = EXFALSE;
    int exec_start_exp = EXFALSE;
    char arg_a[PATH_MAX+1]="";
    char arg_b[NAME_MAX+1]="";
    short slot=EXFAIL;
    
    /* if no arguments given, then it is print */
    ncloptmap_t clopt[] =
    {
        {'A', BFLD_STRING, arg_a, sizeof(arg_a), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Argument a to COMMAND"},
        {'B', BFLD_STRING, arg_b, sizeof(arg_b), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Argument b to COMMAND"},
        {'p', BFLD_STRING, pid, sizeof(pid), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "PID"},
        {'b', BFLD_STRING, binary, sizeof(binary), 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Executable binary name"},
        {'r', BFLD_SHORT, (void *)&regex, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "PID or Binary name is regexp"}, 
        {'a', BFLD_SHORT, (void *)&all, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Apply to all binaries"}, 
        {'n', BFLD_SHORT, (void *)&exec_start, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Apply at startup"},
        {'e', BFLD_SHORT, (void *)&exec_start_exp, 0, 
                                NCLOPT_OPT|NCLOPT_TRUEBOOL, "Apply at startup with expiry"},
        {'s', BFLD_SHORT, (void *)&slot, 0, 
                                NCLOPT_OPT|NCLOPT_HAVE_VALUE, "Slot number to which publish"},
        {0}
    };
    
    _Nunset_error();
    
    if (EXSUCCEED!=ndrx_lcf_supported_int())
    {
        EXFAIL_OUT(ret);
    }
    
    /* Check the argument count... if no args are given, then print current
     * LCF table, If args are given, parse them up...
     */
    if (NULL==ndrx_G_shmcfg)
    {
        /* LCF not loaded...! */
        _Nset_error_fmt(NESYSTEM, "LCF Commands disabled");
    }
    
    if (1==argc || 2==argc && 0==strcmp(argv[1], "-1"))
    {
        print_lcf_data(PAGE_1);
    }
    else if (2==argc && 0==strcmp(argv[1], "-2"))
    {
        print_lcf_data(PAGE_2);
    }
    else if (2==argc && 0==strcmp(argv[1], "-3"))
    {
        print_lcf_data(PAGE_3);
    }
    else
    {
        ndrx_lcf_reg_xadminh_t *xcmd = ndrx_lcf_xadmin_find_int(argv[1]);
        ndrx_lcf_command_t cmd;
                
        if (NULL==xcmd)
        {
            _Nset_error_fmt(NESUPPORT, "Command [%s] not found", argv[1]);
            EXFAIL_OUT(ret);
        }
        
        memset(&cmd, 0, sizeof(cmd));
        
        /* start the next arg, as we strip off the sub-command */
        if (nstd_parse_clopt(clopt, EXTRUE,  argc-1, argv+1, EXFALSE))
        {
            _Nset_error_fmt(NEINVAL, "Failed to parse CLI args");
            EXFAIL_OUT(ret);
        }
        
        /* decide the target, cli has more power over the default */
        if (EXEOS!=pid[0] && EXEOS!=binary[0])
        {
            _Nset_error_fmt(NEINVAL, "-p and -b cannot be mixed");
            EXFAIL_OUT(ret);
        }
        
        /* makes no sense to mix binary with -A (all) flag */
        if ( (EXEOS!=pid[0] || EXEOS!=binary[0]) && all )
        {
            _Nset_error_fmt(NEINVAL, "-p or -b cannot be mixed with -a");
            EXFAIL_OUT(ret);
        }
        
        if (EXEOS!=pid[0])
        {
            NDRX_STRCPY_SAFE(cmd.procid, pid);
            cmd.flags|=NDRX_LCF_FLAG_PID;
        }
        else if (EXEOS!=binary[0])
        {
            NDRX_STRCPY_SAFE(cmd.procid, binary);
            cmd.flags|=NDRX_LCF_FLAG_BIN;
        }
        else if (all)
        {
            cmd.flags|=NDRX_LCF_FLAG_ALL;
        }
        else
        {
            cmd.flags|= (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ALL);
        }
        
        /* Except this rule does not affect disable command */
        if (EXEOS==cmd.procid[0] && !(cmd.flags & NDRX_LCF_FLAG_ALL) &&
                NDRX_LCF_CMD_DISABLED!=xcmd->xcmd.command)
        {
            _Nset_error_fmt(NEINVAL, "There is no process target for command (missing -a/-p/-b and not defaults)");
            EXFAIL_OUT(ret);
        }
        
        /* Check the mandatory arguments */
        
        if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ARGA && EXEOS==arg_a[0])
        {
            _Nset_error_fmt(NEINVAL, "-a argument is required for command");
            EXFAIL_OUT(ret);
        }
        
        if (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_ARGB && EXEOS==arg_b[0])
        {
            _Nset_error_fmt(NEINVAL, "-b argument is required for command");
            EXFAIL_OUT(ret);
        }
        
        NDRX_STRCPY_SAFE(cmd.arg_a, arg_a);
        NDRX_STRCPY_SAFE(cmd.arg_b, arg_b);
        
        /* add regexp flags  */
        
        if (regex)
        {
            cmd.flags|=NDRX_LCF_FLAG_DOREX;
        }
        
        /* copy defaults */
        
        cmd.flags|= (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_DOSTARTUP);
        
        if (exec_start)
        {
            cmd.flags|=NDRX_LCF_FLAG_DOSTARTUP;
        }
        
        cmd.flags|= (xcmd->xcmd.dfltflags & NDRX_LCF_FLAG_DOSTARTUPEXP);
        
        if (exec_start_exp)
        {
            cmd.flags|=NDRX_LCF_FLAG_DOSTARTUPEXP;
        }
        
        if (EXFAIL==slot)
        {
            slot = xcmd->xcmd.dfltslot;
        }
        
        cmd.version=NDRX_LCF_LCMD_VERSION;
        cmd.command = xcmd->xcmd.command;
        NDRX_STRCPY_SAFE(cmd.cmdstr, xcmd->cmdstr);
        
        /* OK let it publish */
        if (EXSUCCEED!=ndrx_lcf_publish(slot, &cmd))
        {
            EXFAIL_OUT(ret);
        }
        
        /* Looks good */
        fprintf(stderr, "OK\n");
    }
    
out:
    
    if (EXSUCCEED!=ret)
    {
        fprintf(stderr, "fail, code: %d: %s\n", Nerror, Nstrerror(Nerror));
    }

    return ret;
}

/* vim: set ts=4 sw=4 et smartindent: */
