/*
g++ MQTTClient_publish.cpp -I../../../paho.mqtt.c/src -L../../../paho.mqtt.c/build/output -lpaho-mqtt3c -DNO_PERSISTENCE=1 -o pub
*/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
#include "MQTTClient.h"
using namespace std;

//#define ADDRESS     "ssl://localhost:1883"
#define CLIENTID    "mqtt.c_MQTTClient_publish"

#define QOS         1
#define TIMEOUT     10000L

#define MQTTCLIENT_PERSISTENCE_NONE 1



int main(int argc, char* argv[])
{
    string url = "tcp://120.27.188.84:1883";
    string payload = "Hello world.";
    string topic = "/1234/videoinfoAsk";
    string tmp;
	char yOrN = 'n';
	
    cout<<"Default Server URL is "<<url<<"  Ok? (y / n) ";
	yOrN = getchar();
	if('n' == yOrN || 'N' == yOrN){
        cout<<"Input Server URL: ";
        cin>>url;    
    }
    cout<<"Default TOPIC is "<<topic<<"  Ok? (y / n) ";
	yOrN = getchar();
	if('n' == yOrN || 'N' == yOrN){
        cout<<"Input TOPIC: ";
        cin>>topic;    
    }    
    cout<<"Default PAYLOAD is "<<payload<<"  Ok? (y / n) ";
	yOrN = getchar();
	if('n' == yOrN || 'N' == yOrN){
        cout<<"Input PAYLOAD: ";
        cin>>payload;    
    }
    
    cout<<"Server URL: "<<url<<" TOPIC: "<<topic<<" PAYLOAD: "<<payload<<endl;
    
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.connectTimeout = 3;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, url.c_str(), CLIENTID,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    
    if (0 != url.compare(0, 15, "tcp://localhost")){
        cout<<"Connect with user name."<<endl;
        conn_opts.username = "easydarwin";
        conn_opts.password = "123456";
    }
	
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
            printf("Failed to connect, return code %d\n", rc);
            exit(EXIT_FAILURE);
    }
    pubmsg.payload = (void*)payload.c_str();
    pubmsg.payloadlen = payload.length();
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    if ((rc = MQTTClient_publishMessage(client, topic.c_str(), &pubmsg, &token)) != MQTTCLIENT_SUCCESS){
		cout<<"MQTTClient_publishMessage rc "<<rc<<endl;
		return 0;
    }
    cout<<"Message published."<<endl;
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\nrc = %d\n", token, rc);
    
    MQTTClient_disconnect(client, 10000);
    cout<<"disconnected."<<endl;
    MQTTClient_destroy(&client);
    return 0;
}