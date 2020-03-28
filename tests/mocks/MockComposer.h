
/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef META_MOCKCOMPOSER_H
#define META_MOCKCOMPOSER_H

#include <gmock/gmock.h>

#include "core/meta/Meta.h"

namespace Meta
{
class MockComposer : public Meta::Composer
{
    public:
    MOCK_METHOD( QString, name, (), ( const ) );
    MOCK_METHOD( QString, prettyName, (), ( const ) );
    MOCK_METHOD( Meta::TrackList, tracks, () );
};
}

#endif
