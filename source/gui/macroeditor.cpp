#include <QScrollBar>
#include <QTimer>
#include <QGraphicsItem>

#include "model/simulationcontext.h"
#include "model/topology.h"
#include "model/entities/cellcluster.h"
#include "model/entities/cell.h"
#include "macroeditor/pixeluniverse.h"
#include "macroeditor/shapeuniverse.h"
#include "gui/guisettings.h"
#include "gui/guisettings.h"


#include "microeditor.h"
#include "macroeditor.h"
#include "ui_macroeditor.h"


MacroEditor::MacroEditor(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::MacroEditor)
	, _activeScene(PIXEL_SCENE)
	, _pixelUniverse(new PixelUniverse(this))
	, _shapeUniverse(new ShapeUniverse(this))
{
    ui->setupUi(this);

    ui->simulationView->horizontalScrollBar()->setStyleSheet(SCROLLBAR_STYLESHEET);
    ui->simulationView->verticalScrollBar()->setStyleSheet(SCROLLBAR_STYLESHEET);

    //start with pixel scene by default
    ui->simulationView->setScene(_pixelUniverse);

    //connect signals
    connect(_shapeUniverse, SIGNAL(updateCell(QList<Cell*>,QList<CellTO>,bool)), this, SIGNAL(updateCell(QList<Cell*>,QList<CellTO>,bool)));
    connect(_shapeUniverse, SIGNAL(defocus()), this, SIGNAL(defocus()), Qt::QueuedConnection);
    connect(_shapeUniverse, SIGNAL(focusCell(Cell*)), this, SIGNAL(focusCell(Cell*)), Qt::QueuedConnection);
    connect(_shapeUniverse, SIGNAL(focusEnergyParticle(EnergyParticle*)), this, SIGNAL(focusEnergyParticle(EnergyParticle*)), Qt::QueuedConnection);
    connect(_shapeUniverse, SIGNAL(energyParticleUpdated(EnergyParticle*)), this, SIGNAL(energyParticleUpdated(EnergyParticle*)), Qt::QueuedConnection);
    connect(_shapeUniverse, SIGNAL(entitiesSelected(int,int)), this, SIGNAL(entitiesSelected(int,int)));

    //set up timer
    _updateTimer = new QTimer(this);
    connect(_updateTimer, SIGNAL(timeout()), this, SLOT(updateTimerTimeout()));
    _updateTimer->start(30);

}

MacroEditor::~MacroEditor()
{
    delete ui;
}

void MacroEditor::reset ()
{
    //reset data
    _pixelUniverseInit = false;
    _shapeUniverseInit = false;
    _pixelUniverseViewMatrix = QMatrix();
    _shapeUniverseViewMatrix = QMatrix();
    ui->simulationView->setTransform(QTransform());

    //reset subobjects
    _pixelUniverse->reset();
}


void MacroEditor::setActiveScene (ActiveScene activeScene)
{
    _activeScene = activeScene;
    if( _activeScene == PIXEL_SCENE ) {

        //save position of shape universe
        _shapeUniverseViewMatrix = ui->simulationView->matrix();
        _shapeUniversePosX = ui->simulationView->horizontalScrollBar()->value();
        _shapeUniversePosY = ui->simulationView->verticalScrollBar()->value();

        //switch scene to pixel universe
        ui->simulationView->setScene(_pixelUniverse);

        //load position of pixel universe
        ui->simulationView->setMatrix(_pixelUniverseViewMatrix);
        ui->simulationView->horizontalScrollBar()->setValue(_pixelUniversePosX);
        ui->simulationView->verticalScrollBar()->setValue(_pixelUniversePosY);
    }
    if( _activeScene == SHAPE_SCENE ) {

        //save position
        _pixelUniverseViewMatrix = ui->simulationView->matrix();
        _pixelUniversePosX = ui->simulationView->horizontalScrollBar()->value();
        _pixelUniversePosY = ui->simulationView->verticalScrollBar()->value();

        //switch scene to shapeuniverse
        ui->simulationView->setScene(_shapeUniverse);

        //load position of shape universe
        ui->simulationView->setMatrix(_shapeUniverseViewMatrix);
        ui->simulationView->horizontalScrollBar()->setValue(_shapeUniversePosX);
        ui->simulationView->verticalScrollBar()->setValue(_shapeUniversePosY);
    }

    //update scene
    _screenUpdatePossible = true;
    universeUpdated(_context, true);
}

QVector3D MacroEditor::getViewCenterPosWithInc ()
{
    //calc center of view
    QPointF posView(ui->simulationView->mapToScene(ui->simulationView->width()/2, ui->simulationView->height()/2));

    //calc center of view in simulation coordinate
    QVector3D pos(posView.x(), posView.y(), 0.0);

	if (_activeScene == SHAPE_SCENE) {
		pos = pos / GRAPHICS_ITEM_SIZE;
	}

    //add increment
    QVector3D posIncrement(_posIncrement, -_posIncrement, 0.0);
    _posIncrement = _posIncrement + 1.0;
    if( _posIncrement > 9.0)
        _posIncrement = 0.0;
    return pos + posIncrement;
}

void MacroEditor::getExtendedSelection (QList< CellCluster* >& clusters, QList< EnergyParticle* >& es)
{
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->getExtendedSelection(clusters, es);
    }
}

