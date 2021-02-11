#include "physics/PhysicsSystem.h"
#include "transform/Transform.h"

using namespace sitara::ecs;

PhysicsSystem::PhysicsSystem() {
	mFoundation = nullptr;
	mPhysics = nullptr;
	mDispatcher = nullptr;
	mCudaContext = nullptr;
	mScene = nullptr;
	mPvd = nullptr;
	mNumberOfThreads = 8;
	mMaterialCount = 0;
	mGpuEnabled = false;
}


PhysicsSystem::~PhysicsSystem() {
	if (mScene) {
		mScene->release();
		mScene = nullptr;
	}
	if (mDispatcher) {
		mDispatcher->release();
		mDispatcher = nullptr;
	}
	if (mCudaContext) {
		mCudaContext->release();
		mCudaContext = nullptr;
	}
	if (mPhysics) {
		mPhysics->release();
		mPhysics = nullptr;
	}
	if (mPvd) {
		physx::PxPvdTransport* transport = mPvd->getTransport();
		mPvd->release();
		mPvd = nullptr;
		if (transport) {
			transport->release();
			transport = nullptr;
		}
	}
	if (mFoundation) {
		mFoundation->release();
		mFoundation = nullptr;
	}
}

void PhysicsSystem::configure(entityx::EntityManager& entities, entityx::EventManager& events) {
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocator, mErrorCallback);
	mDispatcher = physx::PxDefaultCpuDispatcherCreate(mNumberOfThreads);

	mPvd = PxCreatePvd(*mFoundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, physx::PxTolerancesScale(), true, mPvd);

	if (mGpuEnabled) {
		physx::PxCudaContextManagerDesc cudaDesc;
		mCudaContext = PxCreateCudaContextManager(*mFoundation, cudaDesc);
	}

	physx::PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	//sceneDesc.flags = physx::PxSceneFlag::eREQUIRE_RW_LOCK;

	if (mGpuEnabled) {
		sceneDesc.cudaContextManager = mCudaContext;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_GPU_DYNAMICS;
		sceneDesc.broadPhaseType = physx::PxBroadPhaseType::eGPU;
	}

	mScene = mPhysics->createScene(sceneDesc);

	physx::PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
	if (pvdClient) {
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	events.subscribe<entityx::ComponentAddedEvent<DynamicBody>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<DynamicBody>>(*this);
	events.subscribe<entityx::ComponentAddedEvent<StaticBody>>(*this);
	events.subscribe<entityx::ComponentRemovedEvent<StaticBody>>(*this);
}

void PhysicsSystem::update(entityx::EntityManager& entities, entityx::EventManager& events, entityx::TimeDelta dt) {
	entityx::ComponentHandle<sitara::ecs::StaticBody> sBody;
	entityx::ComponentHandle<sitara::ecs::DynamicBody> body;
	entityx::ComponentHandle<sitara::ecs::OverlapDetector> proximityDetector;
	entityx::ComponentHandle<sitara::ecs::Transform> transform;

	/*
	// preprocessing callbacks
	for (auto entity : entities.entities_with_components(body, transform)) {
		for (auto callback : mDynamicBodyPreUpdateFns) {
			callback(body);
		}
	}
	*/

	// run simulation
	float timeStep = static_cast<float>(dt);
	mScene->simulate(timeStep);
	mScene->fetchResults(true);

	// update transform component with new world transform from physx
	for (auto entity : entities.entities_with_components(body, transform)) {
		if (!body->isSleeping()) {
			transform->mPosition = body->getPosition();
			transform->mOrientation = body->getRotation();
		}
	}

	for (auto entity : entities.entities_with_components(sBody, transform)) {
		if (sBody->isDirty()) {
			transform->mPosition = sBody->getPosition();
			transform->mOrientation = sBody->getRotation();
			sBody->setDirty(false);
		}
	}

	/*
	* Iterate over proximity detector components and detect proximities
	*/
	for (auto entity : entities.entities_with_components(proximityDetector, transform)) {
		proximityDetector->swapBuffers();

		size_t num = physx::PxSceneQueryExt::overlapMultiple(*mScene,
			proximityDetector->getGeometry(),
			proximityDetector->getTransform(),
			proximityDetector->getWriteBuffer(),
			proximityDetector->getBufferSize(),
			proximityDetector->getFilter());
		proximityDetector->resizeBuffer(num);

		std::cout << "Detected " << num << " overlaps." << std::endl;

		for (auto& hit : proximityDetector->getResults()) {
			std::cout << "Starting collision detection" << std::endl;
			if (hit.actor != nullptr) {
				std::cout << "Test " << std::endl;
				auto prev = proximityDetector->getPreviousResults();
				auto it = std::find_if(prev.begin(), prev.end(), [&](const physx::PxOverlapHit& h) {return h.actor->userData == hit.actor->userData; });
				if (it != prev.end()) {
					// in current collision + previous collision, still colliding
					std::cout << "Still Colliding..." << std::endl;
				}
			}
		}

		for (auto& hit : proximityDetector->getPreviousResults()) {
			if (hit.actor != nullptr) {
				auto res = proximityDetector->getResults();
				auto it = std::find_if(res.begin(), res.end(), [&](const physx::PxOverlapHit& h) {return h.actor->userData == hit.actor->userData; });
				if (it != res.end()) {
					// in previous collision but NOT in current collision, ending collision
					std::cout << "Ending Collision." << std::endl;
				}
			}
		}
	}

	/*
	// post processing
	for (auto entity : entities.entities_with_components(body, transform)) {
		for (auto callback : mDynamicBodyPostUpdateFns) {
			callback(body);
		}
	}
	*/
}

