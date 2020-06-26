# endurox

## Build (including atmi/ubf tests) status

| OS   |      Status      | OS       |      Status   |OS       |      Status   |
|----------|:-------------:|----------|:-------------:|----------|:-------------:|
| AIX 7.1 |  [![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-aix7_1)](http://www.silodev.com:9090/jenkins/job/endurox-aix7_1/) |Centos 6|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-centos6)](http://www.silodev.com:9090/jenkins/job/endurox-centos6/)|FreeBSD 11|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-freebsd11)](http://www.silodev.com:9090/jenkins/job/endurox-freebsd11/)|
|Oracle Linux 7|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-ol7)](http://www.silodev.com:9090/jenkins/job/endurox-ol7/)|OSX 11.4|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-osx11_4)](http://www.silodev.com:9090/jenkins/job/endurox-osx11_4/)|raspbian10_arv7l|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-raspbian10_arv7l)](http://www.silodev.com:9090/jenkins/job/endurox-raspbian10_arv7l/)|
|SLES 12|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-sles12)](http://www.silodev.com:9090/jenkins/job/endurox-sles12/)|SLES 15|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-sles15)](http://www.silodev.com:9090/jenkins/job/endurox-sles15/)|Solaris 10|[![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-solaris10-sparc)](http://www.silodev.com:9090/jenkins/job/endurox-solaris10-sparc/)|
|Solaris 11| [![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-solaris11_x86)](http://www.silodev.com:9090/jenkins/job/endurox-solaris11_x86/)|Ubuntu 14.04| [![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-ubuntu14)](http://www.silodev.com:9090/jenkins/job/endurox-ubuntu14/)|Ubuntu 18.04| [![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-ubuntu18)](http://www.silodev.com:9090/jenkins/job/endurox-ubuntu18/)|
|RHEL/Oracle Linux 8| [![Build Status](http://www.silodev.com:9090/jenkins/buildStatus/icon?job=endurox-ol8)](http://www.silodev.com:9090/jenkins/job/endurox-ol8/)|||||

## Overview

Enduro/X is Open Source Middleware Platform for Distributed Transaction 
Processing

It is modern, micro-services based middleware for writing distributed, open
systems (program consists of several executables) based applications. Thus 
by using Enduro/X programmers do not have to worry about threads and concurrency 
anymore, the load balacing and multi-CPU loading is done by Enduro/X middleware by it self,
administrator only have to determine how many copies of particular services should
be started. Of-course Enduro/X supports multi-threaded applications too, but now
system architects have a choice either to design multi-threaded executables or
just configure number of executables to start.

For local inter-process-communication (IPC) Enduro/X uses kernel memory based Posix
queues to avoid overhead of the TCP/IP protocol which is used in other middlewares
or REST based micro-service architectures. Thus this approach greatly increases
application speed, as kernel queues is basically a matter of block memory
copy from one process to another (by contrast of 7 layers of TCP/IP stack and
streaming nature of the sockets vs block copy). 

Enduro/X provides SOA architecture for C/C++ applications and allows to cluster 
application in fault tolerant way over multiple physical servers. 
Enduro/X provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, 
including boolean expressions. Other Tuxedo specific APIs are supported, 
such as tpforward() and work with distributed transactions (tpbegin(), 
tpcommit(), etc.).

Platform provides X/Open XATMI and XA interfaces/APIs for C/C++ applications. Enduro/X can 
be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), 
Hitachi OpenTP1 and other XATMI middlewares.

Dual licensed under Affero General Public License Version 3 for use in Open
Source project or Commercial license Acquired from Mavimax Ltd 
(https://www.mavimax.com),

EnduroX have bindings for:
- Golang (client & server)
- Java (client & server)
- PHP (client)
- Perl (client & server)
- Python (client & server)
- Node.js (client)

Enduro/X provides following features:

* Standards based APIs - SCA, The Open Group XATMI
Communication types - Synchronous, Asynchronous, Conversational, Publish/subscribe
* Typed buffers
  * UBF (Unified Buffer Format) which provides emulation of Tuxedo's FML/FML32 
        format. UBF if high performance binary protocol buffer format. Buffer is 
        indexed by binary search on fixed data types.
  * STRING buffer format.
  * CARRAY (byte array) buffer format.
  * JSON buffer format, automatic conversion between JSON and UBF available.
  * VIEW buffer (starting from version 5.0+). This offer C structure sending
        between processes in cross platform way. Also this allows to map UBF 
        fields to VIEW fields, thus helping developer quicker to develop applications, 
        by combining UBF and VIEW buffers.
* Transaction Management - Global Transactions - Two-phase commit protocol - X/Open XA
* Clustering - on peer-to-peer basis
* Event broker (also called publish and subscribe messaging)
* Security - Cluster link encryption with GNU PGP framework
* System process monitoring and self healing (pings and restarts)
* SOA Service cache. XATMI services can be cached to LMDB database. Resulting 
        that next call to service from any local client receives results 
        directly from cache (mainly from direct memory read).
* Dynamic re-configuration
* Custom server polling extensions
* XATMI sub-system is able to work with out main application server daemon (ndrxd)
* Main application server daemon (ndrxd) can be restarted (if crashed). 
        When started back it enters in learning mode for some period of time, 
        in which in gathers information about system, what services are running, 
        etc. After learning =-period, it starts to do normal operations
* tpforward() call
* ATMI server threads may become clients, and can do tpcall()
* Extensive logging & debugging. Enduro/X logging can be configured per binary 
        with different log levels. As ATMI servers can be started outside of 
        appserver, it is possible to debug them from programming IDE or with 
        tools like valgrind.
* For quality assurance project uses automated unit-testing and integration-testing
* Built in ATMI service profiling.
* Environment variables can be updated for XATMI server processes with 
        out full application reboot.
* Generic client process monitor (cpm). Subsystem allows to start/stop/monitor 
        client executables. At client process crashes, cpm will start it back.
* Starting with version 5.2 Enduro/X provides configuration data encryption feature, 
        so that software which is built on top of Enduro/X may comply with 
        Payment Card Industry Data Security Standard (PCI/DSS).
* Application monitoring with TM_MIB interface. For example NetXMS.
* XA Driver for PostgreSQL.
      

![Alt text](doc/Endurox-product.jpg?raw=true "Enduro/x overview")

# Load balancing

![Alt text](doc/endurox-load-balance.jpg?raw=true "Enduro/x service load 
balancer")

Supported operating system: GNU/Linux, starting from 2.6.12 kernel 
(needed for POSIX Queues). Starting with Enduro/X Version 3.1.2 IBM AIX (6.1 and 
7.1), 
Oracle Solaris 11, MAC OS X (experimental) and Cygwin (experimental) 
support is added. Supported compilers: gcc, LLVM clang, IBM xlC.

- Build and installation guides are located at: www.endurox.org/dokuwiki

- Support forum: http://www.endurox.org/projects/endurox/boards

- Binary builds (RPMs & DEBs) for various platforms are here: 
https://www.mavimax.com/downloads

- Documentation available here: http://www.endurox.org/dokuwiki/doku.php

- Source corss reference: http://www.silodev.com/lxr/source

PS, feel free to contact me at madars@mavimax.com. I will gladly help you
 to get EnduroX running ;)

# Call/message forwarding

## Instead of doing calls to each server separately...

![Alt text](doc/exforward_tpcall.png?raw=true "Classical service orchestration")

This is typciall way for example if doing micro-services with HTTP/REST. 
You need to do the calls to each service separately. And that is extra 
overhead to system (multiple returns) and the caller must orchestrate the all 
calls.

## ...Enduro/X offers much effective way - tpforward() where request is 
passed around the system

![Alt text](doc/exforward_forward.png?raw=true "Enhanced service 
orchestration by forwarding the call")

In this case the destination service orchestrates the system, it is up to 
service to choose the next service which will continue the call processing. 
The caller is not aware of final service which will do the reply back 
(tpreturn()).

Note that service after doing "tpforward()" becomes idle and can consume next 
call.

# High availability and self healing

Enduro/X is capable of detecting stalled/hanged XATMI servers and gracefully 
can reboot them. The ping times are configurable on per server basis. In case 
if main daemon dies, it is possible to configure special XATMI server named 
"tprecover" which monitors "ndrxd" and vice versa, ndrxd can monitor tprecover. 
Thus if any of these services are not cleanly shut down, then system will 
heal it self, and boot service/daemons back.

The ndrxd learning mode is some period of time (configurable) in which ndrxd 
will request all the system for active servers and their services, 
to get back the view of the system and be in control. Note that system is able 
to work even with out ndrxd, but at that time no server healing 
and management will be done.

![Alt text](doc/server_monitoring_and_recovery.png?raw=true "Enduro/X high 
availability facility")

# Real time system patching

It is possible to patch the Enduro/X system (update stateless XATMI servers) 
without service interruption. This is achieved by following scheme:

![Alt text](doc/rt-patching.png?raw=true "Enduro/X real time patching")

# Monitoring

Enduro/X provides TM_MIB API for information reading. Out of the box NetXMS 
monitoring suite uses this API to monitor application. Thus full featured
monitoring is possible. Presentation here (video):

[![Enduro/X monitoring with NetXMS](https://img.youtube.com/vi/ubJk27bjKGE/0.jpg)](https://www.youtube.com/watch?v=ubJk27bjKGE)

# Performance

Due to fact that Enduro/X uses memory based queues, performance numbers are 
quite high:

## ATMI client calls server in async way (tpacall(), no reply from server)

![Alt text](doc/benchmark/04_tpacall.png?raw=true "Local tpcall() performance")


## Local one client calls server, and gets back response:

![Alt text](doc/benchmark/01_tpcall.png?raw=true "Local tpcall() performance")

## Multiple clients calls multiple servers

![Alt text](doc/benchmark/03_tpcall_threads.png?raw=true "Multiprocessing 
tpcall() performance")

## Single client calls single server via network (two app servers bridged)

![Alt text](doc/benchmark/02_tpcall_network.png?raw=true "Network tpcall() 
performance")

## Persistent storage (message enqueue to disk via tpenqueue())
The number here are lower because messages are being saved to disk. 
Also internally XA transaction is used, which also requires logging to stable 
storage.

![Alt text](doc/benchmark/05_persistent_storage.png?raw=true "Network 
tpenqueue() performance")


## Tpcall cache benchmark
This benchmark shows the performance of cached XATMI service calls.

![Alt text](doc/benchmark/06_tpcache.png?raw=true "tpcall() cache performance")

# Releases

- Version 2.5.1 released on 18/05/2016. Support for transactional 
persistent message queues. Provides APIs: tpenqueue(), tpdequeue() and 
command line management tools. See doc/persistent_message_queues_overview.adoc, 
doc/manpage/tmqueue.adoc, doc/manpage/xadmin.adoc, doc/manpage/q.config.adoc. 
Use cases can be seen under atmitests/test028_tmq

- Version 3.1.2 released on 25/06/2016. Added support for other Unix systems. 
Currently production grade support is provided for following operating systems: 
GNU/Linux, FreeBSD, IBM AIX and Oracle Solaris. MAC OS X and Cygwin versions 
are 
considered as experimental. FreeBSD installation guides are located at 
doc/freebsd_notes.adoc, AIX install guide: doc/aix_notes.adoc, 
Solaris install guide: doc/solaris_notes.adoc and OS X guide: 
doc/osx_notes.adoc. Also with this version lot of core processing bug fixes are 
made.

- Version 3.2.1 released on 06/07/2016. Major UBF optimization. Now for each 
data type memory offset is maintained. For fixed data types (short, long, char, 
float, double) binary search is performend when field is read from buffer.

- Version 3.2.2 released on 15/07/2016. Bugfixes for UBF binary search. Added 
UBF benchmarking scripts and documents.

- Version 3.3.1 released on 05/09/2016. Implemented common configuration engine 
(can use ini files in alternative to environment variables, debug config and 
persistent queue config).

- Version 3.3.2 released on 01/10/2016. Works on TP logger, user accessible 
logging framework. Set of tplog\* functions.

- Version 3.3.5 released on 10/01/2017. Fixes for RPM build and install on 
Red-hat enterprise linux platform.

- Version 3.3.6 released on 12/01/2017. Fixes for integration mode, changes in 
tpcontinue().

- Version 3.4.1 released on 13/01/2017. New ATMI server flag for developer - 
reloadonchange - if cksum change changed on binary issue sreload.

- Version 3.4.2 released on 20/01/2017. Network protocol framing moved from 2 
bytes to 4 bytes.

- Version 3.4.3 released on 26/01/2017. Memory leak fixes for tmsrv and tmqueue 
servers.

- Version 3.4.4 released on 26/01/2017. Works on documentation. Fixes on 
PScript.

- Version 3.4.5 released on 14/02/2017. Fixes in generators.

- Version 3.5.1 released on 03/03/2017. Updates on tpreturn() - more correct 
processing as in Tuxedo, free up return buffer + free up auto buffer.

- Version 3.5.2 released on 24/03/2017. Bug #105 fix. Abort transaction if it 
was in "preparing" stage, and tmsrv loaded it from logfile. Meaning that caller 
alaready do not wait for tpcommit() anymore and got inconclusive results, thus 
better to abort. 

- Version 3.5.3 released on 26/03/2017. Fixes for Solaris, Sun Studio (SUNPRO) 
compiler

- Version 3.5.4 released on 01/03/2017. Fixed bug #108 - possible CPM crash

- Version 3.5.5 released on 23/03/2017. Works Feature #113 - mqd_t use as file 
descriptor - for quick poll operations.

- Version 3.5.6 released on 02/04/2017. Fixed issues with possible deadlock for 
poll mode on bridge service calls. Fixed issuw with 25 day uptime problem for 
bridges on 32bit platfroms - the messages becomes expired on target server. 
Issues: #112, #145, #140, #139, #113

- Version 3.5.7 release on 23/04/2017. Fixed Bug #148 (Bdelall corrupts ubf 
buffer)
Bug #110 (tpbridge does not report connection status to ndrxd after ndrxd is 
restarted for recovery)

- Version 3.5.9 released on 22/06/2017. Bug fix #160 - retry xa_start with 
xa_close/xa_open. New env variable NDRX_XA_FLAGS, tag "RECON".

- Version 4.0.1 released on 29/06/2017. Multi-threaded bridge implementation. 
tpnotify(), tpbroadcast(), tpsetunsol(), tpchkunsol() API implementation.

- Version 4.0.2 released on 19/07/2017. Fixed ndrxd core dump issue Bug #174.

- Version 4.0.3 released on 26/07/2017. Fixes of #176 #178. NDRX_DUMP, UBF_DUMP 
fixes.

- Version 4.0.4 released on 29/07/2017. Feature #162 - added evt_mib.h

- Version 4.0.5 released on 01/08/2017. Feature #180 - refactoring code towards 
ISO/IEC 9899:201x standard.

- Version 5.0.1 released on 21/08/2017. Feature #96 - Typed View support, Bug 
#182 - fix in bridge protocol.

- Version 5.0.2 released on 22/08/2017. Bug #183

- Version 5.0.3 released on 22/08/2017. Bug #186

- Version 5.0.4 released on 22/08/2017. Bug #186

- Version 5.0.5 released on 27/08/2017. Bug #184, Bug #191, Bug #190, Bug #192, 
Bug #193

- Version 5.0.6 released on 28/08/2017. Bug #195

- Version 5.0.7 released on 01/09/2017. Feature #161 - tmsrv database pings & 
automatic reconnecting in case of network failures.

- Version 5.0.8 released on 17/09/2017. Works on dynamic view access. #99, #206, 
#207, #210

- Version 5.0.9 released on 27/09/2017. (development) Works on #224

- Version 5.0.10 released on 13/10/2017. (stable) Feature #232, Feature #231, 
-O2 optimization by default

- Version 5.1.1  released on 13/10/2017. (development) Feature #127, Bug #229, 
Feature #230, Bug #234, Feature #244, Bug #243, Feature #248, Bug #240, Bug #238 

- basically big message size support (over the 64K)

- Version 5.1.2  released on 18/10/2017. (stable) Bug #250

- Version 5.2.1  released on 18/12/2017. (development) Major work on support for PCI/DSS mandatory
configuration encryption. Introduction of plugin architecture (currently used for
crypto key providers). Implemented tpconvert() ATMI call. Now @global section for ini files are 
read twice. Thus ini file can reference to previosly defined env/global variable. 
Fixes: #261 Bug, #118 Feature, #237 Feature, #236 Bug, #245 Feature, #258
Support, #259 Support, #255 Bug, #254 Bug.

- Version 5.2.2 released on 21/12/2017. (stable) First stable version of Enduor/X 5.2. Rolled

- Version 5.2.4 released on 22/12/2017. (stable) Fixed Bug #268.

- Version 5.2.6 released on 02/01/2018. (stable) Happy New Year! Fixed Bug #269.

- Version 5.2.8 released on 26/01/2018. (stable) Fixed Bug #274 - too many open files,
when threaded logger using for multi-contexting (like go runtime)

- Version 5.2.10 released on 27/01/2018. (stable) Feature #275 - allow to mask

- Version 5.2.12 released on 27/01/2018. (stable) Feature #275 - fixes for server un-init (not critical)

- Version 5.2.14 released on 03/02/2018. (stable) Feature #278 - new fields for compiled connection id

- Version 5.2.15 released on 08/02/2018. (development) Feature #282 - new UBF api Baddfast()

- Version 5.2.16 released on 09/02/2018. (stable) Feature #282 - new UBF api Baddfast(), finished
documentation and added unit tests.

- Version 5.3.2 released on 25/03/2018 (stable) Fixes on Feature #272, Bug #291, Feature #287, Bug #290, Support #279

- Version 5.3.4 released on 26/03/2018 (stable) Feature #294, Bug #293

- Version 5.3.5 released on 28/03/2018 (Development) Working progress on Feature #295

- Version 5.3.6 released on 01/04/2018 (stable) Finished works on Feature #295

- Version 5.3.8 released on 06/04/2018 (stable) Implemented Feature #299 - new flag TPNOABORT

- Version 5.3.9 released on 09/04/2018 (development) Fixed Bug #300

- Version 5.3.10 released on 10/04/2018 (stable) Fixed Support #301

- Version 5.3.12 released on 12/04/2018 (stable) Support #302 - added last 8 chars of hostname to logfile

- Version 5.3.14 released on 21/04/2018 (stable) Bug #306

- Version 5.3.15 released on 23/04/2018 (development) Feature #307

- Version 5.3.16 released on 16/05/2018 (stable) Bug #317

- Version 5.3.17 released on 18/05/2018 (development) Feature #320 - cmdline tag for server in xml config

- Version 5.3.18 released on 27/06/2018 (stable) Bug #325

- Version 5.3.20 released on 10/10/2018 (stable) Backport Bug #321 fix to 5.3

- Version 6.0.1 released on 18/11/2018 (development) Latvia's 100th anniversary. 
Major work on System V queue support, works for Java support, lots of bugs fixed.
Support #252, Feature #281, Feature #307, Support #310, Support #326, 
Bug #330, Feature #331, Feature #333, Feature #334, Bug #338, 
Bug #339, Bug #341, Bug #347, Support #350, Bug #351, Support #352, Bug #353

- Version 6.0.2 released on 02/12/2018 (stable) Bug #355 and other stability issues fixed
released with 6.0.1.

- Version 6.0.3 released on 16/12/2018 (development) Bug #364, Bug #360. 
Added new command "ps" for xadmin, so that unit tests have unified access to 
processing listings across all supported OSes.

- Version 6.0.5 released on 24/12/2018 (development) Feature #366

- Version 6.0.6 released on 08/01/2019 (stable) Marking stable release

- Version 6.0.8 released on 11/01/2019 (stable) Feature #372

- Version 6.0.10 released on 13/01/2019 (stable) Support #373, Bug #374

- Version 6.0.12 released on 16/01/2019 (stable) Bug #375, Bug #376

- Version 6.0.13 released on 18/01/2019 (development) Feature #181

- Version 6.0.14 released on 30/01/2019 (stable) Marking stable version

- Version 6.0.15 released on 01/01/2019 (development) Feature #380 

- Version 6.0.17 released on 02/01/2019 (development) Feature #382, Feature #367, Support #303, Feature #271

- Version 6.0.19 released on 15/03/2019 (development) Bug #394, Feature #393

- Version 6.0.21 released on 19/03/2019 (development) Feature #397

- Version 6.0.23 released on 19/03/2019 (development) Additional work for Feature #397

- Version 6.0.25 released on 22/03/2019 (development) Feature #396

- Version 6.0.26 released on 25/03/2019 (stable) Support #400

- Version 6.0.27 released on 30/03/2019 (development) Feature #402

- Version 6.0.28 released on 11/06/2019 (stable) Feature #419

- Version 7.0.10 released on 03/11/2019 (stable) Release notes are here: https://www.endurox.org/news/18

- Version 7.0.12 released on 13/11/2019 (stable) Fixes on Bug #466

- Version 7.0.14 released on 13/11/2019 (stable) Fixes on Feature #470, Support #471, Support #473, Support #475

- Version 7.0.16 released on 16/11/2019 (stable) Fixes on Support #443, Support #493

- Version 7.0.18 released on 17/01/2020 (stable) Fixes on Bug #412, Support #505

- Version 7.0.20 released on 22/01/2020 (stable) Fixes on Support #502

- Version 7.0.22 released on 31/01/2020 (stable) Feature #509

- Version 7.0.24 released on 01/01/2020 (stable) Support #506

- Version 7.0.26 released on 09/03/2020 (stable) Support #528, Support, #527, Support #524,
Bug #523, Bug #521, Bug #515, Support #512, Bug #507, Support #503

- Version 7.0.28 released on 12/03/2020 (stable) Bug #530, Feature #313, Feature #536

- Version 7.0.30 released on 24/03/2020 (stable) Bug #537

- Version 7.0.32 released on 29/03/2020 (stable) Bug #544

- Version 7.0.34 released on 11/05/2020 (stable) Bug #501

- Version 7.0.36 released on 25/06/2020 (stable) Feature #262

- Version 7.5.1 released on 10/06/2020 (development) Feature #547, Support #553, Feature #545, Feature #540,
Feature #218, Feature #368, Feature #398, Feature #440, Feature #463, Feature #497, Feature #511, Feature #539,
Bug #542, Feature #549

# Build configurations

## Configure make with: 

$ cmake -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/dist .

### Flags:

- To enable System V message queue, pass '-DDEFINE_SYSVQ=ON' to cmake (applies
to Linux only). Default on AIX and Solaris.

- To enable poll() use instead of epoll() in Linux use '-DDEFINE_FORCEPOLL=ON'
(applies to Linux only).

- To force use emulated message queue, add '-DDEFINE_FORCEEMQ=ON' (applies to 
Linux only). Default on Macos.

- To disable GPG_ME, pass additional flag to cmake '-DDEFINE_DISABLEGPGME=ON'

- To disable documentation building add '-DDEFINE_DISABLEDOC=ON'

- To disable platform script building use '-DDEFINE_DISABLEPSCRIPT=ON'

- To do release build, use '-DDEFINE_RELEASEBUILD=ON'

- To log the memory allocation to user log add '-DNDRX_MEMORY_DEBUG=1' 

- To trace of the Object-API use '-DNDRX_OAPI_DEBUG=1' 

- To trace of the Semaphore handling use '-DNDRX_SEM_DEBUG=1' 

- To enable test047 with Oracle PRO*C database access use '-DENABLE_TEST47=ON'
Note that *proc* must be in path and ORACLE_HOME must be set. Also Oracle DB 
libraries must be present in LD_LIBRARY_PATH (or equivalent environment for 
target OS).

- To force memory alignment usage, use '-DDEFINE_ALIGNMENT_FORCE=1', by default
on for SPARC cpus.

- To enable address sanitizer for GCC/Clang on supported hardware platforms,
use '-DDEFINE_SANITIZE=1'

- To enable PostgreSQL XA Driver build use '-DENABLE_POSTGRES=ON'

- To disable PostgreSQL ECPG Driver build use '-DDISABLE_ECPG=ON' (for certain
Operating systems ECPG, postgres-devel package is missing, thus let only pq
driver to build)

- To enable strict mutex checking on GNU platform, use '-DMUTEX_DEBUG=ON' (for
GNU platforms only)

- To force use AIX System V Polling on Linux (for dev build only, non functional),
add '-DDEFINE_SVAPOLL=ON'. Default on AIX.

