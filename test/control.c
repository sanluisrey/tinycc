#include "test.h"

int main() {
    ASSERT(3, ({ if (0) 2; 3; }));
    ASSERT(3, ({ if (1-1) 2; 3; }));
    ASSERT(4, ({ int x; if (0) { 1; 2; x = 3; } else { x = 4; } x; }));

    ASSERT(3, ({ if (1) { 1; 2; 3; } else { 4; } }));
    ASSERT(55, ({ int i; int j; i=0;  j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));
    ASSERT(10, ({ int i;  i=0; while(i<10) i=i+1; i; }));
    ASSERT(3, ({ {1; {2;} 3;} }));
    ASSERT(5, ({ ;;; 5; }));
    ASSERT(10, ({ int i; i=0; while(i<10) i=i+1; i; }));
    ASSERT(10, ({ int i;i =0; while(i<10) i=i+1; i; }));
    ASSERT(55, ({ int i; i=0; int j; j=0; while(i<=10) {j=i+j; i=i+1;} j; }));
    printf("OK\n");
    return 0;
}
