#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_ShaderTemplateDlg.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class WDocument;

class WQtShaderTemplateDlg : public WQtDialog, public Ui_ShaderTemplateDlg
{
  Q_OBJECT

public:
  WQtShaderTemplateDlg(QWidget* pParent, const WDocument* pDoc);

  WString m_sResult;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_Browse_clicked();
  void on_ShaderTemplate_currentIndexChanged(int idx);

private:
  struct Template
  {
    WString m_sName;
    WString m_sPath;
    WString m_sContent;
    WHybridArray<WString, 16> m_Vars;
  };

  WHybridArray<Template, 32> m_Templates;
};
