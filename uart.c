#include <LPC21xx.H>
#include <stdio.h>
#include "uart.h"
#include "string.h"
volatile char uart0_buff[100];
volatile char esp01_buff[100];
volatile char stack[10][100] = {0};// our stack to store the messages
signed char top = -1;// to keep track of the top of the stack
volatile int hb = 0, eb = 0;
volatile char uart0_buff_full_flg = 0;
void uart0rx_irq(void) __irq
{
    unsigned char ch;
    if((U0IIR & 0x0E) == 0x04)
    {
        ch = U0RBR;
        if(ch == '\n')
        {
            /* ignore */ //SEND\r\n
        }
        else
        {
            uart0_buff[hb++] = ch;
            if(ch == '\r')
            {
                uart0_buff[hb-1] = '\0';
                uart0_buff_full_flg = 1;
                hb = 0;
            }
        }

    }
    VICVectAddr = 0;
}
void uart1rx_irq(void) __irq
{	
    unsigned char chr;
    if((U1IIR & 0x0E) == 0x04)
    {
        chr = U1RBR;
		if(eb<100)
	        	esp01_buff[eb++] = chr;
	    if((chr == '\r')||(chr == '\n'))// almost all messg end with \r\n but +IPD stuff end with only \n?
	    {
	        esp01_buff[eb-1] = '\0';//\r \n  o k  \r \n
	        if(eb == 1)// ment only \r\n so ignore
	        {
	            eb = 0;
	            VICVectAddr = 0; 
	            return;
	        }
	        eb = 0;
	        if(!(uartstack_push((char*)esp01_buff)))
	        {
	            uart0str("STACK FULL\r\n");
	        }
	    }
	    VICVectAddr = 0;
    }
}
char uartstack_push(char* ch)
{
    if(top < 9)
    {
        top++;
        strcpy(stack[top],ch);
        return 1;
    }
    return 0;
}
char uartstack_pop(char* ch)
{
    if(top >= 0)
    {
        strcpy(ch,(const char*)stack[top]);
        top--;
        return 1;
    }
    return 0;
}
char is_there_data(void)
{
		if(top >= 0)
		{
			return 1;
		}
		return 0;
}
void uartstack_clr(void)
{
    top = -1;
}

void uart0_config(void)
{
    PINSEL0 |= 0x05;
    U0LCR = 0x83;
    U0DLL = 97;
    U0DLM = 0;
    U0LCR = 0x03;

    VICVectCntl0 = 0x20 | 6;
    VICVectAddr0 = (unsigned int)uart0rx_irq;

    U0IER = 1<<0;
    VICIntEnable |= (1<<6);
}

char Get_ht_Buff(char *buf)
{
    if(uart0_buff_full_flg)
    {
        int i=0;
        while(uart0_buff[i])
        {
            buf[i] = uart0_buff[i];
            i++;
        }
        buf[i] = '\0';
        uart0_buff_full_flg = 0;
        return 1;
    }
    return 0;
}

void uart0tx(char tx)
{
    U0THR = tx;
    while(((U0LSR>>5)&1)==0);
}

void uart0str(const char* s)
{
    while(*s) uart0tx(*s++);
}

void uart0_tx_integer( int num)
{
    char buf[10];
    sprintf(buf,"%d",num);
    uart0str(buf);
}
void uart1_config(void)
{
    VICVectCntl1 = 0x20|7;//uart1 for vic slot1
    VICVectAddr1 =(unsigned int)uart1rx_irq;//addr of isr
    U1IER = 1<<0;//enable uart1 recive interupt
    VICIntEnable |= (1<<7);//enable the interupt
    PINSEL0 |= (1<<16)|(1<<18);//configure uart1 tx rx
    U1LCR = 0x83;// dlab free 
    U1DLL = 97;//assign latch
    U1DLM = 0;//assign latch
    U1LCR = 0x3;//dlab lock 
}
void uart1tx(char tx)
{
    U1THR = tx;
    while(((U1LSR>>5)&1)==0);
}
void uart1str(char* s)
{
    while(*s)
    {
        uart1tx(*s++);
    }
}
