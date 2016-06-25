# endurox
EnduroX is Open Source Middleware Platform for Distributed Transaction Processing

Platform provides X/Open XATMI and XA APIs for C/C++ applications. EnduroX can be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), Hitache OpenTP1 and other XATMI middlewares.

EnduroX provides SOA architecture for C/C++ applications and allows to cluster application in fault tolerant way over multiple physical servers. EnduroX provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, including boolean expressions. Other Tuxedo specific APIs are supported, such as tpforward() and work with distributed transactions (tpbegin(), tpcommit(), etc.).

Supported operating system: GNU/Linux, starting from 2.6.12 kernel (needed for POSIX Queues). Starting with Enduro/X Version 3.1.2 IBM AIX (6.1 and 7.1), Oracle Solaris 11, MAC OS X (experimental) and Cygwin (experimental) support is added. Supported compilers: gcc, LLVM clang, IBM xlC.

- Build and installation guides are located at: http://www.endurox.org

- Binary builds (RPMs & DEBs) for various platforms are here: http://www.endurox.org/projects/endurox/files

- Documentation available here: http://www.endurox.org/dokuwiki/doku.php

PS, feel free to contact me at madars@atrbaltic.com. I will gladly help you to get EnduroX running ;)

# Releases

- Version 2.5.1 released on 18/05/2016. Support for transactional persistent message queues. Provides APIs: tpenqueue(), tpdequeue() and command line management tools. See doc/persistent_message_queues_overview.adoc, doc/manpage/tmqueue.adoc, doc/manpage/xadmin.adoc, doc/manpage/q.config.adoc. Use cases can be seen under atmitests/test028_tmq
- Version 3.1.2 released on 25/06/2016. Added support for other Unix systems. Currently at production grade supported OSes are IBM AIX and Oracle Solaris. MAC OS X and Cygwin versions are considered as experimental. IBM AIX installation guides are located at doc/aix_notes.adoc, Solaris install guide: doc/solaris_notes.adoc, OS X guide: doc/osx_notes.adoc. Also with this version lot of core processing bug fixes are done.
