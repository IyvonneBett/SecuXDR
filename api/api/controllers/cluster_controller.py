# Copyright (C) 2015, Wazuh Inc.
# Created by Wazuh, Inc. <info@wazuh.com>.
# This program is a free software; you can redistribute it and/or modify it under the terms of GPLv2

import datetime
import logging
from typing import Union

from aiohttp import web
from connexion.lifecycle import ConnexionResponse

import wazuh.cluster as cluster
import wazuh.core.common as common
import wazuh.manager as manager
import wazuh.stats as stats
from api.encoder import dumps, prettify
from api.models.base_model_ import Body
from api.util import remove_nones_to_dict, parse_api_param, raise_if_exc, deserialize_date, deprecate_endpoint
from api.validator import check_component_configuration_pair
from wazuh.core.cluster.control import get_system_nodes
from wazuh.core.cluster.dapi.dapi import DistributedAPI
from wazuh.core.results import AffectedItemsWazuhResult

logger = logging.getLogger('wazuh-api')


async def get_cluster_node(request, pretty: bool = False, wait_for_complete: bool = False) -> web.Response:
    """Get basic information about the local node.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=cluster.get_node_wrapper,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_cluster_nodes(request, pretty: bool = False, wait_for_complete: bool = False, offset: int = 0,
                            limit: int = None, sort: str = None, search: str = None, select: str = None,
                            nodes_list: str = None, q: str = None) -> web.Response:
    """Get information about all nodes in the cluster or a list of them.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    offset : int
        First element to return in the collection.
    limit : int
        Maximum number of elements to return.
    select : list
        Select which fields to return (separated by comma).
    sort : str
        Sort the collection by a field or fields (separated by comma). Use +/- at the beginning
        to list in ascending or descending order.
    search : str
        Look for elements with the specified string.
    nodes_list : str
        List of node IDs.
    q : str
        Query to filter results by.

    Returns
    -------
    web.Response
        API response.
    """
    # Get type parameter from query
    type_ = request.query.get('type', 'all')

    f_kwargs = {'filter_node': nodes_list,
                'offset': offset,
                'limit': limit,
                'sort': parse_api_param(sort, 'sort'),
                'search': parse_api_param(search, 'search'),
                'select': select,
                'filter_type': type_,
                'q': q}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=cluster.get_nodes_info,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_master',
                          is_async=True,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          local_client_arg='lc',
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_healthcheck(request, pretty: bool = False, wait_for_complete: bool = False,
                          nodes_list: str = None) -> web.Response:
    """Get cluster healthcheck.

    Returns cluster healthcheck information for all nodes or a list of them. Such information includes last keep alive,
    last synchronization time and number of agents reporting on each node.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    nodes_list : str
        List of node IDs.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'filter_node': nodes_list}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=cluster.get_health_nodes,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_master',
                          is_async=True,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          local_client_arg='lc',
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_nodes_ruleset_sync_status(request, pretty: bool = False, wait_for_complete: bool = False,
                                        nodes_list: str = "*") -> web.Response:
    """Get cluster ruleset synchronization status.

    Returns cluster ruleset synchronization status for all nodes or a list of them.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    nodes_list : list
        Node IDs. Default: '*'

    Returns
    -------
    web.Response
        Nodes ruleset synchronization statuses.
    """
    nodes = raise_if_exc(await get_system_nodes())

    master_dapi = DistributedAPI(f=cluster.get_node_ruleset_integrity,
                                 request_type='local_master',
                                 is_async=True,
                                 wait_for_complete=wait_for_complete,
                                 logger=logger,
                                 local_client_arg='lc',
                                 )
    master_md5 = raise_if_exc(await master_dapi.distribute_function()).dikt

    f_kwargs = {'node_list': nodes_list, 'master_md5': master_md5}
    dapi = DistributedAPI(f=cluster.get_ruleset_sync_status,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type="distributed_master",
                          is_async=True,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          broadcasting=nodes_list == "*",
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_status(request, pretty: bool = False, wait_for_complete: bool = False) -> web.Response:
    """Get cluster status.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {}
    dapi = DistributedAPI(f=cluster.get_status_json,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies']
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_config(request, pretty: bool = False, wait_for_complete: bool = False) -> web.Response:
    """Get the current node cluster configuration.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=cluster.read_config_wrapper,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='local_any',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_status_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's Wazuh daemons status.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.get_status,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_info_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's information.

    Returns basic information about a specified node such as version, compilation date, installation path.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.get_basic_info,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_configuration_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False,
                                 section: str = None, field: str = None,
                                 raw: bool = False) -> Union[web.Response, ConnexionResponse]:
    """Get a specified node's configuration (ossec.conf).

    Parameters
    ----------
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format. It only works when `raw` is False (JSON format). Default `False`
    wait_for_complete : bool
        Disable response timeout or not. Default `False`
    section : str
        Indicates the wazuh configuration section.
    field : str
        Indicates a section child, e.g, fields for rule section are include, decoder_dir, etc.
    raw : bool, optional
        Whether to return the file content in raw or JSON format. Default `False`

    Returns
    -------
    web.Response or ConnexionResponse
        Depending on the `raw` parameter, it will return a web.Response object or a ConnexionResponse object:
            raw=True            -> ConnexionResponse (application/xml)
            raw=False (default) -> web.Response (application/json)
        If any exception was raised, it will return a web.Response with details.
    """
    f_kwargs = {'node_id': node_id,
                'section': section,
                'field': field,
                'raw': raw}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.read_ossec_conf,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    if isinstance(data, AffectedItemsWazuhResult):
        response = web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)
    else:
        response = ConnexionResponse(body=data["message"], mimetype='application/xml', content_type='application/xml')
    return response


