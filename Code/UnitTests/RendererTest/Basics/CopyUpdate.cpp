#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererTest/Basics/CopyUpdate.h>
#include <RendererTest/Basics/RendererTestUtils.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

namespace
{
  constexpr WUInt32 s_uiCopyTexturePaddingBytes = 0;
  constexpr WUInt32 s_uiCopyTextureNpotPaddingBytes = 4;
  constexpr WUInt32 s_uiCopyTextureArrayPaddingBytes = 8;
  constexpr WUInt32 s_uiCopyTextureCubePaddingBytes = 12;
  constexpr WUInt32 s_uiCopyTextureCubeArrayPaddingBytes = 16;
  constexpr WUInt32 s_uiCopyTextureBC1PaddingBytes = 8;

  void FillBufferPattern(WDynamicArray<WUInt8>& out_buffer, WUInt32 uiSize, bool bReverse)
  {
    out_buffer.SetCountUninitialized(uiSize);
    for (WUInt32 i = 0; i < uiSize; ++i)
    {
      out_buffer[i] = static_cast<WUInt8>(bReverse ? 255 - i : i);
    }
  }

  /// Creates a buffer suitable for the buffer copy/update tests.
  ///
  /// We use a structured buffer because that combination of flags supports both copy operations and AheadOfTime updates,
  /// and the readback path doesn't depend on the binding type.
  WGALBufferHandle CreateTestBuffer(WGALDevice* pDevice, WUInt32 uiTotalSize, WArrayPtr<const WUInt8> initialData)
  {
    WGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(WUInt32);
    desc.m_uiTotalSize = uiTotalSize;
    desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;
    desc.m_ResourceAccess.m_bImmutable = false;
    return pDevice->CreateBuffer(desc, initialData);
  }

  WGALTextureCreationDescription CreateTextureDesc(WGALTextureType::Enum type, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevels, WUInt32 uiArraySize = 1, WGALResourceFormat::Enum format = WGALResourceFormat::BGRAUByteNormalizedsRGB)
  {
    WGALTextureCreationDescription desc;
    desc.m_uiWidth = uiWidth;
    desc.m_uiHeight = uiHeight;
    desc.m_uiMipLevelCount = static_cast<WUInt8>(uiMipLevels);
    desc.m_uiArraySize = uiArraySize;
    desc.m_Type = type;
    desc.m_Format = format;
    desc.m_ResourceAccess.m_bImmutable = false;
    return desc;
  }

  /// Returns the total number of array layers a texture description has, expanding cube maps to 6 faces per cube.
  WUInt32 GetTotalSlices(const WGALTextureCreationDescription& desc)
  {
    const bool bIsCube = (desc.m_Type == WGALTextureType::TextureCube || desc.m_Type == WGALTextureType::TextureCubeArray);
    return bIsCube ? desc.m_uiArraySize * 6 : desc.m_uiArraySize;
  }

  WUInt32 GetExtraPaddingRows(WUInt32 uiPaddingBytes, WUInt32 uiBytesPerBlock)
  {
    return uiPaddingBytes / uiBytesPerBlock;
  }

  void CreateBC1Image(WImage& ref_image, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevelCount, WUInt8 uiSeed)
  {
    WImage sourceImage;
    WRendererTestUtils::CreateImage(sourceImage, uiWidth, uiHeight, uiMipLevelCount, false, uiSeed);
    W_TEST_BOOL(WImageConversion::Convert(sourceImage, ref_image, WImageFormat::BC1_UNORM).Succeeded());
  }

  void CreateTestImage(WImage& ref_image, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevelCount, WGALResourceFormat::Enum format, WUInt8 uiSeed)
  {
    if (format == WGALResourceFormat::BC1)
    {
      CreateBC1Image(ref_image, uiWidth, uiHeight, uiMipLevelCount, uiSeed);
      return;
    }

    WRendererTestUtils::CreateImage(ref_image, uiWidth, uiHeight, uiMipLevelCount, false, uiSeed);
  }

