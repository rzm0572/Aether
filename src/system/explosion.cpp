#include "system/explosion.h"
#include "entity/plane.h"

void ExplosionSystem::addMonitor(const Model& model, Plane* go, float duration) {
    monitors_.emplace(go->getUUID(), ExplosionMonitor{ model, go, duration });
}

void ExplosionSystem::handleMonitor(float now) {
    for (const auto& it : monitors_) {
        const ExplosionMonitor& monitor = it.second;
        if (monitor.go->getHealthComponent().isJustDead()) {
            addExplosion(monitor.model, monitor.go, now, monitor.duration);
            monitor.go->disableRendering();
        }
        monitor.go->getHealthComponent().update();
    }
}

