# JPype Build Guide

## Quick Reference

```bash
# Clean build everything
make -f project/dev.mk clean
make -f project/dev.mk all

# Run tests
cd native/jpype_module && mvn test
cd native/jpype_module && mvn test -Dtest=PyTupleNGTest#testOfVarArgs  # Single test
```

## Build System

JPype uses a hybrid build system:
- **Python/C++ extension**: CMake via scikit-build-core (automated by dev.mk)
- **Java classes**: Ant (automated by dev.mk)  
- **Maven**: Only for running tests

The master build script is `project/dev.mk` - use it for all builds.

## Initial Setup

### Prerequisites

```bash
# Install build dependencies
python3 -m pip install scikit-build-core --break-system-packages
```

### First Build

```bash
# Build everything (Java JAR + Python extension)
make -f project/dev.mk all
```

This will:
1. Download Ivy for dependency management
2. Resolve Java dependencies
3. Build Java JAR with Ant
4. Build Python extension with CMake/scikit-build-core
5. Install in editable mode

## Build Outputs

After a successful build:
- `_jpype.so` - Main Python extension (8.4 MB, full JPype)
- `_jpyne.so` - Bootstrap loader (69 KB, minimal JNI bootstrap)
- `org.jpype.jar` - Java classes

## Clean Builds

**Always clean when**:
- Switching branches
- After major changes
- Build behaving strangely
- Moving project directory

```bash
make -f project/dev.mk clean
make -f project/dev.mk all
```

## Cache Management

### The Cache Problem

JPype caches Python interpreter discovery results in `~/.jpype/jpype.properties`. If you move the project directory, the cache will point to the old location and tests will fail with:

```
Can't load library: /old/path/_jpype.so
```

### Cache Fix Options

**Option 1: Delete cache (recommended)**
```bash
rm ~/.jpype/jpype.properties
```

**Option 2: Edit cache manually**
```bash
sed -i 's|/old/path|/new/path|g' ~/.jpype/jpype.properties
```

**Option 3: Check cache contents**
```bash
cat ~/.jpype/jpype.properties
# Look for jpype.lib= line - should point to current _jpype.so
```

### Should We Add Validation?

**Question**: Should `MainInterpreter.java` check if cached paths exist and clear invalid entries?

**Pros**:
- Auto-fixes stale cache after project moves
- Better user experience
- Prevents confusing errors

**Cons**:  
- Adds file I/O on every startup
- Could hide real issues (missing builds)
- Java code would need to handle filesystem checks

**Current workaround**: Manual cache deletion is simple and works. Adding validation could be a future enhancement if this becomes a frequent issue.

## Testing

### Python Tests

```bash
make -f project/dev.mk test-python
```

### Java/Maven Tests

```bash
# All tests (slow)
cd native/jpype_module && mvn test

# Single test
cd native/jpype_module && mvn test -Dtest=ClassName#methodName

# Example
cd native/jpype_module && mvn test -Dtest=PyTupleNGTest#testOfVarArgs
```

### Debug Output

Maven test output goes to multiple places:
- Console: Basic pass/fail info
- `target/surefire-reports/*.txt`: Test results
- `target/surefire-reports/*.dumpstream`: Native stdout/stderr (debug logs)

To see C++ debug output:
```bash
mvn test -Dtest=TestName 2>&1 | tee /tmp/test.log
cat native/jpype_module/target/surefire-reports/*dumpstream | grep '\[DEBUG\]\|\[PROBE\]\|\[INIT\]'
```

## Development Workflow

### After Modifying C++ Code

```bash
make -f project/dev.mk clean  # Clean old builds
make -f project/dev.mk all    # Rebuild
cd native/jpype_module && mvn test -Dtest=YourTest  # Test
```

### After Modifying Python Code

```bash
# Python is in editable mode, no rebuild needed
cd native/jpype_module && mvn test -Dtest=YourTest
```

### After Modifying Java Code

```bash
cd native && ant          # Rebuild JAR
cp native/build/lib/org.jpype.jar .
cd native/jpype_module && mvn test
```

## Common Issues

### Issue: "Can't load library" error

**Cause**: Cache points to old project location
**Fix**: Delete `~/.jpype/jpype.properties` and rerun tests

### Issue: Maven tests crash with SIGSEGV

**Cause**: Python 3.12 subinterpreter threading bug (known issue)
**Workaround**: Tests are currently blocked by this Python bug
**Status**: Type conversion code is working, issue is in Python C-API

### Issue: "Module not found" errors

**Cause**: Extension not built or installed
**Fix**: 
```bash
make -f project/dev.mk clean
make -f project/dev.mk all
```

### Issue: Build fails with "scikit_build_core not found"

**Cause**: Missing build dependency
**Fix**:
```bash
python3 -m pip install scikit-build-core --break-system-packages
```

### Issue: Old .so files interfering

**Cause**: Multiple copies of _jpype.so/_jpyne.so
**Fix**:
```bash
find . -name "_jp*.so" -delete
make -f project/dev.mk all
```

## Project Structure

```
jpype3/
├── jpype/               # Python package
│   ├── _core.py        # Core initialization
│   └── _jbridge.py     # Bridge initialization, Backend
├── native/
│   ├── common/         # C++ core (JPype internal)
│   ├── python/         # Python-specific C++ (pyjp_*)
│   ├── bootstrap/      # JNI bootstrap loader
│   ├── build.xml       # Ant build for Java
│   └── jpype_module/   # Java classes + Maven tests
├── project/
│   └── dev.mk          # Master build script
├── _jpype.so           # Built Python extension
├── _jpyne.so           # Built bootstrap loader
└── org.jpype.jar       # Built Java classes
```

## Build System Details

### scikit-build-core Configuration

Configuration in `pyproject.toml`:
- `cmake.define.BUILD_TEST_HARNESS=ON` - Build bootstrap loader
- `cmake.define.CMAKE_BUILD_TYPE=RelWithDebInfo` - Debug symbols + optimization
- `editable.mode=inplace` - Build directly in source tree

### Why Two .so Files?

- `_jpype.so` - Main extension, depends on Java classes being loaded
- `_jpyne.so` - Bootstrap loader, minimal JNI to load Java classes first

This two-stage bootstrap allows Java to call Python before Python imports Java.

## Advanced: Manual CMake Build

If you need to debug the C++ build:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TEST_HARNESS=ON
make -j4
cp _jpype.so ..
```

But normally, just use `make -f project/dev.mk all`.
