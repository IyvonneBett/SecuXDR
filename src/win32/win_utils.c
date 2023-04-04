/* Copyright (C) 2015, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifdef WIN32
#include "shared.h"
#include "client-agent/agentd.h"
#include "logcollector/logcollector.h"
#include "os_execd/execd.h"
#include "wazuh_modules/wmodules.h"
#include "sysInfo.h"
#include "sym_load.h"
#include "../os_net/os_net.h"
#include "dll_load_notify.h"

#ifdef WAZUH_UNIT_TESTING
#include "unit_tests/wrappers/windows/libc/kernel32_wrappers.h"
#endif

HANDLE hMutex;
int win_debug_level;

void *sysinfo_module = NULL;
sysinfo_networks_func sysinfo_network_ptr = NULL;
sysinfo_free_result_func sysinfo_free_result_ptr = NULL;

/** Prototypes **/
int Start_win32_Syscheck();

/* syscheck main thread */
#ifdef WIN32
DWORD WINAPI skthread(__attribute__((unused)) LPVOID arg)
#else
void *skthread()
#endif
{

    Start_win32_Syscheck();
#ifdef WIN32
    return 0;
#else
    return (NULL);
#endif
}

void stop_wmodules()
{
    wmodule * cur_module;
    for (cur_module = wmodules; cur_module; cur_module = cur_module->next) {
        if (cur_module->context->stop) {
            cur_module->context->stop(cur_module->data);
        }
    }
}

