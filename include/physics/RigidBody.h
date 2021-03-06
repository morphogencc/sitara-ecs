#pragma once

#include "entityx/Entity.h"
#include "cinder/Vector.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "physics/PhysicsUtils.h"

namespace sitara {
	namespace ecs {

		class RigidBody {
		public:
			RigidBody(btRigidBody* body) {
				mRigidBody = body;
			}

			~RigidBody() {
				if (mRigidBody != 0) {
					delete mRigidBody;
				}
				if (mMotionState != 0) {
					delete mMotionState;
				}
				if (mCollisionShape != 0) {
					delete mCollisionShape;
				}
			}

			static btRigidBody* createBox(const ci::vec3& size, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btVector3 halfSize = physics::toBtVector3(size) * 0.5f;
				btCollisionShape* shape = new btBoxShape(halfSize);
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			static btRigidBody* createSphere(float radius, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btCollisionShape* shape = new btSphereShape((btScalar)radius);
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			static btRigidBody* createCone(float radius, float height, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btCollisionShape* shape = new btConeShape(radius, height);
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			static btRigidBody* createCylinder(float radius, float height, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btCollisionShape* shape = new btCylinderShape(physics::toBtVector3(ci::vec3(radius, radius, 0.5f * height)));
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			static btRigidBody* createCapsule(float radius, float height, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btCollisionShape* shape = new btCapsuleShape(radius, height);
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			/*
			static btRigidBody* createHull(btConvexHullShape* shape, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}

			static btRigidBody* createMesh(btBvhTriangleMeshShape* shape, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btRigidBody* body = createRigidBody(shape, mass, position, rotation);
				return body;
			}
			*/

			/*
			ci::mat4 getWorldTransform() {
				btTransform trans;
				if (mRigidBody->getMotionState()) {
					mRigidBody->getMotionState()->getWorldTransform(trans);
				}
				return physics::fromBtTransform(trans);
			}
			*/

			btMotionState* getMotionState() const {
				return mRigidBody->getMotionState();
			}

			btRigidBody* getRigidBody() const {
				return mRigidBody;
			}

			btRigidBody* setElasticity(float elasticity) {
				mRigidBody->setRestitution(ci::clamp<float>(elasticity, 0.0, 1.0));
				return mRigidBody;
			}

			btRigidBody* setFriction(float friction) {
				mRigidBody->setFriction(ci::clamp<float>(friction, 0.0, 1.0));
				return mRigidBody;
			}

			btRigidBody* setDamping(float linearDamping, float angularDamping) {
				mRigidBody->setDamping(linearDamping, angularDamping);
				return mRigidBody;
			}

			void setLinearVelocity(ci::vec3 velocity) {
				mRigidBody->setLinearVelocity(physics::toBtVector3(velocity));
			}

			/*
			btRigidBody* setMassAndIntertia(float mass, ci::vec3 inertia = ci::vec3(0,0,0)) {
				btScalar btMass(mass);
				bool isDynamic = (mass != 0.f); //rigidbody is dynamic if and only if mass is non zero, otherwise static
				btVector3 localInertia(physics::toBtVector3(inertia));

				if (isDynamic) {
					mCollisionShape->calculateLocalInertia(mass, localInertia);
				}

				mRigidBody->setMassProps(mass, physics::toBtVector3(inertia));

				return mRigidBody;
			}
			*/

		protected:
			static btRigidBody* createRigidBody(btCollisionShape* shape, float mass, const ci::vec3& position = ci::vec3(0), const ci::quat& rotation = ci::quat()) {
				btVector3 inertia(0.0f, 0.0f, 0.0f);

				if (mass != 0.0f) {
					shape->calculateLocalInertia(mass, inertia);
				}

				btQuaternion btRotation = physics::toBtQuaternion(rotation);
				btVector3 btPosition = physics::toBtVector3(position);
				btTransform worldTransform(btRotation, btPosition);
				btDefaultMotionState* motionState = new btDefaultMotionState(worldTransform);
				btRigidBody::btRigidBodyConstructionInfo info(mass, motionState, shape, inertia);
				info.m_restitution = 0.0f;
				info.m_friction = 0.0f;

				btRigidBody* body = new btRigidBody(info);

				return body;
			}

			btCollisionShape* mCollisionShape;
			btDefaultMotionState* mMotionState;
			btRigidBody* mRigidBody;
		};
	}
}
