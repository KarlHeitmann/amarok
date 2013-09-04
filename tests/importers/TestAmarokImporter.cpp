/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestAmarokImporter.h"

#include "MetaValues.h"
#include "importers/amarok/AmarokConfigWidget.h"
#include "importers/amarok/AmarokManager.h"
#include "importers/amarok/AmarokProviderEmbedded.h"

#include <qtest_kde.h>

Q_DECLARE_METATYPE( QProcess::ProcessError )
QTEST_KDEMAIN( TestAmarokImporter, GUI )

using namespace StatSyncing;

ProviderPtr
TestAmarokImporter::getProvider()
{
    QVariantMap cfg = AmarokConfigWidget( QVariantMap() ).config();
    cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                          "/importers_files/mysqle" );

    return ProviderPtr( new AmarokProviderEmbedded( cfg, 0 ) );
}

qint64
TestAmarokImporter::reliableStatistics() const
{
    return Meta::valFirstPlayed | Meta::valLastPlayed | Meta::valRating
            | Meta::valPlaycount | Meta::valLabel;
}

void
TestAmarokImporter::initTestCase()
{
    qRegisterMetaType<QProcess::ProcessError>();
}

void
TestAmarokImporter::init()
{
    m_cfg = AmarokConfigWidget( QVariantMap() ).config();
}

void
TestAmarokImporter::configWidgetShouldOnlyShowFieldsRelevantToConnection()
{
    AmarokConfigWidget widget( m_cfg );

    const QList<QWidget*> remoteConfigWidgets = QList<QWidget*>()
            << widget.m_databaseName << widget.m_hostname << widget.m_port
            << widget.m_password << widget.m_username;

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::Embedded );
    QVERIFY( !widget.m_databaseLocation->isHidden() );
    foreach( QWidget *w, remoteConfigWidgets )
        QVERIFY( w->isHidden() );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::External );
    QVERIFY( widget.m_databaseLocation->isHidden() );
    foreach( QWidget *w, remoteConfigWidgets )
        QVERIFY( !w->isHidden() );
}

void
TestAmarokImporter::configWidgetShouldNotSetDriver()
{
    AmarokConfigWidget widget( m_cfg );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::Embedded );
    QVERIFY( !widget.config().contains( "dbDriver" ) );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::External );
    QVERIFY( !widget.config().contains( "dbDriver" ) );
}

void
TestAmarokImporter::configWidgetShouldShowExternalAsDefault()
{
    AmarokConfigWidget widget( m_cfg );
    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::External ) );
}

void
TestAmarokImporter::configWidgetShouldNotBreakOnNonsenseInitialValues()
{
    m_cfg.insert( "dbName", QColor( Qt::red ) );
    m_cfg.insert( "dbPort", "nonsensePort" );
    m_cfg.insert( "dbPath", reinterpret_cast<qptrdiff>( this ) );

    AmarokConfigWidget widget( m_cfg );

    QVERIFY( !widget.m_databaseName->text().isEmpty() );
    QVERIFY( !widget.m_databaseLocation->text().isEmpty() );
}

void
TestAmarokImporter::configWidgetShouldReadSavedConfig()
{
    m_cfg.insert( "embedded", true );
    m_cfg.insert( "dbName", "MyName" );
    m_cfg.insert( "dbPort", 19 );
    m_cfg.insert( "name", "theName" );
    AmarokConfigWidget widget( m_cfg );

    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::Embedded ) );

    QCOMPARE( widget.m_databaseName->text(), QString( "MyName" ) );
    QCOMPARE( widget.m_port->value(), 19 );
    QCOMPARE( widget.m_targetName->text(), QString( "theName" ) );

    m_cfg.insert( "embedded", false );
    AmarokConfigWidget widgetExternal( m_cfg );

    QCOMPARE( widgetExternal.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::External ) );
}

void
TestAmarokImporter::providerShouldIgnoreConfigsDbDriver()
{
    m_cfg.insert( "dbDriver", "QPSQL" );
    m_cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                            "/importers_files/mysqle" );

    AmarokProviderEmbedded provider( m_cfg, 0 );

    // The database isn't accessible by QPSQL driver, but it still should work
    QVERIFY( !provider.artists().empty() );
}

void
TestAmarokImporter::providerShouldHandleNonexistentDbDir()
{
    m_cfg.insert( "dbPath", "/Im/sure/this/wont/exist" );

    AmarokProviderEmbedded provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleInvalidDbDir()
{
    m_cfg.insert( "dbPath", QApplication::applicationDirPath() );

    AmarokProviderEmbedded provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleExternalConnectionError()
{
    m_cfg.insert( "dbHost", "I hope this isn't a valid hostname" );

    AmarokProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbDriver", 19 );
    m_cfg.insert( "dbName", QColor( Qt::red ) );
    m_cfg.insert( "dbPort", "nonsensePort" );

    AmarokProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::managerShouldReturnProviderAdequateToConnectionType()
{
    AmarokManager am( 0, QVariantList() );

    m_cfg.insert( "embedded", true );
    QVERIFY( dynamic_cast<AmarokProviderEmbedded*>( am.createProvider( m_cfg ).data() ) );

    m_cfg.insert( "embedded", false );
    QVERIFY( dynamic_cast<AmarokProvider*>( am.createProvider( m_cfg ).data() ) );
}

#include "TestAmarokImporter.moc"
