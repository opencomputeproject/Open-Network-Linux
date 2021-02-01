#!/usr/bin/python
############################################################
# <bsn.cl fy=2013 v=none>
#
#        Copyright 2013, 2014 BigSwitch Networks, Inc.
#
#
#
# </bsn.cl>
############################################################

import pprint
import json
import os, sys
import re
import yaml
import onl.YamlUtils
import subprocess
import platform
import ast

class OnlInfoObject(object):
    DEFAULT_INDENT="    "

    def __init__(self, d, klass=None):
        self._data = d
        if klass:
            for (m,n) in klass.__dict__.iteritems():
                if m == m.upper():
                    setattr(self, m, None)

                for (k,v) in d.iteritems():
                    for (m,n) in klass.__dict__.iteritems():
                        if n == k:
                            setattr(self, m, v);
                            break

    def __getattr__(self, name):
        if name in self._data:
            return self._data[name]
        else:
            return None

    def __str__(self, indent=DEFAULT_INDENT):
        """String representation of the information container."""
        return OnlInfoObject.string(self._data, indent)

    def update(self, d):
        self._data.update(d)

    @staticmethod
    def string(d, indent=DEFAULT_INDENT):
        return "\n".join( sorted("%s%s: %s" % (indent,k,v) for k,v in d.iteritems() if not k.startswith('_') and d[k] is not None and k != 'CRC'))


############################################################
#
# System-specific information keys.
# These represent information about a particular box.
#
############################################################

class OnieInfo(object):
    PRODUCT_NAME='Product Name'
    PART_NUMBER='Part Number'
    SERIAL_NUMBER='Serial Number'
    MAC_ADDRESS='MAC'
    MAC_RANGE='MAC Range'
    MANUFACTURER='Manufacturer'
    MANUFACTURE_DATE='Manufacture Date'
    VENDOR='Vendor'
    PLATFORM_NAME='Platform Name'
    DEVICE_VERSION='Device Version'
    LABEL_REVISION='Label Revision'
    COUNTRY_CODE='Country Code'
    DIAG_VERSION='Diag Version'
    SERVICE_TAG='Service Tag'
    ONIE_VERSION='ONIE Version'
    oids = {
        PRODUCT_NAME         : '.1',
        PART_NUMBER          : '.2',
        SERIAL_NUMBER        : '.3',
        MAC_ADDRESS          : '.4',
        MAC_RANGE            : '.5',
        MANUFACTURER         : '.6',
        MANUFACTURE_DATE     : '.7',
        VENDOR               : '.8',
        PLATFORM_NAME        : '.9',
        DEVICE_VERSION       : '.10',
        LABEL_REVISION       : '.11',
        COUNTRY_CODE         : '.12',
        DIAG_VERSION         : '.13',
        SERVICE_TAG          : '.14',
        ONIE_VERSION         : '.15',
        }


class PlatformInfo(object):
    CPLD_VERSIONS='CPLD Versions'


