﻿#pragma once
#include <QObject>

#include "ModelBasic/SimulationMonitor.h"

#include "Definitions.h"
#include "UnitObserver.h"

class SimulationMonitorImpl
	: public SimulationMonitor
	, public UnitObserver
{
	Q_OBJECT

public:
	SimulationMonitorImpl(QObject * parent = nullptr);
	virtual ~SimulationMonitorImpl();

	virtual void init(SimulationContext* context) override;

	virtual void requireData() override;
	virtual MonitorData const& retrieveData() override;

	//from UnitObserver
	virtual void unregister() override;
	virtual void accessToUnits() override;

private:
	void calcMonitorData();
	void calcMonitorDataForUnit(Unit* unit);

	SimulationContextCpuImpl* _context = nullptr;
	bool _registered = false;

	bool _dataRequired = false;
	MonitorData _data;
};