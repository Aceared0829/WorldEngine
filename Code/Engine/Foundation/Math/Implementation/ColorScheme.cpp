#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>

WColor WColorScheme::s_Colors[Count][10] = {
  {
    WColorGammaUB(201, 42, 42),   // oc-red-9
    WColorGammaUB(224, 49, 49),   // oc-red-8
    WColorGammaUB(240, 62, 62),   // oc-red-7
    WColorGammaUB(250, 82, 82),   // oc-red-6
    WColorGammaUB(255, 107, 107), // oc-red-5
    WColorGammaUB(255, 135, 135), // oc-red-4
    WColorGammaUB(255, 168, 168), // oc-red-3
    WColorGammaUB(255, 201, 201), // oc-red-2
    WColorGammaUB(255, 227, 227), // oc-red-1
    WColorGammaUB(255, 245, 245), // oc-red-0
  },
  {
    WColorGammaUB(166, 30, 77),   // oc-pink-9
    WColorGammaUB(194, 37, 92),   // oc-pink-8
    WColorGammaUB(214, 51, 108),  // oc-pink-7
    WColorGammaUB(230, 73, 128),  // oc-pink-6
    WColorGammaUB(240, 101, 149), // oc-pink-5
    WColorGammaUB(247, 131, 172), // oc-pink-4
    WColorGammaUB(250, 162, 193), // oc-pink-3
    WColorGammaUB(252, 194, 215), // oc-pink-2
    WColorGammaUB(255, 222, 235), // oc-pink-1
    WColorGammaUB(255, 240, 246), // oc-pink-0
  },
  {
    WColorGammaUB(134, 46, 156),  // oc-grape-9
    WColorGammaUB(156, 54, 181),  // oc-grape-8
    WColorGammaUB(174, 62, 201),  // oc-grape-7
    WColorGammaUB(190, 75, 219),  // oc-grape-6
    WColorGammaUB(204, 93, 232),  // oc-grape-5
    WColorGammaUB(218, 119, 242), // oc-grape-4
    WColorGammaUB(229, 153, 247), // oc-grape-3
    WColorGammaUB(238, 190, 250), // oc-grape-2
    WColorGammaUB(243, 217, 250), // oc-grape-1
    WColorGammaUB(248, 240, 252), // oc-grape-0
  },
  {
    WColorGammaUB(95, 61, 196),   // oc-violet-9
    WColorGammaUB(103, 65, 217),  // oc-violet-8
    WColorGammaUB(112, 72, 232),  // oc-violet-7
    WColorGammaUB(121, 80, 242),  // oc-violet-6
    WColorGammaUB(132, 94, 247),  // oc-violet-5
    WColorGammaUB(151, 117, 250), // oc-violet-4
    WColorGammaUB(177, 151, 252), // oc-violet-3
    WColorGammaUB(208, 191, 255), // oc-violet-2
    WColorGammaUB(229, 219, 255), // oc-violet-1
    WColorGammaUB(243, 240, 255), // oc-violet-0
  },
  {
    WColorGammaUB(54, 79, 199),   // oc-indigo-9
    WColorGammaUB(59, 91, 219),   // oc-indigo-8
    WColorGammaUB(66, 99, 235),   // oc-indigo-7
    WColorGammaUB(76, 110, 245),  // oc-indigo-6
    WColorGammaUB(92, 124, 250),  // oc-indigo-5
    WColorGammaUB(116, 143, 252), // oc-indigo-4
    WColorGammaUB(145, 167, 255), // oc-indigo-3
    WColorGammaUB(186, 200, 255), // oc-indigo-2
    WColorGammaUB(219, 228, 255), // oc-indigo-1
    WColorGammaUB(237, 242, 255), // oc-indigo-0
  },
  {
    WColorGammaUB(24, 100, 171),  // oc-blue-9
    WColorGammaUB(25, 113, 194),  // oc-blue-8
    WColorGammaUB(28, 126, 214),  // oc-blue-7
    WColorGammaUB(34, 139, 230),  // oc-blue-6
    WColorGammaUB(51, 154, 240),  // oc-blue-5
    WColorGammaUB(77, 171, 247),  // oc-blue-4
    WColorGammaUB(116, 192, 252), // oc-blue-3
    WColorGammaUB(165, 216, 255), // oc-blue-2
    WColorGammaUB(208, 235, 255), // oc-blue-1
    WColorGammaUB(231, 245, 255), // oc-blue-0
  },
  {
    WColorGammaUB(11, 114, 133),  // oc-cyan-9
    WColorGammaUB(12, 133, 153),  // oc-cyan-8
    WColorGammaUB(16, 152, 173),  // oc-cyan-7
    WColorGammaUB(21, 170, 191),  // oc-cyan-6
    WColorGammaUB(34, 184, 207),  // oc-cyan-5
    WColorGammaUB(59, 201, 219),  // oc-cyan-4
    WColorGammaUB(102, 217, 232), // oc-cyan-3
    WColorGammaUB(153, 233, 242), // oc-cyan-2
    WColorGammaUB(197, 246, 250), // oc-cyan-1
    WColorGammaUB(227, 250, 252), // oc-cyan-0
  },
  {
    WColorGammaUB(8, 127, 91),    // oc-teal-9
    WColorGammaUB(9, 146, 104),   // oc-teal-8
    WColorGammaUB(12, 166, 120),  // oc-teal-7
    WColorGammaUB(18, 184, 134),  // oc-teal-6
    WColorGammaUB(32, 201, 151),  // oc-teal-5
    WColorGammaUB(56, 217, 169),  // oc-teal-4
    WColorGammaUB(99, 230, 190),  // oc-teal-3
    WColorGammaUB(150, 242, 215), // oc-teal-2
    WColorGammaUB(195, 250, 232), // oc-teal-1
    WColorGammaUB(230, 252, 245), // oc-teal-0
  },
  {
    WColorGammaUB(43, 138, 62),   // oc-green-9
    WColorGammaUB(47, 158, 68),   // oc-green-8
    WColorGammaUB(55, 178, 77),   // oc-green-7
    WColorGammaUB(64, 192, 87),   // oc-green-6
    WColorGammaUB(81, 207, 102),  // oc-green-5
    WColorGammaUB(105, 219, 124), // oc-green-4
    WColorGammaUB(140, 233, 154), // oc-green-3
    WColorGammaUB(178, 242, 187), // oc-green-2
    WColorGammaUB(211, 249, 216), // oc-green-1
    WColorGammaUB(235, 251, 238), // oc-green-0
  },
  {
    WColorGammaUB(92, 148, 13),   // oc-lime-9
    WColorGammaUB(102, 168, 15),  // oc-lime-8
    WColorGammaUB(116, 184, 22),  // oc-lime-7
    WColorGammaUB(130, 201, 30),  // oc-lime-6
    WColorGammaUB(148, 216, 45),  // oc-lime-5
    WColorGammaUB(169, 227, 75),  // oc-lime-4
    WColorGammaUB(192, 235, 117), // oc-lime-3
    WColorGammaUB(216, 245, 162), // oc-lime-2
    WColorGammaUB(233, 250, 200), // oc-lime-1
    WColorGammaUB(244, 252, 227), // oc-lime-0
  },
  {
    WColorGammaUB(230, 119, 0),   // oc-yellow-9
    WColorGammaUB(240, 140, 0),   // oc-yellow-8
    WColorGammaUB(245, 159, 0),   // oc-yellow-7
    WColorGammaUB(250, 176, 5),   // oc-yellow-6
    WColorGammaUB(252, 196, 25),  // oc-yellow-5
    WColorGammaUB(255, 212, 59),  // oc-yellow-4
    WColorGammaUB(255, 224, 102), // oc-yellow-3
    WColorGammaUB(255, 236, 153), // oc-yellow-2
    WColorGammaUB(255, 243, 191), // oc-yellow-1
    WColorGammaUB(255, 249, 219), // oc-yellow-0
  },
  {
    WColorGammaUB(217, 72, 15),   // oc-orange-9
    WColorGammaUB(232, 89, 12),   // oc-orange-8
    WColorGammaUB(247, 103, 7),   // oc-orange-7
    WColorGammaUB(253, 126, 20),  // oc-orange-6
    WColorGammaUB(255, 146, 43),  // oc-orange-5
    WColorGammaUB(255, 169, 77),  // oc-orange-4
    WColorGammaUB(255, 192, 120), // oc-orange-3
    WColorGammaUB(255, 216, 168), // oc-orange-2
    WColorGammaUB(255, 232, 204), // oc-orange-1
    WColorGammaUB(255, 244, 230), // oc-orange-0
  },
  {
    WColorGammaUB(33, 37, 41),    // oc-gray-9
    WColorGammaUB(52, 58, 64),    // oc-gray-8
    WColorGammaUB(73, 80, 87),    // oc-gray-7
    WColorGammaUB(134, 142, 150), // oc-gray-6
    WColorGammaUB(173, 181, 189), // oc-gray-5
    WColorGammaUB(206, 212, 218), // oc-gray-4
    WColorGammaUB(222, 226, 230), // oc-gray-3
    WColorGammaUB(233, 236, 239), // oc-gray-2
    WColorGammaUB(241, 243, 245), // oc-gray-1
    WColorGammaUB(248, 249, 250), // oc-gray-0
  },
};

