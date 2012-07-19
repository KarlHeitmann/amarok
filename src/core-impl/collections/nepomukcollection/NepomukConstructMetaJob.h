/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#ifndef NEPOMUKCONSTRUCTMETAJOB_H
#define NEPOMUKCONSTRUCTMETAJOB_H

#include "NepomukCollection.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core-impl/collections/support/MemoryCollection.h"

#include <ThreadWeaver/Job>
#include <QSharedPointer>

namespace Collections {

class NepomukConstructMetaJob : public ThreadWeaver::Job
{
    Q_OBJECT

public:
    explicit NepomukConstructMetaJob( QSharedPointer<Collections::MemoryCollection> mc );
    void run();
    bool running();

public slots:
    /**
     * Aborts the job as soon as it is safely possible
     */
    void abort();


signals:
    // signals for progress operation:
    void incrementProgress();
    void endProgressOperation( QObject *obj );
    void totalSteps( int steps ); // not used, defined to keep QObject::conect warning quiet

private:
    QSharedPointer<Collections::MemoryCollection> m_mc;
    bool m_aborted;
    bool m_running;
};

}

#endif // NEPOMUKCONSTRUCTMETAJOB_H
