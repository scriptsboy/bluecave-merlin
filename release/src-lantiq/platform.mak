export LINUXDIR := $(SRCBASE)/linux/linux-3.10.104

ifeq ($(EXTRACFLAGS),)
EXTRACFLAGS := -DCONFIG_LANTIQ -DDEBUG_NOISY -DDEBUG_RCTEST -pipe -funit-at-a-time -Wno-pointer-sign -DLINUX30 -mips32r2 -mno-branch-likely -mtune=1004kc
endif
export LINUXDIR := $(SRCBASE)/linux/linux-3.10.104
export KERNEL_BINARY=$(LINUXDIR)/vmlinux
export PLATFORM := mipsel-uclibc
export MODEL_EXT := _lantiq
export CROSS_COMPILE := mips-openwrt-linux-uclibc-
export TOOLCHAIN_NAME=toolchain-mips_mips32_gcc-4.8-linaro_uClibc-0.9.33.2
export TOOLCHAIN_HOST_NAME=host
export WORKING_DIR=${SRCBASE}
export STAGING_DIR=${WORKING_DIR}/tools/${TOOLCHAIN_NAME}
# export PATH := ${STAGING_DIR}/${TOOLCHAIN_NAME}/bin:${STAGING_DIR}/host/bin:$(PATH)
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/opt/brcm/hndtools-mipsel-linux/bin:/opt/brcm/hndtools-mipsel-uclibc/bin:/projects/tools:/projects/hnd/tools/linux/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin:/projects/hnd/tools/linux/hndtools-arm-linux-2.6.36-uclibc-4.5.3/arm-linux/bin:/projects/hnd/tools/linux/hndtools-arm-linux-2.6.36-uclibc-4.5.3/usr/bin:/projects/hnd/tools/linux/hndtools-armeabi-2011.09/bin"
export PATH := $(PATH):${STAGING_DIR}/bin:${WORKING_DIR}/tools/host/bin
export CROSS_COMPILER := $(CROSS_COMPILE)
export READELF := mips-openwrt-linux-uclibc-readelf
export CONFIGURE := ./configure --host=mips-linux --build=$(BUILD)
export HOSTCONFIG := linux-mipsel
export ARCH := mips
export HOST := mipsel-linux
export KERNELCC := $(CROSS_COMPILE)gcc
export KERNELLD := $(CROSS_COMPILE)ld
export TOOLS := $(SRCBASE)/tools/${TOOLCHAIN_NAME}
export CC=mips-openwrt-linux-uclibc-gcc
export RTVER := 0.9.30.1
export RTVER := 0.9.30.1

# Kernel load address and entry address
export LOADADDR := 41508000
export ENTRYADDR := $(LOADADDR)
export s_load_addr := 0xa0020000
export s_entry_addr := 0xa002df00
export image_header := MIPS LTQCPE Linux-3.10.104
export compression_type := lzma
export ENTRYADDR := $(LOADADDR)

export CONFIG_LINUX26=y
export CONFIG_LANTIQ=y

EXTRA_CFLAGS += -DLINUX30
EXTRA_BLUEZ_CFLAGS += -DRTCONFIG_LANTIQ -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/bluez-5.41/src/bleencrypt/include
EXTRA_BLUEZ_LDFLAGS += -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared
export CONFIG_LINUX30=y

# not for ALPINE EXTRA_CFLAGS := -DLINUX26 -DCONFIG_BCMWL5 -DDEBUG_NOISY -DDEBUG_RCTEST -pipe -DTTEST
EXTRA_CFLAGS := -DLINUX26 -DCONFIG_LANTIQ -DCONFIG_INTEL -pipe -DDEBUG_NOISY -DDEBUG_RCTEST -mfpu=vfpv3-d16 -mfloat-abi=softfp
EXTRACFLAGS += -DLINUX26

define platformRouterOptions
	@( \
	if [ "$(INTEL)" = "y" ]; then \
	        sed -i "/RTCONFIG_INTEL\>/d" $(1); \
	        echo "RTCONFIG_INTEL=y" >>$(1); \
	fi; \
	)
endef

define platformBusyboxOptions
endef

