 /*
  * Copyright (c) 2017, 2021 The Linux Foundation. All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions are
  * met:
  *  * Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  *  * Redistributions in binary form must reproduce the above
  *    copyright notice, this list of conditions and the following
  *    disclaimer in the documentation and/or other materials provided
  *    with the distribution.
  *  * Neither the name of The Linux Foundation nor the names of its
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
  * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
  * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
  * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
  * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#include <iostream>
#include <string.h>
#include <chrono>
#include <hardware/bluetooth.h>
#include <hardware/bt_sock.h>
#include "osi/include/alarm.h"
#include "spp_client.hpp"
#include "Gap.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <linux/netlink.h>
#include <pthread.h>

#define LOGTAG_SPP_CLIENT "SPP_CLIENT "


#define SPP_CLIENT_APP_UID (2)

#define VALID_CLI_SOCFD(FD) (-1 == FD?0:1)
#define RESET_CLI_SOCFD(FD) (FD=-1)

#define NETLINK_SPP         31
#define SPP_CTL_ON         1
#define SPP_CTL_OFF        2
#define SPP_CTL_DATA_LEN   9
#define SPP_CTL_OP_POS     0
#define SPP_CTL_INS_POS    1
#define SPP_CTL_MTU_POS    5
#define SPP_DATA_MTU       500

using namespace std;
using std::list;
using std::string;

Spp_Client *pSppClient = NULL;
typedef struct
{
    struct nlmsghdr n;
    uint8_t data[0];
} data_st;

 char srv_bdAddr[18];
 bool connctFlg=false;

static const uint8_t SPP_UUID[]        = {0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00,
                                           0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB};

//static const Uuid SPP_UUID[] = Uuid::FromString("00001101-0000-1000-8000-00805F9B34FB");

/* SPP client receive thread */
static pthread_t client_recv_thread = NULL;
static pthread_mutex_t client_recv_mutex;
static pthread_cond_t start_client_recv_cv;

/* SPP client send thread */
static pthread_t client_send_thread = NULL;
static pthread_mutex_t client_send_mutex;
static pthread_cond_t start_client_send_cv;

/* SPP client send data thread */
static pthread_t client_send_data_thread = NULL;
static pthread_mutex_t client_send_data_mutex;
static pthread_cond_t start_client_send_data_cv;

/* SPP client receive data thread */
static pthread_t client_recv_data_thread = NULL;
static pthread_mutex_t client_recv_data_mutex;
static pthread_cond_t start_client_recv_data_cv;

/* SPP client handle disconnect thread */
static pthread_t client_handle_disconnect_thread = NULL;
static pthread_mutex_t client_handle_disconnect_mutex;
static pthread_cond_t start_client_handle_disconnect_cv;

#ifdef __cplusplus
extern "C" {
#endif

void BtSppClientMsgHandler(void *msg) {

    BtEvent* pEvent = NULL;

    if(!msg) {
        ALOGE("Msg is NULL, bail out!!");
        return;
    }

    pEvent = ( BtEvent *) msg;

    switch(pEvent->event_id) {

        case PROFILE_API_START:

            ALOGD(LOGTAG_SPP_CLIENT "enable spp client");

            if (pSppClient) {
                pSppClient->HandleEnableClient();
            }
            break;

        case PROFILE_API_STOP:

            ALOGD(LOGTAG_SPP_CLIENT "Disable spp client");
            if (pSppClient) {
                pSppClient->HandleDisableClient();
            }
            break;

        default:

            ALOGD(LOGTAG_SPP_CLIENT "default event_id");
            if(pSppClient) {
               pSppClient->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }

    delete pEvent;
}

#ifdef __cplusplus
}
#endif

static void *spp_client_send_thread_func(void *in_param)
{
    pSppClient->sppcli_send_thread_handler();
    return NULL;
}

void Spp_Client::sppcli_send_thread_handler()
{
    while ( getState() != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        pthread_mutex_lock(&client_send_mutex);
        while ( getState() != STATE_SPP_CLIENT_SEND_FILE )
            pthread_cond_wait(&start_client_send_cv,&client_send_mutex);
        pthread_mutex_unlock(&client_send_mutex);
        if (getState() == STATE_SPP_CLIENT_INACTIVE) {
            ALOGD(LOGTAG_SPP_CLIENT "sppcli_send_thread_handler in STATE_SPP_CLIENT_INACTIVE state\n");
            break;
        }
        /* Send File */
        if(VALID_CLI_SOCFD(listen_data_socfd))
        {
            int status = snd_file(file_name.c_str(),listen_data_socfd);

            if(status != SUCCESS)
            {
                ALOGD(LOGTAG_SPP_CLIENT "Send file failed \n");
            }
        }
        else
        {
            ALOGD(LOGTAG_SPP_CLIENT "In-valid socfd, Connection may be lost");
        }

        if( !VALID_CLI_SOCFD(listen_data_socfd) )
        {
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_CLIENT_CONNECTED);
        }
    }

    pthread_mutex_destroy(&client_send_mutex);
    pthread_cond_destroy(&start_client_send_cv);
}

static void *spp_client_recv_thread_func(void *in_param)
{
    pSppClient->sppcli_recv_thread_handler();
    return NULL;
}

void Spp_Client::sppcli_recv_thread_handler()
{
    while ( getState() != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        pthread_mutex_lock(&client_recv_mutex);
        while ( getState() != STATE_SPP_CLIENT_RECEIVE_FILE )
            pthread_cond_wait(&start_client_recv_cv,&client_recv_mutex);
        pthread_mutex_unlock(&client_recv_mutex);
        if (getState() == STATE_SPP_CLIENT_INACTIVE)
            break;
        /* Receive File */
        int status = receive_file(file_name.c_str(),listen_data_socfd);
        if(status != SUCCESS)
        {
            ALOGD(LOGTAG_SPP_CLIENT "receive file failed \n");
        }

        if( !VALID_CLI_SOCFD(listen_data_socfd) )
        {
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_CLIENT_CONNECTED);
        }
    }
    pthread_mutex_destroy(&client_recv_mutex);
    pthread_cond_destroy(&start_client_recv_cv);
}

static void *spp_client_send_data_thread_func(void *in_param)
{
    pSppClient->sppcli_send_data_thread_handler();
    return NULL;
}


