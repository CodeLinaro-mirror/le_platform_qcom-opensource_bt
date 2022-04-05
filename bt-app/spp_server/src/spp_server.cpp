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
#include "spp_server.hpp"
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

#define LOGTAG_SPP_SERVER "SPP_SERVER "

#define WAIT_TIME_FOR_RFCOMM_INIT (1000) //

#define SPP_SERVER_APP_UID (1)

#define VALID_SRV_SOCFD(FD) (-1 == FD?0:1)
#define RESET_SRV_SOCFD(FD) (FD=-1)

#define NETLINK_SPP         31
#define SPP_CTL_ON         1
#define SPP_CTL_OFF        2
#define SPP_CTL_DATA_LEN   9
#define SPP_CTL_OP_POS     0
#define SPP_CTL_INS_POS    1
#define SPP_CTL_MTU_POS    5
#define SPP_DATA_MTU    500

using namespace std;
using std::list;
using std::string;

Spp_Server *pSppServer = NULL;
static pthread_t server_thread = NULL;
typedef struct
{
    struct nlmsghdr n;
    uint8_t data[0];
} data_st;

/* SPP server receive thread */
static pthread_t server_recv_thread = NULL;
static pthread_mutex_t recv_mutex;
static pthread_cond_t start_recv_cv;

/* SPP server send thread */
static pthread_t server_send_thread = NULL;
static pthread_mutex_t send_mutex;
static pthread_cond_t start_send_cv;

#ifdef __cplusplus
extern "C" {
#endif

void BtSppServerMsgHandler(void *msg) {

    BtEvent* pEvent = NULL;

    if(!msg) {
        ALOGE("Msg is NULL, bail out!!");
        return;
    }

    pEvent = ( BtEvent *) msg;

    switch(pEvent->event_id) {

        case PROFILE_API_START:
            {
                ALOGD(LOGTAG_SPP_SERVER "Enable spp server");
                BtEvent *pEvent = new BtEvent;
                pEvent->profile_start_event.status = true;
                pEvent->profile_start_event.event_id = PROFILE_EVENT_START_DONE;
                pEvent->profile_start_event.profile_id = PROFILE_ID_SPP_SERVER;
                PostMessage(THREAD_ID_GAP, pEvent);
            }
            break;

        case PROFILE_API_STOP:

            ALOGD(LOGTAG_SPP_SERVER "Disable spp server");
            if (pSppServer) {
                pSppServer->HandleDisableServer();
            }
            break;

        default:

            ALOGD(LOGTAG_SPP_SERVER "default event_id");
            if(pSppServer) {
               pSppServer->ProcessEvent(( BtEvent *) msg);
            }
            break;
    }

    delete pEvent;
}

#ifdef __cplusplus
}
#endif

static void *spp_server_send_thread_func(void *in_param)
{

    pSppServer->sppsrv_send_thread_handler();
    return NULL;

}

void Spp_Server::sppsrv_send_thread_handler()
{

    while ( mServerState != STATE_SPP_SERVER_INACTIVE )
    {
        /* Wait for receive command from the user */
        while ( mServerState != STATE_SPP_SERVER_SEND_FILE )
            pthread_cond_wait(&start_send_cv,&send_mutex);
        pthread_mutex_unlock(&send_mutex);

        /* Send File */
        if(VALID_SRV_SOCFD(data_socfd))
        {
            int status = snd_file(file_name.c_str(),data_socfd);

            if(status != SUCCESS)
            {
                ALOGD(LOGTAG_SPP_SERVER "Send file failed \n");
            }

        }
        else
        {
            ALOGD(LOGTAG_SPP_SERVER "In-valid socfd, Connection may be lost");
        }

        if( !VALID_SRV_SOCFD(data_socfd) )
        {
            change_state(STATE_SPP_SERVER_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_SERVER_CONNECTED);
        }
    }

    pthread_mutex_destroy(&send_mutex);
    pthread_cond_destroy(&start_send_cv);

}


static void *spp_server_recv_thread_func(void *in_param)
{

    pSppServer->sppsrv_recv_thread_handler();
    return NULL;

}

