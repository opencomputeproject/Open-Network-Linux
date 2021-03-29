#!/usr/bin/python2
############################################################
#
# ONL Root Filesystem Generator
#
############################################################
import argparse
import os
import sys
import logging
import yaml
import tempfile
import shutil
import pprint
import fcntl
import subprocess
import glob
import submodules
import StringIO
from collections import Iterable
import onlyaml
import onlu
import fileinput
import crypt
import string
import random
import re
import json

logger = onlu.init_logging('onlrfs')

class OnlRfsError(Exception):
    """General Error Exception"""
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)




class OnlRfsSystemAdmin(object):

    def __init__(self, chroot):
        self.chroot = chroot

    @staticmethod
    def gen_salt():
        # return an eight character salt value
        salt_map = './' + string.digits + string.ascii_uppercase + \
            string.ascii_lowercase
        rand = random.SystemRandom()
        salt = ''
        for i in range(0, 8):
            salt += salt_map[rand.randint(0, 63)]
        return salt

    @staticmethod
    def chmod(mode, file_):
        onlu.execute("sudo chmod %s %s" % (mode, file_),
                     ex=OnlRfsError("Could not change permissions (%s) on file %s" % (mode, file_)))

    @staticmethod
    def chown(file_, ownspec):
        onlu.execute("sudo chown %s %s" % (ownspec, file_),
                     ex=OnlRfsError("Could not change ownership (%s) on file %s" % (ownspec, file_)))

    def userdel(self, username):
        pf = os.path.join(self.chroot, 'etc/passwd')
        sf = os.path.join(self.chroot, 'etc/shadow')

        self.chmod("a+rwx", os.path.dirname(pf))
        self.chmod("a+rw", pf);
        self.chmod("a+rw", sf);

        # Can't use the userdel command because of potential uid 0 in-user problems while running ourselves
        for line in fileinput.input(pf, inplace=True):
            if not line.startswith('%s:' % username):
                print line,
        for line in fileinput.input(sf, inplace=True):
            if not line.startswith('%s:' % username):
                print line,

        self.chmod("go-wx", pf);
        self.chmod("go-wx", sf);

    def groupadd(self, group, gid=None, unique=True, system=False, force=False, password=None):
        args = [ 'groupadd' ]
        if force:
            args.append("--force")
        if system:
            args.append("--system")
        if not unique:
            args.append("--non-unique")
        if password:
            args = args + [ '--password', password ]
        if gid:
            args = args + [ '--gid', str(gid) ]

        args.append(group)

        onlu.execute(args,
                     chroot=self.chroot,
                     ex=OnlRfsError("Adding group  '%s' failed." % group))

        logger.info("added group %s", group)

    def useradd(self, username, uid=None, gid=None, password=None, shell='/bin/bash', home=None, groups=None, sudo=False, deleteFirst=True):
        args = [ 'useradd', '--create-home' ]

        if uid is not None:
            args = args + [ '--non-unique', '--uid', str(uid) ]

        if gid is not None:
            args = args + [ '--gid', str(gid) ]

        if password:
            epassword=crypt.crypt(password, '$1$%s$' % self.gen_salt());
            args = args + ['-p', epassword ]

        if shell:
            args = args + [ '--shell', shell ]

        if home:
            args = args + [ '--home', home ]

        if groups:
            args = args + [ '--groups', ','.join(groups) ]

        if deleteFirst:
            self.userdel(username)

        args.append(username)

        onlu.execute(args,
                     chroot=self.chroot,
                     ex=OnlRfsError("Adding user '%s' failed." % username))

        if password is None:
            onlu.execute("passwd -d %s" % username,
                         chroot=self.chroot,
                         ex=OnlRfsError("Error deleting password for user '%s'" % username))

        logger.info("user %s password %s", username, password)

        if sudo:
            sudoer = os.path.join(self.chroot, 'etc/sudoers.d', username)
            self.chmod("777", os.path.dirname(sudoer))
            with open(sudoer, "w") as f:
                f.write("%s ALL=(ALL:ALL) NOPASSWD:ALL\n" % username);
            self.chmod("0440", sudoer)
            self.chown(sudoer, "root:root")
            self.chmod("755", os.path.dirname(sudoer))

    def user_password_set(self, username, password):
        logger.info("user %s password now %s", username, password)
        epassword=crypt.crypt(password, '$1$%s$' % self.gen_salt());
        onlu.execute(['usermod', '-p', epassword, username],
                     chroot=self.chroot,
                     ex=OnlRfsError("Error setting password for user '%s'" % username))

    def user_shell_set(self, username, shell):
        onlu.execute('chsh --shell %s %s' % (shell, username), chroot=self.chroot,
                     ex=OnlRfsError("Error setting shell %s for user %s" % (shell, username)))

    def user_disable(self, username):
        self.user_shell_set(username, '/bin/false')



