#include <qobject.h>

#include <qapplication.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qtimer.h>

class HelperObject : public QObject
{
    Q_OBJECT

public:
    HelperObject(QObject *parent, QTextStream *stream)
        : QObject(parent)
        , m_stream(stream)
    {
    }

public slots:
    void printSuggestions(const QStringList &suggestions)
    {
        Q_ASSERT(m_stream);
        if (suggestions.isEmpty())
            *m_stream << "No suggestions." << "\n";
        else
            *m_stream << suggestions.join("\n") << "\n";

        QApplication *app = static_cast<QApplication*>(parent());
        QTimer::singleShot(0, app, SLOT(quit()));
    }

private:
    QTextStream *m_stream;
};