void Spp_Server::sppsrv_recv_thread_handler()
{

    while ( mServerState != STATE_SPP_SERVER_INACTIVE )
    {
        /* Wait for receive command from the user */
        while ( mServerState != STATE_SPP_SERVER_RECEIVE_FILE )
            pthread_cond_wait(&start_recv_cv,&recv_mutex);
        pthread_mutex_unlock(&recv_mutex);

        /* Receive File */
        receive_file(file_name.c_str(),data_socfd);

        if( !VALID_SRV_SOCFD(data_socfd) )
        {
            change_state(STATE_SPP_SERVER_DISCONNECTED);
        }
        else
        {
            change_state(STATE_SPP_SERVER_CONNECTED);
        }
    }

    pthread_mutex_destroy(&recv_mutex);
    pthread_cond_destroy(&start_recv_cv);

}


static void *spp_server_thread_func(void *in_param)
{

    int accept_sockfd = *((int*) in_param);
    pSppServer->spp_server_thread_handler(accept_sockfd);
    return NULL;

}

static void *spp_server_write_thread_func(void *arg)
{
    pSppServer->spp_server_write_thread_handler();
    return NULL;
}

static void *spp_server_read_thread_func(void *arg)
{
    pSppServer->spp_server_read_thread_handler();
    return NULL;
}

void Spp_Server::spp_server_write_thread_close()
{
    ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_close");
    pthread_mutex_lock(&server_write_thread_mutex);
    server_write_thread_stop_thread = true;
    pthread_mutex_unlock(&server_write_thread_mutex);
}

void Spp_Server::spp_server_read_thread_close()
{
    ALOGD(LOGTAG_SPP_SERVER "spp_server_read_thread_close");
    pthread_mutex_lock(&server_read_thread_mutex);
    server_read_thread_stop_thread = true;
    pthread_mutex_unlock(&server_read_thread_mutex);
}

int Spp_Server::spp_server_set_tty_driver_state(uint8_t ttyState)
{
    int instanceId = 1;//Only one Instance is used!!
    int maxFrame = spp_max_frame_size;
    int retVal = -1;
    struct
    {
        struct nlmsghdr n;
        uint8_t data[SPP_CTL_DATA_LEN];
    } ctl;

    ALOGD(LOGTAG_SPP_SERVER "spp_server_set_tty_driver_state");

    memset(&ctl, 0x00, sizeof(ctl));

    ctl.data[SPP_CTL_OP_POS] = ttyState;
    memcpy(ctl.data + SPP_CTL_INS_POS, &instanceId, sizeof(instanceId));

    if (ttyState == SPP_CTL_ON)
    {
        memcpy(ctl.data + SPP_CTL_MTU_POS, &maxFrame, sizeof(maxFrame));
    }

    ctl.n.nlmsg_len = NLMSG_LENGTH(SPP_CTL_DATA_LEN);
    ctl.n.nlmsg_pid = server_read_thread;  /* self pid */
    ctl.n.nlmsg_type  = NETLINK_SPP;
    ctl.n.nlmsg_flags = NL_MSG_CTL;

    ALOGD(LOGTAG_SPP_SERVER "instanceId is %d, maxFrame is %d, nlmsg_pid is %lu, serverSocket is %d, ctl.n.nlmsg_len is %d",
    instanceId, maxFrame, server_read_thread, kernel_sock_fd,  ctl.n.nlmsg_len);

    retVal = send(kernel_sock_fd, &ctl, ctl.n.nlmsg_len, 0);
    return retVal;
}

