# Tutorial: adding a new stdlib type via WrapperService SPI

Written after implementing `plan/Collections.md` (PyDeque/PyOrderedDict/
PyDefaultDict/PyCounter), to save the next person from re-deriving the
`python.io` reference pattern and re-discovering the two bugs below the
hard way. If you're about to write a new `plan/<Something>.md` scoping a
stdlib wrapper (Datetime, Decimal, Pathlib, ChainMap, ...), read this
first, then go copy the concrete files it points at — this is a map, not
a replacement for reading `org/jpype/WrapperService.java`'s Javadoc, which
is the authoritative spec.

## The shape of the mechanism, in one paragraph

A `WrapperService` is a Java `ServiceLoader` provider (`module-info.java`:
`provides org.jpype.WrapperService with ...`) that hands the `Installer` a
list of `.pyspi` resource files. Each `.pyspi` file is a tiny header
(`kind`/`module`/`class`/`interface`, or `kind: backend`/`interface`) plus
a blob of Python source that binds a `METHODS` dict mapping method names
to callables. At startup (or lazily, on first sighting, if `lazy: true`)
the Installer registers those callables against the named Python class, so
that when an instance crosses into Java it gets proxied as the named Java
interface with `METHODS` backing each entry. Nothing touches
`_jbridge.py`. `python.io` is the worked example; `python.collections` is
the second one, now also worth reading as reference.

## Reference files, read in this order

1. `native/jpype_module/src/main/java/org/jpype/WrapperService.java` —
   the authoritative Javadoc for the `.pyspi` format. Read this first,
   every time; don't rely on this tutorial's paraphrase.
2. `native/jpype_module/src/main/java/python/io/PyIOWrapperService.java`
   and `native/jpype_module/src/main/java/python/collections/PyCollectionsWrapperService.java`
   — two real `WrapperService` impls. Both are ~15 lines: module name(s),
   version, `SpiLoader.listPyspiResources(YourClass.class, "/your/spi/dir")`.
   Copy one, rename.
3. `native/jpype_module/src/main/resources/python/io/spi/*.pyspi` and
   `native/jpype_module/src/main/resources/python/collections/spi/*.pyspi`
   — real `.pyspi` files covering every case: eager class registration
   (`_io.BytesIO.pyspi`), lazy (`_io.StringIO.pyspi`,
   `lazy: true` — only pay the registration cost the first time an
   instance of that class actually crosses into Java), empty `METHODS`
   for a subclass that inherits everything from an already-registered
   ancestor interface (`_io.BufferedReader.pyspi`), and a `kind: backend`
   mini-backend for a factory interface
   (`python.io.IO.pyspi`, `python.collections.PyCollections.pyspi`).
4. `native/jpype_module/src/main/java/python/io/IO.java` and
   `native/jpype_module/src/main/java/python/collections/PyCollections.java`
   — the factory-interface pattern (`XxxFoo.using(context).thing()`).
   Every new package needs exactly one of these; it's the only public
   entry point, so instances are never hand-constructed.
5. `native/jpype_module/src/main/java/python/lang/PyDict.java` and
   `PyMapping.java` — **read both, and pick the right one to extend.**
   `PyDict` is for genuine `dict` subclasses (`OrderedDict`, `defaultdict`,
   `Counter` all pass `isinstance(x, dict)`). `PyMapping<K,V>` is the
   weaker `collections.abc.Mapping` protocol — use it for anything that
   only implements the Mapping protocol without being a real `dict`
   (`ChainMap` is `MutableMapping`, not `dict` — extending `PyDict` for it
   would be a lie about its Python type, even though the visible Java
   surface looks the same). Both are core `python.lang` types whose Map
   methods (`get`/`put`/`containsKey`/`remove`/...) are backed by generic
   C-API calls (`PyObject_GetItem` etc.) in `Backend.java` — they work on
   *any* Python object that responds to the right dunder protocol,
   regardless of whether that object arrived via the core dict-proxy path
   or via a WrapperService-built SPI proxy. **This means a new SPI
   interface that extends `PyDict`/`PyMapping` gets the whole inherited
   Map surface for free, with zero new `.pyspi` entries** — confirmed
   empirically (`testInheritedDictOperations` in each of the three new
   NGTest classes) before assuming it, and you should too for any new type
   built on these.
