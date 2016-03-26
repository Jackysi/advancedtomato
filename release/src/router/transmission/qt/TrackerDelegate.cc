/*
 * This file Copyright (C) 2009-2015 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: TrackerDelegate.cc 14633 2015-12-25 10:19:50Z mikedld $
 */

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QTextDocument>

#include <libtransmission/transmission.h>
#include <libtransmission/utils.h>

#include "FaviconCache.h"
#include "Formatter.h"
#include "Torrent.h"
#include "TrackerDelegate.h"
#include "TrackerModel.h"
#include "Utils.h"

/***
****
***/

namespace
{
  const int mySpacing = 6;
  const QSize myMargin (10, 10);

  class ItemLayout
  {
    private:
      QTextDocument myTextDocument;

    public:
      QRect iconRect;
      QRect textRect;

    public:
      ItemLayout(const QString& text, bool suppressColors, Qt::LayoutDirection direction,
                 const QPoint& topLeft, int width);

      QSize size () const
      {
        return (iconRect | textRect).size ();
      }

      QAbstractTextDocumentLayout * textLayout () const
      {
        return myTextDocument.documentLayout ();
      }
  };

  ItemLayout::ItemLayout(const QString& text, bool suppressColors, Qt::LayoutDirection direction,
                         const QPoint& topLeft, int width)
  {
    const QStyle * style (qApp->style ());
    const QSize iconSize = FaviconCache::getIconSize ();

    QRect baseRect (topLeft, QSize (width, 0));

    iconRect = style->alignedRect (direction, Qt::AlignLeft | Qt::AlignTop, iconSize, baseRect);
    Utils::narrowRect (baseRect, iconSize.width () + mySpacing, 0, direction);

    myTextDocument.setDocumentMargin (0);
    myTextDocument.setTextWidth (baseRect.width ());
    QTextOption textOption;
    textOption.setTextDirection (direction);
    if (suppressColors)
      textOption.setFlags (QTextOption::SuppressColors);
    myTextDocument.setDefaultTextOption (textOption);
    myTextDocument.setHtml (text);

    textRect = baseRect;
    textRect.setSize (myTextDocument.size ().toSize ());
  }
}

QSize
TrackerDelegate::margin (const QStyle& style) const
{
  Q_UNUSED (style);

  return myMargin;
}

/***
****
***/

QSize
TrackerDelegate::sizeHint (const QStyleOptionViewItem & option,
                           const TrackerInfo          & info) const
{
  const ItemLayout layout (getText (info), true, option.direction, QPoint (0, 0), option.rect.width () - myMargin.width () * 2);
  return layout.size () + myMargin * 2;
}

QSize
TrackerDelegate::sizeHint (const QStyleOptionViewItem  & option,
                           const QModelIndex           & index) const
{
  const TrackerInfo trackerInfo = index.data (TrackerModel::TrackerRole).value<TrackerInfo>();
  return sizeHint (option, trackerInfo);
}

void
TrackerDelegate::paint (QPainter                    * painter,
                        const QStyleOptionViewItem  & option,
                        const QModelIndex           & index) const
{
  const TrackerInfo trackerInfo = index.data (TrackerModel::TrackerRole).value<TrackerInfo>();
  painter->save();
  painter->setClipRect (option.rect);
  drawBackground (painter, option, index);
  drawTracker (painter, option, trackerInfo);
  drawFocus(painter, option, option.rect);
  painter->restore();
}

