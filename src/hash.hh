#pragma once

#include <cstdint>
#include <compare>
template <typename Traits>
class HashType128 {
private:
    using HashValue = typename Traits::HashType;
    HashValue val;
public:
    HashType128() : val() {}
    HashType128(HashValue val) : val(val) {}
    std::strong_ordering operator<=>(const HashType128& other) const {
        uint64_t a1 = Traits::high64(val);
        uint64_t b1 = Traits::high64(other.val);
        if (a1 < b1) return std::strong_ordering::less;
        if (a1 > b1) return std::strong_ordering::greater;
        uint64_t a0 = Traits::low64(val);
        uint64_t b0 = Traits::low64(other.val);
        if (a0 < b0) return std::strong_ordering::less;
        if (a0 > b0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }
    bool operator==(const HashType128& other) const {
        return Traits::isEqual(val, other.val);
    }
};

#if defined(HASH_FUNCTION_CityHash32)     || \
    defined(HASH_FUNCTION_CityHash64)     || \
    defined(HASH_FUNCTION_CityHash128)    || \
    defined(HASH_FUNCTION_CityHashCrc128)
  #include <city.h>
  #include <citycrc.h>
  #define HASH_FN(key, len) HASH_FUNCTION((const char *) key, len)
  #if defined(HASH_FUNCTION_CityHash32)
    #define HASH_T uint32
  #elif defined(HASH_FUNCTION_CityHash64)
    #define HASH_T uint64
  #elif defined(HASH_FUNCTION_CityHash128) || \
        defined(HASH_FUNCTION_CityHashCrc128)
    #define HASH_T uint128
  #endif

#elif defined(HASH_FUNCTION_FarmHash32) || \
    defined(HASH_FUNCTION_FarmHash64)   || \
    defined(HASH_FUNCTION_FarmHash128)
  #include <farmhash.h>
  #if defined(HASH_FUNCTION_FarmHash32)
    #define HASH_T uint32_t
    #define HASH_FN(key, len) util::Hash32((const char *) key, len)
  #elif defined(HASH_FUNCTION_FarmHash64)
    #define HASH_T uint64_t
    #define HASH_FN(key, len) util::Hash64((const char *) key, len)
  #elif defined(HASH_FUNCTION_FarmHash128)
    #define HASH_T util::uint128_t
    #define HASH_FN(key, len) util::Hash128((const char *) key, len)
  #endif

#elif defined(HASH_FUNCTION_MeowHash)
  #include <meow_hash_x64_aesni.h>
  struct MeowHashTraits {
    using HashType = meow_u128;
    static uint64_t high64(const HashType& val) { return MeowU64From(val, 1); }
    static uint64_t low64(const HashType& val) { return MeowU64From(val, 0); }
    static bool isEqual(const HashType& a, const HashType& b) {
      return MeowHashesAreEqual(a, b);
    }
  };
  #define HASH_T HashType128<MeowHashTraits>
  #define HASH_FN(key, len) HASH_FUNCTION(MeowDefaultSeed, len, key)

#elif defined(HASH_FUNCTION_rapidhash)
  #include <rapidhash.h>
  #define HASH_T uint64_t
  #define HASH_FN(key, len) HASH_FUNCTION(key, len)

#elif defined(HASH_FUNCTION_t1ha0_ia32aes_avx)   || \
      defined(HASH_FUNCTION_t1ha0_ia32aes_avx2)  || \
      defined(HASH_FUNCTION_t1ha0_ia32aes_noavx) || \
      defined(HASH_FUNCTION_t1ha0_32le)          || \
      defined(HASH_FUNCTION_t1ha0_32be)          || \
      defined(HASH_FUNCTION_t1ha1_le)            || \
      defined(HASH_FUNCTION_t1ha1_be)            || \
      defined(HASH_FUNCTION_t1ha2_atonce)
  #include <t1ha.h>
  #define HASH_FN(key, len) HASH_FUNCTION(key, len, 0)
  #if defined(HASH_FUNCTION_t1ha0_32le) || \
      defined(HASH_FUNCTION_t1ha0_32le)
    #define HASH_T uint32_t
  #else
    #define HASH_T uint64_t
  #endif

#elif defined(HASH_FUNCTION_XXH32)        || \
      defined(HASH_FUNCTION_XXH64)        || \
      defined(HASH_FUNCTION_XXH3_64bits)  || \
      defined(HASH_FUNCTION_XXH3_128bits)
  #include <xxhash.h>
  #if defined(HASH_FUNCTION_XXH3_128bits)
    struct XXHash128traits {
      using HashType = XXH128_hash_t;
      static uint64_t high64(const HashType& val) { return val.high64; }
      static uint64_t low64(const HashType& val) { return val.low64; }
      static bool isEqual(const HashType& a, const HashType& b) {
        return XXH128_isEqual(a, b);
      }
    };
    #define HASH_T HashType128<XXHash128traits>
  #elif defined(HASH_FUNCTION_XXH32)
    #define HASH_T uint32_t
  #else
    #define HASH_T uint64_t
  #endif

  #if defined(HASH_FUNCTION_XXH32) || \
      defined(HASH_FUNCTION_XXH64)
    #define HASH_FN(key, len) HASH_FUNCTION(key, len, 0)
  #else
    #define HASH_FN(key, len) HASH_FUNCTION(key, len)
  #endif

#else
  #error "invalid hash function"
#endif
