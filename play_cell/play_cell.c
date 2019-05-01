/*
 * play_cell: Read cell from a DVDs VTS program chain and write it to STDOUT.
 *
 *            Should be used in connection with a tool (e.g. qVamps)
 *            which computes the appropriate numbers. Using play_cell instead
 *            of vamps-play_title avoids this anoying "SCR moves backward"
 *            issue with dvdauthor when authoring streams created by vamps.
 *
 * This file is part of Vamps.
 *
 * Vamps is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Vamps is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Vamps; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * Revision history (latest first):
 *
 *
 * 2006/04/15: Fixed some signed/unsigned issues which caused compiler warnings
 *             on some platforms. No funtional changes.
 * 2006/02/26: Introduced support for detection of cell gaps (-g option). This
 *             enables proper evaporation of some strangely authored DVDs.
 * 2006/02/10: Wanted to just add tolerance for read errors - ended up with an 
 *             almost complete re-write...
 * 2004/10/03: Initial.
 */

/*#define DEBUG*/
#define VERSION		"V0.99.2"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include <dvdread/ifo_read.h>
#include <dvdread/nav_read.h>

/* defines */
#define BUF_SECS	512
/*
 * the minimum time for one DVD sector:
 * 2048 byte = 16384 bit @10.08Mbps
 * -> 1.625396825396825ms = (146.2857142857143/90000)s
 *  = ((146 + 85.71428571428571/300)/90000)s
 * ~= ((146 + 86/300)/90000)s
 */
#define SCR_MIN_FEED	((uint64_t) ((146 << 9) | 86))


/* globals */
uint8_t    buf [BUF_SECS * DVD_VIDEO_LB_LEN];
int        max_read_retries;
int        read_error_count;
int        ignore_read_errors;
int        report_cell_gaps;
const char progname [] = "play_cell";


/* prototypes */
int  get_int (const char *);
void play_cell (const char *, int, int, int);
void usage (void);
void warning (const char *, ...);
void fatal (const char *, ...);


int
main (int argc, char *const argv [])
{
  const char *dev;
  int         vts, pgc, i, c;
  static struct option long_options [] =
  {
    { "ignore-errors", 0, NULL, 'i' },
    { "max-retries",   1, NULL, 'r' },
    { "report-gaps",   0, NULL, 'g' },
    { NULL,            0, NULL, 0   }
  };

  opterr = 1;

  while ((c = getopt_long_only (argc, argv, "ir:g", long_options, NULL)) >= 0)
  {
    switch (c)
    {
      case 'i':
	ignore_read_errors = 1;
	break;

      case 'r':
	max_read_retries = get_int (optarg);
	break;

      case 'g':
	report_cell_gaps = 1;
	break;

      default:
	putc ('\n', stderr);
	usage ();
    }
  }

  if (argc - optind < 4)
    usage ();

  i = optind;

  dev  = argv [i++];
  vts  = get_int (argv [i++]);
  pgc  = get_int (argv [i++]);

  while (i < argc)
  {
    int cell = get_int (argv [i++]);
    play_cell (dev, vts, pgc, cell);
  }

  if (read_error_count)
    warning ("encountered a total of %d read error(s)", read_error_count);

  return 0;
}


/* get integer from string */
int
get_int (const char *arg)
{
  char *s;
  int   rv;

  rv = strtol (arg, &s, 10);

  if (*s)
  {
    fprintf (stderr, "Bad integer value: %s\n", arg);
    usage ();
  }

  return rv;
}


static inline int
dvd_read_blocks (int nav, dvd_file_t *fh, int sector, int count, uint8_t *ptr)
{
#ifdef DEBUG
#define V 3
#define S 10

  int        offs;
  static int bad_vobu, bad_offs, nav_sector;

  if (nav)
  {
    nav_sector = sector;

    if (!(++bad_vobu % V) && --bad_offs < 0)
      bad_offs = S;
  }

  offs = sector - nav_sector;

  if (!(bad_vobu % V) && offs <= bad_offs && offs + count > bad_offs)
    return (bad_vobu & 1) * -1;
#else
  nav = nav;
#endif

  return DVDReadBlocks (fh, sector, count, ptr);
}


