# Universal Launcher C SDK

Universal Launcher 1.9.1 installs product-neutral C ABI 1.9 headers, static and
shared libraries, public schemas, the reviewed ABI snapshot, and relocatable
CMake package metadata.

```cmake
find_package(UniversalLauncher 1.9.1 EXACT CONFIG REQUIRED)
target_link_libraries(my_consumer PRIVATE UniversalLauncher::CoreStatic)
```

Use `UniversalLauncher::CoreShared` for the shared library. Its imported target
propagates the Windows `ULK_USE_SHARED` declaration automatically. Consumers
that only inspect declarations can use `UniversalLauncher::Headers`.

The CMake package version, C ABI version, and contract maturity are separate:
the package is 1.9.1, the C ABI is 1.9, and the composition and session-record
contracts remain fixture-qualified. The caller-rooted session journal records
facts supplied by a consumer; it does not execute or monitor a process. This
SDK grants no process-execution, setup, network, signing, publication, or
product authority.

Every install generates
`share/universal-launcher/provider-package-manifest.v1.json` after all package
files are present. It binds the exact source commit/tree/ref, package and ABI
identity, journal disk formats, public headers, contracts, CMake targets,
toolchain/profile, licences, and SHA-256 for every installed artifact. Package
generation refuses a changed Git worktree or source identity mismatch. A
source archive without Git metadata must supply the exact source identity and
explicitly opt into the archive-source path at configure time.
