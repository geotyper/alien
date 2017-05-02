#include <QThread>

#include "model/context/unit.h"
#include "model/context/unitcontext.h"
#include "model/context/mapcompartment.h"
#include "threadcontrollerimpl.h"

ThreadControllerImpl::ThreadControllerImpl(QObject * parent)
	: ThreadController(parent)
{
}

ThreadControllerImpl::~ThreadControllerImpl()
{
	terminateThreads();
}

void ThreadControllerImpl::init(int maxRunningThreads)
{
	terminateThreads();
	_maxRunningThreads = maxRunningThreads;
	for (auto const& thr : _threads) {
		delete thr;
	}
	_threads.clear();

}

void ThreadControllerImpl::registerUnit(Unit * unit)
{
	auto newThread = new QThread(this);
	newThread->connect(newThread, &QThread::finished, unit, &QObject::deleteLater);
	unit->moveToThread(newThread);
	_threads.push_back(newThread);
	_threadsByContexts[unit->getContext()] = newThread;
}

void ThreadControllerImpl::start()
{
	updateDependencies();
	//newThread->start();
}

void ThreadControllerImpl::updateDependencies()
{
	for (auto const& threadByContext : _threadsByContexts) {
		auto context = threadByContext.first;
		auto thr = threadByContext.second;
		auto compartment = context->getMapCompartment();
		auto getThread = [&](MapCompartment::RelativeLocation rel) {
			return _threadsByContexts[compartment->getNeighborContext(rel)];
		};
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::UpperLeft));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::Upper));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::UpperRight));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::Left));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::Right));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::LowerLeft));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::Lower));
		_dependencies[thr].push_back(getThread(MapCompartment::RelativeLocation::LowerRight));
	}
}

void ThreadControllerImpl::terminateThreads()
{
	for (auto const& thr : _threads) {
		thr->quit();
	}
	for (auto const& thr : _threads) {
		if (!thr->wait(2000)) {
			thr->terminate();
			thr->wait();
		}
	}
}
