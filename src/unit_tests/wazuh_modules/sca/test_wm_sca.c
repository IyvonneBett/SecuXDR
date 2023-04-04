/*
 * Copyright (C) 2015, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 *
 * Test corresponding to the scheduling capacities
 * for SCA Module
 * */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <time.h>
#include <stdlib.h>

#include "shared.h"
#include "wazuh_modules/wmodules.h"
#include "../scheduling/wmodules_scheduling_helpers.h"

#include "../../wrappers/common.h"
#include "../../wrappers/posix/dirent_wrappers.h"
#include "../../wrappers/wazuh/shared/debug_op_wrappers.h"
#include "../../wrappers/wazuh/shared/file_op_wrappers.h"
#include "../../wrappers/wazuh/shared/mq_op_wrappers.h"
#include "../../wrappers/wazuh/shared/pthreads_op_wrappers.h"
#include "../../wrappers/wazuh/shared/validate_op_wrappers.h"
#include "../../wrappers/wazuh/wazuh_modules/wmodules_wrappers.h"
#include "../../wrappers/wazuh/wazuh_modules/wm_exec_wrappers.h"

#define TEST_MAX_DATES 3

static wmodule *sca_module;
static OS_XML *lxml;
extern int test_mode;

extern void wm_sca_send_policies_scanned(wm_sca_t * data);

extern w_queue_t * request_queue;
extern char **last_sha256;
extern OSHash **cis_db;
extern struct cis_db_hash_info_t *cis_db_for_hash;
extern unsigned int policies_count;

void wm_sca_send_policies_scanned(wm_sca_t * data)
{
    // Will wrap this function to check running times in order to check scheduling
    return;
}

/******* Helpers **********/

static void wmodule_cleanup(wmodule *module){
    wm_sca_t* module_data = (wm_sca_t *)module->data;
    int i;
    for(i = 0; i < policies_count; i++) {
        os_free(module_data->policies[i]->policy_path);
        os_free(module_data->policies[i]);
    }
    os_free(module_data->alert_msg);
    os_free(module_data->policies);
    os_free(module_data);
    os_free(module->tag);
    os_free(module);
}

/***  SETUPS/TEARDOWNS  ******/
static int setup_module() {
    sca_module = calloc(1, sizeof(wmodule));
    const char *string =
        "<enabled>yes</enabled>\n"
        "<interval>12h</interval>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n";
    lxml = malloc(sizeof(OS_XML));

    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    XML_NODE nodes = string_to_xml_node(string, lxml);
    int ret = wm_sca_read(lxml, nodes, sca_module);
    OS_ClearNode(nodes);
    test_mode = 1;

    return ret;
}

static int teardown_module(){
    test_mode = 0;
    wm_sca_t* module_data = (wm_sca_t *)sca_module->data;
    wmodule_cleanup(sca_module);
    OS_ClearXML(lxml);
    return 0;
}

static int setup_test_executions(void **state) {
    wm_max_eps = 1;
    return 0;
}

static int teardown_test_executions(void **state) {
    wm_sca_t* module_data = (wm_sca_t *)sca_module->data;
    sched_scan_free(&(module_data->scan_config));
    int i;
    for(i = 0; module_data->policies[i]; i++) {
        os_free(last_sha256[i]);
        OSHash_Free(cis_db[i]);
        os_free(cis_db_for_hash[i].elem);
    }
    queue_free(request_queue);
    return 0;
}

static int setup_test_read(void **state) {
    test_structure *test = calloc(1, sizeof(test_structure));
    test->module =  calloc(1, sizeof(wmodule));
    *state = test;
    return 0;
}

static int teardown_test_read(void **state) {
    test_structure *test = *state;
    OS_ClearNode(test->nodes);
    OS_ClearXML(&(test->xml));
    wmodule *module = test->module;
    wm_sca_t* module_data = (wm_sca_t *)module->data;
    sched_scan_free(&(module_data->scan_config));
    wmodule_cleanup(module);
    os_free(test);
    return 0;
}

/****************************************************************/


