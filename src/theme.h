#ifndef THEME_H
#define THEME_H

#include <QString>

class Theme
{
public:
    static QString styleSheet(const QString &themeName, const QString &startupStyleSheet);
};

#endif // THEME_H