############################################################
#
# ONL Platform Base
# Baseclass for all OnlPlatform objects.
#
############################################################
class OnlPlatformBase(object):

    CONFIG_DIR='/lib/platform-config'
    CURRENT_DIR=os.path.join(CONFIG_DIR, 'current')

    CONFIG_DEFAULT_GRUB = "/lib/vendor-config/onl/platform-config-defaults-x86-64.yml"
    CONFIG_DEFAULT_UBOOT = "/lib/vendor-config/onl/platform-config-defaults-uboot.yml"

    def __init__(self):
        self.add_info_json("onie_info", "%s/onie-info.json" % self.basedir_onl(), OnieInfo,
                           required=False)
        self.add_info_json("platform_info", "%s/platform-info.json" % self.basedir_onl(), PlatformInfo,
                           required=False)

        if hasattr(self, "platform_info"):
            self.platform_info.update(self.dmi_versions())
        else:
            self.add_info_dict("platform_info", self.dmi_versions())

        # Find the base platform config
        if self.platform().startswith('x86-64'):
            y1 = self.CONFIG_DEFAULT_GRUB
        elif self.platform().startswith('powerpc'):
            y1 = self.CONFIG_DEFAULT_UBOOT
        elif self.platform().startswith('arm'):
            y1 = self.CONFIG_DEFAULT_UBOOT
        else:
            y1 = None

        # Find and load the platform config yaml file
        y2 = os.path.join(self.basedir_onl(), "%s.yml" % self.platform())
        if os.path.exists(y1) and os.path.exists(y2):
            self.platform_config = onl.YamlUtils.merge(y1, y2)
            if self.platform() in self.platform_config:
                self.platform_config = self.platform_config[self.platform()]
        elif os.path.exists(y2):
            with open(y2) as fd:
                self.platform_config = yaml.load(fd)
            if self.platform() in self.platform_config:
                self.platform_config = self.platform_config[self.platform()]
        elif os.path.exists(y1):
            with open(y1) as fd:
                self.platform_config = yaml.load(fd)
            if 'default' in self.platform_config:
                self.platform_config = self.platform_config['default']
        else:
            self.platform_config = {}


    def add_info_dict(self, name, d, klass=None):
        setattr(self, name, OnlInfoObject(d, klass))

    def add_info_json(self, name, f, klass=None, required=True):
        if os.path.exists(f):
            try:
                d = json.load(file(f))
                self.add_info_dict(name, d, klass)
            except ValueError, e:
                if required:
                    raise e
                self.add_info_dict(name, {}, klass)
        elif required:
            raise RuntimeError("A required system file (%s) is missing." % f)



    def load_configs(self, reload=False):
        if reload or hasattr(self, 'configs') is False:
            self.configs = {}
            for subsys in os.listdir(self.basedir()):
                cpath = os.path.join(self.basedir(), subsys, "configs")
                if os.path.isdir(cpath):
                    for config in os.listdir(cpath):
                        with file(os.path.join(cpath, config)) as f:
                            if not subsys in self.configs:
                                self.configs[subsys] = {}
                            self.configs[subsys][config] = json.load(f)

    def basedir(self, *args):
        return os.path.join(self.CONFIG_DIR, self.platform(), *args)

    def basedir_onl(self, *args):
        return self.basedir('onl', *args)

    def baseconfig(self):
        return True

    def insmod(self, module, required=True, params={}):
        #
        # Search for modules in this order:
        #
        # 1. Fully qualified platform name
        #    /lib/modules/<kernel>/onl/<vendor>/<platform-name>
        # 2. Basename
        #    /lib/modules/<kernel>/onl/<vendor>/<basename>
        # 3. Vendor common
        #    /lib/modules/<kernel>/onl/<vendor>/common
        # 4. ONL common
        #    /lib/modules/<kernel>/onl/onl/common
        # 5. ONL Top-Level
        #    /lib/modules/<kernel>/onl
        # 5. Kernel Top-level
        #    /lib/modules/<kernel>
        #

        kdir = "/lib/modules/%s" % os.uname()[2]
        basename = "-".join(self.PLATFORM.split('-')[:-1])
        odir = "%s/onl" % kdir
        vdir = "%s/%s" % (odir, self.MANUFACTURER.lower())
        bdir = "%s/%s" % (vdir, basename)
        pdir = "%s/%s" % (vdir, self.PLATFORM)

        searchdirs = [ os.path.join(vdir, self.PLATFORM),
                       os.path.join(vdir, basename),
                       os.path.join(vdir, "common"),
                       os.path.join(odir, "onl", "common"),
                       odir,
                       kdir,
                       ]

        trypaths = []
        for d in searchdirs:
            for e in [ ".ko", "" ]:
                path = os.path.join(d, "%s%s" % (module, e))
                if os.path.exists(path):
                    cmd = "insmod %s %s" % (path, " ".join([ "%s=%s" % (k,v) for (k,v) in params.iteritems() ]))
                    subprocess.check_call(cmd, shell=True);
                    return True
                else:
                    trypaths.append(path)

        if required:
            raise RuntimeError("kernel module %s could not be found.\n The following paths were searched: \n    %s\n" % (module, "\n   ".join(trypaths)))
        else:
            return False

    def insmod_platform(self):
        kv = os.uname()[2]
        # Insert all modules in the platform module directories
        directories = [ self.PLATFORM,
                        '-'.join(self.PLATFORM.split('-')[:-1]) ]

        for subdir in directories:
            d = "/lib/modules/%s/onl/%s/%s" % (kv,
                                               self.MANUFACTURER.lower(),
                                               subdir)
            if os.path.isdir(d):
                for f in os.listdir(d):
                    if f.endswith(".ko"):
                        self.insmod(f)

    def onie_machine_get(self):
        mc = self.basedir_onl("etc/onie/machine.json")
        if not os.path.exists(mc):
            data = {}
            mcconf = subprocess.check_output("""onie-shell -c "IFS=; . /etc/machine*.conf; set | egrep ^onie_.*=" """, shell=True)
            for entry in mcconf.split():
                (k,e,v) = entry.partition('=')
                if v and (v.startswith("'") or v.startswith('"')):
                    v = ast.literal_eval(v)
                if e:
                    data[k] = v

            if not os.path.exists(os.path.dirname(mc)):
                os.makedirs(os.path.dirname(mc))

            with open(mc, "w") as f:
                f.write(json.dumps(data, indent=2))
        else:
            data = json.load(open(mc))

        return data

    ONIE_EEPROM_JSON='etc/onie/eeprom.json'

    def onie_syseeprom_get(self):
        se = self.basedir_onl(self.ONIE_EEPROM_JSON)
        if not os.path.exists(se):
            data = {}
            extensions = []
            syseeprom = subprocess.check_output("""onie-shell -c onie-syseeprom""", shell=True)
            e = re.compile(r'(.*?) (0x[0-9a-fA-F][0-9a-fA-F])[ ]+(\d+) (.*)')
            for line in syseeprom.split('\n'):
                m = e.match(line)
                if m:
                    value = m.groups(0)[3]
                    code = m.groups(0)[1].lower()
                    if code == '0xfd':
                        extensions.append(value)
                    else:
                        data[code] = value
            if len(extensions):
                data['0xfd'] = extensions

            self.onie_syseeprom_set(data)
        else:
            data = json.load(open(se))
        return data

    def onie_syseeprom_set(self, data):
        se = self.basedir_onl(self.ONIE_EEPROM_JSON)
        if not os.path.exists(os.path.dirname(se)):
            os.makedirs(os.path.dirname(se))

        with open(se, "w") as f:
            f.write(json.dumps(data, indent=2))


    def platform(self):
        return self.PLATFORM

    def baseplatform(self):
        p = self.platform()
        p = re.sub(r'-r\d$', '', p)
        return p

    def description(self):
        return "%s %s" % (self.MANUFACTURER, self.MODEL)

    def serialnumber(self):
        return self.onie_info.SERIAL_NUMBER

    def hw_description(self):
        return "%s (%s)" % (self.onie_info.PRODUCT_NAME,
                            self.onie_info.PART_NUMBER)


    # ONL Platform Information Tree
    def platform_info_oid(self):
        return "1.3.6.1.4.1.42623.1.1"

    # ONL Platform Information General Tree
    def platform_info_general_oid(self):
        return self.platform_info_oid() + ".1"

    # ONL Platform Information General Sys Tree
    def platform_info_general_sys_oid(self):
        return self.platform_info_general_oid() + ".1"

    # ONL Platform Information Vendor Tree
    def platform_info_vendor_oid(self):
        return self.platform_info_oid() + ".2"

    def sys_oid_platform(self):
        raise Exception("sys_oid_platform() is not set.")

    def sys_object_id(self):
        return "%s.%s%s" % (self.platform_info_vendor_oid(), self.PRIVATE_ENTERPRISE_NUMBER, self.SYS_OBJECT_ID)

    def onie_version(self):
        return self.onie_info.ONIE_VERSION

    def firmware_version(self):
        return self.platform_info.CPLD_VERSIONS

    def dmi_versions(self):
        rv = {}
        arches = [ 'x86_64' ]
        if platform.machine() in arches:
            try:
                import dmidecode
                fields = [
                    {
                        'name': 'DMI BIOS Version',
                        'subsystem': dmidecode.bios,
                        'dmi_type' : 0,
                        'key' : 'Version',
                    },

                    {
                        'name': 'DMI System Version',
                        'subsystem': dmidecode.system,
                        'dmi_type' : 1,
                        'key' : 'Version',
                    },
                ]
                # Todo -- disable dmidecode library warnings to stderr
                # or figure out how to clear the warning log in the decode module.
                for field in fields:
                    for v in field['subsystem']().values():
                        if type(v) is dict and v['dmi_type'] == field['dmi_type']:
                            rv[field['name']] = v['data'][field['key']]
            except:
                pass
            finally:
                if 'dmidecodemod' in sys.modules:
                    sys.modules['dmidecodemod'].clear_warnings()
        return rv

    def upgrade_manifest(self, type_, override_dir=None):
        if override_dir:
            m = os.path.join(override_dir, "manifest.json")
        else:
            m = os.path.join(self.basedir_onl(), "upgrade", type_, "manifest.json")

        if os.path.exists(m):
            return (os.path.dirname(m), m, json.load(file(m)))
        else:
            return (None, None, None)


    def new_device(self, driver, addr, bus, devdir):
        if not os.path.exists(os.path.join(bus, devdir)):
            try:
                with open("%s/new_device" % bus, "w") as f:
                    f.write("%s 0x%x\n" % (driver, addr))
            except Exception, e:
                print "Unexpected error initialize device %s:0x%x:%s: %s" % (driver, addr, bus, e)
        else:
            print("Device %s:%x:%s already exists." % (driver, addr, bus))

    def new_devices(self, new_device_list):
        for (driver, addr, bus, devdir) in new_device_list:
            self.new_device(driver, addr, bus, devdir)

    def new_i2c_device(self, driver, addr, bus_number):
        bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number
        devdir = "%d-%4.4x" % (bus_number, addr)
        return self.new_device(driver, addr, bus, devdir)

    def new_i2c_devices(self, new_device_list):
        for (driver, addr, bus_number) in new_device_list:
            self.new_i2c_device(driver, addr, bus_number)

    def ifnumber(self):
        # The default assumption for any platform
        # is ma1 and lo
        return 2

    def environment(self, fmt='user'):
        if fmt not in [ 'user', 'yaml', 'dict', 'json' ]:
            raise ValueError("Unsupported format '%s'" % fmt)

        if fmt == 'user':
            return subprocess.check_output(['/bin/onlpd', '-r'])
        else:
            yamlstr = subprocess.check_output(['/bin/onlpd', '-r', '-y'])
            if fmt == 'yaml':
                return yamlstr
            else:
                data = yaml.load(yamlstr)
                if fmt == 'json':
                    return json.dumps(data, indent=2)
                else:
                    return data

    def __str__(self):
        s = """Model: %s
Manufacturer: %s
Ports: %s (%s)
Platform Revision: %s
System Object Id: %s
System Information:
%s
%s
""" % (
            self.MODEL,
            self.MANUFACTURER,
            self.PORT_COUNT,
            self.PORT_CONFIG,
            self.PLATFORM.split('-')[-1],
            self.sys_object_id(),
            str(self.onie_info),
            str(self.platform_info),
            )


        if hasattr(self, 'warning'):
            s += """

Warning: %s

""" % (self.warning())
        return s


