#include <iostream>
#include <xpf/fcontext.h>

xpf::fcontext_t fcm,fc1,fc2;

void f1(xpf::vptr p)
{
    std::cout<<"f1: entered"<<std::endl;
    std::cout<<"f1: call jump_fcontext( & fc1, fc2, 0)"<< std::endl;
    xpf::jump_fcontext(&fc1,fc2,0);
    std::cout<<"f1: return"<<std::endl;
    xpf::jump_fcontext(&fc1,fcm,0);
}

void f2(xpf::vptr p)
{
    std::cout<<"f2: entered"<<std::endl;
    std::cout<<"f2: call jump_fcontext( & fc2, fc1, 0)"<<std::endl;
    xpf::jump_fcontext(&fc2,fc1,0);
    xpfAssert(false&&!"f2: never returns");
}

int main()
{
    std::size_t size(8192);
    char* sp1 = new char[size];
    char* sp2 = new char[size];

	std::cout << "main: preparing fcontext..." << std::endl;
    fc1= xpf::make_fcontext(sp1, size, f1);
    fc2= xpf::make_fcontext(sp2, size, f2);

    std::cout<<"main: call jump_fcontext( & fcm, fc1, 0)"<<std::endl;
    xpf::jump_fcontext(&fcm,fc1,0);

    delete[] sp1;
    delete[] sp2;
    return 0;
}

