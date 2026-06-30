# JPype Development Quick Start

## Build
```bash
make -f project/dev.mk clean
make -f project/dev.mk all
```

## Test
```bash
cd native/jpype_module
mvn test                                      # All tests
mvn test -Dtest=PyTupleNGTest#testOfVarArgs  # Single test
```

## Common Issues

### Tests fail with "Can't load library"
```bash
# Cache points to wrong location
rm ~/.jpype/jpype.properties
```

### After moving project directory
```bash
# Update cache
rm ~/.jpype/jpype.properties
# OR manually edit
sed -i 's|/old/path|/new/path|g' ~/.jpype/jpype.properties
```

### Need debug output
```bash
mvn test -Dtest=TestName 2>&1 | tee /tmp/test.log
cat target/surefire-reports/*.dumpstream | grep '\[DEBUG\]\|\[PROBE\]'
```

## Documentation

- **BUILD.md** - Complete build guide
- **DEBUG.md** - Type conversion investigation  
- **PROGRESS_SUMMARY.md** - Overall status
- **FINDINGS_2026-06-30.md** - Mystery solved!

## Current Status (2026-06-30)

- ✅ Type conversion system working
- ✅ Probe finding correct mappings
- ✅ Dictionary coupling correct
- ❌ Tests blocked by Python 3.12 threading bug

**Bottom line**: Our code works, waiting on Python fix.
