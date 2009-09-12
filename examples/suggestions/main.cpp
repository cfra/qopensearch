#include <qapplication.h>

#include "opensearchengine.h"
#include "opensearchreader.h"
#include "helperobject.h"

#include <qfile.h>
#include <qnetworkaccessmanager.h>
#include <qobject.h>
#include <qtextstream.h>
#include <qtimer.h>

#include <cstdio>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QStringList args = app.arguments();

    QTextStream stream(stdout);

    if (args.count() < 3) {
        stream << QString("Usage: %1 filepath searchterm").arg(args.at(0)) << '\n';
        return 1;
    }

    QString filePath = args.at(1);
    QString searchTerm = args.at(2);

    if (!QFile::exists(filePath)) {
        stream << QString("File %1 does not exist.").arg(filePath) << '\n';
        return 1;
    }

    QFile file(filePath);
    file.open(QIODevice::ReadOnly);

    QNetworkAccessManager manager;

    OpenSearchReader reader;
    OpenSearchEngine *engine = reader.read(&file);
    if (reader.hasError()) {
        stream << QString("Error: %1").arg(reader.errorString()) << '\n';
        return 1;
    }

    if (!engine->isValid()) {
        stream << "The OpenSearch description is invalid." << '\n';
        return 1;
    }

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(10 * 1000);

    HelperObject helper(&app, &stream);
    engine->setNetworkAccessManager(&manager);
    engine->requestSuggestions(searchTerm);
    timer.start();
    QObject::connect(engine, SIGNAL(suggestions(const QStringList &)),
                     &helper, SLOT(printSuggestions(const QStringList &)));
    QObject::connect(&timer, SIGNAL(timeout()),
                     &app, SLOT(quit()));
    app.exec();
}