class OnlMultistrapConfig(object):
    def __init__(self, config):
        self.config = config
        self.__validate()

    def __validate_key(self, d, k, type_, required=False):
        pass

    def __validate(self):
        """Validate the contents of our configuration."""

        # There must be a General section.
        general = self.config.get('General', None)

        if general is None:
            raise OnlRfsError("Section 'General' is missing from the multistrap configuration.")

        self.__validate_key(general, 'arch', str, True)
        self.__validate_key(general, 'debootstrap', str, True)

        for entry in [ 'debootstrap', 'aptsources' ]:
            sectionlist = []
            for e in general[entry].split():
                if e not in self.config:
                    raise OnlRfsError("Section '%s' is specified in the %s option but does not exist in the configuration." % (e, entry))

                if self.config[e].get('arches', None) and general['arch'] not in self.config[e]['arches']:
                    del self.config[e]
                else:
                    sectionlist.append(e)

            general[entry] = " ".join(sectionlist)

        self.localrepos = []

    def generate_handle(self, handle):
        for (name, fields) in self.config.iteritems():
            handle.write("[%s]\n" % name)
            for (k,v) in fields.iteritems():

                if type(v) is bool:
                    v = 'true' if v == True else 'false'
                if type(v) is list:
                    v = " ".join(onlu.sflatten(v))

                if k == 'source' and os.path.exists(v):
                    self.localrepos.append(v)
                    v = "copy:%s ./" % v

                if k == 'packages' and type(v) is list:
                    raise OnlRfsError("packages=%s" % v)

                handle.write("%s=%s\n" % (k, v))
            handle.write("\n")

    def generate_file(self, fname=None):
        if fname is None:
            h = tempfile.NamedTemporaryFile(delete=False)
            fname = h.name
        elif fname == '-' or fname == 'stdout':
            h = sys.stdout
        else:
            h = open(fname, "w")
        self.generate_handle(h)
        return fname

    def get_packages(self):
        pkgs = []
        for (name, fields) in self.config.iteritems():
            for (k,v) in fields.iteritems():
                if k == 'packages':
                    if type(v) is list:
                        pkgs = pkgs + list(onlu.sflatten(v))
                    else:
                        pkgs = pkgs + v.split()
        return pkgs

    def __str__(self):
        handle = StringIO.StringIO()
        self.generate_handle(handle)
        return handle.getvalue()



