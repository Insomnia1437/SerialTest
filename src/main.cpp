#include "mainwindow.h"
#include "mysettings.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QTimer>
#include <QDebug>
#include <QDockWidget>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>

int main(int argc, char *argv[])
{
    QString screenshotPath;
    bool verifyDetachablePanels = false;
    bool verifySettingsScroll = false;
    int initialPanel = -1;
#ifndef Q_OS_ANDROID
    // A trick to handle non-ascii path
    // The application cannot find the plugins when the path contains non ascii characters.
    // However, the plugins will be load after creating MainWindow(or QApplication?).
    // QDir will handle the path correctly.
    QDir* pluginDir = new QDir;
    if(pluginDir->cd("plugins")) // has plugins folder
    {
        qputenv("QT_PLUGIN_PATH", pluginDir->absolutePath().toLocal8Bit());
    }
    delete pluginDir;
#endif

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Insomnia1437");
    QCoreApplication::setApplicationName("SerialTest");

#ifdef Q_OS_ANDROID
    // on Android, use default.
    MySettings::init(QSettings::NativeFormat);
#else

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"config-path",
                      "Use specified file as config file",
                      "file path"});
    parser.addOption({"screenshot",
                      "Save a main-window screenshot and exit (for UI smoke tests)",
                      "file path"});
    parser.addOption({"verify-detachable-panels",
                      "Verify Data and Plot detach/reattach behavior and exit"});
    parser.addOption({"verify-settings-scroll",
                      "Verify that settings controls do not consume scrolling and exit"});
    parser.addOption({"initial-panel",
                      "Open a panel by zero-based index",
                      "index"});
    parser.process(a);
    screenshotPath = parser.value("screenshot");
    verifyDetachablePanels = parser.isSet("verify-detachable-panels");
    verifySettingsScroll = parser.isSet("verify-settings-scroll");
    bool panelIndexValid = false;
    const int requestedPanel = parser.value("initial-panel").toInt(&panelIndexValid);
    if(panelIndexValid)
        initialPanel = requestedPanel;

    // on PC, store preferences in files for portable use
    if(parser.isSet("config-path"))
    {
        qDebug() << "Config file path:" << parser.value("config-path");
        MySettings::init(QSettings::IniFormat, parser.value("config-path"));
    }
    else
    {
        // Keep using an existing portable config, but store new configs in the
        // platform's writable application config directory. A GUI application
        // cannot assume that its launch working directory is writable or stable.
        QString configPath = "preference.ini";
        if(!QFileInfo::exists(configPath))
        {
            QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
            configDir.mkpath(".");
            configPath = configDir.filePath("preference.ini");
        }
        MySettings::init(QSettings::IniFormat, configPath);
    }

#endif

    MySettings* m_settings = MySettings::defaultSettings();
    if(m_settings == nullptr)
    {
        qCritical() << "Failed to initialize application settings.";
        return 2;
    }

    m_settings = nullptr;

    MainWindow w;
    w.show();
    if(initialPanel >= 0)
        w.showUpTab(initialPanel);

    if(verifyDetachablePanels)
    {
        QTimer::singleShot(500, &w, [&a, &w]() {
            bool passed = true;
            const QStringList dockNames{"DataTabDockWidget", "PlotTabDockWidget"};
            for(const QString& dockName : dockNames)
            {
                QDockWidget* dock = w.findChild<QDockWidget*>(dockName);
                QPushButton* button = dock ? dock->findChild<QPushButton*>("dockWindowButton") : nullptr;
                if(!dock || !button)
                {
                    passed = false;
                    break;
                }

                button->click();
                QApplication::processEvents();
                passed = dock->isFloating();
                button->click();
                QApplication::processEvents();
                passed = passed && !dock->isFloating();
                if(!passed)
                    break;
            }
            a.exit(passed ? 0 : 3);
        });
    }

    if(verifySettingsScroll)
    {
        w.showUpTab(5);
        QTimer::singleShot(500, &w, [&a, &w]() {
            QComboBox* themeBox = w.findChild<QComboBox*>("Theme_nameBox");
            QScrollArea* scrollArea = w.findChild<QScrollArea*>("scrollArea");
            if(!themeBox || !scrollArea || scrollArea->verticalScrollBar()->maximum() <= 0)
            {
                a.exit(5);
                return;
            }

            QScrollBar* scrollBar = scrollArea->verticalScrollBar();
            scrollBar->setValue(0);
            const int themeIndex = themeBox->currentIndex();
            QWheelEvent wheelEvent(
                QPointF(4, 4), QPointF(4, 4), QPoint(0, -48), QPoint(),
                Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false
            );
            QApplication::sendEvent(themeBox, &wheelEvent);

            const bool passed = themeBox->currentIndex() == themeIndex
                && scrollBar->value() > 0;
            a.exit(passed ? 0 : 5);
        });
    }

    if(!screenshotPath.isEmpty() && !verifyDetachablePanels && !verifySettingsScroll)
    {
        QTimer::singleShot(500, &w, [&a, &w, screenshotPath]() {
            const bool saved = w.grab().save(screenshotPath);
            a.exit(saved ? 0 : 2);
        });
    }

    return a.exec();
}
