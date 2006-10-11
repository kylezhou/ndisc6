/*
 * tcptraceroute.c - tcptraceroute-like wrapper around rltraceroute6
 * $Id$
 */

/***********************************************************************
 *  Copyright (C) 2005-2006 Rémi Denis-Courmont.                       *
 *  This program is free software; you can redistribute and/or modify  *
 *  it under the terms of the GNU General Public License as published  *
 *  by the Free Software Foundation; version 2 of the license.         *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *  See the GNU General Public License for more details.               *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, you can get it from:              *
 *  http://www.gnu.org/copyleft/gpl.html                               *
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>


#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

static int
quick_usage (const char *path)
{
	fprintf (stderr, _("Try \"%s -h\" for more information.\n"), path);
	return 2;
}


static int
usage (const char *path)
{
	printf (_(
"Usage: %s [options] <IPv6 hostname/address> [port number]\n"
"Print IPv6 network route to a host\n"), path);

	puts (_("\n"
"  -A  send TCP ACK probes\n"
"  -d  enable socket debugging\n"
"  -E  set TCP Explicit Congestion Notification bits in probe packets\n"
"  -f  specify the initial hop limit (default: 1)\n"
"  -g  insert a route segment within a \"Type 0\" routing header\n"
"  -h  display this help and exit\n"
"  -i  force outgoing network interface\n"
//"  -l  display incoming packets hop limit\n" -- FIXME
"  -l  set probes byte size\n"
"  -m  set the maximum hop limit (default: 30)\n"
"  -N  perform reverse name lookups on the addresses of every hop\n"
"  -n  don't perform reverse name lookup on addresses\n"
"  -p  override source TCP port\n"
"  -q  override the number of probes per hop (default: 3)\n"
"  -r  do not route packets\n"
"  -S  send TCP SYN probes (default)\n"
"  -s  specify the source IPv6 address of probe packets\n"
"  -t  set traffic class of probe packets\n"
"  -V, --version  display program version and exit\n"
/*"  -v, --verbose  display all kind of ICMPv6 errors\n"*/
"  -w  override the timeout for response in seconds (default: 5)\n"
"  -z  specify a time to wait (in ms) between each probes (default: 0)\n"
	));

	return 0;
}


static int
version (void)
{
	printf (_("tcptraceroute6: TCP/IPv6 traceroute tool %s (%s)\n"
	          " built %s on %s\n"), VERSION, "$Rev$",
	         __DATE__, PACKAGE_BUILD_HOSTNAME);
	printf (_("Configured with: %s\n"), PACKAGE_CONFIGURE_INVOCATION);
	puts (_("Written by Remi Denis-Courmont\n"));

	printf (_("Copyright (C) %u-%u Remi Denis-Courmont\n"
"This is free software; see the source for copying conditions.\n"
"There is NO warranty; not even for MERCHANTABILITY or\n"
"FITNESS FOR A PARTICULAR PURPOSE.\n"), 2005, 2006);
	return 0;
}


static const struct option opts[] =
{
	{ "ack",      no_argument,       NULL, 'A' },
	{ "debug",    no_argument,       NULL, 'd' },
	{ "ecn",      no_argument,       NULL, 'E' },
	// -F is a stub
	{ "first",    required_argument, NULL, 'f' },
	{ "segment",  required_argument, NULL, 'g' },
	{ "help",     no_argument,       NULL, 'h' },
	{ "iface",    required_argument, NULL, 'i' },
	{ "length",   required_argument, NULL, 'l' },
	{ "max",      required_argument, NULL, 'm' },
	// -N is not really a stub, should have a long name
	{ "numeric",  no_argument,       NULL, 'n' },
	{ "port",     required_argument, NULL, 'p' },
	{ "retry",    required_argument, NULL, 'q' },
	{ "noroute",  no_argument,       NULL, 'r' },
	{ "syn",      no_argument,       NULL, 'S' },
	{ "source",   required_argument, NULL, 's' },
	{ "tclass",   required_argument, NULL, 't' },
	{ "version",  no_argument,       NULL, 'V' },
	/*{ "verbose",  no_argument,       NULL, 'v' },*/
	{ "wait",     required_argument, NULL, 'w' },
	// -x is a stub
	{ "delay",    required_argument, NULL, 'z' },
	{ NULL,       0,                 NULL, 0   }
};


static const char optstr[] = "AdEFf:g:hi:l:m:Nnp:q:rSs:t:Vw:xz:";

int main (int argc, char *argv[])
{
	/* Prepare big enough buffers */
	unsigned len = 0;
	for (int i = 0; i < argc; i++)
		len += strlen (argv[i]);

	char optbuf[3 * len + argc], *buf = optbuf;
	char *optv[argc + len + /* "-S", "-p", NULL */ 3];
	char *psize = NULL;

	int val, optc = 0;

	optv[optc++] = MYEXECDIR"/rltraceroute6";
	optv[optc++] = "-S";

	while ((val = getopt_long (argc, argv, optstr, opts, NULL)) != EOF)
	{
		assert (optbuf + sizeof (optbuf) > buf);
		assert ((sizeof (optv) / sizeof (optv[0])) > (unsigned)optc);

		char name;

		switch (val)
		{
			case 'h':
				return usage (argv[0]);

			case 'V':
				return version ();

			case '?':
				return quick_usage (argv[0]);

			case 'l': /* Packet size */
				psize = optarg;
				continue;

			case 'p': /* Source port number */
				name = 'P'; // traceroute6 secret name for this
				break;

			default:
				name = (char)val;
		}

		assert (strchr (optstr, val) != NULL);

		optv[optc++] = buf;
		*buf++ = '-';
		*buf++ = name;
		*buf++ = '\0';

		if ((strchr (optstr, val))[1] != ':')
			continue; // no_argument option

		optv[optc++] = optarg;
	}

	switch (argc - optind)
	{
		case 2:
		case 1:
			break;

		default:
			return quick_usage (argv[0]);
	}

	/* Destination host */
	char *dsthost = argv[optind++];

	/* Destination port number */
	optv[optc++] = "-p";
	optv[optc++] = (optind < argc) ? argv[optind] : "80";
	optind++;

	/* Inserts destination */
	if (dsthost != NULL)
		optv[optc++] = dsthost;

	/* Inserts packet size */
	if (psize != NULL)
		optv[optc++] = psize;

	assert (optbuf + sizeof (optbuf) >= buf);
	assert ((sizeof (optv) / sizeof (optv[0])) > (unsigned)optc);

	optv[optc] = NULL;

	execv (optv[0], optv);
	perror (optv[0]);
	return 2;
}