class OnlRfsContext(object):
    def __init__(self, directory, resolvconf=True):
        self.directory = directory
        self.dev = os.path.join(self.directory, "dev")
        self.proc = os.path.join(self.directory, "proc")
        self.rc = resolvconf
        if self.rc:
            self.resolvconf = os.path.join(self.directory, "etc", "resolv.conf")
            self.resolvconfb = "%s.onlrfs" % (self.resolvconf)

    def exists(self, fname):
        return os.path.islink(fname) or os.path.exists(fname)

    def __enter__(self):
        try:
            onlu.execute("sudo mount -t devtmpfs dev %s" % self.dev,
                         ex=OnlRfsError("Could not mount dev in rfs."))
            onlu.execute("sudo mount -t proc proc %s" % self.proc,
                         ex=OnlRfsError("Could not mount proc in rfs."))

            if self.rc:
                if self.exists(self.resolvconf):
                    onlu.execute("sudo mv %s %s" % (self.resolvconf, self.resolvconfb),
                                 ex=OnlRfsError("Could not backup resolv.conf"))

                onlu.execute("sudo cp --remove-destination /etc/resolv.conf %s" % (self.resolvconf),
                             ex=OnlRfsError("Could install new resolv.conf"))
            return self

        except Exception, e:
            logger.error("Exception %s in OnlRfsContext::__enter__" % e)
            self.__exit__(None, None, None)
            raise e

    def __exit__(self, eType, eValue, eTrace):
        onlu.execute("sudo umount -l %s %s" % (self.dev, self.proc),
                     ex=OnlRfsError("Could not unmount dev and proc"))

        if self.rc:
            onlu.execute("sudo rm %s" % (self.resolvconf),
                         ex=OnlRfsError("Could not remove new resolv.conf"))
            if self.exists(self.resolvconfb):
                onlu.execute("sudo mv %s %s" % (self.resolvconfb, self.resolvconf),
                             ex=OnlRfsError("Could not restore resolv.conf"))

