#ifndef __ESP01_H_
#define __ESP01_H_
char esp_init(char i);
char esp_send(char* buff);
void get_esp01_data(char*buf1,char*buf2,char*buf3,char*buf4);
char esp_response(char);//1 for init and 2 for esp01 send
char esp_notification(char*);
char esp_status(void);

typedef enum{
    at = 0,
    eccho_off,
    wifi_mode,
    is_wifi_connected,
    connect_wifi,
    connect_server,
    exit_init
}init_state;
#endif
