#include <functional>

#include "Model/SpaceMetricApi.h"

#include "Cuda/CudaShared.cuh"

#include "GpuWorker.h"

GpuWorker::~GpuWorker()
{
	end_Cuda();
}

void GpuWorker::init(SpaceMetricApi* metric)
{
	auto size = metric->getSize();
	init_Cuda({ size.x, size.y });
}

void GpuWorker::getData(IntRect const & rect, ResolveDescription const & resolveDesc, DataDescription & result)
{
	int numCLusters;
	ClusterCuda* clusters;
	result.clear();
	getDataRef_Cuda(numCLusters, clusters);
	for (int i = 0; i < numCLusters; ++i) {
		CellClusterDescription clusterDesc;
		ClusterCuda temp = clusters[i];
		if (rect.isContained({ (int)clusters[i].pos.x, (int)clusters[i].pos.y }))
		for (int j = 0; j < clusters[i].numCells; ++j) {
			auto pos = clusters[i].cells[j].absPos;
			clusterDesc.addCell(CellDescription().setPos({ (float)pos.x, (float)pos.y }).setMetadata(CellMetadata()).setEnergy(100.0f));
		}
		result.addCellCluster(clusterDesc);
	}
}

#include <QThread>

void GpuWorker::calculateTimestep()
{
	calcNextTimestep_Cuda();
	QThread::msleep(20);
	Q_EMIT timestepCalculated();
}
