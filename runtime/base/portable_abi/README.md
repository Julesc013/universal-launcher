# Portable ABI Internals

Portable helpers, ABI-compatible state records, small builders, and test-only
helpers may live here. C++98-style internals are permitted when isolated, but
the folder is named for the ownership boundary rather than the language level.
No C++ type crosses the public ABI.