class OnlPlatformPortConfig_48x1_4x10(object):
    PORT_COUNT=52
    PORT_CONFIG="48x1 + 4x10"

class OnlPlatformPortConfig_48x1_2x10(object):
    PORT_COUNT=50
    PORT_CONFIG="48x1 + 2x10"

class OnlPlatformPortConfig_48x10_4x40(object):
    PORT_COUNT=52
    PORT_CONFIG="48x10 + 4x40"

class OnlPlatformPortConfig_48x10_6x40(object):
    PORT_COUNT=54
    PORT_CONFIG="48x10 + 6x40"

class OnlPlatformPortConfig_48x10_4x100(object):
    PORT_COUNT=52
    PORT_CONFIG="48x10 + 4x100"

class OnlPlatformPortConfig_48x25_6x100(object):
    PORT_COUNT=54
    PORT_CONFIG="48x25 + 6x100"

class OnlPlatformPortConfig_48x25_8x100(object):
    PORT_COUNT=56
    PORT_CONFIG="48x25 + 8x100"

class OnlPlatformPortConfig_48x25_4x100_2x200(object):
    PORT_COUNT=54
    PORT_CONFIG="48x25 + 4x100 + 2x200"

class OnlPlatformPortConfig_32x40(object):
    PORT_COUNT=32
    PORT_CONFIG="32x40"

