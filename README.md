# endurox
EnduroX is Open Source Middleware Platform for Distributed Transaction Processing

Platform provides X/Open XATMI and XA APIs for C/C++ applications. EnduroX can be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), Hitachi OpenTP1 and other XATMI middlewares.

EnduroX provides SOA architecture for C/C++ applications and allows to cluster application in fault tolerant way over multiple physical servers. EnduroX provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, including boolean expressions. Other Tuxedo specific APIs are supported, such as tpforward() and work with distributed transactions (tpbegin(), tpcommit(), etc.).

EnduroX have binddings for:
- Golang (client & server)
- PHP (client)
- Perl (client & server)
- Python (client & server)
- Node.js (client)

![Alt text](doc/Endurox-product.jpg?raw=true "Enduro/x overview")

# Load balancing

![Alt text](doc/endurox-load-balance.jpg?raw=true "Enduro/x service load balancer")

Supported operating system: GNU/Linux, starting from 2.6.12 kernel (needed for POSIX Queues). Starting with Enduro/X Version 3.1.2 IBM AIX (6.1 and 7.1), Oracle Solaris 11, MAC OS X (experimental) and Cygwin (experimental) support is added. Supported compilers: gcc, LLVM clang, IBM xlC.

- Build and installation guides are located at: www.endurox.org/dokuwiki
 
- Support forum: http://www.endurox.org/projects/endurox/boards

- Binary builds (RPMs & DEBs) for various platforms are here: http://www.endurox.org/projects/endurox/files

- Documentation available here: http://www.endurox.org/dokuwiki/doku.php

- Source corss reference: http://www.silodev.com/lxr/source

PS, feel free to contact me at madars@atrbaltic.com. I will gladly help you to get EnduroX running ;)

# Call/message forwarding

## Instead of doing calls to each server separately...

![Alt text](doc/exforward_tpcall.png?raw=true "Classical service orchestration")

This is typciall way for example if doing micro-services with HTTP/REST. You need to do the calls to each service separately. And that is extra overhead to system (multiple returns) and the caller must orchestrate the all calls.

## ...Enduro/X offers much effective way - tpforward() where request is passed around the system

![Alt text](doc/exforward_forward.png?raw=true "Enhanced service orchestration by forwarding the call")

In this case the destination service orchestrates the system, it is up to service to choose the next service which will continue the call processing. The caller is not aware of final service which will do the reply back (tpreturn()).

Note that service after doing "tpforward()" becomes idle and can consume next call.

# High availability and self healing

Enduro/X is capable of detecting stalled/hanged XATMI servers and gracefully can reboot them. The ping times are configurable on per server basis. In case if main daemon dies, it is possible to configure special XATMI server named "tprecover" which monitors "ndrxd" and vice versa, ndrxd can monitor tprecover. Thus if any of these services are not cleanly shut down, then system will heal it self, and boot service/daemons back.

The ndrxd learning mode is some period of time (configurable) in which ndrxd will request all the system for active servers and their services, to get back the view of the system and be in control. Note that system is able to work even with out ndrxd, but at that time no server healing and management will be done.

![Alt text](doc/server_monitoring_and_recovery.png?raw=true "Enduro/X high availability facility")

# Real time system patching

It is possible to patch the Enduro/X system (update stateless XATMI servers) without service interruption. This is achieved by following scheme:

![Alt text](doc/rt-patching.png?raw=true "Enduro/X real time patching")

# Performance

Due to fact that Enduro/X uses memory based queues, performance numbers are quite high:

## ATMI client calls server in async way (tpacall(), no reply from server)

![Alt text](doc/benchmark/04_tpacall.png?raw=true "Local tpcall() performance")


## Local one client calls server, and gets back response:

![Alt text](doc/benchmark/01_tpcall.png?raw=true "Local tpcall() performance")

## Multiple clients calls multiple servers

![Alt text](doc/benchmark/03_tpcall_threads.png?raw=true "Multiprocessing tpcall() performance")

## Single client calls single server via network (two app servers bridged)

![Alt text](doc/benchmark/02_tpcall_network.png?raw=true "Network tpcall() performance")

## Persistent storage (message enqueue to disk via tpenqueue())
The number here are lower because messages are being saved to disk. Also internally XA transaction is used, which also requires logging to stable storage.

![Alt text](doc/benchmark/05_persistent_storage.png?raw=true "Network tpenqueue() performance")



# Releases

- Version 2.5.1 released on 18/05/2016. Support for transactional persistent message queues. Provides APIs: tpenqueue(), tpdequeue() and command line management tools. See doc/persistent_message_queues_overview.adoc, doc/manpage/tmqueue.adoc, doc/manpage/xadmin.adoc, doc/manpage/q.config.adoc. Use cases can be seen under atmitests/test028_tmq
- Version 3.1.2 released on 25/06/2016. Added support for other Unix systems. Currently production grade support is provided for following operating systems: GNU/Linux, FreeBSD, IBM AIX and Oracle Solaris. MAC OS X and Cygwin versions are considered as experimental. FreeBSD installation guides are located at doc/freebsd_notes.adoc, AIX install guide: doc/aix_notes.adoc, Solaris install guide: doc/solaris_notes.adoc and OS X guide: doc/osx_notes.adoc. Also with this version lot of core processing bug fixes are made.
- Version 3.2.1 released on 06/07/2016. Major UBF optimization. Now for each data type memory offset is maintained. For fixed data types (short, long, char, float, double) binary search is performend when field is read from buffer.
- Version 3.2.2 released on 15/07/2016. Bugfixes for UBF binary search. Added UBF benchmarking scripts and documents.
- Version 3.3.1 released on 05/09/2016. Implemented common configuration engine (can use ini files in alternative to environment variables, debug config and persistent queue config).
- Version 3.3.2 released on 01/10/2016. Works on TP logger, user accessible logging framework. Set of tplog\* functions.
- Version 3.3.5 released on 10/01/2017. Fixes for RPM build and install on Red-hat enterprise linux platform.
- Version 3.3.6 released on 12/01/2017. Fixes for integration mode, changes in tpcontinue().
- Version 3.4.1 released on 13/01/2017. New ATMI server flag for developer - reloadonchange - if cksum change changed on binary issue sreload.
- Version 3.4.2 released on 20/01/2017. Network protocol framing moved from 2 bytes to 4 bytes.
- Version 3.4.3 released on 26/01/2017. Memory leak fixes for tmsrv and tmqueue servers.

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
- To trace the Object-API use '-DNDRX_OAPI_DEBUG=1' 

