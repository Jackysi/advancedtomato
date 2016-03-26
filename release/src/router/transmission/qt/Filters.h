/*
 * This file Copyright (C) 2010-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: Filters.h 14539 2015-06-12 22:12:12Z mikedld $
 */

#ifndef QTR_FILTERS_H
#define QTR_FILTERS_H

#include <QMetaType>
#include <QString>
#include <QVariant>

class FilterMode
{
  public:
    enum
    {
      SHOW_ALL,
      SHOW_ACTIVE,
      SHOW_DOWNLOADING,
      SHOW_SEEDING,
      SHOW_PAUSED,
      SHOW_FINISHED,
      SHOW_VERIFYING,
      SHOW_ERROR,
      NUM_MODES
    };

  public:
    FilterMode (int mode = SHOW_ALL): myMode (mode) {}
    FilterMode (const QString& name): myMode (modeFromName (name)) {}

    int mode () const { return myMode; }
    const QString& name () const { return names[myMode]; }

    static int modeFromName (const QString& name);
    static const QString& nameFromMode(int mode) { return names[mode]; }

  private:
    int myMode;

    static const QString names[];
};

Q_DECLARE_METATYPE(FilterMode)

class SortMode
{
  public:
    enum
    {
      SORT_BY_ACTIVITY,
      SORT_BY_AGE,
      SORT_BY_ETA,
      SORT_BY_NAME,
      SORT_BY_PROGRESS,
      SORT_BY_QUEUE,
      SORT_BY_RATIO,
      SORT_BY_SIZE,
      SORT_BY_STATE,
      SORT_BY_ID,
      NUM_MODES
    };

  public:
    SortMode (int mode = SORT_BY_ID): myMode (mode) {}
    SortMode (const QString& name): myMode (modeFromName (name)) {}

    int mode () const { return myMode; }
    const QString& name () const { return names[myMode]; }

    static int modeFromName (const QString& name);
    static const QString& nameFromMode (int mode);

  private:
    int myMode;

    static const QString names[];
};

Q_DECLARE_METATYPE(SortMode)

#endif // QTR_FILTERS_H
