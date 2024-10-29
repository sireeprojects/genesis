/* snprintf example */
#include <iostream>
#include <stdio.h>

using namespace std;

int main ()
{
  // char buffer [100];
  // int cx;

  // cx = snprintf ( buffer, 100, "The half of %d is %d", 60, 60/2 );

  // if (cx>=0 && cx<100)      // check returned value
    // snprintf ( buffer+cx, 100-cx, ", and the half of that is %d.", 60/2/2 );

  // puts (buffer);


    char val[8];
    unsigned int a = 100;
    // snprintf(val, 2, "%x", a);
    sprintf(val, "%x", a);

    // printf("%s%s\n", val[1], val[0]);

    cout << hex << val[0] << val[1] << endl;

  return 0;
}