void Spp_Client::sppcli_send_data_thread_handler()
{
    while ( getState() != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        pthread_mutex_lock(&client_send_data_mutex);
        while ( getState() != STATE_SPP_CLIENT_SEND_DATA )
            pthread_cond_wait(&start_client_send_data_cv,&client_send_data_mutex);
        pthread_mutex_unlock(&client_send_data_mutex);
        if (getState() == STATE_SPP_CLIENT_INACTIVE) {
            ALOGD(LOGTAG_SPP_CLIENT "sppcli_send_data_thread_handler in STATE_SPP_CLIENT_INACTIVE state, break to end thread.\n");
            break;
        }
        /* Send data */
        if(VALID_CLI_SOCFD(listen_data_socfd))
        {
            int status = send_data(dataSize.c_str(),listen_data_socfd);

            if(status != SUCCESS)
            {
                ALOGD(LOGTAG_SPP_CLIENT "Send file failed \n");
            }
        }
        else
        {
            ALOGD(LOGTAG_SPP_CLIENT "In-valid socfd, Connection may be lost");
        }

        if( !VALID_CLI_SOCFD(listen_data_socfd) )
        {
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_CLIENT_CONNECTED);
        }
    }
    pthread_mutex_destroy(&client_send_data_mutex);
    pthread_cond_destroy(&start_client_send_data_cv);
}


static void *spp_client_recv_data_thread_func(void *in_param)
{

    pSppClient->sppcli_recv_data_thread_handler();
    return NULL;

}

void Spp_Client::sppcli_recv_data_thread_handler()
{
    while ( getState() != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        pthread_mutex_lock(&client_recv_data_mutex);
        while ( getState() != STATE_SPP_CLIENT_RECEIVE_DATA )
            pthread_cond_wait(&start_client_recv_data_cv,&client_recv_data_mutex);
        pthread_mutex_unlock(&client_recv_data_mutex);
        if (getState() == STATE_SPP_CLIENT_INACTIVE)
            break;
        /* Receive data */
        int status = receive_data(listen_data_socfd);
        if(status != SUCCESS)
        {
            ALOGD(LOGTAG_SPP_CLIENT "Receive data failed \n");
        }
        if( !VALID_CLI_SOCFD(listen_data_socfd) )
        {
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_CLIENT_CONNECTED);
        }
    }
    pthread_mutex_destroy(&client_recv_data_mutex);
    pthread_cond_destroy(&start_client_recv_data_cv);

}


static void *spp_client_write_thread_func(void *arg)
{
    pSppClient->spp_client_write_thread_handler();
    return NULL;
}

static void *spp_client_read_thread_func(void *arg)
{
    pSppClient->spp_client_read_thread_handler();
    return NULL;
}

void Spp_Client::spp_client_write_thread_close()
{
    ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_close");
    pthread_mutex_lock(&client_write_thread_mutex);
    client_write_thread_stop_thread = true;
    pthread_mutex_unlock(&client_write_thread_mutex);
}

void Spp_Client::spp_client_read_thread_close()
{
    ALOGD(LOGTAG_SPP_CLIENT "spp_client_read_thread_close");
    pthread_mutex_lock(&client_read_thread_mutex);
    client_read_thread_stop_thread = true;
    pthread_mutex_unlock(&client_read_thread_mutex);
}

int Spp_Client::spp_client_set_tty_driver_state(uint8_t ttyState)
{
    int instanceId = 1;//Only one Instance is used!!
    int maxFrame = spp_max_frame_size;
    int retVal = -1;
    struct
    {
        struct nlmsghdr n;
        uint8_t data[SPP_CTL_DATA_LEN];
    } ctl;

    ALOGD(LOGTAG_SPP_CLIENT "spp_client_set_tty_driver_state");

    memset(&ctl, 0x00, sizeof(ctl));

    ctl.data[SPP_CTL_OP_POS] = ttyState;
    memcpy(ctl.data + SPP_CTL_INS_POS, &instanceId, sizeof(instanceId));

    if (ttyState == SPP_CTL_ON)
    {
        memcpy(ctl.data + SPP_CTL_MTU_POS, &maxFrame, sizeof(maxFrame));
    }

    ctl.n.nlmsg_len = NLMSG_LENGTH(SPP_CTL_DATA_LEN);
    ctl.n.nlmsg_pid = client_read_thread;  /* self pid */
    ctl.n.nlmsg_type  = NETLINK_SPP;
    ctl.n.nlmsg_flags = NL_MSG_CTL;

    ALOGD(LOGTAG_SPP_CLIENT "instanceId is %d, maxFrame is %d, nlmsg_pid is %lu, serverSocket is %d, ctl.n.nlmsg_len is %d",
    instanceId, maxFrame, client_read_thread, kernel_sock_fd,  ctl.n.nlmsg_len);

    retVal = send(kernel_sock_fd, &ctl, ctl.n.nlmsg_len, 0);
    return retVal;
}

