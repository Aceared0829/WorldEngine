
namespace WInternal
{
  constexpr WUInt32 MURMUR_M = 0x5bd1e995;
  constexpr WUInt32 MURMUR_R = 24;

  template <size_t N, size_t Loop>
  struct CompileTimeMurmurHash
  {
    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return CompileTimeMurmurHash<N, Loop - 4>()(CompileTimeMurmurHash<N, 4>()(uiHash, str, i), str, i + 4);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 4>
  {
    static constexpr W_ALWAYS_INLINE WUInt32 helper(WUInt32 k) { return (k ^ (k >> MURMUR_R)) * MURMUR_M; }

    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      // In C++11 constexpr local variables are not allowed. Need to express the following without "WUInt32 k"
      // (this restriction is lifted in C++14's generalized constexpr)
      // WUInt32 k = ((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24));
      // k *= MURMUR_M;
      // k ^= (k >> MURMUR_R);
      // k *= MURMUR_M;
      // return (hash * MURMUR_M) ^ k;

      return (uiHash * MURMUR_M) ^ helper(((str[i + 0]) | ((str[i + 1]) << 8) | ((str[i + 2]) << 16) | ((str[i + 3]) << 24)) * MURMUR_M);
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 3>
  {
    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 2] << 16) ^ (str[i + 1] << 8) ^ (str[i + 0])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 2>
  {
    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const
    {
      return (uiHash ^ (str[i + 1] << 8) ^ (str[i])) * MURMUR_M;
    }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 1>
  {
    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const { return (uiHash ^ (str[i])) * MURMUR_M; }
  };

  template <size_t N>
  struct CompileTimeMurmurHash<N, 0>
  {
    constexpr W_ALWAYS_INLINE WUInt32 operator()(WUInt32 uiHash, const char (&str)[N], size_t i) const { return uiHash; }
  };

  constexpr WUInt32 rightShift_and_xorWithPrevSelf(WUInt32 h, WUInt32 uiShift)
  {
    return h ^ (h >> uiShift);
  }
} // namespace WInternal

template <size_t N>
constexpr W_ALWAYS_INLINE WUInt32 WHashingUtils::MurmurHash32String(const char (&str)[N], WUInt32 uiSeed)
{
  // In C++11 constexpr local variables are not allowed. Need to express the following without "WUInt32 h"
  // (this restriction is lifted in C++14's generalized constexpr)
  // const WUInt32 uiStrlen = (WUInt32)(N - 1);
  // WUInt32 h = WInternal::CompileTimeMurmurHash<N - 1>(uiSeed ^ uiStrlen, str, 0);
  // h ^= h >> 13;
  // h *= WInternal::MURMUR_M;
  // h ^= h >> 15;
  // return h;

  return WInternal::rightShift_and_xorWithPrevSelf(
    WInternal::rightShift_and_xorWithPrevSelf(WInternal::CompileTimeMurmurHash<N, N - 1>()(uiSeed ^ static_cast<WUInt32>(N - 1), str, 0), 13) *
      WInternal::MURMUR_M,
    15);
}

W_ALWAYS_INLINE WUInt32 WHashingUtils::MurmurHash32String(WStringView sStr, WUInt32 uiSeed)
{
  return MurmurHash32(sStr.GetStartPointer(), sStr.GetElementCount(), uiSeed);
}
