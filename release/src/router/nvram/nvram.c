/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * Frontend command-line utility for Linux NVRAM layer
 *
 * Copyright 2004, ASUSTeK Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvram.c,v 1.1 2007/06/08 10:22:42 arthur Exp $
 */

#define _GNU_SOURCE	// for stpcpy()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>	// for sleep()
#include <typedefs.h>
#include <stdint.h>

#include <rtconfig.h>
#include <bcmnvram.h>
#include <shared.h>

#include "nvram_mode.h"

#define PROTECT_CHAR	'x'

static int export_mode(char* mode, char* buf_ap, char* buf)
{
	const char **conf = NULL, **hold = NULL;
	const wlc_conf_t *wlc = NULL;
	char *ptr, *item, *value;
	char name[128], nv[128];
	int i, len;

	if (!buf_ap || !buf)
		return -1;

	if (strcmp(mode, "save_ap") == 0) {
		conf = conf_ap;
		hold = hold_ap;
	} else if (strcmp(mode, "save_rp_2g") == 0) {
		conf = conf_rp_2g;
		hold = hold_rp_2g;
		wlc = wlc_rp_2g;
	} else if (strcmp(mode, "save_rp_5g") == 0) {
		conf = conf_rp_5g;
		hold = hold_rp_5g;
		wlc = wlc_rp_5g;
	} else if (strcmp(mode, "save_rp_5g2") == 0) {
		conf = conf_rp_5g;
		hold = hold_rp_5g;
		wlc = wlc_rp_5g2;
	} else
		return -1;

	ptr = buf_ap;

	for (i = 0; conf[i]; i++) {
#ifdef ASUS_DEBUG
		puts(conf[i]);
#endif
		ptr = stpcpy(ptr, conf[i]) + 1;
	}

	for (item = buf; *item; item += strlen(item) + 1) {
		value = strchr(item, '=');
		if (!value)
			continue;
		len = value - item;
		if (len < 0 || len > sizeof(name) - 1)
			continue;

		strncpy(name, item, len);
		name[len] = '\0';
		value++;

		for (i = 0; hold[i]; i++) {
			if (strcmp(name, hold[i]) == 0) {
				snprintf(nv, sizeof(nv), "%s=%s", name, value);
#ifdef ASUS_DEBUG
				puts(nv);
#endif
				ptr = stpcpy(ptr, nv) + 1;
				break;
			}
		}

		// save wlc settings
		if (wlc) {
			for (i = 0; wlc[i].key_assign; i++) {
				if (strcmp(name, wlc[i].key_from) == 0) {
					snprintf(nv, sizeof(nv), "%s=%s", wlc[i].key_assign, value);
#ifdef ASUS_DEBUG
					puts(nv);
#endif
					ptr = stpcpy(ptr, nv) + 1;
				}
			}
		}
	}

	return 0;
}

#ifdef RTCONFIG_NVRAM_ENCRYPT
static int nvram_dec_all(char* buf_ap, char* buf)
{
	struct nvram_tuple *t;
	extern struct nvram_tuple router_defaults[];
	char *ptr, *item, *value;
	char name[128], nv[65535];
	int len;
	char output[NVRAM_ENC_MAXLEN];

	memset(output, 0, sizeof(output));


	if (!buf_ap || !buf)
		return -1;

	ptr = buf_ap;

	for (item = buf; *item; item += strlen(item) + 1) {
		value = strchr(item, '=');
		if (!value)
			continue;
		len = value - item;
		if (len < 0 || len > sizeof(name) - 1)
			continue;

		strncpy(name, item, len);
		name[len] = '\0';
		value++;

		for (t = router_defaults; t->name; t++)
		{
			if (strcmp(name, t->name) == 0 && t->enc == 1) {
				dec_nvram(t->name, value, output);
				value = output;
			}
		}

		snprintf(nv, sizeof(nv), "%s=%s", name, value);
		ptr = stpcpy(ptr, nv) + 1;
#ifdef ASUS_DEBUG
				puts(nv);
#endif
	}

	return 0;
}
#endif

