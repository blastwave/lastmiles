#include <stdio.h>

typedef int Foo[16][16][64][64];

struct Bar {
    Foo* data;
};

void foobar(Bar t)
{
    printf("%i\n", (*t.data)[1][2][3][4]);
}

int main(int argc, char** argv)
{
    struct Bar t;
    Foo data;
    data[1][2][3][4] = 10;
    t.data = &data;
    foobar(t);
    
    return 0;
}

