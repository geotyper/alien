#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include <QtCore/qmath.h>
#include <QMatrix4x4>

#include "gui/guisettings.h"
#include "model/simulationcontext.h"
#include "model/energyparticlemap.h"
#include "model/cellmap.h"
#include "model/topology.h"
#include "model/entities/cell.h"
#include "model/entities/cellcluster.h"
#include "model/entities/energyparticle.h"

#include "pixeluniverse.h"

const int MOUSE_HISTORY = 10;

PixelUniverse::PixelUniverse(QObject* parent)
    : _pixelMap(0), _image(0), _timer(0), _lastMouseDiffs(MOUSE_HISTORY),
      _leftMouseButtonPressed(false), _rightMouseButtonPressed(false)
{
    setBackgroundBrush(QBrush(QColor(0,0,0)));
    _pixelMap = addPixmap(QPixmap());
//    _pixelMap->setScale(_zoom);
    update();

    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(timeout()));
    _timer->start(500);
}

PixelUniverse::~PixelUniverse()
{
    if( _image )
        delete _image;
}

void PixelUniverse::reset ()
{
    if( _image )
        delete _image;
    _image = 0;
//    delete _pixelMap;
//    _pixelMap = addPixmap(QPixmap());
    update();
}

void PixelUniverse::universeUpdated (SimulationContext* context)
{
    _context = context;

    //prepare image
	context->lock();
	IntVector2D size = context->getTopology()->getSize();
	context->unlock();
    if( !_image ) {
//        _pixelMap = addPixmap(QPixmap());
        _image = new QImage(size.x, size.y, QImage::Format_RGB32);
        setSceneRect(0,0,_image->width(), _image->height());
    }
    _image->fill(0xFF000030);

    //draw image
	context->lock();
    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
	EnergyParticleMap* energyMap = context->getEnergyParticleMap();
	CellMap* cellMap = context->getCellMap();
	IntVector2D pos{ 0, 0 };
    for(pos.x = 0; pos.x < size.x; ++pos.x)
        for(pos.y = 0; pos.y < size.y; ++pos.y) {

            //draw energy particle
			EnergyParticle* energy(energyMap->getParticleFast(pos));
            if( energy ) {
                quint32 e(energy->amount+10);
                e *= 5;
                if( e > 150)
                    e = 150;
                _image->setPixel(pos.x, pos.y, (e << 16) | 0x30);
            }

            //draw cell
            Cell* cell = cellMap->getCellFast(pos);
            if( cell ) {
                if(cell->getNumToken() > 0 )
                    _image->setPixel(pos.x, pos.y, 0xFFFFFF);
                else {
                    quint8 color = cell->getMetadata().color;
                    if( color == 0 ) {
                        r = INDIVIDUAL_CELL_COLOR1.red();
                        g = INDIVIDUAL_CELL_COLOR1.green();
                        b = INDIVIDUAL_CELL_COLOR1.blue();
                    }
                    if( color == 1 ) {
                        r = INDIVIDUAL_CELL_COLOR2.red();
                        g = INDIVIDUAL_CELL_COLOR2.green();
                        b = INDIVIDUAL_CELL_COLOR2.blue();
                    }
                    if( color == 2 ) {
                        r = INDIVIDUAL_CELL_COLOR3.red();
                        g = INDIVIDUAL_CELL_COLOR3.green();
                        b = INDIVIDUAL_CELL_COLOR3.blue();
                    }
                    if( color == 3 ) {
                        r = INDIVIDUAL_CELL_COLOR4.red();
                        g = INDIVIDUAL_CELL_COLOR4.green();
                        b = INDIVIDUAL_CELL_COLOR4.blue();
                    }
                    if( color == 4 ) {
                        r = INDIVIDUAL_CELL_COLOR5.red();
                        g = INDIVIDUAL_CELL_COLOR5.green();
                        b = INDIVIDUAL_CELL_COLOR5.blue();
                    }
                    if( color == 5 ) {
                        r = INDIVIDUAL_CELL_COLOR6.red();
                        g = INDIVIDUAL_CELL_COLOR6.green();
                        b = INDIVIDUAL_CELL_COLOR6.blue();
                    }
                    if( color == 6 ) {
                        r = INDIVIDUAL_CELL_COLOR7.red();
                        g = INDIVIDUAL_CELL_COLOR7.green();
                        b = INDIVIDUAL_CELL_COLOR7.blue();
                    }
                    quint32 e(cell->getEnergy()/2.0+20.0);
                    if( e > 150)
                        e = 150;
                    r = r*e/150;
                    g = g*e/150;
                    b = b*e/150;
//                    _image->setPixel(x, y, (e << 16) | ((e*2/3) << 8) | ((e*2/3) << 0)| 0x30);
                    _image->setPixel(pos.x, pos.y, (r << 16) | (g << 8) | b);
                }
            }
        }

    //draw selection markers
    if( !_selectedClusters.empty() ) {
        for(int x = 0; x < size.x; ++x)
            _image->setPixel(x, _selectionPos.y(), 0x202040);
        for(int y = 0; y < size.y; ++y)
            _image->setPixel(_selectionPos.x(), y, 0x202040);
    }

    //draw selected clusters
    foreach(CellCluster* cluster, _selectedClusters) {
        foreach(Cell* cell, cluster->getCellsRef()) {
            QVector3D pos = cell->calcPosition(true);
            _image->setPixel(pos.x(), pos.y(), 0xBFBFBF);
        }
    }

    context->unlock();
    _pixelMap->setPixmap(QPixmap::fromImage(*_image));
}

