#ifndef GRAPHICSTOOLVIEWFACTORY_H
#define GRAPHICSTOOLVIEWFACTORY_H

#include <interfaces/iuicontroller.h>


namespace Julia {

class GraphicsToolViewFactory : public KDevelop::IToolViewFactory
{
public:
    GraphicsToolViewFactory();
    ~GraphicsToolViewFactory() override;

    QWidget* create(QWidget* parent = nullptr) override;
    Qt::DockWidgetArea defaultPosition() const override;
    QString id() const override;
    void loadProjectConfiguration();
    void setExecutionContext(int, QString, QString, int);

private:
    int m_graphicsMethod;
    QString m_graphicsDir;
    QString m_socketHost;
    int m_socketPort;
};

} // namespace Julia

#endif // GRAPHICSTOOLVIEWFACTORY_H
