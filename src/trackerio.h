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

#ifndef COMMHISTORY_TRACKERIO_H
#define COMMHISTORY_TRACKERIO_H

#include <QObject>
#include <QUrl>
#include <QSqlError>

#include "event.h"
#include "libcommhistoryexport.h"

namespace SopranoLive {
    class RDFSelect;
    class RDFVariable;
    class RDFTransaction;
}

namespace CommHistory {

class TrackerIOPrivate;
class Group;
class UpdateQuery;

/**
 * \class TrackerIO
 *
 * Class for handling events with tracker. You can use this if you are
 * implementing your own model.
 */
class LIBCOMMHISTORY_EXPORT TrackerIO : public QObject
{
    Q_OBJECT

public:
    TrackerIO(QObject *parent = 0);
    ~TrackerIO();

    /*!
     * Returns and increases the next available event id.
     */
    int nextEventId();

    /*!
     * Returns and increases the next available group id.
     */
    int nextGroupId();

    /*!
     * Adds required message properties to the query.
     * The optional communicationChannel will optimize the query for the
     * specified channel.
     */
    static void prepareMessageQuery(SopranoLive::RDFSelect &query,
                                    SopranoLive::RDFVariable &message,
                                    const Event::PropertySet &propertyMask,
                                    QUrl communicationChannel = QUrl());

    /*!
     * Adds required message properties to a multiuser chat query.
     */
    static void prepareMUCQuery(SopranoLive::RDFSelect &query,
                                SopranoLive::RDFVariable &message,
                                const Event::PropertySet &propertyMask,
                                QUrl communicationChannel);

    /*!
     * Adds required call properties to the query.
     */
    static void prepareCallQuery(SopranoLive::RDFSelect &query,
                                 SopranoLive::RDFVariable &call,
                                 const Event::PropertySet &propertyMask);

    /*!
     * Adds required message part properties to the query.
     */
    static void prepareMessagePartQuery(SopranoLive::RDFSelect &query,
                                        SopranoLive::RDFVariable &message);

    /*!
     * Adds required message part properties to the query.
     */
    static void prepareGroupQuery(SopranoLive::RDFSelect &query,
                                  const QString &localUid = QString(),
                                  const QString &remoteUid = QString(),
                                  int groupId = -1);

    /*!
     * Helper for prepare*Query() methods.
     */
    static void addMessagePropertiesToQuery(SopranoLive::RDFSelect &query,
                                            const Event::PropertySet &propertyMask,
                                            SopranoLive::RDFVariable &message);

    /*!
     * Add a new event into the database. The id field of the event is
     * updated if successfully added.
     *
     * \param event New event.
     * \return error (isValid() if insertion failed).
     */
    bool addEvent(Event &event);

    /*!
     * Add a new group into the database. The id field of the group is
     * updated if successfully added.
     *
     * \param group New group.
     * \return error (isValid() if insertion failed).
     */
    bool addGroup(Group &group);

    /*!
     * Query a single event by id.
     *
     * \param id Database id of the event.
     * \param event Return value for event details.
     * \return true if successful. Sets lastError() on failure.
     */
    bool getEvent(int id, Event &event);

    /*!
     * Query a single event by uri.
     *
     * \param Uri of the message to be fetched
     * \param event Return value for event details.
     * \return true if successful. Sets lastError() on failure.
     */
    bool getEventByUri(const QUrl &uri, Event &event);

    /*!
     * Query a single event by message token.
     *
     * \param token Message token
     * \param event Return value for event details.
     * \return true if successful. Sets lastError() on failure.
     */
    bool getEventByMessageToken(const QString &token, Event &event);

    /*!
     * Query a single event by message token and group ID.
     *
     * \param token Message token
     * \param groupId Group ID
     * \param event Return value for event details.
     * \return true if successful. Sets lastError() on failure.
     */
    bool getEventByMessageToken(const QString &token, int groupId, Event &event);

    /*!
     * Query a single event by mms id.
     *
     * \param mmsId mms id
     * \param groupId Group ID
     * \param event Return value for event details.
     * \return true if successful. Sets lastError() on failure.
     */
    bool getEventByMmsId(const QString &mmsId, int groupId, Event &event);

    /*!
     * Modifye an event.
     *
     * \param event Existing event.
     * \return true if successful. Sets lastError() on failure.
     */
    bool modifyEvent(Event &event);

    /*!
     * Move an event to a new group
     *
     * \param event Existing event
     * \param groupId new group id
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool moveEvent(Event &event, int groupId);

    /*!
     * Delete an event
     *
     * \param event Existing event to delete
     * \param backgroundThread optional thread (to delete mms attachments)
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool deleteEvent(Event &event, QThread *backgroundThread = 0);

    /*!
     * Query a single group by id.
     *
     * \param id Database id of the group.
     * \param group Return value for group details.
     * \return true if successful
     */
    bool getGroup(int id, Group &group);

    /*!
     * Modifye a group.
     *
     * \param event Existing group.
     * \return true if successful. Sets lastError() on failure.
     */
    bool modifyGroup(Group &group);

    /*!
     * Delete a group
     *
     * \param groupId Existing group id
     * \param deleteMessages flag to delete group's messages
     * \param backgroundThread optional thread (to delete mms attachments)
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool deleteGroup(int groupId, bool deleteMessages = true, QThread *backgroundThread = 0);

    /*!
     * Query the number of events in a group
     *
     * \param groupId Existing group id
     * \param totalEvents result
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool totalEventsInGroup(int groupId, int &totalEvents);

    /*!
     * Mark all messages in a group as read
     *
     * \param groupId Existing group id
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool markAsReadGroup(int groupId);

    /*!
     * Mark messages as read
     *
     * \param eventIds list of events to mark
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool markAsRead(const QList<int> &eventIds);

    /*!
     * Delete events of a certain type
     *
     * \param eventType
     *
     * \return true if successful. Sets lastError() on failure.
     */
    bool deleteAllEvents(Event::EventType eventType);

    /*!
     * Get details of the last error that occurred during the last query.
     *
     * \return error
     */
    QSqlError lastError() const;

    /*!
     * Initate a new tracker transaction.
     *
     * \param syncOnCommit set to perform tracker sync after commit
     */
    void transaction(bool syncOnCommit = false);

    /*!
     * Commits the current transaction.
     *
     * \param isBlocking if true, the call blocks until changes are saved
     *                   if false, the call is asynchronous and returns immediately
     * \return transaction object to track commit progress for non-blocking call
     */
    QSharedPointer<SopranoLive::RDFTransaction> commit(bool isBlocking=false);

    /*!
     * Cancels the current transaction.
     */
    void rollback();


private:
    friend class TrackerIOPrivate;
    TrackerIOPrivate * const d;
};

} // namespace

#endif