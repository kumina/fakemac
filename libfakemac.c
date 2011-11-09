#define	_GNU_SOURCE

#include <sys/ioctl.h>

#include <net/if.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static unsigned char mac[6];
static int (*orig_ioctl)(int, unsigned long, void *) = NULL;

static int
hex_to_bin(char ch)
{

	if (ch >= '0' && ch <= '9')
		return (ch - '0');
	if (ch >= 'A' && ch <= 'F')
		return (ch - 'A' + 10);
	return (ch - 'a' + 10);
}

static int
mac_pton(const char *s, unsigned char mac[6])
{
	size_t i;

	for (i = 0; i < 6; i++) {
		if (strchr("0123456789abcdefABCDEF", s[i * 3]) == NULL)
			return (-1);
		if (strchr("0123456789abcdefABCDEF", s[i * 3 + 1]) == NULL)
			return (-1);
		if (i != 5 && s[i * 3 + 2] != ':')
			return (-1);
	}
	if (s[17] != '\0')
		return (-1);
	for (i = 0; i < 6; i++)
		mac[i] = (hex_to_bin(s[i * 3]) << 4) | hex_to_bin(s[i * 3 + 1]);
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