/** Tests **/
void test_interval_execution(void **state) {
    wm_sca_t* module_data = (wm_sca_t *)sca_module->data;
    *state = module_data;
    module_data->scan_config.next_scheduled_scan_time = 0;
    module_data->scan_config.scan_day = 0;
    module_data->scan_config.scan_wday = -1;
    module_data->scan_config.interval = 60; // 1min
    module_data->scan_config.month_interval = false;

    expect_string(__wrap_StartMQ, path, DEFAULTQUEUE);
    expect_value(__wrap_StartMQ, type, WRITE);
    will_return(__wrap_StartMQ, 0);

    will_return_count(__wrap_FOREVER, 1, TEST_MAX_DATES);
    will_return(__wrap_FOREVER, 0);
    expect_any_always(__wrap__mtinfo, tag);
    expect_any_always(__wrap__mtinfo, formatted_msg);
    expect_any_always(__wrap__mtwarn, tag);
    expect_any_always(__wrap__mtwarn, formatted_msg);

    sca_module->context->start(module_data);
}

void test_fake_tag(void **state) {
    const char *string =
        "<enabled>yes</enabled>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<time>03:30</time>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n"
        "<fake>invalid</fake>";
    test_structure *test = *state;
    test->nodes = string_to_xml_node(string, &(test->xml));

    expect_string(__wrap__mterror, tag, "sca");
    expect_string(__wrap__mterror, formatted_msg, "No such tag 'fake' at module 'sca'.");
    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    assert_int_equal(wm_sca_read(&(test->xml), test->nodes, test->module),-1);
}

void test_read_scheduling_monthday_configuration(void **state) {
    const char *string =
        "<enabled>yes</enabled>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<day>7</day>\n"
        "<time>03:30</time>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n";
    test_structure *test = *state;

    expect_string(__wrap__mwarn, formatted_msg, "Interval must be a multiple of one month. New interval value: 1M");
    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    test->nodes = string_to_xml_node(string, &(test->xml));
    assert_int_equal(wm_sca_read(&(test->xml), test->nodes, test->module),0);
    wm_sca_t* module_data = (wm_sca_t *)test->module->data;
    assert_int_equal(module_data->scan_config.scan_day, 7);
    assert_int_equal(module_data->scan_config.interval, 1);
    assert_int_equal(module_data->scan_config.month_interval, true);
    assert_int_equal(module_data->scan_config.scan_wday, -1);
    assert_string_equal(module_data->scan_config.scan_time, "03:30");
}

void test_read_scheduling_weekday_configuration(void **state) {
    const char *string =
        "<enabled>yes</enabled>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<wday>Monday</wday>\n"
        "<time>04:30</time>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n";
    test_structure *test = *state;

    expect_string(__wrap__mwarn, formatted_msg, "Interval must be a multiple of one week. New interval value: 1w");
    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    test->nodes = string_to_xml_node(string, &(test->xml));
    assert_int_equal(wm_sca_read(&(test->xml), test->nodes, test->module),0);
    wm_sca_t* module_data = (wm_sca_t *)test->module->data;
    assert_int_equal(module_data->scan_config.scan_day, 0);
    assert_int_equal(module_data->scan_config.interval, 604800);
    assert_int_equal(module_data->scan_config.month_interval, false);
    assert_int_equal(module_data->scan_config.scan_wday, 1);
    assert_string_equal(module_data->scan_config.scan_time, "04:30");
}

void test_read_scheduling_daytime_configuration(void **state) {
    const char *string =
        "<enabled>yes</enabled>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<time>05:30</time>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n";
    test_structure *test = *state;

    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    test->nodes = string_to_xml_node(string, &(test->xml));
    assert_int_equal(wm_sca_read(&(test->xml), test->nodes, test->module),0);
    wm_sca_t* module_data = (wm_sca_t *)test->module->data;
    assert_int_equal(module_data->scan_config.scan_day, 0);
    assert_int_equal(module_data->scan_config.interval, WM_DEF_INTERVAL);
    assert_int_equal(module_data->scan_config.month_interval, false);
    assert_int_equal(module_data->scan_config.scan_wday, -1);
    assert_string_equal(module_data->scan_config.scan_time, "05:30");
}

