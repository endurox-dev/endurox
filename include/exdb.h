/** @file exdb.h
 *	@brief Lightning memory-mapped database library
 *
 *	@mainpage	Lightning Memory-Mapped Database Manager (EXDB)
 *
 *	@section intro_sec Introduction
 *	EXDB is a Btree-based database management library modeled loosely on the
 *	BerkeleyDB API, but much simplified. The entire database is exposed
 *	in a memory map, and all data fetches return data directly
 *	from the mapped memory, so no malloc's or memcpy's occur during
 *	data fetches. As such, the library is extremely simple because it
 *	requires no page caching layer of its own, and it is extremely high
 *	performance and memory-efficient. It is also fully transactional with
 *	full ACID semantics, and when the memory map is read-only, the
 *	database integrity cannot be corrupted by stray pointer writes from
 *	application code.
 *
 *	The library is fully thread-aware and supports concurrent read/write
 *	access from multiple processes and threads. Data pages use a copy-on-
 *	write strategy so no active data pages are ever overwritten, which
 *	also provides resistance to corruption and eliminates the need of any
 *	special recovery procedures after a system crash. Writes are fully
 *	serialized; only one write transaction may be active at a time, which
 *	guarantees that writers can never deadlock. The database structure is
 *	multi-versioned so readers run with no locks; writers cannot block
 *	readers, and readers don't block writers.
 *
 *	Unlike other well-known database mechanisms which use either write-ahead
 *	transaction logs or append-only data writes, EXDB requires no maintenance
 *	during operation. Both write-ahead loggers and append-only databases
 *	require periodic checkpointing and/or compaction of their log or database
 *	files otherwise they grow without bound. EXDB tracks free pages within
 *	the database and re-uses them for new write operations, so the database
 *	size does not grow without bound in normal use.
 *
 *	The memory map can be used as a read-only or read-write map. It is
 *	read-only by default as this provides total immunity to corruption.
 *	Using read-write mode offers much higher write performance, but adds
 *	the possibility for stray application writes thru pointers to silently
 *	corrupt the database. Of course if your application code is known to
 *	be bug-free (...) then this is not an issue.
 *
 *	If this is your first time using a transactional embedded key/value
 *	store, you may find the \ref starting page to be helpful.
 *
 *	@section caveats_sec Caveats
 *	Troubleshooting the lock file, plus semaphores on BSD systems:
 *
 *	- A broken lockfile can cause sync issues.
 *	  Stale reader transactions left behind by an aborted program
 *	  cause further writes to grow the database quickly, and
 *	  stale locks can block further operation.
 *
 *	  Fix: Check for stale readers periodically, using the
 *	  #edb_reader_check function or the \ref edb_stat_1 "edb_stat" tool.
 *	  Stale writers will be cleared automatically on most systems:
 *	  - Windows - automatic
 *	  - BSD, systems using SysV semaphores - automatic
 *	  - Linux, systems using POSIX mutexes with Robust option - automatic
 *	  Otherwise just make all programs using the database close it;
 *	  the lockfile is always reset on first open of the environment.
 *
 *	- On BSD systems or others configured with EDB_USE_SYSV_SEM or
 *	  EDB_USE_POSIX_SEM,
 *	  startup can fail due to semaphores owned by another userid.
 *
 *	  Fix: Open and close the database as the user which owns the
 *	  semaphores (likely last user) or as root, while no other
 *	  process is using the database.
 *
 *	Restrictions/caveats (in addition to those listed for some functions):
 *
 *	- Only the database owner should normally use the database on
 *	  BSD systems or when otherwise configured with EDB_USE_POSIX_SEM.
 *	  Multiple users can cause startup to fail later, as noted above.
 *
 *	- There is normally no pure read-only mode, since readers need write
 *	  access to locks and lock file. Exceptions: On read-only filesystems
 *	  or with the #EDB_NOLOCK flag described under #edb_env_open().
 *
 *	- An EXDB configuration will often reserve considerable \b unused
 *	  memory address space and maybe file size for future growth.
 *	  This does not use actual memory or disk space, but users may need
 *	  to understand the difference so they won't be scared off.
 *
 *	- By default, in versions before 0.9.10, unused portions of the data
 *	  file might receive garbage data from memory freed by other code.
 *	  (This does not happen when using the #EDB_WRITEMAP flag.) As of
 *	  0.9.10 the default behavior is to initialize such memory before
 *	  writing to the data file. Since there may be a slight performance
 *	  cost due to this initialization, applications may disable it using
 *	  the #EDB_NOMEMINIT flag. Applications handling sensitive data
 *	  which must not be written should not use this flag. This flag is
 *	  irrelevant when using #EDB_WRITEMAP.
 *
 *	- A thread can only use one transaction at a time, plus any child
 *	  transactions.  Each transaction belongs to one thread.  See below.
 *	  The #EDB_NOTLS flag changes this for read-only transactions.
 *
 *	- Use an EDB_env* in the process which opened it, not after fork().
 *
 *	- Do not have open an EXDB database twice in the same process at
 *	  the same time.  Not even from a plain open() call - close()ing it
 *	  breaks fcntl() advisory locking.  (It is OK to reopen it after
 *	  fork() - exec*(), since the lockfile has FD_CLOEXEC set.)
 *
 *	- Avoid long-lived transactions.  Read transactions prevent
 *	  reuse of pages freed by newer write transactions, thus the
 *	  database can grow quickly.  Write transactions prevent
 *	  other write transactions, since writes are serialized.
 *
 *	- Avoid suspending a process with active transactions.  These
 *	  would then be "long-lived" as above.  Also read transactions
 *	  suspended when writers commit could sometimes see wrong data.
 *
 *	...when several processes can use a database concurrently:
 *
 *	- Avoid aborting a process with an active transaction.
 *	  The transaction becomes "long-lived" as above until a check
 *	  for stale readers is performed or the lockfile is reset,
 *	  since the process may not remove it from the lockfile.
 *
 *	  This does not apply to write transactions if the system clears
 *	  stale writers, see above.
 *
 *	- If you do that anyway, do a periodic check for stale readers. Or
 *	  close the environment once in a while, so the lockfile can get reset.
 *
 *	- Do not use EXDB databases on remote filesystems, even between
 *	  processes on the same host.  This breaks flock() on some OSes,
 *	  possibly memory map sync, and certainly sync between programs
 *	  on different hosts.
 *
 *	- Opening a database can fail if another process is opening or
 *	  closing it at exactly the same time.
 *
 *	@author	Howard Chu, Symas Corporation.
 *
 *	@copyright Copyright 2011-2017 Howard Chu, Symas Corp. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 *
 *	@par Derived From:
 * This code is derived from btree.c written by Martin Hedenfalk.
 *
 * Copyright (c) 2009, 2010 Martin Hedenfalk <martin@bzero.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _EXDB_H_
#define _EXDB_H_

#include <sys/types.h>
#include <inttypes.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Unix permissions for creating files, or dummy definition for Windows */
#ifdef _MSC_VER
typedef	int	edb_mode_t;
#else
typedef	mode_t	edb_mode_t;
#endif

#ifdef _WIN32
# define EDB_FMT_Z	"I"
#else
# define EDB_FMT_Z	"z"			/**< printf/scanf format modifier for size_t */
#endif

#ifndef EDB_VL32
/** Unsigned type used for mapsize, entry counts and page/transaction IDs.
 *
 *	It is normally size_t, hence the name. Defining EDB_VL32 makes it
 *	uint64_t, but do not try this unless you know what you are doing.
 */
typedef size_t	edb_size_t;
# define EDB_SIZE_MAX	SIZE_MAX	/**< max #edb_size_t */
/** #edb_size_t printf formats, \b t = one of [diouxX] without quotes */
# define EDB_PRIy(t)	EDB_FMT_Z #t
/** #edb_size_t scanf formats, \b t = one of [dioux] without quotes */
# define EDB_SCNy(t)	EDB_FMT_Z #t
#else
typedef uint64_t	edb_size_t;
# define EDB_SIZE_MAX	UINT64_MAX
# define EDB_PRIy(t)	PRI##t##64
# define EDB_SCNy(t)	SCN##t##64
# define edb_env_create	edb_env_create_vl32	/**< Prevent mixing with non-VL32 builds */
#endif

