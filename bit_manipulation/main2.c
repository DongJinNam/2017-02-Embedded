#include <stdio.h>

#define BIT(X) (1 << X)

int main()
{
    int i,j,k;

    for (i = 0; i < 4; i++) {
        unsigned char LEDs = 0x01;
        int ans = 1;
        printf("%d,",ans);
        for (j = 0; j < 7; j++) {
            LEDs <<= 1;
            ans = 0;
            for (k = 0; k < 8; k++) {
                if (LEDs & BIT(k)) {
                    ans += BIT(k);
                }
            }
            printf("%d,",ans);
        }
        for (j = 0; j < 7; j++) {
            LEDs >>= 1;
            ans = 0;
            for (k = 0; k < 8; k++) {
                if (LEDs & BIT(k)) {
                    ans += BIT(k);
                }
            }
            if (j != 6) printf("%d,",ans);
            else printf("%d\n",ans);
        }
    }
    return 0;
}
