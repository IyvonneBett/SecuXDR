/*
 * SQL Schema for global database
 * Copyright (C) 2015, Wazuh Inc.
 * June 30, 2016.
 * This program is a free software, you can redistribute it
 * and/or modify it under the terms of GPLv2.
 */

CREATE TABLE IF NOT EXISTS fim_entry (
    full_path TEXT NOT NULL PRIMARY KEY,
    file TEXT,
    type TEXT NOT NULL CHECK (type IN ('file', 'registry_key', 'registry_value')),
    date INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
    changes INTEGER NOT NULL DEFAULT 1,
    arch TEXT CHECK (arch IN (NULL, '[x64]', '[x32]')),
    value_name TEXT,
    value_type TEXT,
    size INTEGER,
    perm TEXT,
    uid TEXT,
    gid TEXT,
    md5 TEXT,
    sha1 TEXT,
    uname TEXT,
    gname TEXT,
    mtime INTEGER,
    inode INTEGER,
    sha256 TEXT,
    attributes TEXT,
    symbolic_path TEXT,
    checksum TEXT
);

CREATE INDEX IF NOT EXISTS fim_full_path_index ON fim_entry (full_path);
CREATE INDEX IF NOT EXISTS fim_file_index ON fim_entry (file);
CREATE INDEX IF NOT EXISTS fim_date_index ON fim_entry (date);

CREATE TABLE IF NOT EXISTS pm_event (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    date_first INTEGER,
    date_last INTEGER,
    log TEXT,
    pci_dss TEXT,
    cis TEXT
);

CREATE INDEX IF NOT EXISTS pm_event_log ON pm_event (log);
CREATE INDEX IF NOT EXISTS pm_event_date ON pm_event (date_last);

CREATE TABLE IF NOT EXISTS sys_netiface (
    scan_id INTEGER,
    scan_time TEXT,
    name TEXT,
    adapter TEXT,
    type TEXT,
    state TEXT,
    mtu INTEGER CHECK (mtu > 0),
    mac TEXT,
    tx_packets INTEGER,
    rx_packets INTEGER,
    tx_bytes INTEGER,
    rx_bytes INTEGER,
    tx_errors INTEGER,
    rx_errors INTEGER,
    tx_dropped INTEGER,
    rx_dropped INTEGER,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (scan_id, name)
);

CREATE INDEX IF NOT EXISTS netiface_id ON sys_netiface (scan_id);

CREATE TABLE IF NOT EXISTS sys_netproto (
    scan_id INTEGER REFERENCES sys_netiface (scan_id),
    iface TEXT REFERENCES sys_netiface (name),
    type TEXT,
    gateway TEXT,
    dhcp TEXT NOT NULL CHECK (dhcp IN ('enabled', 'disabled', 'unknown', 'BOOTP')) DEFAULT 'unknown',
    metric INTEGER,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (scan_id, iface, type)
);

CREATE INDEX IF NOT EXISTS netproto_id ON sys_netproto (scan_id);

CREATE TABLE IF NOT EXISTS sys_netaddr (
    scan_id INTEGER REFERENCES sys_netproto (scan_id),
    iface TEXT REFERENCES sys_netproto (iface),
    proto TEXT REFERENCES sys_netproto (type),
    address TEXT,
    netmask TEXT,
    broadcast TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (scan_id, iface, proto, address)
);

CREATE INDEX IF NOT EXISTS netaddr_id ON sys_netaddr (scan_id);

CREATE TABLE IF NOT EXISTS sys_osinfo (
    scan_id INTEGER,
    scan_time TEXT,
    hostname TEXT,
    architecture TEXT,
    os_name TEXT,
    os_version TEXT,
    os_codename TEXT,
    os_major TEXT,
    os_minor TEXT,
    os_patch TEXT,
    os_build TEXT,
    os_platform TEXT,
    sysname TEXT,
    release TEXT,
    version TEXT,
    os_release TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    os_display_version TEXT,
    triaged INTEGER(1) DEFAULT 0,
    reference TEXT NOT NULL DEFAULT '',
    PRIMARY KEY (scan_id, os_name)
);

CREATE TABLE IF NOT EXISTS sys_hwinfo (
    scan_id INTEGER,
    scan_time TEXT,
    board_serial TEXT,
    cpu_name TEXT,
    cpu_cores INTEGER CHECK (cpu_cores > 0),
    cpu_mhz REAL CHECK (cpu_mhz > 0),
    ram_total INTEGER CHECK (ram_total > 0),
    ram_free INTEGER CHECK (ram_free > 0),
    ram_usage INTEGER CHECK (ram_usage >= 0 AND ram_usage <= 100),
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    PRIMARY KEY (scan_id, board_serial)
);

CREATE TABLE IF NOT EXISTS sys_ports (
    scan_id INTEGER,
    scan_time TEXT,
    protocol TEXT,
    local_ip TEXT,
    local_port INTEGER CHECK (local_port >= 0),
    remote_ip TEXT,
    remote_port INTEGER CHECK (remote_port >= 0),
    tx_queue INTEGER,
    rx_queue INTEGER,
    inode INTEGER,
    state TEXT,
    PID INTEGER,
    process TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (protocol, local_ip, local_port, inode)
);

