#include "uart.h"
#include "esp01.h"
#include "string.h"
#include <stdio.h>
#include "delay.h"
char esp01_rcv_buff[100];
char wifi_name[20];
char wifi_pass[20];
char server_ip[20];
char server_port[10];
char init_rdy_flag = 0;
init_state ist;                      //0       1     2        3           4          5          6        7        8             
const char esp_responses[17][20] ={"INVALID","OK","ERROR","+CWMODE:1","+CWJAP:","+CWJAP:1","+CWJAP:3","No AP","WIFI DISCONNECT"
    ,"CLOSED","CONNECT","ALREADY CONNECTED","+IPD","+CIPSTATUS:","STATUS:4","SEND OK","FAIL"};
//lookup table to check the reply specific to a state
//                         at       echo off   wifi mode  chek_wifi     conwifi        con_serv            send       noti
const char lut [8][5] = {{1,2,'\0'},{1,2,'\0'},{1,2,'\0'},{7,4,2,'\0'},{1,5,6,16,'\0'},{10,11,9,'\0'},{1,15,2,'\0'},{8,9,12,'\0'}};
const char debug_msg[7][10]={"AT","eco_off","wif_mod","wif_IScNT","con_WIF","con_ser","ok_bye"};//for debug_purpose
char esp_status(void)
{
		return((int)ist);
}
void get_esp01_data(char*buf1,char*buf2,char*buf3,char*buf4)
{
    int i =0;
    while(buf1[i])
    {
        wifi_name[i] = buf1[i];
        i++;
    }
    wifi_name[i] = '\0';
    i=0;
    while(buf2[i])
    {
        wifi_pass[i] = buf2[i];
        i++;
    }
    wifi_pass[i] = '\0';
    i=0;
    while(buf3[i])
    {
        server_ip[i] = buf3[i];
        i++;
    }
    server_ip[i] = '\0';
    i=0;
    while(buf4[i])
    {
        server_port[i] = buf4[i];
        i++;
    }
    server_port[i] = '\0';
    init_rdy_flag =1; 
}
char esp_init(char i)  // return value 1 for sucess,0 for fail
{		
    char responce = 0;
    char j =0;
    char t = 3;	 // try 3 times
    esp01_rcv_buff[0] = '\0';// clear the buffer before using it
    if(!init_rdy_flag)
    {
        uart0str("enter the data\r\n");
        return 0;// not ready to init
    }   
    ist = (init_state)i;


    for(j=0;j<16;j++)//trying 16 times
    {
        uart0str("try this: ");
		uart0str(debug_msg[(int)ist]);
        uart0str("\r\n");
        t = 3;
        switch(ist)
        {
            case at:
                uart1str("AT\r\n");
                uart0str("enterAT\r\n");
                delay_ms(2000);
                if(esp_response(1)== 1)
                {
                    ist = eccho_off;// go to next
                }
                break;
            case eccho_off:
                uart1str("ATE0\r\n");
                delay_ms(2000);
                if(esp_response(1)== 1)
                {
                    ist = wifi_mode;// go to next
                }
                break;
            case wifi_mode:
                uart1str("AT+CWMODE=1\r\n");
                delay_ms(2000);
                if(esp_response(1)== 1)
                {
                    ist = is_wifi_connected;
                }
                break;
            case is_wifi_connected:
                uart1str("AT+CWJAP?\r\n");
                delay_ms(2000);
                if((responce = esp_response(1))== 1)
                {
                    ist = connect_server;
                }
                else if(responce == 2)
                {
                    ist = connect_wifi;
                }
                break;
            case connect_wifi:
                uart1str("AT+CWJAP=\"");
                uart1str(wifi_name);
                uart1str("\",\"");
                uart1str(wifi_pass);
                uart1str("\"\r\n");
                while(t)
                {
                    delay_ms(5000);
                    if(esp_response(1)==1)
                    {
                        ist = connect_server;
                        break;
                    }
                    t--;
                }
                break;
            case connect_server:
                uart1str("AT+CIPSTART=\"TCP\",\"");
                uart1str(server_ip);
                uart1str("\",");
                uart1str(server_port);
                uart1str("\r\n");
                while(t)
                {
                    delay_ms(5000);
                    if(esp_response(1)==1)
                    {
                        ist = exit_init;
                        break;
                    }
                    t--;
                }
                break;
            case exit_init:
                break;
        }
        if(ist == exit_init)
        { 
            return 1;// just close and leave
        }
    }  
    if(j == 16)
    {
        uart0str("init failed\r\n");
        uart0_tx_integer(ist);
        return 0;
    }
    return 0;
}
char esp_send(char* buff)
{	
    unsigned int l =0;
    char t = 3,RET = 0;
    char no_of_chara[3];
    uart0str("STRING = ");
    uart0str(buff);
    uart0str("\r\n");
    while(buff[l])
    {
        l++;
    }
	if(l>99)
	{
		return 0;
	}
    uart0str("YES THEY CALLED ME TO SEND\r\n");
    esp01_rcv_buff[0] = '\0';// clear the buffer before using it   
    sprintf(no_of_chara,"%d",l);
    uart1str("AT+CIPSEND=");
    uart1str(no_of_chara);
    uart1str("\r\n");
    delay_ms(3000);
    uart1str(buff);
    uart1str("\r\n");
    uart0str("YES SEND THE DATA THROUGH UART TO ESP01\r\n");
    delay_ms(3000);
    t = 3;
    while(t)
    {
        RET = esp_response(2);
        if(RET == 1)
        {
            return 1;
        }
        else
        {
            delay_ms(3000);// wait for 3 seccond for message
            t--;
        }
    }
    return 0;
}
char esp_response(char op)		// 0 for unmached responce
{							//1 or 2 based on their responcd
    char ret = 0,state = 0;
    char i =0,j = 0,found = 0;			// well 1 mostly ok 2 is error
    if(is_there_data())  //check the top of stack
    {						
        uart0str("got a message\r\n");
        uart0str("\r\n");
        state = (char)ist;
        if(op == 2)// for esp_send function
        {
            state = 6;// if its not inti then its a send function
        }
        
        //this section to find which message it recived -----------------------
        for(j = 0;uartstack_pop(esp01_rcv_buff);j++)// to take each from stack
        {
            for(i=0;lut[state][i];i++)// to check with lut
            {
                if(strstr(esp01_rcv_buff,esp_responses[lut[state][i]])!=0)
                {
                    //for Debug purpose------------
                    uart0str("BREAKED\r\n");
                    uart0str("i: ");						
                    uart0tx(48+(lut[state][i])/10);
                    uart0tx(48+(lut[state][i])%10);
                    uart0str("\r\n");
                    uart0str("by: ");
                    uart0str(esp01_rcv_buff);
                    uart0str("\r\n");
                    //-----------------------------
                    
                    found =1;
                    break;    
                }
            }
            if(found)
            {
                break;
            }
        }
        uartstack_clr();// to clear the remaining message in stack
        if(lut[state][i] == 0)
        {   //for Debug purpose------------
            uart0str("did'nt MATCH\r\n");
            uart0str("reply : ");
            uart0str(esp01_rcv_buff);
            uart0str("\r\n");
            uart0str("i: ");						
            uart0tx(48+(lut[state][i])/10);
            uart0tx(48+(lut[state][i])%10);
            uart0str("\r\n");
            //-----------------------------
            
            return 0;// ment non on the case so unwanted response
        }
        //---------------------------------------------------------------
        
        
        // section for init functon responce-----------------------------------
        if(op == 1)// if it is called from init function only
        {
            switch(ist)
            {
                case at:
                    if(lut[state][i] == 1)
                    {
                        ret = 1;
                    }
                    break;
                case eccho_off:
                    if(lut[state][i] == 1)
                    {
                        ret = 1;
                    }
                    break;
                case wifi_mode:
                    if(lut[state][i] == 1)
                    {
                        ret = 1;
                    }
                    break;
                case is_wifi_connected:
                    if(lut[state][i] == 4)
                    {
                        ret = 1;// ment connected
                    }
                    else if(lut[state][i] == 7)	//no ap
                    {
                        ret = 2;// ment not connected
                    }
                    break;
                case connect_wifi:
                    if(lut[state][i] == 1)
                    {
                        ret = 1;
                    }
                    else if((lut[state][i] == 5)||(lut[state][i] == 6))
                    {
                        ret = 2;
                    }
                    break;
                case connect_server:
                    if((lut[state][i] == 10)||(lut[state][i] == 11))
                    {
                        ret = 1;// connected
                    }
                    else if(lut[state][i] == 9) // closed
                    {
                        ret = 2;// for error
                    }
                    break;
            }
        }
        //-----------------------------------------------------------------------------

        //for esp_send call---------------------------------------------
        else if(op ==2)// called from send function//
        {
            if((lut[state][i] == 1)||(lut[state][i] == 15))// ment send
            {
                ret = 1;
            }
            else
            {
                ret = 0;
            }
        }  
        //---------------------------------------------------------------  
    }
    return ret;
}
char esp_notification(char* m)// zero returened ment no message
{
    char i;
    if(uartstack_pop(esp01_rcv_buff))// got a message
    {
        uart0str("Got a notification");
        uart0str("\r\n");
        for(i=0;lut[7][i];i++)
        {
            if(strstr(esp01_rcv_buff,esp_responses[lut[7][i]])!=0)
            {
                break;
            }
        }
        if(lut[7][i] == 0)
        {
            uart0str("not matched noti \r\n");
			uart0str("msg : ");
			uart0str(esp01_rcv_buff); 
            uart0str("\r\n"); 
            return 0;
        }
        else
        {
            if(lut[7][i] ==8)
            {
                return 1;// if wifi disconnected
            }
            else if(lut[7][i] == 9 )
            {
                return 2;// if tcp dis connected
            }
            else if(lut[7][i] == 12 )
            {
                strcpy(m,esp01_rcv_buff);
                return 3;
            }
            else
            {
                return 0;// i dont know	what will be
            }
        }
    }
    return 0;
}
