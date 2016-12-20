#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"{
    #include "MQTTAsync.h"
    #include "MQTTClient.h"
}

#include "mainProcess.h"

#define QOS         1
#define MQTTCLIENT_PERSISTENCE_NONE 1
#define MAXPATHLEN 100

int publishMq(const char *url, const char *clientId, const char *Topic, const char *PayLoad, const int& timeout) {
    MQTTClient client;
    int rc = 0;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.connectTimeout = timeout;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    if (rc = MQTTClient_create(&client, url, clientId,
            MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect create MQTTClient, return code %d\n", rc);
        return -1;
    }
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = "easydarwin";
    conn_opts.password = "123456";

#if 1
    char pathOfServerPublicKey[MAXPATHLEN] = {0};
    char pathOfPrivateKey[MAXPATHLEN] = {0};
    snprintf(pathOfPrivateKey, MAXPATHLEN - 1, "%s/emqtt.key", getenv("ED"));
    snprintf(pathOfServerPublicKey, MAXPATHLEN - 1, "%s/emqtt.pem", getenv("ED"));
    MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
       
    ssl_opts.trustStore = pathOfServerPublicKey;
    ssl_opts.keyStore = pathOfServerPublicKey;
    ssl_opts.privateKey = pathOfPrivateKey;   
    ssl_opts.enableServerCertAuth = 0;
    conn_opts.ssl = &ssl_opts;
#endif

    
    if (rc = MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect to MQ server, return code %d\n", rc);
        return -2;
    }
    pubmsg.payload = (void *)PayLoad;
    pubmsg.payloadlen = strlen(PayLoad);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    if (rc = MQTTClient_publishMessage(client, Topic, &pubmsg, &token) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to publishMessage to MQ server, return code %d\n", rc);
        return -3;
    }              

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    
    return 0;
}
