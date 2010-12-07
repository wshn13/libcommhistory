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

#ifndef COMMHISTORY_TRACKERIO_P_H
#define COMMHISTORY_TRACKERIO_P_H

#include <QObject>
#include <QUrl>
#include <QtTracker/Tracker>
#include <QtSql>
#include <QHash>

#include "idsource.h"
#include "event.h"

class MmsContentDeleter;

namespace CommHistory {

class Group;
class UpdateQuery;
class TrackerIO;

/**
 * \class TrackerIOPrivate
 *
 * Private data and methods for TrackerIO
 */
class TrackerIOPrivate : public QObject
{
    Q_OBJECT
    TrackerIO *q;

public:
    TrackerIOPrivate(TrackerIO *parent);
    ~TrackerIOPrivate();

    static QUrl uriForIMAddress(const QString &account, const QString &remoteUid);

    /*!
     * Return IMContact node that corresponds to account/target (or
     * account if imID is empty), creating if necessary. Uses internal
     * cache during a transaction (TODO: general cache for all models
     * with refcounts).
     */
    QUrl findLocalContact(UpdateQuery &query,
                          const QString &accountPath);
    QUrl findIMContact(UpdateQuery &query,
                       const QString &accountPath,
                       const QString &imID);
    QUrl findPhoneContact(UpdateQuery &query,
                          const QString &accountPath,
                          const QString &remoteId);
    QUrl findRemoteContact(UpdateQuery &query,
                           const QString &localUid,
                           const QString &remoteUid);

    /*!
     * Helper for inserting and modifying common parts of nmo:Messages.
     *
     * \param query query to add RDF insertions or deletions
     * \param event to use
     * \param modifyMode if true, event.modifiedProperties are used to save
     *                   only changed properties, otherwise event.validProperties
     *                   is used to write all properties.
     */
    void writeCommonProperties(UpdateQuery &query, Event &event, bool modifyMode);
    void writeSMSProperties(UpdateQuery &query, Event &event, bool modifyMode);
    void writeMMSProperties(UpdateQuery &query, Event &event, bool modifyMode);
    void writeCallProperties(UpdateQuery &query, Event &event, bool modifyMode);

    void addMessageParts(UpdateQuery &query, Event &event);
    void setChannel(UpdateQuery &query, Event &event, int channelId);

    /* Used by addEvent(). */
    void addIMEvent(UpdateQuery &query, Event &event);
    void addSMSEvent(UpdateQuery &query, Event &event); // also handles MMS
    void addCallEvent(UpdateQuery &query, Event &event);


    // Helper for getEvent*().
    bool querySingleEvent(SopranoLive::RDFSelect query, Event &event);

    static void calculateParentId(Event& event);
    static void setFolderLastModifiedTime(UpdateQuery &query,
                                          int parentId,
                                          const QDateTime& lastModTime);

    void getMmsListForDeletingByGroup(int groupId, SopranoLive::LiveNodes& model);
    void deleteMmsContentByGroup(int group);
    MmsContentDeleter& getMmsDeleter(QThread *backgroundThread);
    bool isLastMmsEvent(const QString& messageToken);

    /*!
     * \brief Generate query request to get cc/bcc field from tracker
     *        for specific MMS message
     * \param event - MMS event
     */
    template <class CopyOntology>
    static QStringList queryMMSCopyAddresses(Event &event);

    QStringList queryMmsToAddresses(Event &event);
    void checkAndDeletePendingMmsContent(QThread* backgroundThread);

    SopranoLive::RDFTransactionPtr m_transaction;
    SopranoLive::RDFServicePtr m_service;

    // Temporary contact cache, valid during a transaction
    //TODO: rename
    QHash<QUrl, QUrl> m_imContactCache;
    MmsContentDeleter *m_MmsContentDeleter;
    typedef QHash<QString, int> MessageTokenRefCount;
    MessageTokenRefCount m_messageTokenRefCount;

    IdSource m_IdSource;

    Event::PropertySet commonPropertySet;
    Event::PropertySet smsOnlyPropertySet;

    QThread *m_bgThread;

    QSqlError lastError;
};

} // namespace

#endif // COMMHISTORY_TRACKERIO_P_H