/* check if ptr points to nav pack */
static inline int
is_nav_pack (uint8_t *ptr)
{
  uint32_t start_code;

  start_code  = (uint32_t) (ptr [0]) << 24;
  start_code |= (uint32_t) (ptr [1]) << 16;
  start_code |= (uint32_t) (ptr [2]) <<  8;
  start_code |= (uint32_t) (ptr [3]);

  if (start_code != 0x000001ba)
    return 0;

  if ((ptr [4] & 0xc0) != 0x40)
    return 0;

  ptr += 14;

  start_code  = (uint32_t) (ptr [0]) << 24;
  start_code |= (uint32_t) (ptr [1]) << 16;
  start_code |= (uint32_t) (ptr [2]) <<  8;
  start_code |= (uint32_t) (ptr [3]);

  if (start_code != 0x000001bb)
    return 0;

  ptr += 24;

  start_code  = (uint32_t) (ptr [0]) << 24;
  start_code |= (uint32_t) (ptr [1]) << 16;
  start_code |= (uint32_t) (ptr [2]) <<  8;
  start_code |= (uint32_t) (ptr [3]);

  if (start_code != 0x000001bf)
    return 0;

  ptr += 986;

  start_code  = (uint32_t) (ptr [0]) << 24;
  start_code |= (uint32_t) (ptr [1]) << 16;
  start_code |= (uint32_t) (ptr [2]) <<  8;
  start_code |= (uint32_t) (ptr [3]);

  if (start_code != 0x000001bf)
    return 0;

  return 1;
}


static inline uint64_t
extract_scr (const uint8_t *ptr)
{
  uint64_t scr;

  scr  = (uint64_t) (ptr [5]       ) >>  1;	/*  6..0  */
  scr |= (uint64_t) (ptr [4] & 0x03) <<  7;	/*  8..7  */
  scr |= (uint64_t) (ptr [4] & 0xf8) <<  6;	/* 13..9  */
  scr |= (uint64_t) (ptr [3]       ) << 14;	/* 21..14 */
  scr |= (uint64_t) (ptr [2] & 0x03) << 22;	/* 23..22 */
  scr |= (uint64_t) (ptr [2] & 0xf8) << 21;	/* 28..24 */
  scr |= (uint64_t) (ptr [1]       ) << 29;	/* 36..29 */
  scr |= (uint64_t) (ptr [0] & 0x03) << 37;	/* 38..37 */
  scr |= (uint64_t) (ptr [0] & 0x38) << 36;	/* 41..39 */

  return scr;
}


static inline void
merge_scr (uint8_t *ptr, uint64_t scr)
{
  ptr [0] &= 0xc4;
  ptr [2] &= 0x04;
  ptr [4] &= 0x04;
  ptr [5] &= 0x01;

  ptr [5] |= (uint8_t) (scr <<  1);
  ptr [4] |= (uint8_t) (scr >>  7) & 0x03;
  ptr [4] |= (uint8_t) (scr >>  6) & 0xf8;
  ptr [3]  = (uint8_t) (scr >> 14);
  ptr [2] |= (uint8_t) (scr >> 22) & 0x03;
  ptr [2] |= (uint8_t) (scr >> 21) & 0xf8;
  ptr [1]  = (uint8_t) (scr >> 29);
  ptr [0] |= (uint8_t) (scr >> 37) & 0x03;
  ptr [0] |= (uint8_t) (scr >> 36) & 0x38;
}


static inline uint64_t
add_scr (uint64_t a, uint64_t b)
{
  uint32_t ext;
  uint64_t bas;

  bas = (a >> 9)    + (b >> 9);
  ext = (a & 0x1ff) + (b & 0x1ff);

  if (ext >= 300)
  {
    ext -= 300;
    bas++;
  }

  return (bas << 9) | ext;
}


static inline void
insert_nav_pack (uint64_t scr, uint8_t *syshdr)
{
  uint8_t *ptr = buf;
  static uint8_t nav_pack1 [] =
  {
    /* pack header: SCR=0, mux rate=10080000bps, stuffing length=0 */
    0, 0, 1, 0xba, 0x44, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x89, 0xc3, 0xf8,
    /* system header */
    0, 0, 1, 0xbb, 0x00, 0x12,
    /* contents of system header filled in at run time (18 bytes) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* PES header for first private stream 2 packet */
    0, 0, 1, 0xbf, 0x03, 0xd4
  };
  static uint8_t nav_pack2 [] =
  {
    /* PES header for second private stream 2 packet */
    0, 0, 1, 0xbf, 0x03, 0xfa
  };

  memcpy (ptr, nav_pack1, sizeof (nav_pack1));
  merge_scr (ptr + 4, scr);
  memcpy (ptr + 20, syshdr, 18);
  ptr += sizeof (nav_pack1);
  memset (ptr, 0, DVD_VIDEO_LB_LEN/2 - sizeof (nav_pack1));
  ptr = buf + DVD_VIDEO_LB_LEN/2;
  memcpy (ptr, nav_pack2, sizeof (nav_pack2));
  ptr += sizeof (nav_pack2);
  memset (ptr, 0, DVD_VIDEO_LB_LEN/2 - sizeof (nav_pack2));
}


/*
 * try to read a nav pack at given sector
 * if this fails and we are ignoring read errors, scan for next nav pack
 * if the end of the cell is reached
 * without having found another nav pack, return -1
 * on success return 0
 */
