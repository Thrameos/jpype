# Real test coverage + customizers for Kotlin, Scala, Groovy

## Status (2026-07-17): scoped, not started

## The problem

`doc/intro.rst`'s "Languages Other Than Java" section currently says
JPype's JVM/JNI-level design means any JVM language "should work by the
same mechanism as Java," and explicitly disclaims that this is untested —
none of Kotlin, Scala, or Groovy are part of the test suite. That's an
honest thing to write, but it's a gap worth closing rather than a
permanent disclaimer: reflection-level access is necessary but not
sufficient, since each language's compiler emits bytecode shapes that
don't always match what a straight Java caller expects (see "Known risk
areas" per language below). The goal of this plan is to turn "should
work" into "tested, and here's what needed a customizer to actually feel
native."

This is forward-bridge work (Python calling JVM-language code), not
reverse-bridge — no relation to `python.*`/`plan/archive/Pathlib.md`-style
work. The precedent to follow is `jpype.beans` (`jpype/beans.py`): a
small, optional customizer module a user explicitly imports, not
something loaded by default.

## Known risk areas per language

Reflection alone gets you method calls and field access. These are the
places past experience with other JVM-language bridges suggests plain
reflection produces something technically callable but awkward or wrong
from Python:

- **Kotlin**
  - Extension functions compile to static methods taking the receiver as
    an explicit first parameter (`fun String.foo()` → `static foo(String
    receiver, ...)` on a `FooKt` file-class). Called via JPype as-is,
    this works but reads as `FooKt.foo(receiver)` instead of
    `receiver.foo()` — a customizer could rebind these onto the receiver
    type if judged worth the ambiguity risk (see Design questions).
  - `object` declarations (singletons) compile to a class with a static
    `INSTANCE` field, not a Python-importable singleton.
  - Nullable types (`String?`) carry no runtime enforcement beyond
    `@Nullable`/`@NotNull` annotations — from Python's side a nullable
    Kotlin return is just a value that might be Java `null`, no different
    from any other Java method. Likely **not** a real gap, but confirm
    rather than assume.
  - Kotlin `data class` component functions (`component1()`,
    `component2()`, ...) exist for destructuring; Python has no
    destructuring-by-position analog worth building a customizer for, but
    `equals`/`hashCode`/`toString` being auto-generated should just work
    via existing `PyObject`-equivalent Java plumbing.

- **Scala**
  - Scala's own collections (`scala.collection.immutable.List`, `Seq`,
    `Map`, ...) are **not** `java.util.Collection` subtypes — they're a
    separate library hierarchy. JPype's existing `_jcollection.py`
    customizer only fires for real `java.util.Collection`/`Map`
    implementers, so Scala collections will NOT get Pythonic iteration
    for free the way `java.util.ArrayList` does. This is the single
    highest-value gap to confirm and, if real, close with a
    `jpype.scala` customizer targeting `scala.collection.Iterable`/
    `scala.collection.Map`.
  - `Option[T]` (`Some(x)`/`None`) is Scala's null-safety idiom. A
    `JConversion`/`JImplementationFor` mapping `Option` to something
    Python-natural (e.g. `.get()` raising vs. a `None`-returning accessor,
    or truthiness matching `isEmpty`) is a plausible customizer, but the
    right shape needs deciding (see Design questions) rather than
    guessing.
  - Companion objects: `object Foo { def bar() = ... }` paired with
    `class Foo` compiles to `Foo$` (the companion, with a static
    `MODULE$` field holding the singleton) plus `Foo` (the class).
    Static-looking companion calls from Java code normally go through
    forwarder static methods on `Foo` itself, so this is often
    transparent — confirm on a real compiled fixture before assuming a
    customizer is needed at all.
  - Implicit conversions/classes are compile-time-only (erased before
    bytecode), so they're invisible to and out of scope for JPype
    entirely — worth stating explicitly in the eventual `_java.rst`-style
    doc so nobody goes looking for them.

- **Groovy**
  - Ordinary Groovy classes compile close to plain Java (implementing
    `groovy.lang.GroovyObject`) and are expected to be the least risky of
    the three — this is the language most likely to "just work" with zero
    customizer.
  - Groovy's dynamic method dispatch (`invokeMethod`/`getProperty` via
    `MetaClass`) lets a Groovy object respond to methods that don't exist
    as real JVM methods at all. Calling one of those from JPype's
    reflection-based dispatch will fail even though it "works" from
    Groovy or Java-via-`GroovyObject.invokeMethod()` — worth one test
    confirming the failure mode is a clean exception, not a crash, even
    if no customizer is built to paper over it in this pass.
  - Groovy closures (`groovy.lang.Closure`) implementing
    `java.util.concurrent.Callable`/`Runnable` should already work via
    ordinary interface dispatch — a good target for the "confirm it just
    works" bucket rather than a design question.

## Infra: prebuilt fixture jars, runtimes only at test time

Decided (2026-07-17, per user direction): don't pull any compiler chain
into the automated build at all. Only each language's **runtime** library
is a real test dependency:

- Kotlin: `org.jetbrains.kotlin:kotlin-stdlib`
- Scala: `org.scala-lang:scala-library` (2.13 — see version note below)
- Groovy: `org.apache.groovy:groovy`

These go into `ivy.xml` under a new `conf name="jvmlangs"` (kept separate
from the default `deps` conf so the base build/test run doesn't grow
three new jars for everyone). No compiler artifact — `kotlin-compiler
-embeddable`, `scala-compiler`, `groovyc` — is ever a build or CI
dependency.