// We could use a lower brightness here for our dark UI but the colors looks much nicer at higher brightness so we just apply a scale factor instead.
static constexpr WUInt8 DarkUIBrightness = 3;
static constexpr WUInt8 DarkUIGrayBrightness = 4; // gray is too dark at UIBrightness
static constexpr float DarkUISaturation = 0.95f;
static constexpr WColor DarkUIFactor = WColor(0.5f, 0.5f, 0.5f, 1.0f);
WColor WColorScheme::s_DarkUIColors[Count] = {
  GetColor(WColorScheme::Red, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Pink, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Grape, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Violet, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Indigo, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Blue, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Cyan, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Teal, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Green, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Lime, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Yellow, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Orange, DarkUIBrightness, DarkUISaturation) * DarkUIFactor,
  GetColor(WColorScheme::Gray, DarkUIGrayBrightness, DarkUISaturation) * DarkUIFactor,
};

static constexpr WUInt8 LightUIBrightness = 4;
static constexpr WUInt8 LightUIGrayBrightness = 5; // gray is too dark at UIBrightness
static constexpr float LightUISaturation = 1.0f;
WColor WColorScheme::s_LightUIColors[Count] = {
  GetColor(WColorScheme::Red, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Pink, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Grape, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Violet, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Indigo, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Blue, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Cyan, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Teal, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Green, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Lime, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Yellow, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Orange, LightUIBrightness, LightUISaturation),
  GetColor(WColorScheme::Gray, LightUIGrayBrightness, LightUISaturation),
};