  void CopyImageRegionBytes(const WImage& sourceImage, WUInt32 uiSourceMipLevel, const WRectU32& sourceRect, WImage& ref_destinationImage, WUInt32 uiDestinationMipLevel, WVec3U32 vDestinationOffset)
  {
    const WImageFormat::Enum format = sourceImage.GetImageFormat();
    W_ASSERT_DEV(format == ref_destinationImage.GetImageFormat(), "Source and destination image formats must match.");

    const WUInt32 uiBlockWidth = WImageFormat::GetBlockWidth(format);
    const WUInt32 uiBlockHeight = WImageFormat::GetBlockHeight(format);
    W_ASSERT_DEV((sourceRect.x % uiBlockWidth) == 0 && (sourceRect.y % uiBlockHeight) == 0, "Source rectangle must be block-aligned.");
    W_ASSERT_DEV((vDestinationOffset.x % uiBlockWidth) == 0 && (vDestinationOffset.y % uiBlockHeight) == 0, "Destination offset must be block-aligned.");

    const WUInt32 uiSourceBlockX = sourceRect.x / uiBlockWidth;
    const WUInt32 uiSourceBlockY = sourceRect.y / uiBlockHeight;
    const WUInt32 uiDestinationBlockX = vDestinationOffset.x / uiBlockWidth;
    const WUInt32 uiDestinationBlockY = vDestinationOffset.y / uiBlockHeight;
    const WUInt32 uiBlockRows = WImageFormat::GetNumBlocksY(format, sourceRect.height);
    const WUInt32 uiRowSize = static_cast<WUInt32>(WImageFormat::GetRowPitch(format, sourceRect.width));

    for (WUInt32 y = 0; y < uiBlockRows; ++y)
    {
      const WUInt8* pSrc = sourceImage.GetPixelPointer<WUInt8>(uiSourceMipLevel, 0, 0, uiSourceBlockX, uiSourceBlockY + y);
      WUInt8* pDst = ref_destinationImage.GetPixelPointer<WUInt8>(uiDestinationMipLevel, 0, 0, uiDestinationBlockX, uiDestinationBlockY + y);
      WMemoryUtils::Copy(pDst, pSrc, uiRowSize);
    }
  }

  WGALSystemMemoryDescription BuildStridedData(const WImage& image, WUInt32 uiMipLevel, const WRectU32& sourceRect, WUInt32 uiPaddingBytes, WDynamicArray<WUInt8>& out_storage)
  {
    const WImageFormat::Enum format = image.GetImageFormat();
    const WUInt32 uiBlockWidth = WImageFormat::GetBlockWidth(format);
    const WUInt32 uiBlockHeight = WImageFormat::GetBlockHeight(format);
    W_ASSERT_DEV((sourceRect.x % uiBlockWidth) == 0 && (sourceRect.y % uiBlockHeight) == 0, "Source rectangle must be block-aligned.");

    const WUInt32 uiSourceBlockX = sourceRect.x / uiBlockWidth;
    const WUInt32 uiSourceBlockY = sourceRect.y / uiBlockHeight;
    const WUInt32 uiBlockRows = WImageFormat::GetNumBlocksY(format, sourceRect.height);
    const WUInt32 uiBlockColumns = WImageFormat::GetNumBlocksX(format, sourceRect.width);
    const WUInt32 uiTightRowPitch = static_cast<WUInt32>(WImageFormat::GetRowPitch(format, sourceRect.width));
    const WUInt32 uiBytesPerBlock = uiTightRowPitch / uiBlockColumns;
    const WUInt32 uiRowPitch = uiTightRowPitch + uiPaddingBytes;
    const WUInt32 uiSlicePitch = uiRowPitch * (uiBlockRows + GetExtraPaddingRows(uiPaddingBytes, uiBytesPerBlock));

    out_storage.SetCount(uiSlicePitch, 0xCD);
    for (WUInt32 y = 0; y < uiBlockRows; ++y)
    {
      const WUInt8* pSrc = image.GetPixelPointer<WUInt8>(uiMipLevel, 0, 0, uiSourceBlockX, uiSourceBlockY + y);
      WUInt8* pDst = out_storage.GetData() + y * uiRowPitch;
      WMemoryUtils::Copy(pDst, pSrc, uiTightRowPitch);
    }

    WGALSystemMemoryDescription memory;
    memory.m_pData = out_storage.GetByteArrayPtr();
    memory.m_uiRowPitch = uiRowPitch;
    memory.m_uiSlicePitch = uiSlicePitch;
    return memory;
  }