define platformKernelConfig
# prepare config_base
# prepare prebuilt kernel binary
	@( \
	sed -i "/CONFIG_RGMII_BCM_FA/d" $(1); \
	if [ "$(RGMII_BCM_FA)" = "y" ]; then \
		echo "CONFIG_RGMII_BCM_FA=y" >> $(1); \
	else \
		echo "# CONFIG_RGMII_BCM_FA is not set" >> $(1); \
	fi; \
	if [ "$(LACP)" = "y" ]; then \
		sed -i "/CONFIG_LACP/d" $(1); \
		echo "CONFIG_LACP=m" >>$(1); \
		sed -i "/CONFIG_BCM_AGG/d" $(1); \
		echo "CONFIG_BCM_AGG=y" >>$(1); \
	fi; \
	if [ "$(BCMNAND)" = "y" ]; then \
		sed -i "/CONFIG_MTD_NFLASH/d" $(1); \
		echo "CONFIG_MTD_NFLASH=y" >>$(1); \
		sed -i "/CONFIG_MTD_NAND/d" $(1); \
		echo "CONFIG_MTD_NAND=y" >>$(1); \
		echo "CONFIG_MTD_NAND_IDS=y" >>$(1); \
		echo "# CONFIG_MTD_NAND_VERIFY_WRITE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ECC_SMC is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_MUSEUM_IDS is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_DENALI is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_RICOH is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_DISKONCHIP is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_CAFE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_NANDSIM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_PLATFORM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ONENAND is not set" >>$(1); \
		sed -i "/CONFIG_MTD_BRCMNAND/d" $(1); \
		echo "CONFIG_MTD_BRCMNAND=y" >>$(1); \
	fi; \
	if [ "$(ARM)" = "y" ]; then \
		mkdir -p $(SRCBASE)/router/ctf_arm/linux; \
		if [ "$(BCM7)" = "y" ]; then \
			if [ "$(ARMCPUSMP)" = "up" ]; then \
				cp -f $(SRCBASE)/router/ctf_arm/bcm7_up/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			else \
				cp -f $(SRCBASE)/router/ctf_arm/bcm7/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
				cp -f $(SRCBASE)/router/dpsta/bcm7_3200/dpsta.o $(SRCBASE)/router/dpsta/linux; \
			fi; \
		elif [ "$(BCM_7114)" = "y" ]; then \
			if [ "$(GMAC3)" = "y" ]; then \
				cp -f $(SRCBASE)/router/ctf_arm/bcm_7114_gmac3/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			else \
				cp -f $(SRCBASE)/router/ctf_arm/bcm_7114/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			fi; \
			cp -f $(SRCBASE)/router/dpsta/bcm7114/dpsta.o $(SRCBASE)/router/dpsta/linux; \
		elif [ "$(BCM10)" = "y" ]; then \
			if [ "$(ARMCPUSMP)" = "up" ]; then \
				cp -f $(SRCBASE)/router/ctf_arm/bcm7_up/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			else \
				cp -f $(SRCBASE)/router/ctf_arm/bcm7/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			fi; \
		elif [ "$(BCM9)" = "y" ]; then \
                                cp -f $(SRCBASE)/router/ctf_arm/bcm9/ctf.* $(SRCBASE)/router/ctf_arm/linux/;\
		else \
			if [ "$(ARMCPUSMP)" = "up" ]; then \
				cp -f $(SRCBASE)/router/ctf_arm/bcm6_up/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			else \
				cp -f $(SRCBASE)/router/ctf_arm/bcm6/ctf.* $(SRCBASE)/router/ctf_arm/linux/; \
			fi; \
		fi; \
	fi; \
	if [ "$(SFPRAM16M)" = "y" ]; then \
		sed -i "/CONFIG_WL_USE_AP/d" $(1); \
		echo "CONFIG_WL_USE_APSTA=y" >>$(1); \
		echo "# CONFIG_WL_USE_AP is not set" >>$(1); \
		echo "# CONFIG_WL_USE_AP_SDSTD is not set" >>$(1); \
		echo "# CONFIG_WL_USE_AP_ONCHIP_G is not set" >>$(1); \
		echo "# CONFIG_WL_USE_APSTA_ONCHIP_G is not set" >>$(1); \
		sed -i "/CONFIG_INET_GRO/d" $(1); \
		echo "# CONFIG_INET_GRO is not set" >> $(1); \
		sed -i "/CONFIG_INET_GSO/d" $(1); \
		echo "# CONFIG_INET_GSO is not set" >> $(1); \
		sed -i "/CONFIG_NET_SCH_HFSC/d" $(1); \
		echo "# CONFIG_NET_SCH_HFSC is not set" >> $(1); \
		sed -i "/CONFIG_NET_SCH_ESFQ/d" $(1); \
		echo "# CONFIG_NET_SCH_ESFQ is not set" >> $(1); \
		sed -i "/CONFIG_NET_SCH_TBF/d" $(1); \
		echo "# CONFIG_NET_SCH_TBF is not set" >> $(1); \
		sed -i "/CONFIG_NLS/d" $(1); \
		echo "# CONFIG_NLS is not set" >>$(1); \
		echo "# CONFIG_NLS_DEFAULT=\"iso8859-1\"">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_437 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_737 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_775 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_850 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_852 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_855 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_857 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_860 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_861 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_862 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_863 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_864 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_865 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_866 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_869 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_936 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_950 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_932 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_949 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_874 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_8 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_1250 is not set">>$(1); \
		echo "# CONFIG_NLS_CODEPAGE_1251 is not set">>$(1); \
		echo "# CONFIG_NLS_ASCII is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_1 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_2 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_3 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_4 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_5 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_6 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_7 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_9 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_13 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_14 is not set">>$(1); \
		echo "# CONFIG_NLS_ISO8859_15 is not set">>$(1); \
		echo "# CONFIG_NLS_KOI8_R is not set">>$(1); \
		echo "# CONFIG_NLS_KOI8_U is not set">>$(1); \
		echo "# CONFIG_NLS_UTF8 is not set">>$(1); \
		sed -i "/CONFIG_USB/d" $(1); \
		echo "# CONFIG_USB_SUPPORT is not set" >> $(1); \
		sed -i "/CONFIG_SCSI/d" $(1); \
		echo "# CONFIG_SCSI is not set" >> $(1); \
		sed -i "/CONFIG_LBD/d" $(1); \
		echo "# CONFIG_LBD is not set" >> $(1); \
		sed -i "/CONFIG_BLK_DEV_SD/d" $(1); \
		sed -i "/CONFIG_BLK_DEV_SR/d" $(1); \
		sed -i "/CONFIG_CHR_DEV_SG/d" $(1); \
		sed -i "/CONFIG_VIDEO/d" $(1); \
		echo "# CONFIG_VIDEO_DEV is not set" >> $(1); \
		sed -i "/CONFIG_V4L_USB_DRIVERS/d" $(1); \
		sed -i "/CONFIG_SOUND/d" $(1); \
		echo "# CONFIG_SOUND is not set" >> $(1); \
		sed -i "/CONFIG_SND/d" $(1); \
		sed -i "/CONFIG_HID/d" $(1); \
		echo "# CONFIG_HID is not set" >> $(1); \
		sed -i "/CONFIG_MMC/d" $(1); \
		echo "# CONFIG_MMC is not set" >> $(1); \
		sed -i "/CONFIG_PARTITION_ADVANCED/d" $(1); \
		echo "# CONFIG_PARTITION_ADVANCED is not set" >> $(1); \
		sed -i "/CONFIG_TRACE_IRQFLAGS_SUPPORT/d" $(1); \
		sed -i "/CONFIG_SYS_SUPPORTS_KGDB/d" $(1); \
		sed -i "/CONFIG_EXT2_FS/d" $(1); \
		echo "# CONFIG_EXT2_FS is not set" >> $(1); \
		sed -i "/CONFIG_EXT3_FS/d" $(1); \
		echo "# CONFIG_EXT3_FS is not set" >> $(1); \
		sed -i "/CONFIG_JBD/d" $(1); \
		echo "# CONFIG_JBD is not set" >> $(1); \
		sed -i "/CONFIG_REISERFS_FS/d" $(1); \
		echo "# CONFIG_REISERFS_FS is not set" >> $(1); \
		sed -i "/CONFIG_FAT_FS/d" $(1); \
		echo "# CONFIG_FAT_FS is not set" >> $(1); \
		sed -i "/CONFIG_VFAT_FS/d" $(1); \
		echo "# CONFIG_VFAT_FS is not set" >> $(1); \
		sed -i "/CONFIG_NFS_FS/d" $(1); \
		echo "# CONFIG_NFS_FS is not set" >> $(1); \
		sed -i "/CONFIG_NFSD/d" $(1); \
		echo "# CONFIG_NFSD is not set" >> $(1); \
		sed -i "/CONFIG_FUSE_FS/d" $(1); \
		echo "# CONFIG_FUSE_FS is not set" >> $(1); \
		sed -i "/CONFIG_CIFS/d" $(1); \
		echo "# CONFIG_CIFS is not set" >> $(1); \
		sed -i "/CONFIG_FAT/d" $(1); \
		sed -i "/CONFIG_INOTIFY/d" $(1); \
		echo "# CONFIG_INOTIFY is not set" >> $(1); \
		sed -i "/CONFIG_DNOTIFY/d" $(1); \
		echo "# CONFIG_DNOTIFY is not set" >> $(1); \
		sed -i "/CONFIG_CRYPTO_BLKCIPHER/d" $(1); \
		sed -i "/CONFIG_CRYPTO_HASH/d" $(1); \
		sed -i "/CONFIG_CRYPTO_MANAGER/d" $(1); \
		echo "# CONFIG_CRYPTO_MANAGER is not set" >> $(1); \
		sed -i "/CONFIG_CRYPTO_HMAC/d" $(1); \
		echo "# CONFIG_CRYPTO_HMAC is not set" >> $(1); \
		sed -i "/CONFIG_CRYPTO_ECB/d" $(1); \
		echo "# CONFIG_CRYPTO_ECB is not set" >> $(1); \
		sed -i "/CONFIG_CRYPTO_CBC/d" $(1); \
		echo "# CONFIG_CRYPTO_CBC is not set" >> $(1); \
	fi; \
	if [ "$(ARM)" = "y" ]; then \
		if [ "$(BCM7)" = "y" ]; then \
			if [ -d $(SRCBASE)/../../43602/src/wl/sysdeps/$(BUILD_NAME) ]; then \
				if [ -d $(SRCBASE)/../../43602/src/wl/sysdeps/$(BUILD_NAME)/clm ]; then \
					cp -f $(SRCBASE)/../../43602/src/wl/sysdeps/$(BUILD_NAME)/clm/src/wlc_clm_data.c $(SRCBASE)/../../43602/src/wl/clm/src/. ; \
					cp -f $(SRCBASE)/../../43602/src/wl/sysdeps/$(BUILD_NAME)/clm/src/wlc_clm_data_inc.c $(SRCBASE)/../../43602/src/wl/clm/src/. ; \
				fi; \
			else \
				if [ -d $(SRCBASE)/../../43602/src/wl/sysdeps/default/clm ]; then \
					cp -f $(SRCBASE)/../../43602/src/wl/sysdeps/default/clm/src/wlc_clm_data.c $(SRCBASE)/../../43602/src/wl/clm/src/. ; \
					cp -f $(SRCBASE)/../../43602/src/wl/sysdeps/default/clm/src/wlc_clm_data_inc.c $(SRCBASE)/../../43602/src/wl/clm/src/. ; \
				fi; \
			fi; \
			if [ -d $(SRCBASE)/router/wl_arm_7/prebuilt ]; then \
				mkdir $(SRCBASE)/wl/linux ; \
				cp $(SRCBASE)/router/wl_arm_7/prebuilt/wl*.o $(SRCBASE)/wl/linux ; \
				mkdir -p $(SRCBASE)/../../dhd/src/dhd/linux ; \
				cp $(SRCBASE)/router/wl_arm_7/prebuilt/dhd.o $(SRCBASE)/../../dhd/src/dhd/linux ; \
			fi; \
			if [ -d $(SRCBASE)/router/et_arm_7/prebuilt ]; then \
				mkdir -p $(SRCBASE)/et/linux ; \
				cp $(SRCBASE)/router/et_arm_7/prebuilt/et.o $(SRCBASE)/et/linux ; \
			fi; \
		elif [ "$(BCM_7114)" = "y" ]; then \
			if [ -d $(SRCBASE)/router/wl_arm_7114/prebuilt ]; then \
				mkdir -p $(SRCBASE)/../dhd/src/dhd/linux ; \
				cp $(SRCBASE)/router/wl_arm_7114/prebuilt/dhd.o $(SRCBASE)/../dhd/src/dhd/linux ; \
			fi; \
			if [ -d $(SRCBASE)/router/et_arm_7114/prebuilt ]; then \
				mkdir -p $(SRCBASE)/et/linux ; \
				cp $(SRCBASE)/router/et_arm_7114/prebuilt/et.o $(SRCBASE)/et/linux ; \
			fi; \
		elif [ "$(BCM_10)" = "y" ]; then \
			if [ -d $(SRCBASE)/router/wl_arm_10/prebuilt ]; then \
				mkdir $(SRCBASE)/wl/linux ; \
				cp $(SRCBASE)/router/wl_arm_10/prebuilt/wl*.o $(SRCBASE)/wl/linux ; \
			fi; \
			if [ -d $(SRCBASE)/router/et_arm_10/prebuilt ]; then \
				mkdir -p $(SRCBASE)/et/linux ; \
				cp $(SRCBASE)/router/et_arm_10/prebuilt/et.o $(SRCBASE)/et/linux ; \
			fi; \
		elif [ "$(BCM9)" = "y" ]; then \
			if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME) ]; then \
				if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/linux ]; then \
					cp -rf $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/linux $(SRCBASE)/wl/. ; \
				fi; \
				if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/clm ]; then \
					cp -f $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/clm/src/wlc_clm_data.c $(SRCBASE)/wl/clm/src/. ; \
				fi; \
			elif [ -d $(SRCBASE)/wl/sysdeps/default ]; then \
				if [ -d $(SRCBASE)/wl/sysdeps/default/linux ]; then \
					cp -rf $(SRCBASE)/wl/sysdeps/default/linux $(SRCBASE)/wl/. ; \
				fi; \
				if [ -d $(SRCBASE)/wl/sysdeps/default/clm ]; then \
					cp -f $(SRCBASE)/wl/sysdeps/default/clm/src/wlc_clm_data.c $(SRCBASE)/wl/clm/src/. ; \
				fi; \
			fi; \
			if [ -d $(SRCBASE)/router/wl_arm_9/prebuilt ]; then \
				mkdir $(SRCBASE)/wl/linux ; \
				cp $(SRCBASE)/router/wl_arm_9/prebuilt/wl*.o $(SRCBASE)/wl/linux ; \
			fi; \
			if [ -d $(SRCBASE)/router/et_arm_9/prebuilt ]; then \
				mkdir -p $(SRCBASE)/et/linux ; \
				cp $(SRCBASE)/router/et_arm_9/prebuilt/et.o $(SRCBASE)/et/linux ; \
			fi; \
		else \
			if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME) ]; then \
				if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/linux ]; then \
					cp -rf $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/linux $(SRCBASE)/wl/. ; \
				fi; \
				if [ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/clm ]; then \
					cp -f $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/clm/src/wlc_clm_data.c $(SRCBASE)/wl/clm/src/. ; \
				fi; \
			elif [ -d $(SRCBASE)/wl/sysdeps/default ]; then \
				if [ -d $(SRCBASE)/wl/sysdeps/default/linux ]; then \
					cp -rf $(SRCBASE)/wl/sysdeps/default/linux $(SRCBASE)/wl/. ; \
				fi; \
				if [ -d $(SRCBASE)/wl/sysdeps/default/clm ]; then \
					cp -f $(SRCBASE)/wl/sysdeps/default/clm/src/wlc_clm_data.c $(SRCBASE)/wl/clm/src/. ; \
				fi; \
			fi; \
			if [ -d $(SRCBASE)/router/wl_arm/prebuilt ]; then \
				mkdir $(SRCBASE)/wl/linux ; \
				cp $(SRCBASE)/router/wl_arm/prebuilt/wl*.o $(SRCBASE)/wl/linux ; \
			fi; \
			if [ -d $(SRCBASE)/router/et_arm/prebuilt ]; then \
				mkdir -p $(SRCBASE)/et/linux ; \
				cp $(SRCBASE)/router/et_arm/prebuilt/et.o $(SRCBASE)/et/linux ; \
			fi; \
		fi; \
	else \
		[ -d $(SRCBASE)/wl/sysdeps/default ] && \
			cp -rf $(SRCBASE)/wl/sysdeps/default/* $(SRCBASE)/wl/; \
		[ -d $(SRCBASE)/wl/sysdeps/$(BUILD_NAME) ] && \
			cp -rf $(SRCBASE)/wl/sysdeps/$(BUILD_NAME)/* $(SRCBASE)/wl/; \
	fi; \
	if [ "$(SNMPD)" = "y" ]; then \
		if [ -d $(SRCBASE)/router/net-snmp-5.7.2/asus_mibs/sysdeps/$(BUILD_NAME) ]; then \
			rm -f  $(SRCBASE)/router/net-snmp-5.7.2/mibs/RT-*.txt ; \
			rm -f  $(SRCBASE)/router/net-snmp-5.7.2/mibs/TM-*.txt ; \
			rm -rf $(SRCBASE)/router/net-snmp-5.7.2/agent/mibgroup/asus-mib ; \
			cp -rf $(SRCBASE)/router/net-snmp-5.7.2/asus_mibs/sysdeps/$(BUILD_NAME)/$(BUILD_NAME)-MIB.txt $(SRCBASE)/router/net-snmp-5.7.2/mibs ; \
			cp -rf $(SRCBASE)/router/net-snmp-5.7.2/asus_mibs/sysdeps/$(BUILD_NAME)/asus-mib $(SRCBASE)/router/net-snmp-5.7.2/agent/mibgroup ; \
		fi; \
	fi; \
	)
endef
