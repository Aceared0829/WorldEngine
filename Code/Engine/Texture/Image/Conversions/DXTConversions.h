#pragma once

#include <Texture/Image/Image.h>

class WColorLinear16f;

W_TEXTURE_DLL void WDecompressBlockBC1(const WUInt8* pSource, WColorBaseUB* pTarget, bool bForceFourColorMode);
W_TEXTURE_DLL void WDecompressBlockBC4(const WUInt8* pSource, WUInt8* pTarget, WUInt32 uiStride, WUInt8 uiBias);
W_TEXTURE_DLL void WDecompressBlockBC6(const WUInt8* pSource, WColorLinear16f* pTarget, bool bIsSigned);
W_TEXTURE_DLL void WDecompressBlockBC7(const WUInt8* pSource, WColorBaseUB* pTarget);

W_TEXTURE_DLL void WUnpackPaletteBC4(WUInt32 ui0, WUInt32 ui1, WUInt32* pAlphas);
