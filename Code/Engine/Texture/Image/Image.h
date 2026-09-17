#pragma once

#include <Foundation/Containers/Blob.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Logging/Log.h>

#include <Texture/Image/Formats/ImageFileFormat.h>
#include <Texture/Image/ImageHeader.h>

/// A lightweight view to image data without owning the memory.
///
/// WImageView provides read-only access to image data along with the metadata needed to interpret it.
/// It does not own the image data, so the underlying memory must remain valid for the lifetime of the view.
/// This class is ideal for passing image data around without unnecessary copying.
///
/// Use cases:
/// - Passing images to functions that only read data
/// - Creating temporary views to sub-regions of larger images
/// - Interfacing with external image processing libraries
/// - Converting between different image representations
class W_TEXTURE_DLL WImageView : protected WImageHeader
{
public:
  /// Constructs an empty image view.
  WImageView();

  /// Constructs an image view with the given header and image data.
  WImageView(const WImageHeader& header, WConstByteBlobPtr imageData);

  /// Resets to an empty state, releasing the reference to external data.
  void Clear();

  /// Returns false if the image view does not reference any data yet.
  bool IsValid() const;

  /// Resets the view to reference new external image data.
  ///
  /// Any previous data reference is released. The new data must remain valid
  /// for the lifetime of this view.
  void ResetAndViewExternalStorage(const WImageHeader& header, WConstByteBlobPtr imageData);

  /// Convenience function to save the image to the given file.
  WResult SaveTo(WStringView sFileName) const;

  /// Returns the header this image was constructed from.
  const WImageHeader& GetHeader() const;

  /// Returns a view to the entire data contained in this image.
  template <typename T>
  WBlobPtr<const T> GetBlobPtr() const;

  WConstByteBlobPtr GetByteBlobPtr() const;

  /// Returns a view to the given sub-image.
  WImageView GetSubImageView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0) const;

  /// Returns a view to a sub-plane.
  WImageView GetPlaneView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 uiPlaneIndex = 0) const;

  /// Returns a view to z slice of the image.
  WImageView GetSliceView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0) const;

  /// Returns a view to a row of pixels resp. blocks.
  WImageView GetRowView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 y = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0) const;

  /// Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  const T* GetPixelPointer(
    WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 x = 0, WUInt32 y = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0) const;

  /// Reinterprets the image with a given format; the format must have the same size in bits per pixel as the current one.
  void ReinterpretAs(WImageFormat::Enum format);

public:
  using WImageHeader::GetDepth;
  using WImageHeader::GetHeight;
  using WImageHeader::GetWidth;

  using WImageHeader::GetNumArrayIndices;
  using WImageHeader::GetNumFaces;
  using WImageHeader::GetNumMipLevels;
  using WImageHeader::GetPlaneCount;

  using WImageHeader::GetImageFormat;

  using WImageHeader::GetNumBlocksX;
  using WImageHeader::GetNumBlocksY;
  using WImageHeader::GetNumBlocksZ;

  using WImageHeader::GetDepthPitch;
  using WImageHeader::GetRowPitch;

protected:
  WUInt64 ComputeLayout();

  void ValidateSubImageIndices(WUInt32 uiMipLevel, WUInt32 uiFace, WUInt32 uiArrayIndex, WUInt32 uiPlaneIndex) const;
  template <typename T>
  void ValidateDataTypeAccessor(WUInt32 uiPlaneIndex) const;

  const WUInt64& GetSubImageOffset(WUInt32 uiMipLevel, WUInt32 uiFace, WUInt32 uiArrayIndex, WUInt32 uiPlaneIndex) const;

  WHybridArray<WUInt64, 16> m_SubImageOffsets;
  WBlobPtr<WUInt8> m_DataPtr;
};

