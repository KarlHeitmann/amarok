/*
    Copyright (c) 2008 Dan Meltzer <hydrogen@notyetimplemented.com>
              (c) 2008 Seb Ruiz <ruiz@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MYDIROPERATOR_H
#define MYDIROPERATOR_H

#include "MyDirLister.h"

#include "collection/Collection.h"
#include "playlist/PlaylistController.h"
#include "PopupDropperFactory.h"
#include "context/popupdropper/PopupDropper.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "context/popupdropper/PopupDropperItem.h"
#include "SvgHandler.h"


#include <KDirOperator>
#include <KFileItem>
#include <KMenu>
#include <KUrl>

typedef QList<PopupDropperAction *> PopupDropperActionList;

/**
 * Stores a collection associated with an action for move/copy to collection
 */
class CollectionAction : public QAction
{
public:
    CollectionAction( Collection *coll, QObject *parent = 0 )
            : QAction( parent )
            , m_collection( coll )
    {
        setText( m_collection->prettyName() );
    }

    Collection *collection() const
    {
        return m_collection;
    }

private:
    Collection *m_collection;
};

class MyDirOperator : public KDirOperator
{
    Q_OBJECT

public:
    MyDirOperator( const KUrl &url, QWidget *parent );
    ~MyDirOperator();

private slots:
    void aboutToShowContextMenu();
    void fileSelected( const KFileItem & /*file*/ );

    void slotPrepareMoveTracks();
    void slotPrepareCopyTracks();
    void slotMoveTracks( const Meta::TrackList& tracks );
    void slotCopyTracks( const Meta::TrackList& tracks );
    void slotPlayChildTracks();
    void slotAppendChildTracks();

private:
    PopupDropperActionList createBasicActions();
    void playChildTracks( const KFileItemList &items, Playlist::AddOptions insertMode );

    bool mCopyActivated;
    bool mMoveActivated;
    CollectionAction* mCopyAction;
    CollectionAction* mMoveAction;
};

#endif

