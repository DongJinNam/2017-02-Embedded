#include <stdio.h>

#define BIT(X) (1 << X)

int LED_function(int input) {
    return BIT(input);
}

int main()
{
    int i, input;
    for (i = 0; i < 8; i++) {
        int return_value;
        scanf("%d",&input);
        return_value = LED_function(input);
        printf("return_value:%d\n",return_value);
    }
    return 0;
}
