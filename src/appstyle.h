#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>

class AppStyle
{
public:
    static QString styleSheet(const QString& themeName, const QString& fallback = QString());

private:
    static QString readStyleSheet(const QString& path);
};

#endif // APPSTYLE_H
