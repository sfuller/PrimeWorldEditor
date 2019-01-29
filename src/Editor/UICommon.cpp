#include "UICommon.h"
#include <QDesktopServices>
#include <QProcess>

namespace UICommon
{

QWindow* FindWidgetWindowHandle(QWidget *pWidget)
{
    while (pWidget && !pWidget->windowHandle())
        pWidget = pWidget->parentWidget();

    return pWidget ? pWidget->windowHandle() : nullptr;
}

void OpenContainingFolder(const QString& rkPath)
{
#if WIN32
    QStringList Args;
    Args << "/select," << QDir::toNativeSeparators(rkPath);
    QProcess::startDetached("explorer", Args);
#elif __APPLE__
    // NOTE: "APPLE" could also refer to iPhone, but I don't think iPhone will ever be supported by PWE :)
    QStringList Args;
    Args << "--reveal" << QDir::toNativeSeparators(rkPath);
    QProcess::startDetached("open", Args);
#else
#error OpenContainingFolder() not implemented!
#endif
}

bool OpenInExternalApplication(const QString& rkPath)
{
    return QDesktopServices::openUrl( QString("file:///") + QDir::toNativeSeparators(rkPath) );
}

}