  WGALSystemMemoryDescription BuildStridedData(const WImage& image, WUInt32 uiMipLevel, WUInt32 uiPaddingBytes, WDynamicArray<WUInt8>& out_storage)
  {
    return BuildStridedData(image, uiMipLevel, WRectU32(0, 0, image.GetWidth(uiMipLevel), image.GetHeight(uiMipLevel)), uiPaddingBytes, out_storage);
  }

  void BuildInitialData(WArrayPtr<const WImage> images, WUInt32 uiPaddingBytes, WDynamicArray<WGALSystemMemoryDescription>& out_initialData,
    WDynamicArray<WDynamicArray<WUInt8>>& out_initialDataStorage)
  {
    out_initialData.Clear();
    out_initialDataStorage.Clear();
    if (images.IsEmpty())
      return;

    const WUInt32 uiMipLevels = images[0].GetNumMipLevels();
    out_initialData.Reserve(images.GetCount() * uiMipLevels);
    out_initialDataStorage.SetCount(images.GetCount() * uiMipLevels);
    WUInt32 uiSubresourceIndex = 0;
    for (WUInt32 s = 0; s < images.GetCount(); ++s)
    {
      W_ASSERT_DEV(images[s].GetNumMipLevels() == uiMipLevels, "All layered test images must have the same mip count");
      for (WUInt32 m = 0; m < uiMipLevels; ++m)
      {
        out_initialData.PushBack(BuildStridedData(images[s], m, uiPaddingBytes, out_initialDataStorage[uiSubresourceIndex]));
        ++uiSubresourceIndex;
      }
    }
  }

  WGALTextureHandle CreateTexture(WGALDevice* pDevice, const WGALTextureCreationDescription& desc, WArrayPtr<const WImage> initialImages, WUInt32 uiPaddingBytes)
  {
    if (initialImages.IsEmpty())
      return pDevice->CreateTexture(desc, {});

    WDynamicArray<WGALSystemMemoryDescription> initialData;
    WDynamicArray<WDynamicArray<WUInt8>> initialDataStorage;
    BuildInitialData(initialImages, uiPaddingBytes, initialData, initialDataStorage);
    return pDevice->CreateTexture(desc, initialData.GetArrayPtr());
  }

} // namespace


void WRendererTestCopyUpdate::SetupSubTests()
{
  const WGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - CopyUpdateBuffer", SubTests::ST_CopyBuffer);
  AddSubTest("02 - CopyUpdateTexture", SubTests::ST_CopyTexture);
  AddSubTest("03 - CopyUpdateTexture2DArray", SubTests::ST_CopyTextureArray);
  AddSubTest("04 - CopyUpdateTextureCube", SubTests::ST_CopyTextureCube);
  AddSubTest("05 - CopyUpdateTextureCubeArray", SubTests::ST_CopyTextureCubeArray);
  AddSubTest("06 - CopyUpdateTextureNPOT", SubTests::ST_CopyTextureNpot);

  const auto bc1Support = caps.m_FormatSupport[WGALResourceFormat::BC1];
  const bool bCanCreateBC1 = WImageConversion::IsConvertible(WImageFormat::B8G8R8A8_UNORM_SRGB, WImageFormat::BC1_UNORM);
  if (bc1Support.IsSet(WGALResourceFormatSupport::Texture) && bCanCreateBC1)
  {
    AddSubTest("07 - CopyUpdateTextureBC1", SubTests::ST_CopyTextureBC1);
  }
}

