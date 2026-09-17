#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Uuid.h>
#include <QColor>
#include <QDataStream>
#include <QMetaType>
#include <ToolsFoundation/ToolsFoundationDLL.h>

// Configure the DLL Import/Export Define
#if W_ENABLED(W_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_GUIFOUNDATION_LIB
#    define W_GUIFOUNDATION_DLL W_DECL_EXPORT
#  else
#    define W_GUIFOUNDATION_DLL W_DECL_IMPORT
#  endif
#else
#  define W_GUIFOUNDATION_DLL
#endif

class QWidget;
class QObject;
class QKeyEvent;


Q_DECLARE_METATYPE(WUuid);

/// Calls setUpdatesEnabled(false) on all given QObjects, and the reverse in the destructor. Can be nested.
class W_GUIFOUNDATION_DLL WQtScopedUpdatesDisabled
{
public:
  WQtScopedUpdatesDisabled(QWidget* pWidget1, QWidget* pWidget2 = nullptr, QWidget* pWidget3 = nullptr, QWidget* pWidget4 = nullptr,
    QWidget* pWidget5 = nullptr, QWidget* pWidget6 = nullptr);
  ~WQtScopedUpdatesDisabled();

private:
  QWidget* m_pWidgets[6];
};


/// Calls blockSignals(true) on all given QObjects, and the reverse in the destructor. Can be nested.
class W_GUIFOUNDATION_DLL WQtScopedBlockSignals
{
public:
  WQtScopedBlockSignals(QObject* pObject1, QObject* pObject2 = nullptr, QObject* pObject3 = nullptr, QObject* pObject4 = nullptr,
    QObject* pObject5 = nullptr, QObject* pObject6 = nullptr);
  ~WQtScopedBlockSignals();

private:
  QObject* m_pObjects[6];
};

W_ALWAYS_INLINE QColor WToQtColor(const WColorGammaUB& c)
{
  return QColor(c.r, c.g, c.b, c.a);
}

W_ALWAYS_INLINE WColorGammaUB qtToEzColor(const QColor& c)
{
  return WColorGammaUB(c.red(), c.green(), c.blue(), c.alpha());
}

W_ALWAYS_INLINE WString qtToEzString(const QString& sString)
{
  QByteArray data = sString.toUtf8();
  return WString(WStringView(data.data(), static_cast<WUInt32>(data.size())));
}

W_ALWAYS_INLINE QString WMakeQString(WStringView sString)
{
  return QString::fromUtf8(sString.GetStartPointer(), sString.GetElementCount());
}

template <typename T>
void operator>>(QDataStream& inout_stream, T*& rhs)
{
  void* p = nullptr;
  uint len = sizeof(void*);
  inout_stream.readRawData((char*)&p, len);
  rhs = (T*)p;
}


template <typename T>
void operator<<(QDataStream& inout_stream, T* rhs)
{
  inout_stream.writeRawData((const char*)&rhs, sizeof(void*));
}

template <typename T>
void operator>>(QDataStream& inout_stream, WDynamicArray<T>& rhs)
{
  WUInt32 uiIndices = 0;
  inout_stream >> uiIndices;
  rhs.Clear();
  rhs.Reserve(uiIndices);

  for (WUInt32 i = 0; i < uiIndices; ++i)
  {
    T obj = {};
    inout_stream >> obj;
    rhs.PushBack(obj);
  }
}

template <typename T>
void operator<<(QDataStream& inout_stream, WDynamicArray<T>& rhs)
{
  WUInt32 uiIndices = rhs.GetCount();
  inout_stream << uiIndices;

  for (WUInt32 i = 0; i < uiIndices; ++i)
  {
    inout_stream << rhs[i];
  }
}

namespace WQtUtils
{
  /// Uses keyboard layout independent scan-codes to check whether the key of the QKeyEvent represents the desired key.
  ///
  /// Use this when the position of the key on the keyboard is the desired aspect, not the actual character.
  /// For example for navigation (WSAD) in a viewport.
  ///
  /// Assumes the standard US keyboard layout for the reference keys.
  W_GUIFOUNDATION_DLL bool IsEquivalentQtKey(const QKeyEvent* e, Qt::Key reference);

} // namespace WQtUtils
