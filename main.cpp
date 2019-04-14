#include <iostream>
#include "uthreads.cpp"

void func1()
{
    return;
}

void func2()
{
    return;
}


int main()
{
    uthread_init(1000);

    uthread_spawn(func1);
    uthread_spawn(func2);

    return 0;
}
