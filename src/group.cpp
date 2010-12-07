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

#include <QDBusArgument>

#include "group.h"
#include "event.h"

namespace CommHistory {

class GroupPrivate : public QSharedData
{
public:
    GroupPrivate();
    GroupPrivate(const GroupPrivate &other);
    ~GroupPrivate();

    void propertyChanged(Group::Property property) {
        validProperties += property;
        modifiedProperties += property;
    }

    int id;
    QString localUid;
    QStringList remoteUids;
    Group::ChatType chatType;
    QString chatName;
    QDateTime endTime;
    int totalMessages;
    int unreadMessages;
    int sentMessages;
    int lastEventId;
    int contactId;
    QString contactName;
    QString lastMessageText;
    QString lastVCardFileName;
    QString lastVCardLabel;
    Event::EventType lastEventType;
    Event::EventStatus lastEventStatus;
    bool isPermanent;
    QDateTime lastModified;

    Group::PropertySet validProperties;
    Group::PropertySet modifiedProperties;
};

GroupPrivate::GroupPrivate()
        : id(-1)
        , chatType(Group::ChatTypeP2P)
        , totalMessages(0)
        , unreadMessages(0)
        , sentMessages(0)
        , lastEventId(-1)
        , contactId(0)
        , lastEventType(Event::UnknownType)
        , lastEventStatus(Event::UnknownStatus)
        , isPermanent(false)
{
    lastModified = QDateTime::fromTime_t(0);
    validProperties += Group::Id;
}

GroupPrivate::GroupPrivate(const GroupPrivate &other)
        : QSharedData(other)
        , id(other.id)
        , localUid(other.localUid)
        , remoteUids(other.remoteUids)
        , chatType(other.chatType)
        , chatName(other.chatName)
        , endTime(other.endTime)
        , totalMessages(other.totalMessages)
        , unreadMessages(other.unreadMessages)
        , sentMessages(other.sentMessages)
        , lastEventId(other.lastEventId)
        , contactId(other.contactId)
        , contactName(other.contactName)
        , lastMessageText(other.lastMessageText)
        , lastVCardFileName(other.lastVCardFileName)
        , lastVCardLabel(other.lastVCardLabel)
        , lastEventType(other.lastEventType)
        , lastEventStatus(other.lastEventStatus)
        , isPermanent(other.isPermanent)
        , lastModified(other.lastModified)
        , validProperties(other.validProperties)
        , modifiedProperties(other.modifiedProperties)
{
}

GroupPrivate::~GroupPrivate()
{
}

}

using namespace CommHistory;

static Group::PropertySet setOfAllProperties;

Group::PropertySet Group::allProperties()
{
    if (setOfAllProperties.isEmpty()) {
        for (int i = 0; i < Group::NumProperties; i++)
            setOfAllProperties += (Group::Property)i;
    }

    return setOfAllProperties;
}

Group::Group()
        : d(new GroupPrivate)
{
}

Group::Group(const Group &other)
        : d(other.d)
{
}

Group &Group::operator=(const Group &other)
{
    d = other.d;
    return *this;
}

Group::~Group()
{
}

int Group::urlToId(const QString &url)
{
    return url.mid(QString(QLatin1String("conversation:")).length()).toInt();
}

QUrl Group::idToUrl(int id)
{
    return QUrl(QString(QLatin1String("conversation:%1")).arg(id));
}

bool Group::isValid() const
{
    return (d->id != -1);
}

Group::PropertySet Group::validProperties() const
{
    return d->validProperties;
}

Group::PropertySet Group::modifiedProperties() const
{
    return d->modifiedProperties;
}

bool Group::operator==(const Group &other) const
{
    if (this->d->id == other.id() &&
        this->d->localUid == other.localUid()) {
        return true;
    }

    return false;
}

int Group::id() const
{
    return d->id;
}


QUrl Group::url() const
{
    return Group::idToUrl(d->id);
}


QString Group::localUid() const
{
    return d->localUid;
}


QStringList Group::remoteUids() const
{
    return d->remoteUids;
}

Group::ChatType Group::chatType() const
{
    return d->chatType;
}

QString Group::chatName() const
{
    return d->chatName;
}


QDateTime Group::endTime() const
{
    return d->endTime;
}


int Group::totalMessages() const
{
    return d->totalMessages;
}


int Group::unreadMessages() const
{
    return d->unreadMessages;
}


int Group::sentMessages() const
{
    return d->sentMessages;
}


int Group::lastEventId() const
{
    return d->lastEventId;
}

int Group::contactId() const
{
    return d->contactId;
}

QString Group::contactName() const
{
    return d->contactName;
}

QString Group::lastMessageText() const
{
    return d->lastMessageText;
}

QString Group::lastVCardFileName() const
{
    return d->lastVCardFileName;
}

QString Group::lastVCardLabel() const
{
    return d->lastVCardLabel;
}

Event::EventType Group::lastEventType() const
{
    return d->lastEventType;
}

Event::EventStatus Group::lastEventStatus() const
{
    return d->lastEventStatus;
}

