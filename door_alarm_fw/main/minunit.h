/* file: minunit.h from http://www.jera.com/techinfo/jtns/jtn002.html */
/* A MinUnit test case is just a function that returns 0 (null) if the tests pass. 
If the test fails, the function should return a string describing the failing test. 
mu_assert is simply a macro that returns a string if the expression passed to it is false. 
The mu_runtest macro calls another test case and returns if that test case fails. 
That's all there is to it! */

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); mu_tests_run++; \
                                if (message) return message; } while (0)
extern int mu_tests_run;