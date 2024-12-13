#include "pch.h"
#include "World.h"
#include "PhysicsBodyCollisionResult.h"

//internal utility functions
namespace sfp {
	static float Dot(Vector2f a, Vector2f b){
		return (a.x * b.x) + (a.y * b.y);
	}
	static void ResolveCollision(PhysicsBodyCollisionResult &collision)
	{
		PhysicsBody &A = collision.object1;
		PhysicsBody &B = collision.object2;
		// Calculate relative velocity
		Vector2f rv = B.getVelocity() - A.getVelocity();

		// Calculate relative velocity in terms of the normal direction
		float velAlongNormal = Dot(rv, collision.normal);

		// Do not resolve if velocities are separating
		if (velAlongNormal > 0)
			return;

		// Calculate restitution
		float e = min(A.getRestitution(), B.getRestitution());

		// Calculate impulse scalar
		float j = -(1 + e) * velAlongNormal;
		j /= 1 / A.getMass() + 1 / B.getMass();

		
		// Apply impulse
		Vector2f impulse = j * collision.normal;
		float aMass = A.getMass();
		float bMass = B.getMass();
		if (aMass != 0) {
			A.setVelocity(A.getVelocity() - 1 / aMass * impulse);
		}
		if (bMass != 0) {
			B.setVelocity(B.getVelocity() + 1 / bMass * impulse);
		}
		
	}
}


// class functions
sfp::World::World(Vector2f gravity):
	gravity(gravity)
{

}



void sfp::World::AddPhysicsBody(PhysicsBody& obj)
{
	objects.push_back(&obj);
}

void sfp::World::RemovePhysicsBody(PhysicsBody& obj)
{
	removalList.push_back(&obj);
}

void sfp::World::UpdatePhysics(unsigned long deltaMilliseconds,unsigned long msPerTick)
{
	// cleanup first
	for (auto el : removalList) {
		objects.remove(el);
	}
	removalList.clear();
	//check for single pass
	if (msPerTick == 0) {
		msPerTick = deltaMilliseconds;
	}
	deltaAccumulator += deltaMilliseconds;
	while (deltaAccumulator >= msPerTick) {
		for (auto obj : objects) {
			obj->applyImpulse(gravity * (float)msPerTick / 1000.0f);
			// do collision, very stupid right now. long run should not check 
			// objecst that havent moved
			for (auto obj2 : objects) {
				if ((obj != obj2) && (ignoreMovement || obj->hasMoved() || obj2->hasMoved()) && HasCollision(obj->getLayer(), obj2->getLayer())) {
					PhysicsBodyCollisionResult collision =
						obj->collideWith(*obj2);
					if (collision.hasCollided) {
						if (collision.object1.onCollision) {
							collision.object1.onCollision(collision);
						}
						ResolveCollision(collision);
					}
				}
			}
		}
		for (auto obj : objects) {
			obj->update(msPerTick);
		}
		deltaAccumulator -= msPerTick;
	}
}

void sfp::World::VisualizeAllBounds(RenderWindow& window)
{
	for (auto obj : objects) {
		obj->visualizeBounds(window);
	}
}

void sfp::World::setIgnoreMovement(bool  ignore)
{
	ignoreMovement = ignore;
}

void sfp::World::ExcludeCollision(unsigned int layer1, unsigned int layer2)
{
	// Use the lower layer number. This is to prevent overlap.
	int primaryLayer = layer1 < layer2 ? layer1 : layer2;
	int otherLayer = layer1 < layer2 ? layer2 : layer1;

	for (auto layer : layers)
	{
		if (layer.Layer == primaryLayer)
		{
			layer.AddExcludedLayer(otherLayer);
			return;
		}
	}

	PhysicsLayer layer;
	layer.Layer = layer1;
	layer.AddExcludedLayer(layer2);
	layers.push_back(layer);
}

void sfp::World::IncludeCollision(unsigned int layer1, unsigned int layer2)
{
	// Use the lower layer number. This is to prevent overlap.
	int primaryLayer = layer1 < layer2 ? layer1 : layer2;
	int otherLayer = layer1 < layer2 ? layer2 : layer1;

	for (auto layer : layers)
	{
		if (layer.Layer == primaryLayer)
		{
			layer.RemoveExcludedLayer(otherLayer);
			return;
		}
	}
}

bool sfp::World::HasCollision(unsigned int layer1, unsigned int layer2)
{
	// Use the lower layer number. This is to prevent overlap.
	int primaryLayer = layer1 < layer2 ? layer1 : layer2;
	int otherLayer = layer1 < layer2 ? layer2 : layer1;

	for (auto layer : layers)
	{
		if (layer.Layer == primaryLayer)
			return !layer.ExcludesLayer(otherLayer);
	}
	return true;
}