void Spp_Server::spp_server_write_thread_handler()
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

    ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_handler");
    while (1)
    {
        pthread_mutex_lock(&server_write_thread_mutex);

        if (server_write_thread_stop_thread)
        {
            if (tty_opened)
            {
                retVal = spp_server_set_tty_driver_state(SPP_CTL_OFF);

                if (retVal < 0)
                {
                    ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_handler sendmsg ctl message failed!!");
                    pthread_mutex_unlock(&server_write_thread_mutex);
                    continue;
                }
                tty_opened = false;
            }
            pthread_mutex_unlock(&server_write_thread_mutex);
            pthread_mutex_destroy(&server_write_thread_mutex);

           ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_handler Exiting Write Thread");

            if (read_thread_exit == 1)
            {
                ALOGD(LOGTAG_SPP_SERVER "write thread close the socket");
                shutdown(kernel_sock_fd, SHUT_RDWR);
                close(kernel_sock_fd);
                kernel_sock_fd = 0;
            }

            write_thread_exit = 1;
            pthread_exit(NULL);
        }

        if (!tty_opened)
        {
            retVal = spp_server_set_tty_driver_state(SPP_CTL_ON);
            if (retVal < 0)
            {
                ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_handler retVal is %d, Sendmsg ctl message failed,%s", retVal, strerror(errno));
                pthread_mutex_unlock(&server_write_thread_mutex);
                continue;
            }
            tty_opened = true;
            ALOGD(LOGTAG_SPP_SERVER "spp_server_write_thread_handler retVal is %d, Sendmsg ctl message successed", retVal);
        }

        pthread_mutex_unlock(&server_write_thread_mutex);

        ALOGD(LOGTAG_SPP_SERVER "%s data_socfd is %d", __func__, data_socfd);
        count = recvmsg(data_socfd, &message,0);

        ALOGD(LOGTAG_SPP_SERVER "RECVD DATA, count=%d", (int)count);
        for(int i =0; i< count ; i++)
        {
            ALOGD(LOGTAG_SPP_SERVER "buffer[%d]=%c %d",i, buffer[i], buffer[i]);
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
                    send_data->n.nlmsg_pid = server_read_thread;         /* self pid */
                    send_data->n.nlmsg_type  = NETLINK_SPP;
                    send_data->n.nlmsg_flags = NL_MSG_CTL + 1;       /*data flags*/

                    retVal = send(kernel_sock_fd, send_data,
                                  send_data->n.nlmsg_len, 0);

                    ALOGD(LOGTAG_SPP_SERVER "send ret is %d,currentSend is %d",  retVal, currentSend);
                    retVal = (int)(currentSend & 0x0000ffff);
                    osi_free(send_data);
                    send_data = NULL;
                }
                else
                {
                    ALOGD(LOGTAG_SPP_SERVER "tty_opened is false");
                }
                bytesSent += retVal;
            }
        }
        else if( count == 0 )
        {
            ALOGD(LOGTAG_SPP_SERVER " connection closed by the remote");
            RESET_SRV_SOCFD(data_socfd);
            server_write_thread_stop_thread = true;
        }
        else
        {
            ALOGD(LOGTAG_SPP_SERVER "Receive Error count=%d", count);
        }
    }
}

