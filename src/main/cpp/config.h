#include <cstddef>

// Fix for clang + libstdc++ + C++14.
// In this combination, gets is not defined in stdio.h, but libstdc++ is
// configured to think it is.
#if defined(__clang__)					\
 && defined(__GLIBCXX__) && __GLIBCXX__ <= 20140404	\
 && defined(__cplusplus) && __cplusplus >= 201305L
#  include <bits/c++config.h>
#  undef _GLIBCXX_HAVE_GETS
#endif