class OnlRfsBuilder(object):

    DEFAULTS = dict(
        DEBIAN_SUITE='wheezy',
        DEBIAN_MIRROR='mirrors.kernel.org/debian/',
        APT_CACHE='127.0.0.1:3142/'
        )

    MULTISTRAP='/usr/sbin/multistrap'
    QEMU_PPC='/usr/bin/qemu-ppc-static'
    QEMU_ARM='/usr/bin/qemu-arm-static'
    QEMU_ARM64='/usr/bin/qemu-aarch64-static'
    BINFMT_PPC='/proc/sys/fs/binfmt_misc/qemu-ppc'

    def __init__(self, config, arch, **kwargs):
        self.kwargs = kwargs
        self.arch = arch
        self.kwargs['ARCH'] = arch

        # Hack -- we have to pull powerpc from the archive
        # This will need a cleaner fix.
        if arch == 'powerpc':
            self.DEFAULTS['DEBIAN_MIRROR'] = 'archive.debian.org/debian/'

        self.kwargs.update(self.DEFAULTS)
        self.__load(config)
        self.__validate()

    def __load(self, config):
        if not os.path.exists(config):
            raise OnlRfsError("Configuration file '%s' does not exist." % config)
        self.config = onlyaml.loadf(config, self.kwargs)

    def __validate(self):

        if not os.path.exists(self.MULTISTRAP):
            raise OnlRfsError("%s is missing." % self.MULTISTRAP)

        if self.arch == 'powerpc':
            if not os.path.exists(self.QEMU_PPC):
                raise OnlRfsError("%s is missing." % self.QEMU_PPC)

        if self.arch in [ 'armel', 'armhf' ]:
            if not os.path.exists(self.QEMU_ARM):
                raise OnlRfsError("%s is missing." % self.QEMU_ARM)

        if self.arch == 'arm64':
            if not os.path.exists(self.QEMU_ARM64):
                raise OnlRfsError("%s is missing." % self.QEMU_ARM64)

        if not 'Multistrap' in self.config:
            raise OnlRfsError("The Multistrap configuration section is missing.")

        self.ms = OnlMultistrapConfig(self.config['Multistrap'])


    def get_packages(self):
        return self.ms.get_packages()

    def msconfig(self, fname):
        return self.ms.generate_file(fname)

    def multistrap(self, dir_):
        msconfig = self.ms.generate_file()

        # Optional local package updates
        if os.getenv("ONLRFS_NO_PACKAGE_SCAN") is None:
            for r in self.ms.localrepos:
                logger.info("Updating %s" % r)
                if os.path.exists(os.path.join(r, 'Makefile')):
                    onlu.execute("make -C %s" % r)

        if os.path.exists(dir_):
            onlu.execute("sudo rm -rf %s" % dir_,
                         ex=OnlRfsError("Could not remove target directory."))

        if onlu.execute("sudo %s -d %s -f %s" % (self.MULTISTRAP, dir_, msconfig)) == 100:
            raise OnlRfsError("Multistrap APT failure.")

        if os.getenv("MULTISTRAP_DEBUG"):
            raise OnlRfsError("Multistrap debug.")


    def dpkg_configure(self, dir_):
        if self.arch == 'powerpc':
            onlu.execute('sudo cp %s %s' % (self.QEMU_PPC, os.path.join(dir_, 'usr/bin')))
        if self.arch in [ 'armel', 'armhf' ]:
            onlu.execute('sudo cp %s %s' % (self.QEMU_ARM, os.path.join(dir_, 'usr/bin')))
        if self.arch == 'arm64':
            onlu.execute('sudo cp %s %s' % (self.QEMU_ARM64, os.path.join(dir_, 'usr/bin')))

        onlu.execute('sudo cp %s %s' % (os.path.join(os.getenv('ONL'), 'tools', 'scripts', 'base-files.postinst'),
                                        os.path.join(dir_, 'var', 'lib', 'dpkg', 'info', 'base-files.postinst')));

        script = os.path.join(dir_, "tmp/configure.sh")
        with open(script, "w") as f:
            os.chmod(script, 0700)
            f.write("""#!/bin/bash -ex
/bin/echo -e "#!/bin/sh\\nexit 101" >/usr/sbin/policy-rc.d
chmod +x /usr/sbin/policy-rc.d
export DEBIAN_FRONTEND=noninteractive
export DEBCONF_NONINTERACTIVE_SEEN=true
echo "127.0.0.1 localhost" >/etc/hosts
touch /etc/fstab
echo "localhost" >/etc/hostname
if [ -f /var/lib/dpkg/info/dash.preinst ]; then
    /var/lib/dpkg/info/dash.preinst install
fi
if [ -f /usr/sbin/locale-gen ]; then
    echo "en_US.UTF-8 UTF-8" >/etc/locale.gen
    /usr/sbin/locale-gen
    update-locale LANG=en_US.UTF-8
fi

dpkg --configure -a || true
dpkg --configure -a # configure any packages that failed the first time and abort on failure.

rm -f /usr/sbin/policy-rc.d

# Replace generated host keys with symlinks to ONL-CONFIG.
# Host keys are individual for each host, generated at boot
# when missing and preserved on reinstall and upgrade.
for f in /etc/ssh/ssh_host_*key*; do
    ln -snf "/mnt/onl/config/ssh/${f##*/}" "$f"
done
    """)

        logger.info("dpkg-configure filesystem...")

        onlu.execute("sudo chroot %s /tmp/configure.sh" % dir_,
                     ex=OnlRfsError("Post Configuration failed."))
        os.unlink(script)



    def configure(self, dir_):

        if not os.getenv('NO_DPKG_CONFIGURE'):
            with OnlRfsContext(dir_, resolvconf=False):
                self.dpkg_configure(dir_)

        with OnlRfsContext(dir_):
            os_release = os.path.join(dir_, 'etc', 'os-release')
            os_release_dict = {}
            if os.path.exists(os_release):
                # Convert /etc/os-release to /etc/os-release.json
                import shlex
                contents = open(os_release).read()
                os_release_dict = dict(token.split('=') for token in shlex.split(contents))
                ua = OnlRfsSystemAdmin(dir_)
                ua.chmod('a+rwx', os.path.dirname(os_release))
                with open(os.path.join(os.path.dirname(os_release), 'os-release.json'), "w") as f:
                    f.write(json.dumps(os_release_dict))
                ua.chmod('0755', os.path.dirname(os_release))

            Configure = self.config.get('Configure', None)
            if Configure:

                for cmd in Configure.get('run', []):
                    onlu.execute("sudo chroot %s %s" % (dir_, cmd),
                                 ex=OnlRfsError("run command '%s' failed" % cmd))

                for overlay in Configure.get('overlays', []):
                    logger.info("Overlay %s..." % overlay)
                    onlu.execute('tar -C %s -c --exclude "*~" . | sudo tar -C %s -x -v --no-same-owner' % (overlay, dir_),
                                 ex=OnlRfsError("Overlay '%s' failed." % overlay))

                for update in Configure.get('update-rc.d', []):
                    onlu.execute("sudo chroot %s /usr/sbin/update-rc.d %s" % (dir_, update),
                                 ex=OnlRfsError("update-rc.d %s failed." % (update)))

                for script in Configure.get('scripts', []):
                    logger.info("Configuration script %s..." % script)
                    onlu.execute("sudo %s %s" % (script, dir_),
                                 ex=OnlRfsError("script '%s' failed." % script))


                for command in Configure.get('commands', []):
                    if '__rfs__' in command:
                        command = command % dict(__rfs__=dir_)
                    logger.info("Configuration command '%s'..." % command)
                    onlu.execute(command,
                                 ex=OnlRfsError("Command '%s' failed." % command))


                ua = OnlRfsSystemAdmin(dir_)
                for (group, values) in Configure.get('groups', {}).iteritems():
                    ua.groupadd(group=group, **values if values else {})

                for (user, values) in Configure.get('users', {}).iteritems():
                    if user == 'root':
                        if 'password' in values:
                            ua.user_password_set(user, values['password'])
                    else:
                        ua.useradd(username=user, **values)


                options = Configure.get('options', {})
                if options.get('clean', False):
                    logger.info("Cleaning Filesystem...")
                    onlu.execute('sudo chroot %s /usr/bin/apt-get clean' % dir_)
                    onlu.execute('sudo chroot %s /usr/sbin/localepurge' % dir_ )
                    onlu.execute('sudo chroot %s find /usr/share/doc -type f -not -name asr.json -delete' % dir_)
                    onlu.execute('sudo chroot %s find /usr/share/man -type f -delete' % dir_)

                if 'PermitRootLogin' in options:
                    config = os.path.join(dir_, 'etc/ssh/sshd_config')
                    ua.chmod('a+rw', config)
                    lines = open(config).readlines()
                    with open(config, "w") as f:
                        for line in lines:
                            if line.startswith('PermitRootLogin'):
                                v = options['PermitRootLogin']
                                logger.info("Setting PermitRootLogin to %s" % v)
                                f.write('PermitRootLogin %s\n' % v)
                            else:
                                f.write(line)
                    ua.chmod('644', config)

                if not options.get('securetty', True):
                    f = os.path.join(dir_, 'etc/securetty')
                    if os.path.exists(f):
                        logger.info("Removing %s" % f)
                        onlu.execute('sudo rm %s' % f,
                                     ex=OnlRfsError('Could not remove file %s' % f))

                if os.path.exists(os.path.join(dir_, 'etc/inittab')):
                    if not options.get('ttys', False):
                        f = os.path.join(dir_, 'etc/inittab')
                        ua.chmod('a+w', f)
                        ua.chmod('a+w', os.path.dirname(f))

                        logger.info("Clearing %s ttys..." % f)
                        for line in fileinput.input(f, inplace=True):
                            if re.match("^[123456]:.*", line):
                               line = "#" + line
                            print line,

                        ua.chmod('go-w', f)
                        ua.chmod('go-w', os.path.dirname(f))

                    if options.get('console', True):
                        logger.info('Configuring Console Access in %s' % f)
                        f = os.path.join(dir_, 'etc/inittab')
                        ua.chmod('a+w', f)
                        ua.chmod('a+w', os.path.dirname(f))
                        with open(f, 'a') as h:
                            h.write("T0:23:respawn:/sbin/pgetty\n")
                        ua.chmod('go-w', f)
                        ua.chmod('go-w', os.path.dirname(f))

                if options.get('asr', None):
                    asropts = options.get('asr')
                    logger.info("Gathering ASR documentation...")
                    sys.path.append("%s/sm/infra/tools" % os.getenv('ONL'))
                    import asr
                    asro = asr.AimSyslogReference()
                    asro.merge(dir_)
                    asrf = os.path.join(dir_, asropts['file'])
                    OnlRfsSystemAdmin.chmod('777', os.path.dirname(asrf))
                    asro.format(os.path.join(dir_, asropts['file']), fmt=asropts['format'])

                for (mf, fields) in Configure.get('manifests', {}).iteritems():
                    logger.info("Configuring manifest %s..." % mf)
                    if mf.startswith('/'):
                        mf = mf[1:]
                    mname = os.path.join(dir_, mf)
                    onlu.execute("sudo mkdir -p %s" % os.path.dirname(mname))
                    onlu.execute("sudo touch %s" % mname)
                    onlu.execute("sudo chmod a+w %s" % mname)
                    md = {}
                    md['version'] = json.load(open(fields['version']))
                    md['arch'] = self.arch
                    md['os-release'] = os_release_dict

                    if os.path.exists(fields['platforms']):
                        md['platforms'] = yaml.load(open(fields['platforms']))
                    else:
                        md['platforms'] = fields['platforms'].split(',')

                    for (k, v) in fields.get('keys', {}).iteritems():
                        if k in md:
                            md[k].update(v)
                        else:
                            md[k] = v

                    with open(mname, "w") as f:
                        json.dump(md, f, indent=2)
                    onlu.execute("sudo chmod a-w %s" % mname)

                for (fname, v) in Configure.get('files', {}).get('add', {}).iteritems():
                    if fname.startswith('/'):
                        fname = fname[1:]
                    dst = os.path.join(dir_, fname)
                    onlu.execute("sudo mkdir -p %s" % os.path.dirname(dst))
                    onlu.execute("sudo touch %s" % dst)
                    onlu.execute("sudo chmod a+w %s" % dst)
                    if os.path.exists(v):
                        shutil.copy(v, dst)
                    else:
                        with open(dst, "w") as f:
                            f.write("%s\n" % v)

                for fname in Configure.get('files', {}).get('remove', []):
                    if fname.startswith('/'):
                        fname = fname[1:]
                    f = os.path.join(dir_, fname)
                    if os.path.exists(f):
                        onlu.execute("sudo rm -rf %s" % f)

                if Configure.get('issue'):
                    issue = Configure.get('issue')
                    fn = os.path.join(dir_, "etc/issue")
                    onlu.execute("sudo chmod a+w %s" % fn)
                    with open(fn, "w") as f:
                        f.write("%s\n\n" % issue)
                    onlu.execute("sudo chmod a-w %s" % fn)

                    fn = os.path.join(dir_, "etc/issue.net")
                    onlu.execute("sudo chmod a+w %s" % fn)
                    with open(fn, "w") as f:
                        f.write("%s\n" % issue)
                    onlu.execute("sudo chmod a-w %s" % fn)


    def update(self, dir_, packages):

        ONLPM = "%s/tools/onlpm.py" % os.getenv('ONL')

        with OnlRfsContext(dir_):
            for pspec in packages:
                for pkg in pspec.split(','):
                    logger.info("updating %s into %s", pkg, dir_)
                    cmd = (ONLPM, '--verbose',
                           '--sudo',
                           '--extract-dir', pkg, dir_,)
                    onlu.execute(cmd,
                                 ex=OnlRfsError("update of %s failed" % pkg))

    def install(self, dir_, packages):

        ONLPM = "%s/tools/onlpm.py" % os.getenv('ONL')

        with OnlRfsContext(dir_):
            for pspec in packages:
                for pkg in pspec.split(','):

                    cmd = (ONLPM, '--lookup', pkg,)
                    try:
                        buf = subprocess.check_output(cmd)
                    except subprocess.CalledProcessError as ex:
                        logger.error("cannot find %s", pkg)
                        raise ValueError("update failed")

                    if not buf.strip():
                        raise ValueError("cannot find %s" % pkg)
                    src = buf.splitlines(False)[0]
                    d, b = os.path.split(src)
                    dst = os.path.join(dir_, "tmp", b)
                    shutil.copy2(src, dst)
                    src2 = os.path.join("/tmp", b)

                    logger.info("installing %s into %s", pkg, dir_)
                    cmd = ('/usr/bin/rfs-dpkg', '-i', src2,)
                    onlu.execute(cmd,
                                 chroot=dir_,
                                 ex=OnlRfsError("install of %s failed" % pkg))

                    name, _, _ = pkg.partition(':')
                    logger.info("updating dependencies for %s", pkg)
                    cmd = ('/usr/bin/rfs-apt-get', '-f', 'install', name,)
                    onlu.execute(cmd,
                                 chroot=dir_,
                                 ex=OnlRfsError("install of %s failed" % pkg))


