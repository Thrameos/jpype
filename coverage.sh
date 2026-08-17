#!/bin/bash
# Canonical local coverage entrypoint. This is the one script that should
# ever be updated/extended for coverage work -- do not reconstruct this
# logic ad hoc from plan/*.md notes (those are transient, gitignored, and
# not guaranteed current). See CLAUDE.md.
#
# Combines coverage from TWO separate test suites that exercise opposite
# directions of the bridge and are NOT run by the same JVM:
#   1. test/jpypetest (pytest, Python calling into Java) -- always run.
#   2. native/jpype_module's Maven/TestNG suite (Java hosting Python,
#      "reverse embedding", 938 tests as of 2026-08-17) -- attempted;
#      failure here is reported but not fatal to this script, since it's
#      a separate/newer harness than suite 1 and shouldn't block getting
#      suite 1's numbers. If it fails, don't add `-DforkCount=0` or other
#      single-test-class isolation flags to debug it in place -- that
#      changes the suite's execution model (runs the embedded-Python
#      bootstrap inside the already-running Maven JVM instead of a clean
#      fork) and produces misleading crashes unrelated to the real
#      suite (see plan/archive/ReverseEmbeddingBootstrapSegfault.md).
#
# The two suites' JaCoCo output can't be `jacoco:merge`'d directly: ant
# (suite 1's org.jpype.jar) and Maven (suite 2's target/classes) don't
# produce CRC-identical classfiles for the same source, so JaCoCo can't
# correlate one suite's exec data against the other's classes. Instead
# plan/tools/merge_jacoco_reports.py merges at the method level (a method
# counts as covered if EITHER suite covered it) -- see that script's
# docstring for the full rationale.
#
# Usage: ./coverage.sh [venv_dir]
#   venv_dir defaults to /tmp/jpype-coverage-venv (disposable, per
#   CLAUDE.md -- never run this against a real/persistent environment).

set -e

VENV="${1:-/tmp/jpype-coverage-venv}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$REPO_ROOT"

echo "=== 1. Disposable venv + editable build ($VENV) ==="
python3.12 -m venv "$VENV"
"$VENV/bin/pip" install --upgrade pip -q
"$VENV/bin/pip" install -q scikit-build-core pybind11 pytest pytest-randomly pytest-cov numpy build
"$VENV/bin/pip" install --no-build-isolation -e . --config-settings=cmake.define.BUILD_TEST_HARNESS=ON

echo "=== 2. pytest suite (Python -> Java), with --jacoco ==="
mkdir -p build/coverage
"$VENV/bin/python" -m pytest -q test/jpypetest \
  --cov=jpype --cov-report=xml:build/coverage/coverage_py.xml --cov-report=term \
  --classpath="native/jpype_module/target/classes:test/classes" \
  --jacoco --checkjni

echo "=== 3. Java report for suite 1 (against the real org.jpype.jar) ==="
rm -rf build/coverage/jar_extract
mkdir -p build/coverage/jar_extract
( cd build/coverage/jar_extract && jar xf "$REPO_ROOT/org.jpype.jar" )
java -jar lib/org.jacoco.cli-0.8.5-nodeps.jar report build/coverage/jacoco.exec \
  --classfiles build/coverage/jar_extract \
  --xml build/coverage/coverage_java_pytest.xml --html build/coverage/java_pytest \
  --sourcefiles native/jpype_module/src/main/java

echo "=== 4. Maven/TestNG suite (Java -> Python, reverse embedding) ==="
MAVEN_OK=0
(
  cd native/jpype_module
  # ABI-tagged copies at repo root, same convention project/dev.mk uses --
  # org.jpype.Launcher's dev-tree detection needs _jpype/_jpyne co-located
  # and findable from a plain `import _jpype` (see
  # plan/archive/NativeDevTreeLayout.md for how this was found).
  EXT_SUFFIX=$("$VENV/bin/python" -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")
  PYTAG=$("$VENV/bin/python" -c "import sys; print('cp%d%d' % sys.version_info[:2])")
  cp "$REPO_ROOT"/build/$PYTAG-*/_jpype.so "$REPO_ROOT/_jpype$EXT_SUFFIX"
  cp "$REPO_ROOT"/build/$PYTAG-*/_jpyne.so "$REPO_ROOT/_jpyne$EXT_SUFFIX"
  PYTHONPATH="$REPO_ROOT" mvn -o test -Djpype.nocache=true
) && MAVEN_OK=1 || echo "    Maven suite failed. Skipping its coverage -- see output above."

if [ "$MAVEN_OK" = "1" ]; then
  echo "=== 5. Java report for suite 2, merged into suite 1's ==="
  mvn -o -f native/jpype_module/pom.xml jacoco:report
  python3 plan/tools/merge_jacoco_reports.py \
    build/coverage/coverage_java_pytest.xml \
    native/jpype_module/target/site/jacoco/jacoco.xml \
    --out build/coverage/coverage_java_merged.tsv
  echo "    Merged Java coverage: build/coverage/coverage_java_merged.tsv"
else
  echo "=== 5. Skipped (suite 2 did not run) -- Java coverage below is suite 1 (pytest) ONLY ==="
  echo "    and understates real coverage for anything the reverse-embedding suite exercises"
  echo "    (org.jpype.script.*, FunctionalAdapters, GlobalPool, ReferenceSet, MainInterpreter,"
  echo "    Script, SubInterpreter*, Runner, Launcher -- see native/jpype_module/src/test/java)."
fi

echo "=== Done ==="
echo "Python coverage:      build/coverage/coverage_py.xml"
echo "Java coverage (pytest-only): build/coverage/coverage_java_pytest.xml / build/coverage/java_pytest/"
if [ "$MAVEN_OK" = "1" ]; then
  echo "Java coverage (merged, both suites): build/coverage/coverage_java_merged.tsv"
fi
