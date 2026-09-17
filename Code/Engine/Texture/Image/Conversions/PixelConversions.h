#pragma once

#include <Texture/Image/Image.h>

W_TEXTURE_DLL WColorBaseUB WDecompressA4B4G4R4(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressB4G4R4A4(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressB5G6R5(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressB5G5R5X1(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressB5G5R5A1(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressX1B5G5R5(WUInt16 uiColor);
W_TEXTURE_DLL WColorBaseUB WDecompressA1B5G5R5(WUInt16 uiColor);
W_TEXTURE_DLL WUInt16 WCompressA4B4G4R4(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressB4G4R4A4(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressB5G6R5(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressB5G5R5X1(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressB5G5R5A1(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressX1B5G5R5(WColorBaseUB color);
W_TEXTURE_DLL WUInt16 WCompressA1B5G5R5(WColorBaseUB color);