WResult WRendererTestCopyUpdate::InitializeTest()
{
  WStartup::StartupCoreSystems();

  if (SetupRenderer().Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WResult WRendererTestCopyUpdate::DeInitializeTest()
{
  ShutdownRenderer();
  WStartup::ShutdownCoreSystems();
  WMemoryTracker::DumpMemoryLeaks();
  return W_SUCCESS;
}

WResult WRendererTestCopyUpdate::InitializeSubTest(WInt32 iIdentifier)
{
  m_iFrame = -1;
  m_BufferReadback.Reset();
  m_TextureReadback.Reset();

  switch (iIdentifier)
  {
    case ST_CopyBuffer:
    {
      // Source: counter pattern, destination: all 0xCC so we can tell which bytes survived.
      FillBufferPattern(m_BufferSourceData, s_uiBufferSize, false);

      WDynamicArray<WUInt8> destInitial;
      destInitial.SetCount(s_uiBufferSize, 0xCC);

      m_hBufferSource = CreateTestBuffer(m_pDevice, s_uiBufferSize, m_BufferSourceData.GetByteArrayPtr());
      m_hBufferDest = CreateTestBuffer(m_pDevice, s_uiBufferSize, destInitial.GetByteArrayPtr());
      W_TEST_BOOL(!m_hBufferSource.IsInvalidated());
      W_TEST_BOOL(!m_hBufferDest.IsInvalidated());
    }
    break;
    case ST_CopyTexture:
      SetupTextureCopyUpdatePair(WGALTextureType::Texture2D, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTexturePaddingBytes);
      break;
    case ST_CopyTextureNpot:
      SetupTextureCopyUpdatePair(WGALTextureType::Texture2D, 1, s_uiNpotTextureWidth, s_uiNpotTextureHeight, s_uiTextureMips, s_uiCopyTextureNpotPaddingBytes);
      break;
    case ST_CopyTextureArray:
      SetupTextureCopyUpdatePair(WGALTextureType::Texture2DArray, 4, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureArrayPaddingBytes);
      break;
    case ST_CopyTextureCube:
      SetupTextureCopyUpdatePair(WGALTextureType::TextureCube, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureCubePaddingBytes);
      break;
    case ST_CopyTextureCubeArray:
      SetupTextureCopyUpdatePair(
        WGALTextureType::TextureCubeArray, 2, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureCubeArrayPaddingBytes);
      break;
    case ST_CopyTextureBC1:
      SetupTextureCopyUpdatePair(WGALTextureType::Texture2D, 1, s_uiTextureSize, s_uiTextureSize, s_uiTextureMips, s_uiCopyTextureBC1PaddingBytes, WGALResourceFormat::BC1);
      break;
  }

  return W_SUCCESS;
}

WResult WRendererTestCopyUpdate::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_BufferReadback.Reset();
  m_TextureReadback.Reset();

  m_pDevice->DestroyBuffer(m_hBufferSource);
  m_pDevice->DestroyBuffer(m_hBufferDest);
  m_pDevice->DestroyTexture(m_hTextureSource);
  m_pDevice->DestroyTexture(m_hTextureDest);

  m_BufferSourceData.Clear();
  m_TextureSourceImages.Clear();
  m_TextureDestImages.Clear();
  return W_SUCCESS;
}

WTestAppRun WRendererTestCopyUpdate::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;

  if (m_iFrame > 1)
    return WTestAppRun::Quit;

  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      BeginFrame();
      RunCopyBuffer();
      EndFrame();
      break;
    case ST_CopyTexture:
    case ST_CopyTextureNpot:
    case ST_CopyTextureArray:
    case ST_CopyTextureCube:
    case ST_CopyTextureCubeArray:
    case ST_CopyTextureBC1:
      BeginFrame();
      RunCopyTextureRegion(iIdentifier);
      RunCopyTexture();
      RunUpdateTexture(iIdentifier);
      EndFrame();

      RunUpdateTextureForNextFrame(iIdentifier);

      BeginFrame();
      VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
      EndFrame();
      break;
  }

  return WTestAppRun::Quit;
}

