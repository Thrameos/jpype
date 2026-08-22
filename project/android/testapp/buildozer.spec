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

# genericndkbuild is the simplest p4a bootstrap that still exposes a
# JNIEnv getter recipes can hook into (WebView_AndroidGetJNIEnv, see
# project/android/native/android_jnienv.c) without pulling in SDL2/Kivy's
# full UI stack, which this headless test app doesn't need.
p4a.bootstrap = webview

p4a.local_recipes = ../recipes

android.api = 34
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