static inline int
read_nav_pack (dvd_file_t *fh, dsi_t *dsi,
	       unsigned sector, unsigned last_sector, uint64_t last_scr)
{
  int            i, n, offs;
  static uint8_t syshdr [18];

  /* try max_read_retries+1 times */
  for (i = 0; i <= max_read_retries; i++)
  {
    n = dvd_read_blocks (1, fh, sector, 1, buf);

    if (n == 1)
    {
      /* read Ok */
      if (!is_nav_pack (buf))
        fatal ("not a navigation pack at sector %d", sector);

      /* parse contained DSI pack */
      navRead_DSI (dsi, buf + DSI_START_BYTE);

      /* sanity check */
      if (sector != dsi -> dsi_gi.nv_pck_lbn)
	fatal ("bad DSI pack at sector %d", sector);

      /* save contents of system header */
      memcpy (syshdr, buf + 20, sizeof (syshdr));

      return 0;
    }
  }

  if (!ignore_read_errors)
  {
    /* write out an invalid sector to have Vamps fail */
    memset (buf, 0, DVD_VIDEO_LB_LEN);
    fwrite (buf, DVD_VIDEO_LB_LEN, 1, stdout);
    fatal ("read failed for navigation pack at sector %d", sector);
  }

  warning ("read failed for navigation pack at sector %d (ignored)", sector);
  read_error_count++;

  /* scan for next nav pack */
  for (offs = 1; sector + offs <= last_sector; offs++)
  {
    /* again try max_read_retries+1 times for each sector */
    for (i = 0; i <= max_read_retries; i++)
    {
      n = dvd_read_blocks (0, fh, sector + offs, 1, buf);

      if (n == 1 && buf [14] == 0 && buf [15] == 0 && buf [16] == 1)
      {
	/* read Ok */
	switch (buf [17])
	{
	  case 0xbd:
	  case 0xbe:
	  case 0xc0:
	  case 0xc1:
	  case 0xc2:
	  case 0xc3:
	  case 0xc4:
	  case 0xc5:
	  case 0xc6:
	  case 0xc7:
	  case 0xe0:
	    dsi -> dsi_gi.vobu_ea = offs;
	    break;

	  case 0xbb:
	    dsi -> vobu_sri.next_vobu = offs;
	    insert_nav_pack (add_scr (last_scr, SCR_MIN_FEED), syshdr);
	    return 0;

	  default:
	    ;
	}
      }
    }
  }

  /* reached end of cell without any nav pack found */
  return -1;
}


static inline void
insert_dummy_pack (uint64_t scr)
{
  uint8_t *ptr = buf;
  static uint8_t dummy_pack [] =
  {
    /* pack header: SCR=0, mux rate=10080000bps, stuffing length=0 */
    0, 0, 1, 0xba, 0x44, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x89, 0xc3, 0xf8,
    /* PES header for dummy video packet */
    0, 0, 1, 0xe0, 0x07, 0xec, 0x81, 0x00, 0x00
  };

  memcpy (ptr, dummy_pack, sizeof (dummy_pack));
  merge_scr (ptr + 4, scr);
  ptr += sizeof (dummy_pack);
  memset (ptr, 0xff, DVD_VIDEO_LB_LEN - sizeof (dummy_pack));
}


static inline int
read_blocks (dvd_file_t *fh, int sector, int count, uint64_t last_scr)
{
  int i, n;

  /* ensure, we do not read beyond the buffer size */
  if (count > BUF_SECS)
    count = BUF_SECS;

  /*
   * try max_read_tries+2 times:
   * first:              try to read count sectors, if that fails:
   * second:             try to read a single sector,
   *                     if that fails (due to bad sector):
   * third to
   * max_read_retries+2: retry the sector
   */
  for (i = 0; i < max_read_retries + 2; i++)
  {
    n = dvd_read_blocks (0, fh, sector, count, buf);

    if (n == count)
      /* this is what we want */
      return n;

    /* set sector count to 1 for all retries */
    count = 1;
  }

  /* all retries did not help */

  if (!ignore_read_errors)
  {
    /* write out an invalid sector to have Vamps fail */
    memset (buf, 0, DVD_VIDEO_LB_LEN);
    fwrite (buf, DVD_VIDEO_LB_LEN, 1, stdout);
    fatal ("read failed at sector %d", sector);
  }

  warning ("read failed at sector %d (ignored)", sector);
  read_error_count++;

  /* insert a dummy video pack instead of bad sector */
  insert_dummy_pack (add_scr (last_scr, SCR_MIN_FEED));

  return 1;
}


