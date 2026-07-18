#ifndef SESSIONDATA_H
#define SESSIONDATA_H

#include "metadata.h"

#include <QByteArray>
#include <QVector>

class SessionData
{
public:
    struct ReceiveBatch
    {
        QByteArray data;
        QVector<Metadata> metadata;

        bool isEmpty() const { return data.isEmpty(); }
    };

    const QByteArray& receivedData() const { return m_receivedData; }
    const QVector<Metadata>& receivedMetadata() const { return m_receivedMetadata; }
    const QByteArray& sentData() const { return m_sentData; }

    qint64 receivedCount() const { return m_receivedCount; }
    qint64 sentCount() const { return m_sentCount; }

    void appendReceived(const QByteArray& data, qint64 timestamp, bool mergeTimestamp, int mergeInterval);
    void appendSent(const QByteArray& data, qint64 writtenBytes, bool recordData);
    ReceiveBatch takePendingReceived();

    void clearReceived();
    void clearSent();

private:
    QByteArray m_receivedData;
    QVector<Metadata> m_receivedMetadata;
    QByteArray m_sentData;

    QByteArray m_pendingReceivedData;
    QVector<Metadata> m_pendingReceivedMetadata;

    qint64 m_receivedCount = 0;
    qint64 m_sentCount = 0;
};

#endif // SESSIONDATA_H