void
TrackerDelegate::drawTracker (QPainter                    * painter,
                              const QStyleOptionViewItem  & option,
                              const TrackerInfo           & inf) const
{
  const bool isItemSelected ((option.state & QStyle::State_Selected) != 0);
  const bool isItemEnabled ((option.state & QStyle::State_Enabled) != 0);
  const bool isItemActive ((option.state & QStyle::State_Active) != 0);

  QIcon trackerIcon (inf.st.getFavicon());

  const QRect contentRect (option.rect.adjusted (myMargin.width (), myMargin.height (), -myMargin.width (), -myMargin.height ()));
  const ItemLayout layout (getText (inf), isItemSelected, option.direction, contentRect.topLeft (), contentRect.width ());

  painter->save();

  if (isItemSelected)
    {
      QPalette::ColorGroup cg = isItemEnabled ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !isItemActive)
        cg = QPalette::Inactive;

      painter->fillRect (option.rect, option.palette.brush (cg, QPalette::Highlight));
    }

  trackerIcon.paint (painter, layout.iconRect, Qt::AlignCenter, isItemSelected ? QIcon::Selected : QIcon::Normal, QIcon::On);

  QAbstractTextDocumentLayout::PaintContext paintContext;
  paintContext.clip = layout.textRect.translated (-layout.textRect.topLeft ());
  paintContext.palette.setColor (QPalette::Text, option.palette.color (isItemSelected ? QPalette::HighlightedText : QPalette::Text));
  painter->translate (layout.textRect.topLeft());
  layout.textLayout ()->draw (painter, paintContext);

  painter->restore();
}

void
TrackerDelegate::setShowMore (bool b)
{
  myShowMore = b;
}

namespace
{
  QString timeToStringRounded (int seconds)
    {
      if (seconds > 60)
        seconds -=  (seconds % 60);

      return Formatter::timeToString  (seconds);
    }
}

