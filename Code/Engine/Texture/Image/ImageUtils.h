#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/Image/ImageFilter.h>

/// Collection of utility functions for image processing, analysis, and manipulation.
class W_TEXTURE_DLL WImageUtils
{
public:
  /// Computes the absolute difference between two images for comparison analysis.
  ///
  /// Both images must have the same dimensions and format. The output difference image
  /// contains the absolute difference for each channel. Useful for regression testing
  /// and quality analysis.
  static void ComputeImageDifferenceABS(const WImageView& imageA, const WImageView& imageB, WImage& out_difference);

  /// Computes relaxed difference allowing for 1-pixel shifts between images.
  ///
  /// For each pixel in imageA, searches for the minimum difference within a 1-pixel radius
  /// in imageB. This accounts for minor alignment differences or sub-pixel shifts that
  /// shouldn't be considered significant differences.
  static void ComputeImageDifferenceABSRelaxed(const WImageView& imageA, const WImageView& imageB, WImage& out_difference);

  /// Computes the mean square error for the block at (offsetx, offsety) to (offsetx + uiBlockSize, offsety + uiBlockSize).
  /// DifferenceImage is expected to be an image that represents the difference between two images.
  static WUInt32 ComputeMeanSquareError(const WImageView& differenceImage, WUInt8 uiBlockSize, WUInt32 uiOffsetx, WUInt32 uiOffsety);

  /// Computes the mean square error of DifferenceImage, by computing the MSE for blocks of uiBlockSize and returning the maximum MSE
  /// that was found.
  static WUInt32 ComputeMeanSquareError(const WImageView& differenceImage, WUInt8 uiBlockSize);

  /// Rescales pixel values to use the full value range by scaling from [min, max] to [0, 255].
  /// Computes combined min/max for RGB and separate min/max for alpha.
  static void Normalize(WImage& ref_image);
  static void Normalize(WImage& ref_image, WUInt8& ref_uiMinRgb, WUInt8& ref_uiMaxRgb, WUInt8& ref_uiMinAlpha, WUInt8& ref_uiMaxAlpha);

  /// Extracts the alpha channel from 8bpp 4 channel images into a 8bpp single channel image.
  static void ExtractAlphaChannel(const WImageView& inputImage, WImage& ref_outputImage);

  /// Returns the sub-image of \a input that starts at \a offset and has the size \a newsize
  static void CropImage(const WImageView& input, const WVec2I32& vOffset, const WSizeU32& newsize, WImage& ref_output);

  /// rotates a sub image by 180 degrees in place. Only works with uncompressed images.
  static void RotateSubImage180(WImage& ref_image, WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0);

  /// Copies the source image into the destination image at the specified location.
  ///
  /// The image must fit, no scaling or cropping is done. Image formats must be identical. Compressed formats are not supported.
  /// If the target location leaves not enough room for the source image to be copied, bad stuff will happen.
  static WResult Copy(const WImageView& srcImg, const WRectU32& srcRect, WImage& ref_dstImg, const WVec3U32& vDstOffset, WUInt32 uiDstMipLevel = 0,
    WUInt32 uiDstFace = 0, WUInt32 uiDstArrayIndex = 0);

  /// Copies the lower uiNumMips data of a 2D image into another one.
  static WResult ExtractLowerMipChain(const WImageView& src, WImage& ref_dst, WUInt32 uiNumMips);

  /// Mip map generation options
  struct MipMapOptions
  {
    /// The filter to use for mipmap generation. Defaults to bilinear filtering (Triangle filter) if none is given.
    const WImageFilter* m_filter = nullptr;

    /// Rescale RGB components to unit length for normal maps.
    ///
    /// Enable this when generating mipmaps for normal maps to maintain
    /// proper normal vector length after filtering.
    bool m_renormalizeNormals = false;

    /// Preserve alpha coverage for alpha testing.
    ///
    /// When enabled, alpha values are scaled to maintain the same coverage
    /// percentage as the original image when using alpha testing. Essential
    /// for proper rendering of vegetation and other alpha-tested geometry.
    bool m_preserveCoverage = false;

    /// Alpha test threshold for coverage preservation.
    ///
    /// Only used when m_preserveCoverage is true. Pixels with alpha >= threshold
    /// are considered opaque for coverage calculations.
    ///
    /// This must match the alpha test threshold that the renderer will later use for this texture.
    /// Each mip's alpha is rescaled such that this value is the cut-off at which the mip has
    /// the same coverage as the full resolution image.
    float m_alphaThreshold = 0.25f;

    /// The address mode for samples when filtering outside of the image dimensions in the horizontal direction.
    WImageAddressMode::Enum m_addressModeU = WImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the vertical direction.
    WImageAddressMode::Enum m_addressModeV = WImageAddressMode::Clamp;

    /// The address mode for samples when filtering outside of the image dimensions in the depth direction.
    WImageAddressMode::Enum m_addressModeW = WImageAddressMode::Clamp;

    /// Border color for ClampBorder address mode.
    WColor m_borderColor = WColor::Black;

    /// Number of mip levels to generate.
    ///
    /// Set to 0 to generate all possible mip levels down to 1x1.
    /// Set to a specific value to limit the number of generated levels.
    WUInt32 m_numMipMaps = 0;
  };

