[app]
title = JPype Android Test
package.name = jpypetest
package.domain = org.jpype.test
source.dir = .
source.include_exts = py
version = 0.1

# jpype1 is resolved via p4a.local_recipes below, against
# project/android/recipes/jpype1, which builds from this repo's own
# working tree (see that recipe's docstring).
requirements = python3,jpype1

# service_only is a headless, no-UI bootstrap - unlike webview (used
# initially), it doesn't instantiate a real Chromium/WebView GPU surface,
# which was the source of an unrelated crash (a Chromium GPU-thread EGL
# teardown crash in the Android emulator's own EGL software layer, nothing
# to do with jpype - see doc/android_build.rst) that only ever happened
# because webview was pulling in a whole browser engine this test app
# never needed. It still exports the WebView_AndroidGetJNIEnv() hook
# project/android/native/android_jnienv.c needs (confirmed: present in
# service_only's own bootstraps/service_only/build/jni/.../pyjniusjni.c).
p4a.bootstrap = service_only

p4a.local_recipes = ../recipes

# service_only's own launcher (unlike jpype1 itself) builds via the legacy
# ndk-build/Android.mk path (the genericndkbuild recipe it depends on), not
# CMake - this specific NDK version caps that path's supported platform at
# 33 ("android-34 is above the maximum supported version android-33").
# webview's own bootstrap build didn't hit this, only service_only's does.
android.api = 33
android.minapi = 24
android.ndk = 25.1.8937393
android.archs = x86_64

# Point at the SDK/NDK already installed locally (see doc/android_build.rst
# setup step) instead of buildozer's default behavior of downloading its
# own separate copies into ~/.buildozer/android/platform/ - besides being
# wasteful, that default download path has also been observed pulling in a
# stale commandlinetools URL that 404s.
#
# ADJUST THESE TWO PATHS to wherever you installed the SDK/NDK locally -
# these are absolute paths, not something buildozer.spec can make portable.
android.sdk_path = /home/kenelson/android-sdk
android.ndk_path = /home/kenelson/android-sdk/ndk/25.1.8937393

log_level = 2

[buildozer]
warn_on_root = 1
