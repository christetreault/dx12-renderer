#pragma once

#ifndef expect
#define expectTrue(msg, e) \
{ \
   assert((e) && "Truth Assertion Failed:" msg); \
}
#else
#error expectTrue already defined!
#endif

#ifndef expect
#define expectRes(msg, e) \
{ \
   HRESULT res = (e); \
   assert(SUCCEEDED(res) && "Result Assertion Failed:" msg); \
}
#else
#error expectRes already defined!
#endif

namespace dmp
{
   static auto doNothing = []() {};

   static const UINT SWAP_CHAIN_BUFFER_COUNT = 2;

   static inline size_t calcConstantBufferByteSize(size_t unaligned)
   {
      return (unaligned + 255) & ~255;
   }
}