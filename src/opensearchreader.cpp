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

#include "opensearchreader.h"

#include "opensearchengine.h"

#include <qiodevice.h>

/*!
    \class OpenSearchReader
    \brief A class reading a search engine description from an external source

    OpenSearchReader is a class that can be used to read search engine descriptions
    formed using the OpenSearch format.

    It inherits QXmlStreamReader and thus provides additional functions, such as
    QXmlStreamReader::error(), QXmlStreamReader::hasError() that can be used to make sure
    the reading procedure succeeded.

    For more information see:
    http://www.opensearch.org/Specifications/OpenSearch/1.1/Draft_4#OpenSearch_description_document

    \sa OpenSearchEngine, OpenSearchWriter
*/

/*!
    Constructs a new reader.

    \note One instance can be used to read multiple files, one by one.
*/
OpenSearchReader::OpenSearchReader()
    : QXmlStreamReader()
    , m_engine(0)
{
}

/*!
    Reads an OpenSearch engine from the \a device and returns an OpenSearchEngine object,
    filled in with all the data that has been retrieved from the document.

    If the \a device is closed, it will be opened.

    To make sure if the procedure succeeded, check QXmlStreamReader::error().

    \return a new constructed OpenSearchEngine object

    \note The function returns an object of the OpenSearchEngine class even if the document
          is bad formed or doesn't conform to the specification. It needs to be manually
          deleted afterwards, if intended.
    \note The lifetime of the returned OpenSearchEngine object is up to the user.
          The object should be deleted once it is not used anymore to avoid memory leaks.
*/
OpenSearchEngine *OpenSearchReader::read(QIODevice *device)
{
    m_engine = 0;
    clear();

    if (!device->isOpen())
        device->open(QIODevice::ReadOnly);

    setDevice(device);
    readDocument();
    return m_engine;
}

#include <qdebug.h>

void OpenSearchReader::readDocument()
{
    m_engine = new OpenSearchEngine();

    while (!isStartElement() && !atEnd())
        readNext();

    if (name() != QLatin1String("OpenSearchDescription")
        || namespaceUri() != QLatin1String("http://a9.com/-/spec/opensearch/1.1/")) {
        raiseError(QObject::tr("The file is not an OpenSearch 1.1 file."));
        return;
    }

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (!isStartElement())
            continue;

        if (name() == QLatin1String("ShortName"))
            readName();
        else if (name() == QLatin1String("Description"))
            readDescription();
        else if (name() == QLatin1String("Url"))
            readUrl();
        else if (name() == QLatin1String("Image"))
            readImage();
        else if (name() == QLatin1String("Tags"))
            readTags();
        else
            skipSubtree();
    }
}

void OpenSearchReader::readName()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("ShortName"));
    m_engine->setName(readElementText());
}

void OpenSearchReader::readDescription()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("Description"));
    m_engine->setDescription(readElementText());
}

void OpenSearchReader::readUrl()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("Url"));

    QString type = attributes().value(QLatin1String("type")).toString();
    QString url = attributes().value(QLatin1String("template")).toString();
    QString method = attributes().value(QLatin1String("method")).toString();

    if (type.isEmpty() || type == QLatin1String("application/xhtml+xml"))
        type = QLatin1String("text/html");

    if (url.isEmpty()) {
        skipSubtree();
        return;
    }

    if (type == QLatin1String("application/x-suggestions+json")
        && !m_engine->suggestionsUrlTemplate().isEmpty()) {
        skipSubtree();
        return;
    }

    if (type == QLatin1String("text/html")
        && !m_engine->searchUrlTemplate().isEmpty()) {
        skipSubtree();
        return;
    }

    OpenSearchEngine::Parameters parameters;

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (!isStartElement())
            continue;

        if (name() == QLatin1String("Param") || name() == QLatin1String("Parameter"))
            readParameter(&parameters);
        else
            skipSubtree();
    }

    if (type == QLatin1String("application/x-suggestions+json")) {
        m_engine->setSuggestionsUrlTemplate(url);
        m_engine->setSuggestionsParameters(parameters);
        m_engine->setSuggestionsMethod(method);
    } else if (type == QLatin1String("text/html")) {
        m_engine->setSearchUrlTemplate(url);
        m_engine->setSearchParameters(parameters);
        m_engine->setSearchMethod(method);
    }
}

void OpenSearchReader::readParameter(OpenSearchEngine::Parameters *parameters)
{
    Q_ASSERT(isStartElement() && (name() == QLatin1String("Param") || name() == QLatin1String("Parameter")));

    QString key = attributes().value(QLatin1String("name")).toString();
    QString value = attributes().value(QLatin1String("value")).toString();

    if (key.isEmpty() || value.isEmpty())
        return;

    parameters->append(OpenSearchEngine::Parameter(key, value));
    readNext();
}

void OpenSearchReader::readImage()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("Image"));
    m_engine->setImageUrl(readElementText());
}

void OpenSearchReader::readTags()
{
    Q_ASSERT(isStartElement() && name() == QLatin1String("Tags"));
    m_engine->setTags(readElementText().split(QLatin1Char(' '), QString::SkipEmptyParts));
}

void OpenSearchReader::skipSubtree()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            skipSubtree();
    }
}