void WRendererTestCopyUpdate::RunCopyBuffer()
{
  constexpr WUInt32 uiRegionSrcOffset = 32;
  constexpr WUInt32 uiRegionDstOffset = 96;
  constexpr WUInt32 uiRegionByteCount = 64;

  BeginCommands("CopyBufferRegion");
  TransitionBuffer(m_hBufferSource, WGALResourceState::CopySource);
  TransitionBuffer(m_hBufferDest, WGALResourceState::CopyDestination);
  m_pEncoder->CopyBufferRegion(m_hBufferDest, uiRegionDstOffset, m_hBufferSource, uiRegionSrcOffset, uiRegionByteCount);
  EndCommands();

  WDynamicArray<WUInt8> expected;
  expected.SetCount(s_uiBufferSize, 0xCC);
  WMemoryUtils::Copy(expected.GetData() + uiRegionDstOffset, m_BufferSourceData.GetData() + uiRegionSrcOffset, uiRegionByteCount);

  VerifyBufferReadback(m_hBufferDest, expected.GetByteArrayPtr());

  BeginCommands("CopyBuffer");
  TransitionBuffer(m_hBufferSource, WGALResourceState::CopySource);
  TransitionBuffer(m_hBufferDest, WGALResourceState::CopyDestination);
  m_pEncoder->CopyBuffer(m_hBufferDest, m_hBufferSource);
  EndCommands();

  VerifyBufferReadback(m_hBufferDest, m_BufferSourceData.GetByteArrayPtr());

  WDynamicArray<WUInt8> updateData;
  FillBufferPattern(updateData, s_uiBufferSize, true);
  constexpr WUInt32 uiUpdateSplitOffset = s_uiBufferSize / 2;

  BeginCommands("UpdateBuffer");
  // AheadOfTime updates must not overlap within the same frame. Two chunks cover the full buffer and exercise a non-zero offset.
  m_pEncoder->UpdateBuffer(m_hBufferDest, 0, updateData.GetArrayPtr().GetSubArray(0, uiUpdateSplitOffset), WGALUpdateMode::AheadOfTime);
  m_pEncoder->UpdateBuffer(m_hBufferDest, uiUpdateSplitOffset,
    updateData.GetArrayPtr().GetSubArray(uiUpdateSplitOffset, s_uiBufferSize - uiUpdateSplitOffset), WGALUpdateMode::AheadOfTime);
  EndCommands();

  VerifyBufferReadback(m_hBufferDest, updateData.GetByteArrayPtr());
}

