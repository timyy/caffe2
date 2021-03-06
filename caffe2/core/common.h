#ifndef CAFFE2_CORE_COMMON_H_
#define CAFFE2_CORE_COMMON_H_

#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif

// Macros used during the build of this caffe2 instance. This header file
// is automatically generated by the cmake script during build.
#include "caffe2/core/macros.h"

// Caffe2 version. The plan is to increment the minor version every other week
// as a track point for bugs, until we find a proper versioning cycle.

#define CAFFE2_VERSION_MAJOR 0
#define CAFFE2_VERSION_MINOR 6
#define CAFFE2_VERSION_PATCH 0
#define CAFFE2_VERSION                                         \
  (CAFFE2_VERSION_MAJOR * 10000 + CAFFE2_VERSION_MINOR * 100 + \
   CAFFE2_VERSION_PATCH)

namespace caffe2 {

// Data type for caffe2 Index/Size. We use size_t to be safe here as well as for
// large matrices that are common in sparse math.
typedef int64_t TIndex;

// Note(Yangqing): NVCC does not play well with unordered_map on some platforms,
// forcing us to use std::map instead of unordered_map. This may affect speed
// in some cases, but in most of the computation code we do not access map very
// often, so it should be fine for us. I am putting a CaffeMap alias so we can
// change it more easily if things work out for unordered_map down the road.
template <typename Key, typename Value>
using CaffeMap = std::map<Key, Value>;
// using CaffeMap = std::unordered_map;

// Using statements for common classes that we refer to in caffe2 very often.
// Note that we only place it inside caffe2 so the global namespace is not
// polluted.
/* using override */
using std::set;
using std::string;
using std::unique_ptr;
using std::vector;

// Just in order to mark things as not implemented. Do not use in final code.
#define CAFFE_NOT_IMPLEMENTED CAFFE_THROW("Not Implemented.")

// suppress an unused variable.
#ifndef _MSC_VER
#define UNUSED_VARIABLE __attribute__((unused))
#else
#define UNUSED_VARIABLE 
#endif //_MSC_VER

// Disable the copy and assignment operator for a class. Note that this will
// disable the usage of the class in std containers.
#ifndef DISABLE_COPY_AND_ASSIGN
#define DISABLE_COPY_AND_ASSIGN(classname)                              \
private:                                                                       \
  classname(const classname&) = delete;                                        \
  classname& operator=(const classname&) = delete
#endif

// Define enabled when building for iOS or Android devices
#if !defined(CAFFE2_MOBILE)
#if defined(__ANDROID__)
#define CAFFE2_ANDROID 1
#define CAFFE2_MOBILE 1
#elif (defined(__APPLE__) &&                                            \
       (TARGET_IPHONE_SIMULATOR || TARGET_OS_SIMULATOR || TARGET_OS_IPHONE))
#define CAFFE2_IOS 1
#define CAFFE2_MOBILE 1
#elif (defined(__APPLE__) && TARGET_OS_MAC)
#define CAFFE2_IOS 1
#define CAFFE2_MOBILE 0
#else
#define CAFFE2_MOBILE 0
#endif // ANDROID / IOS / MACOS
#endif // CAFFE2_MOBILE

// Define alignment macro that is cross platform
#if defined(_MSC_VER)
#define CAFFE2_ALIGNED(x) __declspec(align(x))
#else
#define CAFFE2_ALIGNED(x) __attribute__((aligned(x)))
#endif

/**
 * Macro for marking functions as having public visibility.
 * Ported from folly/CPortability.h
 */
#ifndef __GNUC_PREREQ
#if defined __GNUC__ && defined __GNUC_MINOR__
#define __GNUC_PREREQ(maj, min) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#define __GNUC_PREREQ(maj, min) 0
#endif
#endif

#if defined(__GNUC__)
#if __GNUC_PREREQ(4, 9)
#define CAFFE2_EXPORT [[gnu::visibility("default")]]
#else
#define CAFFE2_EXPORT __attribute__((__visibility__("default")))
#endif
#else
#define CAFFE2_EXPORT
#endif

// make_unique is a C++14 feature. If we don't have 14, we will emulate
// its behavior. This is copied from folly/Memory.h
#if __cplusplus >= 201402L ||                                              \
    (defined __cpp_lib_make_unique && __cpp_lib_make_unique >= 201304L) || \
    (defined(_MSC_VER) && _MSC_VER >= 1900)
/* using override */ using std::make_unique;
#else

template<typename T, typename... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Allows 'make_unique<T[]>(10)'. (N3690 s20.9.1.4 p3-4)
template<typename T>
typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(const size_t n) {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

// Disallows 'make_unique<T[10]>()'. (N3690 s20.9.1.4 p5)
template<typename T, typename... Args>
typename std::enable_if<
  std::extent<T>::value != 0, std::unique_ptr<T>>::type
make_unique(Args&&...) = delete;

#endif

// to_string implementation for Android related stuff.
#ifndef __ANDROID__
using std::to_string;
#else
template <typename T>
std::string to_string(T value)
{
  std::ostringstream os;
  os << value;
  return os.str();
}
#endif

// dynamic cast reroute: if RTTI is disabled, go to reinterpret_cast
template <typename Dst, typename Src>
inline Dst dynamic_cast_if_rtti(Src ptr) {
#ifdef __GXX_RTTI
  return dynamic_cast<Dst>(ptr);
#else
  return reinterpret_cast<Dst>(ptr);
#endif
}

// SkipIndices are used in operator_fallback_gpu.h and operator_fallback_mkl.h
// as utilty functions that marks input / output indices to skip when we use a
// CPU operator as the fallback of GPU/MKL operator option.
template <int... values>
class SkipIndices {
 private:
  template <int V>
  static inline bool ContainsInternal(const int i) {
    return (i == V);
  }
  template <int First, int Second, int... Rest>
  static inline bool ContainsInternal(const int i) {
    return (i == First) && ContainsInternal<Second, Rest...>(i);
  }

 public:
  static inline bool Contains(const int i) {
    return ContainsInternal<values...>(i);
  }
};

template <>
class SkipIndices<> {
 public:
  static inline bool Contains(const int i) {
    return false;
  }
};

}  // namespace caffe2

#endif  // CAFFE2_CORE_COMMON_H_