void Spp_Server::spp_server_read_thread_handler()
{
    uint8_t *buffer = NULL;
    int retVal = 0;
    pthread_attr_t server_write_thread_attr;

    ALOGD(LOGTAG_SPP_SERVER "spp_server_read_thread_handler");

    pthread_mutex_init(&server_write_thread_mutex, NULL);
    server_write_thread_stop_thread = false;

    pthread_attr_init(&server_write_thread_attr);
    pthread_attr_setdetachstate(&server_write_thread_attr, PTHREAD_CREATE_DETACHED);
    retVal = pthread_create(&server_write_thread, &server_write_thread_attr, spp_server_write_thread_func, NULL);
    pthread_attr_destroy(&server_write_thread_attr);
    if (retVal != 0)
   {
        shutdown(kernel_sock_fd, SHUT_RDWR);
        close(kernel_sock_fd);
        kernel_sock_fd = 0;
        ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! Cannot create server_write_thread!\n");

        pthread_mutex_destroy(&server_write_thread_mutex);
        pthread_mutex_destroy(&server_read_thread_mutex);

        pthread_exit(NULL);
        return;
    }

    while (1)
    {
        if (server_read_thread_stop_thread)
        {
            ALOGD(LOGTAG_SPP_SERVER "Exiting Read Thread!!");
            pthread_mutex_destroy(&server_read_thread_mutex);

            if (write_thread_exit == 1)
            {
                ALOGD(LOGTAG_SPP_SERVER "read thread close the socket\n");
                shutdown(kernel_sock_fd, SHUT_RDWR);
                close(kernel_sock_fd);
                kernel_sock_fd = 0;
            }
            read_thread_exit =1;
            pthread_exit(NULL);
        }

        buffer = (uint8_t *)osi_malloc(spp_max_frame_size+ NLMSG_LENGTH(0));
        if (buffer == NULL)
        {
            ALOGD(LOGTAG_SPP_SERVER "osi_malloc Failed!!\n");
        }
        memset(buffer, 0, spp_max_frame_size + NLMSG_LENGTH(0));

        retVal = recv(kernel_sock_fd, buffer, spp_max_frame_size, 0);
        if (retVal <= 0)
        {
            ALOGD(LOGTAG_SPP_SERVER "retVal is %d!!\n",retVal);
            pthread_mutex_destroy(&server_read_thread_mutex);
            if (buffer != NULL)
            {
                osi_free(buffer);
            }
            break;
        }

        ALOGD(LOGTAG_SPP_SERVER "read thread retVal : %d \n",retVal);
        ALOGD(LOGTAG_SPP_SERVER "read buffer is : %s \n",buffer);

        if (buffer && strstr((char *)buffer, "Read_Thread_Exit") != NULL)
        {
            ALOGD(LOGTAG_SPP_SERVER "%s Read_Thread_Exit!!!!\n",__func__);
            server_read_thread_stop_thread = true;
            osi_free(buffer);
            buffer = NULL;
        }
        else if (buffer && strstr((char *)buffer, "ttySpp") != NULL)
        {
            ALOGD(LOGTAG_SPP_SERVER "%s ttySpp !!!!\n",__func__);
            osi_free(buffer);
            buffer = NULL;
        }
        else
        {
            int count=0 ;

            char send_buffer[1024];
            char extra_char[10] = {0xD, 0xA};// 0XD:Carriage Return, 0xA: New Line Feed

            memcpy(send_buffer,buffer,retVal);
            memcpy(send_buffer+retVal, extra_char, 2);
            for(int i =0; i< retVal+2 ; i++)
            {
                ALOGD(LOGTAG_SPP_SERVER "send_buffer[%d]=%c %d",i, send_buffer[i], send_buffer[i]);
            }

            pthread_mutex_lock(&server_read_thread_mutex);

            ALOGD(LOGTAG_SPP_SERVER "%s data_socfd is %d", __func__, data_socfd);

            count = write(data_socfd, &send_buffer,retVal+2);
            if(count < 0)
            {
                ALOGD(LOGTAG_SPP_SERVER " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
            }
            else
            {
                ALOGD(LOGTAG_SPP_SERVER " sent bytes (%d)",count);
            }
            pthread_mutex_unlock(&server_read_thread_mutex);
            osi_free(buffer);
        }
    }
}

int Spp_Server::spp_start_socket_threads()
{
    pthread_attr_t server_read_thread_attr;
    int retVal = 0;

    write_thread_exit = 0;
    read_thread_exit = 0;

   ALOGD(LOGTAG_SPP_SERVER "spp_start_socket_threads!!");

    pthread_mutex_init(&server_read_thread_mutex, NULL);
    server_read_thread_stop_thread = false;

    pthread_attr_init(&server_read_thread_attr);
    pthread_attr_setdetachstate(&server_read_thread_attr, PTHREAD_CREATE_DETACHED);
    retVal =  pthread_create(&server_read_thread, &server_read_thread_attr, spp_server_read_thread_func, NULL);
    pthread_attr_destroy(&server_read_thread_attr);
    if (retVal != 0) {
        ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! Cannot create server_read_thread!\n");
        pthread_mutex_destroy(&server_read_thread_mutex);
        return -1 ;
    }
    return 0;
}

int Spp_Server::spp_server_create_socket()
{
    struct sockaddr_nl nladdr;
    int sz = 64 * 1024;
    int on = 1;
    int random = rand();

    ALOGD(LOGTAG_SPP_SERVER "spp_server_create_socket random value:%d\n",random);
    memset(&nladdr, 0, sizeof(nladdr));
    nladdr.nl_family = AF_NETLINK;
    nladdr.nl_pid = (unsigned int)random;
    nladdr.nl_pad = 0;
    nladdr.nl_groups = 0;

    if ((kernel_sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_SPP)) < 0)
    {
         ALOGD(LOGTAG_SPP_SERVER "Unable to create uevent socket");
        return -1;
    }

    if (setsockopt(kernel_sock_fd, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz)) < 0)
    {
        ALOGD(LOGTAG_SPP_SERVER "Unable to set uevent socket SO_RECBUFFORCE option");
        close(kernel_sock_fd);
        return -1;
    }

    if (setsockopt(kernel_sock_fd, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on)) < 0)
    {
        ALOGD(LOGTAG_SPP_SERVER "Unable to set uevent socket SO_PASSCRED option");
        close(kernel_sock_fd);
        return -1;
    }

    if ((setsockopt(kernel_sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) < 0)
    {
        ALOGD(LOGTAG_SPP_SERVER "Unable to set uevent socket SO_REUSEADDR option");
        close(kernel_sock_fd);
        return -1;
    }

    ALOGD(LOGTAG_SPP_SERVER "netlink nladdr.nl_pid is %d", nladdr.nl_pid);

    if (bind(kernel_sock_fd, (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0)
    {
        ALOGD(LOGTAG_SPP_SERVER "Unable to bind nladdr %s", strerror(errno));
        close(kernel_sock_fd);
        return -1;
    }

    ALOGD(LOGTAG_SPP_SERVER "kernel_sock_fd is %d", kernel_sock_fd);

    return kernel_sock_fd;
}

void Spp_Server::spp_server_thread_handler(int accept_sockfd)
{
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

    ALOGD(LOGTAG_SPP_SERVER "spp_server_thread_handler started");
    while (mServerState != STATE_SPP_SERVER_INACTIVE)
    {

        // Wait for sock_connect_signal_t message
        int count = recvmsg(accept_sockfd,&message,0);

        if (count==-1)
        {
            ALOGD(LOGTAG_SPP_SERVER "recvmsg returned -1");
            //die("%s",strerror(errno));
        }
        else if (message.msg_flags&MSG_TRUNC)
        {
            //warn("datagram too large for buffer: truncated");
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]MSG_TRUNC\n");
        }
        else if ( 0 == count )
        {
           ALOGE(LOGTAG_SPP_SERVER "ERROR listen sockfd closed");
           spp_server_write_thread_close();
           break;
        }

        // Handle the sock_connect_signal - remote address, server channel and status is received.

        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] count=%d", (int)count);
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]Received sock_connect_signal_t");
        pConnect_Sig = (sock_connect_signal_t *) buffer;

        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] size=%d", pConnect_Sig->size);
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] channel=%d", pConnect_Sig->channel);
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] status=%d", pConnect_Sig->status);

        spp_max_frame_size  = SPP_DATA_MTU;

        for(i =0; i< 6 ; i++)
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] bdadd[%d]= %02X\n",i, pConnect_Sig->bd_addr.address[i]);

        pcmsg = CMSG_FIRSTHDR(&message);
        p_acc_fd = (int*)CMSG_DATA(pcmsg);
        ALOGD(LOGTAG_SPP_SERVER "\n [AKK_DEBUB] CTRL msg len=%d, accept_fd =%d", pcmsg->cmsg_len,*p_acc_fd);

        // Send data structures
        char send_buffer[1024];
        struct iovec send_iov[1];
        send_iov[0].iov_base=send_buffer;
        send_iov[0].iov_len=sizeof(send_buffer);

        struct msghdr send_message;
        send_message.msg_name=&src_addr;
        send_message.msg_namelen=sizeof(src_addr);
        send_message.msg_iov=send_iov;
        send_message.msg_iovlen=1;

        if( !VALID_SRV_SOCFD(data_socfd) )
        {
            ALOGD(LOGTAG_SPP_SERVER "\n [AKK_DEBUB] set data_socfd and move to connected state.\n");
            data_socfd = *p_acc_fd;
            change_state(STATE_SPP_SERVER_CONNECTED);
        }
        else
        {
           ALOGD(LOGTAG_SPP_SERVER "Error -Active data soc fd is present, closing the existing fd(%d)", data_socfd);
           close(data_socfd);
           data_socfd= *p_acc_fd;
           change_state(STATE_SPP_SERVER_CONNECTED);
        }

        if(kernel_sock_fd == 0)   //Below code needs to executed when there is no connection i.e STATE_SPP_SERVER_INACTIVE
        {
            if(spp_server_create_socket() == -1)
            {
                ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! spp_server_create_socket FAILED!!\n");
            }
            else if (spp_start_socket_threads() == -1)
            {
                ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! spp_start_socket_threads FAILED!!\n");
            }
            else
            {
                ALOGD(LOGTAG_SPP_SERVER "spp_server socket, threads creation is SUCCESS!!\n");
            }
        }
    }


    ALOGD(LOGTAG_SPP_SERVER "server thread about to finish");
}