void PixelUniverse::mousePressEvent (QGraphicsSceneMouseEvent* e)
{
    if( !_context )
        return;
	_context->lock();

    //update mouse buttons
    _leftMouseButtonPressed = ((e->buttons() & Qt::LeftButton) == Qt::LeftButton);
    _rightMouseButtonPressed = ((e->buttons() & Qt::RightButton) == Qt::RightButton);

    //left xor right button pressed?
    if( (_leftMouseButtonPressed && (!_rightMouseButtonPressed)) || ((!_leftMouseButtonPressed) && _rightMouseButtonPressed)) {

        //scan for clusters
		IntVector2D size = _context->getTopology()->getSize();
		CellMap* cellMap = _context->getCellMap();
        QMap< quint64, CellCluster* > clusters;
        QVector3D mousePos(e->scenePos().x(), e->scenePos().y(), 0.0);
        for(int rx = -5; rx < 6; ++rx )
            for(int ry = -5; ry < 6; ++ry ) {
                QVector3D scanPos = mousePos + QVector3D(rx,ry,0.0);
                if( (scanPos.x() >= 0.0) && (scanPos.x() < size.x)
                    && (scanPos.y() >= 0.0) && (scanPos.y() < size.y) ) {
                    Cell* cell = cellMap->getCell(scanPos);
                    if( cell)
                        clusters[cell->getCluster()->getId()] = cell->getCluster();
                }
            }

        //remove clusters from simulation (temporarily)
        foreach(CellCluster* cluster, clusters) {
            _context->getClustersRef().removeOne(cluster);
            cluster->clearCellsFromMap();
        }

        //calc center
        QVector3D center;
        int numCells = 0;
        foreach(CellCluster* cluster, clusters) {
            foreach(Cell* cell, cluster->getCellsRef()) {
                center += cell->calcPosition();
            }
            numCells += cluster->getCellsRef().size();
        }
        center = center / numCells;

        //move to selected clusters
        _selectedClusters = clusters.values();
        _selectionPos.setX(center.x());
        _selectionPos.setY(center.y());
		_context->getTopology()->correctPosition(_selectionPos);
    }

    //both buttons pressed?
    if( _leftMouseButtonPressed && _rightMouseButtonPressed ) {

        //move selected clusters to simulation
        foreach(CellCluster* cluster, _selectedClusters) {
            cluster->drawCellsToMap();
        }
		_context->getClustersRef() << _selectedClusters;
        _selectedClusters.clear();
    }
    _context->unlock();
    universeUpdated(_context);
}

void PixelUniverse::mouseReleaseEvent (QGraphicsSceneMouseEvent* e)
{
    if( !_context )
        return;
	_context->lock();

    //update mouse buttons
    _leftMouseButtonPressed = ((e->buttons() & Qt::LeftButton) == Qt::LeftButton);
    _rightMouseButtonPressed = ((e->buttons() & Qt::RightButton) == Qt::RightButton);

    //move selected clusters to simulation
    foreach(CellCluster* cluster, _selectedClusters) {
        cluster->drawCellsToMap();
    }
	_context->getClustersRef() << _selectedClusters;
    _selectedClusters.clear();

	_context->unlock();
    universeUpdated(_context);
}

