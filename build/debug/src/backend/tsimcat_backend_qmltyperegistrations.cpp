/****************************************************************************
** Generated QML type registration code
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if __has_include(<Backend.h>)
#  include <Backend.h>
#endif


#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_TsimCAT_Backend()
{
    QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    qmlRegisterTypesAndRevisions<Backend>("TsimCAT.Backend", 1);
    QT_WARNING_POP
    qmlRegisterModule("TsimCAT.Backend", 1, 0);
}

static const QQmlModuleRegistration tsimCATBackendRegistration("TsimCAT.Backend", qml_register_types_TsimCAT_Backend);
