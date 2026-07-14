# ObjectModel: replace the JPValue thread-local allocator with a fixed per-type offset

STATUS: DESIGN — not started. Blast radius confined to `native/python/` (per user:
`native/common/`'s `JPClass` and everything Java-side stays untouched; `JPClass::m_Host`
already caches the Python type object and is not being replaced).

## Motivation

Every Java-backed Python object today gets a `JPValue` (class pointer + jvalue) appended
after its normal Python payload. The offset of that appended memory is **recomputed on
every access** by `PyJPValue_getJavaSlotOffset()` (`native/python/pyjp_value.cpp:49-78`),
which pokes at CPython-private fields (`PyLongObject::lv_tag`/`ob_size`) to figure out
how big the object's variable part is. The memory itself is added at allocation time by
`PyJPValue_alloc()` (`pyjp_value.cpp:334-397`), which fakes a `tp_basicsize` bump using a
**thread-local dummy heap type** that gets its `tp_basicsize`/`tp_flags` mutated on every
single object allocation, then the resulting object is re-polymorphed (`Py_SET_TYPE`) to
the real type. This is fragile (depends on CPython internals that already changed once
for 3.12, and again for 3.13's `Py_TPFLAGS_INLINE_VALUES`) and does real work per-allocation
that is otherwise constant for a given type.

`model/helloworld.c` is a working, standalone prototype (not integrated with the real
JPClass/JVM machinery) proving out a different idea: **compute the offset once, when the
type is created, and store it directly on the metaclass instance.** Every instance lookup
then becomes a single stored-integer read instead of a runtime layout computation, and
`tp_alloc` can go back to being whatever CPython's normal allocator is (`tp_basicsize`
already includes our slot).

The goal of this plan is to port that idea into the real `native/python/pyjp_class.cpp` /
`pyjp_value.cpp` machinery, file by file, in order of increasing layout difficulty, and to
scope out (not yet solve) the parts of the prototype that don't have a real-code analog yet
(abstract-class auto-concretization, `java.lang.String` as a `PyUnicode` subclass).

## Current architecture (recap, see also memory `jpype_model_replacement` — none yet, this
doc supersedes ad hoc notes)

- `JPClass::m_Host` (`native/common/include/jp_class.h:210`, get/set at `jp_class.h:34-39`)
  — cached `PyTypeObject*` per Java class. **Stays as-is.**
- `PyJPClass` metaclass instance struct (`pyjp_class.cpp:30-35`): `PyHeapTypeObject ht_type;
  JPClass *m_Class; PyObject *m_Doc;`. No offset field today — **this is where the new field(s)
  go.**
- `PyJPClass_FromSpecWithBases(spec, bases)` (`pyjp_class.cpp:63-261`) builds a **shared base
  type** (e.g. `PyJPObject_Type`, `PyJPNumberLong_Type`, `PyJPException_Type`, `PyJPChar_Type`,
  `PyJPArray_Type`) via `PyType_FromMetaclass`/hand-rolled fallback pre-3.12, and always installs
  `tp_alloc = PyJPValue_alloc`, `tp_finalize = PyJPValue_finalize`.
- `PyJPClass_init` (`pyjp_class.cpp:263-365`) is the metaclass `tp_init`, invoked when a live
  wrapper type for a **specific `JPClass`** is created through `PyJPClass_hook`
  (`pyjp_class.cpp:1164-1231`, not shown above but referenced by the research pass). Re-asserts
  the alloc/finalize pair and has "conflict" sanity checks.
- `PyJPClass_getBases` chooses, per `JPClass` (dispatching on `dynamic_cast<JPBoxedType*>`,
  `isArray()`, `cls == context->_java_lang_Throwable`, etc.), which shared base type(s) a given
  Java class's Python wrapper derives from.
- The `PyJPValue_*` slot API (`pyjp_value.cpp`) is consumed from ~14 files, ~90 call sites,
  heaviest in `pyjp_object.cpp`, `pyjp_array.cpp`, `pyjp_char.cpp`, `pyjp_number.cpp`,
  `pyjp_class.cpp`, and the primitive/boxed `jp_*type.cpp` value-construction sites.

## Guiding constraint: the ~90 call sites are not really 90 problems

The research pass counted ~90 call sites of the `PyJPValue_*` slot API across ~14 files, but
almost all of them only call the **public** functions (`PyJPValue_getJavaSlot`,
`_assignJavaSlot`, `_isSetJavaSlot`, `_hasJavaSlot`) with no knowledge of how the slot is
found. None of those callers need to change at all if the public function *signatures* and
*behavior* stay identical. The only files that need to know about the new offset mechanism
are:

