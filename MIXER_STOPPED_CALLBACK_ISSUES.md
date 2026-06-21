# Track stopped-callback: deadlock & lifetime issues

Working notes on the threading/lifetime problems around
`MIX_SetTrackStoppedCallback` in `src_c/_sdl3_mixer_c.c`. Scratch document for
later perusal — not long-term documentation.

## Background

`Track.set_stopped_callback()` registers a C callback
(`pg_track_obj_stopped_callback`) with SDL_mixer via
`MIX_SetTrackStoppedCallback(track, cb, userdata=self)`. SDL_mixer invokes that
callback **from the audio thread, while holding the mixer's internal lock**, and
the callback must `PyGILState_Ensure()` to run the Python-level callback.

Two distinct problems arise from this:

1. A GIL ⇄ mixer-lock deadlock on the normal playback API surface.
2. A lifetime problem: the registered callback can fire against a track object
   that is being torn down.

The first was found via `test_play__start_time` in `test/mixer_music_test.py`,
which hangs once a stopped callback is installed on the music track.

---

## Issue 1 — GIL / mixer-lock deadlock (lock-ordering inversion)

### Symptom

`test_play__start_time` hangs. The callback's `PyGILState_Ensure()` blocks
forever.

### Root cause

A classic two-lock ordering inversion between the Python GIL and SDL_mixer's
internal mixer lock:

- **Audio thread** (callback): inside SDL_mixer's mixing code, it already
  **holds the mixer lock**, then calls the stopped callback, which blocks in
  `PyGILState_Ensure()` → it **wants the GIL**.
- **Main thread** (the test loop): calls `pygame.mixer.music.get_busy()` →
  `Track.playing` → `MIX_TrackPlaying()`, which **wants the mixer lock** — while
  **holding the GIL**.

```
audio thread:  holds mixer-lock, wants GIL
main thread:   holds GIL,        wants mixer-lock
```

Deadlock. It only manifests with a callback installed, because without one the
audio thread never needs the GIL, so there is no inversion.

The SDL_mixer header is explicit (`SDL_mixer.h`, `MIX_LockMixer` docs): *"Just
about every SDL_mixer API also locks the mixer while doing its work."* So **any**
`MIX_*` call made on the live mixing graph while holding the GIL can participate
in this deadlock.

### Fix

The main thread must not hold the GIL while blocking on a lock-taking `MIX_*`
call. Wrap such calls in `Py_BEGIN_ALLOW_THREADS` / `Py_END_ALLOW_THREADS`
(after argument parsing, before touching any Python objects / `RAISE`). Example:

```c
bool playing;
/* drop GIL to avoid deadlock, see pg_track_obj_stopped_callback */
Py_BEGIN_ALLOW_THREADS;
playing = MIX_TrackPlaying(self->track);
Py_END_ALLOW_THREADS;
return PyBool_FromLong(playing);
```

This has been applied across the playback-graph call sites (mixer-level:
`play_audio`, `play_tag`, `stop_tag`, `pause_tag`, `resume_tag`, `set_tag_gain`,
`stop_all_tracks`, `pause_all_tracks`, `resume_all_tracks`, `get_spec`, mixer
gain get/set; track-level: `playing`, `paused`, `loops`, gain get/set,
freq-ratio get/set, `set_audio`/`get_audio`, audiostream set/get, filestream,
`play`, `add_tag`/`remove_tag`, playback-position set/get,
`get_remaining_frames`, `ms_to_frames`/`frames_to_ms`, `stop`, `pause`,
`resume`, stereo, 3d-position set/get, `set_stopped_callback`).

Intentionally **not** wrapped (do not take the mixer lock): immutable-`MIX_Audio`
queries (`MIX_GetAudioFormat`, `MIX_GetAudioDuration`,
`MIX_AudioMSToFrames`/`FramesToMS`, audio properties), pure helpers, and
init/version/decoder functions.

For combined-condition calls (`play_tag`, `play`) the short-circuit must be
preserved so the `MIX_Play*` call still does not run when `pg_populate_play_props`
failed.

---

## Issue 2 — Callback lifetime (use-after-free / resurrection on teardown)

