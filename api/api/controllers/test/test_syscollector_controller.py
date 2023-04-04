import sys
from unittest.mock import ANY, AsyncMock, MagicMock, patch

import pytest
from aiohttp import web_response
from api.controllers.test.utils import CustomAffectedItems

with patch('wazuh.common.wazuh_uid'):
    with patch('wazuh.common.wazuh_gid'):
        sys.modules['wazuh.rbac.orm'] = MagicMock()
        import wazuh.rbac.decorators
        from api.controllers.syscollector_controller import (
            get_hardware_info, get_hotfix_info, get_network_address_info,
            get_network_interface_info, get_network_protocol_info, get_os_info,
            get_packages_info, get_ports_info, get_processes_info)
        from wazuh import syscollector
        from wazuh.tests.util import RBAC_bypasser
        wazuh.rbac.decorators.expose_resources = RBAC_bypasser
        del sys.modules['wazuh.rbac.orm']


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_hardware_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_hardware_info' endpoint is working as expected."""
    result = await get_hardware_info(request=mock_request,
                                     agent_id='001')
    f_kwargs = {'agent_list': ['001'],
                'select': None,
                'element_type': 'hardware'
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_hotfix_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_hotfix_info' endpoint is working as expected."""
    result = await get_hotfix_info(request=mock_request,
                                   agent_id='001')
    filters = {'hotfix': None}
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'hotfixes',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_network_address_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_network_address_info' endpoint is working as expected."""
    result = await get_network_address_info(request=mock_request,
                                            agent_id='001')
    filters = {'iface': None,
               'proto': None,
               'address': None,
               'broadcast': None,
               'netmask': None
               }
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'netaddr',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_network_interface_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_network_interface_info' endpoint is working as expected."""
    result = await get_network_interface_info(request=mock_request,
                                              agent_id='001')
    filters = {'adapter': None,
               'type': mock_request.query.get('type', None),
               'state': None,
               'name': None,
               'mtu': None
               }
    nested = ['tx.packets', 'rx.packets', 'tx.bytes', 'rx.bytes', 'tx.errors', 'rx.errors', 'tx.dropped', 'rx.dropped']
    for field in nested:
        filters[field] = mock_request.query.get(field, None)
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'netiface',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_network_protocol_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_network_protocol_info' endpoint is working as expected."""
    result = await get_network_protocol_info(request=mock_request,
                                             agent_id='001')
    filters = {'iface': None,
               'type': mock_request.query.get('type', None),
               'gateway': None,
               'dhcp': None
               }
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'netproto',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_os_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_os_info' endpoint is working as expected."""
    result = await get_os_info(request=mock_request,
                               agent_id='001')
    f_kwargs = {'agent_list': ['001'],
                'select': None,
                'element_type': 'os'
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_packages_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_packages_info' endpoint is working as expected."""
    result = await get_packages_info(request=mock_request,
                                     agent_id='001')
    filters = {'vendor': None,
               'name': None,
               'architecture': None,
               'format': mock_request.query.get('format', None),
               'version': None
               }
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'packages',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_ports_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_ports_info' endpoint is working as expected."""
    result = await get_ports_info(request=mock_request,
                                  agent_id='001')
    filters = {'pid': None,
               'protocol': None,
               'tx_queue': None,
               'state': None,
               'process': None
               }
    nested = ['local.ip', 'local.port', 'remote.ip']
    for field in nested:
        filters[field] = mock_request.query.get(field, None)
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'ports',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)


@pytest.mark.asyncio
@patch('api.controllers.syscollector_controller.DistributedAPI.distribute_function', return_value=AsyncMock())
@patch('api.controllers.syscollector_controller.remove_nones_to_dict')
@patch('api.controllers.syscollector_controller.DistributedAPI.__init__', return_value=None)
@patch('api.controllers.syscollector_controller.raise_if_exc', return_value=CustomAffectedItems())
async def test_get_processes_info(mock_exc, mock_dapi, mock_remove, mock_dfunc, mock_request=MagicMock()):
    """Verify 'get_processes_info' endpoint is working as expected."""
    result = await get_processes_info(request=mock_request,
                                      agent_id='001')
    filters = {'state': None,
               'pid': None,
               'ppid': None,
               'egroup': None,
               'euser': None,
               'fgroup': None,
               'name': None,
               'nlwp': None,
               'pgrp': None,
               'priority': None,
               'rgroup': None,
               'ruser': None,
               'sgroup': None,
               'suser': None
               }
    f_kwargs = {'agent_list': ['001'],
                'offset': 0,
                'limit': None,
                'select': None,
                'sort': None,
                'search': None,
                'filters': filters,
                'element_type': 'processes',
                'q': None
                }
    mock_dapi.assert_called_once_with(f=syscollector.get_item_agent,
                                      f_kwargs=mock_remove.return_value,
                                      request_type='distributed_master',
                                      is_async=False,
                                      wait_for_complete=False,
                                      logger=ANY,
                                      rbac_permissions=mock_request['token_info']['rbac_policies']
                                      )
    mock_exc.assert_called_once_with(mock_dfunc.return_value)
    mock_remove.assert_called_once_with(f_kwargs)
    assert isinstance(result, web_response.Response)