void Spp_Server::register_sdp_get_accept_socfd()
{

    // Invoke listen with socket type as RFCOMM, NULL UUID - This will map to SPP UUID
    btsock_interface = (btsock_interface_t_v1 *)bluetooth_interface->get_profile_interface(BT_PROFILE_SOCKETS_ID);

    int error = btsock_interface->listen(BTSOCK_RFCOMM,"SPP_SERVER",NULL,SPP_SERVER_CHANNEL,&listen_socfd,0,SPP_SERVER_APP_UID);

    if( error != BT_STATUS_SUCCESS )
    {
        ALOGE(LOGTAG_SPP_SERVER "Error !!!!! SPP server init failed -Unable to listen for incoming RFCOMM socket: %d\n", error);
        fprintf(stderr, "Unable to listen for incoming RFCOMM socket: %d\n", error);
    }
    else
    {
        change_state(STATE_SPP_SERVER_ACTIVE);
    }
}

void Spp_Server::receive_server_channel_info()
{
    char buffer[1024];
    struct sockaddr_storage src_addr;
    struct iovec iov[1];
    iov[0].iov_base=buffer;
    iov[0].iov_len=sizeof(buffer);
    int i =0;

    struct msghdr message;
    message.msg_name=&src_addr;
    message.msg_namelen=sizeof(src_addr);
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_control=0;
    message.msg_controllen=0;

    // Wait for Server channel number message
    int count = recvmsg(listen_socfd,&message,0);

    if (count==-1)
    {
        ALOGD(LOGTAG_SPP_SERVER "recvmsg returned -1");
        //die("%s",strerror(errno));
    }
    else if (message.msg_flags&MSG_TRUNC)
    {
        //warn("datagram too large for buffer: truncated");
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]MSG_TRUNC, count=%d", (int)count);
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]Received Server channel ID");
        for(i =0; i< count ; i++)
        {
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] buffer[%d]=%d",i, buffer[i]);
        }

    }
    else
    {
        // Handle the sock_connect_signal - remote address, server channel and status is received.
        //handle_datagram(buffer,count);

        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] count=%d", (int)count);
        ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]Received Server channel ID");
        for(i =0; i< count ; i++)
        {
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] buffer[%d]=%d",i, buffer[i]);
        }

    }

    if( mServerState == STATE_SPP_SERVER_ACTIVE )
    {
        if (pthread_create(&server_thread, NULL, spp_server_thread_func, &listen_socfd) != 0) {
            ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! Cannot create spp server thread!\n");
            return;
        }

        pthread_mutex_init(&recv_mutex, NULL);
        pthread_cond_init(&start_recv_cv, NULL);
        if (pthread_create(&server_recv_thread, NULL, spp_server_recv_thread_func, NULL) != 0) {
            ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! Cannot create spp server receive thread!\n");
            return;
        }

        pthread_mutex_init(&send_mutex, NULL);
        pthread_cond_init(&start_send_cv, NULL);
        if (pthread_create(&server_send_thread, NULL, spp_server_send_thread_func, NULL) != 0) {
            ALOGD(LOGTAG_SPP_SERVER "!! ERROR !! Cannot create spp server send thread!\n");
            return;
        }
    }
    else
    {
        ALOGE(LOGTAG_SPP_SERVER "Error !!!!! SPP server enable failed\n");
    }

}

