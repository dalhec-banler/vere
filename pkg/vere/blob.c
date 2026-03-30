/// @file

#include "blob.h"
#include "vere.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

//  maximum bytes per single read()/write() call.
//  POSIX allows read()/write() to return EINVAL if count > SSIZE_MAX;
//  macOS returns EINVAL if count > INT_MAX.  cap conservatively at 1 GiB.
//
#define BLOB_IO_MAX  ((size_t)0x40000000UL)

/* _blob_bob_dir(): write path to $pier/.urb/bob/ into [out_c].
*/
static void
_blob_bob_dir(c3_c* out_c, const c3_c* pax_c)
{
  snprintf(out_c, 8192, "%s/.urb/bob", pax_c);
}

/* _blob_mug_dir(): write path to $pier/.urb/bob/<mug>/ into [out_c].
*/
static void
_blob_mug_dir(c3_c* out_c, const c3_c* pax_c, c3_h mug_h)
{
  snprintf(out_c, 8192, "%s/.urb/bob/%" PRIc3_h, pax_c, mug_h);
}

/* _blob_lock_path(): write path to $pier/.urb/bob/<mug>/lock into [out_c].
*/
static void
_blob_lock_path(c3_c* out_c, const c3_c* pax_c, c3_h mug_h)
{
  snprintf(out_c, 8192, "%s/.urb/bob/%" PRIc3_h "/lock", pax_c, mug_h);
}

/* u3_blob_path(): write filesystem path for a blob into [out_c].
*/
void
u3_blob_path(c3_c* out_c, const c3_c* pax_c, c3_h mug_h, c3_w seq_w)
{
  snprintf(out_c, 8192, "%s/.urb/bob/%" PRIc3_h "/%" PRIc3_w,
           pax_c, mug_h, seq_w);
}

/* u3_blob_init(): initialize blob store; create .urb/bob/ if needed.
*/
void
u3_blob_init(const c3_c* pax_c)
{
  c3_c bob_c[8192];
  _blob_bob_dir(bob_c, pax_c);

  if ( 0 != c3_mkdir(bob_c, 0700) && EEXIST != errno ) {
    fprintf(stderr, "blob: failed to create %s: %s\r\n",
            bob_c, strerror(errno));
  }
}

/* _blob_lock_acquire(): acquire mug bucket lock, return next seq number.
**
** Creates the mug directory and lockfile if needed.
** Returns 0 on failure.
*/
static c3_w
_blob_lock_acquire(const c3_c* pax_c, c3_h mug_h)
{
  c3_c dir_c[8192];
  c3_c lck_c[8192];
  _blob_mug_dir(dir_c, pax_c, mug_h);
  _blob_lock_path(lck_c, pax_c, mug_h);

  //  create mug bucket directory if needed
  if ( 0 != c3_mkdir(dir_c, 0700) && EEXIST != errno ) {
    fprintf(stderr, "blob: failed to create bucket %s: %s\r\n",
            dir_c, strerror(errno));
    return 0;
  }

  //  open lockfile, creating if needed
  c3_i lok_i = c3_open(lck_c, O_RDWR | O_CREAT, 0600);
  if ( -1 == lok_i ) {
    fprintf(stderr, "blob: failed to open lock %s: %s\r\n",
            lck_c, strerror(errno));
    return 0;
  }

  //  exclusive advisory lock
  struct flock flk_u = {
    .l_type   = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start  = 0,
    .l_len    = 0,
  };
  if ( -1 == fcntl(lok_i, F_SETLKW, &flk_u) ) {
    fprintf(stderr, "blob: failed to lock %s: %s\r\n",
            lck_c, strerror(errno));
    close(lok_i);
    return 0;
  }

  //  read current next-seq (0 means empty/new file)
  c3_c buf_c[32] = {0};
  ssize_t red_i = read(lok_i, buf_c, sizeof(buf_c) - 1);
  c3_w nex_w = ( red_i > 0 ) ? (c3_w)strtoul(buf_c, 0, 10) : 1;
  if ( 0 == nex_w ) {
    nex_w = 1;
  }

  //  write incremented value back
  if ( -1 == lseek(lok_i, 0, SEEK_SET) ) {
    fprintf(stderr, "blob: lseek failed on %s: %s\r\n",
            lck_c, strerror(errno));
    close(lok_i);
    return 0;
  }
  if ( -1 == ftruncate(lok_i, 0) ) {
    fprintf(stderr, "blob: ftruncate failed on %s: %s\r\n",
            lck_c, strerror(errno));
    close(lok_i);
    return 0;
  }

  c3_c wri_c[32];
  snprintf(wri_c, sizeof(wri_c), "%" PRIc3_w, nex_w + 1);
  if ( -1 == write(lok_i, wri_c, strlen(wri_c)) ) {
    fprintf(stderr, "blob: failed to write lock %s: %s\r\n",
            lck_c, strerror(errno));
    close(lok_i);
    return 0;
  }

  //  fsync and close (releases lock)
  fsync(lok_i);
  close(lok_i);

  return nex_w;
}

