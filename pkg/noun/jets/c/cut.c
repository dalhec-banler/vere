/// @file

#include "jets/q.h"
#include "jets/w.h"

#include "noun.h"

#include <string.h>

  u3_noun
  u3qc_cut(u3_atom a,
           u3_atom b,
           u3_atom c,
           u3_atom d)
  {
    c3_w b_w, c_w;
    if ( !_(u3a_is_cat(a)) || (a >= u3a_word_bits) ) {
      return u3m_bail(c3__fail);
    }
    if ( !_(u3r_safe_word(b, &b_w)) ) {
      return u3m_bail(c3__fail);
    }
    if ( !_(u3r_safe_word(c, &c_w)) ) {
      return u3m_bail(c3__fail);
    }

    {
      c3_g a_g   = a;
      c3_w len_w = u3r_met(a_g, d);

      if ( (0 == c_w) || (b_w >= len_w) ) {
        return 0;
      }
      if ( b_w + c_w > len_w ) {
        c_w = (len_w - b_w);
      }
      if ( (b_w == 0) && (c_w == len_w) ) {
        return u3k(d);
      }

      //  bob-aware fast path for byte-aligned cuts: mmap the blob and
      //  memcpy the requested byte range directly into the slab.  No
      //  full-blob materialization (u3r_chop on a bob atom would call
      //  u3r_blob_load, which allocates and copies the entire file into
      //  the loom).
      //
      //  We require bloq >= 3 so the cut range is a whole number of
      //  bytes; bit-level cuts (a_g < 3) are rare and fall through to
      //  the generic path below.
      //
      if ( (a_g >= 3) && (c3y == u3a_is_bob(d)) ) {
        c3_d        map_d = 0;
        const c3_y* map_y = u3r_blob_map(d, &map_d);

        if ( map_y ) {
          c3_g  shf_g = a_g - 3;                  //  bloq -> byte shift
          c3_d  off_d = (c3_d)b_w << shf_g;       //  byte offset in blob
          c3_d  byt_d = (c3_d)c_w << shf_g;       //  bytes to copy

          //  clamp against actual file size.  len_w (from u3r_met)
          //  reflects the atom's significant-bit length with trailing
          //  zeros stripped; the on-disk file may be a bit shorter
          //  than implied by the bloq count if the tail-word is
          //  partially significant.  read only what's in the file;
          //  u3i_slab_init already zero-initialized the slab so any
          //  bytes past EOF remain as implicit zeros.
          //
          c3_d cpy_d = byt_d;
          if ( off_d >= map_d ) {
            cpy_d = 0;
          }
          else if ( off_d + cpy_d > map_d ) {
            cpy_d = map_d - off_d;
          }

          u3i_slab sab_u;
          u3i_slab_init(&sab_u, a_g, c_w);

          if ( cpy_d ) {
            memcpy(sab_u.buf_y, map_y + off_d, (size_t)cpy_d);
          }

          u3r_blob_unmap(map_y, map_d);
          return u3i_slab_mint(&sab_u);
        }
        //  mmap failed (missing blob file) — fall through to u3r_chop,
        //  which will silently return zero via u3r_blob_load → u3_none.
      }

      {
        u3i_slab sab_u;
        u3i_slab_init(&sab_u, a_g, c_w);

        u3r_chop(a_g, b_w, c_w, 0, sab_u.buf_w, d);

        return u3i_slab_mint(&sab_u);
      }
    }
  }
  u3_noun
  u3wc_cut(u3_noun cor)
  {
    u3_noun a, b, c, d;

    if ( (c3n == u3r_mean(cor, {u3x_sam_2,  &a},
                                {u3x_sam_12, &b},
                                {u3x_sam_13, &c},
                                {u3x_sam_7,  &d})) ||
         (c3n == u3ud(a)) ||
         (c3n == u3ud(b)) ||
         (c3n == u3ud(c)) ||
         (c3n == u3ud(d)) )
    {
      return u3m_bail(c3__exit);
    } else {
      return u3qc_cut(a, b, c, d);
    }
  }

