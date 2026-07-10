# SPI: Java-registered Python dispatch hooks

## TL;DR

The cost of this feature is much lower than it looks. There is no new
mechanism to build — both extension points already exist and are idle:

1. `st->cacheDict[type] = (interfaces, methods)` in `pyjp_probe.cpp` — "this
   Python type satisfies these Java interfaces."
2. The dispatch dict behind `JPProxyIndirectDict` in `jp_proxy.cpp` — "this
   method name resolves to this callable."

The SPI is just: **a file format for declaring injects (class name,
interface list, method-name-to-binding pairs) plus a loader in
`_jbridge.py` that reads it at init and populates those two maps.** No new
C++ dispatch code, no new probe logic — just data plus a populate step.

## Goal

Allow a Java module to register additional handlers into the reverse-proxy
dispatch tables so that Python-side objects gain new callable "special
methods" without hand-writing a full `@JImplements` class per case. Motivating
use case: build classes that behave like numpy arrays (or other
Python-protocol-heavy types) directly from Java, by injecting operator/dunder
implementations at bind time.

Target call shapes, all resolving to a single registered handler named `foo`:

```
$foo(arg)
$foo(arg, arg)
$foo(arg, PyKeyArgs)
$foo(...arg)
```

The `$`-prefix is the marker that says "this call should be resolved through
the SPI dispatch table by stripping the prefix," as opposed to an ordinary
interface method name.

## What already exists (confirmed 2026-07-09)

Dispatch fallback already exists and does most of the needed work:

- `jpype/_jbridge.py:233` — `_PyJPBackendMethods`, a hardcoded Python
  `dict[str, Callable]` built once in `initialize()`.
- `jpype/_jbridge.py:911` — `Backend@JProxy(Backend, dict=_PyJPBackendMethods)`
  constructs the proxy with `dict=`, selecting the indirect-dict resolution
  path.
- `native/python/pyjp_proxy.cpp:87` — `dict=` kwarg selects
  `JPProxyIndirectDict` (vs `JPProxyDirect` / `JPProxyIndirectAttr`).
- `native/common/jp_proxy.cpp:371-379` —
  `JPProxyIndirectDict::getCallable()`: `PyDict_GetItem(m_Instance->m_Dispatch, name)`,
  falling back to `PyObject_GetAttr` on the proxy instance if the name isn't
  in the dict. This is the "fall back to lookup" behavior — but it's an
  **exact-key** lookup only; no prefix stripping.
- Java side: `MethodDescriptor` (`org/jpype/proxy/MethodDescriptor.java:25-40`)
  already carries `name`, `defaultHandler` (MethodHandle), and a `bypass`
  flag per interface method. `ProxyType.buildDescriptor`
  (`org/jpype/proxy/ProxyType.java:160-186`) builds one per interface method;
  `ProxyInstance.invoke()` (`org/jpype/proxy/ProxyInstance.java:37-103`)
  checks `md.bypass` before falling through to `hostInvoke`, which lands in
  `Java_org_jpype_proxy_ProxyInstance_hostInvoke`
  (`native/common/jp_proxy.cpp:128`) and calls `proxy->getCallable(...)`
  (~line 197) — i.e. the dict/attr lookup above.
- Keyword-argument plumbing for this path is done (see
  `PyKeyArgs`/`ProxyInstance.invoke` — commit `14ec2596`): a trailing
  `PyKeyArgs` vararg is already flattened into `numPositional`/`numKeyword`
  before crossing into native code, so `$foo(arg, PyKeyArgs)` requires no new
  work beyond naming/resolution.

**Not implemented yet:**

1. `$`-prefix stripping / name-mangling — grepped, zero hits. Purely
   conceptual today.
2. Runtime registration — `_PyJPBackendMethods` is a static dict literal, not
   an API a Java module can add entries to.
3. There is an **unwired** ServiceLoader precedent already scaffolded:
   `org/jpype/WrapperService.java` + `WrapperProvider.java:12-65`
   (`ServiceLoader.load(WrapperService.class)`, intended to map Python module
   names to Java interfaces). Nothing else in the tree references it, and no
   `META-INF/services/org.jpype.WrapperService` file exists. This looks like
   the closest existing shape for "a Java module injects hooks" and is the
   leading candidate to revive rather than build a parallel mechanism.