/*******************************************************************
* NAME: _secure_romfile
* AUTHOR: Andy Chiu
* CREATE DATE: 2015/06/08
* DESCRIPTION: replace account /password by PROTECT_CHAR with the same string length
* INPUT:  path: the rom file path
* OUTPUT:
* RETURN:  0: success, -1:failed
* NOTE: Andy Chiu, 2015/12/18. Add new tokens.
*           Andy Chiu, 2015/12/24. Add new tokens.
*	     Andy Chiu, 2016/02/18. Add new token, wtf_username.
*******************************************************************/
/* memset list field.
 * member mask is bitmask: 0 - 1st, 2 - 2nd, ... 0x80000000 - 31th member */
static char *nvlist_memset(char *list, int c, unsigned int member_mask)
{
	char *item, *end, *next;
	char *mitem, *mend, *mnext;
	unsigned int mask;

	if (!list || member_mask == 0)
		return list;

	for (item = list; *item; ) {
		end = strchr(item, '<');
		if (end)
			next = end + 1;
		else	next = end = strchr(item, 0);
		for (mitem = item, mask = member_mask;
		     mitem < end && mask; mask >>= 1) {
		        mend = memchr(mitem, '>', end - mitem);
			if (mend)
				mnext = mend + 1;
			else	mnext = mend = end;
			if (mask & 1)
				memset(mitem, c, mend - mitem);
			mitem = mnext;
		}
		item = next;
	}

	return list;
}

static int _secure_conf(char* buf)
{
	char name[128], *value, *item;
	int len, i;

	//name contains token
	const char *keyword_token[] = {"http_username", "passwd", "password",
		NULL};	//Andy Chiu, 2015/12/18

	//name is token
	const char *token1[] = {"wan_pppoe_passwd", "modem_pass", "modem_pincode",
		"http_passwd", "wan0_pppoe_passwd", "dslx_pppoe_passwd", "ddns_passwd_x",
		"wl_wpa_psk",	"wlc_wpa_psk",  "wlc_wep_key",
		"wl0_wpa_psk", "wl0.1_wpa_psk", "wl0.2_wpa_psk", "wl0.3_wpa_psk",
		"wl1_wpa_psk", "wl1.1_wpa_psk", "wl1.2_wpa_psk", "wl1.3_wpa_psk",
		"wl0.1_key1", "wl0.1_key2", "wl0.1_key3", "wl0.1_key4",
		"wl0.2_key1", "wl0.2_key2", "wl0.2_key3", "wl0.2_key4",
		"wl0.3_key1", "wl0.3_key2", "wl0.3_key3", "wl0.3_key4",
		"wl0_key1", "wl0_key2", "wl0_key3", "wl0_key4",
		"wl1.1_key1", "wl1.1_key2", "wl1.1_key3", "wl1.1_key4",
		"wl1.2_key1", "wl1.2_key2", "wl1.2_key3", "wl1.2_key4",
		"wl1.3_key1", "wl1.3_key2", "wl1.3_key3", "wl1.3_key4",
		"wl_key1", "wl_key2", "wl_key3", "wl_key4",
		"wl1_key1", "wl1_key2", "wl1_key3", "wl1_key4",
		"wl0_phrase_x", "wl0.1_phrase_x", "wl0.2_phrase_x", "wl0.3_phrase_x",
		"wl1_phrase_x", "wl1.1_phrase_x", "wl1.2_phrase_x", "wl1.3_phrase_x",
		"wl_phrase_x", "vpnc_openvpn_pwd", "PM_SMTP_AUTH_USER", "PM_MY_EMAIL",
		"PM_SMTP_AUTH_PASS", "wtf_username", "ddns_hostname_x", "ddns_username_x",
		NULL};

	//name is token
	//value is [<]username>password<username...
	const char *token2[] = {"acc_list", "pptpd_clientlist", "vpn_serverx_clientlist",
		NULL};

	//name is token
	//valus is [<]desc>type>index>username>password<desc...
	const char vpnc_token[] = "vpnc_clientlist";

	//name is token
	//value is [<]type>username>password>url>rule>dir>enable<type...
	const char cloud_token[] = "cloud_sync";

	//name is token
	//value is username@domain.tld
	const char pppoe_username_token[] = "pppoe_username";

	if (!buf)
		return -1;

	for (item = buf; *item; item += strlen(item) + 1) {
		value = strchr(item, '=');
		if (!value)
			continue;
		len = value - item;
		if (len < 0 || len > sizeof(name) - 1)
			continue;

		strncpy(name, item, len);
		name[len] = '\0';
		value++;

		//check the password keyword token
		for (i = 0; keyword_token[i]; i++) {
			if (strstr(name, keyword_token[i]) != NULL) {
				memset(value, PROTECT_CHAR, strlen(value));
				goto next;
			}
		}

		//check the first token group
		for (i = 0; token1[i]; i++) {
			if (strcmp(name, token1[i]) == 0) {
				memset(value, PROTECT_CHAR, strlen(value));
				goto next;
			}
		}

		//check the 2nd token group
		//value is [<]username>password<username...
		for (i = 0; token2[i]; i++) {
			if (strcmp(name, token2[i]) == 0) {
				nvlist_memset(value, PROTECT_CHAR, (1u << 1));
				goto next;
			}
		}

		//check vpnc token
		//valus is [<]desc>type>index>username>password<desc...
		if (strcmp(name, vpnc_token) == 0) {
			nvlist_memset(value, PROTECT_CHAR, (1u << 4));
			goto next;
		}

		//check cloud sync token
		//value is [<]type>xxx1>xxx2>xxx3>xxx4>xxx5>xxx6<type...
		if (strcmp(name, cloud_token) == 0) {
			nvlist_memset(value, PROTECT_CHAR, (1u << 2)|(1u << 3)|(1u << 5));
			goto next;
		}

		//check
		//value is password@domain.tld
		if (strstr(name, pppoe_username_token) != NULL) {
			char *e = strchr(value, '@') ? : strchr(value, 0);
			memset(value, PROTECT_CHAR, e - value);
		}

	next:
		continue;
	}

	return 0;
}

