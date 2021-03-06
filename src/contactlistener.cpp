/******************************************************************************
**
** This file is part of libcommhistory.
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Reto Zingg <reto.zingg@nokia.com>
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

#include "contactlistener.h"

#include <QCoreApplication>
#include <QDebug>

#include <qtcontacts-extensions.h>

#include <QContactOnlineAccount>
#include <QContactPhoneNumber>
#include <QContactEmailAddress>
#include <QContactSyncTarget>

#include "commonutils.h"
#include "debug.h"

using namespace CommHistory;

Q_DECLARE_METATYPE(ContactListener::ContactAddress);
Q_DECLARE_METATYPE(QList<ContactListener::ContactAddress>);

QWeakPointer<ContactListener> ContactListener::m_instance;

typedef QPair<QString, QString> StringPair;

int ContactListener::internalContactId(const ApiContactIdType &id)
{
    return QtContactsSqliteExtensions::internalContactId(id);
}

int ContactListener::internalContactId(const QContact &contact)
{
    return QtContactsSqliteExtensions::internalContactId(contact.id());
}

ContactListener::ApiContactIdType ContactListener::apiContactId(int id)
{
    return QtContactsSqliteExtensions::apiContactId(static_cast<quint32>(id));
}

ContactListener::ContactListener(QObject *parent)
    : QObject(parent),
      m_initialized(false)
{
    qRegisterMetaType<QList<ContactListener::ContactAddress> >("QList<ContactAddress>");

    connect(this, SIGNAL(contactAlreadyInCache(quint32, const QString &, const QList<ContactAddress> &)),
            this, SIGNAL(contactUpdated(quint32, const QString &, const QList<ContactAddress> &)),
            Qt::QueuedConnection);
}

ContactListener::~ContactListener()
{
    SeasideCache::unregisterResolveListener(this);
    SeasideCache::unregisterChangeListener(this);
}

QSharedPointer<ContactListener> ContactListener::instance()
{
    QSharedPointer<ContactListener> result;
    if (!m_instance) {
        result = QSharedPointer<ContactListener>(new ContactListener());
        result->init();
        m_instance = result.toWeakRef();
    } else {
        result = m_instance.toStrongRef();
    }

    return result;
}

void ContactListener::init()
{
    if (m_initialized)
        return;

    DEBUG() << Q_FUNC_INFO;

    SeasideCache::registerChangeListener(this);

    m_initialized = true;
}

bool ContactListener::addressMatchesList(const QString &localUid,
                                         const QString &remoteUid,
                                         const QList< QPair<QString,QString> > &contactAddresses)
{
    QListIterator<StringPair> i(contactAddresses);
    while (i.hasNext()) {
        StringPair address = i.next();
        if ((address.first.isEmpty() || address.first == localUid)
            && CommHistory::remoteAddressMatch(remoteUid, address.second)) {
            return true;
        }
    }

    return false;
}

bool ContactListener::addressMatchesList(const QString &localUid,
                                         const QString &remoteUid,
                                         const QList<ContactAddress> &contactAddresses)
{
    QListIterator<ContactAddress> i(contactAddresses);
    while (i.hasNext()) {
        ContactAddress address = i.next();
        if ((address.localUid.isEmpty() || address.localUid == localUid)
            && CommHistory::remoteAddressMatch(remoteUid, address.remoteUid)) {
            return true;
        }
    }

    return false;
}

void ContactListener::resolveContact(const QString &localUid,
                                     const QString &remoteUid)
{
    DEBUG() << Q_FUNC_INFO << localUid << remoteUid;

    const StringPair input(qMakePair(localUid, remoteUid));

    SeasideCache::CacheItem *item = 0;

    // TODO: maybe better to switch on localUid value rather than numeric quality?
    QString number = CommHistory::normalizePhoneNumber(remoteUid);
    if (!number.isEmpty()) {
        m_pending.insert(qMakePair(QString(), remoteUid), input);
        item = SeasideCache::resolvePhoneNumber(this, remoteUid, true);
    } else {
        item = SeasideCache::resolveOnlineAccount(this, localUid, remoteUid, true);
    }

    if (item && (item->contactState == SeasideCache::ContactComplete)) {
        // This contact must be reported asynchronously
        emit contactAlreadyInCache(item->iid, contactName(item->contact), contactAddresses(item->contact));
    }
}

QString ContactListener::contactName(const QContact &contact)
{
    return SeasideCache::generateDisplayLabel(contact, SeasideCache::displayLabelOrder());
}

QList<ContactListener::ContactAddress> ContactListener::contactAddresses(const QContact &contact) const
{
    QList<ContactAddress> addresses;

    foreach (const QContactOnlineAccount &account, contact.details<QContactOnlineAccount>()) {
        QString localUid = account.value<QString>(QContactOnlineAccount__FieldAccountPath);
        addresses += makeContactAddress(localUid, account.accountUri(), IMAccountType);
    }
    foreach (const QContactPhoneNumber &phoneNumber, contact.details<QContactPhoneNumber>()) {
        addresses += makeContactAddress(QString(), phoneNumber.number(), PhoneNumberType);
    }
    foreach (const QContactEmailAddress &emailAddress, contact.details<QContactEmailAddress>()) {
        addresses += makeContactAddress(QString::fromLatin1("email"), emailAddress.emailAddress(), EmailAddressType);
    }

    return addresses;
}

void ContactListener::addressResolved(const QString &first, const QString &second, SeasideCache::CacheItem *item)
{
    if (item) {
        itemUpdated(item);
    } else {
        // This address could not be resolved
        StringPair address(qMakePair(first, second));

        QHash<StringPair, StringPair>::iterator it = m_pending.find(address);
        if (it != m_pending.end()) {
            // Report this address as unresolved
            address = *it;
            m_pending.erase(it);
        }

        emit contactUnknown(address);
    }
}

void ContactListener::itemUpdated(SeasideCache::CacheItem *item)
{
    static const QString aggregateTarget(QString::fromLatin1("aggregate"));

    // Only aggregate contacts are relevant
    QContactSyncTarget syncTarget(item->contact.detail<QContactSyncTarget>());
    if (syncTarget.syncTarget() == aggregateTarget) {
        emit contactUpdated(item->iid, contactName(item->contact), contactAddresses(item->contact));
    }
}

void ContactListener::itemAboutToBeRemoved(SeasideCache::CacheItem *item)
{
    emit contactRemoved(item->iid);
}