void MacroEditor::serializeViewMatrix (QDataStream& stream)
{
    //save position of pixel universe
    if( _activeScene == PIXEL_SCENE ) {
        _pixelUniverseViewMatrix = ui->simulationView->matrix();
        _pixelUniversePosX = ui->simulationView->horizontalScrollBar()->value();
        _pixelUniversePosY = ui->simulationView->verticalScrollBar()->value();
    }

    //save position of shape universe
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverseViewMatrix = ui->simulationView->matrix();
        _shapeUniversePosX = ui->simulationView->horizontalScrollBar()->value();
        _shapeUniversePosY = ui->simulationView->verticalScrollBar()->value();
    }

    //serialize data
    stream << _pixelUniverseViewMatrix << _shapeUniverseViewMatrix;
    stream << _pixelUniversePosX << _pixelUniversePosY;
    stream << _shapeUniversePosX << _shapeUniversePosY;
    stream << _pixelUniverseInit << _shapeUniverseInit;
}

void MacroEditor::loadViewMatrix (QDataStream& stream)
{
    stream >> _pixelUniverseViewMatrix >> _shapeUniverseViewMatrix;
    stream >> _pixelUniversePosX >> _pixelUniversePosY;
    stream >> _shapeUniversePosX >> _shapeUniversePosY;
    stream >> _pixelUniverseInit >> _shapeUniverseInit;

    //load position of pixel universe
    if( _activeScene == PIXEL_SCENE ) {
        ui->simulationView->setMatrix(_pixelUniverseViewMatrix);
        ui->simulationView->horizontalScrollBar()->setValue(_pixelUniversePosX);
        ui->simulationView->verticalScrollBar()->setValue(_pixelUniversePosY);
    }

    //load position of shape universe
    if( _activeScene == SHAPE_SCENE ) {
        ui->simulationView->setMatrix(_shapeUniverseViewMatrix);
        ui->simulationView->horizontalScrollBar()->setValue(_shapeUniversePosX);
        ui->simulationView->verticalScrollBar()->setValue(_shapeUniversePosY);
    }
}

QGraphicsView* MacroEditor::getGraphicsView ()
{
    return ui->simulationView;
}

qreal MacroEditor::getZoomFactor ()
{
    return  ui->simulationView->matrix().m11();
}

void MacroEditor::zoomIn ()
{
    ui->simulationView->scale(2.0,2.0);
}

void MacroEditor::zoomOut ()
{
    ui->simulationView->scale(0.5,0.5);
}

void MacroEditor::newCellRequested ()
{
    //request new cell at pos
    emit requestNewCell(getViewCenterPosWithInc());
}

void MacroEditor::newEnergyParticleRequested ()
{
    //request new energy particle at pos
    emit requestNewEnergyParticle(getViewCenterPosWithInc());
}

void MacroEditor::defocused ()
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->defocused();
    }
}

void MacroEditor::delSelection_Slot ()
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        QList< Cell* > cells;
        QList< EnergyParticle* > es;
        _shapeUniverse->delSelection(cells, es);
        emit delSelection(cells, es);
    }
}

void MacroEditor::delExtendedSelection_Slot ()
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        QList< CellCluster* > clusters;
        QList< EnergyParticle* > es;
        _shapeUniverse->delExtendedSelection(clusters, es);
        emit delExtendedSelection(clusters, es);
    }
}

void MacroEditor::cellCreated (Cell* cell)
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->cellCreated(cell);
    }
}

void MacroEditor::energyParticleCreated(EnergyParticle* e)
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->energyParticleCreated(e);
    }
}

void MacroEditor::energyParticleUpdated_Slot (EnergyParticle* e)
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->energyParticleUpdated_Slot(e);
    }
}

void MacroEditor::reclustered (QList< CellCluster* > clusters)
{
    //function only in shape scene
    if( _activeScene == SHAPE_SCENE ) {
        _shapeUniverse->reclustered(clusters);
    }
    else
        _pixelUniverse->universeUpdated(_context);
}

void MacroEditor::universeUpdated (SimulationContext* context, bool force)
{
    if(context)
        _context = context;
    else
		context = _context;
    if( !context)
        return;

    //update possible? (see updateTimerTimeout())
    if( _screenUpdatePossible || force ) {
        _screenUpdatePossible = false;

        //update active scene
        if( _activeScene == PIXEL_SCENE ) {
            _pixelUniverse->universeUpdated(context);

            //first time? => center view
            if( !_pixelUniverseInit ) {
               _pixelUniverseInit = true;
               ui->simulationView->scale(2.0,2.0);
               centerView(context);
            }
        }
        if( _activeScene == SHAPE_SCENE ) {
            _shapeUniverse->universeUpdated(context);

            //first time? => center view
            if( !_shapeUniverseInit ) {
               _shapeUniverseInit = true;
               ui->simulationView->scale(20.0 / GRAPHICS_ITEM_SIZE, 20.0 / GRAPHICS_ITEM_SIZE);
               centerView(context);
            }
            QGraphicsItem* cellItem = _shapeUniverse->getFocusCenterCell();
            if( cellItem )
                ui->simulationView->centerOn(cellItem);
        }
    }
}

void MacroEditor::metadataUpdated ()
{
    if( _activeScene == SHAPE_SCENE )
        _shapeUniverse->metadataUpdated();
}

void MacroEditor::toggleInformation(bool on)
{
	if (_activeScene == SHAPE_SCENE) {
		_shapeUniverse->toggleInformation(on);
	}
}

void MacroEditor::updateTimerTimeout ()
{
    _screenUpdatePossible = true;
}

void MacroEditor::centerView (SimulationContext* context)
{
    //load size of the universe
	context->lock();
	Topology* topo = context->getTopology();
    qreal sizeX = topo->getSize().x;
    qreal sizeY = topo->getSize().y;
	context->unlock();

    //set view position
    ui->simulationView->centerOn(sizeX/2.0*GRAPHICS_ITEM_SIZE, sizeY/2.0*GRAPHICS_ITEM_SIZE);
}