/** An abstraction for a file handle.
 *	On POSIX systems file handles are small integers. On Windows
 *	they're opaque pointers.
 */
#ifdef _WIN32
typedef	void *edb_filehandle_t;
#else
typedef int edb_filehandle_t;
#endif

/** @defgroup edb EXDB API
 *	@{
 *	@brief OpenLDAP Lightning Memory-Mapped Database Manager
 */
/** @defgroup Version Version Macros
 *	@{
 */
/** Library major version */
#define EDB_VERSION_MAJOR	0
/** Library minor version */
#define EDB_VERSION_MINOR	9
/** Library patch version */
#define EDB_VERSION_PATCH	70

/** Combine args a,b,c into a single integer for easy version comparisons */
#define EDB_VERINT(a,b,c)	(((a) << 24) | ((b) << 16) | (c))

/** The full library version as a single integer */
#define EDB_VERSION_FULL	\
	EDB_VERINT(EDB_VERSION_MAJOR,EDB_VERSION_MINOR,EDB_VERSION_PATCH)

/** The release date of this library version */
#define EDB_VERSION_DATE	"December 19, 2015"

/** A stringifier for the version info */
#define EDB_VERSTR(a,b,c,d)	"EXDB " #a "." #b "." #c ": (" d ")"

/** A helper for the stringifier macro */
#define EDB_VERFOO(a,b,c,d)	EDB_VERSTR(a,b,c,d)

/** The full library version as a C string */
#define	EDB_VERSION_STRING	\
	EDB_VERFOO(EDB_VERSION_MAJOR,EDB_VERSION_MINOR,EDB_VERSION_PATCH,EDB_VERSION_DATE)
/**	@} */

/** @brief Opaque structure for a database environment.
 *
 * A DB environment supports multiple databases, all residing in the same
 * shared-memory map.
 */
typedef struct EDB_env EDB_env;

/** @brief Opaque structure for a transaction handle.
 *
 * All database operations require a transaction handle. Transactions may be
 * read-only or read-write.
 */
typedef struct EDB_txn EDB_txn;

/** @brief A handle for an individual database in the DB environment. */
typedef unsigned int	EDB_dbi;

/** @brief Opaque structure for navigating through a database */
typedef struct EDB_cursor EDB_cursor;

/** @brief Generic structure used for passing keys and data in and out
 * of the database.
 *
 * Values returned from the database are valid only until a subsequent
 * update operation, or the end of the transaction. Do not modify or
 * free them, they commonly point into the database itself.
 *
 * Key sizes must be between 1 and #edb_env_get_maxkeysize() inclusive.
 * The same applies to data sizes in databases with the #EDB_DUPSORT flag.
 * Other data items can in theory be from 0 to 0xffffffff bytes long.
 */
typedef struct EDB_val {
	size_t		 mv_size;	/**< size of the data item */
	void		*mv_data;	/**< address of the data item */
} EDB_val;

/** @brief A callback function used to compare two keys in a database */
typedef int  (EDB_cmp_func)(const EDB_val *a, const EDB_val *b);

/** @brief A callback function used to relocate a position-dependent data item
 * in a fixed-address database.
 *
 * The \b newptr gives the item's desired address in
 * the memory map, and \b oldptr gives its previous address. The item's actual
 * data resides at the address in \b item.  This callback is expected to walk
 * through the fields of the record in \b item and modify any
 * values based at the \b oldptr address to be relative to the \b newptr address.
 * @param[in,out] item The item that is to be relocated.
 * @param[in] oldptr The previous address.
 * @param[in] newptr The new address to relocate to.
 * @param[in] relctx An application-provided context, set by #edb_set_relctx().
 * @todo This feature is currently unimplemented.
 */
typedef void (EDB_rel_func)(EDB_val *item, void *oldptr, void *newptr, void *relctx);

/** @defgroup	edb_env	Environment Flags
 *	@{
 */
	/** mmap at a fixed address (experimental) */
#define EDB_FIXEDMAP	0x01
	/** no environment directory */
#define EDB_NOSUBDIR	0x4000
	/** don't fsync after commit */
#define EDB_NOSYNC		0x10000
	/** read only */
#define EDB_RDONLY		0x20000
	/** don't fsync metapage after commit */
#define EDB_NOMETASYNC		0x40000
	/** use writable mmap */
#define EDB_WRITEMAP		0x80000
	/** use asynchronous msync when #EDB_WRITEMAP is used */
#define EDB_MAPASYNC		0x100000
	/** tie reader locktable slots to #EDB_txn objects instead of to threads */
#define EDB_NOTLS		0x200000
	/** don't do any locking, caller must manage their own locks */
#define EDB_NOLOCK		0x400000
	/** don't do readahead (no effect on Windows) */
#define EDB_NORDAHEAD	0x800000
	/** don't initialize malloc'd memory before writing to datafile */
#define EDB_NOMEMINIT	0x1000000
	/** use the previous meta page rather than the latest one */
#define EDB_PREVMETA	0x2000000
/** @} */

/**	@defgroup	edb_dbi_open	Database Flags
 *	@{
 */
	/** use reverse string keys */
#define EDB_REVERSEKEY	0x02
	/** use sorted duplicates */
#define EDB_DUPSORT		0x04
	/** numeric keys in native byte order, either unsigned int or #edb_size_t.
	 *	(exdb expects 32-bit int <= size_t <= 32/64-bit edb_size_t.)
	 *  The keys must all be of the same size. */
#define EDB_INTEGERKEY	0x08
	/** with #EDB_DUPSORT, sorted dup items have fixed size */
#define EDB_DUPFIXED	0x10
	/** with #EDB_DUPSORT, dups are #EDB_INTEGERKEY-style integers */
#define EDB_INTEGERDUP	0x20
	/** with #EDB_DUPSORT, use reverse string dups */
#define EDB_REVERSEDUP	0x40
	/** create DB if not already existing */
#define EDB_CREATE		0x40000
/** @} */

/**	@defgroup edb_put	Write Flags
 *	@{
 */
/** For put: Don't write if the key already exists. */
#define EDB_NOOVERWRITE	0x10
/** Only for #EDB_DUPSORT<br>
 * For put: don't write if the key and data pair already exist.<br>
 * For edb_cursor_del: remove all duplicate data items.
 */
#define EDB_NODUPDATA	0x20
/** For edb_cursor_put: overwrite the current key/data pair */
#define EDB_CURRENT	0x40
/** For put: Just reserve space for data, don't copy it. Return a
 * pointer to the reserved space.
 */
#define EDB_RESERVE	0x10000
/** Data is being appended, don't split full pages. */
#define EDB_APPEND	0x20000
/** Duplicate data is being appended, don't split full pages. */
#define EDB_APPENDDUP	0x40000
/** Store multiple data items in one call. Only for #EDB_DUPFIXED. */
#define EDB_MULTIPLE	0x80000
/*	@} */

/**	@defgroup edb_copy	Copy Flags
 *	@{
 */
/** Compacting copy: Omit free space from copy, and renumber all
 * pages sequentially.
 */
#define EDB_CP_COMPACT	0x01
/*	@} */

/** @brief Cursor Get operations.
 *
 *	This is the set of all operations for retrieving data
 *	using a cursor.
 */
