# Copyright 2019 Google, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import datetime
import logging
import os
import socket

from m5.defines import buildEnv
import _m5.arm_fast_model

ARM_LICENSE_ENV = "ARMLMD_LICENSE_FILE"
LM_LICENSE_ENV = "LM_LICENSE_FILE"


def set_armlmd_license_file(force=False):
    """Set the ARMLMD_LICENSE_FILE environment variable. If "force" is
    False, then it will only be set if it wasn't already set in the
    environment. The value it's set to is the one gem5 was built with.
    """
    license_file = buildEnv[ARM_LICENSE_ENV]
    if force or ARM_LICENSE_ENV not in os.environ:
        os.environ[ARM_LICENSE_ENV] = license_file


def check_armlmd_server(server, timeout):
    """Check if the "server" passed as parameter is available. server
    can also be a license file"""
    if os.path.exists(server):
        logging.debug(f"License file {server} exists.")
        return True

    tuple = server.split("@")
    if len(tuple) != 2:
        # Probably not a server, and we know the file doesn't exist.
        logging.debug(f'License file "{server}" does not exist.')
        return False

    try:
        start = datetime.datetime.now()
        # Try to connect to license server. This doesn't attempt to
        # communicate with it, just checking reachability.
        s = socket.create_connection(
            (tuple[1], int(tuple[0])), timeout=timeout
        )
        end = datetime.datetime.now()
        s.close()
        time = end - start
        logging.info(
            f"License server {server} is reachable ({time.total_seconds()} seconds)."
        )
        return True
    except Exception as e:
        logging.debug(
            f"Cannot connect to license server {server} ({type(e).__name__}: {e})."
        )
        return False


def check_armlmd_license(timeout, tries):
    """Check if any of the provided license server can be reached, or
    if a license file is provided. This allows to fail early and fast,
    as fastmodel code makes multiple lengthy attempts to connect to
    license server. "timeout" is in seconds. Makes "retries" attempt to
    connect.
    """
    servers = os.environ[ARM_LICENSE_ENV].split(":")

    extras = list()
    # Add LM_LICENSE_ENV to the list, if set.
    if LM_LICENSE_ENV in os.environ and os.environ[LM_LICENSE_ENV]:
        extras += os.environ[LM_LICENSE_ENV].split(":")
    # Fastmodel appears to always add this file.
    extras.append("/opt/arm/licenses/license.dat")
    for extra in extras:
        if extra not in servers:
            servers.append(extra)

    for try_count in range(1, tries + 1):
        for server in servers:
            if check_armlmd_server(server, timeout):
                return
        if try_count == tries:
            raise ConnectionError(
                f"Cannot connect to any of the license servers ({', '.join(servers)})."
            )
        # retry
        logging.warning(
            "Cannot connect to any of the license servers, retrying..."
        )


# These methods wrap much of the SystemC Export API described in section
# 7.6 of the Fast Models User Guide.


def scx_initialize(id):
    # Actually run scx_initialize.
    _m5.arm_fast_model.scx_initialize(id)


def scx_load_application(instance, application):
    _m5.arm_fast_model.scx_load_application(instance, application)


def scx_load_application_all(application):
    _m5.arm_fast_model.scx_load_application_all(application)


def scx_load_data(instance, data, address):
    _m5.arm_fast_model.scx_load_data(instance, data, address)


def scx_load_data_all(data, address):
    _m5.arm_fast_model.scx_load_data_all(data, address)


def scx_set_parameter(name, value):
    _m5.arm_fast_model.scx_set_parameter(name, value)


def scx_get_parameter(name):
    value = ""
    _m5.arm_fast_model.scx_get_parameter(name, value)
    return value


def scx_get_parameter_list():
    return _m5.arm_fast_model.scx_get_parameter_list()


def scx_set_cpi_file(cpi_file_path):
    _m5.arm_fast_model.scx_set_cpi_file(cpi_file_path)


def scx_cpulimit(t):
    _m5.arm_fast_model.scx_cpulimit(t)


def scx_timelimit(t):
    _m5.arm_fast_model.scx_timelimit(t)


def scx_simlimit(t):
    _m5.arm_fast_model.scx_simlimit(t)


def scx_parse_and_configure(self, argc, argv, trailer=None, sig_handler=True):
    _m5.arm_fast_model.scx_parse_and_configure(
        argc, argv, trailer, sig_handler
    )


def scx_start_cadi_server(start=True, run=True, debug=False):
    _m5.arm_fast_model.scx_start_cadi_server(start, run, debug)


def scx_enable_cadi_log(log=True):
    _m5.arm_fast_model.scx_enable_cadi_log(log)


def scx_prefix_appli_output(prefix=True):
    _m5.arm_fast_model.scx_prefix_appli_output(prefix)


def scx_print_port_number(print_=True):
    _m5.arm_fast_model.scx_print_port_number(print_)


def scx_print_statistics(print_=True):
    _m5.arm_fast_model.scx_print_statistics(print_)


def scx_load_plugin(file_):
    _m5.arm_fast_model.scx_load_plugin(file_)


def scx_sync(sync_time):
    _m5.arm_fast_model.scx_sync(sync_time)


def scx_set_min_sync_latency(latency):
    _m5.arm_fast_model.scx_set_min_sync_latency(latency)


def scx_get_min_sync_latency(arg=None):
    if arg:
        return _m5.arm_fast_model.scx_get_min_sync_latency(arg)
    else:
        return _m5.arm_fast_model.scx_get_min_sync_latency()


# This should be called once per simulation
def setup_simulation(
    sim_name,
    min_sync_latency=100.0 / 100000000,
    exit_on_dmi_warning=False,
    license_precheck=False,
    license_precheck_timeout=1,
    license_precheck_tries=3,
):
    set_armlmd_license_file()
    if license_precheck:
        check_armlmd_license(license_precheck_timeout, license_precheck_tries)
    scx_initialize(sim_name)
    scx_set_min_sync_latency(min_sync_latency)
    if exit_on_dmi_warning:
        _m5.arm_fast_model.gem5.enable_exit_on_dmi_warning_handler()
