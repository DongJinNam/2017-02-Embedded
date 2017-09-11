#include <stdio.h>

#define BIT(X) (1 << X)

int main()
{
    unsigned char LEDs = 0x00;
    int cnt_one = 0;
    int ans = 0;
    int i,in;
    scanf("%d",&in);
    for (i = 0; i < 8; i++) {
        if (in & BIT(7-i)) {
            cnt_one++;
        }
    }
    for (i = 0; i < cnt_one; i++) {
        LEDs |= BIT(7-i);
        ans += (1 << (7-i));
    }
    printf("1의 개수? %d\n",cnt_one);
    printf("Shift 시의 값은? %d\n",ans);
    return 0;
}