class OnlPlatformPortConfig_32x400(object):
    PORT_COUNT=32
    PORT_CONFIG="32x400"

class OnlPlatformPortConfig_64x40(object):
    PORT_COUNT=64
    PORT_CONFIG="64x40"

class OnlPlatformPortConfig_32x100(object):
    PORT_COUNT=32
    PORT_CONFIG="32x100"

class OnlPlatformPortConfig_60x100(object):
    PORT_COUNT=60
    PORT_CONFIG="60x100"

class OnlPlatformPortConfig_64x100(object):
    PORT_COUNT=64
    PORT_CONFIG="64x100"

class OnlPlatformPortConfig_80x100(object):
    PORT_COUNT=80
    PORT_CONFIG="80x100"

class OnlPlatformPortConfig_128x100(object):
    PORT_COUNT=128
    PORT_CONFIG="128x100"

class OnlPlatformPortConfig_64x200(object):
    PORT_COUNT=64
    PORT_CONFIG="64x200"

class OnlPlatformPortConfig_24x1_4x10(object):
    PORT_COUNT=28
    PORT_CONFIG="24x1 + 4x10"

class OnlPlatformPortConfig_8x1_8x10(object):
    PORT_COUNT=16
    PORT_CONFIG="8x1 + 8x10"

class OnlPlatformPortConfig_48x10_6x100(object):
    PORT_COUNT=54
    PORT_CONFIG="48x10 + 6x100"

