#include "graphicsoutputview.h"

#include <QPalette>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QPushButton>
#include <QGroupBox>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QResizeEvent>
#include <qcontainerfwd.h>
#include <qcoreevent.h>

#include "juliadebug.h"

namespace Julia {

GraphicsWidget::GraphicsWidget(QWidget* parent,
    int graphicsMethod=0, 
    QString graphicsDir=QStringLiteral("/tmp/graphics/"),
    QString socketHost=QStringLiteral("localhost"),
    int socketPort = 8080     
)
    : QWidget{parent}
    , m_graphicsMethod{graphicsMethod}
    , m_graphicsDirectory{graphicsDir}
    , m_watcher(nullptr)
    , m_tcpServer(nullptr)
    , m_socketServerRunning(false)
    , m_socketHost(socketHost)
    , m_socketPort(socketPort)
{
    setupUI();

    // Use system colors instead of hardcoded white
    // The widget will inherit colors from the parent theme

    // Initialize file system watcher
    if(!graphicsMethod){
        m_watcher = new QFileSystemWatcher(this);
        m_watcher->addPath(m_graphicsDirectory);
        connect(m_watcher, &QFileSystemWatcher::directoryChanged,
                this, &GraphicsWidget::onDirectoryChanged);
        connect(m_watcher, &QFileSystemWatcher::fileChanged,
                this, &GraphicsWidget::onFileChanged);
        // Connect refresh button
        setGraphicsDirectory(m_graphicsDirectory);
        connect(m_refreshButton, &QPushButton::clicked,
                this, &GraphicsWidget::onRefreshClicked);
    }
    // Initialize TCP server
    if(graphicsMethod){
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection,
                this, &GraphicsWidget::onNewConnection);

    }

    // Set default graphics directory and start monitoring
}

GraphicsWidget::~GraphicsWidget()
{
    // Clean up temporary graphics directory
    cleanupTemporaryImages();
}

void GraphicsWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);

    // Button layout
    m_buttonLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    m_refreshButton->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_buttonLayout->addWidget(m_refreshButton);
    m_buttonLayout->addStretch();

    m_layout->addLayout(m_buttonLayout);

    // Scroll area for images
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_imageContainer = new QWidget();
    m_imageLayout = new QVBoxLayout(m_imageContainer);
    m_imageLayout->setAlignment(Qt::AlignTop);

    m_scrollArea->setWidget(m_imageContainer);
    m_layout->addWidget(m_scrollArea);

    m_placeholderLabel = new QLabel(tr("Graphics Output View\nWaiting for images..."), this);
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));
    m_imageLayout->addWidget(m_placeholderLabel);

    setLayout(m_layout);
}

void GraphicsWidget::setGraphicsDirectory(const QString& directory)
{
    if (m_graphicsDirectory == directory) {
        return;
    }

    m_graphicsDirectory = directory;

    // Stop watching old directory
    if (!m_watcher->directories().isEmpty()) {
        m_watcher->removePaths(m_watcher->directories());
    }
    if (!m_watcher->files().isEmpty()) {
        m_watcher->removePaths(m_watcher->files());
    }

    // Create directory if it doesn't exist
    QDir dir(m_graphicsDirectory);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    // Start watching new directory
    bool watchingAdded = m_watcher->addPath(m_graphicsDirectory);
    qCDebug(KDEV_JULIA) << "Watching directory:" << m_graphicsDirectory << "success:" << watchingAdded;

    // Load existing images
    refreshImages();

    qCDebug(KDEV_JULIA) << "Graphics directory set to:" << m_graphicsDirectory;
}

void GraphicsWidget::refreshImages()
{
    clearImages();
    loadImagesFromDirectory();
}

void GraphicsWidget::loadImagesFromDirectory()
{
    QDir dir(m_graphicsDirectory);
    if (!dir.exists()) {
        qCDebug(KDEV_JULIA) << "Graphics directory does not exist:" << m_graphicsDirectory;
        m_placeholderLabel->show();
        return;
    }

    QStringList filters{
            QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
            QStringLiteral("*.bmp"),  QStringLiteral("*.gif"), QStringLiteral("*.svg")
    };
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    qCDebug(KDEV_JULIA) << "Found" << fileList.size() << "image files in" << m_graphicsDirectory;

    if (fileList.isEmpty()) {
        m_placeholderLabel->show();
        return;
    }

    m_placeholderLabel->hide();

    // Sort by modification time (newest first)
    std::sort(fileList.begin(), fileList.end(),
              [](const QFileInfo& a, const QFileInfo& b) {
                  return a.lastModified() > b.lastModified();
              });

    for (const QFileInfo& fileInfo : fileList) {
        qCDebug(KDEV_JULIA) << "Loading image:" << fileInfo.absoluteFilePath();
        addImageToView(fileInfo.absoluteFilePath());
    }
}