void PixelUniverse::mouseMoveEvent (QGraphicsSceneMouseEvent* e)
{
    if( !_context)
        return;

    //update mouse buttons and positions
//    _leftMouseButtonPressed = ((e->buttons() & Qt::LeftButton) == Qt::LeftButton);
//    _rightMouseButtonPressed = ((e->buttons() & Qt::RightButton) == Qt::RightButton);
    QVector3D mousePos(e->scenePos().x(), e->scenePos().y(), 0.0);
    QVector3D lastMousePos(e->lastScenePos().x(), e->lastScenePos().y(), 0.0);
    QVector3D mouseDiff = mousePos - lastMousePos;
    QVector3D cumMouseDiff = mouseDiff;
    for(int i = 0; i < MOUSE_HISTORY; ++i) {
        cumMouseDiff += _lastMouseDiffs[i];
    }
    cumMouseDiff = cumMouseDiff / MOUSE_HISTORY;

    //only left button pressed? => move selected clusters
    if( _leftMouseButtonPressed && (!_rightMouseButtonPressed) ) {
		_context->lock();

        //update position and velocity
        foreach(CellCluster* cluster, _selectedClusters) {
            cluster->setPosition(cluster->getPosition()+mouseDiff);
            cluster->setVel((cumMouseDiff)/5.0);
        }

        //update selection
        _selectionPos +=mouseDiff;
		_context->getTopology()->correctPosition(_selectionPos);

		_context->unlock();
        universeUpdated(_context);
    }

    //only right button pressed? => rotate selected clusters
    if( (!_leftMouseButtonPressed) && _rightMouseButtonPressed ) {
		_context->lock();

        //1. step: rotate each cluster around own center
        foreach(CellCluster* cluster, _selectedClusters) {
            cluster->setAngle(cluster->getAngle()+mouseDiff.x()+mouseDiff.y());
            cluster->setAngularVel((cumMouseDiff.x() + cumMouseDiff.y())/3.0);
        }

        //2. step: rotate cluster around common center
        //calc center
        QVector3D center;
        int numCells = 0;
        foreach(CellCluster* cluster, _selectedClusters) {
            foreach(Cell* cell, cluster->getCellsRef()) {
                center += cell->calcPosition();
            }
            numCells += cluster->getCellsRef().size();
        }
        center = center / numCells;
        QMatrix4x4 transform;
        transform.setToIdentity();
        transform.translate(center);
        transform.rotate(mouseDiff.x()+mouseDiff.y(), 0.0, 0.0, 1.0);
        transform.translate(-center);
        foreach(CellCluster* cluster, _selectedClusters) {
            cluster->setPosition(transform.map(cluster->getPosition()));
        }

		_context->unlock();
        universeUpdated(_context);
    }

    //both buttons pressed? => apply forces along mouse path
    if( _leftMouseButtonPressed && _rightMouseButtonPressed ) {
        if( mousePos != lastMousePos ) {
			_context->lock();
			CellMap* cellMap = _context->getCellMap();

            //calc distance vector and length
            QVector3D dir = mouseDiff.normalized();
            qreal dist = mouseDiff.length();

            //scan mouse path for clusters
            QMap< quint64, CellCluster* > clusters;
            QMap< quint64, Cell* > cells;
            for(int d = 0; d < qFloor(dist)+1; ++d ) {
                for(int rx = -5; rx < 6; ++rx )
                    for(int ry = -5; ry < 6; ++ry ) {
                        QVector3D scanPos = mousePos + dir*d + QVector3D(rx,ry,0.0);
                        Cell* cell = cellMap->getCell(scanPos);
                        if( cell) {
                            clusters[cell->getCluster()->getId()] = cell->getCluster();
                            cells[cell->getCluster()->getId()] = cell;
                        }
                    }
            }

            //apply forces to all encountered cells
            QMapIterator< quint64, Cell* > itCell(cells);
            while(itCell.hasNext()) {
                itCell.next();
                Cell* cell = itCell.value();

                //apply force
                cell->setVel(cell->getVel() + dir*dist*cell->getCluster()->getMass()*0.05);
            }

            //calc effective velocities of the clusters
            QMapIterator< quint64, CellCluster* > itCluster(clusters);
            while(itCluster.hasNext()) {
                itCluster.next();
                CellCluster* cluster = itCluster.value();
                cluster->updateVel_angularVel_via_cellVelocities();
                cluster->updateCellVel(false);
            }
			_context->unlock();
        }
    }
    for(int i = 0; i < MOUSE_HISTORY-1; ++i)
        _lastMouseDiffs[MOUSE_HISTORY-i-1] = _lastMouseDiffs[MOUSE_HISTORY-i-2];
    _lastMouseDiffs[0] = mouseDiff;
}

void PixelUniverse::timeout ()
{
    //set velocity of selected clusters to 0
    foreach(CellCluster* cluster, _selectedClusters) {
        if( _leftMouseButtonPressed )
            cluster->setVel(QVector3D());
        if( _rightMouseButtonPressed )
            cluster->setAngularVel(0.0);
    }
}