6. `native/jpype_module/src/test/java/python/io/PyBytesIONGTest.java` and
   any `native/jpype_module/src/test/java/python/collections/*NGTest.java`
   — test shape: `extends PyTestHarness`, construct via the real factory
   (never a hand-built proxy), one `@Test` per behavior.

## The `.pyspi` method-name convention (confirm before assuming)

Every `.pyspi` `METHODS` key for a `PyObject`-rooted interface method is
**dot-prefixed** (`".getvalue"`, `".close"`, `".mostCommonAllRaw"`) per
the `$`/`.` name-mangling convention (`plan/archive/NameMangling.md`):
plain `foo(...)` on the Java interface becomes a collision-free
`".foo"`-keyed `METHODS` entry; a literal `$foo(...)` on the interface
instead bypasses `METHODS` entirely and reads the real Python attribute
`foo` directly. Every `.pyspi` file in the tree as of this writing uses
the dot-prefixed form exclusively — no interface here has needed `$foo`
yet — but this is a convention that has evolved before
(`plan/archive/DispatchFallback.md`) and could again; grep a *current*
`.pyspi` file before writing a new one instead of trusting this
paragraph.

## Two real bugs found while building python.collections — don't reintroduce them

These weren't wiring mistakes; they were wrong assumptions about how
Python semantics carry across the bridge. Expect the same *class* of bug
in future SPI work, even though these two specific spots are now fixed.

**1. A Java `default` method's hardcoded argument doesn't reach the
Python side "for free."**

`OrderedDict.move_to_end(key, last=True)` has a Python-level default
argument. The natural Java translation is:

```java
void moveToEnd(PyObject key, boolean last);
default void moveToEnd(PyObject key) { moveToEnd(key, true); }
```

This looks like it should work — `moveToEnd(key)` is plain Java method
overloading, resolves at compile time to the two-arg version, done. It
doesn't: JPype's proxy dispatch routes **by Python attribute name**, not
by Java method signature, and (based on observed behavior — worth
re-verifying against `Installer`/`ProxyType` source if this surprises you
again) intercepts the call at the interface-method level rather than
letting the JVM run the default method's body and then dispatch the
*inner* call. The one-arg call reached the `.pyspi` lambda directly with
only `(x, key)`, and `_move_to_end(x, key, last)` (no Python default)
raised `TypeError: missing 1 required positional argument`.

**Fix: put the default on the Python side, not the Java side**, exactly
like `io.IOBase.pyspi`'s `_io_seek(x, offset, whence=0)` already does:

```python
def _move_to_end(x, key, last=True):
    x.move_to_end(key, bool(last))
```

The Java `default void moveToEnd(PyObject key)` overload can stay (it's
harmless, arguably good API surface / Javadoc anchor) but do not rely on
it to supply the default — the `.pyspi` function must be able to handle
being called with the shorter argument list directly.

**2. `dict.get()` does not invoke `__missing__` — a subclass's custom
"missing key" behavior is invisible to inherited `PyDict.get()`.**

`Counter`'s famous "returns 0 for a key it doesn't have" behavior is
implemented via `__missing__`, which only fires through `__getitem__`
(`counter[key]`) — **not** through `.get()`. Plain `dict.get(key)` returns
`None` for a missing key on every `dict` subclass, `Counter` included,
because `dict.get()` is a C-level method that never consults
`__missing__`. It is very easy to assume "well `Counter` IS-A `dict`, so
whatever special behavior it has must show up through the inherited `Map`
surface" — that assumption is simply false for this specific method, and
plausibly for others if a future type overrides more dunders.

**Lesson for future SPI plans**: when a stdlib type's docs advertise
special behavior "for missing keys" / "for out-of-range indices" / etc.,
check *which* protocol method actually implements it
(`__getitem__` vs `.get()`, `__delitem__` vs `.pop()`, ...) before writing
Javadoc that claims an inherited core method already covers it. If in
doubt, a one-line Python REPL check settles it immediately:

```
python3 -c "import collections; c = collections.Counter(); print(c.get('x'), c['x'])"
# None 0   <- these differ; a single inherited method cannot cover both
```

