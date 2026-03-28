/// @file

#ifndef U3_VERE_BLOB_H
#define U3_VERE_BLOB_H

#include "c3/c3.h"
#include "noun.h"

  /* Blob store: content-addressed storage for large atoms.
  **
  ** Files live in $pier/.urb/bob/<mug>/<seq>.
  ** Each mug bucket has a lockfile ($pier/.urb/bob/<mug>/lock) holding
  ** the next available sequence number (ASCII decimal).
  **
  ** Earth is the sole writer; Mars is read-only.
  */

  /* U3_BLOB_THRESH: atoms larger than this (in bytes) are blobified.
  */
#   define U3_BLOB_THRESH  (32ULL * 1024ULL * 1024ULL)

    /* u3_blob_init(): initialize blob store; create .urb/bob/ if needed.
    */
      void
      u3_blob_init(const c3_c* pax_c);

    /* u3_blob_save(): write bytes to blob store.
    **
    ** Deduplicates within the mug bucket (byte-for-byte comparison).
    ** On success, returns c3y and sets *mug_h and *seq_w.
    */
      c3_o
      u3_blob_save(const c3_c* pax_c,
                   const c3_y* dat_y,
                   c3_d        len_d,
                   c3_h*       mug_h,
                   c3_w*       seq_w);

    /* u3_blob_save_fd(): streaming write from open file descriptor.
    **
    ** Reads [len_d] bytes from [fid_i], writes to blob store.
    ** Avoids double-buffering for large file ingestion.
    ** On success, returns c3y and sets *mug_h and *seq_w.
    */
      c3_o
      u3_blob_save_fd(const c3_c* pax_c,
                      c3_i        fid_i,
                      c3_d        len_d,
                      c3_h*       mug_h,
                      c3_w*       seq_w);

    /* u3_blob_load(): read blob into a loom atom.
    **
    ** Returns u3_none on failure.
    */
      u3_weak
      u3_blob_load(const c3_c* pax_c, c3_h mug_h, c3_w seq_w);

    /* u3_blob_exists(): check whether a blob file exists.
    */
      c3_o
      u3_blob_exists(const c3_c* pax_c, c3_h mug_h, c3_w seq_w);

    /* u3_blob_delete(): delete a blob file.
    **
    ** Called when a bob atom's total refcount reaches zero.
    */
      void
      u3_blob_delete(const c3_c* pax_c, c3_h mug_h, c3_w seq_w);

    /* u3_blob_path(): write filesystem path for a blob into [out_c].
    **
    ** [out_c] must be at least 8192 bytes.
    */
      void
      u3_blob_path(c3_c*       out_c,
                   const c3_c* pax_c,
                   c3_h        mug_h,
                   c3_w        seq_w);

#endif /* ifndef U3_VERE_BLOB_H */