### The problem

`MIX_SetTrackStoppedCallback(track, cb, self)` stores the **raw `self` pointer**
as `userdata`, holding **no strong reference**. The track object owns the
`MIX_Track`, and the `MIX_Track` registration points back at the Python object.
The audio thread decides whether to fire the callback based purely on the C
registration — independent of any Python refcount.

If the last Python reference to the track is dropped, `tp_dealloc` runs while a
callback can still fire. The callback does:

```c
PyObject_CallFunctionObjArgs(self->stopped_callback, (PyObject *)self, ...);
//   increfs self 0→1, runs user code, decrefs 1→0 on return → re-entrant dealloc
```

i.e. object **resurrection** at refcount 0 → re-entrant `Py_Dealloc` / corruption.
Without the GIL released in dealloc this instead presents as a **deadlock**
(dealloc holds GIL, `MIX_DestroyTrack` blocks on the mixer lock waiting for the
in-flight callback, which is parked on the GIL).

### Approaches that do NOT solve it (and why)

- **Incref/decref `self` for the duration of the callback body.** Too late — if
  the object was already freed, you dereference freed memory just to do the
  incref. The protection must exist *before* the callback can start. (The
  invocation already temporarily increfs `self` anyway, by passing it as an
  argument.)

- **A Python-level trampoline / bound method stored on the track.** Appealing
  but doesn't help: what gates whether the callback fires is the *C
  registration*, not any Python attribute. A bound method on the track only
  creates a Python-visible self-cycle; it does not make the C registration own a
  reference, so the audio thread can still invoke the thunk against a track
  mid-dealloc. For a Python ref to help it must be the *owned userdata the C
  layer holds* — at which point it is just "strong ref at registration" wearing a
  Python hat, with the same cycle + traverse/clear + GIL-on-unregister
  requirements.

