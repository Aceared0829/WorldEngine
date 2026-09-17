#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Utilities/EnumerableClass.h>

#include <Texture/Image/Image.h>

W_DECLARE_FLAGS(WUInt8, WImageConversionFlags, InPlace);

/// Describes a single conversion step between two image formats.
///
/// Used by conversion step implementations to advertise which format pairs they can handle.
/// The conversion system uses this information to build optimal conversion paths.
struct WImageConversionEntry
{
  WImageConversionEntry(WImageFormat::Enum source, WImageFormat::Enum target, WImageConversionFlags::Enum flags, float fAdditionalPenalty = 0)
    : m_sourceFormat(source)
    , m_targetFormat(target)
    , m_flags(flags)
    , m_fAdditionalPenalty(fAdditionalPenalty)
  {
  }

  const WImageFormat::Enum m_sourceFormat;
  const WImageFormat::Enum m_targetFormat;
  const WBitflags<WImageConversionFlags> m_flags;

  /// Additional cost penalty for this conversion step.
  ///
  /// Used to bias the pathfinding algorithm when multiple conversion routes are available.
  /// Higher penalties make this step less likely to be chosen in the optimal path.
  float m_fAdditionalPenalty = 0.0f;
};

/// Interface for a single image conversion step.
///
/// The actual functionality is implemented as either WImageConversionStepLinear or WImageConversionStepDecompressBlocks.
/// Depending on the types on conversion advertised by GetSupportedConversions(), users of this class need to cast it to a derived type
/// first to access the desired functionality.
class W_TEXTURE_DLL WImageConversionStep : public WEnumerable<WImageConversionStep>
{
  W_DECLARE_ENUMERABLE_CLASS(WImageConversionStep);

protected:
  WImageConversionStep();
  virtual ~WImageConversionStep();

public:
  /// Returns an array pointer of supported conversions.
  ///
  /// \note The returned array must have the same entries each time this method is called.
  virtual WArrayPtr<const WImageConversionEntry> GetSupportedConversions() const = 0;
};

/// Interface for a single image conversion step where both the source and target format are uncompressed.
class W_TEXTURE_DLL WImageConversionStepLinear : public WImageConversionStep
{
public:
  /// Converts a batch of pixels.
  virtual WResult ConvertPixels(WConstByteBlobPtr source, WByteBlobPtr target, WUInt64 uiNumElements, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const = 0;
};

/// Interface for a single image conversion step where the source format is compressed and the target format is uncompressed.
class W_TEXTURE_DLL WImageConversionStepDecompressBlocks : public WImageConversionStep
{
public:
  /// Decompresses the given number of blocks.
  virtual WResult DecompressBlocks(WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumBlocks, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const = 0;
};

/// Interface for a single image conversion step where the source format is uncompressed and the target format is compressed.
class W_TEXTURE_DLL WImageConversionStepCompressBlocks : public WImageConversionStep
{
public:
  /// Compresses the given number of blocks.
  virtual WResult CompressBlocks(WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumBlocksX, WUInt32 uiNumBlocksY,
    WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat) const = 0;
};

/// Interface for a single image conversion step from a linear to a planar format.
class W_TEXTURE_DLL WImageConversionStepPlanarize : public WImageConversionStep
{
public:
  /// Converts a batch of pixels into the given target planes.
  virtual WResult ConvertPixels(const WImageView& source, WArrayPtr<WImage> target, WUInt32 uiNumPixelsX, WUInt32 uiNumPixelsY, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const = 0;
};

/// Interface for a single image conversion step from a planar to a linear format.
class W_TEXTURE_DLL WImageConversionStepDeplanarize : public WImageConversionStep
{
public:
  /// Converts a batch of pixels from the given source planes.
  virtual WResult ConvertPixels(WArrayPtr<WImageView> source, WImage target, WUInt32 uiNumPixelsX, WUInt32 uiNumPixelsY, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat) const = 0;
};


/// High-level image format conversion system with automatic path finding.
///
/// This class provides a complete image conversion system that can automatically find
/// optimal conversion paths between any two supported formats. It uses a plugin-based
/// architecture where conversion steps register themselves at startup.
///
/// **Basic Usage:**
/// ```cpp
/// // Simple format conversion
/// WImage sourceImage;
/// sourceImage.LoadFrom("texture.png");
/// WImage targetImage;
/// WImageConversion::Convert(sourceImage, targetImage, WImageFormat::BC1_UNORM);
/// ```
///
/// **Advanced Usage with Path Caching:**
/// ```cpp
/// // Build reusable conversion path
/// WHybridArray<WImageConversion::ConversionPathNode, 16> path;
/// WUInt32 numScratchBuffers;
/// WImageConversion::BuildPath(sourceFormat, targetFormat, false, path, numScratchBuffers);
///
/// // Use cached path for multiple conversions
/// for (auto& image : images)
/// {
///   WImageConversion::Convert(image, convertedImage, path, numScratchBuffers);
/// }
/// ```
///
/// The conversion system automatically handles:
/// - Multi-step conversions (e.g., BC1 -> RGBA8 -> BC7)
/// - Memory layout differences (linear, block-compressed, planar)
/// - Optimal path selection based on quality and performance
/// - In-place conversions when possible
class W_TEXTURE_DLL WImageConversion
{
public:
  /// Checks if a conversion path exists between two formats.
  ///
  /// This is a fast query that doesn't build the actual conversion path.
  /// Use this to validate format compatibility before attempting conversion.
  static bool IsConvertible(WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat);