async def get_daemon_stats_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False,
                                daemons_list: list = None):
    """Get Wazuh statistical information from the specified daemons of a specified cluster node.

    Parameters
    ----------
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    daemons_list : list
        List of the daemons to get statistical information from.
    """
    daemons_list = daemons_list or []
    f_kwargs = {'node_id': node_id,
                'daemons_list': daemons_list}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.get_daemons_stats,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes)
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_stats_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False,
                         date: str = None) -> web.Response:
    """Get a specified node's stats.

    Returns Wazuh statistical information in node {node_id} for the current or specified date.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    date : str
        Selects the date for getting the statistical information. Format YYYY-MM-DD.

    Returns
    -------
    web.Response
        API response.
    """
    if not date:
        date = datetime.datetime.today()
    else:
        date = deserialize_date(date)

    f_kwargs = {'node_id': node_id,
                'date': date}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.totals,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())
    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_stats_hourly_node(request, node_id: str, pretty: bool = False,
                                wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's stats by hour.

    Returns Wazuh statistical information in node {node_id} per hour. Each number in the averages field represents the
    average of alerts per hour.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.hourly,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_stats_weekly_node(request, node_id: str, pretty: bool = False,
                                wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's stats by week.

    Returns Wazuh statistical information in node {node_id} per week. Each number in the averages field represents the
    average of alerts per hour for that specific day.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.weekly,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


@deprecate_endpoint()
async def get_stats_analysisd_node(request, node_id: str, pretty: bool = False,
                                   wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's analysisd statistics.

    Notes
    -----
    To be deprecated in v5.0.

    Parameters
    ----------
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool, optional
        Whether to disable response timeout or not. Default `False`

    Returns
    -------
    web.Response
    """
    f_kwargs = {'node_id': node_id,
                'filename': common.ANALYSISD_STATS}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.deprecated_get_daemons_stats,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


@deprecate_endpoint()
async def get_stats_remoted_node(request, node_id: str, pretty: bool = False,
                                 wait_for_complete: bool = False) -> web.Response:
    """Get a specified node's remoted statistics.

    Notes
    -----
    To be deprecated in v5.0.

    Parameters
    ----------
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool, optional
        Whether to disable response timeout or not. Default `False`

    Returns
    -------
    web.Response
    """
    f_kwargs = {'node_id': node_id,
                'filename': common.REMOTED_STATS}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=stats.deprecated_get_daemons_stats,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_log_node(request, node_id: str, pretty: bool = False, wait_for_complete: bool = False, offset: int = 0,
                       limit: int = None, sort: str = None, search: str = None, tag: str = None, level: str = None,
                       q: str = None) -> web.Response:
    """Get a specified node's wazuh logs.

    Returns the last 2000 wazuh log entries in node {node_id}.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    offset : int
        First element to return in the collection.
    limit : int
        Maximum number of elements to return.
    sort : str
        Sort the collection by a field or fields (separated by comma). Use +/- at the beginning
        to list in ascending or descending order.
    search : str
        Look for elements with the specified string.
    tag : str
        Filters by category/tag of log.
    level : str
        Filters by log level.
    q : str
        Query to filter results by.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id,
                'offset': offset,
                'limit': limit,
                'sort_by': parse_api_param(sort, 'sort')['fields'] if sort is not None else ['timestamp'],
                'sort_ascending': False if sort is None or parse_api_param(sort, 'sort')['order'] == 'desc' else True,
                'search_text': parse_api_param(search, 'search')['value'] if search is not None else None,
                'complementary_search': parse_api_param(search, 'search')['negation'] if search is not None else None,
                'tag': tag,
                'level': level,
                'q': q}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.ossec_log,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_log_summary_node(request, node_id: str, pretty: bool = False,
                               wait_for_complete: bool = False) -> web.Response:
    """Get a summary of a specified node's wazuh logs.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Cluster node name.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.ossec_log_summary,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_api_config(request, pretty: bool = False, wait_for_complete: bool = False,
                         nodes_list: str = '*') -> web.Response:
    """Get active API configuration in manager or local_node.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    nodes_list : str
        List of node IDs.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_list': nodes_list}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.get_api_config,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          broadcasting=nodes_list == '*',
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def put_restart(request, pretty: bool = False, wait_for_complete: bool = False,
                      nodes_list: str = '*') -> web.Response:
    """Restarts all nodes in the cluster or a list of them.

    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    nodes_list : str
        List of node IDs.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_list': nodes_list}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.restart,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          broadcasting=nodes_list == '*',
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_conf_validation(request, pretty: bool = False, wait_for_complete: bool = False,
                              nodes_list: str = '*') -> web.Response:
    """Check whether the Wazuh configuration in a list of cluster nodes is correct or not.


    Parameters
    ----------
    request : connexion.request
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.
    nodes_list : str
        List of node IDs.

    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_list': nodes_list}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.validation,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          broadcasting=nodes_list == '*',
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def get_node_config(request, node_id: str, component: str, wait_for_complete: bool = False, pretty: bool = False,
                          **kwargs: dict) -> web.Response:
    """Get active configuration in node node_id [on demand]

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Node ID.
    component : str
        Filters by specified component.
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.


    Returns
    -------
    web.Response
        API response.
    """
    f_kwargs = {'node_id': node_id,
                'component': component,
                'config': kwargs.get('configuration', None)
                }

    nodes = raise_if_exc(await get_system_nodes())
    raise_if_exc(check_component_configuration_pair(f_kwargs['component'], f_kwargs['config']))

    dapi = DistributedAPI(f=manager.get_config,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)


async def update_configuration(request, node_id: str, body: dict, pretty: bool = False,
                               wait_for_complete: bool = False) -> web.Response:
    """Update Wazuh configuration (ossec.conf) in node node_id.

    Parameters
    ----------
    request : connexion.request
    node_id : str
        Node ID.
    body : dict
        New content for the Wazuh configuration (ossec.conf).
    pretty : bool
        Show results in human-readable format.
    wait_for_complete : bool
        Disable timeout response.

    Returns
    -------
    web.Response
        API response.
    """
    # Parse body to utf-8
    Body.validate_content_type(request, expected_content_type='application/octet-stream')
    parsed_body = Body.decode_body(body, unicode_error=1911, attribute_error=1912)

    f_kwargs = {'node_id': node_id,
                'new_conf': parsed_body}

    nodes = raise_if_exc(await get_system_nodes())
    dapi = DistributedAPI(f=manager.update_ossec_conf,
                          f_kwargs=remove_nones_to_dict(f_kwargs),
                          request_type='distributed_master',
                          is_async=False,
                          wait_for_complete=wait_for_complete,
                          logger=logger,
                          rbac_permissions=request['token_info']['rbac_policies'],
                          nodes=nodes
                          )
    data = raise_if_exc(await dapi.distribute_function())

    return web.json_response(data=data, status=200, dumps=prettify if pretty else dumps)
