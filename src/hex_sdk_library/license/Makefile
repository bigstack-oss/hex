# HEX SDK

include ../../../../build.mk

$(PRIVATE_PEM):
	$(Q)openssl genrsa -aes256 -out $@ -passout pass:$(PASSPHRASE) 8192
	$(Q)chmod 600 $@

$(PUBLIC_PEM): $(PRIVATE_PEM)
	$(Q)openssl rsa -in $< -outform PEM -passin pass:$(PASSPHRASE) -pubout -out $@ 2>/dev/null
	$(Q)chmod 600 $@

license_key.h: $(PUBLIC_PEM)
	$(Q)echo "#define PUBLIC_KEY \"$(shell cat $< | awk '{printf "%s\\n", $$0}')\"" > $@

SUBDIRS = tests

LIB = $(HEX_SDK_LIB_ARCHIVE)

LIB_SRCS = license.cpp

COMPILE_FOR_SHARED_LIB = 1

BUILD += license_key.h

CLEAN += license_key.h

include $(HEX_MAKEDIR)/hex_sdk.mk
