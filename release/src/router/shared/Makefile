include ../common.mak
include $(SRCBASE)/.config

CFLAGS += -Os -Wall $(EXTRACFLAGS) -fPIC
#CFLAGS += $(PRIVATE_EXTRACFLAGS)
CFLAGS += -I. -I$(TOP)/shared -I$(SRCBASE)/include

ifeq ($(RTCONFIG_REALTEK),y)
CFLAGS += -I$(TOP)/kernel_header/include
else ifneq ($(RTCONFIG_BCMARM),y)
CFLAGS += -I$(TOP)/kernel_header/include
else
ifneq ($(HND_ROUTER),y)
CFLAGS += -I$(LINUXDIR)/include
else
CFLAGS += -DHND_ROUTER -DLINUX26 -DCONFIG_BCMWL5
CFLAGS += -I$(HND_SRC)/bcmdrivers/opensource/include/bcm963xx 
CFLAGS += -I$(HND_SRC)/shared/opensource/include/bcm963xx
#CFLAGS += -I$(TOP)/ethctl -I$(TOP)/ethswctl
#CFLAGS += -I$(TOP)/ethswctl
CFLAGS += -I$(HND_SRC)/userspace/private/include
#LDFLAGS += -L$(TOP)/ethswctl_lib -lethswctl
endif
CFLAGS += -D__EXPORTED_HEADERS__
endif

CFLAGS += -DTEST1

ifeq ($(RTCONFIG_SHP),y)
CFLAGS += -DSHP
endif

ifeq ($(RTCONFIG_RALINK),y)
CFLAGS += -I./sysdeps/ralink -I$(TOP)/wireless_tools/ -DWE_NOLIBM 
endif

ifeq ($(RTCONFIG_QCA),y)
CFLAGS += -I./sysdeps/qca -I$(TOP)/wireless_tools/ -DWE_NOLIBM
CFLAGS += $(if $(RTCONFIG_SOC_IPQ8064),-DSMP)
endif

ifeq ($(RTCONFIG_ALPINE),y)
CFLAGS += -I./sysdeps/alpine -I$(TOP)/wireless_tools/ -DWE_NOLIBM
CFLAGS += -I$(SRCBASE)/common/include
endif

ifeq ($(RTCONFIG_LANTIQ),y)
CFLAGS += -I./sysdeps/lantiq -I$(TOP)/wireless_tools/ -DWE_NOLIBM
CFLAGS += -I$(TOP)/shared/sysdeps/lantiq -I$(TOP)/wireless_tools -I$(TOP)/fapi_wlan_common-1.0.0.1/include -I$(TOP)/libhelper-1.4.0.2/include -DPACKAGE_ID=\"fapi_wlan_common\" -DLOGGING_ID="fapi_wlan_common"
CFLAGS += -I$(TOP)/wlc_nt
endif



ifeq ($(RTCONFIG_BCMWL6),y)
CFLAGS += -I$(SRCBASE)/shared/bcmwifi/include
CFLAGS += -I$(SRCBASE)/common/include
ifeq ($(RTCONFIG_BCM7)$(RTCONFIG_HSPOT),y)
CFLAGS += ${WLAN_StdIncPathA}
endif
else
CFLAGS += -I$(SRCBASE)/shared
endif

ifeq ($(RTCONFIG_QTN),y)
CFLAGS += -I$(TOP)/libqcsapi_client -I$(TOP)/httpd -I./sysdeps/qtn
LDFLAGS += -L$(TOP)/libqcsapi_client -lqcsapi_client
endif

ifeq ($(RTCONFIG_BCMSMP),y)
CFLAGS += -DSMP
endif

ifeq ($(RTCONFIG_5301X),y)
CFLAGS += -DBCM5301X
endif

include $(SRCBASE)/.config

ifeq ($(RTCONFIG_4352_5G),y)
CFLAGS += -DBCM4352
endif

ifeq (2_6_36,$(LINUX_VERSION))
CFLAGS += -DAUTOFW_PORT_DEPRECATED
endif

CFLAGS += -DTRAFFIC_MGMT

ifeq ($(RTCONFIG_BCM7)$(RTCONFIG_HSPOT),y)
CFLAGS += -DD11AC_IOTYPES
endif

ifeq ($(RTCONFIG_CONCURRENTREPEATER),y)
SSID = $(subst -,,$(BUILD_NAME))
CFLAGS += -DDEF_SSID_2G=\"ASUS_$(SSID)\" -DDEF_SSID_5G=\"ASUS_$(SSID)_5G\"
endif

ifeq ($(or $(RTAC68A),$(RT4GAC68U)),y)
CFLAGS += -DRTAC68U
endif

