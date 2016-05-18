# endurox
EnduroX is Open Source Middleware Platform for Distributed Transaction Processing

Platform provides X/Open XATMI and XA APIs for C/C++ applications. EnduroX can be considered as replacement for Oracle(R) Tuxedo(R), Jboss Blacktie (Narayan), Hitache OpenTP1 and other XATMI middlewares.

EnduroX provides SOA architecture for C/C++ applications and allows to cluster application in fault tolerant way over multiple physical servers. EnduroX provides Oracle(R) Tuxedo(R) FML/FML32 library emulation, including boolean expressions. Other Tuxedo specific APIs are supported, such as tpforward() and work with distributed transactions (tpbegin(), tpcommit(), etc.).

Supported operating system: GNU/Linux, starting from 2.6.12 kernel (needed for POSIX Queues).

- Build and installation guides are located at: http://www.endurox.org

- Binary builds (RPMs & DEBs) for various platforms are here: http://www.endurox.org/projects/endurox/files

- Documentation available here: http://www.endurox.org/dokuwiki/doku.php

PS, feel free to contact me at madars@atrbaltic.com. I will gladly help you to get EnduroX running ;)

# Releases

- Version 2.5.1 released on 18/05/2016. Support for transactional persistent message queues. Provides APIs: tpenqueue(), tpdequeue(), command line management tools. See doc/persistent_message_queues_overview.adoc, doc/manpage/tmqueue.adoc, doc/manpage/xadmin.adoc, doc/manpage/q.config.adoc. Use cases can be seen under atmitests/test028_tmq
