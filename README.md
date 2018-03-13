# endurox
EnduroX is Open Source Middleware Platform for Distributed Transaction 
Processing

Platform provides X/Open XATMI and XA APIs for C/C++ applications. EnduroX can 
be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), 
Hitachi OpenTP1 and other XATMI middlewares.

EnduroX provides SOA architecture for C/C++ applications and allows to cluster 
application in fault tolerant way over multiple physical servers. 
EnduroX provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, 
including boolean expressions. Other Tuxedo specific APIs are supported, 
such as tpforward() and work with distributed transactions (tpbegin(), 
tpcommit(), etc.).

EnduroX have binddings for:
- Golang (client & server)
- PHP (client)
- Perl (client & server)
- Python (client & server)
- Node.js (client)

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
http://www.endurox.org/projects/endurox/files

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

- Version 5.3.1 released on 14/03/2018 (development) Feature #272 - tpcall cache

# Build configurations

## Configure make with: 

$ cmake -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/dist .

### Flags:

- To disable GPG_ME, pass additional flag to cmake '-DDEFINE_DISABLEGPGME=ON'

- To disable documentation building add '-DDEFINE_DISABLEDOC=ON'

- To enable poll() use instead of epoll() in Linux use '-DDEFINE_FORCEPOLL=ON'

- To disable platform script building use '-DDEFINE_DISABLEPSCRIPT=ON'

- To do release build, use '-DDEFINE_RELEASEBUILD=ON'

- To force use emulated message queue, add '-DDEFINE_FORCEEMQ=ON'

- To log the memory allocation to user log add '-DNDRX_MEMORY_DEBUG=1' 

- To trace of the Object-API use '-DNDRX_OAPI_DEBUG=1' 

- To enable file descriptor based treating of the queues 
'-DDEFINE_FORCEFDPOLL=ON'

- To trace of the Semaphore handling use '-DNDRX_SEM_DEBUG=1' 

- To enable test047 with Oracle PRO*C database access use '-DENABLE_TEST47=ON'
Note that *proc* must be in path and ORACLE_HOME must be set. Also Oracle DB 
libraries must be present in LD_LIBRARY_PATH (or equivalent environment for 
target OS).
