/*
 * This header is NOT original but it's a derivative work of this public
 * documentation:
 *
 * X/Open CAE Specification
 * Distributed Transaction Processing:
 * The TX (Transaction Demarcation) Specification
 * ISBN: 1-85912-094-6
 * X/Open Document Number: C504
 */

/*
* Start of tx.h header
*
* Define a symbol to prevent multiple inclusions of this header file
*/
#ifndef TX_H
#define TX_H
#define TX_H_VERSION 0 /* current version of this header file */

#include <tx.h>

/*
* A value of -1 in formatID means that the XID is null.
*/
/*
* Definitions for tx_*() routines
*/
/* commit return values */
typedef long COMMIT_RETURN;
#define TX_COMMIT_COMPLETED 0
#define TX_COMMIT_DECISION_LOGGED 1
/* transaction control values */
typedef long TRANSACTION_CONTROL;
#define TX_UNCHAINED 0
#define TX_CHAINED 1
/* type of transaction timeouts */
typedef long TRANSACTION_TIMEOUT;
/* transaction state values */
typedef long TRANSACTION_STATE;

#define TX_ACTIVE 0
#define TX_TIMEOUT_ROLLBACK_ONLY 1
#define TX_ROLLBACK_ONLY 2
/* structure populated by tx_info() */
struct tx_info_t {
    XID xid;
    COMMIT_RETURN when_return;
    TRANSACTION_CONTROL transaction_control;
    TRANSACTION_TIMEOUT transaction_timeout;
    TRANSACTION_STATE transaction_state;
};
typedef struct tx_info_t TXINFO;
/*
* Declarations of routines by which Applications call TMs:
*/
#ifdef __STDC__
extern int tx_begin(void);
extern int tx_close(void);
extern int tx_commit(void);
extern int tx_info(TXINFO *);
extern int tx_open(void);
extern int tx_rollback(void);
extern int tx_set_commit_return(COMMIT_RETURN);
extern int tx_set_transaction_control(TRANSACTION_CONTROL);
extern int tx_set_transaction_timeout(TRANSACTION_TIMEOUT);
#else /* ifndef __STDC__ */
extern int tx_begin();
extern int tx_close();
extern int tx_commit();
extern int tx_info();
extern int tx_open();
extern int tx_rollback();
extern int tx_set_commit_return();
extern int tx_set_transaction_control();
extern int tx_set_transaction_timeout();
#endif /* ifndef __STDC__ */
/*
* tx_*() return codes (transaction manager reports to application)
*/
#define TX_NOT_SUPPORTED 1 /* option not supported */
#define TX_OK 0 /* normal execution */
#define TX_OUTSIDE -1 /* application is in an RM local
transaction */
#define TX_ROLLBACK -2 /* transaction was rolled back */
#define TX_MIXED -3 /* transaction was partially committed
and partially rolled back */


#define TX_HAZARD -4 /* transaction may have been partially
committed and partially rolled back */
#define TX_PROTOCOL_ERROR -5 /* routine invoked in an improper
context */
#define TX_ERROR -6 /* transient error */
#define TX_FAIL -7 /* fatal error */
#define TX_EINVAL -8 /* invalid arguments were given */
#define TX_COMMITTED -9 /* transaction has heuristically
committed */
#define TX_NO_BEGIN -100 /* transaction committed plus new
transaction could not be started */
#define TX_ROLLBACK_NO_BEGIN (TX_ROLLBACK+TX_NO_BEGIN)
/* transaction rollback plus new
transaction could not be started */
#define TX_MIXED_NO_BEGIN (TX_MIXED+TX_NO_BEGIN)
/* mixed plus new transaction could not
be started */
#define TX_HAZARD_NO_BEGIN (TX_HAZARD+TX_NO_BEGIN)
/* hazard plus new transaction could
not be started */
#define TX_COMMITTED_NO_BEGIN (TX_COMMITTED+TX_NO_BEGIN)
/* heuristically committed plus new
transaction could not be started */
#endif /* ifndef TX_H */
/*
* End of tx.h header
*/