  /// Scales the image.
  static WResult Scale(const WImageView& source, WImage& ref_target, WUInt32 uiWidth, WUInt32 uiHeight, const WImageFilter* pFilter = nullptr,
    WImageAddressMode::Enum addressModeU = WImageAddressMode::Clamp, WImageAddressMode::Enum addressModeV = WImageAddressMode::Clamp,
    const WColor& borderColor = WColor::Black);

  /// Scales the image.
  static WResult Scale3D(const WImageView& source, WImage& ref_target, WUInt32 uiWidth, WUInt32 uiHeight, WUInt32 uiDepth,
    const WImageFilter* pFilter = nullptr, WImageAddressMode::Enum addressModeU = WImageAddressMode::Clamp,
    WImageAddressMode::Enum addressModeV = WImageAddressMode::Clamp, WImageAddressMode::Enum addressModeW = WImageAddressMode::Clamp,
    const WColor& borderColor = WColor::Black);

  /// Genererates the mip maps for the image. The input texture must be in WImageFormat::R32_G32_B32_A32_FLOAT
  static void GenerateMipMaps(const WImageView& source, WImage& ref_target, const MipMapOptions& options);

  /// Assumes that the Red and Green components of an image contain XY of an unit length normal and reconstructs the Z component into B
  static void ReconstructNormalZ(WImage& ref_source);

  /// Renormalizes a normal map to unit length.
  static void RenormalizeNormalMap(WImage& ref_image);

  /// Adjust the roughness in lower mip levels so it maintains the same look from all distances.
  static void AdjustRoughness(WImage& ref_roughnessMap, const WImageView& normalMap);

  /// Changes the exposure of an HDR image by 2^bias
  static void ChangeExposure(WImage& ref_image, float fBias);

  /// Creates a cubemap from srcImg and stores it in dstImg.
  ///
  /// If srcImg is already a cubemap, the data will be copied 1:1 to dstImg.
  /// If it is a 2D texture, it is analyzed and sub-images are copied to the proper faces of the output cubemap.
  ///
  /// Supported input layouts are:
  ///  * Vertical Cross
  ///  * Horizontal Cross
  ///  * Spherical mapping
  static WResult CreateCubemapFromSingleFile(WImage& ref_dstImg, const WImageView& srcImg);

  /// Copies the 6 given source images to the faces of dstImg.
  ///
  /// All input images must have the same square, power-of-two dimensions and mustn't be compressed.
  static WResult CreateCubemapFrom6Files(WImage& ref_dstImg, const WImageView* pSourceImages);

  static WResult CreateVolumeTextureFromSingleFile(WImage& ref_dstImg, const WImageView& srcImg);

  static WUInt32 GetSampleIndex(WUInt32 uiNumTexels, WInt32 iIndex, WImageAddressMode::Enum addressMode, bool& out_bUseBorderColor);

  /// Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static WColor NearestSample(const WImageView& image, WImageAddressMode::Enum addressMode, WVec2 vUv);

  /// Samples the image at the given UV coordinates with nearest filtering.
  ///
  /// Prefer this function over the one that takes an WImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as WImage::GetPixelPointer() is rather slow due to validation overhead.
  static WColor NearestSample(const WColor* pPixelPointer, WUInt32 uiWidth, WUInt32 uiHeight, WImageAddressMode::Enum addressMode, WVec2 vUv);

  /// Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// This function has to validate that the image is of the right format, and has to query the pixel pointer, which is slow.
  /// If you need to sample the image very often, use the overload that takes a pixel pointer instead of an image.
  static WColor BilinearSample(const WImageView& image, WImageAddressMode::Enum addressMode, WVec2 vUv);

  /// Samples the image at the given UV coordinates with bilinear filtering.
  ///
  /// Prefer this function over the one that takes an WImageView when you need to sample the image very often,
  /// as it does away with internal validation that would be redundant. Also, the pixel pointer given to this function
  /// should be retrieved only once from the source image, as WImage::GetPixelPointer() is rather slow due to validation overhead.
  static WColor BilinearSample(const WColor* pPixelPointer, WUInt32 uiWidth, WUInt32 uiHeight, WImageAddressMode::Enum addressMode, WVec2 vUv);

  /// Copies a single channel from srcImg into a single channel of ref_dstImg.
  ///
  /// Source and destination may use different formats, as long as both are LINEAR and use the same data type
  /// (e.g. both UNORM or both FLOAT) with uniform bits per channel (8, 16, or 32). The channel counts may differ,
  /// allowing copies between e.g. R8_UNORM and R8G8B8A8_UNORM. Both images must have identical width and height.
  static WResult CopyChannel(WImage& ref_dstImg, WUInt8 uiDstChannelIdx, const WImage& srcImg, WUInt8 uiSrcChannelIdx);

  /// Embeds the image as Base64 encoded text into an HTML file.
  static void EmbedImageData(WStringBuilder& out_sHtml, const WImage& image);

  /// Generates an HTML file containing the given images with mouse-over functionality to compare them.
  static void CreateImageDiffHtml(WStringBuilder& out_sHtml, WStringView sTitle, const WImage& referenceImgRgb, const WImage& referenceImgAlpha, const WImage& capturedImgRgb, const WImage& capturedImgAlpha, const WImage& diffImgRgb, const WImage& diffImgAlpha, WUInt32 uiError, WUInt32 uiThreshold, WUInt8 uiMinDiffRgb, WUInt8 uiMaxDiffRgb, WUInt8 uiMinDiffAlpha, WUInt8 uiMaxDiffAlpha);
};
