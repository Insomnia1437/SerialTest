#include "sessiondata.h"

#include <utility>

void SessionData::appendReceived(const QByteArray& data, qint64 timestamp, bool mergeTimestamp, int mergeInterval)
{
    if(data.isEmpty())
        return;

    Metadata metadata(m_receivedData.size(), data.size(), timestamp);
    if(mergeTimestamp && !m_receivedMetadata.isEmpty()
        && timestamp - m_receivedMetadata.constLast().timestamp < mergeInterval)
    {
        m_receivedMetadata.last().len += metadata.len;
    }
    else
    {
        m_receivedMetadata.append(metadata);
        m_pendingReceivedMetadata.append(metadata);
    }

    m_receivedData.append(data);
    m_pendingReceivedData.append(data);
    m_receivedCount += data.size();
}

void SessionData::appendSent(const QByteArray& data, qint64 writtenBytes, bool recordData)
{
    if(writtenBytes <= 0)
        return;

    if(recordData)
        m_sentData.append(data);
    m_sentCount += writtenBytes;
}

SessionData::ReceiveBatch SessionData::takePendingReceived()
{
    ReceiveBatch batch{std::move(m_pendingReceivedData), std::move(m_pendingReceivedMetadata)};
    m_pendingReceivedData.clear();
    m_pendingReceivedMetadata.clear();
    return batch;
}

void SessionData::clearReceived()
{
    m_receivedData.clear();
    m_receivedMetadata.clear();
    m_pendingReceivedData.clear();
    m_pendingReceivedMetadata.clear();
    m_receivedCount = 0;
}

void SessionData::clearSent()
{
    m_sentData.clear();
    m_sentCount = 0;
}