The fixture bytecode itself is **prebuilt once, offline, and checked in**
as `test/jar/jvmlangs/{kotlin,scala,groovy}-fixtures.jar`, matching the
existing precedent of `test/jar/late/late.jar` and `test/jar/mrjar.jar` —
this repo already checks in binary fixture jars it doesn't rebuild as
part of the normal test run. Building each jar is a deliberate, one-off,
manual step (run once by whoever adds/changes that language's fixture,
using that language's real toolchain — a local `kotlinc`/`scalac`/
`groovyc`/IDE, a scratch project, whatever's convenient), not part of
`ant test` or CI. `.kt`/`.scala`/`.groovy` source stays in
`test/jvmlangs/<lang>/` alongside the jar purely for readability and so
a future rebuild has a starting point — it is documentation, not a build
input.

Tradeoff accepted: if a fixture ever needs to change, someone has to
rebuild the jar out-of-band and re-check it in, rather than CI doing it
automatically. Given these fixtures are meant to be small and stable
(exercising fixed bytecode shapes like "extension function" or "case
class," not evolving business logic), that's judged an acceptable
one-time cost against not carrying any compiler dependency at all.

**Version note (Scala)**: Scala 2.13 and Scala 3 have incompatible
compilers and, more importantly here, `scala.collection` shapes differ
in ways that matter for the "does JPype's collection customizer see
these" risk item. Building the fixture jar with Scala 2.13 is the
default choice (still the more common production target as of this
writing) — if Scala 3 support ends up mattering, that's a second,
separate fixture jar (`scala3-fixtures.jar`), not a retrofit of this
one.

## Fixture source per language

Small, single-purpose source files, one per risk area identified above —
not full test programs, just enough surface for a JPype test to poke at.
Compiled once (see above) into the matching prebuilt jar:

- `test/jvmlangs/kotlin/Fixtures.kt` → `kotlin-fixtures.jar` — one
  `object` singleton, one `data class`, one extension function on a JDK
  type (e.g. `String`), one nullable-returning method.
- `test/jvmlangs/scala/Fixtures.scala` → `scala-fixtures.jar` — one
  `case class`, one method returning `scala.collection.immutable.List`,
  one method returning `Option[String]` (both `Some` and `None` cases
  reachable), one companion object with a real method.
- `test/jvmlangs/groovy/Fixtures.groovy` → `groovy-fixtures.jar` — one
  plain class (baseline "just works" check), one class overriding
  `invokeMethod`/`getProperty` (the dynamic-dispatch failure-mode
  check), one `Closure`-returning method.

## Test modules

New files under `test/jpypetest/`, one per language
(`test_kotlin.py`/`test_scala.py`/`test_groovy.py`), each skipping
cleanly rather than failing when its runtime jar wasn't resolved — same
`common.unittest.skipUnless(...)` idiom `test_buffer.py` already uses for
optional numpy coverage, e.g. `skipUnless(haveKotlinRuntime(), "kotlin
runtime not available")`. Each file exercises the fixtures above both
*without* and (once built) *with* its language's customizer loaded, so
the tests double as the "before/after" evidence for whether each
customizer actually earns its place.

## Customizer modules (only for confirmed real gaps)

`jpype/kotlin.py`, `jpype/scala.py`, `jpype/groovy.py` — optional,
explicit-import modules following the `jpype.beans` precedent exactly
(module-level docstring explaining what it changes and why it isn't
default, registration happens at import time via `@JImplementationFor`/
`@JConversion` against `_jcustomizer.py`'s existing API, nothing
retroactively enabled without the user opting in).

**Do not build a customizer for a risk area until the fixture test above
has actually confirmed the gap is real** — several of the items in "Known
risk areas" are marked as plausible-but-unconfirmed. Building the Scala
collections customizer, for instance, only makes sense once a test
proves `scala.collection.immutable.List` genuinely doesn't iterate
Pythonically today.

## Steps

1. `ivy.xml`: add the `jvmlangs` conf with the Kotlin/Scala/Groovy
   **runtime** dependencies only.
2. Write the three fixture source files; compile each offline (local
   toolchain, one-off) into `test/jar/jvmlangs/*.jar` and check the jars
   in.
3. Write the three test modules against the prebuilt fixture jars, no
   customizer yet — this step alone already confirms or refutes every
   "plausible" item in "Known risk areas," turning guesses into facts.
4. For each confirmed real gap: design + implement the minimal customizer,
   extend that language's test module to cover the "with customizer
   loaded" behavior, document the customizer's existence and rationale
   (a short `jpype.kotlin`/`jpype.scala`/`jpype.groovy` usage note,
   following `jpype.beans`'s docstring shape).
5. Update `doc/intro.rst`'s "Languages Other Than Java" section once any
   of this lands — replace "should work by the same mechanism... not a
   supported, verified configuration" with what's actually now tested,
   keeping the caveat only for whichever of the three still isn't.

## Verification

- New language test modules pass when their runtime jars are resolved
  and the prebuilt fixture jars are present; skip cleanly (not error)
  when `ivy.xml`'s `jvmlangs` conf wasn't fetched, so the base test suite
  has zero new required dependencies.
- Full existing suite still green — customizers are opt-in imports, so
  loading them must not be required for, or interfere with, any existing
  test.
- Each shipped customizer has a test demonstrating the specific
  before/after behavior it fixes (e.g. Scala `List` iterating in a
  Python `for` loop only after `import jpype.scala`), not just a
  smoke-test that it imports without error.