CREATE INDEX IF NOT EXISTS ports_id ON sys_ports (scan_id);

CREATE TABLE IF NOT EXISTS sys_programs (
    scan_id INTEGER,
    scan_time TEXT,
    format TEXT NOT NULL CHECK (format IN ('pacman', 'deb', 'rpm', 'win', 'pkg', 'apk')),
    name TEXT,
    priority TEXT,
    section TEXT,
    size INTEGER CHECK (size >= 0),
    vendor TEXT,
    install_time TEXT,
    version TEXT,
    architecture TEXT,
    multiarch TEXT,
    source TEXT,
    description TEXT,
    location TEXT,
    triaged INTEGER(1),
    cpe TEXT,
    msu_name TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    item_id TEXT,
    PRIMARY KEY (scan_id, name, version, architecture)
);

CREATE INDEX IF NOT EXISTS programs_id ON sys_programs (scan_id);

CREATE TRIGGER obsolete_vulnerabilities
    AFTER DELETE ON sys_programs
    WHEN (old.checksum = 'legacy' AND NOT EXISTS (SELECT 1 FROM sys_programs
                                                  WHERE item_id = old.item_id
                                                  AND scan_id != old.scan_id ))
    OR old.checksum != 'legacy'
    BEGIN
        UPDATE vuln_cves SET status = 'OBSOLETE' WHERE vuln_cves.reference = old.item_id;
END;

CREATE TABLE IF NOT EXISTS sys_hotfixes (
    scan_id INTEGER,
    scan_time TEXT,
    hotfix TEXT,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    PRIMARY KEY (scan_id, hotfix)
);

CREATE INDEX IF NOT EXISTS hotfix_id ON sys_hotfixes (scan_id);

CREATE TRIGGER hotfix_delete
    AFTER DELETE ON sys_hotfixes
    WHEN (old.checksum = 'legacy' AND NOT EXISTS (SELECT 1 FROM sys_hotfixes
                                                  WHERE hotfix = old.hotfix
                                                  AND scan_id != old.scan_id ))
    OR old.checksum != 'legacy'
    BEGIN
        UPDATE sys_osinfo SET triaged = 0;
END;

CREATE TRIGGER hotfix_insert
    AFTER INSERT ON sys_hotfixes
    WHEN (new.checksum = 'legacy' AND NOT EXISTS (SELECT 1 FROM sys_hotfixes
                                                  WHERE hotfix = new.hotfix
                                                  AND scan_id != new.scan_id ))
    OR new.checksum != 'legacy'
    BEGIN
        UPDATE sys_osinfo SET triaged = 0;
END;

CREATE TABLE IF NOT EXISTS sys_processes (
    scan_id INTEGER,
    scan_time TEXT,
    pid TEXT,
    name TEXT,
    state TEXT,
    ppid INTEGER,
    utime INTEGER,
    stime INTEGER,
    cmd TEXT,
    argvs TEXT,
    euser TEXT,
    ruser TEXT,
    suser TEXT,
    egroup TEXT,
    rgroup TEXT,
    sgroup TEXT,
    fgroup TEXT,
    priority INTEGER,
    nice INTEGER,
    size INTEGER,
    vm_size INTEGER,
    resident INTEGER,
    share INTEGER,
    start_time INTEGER,
    pgrp INTEGER,
    session INTEGER,
    nlwp INTEGER,
    tgid INTEGER,
    tty INTEGER,
    processor INTEGER,
    checksum TEXT NOT NULL CHECK (checksum <> ''),
    PRIMARY KEY (scan_id, pid)
);

CREATE INDEX IF NOT EXISTS processes_id ON sys_processes (scan_id);

CREATE TABLE IF NOT EXISTS ciscat_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    scan_id INTEGER,
    scan_time TEXT,
    benchmark TEXT,
    profile TEXT,
    pass INTEGER,
    fail INTEGER,
    error INTEGER,
    notchecked INTEGER,
    unknown INTEGER,
    score INTEGER
);

CREATE INDEX IF NOT EXISTS ciscat_id ON ciscat_results (scan_id);

CREATE TABLE IF NOT EXISTS metadata (
    key TEXT PRIMARY KEY,
    value TEXT
);