## Proposed design (revised 2026-07-10, grounded in `WrapperService`)

### Key finding: the cache lookup is `PyObject_GetItem`, not `PyDict_GetItem`

`PyJP_probe(st, type)` (`native/python/pyjp_probe.cpp:228`) is entirely gated
by a single cache check at line 231:

```cpp
JPPyObject cached = JPPyObject::use(PyObject_GetItem(st->cacheDict, (PyObject*) type));
if (cached.isValid())
        return cached.keep();
PyErr_Clear();
```

`st->cacheDict` is `_jpype._cache`, a plain Python `dict` loaded once via
`loadAttr(module, "_cache")` (`pyjp_module.cpp:163`). Critically, the probe
reads it with `PyObject_GetItem` — the generic subscript operator — not the
C-level `PyDict_GetItem` (which is used a few lines later for
`st->concreteDict`, at line 258, and does *not* respect Python-level
overrides). `PyObject_GetItem` on a `dict` subclass honors `__missing__`
exactly like `collections.defaultdict`.

**This means lazy SPI resolution needs zero new C++ code.** Replace
`_jpype._cache` with a `dict` subclass implementing `__missing__(self, type)`:
on a genuine miss it does the SPI resolution (below), either populates
`self[type]` and returns the tuple (probe never runs), or raises `KeyError`
(the existing `PyErr_Clear()` at line 234 already handles that and falls
through to the normal MRO-scan/`interrogate()` path, completely unchanged).
No pre-seeding, no write-once-cache risk, no new probe logic — the hook lives
entirely in `_jbridge.py`.

The tuple shape a resolved entry must produce is still whatever
`finalizeInterfaces` (`pyjp_probe.cpp:134-157`) / `finalizeMethods`
(`pyjp_probe.cpp:159-198`) produce: `(interfaces_tuple, methods_dict)`. The
`__missing__` hook should call those two conceptually (or replicate their
contract) rather than inventing a second shape.

### Lazy granularity: by module, not by class

A naive design resolves one Python *class* per SPI round trip. But the
motivating use case (`numpy.ndarray`-style third-party types) means the
target class often doesn't exist as a `PyTypeObject*` until its package is
imported — eager, boot-time registration cannot work for it, so lazy is not
optional, it's the primary mechanism. Given that, batch by **module** instead
of by class: one Java round trip resolves everything a provider knows about
for that Python module, and every class in it gets cached in one shot.

Checked empirically against the `io` module (the intended first provider,
see `plan/IO.md`):

```
IOBase             module=io    bases=['_IOBase']
BytesIO            module=_io   bases=['_BufferedIOBase']
StringIO           module=_io   bases=['_TextIOBase']
FileIO             module=_io   bases=['_RawIOBase']
BufferedReader      module=_io   ...
TextIOWrapper       module=_io   ...
```

Gotcha to design around: the *public* abstract classes (`io.IOBase` etc.)
report `__module__ == "io"`, but every concrete class report
`__module__ == "_io"` (the C accelerator module backing the `io` facade).
A hook keyed on the leaf class's own `__module__` alone will never resolve
`BytesIO`. Resolve against the set of `__module__` values across
`type.__mro__`, not just `type.__module__`, and expect real third-party
providers (numpy has the same C-extension/Python-facade split) to need the
same treatment.

```python
_triedModules = set()

class _CacheDict(dict):
    def __missing__(self, type_):
        for mod in {c.__module__ for c in type_.__mro__} - _triedModules:
            _triedModules.add(mod)
            manifest = _jpype.Backend.spiResolveModule(mod)  # one call per unseen module
            if manifest:
                for cls, entry in manifest.items():
                    dict.__setitem__(self, cls, entry)
        if type_ in self:
            return self[type_]
        raise KeyError(type_)  # falls through to normal probe, unchanged
```

Per-class lazy remains available as a fallback for a provider that can't
manifest a whole module up front, but module-batch should be the default
contract.

### The `Installer` surface: eager for interfaces, lazy for class membership

Split the registration surface along a line that falls out of the above:

