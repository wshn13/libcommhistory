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
#include "callmodel.h"
#include "callmodel_p.h"
#include "event.h"
#include "commonutils.h"
#include "contactlistener.h"

using namespace SopranoLive;

namespace CommHistory
{

using namespace CommHistory;

/* ************************************************************************** *
 * ******** P R I V A T E   C L A S S   I M P L E M E N T A T I O N ********* *
 * ************************************************************************** */

CallModelPrivate::CallModelPrivate( EventModel *model )
        : EventModelPrivate( model )
        , sortBy( CallModel::SortByContact )
        , eventType( CallEvent::UnknownCallType )
        , referenceTime( QDateTime() )
        , hasBeenFetched( false )
{
    contactChangesEnabled = true;
    connect(this, SIGNAL(eventsCommitted(const QList<CommHistory::Event>&,bool)),
            this, SLOT(slotEventsCommitted(const QList<CommHistory::Event>&,bool)));
}

bool CallModelPrivate::acceptsEvent( const Event &event ) const
{
    qDebug() << __PRETTY_FUNCTION__ << event.id();
    if ( event.type() != Event::CallEvent )
    {
        return false;
    }

    if(!referenceTime.isNull() && (event.startTime() < referenceTime)) // a reference Time is already set, so any further event addition should be beyond that
    {
        return false;
    }

    if(this->eventType != CallEvent::UnknownCallType)
    {
        if(eventType == CallEvent::MissedCallType && !(event.direction() == Event::Inbound && event.isMissedCall()))
        {
                return false;
        }
        else if(eventType == CallEvent::DialedCallType && !(event.direction() == Event::Outbound) )
        {
            return false;
        }
        else if(eventType == CallEvent::ReceivedCallType && !(event.direction() == Event::Inbound && !event.isMissedCall()))
        {
            return false;
        }
    }

    return true;
}

bool CallModelPrivate::belongToSameGroup( const Event &e1, const Event &e2 )
{
    if (sortBy == CallModel::SortByContact
        && remoteAddressMatch(e1.remoteUid(), e2.remoteUid())
        && e1.localUid() == e2.localUid())
    {
        return true;
    }
    else if (sortBy == CallModel::SortByTime
             && (remoteAddressMatch(e1.remoteUid(), e2.remoteUid())
                 && e1.localUid() == e2.localUid()
                 && e1.direction() == e2.direction()
                 && e1.isMissedCall() == e2.isMissedCall()))
    {
        return true;
    }
    return false;
}

int CallModelPrivate::calculateEventCount( EventTreeItem *item )
{
    int count = -1;

    switch ( sortBy )
    {
        case CallModel::SortByContact :
        {
            // set event count for missed calls only,
            // leave the default value for non-missed ones
            if ( item->event().isMissedCall() )
            {
                count = 1;
                // start looping the list from index number 1, because
                // the index number 0 is the same item as the top level
                // one
                for ( int i = 1; i < item->childCount(); i++ )
                {
                    if ( item->child( i - 1 )->event().isMissedCall() &&
                         item->child( i )->event().isMissedCall() )
                    {
                        count++;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            break;
        }
        case CallModel::SortByTime :
        {
            count = item->childCount();
            break;
        }
        default:
            break;
    }

    return count;
}

bool CallModelPrivate::fillModel( int start, int end, QList<CommHistory::Event> events )
{
    Q_UNUSED( start );
    Q_UNUSED( end );
    Q_Q( CallModel );

    //for flat mode EventModelPrivate::fillModel is sufficient as all the events will be stored at the top level
    if(!isInTreeMode)
    {
        return EventModelPrivate::fillModel(start, end, events);
    }

    if ( events.count() > 0 )
    {
        /*
         * call events are grouped as follows:
         *
         * [event 1] - (event 1)
         *             (event 2)
         *             (event 3)
         * [event 4] - (event 4)
         *             (event 5)
         * ...         ...
         *
         * NOTE:
         * on the top level, there are only the representatives of each group
         * on second level, there are all call events listed (also group reps)
         */

        QList<EventTreeItem *> topLevelItems;
        // get the first event and save it as top level item
        Event event = events.first();
        // add first item to the top and also to the second level
        topLevelItems.append( new EventTreeItem( event ) );
        topLevelItems.last()->appendChild( new EventTreeItem( event, topLevelItems.last() ) );

        switch ( sortBy )
        {
            /*
             * if sorted by contact,
             * then event count is meaningful only for missed calls.
             * it shows how many missed calls there are under the top
             * level one without breaking the continuity of the missed
             * calls.
             *
             * John, 3 missed
             * Mary, 1 received
             * John, 1 dialed
             * John, 1 missed
             * Mary, 2 dialed
             *
             *      ||
             *      \/
             *
             * John, 3 missed
             * Mary, received
             *
             * NOTE 1:
             * there are actually 4 missed calls from John, but there was
             * 1 dialed in between, that is why the event count is 3.
             *
             * NOTE 2:
             * there is no number for the received calls, since only the
             * missed calls have valid even count. (But -1 will be returned.)
             */
            case CallModel::SortByContact :
            {
                // loop through the result set
                for ( int i = 1; i < events.count(); i++ )
                {
                    bool inserted = false;
                    // check if event is groupable with any already existing group
                    foreach ( EventTreeItem *item, topLevelItems )
                    {
                        // if proper group found, then append it to the end
                        if ( belongToSameGroup( events.at( i ), item->event() ) )
                        {
                            item->appendChild( new EventTreeItem( events.at( i ), item ) );
                            inserted = true;
                            break;
                        }
                    }
                    // if event was not yet grouped, then create a new group
                    if ( !inserted )
                    {
                        topLevelItems.append( new EventTreeItem( events.at( i ) ) );
                        topLevelItems.last()->appendChild(new EventTreeItem( events.at( i ),
                                                                             topLevelItems.last() ) );
                    }
                }
                break;
            }
            /*
             * if sorted by time,
             * then event count is the number of events grouped under the
             * top level one
             *
             * John, 3 missed
             * Mary, 1 received
             * John, 1 dialed
             * John, 1 missed
             * Mary, 2 dialed
             */
            case CallModel::SortByTime  :
            {
                // loop through the result set
                for ( int row = 1; row < events.count(); row++ )
                {
                    Event event = events.at( row );
                    // if event is NOT groupable with the last top level one, then create new group
                    if ( !belongToSameGroup( event, topLevelItems.last()->event() ) )
                    {
                        topLevelItems.append( new EventTreeItem( event ) );
                    }
                    // add event to the last group
                    // this is an existing or a freshly created one with the same event as representative
                    topLevelItems.last()->appendChild( new EventTreeItem( event, topLevelItems.last() ) );
                }
                break;
            }
            default:
                break;
        }

        // once the events are grouped,
        // loop through the top level items and update event counts
        foreach ( EventTreeItem *item, topLevelItems )
        {
            item->event().setEventCount( calculateEventCount( item ) );
        }

        // save top level items into the model
        q->beginInsertRows( QModelIndex(), 0, topLevelItems.count() - 1);
        foreach ( EventTreeItem *item, topLevelItems )
        {
            eventRootItem->appendChild( item );
        }
        q->endInsertRows();
    }

    return true;
}

void CallModelPrivate::addToModel( Event &event )
{
    Q_Q(CallModel);
    qDebug() << __PRETTY_FUNCTION__ << event.id();

    if(!isInTreeMode)
    {
            return EventModelPrivate::addToModel(event);
    }

    if (event.contactId() > 0) {
        contactCache.insert(qMakePair(event.localUid(), event.remoteUid()),
                            qMakePair(event.contactId(), event.contactName()));
    } else {
        if (!setContactFromCache(event)) {
            // calls don't have the luxury of only one contact per
            // conversation -> resolve unknowns and add to cache
            startContactListening();
            if (contactListener)
                contactListener->resolveContact(event.localUid(), event.remoteUid());
        }
    }

    switch ( sortBy )
    {
        case CallModel::SortByContact :
        {
            int matchingRow = -1;
            // (1) check if could be added to any existing group
            for ( int i = 0; i < eventRootItem->childCount(); i++ )
            {
                // if matching group found, then store index
                if ( belongToSameGroup( event, eventRootItem->child( i )->event() ) )
                {
                    matchingRow = i;
                    break;
                }
            }

            // (2.a) if yes, then add it there and reorder the groups (last modified goes to top)
            if ( matchingRow > -1 )
            {
                //
                // TODO : signal changes
                //

                // (2.a.1) add event to the found group and update values
                EventTreeItem *matchingTopLevelItem = eventRootItem->child( matchingRow );
                matchingTopLevelItem->prependChild( new EventTreeItem( event, matchingTopLevelItem ) );
                matchingTopLevelItem->setEvent( event );
                matchingTopLevelItem->event().setEventCount( calculateEventCount( matchingTopLevelItem ) );

                // (2.a.2) reorder groups if needed
                if ( matchingRow > 0 )
                {
                    emit q->layoutAboutToBeChanged();
                    eventRootItem->moveChild( matchingRow, 0 );
                    emit q->layoutChanged();
                }
                else
                {
                    emit q->dataChanged( q->createIndex( 0, 0, eventRootItem->child( 0 ) ),
                                         q->createIndex( 0, CallModel::NumberOfColumns - 1, eventRootItem->child( 0 ) ) );
                }
            }

            // (2.b) if not, then just create a new group on the top
            else
            {
                q->beginInsertRows( QModelIndex(), 0, 0 );
                // add new item as first on the list
                eventRootItem->prependChild( new EventTreeItem( event ) );
                // alias
                EventTreeItem *firstTopLevelItem = eventRootItem->child( 0 );
                // add the copy of the event to its local list and refresh event count
                firstTopLevelItem->prependChild( new EventTreeItem( event, firstTopLevelItem ) );
                firstTopLevelItem->event().setEventCount( calculateEventCount( firstTopLevelItem ) );
                q->endInsertRows();
            }
            break;
        }
        case CallModel::SortByTime :
        {
            // if new item is groupable with the first one in the list
            // NOTE: assumption is that time value is ok
            if ( eventRootItem->childCount() && belongToSameGroup( event, eventRootItem->child( 0 )->event() ) )
            {
                // alias
                EventTreeItem *firstTopLevelItem = eventRootItem->child( 0 );
                // add event to the group, set it as top level item and refresh event count
                firstTopLevelItem->prependChild( new EventTreeItem( event, firstTopLevelItem ) );
                firstTopLevelItem->setEvent( event );
                firstTopLevelItem->event().setEventCount( calculateEventCount( firstTopLevelItem ) );
                // only counter and timestamp of first must be updated
                emit q->dataChanged( q->createIndex( 0, 0, eventRootItem->child( 0 ) ),
                                     q->createIndex( 0, CallModel::NumberOfColumns - 1, eventRootItem->child( 0 ) ) );
            }
            // create a new group, otherwise
            else
            {
                // a new row must be inserted
                q->beginInsertRows( QModelIndex(), 0, 0 );
                // add new item as first on the list
                eventRootItem->prependChild( new EventTreeItem( event ) );
                // alias
                EventTreeItem *firstTopLevelItem = eventRootItem->child( 0 );
                // add the copy of the event to its local list and refresh event count
                firstTopLevelItem->prependChild( new EventTreeItem( event, firstTopLevelItem ) );
                firstTopLevelItem->event().setEventCount( calculateEventCount( firstTopLevelItem ) );
                q->endInsertRows();
            }
            break;
        }
        default :
        {
            qWarning() << __PRETTY_FUNCTION__ << "Adding call events to model sorted by type or by service has not been implemented yet.";
            return;
        }
    }
}

void CallModelPrivate::eventsAddedSlot( const QList<Event> &events )
{
    qDebug() << __PRETTY_FUNCTION__ << events.count();
    // TODO: sorting?
    EventModelPrivate::eventsAddedSlot(events);
}

void CallModelPrivate::eventsUpdatedSlot( const QList<Event> &events )
{
    // TODO regrouping of events might occur =(
    qWarning() << __PRETTY_FUNCTION__ << "Specific behaviour has not been implemented yet.";
    EventModelPrivate::eventsUpdatedSlot( events );
}

QModelIndex CallModelPrivate::findEvent( int id ) const
{
    Q_Q( const CallModel );

    if(!isInTreeMode)
    {
        return EventModelPrivate::findEvent(id);
    }

    for ( int row = 0; row < eventRootItem->childCount(); row++ )
    {
        // check top level item
        if ( eventRootItem->child( row )->event().id() == id )
        {
            return q->createIndex( row, 0, eventRootItem->child( row ) );
        }
        // loop through all grouped events
        EventTreeItem *currentGroup = eventRootItem->child( row );
        for ( int column = 0; column < currentGroup->childCount(); column++ )
        {
            if ( currentGroup->child( column )->event().id() == id )
            {
                return q->createIndex( row, column, currentGroup->child( column ) );
            }
        }
    }

    // id was not found, return invalid index
    return QModelIndex();
}

void CallModelPrivate::deleteFromModel( int id )
{
    Q_Q(CallModel);

    if(!isInTreeMode)
    {
        return EventModelPrivate::deleteFromModel(id);
    }

    // TODO : what if an event is deleted from the db through commhistory-tool?

    // seek for the top level item which was deleted
    QModelIndex index = findEvent( id );

    // if id was not found, do nothing
    if ( !index.isValid() )
    {
        qDebug() << __PRETTY_FUNCTION__ << "*** Invalid";
        return;
    }

    // TODO : it works only when sorting is time based

    // if event is a top level item ( i.e. the whole group ), then delete it
    if ( index.column() == 0 )
    {
        int row = index.row();
        bool isRegroupingNeeded = false;
        // regrouping is needed/possible only if sorting is SortByTime...
        // ...and there is a previous row and a following row to group together
        if ( sortBy == CallModel::SortByTime &&
             row - 1 >= 0 && row + 1 < eventRootItem->childCount() )
        {
            EventTreeItem *prev = eventRootItem->child( row - 1 );
            EventTreeItem *next = eventRootItem->child( row + 1 );

            if ( belongToSameGroup( prev->event(), next->event() ) )
            {
                for ( int i = 0; i < next->childCount(); i++ )
                {
                    prev->appendChild( new EventTreeItem( next->child( i )->event() ) );
                }
                prev->event().setEventCount( calculateEventCount( prev ) );
                isRegroupingNeeded = true;
            }
        }

        qDebug() << __PRETTY_FUNCTION__ << "*** Top level";
        // if there is no need to regroup the previous and following items,
        // then delete only one row
        if ( !isRegroupingNeeded )
        {
            q->beginRemoveRows( index.parent(), row, row );
            eventRootItem->removeAt( row );
        }
        // otherwise delete the current and the following one
        // (since we added content of the following to the previous)
        else
        {
            q->beginRemoveRows( index.parent(), row, row + 1 );
            eventRootItem->removeAt( row + 1 );
            eventRootItem->removeAt( row );
            emit q->dataChanged( q->createIndex( row - 1, 0, eventRootItem->child( row - 1 ) ),
                                 q->createIndex( row - 1, 0, eventRootItem->child( row - 1 ) ) );
        }
        q->endRemoveRows();
    }
    // otherwise item is a grouped event
    else
    {
        qDebug() << __PRETTY_FUNCTION__ << "*** Sth else";
        // TODO :
        // delete it from the model
        // update top level item
        // emit dataChanged()
    }
}

void CallModelPrivate::slotEventsCommitted(const QList<CommHistory::Event> &events, bool success)
{
    Q_UNUSED(events);
    Q_Q(CallModel);

    // Empty events list means all events have been deleted (with deleteAll)
    if (success && deleteSync && events.isEmpty()) {
        qWarning() << __PRETTY_FUNCTION__ << "clearing model";
        q->beginResetModel();
        clearEvents();
        q->endResetModel();
        deleteSync = false;
    }
}

/* ************************************************************************** *
 * ********* P U B L I C   C L A S S   I M P L E M E N T A T I O N ********** *
 * ************************************************************************** */

CallModel::CallModel(QObject *parent)
        : EventModel(*new CallModelPrivate(this), parent)
{
    Q_D(CallModel);
    d->isInTreeMode = true;
}

CallModel::CallModel(CallModel::Sorting sorting, QObject* parent = 0)
        : EventModel(*new CallModelPrivate(this), parent)
{
    Q_D( CallModel );
    d->isInTreeMode = true;

    setFilter( sorting );
}

CallModel::~CallModel()
{
}


void CallModel::setQueryMode( EventModel::QueryMode mode )
{
    if (mode == EventModel::StreamedAsyncQuery) {
        qWarning() << __PRETTY_FUNCTION__ << "CallModel can not use streamed query mode.";
    } else {
        EventModel::setQueryMode(mode);
    }
}

bool CallModel::setFilter(CallModel::Sorting sortBy,
                          CallEvent::CallType type,
                          const QDateTime &referenceTime)
{
    Q_D(CallModel);

    // save sorting, reference Time and call event Type for filtering call events
    d->sortBy = sortBy;
    d->eventType = type;
    d->referenceTime = referenceTime;

    if ( d->hasBeenFetched )
    {
        return getEvents();
    }
    return true;
}

bool CallModel::getEvents()
{
    Q_D(CallModel);

    d->hasBeenFetched = true;

    beginResetModel();
    d->clearEvents();
    endResetModel();

    RDFSelect query;
    RDFVariable call = RDFVariable::fromType<nmo::Call>();

    if(!d->referenceTime.isNull())
    {
        RDFVariable date = call.property<nmo::receivedDate>();
        date.greaterOrEqual(LiteralValue(d->referenceTime));
    }

    if(d->eventType != CallEvent::UnknownCallType)
    {
        if(d->eventType == CallEvent::ReceivedCallType ||
           d->eventType == CallEvent::MissedCallType)
        {
            call.property<nmo::isSent>(LiteralValue(false));

            if(d->eventType == CallEvent::MissedCallType)
            {
                call.property<nmo::isAnswered>(LiteralValue(false));
            }
            else
            {
                call.property<nmo::isAnswered>(LiteralValue(true));
            }
        } //event type == CallEvent::DialedCallType
        else
        {
            call.property<nmo::isSent>(LiteralValue(true));
        }
    }

    d->tracker()->prepareCallQuery( query, call, d->propertyMask );

    return d->executeQuery(query);
}

bool CallModel::getEvents(CallModel::Sorting sortBy,
                          CallEvent::CallType type,
                          const QDateTime &referenceTime)
{
    Q_D(CallModel);

    d->hasBeenFetched = true;

    return setFilter( sortBy, type, referenceTime );
}

bool CallModel::deleteAll()
{
    Q_D(CallModel);

    d->tracker()->transaction();

    bool deleted;
    deleted = d->tracker()->deleteAllEvents(Event::CallEvent);
    if (!deleted) {
        qWarning() << __PRETTY_FUNCTION__ << "Failed to delete events";
        d->tracker()->rollback();
        return false;
    }

    d->commitTransaction(QList<Event>());

    return true;
}

bool CallModel::addEvent( Event &event )
{
    return EventModel::addEvent(event);
}

bool CallModel::modifyEvent( Event &event )
{
    qWarning() << __PRETTY_FUNCTION__ << "Specific behaviour has not been implemented yet.";
    return EventModel::modifyEvent( event );
}

bool CallModel::deleteEvent( int id )
{
    Q_D(CallModel);

    if(!d->isInTreeMode)
    {
        return EventModel::deleteEvent(id);
    }

    switch ( d->sortBy )
    {
        case SortByContact :
        case SortByTime :
        {
            // TODO : handle possibility of failure
            QModelIndex index = d->findEvent( id );

            // if id was not found, there is nothing to delete
            if ( !index.isValid() )
            {
                return false;
            }

            EventTreeItem *item = d->eventRootItem->child( index.row() );

            d->tracker()->transaction( d->syncOnCommit );

            QList<Event> deletedEvents;

            // get all events stored in the item and delete them one by one
            for ( int i = 0; i < item->childCount(); i++ )
            {
                // NOTE: when events are sorted by time, the tree hierarchy is only 2 levels deep
                if (!d->tracker()->deleteEvent(item->child(i)->event())) {
                    d->tracker()->rollback();
                    return false;
                }
                deletedEvents << item->child( i )->event();
            }

            d->commitTransaction(deletedEvents);
            // delete event from model (not only from db)
            d->deleteFromModel( id );
            // signal delete in case someone else needs to know it
            emit d->eventDeleted( id );

            return true;
        }
        default :
        {
            qWarning() << __PRETTY_FUNCTION__ << "Deleting of call events from model sorted by type or by service has not been implemented yet.";
            return false;
        }
    }
}

bool CallModel::deleteEvent( Event &event )
{
    return deleteEvent( event.id() );
}

}