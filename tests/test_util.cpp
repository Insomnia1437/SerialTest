#include "util.h"
#include "sessiondata.h"

#include <QTextCodec>
#include <QtTest>

class UtilTest : public QObject
{
    Q_OBJECT

private slots:
    void unescapesControlCharacters();
    void unescapesNumericCharacters();
    void unescapesUnicodeCharacters();
    void sessionMergesNearbyReceiveMetadata();
    void sessionSeparatesReceiveMetadataAndClearsPendingData();
    void sessionTracksAndClearsSentData();
};

void UtilTest::unescapesControlCharacters()
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QVERIFY(codec != nullptr);
    QCOMPARE(Util::unescape(QStringLiteral("AT\\r\\n"), codec), QByteArray("AT\r\n"));
}

void UtilTest::unescapesNumericCharacters()
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QVERIFY(codec != nullptr);
    QCOMPARE(Util::unescape(QStringLiteral("\\x41\\101"), codec), QByteArray("AA"));
}

void UtilTest::unescapesUnicodeCharacters()
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QVERIFY(codec != nullptr);
    QCOMPARE(Util::unescape(QStringLiteral("\\u4F60\\u597D"), codec), QStringLiteral("你好").toUtf8());
}

void UtilTest::sessionMergesNearbyReceiveMetadata()
{
    SessionData session;
    session.appendReceived("abc", 1000, true, 10);
    session.appendReceived("de", 1005, true, 10);

    QCOMPARE(session.receivedData(), QByteArray("abcde"));
    QCOMPARE(session.receivedCount(), 5);
    QCOMPARE(session.receivedMetadata().size(), 1);
    QCOMPARE(session.receivedMetadata().constFirst().len, 5);

    const SessionData::ReceiveBatch batch = session.takePendingReceived();
    QCOMPARE(batch.data, QByteArray("abcde"));
    QCOMPARE(batch.metadata.size(), 1);
    QVERIFY(session.takePendingReceived().isEmpty());
}

void UtilTest::sessionTracksAndClearsSentData()
{
    SessionData session;
    session.appendSent("hello", 5, true);
    session.appendSent("!", 1, false);

    QCOMPARE(session.sentData(), QByteArray("hello"));
    QCOMPARE(session.sentCount(), 6);

    session.clearSent();
    QVERIFY(session.sentData().isEmpty());
    QCOMPARE(session.sentCount(), 0);
}

void UtilTest::sessionSeparatesReceiveMetadataAndClearsPendingData()
{
    SessionData session;
    session.appendReceived("abc", 1000, true, 10);
    session.appendReceived("de", 1010, true, 10);

    QCOMPARE(session.receivedMetadata().size(), 2);
    QCOMPARE(session.receivedMetadata().at(1).pos, 3);

    session.clearReceived();
    QVERIFY(session.receivedData().isEmpty());
    QVERIFY(session.receivedMetadata().isEmpty());
    QCOMPARE(session.receivedCount(), 0);
    QVERIFY(session.takePendingReceived().isEmpty());
}

QTEST_MAIN(UtilTest)
#include "test_util.moc"
