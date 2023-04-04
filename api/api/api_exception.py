# Copyright (C) 2015, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

from api.constants import RELATIVE_CONFIG_FILE_PATH, RELATIVE_SECURITY_PATH
from wazuh.core.exception import DOCU_VERSION


class APIException(Exception):
    """
    Wazuh API exception class.
    """

    def __init__(self, code: int, details: str = None):
        """APIException class constructor.

        Parameters
        ----------
        code : int
            Error code.
        details : str
            Extra details to add to the default exception message to include useful context information.
        """
        self.code = code
        self.details = details
        # show relative paths in exceptions
        self.exceptions = {
            2000: 'Some parameters are not expected in the configuration file '
                  f"(WAZUH_PATH/{RELATIVE_CONFIG_FILE_PATH}). Please check the documentation for further details: "
                  f"https://documentation.wazuh.com/{DOCU_VERSION}/user-manual/api/configuration.html"
                  '#api-configuration-options',
            2001: 'Error creating or reading secrets file. Please, ensure '
                  'there is enough disk space and permission to write in '
                  f'WAZUH_PATH/{RELATIVE_SECURITY_PATH}',
            2002: 'Error migrating configuration from old API version. '
                  'Default configuration will be applied',
            2003: 'Error loading SSL/TLS certificates',
            2004: 'Configuration file could not be loaded',
            2005: 'Body request is not a valid JSON',
            2006: 'Error parsing body request to UTF-8',
            2007: 'Body is empty',
            2008: 'Experimental features are disabled. '
                  'It can be changed in the API configuration',
            2009: 'Semicolon (;) is a reserved character and must '
                  'be percent-encoded (%3B) to use it.',
            2010: 'Error while attempting to bind on address: address already in use',
            2011: 'Error setting up API logger',
            2012: 'Error while attempting to check RBAC database integrity'
        }

    def __str__(self) -> str:
        """Magic method str().

        Returns
        -------
        str
            String representation of the APIException object.
        """
        details = '.' if self.details is None else f': {self.details}.'
        return f"{self.code} - {self.exceptions[self.code]}{details}"


class APIError(APIException):
    pass