ifeq ($(BUILD_NAME), RT-N10P_V3)
CFLAGS += -DRTN10P_V3
endif

OBJS = shutils.o linux_timer.o defaults.o model.o rtstate.o boardapi.o
OBJS += network_utility.o
OBJS += misc.o version.o files.o strings.o process.o 
OBJS += bin_sem_asus.o semaphore.o pids.o $(if $(wildcard notify_rc.c),notify_rc.o,prebuild/notify_rc.o)
OBJS += discover.o
OBJS += base64.o
OBJS += nvparse.o
OBJS += bwdpi_utils.o
OBJS += bwdpi_utils.o
OBJS += mac_name_tbl.o
OBJS += stun.o
OBJS += timer_utils.o
OBJS += scripts.o
ifeq ($(RTCONFIG_TRAFFIC_LIMITER),y)
OBJS += tld_utils.o
endif
ifeq ($(RTCONFIG_REALTEK),y)
	# do nothing #
else ifeq ($(RTCONFIG_BCM7),y)
OBJS += et_linux.o bcmwifi_channels.o
else ifeq ($(RTCONFIG_HSPOT),y)
OBJS += bcmwifi_channels.o
endif
ifeq ($(or $(RTCONFIG_BCM_7114),$(HND_ROUTER)),y)
OBJS += confmtd_utils.o
ifeq ($(RTCONFIG_WBD),y)
OBJS += wbd_rc_shared.o
endif
ifeq ($(RTCONFIG_GMAC3),y)
OBJS += et_linux.o
endif
endif
ifeq ($(RTCONFIG_REALTEK),y)
OBJS += api-realtek.o

else ifeq ($(RTCONFIG_RALINK),y)
OBJS += $(TOP)/wireless_tools/iwlib.o
OBJS += flash_mtd.o api-ralink.o
ifeq ($(RTCONFIG_RALINK_RT3052),y)
OBJS += ra3052.o
endif
ifeq ($(RTN14U)$(RTAC52U)$(RTAC51U)$(RTN11P)$(RTN300)$(RTN54U)$(RTAC1200HP)$(RTN56UB1)$(RTN56UB2)$(RTAC54U)$(RTAC1200GA1)$(RTAC1200GU)$(RPAC87)$(RTAC85U)$(RTAC85P)$(RTN800HP),y)
CFLAGS += -DRA_ESW
OBJS += mt7620.o
else
ifeq ($(RPAC56),y)
OBJS += mt7621.o
else
ifeq ($(RTCONFIG_RALINK_MT7628),y)
OBJS += mt7628.o
else
OBJS += rtkswitch.o
ifeq ($(RTAC53)$(RTAC51UP),y)
OBJS += mtkswitch.o
endif
endif
endif
endif

else ifeq ($(RTCONFIG_QCA),y)
# Qualcomm Atheros
OBJS += $(TOP)/wireless_tools/iwlib.o
OBJS += flash_mtd.o api-qca.o 
ifeq ($(or $(RTCONFIG_SWITCH_RTL8370M_PHY_QCA8033_X2),$(RTCONFIG_SWITCH_RTL8370MB_PHY_QCA8033_X2)),y)
OBJS += qca8033_rtl8370m.o mdio.o
else ifeq ($(RTCONFIG_QCA8033),y)
OBJS += qca8033.o
else ifeq ($(RTCONFIG_SOC_IPQ40XX),y)
OBJS += ipq40xx.o
else
OBJS += qca8337.o
endif
OBJS += qca.o
else ifeq ($(RTCONFIG_ALPINE),y)
OBJS += flash_mtd.o api-alpine.o
ifeq ($(RTCONFIG_EXT_RTL8370MB),y)
CFLAGS += -I$(LINUXDIR)/drivers/char/rtl8370mb
OBJS += rtl8370mb.o mdio.o
endif
else ifeq ($(RTCONFIG_LANTIQ),y)
OBJS += flash_mtd.o
else
# Broadcom
OBJS += wl.o wl_linux.o
OBJS += broadcom.o api-broadcom.o
ifeq ($(RTCONFIG_DSL_TCLINUX),y)
OBJS += api-tclinux.o
endif
ifeq ($(RTCONFIG_EXT_RTL8365MB),y)
CFLAGS += -I$(LINUXDIR)/drivers/char/rtl8365mb
OBJS += rtl_8365mb.o
endif
ifeq ($(RTCONFIG_EXT_RTL8370MB),y)
CFLAGS += -I$(LINUXDIR)/drivers/char/rtl8365mb
OBJS += rtl_8365mb.o
endif
ifeq ($(RTCONFIG_STA_MONITOR),y)
OBJS += bcm_stamon.o
endif
endif

