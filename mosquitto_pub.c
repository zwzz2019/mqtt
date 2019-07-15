#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <mosquitto.h>
#include <time.h>

struct mosquitto *mosq = NULL;

/* Signals handlers */
void signal_end(int signum)
{
    //mosquitto_reconnect(mosq);
    /*printf("Receive a end signal\n");*/
    mosquitto_disconnect(mosq);
}

/* Mosquitto callbacks functions */
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("on_connect\n");
    /*printf("\tstatus code  | %d\n", rc);*/
}

//void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
//{
    /*printf("on_disconnect\n");
    printf("\tstatus code  | %d\n", rc);*/
//}

void on_publish(struct mosquitto *mosq, void *obj, int rc)
{
    /*printf("on_publish\n");
    printf("\tstatus code  | %d\n", rc);*/
    //ioctl(wdf, WDIOC_KEEPALIVE, 0);
    time_t timep;
    time (&timep);
    printf("%s",ctime(&timep));
    printf("send msg to server success\n");
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    /*printf("on_message :\n");
    printf("\tmid         | %d\n", message->mid);
    printf("\ttopic       | %s\n", message->topic);*/
    printf("Receive Order | %s\n", message->payload);
    printf("Order length  | %d\n", message->payloadlen);
    /*printf("\tqos         | %d\n", message->qos);
    printf("\tretain      | %d\n", message->retain);*/
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    printf("subscribe success\n");
    /*printf("\tid          | %d\n", mid);
    printf("\tqos_count   | %d\n", qos_count);
    int i = 0;
    for( i = 0; i < qos_count; i++)
        printf("\tqos sub %d   | %d\n", granted_qos[i]);*/
}

/*void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
    printf("on_unsubscribe\n");
    printf("\tid          | %d\n", mid);
}*/

/*void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
    printf("[LOG] %s\n", str);
}*/

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

    int fd, count_r,count_t,i;
    unsigned char buf[1],buff[27];  // the reading & writing buffer
    unsigned int Temperature[2],CO2[2],TVOC[2],Humidity[2],Illuminance[2],Wap[3];
    unsigned int co2,tvoc,wap;
    double tem,hum,illu;
    struct termios opt;       //uart  confige structure
    int sum,timeout;
    char sendbuf[]="hello";

    /*open tty01 in order to read sensor data*/
    if ((fd = open("/dev/ttyO1", O_RDWR)) < 0)
    {
        perror("UART: Failed to open the UART device:ttyO1.\n");
        sleep(1);
        return 0;
    }
    tcgetattr(fd, &opt); // get the configuration of the UART
    // config UART
    opt.c_cflag = B115200 | CS8 | CREAD | CLOCAL;
    // 9600 baud, 8-bit, enable receiver, no modem control lines
    opt.c_iflag = IGNPAR | ICRNL;
    // ignore partity errors, CR -> newline
    opt.c_iflag &= ~(INLCR | ICRNL); //不要回车和换行转换
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);
    opt.c_oflag &= ~OPOST;
    //turn off software stream control
    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    //关闭回显功能,关闭经典输入 改用原始输入
    tcflush(fd,TCIOFLUSH);        // 清理输入输出缓冲区
    tcsetattr(fd, TCSANOW, &opt); // changes occur immmediately

    /*init the watchdog*/
    /*if ((wdf = open("/dev/watchdog", O_RDWR)) < 0)
    {
          printf("open dog true\n");
          sleep(1);
          continue;
    }
    ioctl(wdf, WDIOC_GETTIMEOUT, &timeout);
    printf("Default timeout: %d\n", timeout);

    timeout = 100;
    printf("Set timeout to %d\n", timeout);
    ioctl(wdf, WDIOC_SETTIMEOUT, &timeout);

    ioctl(wdf, WDIOC_GETTIMEOUT, &timeout);
    printf("New timeout: %d\n", timeout);*/


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
    mosq = mosquitto_new(NULL, clean_session, NULL);//the second argument is set to true, when client break, broker will hold the topic and message until reconnect
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

    // /*set reconnect to the broker*/
    // switch( retval = mosquitto_reconnect(mosq) )
    // {
    //     case MOSQ_ERR_INVAL:
    //         fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));
    //         return EXIT_FAILURE;
    //         break;
    //     case MOSQ_ERR_ERRNO:
    //         fprintf(stderr, "Error : %s\n", strerror(errno));
    //         return EXIT_FAILURE;
    //         break;
    // }
    // /*printf("Mosquitto client reconnect ...\n");*/


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

    /* Define mosquitto callbacks */
    mosquitto_connect_callback_set(mosq, on_connect);
    //mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    //mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
    //mosquitto_log_callback_set(mosq, on_log);

    /*start to read data and process*/
    while(1)
    {
        sleep(1);
        mosquitto_publish( mosq, NULL, "mqtt", strlen(sendbuf), sendbuf, 0, 0);
    }




    /* Infinite network loop
     * return on error or on call to mosquitto_disconnect()
     */
    retval = mosquitto_loop_forever(mosq, -1, 1);
    if( retval != MOSQ_ERR_SUCCESS )
    {
        //mosquitto_reconnect(mosq);
        fprintf(stderr, "Error : %s\n", mosquitto_strerror(retval));

        /* Call to free resources associated with the library*/
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return EXIT_FAILURE;
    }

    /* Call to free resources associated with the library */
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return EXIT_SUCCESS;
}