void Spp_Client::spp_client_write_thread_handler()
{
    char buffer[1024];
    char ctrl_msgbuf[CMSG_SPACE(1)];
    struct sockaddr_storage src_addr;
    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);
    int count;

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control= ctrl_msgbuf;
    message.msg_controllen= sizeof(ctrl_msgbuf);
    int retVal;

    ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_handler");
    while (1)
    {
        pthread_mutex_lock(&client_write_thread_mutex);

        if (client_write_thread_stop_thread)
        {
            if (tty_opened)
            {
                retVal = spp_client_set_tty_driver_state(SPP_CTL_OFF);

                if (retVal < 0)
                {
                    ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_handler sendmsg ctl message failed!!");
                    pthread_mutex_unlock(&client_write_thread_mutex);
                    continue;
                }
                tty_opened = false;
            }
            pthread_mutex_unlock(&client_write_thread_mutex);
            pthread_mutex_destroy(&client_write_thread_mutex);

           ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_handler Exiting Write Thread");

            if (client_read_thread_exit == 1)
            {
                ALOGD(LOGTAG_SPP_CLIENT "write thread close the socket");
                shutdown(kernel_sock_fd, SHUT_RDWR);
                close(kernel_sock_fd);
                kernel_sock_fd = 0;
            }

            client_write_thread_exit = 1;
            pthread_exit(NULL);
        }

        if (!tty_opened)
        {
            retVal = spp_client_set_tty_driver_state(SPP_CTL_ON);
            if (retVal < 0)
            {
                ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_handler retVal is %d, Sendmsg ctl message failed,%s", retVal, strerror(errno));
                pthread_mutex_unlock(&client_write_thread_mutex);
                continue;
            }
            tty_opened = true;
            ALOGD(LOGTAG_SPP_CLIENT "spp_client_write_thread_handler retVal is %d, Sendmsg ctl message successed", retVal);
        }

        pthread_mutex_unlock(&client_write_thread_mutex);
        ALOGD(LOGTAG_SPP_CLIENT "%s listen_data_socfd is %d", __func__, listen_data_socfd);
        count = recvmsg(listen_data_socfd, &message,0);

        ALOGD(LOGTAG_SPP_CLIENT "RECVD DATA, count=%d", (int)count);
        for(int i =0; i< count ; i++)
        {
            ALOGD(LOGTAG_SPP_CLIENT "buffer[%d]=%c",i, buffer[i]);
        }

        if( count > 0 )
        {
            int bytesSent = 0;

            while (bytesSent != count)
            {
                if (tty_opened)
                {
                    int currentSend = count - bytesSent;
                    data_st *send_data = (data_st *)osi_malloc(sizeof(data_st) + currentSend);

                    memset(send_data, 0x00, sizeof(data_st));
                    memcpy(send_data->data, buffer + bytesSent, currentSend);
                    send_data->n.nlmsg_len = NLMSG_LENGTH(currentSend);
                    send_data->n.nlmsg_pid = client_read_thread;         /* self pid */
                    send_data->n.nlmsg_type  = NETLINK_SPP;
                    send_data->n.nlmsg_flags = NL_MSG_CTL + 1;       /*data flags*/

                    retVal = send(kernel_sock_fd, send_data,
                                  send_data->n.nlmsg_len, 0);

                    ALOGD(LOGTAG_SPP_CLIENT "send ret is %d,currentSend is %d",  retVal, currentSend);
                    retVal = (int)(currentSend & 0x0000ffff);
                    osi_free(send_data);
                    send_data = NULL;
                }
                else
                {
                    ALOGD(LOGTAG_SPP_CLIENT "tty_opened is false");
                }
                bytesSent += retVal;
            }
        }
        else if( count == 0 )
        {
            ALOGD(LOGTAG_SPP_CLIENT "connection closed by the remote");
            RESET_CLI_SOCFD(listen_data_socfd);
            client_write_thread_stop_thread = true;
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else
        {
            ALOGD(LOGTAG_SPP_CLIENT "Receive Error count=%d", count);
        }
    }
}

