#ifndef CCAN_CONFIG_H
#define CCAN_CONFIG_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* Always use GNU extensions. */
#endif

#define HAVE_BYTESWAP_H 0
#define HAVE_BSWAP_64 0
#define HAVE_BIG_ENDIAN 0
#define HAVE_LITTLE_ENDIAN 1

#endif /* CCAN_CONFIG_H */