typedef enum EDB_cursor_op {
	EDB_FIRST,				/**< Position at first key/data item */
	EDB_FIRST_DUP,			/**< Position at first data item of current key.
								Only for #EDB_DUPSORT */
	EDB_GET_BOTH,			/**< Position at key/data pair. Only for #EDB_DUPSORT */
	EDB_GET_BOTH_RANGE,		/**< position at key, nearest data. Only for #EDB_DUPSORT */
	EDB_GET_CURRENT,		/**< Return key/data at current cursor position */
	EDB_GET_MULTIPLE,		/**< Return key and up to a page of duplicate data items
								from current cursor position. Move cursor to prepare
								for #EDB_NEXT_MULTIPLE. Only for #EDB_DUPFIXED */
	EDB_LAST,				/**< Position at last key/data item */
	EDB_LAST_DUP,			/**< Position at last data item of current key.
								Only for #EDB_DUPSORT */
	EDB_NEXT,				/**< Position at next data item */
	EDB_NEXT_DUP,			/**< Position at next data item of current key.
								Only for #EDB_DUPSORT */
	EDB_NEXT_MULTIPLE,		/**< Return key and up to a page of duplicate data items
								from next cursor position. Move cursor to prepare
								for #EDB_NEXT_MULTIPLE. Only for #EDB_DUPFIXED */
	EDB_NEXT_NODUP,			/**< Position at first data item of next key */
	EDB_PREV,				/**< Position at previous data item */
	EDB_PREV_DUP,			/**< Position at previous data item of current key.
								Only for #EDB_DUPSORT */
	EDB_PREV_NODUP,			/**< Position at last data item of previous key */
	EDB_SET,				/**< Position at specified key */
	EDB_SET_KEY,			/**< Position at specified key, return key + data */
	EDB_SET_RANGE,			/**< Position at first key greater than or equal to specified key. */
	EDB_PREV_MULTIPLE		/**< Position at previous page and return key and up to
								a page of duplicate data items. Only for #EDB_DUPFIXED */
} EDB_cursor_op;

/** @defgroup  errors	Return Codes
 *
 *	BerkeleyDB uses -30800 to -30999, we'll go under them
 *	@{
 */
	/**	Successful result */
#define EDB_SUCCESS	 0
	/** key/data pair already exists */
#define EDB_KEYEXIST	(-30799)
	/** key/data pair not found (EOF) */
#define EDB_NOTFOUND	(-30798)
	/** Requested page not found - this usually indicates corruption */
#define EDB_PAGE_NOTFOUND	(-30797)
	/** Located page was wrong type */
#define EDB_CORRUPTED	(-30796)
	/** Update of meta page failed or environment had fatal error */
#define EDB_PANIC		(-30795)
	/** Environment version mismatch */
#define EDB_VERSION_MISMATCH	(-30794)
	/** File is not a valid EXDB file */
#define EDB_INVALID	(-30793)
	/** Environment mapsize reached */
#define EDB_MAP_FULL	(-30792)
	/** Environment maxdbs reached */
#define EDB_DBS_FULL	(-30791)
	/** Environment maxreaders reached */
#define EDB_READERS_FULL	(-30790)
	/** Too many TLS keys in use - Windows only */
#define EDB_TLS_FULL	(-30789)
	/** Txn has too many dirty pages */
#define EDB_TXN_FULL	(-30788)
	/** Cursor stack too deep - internal error */
#define EDB_CURSOR_FULL	(-30787)
	/** Page has not enough space - internal error */
#define EDB_PAGE_FULL	(-30786)
	/** Database contents grew beyond environment mapsize */
#define EDB_MAP_RESIZED	(-30785)
	/** Operation and DB incompatible, or DB type changed. This can mean:
	 *	<ul>
	 *	<li>The operation expects an #EDB_DUPSORT / #EDB_DUPFIXED database.
	 *	<li>Opening a named DB when the unnamed DB has #EDB_DUPSORT / #EDB_INTEGERKEY.
	 *	<li>Accessing a data record as a database, or vice versa.
	 *	<li>The database was dropped and recreated with different flags.
	 *	</ul>
	 */
#define EDB_INCOMPATIBLE	(-30784)
	/** Invalid reuse of reader locktable slot */
#define EDB_BAD_RSLOT		(-30783)
	/** Transaction must abort, has a child, or is invalid */
#define EDB_BAD_TXN			(-30782)
	/** Unsupported size of key/DB name/data, or wrong DUPFIXED size */
#define EDB_BAD_VALSIZE		(-30781)
	/** The specified DBI was changed unexpectedly */
#define EDB_BAD_DBI		(-30780)
	/** Unexpected problem - txn should abort */
#define EDB_PROBLEM		(-30779)
	/** The last defined error code */
#define EDB_LAST_ERRCODE	EDB_PROBLEM
/** @} */

/** @brief Statistics for a database in the environment */
typedef struct EDB_stat {
	unsigned int	ms_psize;			/**< Size of a database page.
											This is currently the same for all databases. */
	unsigned int	ms_depth;			/**< Depth (height) of the B-tree */
	edb_size_t		ms_branch_pages;	/**< Number of internal (non-leaf) pages */
	edb_size_t		ms_leaf_pages;		/**< Number of leaf pages */
	edb_size_t		ms_overflow_pages;	/**< Number of overflow pages */
	edb_size_t		ms_entries;			/**< Number of data items */
} EDB_stat;

/** @brief Information about the environment */
typedef struct EDB_envinfo {
	void	*me_mapaddr;			/**< Address of map, if fixed */
	edb_size_t	me_mapsize;				/**< Size of the data memory map */
	edb_size_t	me_last_pgno;			/**< ID of the last used page */
	edb_size_t	me_last_txnid;			/**< ID of the last committed transaction */
	unsigned int me_maxreaders;		/**< max reader slots in the environment */
	unsigned int me_numreaders;		/**< max reader slots used in the environment */
} EDB_envinfo;

	/** @brief Return the EXDB library version information.
	 *
	 * @param[out] major if non-NULL, the library major version number is copied here
	 * @param[out] minor if non-NULL, the library minor version number is copied here
	 * @param[out] patch if non-NULL, the library patch version number is copied here
	 * @retval "version string" The library version as a string
	 */
char *edb_version(int *major, int *minor, int *patch);

	/** @brief Return a string describing a given error code.
	 *
	 * This function is a superset of the ANSI C X3.159-1989 (ANSI C) strerror(3)
	 * function. If the error code is greater than or equal to 0, then the string
	 * returned by the system function strerror(3) is returned. If the error code
	 * is less than 0, an error string corresponding to the EXDB library error is
	 * returned. See @ref errors for a list of EXDB-specific error codes.
	 * @param[in] err The error code
	 * @retval "error message" The description of the error
	 */
