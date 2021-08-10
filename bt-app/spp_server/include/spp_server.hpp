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

#ifndef SPP_SERVER_APP_H
#define SPP_SERVER_APP_H

#include <map>
#include <string>
#include <hardware/bluetooth.h>

#include "osi/include/log.h"
#include "osi/include/thread.h"
#include "osi/include/config.h"
#include "osi/include/allocator.h"
#include "ipc.hpp"
#include "utils.h"
#include <hardware/bt_obex_sock.h>



typedef enum {
    STATE_SPP_SERVER_INACTIVE = 0,
    STATE_SPP_SERVER_ACTIVE,
    STATE_SPP_SERVER_CONNECTED,
    STATE_SPP_SERVER_RECEIVE_FILE,
    STATE_SPP_SERVER_SEND_FILE,
    STATE_SPP_SERVER_DISCONNECTED,
}SppServerState;

#define SPP_SERVER_CHANNEL (5)
#define NETLINK_SPP         31
#define NL_MSG_CTL          0
class Spp_Server {

  private:
    config_t *config;
    const bt_interface_t * bluetooth_interface;
    btsock_interface_t_v1 * btsock_interface;
    int listen_socfd;
    int data_socfd;
    std::string file_name;
    SppServerState mServerState;

    pthread_t server_read_thread = -1;
    pthread_t server_write_thread = -1;
    pthread_mutex_t server_read_thread_mutex;
    pthread_mutex_t server_write_thread_mutex;
    bool server_read_thread_stop_thread = false;
    bool server_write_thread_stop_thread = false;
    int write_thread_exit = 0;
    int read_thread_exit = 0;
    int kernel_sock_fd = 0;
    int spp_max_frame_size = 0;
    bool tty_opened = false;

  public:

    Spp_Server(const bt_interface_t *bt_interface, config_t *config);
    ~Spp_Server();
    void ProcessEvent(BtEvent* pEvent);
    void state_inactive_handler(BtEvent* pEvent);
    void state_connected_handler(BtEvent* pEvent);
    void state_send_receive_handler(BtEvent* pEvent);
    void change_state(SppServerState mState);
    char* dump_message(BluetoothEventId event_id);
    pthread_mutex_t lock;
    void HandleEnableServer();
    void HandleDisableServer();
    void server_init();
    void register_sdp_get_accept_socfd();
    void receive_server_channel_info();
    SppServerState getState() { return mServerState; }
    int receive_file(const char* fname, int &soc_fd);
    int snd_file(const char* fname, int &soc_fd);
    void state_disconnected_active_handler(BtEvent* pEvent);
    void sppsrv_send_thread_handler();
    void sppsrv_recv_thread_handler();
    void spp_server_thread_handler(int accept_sockfd);
    int spp_server_create_socket();
    int spp_start_socket_threads();
    void spp_server_read_thread_handler();
    void spp_server_write_thread_handler();
    int spp_server_set_tty_driver_state(uint8_t ttyState);
    void spp_server_read_thread_close();
    void spp_server_write_thread_close();
};

#endif /* SPP_SERVER_APP_H */