void Spp_Client::spp_client_read_thread_handler()
{
    uint8_t *buffer = NULL;
    int retVal = 0;
    pthread_attr_t client_write_thread_attr;

    ALOGD(LOGTAG_SPP_CLIENT "spp_client_read_thread_handler");

    pthread_mutex_init(&client_write_thread_mutex, NULL);
    client_write_thread_stop_thread = false;

    pthread_attr_init(&client_write_thread_attr);
    pthread_attr_setdetachstate(&client_write_thread_attr, PTHREAD_CREATE_DETACHED);
    retVal = pthread_create(&client_write_thread, &client_write_thread_attr, spp_client_write_thread_func, NULL);
    pthread_attr_destroy(&client_write_thread_attr);
    if (retVal != 0)
   {
        shutdown(kernel_sock_fd, SHUT_RDWR);
        close(kernel_sock_fd);
        kernel_sock_fd = 0;
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create client_write_thread!\n");

        pthread_mutex_destroy(&client_write_thread_mutex);
        pthread_mutex_destroy(&client_read_thread_mutex);

        pthread_exit(NULL);
        return;
    }
    while (1)
    {
        if (client_read_thread_stop_thread)
        {
            ALOGD(LOGTAG_SPP_CLIENT "Exiting Read Thread!!");

            pthread_mutex_destroy(&client_read_thread_mutex);

            if (client_write_thread_exit == 1)
            {
                ALOGD(LOGTAG_SPP_CLIENT "read thread close the socket\n");
                shutdown(kernel_sock_fd, SHUT_RDWR);
                close(kernel_sock_fd);
                kernel_sock_fd = 0;
            }
            client_read_thread_exit =1;
            pthread_exit(NULL);
        }
        buffer = (uint8_t *)osi_malloc(spp_max_frame_size+ NLMSG_LENGTH(0));
        if (buffer == NULL)
        {
            ALOGD(LOGTAG_SPP_CLIENT "osi_malloc Failed!!\n");
        }
        memset(buffer, 0, spp_max_frame_size + NLMSG_LENGTH(0));

        retVal = recv(kernel_sock_fd, buffer, spp_max_frame_size, 0);
        if (retVal <= 0)
        {
            ALOGD(LOGTAG_SPP_CLIENT " retVal : %d \n",retVal);
            pthread_mutex_destroy(&client_read_thread_mutex);
            if (buffer != NULL)
            {
                osi_free(buffer);
            }
            break;
        }

        ALOGD(LOGTAG_SPP_CLIENT "read thread retVal : %d \n",retVal);
        ALOGD(LOGTAG_SPP_CLIENT "read buffer is : %s \n",buffer);

        if (buffer && strstr((char *)buffer, "client_read_thread_exit") != NULL)
        {
            ALOGD(LOGTAG_SPP_CLIENT "%s client_read_thread_exit !!!!\n",__func__);
            client_read_thread_stop_thread = true;
            osi_free(buffer);
            buffer = NULL;
        }
        else if (buffer && strstr((char *)buffer, "ttySpp") != NULL)
        {
            ALOGD(LOGTAG_SPP_CLIENT "%s ttySpp!!!!\n",__func__);
            osi_free(buffer);
            buffer = NULL;
        }
        else
        {
            int count=0 ;
            char send_buffer[1024];
            char extra_char[10] = {0xD, 0xA};  // 0XD:Carriage Return, 0xA: New Line Feed

            memcpy(send_buffer,buffer,retVal);
            memcpy(send_buffer+retVal, extra_char, 2);
            for(int i =0; i< retVal+2 ; i++)
            {
                ALOGD(LOGTAG_SPP_CLIENT "send_buffer[%d]=%c %d",i, send_buffer[i], send_buffer[i]);
            }

            pthread_mutex_lock(&client_read_thread_mutex);

            ALOGD(LOGTAG_SPP_CLIENT "%s listen_data_socfd is %d", __func__, listen_data_socfd);

            count = write(listen_data_socfd,&send_buffer,retVal+2);
            if(count < 0)
            {
                ALOGD(LOGTAG_SPP_CLIENT " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
            }
            else
            {
                ALOGD(LOGTAG_SPP_CLIENT " sent bytes (%d)",count);
            }
            pthread_mutex_unlock(&client_read_thread_mutex);
            osi_free(buffer);
        }
    }
}

int Spp_Client::spp_start_socket_threads()
{
    pthread_attr_t client_read_thread_attr;
    int retVal = 0;

    client_write_thread_exit = 0;
    client_read_thread_exit = 0;

   ALOGD(LOGTAG_SPP_CLIENT "spp_start_socket_threads");

    pthread_mutex_init(&client_read_thread_mutex, NULL);
    client_read_thread_stop_thread = false;

    pthread_mutex_init(&client_read_thread_mutex, NULL);

    pthread_attr_init(&client_read_thread_attr);
    pthread_attr_setdetachstate(&client_read_thread_attr, PTHREAD_CREATE_DETACHED);
    retVal =  pthread_create(&client_read_thread, &client_read_thread_attr, spp_client_read_thread_func, NULL);
    pthread_attr_destroy(&client_read_thread_attr);
    if (retVal != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create client_read_thread!\n");
        pthread_mutex_destroy(&client_read_thread_mutex);
        return -1 ;
    }
    return 0;
}


int Spp_Client::spp_client_create_socket()
{
    struct sockaddr_nl nladdr;
    int sz = 64 * 1024;
    int on = 1;
    int random = rand();

    ALOGD(LOGTAG_SPP_CLIENT "spp_client_create_socket random value:%d\n",random);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = (unsigned int)random;
    nladdr.nl_pad = 0;
    nladdr.nl_groups = 0;

    if ((kernel_sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_SPP)) < 0)
    {
         ALOGD(LOGTAG_SPP_CLIENT "Unable to create uevent socket");
        return -1;
    }

    if (setsockopt(kernel_sock_fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0)
    {
        ALOGD(LOGTAG_SPP_CLIENT "Unable to set uevent socket SO_RECBUFFORCE option");
        close(kernel_sock_fd);
        return -1;
    }

    if (setsockopt(kernel_sock_fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0)
    {
        ALOGD(LOGTAG_SPP_CLIENT "Unable to set uevent socket SO_PASSCRED option");
        close(kernel_sock_fd);
        return -1;
    }

    if ((setsockopt(kernel_sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
    {
        ALOGD(LOGTAG_SPP_CLIENT "Unable to set uevent socket SO_REUSEADDR option");
        close(kernel_sock_fd);
        return -1;
    }

    ALOGD(LOGTAG_SPP_CLIENT "netlink nladdr.nl_pid is %d", nladdr.nl_pid);

    if (bind(kernel_sock_fd, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0)
    {
        ALOGD(LOGTAG_SPP_CLIENT "Unable to bind nladdr %s", strerror(errno));
        close(kernel_sock_fd);
        return -1;
    }

    ALOGD(LOGTAG_SPP_CLIENT "kernel_sock_fd is %d", kernel_sock_fd);

    return kernel_sock_fd;
}

void Spp_Client::process_connect_message()
{

    ALOGD(LOGTAG_SPP_CLIENT "process_connect_message");

    char buffer[1024];
    char ctrl_msgbuf[CMSG_SPACE(1)];
    struct cmsghdr *pcmsg;
    int* p_acc_fd = NULL;
    struct sockaddr_storage src_addr;
    sock_connect_signal_t *pConnect_Sig = NULL;
    int i =0;

    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control= ctrl_msgbuf;
    message.msg_controllen= sizeof(ctrl_msgbuf);

    while ( getState() == STATE_SPP_CLIENT_CONNECTING )
    {

        // Wait for sock_connect_signal_t message
        int count = recvmsg(listen_data_socfd,&message,0);

        if (count==-1)
        {
            ALOGD(LOGTAG_SPP_CLIENT "recvmsg returned -1(%s)", strerror(errno));
        }
        else if (message.msg_flags&MSG_TRUNC)
        {
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]MSG_TRUNC\n");
        }
        else if ( 0 == count )
        {
           ALOGE(LOGTAG_SPP_CLIENT "ERROR listen sockfd closed");
           RESET_CLI_SOCFD(listen_data_socfd);
           change_state(STATE_SPP_CLIENT_IDLE);
           return;
        }

        if(count == 4)
        {
            for(i =0 ; i < count ; i++)
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] Received SCN msg[%d]= %02X\n",i, buffer[i]);

            memset(buffer,0,sizeof(buffer));
            continue;
        }

        // Handle the sock_connect_signal - remote address, server channel and status is received.

        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] count=%d", (int)count);
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]Received sock_connect_signal_t");
        pConnect_Sig = (sock_connect_signal_t *) buffer;

        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] size=%d", pConnect_Sig->size);
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] channel=%d", pConnect_Sig->channel);
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] status=%d", pConnect_Sig->status);
        for(i =0; i< 6 ; i++)
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] bdadd[%d]= %02X\n",i, pConnect_Sig->bd_addr.address[i]);

        spp_max_frame_size  = SPP_DATA_MTU;

        if(spp_client_create_socket() == -1)
        {
            ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! spp_client_create_socket FAILED!!\n");
        }
        else if (spp_start_socket_threads() == -1)
        {
            ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! spp_start_socket_threads FAILED!!\n");
        }
        else
        {
            ALOGD(LOGTAG_SPP_CLIENT "spp_client socket, threads creation is SUCCESS!!\n");
        }
        ALOGD(LOGTAG_SPP_CLIENT "\n [AKK_DEBUB] Moving to connected state.\n");

        snprintf(srv_bdAddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", pConnect_Sig->bd_addr.address[0],
                            pConnect_Sig->bd_addr.address[1], pConnect_Sig->bd_addr.address[2],
                            pConnect_Sig->bd_addr.address[3], pConnect_Sig->bd_addr.address[4],
                            pConnect_Sig->bd_addr.address[5]);
                            connctFlg=true;
        change_state(STATE_SPP_CLIENT_CONNECTED);
        pthread_mutex_lock(&client_handle_disconnect_mutex);
        pthread_cond_signal(&start_client_handle_disconnect_cv);
        pthread_mutex_unlock(&client_handle_disconnect_mutex);
    }

    ALOGD(LOGTAG_SPP_CLIENT "\n [AKK_DEBUB] Out of while loop \n");

}

void Spp_Client::connect(bt_bdaddr_t baddr)
{

    bt_bdaddr_t bd_addr = baddr;

    btsock_interface = (btsock_interface_t_v1 *)bluetooth_interface->get_profile_interface(BT_PROFILE_SOCKETS_ID);

    change_state(STATE_SPP_CLIENT_CONNECTING);

    // Invoke connect with socket type as RFCOMM, SPP UUID
    int status = btsock_interface->connect((bt_bdaddr_t_v1*)&bd_addr,BTSOCK_RFCOMM, SPP_UUID,SPP_CLIENT_CHANNEL,&listen_data_socfd,0,SPP_CLIENT_APP_UID);

    if(status == BT_STATUS_SUCCESS)
    {
        process_connect_message();
    }
    else
    {
        ALOGE(LOGTAG_SPP_CLIENT "Error -btsock_interface->connect, returned %d", status);
    }

}

