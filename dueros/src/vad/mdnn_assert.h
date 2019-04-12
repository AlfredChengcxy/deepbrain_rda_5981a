#ifndef __MDNN_ASSERT_H__
#define __MDNN_ASSERT_H__

#ifndef __UVISION_VERSION
    #include <assert.h>
    #define MDNN_ASSERT(x) assert(x)
#else
    #define MDNN_ASSERT(x)
#endif

#endif
