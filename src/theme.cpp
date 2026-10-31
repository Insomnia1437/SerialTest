#include "theme.h"
#include <QFile>

static QString readResource(const QString &path)
{
    QFile file(path);
    file.open(QFile::ReadOnly | QFile::Text);
    return QString::fromUtf8(file.readAll());
}

QString Theme::styleSheet(const QString &themeName, const QString &startupStyleSheet)
{
    if(themeName == "(none)")
    {
        return startupStyleSheet;
    }

    QString baseQss;
    QString colorQss;
    if(themeName == "qdss_dark")
    {
        baseQss = readResource(":/qdarkstyle/dark/darkstyle.qss");
        colorQss = readResource(":/styles/dark.qss");
    }
    else if(themeName == "qdss_light")
    {
        baseQss = readResource(":/qdarkstyle/light/lightstyle.qss");
        colorQss = readResource(":/styles/light.qss");
    }

    QString commonQss = readResource(":/styles/common.qss");

    return baseQss + "\n" + colorQss + "\n" + commonQss;
}