/// Container for image data with automatic memory management.
///
/// WImage extends WImageView by owning the image data it references. It can use either internal storage
/// or attach to external memory. This class handles allocation, deallocation, and provides convenient
/// methods for loading, saving, and converting images.
///
/// Memory management:
/// - Internal storage: WImage allocates and manages its own memory
/// - External storage: WImage references user-provided memory (user manages lifetime)
/// - Storage can be switched between internal and external as needed
///
/// The sub-images are stored in a predefined order compatible with DDS files:
/// For each array slice: mip level 0, mip level 1, ..., mip level N
/// For cubemaps: +X, -X, +Y, -Y, +Z, -Z faces in that order
/// For texture arrays: array slice 0, array slice 1, ..., array slice N
///
/// Common usage patterns:
/// ```cpp
/// // Load from file
/// WImage image;
/// image.LoadFrom("texture.png");
///
/// // Create with specific format
/// WImageHeader header;
/// header.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);
/// header.SetWidth(256); header.SetHeight(256);
/// WImage image(header);
///
/// // Convert format
/// image.Convert(WImageFormat::BC1_UNORM);
/// ```
class W_TEXTURE_DLL WImage : public WImageView
{
  /// Use Reset() instead
  void operator=(const WImage& rhs) = delete;

  /// Use Reset() instead
  void operator=(const WImageView& rhs) = delete;

  /// Constructs an image with the given header; allocating internal storage for it.
  explicit WImage(const WImageHeader& header);

  /// Constructs an image with the given header backed by user-supplied external storage.
  explicit WImage(const WImageHeader& header, WByteBlobPtr externalData);

  /// Constructor from image view (copies the image data to internal storage)
  explicit WImage(const WImageView& other);

public:
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Constructs an empty image.
  WImage();

  /// Move constructor
  WImage(WImage&& other);

  void operator=(WImage&& rhs);

  /// Constructs an empty image. If the image is attached to an external storage, the attachment is discarded.
  void Clear();

  /// Allocates storage for an image with the given header.
  ///
  /// If currently using external storage and it's large enough, that storage will be reused.
  /// Otherwise, the image will detach from external storage and allocate internal storage.
  /// Any existing data is discarded.
  void ResetAndAlloc(const WImageHeader& header);

  /// Attaches the image to external storage provided by the user.
  ///
  /// The external storage must remain valid for the lifetime of this WImage.
  /// The storage must be large enough to hold the image data described by the header.
  /// Use this when you want to avoid memory allocation or work with memory-mapped files.
  void ResetAndUseExternalStorage(const WImageHeader& header, WByteBlobPtr externalData);

  /// Takes ownership of another image's data via move semantics.
  ///
  /// The other image is left in an empty state. If the other image uses external storage,
  /// this image will also reference that storage and inherit the lifetime requirements.
  void ResetAndMove(WImage&& other);

  /// Copies data from an image view into internal storage.
  ///
  /// If currently attached to external storage, the attachment is discarded and internal
  /// storage is allocated. The source view's data is copied completely.
  void ResetAndCopy(const WImageView& other);

  /// Convenience function to load the image from the given file.
  WResult LoadFrom(WStringView sFileName);

  /// Convenience function to convert the image to the given format.
  WResult Convert(WImageFormat::Enum targetFormat);

  /// Returns a view to the entire data contained in this image.
  template <typename T>
  WBlobPtr<T> GetBlobPtr();

  WByteBlobPtr GetByteBlobPtr();

  using WImageView::GetBlobPtr;
  using WImageView::GetByteBlobPtr;

  /// Returns a view to the given sub-image.
  WImage GetSubImageView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0);

  using WImageView::GetSubImageView;

  /// Returns a view to a sub-plane.
  WImage GetPlaneView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 uiPlaneIndex = 0);

  using WImageView::GetPlaneView;

  /// Returns a view to z slice of the image.
  WImage GetSliceView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0);

  using WImageView::GetSliceView;

  /// Returns a view to a row of pixels resp. blocks.
  WImage GetRowView(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 y = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0);

  using WImageView::GetRowView;

  /// Returns a pointer to a given pixel or block contained in a sub-image.
  template <typename T>
  T* GetPixelPointer(WUInt32 uiMipLevel = 0, WUInt32 uiFace = 0, WUInt32 uiArrayIndex = 0, WUInt32 x = 0, WUInt32 y = 0, WUInt32 z = 0, WUInt32 uiPlaneIndex = 0);

  using WImageView::GetPixelPointer;

private:
  bool UsesExternalStorage() const;

  WBlob m_InternalStorage;
};

#include <Texture/Image/Implementation/Image_inl.h>
