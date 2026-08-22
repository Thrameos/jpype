"""python-for-android recipe for JPype's own `_jpype` C extension.

Modeled on the upstream pyjnius recipe (pythonforandroid/recipes/pyjnius),
which solves the same class of problem (a JNI-based Python<->Java bridge
built as a native extension). Unlike pyjnius, this recipe does not fetch a
release tarball: it builds directly from this repo's own working tree via
`IncludedFilesBehaviour`, since the whole point of this harness is to build
and test whatever is currently checked out, not a released version.

See project/android/README.md for how this recipe fits into the overall
build/deploy/verify loop, and for the current known rough edges.
"""
import glob
from os.path import join, dirname, isfile, realpath
import sh

from pythonforandroid.recipe import PyProjectRecipe, IncludedFilesBehaviour, Recipe
from pythonforandroid.logger import shprint, info, warning
from pythonforandroid.util import current_directory, ensure_dir, rmdir


# Repo root: project/android/recipes/jpype1/__init__.py -> up 4 levels.
_REPO_ROOT = join(dirname(__file__), '..', '..', '..', '..')


class JPype1Recipe(IncludedFilesBehaviour, PyProjectRecipe):
    name = 'jpype1'
    version = 'local'
    site_packages_name = 'jpype'
    depends = [('genericndkbuild', 'sdl2', 'sdl3')]

    # IncludedFilesBehaviour.prepare_build_dir() does a plain `cp -a` of
    # src_filename into the build dir. The jpype repo root also carries
    # .git/, any host-arch build/ directory (CLAUDE.md's build-isolation
    # note applies here too - a stale host build/ must never leak into an
    # Android cross-build), test artifacts, and this project/android/ tree
    # itself. None of that belongs in the Android build, so this recipe
    # overrides prepare_build_dir to rsync with excludes instead of relying
    # on the mixin's unconditional copy.
    src_filename = _REPO_ROOT

    def prepare_build_dir(self, arch):
        if self.src_filename is None:
            raise ValueError('src_filename must be set')
        build_dir = self.get_build_dir(arch)
        rmdir(build_dir)
        shprint(sh.rsync, '-a',
                '--exclude=.git',
                '--exclude=build',
                '--exclude=project/android/recipes',
                '--exclude=project/android/testapp',
                '--exclude=test/classes',
                '--exclude=__pycache__',
                self.src_filename + '/', build_dir)

    def get_recipe_env(self, arch, **kwargs):
        env = super().get_recipe_env(arch, **kwargs)

        # jpype's own build (native/CMakeLists.txt) needs to see ANDROID
        # so it takes the libc++_shared linking branch instead of the
        # generic Linux/glibc one. p4a's NDK toolchain file already sets
        # CMAKE_SYSTEM_NAME=Android; this recipe passes the toolchain file
        # itself and the per-arch ABI/platform through scikit-build-core's
        # generic `cmake.define.<VAR>=<value>` config-settings mechanism
        # (see CLAUDE.md - the same mechanism used for BUILD_TEST_HARNESS
        # in the normal host build), so no pyproject.toml changes are
        # needed on jpype's side for the toolchain plumbing itself.
        toolchain_file = join(self.ctx.ndk_dir, 'build', 'cmake',
                               'android.toolchain.cmake')

        # CMake's FindPython3 (native/CMakeLists.txt's
        # `find_package(Python3 COMPONENTS Interpreter Development.Module
        # REQUIRED)`) can't introspect the target Android Python the way it
        # does a normal host build - there is no sysconfig to query and the
        # target's own python binary can't run on the host CPU anyway. Point
        # it at what p4a already built for exactly this purpose: a
        # host-runnable "hostpython" (self.real_hostpython_location, the
        # same one build_arch() below invokes `python -m build` with) for
        # the Interpreter component, and the target Android sysroot's real
        # headers/libpython for Development.Module.
        python_recipe = Recipe.get_recipe('python3', self.ctx)
        python_include_dir = python_recipe.include_root(arch.arch)
        python_library = join(python_recipe.link_root(arch.arch),
                               python_recipe._libpython)

        # Android_JNI_GetEnv() (project/android/native/android_jnienv.c)
        # calls WebView_AndroidGetJNIEnv(), exported by the webview
        # bootstrap's libmain.so - confirmed present and GLOBAL DEFAULT in
        # its dynamic symbol table. Leaving it as a plain unresolved
        # import (the --unresolved-symbols=ignore-all path in
        # native/CMakeLists.txt) isn't enough on-device: Android's linker
        # namespace isolation blocks a same-process-but-different-namespace
        # symbol lookup for a merely-undefined reference (_jpype.so, loaded
        # via Python's import machinery, ends up in a different linker
        # namespace than libmain.so, loaded by the app's native launcher).
        # An explicit DT_NEEDED dependency on libmain.so, however, *does*
        # get resolved across that boundary - this mirrors exactly what
        # the upstream pyjnius recipe's get_recipe_env() does with its own
        # -L.../libmain*.so linking.
        # Passed as a cmake.define (native/CMakeLists.txt's
        # JPYPE_ANDROID_LIBMAIN_DIR), not via the LDFLAGS environment
        # variable: scikit-build-core does not read ambient LDFLAGS the
        # way legacy setuptools/distutils builds did, so an env-var-only
        # attempt at this silently has no effect on the actual link line.
        libmain_dir = join(self.ctx.bootstrap.build_dir, 'libs', arch.arch)
        self.extra_build_args = self.extra_build_args + [
            '--config-setting=cmake.define.CMAKE_TOOLCHAIN_FILE=' + toolchain_file,
            '--config-setting=cmake.define.ANDROID_ABI=' + arch.arch,
            '--config-setting=cmake.define.ANDROID_PLATFORM=android-' + str(self.ctx.ndk_api),
            '--config-setting=cmake.define.ENABLE_BUILD_JAR=OFF',
            '--config-setting=cmake.define.Python3_EXECUTABLE=' + self.real_hostpython_location,
            '--config-setting=cmake.define.Python3_INCLUDE_DIR=' + python_include_dir,
            '--config-setting=cmake.define.Python3_LIBRARY=' + python_library,
            '--config-setting=cmake.define.JPYPE_ANDROID_LIBMAIN_DIR=' + libmain_dir,
        ]
        return env

    def check_prebuilt(self, arch, msg=""):
        # PyProjectRecipe.lookup_prebuilt() builds a pip requirement string
        # "name==version" to dry-run-check for a prebuilt wheel; version =
        # 'local' isn't a valid PEP440 specifier, so that dry-run raises
        # (harmlessly - p4a's own thread wrapper swallows it and falls
        # through to a real build) but it's noisy. This recipe always
        # builds from this repo's own working tree, so skip the prebuilt
        # lookup outright instead.
        return False

    def build_arch(self, arch):
        # PyProjectRecipe.build_arch() (pythonforandroid/recipe.py) always
        # invokes `python -m build --wheel --config-setting builddir=...`,
        # hardcoding a config-setting name that's specific to meson-python
        # (see the sibling MesonRecipe class, which shares this same base
        # build_arch). scikit-build-core - what jpype's own pyproject.toml
        # actually uses - calls the equivalent option `build-dir`, and
        # rejects an unrecognized `builddir` outright rather than ignoring
        # it. There's no override hook for just this one flag, so this is
        # a full copy of the upstream method with that one line corrected;
        # keep it in sync with upstream's build_arch if it changes.
        if self.check_prebuilt(arch, "skipping build_arch"):
            result = self.install_prebuilt_wheel(arch)
            if result:
                return
            warning("Failed to install prebuilt wheel, falling back to build_arch")

        build_dir = self.get_build_dir(arch.arch)
        if not (isfile(join(build_dir, "pyproject.toml")) or isfile(join(build_dir, "setup.py"))):
            warning("Skipping build because it does not appear to be a Python project.")
            return
        self.install_hostpython_prerequisites(
            packages=["build[virtualenv]", "pip", "setuptools", "patchelf"] + self.hostpython_prerequisites
        )

        env = self.get_recipe_env(arch, with_flags_in_cc=True)
        sub_build_dir = join(build_dir, "p4a_android_build")
        ensure_dir(sub_build_dir)

        build_args = [
            "-m",
            "build",
            "--wheel",
            "--config-setting",
            "build-dir={}".format(sub_build_dir),
        ] + self.extra_build_args

        built_wheels = []
        with current_directory(build_dir):
            shprint(
                sh.Command(self.real_hostpython_location), *build_args, _env=env
            )
            built_wheels = [realpath(whl) for whl in glob.glob("dist/*.whl")]
        self.install_wheel(arch, built_wheels)

    # p4a arch name -> NDK sysroot triple, for locating libc++_shared.so.
    _NDK_TRIPLE = {
        'arm64-v8a': 'aarch64-linux-android',
        'armeabi-v7a': 'arm-linux-androideabi',
        'x86': 'i686-linux-android',
        'x86_64': 'x86_64-linux-android',
    }

    def postbuild_arch(self, arch):
        super().postbuild_arch(arch)

        # _jpype.so is linked against libc++_shared.so (native/CMakeLists.txt's
        # ANDROID branch), but that's an NDK runtime library, not something
        # Android's system image provides - it has to be bundled into the
        # APK's own native library dir explicitly, or dlopen(_jpype.so)
        # fails at runtime with "library libc++_shared.so not found" (it
        # doesn't matter that _jpype.so itself built and installed fine).
        libcxx = join(self.ctx.ndk_dir, 'toolchains', 'llvm', 'prebuilt',
                      'linux-x86_64', 'sysroot', 'usr', 'lib',
                      self._NDK_TRIPLE[arch.arch], 'libc++_shared.so')
        self.install_libs(arch, libcxx)

        # See jp_classloader.cpp: FindClass("org/jpype/JPypeClassLoader")
        # must resolve on-device. That class comes from the same
        # native/jpype_module Java sources the host build packages into
        # org.jpype.jar via Ant - on Android they need to reach the APK's
        # classes.dex instead, which p4a/buildozer handles automatically
        # for anything placed under javaclass_dir before the Java/dex
        # build step runs (the same mechanism pyjnius's postbuild_arch
        # uses for its own org/ sources).
        info('Copying org.jpype Java sources to classes build dir')
        with current_directory(self.get_build_dir(arch.arch)):
            shprint(sh.cp, '-a',
                    join('native', 'jpype_module', 'src', 'main', 'java', 'org'),
                    self.ctx.javaclass_dir)

            # Reflector0.java lives outside that org/ tree on purpose -
            # native/build.xml excludes it from the normal javac pass and
            # compiles it separately (into META-INF/versions/0/, see that
            # file), so its source sits at .../java/exclude/org/jpype/
            # rather than .../java/org/jpype/ and the copy above misses it.
            # JPypeContext.createContext() does
            # `Class.forName("org.jpype.Reflector0", ...)` unconditionally
            # (not Android-specific) to get a dedicated stack frame for
            # invoking caller-sensitive Java methods correctly - without
            # this file compiled in, that lookup fails with "Unable to
            # create reflector", which is a missing-source bug in this
            # recipe, not an Android/ART bytecode-generation limitation.
            shprint(sh.cp,
                    join('native', 'jpype_module', 'src', 'main', 'java',
                         'exclude', 'org', 'jpype', 'Reflector0.java'),
                    join(self.ctx.javaclass_dir, 'org', 'jpype', 'Reflector0.java'))


recipe = JPype1Recipe()