void test_read_scheduling_interval_configuration(void **state) {
    const char *string =
        "<enabled>yes</enabled>\n"
        "<scan_on_start>no</scan_on_start>\n"
        "<interval>2h</interval>\n"
        "<policies>\n"
        "    <policy>/var/ossec/etc/shared/your_policy_file.yml</policy>\n"
        "</policies>\n";
    test_structure *test = *state;

    will_return(__wrap_opendir, 0);
    expect_string(__wrap__mtinfo, tag, "sca");
    expect_any(__wrap__mtinfo, formatted_msg);
    expect_string(__wrap_realpath, path, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_realpath, "/var/ossec/etc/shared/your_policy_file.yml");
    expect_string(__wrap_IsFile, file, "/var/ossec/etc/shared/your_policy_file.yml");
    will_return(__wrap_IsFile, 0);

    test->nodes = string_to_xml_node(string, &(test->xml));
    assert_int_equal(wm_sca_read(&(test->xml), test->nodes, test->module),0);
    wm_sca_t* module_data = (wm_sca_t *)test->module->data;
    assert_int_equal(module_data->scan_config.scan_day, 0);
    assert_int_equal(module_data->scan_config.interval, 7200);
    assert_int_equal(module_data->scan_config.month_interval, false);
    assert_int_equal(module_data->scan_config.scan_wday, -1);
}

/* wm_sort_variables tests */

void test_wm_sort_variables_null(void **state)
{
    char **ret;
    cJSON *variables_policy = NULL;

    ret = wm_sort_variables(variables_policy);
    assert_null(ret);
}

void test_wm_sort_variables_duplicated(void **state)
{
    char **ret;
    char *expected_ret[] = {"$system_root", "$system_root"};
    const char *variables_json_mock = "{\n \
        \"variables\": {\n \
            \"$system_root\": \"/var\",\n \
            \"$system_root\": \"/etc\"\n \
        }\n \
    }";

    cJSON *variables_list = cJSON_Parse(variables_json_mock);
    cJSON *variables_policy = cJSON_GetObjectItem(variables_list, "variables");
    ret = wm_sort_variables(variables_policy);

    for (int i = 0; ret[i]; i++) {
        assert_string_equal(ret[i], expected_ret[i]);
        os_free(ret[i]);
    }
    os_free(ret);
    cJSON_Delete(variables_list);
}

void test_wm_sort_variables(void **state)
{
    char **ret;
    char *expected_ret[] = {"$system_root_file", "$ssh_&_ssl_path", "$system_root", "$file"};
    const char *variables_json_mock = "{\n \
        \"variables\": {\n \
            \"$system_root\": \"/var\",\n \
            \"$file\": \"/\",\n \
            \"$ssh_&_ssl_path\": \"/new/directory\",\n \
            \"$system_root_file\": \"/var\"\n \
        }\n \
    }";

    cJSON *variables_list = cJSON_Parse(variables_json_mock);
    cJSON *variables_policy = cJSON_GetObjectItem(variables_list, "variables");
    ret = wm_sort_variables(variables_policy);

    for (int i = 0; ret[i]; i++) {
        assert_string_equal(ret[i], expected_ret[i]);
        os_free(ret[i]);
    }
    os_free(ret);
    cJSON_Delete(variables_list);
}

/* main */

int main(void) {
    const struct CMUnitTest tests_with_startup[] = {
        cmocka_unit_test_setup_teardown(test_interval_execution, setup_test_executions, teardown_test_executions)
    };
    const struct CMUnitTest tests_without_startup[] = {
        cmocka_unit_test_setup_teardown(test_fake_tag, setup_test_read, teardown_test_read),
        cmocka_unit_test_setup_teardown(test_read_scheduling_monthday_configuration, setup_test_read, teardown_test_read),
        cmocka_unit_test_setup_teardown(test_read_scheduling_weekday_configuration, setup_test_read, teardown_test_read),
        cmocka_unit_test_setup_teardown(test_read_scheduling_daytime_configuration, setup_test_read, teardown_test_read),
        cmocka_unit_test_setup_teardown(test_read_scheduling_interval_configuration, setup_test_read, teardown_test_read),
        cmocka_unit_test(test_wm_sort_variables_null),
        cmocka_unit_test(test_wm_sort_variables_duplicated),
        cmocka_unit_test(test_wm_sort_variables)
    };
    int result;
    result = cmocka_run_group_tests(tests_with_startup, setup_module, teardown_module);
    result += cmocka_run_group_tests(tests_without_startup, NULL, NULL);
    return result;
}
