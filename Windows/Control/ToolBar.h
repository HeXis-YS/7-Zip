// Windows/Control/ToolBar.h
  
#pragma once

#ifndef __WINDOWS_CONTROL_TOOLBAR_H
#define __WINDOWS_CONTROL_TOOLBAR_H

#include "Windows/Window.h"
#include "Windows/Defs.h"

namespace NWindows {
namespace NControl {

class CToolBar: public NWindows::CWindow
{
public:
  bool GetMaxSize(LPSIZE size)
    { return BOOLToBool(SendMessage(TB_GETMAXSIZE, 0, (LPARAM)size)); }
  bool EnableButton(UINT buttonID, bool enable)
    { return BOOLToBool(SendMessage(TB_ENABLEBUTTON, buttonID, 
          MAKELONG(BoolToBOOL(enable), 0))); }
  void ButtonStructSize()
    { SendMessage(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON)); }
  HIMAGELIST SetImageList(UINT listIndex, HIMAGELIST imageList)
    { return HIMAGELIST(SendMessage(TB_SETIMAGELIST, listIndex, (LPARAM)imageList)); }
  bool AddButton(UINT numButtons, LPTBBUTTON buttons)
    { return BOOLToBool(SendMessage(TB_ADDBUTTONS, numButtons, (LPARAM)buttons)); }
};

}}

#endif