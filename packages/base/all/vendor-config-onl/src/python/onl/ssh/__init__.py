#!/usr/bin/python
############################################################
#
# ONL SSH Management
#
############################################################
import sys
import os
import re
import argparse
import logging
import shutil
import tempfile
from onl.mounts import OnlMountManager, OnlMountContextReadOnly, OnlMountContextReadWrite
from onl.util import *



class OnlSSHContextReadWrite(OnlMountContextReadWrite):
    def __init__(self, logger):
        OnlMountContextReadWrite.__init__(self, "ONL-CONFIG", logger)

class OnlSSHContextReadOnly(OnlMountContextReadOnly):
    def __init__(self, logger):
        OnlMountContextReadOnly.__init__(self, "ONL-CONFIG", logger)


class OnlSSH(OnlServiceMixin):
    """Initialize the ONL-CONFIG::SSH credentials."""

    CONFIG_SSH_DIR="/mnt/onl/config/ssh"

    def __init__(self, logger=None):
        self.logger = logger

        if self.logger is None:
            self.logger = logging.getLogger("ONL:SSH")

    def init(self, force=False):
        hostkeys = {}

        with OnlSSHContextReadOnly(self.logger):
            for fname in os.listdir("/etc/ssh"):
                fname = os.path.join("/etc/ssh", fname)

                if not os.path.islink(fname):
                    continue
                fname = os.path.realpath(fname)

                pattern = '^' + self.CONFIG_SSH_DIR + '/ssh_host_((\w+)_)?key$'
                m = re.match(pattern, fname)
                if not m:
                    continue

                keytype = m.group(2)
                if not keytype:
                    keytype = "rsa1"

                hostkeys[keytype] = fname

        if not hostkeys:
            return

        with OnlSSHContextReadWrite(self.logger):
            if not os.path.isdir(self.CONFIG_SSH_DIR):
                os.makedirs(self.CONFIG_SSH_DIR)

            for keytype in hostkeys:
                fname = hostkeys[keytype]

                if not force and os.path.exists(fname):
                    self.logger.info("Using existing SSH host key type %s.", keytype)
                    continue

                self.logger.info("Generating SSH host key type %s", keytype)

                # yes(1) is needed for "Overwrite (y/n)?"
                cmd = "yes 2>/dev/null | ssh-keygen -q -t '%s' -N '' -f '%s'" \
                    % (keytype, fname)

                self._execute(cmd, logLevel=logging.INFO)

    @staticmethod
    def main():
        ap = argparse.ArgumentParser(description="ONL SSH Management")
        ap.add_argument("--init",          action='store_true', help="Initialize SSH files (if necessary)")
        ap.add_argument("--force",   "-f", action='store_true', help="Force regeneration of the host keys during initialization (--init)")
        ap.add_argument("--quiet",   "-q", action='store_true', help="Quiet output.")
        ap.add_argument("--verbose", "-v", action='store_true', help="Verbose output.")

        ops = ap.parse_args()

        logging.basicConfig()
        logger = logging.getLogger("SSH")

        if ops.verbose:
            logger.setLevel(logging.DEBUG)
        elif ops.quiet:
            logger.setLevel(logging.WARNING)
        else:
            logger.setLevel(logging.INFO)

        ssh = OnlSSH(logger)

        if ops.init:
            ssh.init(force=ops.force)
