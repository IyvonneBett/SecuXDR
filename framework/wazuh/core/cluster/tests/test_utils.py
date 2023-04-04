import logging
import os
import sys
from unittest.mock import patch, MagicMock, call

import pytest

with patch('wazuh.core.common.getgrnam'):
    with patch('wazuh.core.common.getpwnam'):
        with patch('wazuh.core.common.wazuh_uid'):
            with patch('wazuh.core.common.wazuh_gid'):
                sys.modules['wazuh.rbac.orm'] = MagicMock()

                from wazuh.core.cluster import utils
                from wazuh import WazuhError, WazuhException, WazuhInternalError
                from wazuh.core.results import WazuhResult

default_cluster_config = {
    'disabled': True,
    'node_type': 'master',
    'name': 'wazuh',
    'node_name': 'node01',
    'key': '',
    'port': 1516,
    'bind_addr': '0.0.0.0',
    'nodes': ['NODE_IP'],
    'hidden': 'no'
}


def test_read_cluster_config():
    """Verify that read_cluster function returns, in this case, the default configuration."""
    config = utils.read_cluster_config()
    assert config == default_cluster_config

    with patch('wazuh.core.cluster.utils.get_ossec_conf', side_effect=WazuhError(1001)):
        with pytest.raises(WazuhError, match='.* 3006 .*'):
            utils.read_cluster_config()

    with patch('wazuh.core.configuration.load_wazuh_xml', return_value=SystemExit):
        with pytest.raises(SystemExit) as pytest_wrapped_e:
            utils.read_cluster_config(from_import=True)
        assert pytest_wrapped_e.type == SystemExit
        assert pytest_wrapped_e.value.code == 0

    with patch('wazuh.core.cluster.utils.get_ossec_conf', side_effect=KeyError(1)):
        with pytest.raises(WazuhError, match='.* 3006 .*'):
            utils.read_cluster_config()

    with patch('wazuh.core.cluster.utils.get_ossec_conf', return_value={'cluster': default_cluster_config}):
        utils.read_config.cache_clear()
        default_cluster_config.pop('hidden')
        default_cluster_config['disabled'] = 'no'
        config = utils.read_cluster_config()
        config_simple = utils.read_config()
        assert config == config_simple
        assert config == default_cluster_config

        default_cluster_config['node_type'] = 'client'
        config = utils.read_cluster_config()
        assert config == default_cluster_config

        default_cluster_config['disabled'] = 'None'
        with pytest.raises(WazuhError, match='.* 3004 .*'):
            utils.read_cluster_config()

        default_cluster_config['disabled'] = 'yes'
        config = utils.read_cluster_config()
        assert config == default_cluster_config

        default_cluster_config['port'] = 'None'
        with pytest.raises(WazuhError, match='.* 3004 .*'):
            utils.read_cluster_config()


def test_get_manager_status():
    """Check that get_manager_status function returns the manager status.

    For this test, the status can be stopped or failed.
    """
    called = 0

    def exist_mock(path):
        if '.failed' in path and called == 0:
            return True
        elif '.restart' in path and called == 1:
            return True
        elif '.start' in path and called == 2:
            return True
        elif '/proc' in path and called == 3:
            return True
        else:
            return False

    status = utils.get_manager_status()
    for value in status.values():
        assert value == 'stopped'

    with patch('wazuh.core.cluster.utils.glob', return_value=['ossec-0.pid']):
        with patch('re.match', return_value='None'):
            status = utils.get_manager_status()
            for value in status.values():
                assert value == 'failed'

        # with patch('wazuh.core.cluster.utils.join', return_value='failed') as join_mock:
        with patch('wazuh.core.cluster.utils.os.path.exists', side_effect=exist_mock):
            status = utils.get_manager_status()
            for value in status.values():
                assert value == 'failed'

            called += 1
            status = utils.get_manager_status()
            for value in status.values():
                assert value == 'restarting'

            called += 1
            status = utils.get_manager_status()
            for value in status.values():
                assert value == 'starting'

            called += 1
            status = utils.get_manager_status()
            for value in status.values():
                assert value == 'running'

@pytest.mark.parametrize('exc', [
    PermissionError,
    FileNotFoundError
])
@patch('os.stat')
def test_get_manager_status_ko(mock_stat, exc):
    """Check that get_manager_status function correctly handles expected exceptions.

    Parameters
    ----------
    exc : Exception
        Expected exception to be handled.
    """
    mock_stat.side_effect = exc
    with pytest.raises(WazuhInternalError, match='.* 1913 .*'):
        utils.get_manager_status()


def test_get_cluster_status():
    """Check if cluster is enabled and running. Also check that cluster is shown as not running when a
    WazuhInternalError is raised."""
    status = utils.get_cluster_status()
    assert {'enabled': 'no', 'running': 'no'} == status

    with patch('wazuh.core.cluster.utils.get_manager_status', side_effect=WazuhInternalError(1913)):
        status = utils.get_cluster_status()
        assert {'enabled': 'no', 'running': 'no'} == status


def test_manager_restart():
    """Verify that manager_restart send to the manager the restart request."""
    with patch('wazuh.core.cluster.utils.open', side_effect=None):
        with patch('fcntl.lockf', side_effect=None):
            with pytest.raises(WazuhInternalError, match='.* 1901 .*'):
                utils.manager_restart()

            with patch('os.path.exists', return_value=True):
                with pytest.raises(WazuhInternalError, match='.* 1902 .*'):
                    utils.manager_restart()

                with patch('socket.socket.connect', side_effect=None):
                    with pytest.raises(WazuhInternalError, match='.* 1014 .*'):
                        utils.manager_restart()

                    with patch('socket.socket.send', side_effect=None):
                        status = utils.manager_restart()
                        assert WazuhResult({'message': 'Restart request sent'}) == status


