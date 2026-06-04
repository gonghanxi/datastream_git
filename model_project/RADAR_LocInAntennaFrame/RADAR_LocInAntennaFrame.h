#pragma once
#include "ModelBuilder.h"

class SYSTEMVUEMODELBUILDER_API RADAR_LocInAntennaFrame : public SystemVueModelBuilder::DFModel
{
public:
	// This Macro is required for all classes derived from DFModel
	DECLARE_MODEL_INTERFACE( RADAR_LocInAntennaFrame );

    enum SelectedXYZFrameTypes { ECIFrame, XYZFrame };
    enum SelectedAntennaPlaneTypes{ XYPlane, YZPlane };
    enum SelectedCoordinateTypes{ RADARCoordinate, AntennaCoordinate };

	// Constructor to initialize parameters
	RADAR_LocInAntennaFrame();
	
	//-------- Function Overloads --------
	virtual bool	Setup();
	virtual bool	Run();

	// Ports
	SystemVueModelBuilder::CircularBuffer<double>	AntRoll, AntYaw, AntPitch;
	SystemVueModelBuilder::CircularBuffer<double>	BodyYaw, BodyPitch, BodyRoll;
	SystemVueModelBuilder::CircularBufferBusT< SystemVueModelBuilder::CircularBuffer<SystemVueModelBuilder::Matrix < double > > >	TargetLoc;
	SystemVueModelBuilder::CircularBuffer< SystemVueModelBuilder::Matrix < double > >	RadarLoc;
	//SystemVueModelBuilder::DoubleCircularBufferBus	Azimuth, Elevation;
	SystemVueModelBuilder::CircularBufferBusT< SystemVueModelBuilder::CircularBuffer<double> >	Azimuth, Elevation;


	// Parameter
	double	TimeStep;
	SelectedXYZFrameTypes		XYZFrameType;
	SelectedAntennaPlaneTypes	AntennaPlaneType;
	SelectedCoordinateTypes		CoordinateType;

private:
	int	TargetNum;
};