char *edb_strerror(int err);

	/** @brief Create an EXDB environment handle.
	 *
	 * This function allocates memory for a #EDB_env structure. To release
	 * the allocated memory and discard the handle, call #edb_env_close().
	 * Before the handle may be used, it must be opened using #edb_env_open().
	 * Various other options may also need to be set before opening the handle,
	 * e.g. #edb_env_set_mapsize(), #edb_env_set_maxreaders(), #edb_env_set_maxdbs(),
	 * depending on usage requirements.
	 * @param[out] env The address where the new handle will be stored
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_create(EDB_env **env);

	/** @brief Open an environment handle.
	 *
	 * If this function fails, #edb_env_close() must be called to discard the #EDB_env handle.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] path The directory in which the database files reside. This
	 * directory must already exist and be writable.
	 * @param[in] flags Special options for this environment. This parameter
	 * must be set to 0 or by bitwise OR'ing together one or more of the
	 * values described here.
	 * Flags set by edb_env_set_flags() are also used.
	 * <ul>
	 *	<li>#EDB_FIXEDMAP
	 *      use a fixed address for the mmap region. This flag must be specified
	 *      when creating the environment, and is stored persistently in the environment.
	 *		If successful, the memory map will always reside at the same virtual address
	 *		and pointers used to reference data items in the database will be constant
	 *		across multiple invocations. This option may not always work, depending on
	 *		how the operating system has allocated memory to shared libraries and other uses.
	 *		The feature is highly experimental.
	 *	<li>#EDB_NOSUBDIR
	 *		By default, EXDB creates its environment in a directory whose
	 *		pathname is given in \b path, and creates its data and lock files
	 *		under that directory. With this option, \b path is used as-is for
	 *		the database main data file. The database lock file is the \b path
	 *		with "-lock" appended.
	 *	<li>#EDB_RDONLY
	 *		Open the environment in read-only mode. No write operations will be
	 *		allowed. EXDB will still modify the lock file - except on read-only
	 *		filesystems, where EXDB does not use locks.
	 *	<li>#EDB_WRITEMAP
	 *		Use a writeable memory map unless EDB_RDONLY is set. This uses
	 *		fewer mallocs but loses protection from application bugs
	 *		like wild pointer writes and other bad updates into the database.
	 *		This may be slightly faster for DBs that fit entirely in RAM, but
	 *		is slower for DBs larger than RAM.
	 *		Incompatible with nested transactions.
	 *		Do not mix processes with and without EDB_WRITEMAP on the same
	 *		environment.  This can defeat durability (#edb_env_sync etc).
	 *	<li>#EDB_NOMETASYNC
	 *		Flush system buffers to disk only once per transaction, omit the
	 *		metadata flush. Defer that until the system flushes files to disk,
	 *		or next non-EDB_RDONLY commit or #edb_env_sync(). This optimization
	 *		maintains database integrity, but a system crash may undo the last
	 *		committed transaction. I.e. it preserves the ACI (atomicity,
	 *		consistency, isolation) but not D (durability) database property.
	 *		This flag may be changed at any time using #edb_env_set_flags().
	 *	<li>#EDB_NOSYNC
	 *		Don't flush system buffers to disk when committing a transaction.
	 *		This optimization means a system crash can corrupt the database or
	 *		lose the last transactions if buffers are not yet flushed to disk.
	 *		The risk is governed by how often the system flushes dirty buffers
	 *		to disk and how often #edb_env_sync() is called.  However, if the
	 *		filesystem preserves write order and the #EDB_WRITEMAP flag is not
	 *		used, transactions exhibit ACI (atomicity, consistency, isolation)
	 *		properties and only lose D (durability).  I.e. database integrity
	 *		is maintained, but a system crash may undo the final transactions.
	 *		Note that (#EDB_NOSYNC | #EDB_WRITEMAP) leaves the system with no
	 *		hint for when to write transactions to disk, unless #edb_env_sync()
	 *		is called. (#EDB_MAPASYNC | #EDB_WRITEMAP) may be preferable.
	 *		This flag may be changed at any time using #edb_env_set_flags().
	 *	<li>#EDB_MAPASYNC
	 *		When using #EDB_WRITEMAP, use asynchronous flushes to disk.
	 *		As with #EDB_NOSYNC, a system crash can then corrupt the
	 *		database or lose the last transactions. Calling #edb_env_sync()
	 *		ensures on-disk database integrity until next commit.
	 *		This flag may be changed at any time using #edb_env_set_flags().
	 *	<li>#EDB_NOTLS
	 *		Don't use Thread-Local Storage. Tie reader locktable slots to
	 *		#EDB_txn objects instead of to threads. I.e. #edb_txn_reset() keeps
	 *		the slot reseved for the #EDB_txn object. A thread may use parallel
	 *		read-only transactions. A read-only transaction may span threads if
	 *		the user synchronizes its use. Applications that multiplex many
	 *		user threads over individual OS threads need this option. Such an
	 *		application must also serialize the write transactions in an OS
	 *		thread, since EXDB's write locking is unaware of the user threads.
	 *	<li>#EDB_NOLOCK
	 *		Don't do any locking. If concurrent access is anticipated, the
	 *		caller must manage all concurrency itself. For proper operation
	 *		the caller must enforce single-writer semantics, and must ensure
	 *		that no readers are using old transactions while a writer is
	 *		active. The simplest approach is to use an exclusive lock so that
	 *		no readers may be active at all when a writer begins.
	 *	<li>#EDB_NORDAHEAD
	 *		Turn off readahead. Most operating systems perform readahead on
	 *		read requests by default. This option turns it off if the OS
	 *		supports it. Turning it off may help random read performance
	 *		when the DB is larger than RAM and system RAM is full.
	 *		The option is not implemented on Windows.
	 *	<li>#EDB_NOMEMINIT
	 *		Don't initialize malloc'd memory before writing to unused spaces
	 *		in the data file. By default, memory for pages written to the data
	 *		file is obtained using malloc. While these pages may be reused in
	 *		subsequent transactions, freshly malloc'd pages will be initialized
	 *		to zeroes before use. This avoids persisting leftover data from other
	 *		code (that used the heap and subsequently freed the memory) into the
	 *		data file. Note that many other system libraries may allocate
	 *		and free memory from the heap for arbitrary uses. E.g., stdio may
	 *		use the heap for file I/O buffers. This initialization step has a
	 *		modest performance cost so some applications may want to disable
	 *		it using this flag. This option can be a problem for applications
	 *		which handle sensitive data like passwords, and it makes memory
	 *		checkers like Valgrind noisy. This flag is not needed with #EDB_WRITEMAP,
	 *		which writes directly to the mmap instead of using malloc for pages. The
	 *		initialization is also skipped if #EDB_RESERVE is used; the
	 *		caller is expected to overwrite all of the memory that was
	 *		reserved in that case.
	 *		This flag may be changed at any time using #edb_env_set_flags().
	 *	<li>#EDB_PREVMETA
	 *		Open the environment with the previous meta page rather than the latest
	 *		one. This loses the latest transaction, but may help work around some
	 *		types of corruption.
	 * </ul>
	 * @param[in] mode The UNIX permissions to set on created files and semaphores.
	 * This parameter is ignored on Windows.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_VERSION_MISMATCH - the version of the EXDB library doesn't match the
	 *	version that created the database environment.
	 *	<li>#EDB_INVALID - the environment file headers are corrupted.
	 *	<li>ENOENT - the directory specified by the path parameter doesn't exist.
	 *	<li>EACCES - the user didn't have permission to access the environment files.
	 *	<li>EAGAIN - the environment was locked by another process.
	 * </ul>
	 */
int  edb_env_open(EDB_env *env, const char *path, unsigned int flags, edb_mode_t mode);

	/** @brief Copy an EXDB environment to the specified path.
	 *
	 * This function may be used to make a backup of an existing environment.
	 * No lockfile is created, since it gets recreated at need.
	 * @note This call can trigger significant file size growth if run in
	 * parallel with write transactions, because it employs a read-only
	 * transaction. See long-lived transactions under @ref caveats_sec.
	 * @param[in] env An environment handle returned by #edb_env_create(). It
	 * must have already been opened successfully.
	 * @param[in] path The directory in which the copy will reside. This
	 * directory must already exist and be writable but must otherwise be
	 * empty.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_copy(EDB_env *env, const char *path);

	/** @brief Copy an EXDB environment to the specified file descriptor.
	 *
	 * This function may be used to make a backup of an existing environment.
	 * No lockfile is created, since it gets recreated at need.
	 * @note This call can trigger significant file size growth if run in
	 * parallel with write transactions, because it employs a read-only
	 * transaction. See long-lived transactions under @ref caveats_sec.
	 * @param[in] env An environment handle returned by #edb_env_create(). It
	 * must have already been opened successfully.
	 * @param[in] fd The filedescriptor to write the copy to. It must
	 * have already been opened for Write access.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_copyfd(EDB_env *env, edb_filehandle_t fd);

	/** @brief Copy an EXDB environment to the specified path, with options.
	 *
	 * This function may be used to make a backup of an existing environment.
	 * No lockfile is created, since it gets recreated at need.
	 * @note This call can trigger significant file size growth if run in
	 * parallel with write transactions, because it employs a read-only
	 * transaction. See long-lived transactions under @ref caveats_sec.
	 * @param[in] env An environment handle returned by #edb_env_create(). It
	 * must have already been opened successfully.
	 * @param[in] path The directory in which the copy will reside. This
	 * directory must already exist and be writable but must otherwise be
	 * empty.
	 * @param[in] flags Special options for this operation. This parameter
	 * must be set to 0 or by bitwise OR'ing together one or more of the
	 * values described here.
	 * <ul>
	 *	<li>#EDB_CP_COMPACT - Perform compaction while copying: omit free
	 *		pages and sequentially renumber all pages in output. This option
	 *		consumes more CPU and runs more slowly than the default.
	 *		Currently it fails if the environment has suffered a page leak.
	 * </ul>
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_copy2(EDB_env *env, const char *path, unsigned int flags);

	/** @brief Copy an EXDB environment to the specified file descriptor,
	 *	with options.
	 *
	 * This function may be used to make a backup of an existing environment.
	 * No lockfile is created, since it gets recreated at need. See
	 * #edb_env_copy2() for further details.
	 * @note This call can trigger significant file size growth if run in
	 * parallel with write transactions, because it employs a read-only
	 * transaction. See long-lived transactions under @ref caveats_sec.
	 * @param[in] env An environment handle returned by #edb_env_create(). It
	 * must have already been opened successfully.
	 * @param[in] fd The filedescriptor to write the copy to. It must
	 * have already been opened for Write access.
	 * @param[in] flags Special options for this operation.
	 * See #edb_env_copy2() for options.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_copyfd2(EDB_env *env, edb_filehandle_t fd, unsigned int flags);

	/** @brief Return statistics about the EXDB environment.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] stat The address of an #EDB_stat structure
	 * 	where the statistics will be copied
	 */
