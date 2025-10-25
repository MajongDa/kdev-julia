#ifndef GRAPHICSOUTPUTVIEW_H
#define GRAPHICSOUTPUTVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QDir>
#include <QFileSystemWatcher>
#include <QPushButton>
#include <QHBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QGroupBox>
#include <QToolButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QBuffer>

namespace Julia {

class GraphicsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GraphicsWidget(QWidget* ,int, QString, QString, int);
    ~GraphicsWidget() override;

    // Методы для работы с файлами
    void setGraphicsDirectory(const QString& directory);
    void refreshImages();

    // Методы-заглушки для сокетов
    void startSocketServer(int port);
    void stopSocketServer();
    void connectToSocket(const QString& host, int port);

public Q_SLOTS:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);
    void onRefreshClicked();
    void onSaveImageClicked();
    void onDeleteImageClicked();
    void rescaleImages();
    void cleanupTemporaryImages();
    void onNewConnection();
    void onSocketReadyRead();
    void onSocketDisconnected();
    void processSocketData(QTcpSocket* clientSocket);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void loadImagesFromDirectory();
    void addImageToView(const QString& filePath);
    void clearImages();

    QVBoxLayout* m_layout;
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_refreshButton;
    QScrollArea* m_scrollArea;
    QWidget* m_imageContainer;
    QVBoxLayout* m_imageLayout;
    QLabel* m_placeholderLabel;

    int m_graphicsMethod;

    QString m_graphicsDirectory;
    QFileSystemWatcher* m_watcher;

    // Socket server components
    QTcpServer* m_tcpServer;
    QList<QTcpSocket*> m_clients;
    QHash<QTcpSocket*, QByteArray> m_clientBuffers;

    // Сокет-заглушки (пока не реализованы)
    bool m_socketServerRunning;
    QString m_socketHost;
    int m_socketPort;
};

} // namespace Julia

#endif // GRAPHICSOUTPUTVIEW_H