CREATE TABLE IF NOT EXISTS scan_info (
    module TEXT PRIMARY KEY,
    first_start INTEGER DEFAULT 0,
    first_end INTEGER DEFAULT 0,
    start_scan INTEGER DEFAULT 0,
    end_scan INTEGER DEFAULT 0,
    fim_first_check INTEGER DEFAULT 0,
    fim_second_check INTEGER DEFAULT 0,
    fim_third_check INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS sca_policy (
   name TEXT,
   file TEXT,
   id TEXT,
   description TEXT,
   `references` TEXT,
   hash_file TEXT
);

CREATE TABLE IF NOT EXISTS sca_scan_info (
   id INTEGER PRIMARY KEY,
   start_scan INTEGER,
   end_scan INTEGER,
   policy_id TEXT REFERENCES sca_policy (id),
   pass INTEGER,
   fail INTEGER,
   invalid INTEGER,
   total_checks INTEGER,
   score INTEGER,
   hash TEXT
);

CREATE TABLE IF NOT EXISTS sca_check (
   scan_id INTEGER REFERENCES sca_scan_info (id),
   id INTEGER PRIMARY KEY,
   policy_id TEXT REFERENCES sca_policy (id),
   title TEXT,
   description TEXT,
   rationale TEXT,
   remediation TEXT,
   file TEXT,
   process TEXT,
   directory TEXT,
   registry TEXT,
   command TEXT,
   `references` TEXT,
   result TEXT,
   reason TEXT,
   condition TEXT
);

CREATE INDEX IF NOT EXISTS policy_id_index ON sca_check (policy_id);

CREATE TABLE IF NOT EXISTS sca_check_rules (
  id_check INTEGER REFERENCES sca_check (id),
  `type` TEXT,
  rule TEXT,
  PRIMARY KEY (id_check, `type`, rule)
);

CREATE INDEX IF NOT EXISTS rules_id_check_index ON sca_check_rules (id_check);

CREATE TABLE IF NOT EXISTS sca_check_compliance (
   id_check INTEGER REFERENCES sca_check (id),
  `key` TEXT,
  `value` TEXT,
   PRIMARY KEY (id_check, `key`, `value`)
);

CREATE INDEX IF NOT EXISTS comp_id_check_index ON sca_check_compliance (id_check);

CREATE TABLE IF NOT EXISTS vuln_metadata (
    LAST_PARTIAL_SCAN INTEGER,
    LAST_FULL_SCAN INTEGER
);

INSERT INTO vuln_metadata (LAST_PARTIAL_SCAN, LAST_FULL_SCAN) VALUES (0, 0);

CREATE TABLE IF NOT EXISTS sync_info (
    component TEXT PRIMARY KEY,
    last_attempt INTEGER DEFAULT 0,
    last_completion INTEGER DEFAULT 0,
    n_attempts INTEGER DEFAULT 0,
    n_completions INTEGER DEFAULT 0,
    last_manager_checksum TEXT NOT NULL DEFAULT '',
    last_agent_checksum TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS vuln_cves (
    name TEXT,
    version TEXT,
    architecture TEXT,
    cve TEXT,
    detection_time TEXT DEFAULT '',
    severity TEXT DEFAULT 'Untriaged' CHECK (severity IN ('Critical', 'High', 'Medium', 'Low', 'None', 'Untriaged')),
    cvss2_score REAL DEFAULT 0,
    cvss3_score REAL DEFAULT 0,
    reference TEXT DEFAULT '' NOT NULL,
    type TEXT DEFAULT '' NOT NULL CHECK (type IN ('OS', 'PACKAGE')),
    status TEXT DEFAULT 'PENDING' NOT NULL CHECK (status IN ('VALID', 'PENDING', 'OBSOLETE')),
    external_references TEXT DEFAULT '',
    condition TEXT DEFAULT '',
    title TEXT DEFAULT '',
    published TEXT '',
    updated TEXT '',
    PRIMARY KEY (reference, cve)
);
CREATE INDEX IF NOT EXISTS packages_id ON vuln_cves (name);
CREATE INDEX IF NOT EXISTS cves_id ON vuln_cves (cve);
CREATE INDEX IF NOT EXISTS cve_type ON vuln_cves (type);
CREATE INDEX IF NOT EXISTS cve_status ON vuln_cves (status);

BEGIN;

INSERT INTO metadata (key, value) VALUES ('db_version', '11');
INSERT INTO scan_info (module) VALUES ('fim');
INSERT INTO scan_info (module) VALUES ('syscollector');
INSERT INTO sync_info (component) VALUES ('fim');
INSERT INTO sync_info (component) VALUES ('fim_file');
INSERT INTO sync_info (component) VALUES ('fim_registry');
INSERT INTO sync_info (component) VALUES ('fim_registry_key');
INSERT INTO sync_info (component) VALUES ('fim_registry_value');
INSERT INTO sync_info (component) VALUES ('syscollector-processes');
INSERT INTO sync_info (component) VALUES ('syscollector-packages');
INSERT INTO sync_info (component) VALUES ('syscollector-hotfixes');
INSERT INTO sync_info (component) VALUES ('syscollector-ports');
INSERT INTO sync_info (component) VALUES ('syscollector-netproto');
INSERT INTO sync_info (component) VALUES ('syscollector-netaddress');
INSERT INTO sync_info (component) VALUES ('syscollector-netinfo');
INSERT INTO sync_info (component) VALUES ('syscollector-hwinfo');
INSERT INTO sync_info (component) VALUES ('syscollector-osinfo');

COMMIT;
