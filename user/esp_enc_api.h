	/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        18.05.2016
Description:    Header for API or 'tunnel' for espconn and enc28j60 stack callbacks and setup

-----------------------------------------------------------------------------------------
*/
#ifndef _ESP_ENC_API_H
  #define _ESP_ENC_API_H

  #include "esp8266.h"
  #include "httpd.h"
  
  /* Sneaky addition to the type enum in struct espconn to distinguish ENC conns */
  #define ESPCONN_TCP_WIRED 0x40
  
  /* Module types supported 
    Add in what ever other module you need here, 
    and make sure they're used in stack_register_tcp_accept()
  */
  #define STACK_NA      0x00
  #define STACK_HTTPD   0x01
  #define STACK_MQTT    0x02  

  sint8 esp_enc_api_sendData (struct espconn *conn, u8* dataIn, int len);
  sint8 esp_enc_api_disconnect (struct espconn *espconn);
  sint8 esp_enc_api_regist_recvcb (struct espconn *espconn, espconn_recv_callback recv_cb);
  sint8 esp_enc_api_regist_reconcb (struct espconn *espconn, espconn_reconnect_callback recon_cb);
  sint8 esp_enc_api_regist_disconcb (struct espconn *espconn, espconn_connect_callback discon_cb);
  sint8 esp_enc_api_regist_sentcb (struct espconn *espconn, espconn_sent_callback sent_cb);
  sint8 esp_enc_api_regist_connectcb (struct espconn *espconn, espconn_connect_callback connect_cb);
  sint8 esp_enc_api_connaccept (struct espconn *espconn, u8 stack_func);
  sint8 esp_enc_api_tcp_set_max_con_allowed (struct espconn *espconn, uint8 num);


#endif /* _ESP_ENC_API_H */
