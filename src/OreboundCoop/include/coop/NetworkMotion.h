#pragma once
namespace orebound {
// A received pose is authoritative. Local collision response must not veto it.
// Keep collision enabled outside this transaction for native combat traces.
template<class Actor,class Position>
bool PlaceNetworkActor(Actor& actor,const Position& position,float yaw) {
    if(actor.isInMovementMode!=0) return false;
    const bool stat=actor.collDetectionStatic,dyn=actor.collDetectionDynamic;
    actor.SetCollDetStat(0);actor.SetCollDetDyn(0);
    actor.BeginMovement();
    actor.SetPositionWorld(position);actor.ResetRotationsWorld();actor.RotateWorldY(yaw);
    actor.EndMovement(1);
    actor.SetCollDetStat(stat);actor.SetCollDetDyn(dyn);
    return true;
}
}
