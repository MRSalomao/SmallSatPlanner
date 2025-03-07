#ifndef PLANNER_H
#define PLANNER_H

#include <ompl/extensions/opende/OpenDESimpleSetup.h>
#include <ompl/base/goals/GoalRegion.h>
#include <ompl/config.h>
#include <iostream>
#include <ode/ode.h>


namespace oc = ompl::control;
namespace og = ompl::geometric;
namespace ob = ompl::base;


// Define our own space, to include a distance function we want and register a default projection
class RigidBodyStateSpace : public oc::OpenDEStateSpace
{
public:

    RigidBodyStateSpace(const oc::OpenDEEnvironmentPtr &env) : oc::OpenDEStateSpace(env)
    {
        ob::RealVectorBounds bounds(3);
        bounds.setLow(-200);
        bounds.setHigh(200);
        setVolumeBounds(bounds);

        bounds.setLow(-20);
        bounds.setHigh(20);
        setLinearVelocityBounds(bounds);

        bounds.setLow(-20);
        bounds.setHigh(20);
        setAngularVelocityBounds(bounds);
    }

    virtual double distance(const ob::State *s1, const ob::State *s2) const
    {
        const double *p1 = s1->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(0);
        const double *p2 = s2->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(0);
        double dx = fabs(p1[0] - p2[0]);
        double dy = fabs(p1[1] - p2[1]);
        double dz = fabs(p1[2] - p2[2]);
        return sqrt(dx * dx + dy * dy + dz * dz);
    }
};


class myStateValidityCheckerClass : public ob::StateValidityChecker
{
public:
    myStateValidityCheckerClass(const ob::SpaceInformationPtr &si) : ob::StateValidityChecker(si)
    {

    }

    virtual bool isValid(const ob::State *state) const
    {
        return true;
    }
};


class RigidBodyEnvironment : public oc::OpenDEEnvironment
{
public:

    // the simulation world
    dWorldID bodyWorld;

    // the space for all objects
    dSpaceID space;

    // the car mass
    dMass    m;

    // the body geom
    dGeomID  boxGeom;

    // the body
    dBodyID  boxBody;

    RigidBodyEnvironment(void) : oc::OpenDEEnvironment()
    {
        createWorld();
    }

    virtual ~RigidBodyEnvironment(void)
    {
        destroyWorld();
    }

    virtual unsigned int getControlDimension(void) const
    {
        return 3;
    }

    virtual void getControlBounds(std::vector<double> &lower, std::vector<double> &upper) const
    {
        static double maxForce = 0.2;
        lower.resize(3);
        lower[0] = -maxForce;
        lower[1] = -maxForce;
        lower[2] = -maxForce;

        upper.resize(3);
        upper[0] = maxForce;
        upper[1] = maxForce;
        upper[2] = maxForce;
    }

    virtual void applyControl(const double *control) const
    {
        dBodyAddForce(boxBody, control[0], control[1], control[2]);
    }

    virtual bool isValidCollision(dGeomID /*geom1*/, dGeomID /*geom2*/, const dContact& /*contact*/) const
    {
        return false;
    }

    virtual void setupContact(dGeomID /*geom1*/, dGeomID /*geom2*/, dContact &contact) const
    {
        contact.surface.mode = dContactSoftCFM | dContactApprox1;
        contact.surface.mu = 0.9;
        contact.surface.soft_cfm = 0.2;
    }

    void setPlanningParameters(void)
    {
        // Fill in parameters for OMPL:
        world_ = bodyWorld;
        collisionSpaces_.push_back(space);
        stateBodies_.push_back(boxBody);
        stepSize_ = 0.05;
        maxContacts_ = 1;
        minControlSteps_ = 10;
        maxControlSteps_ = 500;
    }

    void createWorld(void)
    {
        bodyWorld = dWorldCreate();
        space = dHashSpaceCreate(0);

        dWorldSetGravity(bodyWorld, 0, 0, -0.981);

        double lx = 0.2;
        double ly = 0.2;
        double lz = 0.1;

        dMassSetBox(&m, 1, lx, ly, lz);

        boxGeom = dCreateBox(space, lx, ly, lz);
        boxBody = dBodyCreate(bodyWorld);
        dBodySetMass(boxBody, &m);
        dGeomSetBody(boxGeom, boxBody);

        setPlanningParameters();
    }

    void destroyWorld(void)
    {
        dSpaceDestroy(space);
        dWorldDestroy(bodyWorld);
    }

};


// Define the goal we want to reach
class RigidBodyGoal : public ob::GoalRegion
{
public:

    RigidBodyGoal(const ob::SpaceInformationPtr &si) : ob::GoalRegion(si)
    {
        threshold_ = 0.5;
    }

    virtual double distanceGoal(const ob::State *st) const
    {
        const double *pos = st->as<oc::OpenDEStateSpace::StateType>()->getBodyPosition(0);
        double dx = fabs(pos[0] - 30);
        double dy = fabs(pos[1] - 55);
        double dz = fabs(pos[2] - 35);
        return sqrt(dx * dx + dy * dy + dz * dz);
    }

};


class Planner
{
public:
    Planner();
    ~Planner();
};

#endif // PLANNER_H