def test_get_cluster_items():
    """Verify the cluster files information."""
    utils.get_cluster_items.cache_clear()

    with patch('os.path.abspath', side_effect=FileNotFoundError):
        with pytest.raises(WazuhException, match='.* 3005 .*'):
            utils.get_cluster_items()

    items = utils.get_cluster_items()
    assert items == {'files': {'etc/': {'permissions': 416, 'source': 'master', 'files': ['client.keys'],
                                        'recursive': False, 'restart': False, 'remove_subdirs_if_empty': False,
                                        'extra_valid': False, 'description': 'client keys file database'},
                               'etc/shared/': {'permissions': 432, 'source': 'master', 'files': ['all'],
                                               'recursive': True, 'restart': False, 'remove_subdirs_if_empty': True,
                                               'extra_valid': False, 'description': 'shared configuration files'},
                               'var/multigroups/': {'permissions': 432, 'source': 'master', 'files': ['merged.mg'],
                                                    'recursive': True, 'restart': False,
                                                    'remove_subdirs_if_empty': True, 'extra_valid': False,
                                                    'description': 'shared configuration files'},
                               'etc/rules/': {'permissions': 432, 'source': 'master', 'files': ['all'],
                                              'recursive': True, 'restart': True, 'remove_subdirs_if_empty': False,
                                              'extra_valid': False, 'description': 'user rules'},
                               'etc/decoders/': {'permissions': 432, 'source': 'master', 'files': ['all'],
                                                 'recursive': True, 'restart': True, 'remove_subdirs_if_empty': False,
                                                 'extra_valid': False, 'description': 'user decoders'},
                               'etc/lists/': {'permissions': 432, 'source': 'master', 'files': ['all'],
                                              'recursive': True, 'restart': True, 'remove_subdirs_if_empty': False,
                                              'extra_valid': False, 'description': 'user CDB lists'},
                               'excluded_files': ['ar.conf', 'ossec.conf'],
                               'excluded_extensions': ['~', '.tmp', '.lock', '.swp']},
                     'intervals': {'worker': {'sync_integrity': 9, 'sync_agent_info': 10, 'sync_agent_groups': 30,
                                              'keep_alive': 60, 'connection_retry': 10, 'timeout_agent_groups': 40,
                                              'max_failed_keepalive_attempts': 2, "agent_groups_mismatch_limit": 5},
                                   'master': {'timeout_extra_valid': 40, 'recalculate_integrity': 8,
                                              'check_worker_lastkeepalive': 60,
                                              'max_allowed_time_without_keepalive': 120, 'process_pool_size': 2,
                                              'sync_agent_groups': 10, 'timeout_agent_info': 40,
                                              'max_locked_integrity_time': 1000, 'agent_group_start_delay': 30},
                                   'communication': {'timeout_cluster_request': 20, 'timeout_dapi_request': 200,
                                                     'timeout_receiving_file': 120, 'min_zip_size': 31457280,
                                                     'max_zip_size': 1073741824, 'compress_level': 1,
                                                     'zip_limit_tolerance': 0.2}},
                     'distributed_api': {'enabled': True}}


def test_ClusterFilter():
    """Verify that ClusterFilter adds cluster related information into cluster logs"""
    cluster_filter = utils.ClusterFilter(tag='Cluster', subtag='config')
    record = utils.ClusterFilter(tag='Testing', subtag='config')
    record.update_tag(new_tag='Testing_tag')
    record.update_subtag(new_subtag='Testing_subtag')

    assert cluster_filter.filter(record=record)


def test_ClusterLogger():
    """Verify that ClusterLogger defines the logger used by wazuh-clusterd."""
    current_logger_path = os.path.join(os.path.dirname(__file__), 'testing.log')
    cluster_logger = utils.ClusterLogger(foreground_mode=False, log_path=current_logger_path,
                                         tag='%(asctime)s %(levelname)s: [%(tag)s] [%(subtag)s] %(message)s',
                                         debug_level=1)
    cluster_logger.setup_logger()

    assert cluster_logger.logger.level == logging.DEBUG

    os.path.exists(current_logger_path) and os.remove(current_logger_path)


@patch('os.getpid', return_value=0000)
@patch('wazuh.core.cluster.utils.pyDaemonModule.create_pid')
def test_process_spawn_sleep(pyDaemon_create_pid_mock, get_pid_mock):
    """Check if the cluster pool is properly spawned."""

    child = 1
    utils.process_spawn_sleep(child)

    pyDaemon_create_pid_mock.assert_called_once_with(f'wazuh-clusterd_child_{child}', get_pid_mock.return_value)


@pytest.mark.asyncio
@patch('concurrent.futures.ThreadPoolExecutor')
@patch('wazuh.core.cluster.dapi.dapi.DistributedAPI')
async def test_forward_function(distributed_api_mock, concurrent_mock):
    """Check if the function is correctly distributed to the master node."""

    class ThreadPoolExecutorMock:
        """Auxiliary class."""

        def submit(self, run, function):
            return DAPIMock()

    class DAPIMock:
        """Auxiliary class."""

        def __init__(self):
            pass

        def distribute_function(self):
            pass

        @staticmethod
        def result():
            return 'mock'

    def auxiliary_func():
        """Auxiliary function."""
        pass

    distributed_api_mock.return_value = DAPIMock()
    concurrent_mock.return_value = ThreadPoolExecutorMock()
    assert await utils.forward_function(auxiliary_func) == DAPIMock().result()
    distributed_api_mock.assert_called_once()
    concurrent_mock.assert_called_once()