void GraphicsWidget::addImageToView(const QString& filePath)
{
    QImageReader reader(filePath);
    QImage image = reader.read();

    if (image.isNull()) {
        qCWarning(KDEV_JULIA) << "Failed to load image:" << filePath;
        return;
    }

    // Scale image to fit within tool view width with some margin
    int availableWidth = this->width() - 100; // Account for margins and scroll bar
    if (availableWidth < 100) availableWidth = 400; // Minimum width fallback

    if (image.width() > availableWidth) {
        image = image.scaledToWidth(availableWidth, Qt::SmoothTransformation);
    }

    // Also limit height to reasonable bounds
    int maxHeight = 600;
    if (image.height() > maxHeight) {
        image = image.scaledToHeight(maxHeight, Qt::SmoothTransformation);
    }

    // Create group box for image with controls using system style
    QGroupBox* imageFrame = new QGroupBox();
    // GroupBox will use system style automatically
    imageFrame->setStyleSheet(QStringLiteral("QGroupBox { margin: 2px; padding: 2px; }"));

    QVBoxLayout* frameLayout = new QVBoxLayout(imageFrame);
    frameLayout->setContentsMargins(5, 5, 5, 5);

    // Image label
    QLabel* imageLabel = new QLabel();
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->setAlignment(Qt::AlignCenter);

    // Add filename as tooltip
    QFileInfo fileInfo(filePath);
    imageLabel->setToolTip(fileInfo.fileName());

    // Control buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QToolButton* saveButton = new QToolButton();
    saveButton->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    saveButton->setToolTip(tr("Save image"));
    saveButton->setProperty("imagePath", filePath);

    QToolButton* deleteButton = new QToolButton();
    deleteButton->setIcon(QIcon::fromTheme(QStringLiteral("edit-delete")));
    deleteButton->setToolTip(tr("Delete image"));
    deleteButton->setProperty("imagePath", filePath);

    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(deleteButton);

    // Connect buttons
    connect(saveButton, &QToolButton::clicked, this, &GraphicsWidget::onSaveImageClicked);
    connect(deleteButton, &QToolButton::clicked, this, &GraphicsWidget::onDeleteImageClicked);

    frameLayout->addWidget(imageLabel);
    frameLayout->addLayout(buttonLayout);

    m_imageLayout->addWidget(imageFrame);

    // Watch this specific file
    m_watcher->addPath(filePath);
}

void GraphicsWidget::clearImages()
{
    // Remove all image labels except placeholder
    QLayoutItem* item;
    while ((item = m_imageLayout->takeAt(0)) != nullptr) {
        if (item->widget() != m_placeholderLabel) {
            delete item->widget();
        }
        delete item;
    }

    // Re-add placeholder
    m_imageLayout->addWidget(m_placeholderLabel);
    m_placeholderLabel->show();
}

void GraphicsWidget::onDirectoryChanged(const QString& path)
{
    qCDebug(KDEV_JULIA) << "Directory changed:" << path;
    // Directory changed, refresh images
    refreshImages();
}

void GraphicsWidget::onFileChanged(const QString& path)
{
    qCDebug(KDEV_JULIA) << "File changed:" << path;
    // File changed, refresh images
    refreshImages();
}

void GraphicsWidget::onRefreshClicked()
{
    // Manual refresh when button is clicked
    refreshImages();
}

void GraphicsWidget::onSaveImageClicked()
{
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button) return;

    QString imagePath = button->property("imagePath").toString();
    if (imagePath.isEmpty()) return;

    QFileInfo fileInfo(imagePath);
    QString suggestedName = fileInfo.baseName() + QStringLiteral("_saved.") + fileInfo.suffix();

    QString savePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + suggestedName,
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)")
    );

    if (!savePath.isEmpty()) {
        if (QFile::copy(imagePath, savePath)) {
            qCDebug(KDEV_JULIA) << "Image saved to:" << savePath;
        } else {
            QMessageBox::warning(this, tr("Save Error"),
                tr("Failed to save image to %1").arg(savePath));
        }
    }
}

void GraphicsWidget::onDeleteImageClicked()
{
    QToolButton* button = qobject_cast<QToolButton*>(sender());
    if (!button) return;

    QString imagePath = button->property("imagePath").toString();
    if (imagePath.isEmpty()) return;

    QFileInfo fileInfo(imagePath);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Delete Image"),
        tr("Are you sure you want to delete '%1'?").arg(fileInfo.fileName()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (QFile::remove(imagePath)) {
            qCDebug(KDEV_JULIA) << "Image deleted:" << imagePath;
            // The file watcher will trigger refresh automatically
        } else {
            QMessageBox::warning(this, tr("Delete Error"),
                tr("Failed to delete image %1").arg(fileInfo.fileName()));
        }
    }
}

// Методы для работы с TCP сокетами
void GraphicsWidget::startSocketServer(int port)
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    m_socketPort = port;
    if (m_tcpServer->listen(QHostAddress::Any, port)) {
        m_socketServerRunning = true;
        qCDebug(KDEV_JULIA) << "TCP socket server started on port" << port;
    } else {
        qCWarning(KDEV_JULIA) << "Failed to start TCP socket server on port" << port;
        m_socketServerRunning = false;
    }
}

void GraphicsWidget::stopSocketServer()
{
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
        qCDebug(KDEV_JULIA) << "TCP socket server stopped";
    }

    // Clean up all client connections
    for (QTcpSocket* client : m_clients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
    m_clients.clear();
    m_clientBuffers.clear();

    m_socketServerRunning = false;
}

