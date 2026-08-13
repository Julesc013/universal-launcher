# Universal Launcher C SDK

Universal Launcher 1.9.0 installs product-neutral C ABI 1.9 headers, static and
shared libraries, public schemas, the reviewed ABI snapshot, and relocatable
CMake package metadata.

```cmake
find_package(UniversalLauncher 1.9.0 EXACT CONFIG REQUIRED)
target_link_libraries(my_consumer PRIVATE UniversalLauncher::CoreStatic)
```

Use `UniversalLauncher::CoreShared` for the shared library. Its imported target
propagates the Windows `ULK_USE_SHARED` declaration automatically. Consumers
that only inspect declarations can use `UniversalLauncher::Headers`.

The CMake package version, C ABI version, and contract maturity are separate:
the package is 1.9.0, the C ABI is 1.9, and the composition and session-record
contracts remain fixture-qualified. The caller-rooted session journal records
facts supplied by a consumer; it does not execute or monitor a process. This
SDK grants no process-execution, setup, network, signing, publication, or
product authority.
