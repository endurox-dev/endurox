# endurox
EnduroX is Open Source Middleware Platform for Distributed Transaction Processing

Platform provides X/Open XATMI and XA APIs for C/C++ applications. EnduroX can be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), Hitache OpenTP1 and other XATMI middlewares.

EnduroX provides SOA architecture for C/C++ applications and allows to cluster application in fault tolerant way over multiple physical servers. EnduroX provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, including boolean expressions. Other Tuxedo specific APIs are supported, such as tpforward() and work with distributed transactions (tpbegin(), tpcommit(), etc.).

EnduroX have binddings for:
- Golang (client & server)
- PHP (client)
- Perl (client & server)
- Python (client & server)
- Node.js (client)

![Alt text](doc/Endurox-product.jpg?raw=true "Enduro/x overview")

Supported operating system: GNU/Linux, starting from 2.6.12 kernel (needed for POSIX Queues). Starting with Enduro/X Version 3.1.2 IBM AIX (6.1 and 7.1), Oracle Solaris 11, MAC OS X (experimental) and Cygwin (experimental) support is added. Supported compilers: gcc, LLVM clang, IBM xlC.

- Build and installation guides are located at: www.endurox.org/dokuwiki
 
- Support forum: http://www.endurox.org/projects/endurox/boards

- Binary builds (RPMs & DEBs) for various platforms are here: http://www.endurox.org/projects/endurox/files

- Documentation available here: http://www.endurox.org/dokuwiki/doku.php

PS, feel free to contact me at madars@atrbaltic.com. I will gladly help you to get EnduroX running ;)

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