void Spp_Server::server_init()
{

    register_sdp_get_accept_socfd();

    receive_server_channel_info();

    ALOGD(LOGTAG_SPP_SERVER "Check looks like connection is established");

}

void Spp_Server::HandleEnableServer(void) {

    BtEvent *pEvent = new BtEvent;

    if (bluetooth_interface != NULL)
    {
        server_init();
    }
    else
    {
        fprintf(stdout, "Error - bluetooth_interface is NULL, Enable SPP server failed");
        ALOGE(LOGTAG_SPP_SERVER "Error - bluetooth_interface is NULL, Enable SPP server failed");
    }
}

void Spp_Server::HandleDisableServer(void) {

   change_state(STATE_SPP_SERVER_INACTIVE);

   BtEvent *pEvent = new BtEvent;
   pEvent->profile_stop_event.event_id = PROFILE_EVENT_STOP_DONE;
   pEvent->profile_stop_event.profile_id = PROFILE_ID_SPP_SERVER;
   pEvent->profile_stop_event.status = true;
   PostMessage(THREAD_ID_GAP, pEvent);
}

void Spp_Server::ProcessEvent(BtEvent* pEvent) {

    switch(mServerState) {

        case STATE_SPP_SERVER_INACTIVE:
            state_inactive_handler(pEvent);
            break;

        case STATE_SPP_SERVER_ACTIVE:
            state_disconnected_active_handler(pEvent);
            break;

        case STATE_SPP_SERVER_CONNECTED:
            state_connected_handler(pEvent);
            break;

        case STATE_SPP_SERVER_RECEIVE_FILE:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_SERVER_SEND_FILE:
            state_send_receive_handler(pEvent);
            break;

        case STATE_SPP_SERVER_DISCONNECTED:
            state_disconnected_active_handler(pEvent);
            break;

    }
}

