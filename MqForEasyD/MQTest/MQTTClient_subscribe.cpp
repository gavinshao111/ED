/*
g++ MQTTClient_subscribe.cpp -I$ED/paho.mqtt.c/src -L$ED/paho.mqtt.c/build/output -lpaho-mqtt3c -DNO_PERSISTENCE=1 -o sub
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "MQTTClient.h"
#include <iostream>

#include<unistd.h>
#include<fcntl.h>  
#include<sys/types.h>  
#include <sys/socket.h>  
#include <sys/un.h>   
#define UNIX_DOMAIN "/tmp/UNIX.domain" 

using namespace std;

#define CLIENTID    "ExampleClientSub"
//#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L
#define MQTTCLIENT_PERSISTENCE_NONE 1
volatile MQTTClient_deliveryToken deliveredtoken;

int rwFd = -1;
bool ffmpegIsPush = false;

int parseMQ(const string& MQPL, bool& operationIsBegin, string& url);
void parseMQThenSendCmdToFfmpeg(string& MQPL);
int CreateSocket(void);

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char *)message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    
    string MQPL(payloadptr, message->payloadlen);
    string url = "";
    bool operationIsBegin = false;
    
    int rc = parseMQ(MQPL, operationIsBegin, url);
    if (rc < 0){
        cout << "parseMQ fail, rc = " << rc << endl;        
    }
    else
        sendCmdToFfmpeg(operationIsBegin, url);
    
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{    
    if (0 != CreateSocket()){
        cout << "create socket fail."<<endl;
        return 0;
    }
    
    string MQServerUrl = "tcp://120.27.188.84:1883";
    string topic = "/leapmotorNo1/videoinfoAsk";
    string tmp;
	char yOrN = 'n';
	
    cout<<"Default Server URL is "<<MQServerUrl<<"  Ok? (y / n) ";
    //cin>>tmp;
    //if('n' == tmp.at(0) || 'N' == tmp.at(0)){
	yOrN = getchar();
	if('n' == yOrN || 'N' == yOrN){
        cout<<"Input Server URL: ";
        cin>>MQServerUrl;    
    }
    
        
    cout<<"Default TOPIC is "<<topic<<"  Ok? (y / n) ";
	yOrN = getchar();
	if('n' == yOrN || 'N' == yOrN){
        cout<<"Input TOPIC: ";
        cin>>topic;    
    }

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, MQServerUrl.c_str(), CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    
    if (0 != MQServerUrl.compare(0, 15, "tcp://localhost")){
        cout<<"Connect with user name."<<endl;
        conn_opts.username = "easydarwin";
        conn_opts.password = "123456";
    }
    
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", topic.c_str(), CLIENTID, QOS);
    MQTTClient_subscribe(client, topic.c_str(), QOS);

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    
    return 0;
}

int CreateSocket(void){
    socklen_t clt_addr_len;  
    int listen_fd;  
    int com_fd;  
    int ret;  
    int i;  
    static char recv_buf[1024];   
    int len;  
    struct sockaddr_un clt_addr;  
    struct sockaddr_un srv_addr;  
    listen_fd=socket(PF_UNIX,SOCK_STREAM,0);  
    if(listen_fd<0)  
    {  
        perror("cannot create communication socket");  
        return 1;  
    }    
      
    //set server addr_param  
    srv_addr.sun_family=AF_UNIX;  
    strncpy(srv_addr.sun_path,UNIX_DOMAIN,sizeof(srv_addr.sun_path)-1);  
    unlink(UNIX_DOMAIN);  
    //bind sockfd & addr  
    ret=bind(listen_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));  
    if(ret==-1)  
    {  
        perror("cannot bind server socket");  
        close(listen_fd);  
        unlink(UNIX_DOMAIN);  
        return 1;  
    }  
    //listen sockfd   
    ret=listen(listen_fd,1);  
    if(ret==-1)  
    {  
        perror("cannot listen the client connect request");  
        close(listen_fd);  
        unlink(UNIX_DOMAIN);  
        return 1;  
    }  
    //have connect request use accept  
    len=sizeof(clt_addr);  
    com_fd=accept(listen_fd,(struct sockaddr*)&clt_addr,&len);  
    if(com_fd<0)  
    {  
        perror("cannot accept client connect request");  
        close(listen_fd);  
        unlink(UNIX_DOMAIN);  
        return 1;  
    }  
    //read and printf sent client info  
    rwFd = com_fd;
    return 0;
}

void sendCmdToFfmpeg(const bool& operationIsBegin, const string& url){
    
    
    if (-1 == rwFd){
        cout << "open socket fail." << endl;
        exit(EXIT_FAILURE);
    }

    char readBuf[2] = {0};
    
    if (!operationIsBegin){
        if (-1 == write(rwFd, "s", 1)){
            cout << "socket write fail." <<endl;
            exit(EXIT_FAILURE);
        }
        
        read(rwFd, readBuf, 1)
    }
    else{
        unsigned char urlLength = (unsigned char)url.length();
        string buf = 'b' + urlLength + url;
        int tmp = -1;
        int writtenLength = 0;
        
        for (; writtenLength < buf.length();){
             tmp = write(rwFd, buf.data()+writtenLength, buf.length() - writtenLength);
             if (-1 == tmp){
                cout << "socket write fail." <<endl;
                exit(EXIT_FAILURE);             
             }
             writtenLength += tmp;
             tmp = -1;
        }
        
    }
}
int parseMQ(const string& MQPL, bool& operationIsBegin, string& url){
    if (MQPL.empty())
        return -1;
    int posOfUrl = string::npos;
    int posOfOperation = string::npos;
    
    const string UrlMarker = "\"URL\":\"";
    const string OperationMarker = "\"Operation\":\"";
    
    posOfUrl = MQPL.find(UrlMarker);
    if (string::npos == posOfUrl)
        return -2;
    
    posOfUrl += UrlMarker.length();
    
    url = MQPL.substr(posOfUrl, MQPL.find('\"', posOfUrl));
    
    
    posOfOperation = MQPL.find(OperationMarker, posOfUrl);
    if (string::npos == posOfOperation)
        return -3;
    posOfOperation += OperationMarker.length();
    
    if (MQPL.compare(posOfOperation, 4, "Stop"))
        operationIsBegin = false;
    else if (MQPL.compare(posOfOperation, 5, "Begin"))
        operationIsBegin = true;
    else
        return -4;
    
    return 0;
}