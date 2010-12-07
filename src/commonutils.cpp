/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Alexander Shalamov <alexander.shalamov@nokia.com>
**
** This library is free software; you can redistribute it and/or modify it
** under the terms of the GNU Lesser General Public License version 2.1 as
** published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
** or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
** License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this library; if not, write to the Free Software Foundation, Inc.,
** 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
**
******************************************************************************/

#include <QString>
#include <QRegExp>

#include "commonutils.h"
#include "libcommhistoryexport.h"

namespace CommHistory {

LIBCOMMHISTORY_EXPORT QString normalizePhoneNumber(const QString &number)
{
    QString result(number);

    // artistic reinterpretation of Fremantle code...

    // remove extra punctuation
    result.remove(QRegExp("[()\\-\\. ]"));

    // check for invalid characters
    if (result.indexOf(QRegExp("[^0-9#\\*\\+XxWwPp]")) != -1) {
        return QString();
    }

    // remove everything after and including x/w/p
    result.replace(QRegExp("[XxWwPp].*"), "");

    // can't have + with control codes
    if ((result.indexOf(QLatin1String("*31#")) != -1 ||
         result.indexOf(QLatin1String("#31#")) != -1) &&
         result.indexOf('+') != -1) {
        return QString();
    }

    return result;
}

LIBCOMMHISTORY_EXPORT bool remoteAddressMatch(const QString &uid, const QString &match)
{
    QString phone = normalizePhoneNumber(uid);

    // IM
    if (phone.isEmpty() && uid != match)
        return false;

    // phone number
    QString matchPhone = normalizePhoneNumber(match);
    if (!matchPhone.endsWith(phone.right(PHONE_NUMBER_MATCH_LENGTH)))
        return false;

    return true;
}

}