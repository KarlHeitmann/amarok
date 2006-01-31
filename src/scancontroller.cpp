/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "ScanController"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundle.h"
#include "playlistbrowser.h"
#include "scancontroller.h"
#include "statusbar.h"

#include <qdeepcopy.h>
#include <qfileinfo.h>
#include <qtextcodec.h>

#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocio.h>


////////////////////////////////////////////////////////////////////////////////
// class ScannerProcIO
////////////////////////////////////////////////////////////////////////////////
/**
 * Due to xine-lib, we have to make KProcess close all fds, otherwise we get "device is busy" messages
 * Used by AmaroKProcIO and AmaroKProcess, exploiting commSetupDoneC(), a virtual method that
 * happens to be called in the forked process
 * See bug #103750 for more information.
 */
class ScannerProcIO : public KProcIO {
    public:
    ScannerProcIO();
    virtual int commSetupDoneC() {
        const int i = KProcIO::commSetupDoneC();
        amaroK::closeOpenFiles( KProcIO::out[0],KProcIO::in[0],KProcIO::err[0] );
        return i;
    };
};

/**
 * This constructor is needed so that the correct codec is used. KProcIO defaults
 * to latin1, while the scanner uses UTF-8.
 */
ScannerProcIO::ScannerProcIO()
{
    codec = QTextCodec::codecForName( "UTF-8" );
    if( !codec ) {
        ::error() << "Could not create UTF-8 codec for ScannerProcIO!" << endl;
    }
}


////////////////////////////////////////////////////////////////////////////////
// class ScanController
////////////////////////////////////////////////////////////////////////////////

ScanController::ScanController( CollectionDB* parent, bool incremental, const QStringList& folders )
    : DependentJob( parent, "CollectionScanner" )
    , QXmlDefaultHandler()
    , m_scanner( new ScannerProcIO() )
    , m_folders( folders )
    , m_incremental( incremental )
    , m_scannerCrashed( false )
    , m_hasChanged( false )
{
    DEBUG_BLOCK

    m_reader.setContentHandler( this );
    m_reader.parse( &m_source, true );

    connect( m_scanner, SIGNAL( readReady( KProcIO* ) ), SLOT( slotReadReady() ) );


    *m_scanner << "amarokcollectionscanner";
    *m_scanner << "--nocrashhandler"; // We want to be able to catch SIGSEGV

    // KProcess must be started from the GUI thread, so we're invoking the scanner
    // here in the ctor:
    if( incremental ) {
        setDescription( i18n( "Updating Collection" ) );
        initIncremental();
    }
    else {
        setDescription( i18n( "Building Collection" ) );
        if( AmarokConfig::importPlaylists() ) *m_scanner << "-p";
        if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
        *m_scanner << m_folders;
        m_scanner->start();
    }
}


ScanController::~ScanController()
{
    DEBUG_BLOCK

    if( m_scannerCrashed ) {
        ::error() << "CollectionScanner has crashed! Scan aborted." << endl;

        QFile log( amaroK::saveLocation( QString::null ) + "collection_scan.log" );
        log.open( IO_ReadOnly );
        const QString& path = log.readAll();
        if( path.isEmpty() )
            KMessageBox::error( 0, i18n( "Sorry, the Collection Scanner has crashed." ),
                                   i18n( "Collection Scan Error" ) );
        else
            KMessageBox::error( 0, i18n( "<p>Sorry, the Collection Scanner has crashed while "
                                         "reading the file:</p><p><i>%1</i></p><p>Please remove this "
                                         "file from your collection, then rescan the collection.</p>" )
                                         .arg( path ), i18n( "Collection Scan Error" ) );
    }

    m_scanner->kill();
    delete m_scanner;
}


/**
 * The Incremental Scanner works as follows: Here we check the mtime of every directory in the "directories"
 * table and store all changed directories in m_folders.
 *
 * These directories are then scanned in CollectionReader::doJob(), with m_recursively set according to the
 * user's preference, so the user can add directories or whole directory trees, too. Since we don't want to
 * rescan unchanged subdirectories, CollectionReader::readDir() checks if we are scanning recursively and
 * prevents that.
 */
void
ScanController::initIncremental()
{
    DEBUG_BLOCK

    const QStringList values = CollectionDB::instance()->query( "SELECT dir, changedate FROM directories;" );

    foreach( values ) {
        const QString folder = *it;
        const QString mtime  = *++it;

        const QFileInfo info( folder );
        if( info.exists() ) {
            if( info.lastModified().toTime_t() != mtime.toUInt() ) {
                m_folders << folder;
                debug() << "Collection dir changed: " << folder << endl;
            }
        }
        else {
            // this folder has been removed
            m_folders << folder;
            debug() << "Collection dir removed: " << folder << endl;
        }

        kapp->processEvents(); // Don't block the GUI
    }

    if ( !m_folders.isEmpty() ) {
        debug() << "Collection was modified." << endl;
        m_hasChanged = true;
        amaroK::StatusBar::instance()->shortMessage( i18n( "Updating Collection..." ) );

        // Start scanner process
        if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
        *m_scanner << "-i";
        *m_scanner << m_folders;
        m_scanner->start();
    }
}


