#include <Texture/TexturePCH.h>

W_STATICLINK_LIBRARY(Texture)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(Texture_Image_Conversions_BC7EncConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Conversions_DXTexCpuConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Conversions_PixelConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Conversions_PlanarConversions);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_BmpFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_DdsFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_ExrFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_StbImageFileFormats);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_SvgFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_TgaFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Formats_WicFileFormat);
  W_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageEnums);
  W_STATICLINK_REFERENCE(Texture_Image_Implementation_ImageFormat);
  W_STATICLINK_REFERENCE(Texture_TexConv_Implementation_Processor);
}