/* _blob_dedup(): scan bucket for byte-equal content.
**
** Returns the sequence number of an existing equal blob, or 0 if none.
*/
static c3_w
_blob_dedup(const c3_c* pax_c, c3_h mug_h, c3_w max_w,
            const c3_y* dat_y, c3_d len_d)
{
  for ( c3_w seq_w = 1; seq_w < max_w; seq_w++ ) {
    c3_c fil_c[8192];
    u3_blob_path(fil_c, pax_c, mug_h, seq_w);

    struct stat st_u;
    if ( -1 == stat(fil_c, &st_u) ) {
      continue;
    }
    if ( (c3_d)st_u.st_size != len_d ) {
      continue;
    }

    c3_i fid_i = open(fil_c, O_RDONLY);
    if ( -1 == fid_i ) {
      continue;
    }

    c3_o eql_o = c3y;
    c3_d rem_d = len_d;
    const c3_y* ptr_y = dat_y;
    c3_y buf_y[4096];

    while ( rem_d > 0 ) {
      c3_d ask_d = ( rem_d < sizeof(buf_y) ) ? rem_d : sizeof(buf_y);
      ssize_t got_i = read(fid_i, buf_y, ask_d);
      if ( got_i <= 0 || (c3_d)got_i != ask_d ||
           0 != memcmp(ptr_y, buf_y, ask_d) )
      {
        eql_o = c3n;
        break;
      }
      ptr_y += ask_d;
      rem_d -= ask_d;
    }

    close(fid_i);
    if ( c3y == eql_o ) {
      return seq_w;
    }
  }
  return 0;
}

/* _blob_mug(): compute a 31-bit mug suitable for blob bucketing.
**
**   u3r_mug_bytes takes c3_h (uint32_t) for the length, so we cap each
**   window at 0xFFFFFFFF bytes.  For files > 4 GiB we hash the first
**   window, mix in the high bits of length, and mix in the last window
**   so that files of identical size but different tail content get
**   distinct buckets.
*/
static c3_h
_blob_mug(const c3_y* dat_y, c3_d len_d)
{
  //  cap first window to UINT32_MAX to avoid hashing 0 bytes
  //
  c3_h win_h = ( len_d > 0xFFFFFFFFULL ) ? 0xFFFFFFFFu : (c3_h)len_d;
  c3_h mug_h = u3r_mug_bytes(dat_y, win_h);

  //  for files larger than 4 GiB, also fold in the high length bits
  //  and hash the last 4 GiB window for tail sensitivity
  //
  c3_h hi_h = (c3_h)(len_d >> 32);
  if ( hi_h ) {
    mug_h = u3r_mug_both(mug_h, hi_h);
    //  hash the final window (last min(len_d, 0xFFFFFFFF) bytes)
    c3_d off_d  = len_d > 0xFFFFFFFFULL ? len_d - 0xFFFFFFFFULL : 0;
    c3_h tail_h = u3r_mug_bytes(dat_y + off_d, (c3_h)(len_d - off_d));
    mug_h = u3r_mug_both(mug_h, tail_h);
  }
  return mug_h;
}

/* u3_blob_save(): write bytes to blob store.
*/
c3_o
u3_blob_save(const c3_c* pax_c,
             const c3_y* dat_y,
             c3_d        len_d,
             c3_h*       mug_h,
             c3_w*       seq_w)
{
  *mug_h = _blob_mug(dat_y, len_d);

  //  acquire lock and get next sequence number
  c3_w nex_w = _blob_lock_acquire(pax_c, *mug_h);
  if ( 0 == nex_w ) {
    return c3n;
  }

  //  check for duplicate before writing
  c3_w dup_w = _blob_dedup(pax_c, *mug_h, nex_w, dat_y, len_d);
  if ( 0 != dup_w ) {
    *seq_w = dup_w;
    //  we already incremented the lock counter, but that's harmless —
    //  nex_w slot will simply be skipped (sparse sequence numbers are fine)
    return c3y;
  }

  //  write blob file
  c3_c fil_c[8192];
  u3_blob_path(fil_c, pax_c, *mug_h, nex_w);

  c3_i fid_i = open(fil_c, O_WRONLY | O_CREAT | O_EXCL, 0400);
  if ( -1 == fid_i ) {
    fprintf(stderr, "blob: failed to create %s: %s\r\n",
            fil_c, strerror(errno));
    return c3n;
  }

  c3_d rem_d = len_d;
  const c3_y* ptr_y = dat_y;
  while ( rem_d > 0 ) {
    size_t  ask_i = ( rem_d < BLOB_IO_MAX ) ? (size_t)rem_d : BLOB_IO_MAX;
    ssize_t wrt_i = write(fid_i, ptr_y, ask_i);
    if ( wrt_i <= 0 ) {
      fprintf(stderr, "blob: write failed on %s: %s\r\n",
              fil_c, strerror(errno));
      close(fid_i);
      unlink(fil_c);
      return c3n;
    }
    ptr_y += wrt_i;
    rem_d -= wrt_i;
  }

  fsync(fid_i);
  close(fid_i);

  *seq_w = nex_w;
  return c3y;
}

