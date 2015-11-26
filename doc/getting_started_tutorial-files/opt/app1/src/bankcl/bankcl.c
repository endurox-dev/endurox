#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <atmi.h>
#include <ubf.h>
#include <bank.fd.h>

#define SUCCEED		0
#define FAIL		-1

/**
 * Do the test call to the server
 */
int main(int argc, char** argv) {

	int ret=SUCCEED;
	UBFH *p_ub;
	long rsplen;
	double balance;
	
	/* allocate the call buffer */
	if (NULL== (p_ub = (UBFH *)tpalloc("UBF", NULL, 1024)))
	{
		fprintf(stderr, "Failed to realloc the UBF buffer - %s\n", 
			tpstrerror(tperrno));
		ret=FAIL;
		goto out;
	}
	
	/* Set the data */
	if (SUCCEED!=Badd(p_ub, T_ACCNUM, "ACC00000000001", 0) ||
		SUCCEED!=Badd(p_ub, T_ACCCUR, "USD", 0))
	{
		fprintf(stderr, "Failed to get T_ACCNUM[0]! -  %s\n", 
			Bstrerror(Berror));
		ret=FAIL;
		goto out;
	}
	
	/* Call the server */
	if (FAIL == tpcall("BALANCE", (char *)p_ub, 0L, (char **)&p_ub, &rsplen,0))
	{
		fprintf(stderr, "Failed to call BALANCE - %s\n", 
			tpstrerror(tperrno));
		
		ret=FAIL;
		goto out;
	}
	
	/* Read the balance field */
	
	if (SUCCEED!=Bget(p_ub, T_AMTAVL, 0, (char *)&balance, 0L))
	{
		fprintf(stderr, "Failed to get T_AMTAVL[0]! -  %s\n", 
			Bstrerror(Berror));
		ret=FAIL;
		goto out;
	}
	
	printf("Account balance is: %.2lf USD\n", balance);
	
out:
	/* free the buffer */
	if (NULL!=p_ub)
	{
		tpfree((char *)p_ub);
	}
	
	/* Terminate ATMI session */
	tpterm();
	return ret;
}