ifeq ($(RTCONFIG_USB),y)
OBJS += usb.o

ID_CFLAGS += -I$(TOP)/$(BUSYBOX_DIR)/include
ID_CFLAGS += -I$(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id
ID_CFLAGS += -include $(TOP)/$(BUSYBOX_DIR)/include/autoconf.h
ID_CFLAGS += -D_GNU_SOURCE -DNDEBUG -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#ID_CFLAGS += -fdata-sections -ffunction-sections
ID_OBJS  = $(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/util.o
ID_OBJS += $(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/linux_swap.o
ID_OBJS += $(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/ext.o
ID_OBJS += $(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/fat.o
ID_OBJS += $(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/ntfs.o
ID_OBJS += $(if $(HFS),$(TOP)/$(BUSYBOX_DIR)/util-linux/volume_id/hfs.o)
ID_OBJS += $(if $(HFS),$(patsubst %.c,%.o,$(wildcard $(TOP)/$(BUSYBOX_DIR)/libbb/hash_md5.c)))
ID_OBJS += $(if $(HFS),$(patsubst %.c,%.o,$(wildcard $(TOP)/$(BUSYBOX_DIR)/libbb/bb_bswap_64.c)))
ID_OBJS += volume_id.o
#OBJS += volume_id.lib.o
OBJS += $(notdir $(ID_OBJS))

ifeq ($(RTCONFIG_USB_MODEM),y)
CFLAGS += -I$(TOP)/libdisk
OBJS += at_cmd.o
endif
endif

ifeq ($(RTCONFIG_BCMARM),y)
OBJS += ifaddrs.o
endif

ifeq ($(RTCONFIG_QTN),y)
OBJS += web-qtn.o
OBJS += rpc_raw.o
OBJS += rpc_raw_clnt.o
#OBJS += find_host_addr.o
endif

ifeq ($(RTCONFIG_BLINK_LED),y)
CFLAGS	+= -I$(TOP)/bled
OBJS	+= bled.o
endif

ifeq ($(RTCONFIG_SAVEJFFS),y)
OBJS	+= $(if $(wildcard jffs_cfgs.c),jffs_cfgs.o,prebuild/jffs_cfgs.o)
endif

ifeq ($(RTCONFIG_QCA_PLC_UTILS),y)
OBJS	+= $(if $(wildcard plc_utils.c),plc_utils.o,prebuild/plc_utils.o)
endif

ifeq ($(RTCONFIG_TCODE),y)
OBJS	+= $(if $(wildcard tcode.c),tcode.o,prebuild/tcode.o)
endif

OBJS	+= $(if $(wildcard shutils_private.c),shutils_private.o,prebuild/shutils_private.o)

OBJS += $(if $(wildcard spwenc.c), spwenc.o, prebuild/spwenc.o)

ifeq ($(CONFIG_BCMWL5),y)
ifneq ($(RTCONFIG_DSL)$(HND_ROUTER),y)
CFLAGS += -DTRX_NEW
endif
OBJS += private.o
OBJS += wlif_utils.o UdpLib.o bcmutils.o
OBJS += bcmwifi_channels.o
endif

ifeq ($(HND_ROUTER),y)
OBJS += ethswctl.o ethctl_cmd.o
OBJS += memaccess.o
endif

ifeq ($(or $(RTCONFIG_BCM_7114),$(HND_ROUTER)),y)
OBJS += bcm_usched.o bcm_stamon.o
endif

ifeq ($(RTCONFIG_LP5523),y)
OBJS += lp5523led.o
endif

ifeq ($(RTCONFIG_AMAS),y)
CFLAGS += -I$(TOP)/shared/sysdeps/amas
ifeq ($(RTCONFIG_DWB),y)
ifeq ($(RTCONFIG_BCMBSD),y)
CFLAGS += -DSMART_CONNECT
endif
OBJS += $(if $(wildcard sysdeps/amas/amas_dwb.c),amas_dwb.o,prebuild/amas_dwb.o)
endif
vpath %.c sysdeps/amas
endif

vpath %.c sysdeps $(SRCBASE)/shared/ $(WLAN_ComponentSrcDirs)
ifeq ($(RTCONFIG_REALTEK),y)
CFLAGS += -DTRX_NEW
OBJS += private.o
vpath %.c sysdeps/realtek
else ifeq ($(RTCONFIG_RALINK),y)
ifneq ($(RTCONFIG_DSL),y)
CFLAGS += -DTRX_NEW
endif
OBJS += private.o
vpath %.c sysdeps/ralink
else ifeq ($(RTCONFIG_QCA),y)
CFLAGS += $(if $(IPQ40XX),,-DTRX_NEW)
OBJS += $(if $(wildcard sysdeps/qca/private.c),private.o,prebuild/private.o)
vpath %.c sysdeps/qca
else ifeq ($(RTCONFIG_ALPINE),y)
OBJS += private.o
vpath %.c sysdeps sysdeps/alpine
else ifeq ($(RTCONFIG_LANTIQ),y)
OBJS	+= $(if $(wildcard sysdeps/api-lantiq.c),api-lantiq.o,prebuild/api-lantiq.o)
OBJS	+= $(if $(wildcard sysdeps/lantiq/lantiq.c),lantiq.o,prebuild/lantiq.o)
OBJS	+= $(if $(wildcard sysdeps/lantiq/private.c),private.o,prebuild/private.o)
vpath %.c sysdeps/lantiq
else ifeq ($(RTCONFIG_QTN),y)
vpath %.c sysdeps sysdeps/broadcom sysdeps/qtn $(SRCBASE)/shared/ $(SRCBASE)/shared/bcmwifi/src $(WLAN_StdSrcDirsA) $(WLAN_ComponentSrcDirs) $(TOP)/libqcsapi_client/qcsapi_rpc_common/client  $(TOP)/libqcsapi_client/qcsapi_rpc_common/common
else
vpath %.c sysdeps/broadcom  $(SRCBASE)/shared/bcmwifi/src
endif

ifeq ($(RTCONFIG_SW_HW_AUTH),y)
OBJS += sw_auth.o hw_auth.o
CFLAGS  += -I$(TOP)/sw-hw-auth
vpath %.c $(TOP)/sw-hw-auth
endif

all: libshared.so libshared.a

libshared.so: $(OBJS)
	@echo " [shared] LD $@"
ifeq ($(HND_ROUTER),y)
	@$(CC) $(LDFLAGS) $(EXTRA_LD_FLAGS) -shared -o $@ $^
else
	@$(LD) $(LDFLAGS) -shared -o $@ $^
endif

	$(SIZECHECK)
	$(CPTMP)

libshared.a: $(OBJS)
	@echo " [shared] AR $@"
	@rm -f $@
	@$(AR) cru $@ $^

install: all
	@echo " [shared] Installing as $(INSTALLDIR)/usr/lib/libshared.so"
	@install -d $(INSTALLDIR)/usr/lib
	@install -m 755 libshared.so $(INSTALLDIR)/usr/lib
	@$(STRIP) $(INSTALLDIR)/usr/lib/libshared.so

clean:
	rm -f *.o *.so *.a .*.depend *.prep sysdeps/*.o sysdeps/broadcom/*.o sysdeps/ralink/*.o sysdeps/qtn/*.o

private.o:
ifneq ($(wildcard ./prebuild/private.o),)
	@-cp -f ./prebuild/private.o .
endif

sw_auth.o:
ifeq ($(wildcard $(SRCBASE)/router/sw-hw-auth/*.c),)
	cp -f $(TOP)/sw-hw-auth/prebuild/sw_auth.o ./
endif

hw_auth.o:
ifeq ($(wildcard $(SRCBASE)/router/sw-hw-auth/*.c),)
	cp -f $(TOP)/sw-hw-auth/prebuild/hw_auth.o ./
endif

ifeq ($(RTCONFIG_USB),y)
vpath %.c $(sort $(dir $(ID_OBJS)))

$(notdir $(ID_OBJS)): %.o: %.c .%.depend
	@echo " [shared] CC $@"
	@$(CC) $(CFLAGS) $(ID_CFLAGS) -o $@ -c $<

$(patsubst %.o,.%.depend,$(notdir $(ID_OBJS))): .%.depend: %.c
	@$(CC) $(CFLAGS) $(ID_CFLAGS) -M $< > $@

# ld -r --gc-sections is broken, so don't use it for now
#volume_id.lib.o: $(notdir $(ID_OBJS))
#	@echo " [shared] LD $@"
#	@$(LD) -r -e find_label_or_uuid --gc-sections -o $@ $(notdir $(ID_OBJS))
endif

%.o: %.c .%.depend
	@echo " [shared] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

prebuild/%.o:
	#nothing to do...

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

.defaults.depend .tcode.depend .hw_auth.depend: .%.depend: %.c %.prep
	@$(CC) $(CFLAGS) -M $< > $@

%.prep: %.c
	@$(CC) $(CFLAGS) -E $< > $@

-include $(OBJS:%.o=.%.depend)