int  edb_env_stat(EDB_env *env, EDB_stat *stat);

	/** @brief Return information about the EXDB environment.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] stat The address of an #EDB_envinfo structure
	 * 	where the information will be copied
	 */
int  edb_env_info(EDB_env *env, EDB_envinfo *stat);

	/** @brief Flush the data buffers to disk.
	 *
	 * Data is always written to disk when #edb_txn_commit() is called,
	 * but the operating system may keep it buffered. EXDB always flushes
	 * the OS buffers upon commit as well, unless the environment was
	 * opened with #EDB_NOSYNC or in part #EDB_NOMETASYNC. This call is
	 * not valid if the environment was opened with #EDB_RDONLY.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] force If non-zero, force a synchronous flush.  Otherwise
	 *  if the environment has the #EDB_NOSYNC flag set the flushes
	 *	will be omitted, and with #EDB_MAPASYNC they will be asynchronous.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EACCES - the environment is read-only.
	 *	<li>EINVAL - an invalid parameter was specified.
	 *	<li>EIO - an error occurred during synchronization.
	 * </ul>
	 */
int  edb_env_sync(EDB_env *env, int force);

	/** @brief Close the environment and release the memory map.
	 *
	 * Only a single thread may call this function. All transactions, databases,
	 * and cursors must already be closed before calling this function. Attempts to
	 * use any such handles after calling this function will cause a SIGSEGV.
	 * The environment handle will be freed and must not be used again after this call.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 */
void edb_env_close(EDB_env *env);

	/** @brief Set environment flags.
	 *
	 * This may be used to set some flags in addition to those from
	 * #edb_env_open(), or to unset these flags.  If several threads
	 * change the flags at the same time, the result is undefined.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] flags The flags to change, bitwise OR'ed together
	 * @param[in] onoff A non-zero value sets the flags, zero clears them.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_env_set_flags(EDB_env *env, unsigned int flags, int onoff);

	/** @brief Get environment flags.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] flags The address of an integer to store the flags
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_env_get_flags(EDB_env *env, unsigned int *flags);

	/** @brief Return the path that was used in #edb_env_open().
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] path Address of a string pointer to contain the path. This
	 * is the actual string in the environment, not a copy. It should not be
	 * altered in any way.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_env_get_path(EDB_env *env, const char **path);

	/** @brief Return the filedescriptor for the given environment.
	 *
	 * This function may be called after fork(), so the descriptor can be
	 * closed before exec*().  Other EXDB file descriptors have FD_CLOEXEC.
	 * (Until EXDB 0.9.18, only the lockfile had that.)
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] fd Address of a edb_filehandle_t to contain the descriptor.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_env_get_fd(EDB_env *env, edb_filehandle_t *fd);

	/** @brief Set the size of the memory map to use for this environment.
	 *
	 * The size should be a multiple of the OS page size. The default is
	 * 10485760 bytes. The size of the memory map is also the maximum size
	 * of the database. The value should be chosen as large as possible,
	 * to accommodate future growth of the database.
	 * This function should be called after #edb_env_create() and before #edb_env_open().
	 * It may be called at later times if no transactions are active in
	 * this process. Note that the library does not check for this condition,
	 * the caller must ensure it explicitly.
	 *
	 * The new size takes effect immediately for the current process but
	 * will not be persisted to any others until a write transaction has been
	 * committed by the current process. Also, only mapsize increases are
	 * persisted into the environment.
	 *
	 * If the mapsize is increased by another process, and data has grown
	 * beyond the range of the current mapsize, #edb_txn_begin() will
	 * return #EDB_MAP_RESIZED. This function may be called with a size
	 * of zero to adopt the new size.
	 *
	 * Any attempt to set a size smaller than the space already consumed
	 * by the environment will be silently changed to the current size of the used space.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] size The size in bytes
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified, or the environment has
	 *   	an active write transaction.
	 * </ul>
	 */
int  edb_env_set_mapsize(EDB_env *env, edb_size_t size);

	/** @brief Set the maximum number of threads/reader slots for the environment.
	 *
	 * This defines the number of slots in the lock table that is used to track readers in the
	 * the environment. The default is 126.
	 * Starting a read-only transaction normally ties a lock table slot to the
	 * current thread until the environment closes or the thread exits. If
	 * EDB_NOTLS is in use, #edb_txn_begin() instead ties the slot to the
	 * EDB_txn object until it or the #EDB_env object is destroyed.
	 * This function may only be called after #edb_env_create() and before #edb_env_open().
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] readers The maximum number of reader lock table slots
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified, or the environment is already open.
	 * </ul>
	 */
int  edb_env_set_maxreaders(EDB_env *env, unsigned int readers);

	/** @brief Get the maximum number of threads/reader slots for the environment.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] readers Address of an integer to store the number of readers
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_env_get_maxreaders(EDB_env *env, unsigned int *readers);

	/** @brief Set the maximum number of named databases for the environment.
	 *
	 * This function is only needed if multiple databases will be used in the
	 * environment. Simpler applications that use the environment as a single
	 * unnamed database can ignore this option.
	 * This function may only be called after #edb_env_create() and before #edb_env_open().
	 *
	 * Currently a moderate number of slots are cheap but a huge number gets
	 * expensive: 7-120 words per transaction, and every #edb_dbi_open()
	 * does a linear search of the opened slots.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] dbs The maximum number of databases
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified, or the environment is already open.
	 * </ul>
	 */
int  edb_env_set_maxdbs(EDB_env *env, EDB_dbi dbs);

	/** @brief Get the maximum size of keys and #EDB_DUPSORT data we can write.
	 *
	 * Depends on the compile-time constant #EDB_MAXKEYSIZE. Default 511.
	 * See @ref EDB_val.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @return The maximum size of a key we can write
	 */
int  edb_env_get_maxkeysize(EDB_env *env);

	/** @brief Set application information associated with the #EDB_env.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] ctx An arbitrary pointer for whatever the application needs.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_set_userctx(EDB_env *env, void *ctx);

	/** @brief Get the application information associated with the #EDB_env.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @return The pointer set by #edb_env_set_userctx().
	 */
void *edb_env_get_userctx(EDB_env *env);

	/** @brief A callback function for most EXDB assert() failures,
	 * called before printing the message and aborting.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create().
	 * @param[in] msg The assertion message, not including newline.
	 */
typedef void EDB_assert_func(EDB_env *env, const char *msg);

	/** Set or reset the assert() callback of the environment.
	 * Disabled if libexdb is buillt with NDEBUG.
	 * @note This hack should become obsolete as exdb's error handling matures.
	 * @param[in] env An environment handle returned by #edb_env_create().
	 * @param[in] func An #EDB_assert_func function, or 0.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_env_set_assert(EDB_env *env, EDB_assert_func *func);

	/** @brief Create a transaction for use with the environment.
	 *
	 * The transaction handle may be discarded using #edb_txn_abort() or #edb_txn_commit().
	 * @note A transaction and its cursors must only be used by a single
	 * thread, and a thread may only have a single transaction at a time.
	 * If #EDB_NOTLS is in use, this does not apply to read-only transactions.
	 * @note Cursors may not span transactions.
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] parent If this parameter is non-NULL, the new transaction
	 * will be a nested transaction, with the transaction indicated by \b parent
	 * as its parent. Transactions may be nested to any level. A parent
	 * transaction and its cursors may not issue any other operations than
	 * edb_txn_commit and edb_txn_abort while it has active child transactions.
	 * @param[in] flags Special options for this transaction. This parameter
	 * must be set to 0 or by bitwise OR'ing together one or more of the
	 * values described here.
	 * <ul>
	 *	<li>#EDB_RDONLY
	 *		This transaction will not perform any write operations.
	 *	<li>#EDB_NOSYNC
	 *		Don't flush system buffers to disk when committing this transaction.
	 *	<li>#EDB_NOMETASYNC
	 *		Flush system buffers but omit metadata flush when committing this transaction.
	 * </ul>
	 * @param[out] txn Address where the new #EDB_txn handle will be stored
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_PANIC - a fatal error occurred earlier and the environment
	 *		must be shut down.
	 *	<li>#EDB_MAP_RESIZED - another process wrote data beyond this EDB_env's
	 *		mapsize and this environment's map must be resized as well.
	 *		See #edb_env_set_mapsize().
	 *	<li>#EDB_READERS_FULL - a read-only transaction was requested and
	 *		the reader lock table is full. See #edb_env_set_maxreaders().
	 *	<li>ENOMEM - out of memory.
	 * </ul>
	 */