bool
ScanController::doJob()
{
    DEBUG_BLOCK

    if( !CollectionDB::instance()->isConnected() )
        return false;
    if( m_incremental && !m_hasChanged )
        return true;

    CollectionDB::instance()->createTables( true );
    setProgressTotalSteps( 100 );


    uint delayCount = 100;

    /// Main Loop
    while( !isAborted() ) {
        if( m_xmlData.isNull() ) {
            if( !m_scanner->isRunning() )
                delayCount--;
            // Wait a bit after process has exited, so that we have time to parse all data
            if( delayCount == 0 )
                break;
            msleep( 15 );
        }
        else {
            m_dataMutex.lock();

            QDeepCopy<QString> data = m_xmlData;
            m_source.setData( data );
            m_xmlData = QString::null;

            m_dataMutex.unlock();

            if( !m_reader.parseContinue() )
                ::warning() << "parseContinue() failed: " << errorString() << endl << data << endl;
        }
    }


    if( !isAborted() ) {
        if( m_scanner->normalExit() && !m_scanner->signalled() ) {
            if ( m_incremental ) {
                m_foldersToRemove += m_folders;
                foreach( m_foldersToRemove ) {
                    CollectionDB::instance()->removeSongsInDir( *it );
                    CollectionDB::instance()->removeDirFromCollection( *it );
                }
            }
            else
                CollectionDB::instance()->clearTables( false ); // empty permanent tables

            CollectionDB::instance()->copyTempTables(); // copy temp into permanent tables
        }
        else
            m_scannerCrashed = true;
    }


    if( CollectionDB::instance()->isConnected() ) {
        CollectionDB::instance()->dropTables( true ); // drop temp tables
    }


    return !isAborted();
}


void
ScanController::slotReadReady()
{
    QString line;
    bool partial;

    m_dataMutex.lock();

    while( m_scanner->readln( line, true, &partial ) != -1 )
        m_xmlData += line;

    m_dataMutex.unlock();
}


bool
ScanController::startElement( const QString&, const QString& localName, const QString&, const QXmlAttributes& attrs )
{
    // List of entity names:
    //
    // itemcount     Number of files overall
    // folder        Folder which is being processed
    // dud           Invalid audio file
    // tags          Valid audio file with metadata
    // playlist      Playlist file
    // image         Cover image
    // compilation   Folder to check for compilation


    if( localName == "itemcount") {
        const int totalSteps = attrs.value( "count" ).toInt();
        debug() << "itemcount event: " << totalSteps << endl;
        setProgressTotalSteps( totalSteps );
    }

    if( localName == "dud" || localName == "tags" || localName == "playlist" ) {
        incrementProgress();
    }

    if( localName == "tags") {
        MetaBundle bundle;

        bundle.setPath      ( attrs.value( "path" ) );
        bundle.setTitle     ( attrs.value( "title" ) );
        bundle.setArtist    ( attrs.value( "artist" ) );
        bundle.setComposer  ( attrs.value( "composer" ) );
        bundle.setAlbum     ( attrs.value( "album" ) );
        bundle.setComment   ( attrs.value( "comment" ) );
        bundle.setGenre     ( attrs.value( "genre" ) );
        bundle.setYear      ( attrs.value( "year" ).toInt() );
        bundle.setTrack     ( attrs.value( "track" ).toInt() );
        bundle.setDiscNumber( attrs.value( "discnumber" ).toInt() );
        bundle.setFileType( attrs.value( "filetype" ).toInt() );

        if( attrs.value( "audioproperties" ) == "true" ) {
            bundle.setBitrate   ( attrs.value( "bitrate" ).toInt() );
            bundle.setLength    ( attrs.value( "length" ).toInt() );
            bundle.setSampleRate( attrs.value( "samplerate" ).toInt() );
        }

        if( !attrs.value( "filesize" ).isNull()
                && !attrs.value( "filesize" ).isEmpty() )
        {
            bundle.setFilesize( attrs.value( "filesize" ).toInt() );
        }

        CollectionDB::instance()->addSong( &bundle, m_incremental );
    }

    if( localName == "folder" ) {
        const QString folder = attrs.value( "path" );
        const QFileInfo info( folder );

        // Update dir statistics for rescanning purposes
        if( info.exists() )
            CollectionDB::instance()->updateDirStats( folder, info.lastModified().toTime_t(), true);

        if( m_incremental ) {
            m_foldersToRemove += folder;
        }
    }

    if( localName == "playlist" )
        QApplication::postEvent( PlaylistBrowser::instance(), new PlaylistFoundEvent( attrs.value( "path" ) ) );

    if( localName == "compilation" )
        CollectionDB::instance()->checkCompilations( attrs.value( "path" ), !m_incremental);

    if( localName == "image" ) {
        // Deserialize CoverBundle list
        QStringList list = QStringList::split( "AMAROK_MAGIC", attrs.value( "list" ), true );
        QValueList< QPair<QString, QString> > covers;

        for( uint i = 0; i < list.count(); ) {
            covers += qMakePair( list[i], list[i + 1] );
            i += 2;
        }

        CollectionDB::instance()->addImageToAlbum( attrs.value( "path" ), covers, CollectionDB::instance()->isConnected() );
    }


    return true;
}


#include "scancontroller.moc"
