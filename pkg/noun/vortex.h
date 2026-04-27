/// @file

#ifndef U3_VORTEX_H
#define U3_VORTEX_H

#include "allocate.h"
#include "c3/c3.h"
#include "imprison.h"
#include "version.h"

  /**  Data structures.
  **/
    /* u3v_arvo: modern arvo structure.
    */
      typedef struct _u3v_arvo {
        c3_d  eve_d;                      //  event number
        u3_noun roc;                      //  kernel core
        u3_noun yot;                      //  cached gates
      } u3v_arvo;

    /* u3v_lease: PQ entry for lease TTL expiry.
    **
    **   Tracks a single les_w increment for a blob.  If the king
    **   releases the lease (via %blob-release IPC) before expiry,
    **   dead_o is set to c3y and the PQ sweeper skips the decrement.
    **   If the king crashes, the TTL fires and les_w is decremented.
    */
      typedef struct _u3v_lease {
        c3_d  exp_d;        //  expiry time (Unix ms)
        c3_h  mug_h;        //  blob mug
        c3_w  seq_w;        //  blob seq within mug bucket
        c3_o  dead_o;       //  c3y if lease already released
      } u3v_lease;

    /* u3v_bank: loom-resident blob bank.
    **
    **   Lives in u3v_home, checkpointed in image.bin.
    **
    **   blb_p: HAMT  bid -> u3a_blob* (loom offset of refcount struct)
    **   bob_p: HAMT  bid -> u3a_atom* (loom offset of interned bob atom)
    **
    **   A blob file is deleted when ALL of:
    **     u3a_blob.log_w == 0  (no event-log refs)
    **     u3a_blob.les_w == 0  (no active leases)
    **     bob_p[bid] absent    (no live bob atom in the loom)
    */
      typedef struct _u3v_bank {
        u3p(u3h_root) blb_p;  //  bid -> u3a_blob* (loom offset)
        u3p(u3h_root) bob_p;  //  bid -> u3a_atom* (loom offset, interning)
      } u3v_bank;

    /* u3v_home: all internal (within image) state.
    **       NB: version must first for ease of migration.
    **
    **   ban_u is placed BEFORE rod_u to avoid a memory clobber:
    **   rod_u.cax (last field of u3a_road) was immediately adjacent,
    **   and inner-road initialization was overwriting ban_u.  Placing
    **   ban_u before rod_u puts it at a stable offset (after pam_d,
    **   before the large road struct).  lazy-init in _find_home
    **   handles zero values from pre-blob snapshots.
    */
      typedef struct _u3v_home {
        u3v_version ver_d;                //  version number
        c3_d        pam_d;                //  parameters
        u3v_arvo    arv_u;                //  arvo state
        u3v_bank    ban_u;                //  blob bank
        u3a_road    rod_u;                //  storage state
      } u3v_home;


  /**  Globals.
  **/
      /// Arvo internal state.
      extern u3v_home* u3v_Home;
#       define u3H  u3v_Home
#       define u3A  (&(u3v_Home->arv_u))

  /**  Functions.
  **/
    /* u3v_life(): execute initial lifecycle, producing Arvo core.
    */
      u3_noun
      u3v_life(u3_noun eve);

    /* u3v_boot(): evaluate boot sequence, making a kernel
    */
      c3_o
      u3v_boot(u3_noun eve);

    /* u3v_boot_lite(): light bootstrap sequence, just making a kernel.
    */
      c3_o
      u3v_boot_lite(u3_noun lit);

    /* u3v_wish_w(): text expression with cache.
    */
      u3_noun
      u3v_wish_w(const u3_noun txt);

    /* u3v_do(): use a kernel function.
    */
      u3_noun
      u3v_do(const c3_c* txt_c, u3_noun arg);
#     define u3do(txt_c, a)          u3v_do(txt_c, a)
#     define u3dc(txt_c, a, b)       u3v_do(txt_c, u3nc(a, b))
#     define u3dt(txt_c, a, b, c)    u3v_do(txt_c, u3nt(a, b, c))
#     define u3dq(txt_c, a, b, c, d) u3v_do(txt_c, u3nq(a, b, c, d))

    /* u3v_wish(): text expression with cache.
    */
      u3_noun
      u3v_wish(const c3_c* str_c);

    /* u3v_lily(): parse little atom.
    */
      c3_o
      u3v_lily(u3_noun fot, u3_noun txt, c3_l* tid_l);

    /* u3v_peek(): query the reck namespace.
    */
      u3_noun
      u3v_peek(u3_noun hap);

    /* u3v_soft_peek(): softly query the reck namespace.
    */
      u3_noun
      u3v_soft_peek(c3_w mil_w, u3_noun sam);

    /* u3v_poke(): compute a timestamped ovum.
    */
      u3_noun
      u3v_poke(u3_noun sam);

    /* u3v_poke_sure(): inject an event, saving new state if successful.
    */
      c3_o
      u3v_poke_sure(c3_w mil_w, u3_noun eve, u3_noun* pro);

    /* u3v_tank(): dump single tank.
    */
      void
      u3v_tank(u3_noun blu, c3_l tab_l, u3_noun tac);

    /* u3v_punt(): dump tank list.
    */
      void
      u3v_punt(u3_noun blu, c3_l tab_l, u3_noun tac);

    /* u3v_sway(): print trace.
    */
      void
      u3v_sway(u3_noun blu, c3_l tab_l, u3_noun tax);

    /* u3v_plan(): queue ovum (external).
    */
      void
      u3v_plan(u3_noun pax, u3_noun fav);

    /* u3v_mark(): mark arvo kernel.
    */
      u3m_quac*
      u3v_mark();

    /* u3v_reclaim(): clear ad-hoc persistent caches to reclaim memory.
    */
      void
      u3v_reclaim(void);

    /* u3v_rewrite_compact(): rewrite arvo kernel for compaction.
    */
      void
      u3v_rewrite_compact(void);

#endif /* ifndef U3_VORTEX_H */
