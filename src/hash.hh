#pragma once

#include <compare>

#ifdef HASH_FUNCTION_MEOWHASH
  #include <meow_hash_x64_aesni.h>
  class HashType {
  private:
    __m128i val;

  public:
    HashType() {
      val = _mm_setzero_si128();
    }
    HashType(__m128i val) : val(val) {}

    std::strong_ordering operator<=>(const HashType& other) const {
      // Compare each 32-bit integer in the 128-bit register
      int32_t a3 = _mm_extract_epi32(val, 3);
      int32_t b3 = _mm_extract_epi32(other.val, 3);
      if (a3 < b3) return std::strong_ordering::less;
      if (a3 > b3) return std::strong_ordering::greater;
      int32_t a2 = _mm_extract_epi32(val, 2);
      int32_t b2 = _mm_extract_epi32(other.val, 2);
      if (a2 < b2) return std::strong_ordering::less;
      if (a2 > b2) return std::strong_ordering::greater;
      int32_t a1 = _mm_extract_epi32(val, 1);
      int32_t b1 = _mm_extract_epi32(other.val, 1);
      if (a1 < b1) return std::strong_ordering::less;
      if (a1 > b1) return std::strong_ordering::greater;
      int32_t a0 = _mm_extract_epi32(val, 0);
      int32_t b0 = _mm_extract_epi32(other.val, 0);
      if (a0 < b0) return std::strong_ordering::less;
      if (a0 > b0) return std::strong_ordering::greater;
      return std::strong_ordering::equal;
    }

    bool operator==(const HashType& other) const {
        return _mm_movemask_epi8(_mm_cmpeq_epi8(val, other.val)) == 0xFFFF;
    }
  };

  #define HASH_T HashType
  #define HASH_FN(key, len) MeowHash(MeowDefaultSeed, len, key)
#elifdef HASH_FUNCTION_RAPIDHASH
  #include <rapidhash.h>
  #define HASH_T uint64_t
  #define HASH_FN(key, len) rapidhash(key, len)
#elifdef HASH_FUNCTION_T1HA0
  #include <t1ha.h>
  #define HASH_T uint64_t
  #define HASH_FN(key, len) t1ha0(key, len, 0)
#elifdef HASH_FUNCTION_XXH3
  #include <xxhash.h>
  #define HASH_T uint64_t
  #define HASH_FN(key, len) XXH3_64bits(key, len)
#else
  #error "invalid hash function"
#endif