static void
usage(void)
{
	fprintf(stderr, 
		"usage: nvram [get name] [set name=value] [unset name] [erase] [show] [save file] [restore file] [fb_save file]\n"
		"usage: nvram [save_ap file] [save_rp_2g file] [save_rp_5g file]\n");
	exit(0);
}

#define PROFILE_HEADER		"HDR1"
#ifdef RTCONFIG_DSL
#define PROFILE_HEADER_NEW      "N55U"
#else
#ifdef RTCONFIG_QCA
#define PROFILE_HEADER_NEW      "AC55U"
#elif defined(RTCONFIG_LANTIQ)
#define PROFILE_HEADER_NEW      "BLUE"
#else
#define PROFILE_HEADER_NEW      "HDR2"
#endif
#endif


// save nvram to file
int nvram_save(char *file, char *buf)
{
	FILE *fp;
	char *name;
	unsigned long count, filelen, i;

   	if ((fp=fopen(file, "w"))==NULL) return -1;

	count = 0;
	for (name=buf;*name;name+=strlen(name)+1)
	{	
		puts(name);
		count = count+strlen(name)+1;
	}
   	
   	filelen = count + (1024 - count%1024);	
   	fwrite(PROFILE_HEADER, 1, 4, fp);
   	fwrite(&filelen, 1, 4, fp);
   	fwrite(buf, 1, count, fp);
   	for (i=count;i<filelen;i++) fwrite(name, 1, 1, fp);   	
   	fclose(fp);
	return 0;
}

unsigned char get_rand()
{
	unsigned char buf[1];
	FILE *fp;
//	size_t rc;

	fp = fopen("/dev/urandom", "r");
	if (fp == NULL) {
#ifdef ASUS_DEBUG
		fprintf(stderr, "Could not open /dev/urandom.\n");
#endif
		return 0;
	}
	fread(buf, 1, 1, fp);
	fclose(fp);

	return buf[0];
}

