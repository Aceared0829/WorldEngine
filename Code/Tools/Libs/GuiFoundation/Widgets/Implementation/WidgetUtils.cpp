#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Declarations.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>
#include <QApplication>
#include <QRect>

QScreen& WWidgetUtils::GetClosestScreen(const QPoint& point)
{
  QScreen* pClosestScreen = QApplication::screenAt(point);
  if (pClosestScreen == nullptr)
  {
    QList<QScreen*> screens = QApplication::screens();
    float fShortestDistance = WMath::Infinity<float>();
    for (QScreen* pScreen : screens)
    {
      const QRect geom = pScreen->geometry();
      WBoundingBox WGeom = WBoundingBox::MakeFromCenterAndHalfExtents(WVec3(geom.center().x(), geom.center().y(), 0), WVec3(geom.width() / 2.0f, geom.height() / 2.0f, 0));
      const WVec3 WPoint(point.x(), point.y(), 0);
      if (WGeom.Contains(WPoint))
      {
        return *pScreen;
      }
      float fDistance = WGeom.GetDistanceSquaredTo(WPoint);
      if (fDistance < fShortestDistance)
      {
        fShortestDistance = fDistance;
        pClosestScreen = pScreen;
      }
    }
    W_ASSERT_DEV(pClosestScreen != nullptr, "There are no screens connected, UI cannot function.");
  }
  return *pClosestScreen;
}

void WWidgetUtils::AdjustGridDensity(
  double& ref_fFinestDensity, double& ref_fRoughDensity, WUInt32 uiWindowWidth, double fViewportSceneWidth, WUInt32 uiMinPixelsForStep)
{
  const double fMaxStepsFitInWindow = (double)uiWindowWidth / (double)uiMinPixelsForStep;

  const double fStartDensity = ref_fFinestDensity;

  WInt32 iFactor = 1;
  double fNewDensity = ref_fFinestDensity;
  WInt32 iFactors[2] = {5, 2};
  WInt32 iLastFactor = 0;

  while (true)
  {
    const double fStepsAtDensity = fViewportSceneWidth / fNewDensity;

    if (fStepsAtDensity < fMaxStepsFitInWindow)
      break;

    iFactor *= iFactors[iLastFactor];
    fNewDensity = fStartDensity * iFactor;

    iLastFactor = (iLastFactor + 1) % 2;
  }

  ref_fFinestDensity = fStartDensity * iFactor;

  iFactor *= iFactors[iLastFactor];
  ref_fRoughDensity = fStartDensity * iFactor;
}

void WWidgetUtils::ComputeGridExtentsX(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinX, double& out_fMaxX)
{
  out_fMinX = WMath::RoundDown((double)viewportSceneRect.left(), fGridStops);
  out_fMaxX = WMath::RoundUp((double)viewportSceneRect.right(), fGridStops);
}

void WWidgetUtils::ComputeGridExtentsY(const QRectF& viewportSceneRect, double fGridStops, double& out_fMinY, double& out_fMaxY)
{
  out_fMinY = WMath::RoundDown((double)viewportSceneRect.top(), fGridStops);
  out_fMaxY = WMath::RoundUp((double)viewportSceneRect.bottom(), fGridStops);
}