class OnlPlatformPortConfig_12x10_3x100(object):
    PORT_COUNT=15
    PORT_CONFIG="12x10 + 3x100"

class OnlPlatformPortConfig_12x25_3x100(object):
    PORT_COUNT=15
    PORT_CONFIG="12x25 + 3x100"

class OnlPlatformPortConfig_24x10_2x100(object):
    PORT_COUNT=26
    PORT_CONFIG="24x10 + 2x100"

class OnlPlatformPortConfig_24x25_4x100(object):
    PORT_COUNT=28
    PORT_CONFIG="24x25 + 4x100"

class OnlPlatformPortConfig_20x100(object):
    PORT_COUNT=20
    PORT_CONFIG="20x100"

class OnlPlatformPortConfig_16x10_8x25_2x100(object):
    PORT_COUNT=26
    PORT_CONFIG="16x10 + 8x25 + 2x100"

class OnlPlatformPortConfig_46x10_6x100(object):
    PORT_COUNT=52
    PORT_CONFIG="46x10 + 6x100"

class OnlPlatformPortConfig_20x10_4x25_3x100(object):
    PORT_COUNT=27
    PORT_CONFIG="20x10 + 4x25 + 3x100"

class OnlPlatformPortConfig_24x400_2x10(object):
    PORT_COUNT=26
    PORT_CONFIG="24x400 + 2x10"

class OnlPlatformPortConfig_4x1_8x10(object):
    PORT_COUNT=12
    PORT_CONFIG="4x1 + 8x10"

class OnlPlatformPortConfig_32x400_2x10(object):
    PORT_COUNT=34
    PORT_CONFIG="32x400 + 2x10"

class OnlPlatformPortConfig_6x400_48x50(object):
    PORT_COUNT=54
    PORT_CONFIG="6x400 + 48x50"

class OnlPlatformPortConfig_6x10_8x1_4x1(object):
    PORT_COUNT=18
    PORT_CONFIG="6x10 + 8x1 + 4x1"

class OnlPlatformPortConfig_10x400_18x100_2x10(object):
    PORT_COUNT=30
    PORT_CONFIG="10x400 + 18x100 + 2x10"