  /// Finds the format requiring the least conversion cost from a list of candidates.
  ///
  /// Useful when you have multiple acceptable target formats and want to choose
  /// the one that preserves the most quality or requires the least processing.
  static WImageFormat::Enum FindClosestCompatibleFormat(WImageFormat::Enum format, WArrayPtr<const WImageFormat::Enum> compatibleFormats);

  /// A single node along a computed conversion path.
  struct ConversionPathNode
  {
    W_DECLARE_POD_TYPE();

    const WImageConversionStep* m_step;
    WImageFormat::Enum m_sourceFormat;
    WImageFormat::Enum m_targetFormat;
    WUInt32 m_sourceBufferIndex;
    WUInt32 m_targetBufferIndex;
    bool m_inPlace;
  };

  /// Precomputes an optimal conversion path between two formats and the minimal number of required scratch buffers.
  ///
  /// The generated path can be cached by the user if the same conversion is performed multiple times. The path must not be reused if the
  /// set of supported conversions changes, e.g. when plugins are loaded or unloaded.
  ///
  /// \param sourceFormat           The source format.
  /// \param targetFormat           The target format.
  /// \param sourceEqualsTarget     If true, the generated path is applicable if source and target memory regions are equal, and may contain
  /// additional copy-steps if the conversion can't be performed in-place.
  ///                               A path generated with sourceEqualsTarget == true will work correctly even if source and target are not
  ///                               the same, but may not be optimal. A path generated with sourceEqualsTarget == false will not work
  ///                               correctly when source and target are the same.
  /// \param out_path               The generated path.
  /// \param out_numScratchBuffers The number of scratch buffers required for the conversion path.
  /// \returns                      W_SUCCESS if a path was found, W_FAILURE otherwise.
  static WResult BuildPath(WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat, bool bSourceEqualsTarget, WDynamicArray<ConversionPathNode>& out_path, WUInt32& out_uiNumScratchBuffers);

  ///  Converts the source image into a target image with the given format. Source and target may be the same.
  static WResult Convert(const WImageView& source, WImage& ref_target, WImageFormat::Enum targetFormat);

  /// Converts the source image into a target image using a precomputed conversion path.
  static WResult Convert(const WImageView& source, WImage& ref_target, WArrayPtr<ConversionPathNode> path, WUInt32 uiNumScratchBuffers);

  /// Converts the raw source data into a target data buffer with the given format. Source and target may be the same.
  static WResult ConvertRaw(
    WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumElements, WImageFormat::Enum sourceFormat, WImageFormat::Enum targetFormat);

  /// Converts the raw source data into a target data buffer using a precomputed conversion path.
  static WResult ConvertRaw(
    WConstByteBlobPtr source, WByteBlobPtr target, WUInt32 uiNumElements, WArrayPtr<ConversionPathNode> path, WUInt32 uiNumScratchBuffers);

private:
  WImageConversion();
  WImageConversion(const WImageConversion&);

  static WResult ConvertSingleStep(const WImageConversionStep* pStep, const WImageView& source, WImage& target, WImageFormat::Enum targetFormat);

  static WResult ConvertSingleStepDecompress(const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat, const WImageConversionStep* pStep);

  static WResult ConvertSingleStepCompress(const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat, const WImageConversionStep* pStep);

  static WResult ConvertSingleStepDeplanarize(const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat, const WImageConversionStep* pStep);

  static WResult ConvertSingleStepPlanarize(const WImageView& source, WImage& target, WImageFormat::Enum sourceFormat,
    WImageFormat::Enum targetFormat, const WImageConversionStep* pStep);

  static void RebuildConversionTable();
};