int  edb_txn_begin(EDB_env *env, EDB_txn *parent, unsigned int flags, EDB_txn **txn);

	/** @brief Returns the transaction's #EDB_env
	 *
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 */
EDB_env *edb_txn_env(EDB_txn *txn);

	/** @brief Return the transaction's ID.
	 *
	 * This returns the identifier associated with this transaction. For a
	 * read-only transaction, this corresponds to the snapshot being read;
	 * concurrent readers will frequently have the same transaction ID.
	 *
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @return A transaction ID, valid if input is an active transaction.
	 */
edb_size_t edb_txn_id(EDB_txn *txn);

	/** @brief Commit all the operations of a transaction into the database.
	 *
	 * The transaction handle is freed. It and its cursors must not be used
	 * again after this call, except with #edb_cursor_renew().
	 * @note Earlier documentation incorrectly said all cursors would be freed.
	 * Only write-transactions free cursors.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 *	<li>ENOSPC - no more disk space.
	 *	<li>EIO - a low-level I/O error occurred while writing.
	 *	<li>ENOMEM - out of memory.
	 * </ul>
	 */
int  edb_txn_commit(EDB_txn *txn);

	/** @brief Abandon all the operations of the transaction instead of saving them.
	 *
	 * The transaction handle is freed. It and its cursors must not be used
	 * again after this call, except with #edb_cursor_renew().
	 * @note Earlier documentation incorrectly said all cursors would be freed.
	 * Only write-transactions free cursors.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 */
void edb_txn_abort(EDB_txn *txn);

	/** @brief Reset a read-only transaction.
	 *
	 * Abort the transaction like #edb_txn_abort(), but keep the transaction
	 * handle. #edb_txn_renew() may reuse the handle. This saves allocation
	 * overhead if the process will start a new read-only transaction soon,
	 * and also locking overhead if #EDB_NOTLS is in use. The reader table
	 * lock is released, but the table slot stays tied to its thread or
	 * #EDB_txn. Use edb_txn_abort() to discard a reset handle, and to free
	 * its lock table slot if EDB_NOTLS is in use.
	 * Cursors opened within the transaction must not be used
	 * again after this call, except with #edb_cursor_renew().
	 * Reader locks generally don't interfere with writers, but they keep old
	 * versions of database pages allocated. Thus they prevent the old pages
	 * from being reused when writers commit new data, and so under heavy load
	 * the database size may grow much more rapidly than otherwise.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 */
void edb_txn_reset(EDB_txn *txn);

	/** @brief Renew a read-only transaction.
	 *
	 * This acquires a new reader lock for a transaction handle that had been
	 * released by #edb_txn_reset(). It must be called before a reset transaction
	 * may be used again.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_PANIC - a fatal error occurred earlier and the environment
	 *		must be shut down.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_txn_renew(EDB_txn *txn);

/** Compat with version <= 0.9.4, avoid clash with libedb from EDB Tools project */
#define edb_open(txn,name,flags,dbi)	edb_dbi_open(txn,name,flags,dbi)
/** Compat with version <= 0.9.4, avoid clash with libedb from EDB Tools project */
#define edb_close(env,dbi)				edb_dbi_close(env,dbi)

	/** @brief Open a database in the environment.
	 *
	 * A database handle denotes the name and parameters of a database,
	 * independently of whether such a database exists.
	 * The database handle may be discarded by calling #edb_dbi_close().
	 * The old database handle is returned if the database was already open.
	 * The handle may only be closed once.
	 *
	 * The database handle will be private to the current transaction until
	 * the transaction is successfully committed. If the transaction is
	 * aborted the handle will be closed automatically.
	 * After a successful commit the handle will reside in the shared
	 * environment, and may be used by other transactions.
	 *
	 * This function must not be called from multiple concurrent
	 * transactions in the same process. A transaction that uses
	 * this function must finish (either commit or abort) before
	 * any other transaction in the process may use this function.
	 *
	 * To use named databases (with name != NULL), #edb_env_set_maxdbs()
	 * must be called before opening the environment.  Database names are
	 * keys in the unnamed database, and may be read but not written.
	 *
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] name The name of the database to open. If only a single
	 * 	database is needed in the environment, this value may be NULL.
	 * @param[in] flags Special options for this database. This parameter
	 * must be set to 0 or by bitwise OR'ing together one or more of the
	 * values described here.
	 * <ul>
	 *	<li>#EDB_REVERSEKEY
	 *		Keys are strings to be compared in reverse order, from the end
	 *		of the strings to the beginning. By default, Keys are treated as strings and
	 *		compared from beginning to end.
	 *	<li>#EDB_DUPSORT
	 *		Duplicate keys may be used in the database. (Or, from another perspective,
	 *		keys may have multiple data items, stored in sorted order.) By default
	 *		keys must be unique and may have only a single data item.
	 *	<li>#EDB_INTEGERKEY
	 *		Keys are binary integers in native byte order, either unsigned int
	 *		or #edb_size_t, and will be sorted as such.
	 *		(exdb expects 32-bit int <= size_t <= 32/64-bit edb_size_t.)
	 *		The keys must all be of the same size.
	 *	<li>#EDB_DUPFIXED
	 *		This flag may only be used in combination with #EDB_DUPSORT. This option
	 *		tells the library that the data items for this database are all the same
	 *		size, which allows further optimizations in storage and retrieval. When
	 *		all data items are the same size, the #EDB_GET_MULTIPLE, #EDB_NEXT_MULTIPLE
	 *		and #EDB_PREV_MULTIPLE cursor operations may be used to retrieve multiple
	 *		items at once.
	 *	<li>#EDB_INTEGERDUP
	 *		This option specifies that duplicate data items are binary integers,
	 *		similar to #EDB_INTEGERKEY keys.
	 *	<li>#EDB_REVERSEDUP
	 *		This option specifies that duplicate data items should be compared as
	 *		strings in reverse order.
	 *	<li>#EDB_CREATE
	 *		Create the named database if it doesn't exist. This option is not
	 *		allowed in a read-only transaction or a read-only environment.
	 * </ul>
	 * @param[out] dbi Address where the new #EDB_dbi handle will be stored
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_NOTFOUND - the specified database doesn't exist in the environment
	 *		and #EDB_CREATE was not specified.
	 *	<li>#EDB_DBS_FULL - too many databases have been opened. See #edb_env_set_maxdbs().
	 * </ul>
	 */
int  edb_dbi_open(EDB_txn *txn, const char *name, unsigned int flags, EDB_dbi *dbi);

	/** @brief Retrieve statistics for a database.
	 *
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[out] stat The address of an #EDB_stat structure
	 * 	where the statistics will be copied
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_stat(EDB_txn *txn, EDB_dbi dbi, EDB_stat *stat);

	/** @brief Retrieve the DB flags for a database handle.
	 *
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[out] flags Address where the flags will be returned.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int edb_dbi_flags(EDB_txn *txn, EDB_dbi dbi, unsigned int *flags);

	/** @brief Close a database handle. Normally unnecessary. Use with care:
	 *
	 * This call is not mutex protected. Handles should only be closed by
	 * a single thread, and only if no other threads are going to reference
	 * the database handle or one of its cursors any further. Do not close
	 * a handle if an existing transaction has modified its database.
	 * Doing so can cause misbehavior from database corruption to errors
	 * like EDB_BAD_VALSIZE (since the DB name is gone).
	 *
	 * Closing a database handle is not necessary, but lets #edb_dbi_open()
	 * reuse the handle value.  Usually it's better to set a bigger
	 * #edb_env_set_maxdbs(), unless that value would be large.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 */
