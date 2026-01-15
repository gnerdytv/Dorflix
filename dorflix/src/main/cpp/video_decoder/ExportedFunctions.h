#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Exported JVM access function
JavaVM* getGlobalJVM();

#ifdef __cplusplus
}
#endif