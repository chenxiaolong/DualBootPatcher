#ifndef LIBDBP_GLOBAL_H
#define LIBDBP_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBDBP_LIBRARY)
#  define LIBDBPSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBDBPSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBDBP_GLOBAL_H