void edb_dbi_close(EDB_env *env, EDB_dbi dbi);

	/** @brief Empty or delete+close a database.
	 *
	 * See #edb_dbi_close() for restrictions about closing the DB handle.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] del 0 to empty the DB, 1 to delete it from the
	 * environment and close the DB handle.
	 * @return A non-zero error value on failure and 0 on success.
	 */
int  edb_drop(EDB_txn *txn, EDB_dbi dbi, int del);

	/** @brief Set a custom key comparison function for a database.
	 *
	 * The comparison function is called whenever it is necessary to compare a
	 * key specified by the application with a key currently stored in the database.
	 * If no comparison function is specified, and no special key flags were specified
	 * with #edb_dbi_open(), the keys are compared lexically, with shorter keys collating
	 * before longer keys.
	 * @warning This function must be called before any data access functions are used,
	 * otherwise data corruption may occur. The same comparison function must be used by every
	 * program accessing the database, every time the database is used.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] cmp A #EDB_cmp_func function
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_set_compare(EDB_txn *txn, EDB_dbi dbi, EDB_cmp_func *cmp);

	/** @brief Set a custom data comparison function for a #EDB_DUPSORT database.
	 *
	 * This comparison function is called whenever it is necessary to compare a data
	 * item specified by the application with a data item currently stored in the database.
	 * This function only takes effect if the database was opened with the #EDB_DUPSORT
	 * flag.
	 * If no comparison function is specified, and no special key flags were specified
	 * with #edb_dbi_open(), the data items are compared lexically, with shorter items collating
	 * before longer items.
	 * @warning This function must be called before any data access functions are used,
	 * otherwise data corruption may occur. The same comparison function must be used by every
	 * program accessing the database, every time the database is used.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] cmp A #EDB_cmp_func function
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_set_dupsort(EDB_txn *txn, EDB_dbi dbi, EDB_cmp_func *cmp);

	/** @brief Set a relocation function for a #EDB_FIXEDMAP database.
	 *
	 * @todo The relocation function is called whenever it is necessary to move the data
	 * of an item to a different position in the database (e.g. through tree
	 * balancing operations, shifts as a result of adds or deletes, etc.). It is
	 * intended to allow address/position-dependent data items to be stored in
	 * a database in an environment opened with the #EDB_FIXEDMAP option.
	 * Currently the relocation feature is unimplemented and setting
	 * this function has no effect.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] rel A #EDB_rel_func function
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_set_relfunc(EDB_txn *txn, EDB_dbi dbi, EDB_rel_func *rel);

	/** @brief Set a context pointer for a #EDB_FIXEDMAP database's relocation function.
	 *
	 * See #edb_set_relfunc and #EDB_rel_func for more details.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] ctx An arbitrary pointer for whatever the application needs.
	 * It will be passed to the callback function set by #edb_set_relfunc
	 * as its \b relctx parameter whenever the callback is invoked.
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_set_relctx(EDB_txn *txn, EDB_dbi dbi, void *ctx);

	/** @brief Get items from a database.
	 *
	 * This function retrieves key/data pairs from the database. The address
	 * and length of the data associated with the specified \b key are returned
	 * in the structure to which \b data refers.
	 * If the database supports duplicate keys (#EDB_DUPSORT) then the
	 * first data item for the key will be returned. Retrieval of other
	 * items requires the use of #edb_cursor_get().
	 *
	 * @note The memory pointed to by the returned values is owned by the
	 * database. The caller need not dispose of the memory, and may not
	 * modify it in any way. For values returned in a read-only transaction
	 * any modification attempts will cause a SIGSEGV.
	 * @note Values returned from the database are valid only until a
	 * subsequent update operation, or the end of the transaction.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] key The key to search for in the database
	 * @param[out] data The data corresponding to the key
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_NOTFOUND - the key was not in the database.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_get(EDB_txn *txn, EDB_dbi dbi, EDB_val *key, EDB_val *data);

	/** @brief Store items into a database.
	 *
	 * This function stores key/data pairs in the database. The default behavior
	 * is to enter the new key/data pair, replacing any previously existing key
	 * if duplicates are disallowed, or adding a duplicate data item if
	 * duplicates are allowed (#EDB_DUPSORT).
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] key The key to store in the database
	 * @param[in,out] data The data to store
	 * @param[in] flags Special options for this operation. This parameter
	 * must be set to 0 or by bitwise OR'ing together one or more of the
	 * values described here.
	 * <ul>
	 *	<li>#EDB_NODUPDATA - enter the new key/data pair only if it does not
	 *		already appear in the database. This flag may only be specified
	 *		if the database was opened with #EDB_DUPSORT. The function will
	 *		return #EDB_KEYEXIST if the key/data pair already appears in the
	 *		database.
	 *	<li>#EDB_NOOVERWRITE - enter the new key/data pair only if the key
	 *		does not already appear in the database. The function will return
	 *		#EDB_KEYEXIST if the key already appears in the database, even if
	 *		the database supports duplicates (#EDB_DUPSORT). The \b data
	 *		parameter will be set to point to the existing item.
	 *	<li>#EDB_RESERVE - reserve space for data of the given size, but
	 *		don't copy the given data. Instead, return a pointer to the
	 *		reserved space, which the caller can fill in later - before
	 *		the next update operation or the transaction ends. This saves
	 *		an extra memcpy if the data is being generated later.
	 *		EXDB does nothing else with this memory, the caller is expected
	 *		to modify all of the space requested. This flag must not be
	 *		specified if the database was opened with #EDB_DUPSORT.
	 *	<li>#EDB_APPEND - append the given key/data pair to the end of the
	 *		database. This option allows fast bulk loading when keys are
	 *		already known to be in the correct order. Loading unsorted keys
	 *		with this flag will cause a #EDB_KEYEXIST error.
	 *	<li>#EDB_APPENDDUP - as above, but for sorted dup data.
	 * </ul>
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_MAP_FULL - the database is full, see #edb_env_set_mapsize().
	 *	<li>#EDB_TXN_FULL - the transaction has too many dirty pages.
	 *	<li>EACCES - an attempt was made to write in a read-only transaction.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_put(EDB_txn *txn, EDB_dbi dbi, EDB_val *key, EDB_val *data,
			    unsigned int flags);

	/** @brief Delete items from a database.
	 *
	 * This function removes key/data pairs from the database.
	 * If the database does not support sorted duplicate data items
	 * (#EDB_DUPSORT) the data parameter is ignored.
	 * If the database supports sorted duplicates and the data parameter
	 * is NULL, all of the duplicate data items for the key will be
	 * deleted. Otherwise, if the data parameter is non-NULL
	 * only the matching data item will be deleted.
	 * This function will return #EDB_NOTFOUND if the specified key/data
	 * pair is not in the database.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] key The key to delete from the database
	 * @param[in] data The data to delete
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EACCES - an attempt was made to write in a read-only transaction.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_del(EDB_txn *txn, EDB_dbi dbi, EDB_val *key, EDB_val *data);

	/** @brief Create a cursor handle.
	 *
	 * A cursor is associated with a specific transaction and database.
	 * A cursor cannot be used when its database handle is closed.  Nor
	 * when its transaction has ended, except with #edb_cursor_renew().
	 * It can be discarded with #edb_cursor_close().
	 * A cursor in a write-transaction can be closed before its transaction
	 * ends, and will otherwise be closed when its transaction ends.
	 * A cursor in a read-only transaction must be closed explicitly, before
	 * or after its transaction ends. It can be reused with
	 * #edb_cursor_renew() before finally closing it.
	 * @note Earlier documentation said that cursors in every transaction
	 * were closed when the transaction committed or aborted.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[out] cursor Address where the new #EDB_cursor handle will be stored
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_open(EDB_txn *txn, EDB_dbi dbi, EDB_cursor **cursor);

	/** @brief Close a cursor handle.
	 *
	 * The cursor handle will be freed and must not be used again after this call.
	 * Its transaction must still be live if it is a write-transaction.
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 */
void edb_cursor_close(EDB_cursor *cursor);

	/** @brief Renew a cursor handle.
	 *
	 * A cursor is associated with a specific transaction and database.
	 * Cursors that are only used in read-only
	 * transactions may be re-used, to avoid unnecessary malloc/free overhead.
	 * The cursor may be associated with a new read-only transaction, and
	 * referencing the same database handle as it was created with.
	 * This may be done whether the previous transaction is live or dead.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_renew(EDB_txn *txn, EDB_cursor *cursor);

	/** @brief Return the cursor's transaction handle.
	 *
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 */
EDB_txn *edb_cursor_txn(EDB_cursor *cursor);

	/** @brief Return the cursor's database handle.
	 *
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 */
EDB_dbi edb_cursor_dbi(EDB_cursor *cursor);

	/** @brief Retrieve by cursor.
	 *
	 * This function retrieves key/data pairs from the database. The address and length
	 * of the key are returned in the object to which \b key refers (except for the
	 * case of the #EDB_SET option, in which the \b key object is unchanged), and
	 * the address and length of the data are returned in the object to which \b data
	 * refers.
	 * See #edb_get() for restrictions on using the output values.
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 * @param[in,out] key The key for a retrieved item
	 * @param[in,out] data The data of a retrieved item
	 * @param[in] op A cursor operation #EDB_cursor_op
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_NOTFOUND - no matching key found.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_get(EDB_cursor *cursor, EDB_val *key, EDB_val *data,
			    EDB_cursor_op op);

	/** @brief Store by cursor.
	 *
	 * This function stores key/data pairs into the database.
	 * The cursor is positioned at the new item, or on failure usually near it.
	 * @note Earlier documentation incorrectly said errors would leave the
	 * state of the cursor unchanged.
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 * @param[in] key The key operated on.
	 * @param[in] data The data operated on.
	 * @param[in] flags Options for this operation. This parameter
	 * must be set to 0 or one of the values described here.
	 * <ul>
	 *	<li>#EDB_CURRENT - replace the item at the current cursor position.
	 *		The \b key parameter must still be provided, and must match it.
	 *		If using sorted duplicates (#EDB_DUPSORT) the data item must still
	 *		sort into the same place. This is intended to be used when the
	 *		new data is the same size as the old. Otherwise it will simply
	 *		perform a delete of the old record followed by an insert.
	 *	<li>#EDB_NODUPDATA - enter the new key/data pair only if it does not
	 *		already appear in the database. This flag may only be specified
	 *		if the database was opened with #EDB_DUPSORT. The function will
	 *		return #EDB_KEYEXIST if the key/data pair already appears in the
	 *		database.
	 *	<li>#EDB_NOOVERWRITE - enter the new key/data pair only if the key
	 *		does not already appear in the database. The function will return
	 *		#EDB_KEYEXIST if the key already appears in the database, even if
	 *		the database supports duplicates (#EDB_DUPSORT).
	 *	<li>#EDB_RESERVE - reserve space for data of the given size, but
	 *		don't copy the given data. Instead, return a pointer to the
	 *		reserved space, which the caller can fill in later - before
	 *		the next update operation or the transaction ends. This saves
	 *		an extra memcpy if the data is being generated later. This flag
	 *		must not be specified if the database was opened with #EDB_DUPSORT.
	 *	<li>#EDB_APPEND - append the given key/data pair to the end of the
	 *		database. No key comparisons are performed. This option allows
	 *		fast bulk loading when keys are already known to be in the
	 *		correct order. Loading unsorted keys with this flag will cause
	 *		a #EDB_KEYEXIST error.
	 *	<li>#EDB_APPENDDUP - as above, but for sorted dup data.
	 *	<li>#EDB_MULTIPLE - store multiple contiguous data elements in a
	 *		single request. This flag may only be specified if the database
	 *		was opened with #EDB_DUPFIXED. The \b data argument must be an
	 *		array of two EDB_vals. The mv_size of the first EDB_val must be
	 *		the size of a single data element. The mv_data of the first EDB_val
	 *		must point to the beginning of the array of contiguous data elements.
	 *		The mv_size of the second EDB_val must be the count of the number
	 *		of data elements to store. On return this field will be set to
	 *		the count of the number of elements actually written. The mv_data
	 *		of the second EDB_val is unused.
	 * </ul>
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>#EDB_MAP_FULL - the database is full, see #edb_env_set_mapsize().
	 *	<li>#EDB_TXN_FULL - the transaction has too many dirty pages.
	 *	<li>EACCES - an attempt was made to write in a read-only transaction.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_put(EDB_cursor *cursor, EDB_val *key, EDB_val *data,
				unsigned int flags);

	/** @brief Delete current key/data pair
	 *
	 * This function deletes the key/data pair to which the cursor refers.
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 * @param[in] flags Options for this operation. This parameter
	 * must be set to 0 or one of the values described here.
	 * <ul>
	 *	<li>#EDB_NODUPDATA - delete all of the data items for the current key.
	 *		This flag may only be specified if the database was opened with #EDB_DUPSORT.
	 * </ul>
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EACCES - an attempt was made to write in a read-only transaction.
	 *	<li>EINVAL - an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_del(EDB_cursor *cursor, unsigned int flags);

	/** @brief Return count of duplicates for current key.
	 *
	 * This call is only valid on databases that support sorted duplicate
	 * data items #EDB_DUPSORT.
	 * @param[in] cursor A cursor handle returned by #edb_cursor_open()
	 * @param[out] countp Address where the count will be stored
	 * @return A non-zero error value on failure and 0 on success. Some possible
	 * errors are:
	 * <ul>
	 *	<li>EINVAL - cursor is not initialized, or an invalid parameter was specified.
	 * </ul>
	 */