char* Spp_Server::dump_message(BluetoothEventId event_id) {

    switch(event_id) {

        case SPP_SRV_START:
            return (char*)"SPP_SRV_START";

        case SPP_SRV_RECV_FILE:
            return (char*)"SPP_SRV_RECV_FILE";

        case SPP_SRV_SEND_FILE:
            return (char*)"SPP_SRV_SEND_FILE";

        case SPP_SRV_DISCONNECT:
            return (char*)"SPP_SRV_DISCONNECT";

    }
    return (char*)"UNKNOWN";
}

void Spp_Server::state_inactive_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_SERVER "state_inactive_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        case SPP_SRV_START:
            {
                if (bluetooth_interface != NULL)
                {
                    server_init();
                }
                else
                {
                    fprintf(stdout, "Error - bluetooth_interface is NULL, Enable SPP server failed");
                    ALOGE(LOGTAG_SPP_SERVER "Error - bluetooth_interface is NULL, Enable SPP server failed");
                }
            }
            break;

        default:
            fprintf(stdout, "Event not processed in in-active state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_SERVER " event not handled %d (%s) ", pEvent->event_id,dump_message(pEvent->event_id) );
            break;
    }
}



int Spp_Server::receive_file(const char* fname, int &soc_fd)
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

    ALOGD(LOGTAG_SPP_SERVER "--> receive_file, fname=%s, soc_fd=%d\n", fname, soc_fd);


    ofstream recv_file(fname,ios::out | ios::binary);

    if( recv_file.is_open())
    {
        while ((count = recvmsg(soc_fd,&message,0)) > 0 )
        {
            recv_file.write(buffer,count);
            recv_file.flush();
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB]RECVD DATA, count=%d", (int)count);
            for(i =0; i< count ; i++)
            {
                ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] buffer[%d]=%c",i, buffer[i]);
            }
        }

        recv_file.flush();
        recv_file.close();

        if( count == 0 )
        {
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] connection closed by the remote");
            spp_server_write_thread_close();
            RESET_SRV_SOCFD(data_socfd);
        }
        else
        {
            ALOGD(LOGTAG_SPP_SERVER "[AKK_DEBUB] Receive Error count=%d", count);
            status = FAILED;
        }
    }
    else
    {
        ALOGE(LOGTAG_SPP_SERVER "Error opening file=%s\n", fname);
        status = FAILED;
    }

    ALOGD(LOGTAG_SPP_SERVER "<-- receive_file, fname=%s, soc_fd=%d\n", fname, soc_fd);

    return status;

}

int Spp_Server::snd_file(const char* fname, int &soc_fd)
{
    int count  = 0;
    int status = SUCCESS;
    char buffer[1024];
    int max_read_size=500;

    ALOGD(LOGTAG_SPP_SERVER "--> snd_file, fname=%s, soc_fd=%d\n", fname, soc_fd);


    ifstream snd_file(fname,ios::in | ios::binary);

    if( snd_file.is_open())
    {
        while (!snd_file.eof())
        {
            snd_file.read(buffer,max_read_size);

            count = send(soc_fd,&buffer,snd_file.gcount(),MSG_NOSIGNAL);

            if(count < 0)
            {
                ALOGD(LOGTAG_SPP_SERVER " sendmsg failed (ret=%d,errno=%d,err=%s)",count,errno,strerror(errno));
                ALOGE(LOGTAG_SPP_SERVER " Aborting send_file, Invalid data socfd, connection may be lost\n");
                fprintf(stderr, " Aborting send_file, Invalid data socfd, connection may be lost, error(%s) \n",strerror(errno));
                RESET_SRV_SOCFD(soc_fd);
                status = FAILED;
                break;
            }
            else
            {
                ALOGD(LOGTAG_SPP_SERVER " sent bytes (%d)",count);
            }
        }

        snd_file.close();
    }
    else
    {
        ALOGE(LOGTAG_SPP_SERVER "Error opening file=%s\n", fname);
        status = FAILED;
    }

    ALOGD(LOGTAG_SPP_SERVER "<-- receive_file, fname=%s, soc_fd=%d\n", fname, soc_fd);

    return status;

}