- **`MIX_LockMixer` (or `MIX_DestroyTrack`'s built-in wait) in `dealloc`.** The
  mutex protects the **C `MIX_Track` struct**, not the **PyObject**, whose
  lifetime is governed by refcount. By the time `tp_dealloc` runs, `self` is
  already at refcount 0; making `DestroyTrack`/`LockMixer` *wait* for the
  in-flight callback just means you block until the resurrection happens. Lock
  ordering and refcount lifetime are orthogonal; mutual exclusion on the mixer
  cannot fix a lifetime problem on the Python object.

- **Having the callback itself hold the mixer lock.** It already runs inside the
  locked mixing region (recursive lock makes an explicit lock a no-op), and
  `MIX_DestroyTrack` already waits for the current mix iteration. That
  synchronization already exists and is not sufficient — same reason as above.

### The actual requirement

Guarantee the track's refcount **cannot reach 0 while a callback can still
fire**. That means the registration must own a strong reference to the track,
released only after the callback is unregistered. The unregister must drain any
in-flight callback **against a still-live object** and must release the GIL while
doing so.

### Recommended design (per-track self-reference)

1. **Strong self-ref at construction.** `Py_INCREF(self)` once at the end of a
   successful `init`; record it with a `bool holds_self_ref;` field
   (zero-initialized in `tp_new`). This makes the track collectable only via the
   cyclic GC (the refcount never falls to 0 on its own).

2. **`tp_traverse` must report the self-loop**, guarded by the flag:

   ```c
   if (self->holds_self_ref) {
       Py_VISIT(op);   /* the self-ref the registration relies on */
   }
   ```

   Without this, the GC treats the unreported strong ref as external → the track
   is deemed permanently reachable → **leak** (and dealloc never runs).

3. **A single drain helper** for every unregister — GIL released, lock held so
   the in-flight callback drains and no new one can start; nothing here touches
   Python state:

   ```c
   static void
   pg_track_unregister_stopped_callback(MIX_Mixer *mixer, MIX_Track *track)
   {
       Py_BEGIN_ALLOW_THREADS;
       MIX_LockMixer(mixer);
       MIX_SetTrackStoppedCallback(track, NULL, NULL);
       MIX_UnlockMixer(mixer);
       Py_END_ALLOW_THREADS;
   }
   ```

4. **`tp_clear` drops the self-ref exactly once**, draining *before* clearing
   `mixer_obj` (the drain needs the mixer pointer) and *before* dropping the ref
   (so the callback drains against a live object). The `Py_DECREF(self)` is safe
   during GC because `delete_garbage` does `Py_INCREF(op); clear(op);
   Py_DECREF(op)`:

   ```c
   if (self->holds_self_ref) {
       self->holds_self_ref = false;
       if (self->stopped_callback != NULL) {
           pg_track_unregister_stopped_callback(
               ((PGMixerObject *)self->mixer_obj)->mixer, self->track);
           Py_CLEAR(self->stopped_callback);
           Py_CLEAR(self->stopped_callback_userdata);
       }
       Py_DECREF(self);
   }
   Py_CLEAR(self->mixer_obj);
   Py_CLEAR(self->source_obj);
   ```

5. **`set_stopped_callback`** then needs no self-ref bookkeeping — validate the
   argument first, drain any existing callback, swap the callback objects, and
   re-register under the lock with the GIL released.

6. **`tp_dealloc`** wraps `MIX_DestroyTrack` in `Py_BEGIN/END_ALLOW_THREADS`
   (it blocks on the mixer lock regardless of callbacks). With the self-ref,
   dealloc is only reached via GC after `tp_clear` has already cleared the
   callback.

### Cost / tradeoff

The unconditional self-ref makes **every** track (even callback-less ones)
reclaimable only by a cyclic-GC pass — `del track` defers `MIX_DestroyTrack`
(and the audio resources it holds) to the next collection. Acceptable if Track's
expected usage tolerates deferred destruction; otherwise condition the self-ref
on callback registration (keyed on `stopped_callback != NULL`) so only
callback-bearing tracks pay it.

A residual, unavoidable edge: a callback already *executing* when `tp_clear`
drains it finishes running user Python with the GIL released mid-collection. It
runs against a live object so it is memory-safe, but it is user code during GC.
The alternative that avoids this is a **mixer-owned registry** (the mixer holds
strong refs to callback-bearing tracks and tears them down outside GC), at the
cost of more plumbing.

---

## Review of the in-progress implementation

The current working-tree implementation adds the unconditional `Py_INCREF(self)`
in `init`, routes `dealloc` through `tp_clear`, and adds `Py_DECREF(self)` at the
end of `tp_clear`. Three bugs remain:

1. **Deadlock/race not fixed (most important).** `tp_clear` calls
   `MIX_SetTrackStoppedCallback(NULL)` and `tp_dealloc` calls `MIX_DestroyTrack`
   while holding the GIL, with no `Py_BEGIN_ALLOW_THREADS` and no
   `MIX_LockMixer`. The teardown deadlock and the race against an in-flight
   callback both remain. Needs the drain helper (step 3) and a GIL-released
   `MIX_DestroyTrack`.

2. **`tp_traverse` does not `Py_VISIT(op)`.** The GC cannot see the self-loop, so
   every track is treated as reachable and never collected → leak. (Because the
   refcount also never reaches 0 on its own, dealloc effectively never runs.)

3. **`tp_clear` is not idempotent → double `Py_DECREF(self)`.** It is invoked
   both by the GC (as `tp_clear`, possibly multiple times) and again explicitly
   from `tp_dealloc`. The unconditional decref over-drops the single self-ref
   (negative refcount / re-entrant free). Needs the `holds_self_ref` one-shot
   guard (steps 1 & 4).

Minor: the drain must run before `Py_CLEAR(self->mixer_obj)` (it needs the mixer
pointer); and the "MIX_DestroyTrack will not lead to a callback call" comment is
now slightly out of place in `clear`.

### Status summary

| Item | Status |
|------|--------|
| Issue 1 — playback-API GIL release | Applied across call sites |
| Issue 2 — self-ref skeleton | In progress (3 bugs above) |
| Issue 2 — GIL-released `MIX_LockMixer` drain | **Not yet implemented** |
| Issue 2 — `tp_traverse` self-visit | **Missing** |
| Issue 2 — idempotent self-ref drop | **Missing** |
