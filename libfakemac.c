/*-
 * Copyright (c) 2011 Ed Schouten <ed@kumina.nl>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define	_GNU_SOURCE

#include <sys/ioctl.h>

#include <net/if.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static unsigned char mac[6];
static int (*orig_ioctl)(int, unsigned long, void *);

static int
hex_to_bin(char ch)
{

	if (ch >= '0' && ch <= '9')
		return (ch - '0');
	if (ch >= 'A' && ch <= 'F')
		return (ch - 'A' + 10);
	if (ch >= 'a' && ch <= 'f')
		return (ch - 'a' + 10);
	return (-1);
}

static int
mac_pton(const char *s, unsigned char mac[6])
{
	size_t i;
	int r;

	for (i = 0; i < 6; i++) {
		if ((r = hex_to_bin(s[i * 3])) == -1)
			return (-1);
		mac[i] = r << 4;
		if ((r = hex_to_bin(s[i * 3 + 1])) == -1)
			return (-1);
		mac[i] |= r;
		if (s[i * 3 + 2] != (i == 5 ? '\0' : ':'))
			return (-1);
	}
	return (0);
}

static void
initialize(void)
{
	static int initialized = 0;
	const char *macstr;

	if (!initialized) {
		/* Obtain original ioctl(2) function. */
		orig_ioctl = dlsym(RTLD_NEXT, "ioctl");
		if (orig_ioctl == NULL) {
			fprintf(stderr, "%s", dlerror());
			exit(1);
		}

		/* Parse MAC address. */
		macstr = getenv("MAC_ADDRESS");
		if (macstr == NULL) {
			fprintf(stderr, "MAC_ADDRESS is not set\n");
			exit(1);
		}
		if (mac_pton(macstr, mac) != 0) {
			fprintf(stderr,
			    "The supplied MAC address is invalid\n");
			exit(1);
		}

		initialized = 1;
	}
}

int
ioctl(int fd, unsigned long request, ...)
{
	struct ifreq *ifr;
	va_list va;
	int ret;

	initialize();

	/* Obtain optional parameter through va_arg(). */
	va_start(va, request);
	ifr = va_arg(va, void *);
	va_end(va);

	if ((ret = orig_ioctl(fd, request, ifr)) != 0)
		return (ret);
	if (request == SIOCGIFHWADDR)
		memcpy(ifr->ifr_hwaddr.sa_data, mac, sizeof(mac));
	return (0);
}
