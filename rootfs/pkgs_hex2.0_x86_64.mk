# HEX SDK

### Packages for HEX_MINI_INITRD
# dmidecode by hex_hwdetect scripts
# kmod for depmod & modprobe
# util-linux for switch_root
MINI_PKGS += coreutils dmidecode e2fsprogs e2fsprogs-libs fuse-libs glibc libblkid libcap libcom_err libfdisk libgcc libmount libselinux libsmartcols libss libuuid ncurses-libs pcre2 systemd-libs util-linux zlib popt rsync

### Packages for HEX_BASE_ROOTFS
# kmod for depmod & modprobe
# kexec-tools for kernel dump userspace component
# basic tools: hostname, cpio, less, tree, ncurses
# openssl-devel libyaml-devel glib2-devel for hex_config
# openssl for hex_install
# e2fsprogs for mkfs.ext4
# xfsprogs for mkfs.xfs
# dosfstools for mkfs.vfat
# parted for creating disk partition
# lspci(pciutuls), ethtool for hwdetect
# editor: mg
# GRUB2 boot loader
#     grub2-common
#     grub2-pc
#     grub2-pc-modules
#     grub2-tools
#     grub2-tools-extra
#     grub2-tools-minimal
# GRUB2 EFI
#     grub2-efi-x64
#     grub2-efi-modules-x64
#     shim-x64
#  libyaml-devel
BASE_PKGS += kmod kexec-tools hostname cpio less tree ncurses openssl-devel glib2-devel openssl e2fsprogs xfsprogs dosfstools parted glibc-locale-source glibc-langpack-en pciutils ethtool efibootmgr mg passwd

CENTOS_MIRROR := https://mirror.stream.centos.org/9-stream/BaseOS/x86_64/os/Packages
DISTRO := el9
GRUB2_VER := 2.06-82

BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-common-$(GRUB2_VER).$(DISTRO).noarch.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-pc-$(GRUB2_VER).$(DISTRO).x86_64.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-pc-modules-$(GRUB2_VER).$(DISTRO).noarch.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-tools-$(GRUB2_VER).$(DISTRO).x86_64.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-tools-extra-$(GRUB2_VER).$(DISTRO).x86_64.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-tools-minimal-$(GRUB2_VER).$(DISTRO).x86_64.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-efi-x64-$(GRUB2_VER).$(DISTRO).x86_64.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/grub2-efi-x64-modules-$(GRUB2_VER).$(DISTRO).noarch.rpm
BASE_LOCK_PKGS += $(CENTOS_MIRROR)/shim-x64-15-15.el8_2.x86_64.rpm

### Packages for HEX_FULL_ROOTFS
# System & Netwokring & Storage
#     ifconfig(net-tools), ip(iproute)
#     ping(iputils), traceroute, tcpdump
#     exfat-utils, iostat(sysstat), hdparm, lsscsi, lshw
# Productivity
#     openssh-server, vim, dnf-automatic
# Hex SDK tools
#     zip, unzip, crontabs, logrotate
#     rrdtool, sqlite
#     dhcp-client(dhcp-client)
FULL_PKGS += net-tools iproute iputils traceroute tcpdump sysstat hdparm lsscsi lshw openssh-server vim-enhanced zip unzip logrotate rrdtool sqlite dhcp-client rsyslog cracklib-dicts fio
FULL_PKGS_NOARCH += crontabs dnf-automatic
FULL_PKGS += exfatprogs