void WRendererTestCopyUpdate::RunCopyTextureRegion(WInt32 iIdentifier)
{
  WUInt32 uiSourceOffset = 1;
  WUInt32 uiDestinationOffset = 2;
  WUInt32 uiCopySize = 4;

  WGALTextureSubresource srcSub{0, 0};
  WGALTextureSubresource dstSub{0, 0};
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      W_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
    case ST_CopyTextureNpot:
      break;
    case ST_CopyTextureBC1:
      uiSourceOffset = 0;
      uiDestinationOffset = 4;
      break;
    case ST_CopyTextureArray:
      srcSub.m_uiArraySlice = 1;
      dstSub.m_uiArraySlice = 2;
      break;
    case ST_CopyTextureCube:
      dstSub.m_uiArraySlice = 5;
      break;
    case ST_CopyTextureCubeArray:
      srcSub.m_uiArraySlice = 2;
      dstSub.m_uiArraySlice = 9;
      break;
  }

  WBoundingBoxu32 box;
  box.m_vMin = WVec3U32(uiSourceOffset, uiSourceOffset, 0);
  box.m_vMax = WVec3U32(uiSourceOffset + uiCopySize, uiSourceOffset + uiCopySize, 1);
  WVec3U32 dstPoint(uiDestinationOffset, uiDestinationOffset, 0);

  BeginCommands("CopyTextureRegion");
  TransitionTexture(m_hTextureSource, WGALResourceState::CopySource);
  TransitionTexture(m_hTextureDest, WGALResourceState::CopyDestination);
  m_pEncoder->CopyTextureRegion(m_hTextureDest, dstSub, dstPoint, m_hTextureSource, srcSub, box);
  EndCommands();

  const WRectU32 sourceRect(box.m_vMin.x, box.m_vMin.y, box.m_vMax.x - box.m_vMin.x, box.m_vMax.y - box.m_vMin.y);
  CopyImageRegionBytes(m_TextureSourceImages[srcSub.m_uiArraySlice], srcSub.m_uiMipLevel, sourceRect, m_TextureDestImages[dstSub.m_uiArraySlice], dstSub.m_uiMipLevel, dstPoint);

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void WRendererTestCopyUpdate::RunCopyTexture()
{
  BeginCommands("CopyTexture");
  TransitionTexture(m_hTextureSource, WGALResourceState::CopySource);
  TransitionTexture(m_hTextureDest, WGALResourceState::CopyDestination);
  m_pEncoder->CopyTexture(m_hTextureDest, m_hTextureSource);
  EndCommands();

  for (WUInt32 s = 0; s < m_TextureDestImages.GetCount(); ++s)
  {
    for (WUInt32 m = 0; m < m_TextureSourceImages[s].GetNumMipLevels(); ++m)
    {
      const WRectU32 sourceRect(0, 0, m_TextureSourceImages[s].GetWidth(m), m_TextureSourceImages[s].GetHeight(m));
      CopyImageRegionBytes(m_TextureSourceImages[s], m, sourceRect, m_TextureDestImages[s], m, WVec3U32(0));
    }
  }

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void WRendererTestCopyUpdate::RunUpdateTexture(WInt32 iIdentifier)
{
  WUInt32 uiMipLevel = 1;
  WUInt32 uiSourceOffset = 1;
  WUInt32 uiOffset = 1;
  WUInt32 uiSize = 2;
  WUInt32 uiArraySlice = 0;
  WUInt32 uiPaddingBytes = s_uiCopyTexturePaddingBytes;
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      W_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
      break;
    case ST_CopyTextureNpot:
      uiPaddingBytes = s_uiCopyTextureNpotPaddingBytes;
      break;
    case ST_CopyTextureArray:
      uiArraySlice = 2;
      uiPaddingBytes = s_uiCopyTextureArrayPaddingBytes;
      break;
    case ST_CopyTextureCube:
      uiArraySlice = 5;
      uiPaddingBytes = s_uiCopyTextureCubePaddingBytes;
      break;
    case ST_CopyTextureCubeArray:
      uiArraySlice = 9;
      uiPaddingBytes = s_uiCopyTextureCubeArrayPaddingBytes;
      break;
    case ST_CopyTextureBC1:
      uiMipLevel = 0;
      uiSourceOffset = 0;
      uiOffset = 4;
      uiSize = 4;
      uiPaddingBytes = s_uiCopyTextureBC1PaddingBytes;
      break;
  }

  const WGALTexture* pTexture = m_pDevice->GetTexture(m_hTextureDest);
  const WGALTextureCreationDescription& desc = pTexture->GetDescription();

  WImage updateImage;
  CreateTestImage(updateImage, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, desc.m_Format, static_cast<WUInt8>(200u + uiArraySlice));

  const WRectU32 sourceRect(uiSourceOffset, uiSourceOffset, uiSize, uiSize);
  WDynamicArray<WUInt8> updateDataStorage;
  WGALSystemMemoryDescription srcMem = BuildStridedData(updateImage, uiMipLevel, sourceRect, uiPaddingBytes, updateDataStorage);

  WBoundingBoxu32 destBox;
  destBox.m_vMin = WVec3U32(uiOffset, uiOffset, 0);
  destBox.m_vMax = WVec3U32(uiOffset + uiSize, uiOffset + uiSize, 1);

  BeginCommands("UpdateTexture");
  // UpdateTexture goes through the InitContext which manages its own barriers around the upload.
  WGALTextureSubresource subResource;
  subResource.m_uiMipLevel = uiMipLevel;
  subResource.m_uiArraySlice = uiArraySlice;
  m_pEncoder->UpdateTexture(m_hTextureDest, subResource, destBox, srcMem);
  EndCommands();

  const WVec3U32 destinationOffset(uiOffset, uiOffset, 0);
  CopyImageRegionBytes(updateImage, uiMipLevel, sourceRect, m_TextureDestImages[uiArraySlice], uiMipLevel, destinationOffset);

  VerifyTextureSliceReadback(m_hTextureDest, m_TextureDestImages.GetArrayPtr());
}

