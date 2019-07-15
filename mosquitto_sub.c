#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <mosquitto.h>
#include <time.h>

#include <mysql.h>

struct mosquitto *mosq = NULL;

MYSQL *conn_ptr;
time_t timep;

/* Signals handlers */
void signal_end(int signum)
{
    //printf("Receive a end signal\n");
    mosquitto_disconnect(mosq);
}

/* Mosquitto callbacks functions */
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    /*printf("on_connect\n");
    printf("\tstatus code  | %d\n", rc);*/
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
    /*printf("on_disconnect\n");
    printf("\tstatus code  | %d\n", rc);*/
}

void on_publish(struct mosquitto *mosq, void *obj, int rc)
{
    /*printf("on_publish\n");
    printf("\tstatus code  | %d\n", rc);*/
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    int  res;
    char  cmd[1024]={0};
    time (&timep);
    printf("%s",ctime(&timep));

    printf("Receive Order | %s\n", message->payload);
    printf("Order length  | %d\n", message->payloadlen);

    /*生成数据库语句*/
    sprintf(cmd, "insert into test(time,message) values('%s','%s')",ctime(&timep),message->payload);
    res = mysql_query(conn_ptr, cmd);

    if(!res)  /*ret != 0 表示出错 */
    {
      printf("Inserted by %lu rows\n",(unsigned long)mysql_affected_rows(conn_ptr));
    }
    else  /*打印出错及相关信息*/
    {
      fprintf(stderr, "Insert error %d:%s\n", mysql_errno(conn_ptr), mysql_error(conn_ptr));
      return;
    }

}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    printf("subscribe success\n");
    /*printf("\tid          | %d\n", mid);
    printf("\tqos_count   | %d\n", qos_count);*/
    int i = 0;
    /*for( i = 0; i < qos_count; i++)
        printf("\tqos sub %d   | %d\n", granted_qos[i]);*/
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
    /*printf("on_unsubscribe\n");
    printf("\tid          | %d\n", mid);*/
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
    /*printf("[LOG] %s\n", str);*/
}

/* Main function  */
int main(int argc, char **argv)
{
    int major = 0, minor = 0, revision = 0;
    bool clean_session = true;
    char *host = "129.204.181.40";
    char *username = "zwz";
    char *password = "123456";
    int port = 1883;
    int keepalive = 60;
    char mqtt_message[200];
    char topic[200];
    int retval = 0;

    char    ipv4[20],temperator[10],humidity[10],illuminance[10],co2[10],tvoc[10],wap[10];/*定义所有传感器数组*/

    /* Signals handlers */
    signal(SIGINT, signal_end);
    signal(SIGTERM, signal_end);

    /* Show library version */
    //mosquitto_lib_version(&major, &minor, &revision);
    /*printf("Mosquitto library version : %d.%d.%d\n", major, minor, revision);*/

    /* Init mosquitto library */
    mosquitto_lib_init();
    //mosquitto_lib_cleanup();

    /* Create a new mosquitto client instance */
    mosq = mosquitto_new(NULL, clean_session, NULL);//the second argument is set to false, when client break, broker will hold the topic and message until reconnect
    if( mosq == NULL )
    {
        switch(errno){
            case ENOMEM:
                fprintf(stderr, "Error: Out of memory.\n");
                break;
            case EINVAL:
                fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
                break;
        }
        mosquitto_lib_cleanup();
        return EXIT_FAILURE;
    }

    /*check the username and password*/
    switch( retval=mosquitto_username_pw_set(mosq,username,password) )
    {
      case MOSQ_ERR_INVAL:
          fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));
          return EXIT_FAILURE;
          break;
      case MOSQ_ERR_NOMEM:
          fprintf(stderr, "Error : %s\n", strerror(errno));
          return EXIT_FAILURE;
          break;
    }
    /*printf("Mosquitto user set success ...\n");*/


    /*connect to the broker*/
    switch( retval = mosquitto_connect(mosq, host, port, keepalive) )
    {
        case MOSQ_ERR_INVAL:
            fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));
            return EXIT_FAILURE;
            break;
        case MOSQ_ERR_ERRNO:
            fprintf(stderr, "Error : %s\n", strerror(errno));
            return EXIT_FAILURE;
            break;
    }
    /*printf("Mosquitto client started ...\n");*/

    /*subscribe a topic, set Qos to 2*/
    switch( retval = mosquitto_subscribe(mosq, NULL, "mqtt", 2) )
    {
        case MOSQ_ERR_SUCCESS:
            /*printf("Subscription success.\n");*/
            break;
        case MOSQ_ERR_INVAL:
        case MOSQ_ERR_NOMEM:
        case MOSQ_ERR_NO_CONN:
            fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));
    }

    /*建立数据库连接*/
    conn_ptr = mysql_init(NULL);
    if(!conn_ptr)
    {
      fprintf(stderr,"mysql_init failed\n");
      return 0;
    }

    /*数据库连接参数设置*/
    conn_ptr = mysql_real_connect(conn_ptr,"localhost","root","1234!@#$zwzZWZ..","mosquitto_test",0,NULL,0);

    if(conn_ptr)
      printf("Connection database success\n");
    else
    {
      return 0;
    }


    /* Define mosquitto callbacks */
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
    mosquitto_log_callback_set(mosq, on_log);

    /* Infinite network loop
     * return on error or on call to mosquitto_disconnect()
     */
    retval = mosquitto_loop_forever(mosq, -1, 1);
    if( retval != MOSQ_ERR_SUCCESS )
    {
        fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));

        /* Call to free resources associated with the library */
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return EXIT_FAILURE;
    }

    /* Call to free resources associated with the library */
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return EXIT_SUCCESS;
}

