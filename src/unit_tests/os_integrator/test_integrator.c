/*
 * Copyright (C) 2015, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>

#include "headers/shared.h"
#include "os_integrator/integrator.h"
#include "../wrappers/common.h"
#include "../wrappers/wazuh/shared/debug_op_wrappers.h"
#include "../wrappers/libc/stdio_wrappers.h"

static int test_setup(void **state) {
    test_mode = 1;

    IntegratorConfig *virustotal_config = calloc(1, sizeof(IntegratorConfig));
    IntegratorConfig *pagerduty_config = calloc(1, sizeof(IntegratorConfig));

    virustotal_config->name = "virustotal";
    virustotal_config->apikey = "123456";
    virustotal_config->group = "syscheck";
    virustotal_config->alert_format = "json";
    virustotal_config->enabled = 1;

    pagerduty_config->name = "pagerduty";
    pagerduty_config->apikey = "123456";
    pagerduty_config->group = "syscheck";
    pagerduty_config->enabled = 1;
    pagerduty_config->max_log = 165;

    state[0] = virustotal_config;
    state[1] = pagerduty_config;

    return OS_SUCCESS;
}

static int test_teardown(void **state) {
    test_mode = 0;

    (void) state;

    IntegratorConfig *virustotal_config = state[0];
    IntegratorConfig *pagerduty_config = state[1];

    os_free(virustotal_config->path);
    os_free(pagerduty_config->path);

    os_free(virustotal_config);
    os_free(pagerduty_config);

    return OS_SUCCESS;
}

void test_OS_IntegratorD(void **state) {
    IntegratorConfig *integrator_config[3];
    integrator_config[0] = state[0];
    integrator_config[1] = state[1];
    integrator_config[2] = NULL;

    wfd_t *wfd = NULL;
    os_calloc(1, sizeof(wfd_t), wfd);
    wfd->file_out = (FILE *)1;

    const char *al_string = "{\"timestamp\":\"2022-09-09T23:43:15.168+0200\",\"rule\":{\"level\":7,\"description\":\"Integrity checksum changed.\",\"id\":\"550\",\"mitre\":{\"id\":[\"T1565.001\"],\"tactic\":[\"Impact\"],\"technique\":[\"Stored Data Manipulation\"]},\"firedtimes\":2,\"mail\":false,\"groups\":[\"ossec\",\"syscheck\",\"syscheck_entry_modified\",\"syscheck_file\"],\"pci_dss\":[\"11.5\"],\"gpg13\":[\"4.11\"],\"gdpr\":[\"II_5.1.f\"],\"hipaa\":[\"164.312.c.1\",\"164.312.c.2\"],\"nist_800_53\":[\"SI.7\"],\"tsc\":[\"PI1.4\",\"PI1.5\",\"CC6.1\",\"CC6.8\",\"CC7.2\",\"CC7.3\"]},\"agent\":{\"id\":\"000\",\"name\":\"jellyfish\"},\"manager\":{\"name\":\"jellyfish\"},\"id\":\"1662759795.647670\",\"cluster\":{\"name\":\"wazuh\",\"node\":\"node01\"},\"full_log\":\"File '/tmp/test/test.txt' modified\nMode: realtime\nChanged attributes: size,mtime,md5,sha1,sha256\nSize changed from '54' to '57'\nOld modification time was: '1662759745', now it is '1662759795'\nOld md5sum was: '1e6f0765ec3e57572afde86319d460bf'\nNew md5sum is : '5192496b8adc2f0d705ca01bf3b4adba'\nOld sha1sum was: '652c1e4a301df0d1e7236689cb1e0bd071f2ea14'\nNew sha1sum is : 'dc090d4e165df77333ccf6adaf0d4f96541fb22b'\nOld sha256sum was: '1b32b746fe70a01ddd274f6b71bfaffd8a7fcd8023e18516078f31184da1135c'\nNew sha256sum is : '5583bbc9f63d24e44bbe34298d2f8421da25cdaada00e6c9ac765a16ded4204b'\n\",\"syscheck\":{\"path\":\"/tmp/test/test.txt\",\"mode\":\"realtime\",\"size_before\":\"54\",\"size_after\":\"57\",\"perm_after\":\"rw-r--r--\",\"uid_after\":\"0\",\"gid_after\":\"0\",\"md5_before\":\"1e6f0765ec3e57572afde86319d460bf\",\"md5_after\":\"5192496b8adc2f0d705ca01bf3b4adba\",\"sha1_before\":\"652c1e4a301df0d1e7236689cb1e0bd071f2ea14\",\"sha1_after\":\"dc090d4e165df77333ccf6adaf0d4f96541fb22b\",\"sha256_before\":\"1b32b746fe70a01ddd274f6b71bfaffd8a7fcd8023e18516078f31184da1135c\",\"sha256_after\":\"5583bbc9f63d24e44bbe34298d2f8421da25cdaada00e6c9ac765a16ded4204b\",\"uname_after\":\"root\",\"gname_after\":\"root\",\"mtime_before\":\"2022-09-09T23:42:25\",\"mtime_after\":\"2022-09-09T23:43:15\",\"inode_after\":2362547,\"changed_attributes\":[\"size\",\"mtime\",\"md5\",\"sha1\",\"sha256\"],\"event\":\"modified\"},\"decoder\":{\"name\":\"syscheck_integrity_changed\"},\"location\":\"syscheck\"}";

    cJSON *al_json = NULL;
    al_json = cJSON_Parse(al_string);
    char *unformatted = cJSON_PrintUnformatted(al_json);
    char alert_to_virustotal[2048];
    snprintf(alert_to_virustotal, 2048, "%s\n",unformatted);
    char *alert_to_pagerduty = "alertdate='2022-09-09T23:43:15.168+0200'\nalertlocation='syscheck'\nruleid='550'\nalertlevel='7'\nruledescription='Integrity checksum changed.'\nalertlog='File  /tmp/test/test.txt  modified Mode: realtime Changed attributes: size,mtime,md5,sha1,sha256 Size changed from  54  to  57  Old modification time was:  166275...'\nsrcip=''";

    const char *virustotal_file = "/tmp/virustotal-1111-2222.alert";
    const char *pagerduty_file = "/tmp/pagerduty-1111-2222.alert";

    will_return(__wrap_jqueue_open, 0);

    expect_string(__wrap__mdebug1, formatted_msg, "JSON file queue connected.");

    expect_string(__wrap_File_DateofChange, file, "integrations/virustotal");
    will_return(__wrap_File_DateofChange, 1);

    expect_string(__wrap__minfo, formatted_msg, "Enabling integration for: 'virustotal'.");

    expect_string(__wrap_File_DateofChange, file, "integrations/pagerduty");
    will_return(__wrap_File_DateofChange, 1);

    expect_string(__wrap__minfo, formatted_msg, "Enabling integration for: 'pagerduty'.");

    will_return(__wrap_FOREVER, 1);

    expect_string(__wrap__mdebug2, formatted_msg, "jqueue_next()");

    will_return(__wrap_jqueue_next, al_json);

    expect_string(__wrap__mdebug1, formatted_msg, "sending new alert.");

    will_return(__wrap_time, 1111);

    will_return(__wrap_os_random, 2222);

    expect_string(__wrap_fopen, path, virustotal_file);
    expect_string(__wrap_fopen, mode, "w");
    will_return(__wrap_fopen, (FILE *)1);

    expect_fprintf((FILE *)1, alert_to_virustotal, 0);

    expect_string(__wrap__mdebug2, formatted_msg, "file /tmp/virustotal-1111-2222.alert was written.");

    expect_fclose((FILE *)1, 0);

    expect_string(__wrap__mdebug1, formatted_msg, "Running: integrations /tmp/virustotal-1111-2222.alert 123456   > /dev/null 2>&1");

    will_return(__wrap_wpopenv, wfd);

    expect_value(__wrap_fgets, __stream, wfd->file_out);
    will_return(__wrap_fgets, "test");

    expect_string(__wrap__mdebug2, formatted_msg, "integratord: test");

    expect_value(__wrap_fgets, __stream, wfd->file_out);
    will_return(__wrap_fgets, 0);

    will_return(__wrap_wpclose, 0);

    expect_string(__wrap__mdebug1, formatted_msg, "Command ran successfully.");

    expect_string(__wrap_unlink, file, virustotal_file);
    will_return(__wrap_unlink, 0);

    will_return(__wrap_time, 1111);

    will_return(__wrap_os_random, 2222);

    expect_string(__wrap_fopen, path, pagerduty_file);
    expect_string(__wrap_fopen, mode, "w");
    will_return(__wrap_fopen, (FILE *)1);

    expect_fprintf((FILE *)1, alert_to_pagerduty, 0);

    expect_string(__wrap__mdebug2, formatted_msg, "file /tmp/pagerduty-1111-2222.alert was written.");

    expect_fclose((FILE *)1, 0);

    expect_string(__wrap__mdebug1, formatted_msg, "Running: integrations /tmp/pagerduty-1111-2222.alert 123456   > /dev/null 2>&1");

    will_return(__wrap_wpopenv, wfd);

    expect_value(__wrap_fgets, __stream, wfd->file_out);
    will_return(__wrap_fgets, "test");

    expect_string(__wrap__mdebug2, formatted_msg, "integratord: test");

    expect_value(__wrap_fgets, __stream, wfd->file_out);
    will_return(__wrap_fgets, 0);

    will_return(__wrap_wpclose, 0);

    expect_string(__wrap__mdebug1, formatted_msg, "Command ran successfully.");

    expect_string(__wrap_unlink, file, pagerduty_file);
    will_return(__wrap_unlink, 0);

    will_return(__wrap_FOREVER, NULL);

    OS_IntegratorD(integrator_config);

    os_free(wfd);
    os_free(unformatted);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_OS_IntegratorD, test_setup, test_teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
