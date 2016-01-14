
// Copyright (c) rryqszq4

//gcc -o mad_except mad_except.c ../src/core/mad_except.c -I../src/core -I/usr/local/zeromq/include -L/usr/local/zeromq/lib -lzmq

#include "../src/core/mad_core.h"

int 
main()
{

	mad_except_test();

	return 0;
}