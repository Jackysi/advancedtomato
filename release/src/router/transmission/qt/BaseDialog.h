/*
 * This file Copyright (C) 2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: BaseDialog.h 14561 2015-09-01 20:19:26Z mikedld $
 */

#ifndef QTR_BASE_DIALOG_H
#define QTR_BASE_DIALOG_H

#include <QDialog>

class BaseDialog: public QDialog
{
  public:
    BaseDialog (QWidget * parent = nullptr, Qt::WindowFlags flags = 0):
      QDialog (parent, flags)
    {
      setWindowFlags (windowFlags () & ~Qt::WindowContextHelpButtonHint);
    }
};

#endif // QTR_BASE_DIALOG_H
