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

time_t timep;

int fd;

/* Signals handlers */
void signal_end(int signum)
{
    printf("Receive a end signal\n");
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    close(fd);
    exit(0);
}

/* Mosquitto callbacks functions */
void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("on_connect\n");
}


void on_publish(struct mosquitto *mosq, void *obj, int rc)
{
    time (&timep);
    printf("%s",ctime(&timep));
    printf("send msg to server success\n");
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

    int count_r,count_t,i;
    unsigned char buff[27];  // the reading & writing buffer
    unsigned int Temperature[2],CO2[2],TVOC[2],Humidity[2],Illuminance[2],Wap[3];
    unsigned int co2,tvoc,wap;
    double tem,hum,illu;
    struct termios opt;       //uart  confige structure
    int sum,timeout,num;
    char sendbuf[100];

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
    // 115200 baud, 8-bit, enable receiver, no modem control lines
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

    /* Signals handlers */
    signal(SIGINT, signal_end);
    signal(SIGTERM, signal_end);

    /* Init mosquitto library */
    mosquitto_lib_init();
    mosquitto_lib_cleanup();

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


    /* Define mosquitto callbacks */
    mosquitto_connect_callback_set(mosq, on_connect);
    //mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_publish_callback_set(mosq, on_publish);

    /*start to read data and process*/
    while(1)
    {
        retval = mosquitto_loop(mosq, -1, 1);
        if(retval)
        {
          printf("%s\n",mosquitto_strerror(retval));
          sleep(2);
          mosquitto_reconnect(mosq);
        }
        count_r = read(fd,buff,27);
        if (count_r!=27||buff[26]!=0xa)
        {
          tcflush(fd,TCIOFLUSH);
          continue;
        }
        /*print a completed read buff*/
        for(int i=0;i<27;i++)
        {
          printf("%x ", buff[i]);
        }
        printf("\n");

        if(buff[0]==2&&buff[10]==0)
        {
            Wap[2]=(unsigned int)(buff[8]*65536);
            Wap[1]=(unsigned int)(buff[7]*256);
            Wap[0]=(unsigned int)(buff[6]);
            wap=Wap[0]+Wap[1]+Wap[2];
            printf("wap:%d\n",wap);
            sprintf(sendbuf,"0100%06d",wap);
            printf("%s  %d\n",sendbuf,strlen(sendbuf));
            retval = mosquitto_publish( mosq, NULL, "mqtt", strlen(sendbuf), sendbuf, 0, 0);
            printf("%s\n",mosquitto_strerror(retval));
            continue;
        }

        CO2[0]=(unsigned int)(buff[6]*256);
        CO2[1]=(unsigned int)(buff[7]);
        co2=CO2[0]+CO2[1];
        printf("co2:%d\n",co2);
        if(buff[0]==0)
        {
          sum = sprintf(sendbuf,"0101%06d",co2);
        }
        else if(buff[0]==1)
        {
          sum = sprintf(sendbuf,"0111%06d",co2);
        }
        else if(buff[0]==2&&buff[10]!=0)
        {
          sum = sprintf(sendbuf,"0201%06d",co2);
        }

        TVOC[0]=(unsigned int)(buff[8]*256);
        TVOC[1]=(unsigned int)(buff[9]);
        tvoc=TVOC[0]+TVOC[1];
        printf("tvoc:%d\n",tvoc);
        sum += sprintf(sendbuf + sum, "%010d",tvoc);
        Temperature[0]=(unsigned int)(buff[14]*256);
        Temperature[1]=(unsigned int)(buff[15]);
        tem=(double)(Temperature[0]+Temperature[1]);
        tem=tem/65536*175.72-46.85;
        printf("tem:%.3lf\n",tem);
        sum += sprintf(sendbuf + sum, "%010.3lf",tem);
        Humidity[0]=(unsigned int)(buff[16]*256);
        Humidity[1]=(unsigned int)(buff[17]);
        hum=(double)(Humidity[0]+Humidity[1]);
        hum=hum/65536*125-6;
        printf("hum:%.3lf%%\n",hum);
        sum += sprintf(sendbuf + sum, "%010.3lf",hum);
        Illuminance[0]=(unsigned int)(buff[18]*256);
        Illuminance[1]=(unsigned int)(buff[19]);
        Illuminance[0]=Illuminance[0]+Illuminance[1];
        Illuminance[1]=Illuminance[0]/4096;
        Illuminance[0]=Illuminance[0]%4096;
        illu=pow(2,Illuminance[1])*((double)(Illuminance[0]*0.01));
        printf("illu:%.3lf\n",illu);
        sum += sprintf(sendbuf + sum, "%010.3lf",illu);
        retval = mosquitto_publish( mosq, NULL, "mqtt", strlen(sendbuf), sendbuf, 0, 0);
        printf("%s\n",mosquitto_strerror(retval));
        retval = mosquitto_loop(mosq, -1, 1);
        if(retval)
        {
          printf("%s\n",mosquitto_strerror(retval));
          sleep(2);
          mosquitto_reconnect(mosq);
        }
    }/*end while receive*/

}

