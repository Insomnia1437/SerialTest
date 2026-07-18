#include "appstyle.h"

#include <QFile>
#include <QTextStream>

QString AppStyle::styleSheet(const QString& themeName, const QString& fallback)
{
    if(themeName == "qdss_dark")
        return readStyleSheet(":/qdarkstyle/dark/darkstyle.qss")
            + readStyleSheet(":/theme/modern-dark.qss");

    if(themeName == "qdss_light")
        return readStyleSheet(":/qdarkstyle/light/lightstyle.qss")
            + readStyleSheet(":/theme/modern-light.qss");

    return fallback + readStyleSheet(":/theme/modern-light.qss");
}

QString AppStyle::readStyleSheet(const QString& path)
{
    QFile file(path);
    if(!file.open(QFile::ReadOnly | QFile::Text))
        return {};

    QTextStream stream(&file);
    return stream.readAll();
}
