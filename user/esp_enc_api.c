/*
-----------------------------------------------------------------------------------------
Author:         Mark F (Cicero-MF) mark@cdelec.co.za    
Known Issues:   none
Version:        22.05.2016
Description:    API or 'tunnel' for espconn and enc28j60 stack callbacks and setup

-----------------------------------------------------------------------------------------
*/
#include "esp8266.h"
#include "httpd.h"
#include "stack.h"
#include "esp_enc_api.h"

/* Use this to send data - it looks at the type field to determine interface */
sint8 esp_enc_api_sendData (struct espconn *conn, u8* dataIn, int len) {
  if (conn->type != ESPCONN_TCP_WIRED) {
    return espconn_sent(conn, dataIn, len);
  } else {
    return stack_sendData(conn, dataIn, len);
  }  
}
 
/* Use this to disconnect */
sint8 esp_enc_api_disconnect (struct espconn *espconn) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_disconnect(espconn);
  } else {
    stack_connDisconnect (espconn);
    return 1;
  }  
} 

/* Use this to register the correct callback for receiving */
sint8 esp_enc_api_regist_recvcb (struct espconn *espconn, espconn_recv_callback recv_cb) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_regist_recvcb(espconn, recv_cb);
  } else {
    espconn->recv_callback = recv_cb;
    return 1;
  }  
}
  

/* Use this to register the correct callback for receiving */
sint8 esp_enc_api_regist_reconcb (struct espconn *espconn, espconn_reconnect_callback recon_cb) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_regist_reconcb(espconn, recon_cb);
  } else {
    espconn->proto.tcp->reconnect_callback = recon_cb;
    return 1;
  }  
}  
	
/* Use this to register the disconnect callback */
sint8 esp_enc_api_regist_disconcb (struct espconn *espconn, espconn_connect_callback discon_cb) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_regist_disconcb(espconn, discon_cb);
  } else {
    espconn->proto.tcp->disconnect_callback = discon_cb;
    return 1;
  }  
}   
  
/* Use this to register the sent callback */
sint8 esp_enc_api_regist_sentcb (struct espconn *espconn, espconn_sent_callback sent_cb) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_regist_sentcb(espconn, sent_cb);
  } else {
    espconn->sent_callback = sent_cb;
    return 1;
  }  
}


/* Use this to register the connect callback */
sint8 esp_enc_api_regist_connectcb (struct espconn *espconn, espconn_connect_callback connect_cb) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_regist_connectcb(espconn, connect_cb);
  } else {
    espconn->proto.tcp->connect_callback = connect_cb;
    return 1;
  }  
}

	
/* Use this to accept */
sint8 esp_enc_api_connaccept (struct espconn *espconn, u8 stack_func) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
   return espconn_accept(espconn);
  } else {
    return stack_register_tcp_accept(espconn, stack_func);
  }  
}   
  
/* Use this to set max connections allowed */
sint8 esp_enc_api_tcp_set_max_con_allowed (struct espconn *espconn, uint8 num) {
  if (espconn->type != ESPCONN_TCP_WIRED) {
    return espconn_tcp_set_max_con_allow(espconn, num);
  } else {
    /* FIXME: Set max conns allowed by stack.c for httpd */
    return 1;
  }  
}     