static void *spp_client_handle_disconnect_thread_func(void *in_param)
{

    pSppClient->process_disconnect_message();
    return NULL;

}

void Spp_Client::process_disconnect_message()
{

    ALOGD(LOGTAG_SPP_CLIENT "process_disconnect_message");

    char buffer[1024];
    char ctrl_msgbuf[CMSG_SPACE(1)];
    struct cmsghdr *pcmsg;
    int* p_acc_fd = NULL;
    struct sockaddr_storage src_addr;
    sock_disconnect_signal_t *pDisconnect_Sig = NULL;
    int i =0;

    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control= ctrl_msgbuf;
    message.msg_controllen= sizeof(ctrl_msgbuf);
    while ( mClientState != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for SPP connected */
        while ( mClientState != STATE_SPP_CLIENT_CONNECTED )
            pthread_cond_wait(&start_client_handle_disconnect_cv,&client_handle_disconnect_mutex);
        pthread_mutex_unlock(&client_handle_disconnect_mutex);

        while ( mClientState == STATE_SPP_CLIENT_CONNECTED )
        {
            // Wait for sock_disconnect_signal_t message
            int count = recvmsg(listen_data_socfd,&message,0);

            if (count==-1)
            {
                ALOGD(LOGTAG_SPP_CLIENT "recvmsg returned -1");
            }
            else if (message.msg_flags&MSG_TRUNC)
            {
                ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]MSG_TRUNC\n");
            }
            else if ( 0 == count )
            {
            ALOGE(LOGTAG_SPP_CLIENT "ERROR listen sockfd closed");
            RESET_CLI_SOCFD(listen_data_socfd);
            change_state(STATE_SPP_CLIENT_IDLE);
            return;
            }

            if(count == 4)
            {
                for(i =0 ; i < count ; i++)
                ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] Received SCN msg[%d]= %02X\n",i, buffer[i]);

                memset(buffer,0,sizeof(buffer));
                continue;
            }

            // Handle the sock_disconnect_signal - server channel and status is received.

            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] count=%d", (int)count);
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]Received sock_disconnect_signal_t");
            pDisconnect_Sig = (sock_disconnect_signal_t *) buffer;

            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] size=%d", pDisconnect_Sig->size);
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] channel=%d", pDisconnect_Sig->channel);
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] status=%d", pDisconnect_Sig->status);
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] apsync=%d", pDisconnect_Sig->apsync);

            if(!pDisconnect_Sig->apsync)
                change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }

        ALOGD(LOGTAG_SPP_CLIENT "\n [AKK_DEBUB] Out of while loop \n");
    }
    pthread_mutex_destroy(&client_handle_disconnect_mutex);
    pthread_cond_destroy(&start_client_handle_disconnect_cv);
}

void Spp_Client::HandleEnableClient(void) {

    ALOGD(LOGTAG_SPP_CLIENT "HandleEnableClient ");
    BtEvent *pEvent = new BtEvent;
    pEvent->profile_start_event.status = true;
    change_state(STATE_SPP_CLIENT_IDLE);
    pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
    pEvent->profile_start_event.profile_id = PROFILE_ID_SPP_CLIENT;
    PostMessage(THREAD_ID_GAP, pEvent);
}


void Spp_Client::HandleDisableClient(void) {

    ALOGD(LOGTAG_SPP_CLIENT "HandleDisableClient ");
    change_state(STATE_SPP_CLIENT_INACTIVE);
    //trigger to exit threads
    pthread_mutex_lock(&client_send_mutex);
    pthread_cond_signal(&start_client_send_cv);
    pthread_mutex_unlock(&client_send_mutex);

    pthread_mutex_lock(&client_recv_mutex);
    pthread_cond_signal(&start_client_recv_cv);
    pthread_mutex_unlock(&client_recv_mutex);

    pthread_mutex_lock(&client_send_data_mutex);
    pthread_cond_signal(&start_client_send_data_cv);
    pthread_mutex_unlock(&client_send_data_mutex);

    pthread_mutex_lock(&client_recv_data_mutex);
    pthread_cond_signal(&start_client_recv_data_cv);
    pthread_mutex_unlock(&client_recv_data_mutex);

    BtEvent *pEvent = new BtEvent;
    pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
    pEvent->profile_stop_event.profile_id = PROFILE_ID_SPP_CLIENT;
    pEvent->profile_stop_event.status = true;
    PostMessage(THREAD_ID_GAP, pEvent);
}

void Spp_Client::ProcessEvent(BtEvent* pEvent) {

    switch(getState()) {

        case STATE_SPP_CLIENT_INACTIVE:
            state_inactive_handler(pEvent);
            break;
        case STATE_SPP_CLIENT_IDLE:
            state_active_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_CONNECTING:
            {
                fprintf(stdout, "Event not processed in in-connecting state %d ", pEvent->event_id);
                ALOGE(LOGTAG_SPP_CLIENT " event not handled %d (%s) ", pEvent->event_id,dump_message(pEvent->event_id) );
            }
            break;

        case STATE_SPP_CLIENT_CONNECTED:
            state_connected_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_SEND_FILE:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_RECEIVE_FILE:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_SEND_DATA:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_RECEIVE_DATA:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_CLIENT_DISCONNECTED:
            state_disconnected_handler(pEvent);
            break;
    }
}

char* Spp_Client::dump_message(BluetoothEventId event_id) {

    switch(event_id) {

        case SPP_CLI_CONNECT:
            return (char*)"SPP_CLI_CONNECT";

        case SPP_CLI_SEND_FILE:
            return (char*)"SPP_CLI_SEND_FILE";

        case SPP_CLI_RECV_FILE:
            return (char*)"SPP_CLI_RECV_FILE";

        case SPP_CLI_SEND_DATA:
            return (char*)"SPP_CLI_SEND_DATA";

        case SPP_CLI_RECV_DATA:
            return (char*)"SPP_CLI_RECV_DATA";

        case SPP_CLI_DISCONNECT:
            return (char*)"SPP_CLI_DISCONNECT";

        case SPP_CLI_START_THREADS:
            return (char*)"SPP_CLI_START_THREADS";

    }
    return (char*)"UNKNOWN";
}

void Spp_Client::state_inactive_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_CLIENT "state_inactive_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {


        default:
            fprintf(stdout, "Event not processed in in-active state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_CLIENT " event not handled %d (%s) ", pEvent->event_id,dump_message(pEvent->event_id) );
            break;
    }
}

