/*
 * Copyright 2009 Jakub Wieczorek <faw217@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "opensearchengine.h"

#include <qbuffer.h>
#include <qcoreapplication.h>
#include <qlocale.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>
#include <qregexp.h>
#include <qscriptengine.h>
#include <qscriptvalue.h>
#include <qstringlist.h>

/*!
    \class OpenSearchEngine
    \brief A class representing a single search engine described in OpenSearch format

    OpenSearchEngine is a class that represents a single search engine based on
    the OpenSearch format.
    For more information about the format, see http://www.opensearch.org/.

    Instances of the class hold all the data associated with the corresponding search
    engines, such as name(), description() and also URL templates that are used
    to construct URLs, which can be used later to perform search queries. Search engine
    can also have an image, even an external one, in this case it will be downloaded
    automatically from the network.

    OpenSearchEngine instances can be constructed from scratch but also read from
    external sources and written back to them. OpenSearchReader and OpenSearchWriter
    are the classes provided for reading and writing OpenSearch descriptions.

    Default constructed engines need to be filled with the necessary information before
    they can be used to peform search requests. First of all, a search engine should have
    the metadata including the name and the description.
    However, the most important are URL templates, which are the construction of URLs
    but can also contain template parameters, that are replaced with corresponding values
    at the time of constructing URLs.

    There are two types of URL templates: search URL template and suggestions URL template.
    Search URL template is needed for constructing search URLs, which point directly to
    search results. Suggestions URL template is necessary to construct suggestion queries
    URLs, which are then used for requesting contextual suggestions, a popular service
    offered along with search results that provides search terms related to what has been
    supplied by the user.

    Both types of URLs are constructed by the class, by searchUrl() and suggestionsUrl()
    functions respectively. However, search requests are supposed to be performed outside
    the class, while suggestion queries can be executed using the requestSuggestions()
    method. The class will take care of peforming the network request and parsing the
    JSON response.

    Both the image request and suggestion queries need network access. The class can
    perform network requests on its own, though the client application needs to provide
    a network access manager, which then will to be used for network operations.
    Without that, both images delivered from remote locations and contextual suggestions
    will be disabled.

    \sa OpenSearchReader, OpenSearchWriter
*/

/*!
    Constructs an engine with a given \a parent.
*/
OpenSearchEngine::OpenSearchEngine(QObject *parent)
    : QObject(parent)
    , m_networkAccessManager(0)
    , m_suggestionsReply(0)
    , m_scriptEngine(0)
{
}

/*!
    A destructor.
*/
OpenSearchEngine::~OpenSearchEngine()
{
    if (m_scriptEngine)
        m_scriptEngine->deleteLater();
}

QString OpenSearchEngine::parseTemplate(const QString &searchTerm, const QString &searchTemplate)
{
    QString language = QLocale().name();
    // Simple conversion to RFC 3066.
    language = language.replace(QLatin1Char('_'), QLatin1Char('-'));

    QString result = searchTemplate;
    result.replace(QLatin1String("{count}"), QLatin1String("20"));
    result.replace(QLatin1String("{startIndex}"), QLatin1String("0"));
    result.replace(QLatin1String("{startPage}"), QLatin1String("0"));
    result.replace(QLatin1String("{language}"), language);
    result.replace(QLatin1String("{inputEncoding}"), QLatin1String("UTF-8"));
    result.replace(QLatin1String("{outputEncoding}"), QLatin1String("UTF-8"));
    result.replace(QRegExp(QLatin1String("\\{([^\\}]*:|)source\\??\\}")), QCoreApplication::applicationName());
    result.replace(QLatin1String("{searchTerms}"), searchTerm);

    return result;
}

/*!
    \property OpenSearchEngine::name
    \brief the name of the engine

    \sa description()
*/
QString OpenSearchEngine::name() const
{
    return m_name;
}

void OpenSearchEngine::setName(const QString &name)
{
    m_name = name;
}

/*!
    \property OpenSearchEngine::description
    \brief the description of the engine

    \sa name()
*/
QString OpenSearchEngine::description() const
{
    return m_description;
}

void OpenSearchEngine::setDescription(const QString &description)
{
    m_description = description;
}

/*!
    \property OpenSearchEngine::searchUrlTemplate
    \brief the template of the search URL

    \sa searchUrl(), searchParameters(), suggestionsUrlTemplate()
*/
QString OpenSearchEngine::searchUrlTemplate() const
{
    return m_searchUrlTemplate;
}

void OpenSearchEngine::setSearchUrlTemplate(const QString &searchUrlTemplate)
{
    m_searchUrlTemplate = searchUrlTemplate;
}

