#include "delay.h"
void delay_ms(U32 ms)
{

    T0PR = 15000-1;
    T0TCR = 1;
    while(T0TC<ms);
    T0TCR = 3;
    T0TCR = 0;
}
void delay_sec(U32 s)
{
	T0PR = 15000000-1;
	T0TCR = 1;
	while(T0TC<s);
	T0TCR = 3;
	T0TCR =0;
}