static inline void
insert_private_2_pack (uint64_t scr, int cell_gap)
{
  uint8_t *ptr = buf;
  static uint8_t private_2_pack [] =
  {
    /* pack header: SCR=0, mux rate=10080000bps, stuffing length=0 */
    0, 0, 1, 0xba, 0x44, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x89, 0xc3, 0xf8,
    /* PES header for private stream 2 packet */
    0, 0, 1, 0xbf, 0x07, 0xec,
    /* contents of private stream 2 packet */
    'V', 'a', 'm', 'p', 's', '-', 'd', 'a', 't', 'a', '\0', 0x01, 0, 0
  };

  memcpy (ptr, private_2_pack, sizeof (private_2_pack));
  merge_scr (ptr + 4, scr);
  ptr   += sizeof (private_2_pack) - 2;
  *ptr++ = (uint8_t) (cell_gap >> 8);
  *ptr++ = (uint8_t) cell_gap;
  memset (ptr, 0, DVD_VIDEO_LB_LEN - sizeof (private_2_pack));
}


void
play_cell (const char *dev, int vts_num, int pgc_num, int cell)
{
  pgc_t *       pgc;
  dvd_reader_t *dvd_handle;
  ifo_handle_t *vts_handle;
  dvd_file_t *  file_handle;
  uint64_t      last_scr = 0;
  int           sector, first_sector, last_sector, next_vobu = 0;

  /* open disc */
  dvd_handle = DVDOpen (dev);

  if (!dvd_handle)
    fatal ("can't open DVD: %s", dev);

  /* load information for the given VTS */
  vts_handle = ifoOpen (dvd_handle, vts_num);

  if (!vts_handle)
    fatal ("can't open info file for VTS %d", vts_num);

  /* open VTS data */
  file_handle = DVDOpenFile (dvd_handle, vts_num, DVD_READ_TITLE_VOBS);

  if (!file_handle)
    fatal ("can't open VOB for VTS %d", vts_num);

  /* check program chain number */
  if (pgc_num < 1 || pgc_num > vts_handle -> vts_pgcit -> nr_of_pgci_srp)
    fatal ("bad program chain: %d, VTS only has %d chains",
	   pgc_num, vts_handle -> vts_pgcit -> nr_of_pgci_srp);

  pgc = vts_handle -> vts_pgcit -> pgci_srp [pgc_num - 1].pgc;

  /* check cell number */
  if (cell < 1 || cell > pgc -> nr_of_cells)
    fatal ("bad cell: %d, PGC only has %d cells", cell, pgc -> nr_of_cells);

  first_sector = pgc -> cell_playback [cell - 1].first_sector;
  last_sector  = pgc -> cell_playback [cell - 1].last_sector;

  /* loop until out of the cell */
  for (sector = first_sector;
       next_vobu != SRI_END_OF_CELL; sector += next_vobu & 0x7fffffff)
  {
    dsi_t    dsi;
    unsigned len;
    int      nsectors, offs, cell_gap;

    /* read nav pack */
    if (read_nav_pack (file_handle, &dsi, sector, last_sector, last_scr) < 0)
      /*
       * reached end of cell while scanning
       * for next nav pack (after read error)
       */
      return;

    /* generate an MPEG2 program stream (including nav packs) */
    if (fwrite (buf, DVD_VIDEO_LB_LEN, 1, stdout) != 1)
      fatal ("write failed: %s", strerror (errno));

    last_scr  = extract_scr (buf + 4);
    nsectors  = dsi.dsi_gi.vobu_ea;
    next_vobu = dsi.vobu_sri.next_vobu;

    /* loop until all sectors of VOBU copied */
    for (offs = 1; nsectors; offs += len, nsectors -= len)
    {
      /* read VOBU sectors */
      len = read_blocks (file_handle, sector + offs, nsectors, last_scr);

      /* write VOBU sectors */
      if (fwrite (buf, DVD_VIDEO_LB_LEN, len, stdout) != len)
	fatal ("write failed: %s", strerror (errno));

      last_scr = extract_scr (buf + (len - 1) * DVD_VIDEO_LB_LEN + 4);
    }

    cell_gap = (next_vobu == SRI_END_OF_CELL ?
		(last_sector - sector + 1) : (next_vobu & 0x7fffffff)) - offs;

    if (report_cell_gaps && cell_gap)
    {
      insert_private_2_pack (last_scr, cell_gap);

      if (fwrite (buf, DVD_VIDEO_LB_LEN, 1, stdout) != 1)
	fatal ("write failed: %s", strerror (errno));
    }
  }
}


void
usage (void)
{
  fputs (
      "play_cell " VERSION "\n"
      "\n"
      "Usage: play_cell [-r max-read-retries] [-i] [-g] dvd-dev vts pgc cell\n"
      "\n"
      "-i: ignore read errors\n"
      "-g: report cell gaps to Vamps\n",
         stderr);
  exit (1);
}


void
warning (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: Warning: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
}


void
fatal (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "%s: Fatal: ", progname);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  putc ('\n', stderr);
  exit (1);
}
