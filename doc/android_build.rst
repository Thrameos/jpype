Android Build/Test Harness
===========================

This page documents ``project/android/``, a project-local harness for actually
cross-compiling JPype's native ``_jpype`` extension for Android, packaging it
into a minimal test app, and running it on an emulator. It exists because the
:doc:`android` feature/behavior differences page describes what the
``#ifdef ANDROID`` code in ``native/`` is *supposed* to do, but until this
harness there was no way to actually build or run that code from this repo -
issue `#1257 <https://github.com/jpype-project/jpype/issues/1257>`_ (an
Android crash) had to be root-caused and fixed from a stack trace and static
analysis alone.

This is project-local developer tooling, not something JPype ships or
installs for end users - it is not referenced from ``pyproject.toml`` and has
no effect on a normal ``pip install jpype1``.

Background: why buildozer/python-for-android
----------------------------------------------

Two options were evaluated: `Chaquopy <https://chaquo.com/chaquopy/>`_ and
`buildozer/python-for-android (p4a) <https://python-for-android.readthedocs.io/>`_.
Chaquopy was ruled out: its ``pip install`` mechanism explicitly refuses to
compile native C extensions (there is a public bug report of someone hitting
exactly this wall trying to install a package that depends on ``jpype1``), and
there is no documented way to bundle a prebuilt ``.so`` around that
restriction either.

p4a fits: its sibling project **pyjnius** (also a Python<->Java bridge over
JNI, architecturally the same problem JPype has) already ships a working
recipe (``pythonforandroid/recipes/pyjnius``) that this harness's own recipe
(``project/android/recipes/jpype1``) is modeled on, including reusing the same
``WebView_AndroidGetJNIEnv()`` hook that p4a's "webview" bootstrap exposes for
getting a ``JNIEnv*`` from the already-running Dalvik/ART VM.

What's in ``project/android/``
-------------------------------

``native/android_jnienv.c``
  A reference implementation of ``Android_JNI_GetEnv()`` - declared
  ``extern`` in ``native/python/pyjp_module.cpp`` but deliberately never
  defined there, since it must come from whatever app embeds JPype. This
  file supplies it for p4a's webview bootstrap specifically. A different
  host application (a from-scratch Android Studio project, a different p4a
  bootstrap, ...) would supply its own definition instead - that's why this
  file lives here and not in ``native/``.

