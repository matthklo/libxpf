// NOTE: Test case available from http://www.codeproject.com/Articles/11464/Yet-Another-C-style-Delegate-Class-in-Standard-C

// Delegate.cpp : Defines the entry point for the console application.
//

#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif // #ifdef _MSC_VER

#include <xpf/delegate.h>

using namespace xpf;

static void H(const char* s)
{
    printf("  Static function invoked by %s\n", s);
}

class MyObject
{
public:
    int count;
    MyObject(int n) : count(n) { }

    void G(const char* s)
    {
        printf("  Member function invoked by %s\n", s);
        this->count++;
    }
};

class MyFunctor
{
public:
    void operator()(const char* s)
    {
        printf("  Functor invoked by %s\n", s);
    }
};

int main(int argc, char* argv[])
{
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // #ifdef _MSC_VER

    // Create an empty delegate
    printf("Invoking delegate a0:\n");
    Delegate<void ()> a0;
    a0();
    printf("\n");

    // Create delegate a that references a static function:
    Delegate<void (const char*)> a(&H);
    assert(a == &H);

    printf("Invoking delegate a:\n");
    a("a");
    printf("\n");

    // Create delegate b that references an instance function:
    MyObject myObj(0);
    Delegate<void (const char*)> b(&myObj, &MyObject::G);
    assert(b == std::make_pair(&myObj, &MyObject::G));

    printf("Invoking delegate b:\n");
    b("b");
    assert(myObj.count == 1);
    printf("\n");

    // Create delegate c that references a function object:
    MyFunctor myFunctor;
    Delegate<void (const char*)> c(myFunctor);

    printf("Invoking delegate c:\n");
    c("c");
    printf("\n");

    // Add an instance function and a functor to delegate a
    a += std::make_pair(&myObj, &MyObject::G);
    a += MyFunctor();

    printf("Invoking delegate a:\n");
    a("a");
    assert(myObj.count == 2);
    printf("\n");

    // Remove the static function from delegate a
    a -= &H;

    printf("Invoking delegate a:\n");
    a("a");
    assert(myObj.count == 3);
    printf("\n");

    // Create delegate d from a
    Delegate<void (const char*)> d(a);

    // Add delegate b to delegate d
    d += Delegate<void (const char*)>(&H);

    printf("Invoking delegate d:\n");
    d("d");
    assert(myObj.count == 4);
    printf("\n\nDone\n");

    return 0;
}
