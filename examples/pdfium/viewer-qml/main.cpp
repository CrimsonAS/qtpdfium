/*
 * Copyright (c) 2020 Crimson AS <info@crimson.no>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QGuiApplication>
#include <QPdfium>
#include <QPdfiumPage>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QSGNode>
#include <QSGImageNode>
#include <QQuickWindow>
#include <QQmlContext>
#include <QDebug>

class PdfDocument : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY isValidChanged)
public:
    PdfDocument(QQuickItem* parent = nullptr);

    QString source() const;
    void setSource(const QString&);

    int pageCount() const;
    bool isValid() const;

signals:
    void sourceChanged(const QString& source);
    void pageCountChanged(int pageCount);
    void isValidChanged(bool isValid);

private:
    friend class PdfDocumentView;
    QPdfium& pdfiumHandle() { return m_pdf; }

    QString m_source;
    QPdfium m_pdf;
};

PdfDocument::PdfDocument(QQuickItem* parent)
    : QQuickItem(parent)
{
}

QString PdfDocument::source() const
{
    return m_source;
}

void PdfDocument::setSource(const QString& source)
{
    if (source == m_source) {
        return;
    }

    m_source = source;

    bool wasValid = isValid();
    int oldPageCount = pageCount();

    m_pdf.loadFile(source);
    emit sourceChanged(source);

    if (isValid() != wasValid) {
        emit isValidChanged(isValid());
    }

    if (pageCount() != oldPageCount) {
        emit pageCountChanged(pageCount());
    }
}

int PdfDocument::pageCount() const
{
    return m_pdf.pageCount();
}

bool PdfDocument::isValid() const
{
    return m_pdf.isValid();
}

class PdfDocumentView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(PdfDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int pageNumber READ pageNumber WRITE setPageNumber NOTIFY pageNumberChanged)
    Q_PROPERTY(QString pageText READ pageText NOTIFY pageTextChanged)
public:
    PdfDocumentView(QQuickItem* parent = nullptr);

    PdfDocument* document() const;
    void setDocument(PdfDocument*);

    int pageNumber() const;
    void setPageNumber(int);

    QString pageText() const;

signals:
    void documentChanged(PdfDocument*);
    void pageNumberChanged(int pageNumber);
    void pageTextChanged(const QString& pageText);

protected:
    void updatePolish() override;
    QSGNode* updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*) override;

private:
    PdfDocument* m_document = nullptr;
    QImage m_image;
    QString m_pageText;
    int m_pageNumber = 0;
};

PdfDocumentView::PdfDocumentView(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    connect(this, &QQuickItem::widthChanged, this, &QQuickItem::polish);
    connect(this, &QQuickItem::heightChanged, this, &QQuickItem::polish);
}

PdfDocument* PdfDocumentView::document() const
{ return m_document; }

void PdfDocumentView::setDocument(PdfDocument* document)
{
    if (document == m_document) {
        return;
    }

    if (m_document) {
        disconnect(m_document, &PdfDocument::sourceChanged, this, &QQuickItem::polish);
        disconnect(m_document, &PdfDocument::isValidChanged, this, &QQuickItem::polish);
    }

    m_document = document;

    if (m_document) {
        connect(m_document, &PdfDocument::sourceChanged, this, &QQuickItem::polish);
        connect(m_document, &PdfDocument::isValidChanged, this, &QQuickItem::polish);
    }

    polish();
}

int PdfDocumentView::pageNumber() const
{
    return m_pageNumber;
}

void PdfDocumentView::setPageNumber(int pageNumber)
{
    if (pageNumber == m_pageNumber) {
        return;
    }

    m_pageNumber = pageNumber;
    emit pageNumberChanged(pageNumber);
    polish();
}

QString PdfDocumentView::pageText() const
{
    return m_pageText;
}

void PdfDocumentView::updatePolish()
{
    if (!m_document || !m_document->isValid()) {
        qDebug() << "Unable to load pdf";
        m_image = QImage();
        update();
        m_pageText = QString();
        emit pageTextChanged(m_pageText);
        return;
    }

    auto& pdfium = m_document->pdfiumHandle();
    auto page = pdfium.page(m_pageNumber);
    setImplicitWidth(page.width());
    setImplicitHeight(page.height());
    qreal scale = width() / (qreal)page.width();
    m_image = page.image(scale);
    m_pageText = page.text();
    emit pageTextChanged(m_pageText);
    update();
}

QSGNode* PdfDocumentView::updatePaintNode(QSGNode* oldNode, QQuickItem::UpdatePaintNodeData*)
{
    if (m_image.isNull()) {
        delete oldNode;
        return nullptr;
    }

    QSGImageNode* node = nullptr;
    if (!oldNode) {
        node = window()->createImageNode();
    } else {
        node = static_cast<QSGImageNode*>(oldNode);
        delete node->texture();
    }
    node->setTexture(window()->createTextureFromImage(m_image));
    node->setRect(QRectF(0, 0, width(), height()));
    return node;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

#ifdef Q_OS_IOS
    //Since it's static library on IOS we need to initialize it by hand
    PdfiumGlobal global;
#endif

    qmlRegisterType<PdfDocument>("QtPdfium", 1, 0, "PdfDocument");
    qmlRegisterType<PdfDocumentView>("QtPdfium", 1, 0, "PdfDocumentView");

    QPdfium pdf;
    QQmlApplicationEngine engine;

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}

#include "main.moc"