``recipes/jpype1/__init__.py``
  The p4a recipe. Builds directly from this repo's own working tree (not a
  release tarball) via a custom ``prepare_build_dir`` that ``rsync``\s the
  repo into the build dir (excluding ``.git``, any host ``build/`` directory,
  and this harness's own ``recipes``/``testapp`` subdirectories - a stale
  *host*-arch build must never leak into an Android cross-build, matching
  ``CLAUDE.md``'s isolation rule for normal builds too). Passes the NDK
  toolchain file and per-arch ABI/platform through to jpype's own CMake build
  using scikit-build-core's generic ``cmake.define.<VAR>=<value>``
  config-settings mechanism (the same mechanism ``CLAUDE.md`` documents for
  ``BUILD_TEST_HARNESS`` in the normal host build) - no changes needed on
  jpype's ``pyproject.toml`` side for that part. Its ``postbuild_arch`` copies
  the ``org.jpype.*`` Java sources (``native/jpype_module/src/main/java/org``)
  into p4a's Java class dir so they get compiled to dex and bundled into the
  APK, since ``JPClassLoader`` (``native/common/jp_classloader.cpp``) needs
  ``org/jpype/JPypeClassLoader`` to already be loadable on-device - Android
  can't do jar-based ``addClassPath`` (see :doc:`android`).

``testapp/``
  A minimal headless app (``buildozer.spec`` + ``main.py``) that imports
  jpype, calls a trivial ``JClass``, and runs a regression check for #1257
  (see comments in ``main.py`` for exactly what it checks and why calling
  ``_jpype.bootstrap()`` a second time is used as a stand-in for the original
  crash trigger, which depended on the original reporter's own modified
  bindings and isn't practical to reproduce exactly).

Changes to core ``native/``
-------------------------------------

Building for a real NDK target - not just adding the harness - surfaced
several genuine bugs in the shared ``native/`` and ``native/jpype_module/``
sources, all fixed in core, not worked around in this harness:

- ``native/CMakeLists.txt`` gained an ``ANDROID`` branch (checked before the
  generic ``LINUX OR UNIX`` branch, which Android would otherwise fall into
  since it's Linux-derived): links against NDK's ``libc++_shared.so`` instead
  of glibc's ``libstdc++.so.6``; adds ``--allow-shlib-undefined``/
  ``--unresolved-symbols=ignore-all`` so ``Android_JNI_GetEnv()`` (see below)
  can stay a deliberately-unresolved-at-link-time reference; compiles in
  ``project/android/native/android_jnienv.c`` when present; and links against
  an optional ``JPYPE_ANDROID_LIBMAIN_DIR`` (see the linker-namespace note
  below).
- ``native/common/jp_context.cpp``: ``AttachCurrentThread``/
  ``AttachCurrentThreadAsDaemon`` took a ``(void**) &env`` cast that's correct
  for desktop JNI headers, but Android's NDK ``jni.h`` declares the *same*
  functions with a ``JNIEnv**`` parameter instead - a real, incompatible
  header divergence between platforms, not a style difference. Fixed with an
  ``#ifdef ANDROID`` branch per call site (3 total).
- ``native/jpype_module/.../org/jpype/JPypeContext.java``: ``getHeapMemory()``
  used ``java.lang.management.MemoryMXBean``, unavailable on Android's
  platform API. Replaced with ``Runtime.getRuntime().totalMemory() -
  freeMemory()``, which is portable to every JVM (not just an Android
  workaround) - no platform branching needed since Java has no preprocessor.
- ``native/jpype_module/.../org/jpype/JPypeUtilities.java``: the sealed-class
  detection used ``MethodHandleProxies.asInterfaceInstance``, also
  unavailable on Android. Replaced with a plain ``Method.invoke()``-based
  lambda - portable, and no less correct than the original on desktop.

All three of these were also present, independently, in the original #1257
reporter's own local patch - confirming they're real, load-bearing fixes any
Android build needs, not artifacts of this particular harness.

Setup
-----

Install the Android SDK/NDK/emulator pieces (adjust versions/paths as
needed; this was verified against SDK cmdline-tools already present at
``~/android-sdk``)::

    ~/android-sdk/cmdline-tools/latest/bin/sdkmanager \
        "emulator" \
        "system-images;android-34;google_apis;x86_64" \
        "ndk;25.1.8937393"

    ~/android-sdk/cmdline-tools/latest/bin/avdmanager create avd \
        -n jpype-test -k "system-images;android-34;google_apis;x86_64"

p4a's toolchain (as of the version in use when this was written) still looks
for ``sdkmanager``/``avdmanager`` at the legacy ``tools/bin/`` location
rather than the current ``cmdline-tools/latest/bin/`` one - without this,
buildozer decides the SDK's sdkmanager "is not installed" and tries to
download a second copy of the whole SDK. Symlink the legacy path in::

    mkdir -p ~/android-sdk/tools/bin
    ln -sf ~/android-sdk/cmdline-tools/latest/bin/sdkmanager ~/android-sdk/tools/bin/sdkmanager
    ln -sf ~/android-sdk/cmdline-tools/latest/bin/avdmanager ~/android-sdk/tools/bin/avdmanager

Hardware acceleration needs the running user in the ``kvm`` group
(``sudo usermod -aG kvm $USER``, then a new login session) - otherwise the
emulator still works, just much slower (software rendering).

Use a JDK the bundled Gradle wrapper actually supports for ``JAVA_HOME``
during the build - a too-new JDK fails with
``BUG! exception in phase 'semantic analysis' ... Unsupported class file
major version NN`` (NN-44 is the Java version Gradle can't parse; e.g. 69
means Java 25). JDK 21 was verified working; a JDK newer than what your
Gradle wrapper version supports will not.

Per ``CLAUDE.md``'s build-isolation rule, use a disposable venv for the
build tooling too, kept separate from JPype's own dev venvs (this one never
touches JPype's C extension build directly - p4a drives its own
sub-builds). It must be created with ``--system-site-packages``: p4a's own
bootstrap does a plain ``pip install --user ...`` internally, and pip
unconditionally refuses ``--user`` inside any virtualenv unless
``site.ENABLE_USER_SITE`` is true, which only happens with
``--system-site-packages``::

    python3.12 -m venv --system-site-packages /tmp/venv-android
    /tmp/venv-android/bin/pip install --upgrade pip
    /tmp/venv-android/bin/pip install buildozer cython
    export PATH=/tmp/venv-android/bin:$PATH   # buildozer resolves `cython` etc.
                                               # via plain PATH lookup, not
                                               # sys.executable's directory

Build, deploy, run, verify
----------------------------

From ``project/android/testapp/``, with a JDK 21 (see above) on ``JAVA_HOME``::

    export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64   # adjust to your JDK 21
    buildozer android debug

**Whenever jpype1's source changes** (anything under ``native/``,
``native/jpype_module/``, or ``project/android/recipes/jpype1/``), p4a's own
build caching will otherwise silently reuse a stale build - it does not
hash sources, only checks "does expected output already exist". Before
rebuilding, always clear *all five* of these together (partially clearing
them has caused real failures - corrupting ``build/venv``'s pip install, or
silently reusing a stale ``_jpype.so`` - more than once)::

    D=.buildozer/android/platform/build-x86_64/build
    rm -rf "$D/other_builds/jpype1-genericndkbuild" "$D/javaclasses" \
           "$D/../dists" "$D/python-installs" "$D/venv"

Expect the *first* build to take real iteration - toolchain-flag wrangling
between p4a's NDK cross-compile environment and jpype's CMake build is the
main source of friction - and to be slow (p4a's own dependency chain,
30-60+ minutes uncached). Subsequent rebuilds are much faster, and an
app-only change (just ``testapp/main.py``, no jpype1 source change) rebuilds
in seconds.

Deploy to the emulator (create it once, matching the setup step above)::

    ~/android-sdk/platform-tools/adb install -r bin/jpypetest-0.1-x86_64-debug.apk
    ~/android-sdk/platform-tools/adb logcat -c
    ~/android-sdk/platform-tools/adb shell am start -n org.jpype.test.jpypetest/org.kivy.android.PythonActivity
    ~/android-sdk/platform-tools/adb logcat -d | grep -i 'GOLDEN\|REGRESSION'

If the emulator isn't already running, launch it with KVM acceleration via
``sg kvm`` (works without a full new login session, unlike a bare
``usermod -aG kvm`` which only takes effect on next login)::

    sg kvm -c "~/android-sdk/emulator/emulator -avd jpype-test -no-window -no-audio -no-boot-anim -gpu swiftshader_indirect > /tmp/emulator.log 2>&1" &
    ~/android-sdk/platform-tools/adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 2; done'

A working build/run prints (see ``testapp/main.py``)::

    === jpype android testapp starting ===
    GOLDEN PATH: PASS (...)
    REGRESSION CHECK #1257: PASS - caught ... instead of crashing
    === jpype android testapp done ===

If the golden path fails, ``main.py`` prints ``GOLDEN PATH: FAIL`` and the
exception, then still runs the regression check (it does not exit early -
these are two independent checks). If the regression check fails *and* the
process aborts instead of printing ``REGRESSION CHECK #1257: ...`` at all,
look for a SIGABRT / ``libc++abi: terminating due to uncaught exception`` in
logcat - that is the original #1257 crash signature, meaning something
reintroduced the missing exception handling in ``PyJPModule_bootstrap()``.

Two runtime linking issues, beyond what compiles/links on the host, only
show up once the APK actually runs on-device - both fixed in the recipe,
documented here since they generalize to any similar Android C-extension
recipe:

- **libc++_shared.so must be bundled explicitly.** ``_jpype.so`` links
  against it, but it's an NDK runtime library, not part of Android's system
  image - without copying it into the APK's native lib dir (this recipe's
  ``postbuild_arch`` does so via p4a's ``install_libs()``), you get
  ``dlopen failed: library "libc++_shared.so" not found`` at import time.
- **Android's linker namespace isolation blocks implicit cross-namespace
  symbol resolution**, even for symbols that are genuinely exported.
  ``Android_JNI_GetEnv()`` calls ``WebView_AndroidGetJNIEnv()``, exported
  (confirmed via ``readelf --dyn-syms``) by the webview bootstrap's
  ``libmain.so`` - but ``_jpype.so``, loaded via Python's import machinery,
  ends up in a different linker namespace than ``libmain.so``, loaded by the
  app's native launcher, so an unresolved/deferred-to-runtime reference to
  it fails with ``dlopen failed: cannot locate symbol
  "WebView_AndroidGetJNIEnv"`` even though ``--allow-shlib-undefined``/
  ``--unresolved-symbols=ignore-all`` let it link cleanly. An *explicit*
  ``DT_NEEDED`` dependency, in contrast, does get resolved across that
  boundary - this is exactly what the upstream pyjnius recipe does with its
  own ``-L.../libmain*.so`` linking, and what this recipe now does too, via
  ``native/CMakeLists.txt``'s ``JPYPE_ANDROID_LIBMAIN_DIR`` cmake define
  (**not** the ``LDFLAGS`` environment variable - scikit-build-core doesn't
  read ambient ``LDFLAGS`` the way legacy setuptools/distutils builds did;
  an env-var-only attempt at this silently has no effect on the actual link
  line, which cost real time to notice).

Known limitation: JPype's reflector generation on ART
---------------------------------------------------------

With everything above fixed, the harness reaches real JPype/JVM interaction
- ``PyJPModule_bootstrap()`` succeeds, resources load, ``JClass(...)`` reaches
real Java code - but creating a wrapped class currently fails with::

    java.lang.RuntimeException: Unable to create reflector org.jpype.Reflector0

JPype generates a small reflector class per wrapped Java class as part of its
normal reflection/dispatch mechanism; this generation apparently doesn't work
as-is on ART. This is caught cleanly as a Python ``SystemError`` - not a
crash, so it doesn't indicate a #1257-style regression - but it does mean the
golden path in ``testapp/main.py`` only gets as far as obtaining the
``JClass`` object, not actually calling into it. Making JPype's reflector
mechanism ART-compatible is real follow-up work, out of scope for this
harness itself; this is the concrete next target for whoever picks that up,
now that the harness can actually reproduce and iterate on it.

numpy: a quick attempt, not pursued further
------------------------------------------------

p4a ships a ``numpy`` recipe, so ``requirements = python3,jpype1,numpy`` in
``buildozer.spec`` was tried, to see how close a real ``test/jpypetest`` run
on-device might be. It fails to compile against this NDK's libc++::

    ../numpy/_core/src/multiarray/unique.cpp:123:6: error: no template named
    'unordered_map' in namespace 'std'; did you mean 'unordered_set'?

This is a bug in numpy's own vendored source (that file uses
``std::unordered_map`` while only transitively relying on
``<unordered_set>``'s include of it, which NDK's stricter libc++ doesn't do)
- not a JPype issue, and not something to patch from this repo. The likely
real fix is a small patch adding ``#include <unordered_map>`` to that file,
applied the same way p4a recipes normally patch upstream sources (a
``.patch`` file on the ``numpy`` recipe, analogous to the ``use_cython.patch``
pyjnius carries) - left as a documented next step rather than pursued here,
since it's orthogonal to what this harness itself needed to prove.

Reproducing a *new* Android bug report
-----------------------------------------

Before this harness, an Android-specific bug report could only be
root-caused by reading the stack trace and reasoning about the C++ code
(as #1257 was). With this harness in place, the preferred approach for a
new report is: extend ``testapp/main.py`` to reproduce the reported failure
mode (or as close an approximation as practical, as the #1257 regression
check above does), confirm it actually fails the same way against the
*current* code first, then fix and re-run the same build/deploy/run/logcat
loop to confirm the fix.