**3. `PyMapping.get()`'s default implementation didn't actually match its
own Javadoc — found and fixed while building `PyChainMap`, since
`ChainMap` was the first type in the tree to exercise this default method
at all (`PyDict.get()` is a separate, non-default override, so nothing
using only `PyDict`-rooted types ever hit this path).**

`PyMapping.java`'s `get(Object key)` default was:

```java
default V get(Object key)
{
  return (V) builtin().backend.getitemMappingObject(this, key);
}
```

— a raw `mapping[key]` (`__getitem__`) call, with the Javadoc claiming
"or null if the key is not present." That claim is only true for
`__getitem__` if the object's `__missing__` (or lack thereof) happens to
raise `KeyError` and something downstream catches it — nothing did.
Real Python's `collections.abc.Mapping.get()` mixin is a
`try`/`except KeyError: return default` wrapper around `self[key]`, not
a raw pass-through; confirmed empirically
(`python3 -c "import collections; print(collections.ChainMap({'a':1}).get('missing'))"`
→ `None`, no raise). Fixed by adding the same `try`/`except PyKeyError`
in the Java default (see `PyMapping.java`); full suite re-verified green
on both Python versions after the fix, since this is a shared core
`python.lang` method, not something scoped to `python.collections`.

**Lesson**: a Java default method whose Javadoc describes Python
semantics is a claim about behavior that may never have been exercised
if every prior caller went through a different, unrelated override
(here, every existing `PyMapping`-family type in the tree happened to
also be a real `dict`, so always went through `PyDict.get()` instead).
Adding the first type that actually relies on the shared default is what
surfaces a latent bug like this — write a test for the "or null/default"
half of any such contract explicitly, don't just test the happy path.

## Module-name gotcha to check per type, not assume

`python.io` needs two module names (`"io"`, the public facade where
abstract base classes report `__module__`, and `"_io"`, the C accelerator
where every concrete class actually lives) because CPython's `io` module
does this split. `collections` does not — every class in it
(`deque`, `Counter`, `OrderedDict`, `defaultdict`) reports `"collections"`
as `__module__`, confirmed via:

```
python3 -c "import collections as c; print(c.deque().__module__, c.OrderedDict().__module__, c.defaultdict().__module__, c.Counter().__module__)"
```

Run the equivalent check for whatever module you're wrapping next —
`datetime`, `decimal`, and `pathlib` are all plausible candidates for
having their own `io`/`_io`-style split (C accelerator vs. pure-Python
fallback) and you should not assume single-module-name is the norm.

## Steps, condensed

1. Confirm real `__module__` value(s) for every class you're wrapping
   (see above) — don't assume.
2. Design the Java interface(s). Pick `PyDict` vs `PyMapping` vs "stands
   alone" (like `PyDeque`, which is deliberately not a `List`/`Sequence`
   because `deque` isn't slice-able the way `list` is) based on the
   *real* Python ABC the type implements, not surface-level similarity.
3. Write one `.pyspi` per class under
   `native/jpype_module/src/main/resources/python/<pkg>/spi/`. Only
   Python-specific extras need entries — anything already covered by an
   extended core interface (`PyDict`/`PyMapping`) or an already-registered
   ancestor SPI interface needs no entry (can be `METHODS = {}`).
4. Write (or extend) the package's `WrapperService` impl — one per
   package, scans its own resource dir, nothing to touch when adding a
   class to an already-covered package.
5. Register in `module-info.java` (`provides org.jpype.WrapperService
   with ...`) — only needed once per package, not once per class.
6. Write (or extend) the package's factory interface (`Xxx.using(context)`).
7. Real Javadoc (Audience 1 bar — see `plan/archive/Javadoc.md`), real
   `package-info.java`.
8. NGTest per class, constructed via the real factory. Exercise both the
   inherited generic surface and the type-specific extras. Explicitly
   test any "special missing-key/out-of-range" behavior against both the
   inherited method AND the type-specific method if both exist (lesson
   \#2 above) — don't just test the type-specific one and assume the
   inherited one is consistent.
9. `mvn -o test -Dpython.executable=python3.10` and
   `...python3.12`, full suite, not just the new classes — both green
   before calling it done.
</content>