void Spp_Client::state_active_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_CLIENT "state_active_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        case SPP_CLI_START_THREADS:
            {
                //donothing
            }
            break;

        case SPP_CLI_CONNECT:
            {
                //start the threads
                if (!threads_started)
                    start_send_recv_threads();
                connect(pEvent->spp_cli_event.bd_addr);
            }
            break;

        default:
            fprintf(stdout, "Event not processed in active state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_CLIENT " event not handled %d ", pEvent->event_id);
            break;
    }
}


int Spp_Client::receive_file(const char* fname, int &soc_fd)
{
    int count  = 0;
    int status = SUCCESS;
    int i      = 0;
    char buffer[1024];
    std::chrono::high_resolution_clock::time_point startTime,endTime;

    char ctrl_msgbuf[CMSG_SPACE(1)];
    struct cmsghdr *pcmsg;
    struct sockaddr_storage src_addr;
    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control= ctrl_msgbuf;
    message.msg_controllen= sizeof(ctrl_msgbuf);

    ALOGD(LOGTAG_SPP_CLIENT "--> receive_file, fname=%s, soc_fd=%d\n", fname, soc_fd);


    ofstream recv_file(fname,ios::out | ios::binary);

    if( recv_file.is_open())
    {
        while ((count = recvmsg(soc_fd,&message,0)) > 0 )
        {
            if(strstr(buffer, "SPP_START_SENDING_FILE"))
                startTime = std::chrono::high_resolution_clock::now();
            recv_file.write(buffer,count);
            recv_file.flush();
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]RECVD DATA, count=%d", (int)count);
            //for(i =0; i< count ; i++)
            //{
            //    ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] buffer[%d]=%c",i, buffer[i]);
            //}
            if(strstr(buffer, "SPP_END_SENDING_FILE"))
            {
                endTime = std::chrono::high_resolution_clock::now();
                const auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
                ALOGD(LOGTAG_SPP_CLIENT "::Receive file Time Delta::\n file received successfully in %lld ms \n ",int_ms);
                recv_file.flush();
                recv_file.close();

                fprintf(stdout,"File Receive Complete\n");
                fprintf(stdout,"---------------------\n");
                fprintf(stdout,"Device Address  : %s\n",srv_bdAddr);
                //fprintf(stdout,"soc_fd          : %d\n",soc_fd);
                fprintf(stdout,"File Name       : %s\n",fname);

                return status;
            }

        }
        if(errno)
            ALOGD(LOGTAG_SPP_CLIENT " receive_file errno err=%s\n",strerror(errno));
        if( count == 0 )
        {
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] connection closed by the remote");
            RESET_CLI_SOCFD(soc_fd);
            spp_client_write_thread_close();
            change_state(STATE_SPP_CLIENT_DISCONNECTED);
        }
        else if(count < 0 || errno == ECONNRESET)
        {
            ALOGD(LOGTAG_SPP_CLIENT " recvmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
            ALOGE(LOGTAG_SPP_CLIENT " Aborting receive_file, Invalid data socfd, connection may be lost\n");
            fprintf(stderr, " Aborting receive_file, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
            RESET_CLI_SOCFD(soc_fd);
            status = FAILED;
        }
    }
    else
    {
        ALOGE(LOGTAG_SPP_CLIENT "Error opening file=%s\n", fname);
        status = FAILED;
    }
    return status;

}

