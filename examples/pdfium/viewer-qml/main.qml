import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtPdfium 1.0

ApplicationWindow {
    width: 640
    height:480
    title: "PDF Viewer"
    visible:true

    menuBar: MenuBar{
        Menu{
            title:"File"
            MenuItem{
                text:"Open Pdf file..."
                onTriggered: fileDialog.open()
            }
            MenuItem{
                text: "Close"
                onTriggered: Qt.quit()
            }
        }
    }

    FileDialog{
        id:fileDialog
        title:"Open PDF file"
        onAccepted: {
            pdfDocument.source = fileDialog.fileUrls[0].replace("file://", "")
        }
    }

    Text {
        id: pageSelectorTitle
        text: "Page:"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 10
    }

    ComboBox {
        id: pageSelector
        anchors.leftMargin: 10
        anchors.verticalCenter: pageSelectorTitle.verticalCenter
        anchors.left: pageSelectorTitle.right
        enabled: pdfDocument.isValid
        model: pdfDocument.pageCount
        onModelChanged: update()

        onCurrentIndexChanged: update()

        function update() {
            pdfDocumentView.pageNumber = currentIndex;
        }
    }

    SplitView {
        orientation: Qt.Horizontal
        anchors {
            top: pageSelector.bottom
            left: parent.left
            right:parent.right
            bottom: parent.bottom
        }

        PdfDocumentView {
            id: pdfDocumentView
            width: parent.width/2
            height: parent.height
            smooth: true
            document: PdfDocument {
                id: pdfDocument
            }
        }

        TextArea {
            id:pdf_text
            width: parent.width/2
            height: parent.height
            text: pdfDocumentView.pageText
        }
    }
}

