#include "global.h"

std::function<void(QWidget*)> repolish = [](QWidget * w)
{
   w->style()->unpolish(w);      // 去掉样式
   w->style()->polish(w);        // 刷新
};
