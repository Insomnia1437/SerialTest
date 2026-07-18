#include "util.h"

#include <QTextCodec>
#include <QtTest>

class UtilTest : public QObject
{
    Q_OBJECT

private slots:
    void unescapesControlCharacters();
    void unescapesNumericCharacters();
    void unescapesUnicodeCharacters();
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

QTEST_MAIN(UtilTest)
#include "test_util.moc"
