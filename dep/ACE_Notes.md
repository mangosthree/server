The default files for ACE_LITE were amended in this commit: https://github.com/mangos/mangosDeps/commit/52e4c984dbca474f7a4353b444339067a95ea277

The same changes need to be reapplied if a new version is added.

The following two lines were amended to fix a connection bottleneck (especially on Linux):
- ACE_DEFAULT_BACKLOG - Change 5 to 128 https://github.com/mangos/mangosDeps/blob/Rel21/acelite/ace/Default_Constants.h#L81
- ACE_DEFAULT_ASYNCH_BACKLOG - Change 5 to 128 https://github.com/mangos/mangosDeps/blob/Rel21/acelite/ace/Default_Constants.h#L85

For MacOS X, the ACE_UINT64_TYPE should be defined as follows in `ace/config-macosx-leopard.h`
```
#define ACE_UINT64_TYPE unsigned long long
```

To get Travis CI working with `macos10.12` and `xcode8.3`, the following changes should be made to `ace/config-macosx-yosemite.h`
```diff
 #include "ace/config-macosx-mavericks.h"
+#undef ACE_LACKS_CLOCKID_T
+#undef ACE_LACKS_CLOCK_MONOTONIC
+#undef ACE_LACKS_CLOCK_REALTIME
```

March 7th, 2018 - To build ACE as static library, one has to add the following line to the global CMakeLists.txt
```
add_definitions(-DACE_AS_STATIC_LIBS)
```
