
#include <inc/lib.h>
extern volatile bool printStats;


int fibonacci(int n);

void
_main(void)
{
	printStats = 0;

	int i1=30;

	int res = fibonacci(i1) ;

//	sys_lock_cons();
	{
		cprintf_colored(TEXT_light_magenta, "Finished\n");
		//cprintf_colored(TEXT_light_magenta, "\n>>>> Fibonacci #%d = %d <<<<\n",i1, res);
		//res = fibonacci(i1) ; //in MLFQ: repeat again inside the CS to ensure that it'llbe finished from the same expected Q
	}
//	sys_unlock_cons();

	if (res != 1346269)
		panic("[envID %d] wrong result!", myEnv->env_id);

	//To indicate that it's completed successfully
	inctst();

	return;
}


int fibonacci(int n)
{
	if (n <= 1)
		return 1 ;
	return fibonacci(n-1) + fibonacci(n-2) ;
}

