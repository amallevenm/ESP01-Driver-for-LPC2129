#ifndef __UART_H_
#define __UART_H_

void uart0_config(void);
void uart0tx(char);
void uart0str(const char*);
void uart0_tx_integer( int);

char Get_ht_Buff(char *buf);
//char Get_esp01_Buff(char *buf);
void uart1_config(void);
void uart1str(char*);
void uart1tx(char);
char uartstack_push(char*);
char uartstack_pop(char*);
void uartstack_clr(void);
char is_there_data(void);


#endif