/*!
    Constructs and returns a search URL with a given \a searchTerm.

    The URL template is processed according to the specification:
    http://www.opensearch.org/Specifications/OpenSearch/1.1#OpenSearch_URL_template_syntax

    A list of template parameters currently supported and what they are replaced with:
    \table
    \header \o parameter
            \o value
    \row    \o "{count}"
            \o "20"
    \row    \o "{startIndex}"
            \o "0"
    \row    \o "{startPage}"
            \o "0"
    \row    \o "{language}"
            \o "the default language code (RFC 3066)"
    \row    \o "{inputEncoding}"
            \o "UTF-8"
    \row    \o "{outputEncoding}"
            \o "UTF-8"
    \row    \o "{*:source}"
            \o "application name, QCoreApplication::applicationName()"
    \row    \o "{searchTerms}"
            \o "the string supplied by the user"
    \endtable

    \sa searchUrlTemplate(), searchParameters(), suggestionsUrl()
*/
QUrl OpenSearchEngine::searchUrl(const QString &searchTerm) const
{
    if (m_searchUrlTemplate.isEmpty())
        return QUrl();

    QUrl retVal = QUrl::fromEncoded(parseTemplate(searchTerm, m_searchUrlTemplate).toUtf8());

    Parameters::const_iterator end = m_searchParameters.constEnd();
    Parameters::const_iterator i = m_searchParameters.constBegin();
    for (; i != end; ++i)
        retVal.addQueryItem(i->first, parseTemplate(searchTerm, i->second));

    return retVal;
}

/*!
    \property providesSuggestions
    \brief indicates whether the engine supports contextual suggestions
*/
bool OpenSearchEngine::providesSuggestions() const
{
    return !m_suggestionsUrlTemplate.isEmpty();
}

/*!
    \property OpenSearchEngine::suggestionsUrlTemplate
    \brief the template of the suggestions URL

    \sa suggestionsUrl(), suggestionsParameters(), searchUrlTemplate()
*/
QString OpenSearchEngine::suggestionsUrlTemplate() const
{
    return m_suggestionsUrlTemplate;
}

void OpenSearchEngine::setSuggestionsUrlTemplate(const QString &suggestionsUrlTemplate)
{
    m_suggestionsUrlTemplate = suggestionsUrlTemplate;
}

/*!
    Constructs a suggestions URL with a given \a searchTerm.

    The URL template is processed according to the specification:
    http://www.opensearch.org/Specifications/OpenSearch/1.1#OpenSearch_URL_template_syntax

    See searchUrl() for more information about processing template parameters.

    \sa suggestionsUrlTemplate(), suggestionsParameters(), searchUrl()
*/
QUrl OpenSearchEngine::suggestionsUrl(const QString &searchTerm) const
{
    if (m_suggestionsUrlTemplate.isEmpty())
        return QUrl();

    QUrl retVal = QUrl::fromEncoded(parseTemplate(searchTerm, m_suggestionsUrlTemplate).toUtf8());

    Parameters::const_iterator end = m_suggestionsParameters.constEnd();
    Parameters::const_iterator i = m_suggestionsParameters.constBegin();
    for (; i != end; ++i)
        retVal.addQueryItem(i->first, parseTemplate(searchTerm, i->second));

    return retVal;
}

/*!
    \property searchParameters
    \brief additional parameters that will be included in the search URL

    For more information see:
    http://www.opensearch.org/Specifications/OpenSearch/Extensions/Parameter/1.0
*/
OpenSearchEngine::Parameters OpenSearchEngine::searchParameters() const
{
    return m_searchParameters;
}

void OpenSearchEngine::setSearchParameters(const Parameters &searchParameters)
{
    m_searchParameters = searchParameters;
}

/*!
    \property suggestionsParameters
    \brief additional parameters that will be included in the suggestions URL

    For more information see:
    http://www.opensearch.org/Specifications/OpenSearch/Extensions/Parameter/1.0
*/
OpenSearchEngine::Parameters OpenSearchEngine::suggestionsParameters() const
{
    return m_suggestionsParameters;
}

void OpenSearchEngine::setSuggestionsParameters(const Parameters &suggestionsParameters)
{
    m_suggestionsParameters = suggestionsParameters;
}

/*!
    \property imageUrl
    \brief the image URL of the engine

    When setting a new image URL, it won't be loaded immediately. The first request will be
    deferred until image() is called for the first time.

    \note To be able to request external images, you need to provide a network access manager,
          which will be used for network operations.

    \sa image(), networkAccessManager()
*/
QString OpenSearchEngine::imageUrl() const
{
    return m_imageUrl;
}

void OpenSearchEngine::setImageUrl(const QString &imageUrl)
{
    m_imageUrl = imageUrl;
}

void OpenSearchEngine::loadImage() const
{
    if (!m_networkAccessManager || m_imageUrl.isEmpty())
        return;

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(QUrl::fromEncoded(m_imageUrl.toUtf8())));
    connect(reply, SIGNAL(finished()), this, SLOT(imageObtained()));
}