int nvram_save_new(char *file, char *buf)
{
	FILE *fp;
	char *name;
#if defined(RTCONFIG_SAVEJFFS)
	uint32_t filelen_le32;
#endif
	unsigned long count, filelen, i;
	unsigned char rand = 0, temp;

	if ((fp = fopen(file, "w")) == NULL) return -1;

	count = 0;
	for (name = buf; *name; name += strlen(name) + 1)
	{
#ifdef ASUS_DEBUG
		puts(name);
#endif
		count = count + strlen(name) + 1;
	}

	filelen = count + (1024 - count % 1024);
#if 1
	//workaround for do_upload_post waitfor() timeout
	filelen = filelen >= 1024*3 ? filelen : 1024*3;
#endif

	rand = get_rand() % 30;
#ifdef ASUS_DEBUG
	fprintf(stderr, "random number: %x\n", rand);
#endif
	fwrite(PROFILE_HEADER_NEW, 1, 4, fp);
#if defined(RTCONFIG_SAVEJFFS)
	filelen_le32 = cpu_to_le32(filelen);
	fwrite(&filelen_le32, 1, 3, fp);
#else
	fwrite(&filelen, 1, 3, fp);
#endif
	fwrite(&rand, 1, 1, fp);
#ifdef ASUS_DEBUG
	for (i = 0; i < 4; i++)
	{
		fprintf(stderr, "%2x ", PROFILE_HEADER_NEW[i]);
	}
	for (i = 0; i < 3; i++)
	{
		fprintf(stderr, "%2x ", ((char *)&filelen)[i]);
	}
	fprintf(stderr, "%2x ", ((char *)&rand)[0]);
#endif
	for (i = 0; i < count; i++)
	{
		if (buf[i] == 0x0)
			buf[i] = 0xfd + get_rand() % 3;
		else
			buf[i] = 0xff - buf[i] + rand;
	}
	fwrite(buf, 1, count, fp);
#ifdef ASUS_DEBUG
	for (i = 0; i < count; i++)
	{
		if (i % 16 == 0) fprintf(stderr, "\n");
		fprintf(stderr, "%2x ", (unsigned char) buf[i]);
	}
#endif
	for (i = count; i < filelen; i++)
	{
		temp = 0xfd + get_rand() % 3;
		fwrite(&temp, 1, 1, fp);
#ifdef ASUS_DEBUG
		if (i % 16 == 0) fprintf(stderr, "\n");
		fprintf(stderr, "%2x ", (unsigned char) temp);
#endif
	}
	fclose(fp);
	return 0;
}

int issyspara(char *p)
{
	struct nvram_tuple *t/*eric--, *u*/;
	extern struct nvram_tuple router_defaults[];
	
	// skip checking for wl[]_, wan[], lan[]_
	if(strstr(p, "wl") || strstr(p, "wan") || strstr(p, "lan")
		|| strstr(p, "vpn_server") || strstr(p, "vpn_client")
	)
		return 1;

	for (t = router_defaults; t->name; t++)
	{
		if (strstr(p, t->name))
			break;
	
	}

	if (t->name) return 1;
	else return 0;
}

// restore nvram from file
int nvram_restore(char *file, char *buf)
{
   	FILE *fp;
   	char header[8], *p, *v;
  	unsigned long count, *filelen;

   	if ((fp=fopen(file, "r+"))==NULL) return -1;
    	   
   	count = fread(header, 1, 8, fp);
   	if (count>=8 && strncmp(header, PROFILE_HEADER, 4)==0)
   	{  
	    filelen = (unsigned long *)(header + 4);
   	    count = fread(buf, 1, *filelen, fp);
   	}   
   	fclose(fp);

   	p = buf;       		   
   
   	while (*p)
   	{       
		//printf("nv:%s\n", p);     	     			     	
       		v = strchr(p, '=');

		if (v!=NULL)
		{	
			*v++ = '\0' /*eric--NULL*/;

			if (issyspara(p))
			{
				nvram_set(p, v);
			}

       			p = v + strlen(v) + 1;			
		}
		else 
		{
			nvram_unset(p);
			p = p + 1;
		}
   	}

	nvram_set("x_Setting", "1");
	return 0;
}