/* u3_blob_save_fd(): write from open file descriptor into the blob store.
**
**   Uses mmap() to avoid a large malloc: the OS pages in only what
**   _blob_mug and the dedup scan actually touch, and can evict cold pages
**   immediately.  Works for files of any size that fit in the address space.
*/
c3_o
u3_blob_save_fd(const c3_c* pax_c,
                c3_i        fid_i,
                c3_d        len_d,
                c3_h*       mug_h,
                c3_w*       seq_w)
{
  if ( 0 == len_d ) {
    fprintf(stderr, "blob: refusing to save empty file\r\n");
    return c3n;
  }

  void* map_v = mmap(0, (size_t)len_d, PROT_READ, MAP_PRIVATE, fid_i, 0);
  if ( MAP_FAILED == map_v ) {
    fprintf(stderr, "blob: mmap failed (%" PRIc3_d " bytes): %s\r\n",
            len_d, strerror(errno));
    return c3n;
  }
  madvise(map_v, (size_t)len_d, MADV_SEQUENTIAL);

  c3_o ret_o = u3_blob_save(pax_c, (const c3_y*)map_v, len_d, mug_h, seq_w);
  munmap(map_v, (size_t)len_d);
  return ret_o;
}

/* u3_blob_load(): read blob into a loom atom.
**
**   Uses mmap() and u3i_slab to handle blobs of any size, including >4 GiB.
**   The mapping is released immediately after the loom copy.
*/
u3_weak
u3_blob_load(const c3_c* pax_c, c3_h mug_h, c3_w seq_w)
{
  c3_c fil_c[8192];
  u3_blob_path(fil_c, pax_c, mug_h, seq_w);

  struct stat st_u;
  if ( -1 == stat(fil_c, &st_u) ) {
    fprintf(stderr, "blob: missing blob %" PRIc3_h "/%" PRIc3_w ": %s\r\n",
            mug_h, seq_w, strerror(errno));
    return u3_none;
  }

  c3_d len_d = (c3_d)st_u.st_size;
  c3_i fid_i = open(fil_c, O_RDONLY);
  if ( -1 == fid_i ) {
    fprintf(stderr, "blob: failed to open %s: %s\r\n",
            fil_c, strerror(errno));
    return u3_none;
  }

  void* map_v = mmap(0, (size_t)len_d, PROT_READ, MAP_PRIVATE, fid_i, 0);
  close(fid_i);

  if ( MAP_FAILED == map_v ) {
    fprintf(stderr, "blob: mmap failed on %s: %s\r\n",
            fil_c, strerror(errno));
    return u3_none;
  }
  madvise(map_v, (size_t)len_d, MADV_SEQUENTIAL);

  //  use u3i_slab (c3_d length) to correctly handle blobs >4 GiB.
  //  bloq 3 = bytes; len_d = byte count.
  //
  u3i_slab sab_u;
  u3i_slab_bare(&sab_u, 3, len_d);
  memcpy(sab_u.buf_y, map_v, (size_t)len_d);
  munmap(map_v, (size_t)len_d);

  return u3i_slab_mint_bytes(&sab_u);
}

/* u3_blob_exists(): check whether a blob file exists.
*/
c3_o
u3_blob_exists(const c3_c* pax_c, c3_h mug_h, c3_w seq_w)
{
  c3_c fil_c[8192];
  u3_blob_path(fil_c, pax_c, mug_h, seq_w);

  struct stat st_u;
  return ( 0 == stat(fil_c, &st_u) ) ? c3y : c3n;
}

/* u3_blob_delete(): delete a blob file.
*/
void
u3_blob_delete(const c3_c* pax_c, c3_h mug_h, c3_w seq_w)
{
  c3_c fil_c[8192];
  u3_blob_path(fil_c, pax_c, mug_h, seq_w);

  if ( 0 != unlink(fil_c) && ENOENT != errno ) {
    fprintf(stderr, "blob: failed to delete %s: %s\r\n",
            fil_c, strerror(errno));
  }
}
