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


static void *spp_client_recv_thread_func(void *in_param)
{

    pSppClient->sppcli_recv_thread_handler();
    return NULL;

}


void Spp_Client::sppcli_send_thread_handler()
{

    while ( mClientState != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        while ( mClientState != STATE_SPP_CLIENT_SEND_FILE )
            pthread_cond_wait(&start_client_send_cv,&client_send_mutex);
        pthread_mutex_unlock(&client_send_mutex);

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

void Spp_Client::sppcli_recv_thread_handler()
{

    while ( mClientState != STATE_SPP_CLIENT_INACTIVE )
    {
        /* Wait for receive command from the user */
        while ( mClientState != STATE_SPP_CLIENT_RECEIVE_FILE )
            pthread_cond_wait(&start_client_recv_cv,&client_recv_mutex);
        pthread_mutex_unlock(&client_recv_mutex);

        /* Receive File */
        receive_file(file_name.c_str(),listen_data_socfd);

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

    while ( mClientState == STATE_SPP_CLIENT_CONNECTING )
    {

        // Wait for sock_connect_signal_t message
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
            ALOGD(LOGTAG_SPP_CLIENT "spp_server socket, threads creation is SUCCESS!!\n");
        }
        ALOGD(LOGTAG_SPP_CLIENT "\n [AKK_DEBUB] Moving to connected state.\n");
        change_state(STATE_SPP_CLIENT_CONNECTED);
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

void Spp_Client::HandleEnableClient(void) {
    
    ALOGD(LOGTAG_SPP_CLIENT "HandleEnableClient ");
    BtEvent *pEvent = new BtEvent;
    pEvent->profile_start_event.status = true;
    change_state(STATE_SPP_CLIENT_IDLE);
    pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
    pEvent->profile_start_event.profile_id = PROFILE_ID_SPP_CLIENT;
    PostMessage(THREAD_ID_GAP, pEvent);

    BtEvent *pStartThrdsEvent = new BtEvent;
    pStartThrdsEvent->spp_cli_event.event_id = SPP_CLI_START_THREADS;
    PostMessage (THREAD_ID_SPP_CLIENT, pStartThrdsEvent);
}


void Spp_Client::HandleDisableClient(void) {

   change_state(STATE_SPP_CLIENT_INACTIVE);

   BtEvent *pEvent = new BtEvent;
   pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
   pEvent->profile_stop_event.profile_id = PROFILE_ID_SPP_CLIENT;
   pEvent->profile_stop_event.status = true;
   PostMessage(THREAD_ID_GAP, pEvent);
}

void Spp_Client::ProcessEvent(BtEvent* pEvent) {

    switch(mClientState) {

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
                start_send_recv_threads();
            }
            break;

        case SPP_CLI_CONNECT:
            {
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
            recv_file.write(buffer,count);
            recv_file.flush();
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB]RECVD DATA, count=%d", (int)count);
            for(i =0; i< count ; i++)
            {
                ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] buffer[%d]=%c",i, buffer[i]);
            }
        }

        recv_file.flush();
        recv_file.close();

        if( count == 0 )
        {
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] connection closed by the remote");
            RESET_CLI_SOCFD(soc_fd);
            spp_client_write_thread_close();
        }
        else
        {
            ALOGD(LOGTAG_SPP_CLIENT "[AKK_DEBUB] Receive Error count=%d", count);
            status = FAILED;
        }
    }
    else
    {
        ALOGE(LOGTAG_SPP_CLIENT "Error opening file=%s\n", fname);
        status = FAILED;
    }

    ALOGD(LOGTAG_SPP_CLIENT "<-- receive_file, fname=%s, soc_fd=%d\n", fname, soc_fd);

    return status;

}

int Spp_Client::snd_file(const char* fname, int &soc_fd)
{
    int count  = 0;
    int status = SUCCESS;
    char buffer[1024];
    int max_read_size=500;

    ALOGD(LOGTAG_SPP_CLIENT "--> snd_file, fname=%s, soc_fd=%d\n", fname, soc_fd);


    ifstream snd_file(fname,ios::in | ios::binary);

    if( snd_file.is_open())
    {
        while (!snd_file.eof())
        {
            snd_file.read(buffer,max_read_size);

            count = send(soc_fd,&buffer,snd_file.gcount(),MSG_NOSIGNAL);

            if(count < 0)
            {
                ALOGD(LOGTAG_SPP_CLIENT " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
                ALOGE(LOGTAG_SPP_CLIENT " Aborting send_file, Invalid data socfd, connection may be lost\n");
                fprintf(stderr, " Aborting send_file, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
                RESET_CLI_SOCFD(soc_fd);
                status = FAILED;
                break;
            }
            else
            {
                ALOGD(LOGTAG_SPP_CLIENT " sent bytes (%d)",count);
            }
        }

        snd_file.close();
    }
    else
    {
        ALOGE(LOGTAG_SPP_CLIENT "Error opening file=%s\n", fname);
        status = FAILED;
    }

    ALOGD(LOGTAG_SPP_CLIENT "<-- snd_file, fname=%s, soc_fd=%d\n", fname, soc_fd);

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

    if(mClientState == STATE_SPP_CLIENT_SEND_FILE)
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
            if(mClientState == STATE_SPP_CLIENT_SEND_FILE)
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

    ALOGD(LOGTAG_SPP_CLIENT "<-- start_send_recv_threads");

}

void Spp_Client::change_state(SppClientState mState) {
   ALOGD(LOGTAG_SPP_CLIENT " current State = %d, new state = %d", mClientState, mState);
   pthread_mutex_lock(&lock);
   mClientState = mState;
   pthread_mutex_unlock(&lock);
   ALOGD(LOGTAG_SPP_CLIENT " state changed to %d ", mState);
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
    change_state(STATE_SPP_CLIENT_INACTIVE);
    pthread_mutex_destroy(&lock);
}

