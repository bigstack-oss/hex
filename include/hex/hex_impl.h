// HEX SDK

#ifndef HEX_IMPL_H
#define HEX_IMPL_H

// The standard C assert macro is disabled when the macro NDEBUG is defined, meaning "Not debug".
// This leads to really awful double negative cases like #ifndef NDEBUG //DebuggingCode #endif
// Refine for better readable code
#ifdef NDEBUG
#define HEX_PROD    // production
#else
#define HEX_DEV     // development
#endif

// String concatenation with macro expansion
// Need 2 levels of indirection to get this to work
#define HEX_XCAT(A, B) A ## B
#define HEX_CAT(A, B)  HEX_XCAT(A,B)

// Stringification
// Need 2 levels of indirection to get this to work
#define HEX_XSTR(A) # A
#define HEX_STR(A)  HEX_XSTR(A)

#endif /* endif HEX_IMPL_H */
