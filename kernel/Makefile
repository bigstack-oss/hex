# HEX SDK

include ../../build.mk

KERNEL_VERSION := 6.12.65
KERNEL_BRANCH := $(shell echo $(KERNEL_VERSION) | cut -d. -f1,2)
KERNEL_RPMS := kernel.rpm

RPMBUILD_DIR := rpmbuild
RPMBUILD_SOURCES := $(RPMBUILD_DIR)/SOURCES
RPMBUILD_SRPMS := $(RPMBUILD_DIR)/SRPMS
RPMBUILD_SPECS := $(RPMBUILD_DIR)/SPECS
RPMBUILD_BUILD := $(RPMBUILD_DIR)/BUILD
RPMBUILD_RPMS := $(RPMBUILD_DIR)/RPMS

# prepare the rpm build directory
$(RPMBUILD_DIR):
	$(Q)mkdir $(RPMBUILD_DIR)
	$(Q)cd $(RPMBUILD_DIR) && mkdir SOURCES SRPMS SPECS BUILD RPMS

LINUX_SOURCE_TARBALL := linux-$(KERNEL_VERSION).tar.xz
LINUX_SOURCE_TARBALL_URL := https://www.kernel.org/pub/linux/kernel/v6.x/linux-$(KERNEL_VERSION).tar.xz

$(LINUX_SOURCE_TARBALL):
	$(call RUN_CMD_TIMED, \
		wget -O $@ $(LINUX_SOURCE_TARBALL_URL), \
		"  PULL    $@")

ELREPO_GITHUB_URL := https://github.com/bigstack-oss/kernel-config
ELREPO_GIT_DIR := elrepo_git

# pull down ELRepo kernel rpm spec file
$(ELREPO_GIT_DIR):
	$(call RUN_CMD_TIMED, \
		for i in {1..10} ; do \
		rm -rf $@ && timeout 30 git clone -b $(KERNEL_BRANCH) --depth 1 $(ELREPO_GITHUB_URL).git $@ && break ; \
		done,\
		"  CLONE   $(ELREPO_GITHUB_URL)")

CENTOS_STREAM_9_GITLAB_URL := https://gitlab.com/redhat/centos-stream/src/kernel/centos-stream-9.git
CENTOS_STREAM_9_GIT_DIR := centos_stream_9_git

$(CENTOS_STREAM_9_GIT_DIR):
	$(call RUN_CMD_TIMED, \
		for i in {1..10} ; do \
		rm -rf $@ && timeout 200 git clone --depth 1 $(CENTOS_STREAM_9_GITLAB_URL) $@ && break ; \
		done,\
		"  CLONE   $(CENTOS_STREAM_9_GITLAB_URL)")

# build the kernel rpm packages
$(KERNEL_RPMS): $(RPMBUILD_DIR) $(LINUX_SOURCE_TARBALL) $(ELREPO_GIT_DIR) $(CENTOS_STREAM_9_GIT_DIR)
	$(Q)cp $(ELREPO_GIT_DIR)/kernel-ml/el9/kernel-ml-$(KERNEL_BRANCH).spec $(RPMBUILD_SPECS)/
	$(Q)cp $(LINUX_SOURCE_TARBALL) $(RPMBUILD_SOURCES)/
	$(Q)cp $(ELREPO_GIT_DIR)/kernel-ml/el9/config-$(KERNEL_VERSION)-x86_64 $(RPMBUILD_SOURCES)/
	$(Q)cp $(ELREPO_GIT_DIR)/kernel-ml/el9/config-$(KERNEL_VERSION)-aarch64 $(RPMBUILD_SOURCES)/
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/mod-denylist.sh $(RPMBUILD_SOURCES)/
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/mod-sign.sh $(RPMBUILD_SOURCES)/
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/rhel_files/mod-extra.list.rhel $(RPMBUILD_SOURCES)/mod-extra.list
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/rhel_files/filter-x86_64.sh.rhel $(RPMBUILD_SOURCES)/filter-x86_64.sh
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/rhel_files/filter-aarch64.sh.rhel $(RPMBUILD_SOURCES)/filter-aarch64.sh
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/rhel_files/filter-modules.sh.rhel $(RPMBUILD_SOURCES)/filter-modules.sh
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/cpupower.service $(RPMBUILD_SOURCES)/
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/cpupower.config $(RPMBUILD_SOURCES)/
	$(Q)cp $(CENTOS_STREAM_9_GIT_DIR)/redhat/kvm_stat.logrotate $(RPMBUILD_SOURCES)/
	$(Q)dnf builddep -y $(RPMBUILD_SPECS)/kernel-ml-$(KERNEL_BRANCH).spec
	$(call RUN_CMD_TIMED, \
		rpmbuild -bb \
			--define "_topdir $(CURDIR)/$(RPMBUILD_DIR)" \
			--target x86_64 \
			--with src \
			$(RPMBUILD_DIR)/SPECS/kernel-ml-$(KERNEL_BRANCH).spec, \
		"  BUILD   $(KERNEL_RPMS)")
	$(Q)touch $(KERNEL_RPMS)

BUILD += $(KERNEL_RPMS)
BUILDCLEAN += $(KERNEL_RPMS) $(RPMBUILD_DIR) $(LINUX_SOURCE_TARBALL) $(ELREPO_GIT_DIR) $(CENTOS_STREAM_9_GIT_DIR)

include $(HEX_MAKEDIR)/hex_sdk.mk
