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

#ifndef COMMON_H
#define COMMON_H

#include "eventmodel.h"
#include "event.h"
#include "group.h"

using namespace CommHistory;

const QString ACCOUNT1 = "/org/freedesktop/Telepathy/Account/gabble/jabber/dut_40localhost0";
const QString ACCOUNT2 = "/org/freedesktop/Telepathy/Account/gabble/jabber/dut2_40localhost0";

/* The default load polling interval when waiting system to become idle */
const int IDLE_POLL_INTERVAL = 2000;

/* System is considered idle when system load drops below this value */
const double IDLE_TRESHOLD = 0.05; // 5%

int addTestEvent(EventModel &model,
                 Event::EventType type,
                 Event::EventDirection direction,
                 const QString &account,
                 int groupId,
                 const QString &text = QString("test event"),
                 bool isDraft = false,
                 bool isMissedCall = false,
                 const QDateTime &when = QDateTime::currentDateTime(),
                 const QString &remoteUid = QString(),
                 bool toModelOnly = false,
                 const QString messageToken = QString());

void addTestGroups(Group &group1, Group &group2);
void addTestGroup(Group& grp, QString localUid, QString remoteUid);
void addTestContact(const QString &name, const QString &remoteUid);
bool compareEvents(Event &e1, Event &e2);
void deleteAll();
void deleteSmsMsgs();
QString randomMessage(int words);
double getSystemLoad();
void waitForIdle(int pollInterval = IDLE_POLL_INTERVAL);
bool waitSignal(QSignalSpy &spy, int msec);

#endif