int nvram_restore_new(char *file, char *buf)
{
	FILE *fp;
	char header[8], *p, *v;
	uint32_t *filelenptr;
	unsigned long count, filelen, i;
	unsigned char rand, *randptr;

	if ((fp = fopen(file, "r+")) == NULL) return -1;

	count = fread(header, 1, 8, fp);
	if (count>=8 && strncmp(header, PROFILE_HEADER, 4) == 0)
	{
		filelenptr = (uint32_t *)(header + 4);
#ifdef ASUS_DEBUG
		fprintf(stderr, "restoring original text cfg of length %x\n", *filelenptr);
#endif
		fread(buf, 1, *filelenptr, fp);
	}
	else if (count>=8 && strncmp(header, PROFILE_HEADER_NEW, 4) == 0)
	{
		filelenptr = (uint32_t *)(header + 4);
#if defined(RTCONFIG_SAVEJFFS)
		filelen = le32_to_cpu(*filelenptr) & 0xffffff;
#else
		filelen = *filelenptr & 0xffffff;
#endif
#ifdef ASUS_DEBUG
		fprintf(stderr, "restoring non-text cfg of length %x\n", filelen);
#endif
		randptr = (unsigned char *)(header + 7);
		rand = *randptr;
#ifdef ASUS_DEBUG
		fprintf(stderr, "non-text cfg random number %x\n", rand);
#endif
		count = fread(buf, 1, filelen, fp);
#ifdef ASUS_DEBUG
		fprintf(stderr, "non-text cfg count %x\n", count);
#endif
		for (i = 0; i < count; i++)
		{
			if ((unsigned char) buf[i] > ( 0xfd - 0x1)){
				/* e.g.: to skip the case: 0x61 0x62 0x63 0x00 0x00 0x61 0x62 0x63 */
				if(i > 0 && buf[i-1] != 0x0)
					buf[i] = 0x0;
			}
			else
				buf[i] = 0xff + rand - buf[i];
		}
#ifdef ASUS_DEBUG
		for (i = 0; i < count; i++)
		{
			if (i % 16 == 0) fprintf(stderr, "\n");
			fprintf(stderr, "%2x ", (unsigned char) buf[i]);
		}

		for (i = 0; i < count; i++)
		{
			if (i % 16 == 0) fprintf(stderr, "\n");
			fprintf(stderr, "%c", buf[i]);
		}
#endif
	}
	else
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);

	p = buf;

	while (*p || p-buf<=count)
	{
#if 1
		/* e.g.: to skip the case: 00 2e 30 2e 32 38 00 ff 77 61 6e */
		if (!isprint(*p)) {
			p = p + 1;
			continue;
		}
#endif
		v = strchr(p, '=');

		if (v != NULL)
		{
			*v++ = '\0';

			if (issyspara(p))
				nvram_set(p, v);

			p = v + strlen(v) + 1;
		}
		else
		{
			nvram_unset(p);
			p = p + 1;
		}
	}
	return 0;
}