void WRendererTestCopyUpdate::RunUpdateTextureForNextFrame(WInt32 iIdentifier)
{
  WUInt32 uiMipLevel = 1;
  WUInt32 uiSourceOffset = 0;
  WUInt32 uiOffset = 0;
  WUInt32 uiSize = 2;
  WUInt32 uiArraySlice = 0;
  WUInt32 uiPaddingBytes = s_uiCopyTexturePaddingBytes;
  switch (iIdentifier)
  {
    case ST_CopyBuffer:
      W_ASSERT_NOT_IMPLEMENTED;
      return;
    case ST_CopyTexture:
      break;
    case ST_CopyTextureNpot:
      uiPaddingBytes = s_uiCopyTextureNpotPaddingBytes;
      break;
    case ST_CopyTextureArray:
      uiArraySlice = 2;
      uiPaddingBytes = s_uiCopyTextureArrayPaddingBytes;
      break;
    case ST_CopyTextureCube:
      uiArraySlice = 5;
      uiPaddingBytes = s_uiCopyTextureCubePaddingBytes;
      break;
    case ST_CopyTextureCubeArray:
      uiArraySlice = 9;
      uiPaddingBytes = s_uiCopyTextureCubeArrayPaddingBytes;
      break;
    case ST_CopyTextureBC1:
      uiMipLevel = 0;
      uiSourceOffset = 0;
      uiOffset = 0;
      uiSize = 4;
      uiPaddingBytes = s_uiCopyTextureBC1PaddingBytes;
      break;
  }

  const WGALTexture* pTexture = m_pDevice->GetTexture(m_hTextureDest);
  const WGALTextureCreationDescription& desc = pTexture->GetDescription();

  WImage updateImage;
  CreateTestImage(updateImage, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, desc.m_Format, static_cast<WUInt8>(220u + uiArraySlice));

  const WRectU32 sourceRect(uiSourceOffset, uiSourceOffset, uiSize, uiSize);
  WDynamicArray<WUInt8> updateDataStorage;
  WGALSystemMemoryDescription srcMem = BuildStridedData(updateImage, uiMipLevel, sourceRect, uiPaddingBytes, updateDataStorage);

  WBoundingBoxu32 destBox;
  destBox.m_vMin = WVec3U32(uiOffset, uiOffset, 0);
  destBox.m_vMax = WVec3U32(uiOffset + uiSize, uiOffset + uiSize, 1);

  WGALTextureSubresource subResource;
  subResource.m_uiMipLevel = uiMipLevel;
  subResource.m_uiArraySlice = uiArraySlice;
  m_pDevice->UpdateTextureForNextFrame(m_hTextureDest, srcMem, subResource, destBox);

  const WVec3U32 destinationOffset(uiOffset, uiOffset, 0);
  CopyImageRegionBytes(updateImage, uiMipLevel, sourceRect, m_TextureDestImages[uiArraySlice], uiMipLevel, destinationOffset);
}

