#pragma once

#include <QGraphicsScene>
#include <QVector2D>
#include <QTimer>

#include "Gui/Definitions.h"
#include "Model/Api/Definitions.h"
#include "Model/Api/Descriptions.h"

class PixelUniverseView : public QGraphicsScene
{
    Q_OBJECT
public:
    PixelUniverseView(QObject* parent = nullptr);
    virtual ~PixelUniverseView();

	virtual void init(Notifier* notifier, SimulationController* controller, DataRepository* manipulator, ViewportInterface* viewport);
	virtual void activate();
	virtual void deactivate();

	virtual void refresh();

protected:
	void mouseMoveEvent(QGraphicsSceneMouseEvent* e);

private:
	Q_SLOT void receivedNotifications(set<Receiver> const& targets);
	Q_SLOT void requestData();
	Q_SLOT void retrieveAndDisplayData();
	Q_SLOT void scrolled();

	list<QMetaObject::Connection> _connections;

	Manipulator* _manipulator = nullptr;
	DataRepository* _repository = nullptr;
	SimulationController* _controller = nullptr;
	ViewportInterface* _viewport = nullptr;
    QGraphicsPixmapItem* _pixmap = nullptr;
    QImage* _image = nullptr;
	Notifier* _notifier = nullptr;
};