/* Locally start (after service/win init) */
int local_start()
{
    char *cfg = OSSECCONF;
    WSADATA wsaData;
    DWORD  threadID;
    DWORD  threadID2;

    win_debug_level = getDefine_Int("windows", "debug", 0, 2);

    /* Get debug level */
    int debug_level = win_debug_level;
    while (debug_level != 0) {
        nowDebug();
        debug_level--;
    }

    enable_dll_verification();

    if (sysinfo_module = so_get_module_handle("sysinfo"), sysinfo_module)
    {
        sysinfo_free_result_ptr = so_get_function_sym(sysinfo_module, "sysinfo_free_result");
        sysinfo_network_ptr = so_get_function_sym(sysinfo_module, "sysinfo_networks");
    }

    /* Initialize logging module*/
    w_logging_init();

    /* Start agent */
    os_calloc(1, sizeof(agent), agt);

    /* Configuration file not present */
    if (File_DateofChange(cfg) < 0) {
        merror_exit("Configuration file '%s' not found", cfg);
    }

    /* Start Winsock */
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
        merror_exit("WSAStartup() failed");
    }

    /* Initialize error logging for shared modulesd */
    dbsync_initialize(loggingErrorFunction);
    rsync_initialize(loggingErrorFunction);

    /* Read agent config */
    mdebug1("Reading agent configuration.");
    if (ClientConf(cfg) < 0) {
        merror_exit(CLIENT_ERROR);
    }

    if (!Validate_Address(agt->server)){
        merror(AG_INV_MNGIP, agt->server[0].rip);
        merror_exit(CLIENT_ERROR);
    }

    if (!Validate_IPv6_Link_Local_Interface(agt->server)){
        merror(AG_INV_INT);
        merror_exit(CLIENT_ERROR);
    }

    if (agt->notify_time == 0) {
        agt->notify_time = NOTIFY_TIME;
    }
    if (agt->max_time_reconnect_try == 0 ) {
        agt->max_time_reconnect_try = RECONNECT_TIME;
    }
    if (agt->max_time_reconnect_try <= agt->notify_time) {
        agt->max_time_reconnect_try = (agt->notify_time * 3);
        minfo("Max time to reconnect can't be less than notify_time(%d), using notify_time*3 (%d)", agt->notify_time, agt->max_time_reconnect_try);
    }
    minfo("Using notify time: %d and max time to reconnect: %d", agt->notify_time, agt->max_time_reconnect_try);
    if (agt->force_reconnect_interval) {
        minfo("Using force reconnect interval, Wazuh Agent will reconnect every %ld %s", \
               w_seconds_to_time_value(agt->force_reconnect_interval), w_seconds_to_time_unit(agt->force_reconnect_interval, TRUE));
    }

    /* Read logcollector config file */
    mdebug1("Reading logcollector configuration.");

    /* Init message queue */
    w_msg_hash_queues_init();

    if (LogCollectorConfig(cfg) < 0) {
        merror_exit(CONFIG_ERROR, cfg);
    }

    if(agt->enrollment_cfg && agt->enrollment_cfg->enabled) {
        // If autoenrollment is enabled, we will avoid exit if there is no valid key
        OS_PassEmptyKeyfile();
    } else {
        /* Check auth keys */
        if (!OS_CheckKeys()) {
            merror_exit(AG_NOKEYS_EXIT);
        }
    }
    /* Read keys */
    minfo(ENC_READ);
    OS_ReadKeys(&keys, W_DUAL_KEY, 0);

    /* If there is no file to monitor, create a clean entry
     * for the mark messages.
     */
    if (logff == NULL) {
        os_calloc(2, sizeof(logreader), logff);
        logff[0].file = NULL;
        logff[0].ffile = NULL;
        logff[0].logformat = NULL;
        logff[0].fp = NULL;
        logff[1].file = NULL;
        logff[1].logformat = NULL;

        minfo(NO_FILE);
    }

    /* No sockets defined */
    if (logsk == NULL) {
        os_calloc(2, sizeof(logsocket), logsk);
        logsk[0].name = NULL;
        logsk[0].location = NULL;
        logsk[0].mode = 0;
        logsk[0].prefix = NULL;
        logsk[1].name = NULL;
        logsk[1].location = NULL;
        logsk[1].mode = 0;
        logsk[1].prefix = NULL;
    }

    /* Read execd config */
    if (!WinExecdStart()) {
        agt->execdq = -1;
    }

    /* Initialize sender */
    sender_init();

    /* Initialize random numbers */
    srandom(time(0));
    os_random();

    // Initialize children pool
    wm_children_pool_init();

    /* Start buffer thread */
    if (agt->buffer){
        buffer_init();
        w_create_thread(NULL,
                         0,
                         dispatch_buffer,
                         NULL,
                         0,
                         (LPDWORD)&threadID);
    }else{
        minfo(DISABLED_BUFFER);
    }

    /* state_main thread */
    w_agentd_state_init();
    w_create_thread(NULL,
                     0,
                     state_main,
                     NULL,
                     0,
                     (LPDWORD)&threadID);

    /* Socket connection */
    agt->sock = -1;

    /* Start mutex */
    mdebug1("Creating thread mutex.");
    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (hMutex == NULL) {
        merror_exit("Error creating mutex.");
    }
    /* Start syscheck thread */
    w_create_thread(NULL,
                     0,
                     skthread,
                     NULL,
                     0,
                     (LPDWORD)&threadID);

    /* Launch rotation thread */
    int rotate_log = getDefine_Int("monitord", "rotate_log", 0, 1);
    if (rotate_log) {
        w_create_thread(NULL,
                        0,
                        w_rotate_log_thread,
                        NULL,
                        0,
                        (LPDWORD)&threadID);
    }

    /* Check if server is connected */
    os_setwait();
    start_agent(1);
    os_delwait();
    w_agentd_state_update(UPDATE_STATUS, (void *) GA_STATUS_ACTIVE);

    req_init();

    /* Start receiver thread */
    w_create_thread(NULL,
                     0,
                     receiver_thread,
                     NULL,
                     0,
                     (LPDWORD)&threadID2);

    /* Start request receiver thread */
    w_create_thread(NULL,
                     0,
                     req_receiver,
                     NULL,
                     0,
                     (LPDWORD)&threadID2);

    // Read wodle configuration and start modules

    if (!wm_config() && !wm_check()) {
        wmodule * cur_module;

        for (cur_module = wmodules; cur_module; cur_module = cur_module->next) {
            w_create_thread(NULL,
                            0,
                            cur_module->context->start,
                            cur_module->data,
                            0,
                            (LPDWORD)&threadID2);
        }
    }

    /* Send agent stopped message at exit */
    atexit(send_agent_stopped_message);

    /* Start logcollector -- main process here */
    LogCollectorStart();

    if (sysinfo_module){
        so_free_library(sysinfo_module);
    }

    WSACleanup();
    return (0);
}

/* SendMSGAction for Windows */
int SendMSGAction(__attribute__((unused)) int queue, const char *message, const char *locmsg, char loc)
{
    char loc_buff[OS_SIZE_8192 + 1] = {0};
    char tmpstr[OS_MAXSTR + 2];
    DWORD dwWaitResult;
    int retval = -1;
    tmpstr[OS_MAXSTR + 1] = '\0';

    /* Using a mutex to synchronize the writes */
    while (1) {
        dwWaitResult = WaitForSingleObject(hMutex, 1000000L);

        if (dwWaitResult != WAIT_OBJECT_0) {
            switch (dwWaitResult) {
                case WAIT_TIMEOUT:
                    mdebug2("Sending mutex timeout.");
                    sleep(5);
                    continue;
                case WAIT_ABANDONED:
                    merror("Error waiting mutex (abandoned).");
                    return retval;
                default:
                    merror("Error waiting mutex.");
                    return retval;
            }
        } else {
            /* Lock acquired */
            break;
        }
    }   /* end - while for mutex... */

    if (OS_INVALID == wstr_escape(loc_buff, sizeof(loc_buff), (char *) locmsg, '|', ':')) {
        merror(FORMAT_ERROR);
        return retval;
    }

    snprintf(tmpstr, OS_MAXSTR, "%c:%s:%s", loc, loc_buff, message);

    /* Send events to the manager across the buffer */
    if (!agt->buffer){
        w_agentd_state_update(INCREMENT_MSG_COUNT, NULL);
        if (send_msg(tmpstr, -1) >= 0) {
            retval = 0;
        }
    } else {
        buffer_append(tmpstr);
        retval = 0;
    }

    if (!ReleaseMutex(hMutex)) {
        merror("Error releasing mutex.");
    }
    return retval;
}