- `pyjp_value.cpp` — where the offset is read/written and where the allocator hack currently
  lives. This is where nearly all of the real work happens.
- `pyjp_class.cpp` — the metaclass, which needs the new `offset` field on `struct PyJPClass`
  and must compute it once at type-creation time instead of leaving it to be derived later.
- `pyjp_number.cpp` — not a call-site tweak but a planned **full rewrite**. Its `tp_new`
  (`PyJPNumber_new`) and the boxed long/float allocation path are inherently tied to how the
  slot is placed relative to `PyLongObject`'s variable-length digits (Phase 3 below), so this
  file changes wholesale alongside `pyjp_value.cpp`, not as a downstream call-site fixup.
  Expect `pyjp_char.cpp` (Phase 4) and possibly `pyjp_object.cpp`'s exception/array `tp_new`
  functions (Phases 5-6) to end up in the same "rewritten, not patched" category once their
  phases are reached — only the *plain-call-site* files (jp_*type.cpp, jp_method.cpp,
  jp_classhints.cpp, pyjp_field.cpp, pyjp_monitor.cpp, pyjp_module.cpp, pyjp_buffer.cpp,
  jp_class.cpp, jp_exception.cpp) are expected to need zero changes.

So the real fight is **not** 14 files of call-site churn — it's getting `model/helloworld.c`'s
approach to produce the *same object layouts* the current code already produces (same
`tp_basicsize` per family, same base-class chains) so that `pyjp_value.cpp` can be swapped out
underneath everything else unchanged. First-step strategy: treat `model/helloworld.c` as a
sandbox to prove out layout compatibility (does `PyLongObject` boxing still fit, does the
exception diamond still work, etc.) in isolation, file it down until it's a drop-in swap for
`pyjp_value.cpp`'s two changed entry points (`PyJPValue_alloc` disappears; `getJavaSlotOffset`
becomes a stored-field read), and only then touch `pyjp_class.cpp` to add the field it reads
from. Everything else in the ~14-file call-site list should require zero changes.

## Target design

1. **Metaclass gains an `offset` field** (`Py_ssize_t offset` on `PyJPClass`, mirroring
   `model/helloworld.c:70`), plus, only where abstract/interface auto-concretization is in
   scope (see Phase 5), an `other` pointer for the paired concrete/abstract stand-in
   (`helloworld.c:71`).