bool Group::isPermanent() const
{
    return d->isPermanent;
}

QDateTime Group::lastModified() const
{
    return d->lastModified;
}

void Group::setValidProperties(const Group::PropertySet &properties)
{
    d->validProperties = properties;
}

void Group::resetModifiedProperties()
{
    d->modifiedProperties.clear();
}

void Group::setId(int id)
{
    d->id = id;
    d->propertyChanged(Group::Id);
}

void Group::setLocalUid(const QString &uid)
{
    d->localUid = uid;
    d->propertyChanged(Group::LocalUid);
}

void Group::setRemoteUids(const QStringList &uids)
{
    d->remoteUids = uids;
    d->propertyChanged(Group::RemoteUids);
}

void Group::setChatType(Group::ChatType chatType)
{
    d->chatType = chatType;
    d->propertyChanged(Group::Type);
}

void Group::setChatName(const QString &name)
{
    d->chatName = name;
    d->propertyChanged(Group::ChatName);
}

void Group::setEndTime(const QDateTime &endTime)
{
    d->endTime = endTime;
    d->propertyChanged(Group::EndTime);
}

void Group::setTotalMessages(int total)
{
    d->totalMessages = total;
    d->propertyChanged(Group::TotalMessages);
}

void Group::setUnreadMessages(int unread)
{
    d->unreadMessages = unread;
    d->propertyChanged(Group::UnreadMessages);
}

void Group::setSentMessages(int sent)
{
    d->sentMessages = sent;
    d->propertyChanged(Group::SentMessages);
}

void Group::setLastEventId(int id)
{
    d->lastEventId = id;
    d->propertyChanged(Group::LastEventId);
}

void Group::setContactId(int id)
{
    d->contactId = id;
    d->propertyChanged(Group::ContactId);
}

void Group::setContactName(const QString &name)
{
    d->contactName = name;
    d->propertyChanged(Group::ContactName);
}

void Group::setLastMessageText(const QString &text)
{
    d->lastMessageText = text;
    d->propertyChanged(Group::LastMessageText);
}

void Group::setLastVCardFileName(const QString &filename)
{
    d->lastVCardFileName = filename;
    d->propertyChanged(Group::LastVCardFileName);
}

void Group::setLastVCardLabel(const QString &label)
{
    d->lastVCardLabel = label;
    d->propertyChanged(Group::LastVCardLabel);
}

void Group::setLastEventType(Event::EventType eventType)
{
    d->lastEventType = eventType;
    d->propertyChanged(Group::LastEventType);
}

void Group::setLastEventStatus(Event::EventStatus eventStatus)
{
    d->lastEventStatus = eventStatus;
    d->propertyChanged(Group::LastEventStatus);
}

void Group::setPermanent(bool permanent)
{
    d->isPermanent = permanent;
    d->propertyChanged(Group::IsPermanent);
}

void Group::setLastModified(const QDateTime &modified)
{
    d->lastModified = modified;
    d->propertyChanged(Group::LastModified);
}

QDBusArgument &operator<<(QDBusArgument &argument, const Group &group)
{
    argument.beginStructure();
    argument << group.id() << group.localUid() << group.remoteUids()
             << group.chatType() << group.chatName()
             << group.endTime() << group.totalMessages()
             << group.unreadMessages() << group.sentMessages()
             << group.lastEventId() << group.contactId() << group.contactName()
             << group.lastMessageText() << group.lastVCardFileName()
             << group.lastVCardLabel() << group.lastEventType()
             << group.lastEventStatus() << group.isPermanent() << group.lastModified();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Group &group)
{
    GroupPrivate p;
    int type, status;
    uint chatType;

    argument.beginStructure();
    argument >> p.id >> p.localUid >> p.remoteUids >> chatType
             >> p.chatName >> p.endTime
             >> p.totalMessages >> p.unreadMessages >> p.sentMessages
             >> p.lastEventId >> p.contactId >> p.contactName
             >> p.lastMessageText >> p.lastVCardFileName >> p.lastVCardLabel
             >> type >> status >> p.isPermanent >> p.lastModified;
    argument.endStructure();

    group.setId(p.id);
    group.setLocalUid(p.localUid);
    group.setRemoteUids(p.remoteUids);
    group.setChatType((Group::ChatType)chatType);
    group.setChatName(p.chatName);
    group.setEndTime(p.endTime);
    group.setTotalMessages(p.totalMessages);
    group.setUnreadMessages(p.unreadMessages);
    group.setSentMessages(p.sentMessages);
    group.setLastEventId(p.lastEventId);
    group.setContactId(p.contactId);
    group.setContactName(p.contactName);
    group.setLastMessageText(p.lastMessageText);
    group.setLastVCardFileName(p.lastVCardFileName);
    group.setLastVCardLabel(p.lastVCardLabel);
    group.setLastEventType((Event::EventType)type);
    group.setLastEventStatus((Event::EventStatus)status);
    group.setPermanent(p.isPermanent);
    group.setLastModified(p.lastModified);

    group.resetModifiedProperties();

    return argument;
}