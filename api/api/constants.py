# Copyright (C) 2015, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import os

from wazuh.core import common

API_PATH = os.path.join(common.WAZUH_PATH, 'api')
CONFIG_PATH = os.path.join(API_PATH, 'configuration')
CONFIG_FILE_PATH = os.path.join(CONFIG_PATH, 'api.yaml')
RELATIVE_CONFIG_FILE_PATH = os.path.relpath(CONFIG_FILE_PATH, common.WAZUH_PATH)
SECURITY_PATH = os.path.join(CONFIG_PATH, 'security')
SECURITY_CONFIG_PATH = os.path.join(SECURITY_PATH, 'security.yaml')
RELATIVE_SECURITY_PATH = os.path.relpath(SECURITY_PATH, common.WAZUH_PATH)
API_LOG_PATH = os.path.join(common.WAZUH_PATH, 'logs', 'api')
API_SSL_PATH = os.path.join(CONFIG_PATH, 'ssl')