2. **Offset is computed once**, at the point a concrete Python layout is finalized for a
   base family (`PyJPObject_Type`, `PyJPNumberLong_Type`, `PyJPNumberFloat_Type`,
   `PyJPChar_Type`, `PyJPException_Type`, `PyJPArray_Type`/`PyJPArrayPrimitive_Type`) — i.e.
   inside `PyJPClass_FromSpecWithBases`/`PyJPClass_init`, analogous to `jclass_init`
   (`helloworld.c:469-548`) — and inherited unchanged by every subsequent per-Java-class type
   built on that base (individual `Integer`/`Long`/user exception subtypes etc. all reuse the
   *same* offset as their shared base, since they don't add further Python-visible fields).
3. **`PyJPValue_getJavaSlot`/`_assignJavaSlot`/`_isSetJavaSlot`** stop calling
   `PyJPValue_getJavaSlotOffset()` and instead read `((PyJPClass*) Py_TYPE(self))->offset`
   (walking to the metaclass of `Py_TYPE(self)`, i.e. `Py_TYPE(Py_TYPE(self))`, exactly as
   `model/helloworld.c:212-225` does for `getValue`/`setValue`).
4. **`PyJPValue_alloc` becomes unnecessary as a special allocator** for offset purposes: once
   `tp_basicsize` correctly includes the slot for every type at creation time, plain
   `PyType_GenericAlloc` (or the type's inherited `tp_alloc`) is sufficient. The thread-local
   dummy-type trick in `pyjp_value.cpp:334-397` is deleted entirely. (`tp_finalize`/`tp_free`
   stay — they're about JNI global-ref cleanup, unrelated to slot placement.)
5. Each Python-visible layout family is migrated **in order of difficulty**, verified
   independently before moving to the next (see Phases below), because each has a different
   answer to "where does `tp_basicsize` need to grow and by how much."

## Phases (increasing difficulty; each phase should compile + pass the existing test suite
before starting the next)

### Phase 1 — Metaclass plumbing (no behavior change yet)
- Add `offset` (and, if needed early, `other`) to `struct PyJPClass` in `pyjp_class.cpp`.
- Add `PyJPClass_traverse`/`_clear` handling for `other` once it exists (Phase 5).
- Add a new `PyJPValue_getJavaSlotOffsetFixed(PyObject* self)` (name TBD) that just reads
  the stored `offset` field, alongside (not yet replacing) the existing runtime-computed
  `PyJPValue_getJavaSlotOffset`, so both can be diffed/asserted equal in debug builds during
  migration.

### Phase 2 — Plain object family (`PyJPObject_Type`, easiest: fixed non-variable layout)
- `PyJPObject_Type` (`pyjp_object.cpp`) has no variable-length base and no CPython-internal
  struct poking — this is the `java.lang.Object`/generic-object case, directly analogous to
  `PyJPObject_Type`/`jobject_new` in the prototype (`helloworld.c:634-665`).
- Compute the offset once in `PyJPClass_FromSpecWithBases`/`PyJPClass_init` by
  `align_up(tp_basicsize, sizeof(void*))`, bump `tp_basicsize` by `sizeof(JPValue)`, store the
  offset on the metaclass instance.
- Switch `PyJPValue_getJavaSlot`/`_assignJavaSlot`/`_isSetJavaSlot` to the fixed-offset path
  **only for types whose base chain bottoms out at `PyJPObject_Type`** (i.e. leave boxed/char/
  exception/array on the legacy runtime path until their own phases land) — gate this with a
  per-type flag or by having `PyJPValue_getJavaSlot` prefer the stored offset when non-zero and
  fall back to the runtime computation otherwise, so migration can proceed file-by-file without
  a single flag-day rewrite.
- Delete the special-case thread-local allocator path for this family; confirm `tp_alloc`
  reverts to ordinary `PyType_GenericAlloc`-derived allocation.
- Run full test suite; specifically stress subclassing depth (Python subclasses of Java
  object wrappers already exist and must keep working — this is the `U(_JObject)`/`V(_JObject)`
  case from `model/test.py`).
- **Concrete implementation plan derived 2026-07-12, not yet executed** — see the
  `jpype_phase2_real_port_plan` memory entry for the exact call-site inventory (9 total
  `PyJPClass_FromSpecWithBases` calls, only `PyJPObject_Type`/`PyJPException_Type`/
  `PyJPComparable_Type` in scope this pass), struct/signature changes, and — most
  important — the critical guard fix: `PyJPValue_getJavaSlotOffset`/`_hasJavaSlot`'s entry
  guard currently checks `tp_alloc == PyJPValue_alloc`, which will start returning false
  negatives the moment migrated families stop forcing that allocator, silently breaking
  ~40+ call sites. That check must be dropped (keep only the `tp_finalize` check) in the
  SAME commit as removing the `Py_tp_alloc` override — never split across commits.
  Acceptance gate: real project build (see `jpype_build_env_gotchas` memory for this
  machine's environment fixes) + full test suite back at the 451/451 baseline.

### Phase 3 — Boxed numeric types (`PyJPNumberLong_Type`/`PyJPNumberFloat_Type`,
`Integer`/`Long`/`Byte`/`Short`/`Boolean` on `PyLong_Type`; `Float`/`Double` on `PyFloat_Type`)
- **Hardest case, RESOLVED 2026-07-12 in `model/pyjp_number.cpp`.** `PyFloat_Type` base is
  fixed-size, so it degrades to the Phase 2 pattern (`offsetof`-style fixed offset) — trivial.
- `PyLong_Type` base is variable-length, and the real wart turned out to be CPython's own:
  `long_subtype_new` (`Objects/longobject.c`) is, verbatim in CPython's own comment, a
  "wimpy, slow approach ... first create a regular int from whatever arguments we got, then
  allocate a subtype instance and initialize it from the regular int. The regular int is then
  thrown away." That build-then-copy is unavoidable if construction routes through
  `PyLong_Type.tp_new` at all (it's gated on `type != &PyLong_Type` inside `long_subtype_new`
  itself) — our old prototype code reproduced exactly this shape. Fix: never call into it.
  `PyJPNumberLong_new` now parses the argument via the stable public
  `PyLong_AsLongLongAndOverflow` and hand-writes digits directly into a fixed-size buffer — one
  allocation, no copy, no throwaway object.
- Digit/sign layout confirmed against the actual CPython source (`~/jcef/cpython`, tags
  v3.10.0-v3.14.0, checked out and diffed directly): `<=3.11` uses `ob_size`'s sign
  (identical between 3.10/3.11); `>=3.12` uses `lv_tag` (low 2 bits = sign
  0/1/2=positive/zero/negative, bit 2 = immortal flag, `>>3` = digit count) and is
  byte-for-byte identical across 3.12/3.13/3.14 (only doc comments and unrelated new APIs
  like 3.14's `PyLongWriter` differ). `PyLong_SHIFT`/`PyLong_MASK`/`digit`/
  `PYLONG_BITS_IN_DIGIT` are all public headers, so digit width (30 or 15 bits) is a
  compile-time constant. Fixed budget `JLONG_MAX_DIGITS` = 3 (30-bit digits) or 5 (15-bit
  digits) covers the full 64-bit signed range Java's boxed `Long` needs — no arbitrary-
  precision case exists for real Java values, so the overflow path (one past
  `Long.MAX_VALUE`) is a real, reachable, correctly-rejected error, not dead code.
- Validated: full boundary sweep (0, ±1, int16/int32/int64 boundaries, overflow rejection)
  passes under Python 3.12 through the real `model/` extension; the `<=3.11` `ob_size` branch
  was validated in an isolated standalone extension (outside the `PyJPClass` metaclass, since
  `model/pyjp_class.cpp` only implements the 3.12+ `PyType_FromMetaclass` path) under Python
  3.10 — same sweep, all passing.
- **Correction, 2026-07-12, after decomposing the benchmark:** skipping the wimpy path is
  NOT where the speedup comes from. Isolated apples-to-apples test (a plain `int` subclass
  using stock `long_subtype_new` vs. an identical subclass using our direct digit-write, both
  going through the same single `type->tp_alloc(type, n)` call, no jpype machinery at all)
  measured statistically identical throughput (~15.8M vs ~16.1M ops/s, ~2% apart — noise).
  Why: CPython's `long_new_impl` fast-paths an already-exact-`int` argument by returning it
  via `Py_INCREF` with no new allocation — which is exactly JPype's realistic input shape
  (boxing an already-Python-int value). So `long_subtype_new`'s "throwaway" step costs a free
  incref + a cheap 1-2 digit copy riding on top of the one allocation both approaches need
  anyway, not a second allocation. The ~1.7-1.9x `int_new`/`float_new`/`object_new` speedups
  measured between `model/` and `model2/` come almost entirely from **the allocator
  mechanism** (fixed offset vs. the thread-local dummy-heap-type mutation), not from
  bypassing `PyLong_Type.tp_new`. Bypassing it is still correct and worth keeping (removes a
  dependency on CPython's general-purpose parsing machinery and guarantees the fixed offset
  outright rather than incidentally), but it should not be sold as the source of the speedup.
- jpype's Python floor is moving to 3.10 (dropping 3.8/3.9); this phase's version gate already
  matches with no changes needed, since 3.8/3.9 share the same pre-3.12 `ob_size` layout as
  3.10/3.11.
- **Not yet ported to the real `native/python/`** — proven out in `model/` only so far.
  `model/pyjp_class.cpp`'s metaclass still needs the pre-3.12 hand-rolled
  `PyType_FromSpecWithBases`-style fallback (present in the real `pyjp_class.cpp` today, see
  its pre-3.12 branch at `pyjp_class.cpp:72-251`) ported over before the sandbox itself can
  build/test end-to-end on 3.10/3.11; right now that branch is validated in isolation only.

### Phase 4 — `java.lang.Character` (`PyJPChar_Type`, `pyjp_char.cpp`)
- Already bespoke (manual `PyCompactUnicodeObject` field patching, direct `PyJPValue_alloc`
  call instead of going through `tp_new`) — this phase is mostly "recompute the fixed offset
  analytically instead of via `PyJPValue_getJavaSlotOffset`'s generic `_PyObject_VAR_SIZE`
  path," since `PyJPChar`'s layout (`m_Obj` + `m_Data[4]`) is already fully known at compile
  time (`pyjp_char.cpp:28-32`). Should be low-risk once Phase 2's plumbing exists.

### Phase 5 — Exceptions (`PyJPException_Type`, `pyjp_object.cpp`)
- Real diamond: `PyExc_Exception` + `PyJPObject_Type`. Offset must sit after
  `PyBaseExceptionObject`'s tail, which is itself fixed-size, so this is mechanically like
  Phase 2 but based on a different concrete ancestor. Confirm user-defined Java exception
  subclasses (arbitrary further Python subclassing of a generated exception wrapper) don't
  need a *second* offset — they shouldn't, since Python subclasses of a fixed-layout type don't
  usually grow `tp_basicsize` unless they add slots, which Java-generated wrapper types don't.

### Phase 6 — Arrays (`PyJPArray_Type`/`PyJPArrayPrimitive_Type`, `pyjp_array.cpp`)
- Already carries compiled-in `m_Array`/`m_View` fields ahead of the generic slot — this
  should be closer to Phase 2 (fixed struct, offset = `align_up(sizeof(PyJPArray), ...)`)
  once the buffer-protocol subclass (`PyJPArrayPrimitive_Type`) is confirmed not to add
  further per-instance fields beyond what the buffer protocol needs from the base.

### Phase 7 (open, needs a follow-up design pass, not scoped in detail here) — Abstract
classes / interfaces auto-concretization
- The prototype's `jclass_concrete`/`other`-pointer pattern (`helloworld.c:366-379`,
  `jclass_init` lines 522-541) has **no real-code analog today** — currently abstract
  Java classes/interfaces are simply not instantiable as generic wrappers (Python-side
  proxies for interfaces are a wholly separate mechanism, `jpype/_jproxy.py`, going through
  `JPProxyType`/`pyjp_proxy.cpp`, untouched by this plan). Whether this plan *needs* to add
  auto-concretization at all is an open question for the user — the model prototype does it
  because it's exploring general Python-subclassing of interfaces, but the real codebase may
  not need that capability for this migration to be complete. **Do not implement Phase 7
  until Phases 1-6 are done and the user confirms it's actually in scope.**
- `java.lang.String`-as-`PyUnicode`-subclass is similarly new territory (no existing
  `pyjp_string.cpp`) and explicitly **out of scope** unless the user asks for it — today
  String wrapping either copies to a native `str` (`convert_strings`) or falls back to the
  generic `PyJPObject_Type` layout, and this plan doesn't need to change that to remove the
  fragile allocator.

## Call-site migration checklist (per phase, update only the files relevant to that phase)

- `pyjp_value.cpp` — replace offset computation, delete thread-local alloc hack once no
  phase still depends on it.
- `pyjp_class.cpp` — add offset field + computation in `PyJPClass_FromSpecWithBases`/`_init`.
- `pyjp_object.cpp`, `pyjp_number.cpp`, `pyjp_char.cpp`, `pyjp_array.cpp` — per-phase `tp_new`
  updates.
- `jp_exception.cpp`, `jp_classhints.cpp`, `jp_method.cpp`, `pyjp_monitor.cpp`,
  `pyjp_module.cpp`, `pyjp_field.cpp`, `jp_longtype.cpp`, `jp_floattype.cpp`, `jp_inttype.cpp`,
  `jp_chartype.cpp`, `jp_shorttype.cpp`, `jp_bytetype.cpp`, `jp_doubletype.cpp`,
  `jp_boxedtype.cpp`, `jp_class.cpp`, `pyjp_buffer.cpp` — all call the public
  `PyJPValue_getJavaSlot`/`_assignJavaSlot`/`_isSetJavaSlot`/`_hasJavaSlot` API only; **no
  changes needed** in these files as long as the public function signatures stay stable —
  the offset-vs-runtime-computation swap is fully encapsulated in `pyjp_value.cpp`.

## Testing strategy

- Existing full test suite (`test/` — see `jpype_reverse_bridge_testing` memory, 451/451
  baseline) must stay green after every phase.
- Add a targeted micro-test per phase asserting `PyJPValue_getJavaSlot`/`_assignJavaSlot`
  round-trip correctly, and that `sys.getsizeof`/`__basicsize__` matches the expected fixed
  layout (catches silent double-counting of the slot).
- Phase 3 (boxed long) needs an explicit boundary test at the digit-budget edge (largest
  representable `long`/smallest that would overflow the fixed budget, if such a path is
  reachable at all per the open question above).
- Run under both a debug CPython build and at least 3.11/3.12/3.13 if available, since the
  whole point of this migration is removing dependence on version-specific private-field
  layouts — worth confirming the new fixed-offset path is actually version-independent where
  claimed.

## Open questions for the user

1. Is Phase 7 (abstract-class auto-concretization, String-as-PyUnicode-subclass) actually in
   scope for this effort, or is the goal solely to remove the fragile allocator for the
   *existing* set of wrapped layouts?
2. ~~For Phase 3, can boxed `Long`/`Integer`/etc. ever be constructed from a Python `int`
   wider than 64 bits?~~ RESOLVED: no — Java's boxed types top out at 64-bit `Long`, so the
   overflow path is real and load-bearing, not dead code.
3. Do we want per-phase feature-branch commits (mirroring the `array-region-copy`/`spi`-style
   branch history seen elsewhere in this repo), or one running branch (`model`) with all
   phases landing as sequential commits?