void PhysicsSystem::receive(const entityx::ComponentAddedEvent<sitara::ecs::DynamicBody>& event) {
	/*
	Link the user application entityx::Entity::Id to the NVIDIA PhysX object using the userData pointer.
	Note that this is NOT a pointer to the entity; it's the entity's literal id number.
	You'll want to cast this back to a uint64_t to use it.
	*/
	entityx::ComponentHandle<sitara::ecs::DynamicBody> body = event.component;
	body->mBody->userData = (void*)(event.entity.id().id());
}

void PhysicsSystem::receive(const entityx::ComponentRemovedEvent<sitara::ecs::DynamicBody>& event) {
}

void PhysicsSystem::receive(const entityx::ComponentAddedEvent<sitara::ecs::StaticBody>& event) {
	/*
	Link the user application entityx::Entity::Id to the NVIDIA PhysX object using the userData pointer.
	Note that this is NOT a pointer to the entity; it's the entity's literal id number.
	You'll want to cast this back to a uint64_t to use it.
	*/
	entityx::ComponentHandle<sitara::ecs::StaticBody> body = event.component;
	body->mBody->userData = (void*)(event.entity.id().id());
}

void PhysicsSystem::receive(const entityx::ComponentRemovedEvent<sitara::ecs::StaticBody>& event) {
}

double PhysicsSystem::getElapsedSimulationTime() {
	return mScene->getTimestamp();
}

void PhysicsSystem::setGravity(const ci::vec3& gravity) {
	if (mScene) {
		mScene->setGravity(physx::PxVec3(gravity.x, gravity.y, gravity.z));
	}
	else {
		std::cout << "sitara::ecs::PhysicsSystem ERROR -- must configure() system before you can set gravity." << std::endl;
	}
}

void PhysicsSystem::setNumberOfThread(const uint32_t numThreads) {
	if (!mScene) {
		mNumberOfThreads = numThreads;
	}
	else {
		std::cout << "Number of threads must be set before running PhysicsSystem::configure(); using default number of threads : " << mNumberOfThreads << std::endl;
	}
}

void PhysicsSystem::enableGpu(const bool enable) {
	if (!mScene) {
		std::cout << "GPU features are currently only available for static builds; until a future time these features are disabled." << std::endl;
		mGpuEnabled = false;
	}
	else {
		std::cout << "GPU must be enabled before running PhysicsSystem::configure(); using CPU dispatcher instead." << std::endl;
	}
}

physx::PxRigidStatic* PhysicsSystem::createStaticBody(const ci::vec3& position, const ci::quat& rotation) {
	physx::PxTransform transform = sitara::ecs::physics::to(rotation, position);
	physx::PxRigidStatic* body = mPhysics->createRigidStatic(transform);
	mScene->addActor(*body);
	return body;
}

physx::PxRigidDynamic* PhysicsSystem::createDynamicBody(const ci::vec3& position, const ci::quat& rotation) {
	physx::PxTransform transform = sitara::ecs::physics::to(rotation, position);
	physx::PxRigidDynamic* body = mPhysics->createRigidDynamic(transform);
	mScene->addActor(*body);
	return body;
}

int PhysicsSystem::registerMaterial(const float staticFriction = 0.5f, const float dynamicFriction = 0.5f, const float restitution = 0.0f) {
	mMaterialCount++;
	physx::PxMaterial* material = mPhysics->createMaterial(staticFriction, dynamicFriction, restitution);
	mMaterialRegistry.insert(std::pair<int, physx::PxMaterial*>(mMaterialCount, material));
	return mMaterialCount;
}

physx::PxMaterial* PhysicsSystem::getMaterial(const int id) {
	return mMaterialRegistry[id];
}