QString
TrackerDelegate::getText (const TrackerInfo& inf) const
{
  QString key;
  QString str;
  const time_t now (time (0));
  const QString err_markup_begin = QLatin1String ("<span style=\"color:red\">");
  const QString err_markup_end = QLatin1String ("</span>");
  const QString timeout_markup_begin = QLatin1String ("<span style=\"color:#224466\">");
  const QString timeout_markup_end = QLatin1String ("</span>");
  const QString success_markup_begin = QLatin1String ("<span style=\"color:#008B00\">");
  const QString success_markup_end = QLatin1String ("</span>");

  // hostname
  str += inf.st.isBackup ? QLatin1String ("<i>") : QLatin1String ("<b>");
  char * host = NULL;
  int port = 0;
  tr_urlParse (inf.st.announce.toUtf8().constData(), TR_BAD_SIZE, NULL, &host, &port, NULL);
  str += QString::fromLatin1 ("%1:%2").arg (QString::fromUtf8 (host)).arg (port);
  tr_free (host);
  if (!key.isEmpty()) str += QLatin1String (" - ") + key;
  str += inf.st.isBackup ? QLatin1String ("</i>") : QLatin1String ("</b>");

  // announce & scrape info
  if (!inf.st.isBackup)
    {
      if (inf.st.hasAnnounced && inf.st.announceState != TR_TRACKER_INACTIVE)
        {
          const QString tstr (timeToStringRounded (now - inf.st.lastAnnounceTime));
          str += QLatin1String ("<br/>\n");
          if (inf.st.lastAnnounceSucceeded)
            {
              //: %1 and %2 are replaced with HTML markup, %3 is duration
              str += tr ("Got a list of%1 %Ln peer(s)%2 %3 ago", 0, inf.st.lastAnnouncePeerCount)
                     .arg (success_markup_begin)
                     .arg (success_markup_end)
                     .arg (tstr);
            }
          else if (inf.st.lastAnnounceTimedOut)
            {
              //: %1 and %2 are replaced with HTML markup, %3 is duration
              str += tr ("Peer list request %1timed out%2 %3 ago; will retry")
                     .arg (timeout_markup_begin)
                     .arg (timeout_markup_end)
                     .arg (tstr);
            }
          else
            {
              //: %1 and %3 are replaced with HTML markup, %2 is error message, %4 is duration
              str += tr ("Got an error %1\"%2\"%3 %4 ago")
                     .arg (err_markup_begin)
                     .arg (inf.st.lastAnnounceResult)
                     .arg (err_markup_end)
                     .arg (tstr);
            }
        }

        switch (inf.st.announceState)
          {
            case TR_TRACKER_INACTIVE:
              str += QLatin1String ("<br/>\n");
              str += tr ("No updates scheduled");
              break;

            case TR_TRACKER_WAITING:
              {
                const QString tstr (timeToStringRounded (inf.st.nextAnnounceTime - now));
                str += QLatin1String ("<br/>\n");
                //: %1 is duration
                str += tr ("Asking for more peers in %1").arg (tstr);
                break;
              }

            case TR_TRACKER_QUEUED:
              str += QLatin1String ("<br/>\n");
              str += tr ("Queued to ask for more peers");
              break;

            case TR_TRACKER_ACTIVE: {
              const QString tstr (timeToStringRounded (now - inf.st.lastAnnounceStartTime));
              str += QLatin1String ("<br/>\n");
              //: %1 is duration
              str += tr ("Asking for more peers now... <small>%1</small>").arg (tstr);
              break;
            }
        }

      if (myShowMore)
        {
          if (inf.st.hasScraped)
            {
              str += QLatin1String ("<br/>\n");
              const QString tstr (timeToStringRounded (now - inf.st.lastScrapeTime));
              if (inf.st.lastScrapeSucceeded)
                {
                  if (inf.st.seederCount >= 0 && inf.st.leecherCount >= 0)
                    {
                      //: First part of phrase "Tracker had ... seeder(s) and ... leecher(s) ... ago";
                      //: %1 and %2 are replaced with HTML markup
                      str += tr ("Tracker had%1 %Ln seeder(s)%2", 0, inf.st.seederCount)
                             .arg (success_markup_begin)
                             .arg (success_markup_end);
                      //: Second part of phrase "Tracker had ... seeder(s) and ... leecher(s) ... ago";
                      //: %1 and %2 are replaced with HTML markup, %3 is duration;
                      //: notice that leading space (before "and") is included here
                      str += tr (" and%1 %Ln leecher(s)%2 %3 ago", 0, inf.st.leecherCount)
                             .arg (success_markup_begin)
                             .arg (success_markup_end)
                             .arg (tstr);
                    }
                  else
                    {
                      //: %1 and %2 are replaced with HTML markup, %3 is duration
                      str += tr ("Tracker had %1no information%2 on peer counts %3 ago")
                             .arg (success_markup_begin)
                             .arg (success_markup_end)
                             .arg (tstr);
                    }
                }
              else
                {
                  //: %1 and %3 are replaced with HTML markup, %2 is error message, %4 is duration
                  str += tr ("Got a scrape error %1\"%2\"%3 %4 ago")
                         .arg (err_markup_begin)
                         .arg (inf.st.lastScrapeResult)
                         .arg (err_markup_end)
                         .arg (tstr);
                }
            }

          switch (inf.st.scrapeState)
            {
              case TR_TRACKER_INACTIVE:
                break;

              case TR_TRACKER_WAITING:
                {
                  str += QLatin1String ("<br/>\n");
                  const QString tstr (timeToStringRounded (inf.st.nextScrapeTime - now));
                  //: %1 is duration
                  str += tr ("Asking for peer counts in %1").arg (tstr);
                  break;
                }

              case TR_TRACKER_QUEUED:
                {
                  str += QLatin1String ("<br/>\n");
                  str += tr ("Queued to ask for peer counts");
                  break;
                }

              case TR_TRACKER_ACTIVE:
                {
                  str += QLatin1String ("<br/>\n");
                  const QString tstr (timeToStringRounded (now - inf.st.lastScrapeStartTime));
                  //: %1 is duration
                  str += tr ("Asking for peer counts now... <small>%1</small>").arg (tstr);
                  break;
                }
            }
        }
    }

  return str;
}