/* SendMSG for Windows */
int SendMSG(__attribute__((unused)) int queue, const char *message, const char *locmsg, char loc) {
    os_wait();
    return SendMSGAction(queue, message, locmsg, loc);
}

/* SendMSGPredicated for Windows */
int SendMSGPredicated(__attribute__((unused)) int queue, const char *message, const char *locmsg, char loc, bool (*fn_ptr)()) {
    os_wait_predicate(fn_ptr);
    return SendMSGAction(queue, message, locmsg, loc);
}

/* StartMQ for Windows */
int StartMQWithSpecificOwnerAndPerms(__attribute__((unused)) const char *path
                                     ,__attribute__((unused)) short int type
                                     ,__attribute__((unused)) short int n_tries
                                     ,__attribute__((unused)) uid_t uid
                                     ,__attribute__((unused)) gid_t gid
                                     ,__attribute__((unused)) mode_t perm)
{
    return (0);
}

/* StartMQ for Windows */
int StartMQ(__attribute__((unused)) const char *path, __attribute__((unused)) short int type, __attribute__((unused)) short int n_tries)
{
    return (0);
}

/* MQReconnectPredicated for Windows */
int MQReconnectPredicated(__attribute__((unused)) const char *path, __attribute__((unused)) bool (fn_ptr)())
{
    return (0);
}

char *get_agent_ip()
{
    char agent_ip[IPSIZE + 1] = { '\0' };
    cJSON *object;

    if (sysinfo_network_ptr && sysinfo_free_result_ptr) {
        const int error_code = sysinfo_network_ptr(&object);
        if (error_code == 0) {
            if (object) {
                const cJSON *iface = cJSON_GetObjectItem(object, "iface");
                if (iface) {
                    const int size_ids = cJSON_GetArraySize(iface);
                    for (int i = 0; i < size_ids; ++i){
                        const cJSON *element = cJSON_GetArrayItem(iface, i);
                        if(!element) {
                            continue;
                        }
                        cJSON *gateway = cJSON_GetObjectItem(element, "gateway");
                        if(gateway && cJSON_GetStringValue(gateway) && 0 != strcmp(gateway->valuestring, " ")) {

                            const char * primaryIpType = NULL;
                            const char * secondaryIpType = NULL;

                            if (strchr(gateway->valuestring, ':') != NULL) {
                                // Assume gateway is IPv6. IPv6 IP will be prioritary
                                primaryIpType = "IPv6";
                                secondaryIpType = "IPv4";
                            } else {
                                // Assume gateway is IPv4. IPv4 IP will be prioritary
                                primaryIpType = "IPv4";
                                secondaryIpType = "IPv6";
                            }

                            const cJSON * ip = cJSON_GetObjectItem(element, primaryIpType);
                            if (!ip) {
                                ip = cJSON_GetObjectItem(element, secondaryIpType);
                                if (!ip) {
                                    continue;
                                }
                            }
                            const int size_proto_interfaces = cJSON_GetArraySize(ip);
                            for (int j = 0; j < size_proto_interfaces; ++j) {
                                const cJSON *element_ip = cJSON_GetArrayItem(ip, j);
                                if(!element_ip) {
                                    continue;
                                }
                                cJSON *address = cJSON_GetObjectItem(element_ip, "address");
                                if (address && cJSON_GetStringValue(address))
                                {
                                    strncpy(agent_ip, address->valuestring, IPSIZE);
                                    break;
                                }
                            }
                            if (*agent_ip != '\0') {
                                break;
                            }
                        }
                    }
                }
                sysinfo_free_result_ptr(&object);
            }
        }
        else {
            merror("Unable to get system network information. Error code: %d.", error_code);
        }
    }

    if (strchr(agent_ip, ':') != NULL) {
        OS_ExpandIPv6(agent_ip, IPSIZE);
    }

    return strdup(agent_ip);
}

#endif