void OpenSearchEngine::imageObtained()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply)
        return;

    QByteArray response = reply->readAll();

    reply->close();
    reply->deleteLater();

    if (response.isEmpty())
        return;

    m_image.loadFromData(response);
    emit imageChanged();
}

/*!
    \property image
    \brief the image of the engine

    When no image URL has been set and an image will be set explicitly, a new data URL
    will be constructed, holding the image data encoded with Base64.

    \sa imageUrl()
*/
QImage OpenSearchEngine::image() const
{
    if (m_image.isNull())
        loadImage();
    return m_image;
}

void OpenSearchEngine::setImage(const QImage &image)
{
    if (m_imageUrl.isEmpty()) {
        QBuffer imageBuffer;
        imageBuffer.open(QBuffer::ReadWrite);
        if (image.save(&imageBuffer, "PNG")) {
            m_imageUrl = QString(QLatin1String("data:image/png;base64,%1"))
                         .arg(QLatin1String(imageBuffer.buffer().toBase64()));
        }
    }

    m_image = image;
    emit imageChanged();
}

/*!
    \property valid
    \brief indicates whether the engine is valid i.e. the description was properly formed and included all necessary information
*/
bool OpenSearchEngine::isValid() const
{
    return (!m_name.isEmpty() && !m_searchUrlTemplate.isEmpty());
}

bool OpenSearchEngine::operator==(const OpenSearchEngine &other) const
{
    return (m_name == other.m_name
            && m_description == other.m_description
            && m_imageUrl == other.m_imageUrl
            && m_searchUrlTemplate == other.m_searchUrlTemplate
            && m_suggestionsUrlTemplate == other.m_suggestionsUrlTemplate
            && m_searchParameters == other.m_searchParameters
            && m_suggestionsParameters == other.m_suggestionsParameters);
}

bool OpenSearchEngine::operator<(const OpenSearchEngine &other) const
{
    return (m_name < other.m_name);
}

/*!
    Requests contextual suggestions on the search engine, for a given \a searchTerm.

    If succeeded, suggestions() signal will be emitted once the suggestions are received.

    \note To be able to request suggestions, you need to provide a network access manager,
          which will be used for network operations.
*/
void OpenSearchEngine::requestSuggestions(const QString &searchTerm)
{
    if (searchTerm.isEmpty() || !providesSuggestions())
        return;

    Q_ASSERT(m_networkAccessManager);

    if (!m_networkAccessManager)
        return;

    if (m_suggestionsReply) {
        m_suggestionsReply->disconnect(this);
        m_suggestionsReply->abort();
        m_suggestionsReply->deleteLater();
        m_suggestionsReply = 0;
    }

    m_suggestionsReply = m_networkAccessManager->get(QNetworkRequest(suggestionsUrl(searchTerm)));
    connect(m_suggestionsReply, SIGNAL(finished()), this, SLOT(suggestionsObtained()));
}

void OpenSearchEngine::suggestionsObtained()
{
    QString response(QString::fromUtf8(m_suggestionsReply->readAll()));
    response = response.trimmed();

    m_suggestionsReply->close();
    m_suggestionsReply->deleteLater();
    m_suggestionsReply = 0;

    if (response.isEmpty())
        return;

    if (!response.startsWith(QLatin1Char('[')) || !response.endsWith(QLatin1Char(']')))
        return;

    if (!m_scriptEngine)
        m_scriptEngine = new QScriptEngine();

    // Evaluate the JSON response using QtScript.
    if (!m_scriptEngine->canEvaluate(response))
        return;

    QScriptValue responseParts = m_scriptEngine->evaluate(response);

    if (!responseParts.property(1).isArray())
        return;

    QStringList suggestionsList;
    qScriptValueToSequence(responseParts.property(1), suggestionsList);

    emit suggestions(suggestionsList);
}

/*!
    \property networkAccessManager
    \brief the network access manager that is used to perform network requests

    It is required for network operations: loading external images and requesting
    contextual suggestions.
*/
QNetworkAccessManager *OpenSearchEngine::networkAccessManager() const
{
    return m_networkAccessManager;
}

void OpenSearchEngine::setNetworkAccessManager(QNetworkAccessManager *networkAccessManager)
{
    m_networkAccessManager = networkAccessManager;
}

/*!
    \fn void OpenSearchEngine::imageChanged()

    This signal is emitted whenever the image of the engine changes.

    \sa image(), imageUrl()
*/

/*!
    \fn void OpenSearchEngine::suggestions(const QStringList &suggestions)

    This signal is emitted whenever new contextual suggestions have been provided
    by the search engine. To request suggestions, use requestSuggestions().
    The suggestion set is specified by \a suggestions.

    \sa requestSuggestions()
*/