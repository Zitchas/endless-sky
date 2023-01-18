/* Projectile.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Projectile.h"

#include "Effect.h"
#include "pi.h"
#include "Random.h"
#include "Ship.h"
#include "Visual.h"
#include "Weapon.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {
	// Given the probability of losing a lock in five tries, check randomly
	// whether it should be lost on this try.
	inline bool Check(double probability, double base)
	{
		return (Random::Real() < base * pow(probability, .2));
	}
}



Projectile::Projectile(const Ship &parent, Point position, Angle angle, bool under, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), position, parent.Velocity(), angle),
	weapon(weapon), targetShip(parent.GetTargetShip()), lifetime(weapon->Lifetime())
{
	government = parent.GetGovernment();
	
	// If you are boarding your target, do not fire on it.
	if(parent.IsBoarding() || parent.Commands().Has(Command::BOARD))
		targetShip.reset();
	
	cachedTarget = TargetPtr().get();
	if(cachedTarget)
		targetGovernment = cachedTarget->GetGovernment();
	double inaccuracy = weapon->Inaccuracy();
	if(inaccuracy)
		this->angle += Angle::Random(inaccuracy) - Angle::Random(inaccuracy);
	
	velocity += this->angle.Unit() * (weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	
	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);
    fireUnder = under;
}



Projectile::Projectile(const Projectile &parent, const Weapon *weapon)
	: Body(weapon->WeaponSprite(), parent.position + parent.velocity, parent.velocity, parent.angle),
	weapon(weapon), targetShip(parent.targetShip), lifetime(weapon->Lifetime())
{
	government = parent.government;
	targetGovernment = parent.targetGovernment;
	
	cachedTarget = TargetPtr().get();
	double inaccuracy = weapon->Inaccuracy();
	if(inaccuracy)
	{
		this->angle += Angle::Random(inaccuracy) - Angle::Random(inaccuracy);
		if(!parent.weapon->Acceleration())
		{
			// Move in this new direction at the same velocity.
			double parentVelocity = parent.weapon->Velocity();
			velocity += (this->angle.Unit() - parent.angle.Unit()) * parentVelocity;
		}
	}
	velocity += this->angle.Unit() * (weapon->Velocity() + Random::Real() * weapon->RandomVelocity());
	
	// If a random lifetime is specified, add a random amount up to that amount.
	if(weapon->RandomLifetime())
		lifetime += Random::Int(weapon->RandomLifetime() + 1);
    if(parent.DrawUnder())
        fireUnder = true;
}



// Ship explosion.
Projectile::Projectile(Point position, const Weapon *weapon)
	: weapon(weapon)
{
	this->position = position;
}



// This returns false if it is time to delete this projectile.
void Projectile::Move(vector<Visual> &visuals, vector<Projectile> &projectiles)
{
	if(--lifetime <= 0)
	{
		if(lifetime > -100)
		{
			// This projectile died a "natural" death. Create any death effects
			// and submunitions.
			for(const auto &it : weapon->DieEffects())
				for(int i = 0; i < it.second; ++i)
					visuals.emplace_back(*it.first, position, velocity, angle, DrawUnder());
			
			for(const auto &it : weapon->Submunitions())
				for(int i = 0; i < it.second; ++i)
					projectiles.emplace_back(*this, it.first);
		}
		MarkForRemoval();
		return;
	}
	for(const auto &it : weapon->LiveEffects())
		if(!Random::Int(it.second))
			visuals.emplace_back(*it.first, position, velocity, angle, DrawUnder());
	
	// If the target has left the system, stop following it. Also stop if the
	// target has been captured by a different government.
	const Ship *target = cachedTarget;
	if(target)
	{
		target = TargetPtr().get();
		if(!target || !target->IsTargetable() || target->GetGovernment() != targetGovernment)
		{
			targetShip.reset();
			cachedTarget = nullptr;
			target = nullptr;
		}
	}
	
	double turn = weapon->Turn();
	double accel = weapon->Acceleration();
	homing = weapon->Homing();
	if(target && homing && !Random::Int(60))
		CheckLock(*target);
    if(!target)
        hasLock = false;
	if(target && homing && hasLock)
	{
		// Vector d is the direction we want to turn towards.
		Point d = target->Position() - position;
        // add this back in for projectile innacuracy and remove line above. ajc
        //Point d = target->Position() + Angle::Random().Unit() * sqrt(target->Attributes().Get("radar jamming")) - position;
		Point unit = d.Unit();
		double drag = weapon->Drag();
		double trueVelocity = drag ? accel / drag : velocity.Length();
		double stepsToReach = d.Length() / trueVelocity;
		bool isFacingAway = d.Dot(angle.Unit()) < 0.;
		
		// At the highest homing level, compensate for target motion.
		if(homing >= 4)
		{
			if(unit.Dot(target->Velocity()) < 0.)
			{
				// If the target is moving toward this projectile, the intercept
				// course is where the target and the projectile have the same
				// velocity normal to the distance between them.
				Point normal(unit.Y(), -unit.X());
				double vN = normal.Dot(target->Velocity());
				double vT = sqrt(max(0., trueVelocity * trueVelocity - vN * vN));
				d = vT * unit + vN * normal;
			}
			else
			{
				// Adjust the target's position based on where it will be when we
				// reach it (assuming we're pointed right towards it).
				d += stepsToReach * target->Velocity();
				stepsToReach = d.Length() / trueVelocity;
			}
			unit = d.Unit();
		}
		
		double cross = angle.Unit().Cross(unit);
		
		// The very dumbest of homing missiles lose their target if pointed
		// away from it.
		if(isFacingAway && homing == 1)
			targetShip.reset();
		else
		{
			double desiredTurn = TO_DEG * asin(cross);
			if(fabs(desiredTurn) > turn)
				turn = copysign(turn, desiredTurn);
			else
				turn = desiredTurn;
			
			// Levels 3 and 4 stop accelerating when facing away.
			if(homing >= 3)
			{
				double stepsToFace = desiredTurn / turn;
		
				// If you are facing away from the target, stop accelerating.
				if(stepsToFace * 1.5 > stepsToReach)
					accel = 0.;
			}
		}
	}
	// If a weapon is homing but has no target, do not turn it.
	else if(homing)
		turn = 0.;
	
	if(turn)
		angle += Angle(turn);
	
	if(accel)
	{
		velocity *= 1. - weapon->Drag();
		velocity += accel * angle.Unit();
	}
	
	position += velocity;
	
	// If this projectile is now within its "split range," it should split into
	// sub-munitions next turn.
	if(target && (position - target->Position()).Length() < weapon->SplitRange())
		lifetime = 0;
}



// This projectile hit something. Create the explosion, if any. This also
// marks the projectile as needing deletion.
void Projectile::Explode(vector<Visual> &visuals, double intersection, Point hitVelocity)
{
	clip = intersection;
	for(const auto &it : weapon->HitEffects())
		for(int i = 0; i < it.second; ++i)
		{
			visuals.emplace_back(*it.first, position + velocity * intersection, velocity, angle, hitVelocity);
		}
	lifetime = -100;
}



// Get the amount of clipping that should be applied when drawing this projectile.
double Projectile::Clip() const
{
	return clip;
}



// This projectile was killed, e.g. by an anti-missile system.
void Projectile::Kill()
{
	lifetime = 0;
}



bool Projectile::DrawUnder() const
{
    return fireUnder;
}


// Find out if this is a missile, and if so, how strong it is (i.e. what
// chance an anti-missile shot has of destroying it).
int Projectile::MissileStrength() const
{
	return weapon->MissileStrength();
}



int Projectile::RemainingLifetime() const
{
    return lifetime;
}



bool Projectile::HasLock() const
{
    return homing && hasLock;
}



bool Projectile::HitAll() const
{
	return weapon->HitAll();
}



// Get information on the weapon that fired this projectile.
const Weapon &Projectile::GetWeapon() const
{
	return *weapon;
}


	
// Find out which ship this projectile is targeting.
const Ship *Projectile::Target() const
{
	return cachedTarget;
}



shared_ptr<Ship> Projectile::TargetPtr() const
{
	return targetShip.lock();
}



void Projectile::CheckLock(const Ship &target)
{
	double base = hasLock ? 1. : .5;
	hasLock = false;
	
	// For each tracking type, calculate the probability that a lock will be
	// lost in a given five-second period. Then, since this check is done every
	// second, test against the fifth root of that probability.
	if(weapon->Tracking())
		hasLock |= Check(weapon->Tracking(), base);
	
	// Optical tracking is about 15% for interceptors and 75% for medium warships.
	if(weapon->OpticalTracking())
	{
		double weight = target.Mass() * target.Mass();
		double probability = weapon->OpticalTracking() * weight / (200000. + weight);
		hasLock |= Check(probability, base);
	}
	
	// Infrared tracking is 10% when heat is zero and 100% when heat is full.
	if(weapon->InfraredTracking())
	{
		double probability = weapon->InfraredTracking() * min(1., target.Heat() + .1);
		hasLock |= Check(probability, base);
	}
	
	// Radar tracking depends on whether the target ship has jamming capabilities.
	// Jamming of 1 is enough to increase your chance of dodging to 50%.
	if(weapon->RadarTracking())
	{
		double probability = weapon->RadarTracking() / (1. + target.Attributes().Get("radar jamming"));
		hasLock |= Check(probability, base);
	}
}
