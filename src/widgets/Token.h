/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *           (C) 2008-2009 Seb Ruiz <ruiz@kde.org>                            *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_TOKEN_H
#define AMAROK_TOKEN_H

#include <KIcon>
#include <QFrame>
#include <QLabel>
#include <QPixmap>


//Defines a part of a filename, drag&droppable in the FilenameLayoutWidget bar from the TokenListWidget list.
class Token : public QFrame
{
        Q_OBJECT

    public:

        explicit Token( const QString &text, const QString &iconName, int value, QWidget *parent = 0 );

        KIcon icon() const;
        QString iconName() const;
        QString name() const;
        int value() const;

    private:

        QString     m_name;
        KIcon       m_icon;
        QString     m_iconName;
        int         m_value;

        QLabel      *m_iconContainer;
        QLabel      *m_label;
};

#endif // AMAROK_TOKEN_H