if __name__ == '__main__':

    ap = argparse.ArgumentParser(description="ONL Root Filesystem Generator")
    ap.add_argument("--arch", required=True)
    ap.add_argument("--config", required=True)
    ap.add_argument("--dir")
    ap.add_argument("--show-packages", action='store_true')
    ap.add_argument("--no-build-packages", action='store_true')
    ap.add_argument("--only-build-packages", action='store_true')
    ap.add_argument("--msconfig")
    ap.add_argument("--multistrap-only", action='store_true')
    ap.add_argument("--no-multistrap", action='store_true')
    ap.add_argument("--cpio")
    ap.add_argument("--squash")
    ap.add_argument("--enable-root")

    ap.add_argument("--no-configure", action='store_true')
    ap.add_argument("--update", action='append')
    ap.add_argument("--install", action='append')

    ops = ap.parse_args()

    if ops.enable_root:
        #
        # Fixme -- this should all be rearranged to naturally support
        # arbitrary filesystem modifications.
        #
        sa = OnlRfsSystemAdmin(ops.dir)
        sa.user_password_set('root', ops.enable_root)
        config = os.path.join(ops.dir, 'etc/ssh/sshd_config')
        sa.chmod('a+rw', config)
        lines = open(config).readlines()
        with open(config, "w") as f:
            for line in lines:
                if line.startswith('PermitRootLogin'):
                    v = "Yes"
                    logger.info("Setting PermitRootLogin to %s" % v)
                    f.write('PermitRootLogin %s\n' % v)
                else:
                    f.write(line)
        sa.chmod('644', config)
        sys.exit(0)

    try:
        x = OnlRfsBuilder(ops.config, ops.arch)

        if ops.msconfig:
            x.msconfig(ops.msconfig)
            sys.exit(0)

        if ops.show_packages:
            print "\n".join(x.get_packages())
            sys.exit(0)

        if ops.dir is None:
            raise OnlRfsError("argument --dir is required")

        if not ops.no_build_packages:
            pkgs = x.get_packages()
            # Invoke onlpm to build all required (local) packages.
            onlu.execute("%s/tools/onlpm.py --try-arches %s all --skip-missing --require %s" % (os.getenv('ONL'), ops.arch, " ".join(pkgs)),
                         ex=OnlRfsError("Failed to build all required packages."))
            if ops.only_build_packages:
                sys.exit(0)

        if ops.multistrap_only:
            x.multistrap(ops.dir)
            sys.exit(0)

        if not ops.no_multistrap and not os.getenv('NO_MULTISTRAP'):
            x.multistrap(ops.dir)

        if not ops.no_configure and not os.getenv('NO_DPKG_CONFIGURE'):
            x.configure(ops.dir)

        if ops.update:
            x.update(ops.dir, ops.update)

        if ops.install:
            x.install(ops.dir, ops.install)

        if ops.cpio:
            if onlu.execute("%s/tools/scripts/make-cpio.sh %s %s" % (os.getenv('ONL'), ops.dir, ops.cpio)) != 0:
                raise OnlRfsError("cpio creation failed.")

        if ops.squash:
            if os.path.exists(ops.squash):
                os.unlink(ops.squash)
            if onlu.execute("sudo mksquashfs %s %s -no-progress -noappend -comp gzip" % (ops.dir, ops.squash)) != 0:
                if os.path.exists(ops.squash):
                    os.unlink(ops.squash)
                raise OnlRfsError("Squash creation failed.")

    except (OnlRfsError, onlyaml.OnlYamlError), e:
        logger.error(e.value)