/* NVRAM utility */
int
main(int argc, char **argv)
{
	char *name, *value, *buf;
	int size;
#ifdef RTCONFIG_CFE_NVRAM_CHK
	int ret = 0;
	FILE *fp;
#endif

	/* Skip program name */
	--argc;
	++argv;

	if (!*argv) 
		usage();
	
	buf = malloc (MAX_NVRAM_SPACE);
	if (buf == NULL)	{
		perror ("Out of memory!\n");
		return -1;
	}
	
	/* Process the remaining arguments. */
	for (; *argv; argv++) {
		if (!strncmp(*argv, "get", 3)) {
			if (*++argv) {
				if ((value = nvram_get(*argv)))
					puts(value);
			}
		}
		else if (!strncmp(*argv, "set", 3)) {
			if (*++argv) {
				strncpy(value = buf, *argv, MAX_NVRAM_SPACE);
				name = strsep(&value, "=");
				
#ifdef RTCONFIG_CFE_NVRAM_CHK
				ret = nvram_set(name, value);
				if(ret == 2) {
					fp = fopen("/var/log/cfecommit_ret", "w");
					if(fp!=NULL) {
		                                fprintf(fp,"Illegal nvram\n");
						fclose(fp);
					}
					puts("Illegal nvram format!");
				}
#else
				nvram_set(name, value);
#endif
			}
		}
		else if (!strncmp(*argv, "unset", 5)) {
			if (*++argv)
				nvram_unset(*argv);
		}
		else if (!strncmp(*argv, "commit", 5)) {
			nvram_commit();
		}
		else if (!strcmp(*argv, "save"))
		{
			if (*++argv)
			{
				nvram_getall(buf, MAX_NVRAM_SPACE);
#ifdef RTCONFIG_NVRAM_ENCRYPT
				char *tmp_dnv = malloc(MAX_NVRAM_SPACE);
				if (!tmp_dnv) {
					fprintf(stderr, "Can NOT alloc memory!!!");
					return 0;
				}
				memset(tmp_dnv, 0, MAX_NVRAM_SPACE);
				nvram_dec_all(tmp_dnv, buf);
				nvram_save_new(*argv, tmp_dnv);
				free(tmp_dnv);
#else
				nvram_save_new(*argv, buf);
#endif
			}
		}
		else if (!strncmp(*argv, "save_ap", 7) ||
			 !strncmp(*argv, "save_rp", 7))
		{
			char *mode = *argv;
			if (*++argv) {
				char *tmp_export = malloc(MAX_NVRAM_SPACE);
				if (!tmp_export) {
					fprintf(stderr, "Can NOT alloc memory!!!");
					return 0;
				}
				memset(tmp_export, 0, MAX_NVRAM_SPACE);
				nvram_getall(buf, NVRAM_SPACE);
#ifdef RTCONFIG_NVRAM_ENCRYPT
				char *tmp_dnv = malloc(MAX_NVRAM_SPACE);
				if (!tmp_dnv) {
					fprintf(stderr, "Can NOT alloc memory!!!");
					return 0;
				}
				memset(tmp_dnv, 0, MAX_NVRAM_SPACE);
				nvram_dec_all(tmp_dnv, buf);
				export_mode(mode, tmp_export, tmp_dnv);
				free(tmp_dnv);
#else
				export_mode(mode, tmp_export, buf);
#endif
				nvram_save_new(*argv, tmp_export);
				free(tmp_export);
			}
		}
		//Andy Chiu, 2015/06/09
		else if (!strncmp(*argv, "fb_save", 7))
		{
			if (*++argv) {
				char *tmpbuf = malloc(MAX_NVRAM_SPACE);
				if (!tmpbuf) {
					fprintf(stderr, "Can NOT alloc memory!!!");
					return 0;
				}
				nvram_getall(buf, MAX_NVRAM_SPACE);
				memcpy(tmpbuf, buf, MAX_NVRAM_SPACE);
				_secure_conf(tmpbuf);
				nvram_save_new(*argv, tmpbuf);
				free(tmpbuf);
			}
		}
		else if (!strncmp(*argv, "restore", 7)) 
		{
			if (*++argv) 
			{
				nvram_restore_new(*argv, buf);
			}
			
		}
		else if (!strcmp(*argv, "erase"))
		{
			system("nvram_erase");
		}
		else if (!strncmp(*argv, "show", 4) || !strncmp(*argv, "getall", 6)) {
			nvram_getall(buf, MAX_NVRAM_SPACE);
			for (name = buf; *name; name += strlen(name) + 1)
				puts(name);
			size = sizeof(struct nvram_header) + (int) name - (int) buf;
			fprintf(stderr, "size: %d bytes (%d left)\n", size, MAX_NVRAM_SPACE - size);
		}
		if (!*argv)
			break;
	}

	if (buf != NULL)
		free (buf);
	return 0;
}	