int  edb_cursor_count(EDB_cursor *cursor, edb_size_t *countp);

	/** @brief Compare two data items according to a particular database.
	 *
	 * This returns a comparison as if the two data items were keys in the
	 * specified database.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] a The first item to compare
	 * @param[in] b The second item to compare
	 * @return < 0 if a < b, 0 if a == b, > 0 if a > b
	 */
int  edb_cmp(EDB_txn *txn, EDB_dbi dbi, const EDB_val *a, const EDB_val *b);

	/** @brief Compare two data items according to a particular database.
	 *
	 * This returns a comparison as if the two items were data items of
	 * the specified database. The database must have the #EDB_DUPSORT flag.
	 * @param[in] txn A transaction handle returned by #edb_txn_begin()
	 * @param[in] dbi A database handle returned by #edb_dbi_open()
	 * @param[in] a The first item to compare
	 * @param[in] b The second item to compare
	 * @return < 0 if a < b, 0 if a == b, > 0 if a > b
	 */
int  edb_dcmp(EDB_txn *txn, EDB_dbi dbi, const EDB_val *a, const EDB_val *b);

	/** @brief A callback function used to print a message from the library.
	 *
	 * @param[in] msg The string to be printed.
	 * @param[in] ctx An arbitrary context pointer for the callback.
	 * @return < 0 on failure, >= 0 on success.
	 */
typedef int (EDB_msg_func)(const char *msg, void *ctx);

	/** @brief Dump the entries in the reader lock table.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[in] func A #EDB_msg_func function
	 * @param[in] ctx Anything the message function needs
	 * @return < 0 on failure, >= 0 on success.
	 */
int	edb_reader_list(EDB_env *env, EDB_msg_func *func, void *ctx);

	/** @brief Check for stale entries in the reader lock table.
	 *
	 * @param[in] env An environment handle returned by #edb_env_create()
	 * @param[out] dead Number of stale slots that were cleared
	 * @return 0 on success, non-zero on failure.
	 */
int	edb_reader_check(EDB_env *env, int *dead);
/**	@} */

#ifdef __cplusplus
}
#endif
/** @page tools EXDB Command Line Tools
	The following describes the command line tools that are available for EXDB.
	\li \ref edb_copy_1
	\li \ref edb_dump_1
	\li \ref edb_load_1
	\li \ref edb_stat_1
*/

#endif /* _EXDB_H_ */
