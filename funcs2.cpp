#include "funcs2.h"

void funcs2()
{
	int a = 1;
}

static void func2()
{
	unsigned char buf[256] = { 0u };
	funcs2();
}

static void f1(int)
{
	long l{10};
}

static void f2(int)
{
	long l{10};
}

static void f3(int)
{
	long l{10};
}

static void f4(int)
{
	long l{10};
}

static void (*f[4])(int) = { f1, f2, f3, f4 };

double call(int a);

double call2(int a, double d)
{
	// (f[a])(a);
	f1(a);
	f2(a);
	f3(a);
	f4(a);
	f1(a);
	func2();
	funcs2();
	// call(1);
	return d + 1.1;
}