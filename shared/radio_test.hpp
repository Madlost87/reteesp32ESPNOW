#pragma once
namespace radioTest {
// Atomic admission prevents HTTP requests from touching the radio transaction.
std::atomic<unsigned> state{0},sentSeq{0},receivedCount{0},receivedSeq{0};
std::atomic<uint32_t> receivedAt{0};
portMUX_TYPE textMux=portMUX_INITIALIZER_UNLOCKED;
char sentText[TextCapacity+1]{},receivedText[TextCapacity+1]{};
void outgoingText(char* target){portENTER_CRITICAL(&textMux);memcpy(target,sentText,sizeof(sentText));portEXIT_CRITICAL(&textMux);}
String jsonText(const char* text) {
  String out="\"";
  for(const char* p=text;*p;++p){if(*p=='"' || *p=='\\')out+='\\';out+=*p;}
  out+='"';return out;
}
esp_err_t page(httpd_req_t* req) {
  httpd_resp_set_type(req,"text/html; charset=utf-8");
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  return httpd_resp_send(req,radioPage,sizeof(radioPage)-1);
}
esp_err_t status(httpd_req_t* req) {
  char sent[TextCapacity+1],incomingText[TextCapacity+1];
  portENTER_CRITICAL(&textMux);memcpy(sent,sentText,sizeof(sent));memcpy(incomingText,receivedText,sizeof(incomingText));portEXIT_CRITICAL(&textMux);
  char json[420];const auto count=receivedCount.load();
  snprintf(json,sizeof(json),"{\"role\":\"%s\",\"version\":\"%s\",\"state\":%u,\"sent_seq\":%u,\"received_count\":%u,\"received_seq\":%u,\"received_age_ms\":%ld}",
    role,version,state.load(),sentSeq.load(),count,receivedSeq.load(),count?long(millis()-receivedAt.load()):-1L);
  httpd_resp_set_type(req,"application/json");httpd_resp_set_hdr(req,"Cache-Control","no-store");
  String body=json;body.remove(body.length()-1);body+=",\"sent_text\":";body+=jsonText(sent);body+=",\"received_text\":";body+=jsonText(incomingText);body+="}";
  return httpd_resp_send(req,body.c_str(),body.length());
}
esp_err_t request(httpd_req_t* req) {
  char header[8];
  // No CORS: cross-origin forms cannot supply this custom header.
  if(req->content_len<=0 || req->content_len>TextCapacity || httpd_req_get_hdr_value_str(req,"X-Radio-Test",header,sizeof(header))!=ESP_OK || strcmp(header,"1"))
    return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Messaggio richiesto: massimo 48 caratteri");
  char text[TextCapacity+1]{};size_t length=0;
  while(length<req->content_len) {
    int n=httpd_req_recv(req,text+length,req->content_len-length);
    if(n<=0)return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Corpo incompleto");
    length+=size_t(n);
  }
  bool nonspace=false;for(size_t i=0;i<length;++i)if(text[i]!=' ')nonspace=true;
  if(!nonspace || !validText(text,length))return httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"Testo non valido: massimo 48 caratteri, una riga");
  portENTER_CRITICAL(&textMux);
  const auto old=state.load();
  if(old==1 || old==2) {
    portEXIT_CRITICAL(&textMux);httpd_resp_set_status(req,"409 Conflict");return httpd_resp_send(req,"Invio gia in corso",HTTPD_RESP_USE_STRLEN);
  }
  memcpy(sentText,text,sizeof(sentText));state.store(1);
  portEXIT_CRITICAL(&textMux);
  httpd_resp_set_status(req,"202 Accepted");httpd_resp_set_type(req,"application/json");
  return httpd_resp_send(req,"{\"queued\":true}",HTTPD_RESP_USE_STRLEN);
}
void routes(httpd_handle_t server) {
  httpd_uri_t route{};route.uri="/radio";route.method=HTTP_GET;route.handler=page;
  httpd_register_uri_handler(server,&route);
#if ROLE_TX
  route.uri="/";httpd_register_uri_handler(server,&route);
#endif
  route.uri="/api/radio-test";route.handler=status;httpd_register_uri_handler(server,&route);
  route.method=HTTP_POST;route.handler=request;httpd_register_uri_handler(server,&route);
}
void receive(const Message& m) {
  if(m.event!=Event::RadioTest)return;
  portENTER_CRITICAL(&textMux);memcpy(receivedText,m.text,sizeof(receivedText));portEXIT_CRITICAL(&textMux);
  receivedSeq.store(m.seq);receivedAt.store(millis());++receivedCount;
}
}
