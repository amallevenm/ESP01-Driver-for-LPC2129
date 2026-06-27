#include "esp01.h"
#include "uart.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include<LPC21xx.H>
char wifi_n[20];//="LAPTOP-BBDID";
char wifi_pwd[20];//="123456789";
char ser_ip[20];//="192.168.137.92";
char ser_port[10];//="2023";
char hyperterminal_buff[100];
char rcv_buff[100];
char flag = 0;
int main()
{
    uart0_config();
    uart0str("Enter wifi name:\r\n");
    while(!Get_ht_Buff(wifi_n));
    delay_ms(50);
    uart0str("Enter wifi password:\r\n");
    while(!Get_ht_Buff(wifi_pwd));
    delay_ms(50);
    uart0str("Enter server ip:\r\n");
    while(!Get_ht_Buff(ser_ip));
    delay_ms(50);
    uart0str("Enter server port:\r\n");
    while(!Get_ht_Buff(ser_port));
    delay_ms(50);						
    uart0str("the all details below\r\n");
    uart0str(wifi_n);
    uart0str("\r\n");
    uart0str(wifi_pwd);
    uart0str("\r\n");
    uart0str(ser_ip);
    uart0str("\r\n");
    uart0str(ser_port);
    uart0str("\r\n");
    get_esp01_data(wifi_n,wifi_pwd,ser_ip,ser_port);
    uart1_config();// delayed the uart1 init here to get rid of the garbage send by 
    delay_ms(1000); // esp01 during start up
    uart0str("about to initalize the esp01\r\n");
    if(esp_init(0))
    {
        uart0str("ESP01 Initialized\r\n");
    }
    else
    {
        uart0str("ESP01 Initialization failed\r\n");
    }
    while(1)
    { 
        char *p ,ret;
        ret =  esp_notification(rcv_buff);
        if(ret )
        {
            if(ret == 1)
            {
                uart0str("wifi disconnected\r\n");//1
				
                esp_init(is_wifi_connected);// check if it is disconnected or not
            }
            else if(ret == 2)
            {
                uart0str("tcp_closed\r\n");//2
				uart0str("so wait a minute\r\n");
				delay_sec(60);
                esp_init(connect_wifi);
            }
            else
            {
                uart0str("message received\r\n");//3
                p = strchr(rcv_buff,':');
                if(p)
                {
                    uart0str(p + 1);// to skip the +IPD,21: string from printing
                    uart0str("\r\n");
                }
                else
                {
                    uart0str("well an error occured\r\n");
                }

            }
            ret  = 0;
        }

        if(Get_ht_Buff(hyperterminal_buff))
        {
            if(strstr(hyperterminal_buff,"SEND")!=0)
            {
                if(esp_status() == 6)
                {
                    uart0str("enter the string\r\n");
                    while(!(Get_ht_Buff(hyperterminal_buff)));
                    uart0str("BREAKED THE WHILE ONE\r\n");

                    flag = esp_send(hyperterminal_buff); 
                    if(flag== 1)
                    {
                        uart0str("uart string send\r\n");
                    }
                    else
                    {
                        uart0str("uart string not send\r\n");
                    }
                }
                else
                {
                    uart0str("esp01 is not initalized\r\n");
                }   
            }
            else
            {
                uart0str("worng command\r\n");
            }
        }   
    }
}