void WRendererTestCopyUpdate::VerifyBufferReadback(WGALBufferHandle hBuffer, WArrayPtr<const WUInt8> expected)
{
  BeginCommands("ReadbackBuffer");
  TransitionBuffer(hBuffer, WGALResourceState::CopySource);
  m_BufferReadback.ReadbackBuffer(*m_pEncoder, hBuffer);
  EndCommands();

  WEnum<WGALAsyncResult> res = m_BufferReadback.GetReadbackResult(WTime::MakeFromHours(1));
  if (!W_TEST_BOOL_MSG(res == WGALAsyncResult::Ready, "Buffer readback timed out"))
    return;

  WArrayPtr<const WUInt8> memory;
  WReadbackBufferLock lock = m_BufferReadback.LockBuffer(memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock readback buffer");

  if (W_TEST_INT(memory.GetCount(), expected.GetCount()))
  {
    W_TEST_INT(WMemoryUtils::Compare(memory.GetPtr(), expected.GetPtr(), memory.GetCount()), 0);
  }
}

void WRendererTestCopyUpdate::SetupTextureCopyUpdatePair(
  WGALTextureType::Enum type, WUInt32 uiArraySize, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiMipLevels, WUInt32 uiPaddingBytes, WGALResourceFormat::Enum format)
{
  WGALTextureCreationDescription desc = CreateTextureDesc(type, uiWidth, uiHeight, uiMipLevels, uiArraySize, format);
  const WUInt32 uiTotalSlices = GetTotalSlices(desc);

  m_TextureSourceImages.SetCount(uiTotalSlices);
  m_TextureDestImages.SetCount(uiTotalSlices);

  for (WUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    CreateTestImage(m_TextureSourceImages[s], uiWidth, uiHeight, uiMipLevels, format, static_cast<WUInt8>(s));
    CreateTestImage(m_TextureDestImages[s], uiWidth, uiHeight, uiMipLevels, format, static_cast<WUInt8>(100u + s));
  }

  m_hTextureSource = CreateTexture(m_pDevice, desc, m_TextureSourceImages.GetArrayPtr(), uiPaddingBytes);
  m_hTextureDest = CreateTexture(m_pDevice, desc, m_TextureDestImages.GetArrayPtr(), uiPaddingBytes);
  W_TEST_BOOL(!m_hTextureSource.IsInvalidated());
  W_TEST_BOOL(!m_hTextureDest.IsInvalidated());

  if (!m_hTextureSource.IsInvalidated() && !m_hTextureDest.IsInvalidated())
  {
    const WGALTexture* pSourceTexture = m_pDevice->GetTexture(m_hTextureSource);
    const WGALTexture* pDestTexture = m_pDevice->GetTexture(m_hTextureDest);

    for (WUInt32 uiMipLevel = 0; uiMipLevel < uiMipLevels; ++uiMipLevel)
    {
      const WVec3U32 vSourceImageSize(m_TextureSourceImages[0].GetWidth(uiMipLevel), m_TextureSourceImages[0].GetHeight(uiMipLevel), 1);
      const WVec3U32 vDestImageSize(m_TextureDestImages[0].GetWidth(uiMipLevel), m_TextureDestImages[0].GetHeight(uiMipLevel), 1);

      W_TEST_BOOL(pSourceTexture->GetMipMapSize(uiMipLevel) == vSourceImageSize);
      W_TEST_BOOL(pDestTexture->GetMipMapSize(uiMipLevel) == vDestImageSize);
    }
  }
}

void WRendererTestCopyUpdate::VerifyTextureSliceReadback(WGALTextureHandle hTexture, WArrayPtr<const WImage> expectedLayers)
{
  BeginCommands("ReadbackTextureLayered");
  TransitionTexture(hTexture, WGALResourceState::CopySource);
  m_TextureReadback.ReadbackTexture(*m_pEncoder, hTexture);
  EndCommands();

  WEnum<WGALAsyncResult> res = m_TextureReadback.GetReadbackResult(WTime::MakeFromHours(1));
  if (!W_TEST_BOOL_MSG(res == WGALAsyncResult::Ready, "Layered texture readback timed out"))
    return;

  const WGALTexture* pTexture = m_pDevice->GetTexture(hTexture);
  const WGALTextureCreationDescription& desc = pTexture->GetDescription();
  const WUInt32 uiTotalSlices = expectedLayers.GetCount();
  W_ASSERT_DEV(uiTotalSlices == GetTotalSlices(desc), "Mismatch between expected layer count and texture slice count");
  for (WUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    W_ASSERT_DEV(expectedLayers[s].GetNumMipLevels() == desc.m_uiMipLevelCount, "Mismatch between expected image mip count and texture mip count");
  }

  WDynamicArray<WGALTextureSubresource> subs;
  subs.SetCount(uiTotalSlices * desc.m_uiMipLevelCount);
  WUInt32 uiSubresourceIndex = 0;
  for (WUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    for (WUInt32 m = 0; m < desc.m_uiMipLevelCount; ++m)
    {
      subs[uiSubresourceIndex].m_uiMipLevel = m;
      subs[uiSubresourceIndex].m_uiArraySlice = s;
      ++uiSubresourceIndex;
    }
  }

  WDynamicArray<WGALSystemMemoryDescription> memory;
  WReadbackTextureLock lock = m_TextureReadback.LockTexture(subs.GetArrayPtr(), memory);
  W_ASSERT_ALWAYS(lock, "Failed to lock layered readback texture");

  uiSubresourceIndex = 0;
  for (WUInt32 s = 0; s < uiTotalSlices; ++s)
  {
    for (WUInt32 m = 0; m < desc.m_uiMipLevelCount; ++m)
    {
      const WUInt32 uiCurrentSubresourceIndex = uiSubresourceIndex++;
      WImage actual;
      WTextureUtils::CopySubResourceToImage(desc, subs[uiCurrentSubresourceIndex], memory[uiCurrentSubresourceIndex], actual, false);

      const auto expectedView = expectedLayers[s].GetSubImageView(m);
      if (!W_TEST_INT(actual.GetByteBlobPtr().GetCount(), expectedView.GetByteBlobPtr().GetCount()))
        continue;

      const int cmp = WMemoryUtils::Compare(actual.GetByteBlobPtr().GetPtr(), expectedView.GetByteBlobPtr().GetPtr(), actual.GetByteBlobPtr().GetCount());
      W_TEST_INT_MSG(cmp, 0, "Slice {} mismatch after copy", s);
    }
  }
}

static WRendererTestCopyUpdate g_CopyUpdateTest;