// static
WColor WColorScheme::GetColor(float fIndex, WUInt8 uiBrightness, float fSaturation /*= 1.0f*/, float fAlpha /*= 1.0f*/)
{
  WUInt32 uiIndexA, uiIndexB;
  float fFrac;
  GetInterpolation(fIndex, uiIndexA, uiIndexB, fFrac);

  const WColor a = s_Colors[uiIndexA][uiBrightness];
  const WColor b = s_Colors[uiIndexB][uiBrightness];
  const WColor c = WMath::Lerp(a, b, fFrac);
  const float l = c.GetLuminance();
  return WMath::Lerp(WColor(l, l, l), c, fSaturation).WithAlpha(fAlpha);
}

WColorScheme::CategoryColorFunc WColorScheme::s_CategoryColorFunc = nullptr;

WColor WColorScheme::GetCategoryColor(WStringView sCategory, CategoryColorUsage usage)
{
  if (s_CategoryColorFunc != nullptr)
  {
    const WColor ret = s_CategoryColorFunc(sCategory, usage);

    // if valid, use this color
    if (ret != WColor::MakeZero())
      return ret;
  }

  WInt8 iBrightnessOffset = -3;
  WUInt8 uiSaturationStep = 0;

  if (usage == WColorScheme::CategoryColorUsage::BorderIconColor)
  {
    // don't color these icons at all
    return WColor::MakeZero();
  }

  if (usage == WColorScheme::CategoryColorUsage::MenuEntryIcon || usage == WColorScheme::CategoryColorUsage::AssetMenuIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == WColorScheme::CategoryColorUsage::ViewportIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 2;
  }
  else if (usage == WColorScheme::CategoryColorUsage::OverlayIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == WColorScheme::CategoryColorUsage::SceneTreeIcon)
  {
    iBrightnessOffset = 2;
    uiSaturationStep = 0;
  }
  else if (usage == WColorScheme::CategoryColorUsage::BorderColor)
  {
    iBrightnessOffset = -3;
    uiSaturationStep = 0;
  }

  const WUInt8 uiBrightness = (WUInt8)WMath::Clamp<WInt32>(DarkUIBrightness + iBrightnessOffset, 0, 9);
  const float fSaturation = DarkUISaturation - (uiSaturationStep * 0.2f);

  if (const char* sep = sCategory.FindSubString("/"))
  {
    // chop off everything behind the first separator
    sCategory = WStringView(sCategory.GetStartPointer(), sep);
  }

  if (sCategory.IsEqual_NoCase("AI"))
    return WColorScheme::GetColor(WColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Animation"))
    return WColorScheme::GetColor(WColorScheme::Pink, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Construction"))
    return WColorScheme::GetColor(WColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Custom") || sCategory.IsEqual_NoCase("Game"))
    return WColorScheme::GetColor(WColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Effects"))
    return WColorScheme::GetColor(WColorScheme::Grape, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Gameplay"))
    return WColorScheme::GetColor(WColorScheme::Indigo, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Input"))
    return WColorScheme::GetColor(WColorScheme::Red, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Lighting"))
    return WColorScheme::GetColor(WColorScheme::Violet, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Logic"))
    return WColorScheme::GetColor(WColorScheme::Teal, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Physics"))
    return WColorScheme::GetColor(WColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Prefabs"))
    return WColorScheme::GetColor(WColorScheme::Orange, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Rendering"))
    return WColorScheme::GetColor(WColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Terrain"))
    return WColorScheme::GetColor(WColorScheme::Lime, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Scripting"))
    return WColorScheme::GetColor(WColorScheme::Green, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Sound"))
    return WColorScheme::GetColor(WColorScheme::Blue, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("Utilities") || sCategory.IsEqual_NoCase("Editing"))
    return WColorScheme::GetColor(WColorScheme::Gray, uiBrightness, fSaturation) * DarkUIFactor;

  if (sCategory.IsEqual_NoCase("XR"))
    return WColorScheme::GetColor(WColorScheme::Cyan, uiBrightness, fSaturation) * DarkUIFactor;

  WLog::Warning("Color for category '{}' is undefined.", sCategory);
  return WColor::MakeZero();
}
