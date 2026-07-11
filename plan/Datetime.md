# python.datetime: SPI provider for Python's datetime module

## Status (2026-07-11): scoped, not started

## The problem

There is currently no Java front-end for Python's `datetime` module.
Any Python API that hands back a `datetime.date`/`datetime.datetime`/
`datetime.timedelta` currently crosses into Java as a bare `PyObject`,
with no typed methods and no conversion to the Java standard library's
equivalent types. `datetime` is one of the most commonly used stdlib
modules in real Python code, so this is a real gap for the reverse
bridge's usefulness, not a hypothetical one.

Scoped alongside [[jpype_collections_spi_plan]] (`plan/Collections.md`) as
part of proving the `WrapperService` SPI mechanism generalizes past
`python.io` to more than one third-party-shaped provider without
conflicts. See that plan and `plan/archive/SPI.md` /
[[jpype_spi_installer_status]] for the mechanism itself — this plan only
scopes the `datetime`-specific parts.

## Existing forward-direction integration (do not confuse with this plan)

JPype already has forward-bridge (Python object satisfying a Java method
contract) duck-typing for `datetime`, registered as `JConversion`
customizers in `jpype/protocol.py`: `datetime.datetime` → `java.time.Instant`
(`exact=datetime.datetime`, ~line 124) and `datetime.time`/`date`/`datetime`
→ `java.sql.Time`/`Date`/`Timestamp` (~lines 160-172, for `dbapi2`). This is
a *different* code path — structural, per-call, constructs a fresh Java
object each time via `JPClassHints`/`JPClassHints::matches`, no persistent
identity, and used when a Python value is passed *into* a Java method that
expects a `java.time`/`java.sql` type. It does **not** give Java code a
typed front-end object for a `datetime` value it received *from* Python
(this plan's actual goal, the reverse direction), and this plan's SPI work
doesn't touch or conflict with it. Worth reusing as reference for how
Python↔Java datetime semantics were already resolved (naive vs. aware, unit
precision) rather than re-deriving them from scratch.

## Why SPI, not core

Same reasoning as `Collections.md`: build this as a `WrapperService`
provider (model: `python.io.PyIOWrapperService`,
`native/jpype_module/src/main/java/python/io/PyIOWrapperService.java`),
registered via `module-info.java`'s `provides
org.jpype.WrapperService with ...`, not hand-wired into `_jbridge.py` or
core `python.lang`.

## Scope: three Python `datetime` types

- `datetime.date` → `PyDate` — map read accessors (`year`, `month`,
  `day`) and consider a `toLocalDate()` promotion to `java.time.LocalDate`,
  mirroring how `python.io`'s `PyBufferedIOBase.asInputStream()` promotes
  to a standard Java type rather than only exposing the Python-native
  shape.
- `datetime.datetime` → `PyDateTime` — extends/relates to `PyDate`
  (Python's own `datetime` subclasses `date`); promotion to
  `java.time.LocalDateTime`/`java.time.Instant` (need to decide which,
  given `datetime.datetime` can be naive or timezone-aware — aware
  instances should probably promote to `Instant`/`ZonedDateTime`, naive to
  `LocalDateTime`; check both cases against a real interpreter rather than
  assuming).
- `datetime.timedelta` → `PyTimeDelta` — promotion to `java.time.Duration`
  (straightforward: Python's `timedelta` is already a fixed span, unlike
  the calendar-aware `date`/`datetime` split).

`datetime.time` and `datetime.tzinfo` are lower priority — include only if
trivial once the other three are working, not blocking.

## Design questions to resolve before coding (not decided by this plan)

- Whether `PyDate`/`PyDateTime` should be Java `Comparable`, matching
  Python's own rich comparison support.
- Whether construction should go through the interpreter (Python
  `datetime.date(...)` call) or whether a pure-Java-side construction path
  makes sense for values that never need to touch Python again — check
  what `python.io`/`python.lang` precedent exists for this before
  inventing a new pattern.
- Immutability: Python's `datetime` types are immutable value types: the
  Java front-end should be too (no setters), same spirit as
  `PyBytes`/`PyTuple` in `python.lang`.

## Naming-mangling interaction

Same caveat as `Collections.md`: these are `PyObject`-rooted proxy
interfaces, so `.pyspi` method keys need whatever the current `$`/`.`
mangling convention is at execution time
([[jpype_dispatch_fallback_status]], [[jpype_name_mangling_plan]]) —
verify against a live `.pyspi` file, don't assume it matches this plan's
text.

## Steps (mirror `plan/Collections.md` / `plan/archive/IO.md`)

1. Design the three interfaces in a new `python.datetime` package.
2. Resolve the design questions above against a real Python 3.10+
   interpreter (module/class names, aware-vs-naive `__module__` behavior,
   comparison/immutability shape) before locking the Java API.
3. One `.pyspi` resource per class under
   `native/jpype_module/src/main/resources/python/datetime/spi/`.
4. `PyDatetimeWrapperService` implementing `WrapperService`, registered in
   `module-info.java`.
5. End-user Javadoc at the `plan/archive/Javadoc.md` "Audience 1" bar, plus
   a real `python/datetime/package-info.java`.
6. Tests: one NGTest class (or one per type) constructing through the real
   SPI path, exercising both the Python-native accessors and the
   `java.time` promotion methods.

## Verification

- Full suite green on python3.10 and python3.12.
- Confirm a second SPI provider (`python.io` already live) coexisting with
  this one doesn't produce any registration-order or module-name
  collision — this is part of the "prove more than one provider works
  cleanly" motivation for doing `collections` and `datetime` alongside
  `io`.
</content>