void GraphicsWidget::connectToSocket(const QString& host, int port)
{
    m_socketHost = host;
    m_socketPort = port;
    qCDebug(KDEV_JULIA) << "Socket connection requested to" << host << ":" << port << "(client mode not implemented)";
    // TODO: Implement client mode if needed
}

void GraphicsWidget::onNewConnection()
{
    QTcpSocket* clientSocket = m_tcpServer->nextPendingConnection();
    if (clientSocket) {
        qCDebug(KDEV_JULIA) << "New client connected:" << clientSocket->peerAddress().toString();

        m_clients.append(clientSocket);
        m_clientBuffers[clientSocket] = QByteArray();

        connect(clientSocket, &QTcpSocket::readyRead,
                this, &GraphicsWidget::onSocketReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected,
                this, &GraphicsWidget::onSocketDisconnected);
    }
}

void GraphicsWidget::onSocketReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray& buffer = m_clientBuffers[clientSocket];
    buffer.append(clientSocket->readAll());

    // Try to process complete images from the buffer
    processSocketData(clientSocket);
}

void GraphicsWidget::onSocketDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    qCDebug(KDEV_JULIA) << "Client disconnected:" << clientSocket->peerAddress().toString();

    m_clients.removeAll(clientSocket);
    m_clientBuffers.remove(clientSocket);
    clientSocket->deleteLater();
}

void GraphicsWidget::processSocketData(QTcpSocket* clientSocket)
{
    QByteArray& buffer = m_clientBuffers[clientSocket];

    // Simple protocol: expect images as raw data with a size header
    // Format: [4 bytes size][image data]
    while (buffer.size() >= 4) {
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 imageSize;
        stream >> imageSize;

        if (buffer.size() < 4 + int(imageSize)) {
            // Not enough data yet
            break;
        }

        // Extract image data
        QByteArray imageData = buffer.mid(4, imageSize);
        buffer.remove(0, 4 + imageSize);

        // Try to decode as image
        QImage image;
        if (image.loadFromData(imageData)) {
            // Save image to temporary file
            QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
            QString filename = QStringLiteral("socket_image_%1.png").arg(timestamp);
            QString filePath = QDir(m_graphicsDirectory).absoluteFilePath(filename);

            if (image.save(filePath, "PNG")) {
                qCDebug(KDEV_JULIA) << "Received and saved image from socket:" << filename;
                // The file watcher will automatically detect and display the new image
            } else {
                qCWarning(KDEV_JULIA) << "Failed to save received image";
            }
        } else {
            qCWarning(KDEV_JULIA) << "Failed to decode image data from socket";
        }
    }
}

void GraphicsWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // Trigger rescaling of all images
    rescaleImages();
}

void GraphicsWidget::rescaleImages()
{
    // Rescale all images to fit new width
    QList<QLabel*> imageLabels = findChildren<QLabel*>();
    for (QLabel* label : imageLabels) {
        if (label == m_placeholderLabel) continue;

        // Get original image path from tooltip
        QString imagePath = label->toolTip();
        if (imagePath.isEmpty()) continue;

        // Reload and rescale the image
        QImageReader reader(imagePath);
        QImage image = reader.read();

        if (!image.isNull()) {
            // Scale to current tool view width
            int availableWidth = this->width() - 100;
            if (availableWidth < 100) availableWidth = 400;

            if (image.width() > availableWidth) {
                image = image.scaledToWidth(availableWidth, Qt::SmoothTransformation);
            }

            // Limit height
            int maxHeight = 600;
            if (image.height() > maxHeight) {
                image = image.scaledToHeight(maxHeight, Qt::SmoothTransformation);
            }

            label->setPixmap(QPixmap::fromImage(image));
        }
    }
}

void GraphicsWidget::cleanupTemporaryImages()
{
    // Only clean up if using the default temporary directory
    if (m_graphicsDirectory == QStringLiteral("/tmp/graphics/")) {
        QDir dir(m_graphicsDirectory);
        if (dir.exists()) {
            qCDebug(KDEV_JULIA) << "Cleaning up temporary graphics directory:" << m_graphicsDirectory;

            // Remove all image files
            QStringList filters{
                QStringLiteral("*.png"), QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                QStringLiteral("*.bmp"), QStringLiteral("*.gif"), QStringLiteral("*.svg")
            };

            QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
            for (const QFileInfo& fileInfo : fileList) {
                if (QFile::remove(fileInfo.absoluteFilePath())) {
                    qCDebug(KDEV_JULIA) << "Removed temporary image:" << fileInfo.fileName();
                } else {
                    qCWarning(KDEV_JULIA) << "Failed to remove temporary image:" << fileInfo.fileName();
                }
            }

            // Try to remove the directory itself if it's empty
            if (dir.isEmpty()) {
                if (dir.rmdir(QStringLiteral("."))) {
                    qCDebug(KDEV_JULIA) << "Removed empty temporary graphics directory";
                }
            }
        }
    }
}

} // namespace Julia

#include "moc_graphicsoutputview.cpp"
