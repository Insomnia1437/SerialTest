#include <QApplication>
#include <QTest>
#include <QTemporaryDir>
#include <QCommandLineParser>
#include <QTranslator>
#include <QDir>
#include <QDockWidget>
#include <QPushButton>

#include "mainwindow.h"
#include "mysettings.h"

class UiSmokeTest : public QObject
{
    Q_OBJECT

public:
    UiSmokeTest(MainWindow* mainWindow, const QString& theme, const QString& lang, const QString& screenshotDir, const QString& prefix = QString())
        : m_window(mainWindow), m_theme(theme), m_lang(lang), m_screenshotDir(screenshotDir), m_prefix(prefix)
    {
    }

private slots:
    void testSwitchTabs()
    {
        for(int i = 0; i < 6; ++i)
        {
            m_window->showUpTab(i);
            QTest::qWait(50);
        }
    }

    void testDockFloatAndBack()
    {
        m_window->showUpTab(0);
        QTest::qWait(50);
        QDockWidget* dock = m_window->findChild<QDockWidget*>("DeviceTabDockWidget");
        QVERIFY(dock != nullptr);
        dock->setFloating(true);
        QTest::qWait(50);
        QVERIFY(dock->isFloating());

        QPushButton* backBtn = dock->findChild<QPushButton*>("dockBackButton");
        QVERIFY(backBtn != nullptr);
        backBtn->click();
        QTest::qWait(50);
        QVERIFY(!dock->isFloating());
    }

    void testContentVisible()
    {
        m_window->showUpTab(1);
        QTest::qWait(50);
        QWidget* receivedEdit = m_window->findChild<QWidget*>("receivedEdit");
        QVERIFY(receivedEdit != nullptr);
        QVERIFY(receivedEdit->isVisible());

        m_window->showUpTab(2);
        QTest::qWait(50);
        QWidget* qcpWidget = m_window->findChild<QWidget*>("qcpWidget");
        QVERIFY(qcpWidget != nullptr);
        QVERIFY(qcpWidget->isVisible());
    }

    void testThemeSwitchCycle()
    {
        m_window->onThemeChanged("qdss_light");
        QTest::qWait(50);
        QString lightQss1 = qApp->styleSheet();
        QVERIFY(!lightQss1.isEmpty());

        m_window->onThemeChanged("qdss_dark");
        QTest::qWait(50);
        QString darkQss = qApp->styleSheet();
        QVERIFY(!darkQss.isEmpty());
        QVERIFY(darkQss != lightQss1);

        m_window->onThemeChanged("(none)");
        QTest::qWait(50);
        QString noneQss = qApp->styleSheet();
        QVERIFY(noneQss != lightQss1);
        QVERIFY(noneQss != darkQss);

        m_window->onThemeChanged("qdss_light");
        QTest::qWait(50);
        QString lightQss2 = qApp->styleSheet();
        QCOMPARE(lightQss1, lightQss2);

        m_window->onThemeChanged(m_theme);
        QTest::qWait(50);
    }

    void captureScreenshots()
    {
        if(m_screenshotDir.isEmpty())
        {
            return;
        }

        QDir().mkpath(m_screenshotDir);
        const QString pageNames[6] = {
            "0_connect",
            "1_data",
            "2_plot",
            "3_control",
            "4_file",
            "5_settings"
        };

        QString cleanTheme = m_theme;
        if(cleanTheme == "(none)")
        {
            cleanTheme = "none";
        }
        QString prefix = m_prefix.isEmpty() ? QString("%1_%2").arg(m_lang, cleanTheme) : m_prefix;

        for(int i = 0; i < 6; ++i)
        {
            m_window->showUpTab(i);
            QTest::qWait(100);
            QApplication::processEvents();
            QPixmap pixmap = m_window->grab();
            QString fileName = QString("%1_page%2.png").arg(prefix, pageNames[i]);
            QString filePath = QDir(m_screenshotDir).filePath(fileName);
            pixmap.save(filePath, "PNG");
        }
    }

private:
    MainWindow* m_window;
    QString m_theme;
    QString m_lang;
    QString m_screenshotDir;
    QString m_prefix;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"theme", "Theme: (none), qdss_light, qdss_dark", "theme", "(none)"});
    parser.addOption({"lang", "Language: en, zh_CN", "lang", "en"});
    parser.addOption({"screenshot-dir", "Screenshot output directory", "dir", "ui-screenshots"});
    parser.addOption({"width", "Window width", "width", "1200"});
    parser.addOption({"height", "Window height", "height", "760"});
    parser.addOption({"prefix", "Screenshot file prefix", "prefix", ""});
    parser.addOption({"font-size", "Font point size", "pt", "0"});
    parser.process(app);

    QString theme = parser.value("theme");
    QString lang = parser.value("lang");
    QString screenshotDir = parser.value("screenshot-dir");
    QString prefix = parser.value("prefix");
    int width = parser.value("width").toInt();
    int height = parser.value("height").toInt();
    int fontSize = parser.value("font-size").toInt();
    if(width <= 0)
    {
        width = 1200;
    }
    if(height <= 0)
    {
        height = 760;
    }

    QTemporaryDir tempDir;
    QString configPath = tempDir.filePath("smoke_pref.ini");
    MySettings::init(QSettings::IniFormat, configPath);
    MySettings* settings = MySettings::defaultSettings();
    settings->setValue("Theme_Name", theme);
    if(fontSize > 0)
    {
        settings->setValue("Font_Size", fontSize);
        QFont f = app.font();
        f.setPointSize(fontSize);
        app.setFont(f);
    }

    QTranslator translator;
    if(lang == "zh_CN")
    {
        translator.load(":/i18n/SerialTest_zh_CN.qm");
        app.installTranslator(&translator);
    }

    MainWindow w;
    w.resize(width, height);
    w.onThemeChanged(theme);
    w.show();
    QTest::qWaitForWindowExposed(&w);

    UiSmokeTest test(&w, theme, lang, screenshotDir, prefix);
    return QTest::qExec(&test);
}

#include "ui_smoke.moc"