- **Java interfaces are compiled classes, known at JVM boot.** Their method
  dicts (what currently ships hardcoded as
  `_jpype._methods[_PyBytes] = _PyBytesMethods` etc., `_jbridge.py:1006+`)
  can and should be registered **eagerly**, during `_jbridge.initialize()`,
  regardless of whether any Python type using them has been imported yet.
  There's no reason to defer this — the interface exists whether or not a
  satisfying Python class has shown up.
- **Which Python type satisfies which interface is the part that has to be
  lazy**, per the module-batch hook above, since the Python type may not
  exist yet.

One `Installer` interface, used both ways, unifies this with the existing
`WrapperService`/`WrapperProvider` scaffold
(`org/jpype/WrapperService.java`, `WrapperProvider.java:12-65` — currently
unwired, zero references elsewhere in the tree, no
`META-INF/services/org.jpype.WrapperService` file):

- `installer.registerInterface(Class<?> javaInterface, methodBindings)` —
  called once per provider at init; writes into `_methods` the same way the
  hardcoded lines do today.
- Extend `WrapperService` (currently only `getInterfaces(String clsName)`,
  one class at a time — `WrapperProvider.java:36-39`) with a batch method:
  ```java
  default Map<String, Class<?>[]> getModuleManifest(String moduleName) {
      return null; // provider opts in; null means "fall back to per-class"
  }
  ```
  `WrapperProvider` already shards services by module name
  (`moduleToServiceMap`, keyed by the string before the last `.` in the
  Python qualified name) — the batch method just needs to be called from the
  `__missing__` hook via a new `Backend.spiResolveModule(String)` bridge
  entry point instead of always going through `getInterfaces` one class at a
  time.

This also folds in cleanly as the loader for `io` becoming an actual SPI
provider itself (see `plan/IO.md`) rather than a hardcoded factory — `io`
proves the `Installer`/`WrapperService` contract works for a real,
non-trivial class hierarchy before any third party depends on it.

### The Java-method-name dispatch side (`$foo`)

Two small pieces, not a new dispatch engine:

1. **Registration**: make the dispatch dict extendable per proxy/type rather
   than one hardcoded global. Likely shape: a Java-side resource (map
   entries, similar to how `WrapperProvider` already maps names to
   interfaces) read at bind time and injected into the dict backing
   `JPProxyIndirectDict`, so `foo` becomes resolvable without editing
   `_jbridge.py`.
2. **Dispatch**: when a Java interface method is named `$foo`, strip the `$`
   before doing the existing dict/attr lookup, so it resolves to whatever was
   registered under `foo`. This is a small change in
   `JPProxyIndirectDict::getCallable()` (`jp_proxy.cpp:371`) or in
   `MethodDescriptor`/`ProxyType.buildDescriptor` when the name is interned —
   TBD which layer should own the stripping.

Open questions for next session:
- Should `$foo` registration be per-`ProxyType` (interface) or global (one
  shared dispatch table across all reverse proxies)?
- Where does the actual Python-side callable come from when a Java module
  "injects hooks" — is the resource a class name to instantiate, a static
  method reference, or something else?
- Does the class-membership resolution (`_cache.__missing__`, above) and the
  `$foo` method-dispatch mechanism end up sharing the same `Installer`
  registration call, or stay two entry points on one interface (one for
  "what interfaces does this type satisfy," one for "what does this method
  name resolve to")? Current lean: two methods on one `Installer`, since
  they populate different tables (`_cache`/`_concrete`/`_methods` vs. the
  `JPProxyIndirectDict` dispatch dict) even if a single provider often wants
  to call both.
- Resolved this pass: cache injection is a `__missing__` hook, not a
  pre-seed, so the "is `cacheDict` write-once / can a bad entry be
  permanent" risk from the previous draft no longer applies — a hook can be
  fixed and will simply be consulted again on the next miss for a type not
  yet cached.

## Other work queued behind this (not started)

- **Test bench**: more coverage in general, lower risk, should probably be
  done before/alongside SPI work since it doesn't touch the same code paths.
- **Subinterpreter testing**: flagged as the highest-risk remaining item
  (silent/racy failures, interacts with proxy lifetime and PEP 684 state) —
  should get its own dedicated session, not be interleaved with SPI work.
