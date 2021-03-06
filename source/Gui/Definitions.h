#pragma once

#include <mutex>

#include "Base/Definitions.h"
#include "ModelBasic/Definitions.h"

class QGraphicsItem;
class QGraphicsView;
class QGraphicsScene;
class QTabWidget;
class QTableWidgetItem;
class QSignalMapper;
class QAction;

class CellItem;
class ParticleItem;
class CellConnectionItem;
class ItemConfig;
class MonitorView;
class MetadataManager;
class PixelUniverseView;
class ItemUniverseView;
class ItemManager;
class DataRepository;
class InfoController;
class ViewportInterface;
class ViewportController;
class MarkerItem;
class DataEditController;
class DataEditContext;
class ToolbarController;
class ToolbarContext;
class ToolbarView;
class ActionModel;
class DataEditModel;
class DataEditView;
class DataRepository;
class ClusterEditTab;
class CellEditTab;
class MetadataEditTab;
class CellComputerEditTab;
class ParticleEditTab;
class SelectionEditTab;
class SymbolEditTab;
class HexEditWidget;
class TokenEditTabWidget;
class TokenEditTab;
class Notifier;
class MainView;
class MainModel;
class MainController;
class VersionController;
class VisualEditController;
class ActionHolder;
class ActionController;
class MonitorController;
class ImageSectionItem;

struct MonitorData;
using MonitorDataSP = boost::shared_ptr<MonitorData>;

enum class ActiveScene { PixelScene, ItemScene };
enum class Receiver { Simulation, VisualEditor, DataEditor, ActionController };
enum class UpdateDescription { All, AllExceptToken, AllExceptSymbols };
enum class NotifyScrollChanged { No, Yes };

class _SimulationConfig;
using SimulationConfig = boost::shared_ptr<_SimulationConfig>;

class _SimulationConfigGpu;
using SimulationConfigGpu = boost::shared_ptr<_SimulationConfigGpu>;

enum class ModelComputationType
{
	Gpu = 1
};

class DataAnalyzer;

class Worker;
