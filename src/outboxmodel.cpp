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

#include <QtDBus/QtDBus>
#include <QtTracker/Tracker>
#include <QDebug>
#include <QtTracker/ontologies/nmo.h>

#include "trackerio.h"
#include "eventmodel.h"
#include "eventmodel_p.h"
#include "outboxmodel.h"
#include "event.h"

using namespace SopranoLive;

namespace CommHistory {

using namespace CommHistory;

class OutboxModelPrivate : public EventModelPrivate {
public:
    Q_DECLARE_PUBLIC(OutboxModel);

    OutboxModelPrivate(EventModel *model)
            : EventModelPrivate(model) {
    }

    bool acceptsEvent(const Event &event) const {
        qDebug() << __PRETTY_FUNCTION__ << event.id();
        if ((event.type() == Event::IMEvent
             || event.type() == Event::SMSEvent
             || event.type() == Event::MMSEvent) &&
            !event.isDraft() && event.direction() == Event::Outbound)
        {
            return true;
        }

        return false;
    }
};

OutboxModel::OutboxModel(QObject *parent)
        : EventModel(*new OutboxModelPrivate(this), parent)
{
}

OutboxModel::~OutboxModel()
{
}

bool OutboxModel::getEvents()
{
    Q_D(OutboxModel);

    reset();
    d->clearEvents();

    RDFSelect query;
    RDFVariable message = RDFVariable::fromType<nmo::Message>();
    message.property<nmo::isSent>(LiteralValue(true));
    message.property<nmo::isDraft>(LiteralValue(false));
    message.property<nmo::isDeleted>(LiteralValue(false));

    d->tracker()->prepareMessageQuery(query, message, d->propertyMask);

    return d->executeQuery(query);
}

}