void Spp_Server::state_connected_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_SERVER "state_connected_handler Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        case SPP_SRV_RECV_FILE:
            {
                file_name = pEvent->spp_srv_event.value;
                change_state(STATE_SPP_SERVER_RECEIVE_FILE);
                pthread_mutex_lock(&recv_mutex);
                pthread_cond_signal(&start_recv_cv);
                pthread_mutex_unlock(&recv_mutex);
            }
            break;

        case SPP_SRV_SEND_FILE:
            {
                file_name = pEvent->spp_srv_event.value;
                change_state(STATE_SPP_SERVER_SEND_FILE);
                pthread_mutex_lock(&send_mutex);
                pthread_cond_signal(&start_send_cv);
                pthread_mutex_unlock(&send_mutex);
            }
            break;

        case SPP_SRV_DISCONNECT:
            {
                spp_server_write_thread_close();
                shutdown(data_socfd, SHUT_RDWR);
                close(data_socfd);
                RESET_SRV_SOCFD(data_socfd);
                change_state(STATE_SPP_SERVER_ACTIVE);
            }
            break;


        default:
            fprintf(stdout, "Event not processed in connected state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_SERVER " event not handled %d ", pEvent->event_id);
            break;
    }
}

void Spp_Server::state_send_receive_handler(BtEvent* pEvent) {

    if(mServerState == STATE_SPP_SERVER_SEND_FILE)
    {
        ALOGD(LOGTAG_SPP_SERVER " SPP-SRV 'SEND' state, Processing event %s", dump_message(pEvent->event_id));
    }
    else
    {
        ALOGD(LOGTAG_SPP_SERVER " SPP-SRV 'RECEIVE' state Processing event %s", dump_message(pEvent->event_id));
    }

    switch(pEvent->event_id) {

        case SPP_SRV_DISCONNECT:
            {
                spp_server_write_thread_close();
                shutdown(data_socfd, SHUT_RDWR);
                close(data_socfd);
                RESET_SRV_SOCFD(data_socfd);
                change_state(STATE_SPP_SERVER_ACTIVE);
            }
            break;


        default:
        {
            if(mServerState == STATE_SPP_SERVER_SEND_FILE)
            {
                fprintf(stdout, "Event not processed in 'SEND' state %d ", pEvent->event_id);
            }
            else
            {
                fprintf(stdout, "Event not processed in 'RECEIVE' state %d ", pEvent->event_id);
            }

            ALOGE(LOGTAG_SPP_SERVER " event not handled %d ", pEvent->event_id);
        }
        break;
    }
}


void Spp_Server::state_disconnected_active_handler(BtEvent* pEvent) {

    ALOGD(LOGTAG_SPP_SERVER "state Disconnected or Active Processing event %s", dump_message(pEvent->event_id));

    switch(pEvent->event_id) {

        default:
            fprintf(stdout, "Event not processed in Disconnected or Active state %d ", pEvent->event_id);
            ALOGE(LOGTAG_SPP_SERVER " event not handled %d ", pEvent->event_id);
            break;
    }
}

void Spp_Server::change_state(SppServerState mState) {
   ALOGD(LOGTAG_SPP_SERVER " current State = %d, new state = %d", mServerState, mState);
   pthread_mutex_lock(&lock);
   mServerState = mState;
   pthread_mutex_unlock(&lock);
   ALOGD(LOGTAG_SPP_SERVER " state changed to %d ", mState);
}

Spp_Server :: Spp_Server(const bt_interface_t *bt_interface, config_t *config) {
    ALOGD(LOGTAG_SPP_SERVER " Spp_Server constructor");
    this->bluetooth_interface = bt_interface;
    this->config              = config;
    RESET_SRV_SOCFD(listen_socfd);
    RESET_SRV_SOCFD(data_socfd);
    pthread_mutex_init(&this->lock, NULL);
    change_state(STATE_SPP_SERVER_INACTIVE);
}

Spp_Server :: ~Spp_Server() {
    change_state(STATE_SPP_SERVER_INACTIVE);
    pthread_mutex_destroy(&lock);
    close(listen_socfd);
    close(data_socfd);
    RESET_SRV_SOCFD(listen_socfd);
    RESET_SRV_SOCFD(data_socfd);
}

