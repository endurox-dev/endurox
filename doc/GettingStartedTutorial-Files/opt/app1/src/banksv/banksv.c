#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Enduro/X includes: */
#include <atmi.h>
#include <ubf.h>
#include <bank.fd.h>

#define SUCCEED		0
#define FAIL		-1

/**
 * BALANCE service
 */
void BALANCE (TPSVCINFO *p_svc)
{
	int ret=SUCCEED;
	double balance;
	char account[28+1];
	char currency[3+1];
	BFLDLEN len;

	UBFH *p_ub = (UBFH *)p_svc->data;

	fprintf(stderr, "BALANCE got call\n");

	/* Resize the buffer to have some space in... */
	if (NULL==(p_ub = (UBFH *)tprealloc ((char *)p_ub, 1024)))
	{
		fprintf(stderr, "Failed to realloc the UBF buffer - %s\n", 
			tpstrerror(tperrno));
		ret=FAIL;
		goto out;
	}
	
	
	/* Read the account field */
	len = sizeof(account);
	if (SUCCEED!=Bget(p_ub, T_ACCNUM, 0, account, &len))
	{
		fprintf(stderr, "Failed to get T_ACCNUM[0]! -  %s\n", 
			Bstrerror(Berror));
		ret=FAIL;
		goto out;
	}
	
	/* Read the currency field */
	len = sizeof(currency);
	if (SUCCEED!=Bget(p_ub, T_ACCCUR, 0, currency, &len))
	{
		fprintf(stderr, "Failed to get T_ACCCUR[0]! -  %s\n", 
			Bstrerror(Berror));
		ret=FAIL;
		goto out;
	}
	
	fprintf(stderr, "Got request for account: [%s] currency [%s]\n",
			account, currency);

	srand(time(NULL));
	balance = (double)rand()/(double)RAND_MAX + rand();

	/* Return the value in T_AMTAVL field */
	
	fprintf(stderr, "Retruning balance %lf\n", balance);
	

	if (SUCCEED!=Bchg(p_ub, T_AMTAVL, 0, (char *)&balance, 0L))
	{
		fprintf(stderr, "Failed to set T_AMTAVL! -  %s\n", 
			Bstrerror(Berror));
		ret=FAIL;
		goto out;
	}

out:
	tpreturn(  ret==SUCCEED?TPSUCCESS:TPFAIL,
		0L,
		(char *)p_ub,
		0L,
		0L);
}

/**
 * Do initialization
 */
int tpsvrinit(int argc, char **argv)
{
	if (SUCCEED!=tpadvertise("BALANCE", BALANCE))
	{
		fprintf(stderr, "Failed to initialize BALANCE - %s\n", 
			tpstrerror(tperrno));
	}
}

/**
 * Do de-initialization
 */
void tpsvrdone(void)
{
	fprintf(stderr, "tpsvrdone called\n");
}

