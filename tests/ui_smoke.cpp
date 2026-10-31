#include <QApplication>
#include <QTest>
#include <QTemporaryDir>
#include <QCommandLineParser>
#include <QTranslator>
#include <QDir>
#include <QDockWidget>
#include <QPushButton>
#include <QComboBox>
#include <QStackedWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QFontComboBox>
#include <QSpinBox>
#include <QJsonObject>

#include "mainwindow.h"
#include "datatab.h"
#include "plottab.h"
#include "controlitem.h"
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

    void testProtocolSwitch()
    {
        m_window->showUpTab(0);
        QTest::qWait(50);
        QComboBox* typeBox = m_window->findChild<QComboBox*>("typeBox");
        QVERIFY(typeBox != nullptr);
        QStackedWidget* targetListStack = m_window->findChild<QStackedWidget*>("targetListStack");
        QStackedWidget* argsStack = m_window->findChild<QStackedWidget*>("argsStack");
        QVERIFY(targetListStack != nullptr);
        QVERIFY(argsStack != nullptr);

        for(int i = 0; i < typeBox->count(); ++i)
        {
            typeBox->setCurrentIndex(i);
            QTest::qWait(50);
            QVERIFY(targetListStack->currentWidget() != nullptr);
            QVERIFY(argsStack->currentWidget() != nullptr);
        }
        typeBox->setCurrentIndex(0);
        QTest::qWait(50);
    }

    void testDataTabInteractions()
    {
        m_window->showUpTab(1);
        QTest::qWait(50);
        QPlainTextEdit* receivedEdit = m_window->findChild<QPlainTextEdit*>("receivedEdit");
        QPlainTextEdit* sendedEdit = m_window->findChild<QPlainTextEdit*>("sendedEdit");
        QVERIFY(receivedEdit != nullptr);
        QVERIFY(sendedEdit != nullptr);

        DataTab* dataTab = m_window->findChild<DataTab*>();
        QVERIFY(dataTab != nullptr);

        dataTab->appendReceivedData("Device ready. Temp: 24.5C, Humidity: 60%\n", {});
        dataTab->appendSendedData("AT+INFO\r\n");
        QTest::qWait(50);

        QVERIFY(!receivedEdit->toPlainText().isEmpty());
        QVERIFY(!sendedEdit->toPlainText().isEmpty());

        QPushButton* receivedCopyButton = m_window->findChild<QPushButton*>("receivedCopyButton");
        QVERIFY(receivedCopyButton != nullptr);
        receivedCopyButton->click();

        QCheckBox* receivedHexBox = m_window->findChild<QCheckBox*>("receivedHexBox");
        QVERIFY(receivedHexBox != nullptr);
        receivedHexBox->click();
        QTest::qWait(20);
        receivedHexBox->click();
        QTest::qWait(20);
    }

    void testPlotTabInteractions()
    {
        m_window->showUpTab(2);
        QTest::qWait(50);
        PlotTab* plotTab = m_window->findChild<PlotTab*>();
        QVERIFY(plotTab != nullptr);

        QCheckBox* plot_enaBox = m_window->findChild<QCheckBox*>("plot_enaBox");
        QVERIFY(plot_enaBox != nullptr);
        plot_enaBox->setChecked(true);
        QTest::qWait(50);

        plotTab->newData("12.5\n25.0\n18.3\n35.7\n22.1\n45.0\n30.2\n");
        QTest::qWait(100);

        QPushButton* fitX = m_window->findChild<QPushButton*>("plot_fitXButton");
        QPushButton* fitY = m_window->findChild<QPushButton*>("plot_fitYButton");
        QVERIFY(fitX != nullptr);
        QVERIFY(fitY != nullptr);
        fitX->click();
        fitY->click();
        QTest::qWait(50);

        QCheckBox* plot_advancedBox = m_window->findChild<QCheckBox*>("plot_advancedBox");
        QWidget* plot_advancedWidget = m_window->findChild<QWidget*>("plot_advancedWidget");
        QVERIFY(plot_advancedBox != nullptr);
        QVERIFY(plot_advancedWidget != nullptr);

        plot_advancedBox->setChecked(true);
        QTest::qWait(50);
        QVERIFY(plot_advancedWidget->isVisible());

        plot_advancedBox->setChecked(false);
        QTest::qWait(50);
        QVERIFY(!plot_advancedWidget->isVisible());
    }

    void testControlTabInteractions()
    {
        m_window->showUpTab(3);
        QTest::qWait(50);

        QPushButton* addCmdBtn = m_window->findChild<QPushButton*>("ctrl_addCMDButton");
        QPushButton* addSliderBtn = m_window->findChild<QPushButton*>("ctrl_addSliderButton");
        QPushButton* addCheckBoxBtn = m_window->findChild<QPushButton*>("ctrl_addCheckBoxButton");
        QPushButton* addSpinBoxBtn = m_window->findChild<QPushButton*>("ctrl_addSpinBoxButton");
        QVERIFY(addCmdBtn != nullptr);
        QVERIFY(addSliderBtn != nullptr);
        QVERIFY(addCheckBoxBtn != nullptr);
        QVERIFY(addSpinBoxBtn != nullptr);

        QWidget* ctrlContents = m_window->findChild<QWidget*>("ctrl_itemContents");
        QVERIFY(ctrlContents != nullptr);

        int countBefore = ctrlContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly).count();
        addCmdBtn->click();
        addSliderBtn->click();
        addCheckBoxBtn->click();
        addSpinBoxBtn->click();
        QTest::qWait(50);

        QList<ControlItem*> items = ctrlContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
        QCOMPARE(items.count(), countBefore + 4);

        ControlItem* item0 = items.at(countBefore);
        QPushButton* confBtn0 = item0->findChild<QPushButton*>("confButton");
        QWidget* confGrp0 = item0->findChild<QWidget*>("confGrp");
        QVERIFY(confBtn0 != nullptr);
        QVERIFY(confGrp0 != nullptr);

        confBtn0->click();
        QTest::qWait(50);
        QVERIFY(confGrp0->isVisible());

        QLineEdit* nameEdit = item0->findChild<QLineEdit*>("nameEdit");
        QLineEdit* cmdEdit = item0->findChild<QLineEdit*>("CMDEdit");
        QCheckBox* prefixBox = item0->findChild<QCheckBox*>("prefixBox");
        QLineEdit* prefixEdit = item0->findChild<QLineEdit*>("prefixEdit");
        QCheckBox* suffixBox = item0->findChild<QCheckBox*>("suffixBox");
        QLineEdit* suffixEdit = item0->findChild<QLineEdit*>("suffixEdit");
        QVERIFY(nameEdit != nullptr);
        QVERIFY(cmdEdit != nullptr);
        QVERIFY(prefixBox != nullptr);
        QVERIFY(prefixEdit != nullptr);
        QVERIFY(suffixBox != nullptr);
        QVERIFY(suffixEdit != nullptr);

        nameEdit->setText("TestCMD");
        cmdEdit->setText("AT+PING");
        prefixBox->setChecked(true);
        prefixEdit->setText(">>");
        suffixBox->setChecked(true);
        suffixEdit->setText("\\r\\n");

        QJsonObject saved = item0->save();
        QCOMPARE(saved["name"].toString(), QString("TestCMD"));
        QCOMPARE(saved["content"].toString(), QString("AT+PING"));
        QCOMPARE(saved["prefixEnabled"].toBool(), true);
        QCOMPARE(saved["prefix"].toString(), QString(">>"));
        QCOMPARE(saved["suffixEnabled"].toBool(), true);
        QCOMPARE(saved["suffix"].toString(), QString("\\r\\n"));

        ControlItem reloadItem(ControlItem::Command);
        reloadItem.load(saved);
        QJsonObject reSaved = reloadItem.save();
        QCOMPARE(saved, reSaved);
    }

    void testFileTabInteractions()
    {
        m_window->showUpTab(4);
        QTest::qWait(50);

        QRadioButton* sendMode = m_window->findChild<QRadioButton*>("sendModeButton");
        QRadioButton* recvMode = m_window->findChild<QRadioButton*>("receiveModeButton");
        QPushButton* tipsBtn = m_window->findChild<QPushButton*>("tipsButton");
        QPushButton* tipsBackBtn = m_window->findChild<QPushButton*>("tipsBackButton");
        QStackedWidget* stackedWidget = m_window->findChild<QStackedWidget*>("stackedWidget");

        QVERIFY(sendMode != nullptr);
        QVERIFY(recvMode != nullptr);
        QVERIFY(tipsBtn != nullptr);
        QVERIFY(tipsBackBtn != nullptr);
        QVERIFY(stackedWidget != nullptr);

        recvMode->click();
        QTest::qWait(50);
        sendMode->click();
        QTest::qWait(50);

        tipsBtn->click();
        QTest::qWait(50);
        QCOMPARE(stackedWidget->currentIndex(), 1);

        tipsBackBtn->click();
        QTest::qWait(50);
        QCOMPARE(stackedWidget->currentIndex(), 0);
    }

    void testSettingsTabInteractions()
    {
        m_window->showUpTab(5);
        QTest::qWait(50);

        QComboBox* themeBox = m_window->findChild<QComboBox*>("Theme_nameBox");
        QFontComboBox* fontBox = m_window->findChild<QFontComboBox*>("Font_nameBox");
        QSpinBox* fontSizeBox = m_window->findChild<QSpinBox*>("Font_sizeBox");
        QPushButton* fontSetBtn = m_window->findChild<QPushButton*>("Font_setButton");

        QVERIFY(themeBox != nullptr);
        QVERIFY(fontBox != nullptr);
        QVERIFY(fontSizeBox != nullptr);
        QVERIFY(fontSetBtn != nullptr);

        int origSize = fontSizeBox->value();
        fontSizeBox->setValue(12);
        fontSetBtn->click();
        QTest::qWait(50);

        fontSizeBox->setValue(origSize);
        fontSetBtn->click();
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
            if(i == 1)
            {
                DataTab* dataTab = m_window->findChild<DataTab*>();
                if(dataTab)
                {
                    dataTab->appendReceivedData("Ready for data transfer.\nSensor [0]: Temp=23.4C, Humidity=52%\nSensor [1]: Temp=24.1C, Humidity=50%\n[RAW HEX]: 55 AA 01 02 03 FF 0D 0A\n", {});
                    dataTab->appendSendedData("AT+GET_DATA\r\n01 03 00 00 00 02 C4 0B\r\n");
                }
                QLineEdit* sendEdit = m_window->findChild<QLineEdit*>("sendEdit");
                if(sendEdit)
                {
                    sendEdit->setText("AT+STATUS");
                }
            }
            else if(i == 2)
            {
                PlotTab* plotTab = m_window->findChild<PlotTab*>();
                QCheckBox* plot_enaBox = m_window->findChild<QCheckBox*>("plot_enaBox");
                QPushButton* fitX = m_window->findChild<QPushButton*>("plot_fitXButton");
                QPushButton* fitY = m_window->findChild<QPushButton*>("plot_fitYButton");
                if(plotTab && plot_enaBox)
                {
                    plot_enaBox->setChecked(true);
                    plotTab->newData("10.0\n22.5\n15.3\n38.2\n28.4\n45.0\n32.1\n55.4\n40.0\n");
                    QTest::qWait(100);
                    if(fitX) fitX->click();
                    if(fitY) fitY->click();
                }
            }
            else if(i == 3)
            {
                QWidget* ctrlContents = m_window->findChild<QWidget*>("ctrl_itemContents");
                if(ctrlContents)
                {
                    QList<ControlItem*> items = ctrlContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
                    if(items.isEmpty())
                    {
                        QPushButton* addCmdBtn = m_window->findChild<QPushButton*>("ctrl_addCMDButton");
                        QPushButton* addSliderBtn = m_window->findChild<QPushButton*>("ctrl_addSliderButton");
                        QPushButton* addCheckBoxBtn = m_window->findChild<QPushButton*>("ctrl_addCheckBoxButton");
                        QPushButton* addSpinBoxBtn = m_window->findChild<QPushButton*>("ctrl_addSpinBoxButton");
                        if(addCmdBtn) addCmdBtn->click();
                        if(addSliderBtn) addSliderBtn->click();
                        if(addCheckBoxBtn) addCheckBoxBtn->click();
                        if(addSpinBoxBtn) addSpinBoxBtn->click();
                        items = ctrlContents->findChildren<ControlItem*>(QString(), Qt::FindDirectChildrenOnly);
                    }
                    if(!items.isEmpty())
                    {
                        QPushButton* confBtn = items.at(0)->findChild<QPushButton*>("confButton");
                        QWidget* confGrp = items.at(0)->findChild<QWidget*>("confGrp");
                        if(confBtn && confGrp && !confGrp->isVisible())
                        {
                            confBtn->click();
                        }
                    }
                }
            }
            else if(i == 4)
            {
                QLineEdit* filePathEdit = m_window->findChild<QLineEdit*>("filePathEdit");
                if(filePathEdit)
                {
                    filePathEdit->setText("/path/to/firmware_v1.0.bin");
                }
            }
            QTest::qWait(100);
            QApplication::processEvents();
            QPixmap pixmap = m_window->grab();
            QString fileName = QString("%1_page%2.png").arg(prefix, pageNames[i]);
            QString filePath = QDir(m_screenshotDir).filePath(fileName);
            pixmap.save(filePath, "PNG");

            if(i == 2)
            {
                QCheckBox* plot_advancedBox = m_window->findChild<QCheckBox*>("plot_advancedBox");
                if(plot_advancedBox)
                {
                    plot_advancedBox->setChecked(true);
                    QTest::qWait(100);
                    QApplication::processEvents();
                    QPixmap advPixmap = m_window->grab();
                    QString advFileName = QString("%1_page2_plot_advanced.png").arg(prefix);
                    advPixmap.save(QDir(m_screenshotDir).filePath(advFileName), "PNG");
                    plot_advancedBox->setChecked(false);
                    QTest::qWait(50);
                }
            }

            if(i == 0)
            {
                QStackedWidget* targetListStack = m_window->findChild<QStackedWidget*>("targetListStack");
                QStackedWidget* argsStack = m_window->findChild<QStackedWidget*>("argsStack");
                QWidget* bleListPage = m_window->findChild<QWidget*>("BLECentralListPage");
                QWidget* bleArgsPage = m_window->findChild<QWidget*>("BLECentralArgsPage");
                QWidget* spListPage = m_window->findChild<QWidget*>("SPListPage");
                QWidget* spArgsPage = m_window->findChild<QWidget*>("SPArgsPage");
                if(targetListStack && argsStack && bleListPage && bleArgsPage)
                {
                    targetListStack->setCurrentWidget(bleListPage);
                    argsStack->setCurrentWidget(bleArgsPage);
                    QTest::qWait(100);
                    QApplication::processEvents();
                    QPixmap blePixmap = m_window->grab();
                    QString bleFileName = QString("%1_page0_ble_preview.png").arg(prefix);
                    blePixmap.save(QDir(m_screenshotDir).filePath(bleFileName), "PNG");

                    if(spListPage && spArgsPage)
                    {
                        targetListStack->setCurrentWidget(spListPage);
                        argsStack->setCurrentWidget(spArgsPage);
                        QTest::qWait(50);
                    }
                }
            }
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