int Spp_Client::snd_file(const char* fname, int &soc_fd)
{
    int count  = 0;
    int status = SUCCESS;
    char buffer[1024];
    int max_read_size=500;

    char startFile[]="SPP_START_SENDING_FILE";
    char endFile[]="SPP_END_SENDING_FILE";
    std::chrono::high_resolution_clock::time_point startTime,endTime;

    ALOGD(LOGTAG_SPP_CLIENT "--> snd_file, fname=%s, soc_fd=%d\n", fname, soc_fd);

    ifstream snd_file(fname,ios::in | ios::binary);

    if( snd_file.is_open())
    {
        count = send(soc_fd,&startFile,strlen(startFile),MSG_NOSIGNAL);
        if(count < 0);
        else
        {
            startTime = std::chrono::high_resolution_clock::now();
            while (!snd_file.eof())
            {
                snd_file.read(buffer,max_read_size);
                count = send(soc_fd,&buffer,snd_file.gcount(),MSG_NOSIGNAL);
                if(count < 0)
                    break;
            }
            count = send(soc_fd,&endFile,strlen(endFile),MSG_NOSIGNAL);
        }
        if(errno)
            ALOGD(LOGTAG_SPP_CLIENT " snd_file errno err=%s\n",strerror(errno));
        if(count < 0 || errno == ECONNRESET)
        {
            ALOGD(LOGTAG_SPP_CLIENT " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
            ALOGE(LOGTAG_SPP_CLIENT " Aborting send_file, Invalid data socfd, connection may be lost\n");
            fprintf(stderr, " Aborting send_file, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
            RESET_CLI_SOCFD(soc_fd);
            status = FAILED;
        }
        else{
            endTime = std::chrono::high_resolution_clock::now();
            const auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            //fprintf(stdout, "::Send file Time Delta::\n file sent successfully in %lld ms \n ",int_ms);
            ALOGD(LOGTAG_SPP_CLIENT "<-- snd_file, fname=%s, soc_fd=%d\n", fname, soc_fd);
            fprintf(stdout,"File Transfer Complete\n");
            fprintf(stdout,"---------------------\n");
            fprintf(stdout,"Device Address  : %s\n",srv_bdAddr);
            //fprintf(stdout,"soc_fd          : %d\n",soc_fd);
            fprintf(stdout,"File Name       : %s\n",fname);
            snd_file.close();
        }
    }
    else
    {
        ALOGE(LOGTAG_SPP_CLIENT "Error opening file=%s\n", fname);
        status = FAILED;
    }

    return status;
}

int Spp_Client::send_data(const char* Size, int &soc_fd)
{
    int count  = 1;
    int status = SUCCESS;
    char *buffer = (char*)malloc(1025);
    int max_read_size=500;
    int sentBytes=0;
    int dataLen=stoi(Size);
    int totalBytes=0;
    std::chrono::high_resolution_clock::time_point startTime,endTime;

    ALOGD(LOGTAG_SPP_CLIENT "--> snd_data, dataSize=%s, soc_fd=%d\n", Size, soc_fd);
    for(int i = 0; i < 1024; i++) {
        buffer[i]='a';
    }

    if(soc_fd < 0){
        ALOGE(LOGTAG_SPP_CLIENT "--> snd_data,  Invalid data socfd %d, connection may be lost \n", soc_fd);
        return -1;
    }
    else
    {
        startTime = std::chrono::high_resolution_clock::now();
        sentBytes = send(soc_fd,"Start",5,MSG_NOSIGNAL);
        if(sentBytes < 0);
        else{
            while(count <= dataLen){
                snprintf(&buffer[0],1025,"$%08d%s",count,&buffer[9]);
                sentBytes = send(soc_fd,buffer,1024,MSG_NOSIGNAL);
                if(sentBytes < 0)
                    break;

                totalBytes += sentBytes;
                count++;
            }
            sentBytes = send(soc_fd,"end",3,MSG_NOSIGNAL);

        }
        if(errno)
            ALOGD(LOGTAG_SPP_CLIENT " snd_data errno err=%s",strerror(errno));
        if(sentBytes < 0 || errno == ECONNRESET )
        {
            ALOGD(LOGTAG_SPP_CLIENT " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
            ALOGE(LOGTAG_SPP_CLIENT " Aborting send_data, Invalid data socfd, connection may be lost\n");
            fprintf(stderr, " Aborting send_data, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
            RESET_CLI_SOCFD(soc_fd);
            status = FAILED;
        }
        else
        {
            endTime = std::chrono::high_resolution_clock::now();
            //long long duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            const auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            ALOGD(LOGTAG_SPP_CLIENT "--> snt_data, num of bytes sent %d, soc_fd=%d  buffer = %s \n ", totalBytes, soc_fd, buffer);
            //fprintf(stdout, "::Tx Time Delta::\ndata size %d sent successfully in %lld ms \n ", totalBytes/1024,int_ms);
            float TxTput = ((float) totalBytes * 8 * 1000)/int_ms.count();
            float TxTputk = TxTput / 1000;
            ALOGD(LOGTAG_SPP_CLIENT" write: Through put (send) is approximately(in kbps): %f\n",TxTputk);
            fprintf(stdout,"Tx Results\n");
            fprintf(stdout,"----------\n");
            //fprintf(stdout,"soc_fd               : %d\n",soc_fd);
            fprintf(stdout,"Device Address       : %s\n",srv_bdAddr);
            fprintf(stdout,"Connection Direction : Client\n");
            fprintf(stdout,"Throughput (in kbps) : %f\n",TxTputk);
            totalBytes=0;
        }

    }
    free(buffer);
    return  status;
}


int Spp_Client::receive_data(int &soc_fd)
{
    int count  = 0;
    int status = SUCCESS;
    int i      = 0;
    char buffer[1024];
    int totalBytes = 0;

    std::chrono::high_resolution_clock::time_point startTime,endTime;

    char ctrl_msgbuf[CMSG_SPACE(1)];
    struct cmsghdr *pcmsg;
    struct sockaddr_storage src_addr;
    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control= ctrl_msgbuf;
    message.msg_controllen= sizeof(ctrl_msgbuf);

    ALOGD(LOGTAG_SPP_CLIENT "--> receive_data, soc_fd=%d\n", soc_fd);

    while ((count = recvmsg(soc_fd,&message,0)) > 0 )
    {

        totalBytes += count;
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]num of bytes received :: %d Incoming msg received in ClientSocket :: %s\n",count, buffer);
        //for(i =0; i< count ; i++)
        //{
        //     ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] buffer[%d]=%c",i, buffer[i]);
        // }

        if(strstr(buffer, "Start"))
            startTime = std::chrono::high_resolution_clock::now();
        else if(strstr(buffer, "end"))
        {
            totalBytes -= 8;//Start - end
            endTime = std::chrono::high_resolution_clock::now();
            const auto int_ms = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            ALOGD(LOGTAG_SPP_CLIENT "--> received_data, num of bytes Received %d, soc_fd=%d error(%s)\n ", totalBytes, soc_fd,strerror(errno));

            float RxTput = ((float) totalBytes * 8 * 1000)/int_ms.count();
            float RxTputk = RxTput / 1000;
            ALOGD(LOGTAG_SPP_CLIENT" read: Through put (receive) is approximately(in kbps): %f\n",RxTputk);
            fprintf(stdout,"Rx Results\n");
            fprintf(stdout,"----------\n");
            //fprintf(stdout,"soc_fd               : %d\n",soc_fd);
            fprintf(stdout,"Device Address       : %s\n",srv_bdAddr);
            fprintf(stdout,"Connection Direction : Client\n");
            fprintf(stdout,"Throughput (in kbps) : %f\n",RxTputk);
            totalBytes=0;
            break;
        }
        memset(buffer, 0x00, sizeof(buffer));
    }
    if(errno)
        ALOGD(LOGTAG_SPP_CLIENT " receive_data errno err=%s",strerror(errno));
    if( count == 0 )
    {
        ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] connection closed by the remote");
        RESET_CLI_SOCFD(soc_fd);
        spp_client_write_thread_close();
        status = FAILED;
        change_state(STATE_SPP_CLIENT_DISCONNECTED);
    }
    else if(count < 0 || errno==ECONNRESET)
    {
        ALOGD(LOGTAG_SPP_CLIENT " recvmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
        ALOGE(LOGTAG_SPP_CLIENT " Aborting receive_data, Invalid data socfd, connection may be lost\n");
        fprintf(stderr, " Aborting receive_data, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
        RESET_CLI_SOCFD(soc_fd);
        status = FAILED;
    }

    return status;

}

void Spp_Client::state_connected_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_CLIENT "state_connected_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        case SPP_CLI_SEND_FILE:
            {
                file_name = pEvent->spp_cli_event.value;
                change_state(STATE_SPP_CLIENT_SEND_FILE);
                pthread_mutex_lock(&client_send_mutex);
                pthread_cond_signal(&start_client_send_cv);
                pthread_mutex_unlock(&client_send_mutex);
            }
            break;

        case SPP_CLI_RECV_FILE:
            {
                file_name = pEvent->spp_cli_event.value;
                change_state(STATE_SPP_CLIENT_RECEIVE_FILE);
                pthread_mutex_lock(&client_recv_mutex);
                pthread_cond_signal(&start_client_recv_cv);
                pthread_mutex_unlock(&client_recv_mutex);
            }
            break;

        case SPP_CLI_SEND_DATA:
            {
                dataSize = pEvent->spp_cli_event.value;
                change_state(STATE_SPP_CLIENT_SEND_DATA);
                pthread_mutex_lock(&client_send_data_mutex);
                pthread_cond_signal(&start_client_send_data_cv);
                pthread_mutex_unlock(&client_send_data_mutex);
            }
            break;

        case SPP_CLI_RECV_DATA:
            {
                dataSize = pEvent->spp_cli_event.value;
                change_state(STATE_SPP_CLIENT_RECEIVE_DATA);
                pthread_mutex_lock(&client_recv_data_mutex);
                pthread_cond_signal(&start_client_recv_data_cv);
                pthread_mutex_unlock(&client_recv_data_mutex);
            }
            break;

        case SPP_CLI_DISCONNECT:
            {
                spp_client_write_thread_close();
                shutdown(listen_data_socfd, SHUT_RDWR);
                close(listen_data_socfd);
                RESET_CLI_SOCFD(listen_data_socfd);
                change_state(STATE_SPP_CLIENT_DISCONNECTED);
            }
            break;

        default:
            ALOGE(LOGTAG_SPP_CLIENT " event not handled %d ", pEvent->event_id);
            break;
    }
}

void Spp_Client::state_send_receive_handler(BtEvent* pEvent) {

    if(getState() == STATE_SPP_CLIENT_SEND_FILE)
    {
        ALOGD(LOGTAG_SPP_CLIENT " SPP-CLI 'SEND' state, Processing event %s", dump_message(pEvent->event_id));
    }
    else
    {
        ALOGD(LOGTAG_SPP_CLIENT " SPP-CLI 'RECEIVE' state Processing event %s", dump_message(pEvent->event_id));
    }

    switch(pEvent->event_id) {

        case SPP_CLI_DISCONNECT:
            {
                spp_client_write_thread_close();
                shutdown(listen_data_socfd, SHUT_RDWR);
                close(listen_data_socfd);
                RESET_CLI_SOCFD(listen_data_socfd);
                change_state(STATE_SPP_CLIENT_DISCONNECTED);
            }
            break;

        default:
        {
            if(getState() == STATE_SPP_CLIENT_SEND_FILE)
            {
                fprintf(stdout, "Event not processed in 'SEND' state %d ", pEvent->event_id);
            }
            else
            {
                fprintf(stdout, "Event not processed in 'RECEIVE' state %d ", pEvent->event_id);
            }

            ALOGE(LOGTAG_SPP_CLIENT " event not handled %d ", pEvent->event_id);
        }
        break;
    }
}

void Spp_Client::state_disconnected_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_CLIENT "state_disconnected_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        case SPP_CLI_CONNECT:
            {
                connect(pEvent->spp_cli_event.bd_addr);
            }
            break;

        case SPP_CLI_DISCONNECT:
            {
                state_connected_handler(pEvent);
            }
            break;

        default:
            fprintf(stdout, "Event not processed in disconnected state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_CLIENT " event not handled %d ", pEvent->event_id);
            break;
    }
}


void Spp_Client::start_send_recv_threads()
{
    ALOGD(LOGTAG_SPP_CLIENT "--> start_send_recv_threads");

    pthread_mutex_init(&client_recv_mutex, NULL);
    pthread_cond_init(&start_client_recv_cv, NULL);
    if (pthread_create(&client_recv_thread, NULL, spp_client_recv_thread_func, NULL) != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create spp client receive thread!\n");
        return;
    }

    pthread_mutex_init(&client_send_mutex, NULL);
    pthread_cond_init(&start_client_send_cv, NULL);
    if (pthread_create(&client_send_thread, NULL, spp_client_send_thread_func, NULL) != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create spp client send thread!\n");
        return;
    }

    pthread_mutex_init(&client_send_data_mutex, NULL);
    pthread_cond_init(&start_client_send_data_cv, NULL);
    if (pthread_create(&client_send_data_thread, NULL, spp_client_send_data_thread_func, NULL) != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create spp client send data thread!\n");
        return;
    }

    pthread_mutex_init(&client_recv_data_mutex, NULL);
    pthread_cond_init(&start_client_recv_data_cv, NULL);
    if (pthread_create(&client_recv_data_thread, NULL, spp_client_recv_data_thread_func, NULL) != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create spp client receive data thread!\n");
        return;
    }

    pthread_mutex_init(&client_handle_disconnect_mutex, NULL);
    pthread_cond_init(&start_client_handle_disconnect_cv, NULL);
    if (pthread_create(&client_handle_disconnect_thread, NULL, spp_client_handle_disconnect_thread_func, NULL) != 0) {
        ALOGD(LOGTAG_SPP_CLIENT "!! ERROR !! Cannot create spp client handle disconnect thread!\n");
        return;
    }
    threads_started = true;
    ALOGD(LOGTAG_SPP_CLIENT "<-- start_send_recv_threads");

}

void Spp_Client::change_state(SppClientState mState) {
   ALOGD(LOGTAG_SPP_CLIENT " current State = %d, new state = %d", getState(), mState);
   pthread_mutex_lock(&lock);
   mClientState = mState;
   switch(mClientState)
   {
        case STATE_SPP_CLIENT_CONNECTED:
        if(connctFlg)
        {
            connctFlg=false;
            fprintf(stdout,"Device is Connected\n");
            fprintf(stdout,"----------------------\n");
            fprintf(stdout,"Device Address       : %s\n",srv_bdAddr);
            fprintf(stdout,"Connection Direction : Client\n");
        }
        break;
        case STATE_SPP_CLIENT_DISCONNECTED:
            fprintf(stdout,"Device is Disconnected\n");
        break;
        case STATE_SPP_CLIENT_CONNECTING:
            fprintf(stdout,"Connecting Device...Please wait...!!!\n");
        break;
        default:
        break;

   }
   pthread_mutex_unlock(&lock);
   ALOGD(LOGTAG_SPP_CLIENT " state changed to %d ", mState);
}

SppClientState Spp_Client::getState(void) {
   SppClientState state = STATE_SPP_CLIENT_INACTIVE;
   pthread_mutex_lock(&lock);
   state = mClientState;
   pthread_mutex_unlock(&lock);
   return state;
}

Spp_Client :: Spp_Client(const bt_interface_t *bt_interface, config_t *config) {
    ALOGD(LOGTAG_SPP_CLIENT " Spp_Client constructor");
    this->bluetooth_interface = bt_interface;
    this->config              = config;
    RESET_CLI_SOCFD(listen_data_socfd);
    pthread_mutex_init(&this->lock, NULL);
    change_state(STATE_SPP_CLIENT_INACTIVE);
}

Spp_Client :: ~Spp_Client() {
    ALOGD(LOGTAG_SPP_CLIENT " Spp_Client destructor");
    change_state(STATE_SPP_CLIENT_INACTIVE);
    pthread_mutex_destroy(&lock);
}

