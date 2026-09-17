#include <RendererDX11/RendererDX11PCH.h>

W_STATICLINK_LIBRARY(RendererDX11)
{
  if (bReturn)
    return;

  W_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_DeviceDX11);
}
