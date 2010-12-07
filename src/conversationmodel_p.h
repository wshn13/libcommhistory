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

#ifndef COMMHISTORY_CONVERSATIONMODEL_P_H
#define COMMHISTORY_CONVERSATIONMODEL_P_H

#include "eventmodel_p.h"
#include "conversationmodel.h"
#include "group.h"

namespace CommHistory
{

class ConversationModelPrivate : public EventModelPrivate {
public:
    Q_OBJECT
    Q_DECLARE_PUBLIC(ConversationModel);

    ConversationModelPrivate(EventModel *model);

    bool updateEventsRecursive(int contactId,
                               const QString &contactName,
                               const QString &remoteUid,
                               EventTreeItem *parent);
    QModelIndex findParent(const Event &event);
    bool acceptsEvent(const Event &event) const;
    bool fillModel(int start, int end, QList<CommHistory::Event> events);
    EventTreeItem* findDividerItem(const Event &divider);
    EventTreeItem* findDivider(const Event &event);

public Q_SLOTS:
    void groupsUpdatedFullSlot(const QList<CommHistory::Group> &groups);

public:
    int filterGroupId;
    Event::EventType filterType;
    QString filterAccount;
    Event::EventDirection filterDirection;
    Group::ChatType chatType;
};

}


#endif